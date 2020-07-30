#include "stdafx.h"

#include "subscription.h"

#include "../common.shared/omicron_keys.h"
#include "../libomicron/include/omicron/omicron.h"

namespace core::wim::subscriptions
{
    subscription_base::subscription_base(subscriptions::type _type)
        : type_(_type)
        , request_time_(subscr_clock_t::time_point::min())
    {
        assert(type_ != subscriptions::type::invalid);
    }

    void subscription_base::set_requested()
    {
        request_time_ = subscr_clock_t::now();
    }

    bool subscription_base::is_requested() const
    {
        return request_time_ != subscr_clock_t::time_point::min();
    }

    void subscription_base::reset_request_time()
    {
        request_time_ = subscr_clock_t::time_point::min();
    }

    bool subscription_base::operator==(const subscription_base& _other) const
    {
        if (get_type() != _other.get_type())
            return false;

        if (typeid(*this).hash_code() != typeid(_other).hash_code())
            return false;

        return equal(_other);
    }

    antivirus_subscription::antivirus_subscription(std::string_view _file_hash)
        : subscription_base(subscriptions::type::antivirus)
        , hash_(_file_hash)
    {
        assert(!hash_.empty());
    }

    void antivirus_subscription::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
    {
        _node.AddMember("fileHash", hash_, _a);
    }

    subscr_clock_t::duration antivirus_subscription::renew_interval() const
    {
        constexpr std::chrono::seconds def = std::chrono::minutes(9) + std::chrono::seconds(30);
        return std::chrono::seconds(omicronlib::_o(omicron::keys::subscr_renew_interval_antivirus, def.count()));
    }

    std::vector<std::string> antivirus_subscription::get_log_markers() const
    {
        return { "fileHash" };
    }

    bool antivirus_subscription::equal(const subscription_base& _other) const
    {
        auto av_other = static_cast<const antivirus_subscription&>(_other);
        return hash_ == av_other.hash_;
    }

    status_subscription::status_subscription(std::string_view _contact)
        : subscription_base(subscriptions::type::status)
        , contact_(_contact)
    {
        assert(!contact_.empty());
    }

    void status_subscription::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
    {
        _node.AddMember("sn", contact_, _a);
    }

    subscr_clock_t::duration status_subscription::renew_interval() const
    {
        constexpr std::chrono::seconds def = std::chrono::minutes(1);
        return std::chrono::seconds(omicronlib::_o(omicron::keys::subscr_renew_interval_status, def.count()));
    }

    bool status_subscription::equal(const subscription_base& _other) const
    {
        auto st_other = static_cast<const status_subscription&>(_other);
        return contact_ == st_other.contact_;
    }

    call_room_info_subscription::call_room_info_subscription(std::string_view _room_id)
        : subscription_base(subscriptions::type::call_room_info),
          room_id_(_room_id)
    {

    }

    void call_room_info_subscription::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
    {
        _node.AddMember("roomId", room_id_, _a);
    }

    subscr_clock_t::duration call_room_info_subscription::renew_interval() const
    {
        constexpr std::chrono::seconds def = std::chrono::seconds(270);
        return std::chrono::seconds(omicronlib::_o(omicron::keys::subscr_renew_interval_call_room_info, def.count()));
    }

    bool call_room_info_subscription::equal(const subscription_base& _other) const
    {
        auto call_room_info_other = static_cast<const call_room_info_subscription&>(_other);
        return room_id_ == call_room_info_other.room_id_;
    }
}
