#include "stdafx.h"
#include "join_chat.h"

#include "../../../http_request.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;


join_chat::join_chat(wim_packet_params _params, const std::string& _stamp)
    : robusto_packet(std::move(_params))
    , stamp_(_stamp)
{
}

join_chat::~join_chat() = default;

std::string_view join_chat::get_method() const
{
    return "joinChat";
}

int core::wim::join_chat::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t join_chat::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("stamp", stamp_, a);
    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method());

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t join_chat::on_response_error_code()
{
    if (status_code_ == 40002)
        return wpie_error_user_blocked;

    return robusto_packet::on_response_error_code();
}
