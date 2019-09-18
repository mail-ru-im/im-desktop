#pragma once

namespace omicronlib
{
    class omicron_config;

    namespace build
    {
        constexpr bool is_debug() noexcept
        {
#if defined(_DEBUG) || defined(DEBUG)
            return true;
#else
            return false;
#endif
        }
    }

    class omicron_impl : public std::enable_shared_from_this<omicron_impl>
    {
    public:
        static omicron_impl& instance();

        omicron_code init(const omicron_config& _conf, const std::wstring& _file_name = {});
        bool star_auto_updater();
        omicron_code update_data();
        void cleanup();

        time_t get_last_update_time() const;
        omicron_code get_last_update_status() const;

        void add_fingerprint(std::string _name, std::string _value);

        // instance methods
        bool bool_value(const char* _key_name, bool _default_value) const;
        int int_value(const char* _key_name, int _default_value) const;
        unsigned int uint_value(const char* _key_name, unsigned int _default_value) const;
        int64_t int64_value(const char* _key_name, int64_t _default_value) const;
        uint64_t uint64_value(const char* _key_name, uint64_t _default_value) const;
        double double_value(const char* _key_name, double _default_value) const;
        std::string string_value(const char* _key_name, std::string _default_value) const;
        json_string json_value(const char* _key_name, json_string _default_value) const;

    private:
        class spin_lock
        {
            std::atomic_flag locked = ATOMIC_FLAG_INIT;
        public:
            void lock()
            {
                while (locked.test_and_set(std::memory_order_acquire)) { ; }
            }
            void unlock()
            {
                locked.clear(std::memory_order_release);
            }
        };

        std::unique_ptr<omicron_config> config_;

        std::atomic<bool> is_json_data_init_;
        std::shared_ptr<rapidjson::Document> json_data_;
        mutable spin_lock data_spin_lock_;

        std::atomic<bool> is_schedule_stop_;
        std::mutex schedule_mutex_;
        std::condition_variable schedule_condition_;
        std::unique_ptr<std::thread> schedule_thread_;

        std::atomic<bool> is_updating_;
        std::chrono::system_clock::time_point last_update_time_;
        omicron_code last_update_status_;

        std::wstring file_name_;

        write_to_log_func logger_func_;

        callback_update_data_func callback_updater_func_;

        omicron_impl();

        bool load();
        bool save(const std::string& _data) const;

        std::shared_ptr<const rapidjson::Document> get_json_data() const;
        std::string get_json_data_string() const;

        omicron_code get_omicron_data(bool _is_save = true);

        bool parse_json(const std::string& _json);

        void write_to_log(const std::string& _text) const;

        void callback_updater();
    };
}
