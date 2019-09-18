#pragma once

namespace core
{

    enum class profile_state
    {
        min = 0,

        online,
        dnd,
        invisible,
        away,
        offline,

        max,
    };

    // don't change texts in this func - it uses for sending to server
    inline std::ostream& operator<<(std::ostream &oss, const profile_state arg)
    {
        assert(arg > profile_state::min);
        assert(arg < profile_state::max);

        switch(arg)
        {
            case profile_state::online:    oss << "online"; break;
            case profile_state::dnd:       oss << "dnd"; break;
            case profile_state::invisible: oss << "invisible"; break;
            case profile_state::away:      oss << "away"; break;
            case profile_state::offline:   oss << "offline"; break;

        default:
            assert(!"unknown core::profile_state value");
            break;
        }

        return oss;
    }

    enum class message_type
    {
        min,


        undefined,
        base,
        file_sharing,
        sms,
        sticker,
        chat_event,
        voip_event,

        max
    };

    enum class file_sharing_function
    {
        min,

        download_file,
        download_file_metainfo,
        download_preview_metainfo,
        check_local_copy_exists,

        max
    };

    enum class file_sharing_base_content_type
    {
        min,

        undefined,
        image,
        gif,
        video,
        ptt,

        max
    };

    enum class file_sharing_sub_content_type
    {
        min,

        undefined,
        sticker,

        max
    };

    struct file_sharing_content_type
    {
        file_sharing_base_content_type type_;
        file_sharing_sub_content_type subtype_;

        bool is_image() const {return type_ == file_sharing_base_content_type::image;}
        bool is_gif() const { return type_ == file_sharing_base_content_type::gif; }
        bool is_video() const { return type_ == file_sharing_base_content_type::video; }
        bool is_ptt() const { return type_ == file_sharing_base_content_type::ptt; }
        bool is_sticker() const { return subtype_ == file_sharing_sub_content_type::sticker; }
        bool is_undefined() const { return type_ == file_sharing_base_content_type::undefined; }
        file_sharing_content_type()
            : type_(file_sharing_base_content_type::undefined)
            , subtype_(file_sharing_sub_content_type::undefined) {}
        file_sharing_content_type(
            file_sharing_base_content_type _type,
            file_sharing_sub_content_type _subtype = file_sharing_sub_content_type::undefined) : type_(_type), subtype_(_subtype) {};
    };

    enum class typing_status
    {
        min,

        none,
        typing,
        typed,
        looking,

        max,
    };

    enum class sticker_size
    {
        min,

        small,
        medium,
        large,
        xlarge,
        xxlarge,

        max
    };

    enum class chat_event_type
    {
        // the codes are stored in a db thus do not reorder the items below

        invalid,

        min,

        added_to_buddy_list,

        mchat_add_members,
        mchat_invite,
        mchat_leave,
        mchat_del_members,
        mchat_kicked,

        chat_name_modified,

        buddy_reg,
        buddy_found,

        birthday,

        avatar_modified,

        generic,

        chat_description_modified,

        message_deleted,

        chat_rules_modified,

        max
    };

    enum class voip_event_type
    {
        invalid,

        min,

        missed_call,
        call_ended,
        accept,
        decline,

        max
    };

    inline std::ostream& operator<<(std::ostream &oss, const message_type arg)
    {
        switch(arg)
        {
        case message_type::base:
            oss << "base";
            break;

        case message_type::file_sharing:
            oss << "file_sharing";
            break;

        case message_type::sticker:
            oss << "sticker";
            break;

        case message_type::sms:
            oss << "sms";
            break;

        default:
            assert(!"unknown core::message_type value");
            break;
        }

        return oss;
    }

    inline std::ostream& operator<<(std::ostream &oss, const file_sharing_function arg)
    {
        assert(arg > file_sharing_function::min);
        assert(arg < file_sharing_function::max);

        switch(arg)
        {
        case file_sharing_function::check_local_copy_exists:
            oss << "check_local_copy_exists";
            break;

        case file_sharing_function::download_file:
            oss << "download_file";
            break;

        case file_sharing_function::download_file_metainfo:
            oss << "download_file_metainfo";
            break;

        case file_sharing_function::download_preview_metainfo:
            oss << "download_preview_metainfo";
            break;

        default:
            assert(!"unknown core::file_sharing_function value");
            break;
        }

        return oss;
    }

    inline std::ostream& operator<<(std::ostream &oss, const sticker_size arg)
    {
        assert(arg > sticker_size::min);
        assert(arg < sticker_size::max);

        switch(arg)
        {
        case sticker_size::small:
            oss << "small";
            break;

        case sticker_size::medium:
            oss << "medium";
            break;

        case sticker_size::large:
            oss << "large";
            break;

        case sticker_size::xlarge:
            oss << "xlarge";
            break;

        case sticker_size::xxlarge:
            oss << "xxlarge";
            break;

        default:
            assert(!"unknown core::sticker_size value");
            break;
        }

        return oss;
    }

    inline std::wostream& operator<<(std::wostream &oss, const sticker_size arg)
    {
        assert(arg > sticker_size::min);
        assert(arg < sticker_size::max);

        switch(arg)
        {
        case sticker_size::small:
            oss << L"small";
            break;

        case sticker_size::medium:
            oss << L"medium";
            break;

        case sticker_size::large:
            oss << L"large";
            break;

        case sticker_size::xlarge:
            oss << L"xlarge";
            break;

        case sticker_size::xxlarge:
            oss << L"xxlarge";
            break;

        default:
            assert(!"unknown core::sticker_size value");
            break;
        }

        return oss;
    }

    enum class group_chat_info_errors
    {
        min = 1,

        network_error = 2,
        not_in_chat = 3,
        blocked = 4,

        max,
    };

    namespace stats
    {
        // NOTE: do not change old numbers, add new one if necessary
        enum class stats_event_names
        {
            min = 0,

            service_session_start = 1001,
            start_session = 1002,
            start_session_params_loaded = 1003,

            reg_page_phone = 2001,
            reg_login_phone = 2002,
            reg_page_uin = 2003,
            reg_login_uin = 2004,
            reg_edit_country = 2005,
            reg_error_phone = 2006,
            reg_sms_send = 2007,
            reg_sms_resend = 2008,
            reg_error_code = 2009,
            reg_edit_phone = 2010,
            reg_error_uin = 2011,
            reg_error_other = 2012,
            login_forgot_password = 2013,

            main_window_fullscreen = 3001,

            filesharing_sent_count = 5001,
            filesharing_sent = 5002,
            filesharing_sent_success = 5003,
            filesharing_count = 5004,
            filesharing_cancel = 5009,
            filesharing_download_file = 5010,
            filesharing_download_cancel = 5011,
            filesharing_download_success = 5012,

