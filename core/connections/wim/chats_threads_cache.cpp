#include "stdafx.h"

#include "chats_threads_cache.h"

#include "../../../corelib/collection_helper.h"
#include "../../../common.shared/json_helper.h"

using namespace core;
using namespace wim;

namespace
{
    bool is_thread_feed(std::string_view _id)
    {
        return _id == "~threads~";
    }
}

cached_chat::cached_chat(std::string_view _aimid, int64_t _msg_id, std::string_view _thread_id)
    : aimid_(_aimid)
{
    threads_.emplace(_msg_id, _thread_id);
}

void cached_chat::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    _node.AddMember("aimId",  get_aimid(), _a);

    rapidjson::Value node_threads(rapidjson::Type::kArrayType);
    node_threads.Reserve(threads_.size(), _a);
    for (const auto& [msg_id, thread_id] : threads_)
    {
        rapidjson::Value node_thread(rapidjson::Type::kObjectType);
        node_thread.AddMember("thread_id", thread_id, _a);
        node_thread.AddMember("msg_id", msg_id, _a);
        node_threads.PushBack(std::move(node_thread), _a);
    }
    _node.AddMember("threads", std::move(node_threads), _a);
}

void cached_chat::add_thread(int64_t _msg_id, std::string_view _thread_id)
{
    threads_.emplace(_msg_id, _thread_id);
}

bool cached_chat::remove_thread(std::string_view _thread_id)
{
    auto iter = std::find_if(threads_.begin(), threads_.end(), [id = _thread_id.data()](const auto& _thread) { return _thread.second == id; });
    if (iter != threads_.end())
    {
        threads_.erase(iter);
        return true;
    }
    return false;
}

void cached_chat::remove_thread(int64_t _msg_id)
{
    threads_.erase(_msg_id);
}

int32_t cached_chat::unserialize(const rapidjson::Value& _node)
{
    if (!tools::unserialize_value(_node, "aimId", aimid_))
        return -1;

    auto iter_dialogs = _node.FindMember("threads");
    if (iter_dialogs == _node.MemberEnd() || !iter_dialogs->value.IsArray())
        return -1;

    for (const auto& dialog : iter_dialogs->value.GetArray())
    {
        std::string thread_id;
        int64_t msg_id;
        tools::unserialize_value(dialog, "msg_id", msg_id);
        tools::unserialize_value(dialog, "thread_id", thread_id);
        threads_.emplace(msg_id, std::move(thread_id));
    }

    return 0;
}

void chats_threads_cache::add_thread(std::string_view _aimid, int64_t _msg_id, std::string_view _thread_id)
{
    const auto aimid = _aimid.data();
    if (!contains(_aimid))
    {
        chats_.insert({ aimid, cached_chat(_aimid, _msg_id, _thread_id) });
        return;
    }

    auto& item = chats_[aimid];
    item.add_thread(_msg_id, _thread_id);
}

void chats_threads_cache::remove_thread(std::string_view _aimid, std::string_view _thread_id)
{
    if (contains(_aimid))
    {
        auto& item = chats_[_aimid.data()];
        item.remove_thread(_thread_id);
        if (item.get_threads().empty())
            chats_.erase(_aimid.data());
    }
}

void chats_threads_cache::remove_thread(std::string_view _thread_id)
{
    for (auto& [id, chat] : chats_)
    {
        if (chat.remove_thread(_thread_id))
        {
            if (chat.get_threads().empty())
                chats_.erase(id);
            break;
        }
    }
}

void chats_threads_cache::remove_thread(std::string_view _aimid, int64_t _msg_id)
{
    if (contains(_aimid))
    {
        auto& item = chats_[_aimid.data()];
        item.remove_thread(_msg_id);
        if (item.get_threads().empty())
            chats_.erase(_aimid.data());
    }
}

void chats_threads_cache::remove_chat(std::string_view _aimid)
{
    if (const auto it = chats_.find(_aimid.data()); it != chats_.end())
        chats_.erase(it);
}

void chats_threads_cache::clear()
{
    chats_.clear();
}

std::set<std::string> chats_threads_cache::threads(std::string_view _aimid)
{
    std::set<std::string> result;
    if (is_thread_feed(_aimid))
    {
        for (const auto& [id, chat] : chats_)
        {
            for (const auto& [_, t] : chat.get_threads())
                result.insert(t);
        }
    }
    else
    {
        if (const auto it = chats_.find(_aimid.data()); it != chats_.end())
        {
            for (const auto& [_, t] : it->second.get_threads())
                result.insert(t);
        }
    }
    return result;
}

bool chats_threads_cache::contains(std::string_view _aimid) const
{
    return chats_.find(_aimid.data()) != chats_.end();
}

void chats_threads_cache::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    rapidjson::Value node_contacts(rapidjson::Type::kArrayType);
    node_contacts.Reserve(chats_.size(), _a);
    for (const auto& contact : chats_)
    {
        rapidjson::Value node_contact(rapidjson::Type::kObjectType);
        contact.second.serialize(node_contact, _a);
        node_contacts.PushBack(std::move(node_contact), _a);
    }

    _node.AddMember(tools::make_string_ref("chats"), std::move(node_contacts), _a);
}

int32_t chats_threads_cache::unserialize(const rapidjson::Value& _node)
{
    const auto iter_contacts = _node.FindMember("chats");
    if (iter_contacts == _node.MemberEnd() || !iter_contacts->value.IsArray())
        return -1;

    chats_.reserve(iter_contacts->value.Size());
    for (const auto& contact : iter_contacts->value.GetArray())
    {
        cached_chat c;
        if (c.unserialize(contact) != 0)
            return -1;

        const auto aimid = c.get_aimid();
        chats_.emplace(aimid, std::move(c));
    }

    return 0;
}
