#pragma once

#include "../namespaces.h"
#include <boost/any.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <unordered_map>

CORE_CONFIGURATION_NS_BEGIN

class app_config
{
public:
    enum class AppConfigOption
    {
        disable_server_history = 0,
        forced_dpi = 1,
        is_crash_enabled = 2,
        is_testing_enabled = 3,
        full_log = 4,
        unlock_context_menu_features = 5,
        show_msg_ids = 6,
        save_rtp_dumps = 7,
        gdpr_user_has_agreed = 8,
        gdpr_agreement_reported_to_server = 9,
        gdpr_user_has_logged_in_ever = 10,
        cache_history_pages_secs = 11,
        is_server_search_enabled = 12,
        is_updateble = 13,
        dev_id = 14,
        update_interval = 15,
        curl_log = 16,
        task_trace = 17,
        hide_keyword_pattern = 18,
        show_hidden_themes = 19,
        sys_crash_handler_enabled = 20,
    };

    enum class gdpr_report_to_server_state
    {
        reported_successfully = 0,
        sent = 1,
        failed = 2,
        not_sent = 3
    };

    using AppConfigMap = std::unordered_map<AppConfigOption, boost::any>;

    app_config();
    app_config(AppConfigMap _options);

    boost::property_tree::ptree as_ptree() const;

    bool is_crash_enabled() const;
    bool is_testing_enabled() const;
    bool is_full_log_enabled() const;
    bool is_curl_log_enabled() const;
    bool is_show_msg_ids_enabled() const;
    bool is_server_history_enabled() const;
    bool is_server_search_enabled() const;
    bool unlock_context_menu_features() const;
    bool is_task_trace_enabled() const;
    bool is_hide_keyword_pattern() const;
    bool is_show_hidden_themes() const;
    bool is_sys_crash_handler_enabled() const;

    bool gdpr_user_has_agreed() const;
    int32_t gdpr_agreement_reported_to_server() const;
    bool gdpr_user_has_logged_in_ever() const;
    bool is_updateble() const;

    std::string device_id() const;
    uint32_t update_interval() const;

    int32_t forced_dpi() const;
    bool is_save_rtp_dumps_enabled() const;
    int cache_history_pages_secs() const;

    std::string_view get_stat_base_url() const;
    std::string_view get_url_omicron_data() const;
    std::string_view get_url_attach_phone() const;

    std::string get_update_url(std::string_view _updateble_build_version) const;

    void serialize(Out core::coll_helper &_collection) const;

    template <typename ValueType>
    void set_config_option(AppConfigOption option, ValueType&& value);

private:

    std::string_view get_update_win_alpha_url() const;
    std::string_view get_update_win_beta_url() const;
    std::string_view get_update_win_release_url() const;
    std::string_view get_update_mac_alpha_url() const;
    std::string_view get_update_mac_beta_url() const;
    std::string_view get_update_mac_release_url() const;
    std::string_view get_update_linux_alpha_32_url() const;
    std::string_view get_update_linux_alpha_64_url() const;
    std::string_view get_update_linux_beta_32_url() const;
    std::string_view get_update_linux_beta_64_url() const;
    std::string_view get_update_linux_release_32_url() const;
    std::string_view get_update_linux_release_64_url() const;

private:
    AppConfigMap app_config_options_;
};

const app_config& get_app_config();

void load_app_config(const boost::filesystem::wpath &_path);
void dump_app_config_to_disk(const boost::filesystem::wpath &_path);

template <typename ValueType>
void set_config_option(app_config::AppConfigOption option, ValueType&& value);


CORE_CONFIGURATION_NS_END
