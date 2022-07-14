#pragma once

#include "subscr_types.h"

namespace core::wim::subscriptions
{
    using subscr_clock_t = std::chrono::steady_clock;

    class subscription_base
    {
    public:
        subscription_base(subscriptions::type _type);
        virtual ~subscription_base() = default;

        virtual void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const = 0;
        virtual void serialize_args(rapidjson::Value& _node, rapidjson_allocator& _a) const = 0;
        virtual subscr_clock_t::duration renew_interval() const = 0;

        virtual std::vector<std::string> get_log_markers() const { return {}; }

        void set_requested();
        subscr_clock_t::time_point request_time() const noexcept { return request_time_; }
        bool is_requested() const;

        void reset_request_time();

        subscriptions::type get_type() const noexcept { return type_; }

        bool operator==(const subscription_base& _other) const;

    protected:
        virtual bool equal(const subscription_base& _other) const = 0;

    private:
        subscriptions::type type_;
        subscr_clock_t::time_point request_time_;
    };

    class antivirus_subscription : public subscription_base
    {
    public:
        antivirus_subscription(std::vector<std::string> _file_hashes = {});

        void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const override;
        void serialize_args(rapidjson::Value& _node, rapidjson_allocator& _a) const override;
        subscr_clock_t::duration renew_interval() const override;
        std::vector<std::string> get_log_markers() const override;

    private:
        bool equal(const subscription_base& _other) const override;

    private:
        std::vector<std::string> hashes_;
    };

    class status_subscription : public subscription_base
    {
    public:
        status_subscription(std::vector<std::string> _contacts = {});

        void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const override;
        void serialize_args(rapidjson::Value& _node, rapidjson_allocator& _a) const override;
        subscr_clock_t::duration renew_interval() const override;

    private:
        bool equal(const subscription_base& _other) const override;

    private:
        std::vector<std::string> contacts_;
    };

    class call_room_info_subscription : public subscription_base
    {
    public:
        call_room_info_subscription(std::vector<std::string> _room_ids = {});

        void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const override;
        void serialize_args(rapidjson::Value& _node, rapidjson_allocator& _a) const override;
        subscr_clock_t::duration renew_interval() const override;

    private:
        bool equal(const subscription_base& _other) const override;

    private:
        std::vector<std::string> room_ids_;
    };

    class thread_subscription : public subscription_base
    {
    public:
        thread_subscription(std::vector<std::string> _thread_ids = {});

        void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const override;
        void serialize_args(rapidjson::Value& _node, rapidjson_allocator& _a) const override;
        subscr_clock_t::duration renew_interval() const override;

    private:
        bool equal(const subscription_base& _other) const override;

    private:
        std::vector<std::string> thread_ids_;
    };

    class task_subscription : public subscription_base
    {
    public:
        task_subscription(std::vector<std::string> _room_ids = {});

        void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const override;
        void serialize_args(rapidjson::Value& _node, rapidjson_allocator& _a) const override;
        subscr_clock_t::duration renew_interval() const override;

    private:
        bool equal(const subscription_base& _other) const override;

    private:
        std::vector<std::string> task_ids_;
    };

    class tasks_counter_subscription : public subscription_base
    {
    public:
        tasks_counter_subscription();
        void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const override {};
        void serialize_args(rapidjson::Value& _node, rapidjson_allocator& _a) const override {};
        subscr_clock_t::duration renew_interval() const override;

    private:
        bool equal(const subscription_base&) const override { return true; };
    };

    class mails_counter_subscription : public subscription_base
    {
    public:
        mails_counter_subscription();
        void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const override {};
        void serialize_args(rapidjson::Value& _node, rapidjson_allocator& _a) const override {};
        subscr_clock_t::duration renew_interval() const override;

    private:
        bool equal(const subscription_base&) const override { return true; };
    };

    using subscr_ptr = std::shared_ptr<subscription_base>;
    using subscr_ptr_v = std::vector<subscr_ptr>;

    template<typename ...Args>
    subscr_ptr make_subscription(subscriptions::type _type, Args&&... args)
    {
        switch (_type)
        {
            case subscriptions::type::antivirus:
                return std::make_shared<antivirus_subscription>(std::forward<Args>(args)...);
            case subscriptions::type::status:
                return std::make_shared<status_subscription>(std::forward<Args>(args)...);
            case subscriptions::type::call_room_info:
                return std::make_shared<call_room_info_subscription>(std::forward<Args>(args)...);
            case subscriptions::type::thread:
                return std::make_shared<thread_subscription>(std::forward<Args>(args)...);
            case subscriptions::type::task:
                return std::make_shared<task_subscription>(std::forward<Args>(args)...);
            case subscriptions::type::tasks_counter:
                return std::make_shared<tasks_counter_subscription>();
            case subscriptions::type::mails_counter:
                return std::make_shared<mails_counter_subscription>();
            default:
                im_assert(!"Unknown subscription type");
                break;
        }

        return {};
    }
}
