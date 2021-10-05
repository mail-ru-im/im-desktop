#include "stdafx.h"
#include "get_gateway.h"

#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../utils.h"
#include "../../../tools/system.h"
#include "../../../../common.shared/json_helper.h"
#include "../../urls_cache.h"
#include "../common.shared/string_utils.h"

using namespace core;
using namespace wim;


get_gateway::get_gateway(
    const wim_packet_params& _params,
    const std::string& _file_name,
    int64_t _file_size,
    const std::optional<int64_t>& _duration,
    const std::optional<file_sharing_base_content_type>& _base_content_type,
    const std::optional<std::string>& _locale)
    : wim_packet(_params)
    , file_name_(_file_name)
    , file_size_(_file_size)
    , duration_(_duration)
    , base_content_type_(_base_content_type)
    , locale_(_locale)
{
}

get_gateway::~get_gateway() = default;

std::string_view get_gateway::get_method() const
{
    return "filesCreateUploadSession";
}

int32_t get_gateway::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::map<std::string, std::string> params;
    params["aimsid"] = escape_symbols(params_.aimsid_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["size"] = std::to_string(file_size_);
    params["filename"] = escape_symbols(file_name_);
    params["client"] = escape_symbols(utils::get_app_name());
    params["r"] = core::tools::system::generate_guid();

    if (base_content_type_ && base_content_type_ == file_sharing_base_content_type::ptt)
        params["type"] = "ptt";
    if (duration_)
        params["duration"] = std::to_string(*duration_);
    if (locale_)
        params["language"] = *locale_;

    _request->set_url(su::concat(urls::get_url(urls::url_type::files_host), "/init?", format_get_params(params)));
    _request->set_normalized_url(get_method());
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_gateway::parse_response(const std::shared_ptr<core::tools::binary_stream>& _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    _response->write((char) 0);
    _response->reset_out();

    rapidjson::Document doc;
    if (doc.ParseInsitu(_response->read(_response->available())).HasParseError())
        return wpie_error_parse_response;

    const auto iter_status = doc.FindMember("status");
    if (iter_status == doc.MemberEnd() || !iter_status->value.IsObject())
        return wpie_http_parse_response;

    if (!tools::unserialize_value(iter_status->value, "code", status_code_))
        return wpie_http_parse_response;

    if (status_code_ == 200)
    {
        const auto iter_data = doc.FindMember("result");
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
