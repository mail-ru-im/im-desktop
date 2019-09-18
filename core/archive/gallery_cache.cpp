#include "stdafx.h"

#include "gallery_cache.h"
#include "storage.h"
#include "../network_log.h"
#include "../core.h"
#include "../../corelib/collection_helper.h"

namespace
{
    enum tlv_fields_cache : uint32_t
    {
        tlv_item_pack = 1,

        tlv_msg_id = 2,
        tlv_seq = 3,
        tlv_next_msg_id = 4,
        tlv_next_seq = 5,
        tlv_url = 6,
        tlv_type = 7,
        tlv_sender = 8,
        tlv_outgoing = 9,
        tlv_time = 10,
        tlv_caption = 11,
    };

    enum tlv_fields_state : uint32_t
    {
        tlv_patch_version = 1,
        tlv_last_entry_msg = 2,
        tlv_last_entry_seq = 3,
        tlv_first_entry_msg = 4,
        tlv_first_entry_seq = 5,
        tlv_images = 6,
        tlv_videos = 7,
        tlv_files = 8,
        tlv_links = 9,
        tlv_ptt = 10,
        tlv_audio = 11,
        tlv_patch_version_changed = 12,
    };

    const int32_t max_block_size = 1000;
    const int32_t max_items_size = 20000;

    const auto consistency_valid_value = 0.95;
    const auto consistency_check_period = std::chrono::hours(24);
}

using namespace core::archive;

gallery_entry_id::gallery_entry_id(int64_t _msg_id, int64_t _seq)
    : msg_id_(_msg_id)
    , seq_(_seq)
{
}

bool gallery_entry_id::empty() const
{
    return msg_id_ == 0 && seq_ == 0;
}

bool gallery_entry_id::valid() const
{
    return msg_id_ != -1 && seq_ != -1;
}

bool gallery_entry_id::operator<(const gallery_entry_id& _other) const
{
    if (msg_id_ == _other.msg_id_)
        return seq_ < _other.seq_;

    return msg_id_ < _other.msg_id_;
}

bool gallery_entry_id::operator>(const gallery_entry_id& _other) const
{
    if (msg_id_ == _other.msg_id_)
        return seq_ > _other.seq_;

    return msg_id_ > _other.msg_id_;
}

bool gallery_entry_id::operator==(const gallery_entry_id& _other) const
{
    return msg_id_ == _other.msg_id_ && seq_ == _other.seq_;
}

bool gallery_entry_id::operator!=(const gallery_entry_id& _other) const
{
    return msg_id_ != _other.msg_id_ || seq_ != _other.seq_;
}

bool gallery_item::operator<(const gallery_item& _other) const
{
    return id_ < _other.id_;
}

bool gallery_item::operator==(const gallery_item& _other) const
{
    return id_ == _other.id_;
}

void gallery_item::serialize(core::tools::binary_stream& _data) const
{
    core::tools::tlvpack pack;

    pack.push_child(core::tools::tlv(tlv_fields_cache::tlv_msg_id, (int64_t)id_.msg_id_));
    pack.push_child(core::tools::tlv(tlv_fields_cache::tlv_seq, (int64_t)id_.seq_));
    pack.push_child(core::tools::tlv(tlv_fields_cache::tlv_next_msg_id, (int64_t)next_.msg_id_));
    pack.push_child(core::tools::tlv(tlv_fields_cache::tlv_next_seq, (int64_t)next_.seq_));
    pack.push_child(core::tools::tlv(tlv_fields_cache::tlv_url, (std::string) url_));
    pack.push_child(core::tools::tlv(tlv_fields_cache::tlv_type, (std::string) type_));
    pack.push_child(core::tools::tlv(tlv_fields_cache::tlv_sender, (std::string) sender_));
    pack.push_child(core::tools::tlv(tlv_fields_cache::tlv_outgoing, (bool) outgoing_));
    pack.push_child(core::tools::tlv(tlv_fields_cache::tlv_time, (int32_t) time_));
    pack.push_child(core::tools::tlv(tlv_fields_cache::tlv_caption, (std::string)caption_));

    pack.serialize(_data);
}

void gallery_item::serialize(core::icollection* _collection) const
{
    coll_helper coll(_collection, false);

    coll.set_value_as_int64("msg_id", id_.msg_id_);
    coll.set_value_as_int64("seq", id_.seq_);
    coll.set_value_as_string("url", url_);
    coll.set_value_as_string("caption", caption_);
    coll.set_value_as_string("type", type_);
    coll.set_value_as_string("sender", sender_);
    coll.set_value_as_bool("outgoing", outgoing_);
    coll.set_value_as_int("time", time_);
    coll.set_value_as_string("action", action_ == "edit" ? "del" : action_);//gui doesn't know about edit; replaceed by pair del+add
}

