#include "stdafx.h"
#include "speech_to_text.h"
#include "../../../http_request.h"
#include "../loader/web_file_info.h"
#include "../../../tools/json_helper.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;


speech_to_text::speech_to_text(
    wim_packet_params _params,
    const std::string& _url,
    const std::string& _locale)
    :    wim_packet(std::move(_params)),
    Url_(_url),
    Locale_(_locale),
    Comeback_(0)
{
}


speech_to_text::~speech_to_text()
{
}


int32_t speech_to_text::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;

    auto pos = Url_.rfind('/');
    std::string fileid;
    if (pos != std::string::npos)
        fileid = Url_.substr(pos + 1, Url_.length() - pos - 1);

    ss_url << urls::get_url(urls::url_type::files_host) << std::string_view("/speechtotext/") << fileid;

    std::map<std::string, std::string> params;

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    params["a"] = escape_symbols(params_.a_token_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["ts"] = tools::from_int64(ts);
    params["type"] = "ptt";
    params["locale"] = Locale_;

    const auto sha256 = escape_symbols(get_url_sign(ss_url.str(), params, params_, false));
    params["sig_sha256"] = sha256;

    std::stringstream ss_url_signed;
    ss_url_signed << ss_url.str() << '?' << format_get_params(params);

    _request->set_url(ss_url_signed.str());
    _request->set_normalized_url("pushToTalkRecognition");
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t speech_to_text::execute_request(std::shared_ptr<core::http_request_simple> request)
{
    if (!request->get())
        return wpie_network_error;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
    {
        if (http_code_ > 400 && http_code_ < 500)
            return on_http_client_error();

        if (http_code_ == 202)
        {
            auto response = dynamic_cast<tools::binary_stream*>(request->get_response().get());
            assert(response);
            if (!response->available())
                return wpie_http_empty_response;

            response->write((char) 0);
            uint32_t size = response->available();
            load_response_str((const char*) response->read(size), size);
            response->reset_out();

            rapidjson::Document doc;
            if (doc.ParseInsitu(response->read(size)).HasParseError())
                return wpie_error_parse_response;

            auto iter_comeback = doc.FindMember("comeback");
            if (iter_comeback == doc.MemberEnd() || !iter_comeback->value.IsString())
                return wpie_http_parse_response;

            Comeback_ = std::stoi(iter_comeback->value.GetString());
        }

        return wpie_http_error;
    }

    return 0;
}

int32_t speech_to_text::parse_response(std::shared_ptr<core::tools::binary_stream> _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    _response->write((char) 0);
    uint32_t size = _response->available();
    load_response_str((const char*) _response->read(size), size);
    _response->reset_out();

    rapidjson::Document doc;
    if (doc.ParseInsitu(_response->read(size)).HasParseError())
        return wpie_error_parse_response;

    if (!tools::unserialize_value(doc, "status", status_code_))
        return wpie_http_parse_response;

    if (status_code_ == 200)
        tools::unserialize_value(doc, "text", Text_);

    return 0;
}

std::string speech_to_text::get_text() const
{
    return Text_;
}

int32_t speech_to_text::get_comeback() const
{
    return Comeback_;
}