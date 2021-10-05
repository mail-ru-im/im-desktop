#include "stdafx.h"

#include "threads_unread_cache.h"
#include "../../../../common.shared/json_helper.h"

namespace core::wim
{

int threads_unread_cache::unread_count() const
{
    return unread_count_;
}

int threads_unread_cache::unread_mentions_count() const
{
    return unread_mentions_count_;
}

void threads_unread_cache::set_unread_count(int _unread_count)
{
    unread_count_ = _unread_count;
    changed_ = true;
}

void threads_unread_cache::set_unread_mentions_count(int _unread_mentions_count)
{
    unread_mentions_count_ = _unread_mentions_count;
    changed_ = true;
}

void threads_unread_cache::set_changed(bool _changed)
{
    changed_ = _changed;
}

bool threads_unread_cache::is_changed() const
{
    return changed_;
}

int threads_unread_cache::unserialize(const rapidjson::Value& _node)
{
    tools::unserialize_value(_node, "unreadCount", unread_count_);
    tools::unserialize_value(_node, "unreadMentionsCount", unread_mentions_count_);
    return 0;
}

void threads_unread_cache::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    _node.AddMember("unreadCount", unread_count_, _a);
    _node.AddMember("unreadMentionsCount", unread_mentions_count_, _a);
}

}
