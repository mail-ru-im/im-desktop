#include "stdafx.h"

#include "get_sessions_list.h"
#include "../../../http_request.h"
#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

get_sessions_list::get_sessions_list(wim_packet_params _params)
    : robusto_packet(std::move(_params))
{
}

int32_t get_sessions_list::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();
    setup_common_and_sign(doc, a, _request, "session/list");

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_sessions_list::parse_results(const rapidjson::Value& _node_results)
{
    const auto it = _node_results.FindMember("sessions");
    if (it == _node_results.MemberEnd() || !it->value.IsArray())
        return wpie_error_parse_response;

    sessions_.reserve(it->value.Size());
    for (const auto& session : it->value.GetArray())
    {
        session_info info;
        info.unserialize(session);
        if (info.hash_.empty())
        {
            assert(false);
            continue;
        }

        sessions_.push_back(std::move(info));
    }

    assert(!sessions_.empty());
    assert(std::count_if(sessions_.begin(), sessions_.end(), [](const auto& s) { return s.is_current_; }) == 1);

    return sessions_.empty() ? -1 : 0;
}
