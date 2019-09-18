#include "stdafx.h"

#include "OmicronContainer.h"

#include "../../corelib/collection_helper.h"
#include "../utils/gui_coll_helper.h"

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/document.h>

namespace Omicron
{
    OmicronContainer& OmicronContainer::instance()
    {
        static OmicronContainer instance;
        return instance;
    }

    void OmicronContainer::unserialize(core::coll_helper _collection)
    {
        if (_collection.is_value_exist("data"))
        {
            auto newData = std::make_shared<rapidjson::Document>(rapidjson::kObjectType);
            if (newData->Parse(_collection.get_value_as_string("data")).HasParseError())
                return;

            init_ = true;

            std::lock_guard<SpinLock> lock(dataLock_);
            data_.swap(newData);
        }
    }

    void OmicronContainer::cleanup()
    {
        init_ = false;
        data_.reset();
    }

    bool OmicronContainer::boolValue(std::string_view _keyName, bool _defaultValue) const
    {
        if (init_)
        {
            auto data = getData();
            auto it = data->FindMember(rapidjson::StringRef(_keyName.data(), _keyName.size()));
            if (it != data->MemberEnd() && it->value.IsBool())
                return it->value.GetBool();
        }

        return _defaultValue;
    }

    int OmicronContainer::intValue(std::string_view _keyName, int _defaultValue) const
    {
        if (init_)
        {
            auto data = getData();
            auto it = data->FindMember(rapidjson::StringRef(_keyName.data(), _keyName.size()));
            if (it != data->MemberEnd() && it->value.IsInt())
                return it->value.GetInt();
        }

        return _defaultValue;
    }

    unsigned int OmicronContainer::uintValue(std::string_view _keyName, unsigned int _defaultValue) const
    {
        if (init_)
        {
            auto data = getData();
            auto it = data->FindMember(rapidjson::StringRef(_keyName.data(), _keyName.size()));
            if (it != data->MemberEnd() && it->value.IsInt())
                return it->value.GetUint();
        }

        return _defaultValue;
    }

    int64_t OmicronContainer::int64Value(std::string_view _keyName, int64_t _defaultValue) const
    {
        if (init_)
        {
            auto data = getData();
            auto it = data->FindMember(rapidjson::StringRef(_keyName.data(), _keyName.size()));
            if (it != data->MemberEnd() && it->value.IsInt64())
                return it->value.GetInt64();
        }

        return _defaultValue;
    }

    uint64_t OmicronContainer::uint64Value(std::string_view _keyName, uint64_t _defaultValue) const
    {
        if (init_)
        {
            auto data = getData();
            auto it = data->FindMember(rapidjson::StringRef(_keyName.data(), _keyName.size()));
            if (it != data->MemberEnd() && it->value.IsInt64())
                return it->value.GetUint64();
        }

        return _defaultValue;
    }

    double OmicronContainer::doubleValue(std::string_view _keyName, double _defaultValue) const
    {
        if (init_)
        {
            auto data = getData();
            auto it = data->FindMember(rapidjson::StringRef(_keyName.data(), _keyName.size()));
            if (it != data->MemberEnd() && it->value.IsDouble())
                return it->value.GetDouble();
        }

        return _defaultValue;
    }

    std::string OmicronContainer::stringValue(std::string_view _keyName, const std::string& _defaultValue) const
    {
        if (init_)
        {
            auto data = getData();
            auto it = data->FindMember(rapidjson::StringRef(_keyName.data(), _keyName.size()));
            if (it != data->MemberEnd() && it->value.IsString())
                return std::string(it->value.GetString(), it->value.GetStringLength());
        }

        return _defaultValue;
    }

    QString OmicronContainer::stringValue(std::string_view _keyName, const QString& _defaultValue) const
    {
        if (init_)
        {
            auto data = getData();
            auto it = data->FindMember(rapidjson::StringRef(_keyName.data(), _keyName.size()));
            if (it != data->MemberEnd() && it->value.IsString())
                return QString::fromUtf8(it->value.GetString(), it->value.GetStringLength());
        }

        return _defaultValue;
    }

    std::string OmicronContainer::jsonValue(std::string_view _keyName, const std::string& _defaultValue) const
    {
        if (init_)
        {
            auto data = getData();
            auto it = data->FindMember(rapidjson::StringRef(_keyName.data(), _keyName.size()));
            if (it != data->MemberEnd() && (it->value.IsObject() || it->value.IsArray()))
            {
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                it->value.Accept(writer);
                return std::string(buffer.GetString(), buffer.GetSize());
            }
        }

        return _defaultValue;
    }

    QString OmicronContainer::jsonValue(std::string_view _keyName, const QString& _defaultValue) const
    {
        if (init_)
        {
            auto data = getData();
            auto it = data->FindMember(rapidjson::StringRef(_keyName.data(), _keyName.size()));
            if (it != data->MemberEnd() && (it->value.IsObject() || it->value.IsArray()))
            {
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                it->value.Accept(writer);
                return QString::fromUtf8(buffer.GetString(), buffer.GetSize());
            }
        }

        return _defaultValue;
    }

    OmicronContainer::OmicronContainer()
        : init_(false)
        , data_(std::make_shared<rapidjson::Document>(rapidjson::kObjectType))
    {
    }

    std::shared_ptr<const rapidjson::Document> OmicronContainer::getData() const
    {
        std::lock_guard<SpinLock> lock(dataLock_);
        return data_;
    }
}
