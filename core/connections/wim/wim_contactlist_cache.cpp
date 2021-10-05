#include "stdafx.h"

#include "wim_contactlist_cache.h"
#include "wim_im.h"
#include "../../core.h"

#include "../../../corelib/core_face.h"
#include "../../../corelib/collection_helper.h"
#include "../common.shared/string_utils.h"
#include "../../log/log.h"
#include "../../../common.shared/json_helper.h"
#include "../../tools/system.h"
#include "../libomicron/include/omicron/omicron.h"

using namespace core;
using namespace wim;

void cl_presence::serialize(icollection* _coll)
{
    coll_helper cl(_coll, false);
    cl.set_value_as_string("userType", usertype_);
    cl.set_value_as_string("statusMsg", status_msg_);
    cl.set_value_as_string("otherNumber", other_number_);
    cl.set_value_as_string("smsNumber", sms_number_);
    cl.set_value_as_string("friendly", friendly_);
    cl.set_value_as_string("nick", nick_);
    cl.set_value_as_string("abContactName", ab_contact_name_);
    cl.set_value_as_bool("is_chat", is_chat_);
    cl.set_value_as_bool("mute", muted_);
    cl.set_value_as_bool("official", official_);
    cl.set_value_as_int("outgoingCount", outgoing_msg_count_);
    cl.set_value_as_bool("livechat", is_live_chat_);
    cl.set_value_as_string("iconId", icon_id_);
    cl.set_value_as_string("bigIconId", big_icon_id_);
    cl.set_value_as_string("largeIconId", large_icon_id_);
    cl.set_value_as_bool("public", public_);
    cl.set_value_as_bool("channel", is_channel_);
    cl.set_value_as_bool("autoAddition", auto_added_);
    cl.set_value_as_bool("deleted", deleted_);
    lastseen_.serialize(cl);

    if (!status_.is_empty())
        status_.serialize(cl);
}

void cl_presence::serialize(rapidjson::Value& _node, rapidjson_allocator& _a)
{
    const auto save_bool = [&_node, &_a](std::string_view _name, const bool _value)
    {
        if (_value)
            _node.AddMember(tools::make_string_ref(_name), _value, _a);
    };
    const auto save_string = [&_node, &_a](std::string_view _name, const auto& _value)
    {
        if (!_value.empty())
            _node.AddMember(tools::make_string_ref(_name), _value, _a);
    };

    save_string("userType",  usertype_);
    save_string("friendly",  friendly_);
    save_string("iconId",  icon_id_);
    save_string("bigIconId",  big_icon_id_);
    save_string("largeIconId",  large_icon_id_);
    save_string("statusMsg",  status_msg_);
    save_string("nick", nick_);
    save_bool("mute", muted_);
    save_bool("official", official_);
    save_bool("deleted", deleted_);

    if (outgoing_msg_count_ > 0)
        _node.AddMember("outgoingCount", outgoing_msg_count_, _a);

    if (is_chat_)
    {
        save_bool("public", public_);
        save_bool("livechat", is_live_chat_);
        save_string("chatType", chattype_);
    }
    else
    {
        lastseen_.serialize(_node, _a);

        save_string("abContactName", ab_contact_name_);
        save_string("smsNumber", sms_number_);
        save_string("otherNumber", other_number_);
    }

    if (!status_.is_empty())
        status_.serialize(_node, _a);

    if (auto_added_)
        _node.AddMember("autoAddition", "autoAddSaved", _a);

    if (!capabilities_.empty())
    {
        rapidjson::Value node_capabilities(rapidjson::Type::kArrayType);
        node_capabilities.Reserve(capabilities_.size(), _a);
        for (const auto& c : capabilities_)
            node_capabilities.PushBack(tools::make_string_ref(c), _a);

        _node.AddMember("capabilities", std::move(node_capabilities), _a);
    }
}

void cl_presence::unserialize(const rapidjson::Value& _node)
{
    search_cache_.clear();

    tools::unserialize_value(_node, "userType", usertype_);
    tools::unserialize_value(_node, "statusMsg", status_msg_);
    tools::unserialize_value(_node, "otherNumber", other_number_);
    tools::unserialize_value(_node, "smsNumber", sms_number_);
    tools::unserialize_value(_node, "friendly", friendly_);
    tools::unserialize_value(_node, "nick", nick_);
    tools::unserialize_value(_node, "abContactName", ab_contact_name_);
    tools::unserialize_value(_node, "iconId", icon_id_);
    tools::unserialize_value(_node, "bigIconId", big_icon_id_);
    tools::unserialize_value(_node, "largeIconId", large_icon_id_);
    tools::unserialize_value(_node, "official", official_);
    tools::unserialize_value(_node, "livechat", is_live_chat_);
    tools::unserialize_value(_node, "public", public_);
    tools::unserialize_value(_node, "deleted", deleted_);
    tools::unserialize_value(_node, "mute", muted_);
    tools::unserialize_value(_node, "outgoingCount", outgoing_msg_count_);

    tools::unserialize_value(_node, "chatType", chattype_);
    is_channel_ = chattype_ == "channel";

    lastseen_.unserialize(_node);

    const auto end = _node.MemberEnd();
    const auto iter_capabilities = _node.FindMember("capabilities");
    if (iter_capabilities != end && iter_capabilities->value.IsArray())
    {
        for (const auto& cap : iter_capabilities->value.GetArray())
            capabilities_.insert(rapidjson_get_string(cap));
    }

    const auto iter_status = _node.FindMember("status");
    if (iter_status != end && iter_status->value.IsObject())
        status_.unserialize(iter_status->value);

    if (std::string_view aa; tools::unserialize_value(_node, "autoAddition", aa))
        auto_added_ = !aa.empty();
}

