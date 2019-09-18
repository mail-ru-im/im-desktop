#pragma once

#include "types/friendly.h"

Q_DECLARE_LOGGING_CATEGORY(friendlyContainer)

namespace Logic
{
    struct LastSeenSubscription
    {
        int32_t lastseen_ = -1;
        QDateTime subscriptionTime_;
    };

    class FriendlyContainer : public QObject
    {
        Q_OBJECT

        public:
            explicit FriendlyContainer(QObject* _parent = nullptr);
            ~FriendlyContainer();

            QString getFriendly(const QString& _aimid) const;
            QString getNick(const QString& _aimid) const;
            bool getOfficial(const QString& _aimid) const;
            Data::Friendly getFriendly2(const QString& _aimid) const;
            void updateFriendly(const QString& _aimid);

            void setContactLastSeen(const QString& _aimid, int32_t _lastSeen);

            int32_t getLastSeen(const QString& _aimid, bool _subscribe = false);
            QString getStatusString(int32_t _lastseen);
            QString getStatusString(const QString& _aimid);

            void unsubscribe(const QString& _aimid);

        Q_SIGNALS:
            void friendlyChanged(const QString& _aimid, const QString& _friendly, QPrivateSignal) const;
            void friendlyChanged2(const QString& _aimid, const Data::Friendly& _friendly, QPrivateSignal) const;
            void lastseenChanged(const QString&, QPrivateSignal);

        private:
            void onFriendlyChangedFromCore(const QString& _aimid, const Data::Friendly& _friendly);

        private Q_SLOTS:
            void userLastSeen(const int64_t _seq, const std::map<QString, int32_t>& _lastseens);
            void updateLastSeens();

        private:
            QHash<QString, Data::Friendly> friendlyMap_;
            std::map<QString, LastSeenSubscription> subscriptions_;
            std::map<QString, int32_t> contactsSeens_;
            QTimer* lastseenUpdateTimer_;
    };

    FriendlyContainer* GetFriendlyContainer();
    void ResetFriendlyContainer();
}
