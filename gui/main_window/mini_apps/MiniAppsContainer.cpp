#include "stdafx.h"

#include "MiniAppsContainer.h"

#include "MiniAppsUtils.h"

#include "utils/features.h"
#include "core_dispatcher.h"
#include "utils/InterConnector.h"
#include "../contact_list/Common.h"
#include "styles/ThemesContainer.h"

#include "main_window/contact_list/ContactListModel.h"

namespace
{
    static const std::vector<QString> defaultAppsOrder =
    {
        Utils::MiniApps::getMessengerId(),
        Utils::MiniApps::getSettingsId()
    };
}

namespace Logic
{
    MiniAppsContainer::MiniAppsContainer(QObject* _parent)
        : QObject(_parent)
    {
    }

    void MiniAppsContainer::initApps()
    {
        if (apps_.empty() || appsOrder_.empty())
            updateAppsConfig();
    }

    const std::vector<QString>& MiniAppsContainer::getAppsOrder()
    {
        return appsOrder_;
    }

    const Ui::MiniApp& MiniAppsContainer::getApp(const QString& _type) const
    {
        if (const auto appIter = apps_.find(_type); appIter != apps_.end() && appIter->second.isValid())
            return appIter->second;

        static Ui::MiniApp emptyApp;
        return emptyApp;
    }

    bool MiniAppsContainer::isAppEnabled(const QString& _id)
    {
        const auto& app = getApp(_id);
        return app.isValid() && app.isEnabled();
    }

    bool MiniAppsContainer::isCustomApp(const QString& _id)
    {
        const auto& app = getApp(_id);
        return app.isValid() && app.isCustomApp();
    }

    void MiniAppsContainer::setCurrentApp(const QString& _id)
    {
        currentApp_ = _id;
    }

    void MiniAppsContainer::updateAppsConfig()
    {
        const auto oldOrder = appsOrder_;
        auto oldApps = apps_;

        appsOrder_ = getEnabledMiniApps(Features::getServiceAppsOrder());
        if (appsOrder_.empty())
        {
            appsOrder_ = defaultAppsOrder;
            apps_ = createDefaultApps();
        }
        else
        {
            apps_ = getServiceApps();
            // TODO: remove when deprecated
            if (apps_.empty())
                apps_ = createDefaultApps();
            for (const auto& id : appsOrder_)
            {
                if (auto app = apps_.find(id); app != apps_.end() && app->second.isValid())
                    app->second.setEnabled(true);
            }
        }

        initCustomApps();
        appsOrder_.emplace_back(Utils::MiniApps::getSettingsId());

        const auto needFieldsUpdate = [this, &oldApps, isDark = Styling::getThemesContainer().isDarkTheme()](const QString& _id)
        {
            return oldApps[_id].needUpdate(getApp(_id), isDark);
        };

        const auto isInNewOrder = [this](const QString& _id)
        {
            return std::find(appsOrder_.begin(), appsOrder_.end(), _id) != appsOrder_.end();
        };

        std::vector<QString> tabsToRemove;
        for (size_t idx = 0; idx < oldOrder.size(); ++idx)
        {
            const auto id = oldOrder[idx];
            if (id == Utils::MiniApps::getMessengerId() || id == Utils::MiniApps::getSettingsId())
                continue;

            if (!isAppEnabled(id))
            {
                if (id == currentApp_)
                {
                    updatableApp_.id_ = id;
                    updatableApp_.command_ = UpdateCommand::Delete;
                }

                const auto isCallTab = id == Utils::MiniApps::getCallsId();
                if (!isCallTab || (isCallTab && !isInNewOrder(id)))
                    Q_EMIT removeApp(id, id == currentApp_ ? KeepWebPage::Yes : KeepWebPage::No);

                if (Utils::MiniApps::isFeedApp(id))
                    getContactListModel()->unmarkAsFeed(oldApps[id].getAimid());
            }
            else
            {
                if (needFieldsUpdate(id))
                {
                    if (id == currentApp_)
                    {
                        updatableApp_.id_ = id;
                        updatableApp_.command_ = UpdateCommand::Update;
                    }
                    else
                    {
                        Q_EMIT updateAppFields(id);
                    }
                }

                if (idx >= appsOrder_.size() || id != appsOrder_[idx])
                    Q_EMIT removeTab(id);
            }
        }

        if (!updatableApp_.id_.isEmpty() && updatableApp_.command_ == UpdateCommand::Delete)
        {
            if (isInNewOrder(updatableApp_.id_))
            {
                if (needFieldsUpdate(updatableApp_.id_))
                    updatableApp_.command_ = UpdateCommand::Update;
                else
                    clearUpdatable();
            }
        }

        Q_EMIT updateApps(currentApp_.isEmpty() ? Utils::MiniApps::getMessengerId() : currentApp_);
        currentApp_.clear();
    }

