#include "stdafx.h"

#include "get_smartreplies.h"

#include "../../../http_request.h"
#include "../../../smartreply/smartreply_suggest.h"
#include "../../../../common.shared/smartreply/smartreply_types.h"
#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

get_smartreplies::get_smartreplies(wim_packet_params _params, std::string _aimid, int64_t _msgid, std::vector<smartreply::type> _types)
    : robusto_packet(std::move(_params))
    , aimid_(std::move(_aimid))
    , msgid_(_msgid)
    , types_(std::move(_types))
{
    assert(!aimid_.empty());
    assert(msgid_ >= 0);
    assert(!types_.empty());
}

int32_t get_smartreplies::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    assert(_request);

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);
    node_params.AddMember("msgId", msgid_, a);

    rapidjson::Value type_params(rapidjson::Type::kArrayType);
    type_params.Reserve(types_.size(), a);
    for (const auto type : types_)
        type_params.PushBack(tools::make_string_ref(smartreply::type_2_string(type)), a);
    node_params.AddMember("suggestTypes", std::move(type_params), a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, "getSmartReply");

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_smartreplies::parse_results(const rapidjson::Value& _data)
{
    for (const auto type : types_)
    {
        auto res = smartreply::unserialize_suggests_node(_data, type, aimid_, msgid_);
        if (!res.empty())
            std::move(res.begin(), res.end(), std::back_inserter(suggests_));
    }

    return 0;
}