bool gallery_item::unserialize(core::tools::binary_stream& _data)
{
    core::tools::tlvpack pack;

    if (!pack.unserialize(_data))
        return false;

    for (auto tlv_field = pack.get_first(); tlv_field; tlv_field = pack.get_next())
    {
        switch (static_cast<tlv_fields_cache>(tlv_field->get_type()))
        {
        case tlv_fields_cache::tlv_msg_id:
            id_.msg_id_ = tlv_field->get_value<int64_t>();
            break;
        case tlv_fields_cache::tlv_seq:
            id_.seq_ = tlv_field->get_value<int64_t>();
            break;
        case tlv_fields_cache::tlv_next_msg_id:
            next_.msg_id_ = tlv_field->get_value<int64_t>();
            break;
        case tlv_fields_cache::tlv_next_seq:
            next_.seq_ = tlv_field->get_value<int64_t>();
            break;
        case tlv_fields_cache::tlv_url:
            url_ = tlv_field->get_value<std::string>();
            break;
        case tlv_fields_cache::tlv_type:
            type_ = tlv_field->get_value<std::string>();
            break;
        case tlv_fields_cache::tlv_sender:
            sender_ = tlv_field->get_value<std::string>();
            break;
        case tlv_fields_cache::tlv_outgoing:
            outgoing_ = tlv_field->get_value<bool>();
            break;
        case tlv_fields_cache::tlv_time:
            time_ = tlv_field->get_value<int32_t>();
            break;
        case tlv_fields_cache::tlv_caption:
            caption_ = tlv_field->get_value<std::string>();
            break;

        default:
            assert(!"invalid field");
        }
    }

    return true;
}


int64_t gallery_item::get_memory_usage() const
{
    return (sizeof(id_) + sizeof(next_) + url_.length() + type_.length() + sender_.length() + sizeof(outgoing_) + sizeof(time_) + action_.length());
}

void gallery_state::serialize(core::icollection* _collection, const std::string& _aimid) const
{
    coll_helper coll(_collection, false);

    coll.set_value_as_string("aimid", _aimid);
    coll.set_value_as_int("images", images_count_);
    coll.set_value_as_int("videos", videos_count_);
    coll.set_value_as_int("files", files_count_);
    coll.set_value_as_int("links", links_count_);
    coll.set_value_as_int("ptt", ptt_count_);
    coll.set_value_as_int("audio", audio_count_);
}

bool gallery_state::operator==(const gallery_state& _other) const
{
    return patch_version_ == _other.patch_version_ &&
        last_entry_ == _other.last_entry_ &&
        images_count_ == _other.images_count_ &&
        videos_count_ == _other.videos_count_ &&
        files_count_ == _other.files_count_ &&
        links_count_ == _other.links_count_ &&
        ptt_count_ == _other.ptt_count_ &&
        audio_count_ == _other.audio_count_;
}

bool gallery_state::operator!=(const gallery_state& _other) const
{
    return !(*this == _other);
}

gallery_storage::gallery_storage(const std::wstring& _cache_file_name, const std::wstring& _state_file_name)
    : first_entry_reached_(false)
    , loaded_from_local_(false)
    , delUpTo_(-1)
    , cache_storage_(std::make_unique<storage>(_cache_file_name))
    , state_storage_(std::make_unique<storage>(_state_file_name))
    , hole_requested_(false)
    , first_load_(false)
{
    last_consistency_check_time_ = std::chrono::system_clock::now() - consistency_check_period;
}

gallery_storage::gallery_storage(const std::string& _aimid)
    : aimid_(_aimid)
    , first_entry_reached_(false)
    , loaded_from_local_(false)
    , delUpTo_(-1)
    , hole_requested_(false)
    , first_load_(false)
{
    last_consistency_check_time_ = std::chrono::system_clock::now() - consistency_check_period;
}

gallery_storage::gallery_storage()
    : gallery_storage(std::string())
{
    last_consistency_check_time_ = std::chrono::system_clock::now() - consistency_check_period;
}

