#pragma once

namespace core
{
    namespace stats
    {
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
            u_network_decompression_error = 13,
            u_network_compression_ignored = 14,

            network_communication_event = 1000, // obsolete, use u_network_communication_event
            network_error_event = 1001, // obsolete, use u_network_error_event
            network_http_code_event = 1002, // obsolete, use u_network_http_code_event
            network_parsing_error_event = 1003, // obsolete, use u_network_parsing_error_event
            network_api_error_event = 1004, // obsolete, use u_network_api_error_event

            reg_login_phone = 2000,
            reg_login_uin = 2001,
            reg_sms_resend = 2002,
            reg_error_code = 2003,
            reg_error_uin = 2004,

            messages_from_created_to_sent = 3000,
            messages_from_created_to_delivered = 3001,

            crash = 4000,

            text_sticker_suggested = 5000,
            text_sticker_used = 5001,

            stickers_smartreply_appear_in_input = 5002,
            stickers_smartreply_sticker_sent = 5003,
            smartreply_text_appear_in_input = 5004,
            smartreply_text_sent = 5005,

            stickers_suggests_appear_in_input = 5006,
            stickers_suggested_sticker_sent = 5007,

            smartreply_settings_swtich = 5008,

            installer_ok = 6000,
            installer_open_files_archive = 6001,
            installer_create_product_folder = 6002,
            installer_open_file_for_write = 6003,
            installer_write_file = 6004,
            installer_start_exe = 6005,
            installer_open_registry_key = 6007,
            installer_set_registry_value = 6008,
            installer_get_registry_value = 6009,
            installer_create_registry_key = 6010,
            installer_copy_installer = 6011,
            installer_get_special_folder = 6012,
            installer_write_to_uninstall = 6013,
            installer_copy_installer_to_temp = 6014,
            installer_get_temp_path = 6015,
            installer_get_temp_file_name = 6016,
            installer_get_module_name = 6017,
            installer_start_installer_from_temp = 6018,
            installer_invalid_installer_pack = 6019,
            installer_terminate_connect_timeout = 6020,
            installer_terminate_procid_rcvd = 6021,
            installer_terminate_open_proc_failed = 6022,
            installer_terminate_shutdown_sent = 6023,
            installer_terminate_abnormal_open_proc_failed = 6024,
            installer_terminate_abnormal_wait_timeout = 6025,
            installer_generate_guid = 6026,
            installer_move_file = 6027,

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
            case im_stat_event_names::u_network_decompression_error: return std::string_view("u_network_decompression_error");
            case im_stat_event_names::u_network_compression_ignored: return std::string_view("u_network_compression_ignored");

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

            case im_stat_event_names::messages_from_created_to_sent: return std::string_view("messages_from_created_to_sent");
            case im_stat_event_names::messages_from_created_to_delivered: return std::string_view("messages_from_created_to_delivered");

            case im_stat_event_names::crash: return std::string_view("im_crash");

            case im_stat_event_names::text_sticker_suggested: return std::string_view("im-text-sticker-suggested");
            case im_stat_event_names::text_sticker_used: return std::string_view("im-text-sticker-used");

            case im_stat_event_names::stickers_smartreply_appear_in_input: return std::string_view("stickers_smartreply_appear_in_input");
            case im_stat_event_names::stickers_smartreply_sticker_sent: return std::string_view("stickers_smartreply_sticker_sent");
            case im_stat_event_names::smartreply_text_appear_in_input: return std::string_view("smartreply_text_appear_in_input");
            case im_stat_event_names::smartreply_text_sent: return std::string_view("smartreply_text_sent");

            case im_stat_event_names::stickers_suggests_appear_in_input: return std::string_view("stickers_suggests_appear_in_input");
            case im_stat_event_names::stickers_suggested_sticker_sent: return std::string_view("stickers_suggested_sticker_sent");

            case im_stat_event_names::smartreply_settings_swtich: return std::string_view("settingsscr_smartreply_action");

            case im_stat_event_names::installer_ok: return std::string_view("im_installer_ok");
            case im_stat_event_names::installer_open_files_archive: return std::string_view("im_installer_open_files_archive");
            case im_stat_event_names::installer_create_product_folder: return std::string_view("im_installer_create_product_folder");
            case im_stat_event_names::installer_open_file_for_write: return std::string_view("im_installer_open_file_for_write");
            case im_stat_event_names::installer_write_file: return std::string_view("im_installer_write_file");
            case im_stat_event_names::installer_start_exe: return std::string_view("im_installer_start_exe");
            case im_stat_event_names::installer_open_registry_key: return std::string_view("im_installer_open_registry_key");
            case im_stat_event_names::installer_set_registry_value: return std::string_view("im_installer_set_registry_value");
            case im_stat_event_names::installer_get_registry_value: return std::string_view("im_installer_get_registry_value");
            case im_stat_event_names::installer_create_registry_key: return std::string_view("im_installer_create_registry_key");
            case im_stat_event_names::installer_copy_installer: return std::string_view("im_installer_copy_installer");
            case im_stat_event_names::installer_get_special_folder: return std::string_view("im_installer_get_special_folder");
            case im_stat_event_names::installer_write_to_uninstall: return std::string_view("im_installer_write_to_uninstall");
            case im_stat_event_names::installer_copy_installer_to_temp: return std::string_view("im_installer_copy_installer_to_temp");
            case im_stat_event_names::installer_get_temp_path: return std::string_view("im_installer_get_temp_path");
            case im_stat_event_names::installer_get_temp_file_name: return std::string_view("im_installer_get_temp_file_name");
            case im_stat_event_names::installer_get_module_name: return std::string_view("im_installer_get_module_name");
            case im_stat_event_names::installer_start_installer_from_temp: return std::string_view("im_installer_start_installer_from_temp");
            case im_stat_event_names::installer_invalid_installer_pack: return std::string_view("im_installer_invalid_installer_pack");
            case im_stat_event_names::installer_terminate_connect_timeout: return std::string_view("im_installer_terminate_connect_timeout");
            case im_stat_event_names::installer_terminate_procid_rcvd: return std::string_view("im_installer_terminate_procid_rcvd");
            case im_stat_event_names::installer_terminate_open_proc_failed: return std::string_view("installer_terminate_open_proc_failed");
            case im_stat_event_names::installer_terminate_shutdown_sent: return std::string_view("im_installer_terminate_shutdown_sent");
            case im_stat_event_names::installer_terminate_abnormal_open_proc_failed: return std::string_view("im_installer_terminate_abnormal_open_proc_failed");
            case im_stat_event_names::installer_terminate_abnormal_wait_timeout: return std::string_view("im_installer_terminate_abnormal_wait_timeout");
            case im_stat_event_names::installer_generate_guid: return std::string_view("im_installer_generate_guid");
            case im_stat_event_names::installer_move_file: return std::string_view("im_installer_move_file");

            default:
                return std::string_view();
            }
        }
    }
}