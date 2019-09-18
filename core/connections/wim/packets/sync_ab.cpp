#include "stdafx.h"
#include "sync_ab.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"

#include "../../urls_cache.h"

using namespace core;
using namespace wim;

core::wim::sync_ab::sync_ab(wim_packet_params _packet_params, const std::string& _keyword, const std::vector<std::string>& _phone)
    : robusto_packet(std::move(_packet_params))
    , keyword_(_keyword)
    , phone_(_phone)
    , status_code_(0)
{
}

int32_t core::wim::sync_ab::get_status_code()
{
    return status_code_;
}

int32_t core::wim::sync_ab::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    if (keyword_.empty() && phone_.empty())
        return -1;

    constexpr char method[] = "addressBookSync";

    _request->set_url(urls::get_url(urls::url_type::rapi_host));
    _request->set_normalized_url(method);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    doc.AddMember("method", method, a);
    doc.AddMember("reqId", get_req_id(), a);

    rapidjson::Value node_update(rapidjson::Type::kArrayType);

    rapidjson::Value member_update(rapidjson::Type::kObjectType);

    member_update.AddMember("name", keyword_, a);

    if (!phone_.empty())
    {
        rapidjson::Value phones(rapidjson::Type::kArrayType);
        phones.Reserve(phone_.size(), a);
        for (const auto &phone : phone_)
        {
            rapidjson::Value member_params(rapidjson::Type::kStringType);
            member_params.SetString(phone.c_str(), phone.size());
            phones.PushBack(std::move(member_params), a);
        }
        member_update.AddMember("phoneList", std::move(phones), a);
    }

    node_update.PushBack(std::move(member_update), a);
    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("update", node_update, a);
    doc.AddMember("params", node_params, a);

    sign_packet(doc, a, _request);

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        f.add_json_marker("keyword");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t core::wim::sync_ab::execute_request(std::shared_ptr<core::http_request_simple> request)
{
    if (!request->post())
        return wpie_network_error;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}

int32_t core::wim::sync_ab::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    uint32_t size = _response->available();
    load_response_str((const char*)_response->read(size), size);
    try
    {
        rapidjson::Document doc;
        if (doc.Parse(response_str().c_str()).HasParseError())
            return wpie_error_parse_response;

        auto iter_status = doc.FindMember("status");
        if (iter_status == doc.MemberEnd())
            return wpie_error_parse_response;

        auto iter_code = iter_status->value.FindMember("code");
        if (iter_code == iter_status->value.MemberEnd())
            return wpie_error_parse_response;

        status_code_ = iter_code->value.GetUint();

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
