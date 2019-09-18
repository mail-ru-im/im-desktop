#pragma once

namespace core
{
    class coll_helper;
}

namespace Omicron
{
    void unserialize(core::coll_helper _collection);

    void cleanup();

    bool _o(std::string_view _keyName, bool _defaultValue);
    int _o(std::string_view _keyName, int _defaultValue);
    unsigned int _o(std::string_view _keyName, unsigned int _defaultValue);
    int64_t _o(std::string_view _keyName, int64_t _defaultValue);
    uint64_t _o(std::string_view _keyName, uint64_t _defaultValue);
    double _o(std::string_view _keyName, double _defaultValue);
    std::string _o(std::string_view _keyName, const std::string& _defaultValue);
    QString _o(std::string_view _keyName, const QString& _defaultValue);

    std::string _o_json(std::string_view _keyName, const std::string&  _defaultValue);
    QString _o_json(std::string_view _keyName, const QString& _defaultValue);
}
