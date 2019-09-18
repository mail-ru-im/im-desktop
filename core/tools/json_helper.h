#pragma once

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
            if (const auto iter = _node.FindMember(make_string_ref(_name)); iter != _node.MemberEnd() && iter->value.IsString())
            {
                _out_param = rapidjson_get_string(iter->value);
                return true;
            }

            return false;
        }

        template <class U>
        inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out std::string_view& _out_param)
        {
            if (const auto iter = _node.FindMember(make_string_ref(_name)); iter != _node.MemberEnd() && iter->value.IsString())
            {
                _out_param = rapidjson_get_string_view(iter->value);
                return true;
            }

            return false;
        }

        template <class U>
        inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out int& _out_param)
        {
            if (const auto iter = _node.FindMember(make_string_ref(_name)); iter != _node.MemberEnd() && iter->value.IsInt())
            {
                _out_param = iter->value.GetInt();
                return true;
            }

            return false;
        }

        template <class U>
        inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out int64_t& _out_param)
        {
            if (const auto iter = _node.FindMember(make_string_ref(_name)); iter != _node.MemberEnd() && iter->value.IsInt64())
            {
                _out_param = iter->value.GetInt64();
                return true;
            }

            return false;
        }

        template <class U>
        inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out uint64_t& _out_param)
        {
            if (const auto iter = _node.FindMember(make_string_ref(_name)); iter != _node.MemberEnd() && iter->value.IsUint64())
            {
                _out_param = iter->value.GetUint64();
                return true;
            }

            return false;
        }

        template <class U>
        inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out unsigned int& _out_param)
        {
            if (const auto iter = _node.FindMember(make_string_ref(_name)); iter != _node.MemberEnd() && iter->value.IsUint())
            {
                _out_param = iter->value.GetUint();
                return true;
            }

            return false;
        }

        template <class U>
        inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out bool& _out_param)
        {
            if (const auto iter = _node.FindMember(make_string_ref(_name)); iter != _node.MemberEnd())
            {
                if (iter->value.IsBool())
                {
                    _out_param = iter->value.GetBool();
                    return true;
                }
                else if (iter->value.IsInt() || iter->value.IsUint())
                {
                    _out_param = iter->value != 0;
                    return true;
                }
            }

            return false;
        }
    }
}
