#include "stdafx.h"
#include "phoneinfo.h"

#include "../../../http_request.h"
#include "../../../corelib/enumerations.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"

using namespace core;
using namespace wim;

phoneinfo::phoneinfo(wim_packet_params params, const std::string &phone, const std::string &gui_locale):
    wim_packet(std::move(params)), phone_(phone), gui_locale_(gui_locale), is_phone_valid_(false)
{
}

phoneinfo::~phoneinfo()
{
}

int32_t phoneinfo::init_request(std::shared_ptr<core::http_request_simple> request)
{
    std::stringstream get_request;
    get_request << urls::get_url(urls::url_type::smsapi_host) << "/fcgi-bin/smsphoneinfo" <<
        "?service=icq_registration" <<
        "&info=typing_check,score,iso_country_code" <<
        "&lang=" << (gui_locale_.length() == 2 ? gui_locale_ : "en") <<
        "&phone=" << escape_symbols(phone_) <<
        "&id=" << core::tools::system::generate_guid();

    request->set_url(get_request.str());
    request->set_normalized_url("phoneNumberValidate");
    request->set_keep_alive();


    return 0;
}

int32_t phoneinfo::parse_response(std::shared_ptr<core::tools::binary_stream> response)
{
    if (!response->available())
        return wpie_http_empty_response;

    response->write((char)0);
    uint32_t size = response->available();
    load_response_str((const char *)response->read(size), size);
    response->reset_out();

    try
    {
        const auto json_str = response->read(response->available());

        rapidjson::Document doc;
        if (doc.ParseInsitu(json_str).HasParseError())
            return wpie_error_parse_response;

        const auto _info = doc.FindMember("info");
        if (_info != doc.MemberEnd() && _info->value.IsObject())
        {
            const auto _operator = _info->value.FindMember("operator");
            if (_operator != _info->value.MemberEnd() && _operator->value.IsString())
                info_operator_ = rapidjson_get_string(_operator->value);

            const auto _phone = _info->value.FindMember("phone");
            if (_phone != _info->value.MemberEnd() && _phone->value.IsString())
                info_phone_ = rapidjson_get_string(_phone->value);

            const auto _iso_country = _info->value.FindMember("iso_country_code");
            if (_iso_country != _info->value.MemberEnd() && _iso_country->value.IsString())
                info_iso_country_ = rapidjson_get_string(_iso_country->value);
        }

        const auto _printable = doc.FindMember("printable");
        if (_printable != doc.MemberEnd() && _printable->value.IsArray())
        {
            for (auto it = _printable->value.Begin(), itend = _printable->value.End(); it != itend; ++it)
            {
                if (it->IsString())
                    printable_.push_back(rapidjson_get_string(*it));
            }
        }

        const auto _status = doc.FindMember("status");
        if (_status != doc.MemberEnd() && _status->value.IsString())
            status_ = rapidjson_get_string(_status->value);

        is_phone_valid_ = (status_ == "OK");

        const auto _typing_check = doc.FindMember("typing_check");
        if (_typing_check != doc.MemberEnd() && _typing_check->value.IsObject())
        {
            const auto _trunk_code = _typing_check->value.FindMember("trunk_code");
            if (_trunk_code != _typing_check->value.MemberEnd() && _trunk_code->value.IsString())
                trunk_code_ = rapidjson_get_string(_trunk_code->value);

            const auto _modified_phone_number = _typing_check->value.FindMember("modified_phone_number");
            if (_modified_phone_number != _typing_check->value.MemberEnd() && _modified_phone_number->value.IsString())
                modified_phone_number_ = rapidjson_get_string(_modified_phone_number->value);

            const auto _remaining_lengths = _typing_check->value.FindMember("remaining_lengths");
            if (_remaining_lengths != _typing_check->value.MemberEnd() && _remaining_lengths->value.IsArray())
            {
                for (auto it = _remaining_lengths->value.Begin(), itend = _remaining_lengths->value.End(); it != itend; ++it)
                {
                    if (it->IsInt())
                    {
                        const auto value = it->GetInt();
                        remaining_lengths_.push_back(value);
                    }
                }
            }

            const auto _prefix_state = _typing_check->value.FindMember("prefix_state");
            if (_prefix_state != _typing_check->value.MemberEnd() && _prefix_state->value.IsArray())
            {
                for (auto it = _prefix_state->value.Begin(), itend = _prefix_state->value.End(); it != itend; ++it)
                {
                    if (it->IsString())
                        prefix_state_.push_back(rapidjson_get_string(*it));
                }
            }

            const auto _modified_prefix = _typing_check->value.FindMember("modified_prefix");
            if (_modified_prefix != _typing_check->value.MemberEnd() && _modified_prefix->value.IsString())
                modified_prefix_ = rapidjson_get_string(_modified_prefix->value);
        }
    }
    catch (...)
    {
        return 0;
    }

    return 0;
}
