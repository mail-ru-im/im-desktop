#pragma once

namespace core
{
    class coll_helper;

    class status
    {
    public:
        using clock_t = std::chrono::system_clock;

        void serialize(coll_helper& _coll) const;
        void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
        bool unserialize(const coll_helper& _coll);
        bool unserialize(const rapidjson::Value& _node);

        void set_status(std::string_view _status) { status_ = _status; }
        const std::string& get_status() const noexcept { return status_; }

        void set_start_time(const clock_t::time_point _start_time) { start_time_ = _start_time; }
        clock_t::time_point get_start_time() const noexcept { return start_time_; }

        void set_end_time(const clock_t::time_point _end_time) { end_time_ = _end_time; }
        std::optional<clock_t::time_point> get_end_time() const noexcept { return end_time_; }

        void set_sending(const bool _sending) { my_status_sending_ = _sending; }
        bool is_sending() const noexcept { return my_status_sending_; }

        bool is_empty() const;

    private:
        std::string status_;
        clock_t::time_point start_time_;
        std::optional<clock_t::time_point> end_time_;

        bool my_status_sending_ = false;
    };
}
