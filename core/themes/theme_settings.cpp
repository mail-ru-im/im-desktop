#include "stdafx.h"

#include "theme_settings.h"
#include "wallpaper_loader.h"
#include "../utils.h"

void core::theme_settings::set_value(std::string_view _name, tools::binary_stream&& _data)
{
    if (_data.available() > 0)
        values_[std::string(_name)] = std::move(_data);
    else
        clear_value(_name);

    changed_ = true;
}

void core::theme_settings::set_default_theme(const std::string_view _id)
{
    if (!_id.empty())
    {
        tools::binary_stream bs;
        bs.write(_id.data(), _id.size());
        set_value("theme", std::move(bs));
    }

    changed_ = true;
}

void core::theme_settings::set_default_wallpaper(const std::string_view _id)
{
    if (!_id.empty())
    {
        tools::binary_stream bs;
        bs.write(_id.data(), _id.size());
        set_value(get_global_wp_id_setting_field(), std::move(bs));
    }
    changed_ = true;
}

void core::theme_settings::reset_default_wallpaper()
{
    clear_value(get_global_wp_id_setting_field());
}

void core::theme_settings::set_wallpaper_urls(const themes::wallpaper_info_v& _wallpapers)
{
    assert(wallpapers_.empty());

    wallpapers_ = _wallpapers;
}
