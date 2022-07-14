#include "stdafx.h"
#include "http_request.h"
#include "get_thread_info.h"
#include "../../../../common.shared/json_helper.h"
#include "../log_replace_functor.h"

using namespace core::wim;


get_thread_info::get_thread_info(wim_packet_params _params, get_thread_info_params _thread_info_params)
    : robusto_packet(std::move(_params))
    , params_(_thread_info_params)
{
}

std::string_view get_thread_info::get_method() const
{
    return "thread/get";
}

int core::wim::get_thread_info::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t get_thread_info::init_request(const std::shared_ptr<http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("threadId", params_.thread_id_, a);
    doc.AddMember("params", std::move(node_params), a);
    setup_common_and_sign(doc, a, _request, get_method());

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_message_markers();
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_thread_info::parse_results(const rapidjson::Value& _node_results)
{
    result_.unserialize(_node_results);
    return 0;
}
