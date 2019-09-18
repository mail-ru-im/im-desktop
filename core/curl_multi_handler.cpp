#include "stdafx.h"
#include "curl_handler.h"
#include "curl_multi_handler.h"
#include "curl_context.h"
#include "utils.h"
#include <curl.h>
#include <iostream>

namespace
{
    boost::asio::io_service io_service;
    boost::asio::deadline_timer timer(io_service);
    std::map<curl_socket_t, boost::asio::ip::tcp::socket *> socket_map;
    std::set<int*> socket_actions;

    struct GlobalInfo
    {
        CURLM *multi = nullptr;
        int still_running = 0;
    };

    GlobalInfo global;

    std::map<CURL*, std::shared_ptr<core::curl_task>> tasks;
    std::mutex tasks_mutex;

    std::deque<std::shared_ptr<core::curl_task>> tasks_queue;
    std::mutex tasks_queue_mutex;

    std::condition_variable condition;

    std::atomic_bool stop = false;

    std::mutex timer_mutex;

    size_t max_easy_count = 10;

    static void timer_cb(const boost::system::error_code & error, GlobalInfo *g);

    static int multi_timer_cb(CURLM *multi, long timeout_ms, GlobalInfo *g)
    {
        timer.cancel();
        if (timeout_ms > 0)
        {
            boost::system::error_code error;
            timer.expires_from_now(boost::posix_time::millisec(timeout_ms), error);
            timer.async_wait(boost::bind(&timer_cb, _1, g));
        }
        else if (timeout_ms == 0)
        {
            boost::system::error_code error;
            std::unique_lock<std::mutex> lock(timer_mutex, std::try_to_lock);
            if (!lock.owns_lock())
            {
                io_service.post(boost::bind(&timer_cb, error, g));
                return 0;
            }
            timer_cb(error, g);
        }

        return 0;
    }

    static void check_multi_info(GlobalInfo *g)
    {
        CURLMsg *msg;
        int msgs_left;
        CURL *easy;
        CURLcode res;

        while (msg = curl_multi_info_read(g->multi, &msgs_left))
        {
            if (msg->msg == CURLMSG_DONE)
            {
                easy = msg->easy_handle;
                res = msg->data.result;
                if (easy)
                {
                    std::shared_ptr<core::curl_task> shared_task;
                    if (auto task = tasks.find(easy); task != tasks.end())
                    {
                        shared_task = std::move(task->second);
                        tasks.erase(task);
                    }

                    if (shared_task)
                        shared_task->finish_multi(&global, easy, res);
                }
            }
        }
    }

    static void event_cb(GlobalInfo *g, curl_socket_t s, int action, const boost::system::error_code & error, int *fdp)
    {
        if (socket_map.find(s) == socket_map.end() || socket_actions.find(fdp) == socket_actions.end())
            return;

        if (*fdp == action || *fdp == CURL_POLL_INOUT || error)
        {
            if (error)
                action = CURL_CSELECT_ERR;

            const auto rc = curl_multi_socket_action(g->multi, s, action, &g->still_running);

            check_multi_info(g);

            if (g->still_running <= 0)
            {
                boost::system::error_code err;
                timer.cancel(err);
            }

            if (error)
                return;

            if (auto iter = socket_map.find(s); iter != socket_map.end())
            {
                boost::asio::ip::tcp::socket *tcp_socket = iter->second;
                try
                {
                    if (action == CURL_POLL_IN)
                        tcp_socket->async_read_some(boost::asio::null_buffers(), boost::bind(&event_cb, g, s, action, _1, fdp));
                    if (action == CURL_POLL_OUT)
                        tcp_socket->async_write_some(boost::asio::null_buffers(), boost::bind(&event_cb, g, s, action, _1, fdp));
                }
                catch (const boost::system::system_error& _error)
                {
                    io_service.post(boost::bind(&event_cb, g, s, action, _error.code(), fdp));
                }
                catch (const std::exception&)//just in case
                {
                    auto err = boost::system::error_code(boost::system::errc::io_error, boost::system::detail::generic_error_category());
                    io_service.post(boost::bind(&event_cb, g, s, action, err, fdp));
                }
            }
        }
    }

    static void timer_cb(const boost::system::error_code & error, GlobalInfo *g)
    {
        if (!error && !stop.load())
        {
            const auto rc = curl_multi_socket_action(g->multi, CURL_SOCKET_TIMEOUT, 0, &g->still_running);
            check_multi_info(g);
        }
    }

    static void remsock(int *f, GlobalInfo *g)
    {
        socket_actions.erase(f);
        if (f)
            free(f);
    }

