#pragma once

#include "proxy_settings.h"

namespace core
{
    class coll_helper;
    class async_executer;

    namespace tools
    {
        class binary_stream;
    }

    const static std::string flurry_url = "https://data.flurry.com/aah.do";

#ifdef DEBUG
    const static auto send_interval = std::chrono::minutes(1);
    const static auto fetch_ram_usage_interval = std::chrono::seconds(30);
#else
    const static auto send_interval = std::chrono::hours(1);
    const static auto fetch_ram_usage_interval = std::chrono::hours(1);
#endif // DEBUG

    const static auto save_to_file_interval = std::chrono::seconds(10);
    const static auto delay_send_on_start = std::chrono::seconds(10);
    const static auto fetch_disk_size_interval = std::chrono::hours(24);

    namespace stats
    {
        enum class stats_event_names;

        class statistics : public std::enable_shared_from_this<statistics>
        {
        public:
            struct disk_stats
            {
                disk_stats(size_t _user_folder_size = -1)
                    : user_folder_size_(_user_folder_size)
                {}
                bool is_initialized() const { return user_folder_size_ != 0; }
                size_t user_folder_size_ = 0;
            };

        private:

            class stats_event
            {
            public:
                std::string to_string(time_t _start_time) const;
                stats_event(stats_event_names _name, std::chrono::system_clock::time_point _event_time, int32_t _event_id, const event_props_type& props);
                stats_event_names get_name() const;
                event_props_type get_props() const;
                static void reset_session_event_id();
                std::chrono::system_clock::time_point get_time() const;
                int32_t get_id() const;
                template <typename ValueType>
                void increment_prop(event_prop_key_type _key, ValueType _value);

                template <typename ValueType>
                void set_prop(event_prop_key_type _key, ValueType _value);

                bool finished_accumulating();

            private:
                stats_event_names name_;
                int32_t event_id_; // natural serial number, starting from 1
                event_props_type props_;
                static long long session_event_id_;
                std::chrono::system_clock::time_point event_time_;
            };

            struct stop_objects
            {
                std::atomic_bool is_stop_;
                stop_objects()
                {
                    is_stop_ = false;
                }
            };

            uint64_t last_used_ram_mb_ = 0;

            typedef std::vector<stats_event>::const_iterator events_ci;

            static std::shared_ptr<stop_objects> stop_objects_;
            std::map<std::string, tools::binary_stream> values_;
            std::wstring file_name_;

            disk_stats disk_stats_;

            bool changed_;
            uint32_t save_timer_;
            uint32_t disk_stats_timer_;
            uint32_t ram_stats_timer_;
            uint32_t send_timer_;
            uint32_t start_send_timer_;
            std::unique_ptr<async_executer> stats_thread_;
            std::vector<stats_event> events_;
            std::vector<stats_event> accumulated_events_;
            std::string events_to_json(events_ci begin, events_ci end, time_t _start_time) const;
            std::chrono::system_clock::time_point last_sent_time_;
            std::vector<std::string> get_post_data() const;
            static bool send(const proxy_settings& _user_proxy, const std::string& post_data, const std::wstring& _file_name);

            void serialize(tools::binary_stream& _bs) const;
            bool unserialize(tools::binary_stream& _bs);
            void save_if_needed();
            void send_async();
            void set_disk_stats(const disk_stats &_stats);
            void query_disk_size_async();
            bool load();
            void start_save();
            void start_send();
            void insert_event(stats_event_names _event_name, const event_props_type& _props,
                std::chrono::system_clock::time_point _event_time, int32_t _event_id);
            void insert_accumulated_event(stats_event_names _event_name, core::stats::event_props_type _props);
            void clear();
            void delayed_start_send();
            void start_disk_operations();
            void start_ram_usage_monitoring();
            void check_ram_usage();

        public:
            statistics(std::wstring _file_name);
            virtual ~statistics();

            void init();
            void insert_event(stats_event_names _event_name, const event_props_type& _props);
            void insert_event(stats_event_names _event_name);

            // Inserts if doesn't exist
            template <typename ValueType>
            void increment_event_prop(stats_event_names _event_name,
                                      event_prop_key_type _prop_key,
                                      ValueType _value);
        };
    }
}
