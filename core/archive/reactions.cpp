#include "stdafx.h"

#include "../tools/json_helper.h"
#include "../../corelib/collection_helper.h"
#include "tools/system.h"

#include "reactions.h"

namespace
{

enum data_tlv_fields : uint32_t
{
    tlv_msg_id = 1,
    tlv_my_reaction = 2,
    tlv_reaction_item = 3,
    tlv_reaction = 4,
    tlv_counter = 5
};

enum index_tlv_fields : uint32_t
{
    tlv_index_offset = 1,
    tlv_index_msg_id = 2,
};

int64_t reactions_reserved_size(int32_t _reactions_count, int64_t _current_size)
{
    int64_t added_size = _current_size * 0.2; // heuristic determining the maximum record size
    return _current_size + added_size;
}

}

namespace core::archive
{

void reactions_data::reaction::serialize(icollection* _collection) const
{
    coll_helper coll(_collection, false);
    coll.set_value_as_string("reaction", reaction_);
    coll.set_value_as_int64("count", count_);
}

void reactions_data::reaction::serialize(core::tools::tlvpack& _pack) const
{
    core::tools::tlvpack pack;
    pack.push_child(core::tools::tlv(tlv_reaction, reaction_));
    pack.push_child(core::tools::tlv(tlv_counter, count_));
    _pack.push_child(core::tools::tlv(tlv_reaction_item, pack));
}

bool reactions_data::reaction::unserialize(core::tools::tlvpack& _pack)
{
    if (const auto reaction_item = _pack.get_item(tlv_reaction))
    {
        reaction_ = reaction_item->get_value<std::string>();

        if (const auto count_item = _pack.get_item(tlv_counter))
            count_ = count_item->get_value<int64_t>();

        return true;
    }

    return false;
}

bool reactions_data::reaction::unserialize(const rapidjson::Value& _node)
{
    auto result = tools::unserialize_value(_node, "reaction", reaction_);
    result &= tools::unserialize_value(_node, "counter", count_);

    return result;
}

void reactions_data::serialize(icollection* _collection) const
{
    coll_helper coll(_collection, false);

    core::ifptr<core::iarray> reactions_array(_collection->create_array());
    reactions_array->reserve(reactions_.size());

    for (const auto& reaction : reactions_)
    {
        coll_helper reaction_collection(_collection->create_collection(), true);
        reaction.serialize(reaction_collection.get());

        core::ifptr<core::ivalue> answers_value(_collection->create_value());
        answers_value->set_as_collection(reaction_collection.get());
        reactions_array->push_back(answers_value.get());
    }
    coll.set_value_as_array("reactions", reactions_array.get());
    coll.set_value_as_string("my_reaction", my_reaction_);
    coll.set_value_as_int64("msg_id", msg_id_);
}

void reactions_data::serialize(core::tools::binary_stream& _data) const
{
    core::tools::tlvpack pack;
    pack.push_child(core::tools::tlv(tlv_msg_id, msg_id_));
    pack.push_child(core::tools::tlv(tlv_my_reaction, my_reaction_));

    for (auto& reaction : reactions_)
        reaction.serialize(pack);

    pack.serialize(_data);
}

bool reactions_data::unserialize(core::tools::binary_stream& _data)
{
    core::tools::tlvpack pack;
    if (!pack.unserialize(_data))
        return false;

    for (auto field = pack.get_first(); field; field = pack.get_next())
    {
        switch ((data_tlv_fields) field->get_type())
        {
            case tlv_msg_id:
                msg_id_ = field->get_value<int64_t>(0);
                break;
            case tlv_my_reaction:
                my_reaction_ = field->get_value<std::string>(std::string());
                break;
            case tlv_reaction_item:
            {
                reaction item;
                core::tools::tlvpack pack = field->get_value<core::tools::tlvpack>();
                if (item.unserialize(pack))
                    reactions_.push_back(std::move(item));
            }
            default:
                break;
        }
    }

    return msg_id_ != 0;
}

bool reactions_data::unserialize(const rapidjson::Value& _node)
{
    auto result = tools::unserialize_value(_node, "msgId", msg_id_);
    result &= tools::unserialize_value(_node, "chatId", chat_id_);
    tools::unserialize_value(_node, "myReaction", my_reaction_);

    auto it_reactions = _node.FindMember("reactions");
    if (it_reactions != _node.MemberEnd() && it_reactions->value.IsArray())
    {
        for (const auto& reaction_item : it_reactions->value.GetArray())
        {
            reaction item;
            if (item.unserialize(reaction_item))
                reactions_.push_back(std::move(item));
        }
    }

    return result;
}

reactions_storage::reactions_storage(std::wstring _index_path, std::wstring _data_path)
    : index_storage_(std::make_unique<storage>(std::move(_index_path))),
      data_storage_(std::make_unique<storage>(std::move(_data_path)))
{

}

void reactions_storage::get_reactions(const msgids_list& _ids_to_load, std::vector<reactions_data>& _reactions, msgids_list& _missing)
{
    if (!index_loaded_)
        load_index();

    storage_mode mode;
    mode.flags_.read_ = true;

    if (!data_storage_->open(mode))
    {
        _missing = _ids_to_load;
        return;
    }

    core::tools::auto_scope lb([data_storage = data_storage_.get()]{ data_storage->close(); });

    core::tools::binary_stream data;

    for (auto id : _ids_to_load)
    {
        auto loaded = false;

        auto it = index_.find(id);
        if (it != index_.end())
        {
            auto& offset = it->second;
            reactions_data reactions;

            if (data_storage_->read_data_block(offset, data) && reactions.unserialize(data))
            {
                _reactions.push_back(std::move(reactions));
                loaded = true;
            }
        }

        if (!loaded)
            _missing.push_back(id);
    }
}

void reactions_storage::insert_reactions(std::vector<reactions_data>& _reactions)
{
    if (!index_loaded_)
        load_index();

    storage_mode mode;        
    mode.flags_.write_ = true;
    mode.flags_.at_end_ = true;

    if (core::tools::system::is_exist(data_storage_->get_file_name()))
        mode.flags_.read_ = true;

    if (!data_storage_->open(mode))
        return;

    core::tools::auto_scope lb([data_storage = data_storage_.get()]{ data_storage->close(); });

    std::vector<index_record> added_records;

    for (auto& reactions_item : _reactions)
    {
        auto record_it = index_.find(reactions_item.msg_id_);

        if (record_it == index_.end())
        {
            added_records.push_back(add_record(reactions_item));
        }
        else
        {
            auto added_record = update_record_at(reactions_item, record_it->second);
            if (added_record)
                added_records.push_back(*added_record);
        }
    }

    save_index_records(added_records);
}

void reactions_storage::load_index()
{
    storage_mode mode;
    mode.flags_.read_ = true;

    if (!index_storage_->open(mode))
        return;

    core::tools::auto_scope lb([index_storage = index_storage_.get()]{ index_storage->close(); });

    core::tools::binary_stream data;

    int64_t offset = 0;
    while (index_storage_->read_data_block(-1, data))
    {
        index_record record;

        if (record.unserialize(data))
            index_[record.msg_id_] = record.offset_;

        index_offsets_[record.msg_id_] = offset;

        offset = index_storage_->get_offset();
        data.reset();
    }

    index_loaded_ = true;
}

void reactions_storage::save_index_records(const std::vector<reactions_storage::index_record>& _records)
{
    storage_mode mode;
    mode.flags_.write_ = true;
    mode.flags_.at_end_ = true;

    if (core::tools::system::is_exist(index_storage_->get_file_name()))
        mode.flags_.read_ = true;

    if (!index_storage_->open(mode))
        return;

    core::tools::auto_scope lb([index_storage = index_storage_.get()]{ index_storage->close(); });

    core::tools::binary_stream data;

    for (const auto& record : _records)
    {
        record.serialize(data);

        const auto it = index_offsets_.find(record.msg_id_);
        if (it != index_offsets_.end())
        {
            index_storage_->write_data_block_at(data, it->second);
        }
        else
        {
            int64_t offset;
            index_storage_->write_data_block(data, offset);
            index_offsets_[record.msg_id_] = offset;
        }

        data.reset();
    }
}

reactions_storage::index_record reactions_storage::add_record(const reactions_data& _reactions)
{
    core::tools::binary_stream data;

    _reactions.serialize(data);
    const auto reserved_size = reactions_reserved_size(_reactions.reactions_.size(), data.available());

    // reserve size
    data.set_input(reserved_size);
    data.resize(reserved_size, 0);

    int64_t offset = 0;
    data_storage_->write_data_block(data, offset);

    index_[_reactions.msg_id_] = offset;

    index_record added_record;
    added_record.msg_id_ = _reactions.msg_id_;
    added_record.offset_ = offset;

    return added_record;
}

std::optional<reactions_storage::index_record> reactions_storage::update_record_at(const reactions_data& _reactions, int64_t _offset)
{
    core::tools::binary_stream new_data;
    core::tools::binary_stream existing_data;

    std::optional<index_record> added_record;

    const auto read_successful = data_storage_->read_data_block(_offset, existing_data);
    _reactions.serialize(new_data);

    auto existing_size = existing_data.available();

    if (read_successful && new_data.available() <= existing_size)
    {
        new_data.set_input(existing_size);
        new_data.resize(existing_size, 0);
        data_storage_->write_data_block_at(new_data, _offset);
    }
    else
    {
        added_record = add_record(_reactions);
    }

    return added_record;
}

void reactions_storage::index_record::serialize(core::tools::binary_stream& _data) const
{
    core::tools::tlvpack pack;
    pack.push_child(core::tools::tlv(tlv_index_offset, offset_));
    pack.push_child(core::tools::tlv(tlv_index_msg_id, msg_id_));
    pack.serialize(_data);
}

bool reactions_storage::index_record::unserialize(core::tools::binary_stream& _data)
{
    core::tools::tlvpack pack;
    if (!pack.unserialize(_data))
        return false;

    if (const auto msg_id_item = pack.get_item(tlv_index_msg_id))
    {
        msg_id_ = msg_id_item->get_value<int64_t>();

        if (const auto offset_item = pack.get_item(tlv_index_offset))
        {
            offset_ = offset_item->get_value<int64_t>();
            return true;
        }
    }

    return false;
}


}
