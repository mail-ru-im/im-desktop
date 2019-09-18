#include "stdafx.h"
#include "get_attach_phone_info.h"

#include "../../../http_request.h"

#include "../../../tools/json_helper.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

std::string get_attach_phone_info_host()
{
    std::stringstream ss_host;
    ss_host << urls::get_url(urls::url_type::wapi_host) << std::string_view("/static/popup");

    return ss_host.str();
}

get_attach_phone_info::get_attach_phone_info(
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
    , show_(false)
    , close_(false)
    , timeout_(-1)
{
}

get_attach_phone_info::~get_attach_phone_info()
{
}

int32_t get_attach_phone_info::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss;
    ss << get_attach_phone_info_host() << '/'
       << app_name_ << '/'
       << platform_ << '/'
       << locale_ << '/'
       << (country_.empty() ? "any" : country_) << '/'
       << version_ << "?login" << params_.aimid_;

    _request->set_url(ss.str());
    _request->set_normalized_url("getAttachPhoneInfo");
    _request->set_keep_alive();

    return 0;
}

int32_t get_attach_phone_info::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    _response->write((char)0);

    uint32_t size = _response->available();

    load_response_str((const char*)_response->read(size), size);

    _response->reset_out();

    try
    {
        const auto json_str = _response->read(_response->available());
        rapidjson::Document doc;
        if (doc.ParseInsitu(json_str).HasParseError())
            return wpie_error_parse_response;

        tools::unserialize_value(doc, "show", show_);
        tools::unserialize_value(doc, "close_btn", close_);
        tools::unserialize_value(doc, "show_timeout", timeout_);
        tools::unserialize_value(doc, "text", text_);
        tools::unserialize_value(doc, "title", title_);
    }
    catch (...)
    {
        return 0;
    }

    return 0;
}
