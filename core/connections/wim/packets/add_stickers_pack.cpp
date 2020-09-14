#include "stdafx.h"
#include "add_stickers_pack.h"
#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../tools/system.h"
#include "../../../utils.h"

#include "../../urls_cache.h"

using namespace core;
using namespace wim;

add_stickers_pack_packet::add_stickers_pack_packet(wim_packet_params _params, const int32_t _pack_id, std::string _store_id)
    :   wim_packet(std::move(_params)),
        pack_id_(_pack_id),
        store_id_(std::move(_store_id))
{
}


add_stickers_pack_packet::~add_stickers_pack_packet()
{
}

int32_t add_stickers_pack_packet::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::map<std::string, std::string> params;

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    params["a"] = escape_symbols(params_.a_token_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["platform"] = utils::get_protocol_platform_string();

    _request->push_post_parameter("product", store_id_);

    params["ts"] = std::to_string((int64_t) ts);
    params["r"] = core::tools::system::generate_guid();

    params["client"] = core::utils::get_client_string();

    params["lang"] = params_.locale_;

    std::stringstream ss_url;
    ss_url << urls::get_url(urls::url_type::stickers_store_host) << std::string_view("/store/buy/free");

    std::stringstream ss_url_signed;
    ss_url_signed << ss_url.str() << '?' << format_get_params(params);

    _request->set_url(ss_url_signed.str());
    _request->set_normalized_url("stickersStoreBuy");
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t add_stickers_pack_packet::execute_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    url_ = _request->get_url();

    if (auto error_code = get_error(_request->post()))
        return *error_code;

    http_code_ = (uint32_t)_request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}


int32_t add_stickers_pack_packet::parse_response(const std::shared_ptr<core::tools::binary_stream>& _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    response_ = _response;

    return 0;
}

priority_t add_stickers_pack_packet::get_priority() const
{
    return packets_priority_high();
}
