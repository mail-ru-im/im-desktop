#include "stdafx.h"
#include "robusto_packet.h"

#include <sstream>

#include "../../http_request.h"
#include "../../tools/hmac_sha_base64.h"
#include "../../utils.h"
#include "../../tools/system.h"
#include "../urls_cache.h"

using namespace core;
using namespace wim;

static robusto_packet_params make_robusto_params()
{
    static std::atomic<uint32_t> rubusto_req_id(0);

    return robusto_packet_params(++rubusto_req_id);
}


robusto_packet::robusto_packet(wim_packet_params params)
    : wim_packet(std::move(params))
    , robusto_params_(make_robusto_params())
{

}

robusto_packet::~robusto_packet()
{

}

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
    time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    std::stringstream ss;
    ss << robusto_params_.robusto_req_id_ << '-' << (uint32_t) ts;

    return ss.str();
}


int32_t robusto_packet::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    uint32_t size = _response->available();
    load_response_str((const char*) _response->read(size), size);
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


int32_t robusto_packet::on_response_error_code()
{
    if (40200 <= status_code_ && status_code_ < 40300)
        return wpie_error_need_relogin;

    if (status_code_ >= 40501 && status_code_ <= 40504)//rate limits
        return wpie_error_robuso_too_fast_sending;

    if (status_code_ == 50000)
        return wpie_robusto_timeout;

    return wpie_error_message_unknown;
}

int32_t robusto_packet::execute_request(std::shared_ptr<core::http_request_simple> _request)
{
    if (!_request->post())
        return wpie_network_error;

    http_code_ = (uint32_t)_request->get_response_code();

    if (http_code_ >= 500 && http_code_ < 600)
        return wpie_error_resend;

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}

void robusto_packet::execute_request_async(std::shared_ptr<http_request_simple> _request, wim_packet::handler_t _handler)
{
    auto wr_this = weak_from_this();

    auto ptr_this = wr_this.lock();
    if (!ptr_this)
        return;

    _request->post_async([_request, _handler, wr_this](curl_easy::completion_code _completion_code)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_completion_code != curl_easy::completion_code::success)
        {
            if (_handler)
                _handler(wpie_network_error);
            return;
        }

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

void robusto_packet::sign_packet(
    rapidjson::Value& _node,
    rapidjson_allocator& _a,
    std::shared_ptr<core::http_request_simple> _request)
{
    _node.AddMember("aimsid", params_.aimsid_, _a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    _node.Accept(writer);

    std::string json_string = rapidjson_get_string(buffer);

    char sha_buffer[65] = { 0 };
    core::tools::sha256(json_string, sha_buffer);

    std::map<std::string, std::string> params;

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;
    const auto host = urls::get_url(urls::url_type::rapi_host);

    params["a"] = escape_symbols(params_.a_token_);
    params["k"] = params_.dev_id_;
    params["ts"] = std::to_string((int64_t) ts);
    params["client"] = core::utils::get_client_string();
    params["lang"] = g_core->get_locale();
    params["body_checksum"] = sha_buffer;

    auto sha256 = escape_symbols(get_url_sign(host, params, params_, true));
    params["sig_sha256"] = std::move(sha256);

    std::stringstream ss_url;
    ss_url << host << '?' << format_get_params(params);

    _request->set_url(ss_url.str());

    _request->set_custom_header_param("Content-Type: application/json;charset=utf-8");

    _request->push_post_parameter(std::move(json_string), std::string());
}
