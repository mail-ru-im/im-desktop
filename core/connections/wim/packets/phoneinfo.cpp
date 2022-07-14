#include "stdafx.h"
#include "phoneinfo.h"

#include "../../../http_request.h"
#include "../../../corelib/enumerations.h"
#include "../../../tools/system.h"
#include "../../../../common.shared/json_helper.h"
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

std::string_view phoneinfo::get_method() const
{
    return "phoneNumberValidate";
}

int core::wim::phoneinfo::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t phoneinfo::init_request(const std::shared_ptr<core::http_request_simple>& request)
{
    std::stringstream get_request;
    get_request << urls::get_url(urls::url_type::smsapi_host) << "/fcgi-bin/smsphoneinfo" <<
        "?service=icq_registration" <<
        "&info=typing_check,score,iso_country_code" <<
        "&lang=" << (gui_locale_.length() == 2 ? gui_locale_ : "en") <<
        "&phone=" << escape_symbols(phone_) <<
        "&id=" << core::tools::system::generate_guid();

    request->set_url(get_request.str());
    request->set_normalized_url(get_method());
    request->set_keep_alive();


    return 0;
}

int32_t phoneinfo::parse_response(const std::shared_ptr<core::tools::binary_stream>& response)
{
    if (!response->available())
        return wpie_http_empty_response;

    response->write((char)0);
    auto size = response->available();
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
            tools::unserialize_value(_info->value, "operator", info_operator_);
            tools::unserialize_value(_info->value, "phone", info_phone_);
            tools::unserialize_value(_info->value, "iso_country_code", info_iso_country_);
        }

        const auto _printable = doc.FindMember("printable");
        if (_printable != doc.MemberEnd() && _printable->value.IsArray())
        {
            printable_.reserve(_printable->value.Size());
            for (const auto& it : _printable->value.GetArray())
            {
                if (it.IsString())
                    printable_.push_back(rapidjson_get_string(it));
            }
        }

        tools::unserialize_value(doc, "status", status_);
        is_phone_valid_ = (status_ == "OK");

        const auto _typing_check = doc.FindMember("typing_check");
        if (_typing_check != doc.MemberEnd() && _typing_check->value.IsObject())
        {
            tools::unserialize_value(_typing_check->value, "trunk_code", trunk_code_);
            tools::unserialize_value(_typing_check->value, "modified_phone_number", modified_phone_number_);
            tools::unserialize_value(_typing_check->value, "modified_prefix", modified_prefix_);

            const auto _remaining_lengths = _typing_check->value.FindMember("remaining_lengths");
            if (_remaining_lengths != _typing_check->value.MemberEnd() && _remaining_lengths->value.IsArray())
            {
                remaining_lengths_.reserve(_remaining_lengths->value.Size());
                for (const auto& it : _remaining_lengths->value.GetArray())
                {
                    if (it.IsInt())
                    {
                        const auto value = it.GetInt();
                        remaining_lengths_.push_back(value);
                    }
                }
            }

            const auto _prefix_state = _typing_check->value.FindMember("prefix_state");
            if (_prefix_state != _typing_check->value.MemberEnd() && _prefix_state->value.IsArray())
            {
                prefix_state_.reserve(_prefix_state->value.Size());
                for (const auto& it : _prefix_state->value.GetArray())
                {
                    if (it.IsString())
                        prefix_state_.push_back(rapidjson_get_string(it));
                }
            }
        }
    }
    catch (...)
    {
        return 0;
    }

    return 0;
}
