#include "stdafx.h"
#include "draft_storage.h"
#include "../corelib/collection_helper.h"
#include "../../common.shared/json_helper.h"
#include "tools/system.h"

namespace
{

enum data_tlv_fields : uint32_t
{
    tlv_state = 1,
    tlv_timestamp = 2,
    tlv_message = 3,
    tlv_local_timestamp = 4,
    tlv_friendly = 5,
};

bool is_first_same_or_newer(const core::archive::draft& first, const core::archive::draft& second)
{
    auto both_have_local_timestamps = std::min(first.local_timestamp_, second.local_timestamp_) > 0;
    if (both_have_local_timestamps)
        return first.local_timestamp_ >= second.local_timestamp_;

    return first.timestamp_ >= second.timestamp_;
}

}

namespace core::archive
{

bool draft::empty() const
{
    return !message_ || (message_->get_text().empty() && message_->get_quotes().empty());
}

void draft::serialize(coll_helper& _coll, time_t _time_offset) const
{
    _coll.set_value_as_bool("synced", state_ == state::synced);
    if (local_timestamp_ > 0)
        _coll.set_value_as_int64("local_timestamp", local_timestamp_);
    else
        _coll.set_value_as_int64("local_timestamp", (timestamp_ > 0 ? timestamp_ + _time_offset : timestamp_) * 1000);

    _coll.set_value_as_int("server_timestamp", timestamp_);

    coll_helper message_coll(_coll->create_collection(), true);
    if (message_)
    {
        message_->serialize(message_coll.get(), 0);
        _coll.set_value_as_collection("message", message_coll.get());
    }
}

void draft::serialize(core::tools::binary_stream& _data) const
{
    core::tools::tlvpack pack;
    pack.push_child(core::tools::tlv(tlv_state, state_));
    pack.push_child(core::tools::tlv(tlv_timestamp, timestamp_));
    pack.push_child(core::tools::tlv(tlv_local_timestamp, local_timestamp_));
    pack.push_child(core::tools::tlv(tlv_friendly, friendly_));

    core::tools::binary_stream bs_message;
    if (message_)
        message_->serialize(bs_message);
    pack.push_child(tools::tlv(tlv_message, bs_message));

    pack.serialize(_data);
}

bool draft::unserialize(const core::tools::binary_stream& _data)
{
    core::tools::tlvpack pack;
    if (!pack.unserialize(_data))
        return false;

    auto state_item = pack.get_item(tlv_state);
    auto message_item = pack.get_item(tlv_message);
    auto timestamp_item = pack.get_item(tlv_timestamp);
    auto local_timestamp_item = pack.get_item(tlv_local_timestamp);
    auto friendly_item = pack.get_item(tlv_friendly);

    if (state_item)
        state_ = state_item->get_value<state>(state::local);

    if (timestamp_item)
        timestamp_ = timestamp_item->get_value<int32_t>(0);

    if (local_timestamp_item)
        local_timestamp_ = local_timestamp_item->get_value<int64_t>(0);

    if (friendly_item)
        friendly_ = friendly_item->get_value<std::string>(std::string());

    if (message_item)
    {
        core::tools::binary_stream stream = message_item->get_value<core::tools::binary_stream>();
        auto message = std::make_shared<history_message>();
        if (!message->unserialize(stream))
            message_ = message;
    }

    return message_ != nullptr && timestamp_ > 0;
}

bool draft::unserialize(const rapidjson::Value& _node, const std::string& _contact)
{
    message_ = std::make_shared<history_message>();
    tools::unserialize_value(_node, "time", timestamp_);
    return timestamp_ > 0 && message_->unserialize(_node, _contact);
}

draft_storage::draft_storage(std::wstring _data_path)
    : path_(std::move(_data_path))
{
    load();
}

const draft& draft_storage::get_draft() noexcept
{
    return draft_;
}

void draft_storage::set_draft(const draft& _draft)
{
    if (is_first_same_or_newer(_draft, draft_))
    {
        draft_ = _draft;
        write_file();
    }
}

void draft_storage::load()
{
    core::tools::binary_stream bstream;
    if (!bstream.load_from_file(path_))
        return;

    if (!draft_.unserialize(bstream))
    {
        core::tools::system::delete_file(path_);
        draft_ = {};
    }
}

void draft_storage::write_file()
{
    core::tools::binary_stream bstream;
    draft_.serialize(bstream);
    bstream.save_2_file(path_);
}

}
