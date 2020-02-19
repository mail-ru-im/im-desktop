#pragma once

#include <variant>
#include <array>
#include <string_view>
#include <optional>

namespace config
{
    enum class features
    {
        feedback_selected = 0,
        new_avatar_rapi = 1,
        ptt_recognition = 2,
        snippet_in_chat = 3,
        add_contact = 4,
        remove_contact = 5,
        force_avatar_update = 6,
        call_quality_stat = 7,
        show_data_visibility_link = 8,
        show_security_call_link = 9,
        contact_us_via_mail_default = 10,
        searchable_recents_placeholder = 11,
        auto_accepted_gdpr = 12,
        phone_allowed = 13,
        show_attach_phone_number_popup = 14,
        attach_phone_number_popup_modal = 15,
        login_by_phone_allowed = 16,
        login_by_mail_default = 17,
        login_by_uin_allowed = 18,
        forgot_password = 19,
        explained_forgot_password = 20,
        unknown_contacts = 21,
        stranger_contacts = 22,
        otp_login = 23,
        show_notification_text = 24,
        changeable_name = 25,
        avatar_change_allowed = 26,
        beta_update = 27,
        ssl_verify_peer = 28,
        profile_agent_as_domain_url = 29,
        need_gdpr_window = 30,
        need_introduce_window = 31,
        has_nicknames = 32,
        zstd_request_enabled = 33,
        zstd_response_enabled = 34,
        external_phone_attachment = 35,
        external_url_config = 36,
        omicron_enabled = 37,
        sticker_suggests = 38,
        sticker_suggests_server = 39,
        smartreplies = 40,
        force_show_chat_popup_default = 41,

        max_size
    };

    enum class urls
    {
        base = 0,
        base_binary = 1,
        files_parser_url = 2,
        profile = 3,
        profile_agent = 4,
        auth_mail_ru = 5,
        r_mail_ru = 6,
        win_mail_ru = 7,
        read_msg = 8,
        stickerpack_share = 9,
        cicq_com = 10,
        update_win_alpha = 11,
        update_win_beta = 12,
        update_win_release = 13,
        update_mac_alpha = 14,
        update_mac_beta = 15,
        update_mac_release = 16,
        update_linux_alpha_32 = 17,
        update_linux_alpha_64 = 18,
        update_linux_beta_32 = 19,
        update_linux_beta_64 = 20,
        update_linux_release_32 = 21,
        update_linux_release_64 = 22,
        omicron = 23,
        stat_base = 24,
        feedback = 25,
        legal_terms = 26,
        legal_privacy_policy = 27,
        installer_help = 28,
        data_visibility = 29,
        password_recovery = 30,
        zstd_request_dict = 31,
        zstd_response_dict = 32,
        attach_phone = 33,

        // add type before this place

        max_size
    };

    enum class values
    {
        product_path = 0,
        product_path_mac = 1,

        app_name_win = 2,
        app_name_mac = 3,
        app_name_linux = 4,
        user_agent_app_name = 5,
        client_rapi = 6,
        product_name = 7,
        product_name_short = 8,
        product_name_full = 9,
        main_instance_mutex_linux = 10,
        main_instance_mutex_win = 11,
        crossprocess_pipe = 12,
        register_url_scheme_csv = 13,
        installer_shortcut_win = 14,
        installer_menu_folder_win = 15,
        installer_product_exe_win = 16,
        installer_exe_win = 17,
        installer_hkey_class_win = 18,
        installer_main_instance_mutex_win = 19,
        updater_main_instance_mutex_win = 20,
        company_name = 21,
        app_user_model_win = 22,
        installer_hockey_app_id_win = 23,
        hockey_app_id_win = 24,
        flurry_key = 25,
        feedback_version_id = 26,
        feedback_platform_id = 27,
        feedback_aimid_id = 28,
        feedback_selected_id = 29,
        omicron_dev_id_win = 30,
        omicron_dev_id_mac = 31,
        omicron_dev_id_linux = 32,
        dev_id_win = 33,
        dev_id_mac = 34,
        dev_id_linux = 35,
        additional_fs_parser_urls_csv = 36,
        uins_for_send_log_csv = 37,
        zstd_compression_level = 38,

        max_size
    };

    enum class translations
    {
        installer_title_win = 0,
        installer_failed_win = 1,
        installer_welcome_win = 2,
        app_title = 3,
        app_install_mobile = 4,
        gdpr_welcome = 5,

        max_size
    };

    using value_type = std::variant<int64_t, double, std::string>;

    using urls_array = std::array<std::pair<urls, std::string>, static_cast<size_t>(urls::max_size)>;
    using features_array = std::array<std::pair<features, bool>, static_cast<size_t>(features::max_size)>;
    using values_array = std::array<std::pair<values, value_type>, static_cast<size_t>(values::max_size)>;
    using translations_array = std::array<std::pair<translations, std::string>, static_cast<size_t>(translations::max_size)>;

    // all strings are null terminated
    class configuration
    {
    public:
        explicit configuration(std::string_view json, bool external);

        bool is_valid() const noexcept;

        const value_type& value(values) const noexcept;

        std::string_view string(values) const noexcept;

        template <typename T,
                  typename = std::enable_if_t<std::is_same_v<T, int64_t> || std::is_same_v<T, double>>>
        inline std::optional<T> number(values _v) const noexcept
        {
            if (const auto& v = value(_v); auto ptr = std::get_if<T>(&v))
                return static_cast<T>(*ptr);
            return std::nullopt;
        }

        std::string_view url(urls) const noexcept;

        std::string_view translation(translations) const noexcept;

        bool is_on(features) const noexcept;

        bool is_external() const noexcept;

    private:
        bool is_external_ = false;
        bool is_valid_ = false;

        urls_array urls_;
        features_array features_;
        values_array values_;
        translations_array translations_;
    };
}

namespace config
{
    const configuration& get();
}