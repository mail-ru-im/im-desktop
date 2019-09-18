#include "stdafx.h"

#include "MessageItemBase.h"
#include "../../gui_settings.h"

namespace Ui
{
    MessageItemBase::MessageItemBase(QWidget* _parent)
        : HistoryControlPageItem(_parent)
        , nextHasSenderName_(false)
        , nextIsOutgoing_(false)
    {
    }

    MessageItemBase::~MessageItemBase() = default;

    bool MessageItemBase::isEditable() const
    {
        const auto diff = std::chrono::system_clock::now() - std::chrono::system_clock::from_time_t(getTime());
        return isOutgoing() && diff <= std::chrono::hours(48);
    }

    void MessageItemBase::setNextHasSenderName(bool _nextHasSender)
    {
        nextHasSenderName_ = _nextHasSender;
        updateSize();
    }

    void MessageItemBase::setBuddy(const Data::MessageBuddy& _msg)
    {
        msg_ = _msg;
    }

    Data::MessageBuddy& MessageItemBase::buddy()
    {
        return msg_;
    }

    const Data::MessageBuddy& MessageItemBase::buddy() const
    {
        return msg_;
    }

    bool MessageItemBase::nextHasSenderName() const
    {
        return nextHasSenderName_;
    }

    void MessageItemBase::setNextIsOutgoing(bool _nextIsOutgoing)
    {
        nextIsOutgoing_ = _nextIsOutgoing;
        updateSize();
    }

    bool MessageItemBase::nextIsOutgoing() const
    {
        return nextIsOutgoing_;
    }
}
