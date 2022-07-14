#include "stdafx.h"
#include "set_stickers_order.h"
#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../tools/system.h"
#include "../../../utils.h"
#include "../../urls_cache.h"
#include "../common.shared/string_utils.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

set_stickers_order_packet::set_stickers_order_packet(wim_packet_params _params, std::vector<int32_t>&& _values)
    :   wim_packet(std::move(_params)),
        values_(std::move(_values))
{
}


set_stickers_order_packet::~set_stickers_order_packet() = default;

int32_t set_stickers_order_packet::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::map<std::string, std::string> params;
    params["aimsid"] = escape_symbols(params_.aimsid_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["platform"] = utils::get_protocol_platform_string();

    for (size_t i = 0; i < values_.size(); ++i)
    {
        std::stringstream ss_value;
        ss_value << "priority[" << values_[i] << ']';

        params[escape_symbols(ss_value.str())] = std::to_string(i + 1);
    }

    _request->push_post_parameter("empty", "empty");  // to fix infinite waiting in case of post request without params

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;
    params["ts"] = std::to_string((int64_t) ts);
    params["r"] = core::tools::system::generate_guid();

    params["client"] = core::utils::get_client_string();

    params["lang"] = params_.locale_;

    const auto ss_url = su::concat(urls::get_url(urls::url_type::stickers_store_host), "/store/order_set");

    _request->set_url(su::concat(ss_url, '?', format_get_params(params)));
    _request->set_normalized_url(get_method());
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t set_stickers_order_packet::execute_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    url_ = _request->get_url();

    if (auto error_code = get_error(_request->post()))
        return *error_code;

    http_code_ = (uint32_t)_request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}


int32_t set_stickers_order_packet::parse_response(const std::shared_ptr<core::tools::binary_stream>& _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    response_ = _response;

    return 0;
}

priority_t set_stickers_order_packet::get_priority() const
{
    return packets_priority_high();
}

std::string_view set_stickers_order_packet::get_method() const
{
    return "stickersStoreOrderSet";
}

int core::wim::set_stickers_order_packet::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}
