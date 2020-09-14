#include "stdafx.h"

#include "set_privacy_settings.h"
#include "../../../http_request.h"

namespace core::wim
{
    set_privacy_settings::set_privacy_settings(wim_packet_params _params, privacy_settings _settings)
        : robusto_packet(std::move(_params))
        , settings_(std::move(_settings))
    {
    }

    int32_t set_privacy_settings::init_request(const std::shared_ptr<core::http_request_simple>& _request)
    {
        rapidjson::Document doc(rapidjson::Type::kObjectType);
        auto& a = doc.GetAllocator();

        rapidjson::Value node_params(rapidjson::Type::kObjectType);

        assert(settings_.has_values_set());
        settings_.serialize(node_params, a);

        doc.AddMember("params", std::move(node_params), a);

        setup_common_and_sign(doc, a, _request, "updatePrivacySettings");

        if (!params_.full_log_)
        {
            log_replace_functor f;
            f.add_json_marker("aimsid", aimsid_range_evaluator());
            _request->set_replace_log_function(f);
        }

        return 0;
    }
}