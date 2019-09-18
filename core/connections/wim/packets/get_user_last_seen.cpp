#include "stdafx.h"
#include "get_user_last_seen.h"

#include "../../../http_request.h"
#include "../../../tools/json_helper.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;


get_user_last_seen::get_user_last_seen(wim_packet_params _params, const std::vector<std::string>& _aimids)
    :    robusto_packet(std::move(_params))
    ,   aimids_(_aimids)
{
}


get_user_last_seen::~get_user_last_seen()
{

}

int32_t get_user_last_seen::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    constexpr char method[] = "getUserLastseen";

    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    rapidjson::Value contacts_array(rapidjson::Type::kArrayType);
    contacts_array.Reserve(aimids_.size(), a);
    for (auto &sn : aimids_)
    {
        rapidjson::Value value(rapidjson::Type::kStringType);
        value.SetString(sn.c_str(), sn.size(), a);
        contacts_array.PushBack(std::move(value), a);
    }

    node_params.AddMember("ids", std::move(contacts_array), a);

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

int32_t get_user_last_seen::parse_results(const rapidjson::Value& _node_results)
{
    const auto iter_entries = _node_results.FindMember("entries");
    if (iter_entries != _node_results.MemberEnd() && iter_entries->value.IsArray())
    {
        for (const auto& iter : iter_entries->value.GetArray())
        {
            std::string sn;
            tools::unserialize_value(iter, "sn", sn);

            int32_t lastseen = -1;
            tools::unserialize_value(iter, "lastseen", lastseen);

            result_.insert(std::make_pair(std::move(sn), lastseen));
        }
    }

    return 0;
}

const std::map<std::string, time_t>& get_user_last_seen::get_result() const
{
    return result_;
}
