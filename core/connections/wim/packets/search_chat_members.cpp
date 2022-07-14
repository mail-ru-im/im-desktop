#include "stdafx.h"
#include "search_chat_members.h"

#include "../../../http_request.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

search_chat_members::search_chat_members(wim_packet_params _params, std::string_view _aimid, std::string_view _role, std::string_view _keyword, uint32_t _page_size, std::string_view _cursor)
    : robusto_packet(std::move(_params))
    , aimid_(_aimid)
    , role_(_role)
    , keyword_(_keyword)
    , page_size_(_page_size)
    , cursor_(_cursor)
    , reset_pages_(false)
{
}

search_chat_members::~search_chat_members() = default;

std::string_view search_chat_members::get_method() const
{
    return "searchChatMembers";
}

int core::wim::search_chat_members::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t search_chat_members::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("id", aimid_, a);
    node_params.AddMember("pageSize", page_size_, a);

    if (cursor_.empty())
    {
        rapidjson::Value filter_params(rapidjson::Type::kObjectType);
        if (!role_.empty())
            filter_params.AddMember("role", role_, a);
        filter_params.AddMember("keyword", keyword_, a);
        node_params.AddMember("filter", std::move(filter_params), a);
    }
    else
    {
        node_params.AddMember("cursor", cursor_, a);
    }

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method());

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t search_chat_members::parse_results(const rapidjson::Value& _node_results)
{
    if (get_status_code() == robusto_protocol_error::reset_page)
        reset_pages_ = true;

    if (result_.unserialize(_node_results) != 0)
        return wpie_http_parse_response;

    return 0;
}

int32_t search_chat_members::on_response_error_code()
{
    if (status_code_ == 40001)
        return wpie_error_robusto_you_are_not_chat_member;
    else if (status_code_ == 40002)
        return wpie_error_robusto_you_are_blocked;

    return robusto_packet::on_response_error_code();
}

bool search_chat_members::is_status_code_ok() const
{
    return get_status_code() == robusto_protocol_error::reset_page || robusto_packet::is_status_code_ok();
}
