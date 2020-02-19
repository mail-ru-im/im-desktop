#include "stdafx.h"
#include "get_stickers_pack_info.h"
#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../tools/system.h"
#include "../../../utils.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

get_stickers_pack_info_packet::get_stickers_pack_info_packet(
    wim_packet_params _params,
    const int32_t _pack_id,
    const std::string& _store_id,
    const std::string& _file_id)
    :   wim_packet(std::move(_params)),
        pack_id_(_pack_id),
        store_id_(_store_id),
        file_id_(_file_id)
{
}


get_stickers_pack_info_packet::~get_stickers_pack_info_packet()
{
}

bool get_stickers_pack_info_packet::support_async_execution() const
{
    return true;
}

int32_t get_stickers_pack_info_packet::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::map<std::string, std::string> params;

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    std::stringstream ss_host;
    ss_host << urls::get_url(urls::url_type::stickers_store_host) << std::string_view("/openstore/filespackinfowithmeta");

    params["a"] = escape_symbols(params_.a_token_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["ts"] = std::to_string((int64_t) ts);
    params["r"] = core::tools::system::generate_guid();
    params["platform"] = utils::get_protocol_platform_string();

    params["client"] = core::utils::get_client_string();

    params["lang"] = params_.locale_;

    if (pack_id_ > 0)
        params["id"] = std::to_string(pack_id_);
    else if (!store_id_.empty())
        params["store_id"] = escape_symbols(store_id_);
    else if (!file_id_.empty())
        params["file_id"] = escape_symbols(file_id_);

    auto sha256 = escape_symbols(get_url_sign(ss_host.str(), params, params_, false));
    params["sig_sha256"] = std::move(sha256);

    std::stringstream ss_url;
    ss_url << ss_host.str() << '?' << format_get_params(params);

    _request->set_url(ss_url.str());
    _request->set_normalized_url("stikersStorePackinfo");
    _request->set_keep_alive();
    _request->set_priority(high_priority());

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_marker("file_id");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_stickers_pack_info_packet::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    response_ = _response;

    return 0;
}

std::shared_ptr<core::tools::binary_stream> get_stickers_pack_info_packet::get_response() const
{
    return response_;
}
