#include "stdafx.h"

#include "DateInserter.h"

#include "Message.h"
#include "MessageBuilder.h"

#include "../HistoryControlPageItem.h"
#include "../../contact_list/ContactListModel.h"

namespace hist
{
    DateInserter::DateInserter(const QString& _contact, QObject* _parent)
        : QObject(_parent), contact_(_contact)
    {
    }
    bool DateInserter::needDate(const Logic::MessageKey& prev, const Logic::MessageKey& key)
    {
        return needDate(prev.getDate(), key.getDate());
    }

    bool DateInserter::needDate(const QDate& prev, const QDate& key)
    {
        return prev != key;
    }

    bool DateInserter::isChat() const
    {
        return Utils::isChat(contact_);
    }

    Logic::MessageKey DateInserter::makeDateKey(const Logic::MessageKey& key) const
    {
        auto dateKey = key;
        dateKey.setType(core::message_type::undefined);
        dateKey.setControlType(Logic::control_type::ct_date);
        return dateKey;
    }

    std::unique_ptr<QWidget> DateInserter::makeDateItem(const Logic::MessageKey& key, int width, QWidget* parent) const
    {
        const auto& dateKey = key.isDate() ? makeDateKey(key) : key;
        Data::MessageBuddy message;
        message.Id_ = dateKey.getId();
        message.Prev_ = dateKey.getPrev();
        message.InternalId_ = dateKey.getInternalId();
        message.AimId_ = contact_;
        message.SetTime(0);
        message.SetDate(dateKey.getDate());
        message.SetType(core::message_type::undefined);

        return hist::MessageBuilder::makePageItem(message, width, parent);
    }
}