bool cl_presence::are_icons_equal(const cl_presence& _other) const
{
    return are_icons_equal(*this, _other);
}

bool cl_presence::are_icons_equal(const cl_presence& _lhs, const cl_presence& _rhs)
{
    return std::make_tuple(std::cref(_lhs.large_icon_id_), std::cref(_lhs.big_icon_id_), std::cref(_lhs.icon_id_))
        == std::make_tuple(std::cref(_rhs.large_icon_id_), std::cref(_rhs.big_icon_id_), std::cref(_rhs.icon_id_));
}

void cl_buddy::prepare_search_cache()
{
    if (presence_ && presence_->search_cache_.is_empty())
    {
        presence_->search_cache_.aimid_ = tools::system::to_upper(aimid_);
        presence_->search_cache_.friendly_ = tools::system::to_upper(presence_->friendly_);
        presence_->search_cache_.nick_ = tools::system::to_upper(presence_->nick_);
        presence_->search_cache_.ab_ = tools::system::to_upper(presence_->ab_contact_name_);
        presence_->search_cache_.sms_number_ = presence_->sms_number_;
        presence_->search_cache_.friendly_words_ = tools::get_words(presence_->search_cache_.friendly_);
        presence_->search_cache_.ab_words_ = tools::get_words(presence_->search_cache_.ab_);
    }
}

contactlist::contactlist()
    : persons_(std::make_shared<core::archive::persons_map>())
{
}

cl_update_result contactlist::update_cl(const contactlist& _cl)
{
    cl_update_result result;

    groups_ = _cl.groups_;

    std::vector<std::string> avatar_updates;
    std::map<std::string, int32_t> out_counts;
    for (const auto& [aimid, buddy] : contacts_index_)
    {
        out_counts[aimid] = buddy->presence_->outgoing_msg_count_;
        if (const auto it = std::as_const(_cl.contacts_index_).find(aimid); it != std::as_const(_cl.contacts_index_).end())
        {
            if (!(buddy->presence_->are_icons_equal(*(it->second->presence_))))
                avatar_updates.emplace_back(aimid);
        }
        else
        {
            avatar_updates.emplace_back(aimid);
        }
    }

    if (contacts_index_.empty())
        for (const auto& [aimid, _] : _cl.contacts_index_)
            avatar_updates.emplace_back(aimid);

    contacts_index_ = _cl.contacts_index_;

    if (!out_counts.empty())
    {
        for (auto& [aimid, buddy] : contacts_index_)
        {
            if (const auto it = std::as_const(out_counts).find(aimid); it != std::as_const(out_counts).end())
                buddy->presence_->outgoing_msg_count_ = it->second;
        }
    }

    set_changed_status(contactlist::changed_status::full);
    set_need_update_cache(true);

    result.update_avatars_ = std::move(avatar_updates);
    return result;
}

void contactlist::update_ignorelist(const ignorelist_cache& _ignorelist)
{
    ignorelist_ = _ignorelist;

    set_changed_status(contactlist::changed_status::full);
    set_need_update_cache(true);
}

void contactlist::update_trusted(const std::string& _aimid, const std::string& _friendly, const std::string& _nick, const bool _is_trusted)
{
    if (exist(_aimid))
        return;

    auto it = trusted_contacts_.find(_aimid);
    if (it != trusted_contacts_.end() && !_is_trusted)
    {
        trusted_contacts_.erase(it);
    }
    else if (it == trusted_contacts_.end() && _is_trusted)
    {
        auto buddy = std::make_shared<cl_buddy>();
        buddy->aimid_ = _aimid;
        buddy->presence_->friendly_ = _friendly;
        buddy->presence_->nick_ = _nick;

        trusted_contacts_[_aimid] = std::move(buddy);
    }
}

void contactlist::set_changed_status(changed_status _status) noexcept
{
    if (changed_status_ == changed_status::full && _status == changed_status::presence)
    {
        // don't drop status
        return;
    }

    changed_status_ = _status;
}

contactlist::changed_status contactlist::get_changed_status() const noexcept
{
    return changed_status_;
}