    static void setsock(int *fdp, curl_socket_t s, CURL *e, int act, int oldact, GlobalInfo *g)
    {
        std::map<curl_socket_t, boost::asio::ip::tcp::socket *>::iterator it = socket_map.find(s);

        if (it == socket_map.end())
            return;

        boost::asio::ip::tcp::socket * tcp_socket = it->second;

        *fdp = act;

        try
        {
            if (act == CURL_POLL_IN)
            {
                if (oldact != CURL_POLL_IN && oldact != CURL_POLL_INOUT)
                    tcp_socket->async_read_some(boost::asio::null_buffers(), boost::bind(&event_cb, g, s, CURL_POLL_IN, _1, fdp));
            }
            else if (act == CURL_POLL_OUT)
            {
                if (oldact != CURL_POLL_OUT && oldact != CURL_POLL_INOUT)
                    tcp_socket->async_write_some(boost::asio::null_buffers(), boost::bind(&event_cb, g, s, CURL_POLL_OUT, _1, fdp));
            }
            else if (act == CURL_POLL_INOUT)
            {
                if (oldact != CURL_POLL_IN && oldact != CURL_POLL_INOUT)
                    tcp_socket->async_read_some(boost::asio::null_buffers(), boost::bind(&event_cb, g, s, CURL_POLL_IN, _1, fdp));
                if (oldact != CURL_POLL_OUT && oldact != CURL_POLL_INOUT)
                    tcp_socket->async_write_some(boost::asio::null_buffers(), boost::bind(&event_cb, g, s, CURL_POLL_OUT, _1, fdp));
            }
        }
        catch (const boost::system::system_error& _error)
        {
            io_service.post(boost::bind(&event_cb, g, s, act, _error.code(), fdp));
        }
        catch (const std::exception&) //just in case
        {
            auto err = boost::system::error_code(boost::system::errc::io_error, boost::system::detail::generic_error_category());
            io_service.post(boost::bind(&event_cb, g, s, act, err, fdp));
        }
    }

    static void addsock(curl_socket_t s, CURL *easy, int action, GlobalInfo *g)
    {
        int *fdp = (int *)calloc(sizeof(int), 1);
        socket_actions.insert(fdp);

        setsock(fdp, s, easy, action, 0, g);
        curl_multi_assign(g->multi, s, fdp);
    }

    static int sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp)
    {
        GlobalInfo *g = (GlobalInfo*)cbp;
        int *actionp = (int *)sockp;

        if (what == CURL_POLL_REMOVE)
        {
            remsock(actionp, g);
        }
        else
        {
            if (!actionp)
                addsock(s, e, what, g);
            else
                setsock(actionp, s, e, what, *actionp, g);
        }

        return 0;
    }

    static curl_socket_t opensocket(void *clientp, curlsocktype purpose, struct curl_sockaddr *address)
    {
        curl_socket_t sockfd = CURL_SOCKET_BAD;

        if (purpose == CURLSOCKTYPE_IPCXN && address->family == AF_INET) {
            boost::asio::ip::tcp::socket *tcp_socket = new boost::asio::ip::tcp::socket(io_service);

            boost::system::error_code ec;
            tcp_socket->open(boost::asio::ip::tcp::v4(), ec);

            if (!ec)
            {
                sockfd = tcp_socket->native_handle();
                socket_map.insert(std::pair<curl_socket_t, boost::asio::ip::tcp::socket *>(sockfd, tcp_socket));
            }
        }

        return sockfd;
    }

    static int close_socket(void *clientp, curl_socket_t item)
    {
        std::map<curl_socket_t, boost::asio::ip::tcp::socket *>::iterator it = socket_map.find(item);

        if (it != socket_map.end())
        {
            delete it->second;
            socket_map.erase(it);
        }

        return 0;
    }

    void cancel_tasks()
    {
        {
            std::lock_guard<std::mutex> guard(tasks_queue_mutex);
            while (!tasks_queue.empty())
            {
                tasks_queue.front()->cancel();
                tasks_queue.pop_front();
            }
            condition.notify_one();
        }

        for (auto it = tasks.begin(); it != tasks.end();)
        {
            auto easy = it->first;
            auto task = it->second;
            if (task)
                task->finish_multi(&global, easy, CURLE_ABORTED_BY_CALLBACK);

            it = tasks.erase(it);
        }

        for (auto it = socket_map.begin(); it != socket_map.end();)
        {
            delete it->second;
            it = socket_map.erase(it);
        }
    }

    void add_task()
    {
        {
            std::lock_guard<std::mutex> guard(tasks_queue_mutex);
            while (!tasks_queue.empty() && (tasks.size() < max_easy_count || tasks_queue.front()->get_priority() <= core::priority_protocol()))
            {
                auto task = tasks_queue.front();
                tasks_queue.pop_front();

                auto easy = task->init_handler();
                if (easy)
                {
                    curl_easy_setopt(easy, CURLOPT_OPENSOCKETFUNCTION, opensocket);
                    curl_easy_setopt(easy, CURLOPT_CLOSESOCKETFUNCTION, close_socket);

                    tasks[easy] = task;
                    task->execute_multi(global.multi, easy);
                }
            }
        }

        curl_multi_perform(global.multi, &global.still_running);
        check_multi_info(&global);
    }
}

