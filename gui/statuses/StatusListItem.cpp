#include "stdafx.h"

#include "StatusListItem.h"
#include "SelectStatusWidget.h"

namespace Statuses
{

QRect AccessibleStatusesListItem::rect() const
{
    auto list = qobject_cast<Statuses::StatusesList*>(item_->parent());
    if (list)
        return QRect(list->mapToGlobal(item_->rect_.topLeft()), item_->rect_.size());

    return QRect();
}

QString AccessibleStatusesListItem::text(QAccessible::Text t) const
{
    return QAccessible::Text::Name == t ? u"AS Statuses StatusesListItem_" % item_->description_ : QString();
}

}
