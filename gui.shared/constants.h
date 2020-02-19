#pragma once

#if defined (_WIN32)
#define CORELIBRARY "corelib.dll"
#elif defined (__APPLE__)
#define CORELIBRARY "libcorelib.dylib"
#elif defined (__linux__)
#define CORELIBRARY "libcorelib.so"
#endif

const auto updates_folder_short = QLatin1String("updates");
const auto update_final_command = QLatin1String("-update_final");
const auto delete_updates_command = QLatin1String("-delete_updates");
const auto autoupdate_from_8x = QLatin1String("-autoupdate");
const auto nolaunch_from_8x = QLatin1String("-nolaunch");
const auto send_dump_arg = QLatin1String("-send_dump");

#define crossprocess_pipe_name_postfix ".client"
const QString crossprocess_message_get_process_id = QLatin1String("get_process_id");
const QString crossprocess_message_get_hwnd_activate = QLatin1String("get_hwnd_and_activate");
const QString crossprocess_message_shutdown_process = QLatin1String("shutdown_process");
const QString crossprocess_message_execute_url_command = QLatin1String("execute_url_command");

#define url_command_join_livechat "chat"
#define url_command_stickerpack_info "s"
#define url_command_open_profile "people"
#define url_command_app "app"
#define url_command_show_public_livechats "show_public_livechats"
#define url_command_service "service"
#define url_commandpath_service_getlogs           "getlogs"
#define url_commandpath_service_getlogs_with_rtp  "getlogswithrtp"
#define url_commandpath_agent_profile "profile"
#define url_commandpath_service_clear_avatars         "clear/avatars"
#define url_commandpath_service_clear_cache           "clear/cache"
#define url_commandpath_service_debug           "debug"
#define url_commandpath_service_update          "update"

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
