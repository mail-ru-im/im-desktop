#include "stdafx.h"
#include "block_chat_member.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

#include "../../urls_cache.h"

#include "../../urls_cache.h"

using namespace core;
using namespace wim;

block_chat_member::block_chat_member(
    wim_packet_params _params,
    const std::string& _aimid,
    const std::string& _contact,
    bool _block)
    : robusto_packet(std::move(_params))
    , aimid_(_aimid)
    , contact_(_contact)
    , block_(_block)
{
}

block_chat_member::~block_chat_member()
{
}

int32_t block_chat_member::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    auto method = block_ ? std::string("blockChatMembers") : std::string("unblockChatMembers");

    _request->set_gzip(true);
    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", std::move(method), a);

    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);

    rapidjson::Value node_member(rapidjson::Type::kObjectType);
    node_member.AddMember("sn", contact_, a);

    rapidjson::Value node_members(rapidjson::Type::kArrayType);
    node_members.Reserve(1, a);
    node_members.PushBack(std::move(node_member), a);

    node_params.AddMember("members", std::move(node_members), a);
    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("authToken");
        f.add_marker("a");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t block_chat_member::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
