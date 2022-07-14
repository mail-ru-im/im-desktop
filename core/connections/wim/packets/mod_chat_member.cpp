#include "stdafx.h"
#include "mod_chat_member.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

mod_chat_member::mod_chat_member(
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

mod_chat_member::~mod_chat_member()
{
}

std::string_view mod_chat_member::get_method() const
{
    return "modChatMember";
}

int core::wim::mod_chat_member::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t mod_chat_member::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);
    node_params.AddMember("memberSn", contact_, a);
    node_params.AddMember("role", role_, a);
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

int32_t mod_chat_member::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
