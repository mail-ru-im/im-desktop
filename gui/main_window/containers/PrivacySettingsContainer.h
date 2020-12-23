#pragma once

#include "utils/StringComparator.h"
#include "../../../corelib/enumerations.h"

namespace Logic
{
    class PrivacySettingsContainer : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void blacklistSize(const QString& _privacyGroup, int _size, QPrivateSignal) const;
        void valueChanged(const QString& _privacyGroup, core::privacy_access_right, QPrivateSignal) const;

    public:
        PrivacySettingsContainer(QObject* _parent = nullptr);

        void requestValues();
        core::privacy_access_right getCachedValue(QStringView _privacyGroup) const;
        void setValue(const QString& _settingsGroup, core::privacy_access_right _value);

    private:
        void onSetValueResult(int64_t _seq, bool _success);

    private:
        int64_t seqGet_ = -1;

        struct PrivacySetting
        {
            int64_t setSeq_ = -1;
            core::privacy_access_right value_ = core::privacy_access_right::not_set;
            core::privacy_access_right setValue_ = core::privacy_access_right::not_set;
            int blackListSize_ = 0;
        };
        std::map<QString, PrivacySetting, Utils::StringComparator> settings_;
    };

    PrivacySettingsContainer* GetPrivacySettingsContainer();
    void ResetPrivacySettingsContainer();
}