gallery_storage::gallery_storage(const gallery_storage& _other)
    : aimid_(_other.aimid_)
    , state_(_other.state_)
    , items_(_other.items_)
    , patches_(_other.patches_)
    , older_entry_id_(_other.older_entry_id_)
    , first_entry_reached_(_other.first_entry_reached_)
    , loaded_from_local_(false)
    , delUpTo_(_other.delUpTo_)
    , hole_requested_(false)
    , first_load_(_other.first_load_)
    , last_consistency_check_time_(_other.last_consistency_check_time_)
{

}

gallery_storage::~gallery_storage()
{

}

void gallery_storage::load_from_local()
{
    if (loaded_from_local_)
        return;

    load_state_from_local();
    load_cache_from_local();

    first_load_ = true;
    loaded_from_local_ = true;
}

void gallery_storage::free()
{
    aimid_ = std::string();
    state_ = gallery_state();
    items_.clear();
    patches_.clear();
    older_entry_id_ = gallery_entry_id();
    first_entry_reached_ = false;
    loaded_from_local_ = false;
    first_load_ = false;
}

void gallery_storage::set_aimid(const std::string& _aimid)
{
    aimid_ = _aimid;
}

void gallery_storage::set_my_aimid(const std::string& _aimid)
{
    my_aimid_ = _aimid;
}

void gallery_storage::set_first_entry_reached(bool _reached)
{
    first_entry_reached_ = _reached;
}

std::string gallery_storage::get_aimid() const
{
    return aimid_;
}

gallery_state gallery_storage::get_gallery_state() const
{
    return state_;
}

void gallery_storage::merge_from_server(const gallery_storage& _other, const gallery_entry_id& _from, const gallery_entry_id& _till, std::vector<gallery_item>& _changes)
{
    if (_other.delUpTo_ != -1)
    {
        auto iter = items_.begin();
        while (iter != items_.end())
        {
            if (iter->id_.msg_id_ > _other.delUpTo_)
                break;

            iter = items_.erase(iter);
        }

        if (items_.empty())
            state_.first_entry_ = gallery_entry_id();
        else if (state_.first_entry_.valid() && !state_.first_entry_.empty())
            state_.first_entry_ = items_.front().id_;

        save_state();
    }

    auto contains = [this](const auto& _item_id)
    {
        return std::any_of(items_.begin(), items_.end(), [_item_id](const auto& iter) { return iter.id_ == _item_id; });
    };

    auto hole = false;
    if (_other.older_entry_id_.empty())
    {
        if (_till.empty())
            hole = false;
        else
            hole = !contains(_till);
    }
    else
    {
        hole = !contains(_other.older_entry_id_);
    }

    auto max_count_reached = false;
    for (const auto& i : _other.items_)
    {
        if (std::find(items_.begin(), items_.end(), i) != items_.end())
            continue;

        auto ch = i;
        ch.action_ = "add";
        _changes.push_back(ch);

        if (items_.empty())
        {
            items_.push_back(i);
            continue;
        }

        auto first = items_.front();
        if (i < first)
        {
            items_.push_front(i);
            items_.front().next_ = first.id_;
            continue;
        }

        auto prev = items_.end();
        auto iter = items_.begin();
        for (;iter != items_.end(); ++iter)
        {
            if (*(iter) < i)
            {
                prev = iter;
                continue;
            }

            break;
        }

        auto next = iter == items_.end() ? gallery_entry_id() : iter->id_;
        if (prev != items_.end() && !hole)
            prev->next_ = i.id_;

        auto inserted = items_.insert(iter, i);
        inserted->next_ = next;

        while (items_.size() > max_items_size)
        {
            items_.pop_front();
            max_count_reached = true;
        }
    }

    if (_other.first_entry_reached_ || max_count_reached)
    {
        state_.first_entry_ = items_.empty() ? gallery_entry_id() : items_.front().id_;
        if (items_.empty())
            state_.last_entry_ = gallery_entry_id();

        save_state();
    }
    else if (!state_.first_entry_.valid())
    {
        //mark gallery requested
        state_.first_entry_ = gallery_entry_id();
        state_.patch_version_ = _other.state_.patch_version_;
        save_state();
    }

    if (!_other.patches_.empty())
    {
        for (const auto& p : _other.patches_)
        {
        auto iter = items_.begin();
        while (iter != items_.end())
        {
            if (iter->id_.msg_id_ == p.msg_id_)
            {
                iter = items_.erase(iter);
                if (!items_.empty() && iter != items_.begin() && p.type_ == "del")
                {
                    auto prev = std::prev(iter);
                    prev->next_ = (iter == items_.end()) ? gallery_entry_id() : iter->id_;
                }
            }
            else
            {
                ++iter;
            }
        }

        auto ch = gallery_item();
        ch.id_.msg_id_ = p.msg_id_;
        ch.action_ = p.type_;
        _changes.push_back(ch);
        }
    }

    auto erase_item = [this](auto _id, auto pred)
    {
        auto state_changed = false;
        auto found = std::find_if(items_.begin(), items_.end(), pred);
        if (found == items_.end())
            return;

        auto new_item = items_.erase(found);
        if (!items_.empty())
        {
            if (new_item != items_.begin())
            {
                auto prev = std::prev(new_item);
                prev->next_ = (new_item == items_.end()) ? gallery_entry_id() : new_item->id_;
                if (state_.last_entry_ == _id)
                {
                    state_.last_entry_ = gallery_entry_id();
                    state_changed = true;
                }
            }
            else
            {
                state_.first_entry_ = items_.front().id_;
                state_changed = true;
            }
        }
        else
        {
            state_.last_entry_ = gallery_entry_id();
            state_.first_entry_ = gallery_entry_id(-1, -1);
            state_changed = true;
        }

        if (state_changed)
            save_state();
    };

    auto find_from = [_from](auto iter) { return iter.id_ == _from; };
    auto contains_from = (std::find_if(_other.items_.begin(), _other.items_.end(), find_from) != _other.items_.end());
    if (!_from.empty() && !contains_from)
        erase_item(_from, find_from);

    auto find_till = [_till](auto iter) { return iter.id_ == _till; };
    auto contains_till = (std::find_if(_other.items_.begin(), _other.items_.end(), find_till) != _other.items_.end());
    if (_other.items_.size() < 1000 && !_till.empty() && !contains_till)
        erase_item(_till, find_till);

    if (_other.items_.size() < 1000 && !_from.empty() && !_till.empty())
    {
        auto reached = false;
        auto iter = items_.begin();
        while (iter != items_.end())
        {
            if (iter->id_ > _from)
            {
                if (reached)
                    break;

                reached = true;
            }

            if (iter->id_ < _till)
            {
                ++iter;
                continue;
            }

            if (iter != items_.begin())
            {
                auto prev = std::prev(iter);
                prev->next_ = iter->id_;
            }

            ++iter;
        }
    }

    save_cache();

    if (state_.patch_version_ != _other.state_.patch_version_)
    {
        state_.patch_version_changed = true;
        save_state();
    }
}

