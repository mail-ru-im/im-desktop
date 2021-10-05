#pragma once

#include <rapidjson/document.h>
#include <rapidjson/schema.h>

#include <string>
#include <string_view>

namespace common { namespace json
{
    template <typename T>
    inline std::string rapidjson_get_string(const T& value) { return std::string(value.GetString(), value.GetStringLength()); }

    template <>
    inline std::string rapidjson_get_string<rapidjson::StringBuffer>(const rapidjson::StringBuffer& value) { return std::string(value.GetString(), value.GetSize()); }

    template <typename T>
    inline std::string_view rapidjson_get_string_view(const T& value) { return std::string_view(value.GetString(), value.GetStringLength()); }

    template <>
    inline std::string_view rapidjson_get_string_view<rapidjson::StringBuffer>(const rapidjson::StringBuffer& value) { return std::string_view(value.GetString(), value.GetSize()); }
}} 

namespace common { namespace json
{
    inline auto make_string_ref(std::string_view str)
    {
        return rapidjson::StringRef(str.data(), str.size());
    };

    inline namespace v2
    {
        template <class T>
        auto get_value(const rapidjson::Value& _node, std::string_view _name)->std::optional<T> = delete;
    }

    template <>
    inline std::optional<std::string> get_value<std::string>(const rapidjson::Value& _node, std::string_view _name)
    {
        if (_node.IsObject())
        {
            if (const auto iter = _node.FindMember(make_string_ref(_name)); iter != _node.MemberEnd() && iter->value.IsString())
                return rapidjson_get_string(iter->value);
        }

        return {};
    }

    template <>
    inline std::optional<std::string_view> get_value<std::string_view>(const rapidjson::Value& _node, std::string_view _name)
    {
        if (_node.IsObject())
        {
            if (const auto iter = _node.FindMember(make_string_ref(_name)); iter != _node.MemberEnd() && iter->value.IsString())
                return rapidjson_get_string_view(iter->value);
        }

        return {};
    }

    template <>
    inline std::optional<int> get_value<int>(const rapidjson::Value& _node, std::string_view _name)
    {
        if (_node.IsObject())
        {
            if (const auto iter = _node.FindMember(make_string_ref(_name)); iter != _node.MemberEnd() && iter->value.IsInt())
                return iter->value.GetInt();
        }

        return {};
    }

    template <>
    inline std::optional<unsigned int> get_value<unsigned int>(const rapidjson::Value& _node, std::string_view _name)
    {
        if (_node.IsObject())
        {
            if (const auto iter = _node.FindMember(make_string_ref(_name)); iter != _node.MemberEnd() && iter->value.IsUint())
                return iter->value.GetUint();
        }

        return {};
    }

    template <>
    inline std::optional<int64_t> get_value<int64_t>(const rapidjson::Value& _node, std::string_view _name)
    {
        if (_node.IsObject())
        {
            if (const auto iter = _node.FindMember(make_string_ref(_name)); iter != _node.MemberEnd() && iter->value.IsInt64())
                return iter->value.GetInt64();
        }

        return {};
    }

    template <>
    inline std::optional<uint64_t> get_value<uint64_t>(const rapidjson::Value& _node, std::string_view _name)
    {
        if (_node.IsObject())
        {
            if (const auto iter = _node.FindMember(make_string_ref(_name)); iter != _node.MemberEnd() && iter->value.IsUint64())
                return iter->value.GetUint64();
        }

        return {};
    }

    template <>
    inline std::optional<double> get_value<double>(const rapidjson::Value& _node, std::string_view _name)
    {
        if (_node.IsObject())
        {
            if (const auto iter = _node.FindMember(make_string_ref(_name)); iter != _node.MemberEnd() && iter->value.IsDouble())
                return iter->value.GetDouble();
        }

        return {};
    }

    template <>
    inline std::optional<bool> get_value<bool>(const rapidjson::Value& _node, std::string_view _name)
    {
        if (_node.IsObject())
        {
            if (const auto iter = _node.FindMember(make_string_ref(_name)); iter != _node.MemberEnd())
            {
                if (iter->value.IsBool())
                    return iter->value.GetBool();
                else if (iter->value.IsInt() || iter->value.IsUint())
                    return iter->value != 0;
            }
        }

        return {};
    }
}}
