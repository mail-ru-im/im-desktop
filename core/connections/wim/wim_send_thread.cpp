#include "stdafx.h"
#include "wim_send_thread.h"

#include "wim_packet.h"

namespace
{
    constexpr auto high_priority = core::top_priority();

    bool is_high_priority(core::priority_t _priority)
    {
        return _priority >= high_priority;
    }

    constexpr uint32_t sent_repeat_count = 2;
    constexpr auto robusto_rate_limit_timeout = std::chrono::seconds(10);
} // namespace


std::string_view core::wim::wim_send_thread::packet_key::key(const task_and_params& _task) const
{
    return _task.packet_->get_method();
}

core::priority_t core::wim::wim_send_thread::packet_key::priority(const task_and_params& _task) const
{
    return _task.packet_->get_priority();
}


core::wim::wim_send_thread::wim_send_thread()
    : async_executer("wim_send")
    , is_sending_locked_(false)
{
}

void core::wim::wim_send_thread::set_max_parallel_packets_count(size_t _count)
{
    const auto low_limit_count = _count / 2;
    low_priority_queue_.set_limit(low_limit_count);
    high_priority_queue_.set_limit(_count - low_limit_count);
}

void core::wim::wim_send_thread::insert_packet_in_queue(task_and_params _task)
{
    auto& queue = is_high_priority(_task.packet_->get_priority()) ? high_priority_queue_ : low_priority_queue_;
    queue.enqueue(std::move(_task));
}

void core::wim::wim_send_thread::post_packet(std::shared_ptr<wim_packet> _packet, std::function<void(int32_t)>&& _error_handler)
{
    if (!_packet->is_valid())
        return;

    if (!_packet->supports_current_api_version())
    {
        g_core->execute_core_context({ [handler = std::move(_error_handler)]
        {
            handler(wpie_require_higher_api_version);
        } });
        return;
    }

    // need wait for timeout (ratelimts)
    if (const auto current_time = std::chrono::steady_clock::now(); current_time < cancel_packets_time_)
    {
        g_core->execute_core_context({ [wr_this = weak_from_this()] ()
        {
            if (auto ptr_this = wr_this.lock())
                ptr_this->execute_packet_from_queue();
        } });

        return;
    }

    insert_packet_in_queue(task_and_params{ std::move(_packet), std::move(_error_handler) });
    execute_packet_from_queue();
}

void core::wim::wim_send_thread::execute_packet_from_queue()
{
    if (is_sending_locked_)
        return;
    const auto has_high_priority_packets = high_priority_queue_.can_pop_element();
    if (!has_high_priority_packets && !low_priority_queue_.can_pop_element())
        return;

    auto& queue = has_high_priority_packets ? high_priority_queue_ : low_priority_queue_;
    auto queue_task = queue.pop();

    auto internal_handlers = run_async_function([packet = queue_task->packet_]()->int32_t
    {
        return packet->execute();
    });

    internal_handlers->on_result_ = [wr_this = weak_from_this(), queue_task = std::move(queue_task), &queue](int32_t _error) mutable
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        core::tools::auto_scope scope([method = queue_task->packet_->get_method(), &ptr_this, &queue]()
        {
            queue.allow_using(method);
            ptr_this->execute_packet_from_queue();
        });

        if (wim_packet::is_network_error(_error) && (queue_task->packet_->get_repeat_count() < sent_repeat_count) && (!queue_task->packet_->is_stopped()))
        {
            ptr_this->insert_packet_in_queue(std::move(*queue_task));
            return;
        }

        if (_error == wpie_error_too_fast_sending)
            ptr_this->cancel_packets_time_ = std::chrono::steady_clock::now() + rate_limit_timeout();
        else if (_error == wpie_error_robusto_too_fast_sending)
            ptr_this->cancel_packets_time_ = std::chrono::steady_clock::now() + robusto_rate_limit_timeout;

        if (const auto& error_handler = queue_task->error_handler_)
            error_handler(_error);
    };

    execute_packet_from_queue();
}

void core::wim::wim_send_thread::clear()
{
    high_priority_queue_.clear();
    low_priority_queue_.clear();
}

void core::wim::wim_send_thread::lock()
{
    is_sending_locked_ = true;
}

void core::wim::wim_send_thread::unlock_and_execute_queued()
{
    is_sending_locked_ = false;
    execute_packet_from_queue();
}
