#include "stdafx.h"
#include "pin_message.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

pin_message::pin_message(wim_packet_params _params, const std::string& _aimId, const int64_t _message, const bool _is_unpin)
    : robusto_packet(std::move(_params))
    , aimid_(_aimId)
    , msg_id_(_message)
    , unpin_(_is_unpin)
{
}

std::string_view pin_message::get_method() const
{
    return "pinMessage";
}

int core::wim::pin_message::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t pin_message::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);
    node_params.AddMember("msgId", msg_id_, a);

    if (unpin_)
        node_params.AddMember("unpin", unpin_, a);

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

int32_t pin_message::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}