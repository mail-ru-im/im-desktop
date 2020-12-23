#include "stdafx.h"

#include "PrivacySettingsContainer.h"

#include "core_dispatcher.h"

namespace
{
    std::chrono::milliseconds getTimeout() noexcept { return std::chrono::milliseconds(700); }
}

namespace Logic
{
    PrivacySettingsContainer::PrivacySettingsContainer(QObject* _parent)
        : QObject(_parent)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::setPrivacyParamsResult, this, &PrivacySettingsContainer::onSetValueResult);
    }

    void PrivacySettingsContainer::requestValues()
    {
        if (seqGet_ >= 0)
            return;

        const auto onFailure = [this]()
        {
            seqGet_ = -1;
            Q_EMIT valueChanged(QString(), core::privacy_access_right::not_set, QPrivateSignal());
        };

        seqGet_ = Ui::GetDispatcher()->post_message_to_core("privacy_settings/get", nullptr, this, [this, onFailure](core::icollection* _coll)
        {
            // privacy_settings/get/result
            seqGet_ = -1;

            Ui::gui_coll_helper coll(_coll, false);
            int32_t err = coll.get_value_as_int("error", 0);
            if (err == 0)
            {
                const auto array = coll.get_value_as_array("privacy_groups");
                for (auto i = 0, size = array->size(); i < size; ++i)
                {
                    auto grp_coll = array->get_at(i)->get_as_collection();
                    Ui::gui_coll_helper grpHelper(grp_coll, false);
                    const auto grpName = grpHelper.get<QString>("name", "");
                    if (!grpName.isEmpty())
                    {
                        const auto val = grpHelper.get_value_as_enum<core::privacy_access_right>("allowTo", core::privacy_access_right::not_set);
                        const auto blSize = grpHelper.get_value_as_int("blacklistSize", 0);

                        auto& setting = settings_[grpName];
                        setting.value_ = val;
                        setting.blackListSize_ = blSize;

                        Q_EMIT valueChanged(grpName, val, QPrivateSignal());
                        Q_EMIT blacklistSize(grpName, blSize, QPrivateSignal());
                    }
                }
            }
            else
            {
                onFailure();
            }
        });

        QTimer::singleShot(getTimeout(), this, [seq = seqGet_, onFailure, this]()
        {
            if (seq == seqGet_)
                onFailure();
        });
    }

    core::privacy_access_right PrivacySettingsContainer::getCachedValue(QStringView _privacyGroup) const
    {
        if (const auto it = settings_.find(_privacyGroup); it != settings_.end())
            return it->second.value_;
        return core::privacy_access_right::not_set;
    }

    void PrivacySettingsContainer::setValue(const QString& _settingsGroup, core::privacy_access_right _value)
    {
        if (_settingsGroup.isEmpty())
            return;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("group", _settingsGroup);
        collection.set_value_as_enum("value", _value);
        const auto seq = Ui::GetDispatcher()->post_message_to_core("privacy_settings/set", collection.get());

        auto& setting = settings_[_settingsGroup];
        setting.setSeq_ = seq;
        setting.setValue_ = _value;

        QTimer::singleShot(getTimeout(), this, [this, _settingsGroup, seq]()
        {
            if (auto& s = settings_[_settingsGroup]; s.setSeq_ == seq)
                onSetValueResult(s.setSeq_, false);
        });
    }

    void PrivacySettingsContainer::onSetValueResult(int64_t _seq, bool _success)
    {
        auto it = std::find_if(settings_.begin(), settings_.end(), [_seq](const auto& p){ return p.second.setSeq_ == _seq; });
        if (it != settings_.end())
        {
            auto& setting = it->second;
            const auto val = _success ? setting.setValue_ : core::privacy_access_right::not_set;

            setting.value_ = val;
            setting.setValue_ = core::privacy_access_right::not_set;
            setting.setSeq_ = -1;

            Q_EMIT valueChanged(it->first, val, QPrivateSignal());
        }
    }

    static std::unique_ptr<PrivacySettingsContainer> g_container; // global. TODO/FIXME encapsulate with other global objects like recentsModel, contactModel

    PrivacySettingsContainer* GetPrivacySettingsContainer()
    {
        if (!g_container)
            g_container = std::make_unique<PrivacySettingsContainer>(nullptr);

        return g_container.get();
    }

    void ResetPrivacySettingsContainer()
    {
        g_container.reset();
    }
}