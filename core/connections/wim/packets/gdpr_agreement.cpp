#include "stdafx.h"

#include "gdpr_agreement.h"

#include "../../../http_request.h"
#include "../../urls_cache.h"
#include "../log_replace_functor.h"


using namespace core;
using namespace wim;

gdpr_agreement::gdpr_agreement(wim_packet_params _params, agreement_state _state)
    : robusto_packet(std::move(_params))
    , state_(_state)
{
}

int32_t gdpr_agreement::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);

    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("name", "gdpr_pp", a);
    node_params.AddMember("enable", (state_ == agreement_state::accepted), a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method());

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t gdpr_agreement::get_response_error_code()
{
    switch (status_code_)
    {
    case 20000:
        return 0;
    case 40000:
        return wpie_error_invalid_request;
    case 40200:
        return wpie_error_robusto_token_invalid;
    }
    return robusto_packet::on_response_error_code();
}

std::string_view gdpr_agreement::get_method() const
{
    return "setUserAgreement";
}

int core::wim::gdpr_agreement::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}
