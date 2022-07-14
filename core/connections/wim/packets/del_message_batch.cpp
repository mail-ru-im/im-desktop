#include "stdafx.h"

#include "../../../log/log.h"
#include "../../../http_request.h"
#include "../../../../common.shared/json_helper.h"
#include "../../../tools/features.h"
#include "../log_replace_functor.h"

#include "del_message_batch.h"

CORE_WIM_NS_BEGIN

del_message_batch::del_message_batch(
    wim_packet_params _params,
    std::vector<int64_t>&& _message_ids,
    const std::string &_contact_aimid,
    const bool _for_all,
    const bool _silent
)
    : robusto_packet(std::move(_params))
    , message_ids_(std::move(_message_ids))
    , contact_aimid_(_contact_aimid)
    , for_all_(_for_all)
    , silent_(_silent)
    , silent_responce_(false)
{
    im_assert(!contact_aimid_.empty());
}

int32_t del_message_batch::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    im_assert(_request);
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", contact_aimid_, a);

    rapidjson::Value node_ids(rapidjson::Type::kArrayType);
    node_ids.Reserve(message_ids_.size(), a);
    for (const auto& id : message_ids_)
    {
        rapidjson::Value node_id(rapidjson::Type::kStringType);
        node_id.SetUint64(id);
        node_ids.PushBack(std::move(node_id), a);
    }
    node_params.AddMember("msgIds", std::move(node_ids), a);

    node_params.AddMember("shared", for_all_, a);

    if (for_all_ && features::is_silent_delete_enabled())//they said not to send
        node_params.AddMember("silent", silent_, a);

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

int32_t del_message_batch::parse_results(const rapidjson::Value& _data)
{
    tools::unserialize_value(_data, "silent", silent_responce_);
    return 0;
}

int32_t del_message_batch::on_response_error_code()
{
    return robusto_packet::on_response_error_code();
}

std::string_view del_message_batch::get_method() const
{
    return "delMsgBatch";
}

int del_message_batch::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

CORE_WIM_NS_END
