#include "stdafx.h"
#include "get_themes_index.h"
#include "../../../tools/strings.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../tools/binary_stream.h"
#include "../../../core.h"
#include "../../../http_request.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

std::string get_themes_url()
{
    std::stringstream ss;
    ss << std::string(urls::get_url(urls::url_type::misc_www_host)) << std::string_view("/wallpaperlist/v2/windows");

    return ss.str();
}

get_themes_index::get_themes_index(wim_packet_params _params, const std::string &etag): wim_packet(std::move(_params)), etag_(etag)
{
}

get_themes_index::~get_themes_index()
{
}

bool get_themes_index::support_async_execution() const
{
    return true;
}

int32_t get_themes_index::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_etag(etag_);
    _request->set_url(get_themes_url());
    _request->set_normalized_url("wallpaperList");
    return 0;
}

int32_t get_themes_index::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
    {
        return wpie_http_empty_response;
    }

    response_ = _response;
    header_etag_ = extract_etag();

    return 0;
}

std::shared_ptr<core::tools::binary_stream> get_themes_index::get_response() const
{
    return response_;
}

std::string get_themes_index::get_header_etag() const
{
    return header_etag_;
}

