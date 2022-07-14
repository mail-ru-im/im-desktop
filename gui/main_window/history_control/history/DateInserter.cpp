#include "stdafx.h"

#include "DateInserter.h"

#include "Message.h"
#include "MessageBuilder.h"

#include "../HistoryControlPageItem.h"
#include "../../contact_list/ContactListModel.h"
#include "../complex_message/ComplexMessageItem.h"

namespace hist
{
    DateInserter::DateInserter(const QString& _contact, QObject* _parent)
        : QObject(_parent), contact_(_contact)
    {
    }
    bool DateInserter::needDate(const Logic::MessageKey& _prev, const Logic::MessageKey& _key)
    {
        return needDate(_prev.getDate(), _key.getDate());
    }

    bool DateInserter::needDate(const QDate& _prev, const QDate& _key)
    {
        return _prev != _key;
    }

    bool DateInserter::isChat() const
    {
        return Utils::isChat(contact_);
    }

    Logic::MessageKey DateInserter::makeDateKey(const Logic::MessageKey& _key) const
    {
        auto dateKey = _key;
        dateKey.setType(core::message_type::undefined);
        dateKey.setControlType(Logic::ControlType::Date);
        return dateKey;
    }

    Data::MessageBuddy DateInserter::makeDateMessage(const Logic::MessageKey& _key) const
    {
        const auto& dateKey = _key.isDate() ? makeDateKey(_key) : _key;
        Data::MessageBuddy message;
        message.Id_ = dateKey.getId();
        message.Prev_ = dateKey.getPrev();
        message.InternalId_ = dateKey.getInternalId();
        message.AimId_ = contact_;
        message.SetTime(0);
        message.SetDate(dateKey.getDate());
        message.SetType(core::message_type::undefined);

        return message;
    }

    std::unique_ptr<Ui::HistoryControlPageItem> DateInserter::makeDateItem(const Logic::MessageKey& _key, int _width, QWidget* _parent) const
    {
        Data::MessageBuddy message = makeDateMessage(_key);
        return hist::MessageBuilder::makePageItem(message, _width, _parent);
    }

    std::unique_ptr<Ui::HistoryControlPageItem> DateInserter::makeDateItem(const Data::MessageBuddy& _message, int _width, QWidget* _parent) const
    {
        return hist::MessageBuilder::makePageItem(_message, _width, _parent);
    }
}
