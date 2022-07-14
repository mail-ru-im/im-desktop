#pragma once

namespace Ui
{
    class MiniApp;
}

namespace Utils
{
    namespace MiniApps
    {
        const QString& getMessengerId();
        const QString& getSettingsId();
        const QString& getCallsId();
        const QString& getOrgstructureId();
        const QString& getMailId();
        const QString& getCloudId();
        const QString& getTasksId();
        const QString& getCalendarId();
        const QString& getTarmMailId();
        const QString& getTarmCloudId();
        const QString& getTarmCallsId();
        const QString& getAccountDeleteId();
        const QString& getNewsId();
        const QString& getPollsId();

        bool isTarmApp(const QString& _id);
        const std::unordered_set<QString>& getMessengerAppTypes();

        bool isFeedApp(const QString& _id);
        const std::unordered_set<QString>& getFeedAppTypes();

        bool setDefaultAvatar(Ui::MiniApp& _app);

        bool isAppUseMainAimsid(const QString& _miniAppId);

        QUrl composeAppUrl(const QString& _id, const QString& _url, bool _dark);
        // TODO: remove when deprecated in server configs
        QUrl composeDefaultAppUrl(const QString& _id, bool _dark);
        QString getDefaultName(const QString& _id);

        Ui::MiniApp createDefaultApp(const QString& _id, bool _enabled, bool _needsAuth, bool _external);

        Ui::MiniApp getDefaultTasks();
        Ui::MiniApp getDefaultMail();
        Ui::MiniApp getDefaultCloud();
        Ui::MiniApp getDefaultCalendar();
        Ui::MiniApp getDefaultOrgstructure();
        Ui::MiniApp getDefaultTarmMail();
        Ui::MiniApp getDefaultTarmCloud();
        Ui::MiniApp getDefaultTarmCalls();
    }
}
