#include "stdafx.h"
#include "pending_drafts.h"
#include "tools/system.h"

namespace
{

enum data_tlv_fields : uint32_t
{
    tlv_contact = 1,
    tlv_timestamp = 2,
};

}

namespace core::archive
{

pending_drafts::pending_drafts(std::wstring _data_path)
 : storage_(std::make_unique<storage>(std::move(_data_path)))
{
    load();
}

std::string pending_drafts::get_next_pending_draft_contact() const
{
    if (prev_pending_draft_contacts_.empty())
        return std::string();

    return prev_pending_draft_contacts_.back().second;
}

void pending_drafts::remove_pending_draft_contact(const std::string& _contact, int64_t _timestamp)
{
    bool needToSerialize = false;
    if (!prev_pending_draft_contacts_.empty() && prev_pending_draft_contacts_.back().second == _contact && prev_pending_draft_contacts_.back().first <= _timestamp)
    {
        prev_pending_draft_contacts_.pop_back();
        needToSerialize = true;
    }

    auto current_it = current_pending_draft_contacts_.find(_contact);
    if (current_it != current_pending_draft_contacts_.end() && current_it->second <= _timestamp)
    {
        current_pending_draft_contacts_.erase(current_it);
        needToSerialize = true;
    }

    if(needToSerialize)
        write_file();
}

void pending_drafts::add_pending_draft_contact(const std::string& _contact, int64_t _timestamp)
{   
    auto it = current_pending_draft_contacts_.find(_contact);
    if (it == current_pending_draft_contacts_.end())
        current_pending_draft_contacts_[_contact] = _timestamp;
    else if (it->second < _timestamp)
        it->second = _timestamp;

    write_file();
}

void pending_drafts::load()
{
    storage_mode mode;
    mode.flags_.read_ = true;

    if (!storage_->open(mode))
        return;

    core::tools::auto_scope lb([storage = storage_.get()]{ storage->close(); });

    core::tools::binary_stream data;

    while (storage_->read_data_block(-1, data))
    {
        core::tools::tlvpack pack;
        if (!pack.unserialize(data))
            return;

        auto contact_item = pack.get_item(tlv_contact);
        auto timestamp_item = pack.get_item(tlv_timestamp);

        if (contact_item && timestamp_item)
        {
            auto timestamp = timestamp_item->get_value<int64_t>();
            auto contact = contact_item->get_value<std::string>();

            if (timestamp >= 0 && !contact.empty())
                prev_pending_draft_contacts_.push_back({timestamp, contact});
        }

        data.reset();
    }

    std::sort(prev_pending_draft_contacts_.begin(), prev_pending_draft_contacts_.end(), std::greater<std::pair<int64_t, std::string>>());
}

void pending_drafts::write_file()
{
    storage_mode mode;
    mode.flags_.write_ = true;
    mode.flags_.truncate_ = true;

    if (core::tools::system::is_exist(storage_->get_file_name()))
        mode.flags_.read_ = true;

    if (!storage_->open(mode))
        return;

    core::tools::auto_scope lb([storage = storage_.get()]{ storage->close(); });

    core::tools::binary_stream data;
    int64_t offset;

    auto serialize = [this, &data, &offset](const std::string& _contact, int64_t _timestamp)
    {
        core::tools::tlvpack pack;
        pack.push_child(core::tools::tlv(tlv_contact, _contact));
        pack.push_child(core::tools::tlv(tlv_timestamp, _timestamp));
        pack.serialize(data);

        storage_->write_data_block(data, offset);
        data.reset();
    };

    for (const auto& [timestamp, contact] : prev_pending_draft_contacts_)
        serialize(contact, timestamp);

    for (const auto& [contact, timestamp] : current_pending_draft_contacts_)
        serialize(contact, timestamp);
}

}