            smile_sent_picker = 6001,
            smile_sent_from_recents = 6002,
            sticker_sent_from_picker = 6003,
            sticker_sent_from_recents = 6004,
            picker_cathegory_click = 6005,
            picker_tab_click = 6006,

            stickers_download_pack = 6007,
            stickers_share_pack = 6008,
            stickers_discover_icon_picker = 6009,
            stickers_discover_pack_tap = 6010,
            stickers_discover_pack_download = 6011,
            stickers_discover_addbot_bottom_tap = 6012,
            stickers_discover_sendbot_message = 6013,
            stickers_longtap_picker = 6014,
            stickers_longtap_preview = 6015,
            stickers_sticker_tap_in_chat = 6016,
            stickers_addpack_button_tap_in_chat = 6017,
            stickers_pack_delete = 6018,
            stickers_emoji_added_in_input = 6019,
            stickers_suggests_appear_in_input  = 6020,
            stickers_suggested_sticker_sent = 6021,
            stickers_search_input_tap = 6022,
            stickers_search_request_send = 6023,
            stickers_search_results_receive = 6024,
            stickers_search_pack_tap = 6025,
            stickers_search_add_pack_button_tap = 6026,
            stickers_search_pack_download = 6027,

            alert_click = 7001,
            alert_viewall = 7002,
            alert_close = 7003,
            alert_mail_common = 7004,
            alert_mail_letter = 7005,
            tray_mail = 7006,
            titlebar_message = 7007,
            titlebar_mail = 7008,

            spam_contact = 8010,
            ignore_contact = 8011,
            delete_contact = 8012,
            add_contact = 8013,

            recents_close = 9001,
            mark_read = 9002,
            mark_read_all = 9003,
            mute_recents_menu = 9004,
            mute_sidebar = 9005,
            unmute = 9006,
            mark_unread = 9007,

            ignorelist_open = 10006,
            ignorelist_remove = 10007,

            cl_empty_android = 11002,
            cl_empty_ios = 11003,
            cl_search = 11005,
            cl_search_openmessage = 11007,
            cl_search_dialog = 11008,
            cl_search_dialog_openmessage = 11009,
            cl_search_nohistory = 11010,

            chat_search_dialog_opened = 11021,
            chat_search_dialog_suggest_click = 11022,
            chat_search_dialog_first_symb_typed = 11023,
            chat_search_dialog_cancel = 11024,
            chat_search_dialog_server_first_req = 11025,
            chat_search_dialog_server_first_ans = 11026,
            chat_search_dialog_item_click = 11027,
            chat_search_dialog_scroll = 11028,

            myprofile_edit = 12001,
            myprofile_open = 12002,
            profile_open = 12003,
            profile_sidebar = 13008,
            profile_write_message = 13009,

            profileeditscr_view = 14000,
            profileeditscr_avatar_action = 14001,
            profileeditscr_name_action = 14002,
            profileeditscr_aboutme_action = 14003,
            profileeditscr_changenum_action = 14004,
            profileeditscr_nickedit_action = 14005,
            profileeditscr_access_action = 14006,
            profileeditscr_signout_action = 14007,

            nickeditscr_edit_action = 15000,

            message_send_button = 16001,
            message_enter_button = 16002,

            open_chat = 17004,

            message_pending = 18001,
            message_delete_my = 18002,
            message_delete_all = 18003,

            history_delete = 19003,
            more_button_click = 19004,

            feedback_show = 22001,
            feedback_sent = 22002,
            feedback_error = 22003,

            call_audio = 23010,
            call_video = 23011,

            aboutus_show = 25001,
            client_settings = 25002,

            proxy_open = 26001,
            proxy_set = 26002,

            strange_event = 27001,

            favorites_set = 29001,
            favorites_unset = 29002,
            favorites_load = 29003,

            profile_avatar_changed = 31001,
            introduce_name_set = 31002,
            introduce_avatar_changed = 31003,
            introduce_avatar_fail = 31004,

            masks_open = 32001,
            masks_select = 32002,

            //Groupchats, livechats
            chats_create_open = 33001,
            chats_create_public = 33002,
            chats_create_approval = 33003,
            chats_create_readonly = 33004,
            chats_create_rename = 33005,
            chats_create_avatar = 33006,
            chats_created = 33007,
            chats_leave = 33008,
            chats_rename = 33009,
            chats_avatar_changed = 33010,
            chats_add_member_sidebar = 33011,
            chats_add_member_dialog = 33012,
            chats_join_frompopup = 33013,
            chats_admins = 33014,
            chats_blocked = 33015,
            chats_open_popup = 33016,

            quotes_send_alone = 34001,
            quotes_send_answer = 34002,
            quotes_messagescount = 34003,

            forward_send_frommenu = 35001,
            forward_send_frombutton = 35002,
            forward_send_preview = 35003,
            forward_messagescount = 35004,

            unknowns_close = 36001,
            unknowns_closeall = 36002,

            chat_down_button = 36003,

            pencil_click = 37000,
            addcontactbutton_click = 37001,
            chats_createbutton_click = 37002,
            addcontactbutton_burger_click = 37003,

            mentions_popup_show = 38000,
            mentions_inserted = 38001,
            mentions_sent = 38002,
            mentions_click_in_chat = 38003,

            messages_from_created_2_sent = 40000,
            messages_from_created_2_delivered = 40001,

            // Voip status.
            voip_calls_users_tech = 41000,
            voip_calls_interface_buttons = 41001,
            voip_calls_users_caller = 41002,

            // gui metrics
            app_start = 50000,
            chat_open = 50001,
            forward_list = 50004,
            photo_open = 50005,
            video_open = 50006,
            groupchat_create = 50007,

            videocall_from_click_to_window = 50008,
            videocall_from_click_to_calling = 50009,
            videocall_from_click_to_camera = 50010,
            audiocall_from_click_to_window = 50011,
            audiocall_from_click_to_calling = 50012,

            mentions_suggest_request_roundtrip = 50013,

            chats_unknown = 51000,
            chats_unknown_senders = 51001,
            chats_unknown_senders_close_all = 51002,
            chats_unknown_senders_add = 51003,
            chats_unknown_senders_close = 51004,

            chat_unknown = 51005,
            chat_unknown_message = 51006,
            chat_unknown_call = 51007,
            chat_unknown_add = 51008,
            chat_unknown_report = 51009,
            chat_unknown_report_result = 51010,
            chat_unknown_close = 51011,
            chat_unknown_close_result = 51012,

            chats_unknown_close = 51013,
            chats_unknown_close_choose = 51014,
            chats_unknown_close_report = 51015,
            chats_unknown_close_close = 51016,

