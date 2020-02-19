#pragma once

#include "../common.shared/json_unserialize_helpers.h"

#ifndef _RAPIDJSON_GET_STRING_
#define _RAPIDJSON_GET_STRING_
namespace core
{
    template <typename T>
    inline std::string rapidjson_get_string(const T& value) { return std::string(value.GetString(), value.GetStringLength()); }

    template <>
    inline std::string rapidjson_get_string<rapidjson::StringBuffer>(const rapidjson::StringBuffer& value) { return std::string(value.GetString(), value.GetSize()); }

    template <typename T>
    inline std::string_view rapidjson_get_string_view(const T& value) { return std::string_view(value.GetString(), value.GetStringLength()); }

    template <>
    inline std::string_view rapidjson_get_string_view<rapidjson::StringBuffer>(const rapidjson::StringBuffer& value) { return std::string_view(value.GetString(), value.GetSize()); }
}
#endif // _RAPIDJSON_GET_STRING_

namespace core
{
    namespace tools
    {
        inline auto make_string_ref(std::string_view str)
        {
            return rapidjson::StringRef(str.data(), str.size());
        };

        template <class T, class U>
        bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out T& _out_param) = delete;

        template <class U>
        inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out std::string& _out_param)
        {
            return common::json::unserialize_value(_node, _name, _out_param);
        }

        template <class U>
        inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out std::string_view& _out_param)
        {
            return common::json::unserialize_value(_node, _name, _out_param);
        }

        template <class U>
        inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out int& _out_param)
        {
            return common::json::unserialize_value(_node, _name, _out_param);
        }

        template <class U>
        inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out int64_t& _out_param)
        {
            return common::json::unserialize_value(_node, _name, _out_param);
        }

        template <class U>
        inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out uint64_t& _out_param)
        {
            return common::json::unserialize_value(_node, _name, _out_param);
        }

        template <class U>
        inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out unsigned int& _out_param)
        {
            return common::json::unserialize_value(_node, _name, _out_param);
        }

        template <class U>
        inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out double& _out_param)
        {
            return common::json::unserialize_value(_node, _name, _out_param);
        }

        template <class U>
        inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out bool& _out_param)
        {
            return common::json::unserialize_value(_node, _name, _out_param);
        }

        void sort_json_keys_by_name(rapidjson::Value* _node);
    }
}