void contactlist::update_presence(const std::string& _aimid, const std::shared_ptr<cl_presence>& _presence)
{
    auto contact_presence = get_presence(_aimid);
    if (!contact_presence)
    {
        g_core->write_string_to_network_log(su::concat("contact-list-debug",
            " update_presence failed: cannot get presence for ", _aimid,
            "\r\n"));
        return;
    }

    if (_presence->friendly_ != contact_presence->friendly_ ||
        _presence->ab_contact_name_ != contact_presence->ab_contact_name_ ||
        _presence->nick_ != contact_presence->nick_)
    {
        contact_presence->search_cache_.clear();
    }

    contact_presence->usertype_ = _presence->usertype_;
    contact_presence->chattype_ = _presence->chattype_;
    contact_presence->status_msg_ = _presence->status_msg_;
    contact_presence->other_number_ = _presence->other_number_;
    contact_presence->sms_number_ = _presence->sms_number_;
    contact_presence->capabilities_ = _presence->capabilities_;
    contact_presence->friendly_ = _presence->friendly_;
    contact_presence->nick_ = _presence->nick_;
    contact_presence->is_chat_ = _presence->is_chat_;
    contact_presence->muted_ = _presence->muted_;
    contact_presence->is_live_chat_ = _presence->is_live_chat_;
    contact_presence->official_ = _presence->official_;
    contact_presence->public_ = _presence->public_;
    contact_presence->auto_added_ = _presence->auto_added_;
    contact_presence->deleted_ = _presence->deleted_;

    if (!contact_presence->lastseen_.is_bot())
        contact_presence->lastseen_ = _presence->lastseen_;

    contact_presence->status_ = _presence->status_;

    // no need to update outgoing_msg_count_ here
    {
        need_update_avatar_ = !cl_presence::are_icons_equal(*contact_presence, *_presence);

        contact_presence->icon_id_ = _presence->icon_id_;
        contact_presence->big_icon_id_ = _presence->big_icon_id_;
        contact_presence->large_icon_id_ = _presence->large_icon_id_;
    }

    set_changed_status(contactlist::changed_status::presence);
}

void contactlist::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    rapidjson::Value node_groups(rapidjson::Type::kArrayType);
    node_groups.Reserve(groups_.size(), _a);
    for (const auto& group : groups_)
    {
        rapidjson::Value node_group(rapidjson::Type::kObjectType);

        node_group.AddMember("name",  group->name_, _a);
        node_group.AddMember("id", group->id_, _a);

        rapidjson::Value node_buddies(rapidjson::Type::kArrayType);
        node_buddies.Reserve(group->buddies_.size(),_a);
        for (const auto& buddy : group->buddies_)
        {
            rapidjson::Value node_buddy(rapidjson::Type::kObjectType);

            node_buddy.AddMember("aimId",  buddy->aimid_, _a);

            buddy->presence_->serialize(node_buddy, _a);

            node_buddies.PushBack(std::move(node_buddy), _a);
        }

        node_group.AddMember("buddies", std::move(node_buddies), _a);
        node_groups.PushBack(std::move(node_group), _a);
    }

    _node.AddMember("groups", std::move(node_groups), _a);

    rapidjson::Value node_ignorelist(rapidjson::Type::kArrayType);
    node_ignorelist.Reserve(ignorelist_.size(), _a);
    for (const auto& _aimid : ignorelist_)
    {
        rapidjson::Value node_aimid(rapidjson::Type::kObjectType);

        node_aimid.AddMember("aimId", _aimid, _a);

        node_ignorelist.PushBack(std::move(node_aimid), _a);
    }

    _node.AddMember("ignorelist", std::move(node_ignorelist), _a);
}

void core::wim::contactlist::serialize(icollection* _coll, const std::string& type) const
{
    coll_helper cl(_coll, false);

    ifptr<iarray> groups_array(_coll->create_array());
    groups_array->reserve((int32_t)groups_.size());

    for (const auto& group : groups_)
    {
        coll_helper group_coll(_coll->create_collection(), true);
        group_coll.set_value_as_string("group_name", group->name_);
        group_coll.set_value_as_int("group_id", group->id_);
        group_coll.set_value_as_bool("added", group->added_);
        group_coll.set_value_as_bool("removed", group->removed_);

        ifptr<iarray> contacts_array(_coll->create_array());
        contacts_array->reserve((int32_t)group->buddies_.size());

        for (const auto& buddy : group->buddies_)
        {
            if (is_ignored(buddy->aimid_))
                continue;

            coll_helper contact_coll(_coll->create_collection(), true);
            contact_coll.set_value_as_string("aimId", buddy->aimid_);

            buddy->presence_->serialize(contact_coll.get());

            ifptr<ivalue> val_contact(_coll->create_value());
            val_contact->set_as_collection(contact_coll.get());
            contacts_array->push_back(val_contact.get());
        }

        ifptr<ivalue> val_group(_coll->create_value());
        val_group->set_as_collection(group_coll.get());
        groups_array->push_back(val_group.get());

        group_coll.set_value_as_array("contacts", contacts_array.get());
    }

    if (!type.empty())
        cl.set_value_as_string("type", type);

    cl.set_value_as_array("groups", groups_array.get());
}

void contactlist::add_to_search_results(cl_buddies_map& _res_cache, const size_t _base_word_count, const cl_buddy_ptr& _buddy, std::set<std::string> _highlights, const int _priority)
{
    _res_cache.emplace(_buddy->aimid_, _buddy);

    if (_base_word_count <= 1 || _highlights.size() > 1 || (!_highlights.empty() && core::tools::get_words(*_highlights.begin()).size() == _base_word_count))
    {
        auto it = search_priority_.find(_buddy->aimid_);
        if (it != search_priority_.end())
        {
            it->second = std::min(it->second, _priority);

            auto res_it = std::find_if(search_results_.begin(), search_results_.end(), [&_buddy](const auto& _res) { return _res.aimid_ == _buddy->aimid_; });
            if (res_it != search_results_.end())
                res_it->highlights_.insert(std::make_move_iterator(_highlights.begin()), std::make_move_iterator(_highlights.end()));
        }
        else
        {
            search_priority_.emplace(_buddy->aimid_, _priority);

            cl_search_resut res;
            res.aimid_ = _buddy->aimid_;
            res.highlights_ = std::move(_highlights);
            search_results_.push_back(std::move(res));
        }
    }
}

