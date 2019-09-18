/*! \file omicron_conf.h
    \brief Omicron configuration (header).

    Parameters for setting up work with the omicron service.
*/
#pragma once

#include <string>

namespace omicronlib
{
    //!@name Omicron constants.
    //@{
    //! Default update interval from omicron (seconds).
    constexpr inline uint32_t default_update_interval() noexcept
    {
#ifdef DEBUG
        return 60;
#else
        return 21600;
#endif
    }
    //! Internal tick timeout for service thread (seconds).
    constexpr inline uint32_t internal_tick_timeout() noexcept
    {
#ifdef DEBUG
        return 10;
#else
        return 60;
#endif
    }
    //@}

    //! Get default endpoint for the omicron service.
    inline std::string get_default_api_url() noexcept
    {
        return "https://u.icq.net/omicron";
    }

    //! Settings for the proxy.
    struct omicron_proxy_settings
    {
        bool use_proxy_; //!< Proxy settings required.
        std::string	server_; //!< Name of proxy.
        int32_t port_; //!< Port of proxy.
        int32_t type_; //!< Type of proxy.
        bool need_auth_; //!< Proxy authorization required.
        //!@name User name and password to use with proxy.
        //@{
        std::string login_;
        std::string password_;
        //@}

        //! Default constructor.
        omicron_proxy_settings()
            : use_proxy_(false)
            , port_(0)
            , type_(0)
            , need_auth_(false)
        {
        }
    };

    //! A type definition for the omicron download function.
    /*!
        This function is used for getting json-data from the omicron service.
        Users can implement their own download function.
        \param[in] _proxy_settings A proxy settings.
        \param[in] _url A requested URL (GET request).
        \param[out] _json_data A response json-data.
        \param[out] _response_code A response status code (HTTP status code).
        \return Successful or not downloading.
    */
    using download_json_conf_func = bool (*)(const omicron_proxy_settings& _proxy_settings, const std::string& _url, std::string& _json_data, long& _response_code);

    //! A type definition for the logger function.
    /*!
        This function is used for write log information from the library, if user implemented it.
        \param[in] _text A text for log.
    */
    using write_to_log_func = void (*)(const std::string& _text);

    //! A type definition for the callback function, which is called after omicron updating.
    /*!
        This function is used for write log information from the library, if user implemented it.
        \param[in] _data A new omicron values.
    */
    using callback_update_data_func = void (*)(const std::string& _data);

    //! A type definition for the function, which returns proxy settings.
    /*!
        This function is used to get external settings for the proxy, if user implemented it.
        \return Proxy settings.
    */
    using external_proxy_settings_func = omicron_proxy_settings (*)();

    //! An environment type.
    enum class environment_type
    {
        alpha = 0,
        beta,
        release
    };
    //! Convert environment type into string.
    /*!
        \param _env An environment type.
        \return An environment type as a string.
    */
    std::string environment_to_string(const environment_type _env);

    //! Omicron configuration.
    class omicron_config
    {
    public:
        //! Escape symbols.
        /*!
            Replace some characters in the string \a _data with their hex-code.
            \param[in] _data An input string.
            \return A converted string.
        */
        static std::string escape_symbols(const std::string& _data);

        //!@name Initialization group.
        //@{
        omicron_config(const std::string& _api_url,
                       const std::string& _app_id,
                       uint32_t _update_interval = default_update_interval(),
                       environment_type _environment = environment_type::alpha);

        omicron_config(const omicron_config&) = default;
        omicron_config(omicron_config&&) noexcept = default;
        omicron_config& operator=(omicron_config&&) noexcept = default;
        //@}

        //! Check if the omicron endpoint is not specified.
        bool is_empty_url() const;

        //! Generate query string in the omicron service.
        std::string generate_request_string() const;

        //! Set custom downloader.
        /*!
            \param[in] _custom_json_downloader A pointer to a user-defined function.
        */
        void set_json_downloader(download_json_conf_func _custom_json_downloader);
        //! Get custom downloader.
        /*!
            \return A pointer to a user-defined function.
        */
        download_json_conf_func get_json_downloader() const;

        //! Set custom logger.
        /*!
            \param[in] _custom_logger A pointer to a user-defined function.
        */
        void set_logger(write_to_log_func _custom_logger);
        //! Get custom downloader.
        /*!
            \return A pointer to a user-defined function.
        */
        write_to_log_func get_logger() const;

        //! Set callback updater.
        /*!
            \param[in] _callback A pointer to a user-defined function.
        */
        void set_callback_updater(callback_update_data_func _callback);

        //! Get callback updater.
        /*!
            \return A pointer to a user-defined function.
        */
        callback_update_data_func get_callback_updater() const;

        //! Set function to get external proxy settings.
        /*!
            \param[in] _external_func A pointer to a user-defined function.
        */
        void set_external_proxy_settings(external_proxy_settings_func _external_func);

        //! Get update interval.
        /*!
            \return The specified value of the update interval.
        */
        uint32_t get_update_interval() const;

        //! Add a fingerprint.
        /*!
            The fingerprint is used to personalize the client.
            \param[in] _name Fingerprint name.
            \param[in] _value Fingerprint value.
        */
        void add_fingerprint(std::string _name, std::string _value);
        //! Reset all fingerprints.
        void reset_fingerprints();
        //! Set configuration version.
        /*!
            \param[in] _config_v Version number.
        */
        void set_config_v(int _config_v);
        //! Reset configuration version.
        void reset_config_v();
        //! Set conditional hash-string.
        /*!
            \param[in] _cond_s Ñonvolution of a configuration.
        */
        void set_cond_s(std::string _cond_s);
        //! Reset conditional hash-string.
        void reset_cond_s();
        //! Add a segment.
        /*!
            The segment is used for A-B testing.
            \param[in] _name Segment name.
            \param[in] _value Segment value.
        */
        void add_segment(std::string _name, std::string _value);
        //! Reset all segments.
        void reset_segments();
        //! Set up proxy settings.
        /*!
            \param[in] _settings Proxy settings.
        */
        void set_proxy_setting(omicron_proxy_settings _settings);
        //! Get proxy settings.
        /*!
            \return The current proxy settings.
        */
        omicron_proxy_settings get_proxy_settings() const;

    private:
        std::string api_url_; //!< Endpoint of the omicron service.
        std::string app_id_; //!< Unique application ID.
        omicron_proxy_settings proxy_settings_; //!< Proxy settings.
        uint32_t update_interval_; //!< Update interval for the omicron service.
        environment_type environment_; //!< Environment type.
        std::map<std::string, std::string> fingerprints_; //!< Fingerprints for the additional personalization of the client in the omicron.
        download_json_conf_func custom_json_downloader_; //!< Custom download function.
        write_to_log_func custom_logger_; //!< Custom logger function.
        callback_update_data_func callback_updater_; //!< Callback updater function.
        external_proxy_settings_func external_proxy_settings_; //!< Function to get external proxy settings.

        std::string config_v_; //!< Configuration version.
        std::string cond_s_; //!< Conditional hash-string.
        std::map<std::string, std::string> segments_; //!< Segments of A-B testing from omicron.
    };
}
