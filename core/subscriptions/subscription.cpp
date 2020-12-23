#include "stdafx.h"

#include "subscription.h"

#include "../common.shared/omicron_keys.h"
#include "../libomicron/include/omicron/omicron.h"
#include "../tools/json_helper.h"

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

    antivirus_subscription::antivirus_subscription(std::vector<std::string> _file_hashes)
        : subscription_base(subscriptions::type::antivirus)
        , hashes_(std::move(_file_hashes))
    {
        assert(!hashes_.empty());
    }

    void antivirus_subscription::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
    {
        rapidjson::Value hashes_array(rapidjson::Type::kArrayType);
        hashes_array.Reserve(hashes_.size(), _a);
        for (auto& id : hashes_)
            hashes_array.PushBack(tools::make_string_ref(id), _a);

        _node.AddMember("fileHashes", std::move(hashes_array), _a);
    }

    subscr_clock_t::duration antivirus_subscription::renew_interval() const
    {
        constexpr std::chrono::seconds def = std::chrono::minutes(9) + std::chrono::seconds(30);
        return std::chrono::seconds(omicronlib::_o(omicron::keys::subscr_renew_interval_antivirus, def.count()));
    }

    std::vector<std::string> antivirus_subscription::get_log_markers() const
    {
        return { "fileHashes" };
    }

    bool antivirus_subscription::equal(const subscription_base& _other) const
    {
        auto av_other = static_cast<const antivirus_subscription&>(_other);
        return hashes_ == av_other.hashes_;
    }

    status_subscription::status_subscription(std::vector<std::string> _contacts)
        : subscription_base(subscriptions::type::status)
        , contacts_(std::move(_contacts))
    {
        assert(!contacts_.empty());
    }

    void status_subscription::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
    {
        rapidjson::Value contacts_array(rapidjson::Type::kArrayType);
        contacts_array.Reserve(contacts_.size(), _a);
        for (auto& sn : contacts_)
            contacts_array.PushBack(tools::make_string_ref(sn), _a);

        _node.AddMember("contacts", std::move(contacts_array), _a);
    }

    subscr_clock_t::duration status_subscription::renew_interval() const
    {
        constexpr std::chrono::seconds def = std::chrono::minutes(1);
        return std::chrono::seconds(omicronlib::_o(omicron::keys::subscr_renew_interval_status, def.count()));
    }

    bool status_subscription::equal(const subscription_base& _other) const
    {
        auto st_other = static_cast<const status_subscription&>(_other);
        return contacts_ == st_other.contacts_;
    }

    call_room_info_subscription::call_room_info_subscription(std::vector<std::string> _room_ids)
        : subscription_base(subscriptions::type::call_room_info)
        , room_ids_(std::move(_room_ids))
    {

    }

    void call_room_info_subscription::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
    {
        rapidjson::Value ids_array(rapidjson::Type::kArrayType);
        ids_array.Reserve(room_ids_.size(), _a);
        for (auto& id : room_ids_)
            ids_array.PushBack(tools::make_string_ref(id), _a);

        _node.AddMember("roomIds", std::move(ids_array), _a);
    }

    subscr_clock_t::duration call_room_info_subscription::renew_interval() const
    {
        constexpr std::chrono::seconds def = std::chrono::seconds(270);
        return std::chrono::seconds(omicronlib::_o(omicron::keys::subscr_renew_interval_call_room_info, def.count()));
    }

    bool call_room_info_subscription::equal(const subscription_base& _other) const
    {
        auto call_room_info_other = static_cast<const call_room_info_subscription&>(_other);
        return room_ids_ == call_room_info_other.room_ids_;
    }
}
