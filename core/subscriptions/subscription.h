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
        antivirus_subscription(std::string_view _file_hash);

        void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const override;
        subscr_clock_t::duration renew_interval() const override;
        std::vector<std::string> get_log_markers() const override;

    private:
        bool equal(const subscription_base& _other) const override;

    private:
        std::string hash_;
    };

    class status_subscription : public subscription_base
    {
    public:
        status_subscription(std::string_view _contact);

        void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const override;
        subscr_clock_t::duration renew_interval() const override;

    private:
        bool equal(const subscription_base& _other) const override;

    private:
        std::string contact_;
    };

    class call_room_info_subscription : public subscription_base
    {
    public:
        call_room_info_subscription(std::string_view _room_id);

        void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const override;
        subscr_clock_t::duration renew_interval() const override;

    private:
        bool equal(const subscription_base& _other) const override;

    private:
        std::string room_id_;
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
            default:
                break;
        }

        return {};
    }
}
