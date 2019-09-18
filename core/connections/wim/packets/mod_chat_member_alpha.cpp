#include "stdafx.h"
#include "mod_chat_member_alpha.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

#include "../../urls_cache.h"

using namespace core;
using namespace wim;

mod_chat_member_alpha::mod_chat_member_alpha(
    wim_packet_params _params,
    const std::string& _aimid,
    const std::string& _contact,
    const std::string& _role)
    : robusto_packet(std::move(_params))
    , aimid_(_aimid)
    , contact_(_contact)
    , role_(_role)
{
}

mod_chat_member_alpha::~mod_chat_member_alpha()
{
}

int32_t mod_chat_member_alpha::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    constexpr char method[] = "modChatMemberAlpha";

    _request->set_gzip(true);
    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method",method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);
    node_params.AddMember("memberSn", contact_, a);
    node_params.AddMember("role", role_, a);
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

int32_t mod_chat_member_alpha::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
