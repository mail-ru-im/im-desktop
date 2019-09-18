#pragma once

#include "../../../types/chatheads.h"

Q_DECLARE_LOGGING_CATEGORY(heads)

namespace Heads
{
    class HeadContainer : public QObject
    {
        Q_OBJECT
    public:
        explicit HeadContainer(const QString& _aimId, QObject* _parent = nullptr);
        ~HeadContainer();

        const Data::HeadsById& headsById() const;
        bool hasHeads(qint64 _id) const;

    signals:
        void headChanged(const Data::ChatHeads& chatHeads, QPrivateSignal);
        void hide(QPrivateSignal);

    private:
        void onShowHeads();
        void onHideHeads();
        void onHeads(const Data::ChatHeads& chatHeads);

    private:
        const QString aimId_;
        Data::ChatHeads chatHeads_;
    };
}