cl_search_resut_v core::wim::contactlist::search(const std::vector<std::vector<std::string>>& _search_patterns, unsigned _fixed_patterns_count, const std::string_view _pattern)
{
    search_results_.clear();
    search_results_.reserve(get_contacts_count());

    if (_search_patterns.empty() && _pattern.empty())
    {
        tmp_cache_ = contacts_index_;
        tmp_cache_.insert(trusted_contacts_.begin(), trusted_contacts_.end());

        for (const auto& c : tmp_cache_)
            search_results_.push_back({ c.second->aimid_ });

        g_core->end_cl_search();

        return search_results_;
    }

    if (!_pattern.empty())
    {
        search(_pattern, true, 0, _fixed_patterns_count);
        if (g_core->end_cl_search() == 0)
            set_last_search_pattern(_pattern);
    }
    else
    {
        for (unsigned i = 0; i < _fixed_patterns_count; ++i)
        {
            std::string pat;
            pat.reserve(_search_patterns.size());
            for (const auto& search_pattern : _search_patterns)
            {
                if (search_pattern.size() > i)
                    pat += search_pattern[i];
            }

            boost::trim(pat);
            if (!pat.empty())
                search(pat, i == 0, i, _fixed_patterns_count);
        }

        search(_search_patterns, _fixed_patterns_count);
    }

    return search_results_;
}

void core::wim::contactlist::search(const std::vector<std::vector<std::string>>& search_patterns, int32_t fixed_patterns_count)
{
    cl_buddies_map result_cache;

    std::string base_word;
    for (const auto& symbol : search_patterns)
        base_word.append(symbol[0]);
    const auto base_word_count = core::tools::get_words(base_word).size();

    bool check_first = search_patterns.size() >= 3;
    auto cur = 0u;
    for (const auto& p : search_patterns)
    {
        if (p[0] != " ")
            ++cur;
        else
            cur = 0;

        if (cur > 1)
        {
            check_first = false;
            break;
        }
    }

    auto add_to_results = [&result_cache, base_word_count, this](const cl_buddy_ptr& _buddy, std::set<std::string>&& _highlights, const int _priority)
    {
        add_to_search_results(result_cache, base_word_count, _buddy, std::move(_highlights), _priority);
    };

    auto check = [&add_to_results](
        const cl_buddy_ptr& _buddy,
        const std::vector<std::vector<std::string>>& search_patterns,
        const std::string_view word,
        int32_t fixed_patterns_count)
    {
        if (word.empty())
            return false;

        int32_t priority = -1;
        if (auto [is_found, found_pat] = tools::contains(search_patterns, word, fixed_patterns_count, priority); is_found)
        {
            add_to_results(_buddy, { std::move(found_pat) }, priority);
            return true;
        }

        return false;
    };

    auto check_multi = [&add_to_results](
        const cl_buddy_ptr& _buddy,
        std::vector<std::vector<std::string>> search_patterns,
        const std::vector<std::string_view>& word,
        int32_t fixed_patterns_count)
    {
        if (word.empty())
            return false;

        int32_t priority = -1;

        for (const auto& w : word)
        {
            std::vector<std::vector<std::string>> pat;
            std::vector<std::vector<std::string>>::iterator is = search_patterns.begin();
            while (is != search_patterns.end())
            {
                if (!is->empty() && (*is)[0] == " ")
                {
                    is = search_patterns.erase(is);
                    break;
                }

                pat.push_back(*is);
                is = search_patterns.erase(is);
            }

            if (auto [is_found, found_pat] = tools::contains(pat, w, fixed_patterns_count, priority, true); is_found && !found_pat.empty())
            {
                add_to_results(_buddy, { std::move(found_pat) }, 0);
                return true;
            }
        }

        return false;
    };

    auto check_first_chars = [&add_to_results](
        const cl_buddy_ptr& _buddy,
        std::vector<std::vector<std::string>> search_patterns,
        const std::vector<std::string_view>& word,
        int32_t fixed_patterns_count)
    {
        if (word.empty())
            return false;

        std::string hl;

        auto w = word.rbegin();
        while (w != word.rend())
        {
            std::vector<std::vector<std::string>> pat;
            std::vector<std::vector<std::string>>::iterator is = search_patterns.begin();
            while (is != search_patterns.end())
            {
                if (!is->empty() && (*is)[0] == " ")
                {
                    is = search_patterns.erase(is);
                    break;
                }

                pat.push_back(*is);
                is = search_patterns.erase(is);
            }


            if (pat.empty())
                return false;

            auto ch = core::tools::from_utf8(*w)[0];
            bool found = false;
            for (const auto& s : pat[0])
            {
                const auto widestr = core::tools::from_utf8(s);
                if (widestr[0] == ch)
                {
                    found = true;
                    hl = core::tools::from_utf16(std::wstring_view(&widestr[0], 1));
                    break;
                }
            }

            if (!found)
                return false;

            ++w;
        }

        add_to_results(_buddy, { std::move(hl) }, 0);
        return true;
    };

    auto iter = tmp_cache_.cbegin();
    while (g_core->is_valid_cl_search() && iter != tmp_cache_.cend())
    {
        if (iter->second->presence_->deleted_ || is_ignored(iter->first) || iter->second->presence_->usertype_ == "sms")
        {
            ++iter;
            continue;
        }

        const auto& buddy = iter->second;

        buddy->prepare_search_cache();
        const auto& cached = buddy->presence_->search_cache_;

        check(buddy, search_patterns, cached.friendly_, fixed_patterns_count);
        check(buddy, search_patterns, cached.ab_, fixed_patterns_count);
        check(buddy, search_patterns, cached.nick_, fixed_patterns_count);

        if (!buddy->presence_->is_chat_)
        {
            check(buddy, search_patterns, cached.aimid_, fixed_patterns_count);
            check(buddy, search_patterns, cached.sms_number_, fixed_patterns_count);
        }

        check_multi(buddy, search_patterns, cached.friendly_words_, fixed_patterns_count);
        if (check_first)
            check_first_chars(buddy, search_patterns, cached.friendly_words_, fixed_patterns_count);
        check_multi(buddy, search_patterns, cached.ab_words_, fixed_patterns_count);
        if (check_first)
            check_first_chars(buddy, search_patterns, cached.ab_words_, fixed_patterns_count);

        ++iter;
    }

    if (g_core->is_valid_cl_search())
    {
        search_cache_.insert(std::make_move_iterator(result_cache.begin()), std::make_move_iterator(result_cache.end()));

        if (g_core->end_cl_search() == 0)
            last_search_pattern_ = base_word;
    }
    else
    {
        last_search_pattern_.clear();
        g_core->end_cl_search();
    }
}

