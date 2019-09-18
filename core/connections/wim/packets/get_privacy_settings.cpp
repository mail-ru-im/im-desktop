#include "stdafx.h"

#include "get_privacy_settings.h"
#include "../../urls_cache.h"
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

    int32_t get_privacy_settings::init_request(std::shared_ptr<core::http_request_simple> _request)
    {
        constexpr char method[] = "getPrivacySettings";

        _request->set_url(urls::get_url(urls::url_type::rapi_host));
        _request->set_normalized_url(method);
        _request->set_keep_alive();

        rapidjson::Document doc(rapidjson::Type::kObjectType);
        auto& a = doc.GetAllocator();

        doc.AddMember("method", method, a);
        doc.AddMember("reqId", get_req_id(), a);

        sign_packet(doc, a, _request);

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