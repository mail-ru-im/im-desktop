#pragma once

#include "../namespaces.h"
#include <boost/any.hpp>
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
        // urls
        url_base = 100,
        url_files = 101,
        url_files_get = 102,
        url_profile_agent = 103,
        url_auth_mail_ru = 104,
        url_r_mail_ru = 105,
        url_win_mail_ru = 106,
        url_read_msg = 107,
        url_cicq_org = 108,
        url_cicq_com = 109
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

    bool gdpr_user_has_agreed() const;
    int32_t gdpr_agreement_reported_to_server() const;
    bool gdpr_user_has_logged_in_ever() const;
    bool is_updateble() const;

    std::string device_id() const;
    uint32_t update_interval() const;

    int32_t forced_dpi() const;
    bool is_save_rtp_dumps_enabled() const;
    int cache_history_pages_secs() const;

    std::string get_url_base() const;
    std::string get_url_files() const;
    std::string get_url_files_get() const;
    std::string get_url_profile_agent() const;
    std::string get_url_auth_mail_ru() const;
    std::string get_url_r_mail_ru() const;
    std::string get_url_win_mail_ru() const;
    std::string get_url_read_msg() const;
    std::string get_url_cicq_org() const;
    std::string get_url_cicq_com() const;

    std::string get_url_files_if_not_default() const;

    void serialize(Out core::coll_helper &_collection) const;

    template <typename ValueType>
    void set_config_option(AppConfigOption option, ValueType&& value);

private:
    AppConfigMap app_config_options_;
};

const app_config& get_app_config();

void load_app_config(const boost::filesystem::wpath &_path);
void dump_app_config_to_disk(const boost::filesystem::wpath &_path);

template <typename ValueType>
void set_config_option(app_config::AppConfigOption option, ValueType&& value);


CORE_CONFIGURATION_NS_END
