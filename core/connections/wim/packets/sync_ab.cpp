#include "stdafx.h"
#include "sync_ab.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;

core::wim::sync_ab::sync_ab(wim_packet_params _params, const std::string& _keyword, const std::vector<std::string>& _phone)
    : robusto_packet(std::move(_params))
    , keyword_(_keyword)
    , phone_(_phone)
    , status_code_(0)
{
}

int32_t core::wim::sync_ab::get_status_code()
{
    return status_code_;
}

int32_t core::wim::sync_ab::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    if (keyword_.empty() && phone_.empty())
        return -1;

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_update(rapidjson::Type::kArrayType);

    rapidjson::Value member_update(rapidjson::Type::kObjectType);

    member_update.AddMember("name", keyword_, a);

    if (!phone_.empty())
    {
        rapidjson::Value phones(rapidjson::Type::kArrayType);
        phones.Reserve(phone_.size(), a);
        for (const auto& phone : phone_)
            phones.PushBack(tools::make_string_ref(phone), a);
        member_update.AddMember("phoneList", std::move(phones), a);
    }

    node_update.PushBack(std::move(member_update), a);
    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("update", std::move(node_update), a);
    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, "addressBookSync");

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        f.add_json_marker("keyword");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t core::wim::sync_ab::execute_request(const std::shared_ptr<core::http_request_simple>& request)
{
    url_ = request->get_url();

    if (auto error_code = get_error(request->post()))
        return *error_code;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}

int32_t core::wim::sync_ab::parse_response(const std::shared_ptr<core::tools::binary_stream>& _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    auto size = _response->available();
    load_response_str((const char*)_response->read(size), size);
    try
    {
        rapidjson::Document doc;
        if (doc.Parse(response_str().c_str()).HasParseError())
            return wpie_error_parse_response;

        auto iter_status = doc.FindMember("status");
        if (iter_status == doc.MemberEnd())
            return wpie_error_parse_response;

        if (!tools::unserialize_value(iter_status->value, "code", status_code_))
            return wpie_error_parse_response;

        if (status_code_ == 20000)
        {
            auto iter_result = doc.FindMember("results");
            if (iter_result != doc.MemberEnd())
                return parse_results(iter_result->value);
        }
        else
        {
            return on_response_error_code();
        }
    }
    catch (...)
    {
        return 0;
    }

    return 0;
}
