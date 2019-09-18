#include "stdafx.h"
#include "get_voip_calls_quality_popup_conf.h"

#include "../../../http_request.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

std::string get_voip_call_quality_host()
{
    std::stringstream ss_host;
    ss_host << urls::get_url(urls::url_type::wapi_host) << std::string_view("/static/popup");

    return ss_host.str();
}

get_voip_calls_quality_popup_conf::get_voip_calls_quality_popup_conf(
    wim_packet_params _params,
    const std::string& _app_name,
    const std::string& _platform,
    const std::string& _locale,
    const std::string& _country,
    const std::string& _version
    )
    : wim_packet(std::move(_params))
    , app_name_(_app_name)
    , platform_(_platform)
    , locale_(_locale)
    , country_(_country)
    , version_(_version)
{
    clear();
}

get_voip_calls_quality_popup_conf::~get_voip_calls_quality_popup_conf()
{
}

const voip_call_quality_popup_conf &get_voip_calls_quality_popup_conf::get_popup_configuration() const
{
    return conf_;
}

int32_t get_voip_calls_quality_popup_conf::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss;
    ss << get_voip_call_quality_host() << "/"
       << app_name_ << "/"
       << platform_ << "/"
       << locale_ << "/"
       << (country_.empty() ? "any" : country_) << "/"
       << version_ << "?login" << params_.aimid_;

    _request->set_url(ss.str());
    _request->set_normalized_url("getVoipCallsQuality");
    _request->set_keep_alive();

    return 0;
}

int32_t get_voip_calls_quality_popup_conf::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    _response->write((char)0);

    uint32_t size = _response->available();

    load_response_str((const char*)_response->read(size), size);

    _response->reset_out();

    clear();

    try
    {
        const auto json_str = _response->read(_response->available());
        rapidjson::Document doc;
        if (doc.ParseInsitu(json_str).HasParseError())
            return wpie_error_parse_response;

        conf_.unserialize(doc);
    }
    catch (...)
    {
        return 0;
    }

    return 0;
}

void get_voip_calls_quality_popup_conf::clear()
{

}
