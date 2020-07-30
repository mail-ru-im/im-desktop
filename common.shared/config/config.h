#pragma once

#include <variant>
#include <array>
#include <string_view>
#include <optional>
#include <tuple>

namespace config
{
    enum class features
    {
        feedback_selected,
        new_avatar_rapi,
        ptt_recognition,
        snippet_in_chat,
        add_contact,
        remove_contact,
        force_avatar_update,
        call_quality_stat,
        show_data_visibility_link,
        show_security_call_link,
        contact_us_via_mail_default,
        searchable_recents_placeholder,
        auto_accepted_gdpr,
        phone_allowed,
        show_attach_phone_number_popup,
        attach_phone_number_popup_modal,
        login_by_phone_allowed,
        login_by_mail_default,
        login_by_uin_allowed,
        forgot_password,
        explained_forgot_password,
        unknown_contacts,
        stranger_contacts,
        otp_login,
        show_notification_text,
        changeable_name,
        avatar_change_allowed,
        beta_update,
        ssl_verify_peer,
        profile_agent_as_domain_url,
        need_gdpr_window,
        need_introduce_window,
        has_nicknames,
        zstd_request_enabled,
        zstd_response_enabled,
        external_phone_attachment,
        external_url_config,
        omicron_enabled,
        sticker_suggests,
        sticker_suggests_server,
        smartreplies,
        spell_check,
        favorites_message_onpremise,
        info_change_allowed,
        vcs_call_by_link_enabled,
        vcs_webinar_enabled,
        statuses_enabled,
        global_contact_search_allowed,
        show_reactions,
        open_icqcom_link,
        statistics,
        force_update_check_allowed,
        call_room_info_enabled,
        external_config_use_preset_url,
        store_version,
        has_connect_by_ip_option,
        
        max_size
    };

    enum class urls
    {
        base,
        base_binary,
        files_parser_url,
        profile,
        profile_agent,
        auth_mail_ru,
        r_mail_ru,
        win_mail_ru,
        read_msg,
        stickerpack_share,
        cicq_com,
        update_win_alpha,
        update_win_beta,
        update_win_release,
        update_mac_alpha,
        update_mac_beta,
        update_mac_release,
        update_linux_alpha_32,
        update_linux_alpha_64,
        update_linux_beta_32,
        update_linux_beta_64,
        update_linux_release_32,
        update_linux_release_64,
        omicron,
        stat_base,
        feedback,
        legal_terms,
        legal_privacy_policy,
        installer_help,
        data_visibility,
        password_recovery,
        zstd_request_dict,
        zstd_response_dict,
        attach_phone,
        update_app_url,
        vcs_room,
        to_replace_hosts,

        // add type before this place

        max_size
    };

    enum class values
    {
        product_path,
        product_path_mac,

        app_name_win,
        app_name_mac,
        app_name_linux,
        user_agent_app_name,
        client_rapi,
        product_name,
        product_name_short,
        product_name_full,
        main_instance_mutex_linux,
        main_instance_mutex_win,
        crossprocess_pipe,
        register_url_scheme_csv,
        installer_shortcut_win,
        installer_menu_folder_win,
        installer_product_exe_win,
        installer_exe_win,
        installer_hkey_class_win,
        installer_main_instance_mutex_win,
        updater_main_instance_mutex_win,
        company_name,
        app_user_model_win,
        flurry_key,
        feedback_version_id,
        feedback_platform_id,
        feedback_aimid_id,
        feedback_selected_id,
        omicron_dev_id_win,
        omicron_dev_id_mac,
        omicron_dev_id_linux,
        dev_id_win,
        dev_id_mac,
        dev_id_linux,
        additional_fs_parser_urls_csv,
        uins_for_send_log_csv,
        zstd_compression_level,
        server_search_messages_min_symbols,
        server_search_contacts_min_symbols,
        voip_call_user_limit,
        voip_video_user_limit,
        voip_big_conference_boundary,
        external_config_preset_url,
        server_api_version,
        server_mention_timeout,
        support_uin,

        max_size
    };

    enum class translations
    {
        installer_title_win,
        installer_failed_win,
        installer_welcome_win,
        app_title,
        app_install_mobile,
        gdpr_welcome,

        max_size
    };

    using value_type = std::variant<int64_t, double, std::string>;

    using urls_array = std::array<std::pair<urls, std::string>, static_cast<size_t>(urls::max_size)>;
    using features_array = std::array<std::tuple<features, bool, bool>, static_cast<size_t>(features::max_size)>;
    using values_array = std::array<std::pair<values, value_type>, static_cast<size_t>(values::max_size)>;
    using translations_array = std::array<std::pair<translations, std::string>, static_cast<size_t>(translations::max_size)>;

    // all strings are null terminated
    class configuration
    {
    public:
        enum class type_index
        {
            key,
            value,
            override
        };

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

        bool is_overridden(features) const noexcept;

        bool is_external() const noexcept;

        void override_feature(features _f, bool _value);

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
    void override_feature(features _f, bool _value);
    bool is_overridden(urls _v);
    bool is_overridden(features _v);
    bool is_overridden(values _v);
}
