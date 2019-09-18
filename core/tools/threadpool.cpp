#include "stdafx.h"
#include "threadpool.h"

#include "../utils.h"

#ifdef _WIN32
#include "../common.shared/win32/crash_handler.h"
#endif

using namespace core;
using namespace tools;

task::task()
    : id_(-1)
{
}

task::task(std::function<void()> _action, int64_t _id)
    : action_(std::move(_action))
    , id_(_id)
{
    if (build::is_debug() || platform::is_apple())
    {
        st_ = std::make_unique<boost::stacktrace::stacktrace>();
    }
}

void task::execute()
{
    action_();
}

int64_t task::get_id() const noexcept
{
    return id_;
}

task::operator bool() const noexcept
{
    return action_.operator bool();
}

const std::unique_ptr<boost::stacktrace::stacktrace>& task::get_stack_trace() const
{
    return st_;
}


threadpool::threadpool(
    const std::string_view _name,
    const size_t count,
    std::function<void()> _on_thread_exit)

    : on_task_finish_([](const std::chrono::milliseconds, const core_stacktrace&) {})
    , stop_(false)
{
    creator_thread_id_ = std::this_thread::get_id();

    threads_.reserve(count);
    threads_ids_.reserve(count);

    const auto worker = [this, _on_thread_exit, name = std::string(_name)]
    {
        utils::set_this_thread_name(name);

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
        const auto start_time = std::chrono::system_clock::now();

        next_task.execute();

        const auto finish_time = std::chrono::system_clock::now();

        on_task_finish_(
            std::chrono::duration_cast<std::chrono::milliseconds>(finish_time - start_time),
            next_task.get_stack_trace());
    }
    else
    {
        assert(!"threadpool: task is empty");
    }
    return true;
}

bool threadpool::run_task()
{
    if constexpr (build::is_debug())
        return run_task_impl();

#ifdef _WIN32
    if constexpr (!core::dump::is_crash_handle_enabled())
        return run_task_impl();
#endif // _WIN32

#ifdef _WIN32
     __try
#endif // _WIN32
    {
         return run_task_impl();
    }

#ifdef _WIN32
    __except(::core::dump::crash_handler::seh_handler(GetExceptionInformation()))
    {
    }
#endif // _WIN32
    return true;
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

bool threadpool::push_back(task_action _task, int64_t _id)
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_)
            return false;

#ifdef _WIN32
        tasks_.emplace_back([_task = std::move(_task)]
        {
            core::dump::crash_handler handler("icq.desktop", utils::get_product_data_path(), false);
            handler.set_thread_exception_handlers();
            if (_task)
            {
                _task();
            }
            else
            {
                assert(!"threadpool: _task is empty");
            }
        }, _id);
#else
        tasks_.emplace_back(std::move(_task), _id);
#endif // _WIN32
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

bool threadpool::push_front(task_action _task, int64_t _id)
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_)
            return false;

#ifdef _WIN32
        tasks_.emplace_front([_task = std::move(_task)]
        {
            core::dump::crash_handler handler("icq.desktop", utils::get_product_data_path(), false);
            handler.set_thread_exception_handlers();
            if (_task)
            {
                _task();
            }
            else
            {
                assert(!"threadpool: _task is empty");
            }
        }, _id);
#else
        tasks_.emplace_front(std::move(_task), _id);
#endif // _WIN32
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
