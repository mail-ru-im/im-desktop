#include "stdafx.h"
#include "group_inviteblacklist_search.h"

#include "../../../http_request.h"
#include "../../../../common.shared/json_helper.h"

using namespace core;
using namespace wim;

group_inviteblacklist_search::group_inviteblacklist_search(wim_packet_params _params, std::string_view _keyword, std::string_view _cursor, uint32_t _page_size)
    : robusto_packet(std::move(_params))
    , keyword_(_keyword)
    , cursor_(_cursor)
    , page_size_(_page_size)
    , persons_(std::make_shared<core::archive::persons_map>())
{
    im_assert(page_size_ > 0);
}

group_inviteblacklist_search::~group_inviteblacklist_search() = default;

std::string_view group_inviteblacklist_search::get_method() const
{
    return is_search_request() ? "privacy/groups/inviteBlacklist/search" : "privacy/groups/inviteBlacklist/get";
}

int32_t group_inviteblacklist_search::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("pageSize", page_size_, a);
    if (!cursor_.empty())
        node_params.AddMember("cursor", cursor_, a);
    if (!keyword_.empty())
        node_params.AddMember("keyword", keyword_, a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method());

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("keyword");
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t group_inviteblacklist_search::parse_results(const rapidjson::Value& _node_results)
{
    if (get_status_code() == robusto_protocol_error::reset_page)
        reset_pages_ = true;

    tools::unserialize_value(_node_results, "cursor", cursor_);

    if (const auto it = _node_results.FindMember("blacklist"); it != _node_results.MemberEnd() && it->value.IsArray())
    {
        contacts_.reserve(it->value.Size());
        for (const auto& node : it->value.GetArray())
        {
            if (node.IsString())
                contacts_.push_back(rapidjson_get_string(node));
        }
    }

    *persons_ = parse_persons(_node_results);

    return 0;
}

bool group_inviteblacklist_search::is_status_code_ok() const
{
    return get_status_code() == robusto_protocol_error::reset_page || robusto_packet::is_status_code_ok();
}

bool group_inviteblacklist_search::is_search_request() const
{
    return !keyword_.empty();
}