            gdpr_show_start = 52000,
            gdpr_show_update = 52001,
            gdpr_accept_start = 52002,
            gdpr_accept_update = 52003,

            send_used_traffic_size_event = 53000,
            appstart_autostart_event = 53001,
            appstart_byicon_event = 53002,
            appstart_fromsuspend_event = 53003,
            appforeground_byicon_event = 53004,
            appforeground_bynotif_event = 53005,
            thresholdreached_event = 53006,
            appconnected_event = 53007,
            appcontentupdated_event = 53008,
            chatscr_conmodeconnected_event = 53009,
            chatscr_conmodeupdated_event = 53010,

            conlistscr_addcon_action = 54000,
            scr_addcon_action = 54001,

            chatscr_dtpscreensize_event = 55000,
            chatscr_chatsize_event = 55001,
            chatscr_dtpframescount_event = 55002,
            chatscr_dtpscreentype_event = 55003,

            profilescr_chatgallery_action = 56000,
            chatgalleryscr_media_action = 56001,
            chatgalleryscr_media_event = 56002,
            chatgalleryscr_topmenu_action = 56003,
            chatgalleryscr_selectcat_action = 56004,
            chatgalleryscr_filemenu_action = 56005,
            chatgalleryscr_linkmenu_action = 56006,
            chatgalleryscr_phmenu_action = 56007,
            chatgalleryscr_vidmenu_action = 56008,
            chatgalleryscr_pttmenu_action = 56009,
            fullmediascr_view = 56010,
            fullmediascr_tap_action = 56011,

            settings_main_readall_action = 57000,
            chat_scr_read_all_event = 57001,

            chatscr_sendmedia_action = 58000,
            chatscr_sendmediawcapt_action = 58001,

            chat_reply_action = 59000,
            chat_copy_action = 59001,
            chat_pin_action = 59002,
            chat_unpin_action = 59003,
            chat_edit_action = 59004,
            chat_delete_action = 59005,
            chat_forward_action = 59006,
            chat_report_action = 59007,

            memory_state = 60000,
            memory_state_limit = 60001,

            chatscr_blockbar_event = 61000,
            chatscr_blockbar_action = 61001,
            profilescr_blockbuttons_action = 61002,

            chatscr_view = 62000,

            settingsscr_view = 63000,
            settingsscr_nickedit_action = 63001,
            settingsscr_nick_action = 63002,
            settingsscr_settings_action = 63003,
            settingsscr_notifications_action = 63004,
            settingsscr_privacy_action = 63005,
            settingsscr_appearance_action = 63006,
            settingsscr_stickers_action = 63007,
            settingsscr_video_action = 63008,
            settingsscr_hotkeys_action = 63009,
            settingsscr_language_action = 63010,
            settingsscr_writeus_action = 63011,
            settingsscr_restart_action = 63012,
            settingsscr_aboutapp_action = 63013,
            privacyscr_calltype_action = 63014,
            privacyscr_grouptype_action = 63015,

            profilescr_groupurl_action = 64000,
            sharingscr_choicecontact_action = 64001,
            chatscr_contactinfo_action = 64002,
            sharedcontactscr_action = 64003,
            profilescr_view = 64004,
            profilescr_message_action = 64005,
            profilescr_call_action = 64006,
            profilescr_unlock_action = 64007,
            profilescr_adding_action = 64008,
            adtochatscr_adding_action = 64009,
            profilescr_sharecontact_action = 64010,
            profilescr_notifications_event = 64011,
            profilescr_settings_event = 64012,
            groupsettingsscr_action = 64013,
            profilescr_chattomembers_action = 64014,
            profilescr_mobilenumber_action = 64015,
            profilescr_groups_action = 64016,
            profilescr_newgroup_action = 64017,
            profilescr_wallpaper_action = 64018,
            wallpaperscr_action = 64019,
            profilescr_members_action = 64020,
            profilescr_blocked_action = 64021,

            chatscr_join_action = 65000,
            chatscr_leave_action = 65001,
            chatscr_unblockuser_action = 65002,
            chatscr_mute_action = 65003,
            chatscr_unmute_action = 65004,
            chatscr_opencamera_action = 65005,
            chatscr_openmedgallery_action = 65006,
            chatscr_openfile_action = 65007,
            chatscr_opencontact_action = 65008,
            chatscr_opengeo_action = 65009,
            chatscr_openptt_action = 65010,
            chatscr_switchtostickers_action = 65011,
            chatscr_sentptt_action = 65012,
            chatscr_recordptt_action = 65013,
            chatscr_playptt_action = 65014,
            chatscr_recognptt_action = 65015,

            sharecontactscr_contact_action = 66000,

            max,
        };

