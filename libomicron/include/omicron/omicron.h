/*! \file omicron.h
    \brief Omicron interface (header).

    User interface for working with "omicron" service.
*/
#pragma once

#include "omicron_conf.h"

namespace omicronlib
{
    //! Error codes.
    enum omicron_code
    {
        OMICRON_OK = 0,
        OMICRON_NOT_INITIALIZED,
        OMICRON_INIT_FAILED,
        OMICRON_DOWNLOAD_FAILED,
        OMICRON_DOWNLOAD_NOT_IMPLEMENTED,
        OMICRON_WRONG_JSON_DATA,
        OMICRON_IS_UPDATING,
        OMICRON_ALREADY_UPDATING,
    };

    //! A string with json data.
    /*!
        This type is used for store json data (object or array) as a string.
    */
    class json_string : public std::string
    {
    public:
        using std::string::string;
    };

    //! Init omicron session.
    /*!
        All necessary parameters for initialization are specified in the \a _config.
        If \a _file_name is specified, the omicron data will be saved and than be loaded in the following work sessions.
        \param[in] _config An omicron configuration.
        \param[in] _file_name A file name for storing information.
        \return The result of initialization of work.
    */
    omicron_code omicron_init(const omicron_config& _config, const std::wstring& _file_name = {});

    //! Start data update operation.
    /*!
        \return The result of operation start.
    */
    omicron_code omicron_update();

    //! Start auto-update service.
    /*!
        \return Success or failure of starting.
    */
    bool omicron_start_auto_updater();

    //! Close omicron session.
    void omicron_cleanup();

    //! Get the last update time.
    /*!
        \return The time of the last update data from the omicron service.
    */
    time_t omicron_get_last_update_time();

    //! Get the last update status.
    /*!
        \return The result of the last update data from the omicron service.
    */
    omicron_code omicron_get_last_update_status();

    //! Add a fingerprint to config.
    /*!
        The fingerprint is used to personalize the client.
        \param[in] _name Fingerprint name.
        \param[in] _value Fingerprint value.
    */
    void omicron_add_fingerprint(std::string _name, std::string _value);

    //!@name Get a value from the omicron service.
    /*!
        Functions from this group try to get the value of the required parameter \a _key_name with type as in \a _default_value.
        If the \a _key_name with the required type is not found the user will get the default value \a _default_value.
        \param[in] _key_name A parameter name.
        \param[in] _default_value A default value.
        \return A value with the same type as in \a _default_value.
    */
    //@{
    bool _o(const char* _key_name, bool _default_value);
    int _o(const char* _key_name, int _default_value);
    unsigned int _o(const char* _key_name, unsigned int _default_value);
    int64_t _o(const char* _key_name, int64_t _default_value);
    uint64_t _o(const char* _key_name, uint64_t _default_value);
    double _o(const char* _key_name, double _default_value);
    std::string _o(const char* _key_name, const char* _default_value);
    std::string _o(const char* _key_name, std::string _default_value);
    json_string _o(const char* _key_name, json_string _default_value);
    //@}
}
