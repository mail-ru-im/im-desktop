#include "stdafx.h"
#include "get_stickers_index.h"
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

get_stickers_index::get_stickers_index(wim_packet_params _params, std::string_view _etag)
    : wim_packet(std::move(_params))
    , request_etag_(_etag)
{
}

get_stickers_index::~get_stickers_index() = default;

int32_t get_stickers_index::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::map<std::string, std::string> params;

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    params["aimsid"] = escape_symbols(params_.aimsid_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["ts"] = std::to_string((int64_t) ts);
    params["r"] = core::tools::system::generate_guid();
    params["only_my"] = "1";
    params["with_emoji"] = "1";
    params["suggest"] = "1";
    params["client"] = core::utils::get_client_string();
    params["lang"] = params_.locale_;

    const auto url = su::concat(urls::get_url(urls::url_type::stickers_store_host), "/store/my?", format_get_params(params));
    _request->set_url(url);
    _request->set_normalized_url(get_method());
    _request->set_keep_alive();

    if (!request_etag_.empty())
        _request->set_etag(request_etag_);

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
    response_etag_ = extract_etag();

    return 0;
}

priority_t get_stickers_index::get_priority() const
{
    return packets_priority_high();
}

std::string_view get_stickers_index::get_method() const
{
    return "stikersStoreContentlist";
}

int core::wim::get_stickers_index::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}