namespace core
{
    curl_easy::future_t curl_multi_handler::perform(std::shared_ptr<curl_context> _context)
    {
        auto promise = curl_easy::promise_t();
        auto future = promise.get_future();
        if (is_stopped())
        {
            promise.set_value(CURLE_ABORTED_BY_CALLBACK);
            return future;
        }

        add_task_to_queue(std::make_shared<curl_task>(_context, promise));

        return future;
    }

    void curl_multi_handler::perform_async(std::shared_ptr<curl_context> _context, curl_easy::completion_function _completion_func)
    {
        if (is_stopped())
        {
            _completion_func(core::curl_task::get_completion_code(CURLE_ABORTED_BY_CALLBACK));
            return;
        }

        add_task_to_queue(std::make_shared<curl_task>(_context, _completion_func));
    }

    void curl_multi_handler::raise_task(int64_t _id)
    {
        std::lock_guard<std::mutex> guard(tasks_queue_mutex);
        auto task = std::find_if(tasks_queue.begin(), tasks_queue.end(), [_id](const auto& it) { return it->get_id() == _id; });
        if (task == tasks_queue.end())
            return;

        auto task_ptr = (*task);
        tasks_queue.erase(task);

        auto inserted = false;
        auto iter = tasks_queue.begin();
        while (iter != tasks_queue.end())
        {
            if (task_ptr->get_priority() < (*iter)->get_priority())
            {
                tasks_queue.insert(iter, task_ptr);
                inserted = true;
                break;
            }
            ++iter;
        }

        if (!inserted)
            tasks_queue.push_back(task_ptr);
    }

    void curl_multi_handler::init()
    {
        service_thread_ = std::thread([]()
        {
            utils::set_this_thread_name("curl multi thread");

            global.multi = curl_multi_init();

            curl_multi_setopt(global.multi, CURLMOPT_SOCKETFUNCTION, sock_cb);
            curl_multi_setopt(global.multi, CURLMOPT_SOCKETDATA, &global);
            curl_multi_setopt(global.multi, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
            curl_multi_setopt(global.multi, CURLMOPT_TIMERDATA, &global);

            while (1)
            {
                boost::system::error_code error;
                io_service.run(error);
                io_service.reset();

                if (stop.load())
                    break;

                {
                    std::lock_guard<std::mutex> guard(tasks_queue_mutex);
                    if (!tasks_queue.empty())
                    {
                        io_service.post(boost::bind(&add_task));
                        condition.notify_one();
                    }
                }

                std::unique_lock<std::mutex> lock(tasks_queue_mutex);
                condition.wait(lock);
            }

            curl_multi_cleanup(global.multi);
        });
    }

    void curl_multi_handler::cleanup()
    {
        stop = true;
        io_service.post(boost::bind(&cancel_tasks));
        {
            std::lock_guard<std::mutex> guard(tasks_queue_mutex);
            condition.notify_one();
        }
        service_thread_.join();
        io_service.stop();
    }

    void curl_multi_handler::reset()
    {
        io_service.post([]()
        {
            cancel_tasks();
            curl_multi_cleanup(global.multi);

            global.multi = curl_multi_init();
            curl_multi_setopt(global.multi, CURLMOPT_SOCKETFUNCTION, sock_cb);
            curl_multi_setopt(global.multi, CURLMOPT_SOCKETDATA, &global);
            curl_multi_setopt(global.multi, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
            curl_multi_setopt(global.multi, CURLMOPT_TIMERDATA, &global);
        });

        std::lock_guard<std::mutex> guard(tasks_queue_mutex);
        condition.notify_one();
    }

    void curl_multi_handler::process_stopped_tasks()
    {
        io_service.post([]()
        {
            for (auto it = tasks.begin(); it != tasks.end();)
            {
                auto easy = it->first;
                auto task = it->second;
                if ((task && task->is_stopped()) || !task)
                {
                    if (task)
                        task->finish_multi(&global, easy, CURLE_ABORTED_BY_CALLBACK);

                    it = tasks.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        });

        std::lock_guard<std::mutex> guard(tasks_queue_mutex);
        condition.notify_one();
    }


    bool curl_multi_handler::is_stopped() const
    {
        return stop.load();
    }

    void curl_multi_handler::add_task_to_queue(std::shared_ptr<core::curl_task> _task)
    {
        std::lock_guard<std::mutex> guard(tasks_queue_mutex);

        auto inserted = false;
        auto iter = tasks_queue.begin();
        while (iter != tasks_queue.end())
        {
            if (_task->get_priority() < (*iter)->get_priority())
            {
                tasks_queue.insert(iter, _task);
                inserted = true;
                break;
            }
            ++iter;
        }
        if (!inserted)
            tasks_queue.push_back(_task);

        io_service.post(boost::bind(&add_task));
        condition.notify_one();
    }
}