void core::wim::contactlist::search(const std::string_view search_pattern, bool first, int32_t search_priority, int32_t fixed_patterns_count)
{
    cl_buddies_map result_cache;

    if (first)
        search_priority_.clear();

    const auto patterns = core::tools::get_words(search_pattern);

    if (first)
    {
        const auto need_new_cache =
            need_update_search_cache_ ||
            last_search_pattern_.empty() ||
            search_pattern.size() < last_search_pattern_.size() ||
            (!search_pattern.empty() && search_pattern.front() == '@') ||
            search_pattern.find(last_search_pattern_) == std::string_view::npos ||
            core::tools::get_words(last_search_pattern_).size() != patterns.size();

        if (need_new_cache)
        {
            tmp_cache_ = contacts_index_;
            tmp_cache_.insert(trusted_contacts_.begin(), trusted_contacts_.end());

            set_need_update_cache(false);
            search_cache_.clear();
        }
        else
        {
            tmp_cache_ = search_cache_;
        }
    }

    auto add_to_results = [&result_cache, base_word_count = patterns.size(), this](const cl_buddy_ptr& _buddy, std::set<std::string>&& _highlights, const int _priority)
    {
        add_to_search_results(result_cache, base_word_count, _buddy, std::move(_highlights), _priority);
    };

    auto check = [&add_to_results](
        const cl_buddy_ptr& _buddy,
        const std::string_view search_pattern,
        const std::string_view word)
    {
        auto pos = word.find(search_pattern);
        if (pos != std::string_view::npos)
        {
            const int32_t priority = (word.length() == search_pattern.length() || pos == 0 || word[pos - 1] == ' ') ? 0 : 1;

            add_to_results(_buddy, { std::string(search_pattern) }, priority);
            return true;
        }

        return false;
    };

    auto check_at_start = [&add_to_results](
        const cl_buddy_ptr& _buddy,
        const std::string_view search_pattern,
        const std::string_view word)
    {
        auto pos = word.find(search_pattern);
        if (pos == 0)
        {
            const int32_t priority = word.length() == search_pattern.length() ? 0 : 1;

            add_to_results(_buddy, { std::string(search_pattern) }, priority);
            return true;
        }

        return false;
    };

    auto check_multi = [&add_to_results](
        const cl_buddy_ptr& _buddy,
        const std::vector<std::string_view>& search_pattern,
        const std::vector<std::string_view>& word,
        int32_t fixed_patterns_count)
    {
        for (auto iter_search = search_pattern.begin(), iter_word = word.begin(); iter_search != search_pattern.end() && iter_word != word.end();)
        {
            if (iter_word->find(*iter_search) != 0)
                break;

            if (++iter_word == word.end())
            {
                std::set<std::string> hl;
                for (auto sp : search_pattern)
                    hl.insert(std::string(sp));
                add_to_results(_buddy, std::move(hl), 0);
                return true;
            }

            ++iter_search;
        }

        return false;
    };

    auto check_first_chars = [&add_to_results](
        const cl_buddy_ptr& _buddy,
        const std::vector<std::string_view>& search_pattern,
        const std::vector<std::string_view>& word,
        int32_t fixed_patterns_count)
    {
        auto found = true;
        auto iter_search = search_pattern.rbegin();
        auto iter_word = word.begin();
        while (iter_search != search_pattern.rend() && iter_word != word.end())
        {
            auto f = core::tools::from_utf8(*iter_word);
            auto s = core::tools::from_utf8(*iter_search);
            if (f.length() > 0 && s.length() > 0 && f[0] != s[0])
            {
                found = false;
                break;
            }

            ++iter_word;
            ++iter_search;
        }

        if (found)
        {
            add_to_results(_buddy, {}, 0);
            return true;
        }

        return false;
    };

    std::string_view with_at_cropped;
    if (search_pattern.size() > 1 && search_pattern.front() == '@')
        with_at_cropped = search_pattern.substr(1);

    if (!search_pattern.empty())
    {
        const bool check_first = patterns.size() >= 2 && std::all_of(patterns.cbegin(), patterns.cend(), [](const std::string_view p) { return p.size() == 1; });

        auto iter = tmp_cache_.cbegin();
        while (g_core->is_valid_cl_search() && iter != tmp_cache_.cend())
        {
            if (iter->second->presence_->deleted_ || is_ignored(iter->first) || iter->second->presence_->usertype_ == "sms")
            {
                ++iter;
                continue;
            }

            const auto& buddy = iter->second;

            buddy->prepare_search_cache();
            const auto& cached = buddy->presence_->search_cache_;

            check(buddy, search_pattern, cached.friendly_);

            if (!cached.nick_.empty())
            {
                check(buddy, search_pattern, cached.nick_);
                if (!with_at_cropped.empty() && !core::tools::is_email(cached.aimid_))
                    check_at_start(buddy, with_at_cropped, cached.nick_);
            }

            check(buddy, search_pattern, cached.ab_);

            if (!buddy->presence_->is_chat_)
            {
                check(buddy, search_pattern, cached.aimid_);
                if (!with_at_cropped.empty())
                    check_at_start(buddy, with_at_cropped, cached.aimid_);

                check(buddy, search_pattern, cached.sms_number_);
            }

            if (cached.friendly_words_.size() == patterns.size())
            {
                check_multi(buddy, patterns, cached.friendly_words_, fixed_patterns_count);
                if (check_first)
                    check_first_chars(buddy, patterns, cached.friendly_words_, fixed_patterns_count);
            }

            if (cached.ab_words_.size() == patterns.size())
            {
                check_multi(buddy, patterns, cached.ab_words_, fixed_patterns_count);
                if (check_first)
                    check_first_chars(buddy, patterns, cached.ab_words_, fixed_patterns_count);
            }

            ++iter;
        }
    }

    if (g_core->is_valid_cl_search())
    {
        if (first)
            search_cache_.clear();

        search_cache_.insert(std::make_move_iterator(result_cache.begin()), std::make_move_iterator(result_cache.end()));
    }
    else
    {
        last_search_pattern_.clear();
    }
}

