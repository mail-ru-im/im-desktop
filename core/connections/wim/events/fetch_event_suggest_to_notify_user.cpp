#include "stdafx.h"

#include "../wim_im.h"
#include "../common.shared/json_helper.h"
#include "fetch_event_suggest_to_notify_user.h"

namespace core::wim
{

void fetch_event_suggest_to_notify_user::notify_user_info::serialize(coll_helper& _coll) const
{
    _coll.set_value_as_string("contact", sn_);
    _coll.set_value_as_string("smsNotifyContext", sms_notify_context_);
}

bool fetch_event_suggest_to_notify_user::notify_user_info::unserialize(const rapidjson::Value& _node)
{
    auto result = tools::unserialize_value(_node, "sn", sn_);
    result &= tools::unserialize_value(_node, "smsNotifyContext", sms_notify_context_);
    return result;
}

int32_t fetch_event_suggest_to_notify_user::parse(const rapidjson::Value& _node_event_data)
{
    return info_.unserialize(_node_event_data);
}

void fetch_event_suggest_to_notify_user::on_im(std::shared_ptr<im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_suggest_to_notify_user(this, std::move(_on_complete));
}

const fetch_event_suggest_to_notify_user::notify_user_info& fetch_event_suggest_to_notify_user::get_info() const
{
    return info_;
}


}
