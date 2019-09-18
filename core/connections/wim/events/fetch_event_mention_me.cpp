#include "stdafx.h"

#include "fetch_event_mention_me.h"
#include "../wim_im.h"
#include "../wim_packet.h"
#include "../wim_history.h"
#include "../../../archive/archive_index.h"

using namespace core;
using namespace wim;

fetch_event_mention_me::fetch_event_mention_me()
    : persons_(std::make_shared<core::archive::persons_map>())
{
}


fetch_event_mention_me::~fetch_event_mention_me() = default;

int32_t fetch_event_mention_me::parse(const rapidjson::Value& _node_event_data)
{
    const auto iter_sn = _node_event_data.FindMember("sn");
    if (iter_sn == _node_event_data.MemberEnd() || !iter_sn->value.IsString())
        return wpie_error_parse_response;

    aimid_ = rapidjson_get_string(iter_sn->value);

    const auto iter_message = _node_event_data.FindMember("message");
    if (iter_message == _node_event_data.MemberEnd() || !iter_message->value.IsObject())
        return wpie_error_parse_response;

    message_ = std::make_shared<archive::history_message>();
    if (0 != message_->unserialize(iter_message->value, aimid_))
        return wpie_error_parse_response;

    *persons_ = parse_persons(_node_event_data);
    message_->apply_persons_to_mentions(*persons_);

    return 0;
}

void fetch_event_mention_me::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_mention_me(this, _on_complete);
}

const std::string& fetch_event_mention_me::get_aimid() const
{
    return aimid_;
}

std::shared_ptr<archive::history_message> fetch_event_mention_me::get_message() const
{
    return message_;
}
