#pragma once

#include "types/lastseen.h"
#include "types/contact.h"

namespace Logic
{
    enum LastSeenRequestType
    {
        Default = 0,
        Subscribe = 1,
        CacheOnly = 2,
    };

    class LastseenContainer : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void lastseenChanged(const QString&, QPrivateSignal) const;

    public:
        LastseenContainer(QObject* _parent = nullptr);
        ~LastseenContainer() = default;

        void setContactLastSeen(const QString& _aimid, const Data::LastSeen& _lastSeen);
        void setContactBot(const QString& _aimid);

        bool isOnline(const QString& _aimId) const;
        bool isBot(const QString& _aimId) const;

        const Data::LastSeen& getLastSeen(const QString& _aimid, const LastSeenRequestType& _type = LastSeenRequestType::Default) const;

        void unsubscribe(const QString& _aimid);

    private:
        void emitChanged(const QString& _aimid) const;
        void userLastSeen(const int64_t, const std::map<QString, Data::LastSeen>& _lastseens);
        void userInfo(const int64_t, const QString& _aimid, const Data::UserInfo& _info);
        void updateLastSeens();
        void requestLastSeens();

    private:
        std::map<QString, Data::LastSeen> contactsSeens_;

        mutable std::map<QString, QDateTime> subscriptions_;
        mutable std::vector<QString> toRequest_;

        QTimer* updateTimer_;
        QTimer* requestTimer_;
    };

    LastseenContainer* GetLastseenContainer();
    void ResetLastseenContainer();
}