/*! \file omicron.cpp
    \brief Omicron interface (source).
*/
#include "stdafx.h"

#include <omicron/omicron_conf.h>
#include <omicron/omicron.h>

#include "omicron_impl.h"

namespace omicronlib
{
    omicron_code omicron_init(const omicron_config& _config, const std::wstring& _file_name)
    {
        return omicron_impl::instance().init(_config, _file_name);
    }

    omicron_code omicron_update()
    {
        return omicron_impl::instance().update_data();
    }

    bool omicron_start_auto_updater()
    {
        return omicron_impl::instance().star_auto_updater();
    }

    void omicron_cleanup()
    {
        omicron_impl::instance().cleanup();
    }

    time_t omicron_get_last_update_time()
    {
        return omicron_impl::instance().get_last_update_time();
    }

    omicron_code omicron_get_last_update_status()
    {
        return omicron_impl::instance().get_last_update_status();
    }

    void omicron_add_fingerprint(std::string _name, std::string _value)
    {
        return omicron_impl::instance().add_fingerprint(std::move(_name), std::move(_value));
    }

    bool _o(const char* _key_name, bool _default_value)
    {
        return omicron_impl::instance().bool_value(_key_name, _default_value);
    }

    int _o(const char* _key_name, int _default_value)
    {
        return omicron_impl::instance().int_value(_key_name, _default_value);
    }

    unsigned int _o(const char* _key_name, unsigned int _default_value)
    {
        return omicron_impl::instance().uint_value(_key_name, _default_value);
    }

    int64_t _o(const char* _key_name, int64_t _default_value)
    {
        return omicron_impl::instance().int64_value(_key_name, _default_value);
    }

    uint64_t _o(const char* _key_name, uint64_t _default_value)
    {
        return omicron_impl::instance().uint64_value(_key_name, _default_value);
    }

    double _o(const char* _key_name, double _default_value)
    {
        return omicron_impl::instance().double_value(_key_name, _default_value);
    }

    std::string _o(const char* _key_name, const char* _default_value)
    {
        return omicron_impl::instance().string_value(_key_name, _default_value);
    }

    std::string _o(const char* _key_name, std::string _default_value)
    {
        return omicron_impl::instance().string_value(_key_name, std::move(_default_value));
    }

    json_string _o(const char* _key_name, json_string _default_value)
    {
        return omicron_impl::instance().json_value(_key_name, std::move(_default_value));
    }
}
