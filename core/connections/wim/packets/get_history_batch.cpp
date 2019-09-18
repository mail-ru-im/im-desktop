#include "stdafx.h"
#include "get_history_batch.h"

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

get_history_batch_params::get_history_batch_params(const std::string_view _aimid, std::vector<int64_t>&& _ids, const std::string_view _patch_version)
    : aimid_(_aimid)
    , ids_(std::move(_ids))
    , patch_version_(_patch_version)
    , count_early_(0)
    , count_after_(0)
    , seq_(0)
    , is_context_request_(false)
{
    assert(!aimid_.empty());
    assert(!patch_version_.empty());
    assert(!ids_.empty());
}

get_history_batch_params::get_history_batch_params(const std::string_view _aimid, const std::string_view _patch_version, const int64_t _id, const int32_t _count_early, const int32_t _count_after, int64_t _seq)
    : aimid_(_aimid)
    , ids_({ _id })
    , patch_version_(_patch_version)
    , count_early_(_count_early)
    , count_after_(_count_after)
    , seq_(_seq)
    , is_context_request_(true)
{
    assert(!aimid_.empty());
    assert(!patch_version_.empty());
    assert(count_early_ > 0 || count_after_ > 0);
}

get_history_batch::get_history_batch(wim_packet_params _params, get_history_batch_params&& _hist_params, std::string&& _locale)
    : robusto_packet(std::move(_params))
    , hist_params_(std::move(_hist_params))
    , messages_(std::make_shared<archive::history_block>())
    , dlg_state_(std::make_shared<archive::dlg_state>())
    , locale_(std::move(_locale))
    , unpinned_(false)
    , persons_(std::make_shared<core::archive::persons_map>())
{
}

get_history_batch::~get_history_batch()
{

}

int32_t get_history_batch::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    constexpr char method[] = "getHistoryBatch";

    _request->set_gzip(true);
    _request->set_normalized_url(method);
    _request->set_keep_alive();
    _request->set_priority(priority_protocol());

    rapidjson::Document doc(rapidjson::Type::kObjectType);

    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("sn", hist_params_.aimid_, a);

    node_params.AddMember("patchVersion", hist_params_.patch_version_, a);
    if (!locale_.empty())
        node_params.AddMember("lang", locale_, a);

    rapidjson::Value ment_params(rapidjson::Type::kObjectType);
    ment_params.AddMember("resolve", false, a);
    node_params.AddMember("mentions", std::move(ment_params), a);

    rapidjson::Value subreqs(rapidjson::Type::kArrayType);
    subreqs.Reserve(hist_params_.ids_.size(), a);

    if (hist_params_.is_message_context_params())
    {
        for (auto cnt : { -hist_params_.count_early_, hist_params_.count_after_ })
        {
            if (cnt == 0)
                continue;

            auto from_id = hist_params_.ids_.front();
            if (cnt > 0)
            {
                --from_id;
                ++cnt;
            }

            rapidjson::Value subreq(rapidjson::Type::kObjectType);
            subreq.AddMember("fromMsgId", from_id, a);
            subreq.AddMember("count", cnt, a);

            subreq.AddMember("subreqId", tools::system::generate_guid() + (cnt > 0 ? 'd' : 'r'), a);

            subreqs.PushBack(std::move(subreq), a);
        }
    }
    else
    {
        for (auto id : hist_params_.ids_)
        {
            rapidjson::Value subreq(rapidjson::Type::kObjectType);
            subreq.AddMember("fromMsgId", id - 1, a);
            subreq.AddMember("count", 1, a);

            subreq.AddMember("subreqId", tools::system::generate_guid() + 'r', a);

            subreqs.PushBack(std::move(subreq), a);
        }
    }
    node_params.AddMember("subreqs", std::move(subreqs), a);

    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        f.add_json_marker("text");
        f.add_json_marker("message");
        f.add_json_marker("preview_url");
        f.add_json_marker("url");
        f.add_json_marker("snippet");
        f.add_json_marker("title");
        f.add_json_marker("caption");
        f.add_marker("a");
        _request->set_replace_log_function(std::move(f));
    }

    return 0;
}


int32_t get_history_batch::parse_results(const rapidjson::Value& _node_results)
{
    if (int32_t unreads = 0; tools::unserialize_value(_node_results, "unreadCnt", unreads))
        dlg_state_->set_unread_count(unreads);

    if (int32_t unreads = 0; tools::unserialize_value(_node_results, "unreadMentionMeCount", unreads))
        dlg_state_->set_unread_mentions_count(unreads);

    if (int64_t id = 0; tools::unserialize_value(_node_results, "lastMsgId", id))
        dlg_state_->set_last_msgid(id);

    if (const auto it = _node_results.FindMember("yours"); it != _node_results.MemberEnd())
    {
        if (int64_t id = 0; tools::unserialize_value(it->value, "lastRead", id))
            dlg_state_->set_yours_last_read(id);

        if (int64_t id = 0; tools::unserialize_value(it->value, "lastReadMention", id))
            dlg_state_->set_last_read_mention(id);
    }

    if (const auto iter_theirs = _node_results.FindMember("theirs"); iter_theirs != _node_results.MemberEnd())
    {
        if (int64_t id = 0; tools::unserialize_value(iter_theirs->value, "lastDelivered", id))
            dlg_state_->set_theirs_last_delivered(id);

        if (int64_t id = 0; tools::unserialize_value(iter_theirs->value, "lastRead", id))
            dlg_state_->set_theirs_last_read(id);
    }

    if (std::string patch_version; tools::unserialize_value(_node_results, "patchVersion", patch_version))
    {
        assert(!patch_version.empty());
        if (!patch_version.empty())
            dlg_state_->set_history_patch_version(common::tools::patch_version(std::move(patch_version)));
    }

    __INFO(
        "delete_history",
        "parsing incoming history from a server\n"
        "    contact=<%1%>\n"
        "    history-patch=<%2%>",
        hist_params_.aimid_ % dlg_state_->get_history_patch_version().as_string()
    );

    if (const auto it = _node_results.FindMember("patch"); it != _node_results.MemberEnd())
        history_patches_ = parse_patches_json(it->value);

    *persons_ = parse_persons(_node_results);

    if (const auto iter_heads = _node_results.FindMember("lastMessageHeads"); iter_heads != _node_results.MemberEnd() && iter_heads->value.IsArray())
        dlg_state_->set_heads(parse_heads(iter_heads->value, *persons_));

    if (const auto iter_subreqs = _node_results.FindMember("subreqs"); iter_subreqs != _node_results.MemberEnd() && iter_subreqs->value.IsArray())
    {
        messages_->reserve(iter_subreqs->value.Size());

        for (auto it = iter_subreqs->value.Begin(), end = iter_subreqs->value.End(); it != end; ++it)
        {
            const auto& subreq = *it;

            int64_t older_msgid = -1;
            tools::unserialize_value(subreq, "olderMsgId", older_msgid);

            std::string reqid;
            tools::unserialize_value(subreq, "subreqId", reqid);

            const auto msg_order = (!reqid.empty() && reqid.back() == 'd') ? message_order::direct : message_order::reverse;

            if (!parse_history_messages_json(subreq, older_msgid, hist_params_.aimid_, *messages_, *persons_, msg_order))
                return wpie_http_parse_response;
        }
    }

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

    set_last_message(*messages_, *dlg_state_);

    apply_patches(history_patches_, *messages_, unpinned_, *dlg_state_);

    return 0;
}