void gallery_storage::set_state(const gallery_state& _state, bool _store_patch_version)
{
    auto first = state_.first_entry_;
    auto local_path_version = state_.patch_version_;
    state_ = _state;
    state_.first_entry_ = first;
    if (!_store_patch_version && state_.first_entry_.valid())
    {
        state_.patch_version_ = local_path_version;
        state_.patch_version_changed = (local_path_version != _state.patch_version_);
    }
    else
    {
        state_.patch_version_changed = false;
    }

    save_state();
}

int32_t gallery_storage::unserialize(const rapidjson::Value& _node, bool _parse_for_patches)
{
    auto delUpTo = _node.FindMember("delUpto");
    if (delUpTo != _node.MemberEnd() && delUpTo->value.IsInt64())
        delUpTo_ = delUpTo->value.GetInt64();

    auto node_state = _node.FindMember("galleryState");
    if (node_state == _node.MemberEnd() || !node_state->value.IsObject())
        return 1;

    auto path_version = node_state->value.FindMember("galleryPatchVersion");
    if (path_version != node_state->value.MemberEnd() && path_version->value.IsString())
        state_.patch_version_ = path_version->value.GetString();

    auto last_entry = node_state->value.FindMember("lastEntryId");
    if (last_entry != node_state->value.MemberEnd() && last_entry->value.IsObject())
    {
        auto id = last_entry->value.FindMember("mid");
        if (id != last_entry->value.MemberEnd() && id->value.IsInt64())
            state_.last_entry_.msg_id_ = id->value.GetInt64();

        auto seq = last_entry->value.FindMember("seq");
        if (seq != last_entry->value.MemberEnd() && seq->value.IsInt())
            state_.last_entry_.seq_ = seq->value.GetInt();
    }

    auto items_count = node_state->value.FindMember("itemsCount");
    if (items_count != node_state->value.MemberEnd() && items_count->value.IsObject())
    {
        auto images_count = items_count->value.FindMember("image");
        if (images_count != items_count->value.MemberEnd() && images_count->value.IsInt())
            state_.images_count_ = images_count->value.GetInt();

        auto videos_count = items_count->value.FindMember("video");
        if (videos_count != items_count->value.MemberEnd() && videos_count->value.IsInt())
            state_.videos_count_ = videos_count->value.GetInt();

        auto files_count = items_count->value.FindMember("file");
        if (files_count != items_count->value.MemberEnd() && files_count->value.IsInt())
            state_.files_count_ = files_count->value.GetInt();

        auto links_count = items_count->value.FindMember("link");
        if (links_count != items_count->value.MemberEnd() && links_count->value.IsInt())
            state_.links_count_ = links_count->value.GetInt();

        auto ptt_count = items_count->value.FindMember("ptt");
        if (ptt_count != items_count->value.MemberEnd() && ptt_count->value.IsInt())
            state_.ptt_count_ = ptt_count->value.GetInt();

        auto audio_count = items_count->value.FindMember("audio");
        if (audio_count != items_count->value.MemberEnd() && audio_count->value.IsInt())
            state_.audio_count_ = audio_count->value.GetInt();
    }

    auto unserialize_entries = [this](const auto& _node)
    {
        auto entries = _node->FindMember("entries");
        if (entries != _node->MemberEnd() && entries->value.IsArray())
        {
            for (auto iter = entries->value.Begin(); iter != entries->value.End(); ++iter)
            {
                gallery_entry_id item_id;
                {
                    auto entry_id = iter->FindMember("id");
                    auto id = entry_id->value.FindMember("mid");
                    if (id != entry_id->value.MemberEnd() && id->value.IsInt64())
                        item_id.msg_id_ = id->value.GetInt64();

                    auto seq = entry_id->value.FindMember("seq");
                    if (seq != entry_id->value.MemberEnd() && seq->value.IsInt())
                        item_id.seq_ = seq->value.GetInt();
                }
                gallery_item item;
                {
                    item.id_ = item_id;

                    auto caption = iter->FindMember("caption");
                    if (caption != iter->MemberEnd() && caption->value.IsString())
                        item.caption_ = caption->value.GetString();

                    auto type = iter->FindMember("type");
                    if (type != iter->MemberEnd() && type->value.IsString())
                        item.type_ = type->value.GetString();

                    auto url = iter->FindMember("url");
                    if (url != iter->MemberEnd() && url->value.IsString())
                        item.url_ = url->value.GetString();

                    auto sender = iter->FindMember("sender");
                    if (sender != iter->MemberEnd() && sender->value.IsString())
                        item.sender_ = sender->value.GetString();

                    auto outgoing = iter->FindMember("outgoing");
                    if (outgoing != iter->MemberEnd() && outgoing->value.IsBool())
                        item.outgoing_ = outgoing->value.GetBool();

                    item.outgoing_ |= (my_aimid_ == item.sender_);

                    auto time = iter->FindMember("time");
                    if (time != iter->MemberEnd() && time->value.IsInt())
                        item.time_ = time->value.GetInt();
                }

                items_.push_back(item);
            }
        }
    };


    auto subreqs = _node.FindMember("subreqs");
    if (subreqs != _node.MemberEnd() && subreqs->value.IsArray() && subreqs->value.Size() != 0)
    {
        auto includeStart = subreqs->value.Begin()->FindMember("includeStart");
        if (includeStart != subreqs->value.Begin()->MemberEnd() && includeStart->value.IsBool())
            first_entry_reached_ = includeStart->value.GetBool();

        unserialize_entries(subreqs->value.Begin());
    }

    auto tail = _node.FindMember("tail");
    if (tail != _node.MemberEnd() && tail->value.IsObject())
    {
        unserialize_entries(&(tail->value));
        auto older_entry_id = tail->value.FindMember("olderEntryId");
        if (older_entry_id != tail->value.MemberEnd() && older_entry_id->value.IsObject())
        {
            auto id = older_entry_id->value.FindMember("mid");
            if (id != older_entry_id->value.MemberEnd() && id->value.IsInt64())
                older_entry_id_.msg_id_ = id->value.GetInt64();

            auto seq = older_entry_id->value.FindMember("seq");
            if (seq != older_entry_id->value.MemberEnd() && seq->value.IsInt())
                older_entry_id_.seq_ = seq->value.GetInt();
        }
    }

    if (_parse_for_patches)
    {
        auto patch = _node.FindMember("patch");
        if (patch != _node.MemberEnd() && patch->value.IsArray() && patch->value.Size() != 0)
        {
            for (auto iter = patch->value.Begin(); iter != patch->value.End(); ++iter)
            {
                gallery_patch p;
                auto action = iter->FindMember("action");
                if (action != iter->MemberEnd() && action->value.IsObject())
                {
                    auto type = action->value.FindMember("type");
                    if (type != action->value.MemberEnd() && type->value.IsString())
                        p.type_ = type->value.GetString();

                    auto msgid = action->value.FindMember("msgId");
                    if (msgid != action->value.MemberEnd() && msgid->value.IsInt64())
                        p.msg_id_ = msgid->value.GetInt64();
                }

                patches_.push_back(p);
            }
        }
    }

    return 0;
}

