#pragma once

#include "HistoryControlPageItem.h"

#include "../../types/message.h"

namespace Ui
{
    class MessageItemBase : public HistoryControlPageItem
    {
        Q_OBJECT

    public:
        MessageItemBase(QWidget* _parent);

        virtual ~MessageItemBase() = 0;

        virtual int32_t getTime() const = 0;

        virtual bool isEditable() const;
        virtual bool isUpdateable() const { return true; }

        virtual void callEditing() {}

        virtual void setNextHasSenderName(bool _nextHasSenderName);

        void setBuddy(const Data::MessageBuddy& _msg);
        const Data::MessageBuddy& buddy() const;
        Data::MessageBuddy& buddy();

        bool nextHasSenderName() const;

        virtual void setNextIsOutgoing(bool _nextIsOutgoing);

        bool nextIsOutgoing() const;

    Q_SIGNALS:
        void pressedDestroyed();

    private:
        bool nextHasSenderName_;
        Data::MessageBuddy msg_;
        bool nextIsOutgoing_;
    };

}

