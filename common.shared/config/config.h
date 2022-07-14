#pragma once

#include <variant>
#include <array>
#include <string_view>
#include <optional>
#include <tuple>
#include <memory>
#include <vector>
#include <mutex>
#include <algorithm>

namespace common::tools
{
    class spin_lock;
}

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
        login_by_oauth2_allowed,
        forgot_password,
        explained_forgot_password,
        unknown_contacts,
        stranger_contacts,
        otp_login,
        hiding_message_text_enabled,
        hiding_message_info_enabled,
        changeable_name,
        allow_contacts_rename,
        avatar_change_allowed,
        beta_update,
        ssl_verify,
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
        call_link_v2_enabled,
        vcs_call_by_link_enabled,
        vcs_webinar_enabled,
        statuses_enabled,
        global_contact_search_allowed,
        show_reactions,
        open_icqcom_link,
        statistics,
        statistics_mytracker,
        force_update_check_allowed,
        call_room_info_enabled,
        store_version,
        otp_login_open_mail_link,
        dns_workaround,
        external_emoji,
        show_your_invites_to_group_enabled,
        group_invite_blacklist_enabled,
        contact_us_via_backend,
        update_from_backend,
        invite_by_sms,
        show_sms_notify_setting,
        silent_message_delete,
        remove_deleted_from_notifications,
        long_path_tooltip_enabled,
        installer_crash_send,
        custom_statuses_enabled,
        formatting_in_bubbles,
        formatting_in_input,
        apps_bar_enabled,
        status_in_apps_bar,
        scheduled_messages_enabled,
        threads_enabled,
        reminders_enabled,
        support_shared_federation_stickerpacks,
        url_ftp_protocols_allowed,
        draft_enabled,
        message_corner_menu,
        task_creation_in_chat_enabled,
        smartreply_suggests_feature_enabled,
        smartreply_suggests_text,
        smartreply_suggests_stickers,
        smartreply_suggests_for_quotes,
        compact_mode_by_default,
        expanded_gallery,
        restricted_files_enabled,
        antivirus_check_enabled,
        antivirus_check_progress_visible,
        external_user_agreement,
        user_agreement_enabled,
        delete_account_enabled,
        delete_account_via_admin,
        has_registry_about,
        // TODO: remove when deprecated
        organization_structure_enabled,
        tasks_enabled,
        calendar_enabled,
        tarm_mail,
        tarm_cloud,
        tarm_calls,
        calendar_self_auth,
        mail_enabled,
        mail_self_auth,
        cloud_self_auth,
        digital_assistant_search_positioning,
        leading_last_name,
        report_messages_enabled,

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
        oauth_url,
        token_url,
        redirect_uri,
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
        dns_cache,
        external_emoji,
        privacy_policy_url,
        terms_of_use_url,
        delete_account_url,
        delete_account_url_email,

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
        client_b64,
        client_id,
        client_rapi,
        oauth_type,
        oauth_scope,
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
        company_name,
        app_user_model_win,
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
        maximum_history_file_size,
        external_config_preset_url,
        client_api_version,
        server_api_version,
        server_mention_timeout,
        support_uin,
        mytracker_app_id_win,
        mytracker_app_id_mac,
        mytracker_app_id_linux,
        status_banner_emoji_csv,
        draft_timeout_sec,
        draft_max_len,
        smartreply_suggests_click_hide_timeout,
        smartreply_suggests_msgid_cache_size,
        base_retry_interval_sec,
        product_path_old,
        product_name_old,
        crossprocess_pipe_old,
        main_instance_mutex_win_old,
        bots_commands_disabled,
        service_apps_order,
        service_apps_config,
        service_apps_desktop,
        custom_miniapps,
        wim_parallel_packets_count,
        digital_assistant_bot_aimid,
        additional_theme,
        max_delete_files,
        delete_files_older_sec,
        cleanup_period_sec,

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
    using features_array = std::array<std::pair<features, bool>, static_cast<size_t>(features::max_size)>;
    using values_array = std::array<std::pair<values, value_type>, static_cast<size_t>(values::max_size)>;
    using translations_array = std::array<std::pair<translations, std::string>, static_cast<size_t>(translations::max_size)>;

    using features_vector = std::vector<std::pair<features, bool>>;
    using values_vector = std::vector<std::pair<values, value_type>>;

    struct external_configuration
    {
        features_vector features;
        values_vector values;
    };

    void set_external(std::shared_ptr<external_configuration> _f, bool _is_develop);

    // all strings are null terminated
    class configuration
    {
    public:
        explicit configuration(std::string_view json, bool _is_debug);
        configuration(configuration&&) = default;
        ~configuration();

        void set_develop_command_line_flag(bool _flag);

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
        bool is_overridden(values) const noexcept;

        bool is_debug() const noexcept;
        bool is_develop() const noexcept;
        bool has_develop_cli_flag() const noexcept;

        void set_profile_link(const std::wstring& _profile);
        std::wstring get_profile_link(bool _secure = true) const;

    private:
        bool is_external_config_enabled() const noexcept;

        std::shared_ptr<external_configuration> get_external() const;
        void set_external(std::shared_ptr<external_configuration>, bool);

        friend void config::set_external(std::shared_ptr<external_configuration> _f, bool _is_develop);

    private:
        struct default_c
        {
            urls_array urls;
            features_array features;
            values_array values;
            translations_array translations;
        } c_;

        std::unique_ptr<common::tools::spin_lock> spin_lock_;
        std::shared_ptr<external_configuration> e_;

        const bool is_debug_ = false;
        bool is_develop_ = false;
        bool is_valid_ = false;
        bool has_develop_command_line_flag_ = false;
        std::wstring current_profile_link_ = {};
    };
}

namespace config
{
    const configuration& get();
    configuration& get_mutable();
    bool try_replace_with_develop_config();
    bool is_overridden(features _v);
    bool is_overridden(values _v);
    void reset_external();
    void reset_config();
}
