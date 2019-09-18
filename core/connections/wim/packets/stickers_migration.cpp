#include "stdafx.h"
#include "stickers_migration.h"
#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../tools/system.h"
#include "../../../utils.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

stickers_migration::stickers_migration(wim_packet_params _params, std::vector<std::pair<int32_t, int32_t>> _ids)
    : wim_packet(std::move(_params)),
      ids_(std::move(_ids))
{

}

stickers_migration::~stickers_migration() = default;

std::shared_ptr<core::tools::binary_stream> core::wim::stickers_migration::get_response() const
{
    return response_;
}

bool stickers_migration::support_async_execution() const
{
    return true;
}

int32_t stickers_migration::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::map<std::string, std::string> params;

    std::stringstream ss_host;
    ss_host << urls::get_url(urls::url_type::stickers_store_host) << std::string_view("/openstore/matchtofiles");

    std::stringstream ss_stickers;

    for (auto [pack_id, sticker_id] : ids_)
        ss_stickers << pack_id << ':' << sticker_id << ',';

    auto stickers = ss_stickers.str();
    if (!stickers.empty())
        stickers.pop_back();

    params["stickers"] = escape_symbols(stickers);

    std::stringstream ss_url;
    ss_url << ss_host.str() << '?' << format_get_params(params);

    _request->set_url(ss_url.str());
    _request->set_normalized_url("stickersMatchToFile");
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

int32_t stickers_migration::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    response_ = _response;

    return 0;
}
