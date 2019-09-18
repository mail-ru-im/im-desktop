#pragma once

#include "utils/utils.h"

namespace core
{
    enum class privacy_access_right;
}

namespace Logic
{
    class PrivacySettingsManager : public QObject
    {
        Q_OBJECT

    public:
        PrivacySettingsManager(QObject* _parent = nullptr);

        using setValueCallback = std::function<void(bool)>;
        void setValue(const QString& _settingsGroup, const core::privacy_access_right _value, const QObject* _object, setValueCallback _callback);

        using getValueCallback = std::function<void(const core::privacy_access_right)>;
        void getValue(const QString& _settingsGroup, const QObject* _object, getValueCallback _callback);

    private:
        void onSetValueResult(const int64_t _seq, const bool _success);
        void onObjectDestroyed(QObject* _obj);

        int64_t getSeq_;

        template <typename T>
        struct CallbackInfo
        {
            T callback_;
            const QObject* object_;

            CallbackInfo(T&& _callback, const QObject* _object)
                : callback_(std::move(_callback)), object_(_object)
            {}
        };
        using setCallbackInfo = CallbackInfo<setValueCallback>;
        std::unordered_map<int64_t, setCallbackInfo> setCallbacks_;

        using getCallbackInfo = CallbackInfo<getValueCallback>;
        std::unordered_map<QString, getCallbackInfo, Utils::QStringHasher> getCallbacks_;
    };
}