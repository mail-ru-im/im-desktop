#pragma once

#include "../../async_task.h"
#include "queue/packet_queue.h"

namespace core::wim
{
    class wim_packet;

    class wim_send_thread : public async_executer, public std::enable_shared_from_this<wim_send_thread>
    {
        struct task_and_params
        {
            std::shared_ptr<wim_packet> packet_;
            std::function<void(int32_t)> error_handler_;

            explicit task_and_params() = default;
            explicit task_and_params(
                std::shared_ptr<wim_packet> _packet,
                std::function<void(int32_t)> _error_handler)
                : packet_(std::move(_packet))
                , error_handler_(std::move(_error_handler))
            {
            }
        };

        struct packet_key
        {
            std::string_view key(const task_and_params& _task) const;
            priority_t priority(const task_and_params& _task) const;
        };

        bool is_sending_locked_;

        using wim_packets_queue = packet_queue<task_and_params, packet_key>;
        wim_packets_queue high_priority_queue_;
        wim_packets_queue low_priority_queue_;

        std::chrono::steady_clock::time_point cancel_packets_time_;

        void execute_packet_from_queue();

        void insert_packet_in_queue(task_and_params _task);

    public:
        wim_send_thread();

        void post_packet(std::shared_ptr<wim_packet> _packet, std::function<void(int32_t)>&& _error_handler);
        void clear();

        void lock();
        void unlock_and_execute_queued();

        void set_max_parallel_packets_count(size_t _count);

        constexpr static std::chrono::seconds rate_limit_timeout() { return std::chrono::seconds(30); }
    };

} // namespace core::wim
