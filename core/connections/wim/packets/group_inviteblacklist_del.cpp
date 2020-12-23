#include "stdafx.h"

#include "group_inviteblacklist_del.h"
#include "../../../http_request.h"
#include "tools/json_helper.h"

using namespace core;
using namespace wim;

group_inviteblacklist_del::group_inviteblacklist_del(wim_packet_params _params, std::vector<std::string> _contacts_to_del, bool _del_all)
    : robusto_packet(std::move(_params))
    , contacts_(std::move(_contacts_to_del))
    , all_(_del_all)
{
    assert(!contacts_.empty() || _del_all);
}

std::string_view group_inviteblacklist_del::get_method() const
{
    return "privacy/groups/inviteBlacklist/del";
}

int32_t group_inviteblacklist_del::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    if (all_)
    {
        node_params.AddMember("everyone", true, a);
    }
    else
    {
        rapidjson::Value users(rapidjson::Type::kArrayType);
        users.Reserve(contacts_.size(), a);
        for (const auto& c : contacts_)
            users.PushBack(tools::make_string_ref(c), a);
        node_params.AddMember("users", std::move(users), a);
    }
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