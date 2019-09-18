#pragma once

#if defined (_WIN32)
#define CORELIBRARY "corelib.dll"
#elif defined (__APPLE__)
#define CORELIBRARY "libcorelib.dylib"
#elif defined (__linux__)
#define CORELIBRARY "libcorelib.so"
#endif

const QString product_name_icq = QLatin1String("icq.desktop");
const QString product_name_agent = QLatin1String("agent.desktop");
const QString product_name_biz = QLatin1String("myteam.desktop");
const QString product_name_dit = QLatin1String("messenger.desktop");

const QString updates_folder_short = QLatin1String("updates");
const QString installer_exe_name_agent = QLatin1String("magentsetup.exe");
const QString installer_exe_name_icq = QLatin1String("icqsetup.exe");
const QString installer_exe_name_biz = QLatin1String("myteamsetup.exe");
const QString installer_exe_name_dit = QLatin1String("messengersetup.exe");
const QString update_final_command = QLatin1String("-update_final");
const QString delete_updates_command = QLatin1String("-delete_updates");
const QString autoupdate_from_8x = QLatin1String("-autoupdate");
const QString nolaunch_from_8x = QLatin1String("-nolaunch");
const QString send_dump_arg = QLatin1String("-send_dump");

#define crossprocess_mutex_name_icq L"{DB17D844-6D3A-4039-968B-0B1D10B8AA81}"
#define crossprocess_mutex_name_agent L"{F54EF375-7D0C-4235-A692-EDE9712B450E}"
#define crossprocess_mutex_name_biz L"{B9E7958E-4000-4699-836C-232DFADFA4C8}"
#define crossprocess_mutex_name_dit L"{985205B9-BBF9-490B-BAFE-488ED1ABCB6B}"

const QString crossprocess_pipe_name_icq = QLatin1String("{4FA385C7-06F7-4C20-A23E-6B0AB2C02FD3}");
const QString crossprocess_pipe_name_agent = QLatin1String("{B3A5CAD7-9CDE-466F-9CD9-D8DF08B20460}");
const QString crossprocess_pipe_name_biz = QLatin1String("{8CDD298A-EA95-45FD-A9BA-DFCB89B05173}");
const QString crossprocess_pipe_name_dit = QLatin1String("{7D79DCD8-3E83-40A1-846B-36B3A3B4F411}");

#define crossprocess_pipe_name_postfix ".client"
const QString crossprocess_message_get_process_id = QLatin1String("get_process_id");
const QString crossprocess_message_get_hwnd_activate = QLatin1String("get_hwnd_and_activate");
const QString crossprocess_message_shutdown_process = QLatin1String("shutdown_process");
const QString crossprocess_message_execute_url_command = QLatin1String("execute_url_command");

#define application_user_model_id_icq L"ICQ.Client"
#define application_user_model_id_agent L"MailRu.Agent.Client"
#define application_user_model_id_biz L"Myteam.Client"
#define application_user_model_id_dit L"Messenger.Client"

#define url_scheme_icq "icq"
#define url_scheme_agent "magent"
#define url_scheme_dit "itd-messenger"
#define url_scheme_biz "myteam-messenger"

#define url_command_join_livechat "chat"
#define url_command_stickerpack_info "s"
#define url_command_open_profile "people"
#define url_command_app "app"
#define url_command_show_public_livechats "show_public_livechats"
#define url_command_service "service"
#define url_commandpath_service_getlogs           "getlogs"
#define url_commandpath_service_getlogs_with_rtp  "getlogswithrtp"
#define url_commandpath_agent_profile "profile"

#define icq_feedback_url "https://help.mail.ru/icqdesktop-support/all"
#define agent_feedback_url "https://help.mail.ru/agentdesktop-support/all"
#define biz_feedback_url "https://help.mail.ru/myteam/support"
#define dit_feedback_url "https://help.mail.ru/dit/support"


const std::wstring updater_singlton_mutex_name_icq = L"{D7364340-9348-4397-9F56-73CE62AAAEA8}";
const std::wstring updater_singlton_mutex_name_agent = L"{E9B7BB24-D200-401E-B797-E9A85796D506}";
const std::wstring updater_singlton_mutex_name_biz = L"{E2E1E89B-C522-45E2-9CB9-00E9F79FA144}";
const std::wstring updater_singlton_mutex_name_dit = L"{D000FD8E-3057-44EA-9B01-5996F922E670}";

const QString TermsUrlICQ = QLatin1String("https://privacy.icq.com/legal/terms");
const QString TermsUrlAgent = QLatin1String("https://agent.mail.ru/legal/terms");
const QString TermsUrlBiz = QLatin1String("https://privacy.icq.com/legal/terms");
const QString TermsUrlDit = QLatin1String("");
const QString PrivacyPolicyUrlICQ = QLatin1String("https://privacy.icq.com/legal/ppr");
const QString PrivacyPolicyUrlAgent = QLatin1String("https://agent.mail.ru/legal/pp");
const QString PrivacyPolicyUrlBiz = QLatin1String("https://agent.mail.ru/legal/pp");
const QString PrivacyPolicyUrlDit = QLatin1String("");


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
