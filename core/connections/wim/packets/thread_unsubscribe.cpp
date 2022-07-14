#include "stdafx.h"

#include "thread_unsubscribe.h"

#include "../common.shared/json_helper.h"
#include "http_request.h"
#include "../log_replace_functor.h"

using namespace core::wim;


thread_unsubscribe::thread_unsubscribe(wim_packet_params _params, std::string_view _thread_id)
    : robusto_packet(std::move(_params))
    , thread_id_(_thread_id.cbegin(), _thread_id.cend())
{
}

std::string_view thread_unsubscribe::get_method() const
{
    return "thread/unsubscribe";
}

int core::wim::thread_unsubscribe::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t thread_unsubscribe::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("threadId", thread_id_, a);
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

int32_t thread_unsubscribe::parse_results(const rapidjson::Value& _node_results)
{
    return 0;
}
