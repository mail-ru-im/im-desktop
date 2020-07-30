#include "stdafx.h"
#include "curl_handler.h"
#include "curl_multi_handler.h"
#include "curl_context.h"
#include "utils.h"
#include "core.h"
#include "../common.shared/string_utils.h"
#include <curl.h>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#endif //_WIN32

namespace
{
    struct GlobalInfo
    {
        CURLM* multi = nullptr;
        int still_running = 0;
    };

    GlobalInfo global;

#ifndef _WIN32
    int setNonblocking(int fd)
    {
        int flags;
#if defined(O_NONBLOCK)
        if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
            flags = 0;
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
        flags = 1;
        return ioctl(fd, FIOBIO, &flags);
#endif
    }
#endif //_WIN32

#ifdef _WIN32
    HANDLE events[WSA_MAXIMUM_WAIT_EVENTS];
    constexpr auto ping_signal_index = 0;
#else
    curl_socket_t sockets[2];
#endif //_WIN32

    bool signal_inited = false;

    void init_signals()
    {
        if (signal_inited)
            return;

#ifdef _WIN32
        WSADATA wsaData = { 0 };
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        for (auto i = 0; i < WSA_MAXIMUM_WAIT_EVENTS; ++i)
        {
            events[i] = WSACreateEvent();
            WSAResetEvent(events[i]);
        }
#else
        socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
        setNonblocking(sockets[0]);
        setNonblocking(sockets[1]);
#endif //_WIN32

        signal_inited = true;
    }

    void deinit_signals()
    {
        if (!signal_inited)
            return;

#ifdef _WIN32
        for (auto i = 0; i < WSA_MAXIMUM_WAIT_EVENTS; ++i)
            WSACloseEvent(events[i]);

        WSACleanup();
#else
        close(sockets[0]);
        close(sockets[1]);
#endif //_WIN32
        signal_inited = false;
    }

    void emit_signal()
    {
        if (!signal_inited)
        {
            init_signals();
            return;
        }

#ifdef _WIN32
        WSASetEvent(events[ping_signal_index]);
#else
        constexpr char buffer[] = "ping";
        write(sockets[0], buffer, std::size(buffer));
#endif //_WIN32
    }

    void clear_signal()
    {
        if (!signal_inited)
            return;

#ifdef _WIN32
        WSAResetEvent(events[ping_signal_index]);
#else
        char buf[1024];
        int ret = 0;
        while((ret = read(sockets[1], buf, std::size(buf) - 1)) > 0);
#endif //_win32
    }

#ifndef _WIN32
    curl_socket_t get_signal()
    {
        if (!signal_inited)
            init_signals();

        return sockets[1];
    }
#endif //_WIN32

    int perform_sleep(std::chrono::milliseconds msec)
    {
        std::this_thread::sleep_for(msec);
        return 0;
    }

    void check_socket_bad_write(const curl_socket_t s);

    int perform_select(int maxfd, fd_set fdread, fd_set fdwrite, fd_set fdexcep, struct timeval timeout)
    {
        int rc = 0;
#ifdef _WIN32
        init_signals();
        auto i = ping_signal_index + 1;
        if (fdread.fd_count > 0)
        {
            for (unsigned j = 0; j < fdread.fd_count; ++j)
            {
                WSAEventSelect(fdread.fd_array[j], events[i], FD_READ);
                ++i;
            }
        }
        if (fdwrite.fd_count > 0)
        {
            for (unsigned j = 0; j < fdwrite.fd_count; ++j)
            {
                WSAEventSelect(fdwrite.fd_array[j], events[i], FD_WRITE);
                ++i;
            }
        }

        auto timemout_win = timeout.tv_sec == 0 ? timeout.tv_usec / 1000 : timeout.tv_sec * 1000;
        auto r = WSAWaitForMultipleEvents(i, events, FALSE, timemout_win, TRUE);
        if (r == WSA_WAIT_EVENT_0)
        {
            clear_signal();
        }
        else if (r != WSA_WAIT_TIMEOUT)
        {
            WSAResetEvent(events[r - WSA_WAIT_EVENT_0 - ping_signal_index]);
            rc = 1;
        }
#else
        FD_SET(get_signal(), &fdread);
        rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
        if (rc)
        {
            if (FD_ISSET(get_signal(), &fdread))
                clear_signal();

#ifdef __APPLE__
            for (auto i = 0; i < maxfd; ++i)
                if (FD_ISSET(i, &fdwrite))
                    check_socket_bad_write(i);
#endif //__APPLE__
        }
#endif //_WIN32

        return rc;
    }

