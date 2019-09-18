#include "stdafx.h"
#include "get_mentions_suggests.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../../archive/history_message.h"
#include "../wim_history.h"

#include "../../urls_cache.h"

using namespace core;
using namespace wim;

get_mentions_suggests::get_mentions_suggests(wim_packet_params _params, const std::string& _aimId, const std::string& _pattern)
    : robusto_packet(std::move(_params))
    , aimid_(_aimId)
    , pattern_(_pattern)
    , persons_(std::make_shared<core::archive::persons_map>())
{
}

const mention_suggest_vec& get_mentions_suggests::get_suggests() const
{
    return suggests_;
}

int32_t get_mentions_suggests::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    constexpr char method[] = "getRecentWriters";

    _request->set_gzip(true);
    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);

    if (!pattern_.empty())
        node_params.AddMember("keyword", pattern_, a);

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

int32_t get_mentions_suggests::parse_results(const rapidjson::Value& _data)
{
    *persons_ = parse_persons(_data);

    const auto writers = _data.FindMember("writers");
    if (writers == _data.MemberEnd() || !writers->value.IsArray())
        return wpie_error_parse_response;

    suggests_.reserve(writers->value.Size());
    for (const auto& writer_val : writers->value.GetArray())
    {
        const auto writer = rapidjson_get_string_view(writer_val);

        mention_suggest suggest;
        suggest.aimid_ = writer;

        const auto pers_it = persons_->find(writer);
        if (pers_it != persons_->end())
        {
            suggest.friendly_ = pers_it->second.friendly_;
            suggest.nick_ = pers_it->second.nick_;
        }
        else
        {
            suggest.friendly_ = writer;
        }

        suggests_.push_back(std::move(suggest));
    }

    return 0;
}