#include "stdafx.h"
#include "get_history.h"

#include "../../../http_request.h"

#include "../../../archive/history_message.h"
#include "../../../archive/history_patch.h"
#include "../../../archive/dlg_state.h"

#include "../../../log/log.h"

#include "../wim_history.h"
#include "../../../utils.h"
#include "../../../tools/system.h"
#include "../../../tools/json_helper.h"

#include "openssl/sha.h"

#include <boost/range/adaptor/reversed.hpp>


using namespace core;
using namespace wim;

get_history_params::get_history_params(
    const std::string &_aimid,
    const int64_t _from_msg_id,
    const int64_t _till_msg_id,
    const int32_t _count,
    const std::string &_patch_version,
    bool _init,
    bool /*_from_deleted*/,
    bool _from_editing,
    bool _from_search,
    int64_t _seq
    )
    : aimid_(_aimid)
    , till_msg_id_(_till_msg_id)
    , from_msg_id_(_from_msg_id)
    , seq_(_seq)
    , count_(_count)
    , patch_version_(_patch_version)
    , init_(_init)
    , from_editing_(_from_editing)
    , from_search_(_from_search)
{
    assert(!aimid_.empty());
    assert(!patch_version_.empty());
}

get_history::get_history(wim_packet_params _params, const get_history_params& _hist_params, const std::string& _locale)
    : robusto_packet(std::move(_params))
    , older_msgid_(-1)
    , hist_params_(_hist_params)
    , messages_(std::make_shared<archive::history_block>())
    , dlg_state_(std::make_shared<archive::dlg_state>())
    , locale_(_locale)
    , unpinned_(false)
    , persons_(std::make_shared<core::archive::persons_map>())
{
}

get_history::~get_history() = default;

int32_t get_history::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("sn", hist_params_.aimid_, a);
    node_params.AddMember("fromMsgId", hist_params_.from_msg_id_, a);

    if (hist_params_.till_msg_id_ > 0)
        node_params.AddMember("tillMsgId", hist_params_.till_msg_id_, a);

    node_params.AddMember("count", hist_params_.count_, a);
    node_params.AddMember("patchVersion", hist_params_.patch_version_, a);
    if (!locale_.empty())
        node_params.AddMember("lang", locale_ , a);

    rapidjson::Value ment_params(rapidjson::Type::kObjectType);
    ment_params.AddMember("resolve", false, a);
    node_params.AddMember("mentions", std::move(ment_params), a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, "getHistory");

    __INFO(
        "delete_history",
        "get history request initialized\n"
        "    contact=<%1%>\n"
        "    patch-version=<%2%>",
        hist_params_.aimid_ % hist_params_.patch_version_
    );

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        f.add_message_markers();
        f.add_marker("a");
        _request->set_replace_log_function(f);
    }

    return 0;
}


int32_t get_history::parse_results(const rapidjson::Value& _node_results)
{
    if (int32_t unreads = 0; tools::unserialize_value(_node_results, "unreadCnt", unreads))
        dlg_state_->set_unread_count(unreads);

    if (int32_t unreads = 0; tools::unserialize_value(_node_results, "unreadMentionMeCount", unreads))
        dlg_state_->set_unread_mentions_count(unreads);

    tools::unserialize_value(_node_results, "olderMsgId", older_msgid_);

    if (int64_t last_msgid = 0; tools::unserialize_value(_node_results, "lastMsgId", last_msgid))
        dlg_state_->set_last_msgid(last_msgid);

    if (const auto it = _node_results.FindMember("yours"); it != _node_results.MemberEnd())
    {
        if (int64_t last_read = 0; tools::unserialize_value(it->value, "lastRead", last_read))
            dlg_state_->set_yours_last_read(last_read);

        if (int64_t last_read = 0; tools::unserialize_value(it->value, "lastReadMention", last_read))
            dlg_state_->set_last_read_mention(last_read);
    }

    if (const auto iter_theirs = _node_results.FindMember("theirs"); iter_theirs != _node_results.MemberEnd())
    {
        int64_t theirs_last_delivered = 0;
        if (tools::unserialize_value(iter_theirs->value, "lastDelivered", theirs_last_delivered))
            dlg_state_->set_theirs_last_delivered(theirs_last_delivered);

        int64_t theirs_last_read = 0;
        if (tools::unserialize_value(iter_theirs->value, "lastRead", theirs_last_read))
            dlg_state_->set_theirs_last_read(theirs_last_read);
    }

    if (std::string patch_version; tools::unserialize_value(_node_results, "patchVersion", patch_version))
    {
        assert(!patch_version.empty());
        if (!patch_version.empty())
            dlg_state_->set_history_patch_version(std::move(patch_version));
    }

    __INFO(
        "delete_history",
        "parsing incoming history from a server\n"
        "    contact=<%1%>\n"
        "    history-patch=<%2%>",
        hist_params_.aimid_ % dlg_state_->get_history_patch_version().as_string()
    );

    if (const auto iter_patch = _node_results.FindMember("patch"); iter_patch != _node_results.MemberEnd())
        history_patches_ = parse_patches_json(iter_patch->value);

    *persons_ = parse_persons(_node_results);

    if (const auto iter_heads = _node_results.FindMember("lastMessageHeads"); iter_heads != _node_results.MemberEnd() && iter_heads->value.IsArray())
        dlg_state_->set_heads(parse_heads(iter_heads->value, *persons_));

    const auto parse_order = hist_params_.count_ > 0 ? message_order::direct : message_order::reverse;
    if (!parse_history_messages_json(_node_results, older_msgid_, hist_params_.aimid_, *messages_, *persons_, parse_order))
        return wpie_http_parse_response;

    if (const auto iter_person = persons_->find(hist_params_.aimid_); iter_person != persons_->end())
    {
        dlg_state_->set_friendly(iter_person->second.friendly_);
        dlg_state_->set_official(iter_person->second.official_.value_or(false));
    }

    if (archive::history_block pin_block; parse_history_messages_json(_node_results, -1, hist_params_.aimid_, pin_block, *persons_, message_order::reverse, "pinned"))
    {
        if (!pin_block.empty())
            dlg_state_->set_pinned_message(*pin_block.front());
    }

    if (const auto iter_no_recents_update = _node_results.FindMember("noRecentsUpdate"); iter_no_recents_update != _node_results.MemberEnd() && iter_no_recents_update->value.IsBool())
    {
        dlg_state_->set_no_recents_update(iter_no_recents_update->value.GetBool());
    }

    set_last_message(*messages_, *dlg_state_);

    apply_patches(history_patches_, *messages_, unpinned_, *dlg_state_);

    return 0;
}

priority_t get_history::get_priority() const
{
    return priority_protocol();
}