        constexpr std::string_view stat_event_to_string(const stats_event_names _arg)
        {
            assert(_arg > stats_event_names::min);
            assert(_arg < stats_event_names::max);

            switch (_arg)
            {
            case stats_event_names::start_session : return std::string_view("Start_Session [session enable]");
            case stats_event_names::start_session_params_loaded : return std::string_view("Start_Session_Params_Loaded");

            // registration
            case stats_event_names::reg_page_phone : return std::string_view("Reg_Page_Phone");
            case stats_event_names::reg_login_phone : return std::string_view("Reg_Login_Phone");
            case stats_event_names::reg_page_uin : return std::string_view("Reg_Page_Uin");
            case stats_event_names::reg_login_uin : return std::string_view("Reg_Login_UIN");
            case stats_event_names::reg_edit_country : return std::string_view("Reg_Edit_Country");
            case stats_event_names::reg_error_phone : return std::string_view("Reg_Error_Phone");
            case stats_event_names::reg_sms_send : return std::string_view("Reg_Sms_Send");
            case stats_event_names::reg_sms_resend : return std::string_view("Reg_Sms_Resend");
            case stats_event_names::reg_error_code : return std::string_view("Reg_Error_Code");
            case stats_event_names::reg_edit_phone : return std::string_view("Reg_Edit_Phone");
            case stats_event_names::reg_error_uin : return std::string_view("Reg_Error_UIN");
            case stats_event_names::reg_error_other : return std::string_view("Reg_Error_Other");
            case stats_event_names::login_forgot_password : return std::string_view("Login_Forgot_Password");

            // main window
            case stats_event_names::main_window_fullscreen : return std::string_view("Mainwindow_Fullscreen");

            // filesharing
            case stats_event_names::filesharing_sent : return std::string_view("Filesharing_Sent");
            case stats_event_names::filesharing_sent_count : return std::string_view("Filesharing_Sent_Count");
            case stats_event_names::filesharing_sent_success : return std::string_view("Filesharing_Sent_Success_2");
                break;
            case stats_event_names::filesharing_count : return std::string_view("Filesharing_Count");
            case stats_event_names::filesharing_cancel : return std::string_view("Filesharing_Cancel");
            case stats_event_names::filesharing_download_file : return std::string_view("Filesharing_Download_File");
            case stats_event_names::filesharing_download_cancel : return std::string_view("Filesharing_Download_Cancel");
            case stats_event_names::filesharing_download_success : return std::string_view("Filesharing_Download_Success");

            // smiles and stickers
            case stats_event_names::smile_sent_picker : return std::string_view("Smile_Sent_Picker");
            case stats_event_names::smile_sent_from_recents : return std::string_view("Smile_Sent_From_Recents");
            case stats_event_names::sticker_sent_from_picker : return std::string_view("Sticker_Sent_From_Picker");
            case stats_event_names::sticker_sent_from_recents : return std::string_view("Sticker_Sent_From_Recents");
            case stats_event_names::picker_cathegory_click : return std::string_view("Picker_Cathegory_Click");
            case stats_event_names::picker_tab_click : return std::string_view("Picker_Tab_Click");

            case stats_event_names::stickers_download_pack : return std::string_view("Stickers_Download_Pack");
            case stats_event_names::stickers_share_pack : return std::string_view("Stickers_Share_Pack");
            case stats_event_names::stickers_discover_icon_picker : return std::string_view("Stickers_Discover_Icon_Picker");
            case stats_event_names::stickers_discover_pack_tap : return std::string_view("Stickers_Discover_Pack_Tap");
            case stats_event_names::stickers_discover_pack_download : return std::string_view("Stickers_Discover_Pack_Download");
            case stats_event_names::stickers_discover_addbot_bottom_tap : return std::string_view("Stickers_Discover_Addbot_Bottom_Tap");
            case stats_event_names::stickers_discover_sendbot_message : return std::string_view("Stickers_Discover_Sendbot_Message");
            case stats_event_names::stickers_longtap_picker : return std::string_view("Stickers_Longtap_Picker");
            case stats_event_names::stickers_longtap_preview : return std::string_view("Stickers_Longtap_Preview");
            case stats_event_names::stickers_sticker_tap_in_chat : return std::string_view("Stickers_Sticker_Tap_In_Chat");
            case stats_event_names::stickers_addpack_button_tap_in_chat : return std::string_view("Stickers_AddPack_Button_Tap_In_Chat");
            case stats_event_names::stickers_pack_delete : return std::string_view("Stickers_Pack_Delete");
            case stats_event_names::stickers_emoji_added_in_input : return std::string_view("Stickers_Emoji_Added_In_Input");
            case stats_event_names::stickers_suggests_appear_in_input  : return std::string_view("Stickers_Suggests_Appear_In_Input");
            case stats_event_names::stickers_suggested_sticker_sent : return std::string_view("Stickers_Suggested_Sticker_Sent");
            case stats_event_names::stickers_search_input_tap : return std::string_view("Stickers_Search_Input_Tap");
            case stats_event_names::stickers_search_request_send : return std::string_view("Stickers_Search_Request_Send");
            case stats_event_names::stickers_search_results_receive : return std::string_view("Stickers_Search_Results_Receive");
            case stats_event_names::stickers_search_pack_tap : return std::string_view("Stickers_Search_Pack_Tap");
            case stats_event_names::stickers_search_add_pack_button_tap : return std::string_view("Stickers_Search_Add_Pack_Button_Tap");
            case stats_event_names::stickers_search_pack_download : return std::string_view("Stickers_Search_Pack_Download");

            // alerts
            case stats_event_names::alert_click : return std::string_view("Alert_Click");
            case stats_event_names::alert_viewall : return std::string_view("Alert_ViewAll");
            case stats_event_names::alert_close : return std::string_view("Alert_Close");
            case stats_event_names::alert_mail_common:  return std::string_view("Alert_Mail_Common");
            case stats_event_names::alert_mail_letter: return std::string_view("Alert_Mail_Letter");
            case stats_event_names::tray_mail: return std::string_view("Tray_Mail");
            case stats_event_names::titlebar_message: return std::string_view("Titlebar_Message");
            case stats_event_names::titlebar_mail: return std::string_view("Titlebar_Mail");

            // cl options
            case stats_event_names::add_contact: return std::string_view("Add_Contact");
            case stats_event_names::spam_contact : return std::string_view("Spam_Contact");
            case stats_event_names::ignore_contact: return std::string_view("Ignore_Contact");
            case stats_event_names::delete_contact: return std::string_view("Delete_Contact");

            // cl
            case stats_event_names::recents_close : return std::string_view("Recents_Close");
            case stats_event_names::mark_read : return std::string_view("Mark_Read");
            case stats_event_names::mark_read_all : return std::string_view("Mark_Read_All");

            case stats_event_names::mute_recents_menu : return std::string_view("Mute_Recents_Menu");
            case stats_event_names::mute_sidebar : return std::string_view("Mute_Sidebar");
            case stats_event_names::unmute : return std::string_view("Unmute");
            case stats_event_names::mark_unread: return std::string_view("Mark_Unread");

            case stats_event_names::ignorelist_open : return std::string_view("Ignorelist_Open");
            case stats_event_names::ignorelist_remove : return std::string_view("Ignorelist_Remove");

            case stats_event_names::cl_empty_android : return std::string_view("CL_Empty_Android");
            case stats_event_names::cl_empty_ios : return std::string_view("CL_Empty_IOS");

            case stats_event_names::cl_search : return std::string_view("CL_Search");
            case stats_event_names::cl_search_openmessage : return std::string_view("CL_Search_OpenMessage");
            case stats_event_names::cl_search_dialog : return std::string_view("CL_Search_Dialog");
            case stats_event_names::cl_search_dialog_openmessage : return std::string_view("CL_Search_Dialog_OpenMessage");
            case stats_event_names::cl_search_nohistory : return std::string_view("CL_Search_NoHistory");

            case stats_event_names::chat_search_dialog_opened: return std::string_view("ChatSearchScr_View");
            case stats_event_names::chat_search_dialog_suggest_click: return std::string_view("ChatScr_HistSelect_Action");
            case stats_event_names::chat_search_dialog_first_symb_typed: return std::string_view("ChatScr_Type_Action");
            case stats_event_names::chat_search_dialog_cancel: return std::string_view("ChatScr_Clear_Action");
            case stats_event_names::chat_search_dialog_server_first_req: return std::string_view("ChatScr_ResRequest_Event");
            case stats_event_names::chat_search_dialog_server_first_ans: return std::string_view("ChatScr_ResLoaded_Event");
            case stats_event_names::chat_search_dialog_item_click: return std::string_view("ChatScr_Res_Action");
            case stats_event_names::chat_search_dialog_scroll: return std::string_view("ChatScr_ResScroll_Action");

            // profile
            case stats_event_names::myprofile_open : return std::string_view("Myprofile_Open");
            case stats_event_names::myprofile_edit: return std::string_view("Myprofile_Edit");
            case stats_event_names::profile_open : return std::string_view("Profile_Open");
            case stats_event_names::profile_sidebar : return std::string_view("Profile_Sidebar");
            case stats_event_names::profile_write_message: return std::string_view("Profile_Write_Message");

            case stats_event_names::profileeditscr_view: return std::string_view("ProfileEditScr_View");
            case stats_event_names::profileeditscr_avatar_action: return std::string_view("ProfileEditScr_Avatar_Action");
            case stats_event_names::profileeditscr_name_action: return std::string_view("ProfileEditScr_Name_Action");
            case stats_event_names::profileeditscr_aboutme_action: return std::string_view("ProfileEditScr_AboutMe_Action");
            case stats_event_names::profileeditscr_changenum_action: return std::string_view("ProfileEditScr_ChangeNum_Action");
            case stats_event_names::profileeditscr_nickedit_action: return std::string_view("ProfileEditScr_NickEdit_Action");
            case stats_event_names::profileeditscr_access_action: return std::string_view("ProfileEditScr_Access_Action");
            case stats_event_names::profileeditscr_signout_action: return std::string_view("ProfileEditScr_SignOut_Action");

            case stats_event_names::nickeditscr_edit_action: return std::string_view("NickEditScr_Edit_Action");

            // unknowns
            case stats_event_names::unknowns_close: return std::string_view("Unknowns_Close");
            case stats_event_names::unknowns_closeall: return std::string_view("Unknowns_CloseAll");

            // messaging
            case stats_event_names::message_send_button : return std::string_view("Message_Send_Button");
            case stats_event_names::message_enter_button : return std::string_view("Message_Enter_Button");

            case stats_event_names::open_chat: return std::string_view("Open_Chat");

            case stats_event_names::message_pending : return std::string_view("Message_Pending");
            case stats_event_names::message_delete_my : return std::string_view("message_delete_my");
            case stats_event_names::message_delete_all : return std::string_view("message_delete_all");

            case stats_event_names::history_delete : return std::string_view("history_delete");
            case stats_event_names::more_button_click: return std::string_view("More_Button_Click");

            case stats_event_names::feedback_show : return std::string_view("Feedback_Show");
            case stats_event_names::feedback_sent : return std::string_view("Feedback_Sent");
            case stats_event_names::feedback_error : return std::string_view("Feedback_Error");

            case stats_event_names::call_audio : return std::string_view("Call_Audio");
            case stats_event_names::call_video: return std::string_view("Call_Video");

            case stats_event_names::aboutus_show : return std::string_view("AboutUs_Show");
            case stats_event_names::client_settings : return std::string_view("Client_Settings");

            case stats_event_names::proxy_open : return std::string_view("proxy_open");
            case stats_event_names::proxy_set : return std::string_view("proxy_set");

            case stats_event_names::favorites_set : return std::string_view("favorites_set");
            case stats_event_names::favorites_unset : return std::string_view("favorites_unset");
            case stats_event_names::favorites_load : return std::string_view("favorites_load");

            case stats_event_names::profile_avatar_changed : return std::string_view("Profile_Avatar_Changed");
            case stats_event_names::introduce_name_set : return std::string_view("Introduce_Name_Set");
            case stats_event_names::introduce_avatar_changed : return std::string_view("Introduce_Avatar_Changed");
            case stats_event_names::introduce_avatar_fail : return std::string_view("Introduce_Avatar_Fail");

            // Masks
            case stats_event_names::masks_open : return std::string_view("Masks_Open");
            case stats_event_names::masks_select : return std::string_view("Masks_Select");

            // Groupchats, livechats
            case stats_event_names::chats_create_open : return std::string_view("Chats_Create_Open");
            case stats_event_names::chats_create_public : return std::string_view("Chats_Create_Public");
            case stats_event_names::chats_create_approval : return std::string_view("Chats_Create_Approval");
            case stats_event_names::chats_create_readonly : return std::string_view("Chats_Create_Readonly");
            case stats_event_names::chats_create_rename : return std::string_view("Chats_Create_Rename");
            case stats_event_names::chats_create_avatar : return std::string_view("Chats_Create_Avatar");
            case stats_event_names::chats_created : return std::string_view("Chats_Created");
            case stats_event_names::chats_leave: return std::string_view("Chats_Leave");
            case stats_event_names::chats_rename: return std::string_view("Chats_Rename");
            case stats_event_names::chats_avatar_changed: return std::string_view("Chats_Avatar_Changed");
            case stats_event_names::chats_add_member_sidebar: return std::string_view("Chats_Add_member_Sidebar");
            case stats_event_names::chats_add_member_dialog: return std::string_view("Chats_Add_member_Dialog");
            case stats_event_names::chats_join_frompopup: return std::string_view("Chats_Join_FromPopup");
            case stats_event_names::chats_admins: return std::string_view("Chats_Admins");
            case stats_event_names::chats_blocked: return std::string_view("Chats_Blocked");
            case stats_event_names::chats_open_popup: return std::string_view("Chats_Open_Popup");


            // Replies and forwards
            case stats_event_names::quotes_send_alone : return std::string_view("Quotes_Send_Alone");
            case stats_event_names::quotes_send_answer : return std::string_view("Quotes_Send_Answer");
            case stats_event_names::quotes_messagescount : return std::string_view("Quotes_MessagesCount");

            case stats_event_names::forward_send_frommenu : return std::string_view("Forward_Send_FromMenu");
            case stats_event_names::forward_send_frombutton : return std::string_view("Forward_Send_FromButton");
            case stats_event_names::forward_send_preview : return std::string_view("Forward_Send_Preview");
            case stats_event_names::forward_messagescount : return std::string_view("Forward_MessagesCount");

            case stats_event_names::chat_down_button: return std::string_view("Chat_DownButton");

            case stats_event_names::pencil_click: return std::string_view("Pencil_Click");
            case stats_event_names::addcontactbutton_click: return std::string_view("AddContactButton_Click");
            case stats_event_names::chats_createbutton_click: return std::string_view("Chats_CreateButton_Click");
            case stats_event_names::addcontactbutton_burger_click: return std::string_view("AddContactButton_Burger_Click");

            case stats_event_names::mentions_popup_show: return std::string_view("Mentions_Popup_Show");
            case stats_event_names::mentions_inserted: return std::string_view("Mentions_Inserted");
            case stats_event_names::mentions_sent: return std::string_view("Mentions_Sent");
            case stats_event_names::mentions_click_in_chat: return std::string_view("Mentions_Click_In_Chat");

            case stats_event_names::messages_from_created_2_sent: return std::string_view("Messages_fromCreatedToSent");
            case stats_event_names::messages_from_created_2_delivered: return std::string_view("Messages_fromCreatedToDelivered");

            case stats_event_names::voip_calls_users_tech: return std::string_view("Calls_Users_Tech");
            case stats_event_names::voip_calls_interface_buttons: return std::string_view("Calls_Interface_Buttons");
            case stats_event_names::voip_calls_users_caller: return std::string_view("Calls_Users_Caller");

            case stats_event_names::app_start: return std::string_view("APP_START");
            case stats_event_names::chat_open: return std::string_view("CHAT_OPEN");

            case stats_event_names::forward_list: return std::string_view("FORWARD_LIST");
            case stats_event_names::photo_open: return std::string_view("PHOTO_OPEN");
            case stats_event_names::video_open: return std::string_view("VIDEO_OPEN");
            case stats_event_names::groupchat_create: return std::string_view("GROUPCHAT_CREATE");

            case stats_event_names::videocall_from_click_to_window: return std::string_view("VIDEOCALL_FROM_CLICK_TO_WINDOW");
            case stats_event_names::videocall_from_click_to_calling: return std::string_view("VIDEOCALL_FROM_CLICK_TO_CALLING");
            case stats_event_names::videocall_from_click_to_camera: return std::string_view("VIDEOCALL_FROM_CLICK_TO_CAMERA");
            case stats_event_names::audiocall_from_click_to_window: return std::string_view("AUDIOCALL_FROM_CLICK_TO_WINDOW");
            case stats_event_names::audiocall_from_click_to_calling: return std::string_view("AUDIOCALL_FROM_CLICK_TO_CALLING");

            case stats_event_names::mentions_suggest_request_roundtrip: return std::string_view("Mentions_Suggest_Request_Roundtrip");

            case stats_event_names::chats_unknown: return std::string_view("Chats_Unknown");
            case stats_event_names::chats_unknown_senders: return std::string_view("Chats_Unknown_Senders");
            case stats_event_names::chats_unknown_senders_close_all: return std::string_view("Chats_Unknown_Senders_Close_All");
            case stats_event_names::chats_unknown_senders_add: return std::string_view("Chats_Unknown_Senders_Add");
            case stats_event_names::chats_unknown_senders_close: return std::string_view("Chats_Unknown_Senders_Close");

            case stats_event_names::chat_unknown: return std::string_view("Chat_Unknown");
            case stats_event_names::chat_unknown_message: return std::string_view("Chat_Unknown_Message");
            case stats_event_names::chat_unknown_call: return std::string_view("Chat_Unknown_Call");
            case stats_event_names::chat_unknown_add: return std::string_view("Chat_Unknown_Add");
            case stats_event_names::chat_unknown_report: return std::string_view("Chat_Unknown_Report");
            case stats_event_names::chat_unknown_report_result: return std::string_view("Chat_Unknown_Report_Result");
            case stats_event_names::chat_unknown_close: return std::string_view("Chat_Unknown_Close");
            case stats_event_names::chat_unknown_close_result: return std::string_view("Chat_Unknown_Close_Result");

            case stats_event_names::chats_unknown_close: return std::string_view("Chats_Unknown_Close");
            case stats_event_names::chats_unknown_close_choose: return std::string_view("Chats_Unknown_Close_Choose");
            case stats_event_names::chats_unknown_close_report: return std::string_view("Chats_Unknown_Close_Report");
            case stats_event_names::chats_unknown_close_close: return std::string_view("Chats_Unknown_Close_Close");


            case stats_event_names::gdpr_accept_start: return std::string_view("GDPR_Accept_Start");
            case stats_event_names::gdpr_accept_update: return std::string_view("GDPR_Accept_Update");
            case stats_event_names::gdpr_show_start: return std::string_view("GDPR_Show_Start");
            case stats_event_names::gdpr_show_update: return std::string_view("GDPR_Show_Update");

            case stats_event_names::send_used_traffic_size_event: return std::string_view("SendUsedTrafficSize_Event");
            case stats_event_names::appstart_autostart_event: return std::string_view("AppStart_Autostart_Event");
            case stats_event_names::appstart_byicon_event: return std::string_view("AppStart_ByIcon_Event");
            case stats_event_names::appstart_fromsuspend_event: return std::string_view("AppStart_FromSuspend_Event");
            case stats_event_names::appforeground_byicon_event: return std::string_view("AppForeground_ByIcon_Event");
            case stats_event_names::appforeground_bynotif_event: return std::string_view("AppForeground_ByNotif_Event");
            case stats_event_names::thresholdreached_event: return std::string_view("ThresholdReached_Event");
            case stats_event_names::appconnected_event: return std::string_view("AppConnected_Event");
            case stats_event_names::appcontentupdated_event: return std::string_view("AppContentUpdated_Event");
            case stats_event_names::chatscr_conmodeconnected_event: return std::string_view("ChatScr_ConModeConnected_Event");
            case stats_event_names::chatscr_conmodeupdated_event: return std::string_view("ChatScr_ConModeUpdated_Event");

            case stats_event_names::conlistscr_addcon_action: return std::string_view("ConListScr_AddCon_Action");
            case stats_event_names::scr_addcon_action: return std::string_view("Scr_AddCon_Action");

            case stats_event_names::chatscr_dtpscreensize_event: return std::string_view("ChatScr_DTPScreenSize_Event");
            case stats_event_names::chatscr_chatsize_event: return std::string_view("ChatScr_ChatSize_Event");
            case stats_event_names::chatscr_dtpframescount_event: return std::string_view("ChatScr_DTPFramesCount_Event");
            case stats_event_names::chatscr_dtpscreentype_event: return std::string_view("ChatScr_DTPScreenSize_Event");

            case stats_event_names::profilescr_chatgallery_action: return std::string_view("ProfileScr_ChatGallery_Action");
            case stats_event_names::chatgalleryscr_media_action: return std::string_view("ChatGalleryScr_Media_Action");
            case stats_event_names::chatgalleryscr_media_event: return std::string_view("ChatGalleryScr_Media_Event");
            case stats_event_names::chatgalleryscr_topmenu_action: return std::string_view("ChatGalleryScr_TopMenu_Action");
            case stats_event_names::chatgalleryscr_selectcat_action: return std::string_view("ChatGalleryScr_SelectCat_Action");
            case stats_event_names::chatgalleryscr_filemenu_action: return std::string_view("ChatGalleryScr_FileMenu_Action");
            case stats_event_names::chatgalleryscr_linkmenu_action: return std::string_view("ChatGalleryScr_LinkMenu_Action");
            case stats_event_names::chatgalleryscr_phmenu_action: return std::string_view("ChatGalleryScr_PhMenu_Action");
            case stats_event_names::chatgalleryscr_vidmenu_action: return std::string_view("ChatGalleryScr_VidMenu_Action");
            case stats_event_names::chatgalleryscr_pttmenu_action: return std::string_view("ChatGalleryScr_PttMenu_Action");
            case stats_event_names::fullmediascr_view: return std::string_view("FullMediaScr_View");
            case stats_event_names::fullmediascr_tap_action: return std::string_view("FullMediaScr_Tap_Action");

            case stats_event_names::settings_main_readall_action: return std::string_view("SettingsMain_ReadAll_Action");
            case stats_event_names::chat_scr_read_all_event: return std::string_view("ChatScr_ReadAll_Event");

            case stats_event_names::chatscr_sendmedia_action: return std::string_view("ChatScr_SendMedia_Action");
            case stats_event_names::chatscr_sendmediawcapt_action: return std::string_view("ChatScr_SendMediawCapt_Action");

            case stats_event_names::chat_reply_action: return std::string_view("Chat_action_reply");
            case stats_event_names::chat_copy_action: return std::string_view("Chat_action_copy");
            case stats_event_names::chat_pin_action: return std::string_view("Chat_action_pin");
            case stats_event_names::chat_unpin_action: return std::string_view("Chat_action_unpin");
            case stats_event_names::chat_edit_action: return std::string_view("Chat_action_edit");
            case stats_event_names::chat_delete_action: return std::string_view("Chat_action_delete");
            case stats_event_names::chat_forward_action: return std::string_view("Chat_action_forward");
            case stats_event_names::chat_report_action: return std::string_view("Chat_action_report");

            case stats_event_names::memory_state: return std::string_view("memory_state");
            case stats_event_names::memory_state_limit: return std::string_view("memory_state_limit");

            case stats_event_names::chatscr_blockbar_event: return std::string_view("ChatScr_BlockBar_Event");
            case stats_event_names::chatscr_blockbar_action: return std::string_view("ChatScr_BlockBar_Action");
            case stats_event_names::profilescr_blockbuttons_action: return std::string_view("ProfileScr_BlockButtons_Action");

            case stats_event_names::chatscr_view: return std::string_view("ChatScr_View");

            case stats_event_names::settingsscr_view: return std::string_view("SettingsScr_View");
            case stats_event_names::settingsscr_nickedit_action: return std::string_view("SettingsScr_NickEdit_Action");
            case stats_event_names::settingsscr_nick_action: return std::string_view("SettingsScr_Nick_Action");
            case stats_event_names::settingsscr_settings_action: return std::string_view("SettingsScr_Settings_Action");
            case stats_event_names::settingsscr_notifications_action: return std::string_view("SettingsScr_Notifications_Action");
            case stats_event_names::settingsscr_privacy_action: return std::string_view("SettingsScr_Privacy_Action");
            case stats_event_names::settingsscr_appearance_action: return std::string_view("SettingsScr_Appearance_Action");
            case stats_event_names::settingsscr_stickers_action: return std::string_view("SettingsScr_Stickers_Action");
            case stats_event_names::settingsscr_video_action: return std::string_view("SettingsScr_Video_Action");
            case stats_event_names::settingsscr_hotkeys_action: return std::string_view("SettingsScr_HotKeys_Action");
            case stats_event_names::settingsscr_language_action: return std::string_view("SettingsScr_Language_Action");
            case stats_event_names::settingsscr_writeus_action: return std::string_view("SettingsScr_WriteUs_Action");
            case stats_event_names::settingsscr_restart_action: return std::string_view("SettingsScr_ReStart_Action");
            case stats_event_names::settingsscr_aboutapp_action: return std::string_view("SettingsScr_AboutApp_Action");
            case stats_event_names::privacyscr_calltype_action: return std::string_view("PrivacyScr_CallType_Action");
            case stats_event_names::privacyscr_grouptype_action: return std::string_view("PrivacyScr_GroupType_Action");

            case stats_event_names::profilescr_groupurl_action: return std::string_view("ProfileScr_GroupUrl_Action");
            case stats_event_names::sharingscr_choicecontact_action: return std::string_view("SharingScr_choiceContact_Action");
            case stats_event_names::chatscr_contactinfo_action: return std::string_view("ChatScr_ContactInfo_Action");
            case stats_event_names::sharedcontactscr_action: return std::string_view("SharedContactScr_Action");
            case stats_event_names::profilescr_view: return std::string_view("ProfileScr_View");
            case stats_event_names::profilescr_message_action: return std::string_view("ProfileScr_Message_Action");
            case stats_event_names::profilescr_call_action: return std::string_view("ProfileScr_Call_Action");
            case stats_event_names::profilescr_unlock_action: return std::string_view("ProfileScr_Unlock_Action");
            case stats_event_names::profilescr_adding_action: return std::string_view("ProfileScr_Adding_Action");
            case stats_event_names::adtochatscr_adding_action: return std::string_view("AdToChatScr_Adding_Action");
            case stats_event_names::profilescr_sharecontact_action: return std::string_view("ProfileScr_ShareContact_Action");
            case stats_event_names::profilescr_notifications_event: return std::string_view("ProfileScr_Notifications_Event");
            case stats_event_names::profilescr_settings_event: return std::string_view("ProfileScr_Settings_Event");
            case stats_event_names::groupsettingsscr_action: return std::string_view("GroupSettingsScr_Action");
            case stats_event_names::profilescr_chattomembers_action: return std::string_view("ProfileScr_ChatToMembers_Action");
            case stats_event_names::profilescr_mobilenumber_action: return std::string_view("ProfileScr_MobileNumber_Action");
            case stats_event_names::profilescr_groups_action: return std::string_view("ProfileScr_Groups_Action");
            case stats_event_names::profilescr_newgroup_action: return std::string_view("ProfileScr_NewGroup_Action");
            case stats_event_names::profilescr_wallpaper_action: return std::string_view("ProfileScr_Wallpaper_Action");
            case stats_event_names::wallpaperscr_action: return std::string_view("WallpaperScr_Action");
            case stats_event_names::profilescr_members_action: return std::string_view("ProfileScr_Members_Action");
            case stats_event_names::profilescr_blocked_action: return std::string_view("ProfileScr_Blocked_Action");

            case stats_event_names::chatscr_join_action: return std::string_view("ChatScr_Join_Action");
            case stats_event_names::chatscr_leave_action: return std::string_view("ChatScr_Leave_Action");
            case stats_event_names::chatscr_unblockuser_action: return std::string_view("ChatScr_UnblockUser_Action");
            case stats_event_names::chatscr_mute_action: return std::string_view("ChatScr_Mute_Action");
            case stats_event_names::chatscr_unmute_action: return std::string_view("ChatScr_Unmute_Action");
            case stats_event_names::chatscr_opencamera_action: return std::string_view("ChatScr_OpenCamera_Action");
            case stats_event_names::chatscr_openmedgallery_action: return std::string_view("ChatScr_OpenMedGallery_Action");
            case stats_event_names::chatscr_openfile_action: return std::string_view("ChatScr_OpenFile_Action");
            case stats_event_names::chatscr_opencontact_action: return std::string_view("ChatScr_OpenContact_Action");
            case stats_event_names::chatscr_opengeo_action: return std::string_view("ChatScr_OpenGeo_Action");
            case stats_event_names::chatscr_openptt_action: return std::string_view("ChatScr_OpenPTT_Action");
            case stats_event_names::chatscr_switchtostickers_action: return std::string_view("ChatScr_SwitchToStickers_Action");
            case stats_event_names::chatscr_sentptt_action: return std::string_view("ChatScr_PTTSent_Action");
            case stats_event_names::chatscr_recordptt_action: return std::string_view("ChatScr_PTTRecord_Action");
            case stats_event_names::chatscr_playptt_action: return std::string_view("ChatScr_PTTPlay_Action");
            case stats_event_names::chatscr_recognptt_action: return std::string_view("ChatScr_PTTRecogn_Action");

            case stats_event_names::sharecontactscr_contact_action: return std::string_view("ShareContactScr_Contact_Action");

            default:
                return std::string_view();
            }
        }

