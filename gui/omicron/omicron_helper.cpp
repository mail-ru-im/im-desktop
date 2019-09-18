/*! \file omicron.cpp
    \brief Omicron interface (source).
*/
#include "stdafx.h"

#include "omicron_helper.h"
#include "OmicronContainer.h"

#include "../../corelib/collection_helper.h"

namespace Omicron
{
    void unserialize(core::coll_helper _collection)
    {
        OmicronContainer::instance().unserialize(_collection);
    }

    void cleanup()
    {
        OmicronContainer::instance().cleanup();
    }

    bool _o(std::string_view _keyName, bool _defaultValue)
    {
        return OmicronContainer::instance().boolValue(_keyName, _defaultValue);
    }

    int _o(std::string_view _keyName, int _defaultValue)
    {
        return OmicronContainer::instance().intValue(_keyName, _defaultValue);
    }

    unsigned int _o(std::string_view _keyName, unsigned int _defaultValue)
    {
        return OmicronContainer::instance().uintValue(_keyName, _defaultValue);
    }

    int64_t _o(std::string_view _keyName, int64_t _defaultValue)
    {
        return OmicronContainer::instance().int64Value(_keyName, _defaultValue);
    }

    uint64_t _o(std::string_view _keyName, uint64_t _defaultValue)
    {
        return OmicronContainer::instance().uint64Value(_keyName, _defaultValue);
    }

    double _o(std::string_view _keyName, double _defaultValue)
    {
        return OmicronContainer::instance().doubleValue(_keyName, _defaultValue);
    }

    std::string _o(std::string_view _keyName, const std::string& _defaultValue)
    {
        return OmicronContainer::instance().stringValue(_keyName, _defaultValue);
    }

    QString _o(std::string_view _keyName, const QString& _defaultValue)
    {
        return OmicronContainer::instance().stringValue(_keyName, _defaultValue);
    }

    std::string _o_json(std::string_view _keyName, const std::string& _defaultValue)
    {
        return OmicronContainer::instance().jsonValue(_keyName, _defaultValue);
    }

    QString _o_json(std::string_view _keyName, const QString& _defaultValue)
    {
        return OmicronContainer::instance().jsonValue(_keyName, _defaultValue);
    }
}
