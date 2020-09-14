#include "stdafx.h"

#include "../../../log/log.h"
#include "../../../http_request.h"

#include "del_message_batch.h"

CORE_WIM_NS_BEGIN

del_message_batch::del_message_batch(
    wim_packet_params _params,
    std::vector<int64_t>&& _message_ids,
    const std::string &_contact_aimid,
    const bool _for_all
)
    : robusto_packet(std::move(_params))
    , message_ids_(std::move(_message_ids))
    , contact_aimid_(_contact_aimid)
    , for_all_(_for_all)
{
    assert(!contact_aimid_.empty());
}

int32_t del_message_batch::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    assert(_request);
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

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, "delMsgBatch");

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t del_message_batch::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

int32_t del_message_batch::on_response_error_code()
{
    return robusto_packet::on_response_error_code();
}

CORE_WIM_NS_END