        enum class im_stat_event_names
        {
            // drop old events name 1-5
            min = 5,

            memory_state = 6,
            memory_state_limit = 7,

            u_network_communication_event = 8,
            u_network_error_event = 9,
            u_network_http_code_event = 10,
            u_network_parsing_error_event = 11,
            u_network_api_error_event = 12,

            network_communication_event = 1000,
            network_error_event = 1001,
            network_http_code_event = 1002,
            network_parsing_error_event = 1003,
            network_api_error_event = 1004,

            reg_login_phone = 2000,
            reg_login_uin = 2001,
            reg_sms_resend = 2002,
            reg_error_code = 2003,
            reg_error_uin = 2004,

            messages_from_created_2_sent = 3000,
            messages_from_created_2_delivered = 3001,

            crash = 4000,

            max
        };

        constexpr std::string_view im_stat_event_to_string(const im_stat_event_names _arg)
        {
            assert(_arg > im_stat_event_names::min);
            assert(_arg < im_stat_event_names::max);

            switch (_arg)
            {
            case im_stat_event_names::memory_state: return std::string_view("memory_state");
            case im_stat_event_names::memory_state_limit: return std::string_view("memory_state_limit");

            case im_stat_event_names::u_network_communication_event: return std::string_view("u_im_network_communication");
            case im_stat_event_names::u_network_error_event: return std::string_view("u_im_network_error");
            case im_stat_event_names::u_network_http_code_event: return std::string_view("u_im_network_http_code");
            case im_stat_event_names::u_network_parsing_error_event: return std::string_view("u_im_network_parsing_error");
            case im_stat_event_names::u_network_api_error_event: return std::string_view("u_im_network_api_error");

            case im_stat_event_names::network_communication_event: return std::string_view("im_network_communication");
            case im_stat_event_names::network_error_event: return std::string_view("im_network_error");
            case im_stat_event_names::network_http_code_event: return std::string_view("im_network_http_code");
            case im_stat_event_names::network_parsing_error_event: return std::string_view("im_network_parsing_error");
            case im_stat_event_names::network_api_error_event: return std::string_view("im_network_api_error");

            case im_stat_event_names::reg_login_phone: return std::string_view("im_reg_login_phone");
            case im_stat_event_names::reg_login_uin: return std::string_view("im_reg_login_uin");
            case im_stat_event_names::reg_sms_resend: return std::string_view("im_reg_sms_resend");
            case im_stat_event_names::reg_error_code: return std::string_view("im_reg_error_code");
            case im_stat_event_names::reg_error_uin: return std::string_view("im_reg_error_uin");

            case im_stat_event_names::messages_from_created_2_sent: return std::string_view("messages_from_created_2_sent");
            case im_stat_event_names::messages_from_created_2_delivered: return std::string_view("messages_from_created_2_delivered");

            case im_stat_event_names::crash: return std::string_view("im_crash");

            default:
                return std::string_view();
            }
        }
    }