int32_t contactlist::get_search_priority(const std::string& _aimid) const
{
    if (const auto it = search_priority_.find(_aimid); it != search_priority_.end())
        return it->second;

    return 0;
}

void contactlist::set_last_search_pattern(const std::string_view _pattern)
{
    last_search_pattern_ = _pattern;
}

void core::wim::contactlist::serialize_search(icollection* _coll) const
{
    coll_helper cl(_coll, false);

    ifptr<iarray> contacts_array(_coll->create_array());
    contacts_array->reserve((int32_t)search_cache_.size());

    for (const auto& buddy : search_cache_)
    {
        coll_helper contact_coll(_coll->create_collection(), true);
        contact_coll.set_value_as_string("aimId", buddy.first);
        ifptr<ivalue> val_contact(_coll->create_value());
        val_contact->set_as_collection(contact_coll.get());
        contacts_array->push_back(val_contact.get());
    }
    cl.set_value_as_array("contacts", contacts_array.get());
}

void contactlist::serialize_ignorelist(icollection* _coll) const
{
    coll_helper cl(_coll, false);

    ifptr<iarray> ignore_array(cl->create_array());
    ignore_array->reserve(int32_t(ignorelist_.size()));
    for (const auto& x : ignorelist_)
    {
        coll_helper coll_chatter(cl->create_collection(), true);
        ifptr<ivalue> val(cl->create_value());
        val->set_as_string(x.c_str(), (int32_t) x.length());
        ignore_array->push_back(val.get());
    }

    cl.set_value_as_array("aimids", ignore_array.get());
}

void contactlist::serialize_contact(const std::string& _aimid, icollection* _coll) const
{
    coll_helper coll(_coll, false);

    ifptr<iarray> groups_array(_coll->create_array());

    for (const auto& _group : groups_)
    {
        for (const auto& _buddy : _group->buddies_)
        {
            if (_buddy->aimid_ == _aimid)
            {
                coll_helper coll_group(_coll->create_collection(), true);
                coll_group.set_value_as_string("group_name", _group->name_);
                coll_group.set_value_as_int("group_id", _group->id_);
                coll_group.set_value_as_bool("added", _group->added_);
                coll_group.set_value_as_bool("removed", _group->removed_);

                ifptr<iarray> contacts_array(_coll->create_array());

                coll_helper coll_contact(coll->create_collection(), true);
                coll_contact.set_value_as_string("aimId", _buddy->aimid_);

                _buddy->presence_->serialize(coll_contact.get());

                ifptr<ivalue> val_contact(_coll->create_value());
                val_contact->set_as_collection(coll_contact.get());
                contacts_array->push_back(val_contact.get());

                ifptr<ivalue> val_group(_coll->create_value());
                val_group->set_as_collection(coll_group.get());
                groups_array->push_back(val_group.get());

                coll_group.set_value_as_array("contacts", contacts_array.get());

                coll.set_value_as_array("groups", groups_array.get());

                return;
            }
        }
    }
}

std::string contactlist::get_contact_friendly_name(const std::string& contact_login) const
{
    auto presence = get_presence(contact_login);
    if (presence)
        return presence->friendly_;
    return std::string();
}

void contactlist::add_to_persons(const std::shared_ptr<cl_buddy>& _buddy)
{
    archive::person p;
    p.friendly_ = _buddy->presence_->friendly_;
    p.official_ = _buddy->presence_->official_;
    p.nick_ = _buddy->presence_->nick_;
    persons_->emplace(_buddy->aimid_, std::move(p));
}

