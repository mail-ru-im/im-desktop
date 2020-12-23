#include "stdafx.h"

#include "../../../http_request.h"
#include "../../../tools/json_helper.h"
#include "archive/history_message.h"

#include "vote_in_poll.h"

namespace core::wim
{

vote_in_poll::vote_in_poll(wim_packet_params _params, const std::string& _poll_id, const std::string& _answer_id)
    : robusto_packet(std::move(_params)),
      poll_id_(_poll_id),
      answer_id_(_answer_id)
{

}

const archive::poll_data& vote_in_poll::get_result() const
{
    return poll_;
}

std::string_view vote_in_poll::get_method() const
{
    return "poll/vote";
}

int32_t vote_in_poll::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("id", poll_id_, a);
    node_params.AddMember("answerId", answer_id_, a);

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

int32_t vote_in_poll::parse_results(const rapidjson::Value& _node_results)
{
    poll_.unserialize(_node_results);
    poll_.id_ = poll_id_;
    poll_.my_answer_id_ = answer_id_;

    return 0;
}

}
