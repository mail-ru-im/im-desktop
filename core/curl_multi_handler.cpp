#include "stdafx.h"
#include "curl_handler.h"
#include "curl_multi_handler.h"
#include "curl_context.h"
#include "utils.h"
#include "core.h"
#include <curl.h>
#include <iostream>

namespace
{
    boost::asio::io_service io_service;
    boost::asio::deadline_timer timer(io_service);

    struct SocketData
    {
        boost::asio::ip::tcp::socket* socket_ = nullptr;
        int timeout_cnt_ = 0;
        int bad_write_cnt_ = 0;
    };
    std::map<curl_socket_t, SocketData> socket_map;

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

    constexpr size_t max_easy_count = 10;
    constexpr int max_timeouts = 3;
    constexpr int max_bad_writes = 1000;

    static void timer_cb(const boost::system::error_code & error, GlobalInfo *g);
    static int close_socket(void* clientp, curl_socket_t item);

    void write_log(std::string_view _text)
    {
        using namespace core;
        if (g_core)
            g_core->write_string_to_network_log(_text);
    }

    void match_task_socket(curl_socket_t s, CURL *e)
    {
        if (auto task = tasks.find(e); task != tasks.end())
        {
            assert(s != CURL_SOCKET_BAD);
            if (task->second)
                task->second->set_socket(s);
        }
    }

    void on_socket_ok(const curl_socket_t s)
    {
        assert(s != CURL_SOCKET_BAD);
        if (s == CURL_SOCKET_BAD)
            return;

        auto it = socket_map.find(s);
        if (it == socket_map.end())
            return;

        it->second.timeout_cnt_ = 0;
    }

    void on_socket_timed_out(const curl_socket_t s)
    {
        assert(s != CURL_SOCKET_BAD);
        if (s == CURL_SOCKET_BAD)
            return;

        auto it = socket_map.find(s);
        if (it == socket_map.end())
            return;

        const auto cur = ++(it->second.timeout_cnt_);
        if (cur >= max_timeouts)
        {
            write_log(std::string("socket ") + std::to_string(int(s)) + std::string(" timed out >= ") + std::to_string(max_timeouts) + std::string(" times, killing socket and tasks"));

            for (auto it = tasks.begin(); it != tasks.end();)
            {
                auto easy = it->first;
                auto task = it->second;
                if (task)
                {
                    if (task->get_socket() == s)
                    {
                        task->finish_multi(&global, easy, CURLE_ABORTED_BY_CALLBACK);
                        it = tasks.erase(it);
                        continue;
                    }
                }
                it++;
            }

            close_socket(nullptr, s);
        }
    }

    void check_socket_bad_write(const curl_socket_t s, int& action)
    {
        assert(s != CURL_SOCKET_BAD);
        if (auto it = socket_map.find(s); it != socket_map.end())
        {
            auto& data = it->second;
            if (action == CURL_POLL_OUT)
            {
                if (data.socket_->available() == 0)
                    data.bad_write_cnt_++;
                else
                    data.bad_write_cnt_ = 0;
            }

            if (data.bad_write_cnt_ >= max_bad_writes)
            {
                write_log(std::string("forcing CURL_CSELECT_ERR on socket ") + std::to_string(int(s)));

                action = CURL_CSELECT_ERR;
                data.bad_write_cnt_ = 0;
            }
        }
    }

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
        CURLMsg* msg = nullptr;
        int msgs_left = 0;
        CURL* easy = nullptr;
        CURLcode res = CURLE_OK;

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
                    {
                        const auto s = shared_task->get_socket();

                        shared_task->finish_multi(&global, easy, res);

                        if (s != CURL_SOCKET_BAD)
                        {
                            if (res == CURLE_OPERATION_TIMEDOUT)
                                on_socket_timed_out(s);
                            else if (res == CURLE_OK)
                                on_socket_ok(s);
                        }
                    }
                }
            }
        }
    }

    static void event_cb(GlobalInfo* g, curl_socket_t s, int action, const boost::system::error_code& error, int* fdp)
    {
        if (socket_map.find(s) == socket_map.end() || socket_actions.find(fdp) == socket_actions.end())
            return;

        if (*fdp == action || *fdp == CURL_POLL_INOUT || error)
        {
            if constexpr (platform::is_apple())
                check_socket_bad_write(s, action);

            if (error)
                action = CURL_CSELECT_ERR;

            const auto rc = curl_multi_socket_action(g->multi, s, action, &g->still_running);
            assert(rc == CURLM_OK);

            check_multi_info(g);

            if (g->still_running <= 0)
            {
                boost::system::error_code err;
                timer.cancel(err);
            }

            if (error)
                return;

            if (auto it = socket_map.find(s); it != socket_map.end())
            {
                boost::asio::ip::tcp::socket* tcp_socket = it->second.socket_;
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
        auto it = socket_map.find(s);
        if (it == socket_map.end())
            return;

        boost::asio::ip::tcp::socket* tcp_socket = it->second.socket_;

        *fdp = act;

        match_task_socket(s, e);

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
            match_task_socket(s, e);

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

        if (purpose == CURLSOCKTYPE_IPCXN && (address->family == AF_INET || address->family == AF_INET6))
        {
            auto tcp_socket = new boost::asio::ip::tcp::socket(io_service);

            boost::system::error_code ec;
            tcp_socket->open(address->family == AF_INET ? boost::asio::ip::tcp::v4() : boost::asio::ip::tcp::v6(), ec);

            if (!ec)
            {
                sockfd = tcp_socket->native_handle();
                socket_map.insert({ sockfd, { tcp_socket } });
            }
        }

        return sockfd;
    }

    static int close_socket(void* clientp, curl_socket_t item)
    {
        if (auto it = socket_map.find(item); it != socket_map.end())
        {
            delete it->second.socket_;
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
            delete it->second.socket_;
            it = socket_map.erase(it);
        }

        write_log("multi_handle: cancel_tasks");
    }

    void add_task()
    {
        {
            std::lock_guard<std::mutex> guard(tasks_queue_mutex);
            while (!tasks_queue.empty() && (tasks.size() < max_easy_count || tasks_queue.front()->get_priority() <= core::priority_protocol()))
            {
                auto task = std::move(tasks_queue.front());
                tasks_queue.pop_front();

                if (const auto easy = task->init_handler(); easy.handler)
                {
                    curl_easy_setopt(easy.handler, CURLOPT_OPENSOCKETFUNCTION, opensocket);
                    curl_easy_setopt(easy.handler, CURLOPT_CLOSESOCKETFUNCTION, close_socket);

                    if (socket_map.size() == 1)
                        task->set_socket(socket_map.begin()->first);

                    tasks[easy.handler] = task;
                    if (const auto code = task->execute_multi(global.multi, easy.handler); code != CURLE_OK)
                        write_log(std::string("add_task: execute_multi fail: ") + curl_easy_strerror(code));
                }
                else
                {
                    write_log(std::string("add_task: init_handler fail: ") + curl_easy_strerror(easy.code));
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

        add_task_to_queue(std::make_shared<curl_task>(_context, std::move(promise)));

        return future;
    }

    void curl_multi_handler::perform_async(std::shared_ptr<curl_context> _context, curl_easy::completion_function _completion_func)
    {
        if (is_stopped())
        {
            _completion_func(core::curl_task::get_completion_code(CURLE_ABORTED_BY_CALLBACK));
            return;
        }

        add_task_to_queue(std::make_shared<curl_task>(_context, std::move(_completion_func)));
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
            curl_multi_setopt(global.multi, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);

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
