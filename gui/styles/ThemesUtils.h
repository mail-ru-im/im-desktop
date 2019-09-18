#pragma once

namespace StylingUtils
{
    QString getString(const rapidjson::Value& _node);
    QColor getColor(const rapidjson::Value& _value);

    template <class T, class U>
    bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out T& _out_param) = delete;

    template <class U>
    inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out int& _out_param)
    {
        if (const auto iter = _node.FindMember(_name); iter != _node.MemberEnd() && iter->value.IsInt())
        {
            _out_param = iter->value.GetInt();
            return true;
        }

        return false;
    }

    template <class U>
    inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out bool& _out_param)
    {
        if (const auto iter = _node.FindMember(_name); iter != _node.MemberEnd())
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

    template <class U>
    inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out QString& _out_param)
    {
        if (const auto iter = _node.FindMember(_name); iter != _node.MemberEnd() && iter->value.IsString())
        {
            _out_param = getString(iter->value);
            return true;
        }

        return false;
    }

    template <class U>
    inline bool unserialize_value(const rapidjson::Value& _node, const U& _name, Out QColor& _out_param)
    {
        if (const auto iter = _node.FindMember(_name); iter != _node.MemberEnd() && iter->value.IsString())
        {
            _out_param = getColor(iter->value);
            return true;
        }

        return false;
    }
}