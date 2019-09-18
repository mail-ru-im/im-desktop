#pragma once

#include "proxy_settings.h"
#include "../../connections/urls_cache.h"

namespace core
{
    class async_executer;

    namespace tools
    {
        class binary_stream;
        class tlvpack;
    }

    const static auto save_events_to_file_interval = std::chrono::seconds(10);
    const static auto delay_send_events_on_start = std::chrono::seconds(10);


    namespace stats
    {
        enum class im_stat_event_names;

        class im_stats : public std::enable_shared_from_this<im_stats>
        {
        public:
            im_stats(std::wstring _file_name);
            virtual ~im_stats();

            void init();
            void insert_event(im_stat_event_names _event_name, event_props_type&& _props);

        private:
            class im_stats_event
            {
            public:
                im_stats_event(im_stat_event_names _event_name,
                               event_props_type&& _event_props,
                               std::chrono::system_clock::time_point _event_time);

                im_stats_event(const im_stats_event& _event) = default;
                im_stats_event(im_stats_event&& _event) noexcept = default;
                im_stats_event& operator=(im_stats_event&&) noexcept = default;

                void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;

                im_stat_event_names get_name() const;
                const event_props_type& get_props() const;
                std::chrono::system_clock::time_point get_time() const;

                void mark_event_sent(bool _is_sent);
                bool is_sent() const;

                bool is_equal(const im_stats_event& _event) const;

            private:
                im_stat_event_names name_;
                event_props_type props_;
                std::chrono::system_clock::time_point time_;
                bool event_sent_;
            };

            struct stop_objects
            {
                std::atomic_bool is_stop_;
                stop_objects()
                {
                    is_stop_ = false;
                }
            };

            static std::shared_ptr<stop_objects> stop_objects_;
            static int32_t send(const proxy_settings& _user_proxy,
                                const std::string& _post_data,
                                const std::wstring& _file_name);

            std::wstring file_name_;
            bool changed_;
            bool is_sending_;
            uint32_t save_timer_id_;
            uint32_t send_timer_id_;
            uint32_t start_send_timer_id_;
            std::unique_ptr<async_executer> stats_thread_;
            std::vector<im_stats_event> events_;

            std::chrono::system_clock::time_point last_sent_time_;
            std::chrono::system_clock::time_point start_send_time_;
            std::chrono::seconds events_send_interval_;
            std::chrono::seconds events_max_store_interval_;

            void serialize_events(rapidjson::Value& _node, rapidjson_allocator& _a) const;
            std::string get_post_data() const;

            void serialize(tools::binary_stream& _bs) const;
            bool unserialize_props(tools::tlvpack& prop_pack, event_props_type* props);
            bool unserialize(tools::binary_stream& _bs);

            bool load();
            void clear(int32_t _send_result);
            void save_if_needed();
            void send_async();
            void start_save();
            void start_send(bool _check_start_now = true);
            void delayed_start_send();

            void insert_event(im_stat_event_names _event_name,
                              event_props_type&& _props,
                              std::chrono::system_clock::time_point _event_time);

            void mark_all_events_sent(bool _is_sent);
        };
    }
}
