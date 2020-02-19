#include "stdafx.h"

#include "get_smartreply_suggests.h"

#include "../../../http_request.h"
#include "../../../smartreply/smartreply_suggest.h"
#include "../../../../common.shared/smartreply/smartreply_types.h"
#include "../../urls_cache.h"
#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

get_smartreply_suggests::get_smartreply_suggests(wim_packet_params _params, std::string _aimid, std::vector<smartreply::type> _types, std::string _text)
    : robusto_packet(std::move(_params))
    , aimid_(std::move(_aimid))
    , text_(std::move(_text))
    , types_(std::move(_types))
{
}

int32_t get_smartreply_suggests::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    assert(_request);
    assert(!aimid_.empty());
    assert(!types_.empty());

    constexpr char method[] = "getSuggests";

    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);

    rapidjson::Value type_params(rapidjson::Type::kArrayType);
    type_params.Reserve(types_.size(), a);
    for (const auto& type : types_)
    {
        rapidjson::Value node_type(rapidjson::Type::kObjectType);
        node_type.AddMember("type", tools::make_string_ref(smartreply::type_2_string(type)), a);
        node_type.AddMember("text", text_, a);
        type_params.PushBack(std::move(node_type), a);
    }

    node_params.AddMember("suggestTypes", std::move(type_params), a);

    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        f.add_json_marker("text");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_smartreply_suggests::parse_results(const rapidjson::Value& _data)
{
    const auto iter_suggest = _data.FindMember("suggests");

    if (iter_suggest == _data.MemberEnd() || !iter_suggest->value.IsArray())
        return wpie_error_parse_response;

    for (const auto& s : iter_suggest->value.GetArray())
    {
        auto res = smartreply::unserialize_suggests_node(s);
        if (!res.empty())
            std::move(res.begin(), res.end(), std::back_inserter(suggests_));
    }
    return 0;
}

