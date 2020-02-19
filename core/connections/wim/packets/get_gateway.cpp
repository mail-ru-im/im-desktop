#include "stdafx.h"
#include "get_gateway.h"

#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../utils.h"
#include "../../../tools/system.h"
#include "../../../tools/json_helper.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;


get_gateway::get_gateway(const wim_packet_params& _params, const std::string& _file_name, int64_t _file_size, const std::optional<int64_t>& _duration,
    const std::optional<file_sharing_base_content_type>& _base_content_type, const std::optional<std::string>& _locale)
    :    wim_packet(_params), file_name_(_file_name), file_size_(_file_size), duration_(_duration), base_content_type_(_base_content_type), locale_(_locale)
{
}


get_gateway::~get_gateway()
{
}


int32_t get_gateway::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::map<std::string, std::string> params;

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;
    std::stringstream ss_host;
    ss_host << urls::get_url(urls::url_type::files_host) << std::string_view("/init");

    std::stringstream ss_file_size;
    ss_file_size << file_size_;

    params["a"] = escape_symbols(params_.a_token_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["ts"] = tools::from_int64(ts);
    params["size"] = ss_file_size.str();
    params["filename"] = escape_symbols(file_name_);
    params["client"] = escape_symbols(utils::get_app_name());
    params["r"] = core::tools::system::generate_guid();

    if (base_content_type_ && base_content_type_ == file_sharing_base_content_type::ptt)
        params["type"] = "ptt";
    if (duration_)
        params["duration"] = std::to_string(*duration_);
    if (locale_)
        params["language"] = *locale_;

    auto sha256 = escape_symbols(get_url_sign(ss_host.str(), params, params_, false));
    params["sig_sha256"] = std::move(sha256);

    std::stringstream ss_url;
    ss_url << ss_host.str() << '?' << format_get_params(params);

    _request->set_url(ss_url.str());
    _request->set_normalized_url("filesCreateUploadSession");
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_gateway::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    _response->write((char) 0);
    _response->reset_out();

    rapidjson::Document doc;
    if (doc.ParseInsitu(_response->read(_response->available())).HasParseError())
        return wpie_error_parse_response;

    if (!tools::unserialize_value(doc, "status", status_code_))
        return wpie_http_parse_response;

    if (status_code_ == 200)
    {
        auto iter_data = doc.FindMember("data");
        if (iter_data == doc.MemberEnd() || !iter_data->value.IsObject())
            return wpie_http_parse_response;

        if (!tools::unserialize_value(iter_data->value, "host", host_) || !tools::unserialize_value(iter_data->value, "url", url_))
            return wpie_http_parse_response;
    }

    return 0;
}

int32_t get_gateway::on_http_client_error()
{
    if (http_code_ == 413)
        return wpie_error_too_large_file;

    return wpie_client_http_error;
}


std::string get_gateway::get_host() const
{
    return host_;
}

std::string get_gateway::get_url() const
{
    return url_;
}
