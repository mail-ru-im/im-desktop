/*! \file omicron_conf.cpp
    \brief Omicron configuration (source).
*/
#include "stdafx.h"

#include <omicron/omicron_conf.h>

namespace omicronlib
{
    std::string environment_to_string(const environment_type _env)
    {
        switch (_env)
        {
        case environment_type::alpha:
            return "Alpha";
        case environment_type::beta:
            return "Beta";
        case environment_type::release:
            return "Release";
        default:
            return "Unknown";
        }
    }

    std::string omicron_config::escape_symbols(const std::string& _data)
    {
        std::stringstream ss_out;
        std::array<char, 4> buffer;

        for (auto sym : _data)
        {
            if (std::isalpha(static_cast<unsigned char>(sym)) || std::isdigit(static_cast<unsigned char>(sym)) || std::strchr("-._~", sym))
            {
                ss_out << sym;
            }
            else
            {
                std::snprintf(buffer.data(), buffer.size(), "%%%.2X", static_cast<unsigned char>(sym));
                buffer.back() = '\0';
                ss_out << buffer.data();
            }
        }

        return ss_out.str();
    }

    omicron_config::omicron_config(const std::string& _api_url, const std::string& _app_id, uint32_t _update_interval, environment_type _environment)
        : api_url_(_api_url)
        , app_id_(_app_id)
        , update_interval_(_update_interval)
        , environment_(_environment)
        , custom_json_downloader_(nullptr)
        , custom_logger_(nullptr)
        , callback_updater_(nullptr)
    {
    }

    void omicron_config::add_fingerprint(std::string _name, std::string _value)
    {
        if (_name.empty() || _value.empty())
            return;

        fingerprints_[std::move(_name)] = std::move(_value);
    }

    void omicron_config::reset_fingerprints()
    {
        fingerprints_.clear();
    }

    void omicron_config::set_json_downloader(download_json_conf_func _custom_json_downloader)
    {
        custom_json_downloader_ = _custom_json_downloader;
    }

    download_json_conf_func omicron_config::get_json_downloader() const
    {
        return custom_json_downloader_;
    }

    void omicron_config::set_logger(write_to_log_func _custom_logger)
    {
        custom_logger_ = _custom_logger;
    }

    write_to_log_func omicron_config::get_logger() const
    {
        return custom_logger_;
    }

    void omicron_config::set_callback_updater(callback_update_data_func _callback)
    {
        callback_updater_ = _callback;
    }

    callback_update_data_func omicron_config::get_callback_updater() const
    {
        return callback_updater_;
    }

    void omicron_config::set_external_proxy_settings(external_proxy_settings_func _external_func)
    {
        external_proxy_settings_ = _external_func;
    }

    std::string omicron_config::generate_request_string() const
    {
        std::stringstream request;

        request << api_url_ << '?';
        request << "mytracker_id=" << escape_symbols(app_id_);
        request << "&app_env=" << escape_symbols(environment_to_string(environment_));

        // add config version and condition
        if (!config_v_.empty())
            request << "&config_v=" << config_v_;
        if (!cond_s_.empty())
            request << "&cond_s=" << cond_s_;

        // add users' fingerprints
        for (const auto& kv : fingerprints_)
            request << '&' << escape_symbols(kv.first) << '=' << escape_symbols(kv.second);

        // add segments
        bool first = true;
        for (const auto& kv : segments_)
        {
            if (first)
            {
                request << "&segments=";
                first = false;
            }
            else
            {
                request << ',';
            }

            request << escape_symbols(kv.second);
        }

        return request.str();
    }

    bool omicron_config::is_empty_url() const
    {
        return api_url_.empty();
    }

    uint32_t omicron_config::get_update_interval() const
    {
        return update_interval_;
    }

    void omicron_config::set_config_v(int _config_v)
    {
        config_v_ = std::to_string(_config_v);
    }

    void omicron_config::reset_config_v()
    {
        config_v_.clear();
    }

    void omicron_config::set_cond_s(std::string _cond_s)
    {
        cond_s_ = std::move(_cond_s);
    }

    void omicron_config::reset_cond_s()
    {
        cond_s_.clear();
    }

    void omicron_config::add_segment(std::string _name, std::string _value)
    {
        if (_name.empty() || _value.empty())
            return;

        segments_[std::move(_name)] = std::move(_value);
    }

    void omicron_config::reset_segments()
    {
        segments_.clear();
    }

    void omicron_config::set_proxy_setting(omicron_proxy_settings _settings)
    {
        proxy_settings_ = std::move(_settings);
    }

    omicron_proxy_settings omicron_config::get_proxy_settings() const
    {
        return external_proxy_settings_ ? external_proxy_settings_() : proxy_settings_;
    }
}
