#include "stdafx.h"
#include "create_chat.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

#include "../../urls_cache.h"

using namespace core;
using namespace wim;

create_chat::create_chat(
    wim_packet_params _params,
    const std::string& _aimid,
    const std::string& _chatName,
    std::vector<std::string> _chatMembers)
    : robusto_packet(std::move(_params))
    , aimid_(_aimid)
    , chat_members_(std::move(_chatMembers))
{
    chat_params_.set_name(_chatName);
}

create_chat::~create_chat()
{
}

const chat_params& create_chat::get_chat_params() const
{
    return chat_params_;
}

void create_chat::set_chat_params(chat_params _chat_params)
{
    chat_params_ = std::move(_chat_params);
}

int32_t create_chat::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    constexpr char method[] = "createChat";

    _request->set_gzip(true);
    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    {
        if (chat_params_.get_name())
            node_params.AddMember("name", *chat_params_.get_name(), a);
        node_params.AddMember("aimSid", get_params().aimsid_, a);
        if (chat_params_.get_avatar())
            node_params.AddMember("avatarId", *chat_params_.get_avatar(), a);
        if (!chat_members_.empty())
        {
            rapidjson::Value members_array(rapidjson::Type::kArrayType);
            members_array.Reserve(chat_members_.size(), a);
            for (const auto &sn: chat_members_)
            {
                rapidjson::Value member_params(rapidjson::Type::kObjectType);
                member_params.AddMember("sn", sn, a);
                members_array.PushBack(std::move(member_params), a);
            }
            node_params.AddMember("members", std::move(members_array), a);
        }
        if (chat_params_.get_about())
            node_params.AddMember("about", *chat_params_.get_about(), a);
        if (chat_params_.get_public())
            node_params.AddMember("public", *chat_params_.get_public(), a);
        if (chat_params_.get_join())
            node_params.AddMember("joinModeration", *chat_params_.get_join(), a);
        if (chat_params_.get_joiningByLink())
            node_params.AddMember("live", *chat_params_.get_joiningByLink(), a);
        if (chat_params_.get_readOnly())
            node_params.AddMember("defaultRole", std::string(*chat_params_.get_readOnly() ? "readonly" : "member"), a);
    }
    doc.AddMember("params", std::move(node_params), a);

    sign_packet(doc, a, _request);

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimSid", aimsid_range_evaluator());
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        f.add_marker("a");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t create_chat::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
