#include "stdafx.h"

#include "thread_update_storage.h"
#include "thread_update.h"

#include "tools/system.h"

namespace
{
    enum index_tlv_fields : uint32_t
    {
        tlv_index_offset = 1,
        tlv_index_msg_id = 2,
    };

    int64_t reserved_size(int64_t _current_size)
    {
        int64_t added_size = _current_size * 0.2; // heuristic determining the maximum record size
        return _current_size + added_size;
    }
}

namespace core::archive
{
    thread_update_storage::thread_update_storage(std::wstring _index_path, std::wstring _data_path)
        : index_storage_(std::make_unique<storage>(std::move(_index_path)))
        , data_storage_(std::make_unique<storage>(std::move(_data_path)))
    {

    }

    void thread_update_storage::get_thread_updates(const msgids_list& _ids_to_load, std::vector<thread_update>& _thread_updates, msgids_list& _missing)
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

        _thread_updates.reserve(_ids_to_load.size());
        for (auto id : _ids_to_load)
        {
            auto loaded = false;

            if (const auto it = index_.find(id); it != index_.end())
            {
                if (data_storage_->read_data_block(it->second, data))
                {
                    if (thread_update update; update.unserialize(data))
                    {
                        _thread_updates.push_back(std::move(update));
                        loaded = true;
                    }
                }
            }

            if (!loaded)
                _missing.push_back(id);
        }
    }

    void thread_update_storage::insert_thread_updates(const std::vector<thread_update>& _thread_updates)
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
        added_records.reserve(_thread_updates.size());

        for (const auto& update : _thread_updates)
        {
            auto record_it = index_.find(update.get_msgid());

            if (record_it == index_.end())
            {
                added_records.push_back(add_record(update));
            }
            else
            {
                auto added_record = update_record_at(update, record_it->second);
                if (added_record)
                    added_records.push_back(*added_record);
            }
        }

        save_index_records(added_records);
    }

    void thread_update_storage::load_index()
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

    void thread_update_storage::save_index_records(const std::vector<thread_update_storage::index_record>& _records)
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

    thread_update_storage::index_record thread_update_storage::add_record(const thread_update& _update)
    {
        core::tools::binary_stream data;
        _update.serialize(data);
        return add_record(_update.get_msgid(), std::move(data));
    }

    thread_update_storage::index_record thread_update_storage::add_record(int64_t _msgid, core::tools::binary_stream _update_data)
    {
        const auto rs = reserved_size(_update_data.available());

        // reserve size
        _update_data.set_input(rs);
        _update_data.resize(rs, 0);

        int64_t offset = 0;
        data_storage_->write_data_block(_update_data, offset);

        index_[_msgid] = offset;

        index_record added_record;
        added_record.msg_id_ = _msgid;
        added_record.offset_ = offset;

        return added_record;
    }

    std::optional<thread_update_storage::index_record> thread_update_storage::update_record_at(const thread_update& _update, int64_t _offset)
    {
        core::tools::binary_stream new_data;
        core::tools::binary_stream existing_data;

        std::optional<index_record> added_record;

        const auto read_successful = data_storage_->read_data_block(_offset, existing_data);
        _update.serialize(new_data);

        const auto existing_size = existing_data.available();

        if (read_successful && new_data.available() <= existing_size)
        {
            new_data.set_input(existing_size);
            new_data.resize(existing_size, 0);
            data_storage_->write_data_block_at(new_data, _offset);
        }
        else
        {
            added_record = add_record(_update.get_msgid(), std::move(new_data));
        }

        return added_record;
    }

    void thread_update_storage::index_record::serialize(core::tools::binary_stream& _data) const
    {
        core::tools::tlvpack pack;
        pack.push_child(core::tools::tlv(tlv_index_offset, offset_));
        pack.push_child(core::tools::tlv(tlv_index_msg_id, msg_id_));
        pack.serialize(_data);
    }

    bool thread_update_storage::index_record::unserialize(core::tools::binary_stream& _data)
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