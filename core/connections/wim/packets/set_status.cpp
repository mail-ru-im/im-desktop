#include "stdafx.h"

#include "set_status.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../../../common.shared/json_helper.h"
#include "../log_replace_functor.h"

namespace
{
    std::string_view statusType(std::string_view _status, std::string_view _description)
    {
        if (_status.empty())
            return "empty";
        else if (!_description.empty())
            return "emoji";
        else
            return "predefined";
    }
}

using namespace core;
using namespace wim;

wim::set_status::set_status(wim_packet_params _params, std::string_view _status, std::chrono::seconds _duration, std::string_view _description)
    : robusto_packet(std::move(_params))
    , status_(std::move(_status))
    , description_(_description)
    , duration_(_duration)
{
}

int32_t set_status::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("type", tools::make_string_ref(statusType(status_, description_)), a);
    if (!status_.empty())
        node_params.AddMember("media", status_, a);

    if (!description_.empty())
        node_params.AddMember("text", description_, a);

    if (duration_.count() > 0)
        node_params.AddMember("duration", duration_.count(), a);

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

std::string_view set_status::get_method() const
{
    return "status/set";
}

int core::wim::set_status::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}
