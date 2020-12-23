#include "stdafx.h"

#include "create_conference.h"

#include "../../../http_request.h"
#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

create_conference::create_conference(wim_packet_params _params, std::string_view _name, conference_type _type, std::vector<std::string> _participants, bool _call_participants)
    : robusto_packet(std::move(_params))
    , name_(_name)
    , type_(_type)
    , participants_(std::move(_participants))
    , call_participants_(_call_participants)
    , expires_on_(-1)
{
}

std::string_view create_conference::get_method() const
{
    return "conference/create";
}

int32_t create_conference::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    assert(_request);
    assert(!name_.empty());

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("name", name_, a);
    auto type_name = get_conference_type_name(type_);
    node_params.AddMember("type", tools::make_string_ref(type_name), a);

    rapidjson::Value participants(rapidjson::Type::kArrayType);
    participants.Reserve(participants_.size(), a);
    for (const auto& p : participants_)
        participants.PushBack(tools::make_string_ref(p), a);
    node_params.AddMember("participants", std::move(participants), a);

    node_params.AddMember("callParticipants", call_participants_, a);

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

int32_t create_conference::parse_results(const rapidjson::Value& _data)
{
    if (!tools::unserialize_value(_data, "conferenceUrl", conference_url_) || conference_url_.empty())
        return wpie_error_parse_response;

    tools::unserialize_value(_data, "expiresOn", expires_on_);

    return 0;
}

