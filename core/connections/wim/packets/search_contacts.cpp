#include "stdafx.h"
#include "search_contacts.h"

#include "../../../http_request.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

search_contacts::search_contacts(wim_packet_params _packet_params, const std::string_view _keyword, const std::string_view _phone, const bool _hide_keyword)
    : robusto_packet(std::move(_packet_params))
    , keyword_(_keyword)
    , phone_(_phone)
    , hide_keyword_(_hide_keyword)
{
}

const std::shared_ptr<core::archive::persons_map>& search_contacts::get_persons() const
{
    return response_.get_persons();
}

std::string_view search_contacts::get_method() const
{
    return "search";
}

int core::wim::search_contacts::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t search_contacts::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    if (keyword_.empty() && phone_.empty())
        return -1;

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    if (!phone_.empty())
        node_params.AddMember("phonenum", phone_, a);
    else
        node_params.AddMember("keyword", keyword_, a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method());

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        if (hide_keyword_)
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
