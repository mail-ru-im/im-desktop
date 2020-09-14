#include "stdafx.h"
#include "my_info.h"
#include "../../core.h"
#include "../../tools/json_helper.h"
#include "../../archive/storage.h"

using namespace core;
using namespace wim;

my_info::my_info() = default;

int32_t my_info::unserialize(const rapidjson::Value& _node)
{
    if (!tools::unserialize_value(_node, "aimId", aimId_))
        return -1;

    tools::unserialize_value(_node, "nick", nick_);
    tools::unserialize_value(_node, "friendly", friendly_);
    tools::unserialize_value(_node, "state", state_);
    tools::unserialize_value(_node, "userType", userType_);
    tools::unserialize_value(_node, "attachedPhoneNumber", phoneNumber_);
    tools::unserialize_value(_node, "globalFlags", flags_);
    tools::unserialize_value(_node, "hasMail", hasMail_);
    tools::unserialize_value(_node, "official", official_);

    const auto iter_expressions = _node.FindMember("expressions");
    if (iter_expressions != _node.MemberEnd())
        tools::unserialize_value(iter_expressions->value, "largeBuddyIcon", largeIconId_);

    auto item_user_agreement = _node.FindMember("userAgreement");
    if (item_user_agreement != _node.MemberEnd() && item_user_agreement->value.IsArray())
        user_agreement_info_.unserialize(item_user_agreement->value);

    if (state_ == "occupied" || state_ == "na" || state_ == "busy")
        state_ = "dnd";
    else if (state_ != "away" && state_ != "offline" && state_ != "invisible")
        state_ = "online";

    return 0;
}

int32_t my_info::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    _node.AddMember("aimId", aimId_, _a);
    _node.AddMember("nick", nick_, _a);
    _node.AddMember("friendly", friendly_, _a);
    _node.AddMember("state", state_, _a);
    _node.AddMember("userType", userType_, _a);
    _node.AddMember("attachedPhoneNumber", phoneNumber_, _a);
    _node.AddMember("globalFlags", flags_, _a);
    _node.AddMember("hasMail", hasMail_, _a);
    _node.AddMember("official", official_, _a);

    return 0;
}

void my_info::serialize(core::coll_helper _coll)
{
    _coll.set_value_as_string("aimId", aimId_);
    _coll.set_value_as_string("nick", nick_);
    _coll.set_value_as_string("friendly", friendly_);
    _coll.set_value_as_string("state", state_);
    _coll.set_value_as_string("userType", userType_);
    _coll.set_value_as_string("attachedPhoneNumber", phoneNumber_);
    _coll.set_value_as_uint("globalFlags", flags_);
    _coll.set_value_as_string("largeIconId", largeIconId_);
    _coll.set_value_as_bool("hasMail", hasMail_);
    _coll.set_value_as_bool("official", official_);

    user_agreement_info_.serialize(_coll);
}

bool my_info::is_phone_number_exists() const
{
    return !phoneNumber_.empty();
}

bool my_info::operator==(const my_info& _right) const
{
    return
        aimId_ == _right.aimId_ &&
        nick_ == _right.nick_ &&
        friendly_ == _right.friendly_ &&
        state_ == _right.state_ &&
        userType_ == _right.userType_ &&
        phoneNumber_ == _right.phoneNumber_ &&
        flags_ == _right.flags_ &&
        largeIconId_ == _right.largeIconId_ &&
        hasMail_ == _right.hasMail_ &&
        official_ == _right.official_ &&
        user_agreement_info_ == _right.user_agreement_info_;
}

bool my_info::operator!=(const my_info& _right) const
{
    return !(*this == _right);
}


my_info_cache::my_info_cache()
    : info_(std::make_shared<my_info>())
{

}

void my_info_cache::set_info(const std::shared_ptr<my_info>& _info)
{
    *info_ = *_info;

    changed_ = true;
}

void my_info_cache::set_status(status _status)
{
    status_ = std::move(_status);
    changed_ = true;
}

bool my_info_cache::is_phone_number_exists() const
{
    return info_ && info_->is_phone_number_exists();
}

int32_t my_info_cache::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    rapidjson::Value info_node(rapidjson::Type::kObjectType);
    info_->serialize(info_node, _a);
    _node.AddMember("info", std::move(info_node), _a);

    if (!status_.is_empty())
        status_.serialize(_node, _a);

    return 0;
}

int32_t my_info_cache::load(std::wstring_view _filename)
{
    int32_t ret = -1;
    core::tools::binary_stream bstream;
    if (!bstream.load_from_file(_filename))
        return ret;

    bstream.write<char>('\0');

    rapidjson::Document doc;
    if (doc.Parse((const char*) bstream.read(bstream.available())).HasParseError())
        return ret;

    bool old_scheme = true;

    if (const auto iter_status = doc.FindMember("status"); iter_status != doc.MemberEnd() && iter_status->value.IsObject())
    {
        ret = status_.unserialize(iter_status->value);
        old_scheme = false;
    }
    if (const auto iter_info = doc.FindMember("info"); iter_info != doc.MemberEnd() && iter_info->value.IsObject())
    {
        ret = info_->unserialize(iter_info->value);
        old_scheme = false;
    }

    if (old_scheme)
        return info_->unserialize(doc);

    return ret;
}
