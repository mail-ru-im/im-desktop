#include "stdafx.h"
#include "robusto_packet.h"

#include "../../http_request.h"
#include "../../tools/hmac_sha_base64.h"
#include "../../utils.h"
#include "../../tools/system.h"
#include "../../tools/json_helper.h"
#include "../../tools/features.h"
#include "../urls_cache.h"
#include "../common.shared/string_utils.h"

using namespace core;
using namespace wim;

static robusto_packet_params make_robusto_params()
{
    static std::atomic<uint32_t> robusto_req_id(0);

    return robusto_packet_params(++robusto_req_id);
}


robusto_packet::robusto_packet(wim_packet_params params)
    : wim_packet(std::move(params))
    , robusto_params_(make_robusto_params())
{

}

robusto_packet::~robusto_packet() = default;

void robusto_packet::set_robusto_params(const robusto_packet_params& _params)
{
    robusto_params_ = _params;
}

int32_t robusto_packet::parse_results(const rapidjson::Value& /*_node_results*/)
{
    return 0;
}


std::string robusto_packet::get_req_id() const
{
    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    std::stringstream ss;
    ss << robusto_params_.robusto_req_id_ << '-' << ts;

    return ss.str();
}


int32_t robusto_packet::parse_response(const std::shared_ptr<core::tools::binary_stream>& _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    auto size = _response->available();
    load_response_str((const char*) _response->read(size), size);
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

        if (is_status_code_ok())
        {
            const auto iter_result = doc.FindMember("results");
            if (iter_result != doc.MemberEnd() && iter_result->value.IsObject())
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


int32_t robusto_packet::on_response_error_code()
{
    if (40200 <= status_code_ && status_code_ < 40300)
        return wpie_error_need_relogin;

    if (status_code_ >= 40501 && status_code_ <= 40504)//rate limits
        return wpie_error_robuso_too_fast_sending;

    if (status_code_ == 50000)
        return wpie_robusto_timeout;

    if (status_code_ == 40401)
        return wpie_error_robusto_target_not_found;

    return wpie_error_message_unknown;
}

int32_t robusto_packet::execute_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    url_ = _request->get_url();

    auto res = _request->post();
    if (res != curl_easy::completion_code::success)
    {
        if (res == curl_easy::completion_code::resolve_failed)
            return wpie_couldnt_resolve_host;

        return wpie_network_error;
    }

    http_code_ = (uint32_t)_request->get_response_code();

    if (http_code_ >= 500 && http_code_ < 600)
        return wpie_error_resend;

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}

void robusto_packet::execute_request_async(const std::shared_ptr<http_request_simple>& _request, wim_packet::handler_t _handler)
{
    auto wr_this = weak_from_this();

    auto ptr_this = wr_this.lock();
    if (!ptr_this)
        return;

    _request->post_async([_request, _handler = std::move(_handler), wr_this](curl_easy::completion_code _completion_code)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_completion_code != curl_easy::completion_code::success)
        {
            if (_handler)
                _handler(_completion_code == curl_easy::completion_code::resolve_failed ? wpie_couldnt_resolve_host : wpie_network_error);
            return;
        }

        ptr_this->set_url(_request->get_url());

        const auto http_code = (uint32_t)_request->get_response_code();

        ptr_this->set_http_code(http_code);

        if (http_code >= 500 && http_code < 600)
            _handler(wpie_error_resend);

        else if (http_code != 200)
            _handler(wpie_http_error);

        else
            _handler(0);
    });
}

void robusto_packet::setup_common_and_sign(rapidjson::Value& _node, rapidjson_allocator& _a, const std::shared_ptr<core::http_request_simple>& _request, std::string_view _method, use_aimsid _use_aimsid)
{
    if (_use_aimsid == use_aimsid::yes)
        _node.AddMember("aimsid", params_.aimsid_, _a);

    _node.AddMember("reqId", get_req_id(), _a);

    // for the best zstd-compression, json data should be sorted lexicographically
    if (features::is_zstd_request_enabled())
        tools::sort_json_keys_by_name(_node);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    _node.Accept(writer);

    std::string json_string = rapidjson_get_string(buffer);

    _request->set_url(su::concat(urls::get_url(urls::url_type::rapi_host), _method));
    _request->set_normalized_url(_method);

    _request->set_keep_alive();

    _request->set_custom_header_param("Content-Type: application/json;charset=utf-8");

    _request->push_post_parameter(std::move(json_string), std::string());

    if (!_request->is_compressed())
        _request->set_compression_auto();
}
