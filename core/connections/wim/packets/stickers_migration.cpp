#include "stdafx.h"
#include "stickers_migration.h"
#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../tools/system.h"
#include "../../../utils.h"
#include "../../urls_cache.h"
#include "../common.shared/string_utils.h"

using namespace core;
using namespace wim;

stickers_migration::stickers_migration(wim_packet_params _params, std::vector<std::pair<int32_t, int32_t>> _ids)
    : wim_packet(std::move(_params)),
      ids_(std::move(_ids))
{

}

stickers_migration::~stickers_migration() = default;

const std::shared_ptr<core::tools::binary_stream>& core::wim::stickers_migration::get_response() const
{
    return response_;
}

int32_t stickers_migration::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::stringstream ss_stickers;

    for (auto [pack_id, sticker_id] : ids_)
        ss_stickers << pack_id << ':' << sticker_id << ',';

    auto stickers = ss_stickers.str();
    if (!stickers.empty())
        stickers.pop_back();

    std::pair<std::string_view, std::string> p[] = { { std::string_view("stickers"), escape_symbols(stickers) } };

    _request->set_url(su::concat(urls::get_url(urls::url_type::stickers_store_host), "/openstore/matchtofiles?", format_get_params(p)));
    _request->set_normalized_url("stickersMatchToFile");
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t stickers_migration::parse_response(const std::shared_ptr<core::tools::binary_stream>& _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    response_ = _response;

    return 0;
}

priority_t stickers_migration::get_priority() const
{
    return packets_priority_high();
}
