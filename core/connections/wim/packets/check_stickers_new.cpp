#include "stdafx.h"
#include "check_stickers_new.h"
#include "../../../tools/strings.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../tools/binary_stream.h"
#include "../../../core.h"
#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../../utils.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

check_stickers_new::check_stickers_new(wim_packet_params _params)
    : wim_packet(std::move(_params))
{
}

check_stickers_new::~check_stickers_new()
{
}

bool check_stickers_new::support_async_execution() const
{
    return true;
}

int32_t check_stickers_new::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::map<std::string, std::string> params;

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    std::stringstream ss_url;
    ss_url << urls::get_url(urls::url_type::stickers_store_host) << std::string_view("/openstore/getnew?platform=") <<
        utils::get_protocol_platform_string() << std::string_view("&client=") << core::utils::get_client_string();

    _request->set_url(ss_url.str());
    _request->set_keep_alive();
    _request->set_priority(high_priority());
    _request->set_timeout(std::chrono::seconds(3));
    _request->set_connect_timeout(std::chrono::seconds(2));

    return 0;
}

int32_t check_stickers_new::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
    {
        return wpie_http_empty_response;
    }

    response_ = _response;

    return 0;
}

std::shared_ptr<core::tools::binary_stream> check_stickers_new::get_response()
{
    return response_;
}
