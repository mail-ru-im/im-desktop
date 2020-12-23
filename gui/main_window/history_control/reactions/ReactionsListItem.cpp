#include "stdafx.h"

#include "ReactionsListItem.h"
#include "ReactionsList.h"

namespace Ui
{

int AccessibleReactionsListItem::childCount() const
{
    return item_ && item_->myReaction_;
}

QAccessibleInterface* AccessibleReactionsListItem::child(int _index) const
{
    if (!item_ || !item_->myReaction_ || _index)
        return nullptr;

    return QAccessible::queryAccessibleInterface(cancel_);
}

int AccessibleReactionsListItem::indexOfChild(const QAccessibleInterface* _child) const
{
    if (item_ && _child && cancel_ == qobject_cast<CancelReaction*>(_child->object()))
        return 0;
    return -1;
}

QRect AccessibleReactionsListItem::rect() const
{
    if (item_)
        if (auto list = qobject_cast<Ui::ReactionsListContent*>(item_->parent()))
            return QRect(list->mapToGlobal(item_->rect_.topLeft()), item_->rect_.size());

    return QRect();
}

QString AccessibleReactionsListItem::text(QAccessible::Text _type) const
{
    return item_ && _type == QAccessible::Text::Name ? u"AS ReactionPopup member " % item_->contact_ : QString();
}

QRect AccessibleCancelReaction::rect() const
{
    if (cancel_ && cancel_->item_)
        if (auto list = qobject_cast<Ui::ReactionsListContent*>(cancel_->item_->parent()))
            return QRect(list->mapToGlobal(cancel_->item_->buttonRect_.topLeft()), cancel_->item_->buttonRect_.size());

    return QRect();
}

QString AccessibleCancelReaction::text(QAccessible::Text _type) const
{
    const auto allowName = cancel_ && cancel_->item_ && cancel_->item_->myReaction_;
    return allowName && _type == QAccessible::Text::Name? qsl("AS ReactionPopup reactionCancel") : QString();
}

}
