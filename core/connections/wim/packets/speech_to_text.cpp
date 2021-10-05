#include "stdafx.h"
#include "speech_to_text.h"
#include "../../../http_request.h"
#include "../../../../common.shared/json_helper.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;


speech_to_text::speech_to_text(
    wim_packet_params _params,
    const std::string& _url,
    const std::string& _locale)
    : wim_packet(std::move(_params)),
    Url_(_url),
    Locale_(_locale),
    Comeback_(0)
{
}


speech_to_text::~speech_to_text() = default;

int32_t speech_to_text::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::string ss_url;

    auto pos = Url_.rfind('/');
    std::string_view fileid;
    if (pos != std::string::npos)
        fileid = std::string_view(Url_).substr(pos + 1, Url_.length() - pos - 1);

    ss_url += urls::get_url(urls::url_type::files_host);
    ss_url += "/speechtotext/";
    ss_url += fileid;

    std::map<std::string, std::string> params;

    params["aimsid"] = escape_symbols(params_.aimsid_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["type"] = "ptt";
    params["locale"] = Locale_;

    ss_url += '?';
    ss_url += format_get_params(params);

    _request->set_url(ss_url);
    _request->set_normalized_url(get_method());
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        f.add_marker("aimsid", aimsid_range_evaluator());
        f.add_json_marker("text");
        f.add_url_marker("speechtotext/", speechtotext_fileid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t speech_to_text::execute_request(const std::shared_ptr<core::http_request_simple>& request)
{
    url_ = request->get_url();

    if (auto error_code = get_error(request->get()))
        return *error_code;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
    {
        if (http_code_ > 400 && http_code_ < 500)
            return on_http_client_error();

        if (http_code_ == 202)
        {
            auto response = dynamic_cast<tools::binary_stream*>(request->get_response().get());
            im_assert(response);
            if (!response->available())
                return wpie_http_empty_response;

            response->write((char) 0);
            auto size = response->available();
            load_response_str((const char*) response->read(size), size);
            response->reset_out();

            rapidjson::Document doc;
            if (doc.ParseInsitu(response->read(size)).HasParseError())
                return wpie_error_parse_response;

            if (const auto iter_comeback = doc.FindMember("comeback"); iter_comeback != doc.MemberEnd())
            {
                if (iter_comeback->value.IsString())
                    Comeback_ = std::stoi(rapidjson_get_string(iter_comeback->value));
                else if (iter_comeback->value.IsInt())
                    Comeback_ = iter_comeback->value.GetInt();
                else
                    return wpie_http_parse_response;
            }
            else
            {
                return wpie_http_parse_response;
            }
        }

        return wpie_http_error;
    }

    return 0;
}

int32_t speech_to_text::parse_response(const std::shared_ptr<core::tools::binary_stream>& _response)
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

    if (!tools::unserialize_value(doc, "status", status_code_))
        return wpie_http_parse_response;

    if (status_code_ == 200)
        tools::unserialize_value(doc, "text", Text_);

    return 0;
}

const std::string& speech_to_text::get_text() const
{
    return Text_;
}

int32_t speech_to_text::get_comeback() const
{
    return Comeback_;
}

std::string_view speech_to_text::get_method() const
{
    return "pushToTalkRecognition";
}
