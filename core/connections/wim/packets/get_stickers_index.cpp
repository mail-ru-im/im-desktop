#include "stdafx.h"
#include "get_stickers_index.h"
#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../tools/system.h"
#include "../../../utils.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

get_stickers_index::get_stickers_index(wim_packet_params _params, const std::string& _md5)
    : wim_packet(std::move(_params))
    , md5_(_md5)
{
}

get_stickers_index::~get_stickers_index()
{
}

int32_t get_stickers_index::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::map<std::string, std::string> params;

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    std::stringstream ss_host;
    ss_host << urls::get_url(urls::url_type::stickers_store_host) << "/store/my";

    params["a"] = escape_symbols(params_.a_token_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["ts"] = std::to_string((int64_t) ts);
    params["r"] = core::tools::system::generate_guid();
    params["only_my"] = "1";
    params["with_emoji"] = "1";
    params["suggest"] = "1";

    if (!md5_.empty())
        params["md5"] = md5_;

    params["client"] = core::utils::get_client_string();
    params["lang"] = params_.locale_;

    std::stringstream ss_url;
    ss_url << ss_host.str() << '?' << format_get_params(params);

    _request->set_url(ss_url.str());
    _request->set_normalized_url("stikersStoreContentlist");
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_stickers_index::parse_response(const std::shared_ptr<core::tools::binary_stream>& _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    response_ = _response;

    return 0;
}

const std::shared_ptr<core::tools::binary_stream>& get_stickers_index::get_response() const noexcept
{
    return response_;
}

priority_t get_stickers_index::get_priority() const
{
    return packets_priority_high();
}
