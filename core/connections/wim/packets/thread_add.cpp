#include "stdafx.h"

#include "thread_add.h"

#include "../common.shared/json_helper.h"
#include "http_request.h"

namespace core::wim
{
    thread_add::thread_add(wim_packet_params _params, archive::thread_parent_topic _parent_topic)
        : robusto_packet(std::move(_params))
        , topic_(std::move(_parent_topic))
        , persons_(std::make_shared<core::archive::persons_map>())
    {
    }

    std::string_view thread_add::get_method() const
    {
        return "thread/add";
    }

    int32_t thread_add::init_request(const std::shared_ptr<core::http_request_simple>& _request)
    {
        rapidjson::Document doc(rapidjson::Type::kObjectType);
        auto& a = doc.GetAllocator();

        rapidjson::Value node_params(rapidjson::Type::kObjectType);

        rapidjson::Value node_topic(rapidjson::Type::kObjectType);
        topic_.serialize(node_topic, a);
        node_params.AddMember("parentTopic", std::move(node_topic), a);

        doc.AddMember("params", std::move(node_params), a);

        setup_common_and_sign(doc, a, _request, get_method());

        if (!robusto_packet::params_.full_log_)
        {
            log_replace_functor f;
            f.add_json_marker("aimsid", aimsid_range_evaluator());
            _request->set_replace_log_function(f);
        }

        return 0;
    }

    int32_t thread_add::parse_results(const rapidjson::Value& _node_results)
    {
        if (!tools::unserialize_value(_node_results, "threadId", thread_id_))
            return wpie_error_parse_response;

        *persons_ = parse_persons(_node_results);

        return 0;
    }
}