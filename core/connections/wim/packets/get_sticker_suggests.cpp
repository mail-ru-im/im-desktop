#include "stdafx.h"

#include "get_sticker_suggests.h"

#include "../../../http_request.h"
#include "../../../smartreply/smartreply_suggest.h"
#include "../../../../common.shared/smartreply/smartreply_types.h"
#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

constexpr auto suggest_type = smartreply::type::sticker_by_text;

get_sticker_suggests::get_sticker_suggests(wim_packet_params _params, std::string _aimid, std::string _text)
    : robusto_packet(std::move(_params))
    , aimid_(std::move(_aimid))
    , text_(std::move(_text))
{
}

int32_t get_sticker_suggests::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    assert(_request);
    assert(!aimid_.empty());

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);

    rapidjson::Value type_params(rapidjson::Type::kArrayType);
    rapidjson::Value node_type(rapidjson::Type::kObjectType);
    node_type.AddMember("type", tools::make_string_ref(smartreply::type_2_string(suggest_type)), a);
    node_type.AddMember("text", text_, a);
    type_params.PushBack(std::move(node_type), a);
    node_params.AddMember("suggestTypes", std::move(type_params), a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, "getSuggests");

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        f.add_json_marker("text");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_sticker_suggests::parse_results(const rapidjson::Value& _data)
{
    const auto iter_suggest = _data.FindMember("suggests");

    if (iter_suggest == _data.MemberEnd() || !iter_suggest->value.IsArray())
        return wpie_error_parse_response;

    std::vector<smartreply::suggest> smartreplies;
    smartreplies.reserve(iter_suggest->value.Size());
    for (const auto& s : iter_suggest->value.GetArray())
    {
        auto res = smartreply::unserialize_suggests_node(s);
        if (!res.empty())
            std::move(res.begin(), res.end(), std::back_inserter(smartreplies));
    }

    stickers_.reserve(smartreplies.size());
    for (const auto& s : smartreplies)
    {
        if (s.get_type() == suggest_type)
            stickers_.push_back(s.get_data());
    }

    return 0;
}