std::vector<gallery_patch> gallery_storage::get_patch() const
{
    return patches_;
}

gallery_items_block gallery_storage::get_items() const
{
    return items_;
}

std::vector<gallery_item> gallery_storage::get_items(const gallery_entry_id& _from, const std::vector<std::string>& _types, int _page_size, bool& _exhausted)
{
    std::vector<archive::gallery_item> result;
    if (items_.empty())
    {
        _exhausted = true;
        return result;
    }

    auto count = -1;
    if (_from.empty())
    {
        auto iter = items_.crbegin();
        for (; iter != items_.crend(); ++iter)
        {
            if (count == _page_size)
            {
                break;
            }

            if (_from.empty() || iter->id_ < _from)
            {
                if (std::find(_types.begin(), _types.end(), iter->type_) != _types.end())
                {
                    result.push_back(*iter);
                    ++count;
                }
            }
        }

        if (iter == items_.crend())
            _exhausted = !state_.first_entry_.empty();
    }
    else
    {
        auto iter = std::find_if(items_.begin(), items_.end(), [_from](const auto& i) { return i.id_ == _from; });
        if (iter == items_.end())
        {
            _exhausted = true;
            return result;
        }

        if (_page_size > 0 && iter == items_.begin())
        {
            _exhausted = !state_.first_entry_.empty();
            return result;
        }

        if (_page_size > 0)
            --iter;
        else
            ++iter;

        while (iter != items_.end())
        {
            if (count == abs(_page_size))
                break;

            if (std::find(_types.begin(), _types.end(), iter->type_) != _types.end())
            {
                result.push_back(*iter);
                ++count;
            }

            if (_page_size > 0)
            {
                if (iter == items_.begin())
                    break;

                --iter;
            }
            else
            {
                ++iter;
            }
        }

        if (iter == items_.end())
            _exhausted = !state_.first_entry_.empty();
    }

    if (count == abs(_page_size))
        result.pop_back();

    return result;
}

