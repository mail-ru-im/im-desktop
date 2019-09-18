#include "stdafx.h"
#include "mute_buddy.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

mute_buddy::mute_buddy(
    wim_packet_params _params,
    const std::string& _aimid,
    bool mute)
    :    wim_packet(std::move(_params)),
    aimid_(_aimid),
    mute_(mute)
{
}

mute_buddy::~mute_buddy()
{
}

int32_t mute_buddy::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;

    ss_url << urls::get_url(urls::url_type::wim_host) << "buddylist/Mute" <<
        "?f=json" <<
        "&aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&buddy=" << escape_symbols(aimid_) <<
        "&r=" << core::tools::system::generate_guid() <<
        "&eternal=" << (mute_ ? '1' : '0');

    _request->set_url(ss_url.str());
    _request->set_normalized_url("mute");
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t mute_buddy::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
