#pragma once

namespace core
{
    class coll_helper;

    struct lastseen
    {
        enum class seen_state
        {
            active,
            absent,
            blocked,
            bot
        };
        seen_state state_ = seen_state::active;
        std::optional<int64_t> time_;

        bool is_bot() const noexcept { return state_ == seen_state::bot; }

        void unserialize(const rapidjson::Value& _node);
        void serialize(core::coll_helper _coll) const;
        void serialize(rapidjson::Value& _node, rapidjson_allocator& _a);
    };
}