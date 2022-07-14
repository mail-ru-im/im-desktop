#include "stdafx.h"

#include "MiniAppsUtils.h"
#include "MiniApps.h"
#include "MiniAppsContainer.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "url_config.h"
#include "my_info.h"
#include "utils/features.h"

namespace
{
    using IconsPair = std::pair<QString, QString>;
    IconsPair getDefaultIconPaths(const QString& _id)
    {
        static const std::unordered_map<QString, IconsPair> defaultIcons =
        {
            { Utils::MiniApps::getOrgstructureId(), { qsl(":/tab/contacts"),   qsl(":/tab/contacts_active") } },
            { Utils::MiniApps::getMailId(),         { qsl(":/tab/mail"),       qsl(":/tab/mail_active")     } },
            { Utils::MiniApps::getCloudId(),         { qsl(":/tab/tarm_cloud"),qsl(":/tab/tarm_cloud_active")      } },
            { Utils::MiniApps::getTasksId(),        { qsl(":/tab/tasks"),      qsl(":/tab/tasks_active")    } },
            { Utils::MiniApps::getCalendarId(),     { qsl(":/tab/calendar"),   qsl(":/tab/calendar_active") } },
            { Utils::MiniApps::getTarmMailId(),     { qsl(":/tab/tarm_mail"),  qsl(":/tab/tarm_mail")       } },
            { Utils::MiniApps::getTarmCallsId(),    { qsl(":/tab/tarm_calls"), qsl(":/tab/tarm_calls")      } },
            { Utils::MiniApps::getTarmCloudId(),    { qsl(":/tab/tarm_cloud"), qsl(":/tab/tarm_cloud")      } },
            { Utils::MiniApps::getNewsId(),         { qsl(":/tab/news"),       qsl(":/tab/news_active")     } },
            { Utils::MiniApps::getPollsId(),        { qsl(":/tab/polls"),      qsl(":/tab/polls_active")    } },
        };

        if (const auto& appIter = defaultIcons.find(_id); appIter != defaultIcons.end())
            return appIter->second;

        return {};
    }

    int randomNumber()
    {
        static std::random_device rd;
        static std::mt19937 gen{ rd() };
        static std::uniform_int_distribution distrib{ 100, 999 };
        return distrib(gen);
    }
}

namespace Utils
{
    namespace MiniApps
    {
        const QString& getMessengerId()
        {
            static const QString str(qsl("messenger"));
            return str;
        }

        const QString& getSettingsId()
        {
            static const QString str(qsl("settings"));
            return str;
        }

        const QString& getCallsId()
        {
            static const QString str(qsl("calls"));
            return str;
        }

        const QString& getOrgstructureId()
        {
            static const QString str(qsl("orgstructure"));
            return str;
        }

        const QString& getMailId()
        {
            static const QString str(qsl("mail"));
            return str;
        }

        const QString& getCloudId()
        {
            static const QString str(qsl("cloud"));
            return str;
        }

        const QString& getTasksId()
        {
            static const QString str(qsl("tasks"));
            return str;
        }

        const QString& getCalendarId()
        {
            static const QString str(qsl("calendar"));
            return str;
        }

        const QString& getTarmMailId()
        {
            static const QString str(qsl("tarm-mail"));
            return str;
        }

        const QString& getTarmCloudId()
        {
            static const QString str(qsl("tarm-cloud"));
            return str;
        }

        const QString& getTarmCallsId()
        {
            static const QString str(qsl("tarm-calls"));
            return str;
        }

        const QString& getAccountDeleteId()
        {
            static const QString str(qsl("account_delete"));
            return str;
        }

        const QString& getNewsId()
        {
            static const QString str(qsl("news"));
            return str;
        }

        const QString& getPollsId()
        {
            static const QString str(qsl("polls"));
            return str;
        }

        bool isTarmApp(const QString& _id)
        {
            return _id == getTarmMailId() || _id == getTarmCloudId() || _id == getTarmCallsId();
        }

        const std::unordered_set<QString>& getMessengerAppTypes()
        {
            static const std::unordered_set<QString> types =
            {
                getMessengerId(),
                getSettingsId(),
                getCallsId(),
                getTarmMailId(),
                getTarmCallsId(),
                getTarmCloudId(),
                getAccountDeleteId(),
                getNewsId(),
                getPollsId()
            };
            return types;
        }

        bool isFeedApp(const QString& _id)
        {
            return getFeedAppTypes().find(_id) != getFeedAppTypes().end();
        }

        const std::unordered_set<QString>& getFeedAppTypes()
        {
            static const std::unordered_set<QString> types =
            {
                getNewsId(),
                getPollsId()
            };
            return types;
        }

        bool setDefaultAvatar(Ui::MiniApp& _app)
        {
            if (const auto [iconPath, iconActivePath] = getDefaultIconPaths(_app.getId());
                !iconPath.isEmpty() && !iconActivePath.isEmpty())
            {
                _app.setDefaultIconPaths(iconPath, iconActivePath);
                return true;
            }

            return false;
        }

        bool isAppUseMainAimsid(const QString& _miniAppId)
        {
            return _miniAppId == Utils::MiniApps::getMailId() || _miniAppId == Utils::MiniApps::getCalendarId() ||
                _miniAppId == Utils::MiniApps::getOrgstructureId() || Logic::GetAppsContainer()->isCustomApp(_miniAppId);
        }