int32_t contactlist::unserialize(const rapidjson::Value& _node)
{
    static long buddy_id = 0;

    constexpr std::string_view chat_domain = "@chat.agent";

    const auto node_end = _node.MemberEnd();
    const auto iter_groups = _node.FindMember("groups");
    if (iter_groups == node_end || !iter_groups->value.IsArray())
        return 0;

    for (const auto& grp : iter_groups->value.GetArray())
    {
        int grp_id;
        if (!tools::unserialize_value(grp, "id", grp_id))
        {
            im_assert(false);
            continue;
        }

        auto group = std::make_shared<core::wim::cl_group>();
        group->id_ = grp_id;
        if (!tools::unserialize_value(grp, "name", group->name_))
            im_assert(false);

        if (const auto iter_buddies = grp.FindMember("buddies"); iter_buddies != grp.MemberEnd() && iter_buddies->value.IsArray())
        {
            group->buddies_.reserve(iter_buddies->value.Size());
            for (const auto& bd : iter_buddies->value.GetArray())
            {
                std::string_view aimid;
                if (!tools::unserialize_value(bd, "aimId", aimid))
                {
                    im_assert(false);
                    continue;
                }

                auto buddy = std::make_shared<wim::cl_buddy>();
                buddy->id_ = (uint32_t)++buddy_id;
                buddy->aimid_ = aimid;
                buddy->presence_->unserialize(bd);

                if (buddy->aimid_.length() > chat_domain.length() && aimid.substr(aimid.length() - chat_domain.length(), chat_domain.length()) == chat_domain)
                    buddy->presence_->is_chat_ = true;

                add_to_persons(buddy);

                contacts_index_[buddy->aimid_] = buddy;

                group->buddies_.push_back(std::move(buddy));
            }
        }

        groups_.push_back(std::move(group));
    }

    const auto iter_ignorelist = _node.FindMember("ignorelist");
    if (iter_ignorelist == node_end || !iter_ignorelist->value.IsArray())
        return 0;

    for (const auto& ignore : iter_ignorelist->value.GetArray())
    {
        if (std::string aimid; tools::unserialize_value(ignore, "aimId", aimid))
            ignorelist_.emplace(std::move(aimid));
    }

    return 0;
}

int32_t contactlist::unserialize_from_diff(const rapidjson::Value& _node)
{
    static long buddy_id = 0;

    constexpr std::string_view chat_domain = "@chat.agent";

    for (const auto& grp : _node.GetArray())
    {
        int grp_id;
        if (!tools::unserialize_value(grp, "id", grp_id))
        {
            im_assert(false);
            continue;
        }

        auto group = std::make_shared<core::wim::cl_group>();
        group->id_ = grp_id;

        if (!tools::unserialize_value(grp, "name", group->name_))
            im_assert(false);

        const auto iter_grp_end = grp.MemberEnd();
        const auto iter_group_added = grp.FindMember("added");
        if (iter_group_added != iter_grp_end)
            group->added_ = true;

        const auto iter_group_removed = grp.FindMember("removed");
        if (iter_group_removed != iter_grp_end)
            group->removed_ = true;

        if (const auto iter_buddies = grp.FindMember("buddies"); iter_buddies != iter_grp_end && iter_buddies->value.IsArray())
        {
            group->buddies_.reserve(iter_buddies->value.Size());
            for (const auto& bd : iter_buddies->value.GetArray())
            {
                std::string_view aimid;
                if (!tools::unserialize_value(bd, "aimId", aimid))
                {
                    im_assert(false);
                    continue;
                }

                auto buddy = std::make_shared<wim::cl_buddy>();
                buddy->id_ = (uint32_t)++buddy_id;
                buddy->aimid_ = aimid;
                buddy->presence_->unserialize(bd);

                if (buddy->aimid_.length() > chat_domain.length() && aimid.substr(aimid.length() - chat_domain.length(), chat_domain.length()) == chat_domain)
                    buddy->presence_->is_chat_ = true;

                add_to_persons(buddy);

                contacts_index_[buddy->aimid_] = buddy;

                group->buddies_.push_back(std::move(buddy));
            }
        }

        groups_.push_back(std::move(group));
    }

    return 0;
}

namespace
{
    struct int_count
    {
        explicit int_count() : count{ 0 } {}
        int count;

        int_count& operator++()
        {
            ++count;
            return *this;
        }
    };
}

