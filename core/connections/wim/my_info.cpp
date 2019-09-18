#include "stdafx.h"
#include "my_info.h"
#include "../../tools/json_helper.h"

using namespace core;
using namespace wim;

namespace
{
    enum info_fields
    {
        aimId = 1,
        displayId, // now unused, replaced with nick
        friendly,
        state,
        userType,
        phoneNumber,
        flags,
        autoCreated,
        hasMail,
        userAgreement,
        official,
        nick,
    };
}

my_info::my_info() = default;

int32_t my_info::unserialize(const rapidjson::Value& _node)
{
    tools::unserialize_value(_node, "aimId", aimId_);
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

void my_info::serialize(core::tools::binary_stream& _data) const
{
    core::tools::tlvpack info_pack;
    info_pack.push_child(core::tools::tlv(info_fields::aimId, aimId_));
    info_pack.push_child(core::tools::tlv(info_fields::nick, nick_));
    info_pack.push_child(core::tools::tlv(info_fields::friendly, friendly_));
    info_pack.push_child(core::tools::tlv(info_fields::state, state_));
    info_pack.push_child(core::tools::tlv(info_fields::userType, userType_));
    info_pack.push_child(core::tools::tlv(info_fields::phoneNumber, phoneNumber_));
    info_pack.push_child(core::tools::tlv(info_fields::flags, flags_));
    info_pack.push_child(core::tools::tlv(info_fields::hasMail, hasMail_));
    info_pack.push_child(core::tools::tlv(info_fields::official, official_));
// not needed?   info_pack.push_child(core::tools::tlv(info_fields::userAgreement, user_agreement_info_));

    info_pack.serialize(_data);
}

bool my_info::unserialize(core::tools::binary_stream& _data)
{
    core::tools::tlvpack info_pack;
    if (!info_pack.unserialize(_data))
        return false;

    const auto item_aimId =  info_pack.get_item(info_fields::aimId);
    const auto item_nick = info_pack.get_item(info_fields::nick);
    const auto item_friendly = info_pack.get_item(info_fields::friendly);
    const auto item_state = info_pack.get_item(info_fields::state);
    const auto item_userType = info_pack.get_item(info_fields::userType);
    const auto item_phoneNumber = info_pack.get_item(info_fields::phoneNumber);
    const auto item_flags = info_pack.get_item(info_fields::flags);
    const auto item_has_mail = info_pack.get_item(info_fields::hasMail);
    const auto item_official = info_pack.get_item(info_fields::official);

    if (!item_aimId || !item_nick  || !item_friendly || !item_state || !item_userType || (!item_phoneNumber && !item_flags))
        return false;

    aimId_ = item_aimId->get_value<std::string>(std::string());
    nick_ = item_nick->get_value<std::string>(std::string());
    friendly_ = item_friendly->get_value<std::string>(std::string());
    state_ = item_state->get_value<std::string>(std::string());
    userType_ = item_userType->get_value<std::string>(std::string());
    phoneNumber_ = item_phoneNumber->get_value<std::string>(std::string());
    flags_ = item_flags->get_value<uint32_t>(0);
    if (item_has_mail)
        hasMail_ = item_has_mail->get_value<bool>(false);

    if (item_official)
        official_ = item_official->get_value<bool>(false);

    return true;
}

bool my_info::is_phone_number_exists() const
{
    return !phoneNumber_.empty();
}

my_info_cache::my_info_cache()
    : changed_(false)
    , info_(std::make_shared<my_info>())
{

}

std::shared_ptr<my_info> my_info_cache::get_info() const
{
    return info_;
}

void my_info_cache::set_info(std::shared_ptr<my_info> _info)
{
    *info_ = *_info;
    changed_ = true;
}


bool my_info_cache::is_changed() const
{
    return changed_;
}

bool my_info_cache::is_phone_number_exists() const
{
    return info_ && info_->is_phone_number_exists();
}

int32_t my_info_cache::save(const std::wstring& _filename)
{
    core::archive::storage storage(_filename);

    archive::storage_mode mode;
    mode.flags_.write_ = true;
    mode.flags_.truncate_ = true;
    if (!storage.open(mode))
        return 1;

    core::tools::binary_stream block_data;
    info_->serialize(block_data);

    int64_t offset = 0;
    if (storage.write_data_block(block_data, offset))
    {
        changed_ = false;
        return 0;
    }

    return 1;
}

int32_t my_info_cache::load(const std::wstring& _filename)
{
    core::archive::storage storage(_filename);

    archive::storage_mode mode;
    mode.flags_.read_ = true;

    if (!storage.open(mode))
        return 1;

    core::tools::binary_stream state_stream;
    if (!storage.read_data_block(-1, state_stream))
        return 1;

    return info_->unserialize(state_stream) ? 0 : 1;
}
