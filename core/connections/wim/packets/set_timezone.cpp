#include "stdafx.h"
#include "set_timezone.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

set_timezone::set_timezone(wim_packet_params _params)
    : wim_packet(std::move(_params))
{
}

set_timezone::~set_timezone()
{
}

std::string_view set_timezone::get_method() const
{
    return "timezoneSet";
}

int core::wim::set_timezone::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t set_timezone::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    time_t server_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_local_;

    time_t time_offset = boost::posix_time::to_time_t(boost::posix_time::second_clock::local_time()) - server_time;

    std::stringstream ss_url;

    ss_url << urls::get_url(urls::url_type::wim_host) << "timezone/set" <<
        "?f=json" <<
        "&aimsid=" << escape_symbols(get_params().aimsid_) <<
        "&r=" <<  core::tools::system::generate_guid() <<
        "&TimeZoneOffset=" << time_offset;

    _request->set_url(ss_url.str());
    _request->set_normalized_url(get_method());
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t set_timezone::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}

int32_t set_timezone::on_empty_data()
{
    return 0;
}
