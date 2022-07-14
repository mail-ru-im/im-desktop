#pragma once

#if defined (_WIN32)
#define CORELIBRARY "corelib.dll"
#elif defined (__APPLE__)
#define CORELIBRARY "libcorelib.dylib"
#elif defined (__linux__)
#define CORELIBRARY "libcorelib.so"
#endif

constexpr auto updates_folder_short() noexcept { return QStringView(u"updates"); }
constexpr auto update_final_command() noexcept { return QStringView(u"-update_final"); }
constexpr auto delete_updates_command() noexcept { return QStringView(u"-delete_updates"); }
constexpr auto installer_start_client_minimized() noexcept { return QStringView(u"-startup"); }
constexpr auto arg_start_client_minimized() noexcept { return QStringView(u"/startup"); }
constexpr auto autoupdate_from_8x() noexcept { return QStringView(u"-autoupdate"); }
constexpr auto nolaunch_from_8x() noexcept { return QStringView(u"-nolaunch"); };
constexpr auto send_dump_arg() noexcept { return QStringView(u"-send_dump"); }

#define crossprocess_pipe_name_postfix ".client"
constexpr auto crossprocess_message_get_process_id() noexcept { return QStringView(u"get_process_id"); }
constexpr auto crossprocess_message_get_hwnd_activate() noexcept { return QStringView(u"get_hwnd_and_activate"); }
constexpr auto crossprocess_message_shutdown_process() noexcept { return QStringView(u"shutdown_process"); }
constexpr auto crossprocess_message_execute_url_command() noexcept { return QStringView(u"execute_url_command"); }

constexpr auto url_command_join_livechat() noexcept { return u"chat"; }
constexpr auto url_command_stickerpack_info() noexcept { return u"s"; }
constexpr auto url_command_open_profile() noexcept { return u"people"; }
constexpr auto url_command_open_both_profile_and_chat() noexcept { return u"profile"; }
constexpr auto url_command_open_app() noexcept { return u"app"; }
constexpr auto url_command_open_miniapp() noexcept { return u"miniapp"; }
constexpr auto url_command_service() noexcept { return u"service"; }
constexpr auto url_command_vcs_call() noexcept { return u"call"; }
constexpr auto url_command_open_auth_modal_response() noexcept { return u"authmodal"; }
constexpr auto url_commandpath_service_getlogs() noexcept { return u"getlogs"; }
constexpr auto url_commandpath_service_getlogs_with_rtp() noexcept { return u"getlogswithrtp"; }
constexpr auto url_commandpath_agent_profile() noexcept { return u"profile"; }
constexpr auto url_commandpath_service_clear_avatars() noexcept { return u"clear/avatars"; }
constexpr auto url_commandpath_service_clear_cache() noexcept { return u"clear/cache"; }
constexpr auto url_commandpath_service_debug() noexcept { return u"debug"; }
constexpr auto url_commandpath_service_update() noexcept { return u"update"; }
constexpr auto url_command_create_group() noexcept { return u"newchat"; }
constexpr auto url_command_create_channel() noexcept { return u"newchannel"; }
constexpr auto url_command_copy_to_clipboard() noexcept { return u"copy_to_buffer"; }

constexpr auto mimetype_marker() noexcept { return "imdesktop"; }

// special hash, looks line filesharing, but invalid
// used for restricted files, same on all platforms
constexpr QStringView filesharingPlaceholder() noexcept { return u"N08262af1561a4e1a936b07287cb49101"; }

namespace Ui
{
    enum KeyToSendMessage
    {
        Enter       = 0,
        Enter_Old   = 1,                // deprecated
        Shift_Enter = Qt::Key_Shift,    // 0x01000020
        Ctrl_Enter  = Qt::Key_Control,  // 0x01000021 - cmd+enter on mac
        CtrlMac_Enter = Qt::Key_Meta    // 0x01000022 - ctrl+enter, mac only
    };

    enum class ShortcutsCloseAction
    {
        min = 0,

        RollUpWindow,
        Default = RollUpWindow,
        RollUpWindowAndChat,
        CloseChat,

        max
    };

    enum class ShortcutsSearchAction
    {
        min = 0,

        SearchInChat,
        Default = SearchInChat,
        GlobalSearch,

        max
    };

    namespace KeySymbols
    {
        namespace Mac
        {
            const auto command = QChar(0x2318);
            const auto option = QChar(0x2325);
            const auto shift = QChar(0x21E7);
            const auto control = QChar(0x2303);
            const auto backspace = QChar(0x232B);
            const auto esc = QChar(0x238B);
            const auto capslock = QChar(0x21EA);
            const auto enter = QChar(0x23CE);
            const auto tab = QChar(0x21E5);
        }

        const auto arrow_left = QChar(0x2190);
        const auto arrow_right = QChar(0x2192);
        const auto arrow_up = QChar(0x2191);
        const auto arrow_down = QChar(0x2193);
    }
}
