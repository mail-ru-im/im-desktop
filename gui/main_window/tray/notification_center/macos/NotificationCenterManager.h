#pragma once

namespace Ui
{
    enum class AlertType;

    class NotificationCenterManager : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void messageClicked(const QString _aimId, const QString _mailId, const qint64 _messageId, AlertType _alertType);

        void osxThemeChanged();

    private Q_SLOTS:
        void avatarChanged(QString);
        void avatarTimer();
        void displayTimer();
    private:
        void HideQueuedNotifications(const QString& _aimId);

        std::set<QString> changedAvatars_;
        QTimer* avatarTimer_;
        QTimer* displayTimer_;
        bool isScreenLocked_;

    public:
        NotificationCenterManager();
        ~NotificationCenterManager();

        void DisplayNotification(
            const QString& alertType,
            const QString& aimdId,
            const QString& senderNick,
            const QString& message,
            const QString& mailId,
            const QString& displayName,
            const QString& messageId);

        void HideNotifications(const QString& aimId);

        void Activated(
            const QString& alertType,
            const QString& aimId,
            const QString& mailId,
            const QString& messageId);

        void themeChanged();

        void setScreenLocked(const bool _locked);

        void reinstallDelegate();

        void removeAllNotifications();

        static void updateBadgeIcon(int unreads);
    };
}