    enum class proxy_type
    {
        auto_proxy = -1,

        // positive values are reserved for curl_proxytype
        http_proxy = 0,
        socks4 = 4,
        socks5 = 5,
    };

    enum class proxy_auth
    {
        basic = 0,
        digest,
        negotiate,
        ntlm,
    };

    inline std::ostream& operator<<(std::ostream &oss, const file_sharing_content_type arg)
    {
        switch (arg.type_)
        {
            case file_sharing_base_content_type::gif: return (oss << "type = gif");
            case file_sharing_base_content_type::image: return (oss << "type = image");
            case file_sharing_base_content_type::ptt: return (oss << "type = ptt");
            case file_sharing_base_content_type::undefined: return (oss << "type = undefined");
            case file_sharing_base_content_type::video: return (oss << "type = video");
            default:
                assert(!"unexpected file sharing content type");
        }

        switch (arg.subtype_)
        {
        case file_sharing_sub_content_type::sticker: return (oss << "subtype = sticker");
        default:
            assert(!"unexpected file sharing content subtype");
        }

        return oss;
    }

    enum class privacy_access_right
    {
        not_set = 0,

        everybody = 1,
        my_contacts = 2,
        nobody = 3,
    };

    inline std::string to_string(const privacy_access_right _value)
    {
        switch (_value)
        {
        case privacy_access_right::everybody:
            return "everybody";

        case privacy_access_right::my_contacts:
            return "myContacts";

        case privacy_access_right::nobody:
            return "nobody";

        default:
            break;
        }

        assert(!"unknown value");
        return std::string();
    }

    inline privacy_access_right from_string(const std::string_view _string)
    {
        if (_string == "everybody")
            return privacy_access_right::everybody;
        else if (_string == "myContacts")
            return privacy_access_right::my_contacts;
        else if (_string == "nobody")
            return privacy_access_right::nobody;

        assert(!"unknown value");
        return privacy_access_right::not_set;
    }

    enum class nickname_errors
    {
        success = 0,
        unknown_error = 1,
        bad_value = 2,
        already_used = 3,
        not_allowed = 4
    };

    enum class token_type
    {
        basic,
        otp_via_email
    };
}