std::vector<gallery_item> gallery_storage::get_items(int64_t _msg_id, const std::vector<std::string>& _types, int& _index, int& _total)
{
    std::vector<archive::gallery_item> result;
    if (items_.empty())
        return result;

    _index = 0;
    _total = 0;
    for (auto iter = items_.begin(); iter != items_.end(); ++iter)
    {
        if (std::find(_types.begin(), _types.end(), iter->type_) == _types.end())
            continue;

        if (iter->id_.msg_id_ ==_msg_id)
        {
            result.push_back(*iter);
        }
        else if (iter->id_.msg_id_ < _msg_id)
        {
            ++_index;
        }
        ++_total;
    }

    return result;
}

bool gallery_storage::get_next_hole(gallery_entry_id& _from, gallery_entry_id& _till)
{
    _from = gallery_entry_id();
    _till = gallery_entry_id();

    if (hole_requested_)
        return false;

    if (first_load_)
    {
        if (!state_.last_entry_.empty())
            _till = state_.last_entry_;
        hole_requested_ = true;
        first_load_ = false;
        return true;
    }

    if (items_.empty())
    {
        if (!state_.first_entry_.valid())
        {
            hole_requested_ = true;
            return true;
        }

        if (!state_.last_entry_.empty())
        {
            _from = state_.last_entry_;
            return true;
        }

        return false;
    }

    if (state_.first_entry_.empty() || state_.first_entry_ != items_.front().id_)
    {
        _from = items_.front().id_;
        hole_requested_ = true;
        return true;
    }

    auto lastId = items_.back().id_;
    if (lastId != state_.last_entry_ && lastId < state_.last_entry_)
    {
        _from = state_.last_entry_;
        _till = lastId;
        hole_requested_ = true;
        return true;
    }

    auto iter = items_.begin();
    auto prevId = iter->id_;
    auto prevNext = iter->next_;
    ++iter;
    while (iter != items_.end())
    {
        if (prevNext != iter->id_)
        {
            _from = iter->id_;
            _till = prevId;
            hole_requested_ = true;
            return true;
        }

        prevId = iter->id_;
        prevNext = iter->next_;
        ++iter;
    }

    if (!check_consistency())
    {
        assert(!"gallery is inconsistent");
        clear_gallery();
        hole_requested_ = true;
        return true;
    }

    return false;
}

