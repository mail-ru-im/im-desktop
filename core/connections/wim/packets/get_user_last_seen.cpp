#include "stdafx.h"
#include "get_user_last_seen.h"

#include "../../../http_request.h"
#include "../../../tools/json_helper.h"
#include "../lastseen.h"

using namespace core;
using namespace wim;


get_user_last_seen::get_user_last_seen(wim_packet_params _params, std::vector<std::string> _aimids)
    : robusto_packet(std::move(_params))
    , aimids_(std::move(_aimids))
{
}

get_user_last_seen::~get_user_last_seen() = default;

int32_t get_user_last_seen::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    rapidjson::Value contacts_array(rapidjson::Type::kArrayType);
    contacts_array.Reserve(aimids_.size(), a);
    for (auto &sn : aimids_)
        contacts_array.PushBack(tools::make_string_ref(sn), a);

    node_params.AddMember("ids", std::move(contacts_array), a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, "getUserLastseen");

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_user_last_seen::parse_results(const rapidjson::Value& _node_results)
{
    const auto iter_entries = _node_results.FindMember("entries");
    if (iter_entries != _node_results.MemberEnd() && iter_entries->value.IsArray())
    {
        for (const auto& iter : iter_entries->value.GetArray())
        {
            if (std::string sn; tools::unserialize_value(iter, "sn", sn))
            {
                lastseen seen;
                seen.unserialize(iter);

                result_.insert({ std::move(sn), std::move(seen) });
            }
        }
    }

    return 0;
}

const std::map<std::string, lastseen>& get_user_last_seen::get_result() const
{
    return result_;
}
