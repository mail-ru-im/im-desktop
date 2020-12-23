#include "stdafx.h"
#include "core_settings.h"
#include "core.h"

#include "tools/system.h"
#include "tools/device_id.h"
#include "proxy_settings.h"
#include "../common.shared/string_utils.h"

using namespace core;

core_settings::core_settings(const boost::filesystem::wpath& _path, const boost::filesystem::wpath& _file_name_exported)
    : file_name_(_path.wstring())
    , file_name_exported_(_file_name_exported.wstring())
{
}


core_settings::~core_settings()
{
}

bool core_settings::end_init_default()
{
    auto need_save = false;
    if (
        !value_exists(core_settings_values::csv_device_id) ||
        get_value<std::string>(core_settings_values::csv_device_id, std::string()).length() < 10)
    {
        set_value<std::string>(core_settings_values::csv_device_id, core::tools::get_device_id());
        need_save = true;
    }

    if (!value_exists(core_settings_values::core_settings_locale))
    {
        std::locale loc = boost::locale::generator().generate(std::string());
        std::string lang = std::use_facet<boost::locale::info>(loc).language();
        std::string country = std::use_facet<boost::locale::info>(loc).country();
        std::string locale;
        if (!lang.empty() && !country.empty())
            locale = lang + '-' + country;

        set_value(core_settings_values::core_settings_locale, locale.empty() ? locale : boost::locale::to_lower(locale, loc));
        need_save = true;
    }

    return need_save;
}

bool core_settings::save()
{
    core::tools::binary_stream bstream;
    serialize(bstream);
    return bstream.save_2_file(file_name_);
}

bool core_settings::load()
{
    core::tools::binary_stream bstream;
    if (bstream.load_from_file(file_name_))
    {
        const auto was_unserialized = core::tools::settings::unserialize(bstream);
        if (!was_unserialized)
        {
            g_core->write_string_to_network_log(su::concat(
                "failed to unserialize ",
                tools::from_utf16(tools::system::get_file_name(file_name_)), "\r\n"));
        }
        return was_unserialized;
    }

    return load_exported();
}

void core_settings::set_user_proxy_settings(const proxy_settings& _user_proxy_settings)
{
    tools::binary_stream bs_data;
    _user_proxy_settings.serialize(bs_data);
    set_value(core_settings_values::core_settings_proxy, bs_data);
    save();
}

void core_settings::set_locale_was_changed(bool _was_changed)
{
    set_value(core_settings_values::core_settings_locale_was_changed, _was_changed);
    save();
}

bool core_settings::get_locale_was_changed() const
{
    return get_value(core_settings_values::core_settings_locale_was_changed, true);
}

void core_settings::set_locale(const std::string& _locale)
{
    set_value(core_settings_values::core_settings_locale, _locale);
    save();
}

std::string core_settings::get_locale() const
{
    return get_value<std::string>(core_settings_values::core_settings_locale, std::string());
}

proxy_settings core_settings::get_user_proxy_settings()
{
    proxy_settings core_proxy_settings;
    auto stream = get_value<tools::binary_stream>(core_settings_values::core_settings_proxy, tools::binary_stream());
    core_proxy_settings.unserialize(stream);
    return core_proxy_settings;
}

bool core_settings::get_voip_mute_fix_flag() const
{
    bool res = false;
    get_value(core_settings_values::voip_mute_fix_flag, &res);
    return res;
}

void core_settings::set_voip_mute_fix_flag(bool bValue)
{
    set_value(core_settings_values::voip_mute_fix_flag, bValue);
    save();
}

bool core_settings::unserialize(const rapidjson::Value& _node)
{
    return true;
}

bool core_settings::load_exported()
{
    if (tools::binary_stream bstream_exported; bstream_exported.load_from_file(file_name_exported_))
    {
        bstream_exported.write<char>('\0');

        rapidjson::Document doc;
        if (!doc.ParseInsitu(bstream_exported.read_available()).HasParseError())
        {
            if (unserialize(doc))
            {
                save();
                return true;
            }
        }
    }

    return false;
}

