#include "stdafx.h"

#include "lastseen.h"
#include "../../../common.shared/json_helper.h"
#include "../../../corelib/collection_helper.h"

namespace core
{
    static constexpr lastseen::seen_state string_2_state(const std::string_view _state)
    {
        if (_state == "absent")
            return lastseen::seen_state::absent;
        else if (_state == "blocked")
            return lastseen::seen_state::blocked;
        else if (_state == "bot")
            return lastseen::seen_state::bot;

        return lastseen::seen_state::active;
    }

    static constexpr std::string_view state_2_string(const lastseen::seen_state _state)
    {
        if (_state == lastseen::seen_state::absent)
            return "absent";
        else if (_state == lastseen::seen_state::blocked)
            return "blocked";
        else if (_state == lastseen::seen_state::bot)
            return "bot";

        return "active";
    }

    void lastseen::unserialize(const rapidjson::Value& _node)
    {
        time_ = {};
        state_ = seen_state::active;

        const auto unserialize_time = [this](const auto& _node)
        {
            if (int64_t time; tools::unserialize_value(_node, "lastseen", time))
                time_ = std::make_optional(time);
        };

        const auto iter_state = _node.FindMember("userState");
        if (iter_state != _node.MemberEnd())
        {
            if (iter_state->value.IsObject())
            {
                unserialize_time(iter_state->value);
                if (std::string_view str; tools::unserialize_value(iter_state->value, "state", str))
                    state_ = string_2_state(str);
            }
            else if (iter_state->value.IsString())
            {
                state_ = string_2_state(rapidjson_get_string_view(iter_state->value));
                unserialize_time(_node);
            }
        }
        else
        {
            unserialize_time(_node);
        }

        if (bool is_bot = false; tools::unserialize_value(_node, "bot", is_bot) && is_bot)
            state_ = seen_state::bot;

        if (state_ != seen_state::active || (time_ && *time_ < 0))
            time_ = {};
    }

    void lastseen::serialize(core::coll_helper _coll) const
    {
        coll_helper coll_ls(_coll->create_collection(), true);
        if (state_ == seen_state::active)
        {
            if (time_)
                coll_ls.set_value_as_int64("time", *time_);
        }
        else
        {
            coll_ls.set_value_as_string("state", state_2_string(state_));
        }
        _coll.set_value_as_collection("lastseen", coll_ls.get());
    }

    void lastseen::serialize(rapidjson::Value& _node, rapidjson_allocator& _a)
    {
        rapidjson::Value seen(rapidjson::Type::kObjectType);
        if (state_ == seen_state::active)
        {
            if (time_ && *time_ >= 0)
                seen.AddMember("lastseen", *time_, _a);
        }
        else
        {
            seen.AddMember("state", tools::make_string_ref(state_2_string(state_)), _a);
        }

        if (seen.MemberCount())
            _node.AddMember("userState", std::move(seen), _a);
    }
}