    MiniAppMap MiniAppsContainer::getServiceApps()
    {
        MiniAppMap apps;
        rapidjson::Document doc;
        if (!doc.Parse(Features::getServiceAppsJson()).HasParseError())
        {
            const auto iterApps = doc.FindMember("apps-config");
            if (iterApps != doc.MemberEnd() && iterApps->value.IsObject())
            {
                for (auto iter = iterApps->value.MemberBegin(); iter != iterApps->value.MemberEnd(); ++iter)
                {
                    Ui::MiniApp app;
                    app.setId(QString::fromUtf8(iter->name.GetString()));
                    app.unserialize(iter->value.GetObject());
                    if (app.isValid())
                    {
                        im_assert(!app.getId().isEmpty());
                        app.setService(true);
                        apps[app.getId()] = std::move(app);
                    }
                }
            }
        }

        return apps;
    }

    void MiniAppsContainer::initCustomApps()
    {
        rapidjson::Document doc;
        if (!doc.Parse(Features::getCustomAppsJson()).HasParseError())
        {
            const auto iterApps = doc.FindMember("custom-miniapps");
            if (iterApps != doc.MemberEnd() &&iterApps->value.IsArray())
            {
                for (const auto& appNode : iterApps->value.GetArray())
                {
                    Ui::MiniApp app;
                    app.unserialize(appNode);
                    if (app.isValid())
                    {
                        app.setEnabled(true);
                        appsOrder_.push_back(app.getId());
                        apps_[app.getId()] = std::move(app);
                    }
                }
            }
        }
    }

    std::vector<QString> MiniAppsContainer::getEnabledMiniApps(const QString& _text)
    {
        std::vector<QString> res;
        rapidjson::Document doc;

        const auto pushApps = [&res](auto _array)
        {
            for (const auto& app : _array)
                res.push_back(QString::fromUtf8(app.GetString()));
        };

        if (!doc.Parse(_text.toStdString().c_str()).HasParseError())
        {
            if (!doc.IsArray())
            {
                const auto iterApps = doc.FindMember("apps-order");
                if (iterApps != doc.MemberEnd() && iterApps->value.IsArray())
                    pushApps(iterApps->value.GetArray());
            }
            else
            {
                pushApps(doc.GetArray());
            }
        }
        return res;
    }

    MiniAppMap MiniAppsContainer::createDefaultApps()
    {
        MiniAppMap defaultApps;
        defaultApps[Utils::MiniApps::getTasksId()] = Utils::MiniApps::getDefaultTasks();
        defaultApps[Utils::MiniApps::getMailId()] = Utils::MiniApps::getDefaultMail();
        defaultApps[Utils::MiniApps::getCloudId()] = Utils::MiniApps::getDefaultCloud();
        defaultApps[Utils::MiniApps::getOrgstructureId()] = Utils::MiniApps::getDefaultOrgstructure();
        defaultApps[Utils::MiniApps::getCalendarId()] = Utils::MiniApps::getDefaultCalendar();
        defaultApps[Utils::MiniApps::getTarmMailId()] = Utils::MiniApps::getDefaultTarmMail();
        defaultApps[Utils::MiniApps::getTarmCallsId()] = Utils::MiniApps::getDefaultTarmCalls();
        defaultApps[Utils::MiniApps::getTarmCloudId()] = Utils::MiniApps::getDefaultTarmCloud();
        return defaultApps;
    }

    void MiniAppsContainer::clearUpdatable()
    {
        updatableApp_.id_.clear();
        updatableApp_.command_ = UpdateCommand::None;
    }

    void MiniAppsContainer::onAppHidden(const QString& _id)
    {
        if (updatableApp_.id_ == _id)
        {
            if (updatableApp_.command_ == UpdateCommand::Delete)
                Q_EMIT removeHiddenPage(_id);
            else if (updatableApp_.command_ == UpdateCommand::Update)
                Q_EMIT updateAppFields(_id);

            clearUpdatable();
        }
    }

    static QObjectUniquePtr<MiniAppsContainer> g_miniapps;

    MiniAppsContainer* GetAppsContainer()
    {
        Utils::ensureMainThread();
        if (!g_miniapps)
            g_miniapps = makeUniqueQObjectPtr<MiniAppsContainer>();

        return g_miniapps.get();
    }

    void ResetAppsContainer()
    {
        Utils::ensureMainThread();
        g_miniapps.reset();
    }
}
