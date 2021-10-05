#include "stdafx.h"

#include "fetch_event_dlg_state.h"
#include "../wim_history.h"
#include "../wim_im.h"
#include "../wim_packet.h"
#include "../../../archive/archive_index.h"
#include "../../../log/log.h"
#include "../../../../common.shared/json_helper.h"

namespace
{
    constexpr bool is_message_parsing_enabled() noexcept { return true; }
}


using namespace core;
using namespace wim;

fetch_event_dlg_state::fetch_event_dlg_state()
    : tail_messages_(std::make_shared<archive::history_block>())
    , intro_messages_(std::make_shared<archive::history_block>())
    , persons_(std::make_shared<core::archive::persons_map>())
{
}


fetch_event_dlg_state::~fetch_event_dlg_state() = default;

template <typename T>
int64_t get_older_msg_id(const T& value)
{
    int64_t msgid = -1;
    tools::unserialize_value(value, "olderMsgId", msgid);
    return msgid;
}

int32_t fetch_event_dlg_state::parse(const rapidjson::Value& _node_event_data)
{
    const auto iter_sn = _node_event_data.FindMember("sn");
    const auto iter_last_msg_id = _node_event_data.FindMember("lastMsgId");

    if (
        iter_sn == _node_event_data.MemberEnd() ||
        iter_last_msg_id == _node_event_data.MemberEnd() ||
        !iter_sn->value.IsString() ||
        !iter_last_msg_id->value.IsInt64())
    {
        __TRACE(
            "delivery",
            "%1%",
            "failed to parse incoming dlg state event");

        return wpie_error_parse_response;
    }

    aimid_ = rapidjson_get_string(iter_sn->value);
    state_.set_last_msgid(iter_last_msg_id->value.GetInt64());

    if (uint32_t unread = 0; tools::unserialize_value(_node_event_data, "unreadCnt", unread))
        state_.set_unread_count(unread);

    if (bool att = false; tools::unserialize_value(_node_event_data, "attention", att))
        state_.set_attention(att);

    if (uint32_t unread = 0; tools::unserialize_value(_node_event_data, "unreadMentionMeCount", unread))
        state_.set_unread_mentions_count(unread);

    if (int64_t lastread = 0; tools::unserialize_value(_node_event_data, "lastReadMention", lastread))
        state_.set_last_read_mention(lastread);

    if (const auto it_yours = _node_event_data.FindMember("yours"); it_yours != _node_event_data.MemberEnd() && it_yours->value.IsObject())
    {
        if (int64_t lastread = 0; tools::unserialize_value(it_yours->value, "lastRead", lastread))
            state_.set_yours_last_read(lastread);
    }

    if (const auto it_theirs = _node_event_data.FindMember("theirs"); it_theirs != _node_event_data.MemberEnd() && it_theirs->value.IsObject())
    {
        if (int64_t lastread = 0; tools::unserialize_value(it_theirs->value, "lastRead", lastread))
            state_.set_theirs_last_read(lastread);

        if (int64_t lastdeliver = 0; tools::unserialize_value(it_theirs->value, "lastDelivered", lastdeliver))
            state_.set_theirs_last_delivered(lastdeliver);
    }

    if (std::string patch_version; tools::unserialize_value(_node_event_data, "patchVersion", patch_version))
    {
        im_assert(!patch_version.empty());
        state_.set_dlg_state_patch_version(std::move(patch_version));
    }

    if (int64_t delupto = 0; tools::unserialize_value(_node_event_data, "delUpto", delupto))
        state_.set_del_up_to(delupto);

    if (bool susp = false; tools::unserialize_value(_node_event_data, "suspicious", susp))
        state_.set_suspicious(susp);

    if (bool stranger = false; tools::unserialize_value(_node_event_data, "stranger", stranger))
        state_.set_stranger(stranger);

    if (bool no_recents_update = false; tools::unserialize_value(_node_event_data, "noRecentsUpdate", no_recents_update))
        state_.set_no_recents_update(no_recents_update);

    if (const auto it_mchat_state = _node_event_data.FindMember("mchatState"); it_mchat_state != _node_event_data.MemberEnd() && it_mchat_state->value.IsObject())
    {
        if (std::string info_version; tools::unserialize_value(it_mchat_state->value, "infoVersion", info_version))
            state_.set_info_version(info_version);

        if (std::string members_version; tools::unserialize_value(it_mchat_state->value, "membersVersion", members_version))
            state_.set_members_version(members_version);
    }

    *persons_ = parse_persons(_node_event_data);
    if (const auto it = _node_event_data.FindMember("tail"); it != _node_event_data.MemberEnd() && it->value.IsObject() && is_message_parsing_enabled())
    {
        const int64_t older_msg_id = get_older_msg_id(it->value);

        if (!parse_history_messages_json(it->value, older_msg_id, aimid_, *tail_messages_, *persons_, message_order::reverse))
        {
            __TRACE(
                "delivery",
                "%1%",
                "failed to parse incoming tail dlg state messages");

            return wpie_error_parse_response;
        }
    }

    if (const auto it = _node_event_data.FindMember("intro"); it != _node_event_data.MemberEnd() && it->value.IsObject() && is_message_parsing_enabled())
    {
        const int64_t older_msg_id = get_older_msg_id(it->value);

        if (!parse_history_messages_json(it->value, older_msg_id, aimid_, *intro_messages_, *persons_, message_order::direct))
        {
            __TRACE(
                "delivery",
                "%1%",
                "failed to parse incoming intro dlg state messages");

            return wpie_error_parse_response;
        }
    }


    if (const auto it = persons_->find(aimid_); it != persons_->end())
    {
        state_.set_friendly(it->second.friendly_);
        state_.set_official(it->second.official_.value_or(false));
    }

    if (const auto iter_heads = _node_event_data.FindMember("lastMessageHeads"); iter_heads != _node_event_data.MemberEnd() && iter_heads->value.IsArray())
        state_.set_heads(parse_heads(iter_heads->value, *persons_));

    if (const auto it = _node_event_data.FindMember("parentTopic"); it != _node_event_data.MemberEnd() && it->value.IsObject())
    {
        archive::thread_parent_topic topic;
        topic.unserialize(it->value);
        state_.set_parent_topic(std::move(topic));
    }

    if constexpr (::build::is_debug())
    {
        __INFO(
            "delete_history",
            "incoming dlg state event\n"
            "    contact=<%1%>\n"
            "    dlg-patch=<%2%>\n"
            "    del-up-to=<%3%>",
            aimid_ % state_.get_dlg_state_patch_version() % state_.get_del_up_to()
            );

        __TRACE(
            "delivery",
            "parsed incoming tail dlg state event\n"
            "    size=<%1%>\n"
            "    last_msgid=<%2%>",
            tail_messages_->size() %
            state_.get_last_msgid());

        __TRACE(
            "delivery",
            "parsed incoming intro dlg state event\n"
            "    size=<%1%>\n"
            "    last_msgid=<%2%>",
            intro_messages_->size() %
            state_.get_last_msgid());

        for (const auto &message : *tail_messages_)
        {
            (void)message;
            __TRACE(
                "delivery",
                "parsed incoming tail dlg state message\n"
                "    id=<%1%>",
                message->get_msgid());
        }

        for (const auto &message : *intro_messages_)
        {
            (void)message;
            __TRACE(
                "delivery",
                "parsed incoming intro dlg state message\n"
                "    id=<%1%>",
                message->get_msgid());
        }
    }

    return 0;
}

void fetch_event_dlg_state::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_dlg_state(this, _on_complete);
}
