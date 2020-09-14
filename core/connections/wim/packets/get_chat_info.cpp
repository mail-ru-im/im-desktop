#include "stdafx.h"
#include "get_chat_info.h"

#include "../../../http_request.h"

using namespace core;
using namespace wim;

constexpr int32_t max_members_count = 150;

get_chat_info::get_chat_info(wim_packet_params _params, get_chat_info_params _chat_params)
    :   robusto_packet(std::move(_params)),
        params_(std::move(_chat_params))
{
}


get_chat_info::~get_chat_info()
{

}

int32_t get_chat_info::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    if (!params_.stamp_.empty())
        node_params.AddMember("stamp", params_.stamp_, a);
    else if (!params_.aimid_.empty())
        node_params.AddMember("sn", params_.aimid_, a);

    node_params.AddMember("memberLimit", params_.members_limit_ ? params_.members_limit_ : max_members_count, a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, "getChatInfo");

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_chat_info::parse_results(const rapidjson::Value& _node_results)
{
    if (result_.unserialize(_node_results) != 0)
        return wpie_http_parse_response;

    return 0;
}

int32_t get_chat_info::on_response_error_code()
{
    if (status_code_ == 40000)
        return wpie_error_profile_not_loaded;
    else if (status_code_ == 40001)
        return wpie_error_robusto_you_are_not_chat_member;
    else if (status_code_ == 40002)
        return wpie_error_robusto_you_are_blocked;

    return robusto_packet::on_response_error_code();
}
