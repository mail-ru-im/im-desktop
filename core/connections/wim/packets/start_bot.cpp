#include "stdafx.h"

#include "start_bot.h"

#include "../../../tools/json_helper.h"
#include "../../../http_request.h"

namespace core::wim
{
    start_bot::start_bot(wim_packet_params _params, std::string_view _nick, std::string_view _payload)
        : robusto_packet(std::move(_params))
        , nick_(_nick)
        , payload_(_payload)
    {
    }

    int32_t start_bot::init_request(const std::shared_ptr<core::http_request_simple>& _request)
    {
        rapidjson::Document doc(rapidjson::Type::kObjectType);
        auto& a = doc.GetAllocator();

        rapidjson::Value node_params(rapidjson::Type::kObjectType);

        node_params.AddMember("bot", nick_, a);
        node_params.AddMember("startParameter", payload_, a);

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

    std::string_view start_bot::get_method() const
    {
        return "startBot";
    }
}