void contactlist::merge_from_diff(const std::string& _type, const std::shared_ptr<contactlist>& _diff, const std::shared_ptr<std::list<std::string>>& removedContacts)
{
    changed_status status = changed_status::none;
    if (_type == "created")
    {
        status = changed_status::full;
        for (const auto& diff_group : _diff->groups_)
        {
            const auto diff_group_id = diff_group->id_;
            if (diff_group->added_)
            {
                auto group = std::make_shared<cl_group>();
                group->id_ = diff_group_id;
                group->name_ = diff_group->name_;
                groups_.push_back(std::move(group));
            }

            for (const auto& group : groups_)
            {
                if (group->id_ != diff_group_id)
                    continue;

                for (const auto& diff_buddy : diff_group->buddies_)
                {
                    g_core->write_string_to_network_log(su::concat("contact-list-debug",
                        " merge_from_diff add person. buddie, cl index ", diff_buddy->aimid_,
                        "\r\n"));
                    add_to_persons(diff_buddy);
                    group->buddies_.push_back(diff_buddy);
                    contacts_index_[diff_buddy->aimid_] = diff_buddy;
                }
            }
        }
    }
    else if (_type == "updated")
    {
        status = changed_status::presence;
        for (const auto& diff_group : _diff->groups_)
        {
            for (const auto& buddie : diff_group->buddies_)
            {
                add_to_persons(buddie);
                update_presence(buddie->aimid_, buddie->presence_);
            }
        }
    }
    else if (_type == "deleted")
    {
        status = changed_status::full;
        std::map<std::string, int_count> contacts;

        for (const auto& diff_group : _diff->groups_)
        {
            const auto diff_group_id = diff_group->id_;
            for (auto group_iter = groups_.begin(); group_iter != groups_.end();)
            {
                if ((*group_iter)->id_ != diff_group_id)
                {
                    for (const auto& diff_buddy : diff_group->buddies_)
                    {
                        const auto& aimId = diff_buddy->aimid_;
                        const auto& group_buddies = (*group_iter)->buddies_;
                        const auto contains = std::any_of(group_buddies.begin(), group_buddies.end(), [&aimId](const cl_buddy_ptr& value) { return value->aimid_ == aimId; });
                        if (contains)
                            ++contacts[aimId];
                    }

                    ++group_iter;
                    continue;
                }

                for (const auto& diff_buddy : diff_group->buddies_)
                {
                    const auto& aimId = diff_buddy->aimid_;
                    auto& group_buddies = (*group_iter)->buddies_;
                    const auto end = group_buddies.end();
                    group_buddies.erase(std::remove_if(group_buddies.begin(), end,
                        [&aimId, &contacts](const cl_buddy_ptr& value)
                        {
                            if (value->aimid_ == aimId)
                            {
                                ++contacts[aimId];
                                return true;
                            }
                            return false;
                        }
                    ), end);
                }

                if (diff_group->removed_)
                    group_iter = groups_.erase(group_iter);
                else
                    ++group_iter;
            }
        }

        for (const auto& c : contacts)
        {
            if (c.second.count == 1)
            {
                contacts_index_.erase(c.first);
                trusted_contacts_.erase(c.first);
                removedContacts->push_back(c.first);
            }
        }
    }
    else
    {
        return;
    }
    set_changed_status(status);
    set_need_update_cache(true);
}

int32_t contactlist::get_contacts_count() const
{
    return contacts_index_.size();
}

int32_t contactlist::get_phone_contacts_count() const
{
    return std::count_if(contacts_index_.begin(), contacts_index_.end(), [](const std::pair<std::string, cl_buddy_ptr>& contact)
    { return contact.second->aimid_.find('+') != std::string::npos; });
}

int32_t contactlist::get_groupchat_contacts_count() const
{
    return std::count_if(contacts_index_.begin(), contacts_index_.end(), [](const std::pair<std::string, cl_buddy_ptr>& contact)
    { return contact.second->aimid_.find("@chat.agent") != std::string::npos; });
}

void contactlist::add_to_ignorelist(const std::string& _aimid)
{
    const auto res = ignorelist_.emplace(_aimid);
    if (res.second)
        set_changed_status(changed_status::full);
}

void contactlist::remove_from_ignorelist(const std::string& _aimid)
{
    const auto iter = ignorelist_.find(_aimid);
    if (iter != ignorelist_.end())
    {
        ignorelist_.erase(iter);
        set_changed_status(changed_status::full);
    }
}

bool contactlist::contains(const std::string& _aimid) const
{
    return contacts_index_.find(_aimid) != contacts_index_.end();
}

std::shared_ptr<cl_presence> core::wim::contactlist::get_presence(const std::string & _aimid) const
{
    auto it = contacts_index_.find(_aimid);
    if (it != contacts_index_.end() && it->second->presence_)
        return it->second->presence_;

    return nullptr;
}

void contactlist::set_outgoing_msg_count(const std::string& _contact, int32_t _count)
{
    if (_count != -1)
    {
        auto presence = get_presence(_contact);
        if (presence)
        {
            presence->outgoing_msg_count_ = _count;
            set_changed_status(changed_status::presence);
        }
    }
}

void contactlist::reset_outgoung_counters()
{
    for (auto& c : contacts_index_)
        c.second->presence_->outgoing_msg_count_ = 0;
}

std::shared_ptr<cl_group> contactlist::get_first_group() const
{
    if (groups_.empty())
    {
        return nullptr;
    }

    return (*groups_.begin());
}

bool contactlist::is_ignored(const std::string& _aimid) const
{
    return (ignorelist_.find(_aimid) != ignorelist_.end());
}

void contactlist::set_need_update_cache(bool _need_update_search_cache)
{
    need_update_search_cache_ = _need_update_search_cache;
}

bool contactlist::is_empty() const
{
    return contacts_index_.empty();
}

std::vector<std::string> core::wim::contactlist::get_ignored_aimids() const
{
    std::vector<std::string> result;
    result.reserve(ignorelist_.size());

    for (const auto& item : ignorelist_)
        result.push_back(item);

    return result;
}

std::vector<std::string> contactlist::get_aimids() const
{
    std::vector<std::string> result;
    result.reserve(contacts_index_.size());

    for (const auto& item : contacts_index_)
        result.push_back(item.first);

    return result;
}
