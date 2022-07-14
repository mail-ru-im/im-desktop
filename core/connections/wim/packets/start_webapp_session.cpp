#include "stdafx.h"

#include "start_webapp_session.h"

#include "../common.shared/json_helper.h"
#include "http_request.h"
#include "../log_replace_functor.h"

namespace core::wim
{
    start_webapp_session::start_webapp_session(wim_packet_params _params, std::string_view _miniapp_id)
        : robusto_packet(std::move(_params))
        , id_(_miniapp_id)
    {
        im_assert(!id_.empty());
    }

    std::string_view start_webapp_session::get_method() const
    {
        return "webapp/session/start";
    }

    int start_webapp_session::minimal_supported_api_version() const
    {
        return core::urls::api_version::instance().minimal_supported();
    }

    int32_t start_webapp_session::init_request(const std::shared_ptr<core::http_request_simple>& _request)
    {
        rapidjson::Document doc(rapidjson::Type::kObjectType);
        auto& a = doc.GetAllocator();

        rapidjson::Value node_params(rapidjson::Type::kObjectType);
        node_params.AddMember("miniAppId", id_, a);
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

    int32_t start_webapp_session::parse_results(const rapidjson::Value& _node_results)
    {
        if (!tools::unserialize_value(_node_results, "aimsid", aimsid_))
            return wpie_error_parse_response;
        return 0;
    }

    int32_t start_webapp_session::on_response_error_code()
    {
        if (status_code_ == 40300)
            return wpie_error_miniapp_unavailable;

        return robusto_packet::on_response_error_code();
    }
}