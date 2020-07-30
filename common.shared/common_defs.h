#ifndef __COMMON_DEFS_H_
#define __COMMON_DEFS_H_

namespace common
{
#ifdef _WIN32
    std::wstring get_user_profile();
    std::wstring get_local_user_profile();
    std::wstring get_guid();
    std::string get_win_os_version_string();
    std::string get_win_device_id();
#else //_WIN32
    std::string get_home_directory();
#endif

    std::string_view get_dev_id();

    struct core_gui_settings
    {
        core_gui_settings() = default;

        core_gui_settings(
            int32_t _recents_avatars_size,
            std::string&& _os_version,
            std::string&& _locale)
            :   recents_avatars_size_(_recents_avatars_size),
                os_version_(std::move(_os_version)),
                locale_(std::move(_locale))
        {}

        int32_t recents_avatars_size_ = -1;
        std::string os_version_;
        std::string locale_;
    };
}

namespace common
{
    uint32_t get_limit_search_results();

    inline std::wstring get_final_act_filename()
    {
        return L"finalact.lst";
    }

}

#endif // __COMMON_DEFS_H_