bool gallery_storage::erased() const
{
    return delUpTo_ != -1;
}

void gallery_storage::clear_hole_request()
{
    hole_requested_ = false;
}

void gallery_storage::make_hole(int64_t _from, int64_t _till)
{
    if (_from == -1)
    {
        items_.clear();
    }
    else
    {
        auto find_till = (_till == -1);
        auto iter = items_.begin();

        while (iter != items_.end())
        {
            if (iter->id_.msg_id_ == _from)
                break;

            if (iter->id_.msg_id_ == _till && !find_till)
            {
                find_till = true;
                iter->next_ = gallery_entry_id();
                ++iter;
                continue;
            }

            if (find_till)
                iter = items_.erase(iter);
            else
                ++iter;
        }

    }
    save_cache();
}

int64_t gallery_storage::get_memory_usage() const
{
    int64_t memory_usage = 0;

    for (const auto& _item : items_)
        memory_usage += _item.get_memory_usage();

    return memory_usage;
}

bool gallery_storage::load_cache_from_local()
{
    if (!cache_storage_)
    {
        assert(!"cache storage");
        return false;
    }

    archive::storage_mode mode;
    mode.flags_.read_ = true;

    if (!cache_storage_->open(mode))
        return false;

    core::tools::auto_scope lb([this] { cache_storage_->close(); });

    core::tools::binary_stream stream;
    while (cache_storage_->read_data_block(-1, stream))
    {
        while (stream.available())
        {
            core::tools::tlvpack block;
            if (!block.unserialize(stream))
                return false;

            for (auto iter = block.get_first(); iter; iter = block.get_next())
            {
                gallery_item item;
                auto s = iter->get_value<core::tools::binary_stream>();
                if (!item.unserialize(s))
                    return false;

                items_.push_back(item);
            }
        }

        stream.reset();
    }

    return cache_storage_->get_last_error() == archive::error::end_of_file;
}

bool gallery_storage::load_state_from_local()
{
    if (!state_storage_)
    {
        assert(!"state storage");
        return false;
    }

    archive::storage_mode mode;
    mode.flags_.read_ = true;

    if (!state_storage_->open(mode))
        return false;

    core::tools::auto_scope lb([this] { state_storage_->close(); });

    core::tools::binary_stream stream;
    while (state_storage_->read_data_block(-1, stream))
    {
        while (stream.available())
        {
            core::tools::tlvpack pack;
            if (!pack.unserialize(stream))
                return false;

            for (auto tlv_field = pack.get_first(); tlv_field; tlv_field = pack.get_next())
            {
                switch (static_cast<tlv_fields_state>(tlv_field->get_type()))
                {
                case tlv_fields_state::tlv_patch_version:
                    state_.patch_version_ = tlv_field->get_value<std::string>();
                    break;

                case tlv_fields_state::tlv_last_entry_msg:
                    state_.last_entry_.msg_id_ = tlv_field->get_value<int64_t>();
                    break;

                case tlv_fields_state::tlv_last_entry_seq:
                    state_.last_entry_.seq_ = tlv_field->get_value<int64_t>();
                    break;

                case tlv_fields_state::tlv_first_entry_msg:
                    state_.first_entry_.msg_id_ = tlv_field->get_value<int64_t>();
                    break;

                case tlv_fields_state::tlv_first_entry_seq:
                    state_.first_entry_.seq_ = tlv_field->get_value<int64_t>();
                    break;

                case tlv_fields_state::tlv_images:
                    state_.images_count_ = tlv_field->get_value<int>();
                    break;

                case tlv_fields_state::tlv_videos:
                    state_.videos_count_ = tlv_field->get_value<int>();
                    break;

                case tlv_fields_state::tlv_files:
                    state_.files_count_ = tlv_field->get_value<int>();
                    break;

                case tlv_fields_state::tlv_links:
                    state_.links_count_ = tlv_field->get_value<int>();
                    break;

                case tlv_fields_state::tlv_ptt:
                    state_.ptt_count_ = tlv_field->get_value<int>();
                    break;

                case tlv_fields_state::tlv_audio:
                    state_.audio_count_ = tlv_field->get_value<int>();
                    break;

                case tlv_fields_state::tlv_patch_version_changed:
                    state_.patch_version_changed = tlv_field->get_value<bool>();
                    break;

                default:
                    assert(!"invalid field");
                }
            }
        }

        stream.reset();
    }

    return state_storage_->get_last_error() == archive::error::end_of_file;
}

