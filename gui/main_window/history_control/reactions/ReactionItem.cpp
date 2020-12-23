#include "stdafx.h"

#include "ReactionItem.h"
#include "ReactionsPlate.h"

namespace Ui
{

QRect AccessibleReactionItem::rect() const
{
    if (item_)
    {
        if (auto plate = qobject_cast<Ui::ReactionsPlate *>(item_->parent()))
            return QRect(plate->mapToGlobal(item_->rect_.topLeft()), item_->rect_.size());
    }

    return QRect();
}

QString AccessibleReactionItem::text(QAccessible::Text _type) const
{
    return item_ && _type == QAccessible::Text::Name ? u"AS HistoryPage messageReaction " % item_->reaction_ % u' ' % QString::number(item_->msgId_) : QString();
}

}
