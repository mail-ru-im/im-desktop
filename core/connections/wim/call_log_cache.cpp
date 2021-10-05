#include "stdafx.h"
#include "call_log_cache.h"

#include "../../../corelib/core_face.h"
#include "../../../corelib/collection_helper.h"
#include "../../../common.shared/json_helper.h"
#include "../../tools/tlv.h"

namespace core
{
    namespace archive
    {
        int32_t call_info::unserialize(const rapidjson::Value& _node, const archive::persons_map& _persons)
        {
            tools::unserialize_value(_node, "sn", aimid_);
            const auto node_message = _node.FindMember("message");
            if (node_message != _node.MemberEnd() && node_message->value.IsObject())
            {
                message_.unserialize(node_message->value, aimid_);
                message_.apply_persons_to_voip(_persons);
            }

            return 0;
        }

        int32_t call_info::unserialize(core::tools::binary_stream& _data)
        {
            return message_.unserialize_call(_data, aimid_);
        }

        void call_info::serialize(icollection* _collection, const time_t _offset) const
        {
            coll_helper coll(_collection, false);
            coll.set_value_as_string("aimid", aimid_);
            coll_helper coll_message(coll->create_collection(), true);
            message_.serialize(coll_message.get(), _offset);
            coll.set_value_as_collection("message", coll_message.get());
        }

        void call_info::serialize(core::tools::binary_stream& _data) const
        {
            message_.serialize_call(_data, aimid_);
        }

        call_log_cache::call_log_cache(const std::wstring& _filename)
            : storage_(std::make_unique<storage>(_filename))
        {
        }

        int32_t call_log_cache::unserialize(const rapidjson::Value& _node, const archive::persons_map& _persons)
        {
            calls_.clear();
            auto iter_calls = _node.FindMember("recentCalls");
            if (iter_calls != _node.MemberEnd() && iter_calls->value.IsArray())
            {
                calls_.reserve(iter_calls->value.Size());
                for (const auto& call : iter_calls->value.GetArray())
                {
                    call_info info;
                    info.unserialize(call, _persons);
                    calls_.push_back(info);
                }
            }

            return 0;
        }

        void call_log_cache::serialize(icollection* _collection, const time_t _offset) const
        {
            coll_helper coll(_collection, false);
            ifptr<iarray> calls_array(coll->create_array());
            calls_array->reserve((int32_t)calls_.size());
            for (const auto& c : calls_)
            {
                coll_helper coll_call(coll->create_collection(), true);
                c.serialize(coll_call.get(), _offset);
                ifptr<ivalue> val(coll->create_value());
                val->set_as_collection(coll_call.get());
                calls_array->push_back(val.get());
            }

            coll.set_value_as_array("calls", calls_array.get());
        }

        int32_t call_log_cache::load()
        {
            load_cache();
            return 0;
        }

        bool call_log_cache::save_cache() const
        {
            if (!storage_)
            {
                im_assert(!"cache storage");
                return false;
            }

            archive::storage_mode mode;
            mode.flags_.write_ = true;
            mode.flags_.truncate_ = true;

            if (!storage_->open(mode))
                return false;

            core::tools::auto_scope lb([this] { storage_->close(); });

            core::tools::tlvpack block;

            for (const auto& i : calls_)
            {
                core::tools::binary_stream data;
                i.serialize(data);
                block.push_child(core::tools::tlv(0, data));
            }

            core::tools::binary_stream stream;
            block.serialize(stream);

            int64_t offset = 0;
            return storage_->write_data_block(stream, offset);
        }

        bool call_log_cache::load_cache()
        {
            if (!storage_)
            {
                im_assert(!"cache storage");
                return false;
            }

            archive::storage_mode mode;
            mode.flags_.read_ = true;

            if (!storage_->open(mode))
                return false;

            core::tools::auto_scope lb([this] { storage_->close(); });

            core::tools::binary_stream stream;
            while (storage_->read_data_block(-1, stream))
            {
                while (stream.available())
                {
                    core::tools::tlvpack block;
                    if (!block.unserialize(stream))
                        return false;

                    for (auto iter = block.get_first(); iter; iter = block.get_next())
                    {
                        call_info info;
                        auto s = iter->get_value<core::tools::binary_stream>();
                        if (info.unserialize(s) != 0)
                            return false;

                        calls_.push_back(info);
                    }
                }

                stream.reset();
            }

            return storage_->get_last_error() == archive::error::end_of_file;
        }

        void call_log_cache::merge(const call_log_cache& _other)
        {
            calls_ = _other.calls_;

            save_cache();
        }

        bool call_log_cache::merge(const call_info& _call)
        {
            auto iter = std::find_if(calls_.begin(), calls_.end(), [_call](const auto& call) { return call.message_.get_msgid() == _call.message_.get_msgid() && call.aimid_ == _call.aimid_; });
            bool found = iter != calls_.end();
            if (found)
                *iter = _call;
            else
                calls_.push_back(_call);

            save_cache();
            return !found;
        }

        bool call_log_cache::remove_by_id(const std::string& _contact, int64_t _msg_id)
        {
            auto prevSize = calls_.size();

            calls_.erase(std::remove_if(calls_.begin(), calls_.end(), [_contact, _msg_id](const auto& call) { return call.message_.get_msgid() == _msg_id && _contact == call.aimid_; }), calls_.end());

            save_cache();

            return prevSize != calls_.size();
        }

        bool call_log_cache::remove_till(const std::string& _contact, int64_t _del_up_to)
        {
            auto prevSize = calls_.size();

            calls_.erase(std::remove_if(calls_.begin(), calls_.end(), [_contact, _del_up_to](const auto& call) { return call.message_.get_msgid() <= _del_up_to && _contact == call.aimid_; }), calls_.end());

            save_cache();

            return prevSize != calls_.size();
        }

        bool call_log_cache::remove_contact(const std::string& _contact)
        {
            auto prevSize = calls_.size();

            calls_.erase(std::remove_if(calls_.begin(), calls_.end(), [_contact](const auto& call) { return _contact == call.aimid_; }), calls_.end());

            save_cache();

            return prevSize != calls_.size();
        }
    }
}