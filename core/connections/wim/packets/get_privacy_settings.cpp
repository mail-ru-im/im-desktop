#include "stdafx.h"

#include "get_privacy_settings.h"
#include "../../../http_request.h"

namespace core::wim
{
    get_privacy_settings::get_privacy_settings(wim_packet_params _params)
        : robusto_packet(std::move(_params))
    {
    }

    const privacy_settings& get_privacy_settings::get_settings() const
    {
        return settings_;
    }

    int32_t get_privacy_settings::init_request(const std::shared_ptr<core::http_request_simple>& _request)
    {
        rapidjson::Document doc(rapidjson::Type::kObjectType);
        auto& a = doc.GetAllocator();
        setup_common_and_sign(doc, a, _request, "getPrivacySettings");

        if (!params_.full_log_)
        {
            log_replace_functor f;
            f.add_marker("a");
            f.add_json_marker("aimsid", aimsid_range_evaluator());
            _request->set_replace_log_function(f);
        }

        return 0;
    }

    int32_t get_privacy_settings::parse_results(const rapidjson::Value& _data)
    {
        return settings_.unserialize(_data);
    }
}