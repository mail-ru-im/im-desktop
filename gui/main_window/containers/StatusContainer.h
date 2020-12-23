#pragma once

#include "statuses/Status.h"

namespace Logic
{
    class StatusContainer : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void statusChanged(const QString& _aimid, QPrivateSignal) const;

    public:
        StatusContainer(QObject* _parent = nullptr);
        ~StatusContainer() = default;

        const Statuses::Status& getStatus(const QString& _aimid) const;

        void setAvatarVisible(const QString& _aimid, bool _visible);
        void setStatus(const QString& _aimid, const Statuses::Status& _status);

    private:
        void onUpdateTimer();

        void updateSubscription(const QString& _aimid) const;
        void discardMyStatusIfTimeIsOver();

    private:
        std::map<QString, Statuses::Status> statuses_;
        mutable std::map<QString, QDateTime> subscriptions_;
        std::map<QString, int> visibleAvatars_;
        QTimer* updateTimer_;
    };

    StatusContainer* GetStatusContainer();
    void ResetStatusContainer();
}
