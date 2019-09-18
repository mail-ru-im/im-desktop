#include "stdafx.h"
#include "search_contacts.h"

#include "../../../http_request.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

search_contacts::search_contacts(wim_packet_params _packet_params, const std::string_view _keyword, const std::string_view _phone)
    : robusto_packet(std::move(_packet_params))
    , keyword_(_keyword)
    , phone_(_phone)
{
}

const std::shared_ptr<core::archive::persons_map>& search_contacts::get_persons() const
{
    return response_.get_persons();
}

int32_t search_contacts::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    if (keyword_.empty() && phone_.empty())
        return -1;

    constexpr char method[] = "search";

    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    if (!phone_.empty())
        node_params.AddMember("phonenum", phone_, a);
    else
        node_params.AddMember("keyword", keyword_, a);

    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        f.add_json_marker("keyword");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t search_contacts::parse_results(const rapidjson::Value& node)
{
    if (!response_.unserialize(node))
        return wpie_http_parse_response;

    return 0;
}
