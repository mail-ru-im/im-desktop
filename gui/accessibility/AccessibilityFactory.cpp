#include "stdafx.h"

#include "AccessibilityFactory.h"
#include "LinkAccessibleWidget.h"

#include "../previewer/Drawable.h"
#include "../previewer/GalleryFrame.h"
#include "../previewer/GalleryWidget.h"

#include "statuses/SelectStatusWidget.h"
#include "statuses/StatusListItem.h"

#include "../main_window/history_control/reactions/ReactionsPlate.h"
#include "../main_window/history_control/reactions/ReactionItem.h"
#include "../main_window/history_control/reactions/ReactionsList.h"
#include "../main_window/history_control/reactions/ReactionsListItem.h"
#include "../main_window/history_control/HistoryControlPageItem.h"

#include "controls/ContactAvatarWidget.h"


namespace Ui::Accessibility
{

    QAccessibleInterface* customWidgetsAccessibilityFactory(const QString& classname, QObject* object)
    {
        QAccessibleInterface* result = nullptr;

        if (u"Previewer::GalleryFrame" == classname)
        {
            if (auto objectCasted = qobject_cast<Previewer::GalleryFrame*>(object))
                result = new Previewer::AccessibleGalleryFrame(objectCasted);
        }
        else if (u"Previewer::GalleryWidget" == classname)
        {
            if (auto objectCasted = qobject_cast<Previewer::GalleryWidget*>(object))
                result = new Previewer::AccessibleGalleryWidget(objectCasted);
        }
        else if (u"Ui::ButtonAccessible" == classname)
        {
            if (auto objectCasted = qobject_cast<Ui::ButtonAccessible*>(object))
                result = new Ui::AccessibleButton(objectCasted);
        }
        else if (u"Ui::Accessibility::LinkAccessibilityObject" == classname)
        {
            if (auto objectCasted = qobject_cast<Ui::Accessibility::LinkAccessibilityObject*>(object))
                result = new Ui::Accessibility::AccessibleTextLink(objectCasted);
        }
        else if (auto objectCasted = qobject_cast<Ui::LinkAccessibleWidget*>(object))
        {
            result = new Ui::Accessibility::AccessibleLinkWidget(objectCasted);
        }
        else if (u"Statuses::StatusesList" == classname)
        {
            result = new Statuses::AccessibleStatusesList(static_cast<Statuses::StatusesList*>(object));
        }
        else if (u"Statuses::StatusesListItem" == classname)
        {
            result = new Statuses::AccessibleStatusesListItem(static_cast<Statuses::StatusesListItem*>(object));
        }
        else if (u"Ui::ReactionsPlate" == classname)
        {
            result = new Ui::AccessibleReactionsPlate(static_cast<Ui::ReactionsPlate*>(object));
        }
        else if (u"Ui::ReactionItem" == classname)
        {
            result = new Ui::AccessibleReactionItem(static_cast<Ui::ReactionItem*>(object));
        }
        else if (u"Ui::ReactionsListContent" == classname)
        {
            result = new Ui::AccessibleReactionsListContent(static_cast<Ui::ReactionsListContent*>(object));
        }
        else if (u"Ui::ReactionsListItem" == classname)
        {
            result = new Ui::AccessibleReactionsListItem(static_cast<Ui::ReactionsListItem*>(object));
        }
        else if (u"Ui::CancelReaction" == classname)
        {
            result = new Ui::AccessibleCancelReaction(static_cast<Ui::CancelReaction*>(object));
        }
        else if (u"Ui::HistoryControlPageItem" == classname)
        {
            result = new Ui::AccessibleHistoryControlPageItem(static_cast<Ui::HistoryControlPageItem*>(object));
        }
        else if (u"Ui::ContactAvatarWidget" == classname)
        {
            result = new Ui::AccessibleAvatarWidget(static_cast<Ui::ContactAvatarWidget*>(object));
        }

        return result;
    }

} // namespace Ui::Accessibility
