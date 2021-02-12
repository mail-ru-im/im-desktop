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
                std::string to_log_string(time_t _start_time) const;
                std::string dump_params_to_json() const;
                stats_event(stats_event_names _name, std::chrono::system_clock::time_point _event_time, int32_t _event_id, event_props_type&& props);
                stats_event_names get_name() const;
                event_props_type get_props() const;
                bool has_props() const;
                static void reset_session_event_id();
                std::chrono::system_clock::time_point get_time() const;
                int32_t get_id() const;
                template <typename ValueType>
                void increment_prop(event_prop_key_type _key, ValueType _value);

                template <typename ValueType>
                void set_prop(event_prop_key_type _key, ValueType _value);

                bool finished_accumulating();

                void increment_send_num_attempts() { num_attempt_to_send_++; }
                int get_num_attempts_to_send() const { return num_attempt_to_send_; };

            private:
                stats_event_names name_;
                int32_t event_id_; // natural serial number, starting from 1
                event_props_type props_;
                static long long session_event_id_;
                std::chrono::system_clock::time_point event_time_;
                int num_attempt_to_send_;
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
            std::wstring file_name_mytracker_;

            disk_stats disk_stats_;

            bool changed_mytracker_;
            uint32_t save_timer_;
            uint32_t disk_stats_timer_;
            uint32_t ram_stats_timer_;
            uint32_t send_mytracker_timer_;
            uint32_t start_send_timer_;
            std::unique_ptr<async_executer> stats_thread_;
            std::vector<stats_event> events_;

            //! Contains events not sent successfully yet
            std::set<std::shared_ptr<stats_event>> events_for_mytracker_;

            std::vector<stats_event> accumulated_events_;
            std::string dump_events_to_mytracker_json(events_ci begin, events_ci end, time_t _start_time) const;
            std::chrono::system_clock::time_point last_sent_time_;

            static std::string get_mytracker_post_data(const stats_event& event);
            static long send_mytracker(const proxy_settings& _user_proxy, std::string_view post_data, stats_event_names event_name, std::wstring_view _file_name);

            void serialize_mytracker(tools::binary_stream& _bs) const;
            bool unserialize_mytracker(tools::binary_stream& _bs);
            void save_if_needed();
            void save_mytracker();
            void send_mytracker_async(const std::shared_ptr<stats_event>& event);
            void resend_failed_mytracker_async();
            void set_disk_stats(const disk_stats &_stats);
            void query_disk_size_async();
            bool load();
            void start_save();
            void start_send();
            void insert_event_mytracker(stats_event_names _event_name, event_props_type&& _props,
                std::chrono::system_clock::time_point _event_time, int32_t _event_id);
            void insert_accumulated_event(stats_event_names _event_name, core::stats::event_props_type _props);
            void delayed_start_send();
            void start_disk_operations();
            void start_ram_usage_monitoring();
            void check_ram_usage();

        public:
            statistics(std::wstring _file_name_mytracker);
            virtual ~statistics();

            void init();
            void insert_event(stats_event_names _event_name, event_props_type _props);
            void insert_event(stats_event_names _event_name);

            // Inserts if doesn't exist
            template <typename ValueType>
            void increment_event_prop(stats_event_names _event_name,
                                      event_prop_key_type _prop_key,
                                      ValueType _value);
        };
    }
}
