#include "stdafx.h"

#include "fetch_event_chat_heads.h"
#include "../wim_im.h"
#include "../wim_packet.h"
#include "../wim_history.h"

#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

chat_heads::chat_heads()
    : reset_state_(false)
    , persons_(std::make_shared<core::archive::persons_map>())
{
}

void core::wim::chat_heads::merge(const chat_heads& _other)
{
    assert(chat_aimid_ == _other.chat_aimid_);
    assert(!reset_state_ && !_other.reset_state_);

    if (_other.persons_)
    {
        for (const auto& [aimid, p] : *_other.persons_)
        {
            const auto it = persons_->find(aimid);
            if (it != persons_->end())
                it->second = p;
            else
                persons_->insert({ aimid, p });
        }
    }

    for (const auto& [omsgid, ohead_v] : _other.chat_heads_)
    {
        for (const auto& oh : ohead_v)
        {
            for (auto&[_, heads] : chat_heads_)
                heads.erase(std::remove_if(heads.begin(), heads.end(), [&oh](const auto& _h) { return _h == oh; }), heads.end());
        }

        auto& our_heads = chat_heads_[omsgid];
        our_heads.insert(our_heads.begin(), ohead_v.begin(), ohead_v.end());
    }

    for (auto it = chat_heads_.begin(); it != chat_heads_.end(); )
    {
        if (it->second.empty())
            it = chat_heads_.erase(it);
        else
            ++it;
    }
}

fetch_event_chat_heads::fetch_event_chat_heads()
    : heads_(std::make_shared<chat_heads>())
{
}

fetch_event_chat_heads::~fetch_event_chat_heads() = default;

int32_t fetch_event_chat_heads::parse(const rapidjson::Value& _node_event_data)
{
    if (!tools::unserialize_value(_node_event_data, "sn", heads_->chat_aimid_))
        return wpie_error_parse_response;

    tools::unserialize_value(_node_event_data, "resetState", heads_->reset_state_);

    *heads_->persons_ = parse_persons(_node_event_data);

    if (const auto iter_positions = _node_event_data.FindMember("positions"); iter_positions != _node_event_data.MemberEnd() && iter_positions->value.IsArray())
    {
        for (const auto& pos : iter_positions->value.GetArray())
        {
            auto iter_heads = pos.FindMember("heads");
            if (iter_heads == pos.MemberEnd() || !iter_heads->value.IsArray())
                continue;

            int64_t msg_id;
            if (!tools::unserialize_value(pos, "msgId", msg_id))
                continue;

            std::vector<std::string> heads;
            heads.reserve(iter_heads->value.Size());
            for (const auto& h : iter_heads->value.GetArray())
            {
                if (std::string head; tools::unserialize_value(h, "sn", head))
                    heads.push_back(std::move(head));
            }

            if (!heads.empty())
                heads_->chat_heads_[msg_id] = std::move(heads);
        }
    }

    return 0;
}

void fetch_event_chat_heads::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_chat_heads(this, _on_complete);
}