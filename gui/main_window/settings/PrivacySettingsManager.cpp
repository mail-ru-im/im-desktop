#include "PrivacySettingsManager.h"

#include "core_dispatcher.h"
#include "utils/gui_coll_helper.h"

namespace
{
    std::chrono::milliseconds getTimeout() noexcept { return std::chrono::milliseconds(700); }
}

namespace Logic
{
    std::unique_ptr<PrivacySettingsManager> g_privacy_settings_manager;

    PrivacySettingsManager::PrivacySettingsManager(QObject* _parent)
        : QObject(_parent)
        , getSeq_(-1)
    {
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::setPrivacyParamsResult, this, &PrivacySettingsManager::onSetValueResult);
    }

    void PrivacySettingsManager::requestPrivacySettings()
    {
        if (getSeq_ >= 0)
            return;

        getSeq_ = Ui::GetDispatcher()->post_message_to_core("privacy_settings/get", nullptr, this, [this](core::icollection* _coll)
        {
            getSeq_ = -1;

            Ui::gui_coll_helper coll(_coll, false);
            int32_t err = coll.get_value_as_int("error", 0);
            if (err == 0)
            {
                const auto array = coll.get_value_as_array("privacy_groups");
                for (auto i = 0, size = array->size(); i < size; ++i)
                {
                    auto grp_coll = array->get_at(i)->get_as_collection();
                    Ui::gui_coll_helper grpHelper(grp_coll, false);
                    auto grpName = grpHelper.get<QString>("name", "");
                    const auto val = grpHelper.get_value_as_enum<core::privacy_access_right>("allowTo", core::privacy_access_right::not_set);

                    cached_values_[std::move(grpName)] = val;
                }
            }
        });
    }

    void PrivacySettingsManager::setValue(const QString& _settingsGroup, const core::privacy_access_right _value, const QObject* _object, setValueCallback _callback)
    {
        connect(_object, &QObject::destroyed, this, &PrivacySettingsManager::onObjectDestroyed, Qt::UniqueConnection);

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("group", _settingsGroup);
        collection.set_value_as_enum("value", _value);
        const auto it = setCallbacks_.emplace(Ui::GetDispatcher()->post_message_to_core("privacy_settings/set", collection.get()), setCallbackInfo(std::move(_callback), _object));
        assert(it.second);

        QTimer::singleShot(getTimeout(), this, [this, _seq = it.first->first]()
        {
            onSetValueResult(_seq, false);
        });
    }

    void PrivacySettingsManager::getValue(const QString& _settingsGroup, const QObject* _object, getValueCallback _callback)
    {
        getCallbacks_.emplace(_settingsGroup, getCallbackInfo(std::move(_callback), _object));
        connect(_object, &QObject::destroyed, this, &PrivacySettingsManager::onObjectDestroyed, Qt::UniqueConnection);

        if (getSeq_ >= 0)
            return;

        const auto onFailure = [this]()
        {
            getSeq_ = -1;
            for (auto c : getCallbacks_)
                c.second.callback_(core::privacy_access_right::not_set);
            getCallbacks_.clear();
        };

        getSeq_ = Ui::GetDispatcher()->post_message_to_core("privacy_settings/get", nullptr, this, [this, onFailure](core::icollection* _coll)
        {
            // privacy_settings/get/result
            getSeq_ = -1;

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
                    const auto val = grpHelper.get_value_as_enum<core::privacy_access_right>("allowTo", core::privacy_access_right::not_set);

                    cached_values_[grpName] = val;
                    if (const auto it = getCallbacks_.find(grpName); it != getCallbacks_.end())
                    {
                        it->second.callback_(val);
                        getCallbacks_.erase(it);
                    }
                }
            }
            else
            {
                onFailure();
            }
        });

        QTimer::singleShot(getTimeout(), this, [seq = getSeq_, onFailure, this]()
        {
            if (seq == getSeq_)
                onFailure();
        });
    }

    core::privacy_access_right PrivacySettingsManager::get_cached_value(const QString& _settingsGroup)
    {
        const auto iter = cached_values_.find(_settingsGroup);
        if (iter == cached_values_.end())
            return core::privacy_access_right::not_set;

        return iter->second;
    }

    void PrivacySettingsManager::onSetValueResult(const int64_t _seq, const bool _success)
    {
        if (const auto it = setCallbacks_.find(_seq); it != setCallbacks_.end())
        {
            it->second.callback_(_success);
            setCallbacks_.erase(it);
        }
    }

    void PrivacySettingsManager::onObjectDestroyed(QObject* _obj)
    {
        const auto eraseFormMap = [_obj](auto& _map)
        {
            for (auto iter = _map.cbegin(), end = _map.cend(); iter != end; ++iter)
            {
                if (iter->second.object_ == _obj)
                    _map.erase(iter);
            }
        };

        eraseFormMap(getCallbacks_);
        eraseFormMap(setCallbacks_);
    }

    PrivacySettingsManager* getPrivacySettingsManager()
    {
        if (!g_privacy_settings_manager)
            g_privacy_settings_manager = std::make_unique<Logic::PrivacySettingsManager>(nullptr);

        return g_privacy_settings_manager.get();
    }

    void ResetPrivacySettingsManager()
    {
        if (g_privacy_settings_manager)
            g_privacy_settings_manager.reset();
    }
}
