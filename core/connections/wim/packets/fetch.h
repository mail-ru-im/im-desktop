#ifndef __FETCH_H__
#define __FETCH_H__

#pragma once


#include "../wim_packet.h"
#include "../auth_parameters.h"

namespace core
{
    namespace smartreply
    {
        enum class type;
    }

    namespace wim
    {
        class fetch_event;

        enum class relogin
        {
            none,
            relogin_with_error,
            relogin_without_error,
        };

        class fetch : public wim_packet
        {
            std::string fetch_url_;
            std::chrono::milliseconds timeout_;
            std::function<bool(std::chrono::milliseconds)> wait_function_;
            timepoint fetch_time_;
            relogin relogin_;

            virtual int32_t init_request(std::shared_ptr<core::http_request_simple> request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            virtual int32_t on_response_error_code() override;
            virtual int32_t execute_request(std::shared_ptr<core::http_request_simple> request) override;

            void on_session_ended(const rapidjson::Value& _data);

            std::list< std::shared_ptr<core::wim::fetch_event> > events_;

            std::string next_fetch_url_;
            timepoint next_fetch_time_;
            time_t ts_;
            time_t time_offset_;
            time_t time_offset_local_;
            time_t execute_time_;
            time_t now_;
            double request_time_;
            long long timezone_offset_;
            const bool hidden_;
            int32_t events_count_;
            std::string my_aimid_;
            std::chrono::seconds next_fetch_timeout_;
            const std::vector<smartreply::type> suggest_types_;

        public:

            const std::string& get_next_fetch_url() const { return next_fetch_url_; }
            timepoint get_next_fetch_time() const noexcept { return next_fetch_time_; }
            std::chrono::seconds get_next_fetch_timeout() const noexcept { return next_fetch_timeout_; }
            time_t get_ts() const noexcept { return ts_; }
            time_t get_time_offset() const noexcept { return time_offset_; }
            time_t get_time_offset_local() const noexcept { return time_offset_local_; }
            time_t now() const noexcept { return now_; }
            time_t get_start_execute_time() const noexcept { return execute_time_; }
            std::chrono::milliseconds timeout() const noexcept { return timeout_; }
            double get_request_time() const noexcept { return request_time_; }
            long long get_timezone_offset() const noexcept { return timezone_offset_; }
            relogin need_relogin() const;
            int32_t get_events_count() const;


            std::shared_ptr<core::wim::fetch_event> push_event(std::shared_ptr<core::wim::fetch_event> _event);
            std::shared_ptr<core::wim::fetch_event> pop_event();

            fetch(
                wim_packet_params params,
                const fetch_parameters& _fetch_params,
                std::chrono::milliseconds _timeout,
                const bool _hidden,
                std::function<bool(std::chrono::milliseconds)> _wait_function);
            virtual ~fetch();
        };

    }
}


#endif//__FETCH_H__
