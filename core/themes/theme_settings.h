#pragma once

#include "../gui_settings.h"

namespace core
{
    namespace themes
    {
        using wallpaper_info_v = std::vector<struct wallpaper_info>;
    }

    class theme_settings : public gui_settings
    {
    public:
        theme_settings(const std::wstring& _file_name) : gui_settings(_file_name, L"") {}
        virtual void set_value(std::string_view _name, tools::binary_stream&& _data) override;

        void set_default_theme(const std::string_view _id);
        void set_default_wallpaper(const std::string_view _id);
        void reset_default_wallpaper();

        void set_wallpaper_urls(const themes::wallpaper_info_v& _wallpapers);
        const themes::wallpaper_info_v& get_wallpaper_urls() const noexcept { return wallpapers_; }

    private:
        themes::wallpaper_info_v wallpapers_;
    };

}