    struct SocketData
    {
        int timeout_cnt_ = 0;
        int bad_write_cnt_ = 0;
    };
    std::map<curl_socket_t, SocketData> socket_map;

    std::map<CURL*, std::shared_ptr<core::curl_task>> tasks;

    std::deque<std::shared_ptr<core::curl_task>> tasks_queue;
    std::mutex tasks_queue_mutex;

    std::condition_variable condition;

    std::atomic_bool stop = false;
    std::atomic_bool tasks_process = false;
    std::atomic_bool tasks_cancel = false;
    std::atomic_bool reset_multi = false;

    constexpr size_t max_easy_count = 10;
    constexpr int max_timeouts = 3;
    constexpr int max_bad_writes = 1000;

    static int close_socket(void* clientp, curl_socket_t item);

    void write_log(std::string_view _text)
    {
        using namespace core;
        if (g_core)
            g_core->write_string_to_network_log(_text);
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

            for (auto iter = tasks.begin(); iter != tasks.end();)
            {
                auto easy = iter->first;
                auto task = iter->second;
                if (task)
                {
                    curl_socket_t sockfd;
                    curl_easy_getinfo(easy, CURLINFO_ACTIVESOCKET, &sockfd);
                    if (sockfd == s)
                    {
                        task->finish_multi(&global, easy, CURLE_ABORTED_BY_CALLBACK);
                        iter = tasks.erase(iter);
                        continue;
                    }
                }
                ++iter;
            }

            close_socket(nullptr, s);
        }
    }

    void check_socket_bad_write(const curl_socket_t s)
    {
        assert(s != CURL_SOCKET_BAD);
        if (auto it = socket_map.find(s); it != socket_map.end())
        {
            auto& data = it->second;
            unsigned long bytes = 0;
#ifdef _WIN32
            ioctlsocket(s, FIONREAD, &bytes);
#else
            ioctl(s, FIONREAD, &bytes);
#endif //_WIN32
            if (bytes == 0)
                data.bad_write_cnt_++;
            else
                data.bad_write_cnt_ = 0;

            if (data.bad_write_cnt_ >= max_bad_writes)
            {
                write_log(std::string("forcing CURL_CSELECT_ERR on socket ") + std::to_string(int(s)));
                curl_multi_socket_action(global.multi, s, CURL_CSELECT_ERR, &global.still_running);
                data.bad_write_cnt_ = 0;
            }
        }
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
                    if (auto shared_task_node = tasks.extract(easy); !shared_task_node.empty() && shared_task_node.mapped())
                    {
                        curl_socket_t sockfd;
                        curl_easy_getinfo(easy, CURLINFO_ACTIVESOCKET, &sockfd);
                        shared_task_node.mapped()->finish_multi(&global, easy, res);

                        if (sockfd != CURL_SOCKET_BAD)
                        {
                            if (res == CURLE_OPERATION_TIMEDOUT)
                                on_socket_timed_out(sockfd);
                            else if (res == CURLE_OK)
                                on_socket_ok(sockfd);
                        }
                    }
                }
            }
        }
    }

    static curl_socket_t opensocket(void *clientp, curlsocktype purpose, struct curl_sockaddr *address)
    {
        curl_socket_t sockfd = CURL_SOCKET_BAD;
        if (purpose == CURLSOCKTYPE_IPCXN && (address->family == AF_INET || address->family == AF_INET6))
        {
            sockfd = socket(address->family, address->socktype, address->protocol);
            if (sockfd != CURL_SOCKET_BAD)
                socket_map.insert({ sockfd, { } });
        }

        return sockfd;
    }

    void close_socket_internal(curl_socket_t s)
    {
#ifdef _WIN32
        closesocket(s);
#else
        close(s);
#endif //_WIN32
    }

    static int close_socket(void* clientp, curl_socket_t item)
    {
        if (auto it = socket_map.find(item); it != socket_map.end())
        {
            close_socket_internal(it->first);
            socket_map.erase(it);
        }

        return 0;
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
        write_log(su::concat("raising_task: ", task_ptr->normalized_url()));

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
        service_thread_ = std::thread([this]()
        {
            utils::set_this_thread_name("curl multi thread");

            global.multi = curl_multi_init();

            curl_multi_setopt(global.multi, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);

            auto tasks_stuff = [this]()
            {
                if (tasks_process.load())
                {
                    process_stopped_tasks_internal();
                    tasks_process = false;
                }

                if (tasks_cancel.load())
                {
                    cancel_tasks();
                    tasks_cancel = false;
                }

                if (reset_multi.load())
                {
                    curl_multi_cleanup(global.multi);
                    global.multi = curl_multi_init();

                    curl_multi_setopt(global.multi, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
                    reset_multi = false;
                }

                check_multi_info(&global);
                add_task();

                return !stop.load();
            };

            while (1)
            {
                int rc = -1, maxfd = -1;
                CURLMcode mc;
                long curl_timeo = -1;

                fd_set fdread, fdwrite, fdexcep;
                FD_ZERO(&fdread);
                FD_ZERO(&fdwrite);
                FD_ZERO(&fdexcep);

                struct timeval timeout;
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;

                while (global.still_running)
                {
                    if (!tasks_stuff())
                        break;

                    curl_multi_timeout(global.multi, &curl_timeo);
                    if (curl_timeo >= 0)
                    {
                        timeout.tv_sec = curl_timeo / 1000;
                        if (timeout.tv_sec > 1)
                            timeout.tv_sec = 1;
                        else
                            timeout.tv_usec = (curl_timeo % 1000) * 1000;
                    }

                    mc = curl_multi_fdset(global.multi, &fdread, &fdwrite, &fdexcep, &maxfd);

                    if (mc != CURLM_OK)
                        break;

                    if (maxfd == -1)
                    {
                        rc = perform_sleep(std::chrono::milliseconds(100));
                    }
                    else
                    {
                        rc = perform_select(maxfd, fdread, fdwrite, fdexcep, timeout);

                    }

                    if (rc != -1)
                        curl_multi_perform(global.multi, &global.still_running);

                    if (!tasks_stuff())
                        break;
                }

                if (!tasks_stuff())
                    break;

                if (!global.still_running)
                {
                    std::unique_lock<std::mutex> lock(tasks_queue_mutex);
                    condition.wait(lock);
                }
            }
            curl_multi_cleanup(global.multi);
            deinit_signals();
        });
    }

    void curl_multi_handler::cleanup()
    {
        stop = true;
        tasks_cancel = true;

        notify();
        service_thread_.join();
    }

    void curl_multi_handler::reset()
    {
        tasks_cancel = true;
        reset_multi = true;

        notify();
    }

    void curl_multi_handler::process_stopped_tasks()
    {
        tasks_process = true;

        notify();
    }

    bool curl_multi_handler::is_stopped() const
    {
        return stop.load();
    }

    void curl_multi_handler::add_task()
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

                    tasks[easy.handler] = task;
                    if (const auto code = task->execute_multi(global.multi, easy.handler); code != CURLE_OK)
                        write_log(su::concat("add_task: execute_multi fail: ", curl_easy_strerror(code)));
                }
                else
                {
                    write_log(su::concat("add_task: init_handler fail: ", curl_easy_strerror(easy.code)));
                }
            }
        }

        curl_multi_perform(global.multi, &global.still_running);
        check_multi_info(&global);
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

        emit_signal();
        condition.notify_one();
    }

    void curl_multi_handler::cancel_tasks()
    {
        {
            std::lock_guard<std::mutex> guard(tasks_queue_mutex);
            for (auto& task : std::exchange(tasks_queue, {}))
                task->cancel();

            emit_signal();
            condition.notify_one();
        }

        for (auto& [easy, task] : std::exchange(tasks, {}))
            if (task)
                task->finish_multi(&global, easy, CURLE_ABORTED_BY_CALLBACK);

        for (auto [s, _] : std::exchange(socket_map, {}))
            close_socket_internal(s);

        write_log("multi_handle: cancel_tasks");
    }

    void curl_multi_handler::process_stopped_tasks_internal()
    {
        for (auto it = tasks.begin(); it != tasks.end();)
        {
            auto easy = it->first;
            auto task = it->second;
            if (!task || task->is_stopped())
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
    }

    void curl_multi_handler::notify()
    {
        emit_signal();
        std::lock_guard<std::mutex> guard(tasks_queue_mutex);
        condition.notify_one();
    }
}
