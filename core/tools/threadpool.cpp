#include "stdafx.h"
#include "threadpool.h"

#include "../utils.h"

#include "../core.h"
#include "../network_log.h"

#ifndef STRIP_CRASH_HANDLER
#include "../common.shared/crash_report/crash_reporter.h"
#endif // !STRIP_CRASH_HANDLER

using namespace core;
using namespace tools;

task::task()
    : id_(-1)
    , time_stamp_(std::chrono::steady_clock::now())
{
}

task::task(stacked_task _action, int64_t _id, std::string_view _name, std::chrono::steady_clock::time_point _time_stamp, std::function<bool()> _cancel)
    : action_(std::move(_action))
    , cancel_(std::move(_cancel))
    , id_(_id)
    , name_(_name)
    , time_stamp_(_time_stamp)
{
}

void task::execute()
{
    if (!cancel_ || !cancel_())
        action_.execute();
}

int64_t task::get_id() const noexcept
{
    return id_;
}

std::string_view core::tools::task::get_name() const noexcept
{
    return name_;
}

std::chrono::steady_clock::time_point core::tools::task::get_time_stamp() const noexcept
{
    return time_stamp_;
}

task::operator bool() const noexcept
{
    return action_.operator bool();
}

stack_vec task::get_stack_trace() const
{
    return action_.get_stack();
}

threadpool::threadpool(
    const std::string_view _name,
    const size_t count,
    std::function<void()> _on_thread_exit,
    bool _task_trace)

    : on_task_finish_([](std::chrono::milliseconds, const stack_vec&, std::string_view) {})
    , stop_(false)
    , task_trace_(_task_trace)
{
    creator_thread_id_ = std::this_thread::get_id();

    threads_.reserve(count);
    threads_ids_.reserve(count);

    const auto worker = [this, _on_thread_exit, name = std::string(_name)]
    {
        utils::set_this_thread_name(name);
#ifndef STRIP_CRASH_HANDLER
#ifdef _WIN32
    crash_system::reporter::instance().set_thread_exception_handlers();
#endif
#endif // !STRIP_CRASH_HANDLER

        for(;;)
        {
            if (!run_task())
                break;
        }

        if (_on_thread_exit)
            _on_thread_exit();
    };

    for (size_t i = 0; i < count; ++i)
    {
        threads_.emplace_back(worker);
        threads_ids_.emplace_back(threads_[i].get_id());
    }
}

bool threadpool::run_task_impl()
{
    task next_task;
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        while (!(stop_ || !tasks_.empty()))
        {
           condition_.wait(lock);
        }

        if (stop_ && tasks_.empty())
        {
            return false;
        }

        next_task = std::move(tasks_.front());

        tasks_.pop_front();
    }
    if (next_task)
    {
        const auto start_time = std::chrono::steady_clock::now();

        if (task_trace_)
        {
            std::stringstream ss;
            ss << "async_task: run <" << next_task.get_name()
                << "> (waited " << std::chrono::duration_cast<std::chrono::milliseconds>(start_time - next_task.get_time_stamp()).count()
                << " ms)\r\n";
            g_core->write_string_to_network_log(ss.str());
        }

        next_task.execute();

        const auto finish_time = std::chrono::steady_clock::now();

        on_task_finish_(
            std::chrono::duration_cast<std::chrono::milliseconds>(finish_time - start_time),
            next_task.get_stack_trace(), next_task.get_name());

        if (task_trace_)
        {
            std::stringstream ss;
            ss << "async_task: <" << next_task.get_name()
                << "> completed in " << std::chrono::duration_cast<std::chrono::milliseconds>(finish_time - start_time).count()
                << " ms\r\n";
            g_core->write_string_to_network_log(ss.str());
        }
    }
    else
    {
        assert(!"threadpool: task is empty");
    }
    return true;
}

bool threadpool::run_task()
{
    if constexpr (!core::dump::is_crash_handle_enabled() || !platform::is_windows())
        return run_task_impl();
#ifndef STRIP_CRASH_HANDLER
#ifdef _WIN32
    __try
    {
        return run_task_impl();
    }
    __except (crash_system::reporter::seh_handler(GetExceptionInformation()))
    {
    }
#endif // _WIN32
    return true;
#else
    return run_task_impl();
#endif // !STRIP_CRASH_HANDLER
}

threadpool::~threadpool()
{
    if (creator_thread_id_ != std::this_thread::get_id())
    {
        assert(!"invalid destroy thread");
    }

    stop_ = true;

    condition_.notify_all();

    for (auto &worker: threads_)
    {
        worker.join();
    }

}

bool threadpool::push_back(stacked_task _task, int64_t _id, std::string_view _name, std::function<bool()> _cancel)
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_)
            return false;

        tasks_.emplace_back(std::move(_task), _id, std::move(_name), std::chrono::steady_clock::now(), std::move(_cancel));
    }

    condition_.notify_one();

    return true;
}

void threadpool::raise_task(int64_t _id)
{
    if (_id == -1)
        return;

    std::unique_lock<std::mutex> lock(queue_mutex_);
    if (tasks_.size() <= 1)
        return;

    task tmp;
    for (auto iter = tasks_.begin(); iter != tasks_.end(); ++iter)
    {
        if (iter->get_id() == _id)
        {
            if (iter == tasks_.begin())
                return;

            tmp = std::move(*iter);
            tasks_.erase(iter);
            break;
        }
    }

    if (tmp)
        tasks_.push_front(std::move(tmp));
}

bool threadpool::push_front(stacked_task _task, int64_t _id, std::string_view _name, std::function<bool()> _cancel)
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_)
            return false;

        tasks_.emplace_front(std::move(_task), _id, std::move(_name), std::chrono::steady_clock::now(), std::move(_cancel));
    }

    condition_.notify_one();

    return true;
}

const std::vector<std::thread::id>& threadpool::get_threads_ids() const
{
    return threads_ids_;
}

void threadpool::set_task_finish_callback(finish_action _on_task_finish)
{
    on_task_finish_ = std::move(_on_task_finish);
}
