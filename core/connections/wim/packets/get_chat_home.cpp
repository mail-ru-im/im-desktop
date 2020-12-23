#include "stdafx.h"
#include "get_chat_home.h"

#include "../../../http_request.h"
#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;


get_chat_home::get_chat_home(wim_packet_params _params, const std::string& _new_tag)
    :    robusto_packet(std::move(_params))
    ,   new_tag_(_new_tag)
    ,   need_restart_(false)
    ,   finished_(false)
{
}


get_chat_home::~get_chat_home() = default;

std::string_view get_chat_home::get_method() const
{
    return "getChatHome";
}

int32_t get_chat_home::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    if (!new_tag_.empty())
        node_params.AddMember("tag", new_tag_, a);
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

int32_t get_chat_home::parse_results(const rapidjson::Value& _node_results)
{
    const auto end = _node_results.MemberEnd();
    const auto iter_restart = _node_results.FindMember("restart");
    if (iter_restart != end && iter_restart->value.IsBool())
        need_restart_ = iter_restart->value.GetBool();

    if (need_restart_)
        return 0;

    const auto iter_finished = _node_results.FindMember("finish");
    if (iter_finished != end && iter_finished->value.IsBool())
        finished_ = iter_finished->value.GetBool();

    const auto iter_new_tag = _node_results.FindMember("newTag");
    if (iter_new_tag != end && iter_new_tag->value.IsString())
        result_tag_ = rapidjson_get_string(iter_new_tag->value);

    const auto iter_chats = _node_results.FindMember("chats");
    if (iter_chats != end && iter_chats->value.IsArray())
    {
        for (auto iter_chat = iter_chats->value.Begin(), chats_end = iter_chats->value.End(); iter_chat != chats_end; ++iter_chat)
        {
            const auto &node = *iter_chat;
            chat_info info;
            if (info.unserialize(node) != 0)
                return wpie_http_parse_response;
            result_.push_back(std::move(info));
        }
    }

    return 0;
}
