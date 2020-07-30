#pragma once

#include "types/friendly.h"

Q_DECLARE_LOGGING_CATEGORY(friendlyContainer)

namespace Logic
{
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

        Q_SIGNALS:
            void friendlyChanged(const QString& _aimid, const QString& _friendly, QPrivateSignal) const;
            void friendlyChanged2(const QString& _aimid, const Data::Friendly& _friendly, QPrivateSignal) const;

        private:
            void onFriendlyChangedFromCore(const QString& _aimid, const Data::Friendly& _friendly);

        private:
            QHash<QString, Data::Friendly> friendlyMap_;
    };

    FriendlyContainer* GetFriendlyContainer();
    void ResetFriendlyContainer();
}