        QUrl composeAppUrl(const QString& _id, const QString& _url, bool _dark)
        {
            if (_url.isEmpty() || _id.isEmpty())
                return {};

            const QString lang = Utils::GetTranslator()->getLang();
            if (_id == getOrgstructureId())
                return QUrl(Utils::addHttpsIfNeeded(_url + qsl("?avatar_url=") + Ui::getUrlConfig().getAvatarsUrl()));

            if (_id == getTasksId())
            {
                return QUrl(Utils::addHttpsIfNeeded(_url + qsl("?page=tasklist")
                    + qsl("&config_host=") + QUrl(Ui::getUrlConfig().getConfigHost()).host() // doing "u.myteam.vmailru.net" from the original configHost
                    + qsl("&dark=") + (_dark ? qsl("1") : qsl("0"))
                    + qsl("&lang=") + lang
                    + qsl("&r=") + QString::number(randomNumber())
                    + qsl("&topbar=off")));
            }

            if (_id == getCalendarId())
                return QUrl(Utils::addHttpsIfNeeded(_url + qsl("?theme=") + (_dark ? qsl("dark") : qsl("light"))));

            if (_id == getMailId())
            {
                return QUrl(Utils::addHttpsIfNeeded(_url + qsl("?wv=1")
                       + qsl("&wv-dark=") + (_dark ? qsl("1") : qsl("0"))
                       + qsl("&email=") + Ui::MyInfo()->aimId()));
            }

            return Utils::addHttpsIfNeeded(_url);
        }

        QUrl composeDefaultAppUrl(const QString& _id, bool _dark)
        {
            auto composeUrl = [_id, _dark](const QString& _url)->QUrl
            {
                return composeAppUrl(_id, _url, _dark);
            };

            if (_id == getOrgstructureId())
                return composeUrl(Ui::getUrlConfig().getDi(_dark));

            if (_id == getTasksId())
                return composeUrl(Ui::getUrlConfig().getTasks());

            if (_id == getCalendarId())
                return composeUrl(Ui::getUrlConfig().getCalendar());

            if (_id == getMailId())
                return composeUrl(Ui::getUrlConfig().getMail());

            return {};
        }

        QString getDefaultName(const QString& _id)
        {
            static const std::unordered_map<QString, QString> defaultName =
            {
                { Utils::MiniApps::getOrgstructureId(), QT_TRANSLATE_NOOP("tab", "Contacts")},
                { Utils::MiniApps::getMailId(),         QT_TRANSLATE_NOOP("tab", "Mail")    },
                { Utils::MiniApps::getCloudId(),        QT_TRANSLATE_NOOP("tab", "Cloud")   },
                { Utils::MiniApps::getTasksId(),        QT_TRANSLATE_NOOP("tab", "Tasks")   },
                { Utils::MiniApps::getCalendarId(),     QT_TRANSLATE_NOOP("tab", "Calendar")},
                { Utils::MiniApps::getTarmMailId(),     QT_TRANSLATE_NOOP("tab", "Mail")    },
                { Utils::MiniApps::getTarmCallsId(),    qsl("TrueConf")                     },
                { Utils::MiniApps::getTarmCloudId(),    QT_TRANSLATE_NOOP("tab", "Cloud")   },
                { Utils::MiniApps::getNewsId(),         QT_TRANSLATE_NOOP("tab", "News") },
                { Utils::MiniApps::getPollsId(),        QT_TRANSLATE_NOOP("tab", "Polls") },
            };

            if (const auto& appIter = defaultName.find(_id); appIter != defaultName.end())
                return appIter->second;

            return {};
        }

        Ui::MiniApp createDefaultApp(const QString& _id, bool _enabled, bool _needsAuth, bool _external)
        {
            const auto [icon, iconActive] = getDefaultIconPaths(_id);
            Ui::AppIcon iconFilled;
            Ui::AppIcon iconOutline;
            iconFilled.defaultIconPath_ = iconActive;
            iconOutline.defaultIconPath_ = icon;

            return { _id,
                     {},
                     getDefaultName(_id),
                     composeDefaultAppUrl(_id, false).toString(),
                     composeDefaultAppUrl(_id, true).toString(),
                     {},
                     {},
                     iconFilled,
                     iconOutline,
                     _enabled,
                     _needsAuth,
                     _external,
                     true};
        }

        Ui::MiniApp getDefaultTasks()
        {
            return createDefaultApp(getTasksId(), Features::isTasksEnabled(), true, false);
        }

        Ui::MiniApp getDefaultMail()
        {
            return createDefaultApp(getMailId(), Features::isMailEnabled(), !Features::mailSelfAuth(), false);
        }

        Ui::MiniApp getDefaultCloud()
        {
            return createDefaultApp(getCloudId(), Logic::GetAppsContainer()->isAppEnabled(Utils::MiniApps::getMailId()), !Features::cloudSelfAuth(), false);
        }

        Ui::MiniApp getDefaultCalendar()
        {
            return createDefaultApp(getCalendarId(), Features::isCalendarEnabled(), !Features::calendarSelfAuth(), false);
        }

        Ui::MiniApp getDefaultOrgstructure()
        {
            return createDefaultApp(getOrgstructureId(), Features::isContactsEnabled(), true, false);
        }

        Ui::MiniApp getDefaultTarmMail()
        {
            return createDefaultApp(getTarmMailId(), Features::isTarmMailEnabled(), true, true);
        }

        Ui::MiniApp getDefaultTarmCloud()
        {
            return createDefaultApp(getTarmCloudId(), Features::isTarmCloudEnabled(), true, true);
        }

        Ui::MiniApp getDefaultTarmCalls()
        {
            return createDefaultApp(getTarmCallsId(), Features::isTarmCallsEnabled(), true, true);
        }
    }
}
