#include "stdafx.h"
#include "set_stickers_order.h"
#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../tools/system.h"
#include "../../../utils.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

set_stickers_order_packet::set_stickers_order_packet(wim_packet_params _params, const std::vector<int32_t> _values)
    :   wim_packet(std::move(_params)),
        values_(_values)
{
}


set_stickers_order_packet::~set_stickers_order_packet()
{
}

int32_t set_stickers_order_packet::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::map<std::string, std::string> params;

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    const std::string host(urls::get_url(urls::url_type::stickers_store_host));

    params["a"] = escape_symbols(params_.a_token_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["platform"] = utils::get_protocol_platform_string();

    for (unsigned int i = 0; i < values_.size(); ++i)
    {
        std::stringstream ss_value;
        ss_value << "priority[" << values_[i] << "]";

        params[escape_symbols(ss_value.str())] = std::to_string(i + 1);
    }

    _request->push_post_parameter("empty", "empty");  // to fix infinite waiting in case of post request without params

    params["ts"] = std::to_string((int64_t) ts);
    params["r"] = core::tools::system::generate_guid();

    params["client"] = core::utils::get_client_string();

    params["lang"] = g_core->get_locale();

    std::stringstream ss_url;
    ss_url << host << "/store/order_set";

    auto sha256 = escape_symbols(get_url_sign(ss_url.str(), params, params_, false));
    params["sig_sha256"] = std::move(sha256);

    std::stringstream ss_url_signed;
    ss_url_signed << ss_url.str() << '?' << format_get_params(params);

    _request->set_url(ss_url_signed.str());
    _request->set_normalized_url("stickersStoreOrderSet");
    _request->set_keep_alive();
    _request->set_priority(high_priority());

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t set_stickers_order_packet::execute_request(std::shared_ptr<core::http_request_simple> _request)
{
    bool res = _request->post();
    if (!res)
        return wpie_network_error;

    http_code_ = (uint32_t)_request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}


int32_t set_stickers_order_packet::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    response_ = _response;

    return 0;
}
