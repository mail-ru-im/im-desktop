#pragma once

namespace core::wim
{
    class threads_unread_cache
    {
    public:
        int unread_count() const;
        int unread_mentions_count() const;

        void set_unread_count(int _unread_count);
        void set_unread_mentions_count(int _unread_mentions_count);
        void set_changed(bool _changed);
        bool is_changed() const;

        int unserialize(const rapidjson::Value& _node);
        void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;
    private:
        int unread_count_ = 0;
        int unread_mentions_count_ = 0;
        bool changed_ = false;
    };
}