bool gallery_storage::save_cache()
{
    archive::storage_mode mode;
    mode.flags_.write_ = true;
    mode.flags_.truncate_ = true;

    if (!cache_storage_->open(mode))
        return false;

    core::tools::auto_scope lb([this] { cache_storage_->close(); });

    auto serialize = [this](const std::vector<gallery_item>& _items)
    {
        core::tools::tlvpack block;

        for (const auto& i : _items)
        {
            core::tools::binary_stream data;
            i.serialize(data);
            block.push_child(core::tools::tlv(tlv_fields_cache::tlv_item_pack, data));
        }

        core::tools::binary_stream stream;
        block.serialize(stream);

        int64_t offset = 0;
        return cache_storage_->write_data_block(stream, offset);
    };

    std::vector<gallery_item> items;
    for (const auto& item : items_)
    {
        items.push_back(item);
        if (items.size() >= max_block_size)
        {
            if (!serialize(items))
                return false;

            items.clear();
        }
    }

    if (!items.empty())
    {
        return serialize(items);
    }

    return true;
}

bool gallery_storage::save_state()
{
    archive::storage_mode mode;
    mode.flags_.write_ = true;
    mode.flags_.truncate_ = true;

    if (!state_storage_->open(mode))
        return false;

    core::tools::auto_scope lb([this] { state_storage_->close(); });

    core::tools::tlvpack pack;

    pack.push_child(core::tools::tlv(tlv_fields_state::tlv_patch_version, (std::string)state_.patch_version_));
    pack.push_child(core::tools::tlv(tlv_fields_state::tlv_last_entry_msg, (int64_t)state_.last_entry_.msg_id_));
    pack.push_child(core::tools::tlv(tlv_fields_state::tlv_last_entry_seq, (int64_t)state_.last_entry_.seq_));
    pack.push_child(core::tools::tlv(tlv_fields_state::tlv_first_entry_msg, (int64_t)state_.first_entry_.msg_id_));
    pack.push_child(core::tools::tlv(tlv_fields_state::tlv_first_entry_seq, (int64_t)state_.first_entry_.seq_));
    pack.push_child(core::tools::tlv(tlv_fields_state::tlv_images, (int)state_.images_count_));
    pack.push_child(core::tools::tlv(tlv_fields_state::tlv_videos, (int)state_.videos_count_));
    pack.push_child(core::tools::tlv(tlv_fields_state::tlv_files, (int)state_.files_count_));
    pack.push_child(core::tools::tlv(tlv_fields_state::tlv_links, (int)state_.links_count_));
    pack.push_child(core::tools::tlv(tlv_fields_state::tlv_ptt, (int)state_.ptt_count_));
    pack.push_child(core::tools::tlv(tlv_fields_state::tlv_audio, (int)state_.audio_count_));
    pack.push_child(core::tools::tlv(tlv_fields_state::tlv_patch_version_changed, (bool)state_.patch_version_changed));

    core::tools::binary_stream stream;
    pack.serialize(stream);

    int64_t offset = 0;
    return state_storage_->write_data_block(stream, offset);
}

bool gallery_storage::check_consistency()
{
    if (!loaded_from_local_ || (std::chrono::system_clock::now() - last_consistency_check_time_) < consistency_check_period)
        return true;

    auto count_from_state = state_.audio_count_ + state_.files_count_ + state_.images_count_ + state_.links_count_ + state_.videos_count_ + state_.ptt_count_;
    auto consistency_value = 0.0;
    if (items_.size() <= (size_t)count_from_state)
        consistency_value = (double)items_.size() / count_from_state;
    else
        consistency_value = (double)count_from_state / items_.size();

    last_consistency_check_time_ = std::chrono::system_clock::now();

    if (consistency_value != 1.0)
    {
        tools::binary_stream bs;
        bs.write<std::string_view>("gallery inconsistency has been detected\r\n");
        std::stringstream s;
        s << "items in cache: " << items_.size() << "; items in state: " << count_from_state << "\r\n";
        bs.write(s.str());
        if (consistency_value < consistency_value)
            bs.write<std::string_view>("gallery is going to be requested again\r\n");

        g_core->write_data_to_network_log(std::move(bs));
    }

    return consistency_value >= consistency_valid_value;
}

void gallery_storage::clear_gallery()
{
    items_.clear();
    state_ = gallery_state();

    save_cache();
    save_state();
}