#include "stdafx.h"
#include "send_file.h"

#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../tools/system.h"
#include "../../../tools/json_helper.h"

using namespace core;
using namespace wim;


send_file::send_file(
    wim_packet_params _params,
    const send_file_params& _chunk,
    const std::string& _host,
    const std::string& _url)
    :    wim_packet(std::move(_params)),
    host_(_host),
    url_(_url),
    chunk_(_chunk)
{
}

send_file::~send_file() = default;

int32_t send_file::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    _request->set_timeout(std::chrono::seconds(15));

    _request->set_write_data_log(params_.full_log_);
    _request->set_keep_alive();

    std::stringstream ss_id;
    ss_id << "Session-ID: " << chunk_.session_id_;
    _request->set_custom_header_param(ss_id.str());

    std::stringstream ss_disp;
    ss_disp << "Content-Disposition: attachment; filename=\"" << chunk_.file_name_ << "\"";
    _request->set_custom_header_param(ss_disp.str());

    _request->set_custom_header_param("Content-Type: application/octet-stream");

    std::stringstream ss_range;
    ss_range << "bytes " << chunk_.size_already_sent_ << '-' << (chunk_.size_already_sent_ + chunk_.current_chunk_size_ - 1) << '/' <<  chunk_.full_data_size_;

    std::stringstream ss_content_range;
    ss_content_range << "Content-Range: " << ss_range.str();
    _request->set_custom_header_param(ss_content_range.str());

    _request->set_post_data(chunk_.data_, (uint32_t)chunk_.current_chunk_size_, false);

    std::stringstream ss_url;
    ss_url << "https://" << host_ << url_;

    std::map<std::string, std::string> params;

    params["aimsid"] = escape_symbols(params_.aimsid_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["r"] = core::tools::system::generate_guid();

    std::stringstream ss_url_signed;
    ss_url_signed << ss_url.str() << '?' << format_get_params(params);

    _request->set_url(ss_url_signed.str());
    _request->set_normalized_url(get_method());

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_message_markers();
        f.add_marker("a");
        f.add_marker("aimsid", aimsid_range_evaluator());
        f.add_json_marker("fileid");
        f.add_json_marker("static_url");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t send_file::parse_response(const std::shared_ptr<core::tools::binary_stream>& _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    _response->write((char) 0);
    auto size = _response->available();
    load_response_str((const char*) _response->read(size), size);
    _response->reset_out();

    rapidjson::Document doc;
    if (doc.ParseInsitu(_response->read(size)).HasParseError())
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

        if (!tools::unserialize_value(iter_data->value, "static_url", file_url_))
            return wpie_http_parse_response;

        if (!tools::unserialize_value(iter_data->value, "fileid", file_id_))
            return wpie_http_parse_response;
    }

    return 0;
}

int32_t send_file::execute_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    url_ = _request->get_url();

    if (auto error_code = get_error(_request->post()))
        return *error_code;

    http_code_ = (uint32_t)_request->get_response_code();

    if (http_code_ != 200 && http_code_ != 206 && http_code_ != 201)
    {
        if (http_code_ >= 400 && http_code_ < 500)
            return wpie_client_http_error;

        return wpie_http_error;
    }

    return 0;
}

const std::string& send_file::get_file_url() const
{
    return file_url_;
}

const std::string& send_file::get_file_id() const
{
    return file_id_;
}

std::string_view send_file::get_method() const
{
    return "filesUploadRange";
}
