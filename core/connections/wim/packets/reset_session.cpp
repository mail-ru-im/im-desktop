#include "stdafx.h"
#include "reset_session.h"
#include "../../../http_request.h"
#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

reset_session::reset_session(wim_packet_params _params, std::string_view _hash)
    : robusto_packet(std::move(_params))
    , hash_(_hash)
{
}

std::string_view reset_session::get_method() const
{
    return is_reset_all() ? "session/resetAll" : "session/reset";
}

int32_t reset_session::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    if (!is_reset_all())
    {
        rapidjson::Value node_params(rapidjson::Type::kObjectType);
        node_params.AddMember("hash", hash_, a);
        doc.AddMember("params", std::move(node_params), a);
    }

    setup_common_and_sign(doc, a, _request, get_method());

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t reset_session::on_response_error_code()
{
    if (!is_reset_all() && status_code_ == 40401)
        return wpie_error_invalid_session_hash;

    return robusto_packet::on_response_error_code();
}
