#include "stdafx.h"

#include "../../../http_request.h"

#include "create_task.h"

namespace core::wim
{
    create_task::create_task(wim_packet_params _params, const core::tasks::task_data& _task)
        : robusto_packet(std::move(_params))
        , task_(_task)
    {

    }

    std::string_view create_task::get_method() const
    {
        return "add";
    }

    int32_t create_task::init_request(const std::shared_ptr<core::http_request_simple>& _request)
    {
        rapidjson::Document doc(rapidjson::Type::kObjectType);
        auto& a = doc.GetAllocator();
        
        rapidjson::Value node_params(rapidjson::Type::kObjectType);
        task_.serialize(node_params, a);
        doc.AddMember("params", std::move(node_params), a);
        
        setup_common_and_sign(doc, a, _request, get_method(), robusto_packet::use_aimsid::yes, urls::url_type::tasks_host);
        
        if (!robusto_packet::params_.full_log_)
        {
            log_replace_functor f;
            f.add_json_marker("aimsid", aimsid_range_evaluator());
            _request->set_replace_log_function(f);
        }

        return 0;
    }
}