#include "stdafx.h"

#include "set_privacy_settings.h"
#include "../../urls_cache.h"
#include "../../../http_request.h"

namespace core::wim
{
    set_privacy_settings::set_privacy_settings(wim_packet_params _params, privacy_settings _settings)
        : robusto_packet(_params)
        , settings_(std::move(_settings))
    {
    }

    int32_t set_privacy_settings::init_request(std::shared_ptr<core::http_request_simple> _request)
    {
        constexpr char method[] = "updatePrivacySettings";

        _request->set_url(urls::get_url(urls::url_type::rapi_host));
        _request->set_normalized_url(method);
        _request->set_keep_alive();

        rapidjson::Document doc(rapidjson::Type::kObjectType);
        auto& a = doc.GetAllocator();

        doc.AddMember("method", method, a);
        doc.AddMember("reqId", get_req_id(), a);

        rapidjson::Value node_params(rapidjson::Type::kObjectType);

        assert(settings_.has_values_set());
        settings_.serialize(node_params, a);

        doc.AddMember("params", std::move(node_params), a);

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
}