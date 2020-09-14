#include "stdafx.h"

#include "AccessibilityFactory.h"

#include "../previewer/Drawable.h"
#include "../previewer/GalleryFrame.h"
#include "../previewer/GalleryWidget.h"

#include "../main_window/smiles_menu/Store.h"


namespace Ui::Accessibility
{

    QAccessibleInterface* customWidgetsAccessibilityFactory(const QString& classname, QObject* object)
    {
        QAccessibleInterface* result = nullptr;

        if (u"Ui::Stickers::PackInfoObject" == classname)
        {
            if (auto objectCasted = qobject_cast<Ui::Stickers::PackInfoObject*>(object))
                result = new Ui::Stickers::AccessibleStickerPackButton(objectCasted);
        }
        else if (u"Previewer::GalleryFrame" == classname)
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
        else if (u"Ui::Stickers::PacksView" == classname)
        {
            if (auto objectCasted = qobject_cast<Ui::Stickers::PacksView*>(object))
                result = new Ui::Stickers::AccessiblePacksView(objectCasted);
        }

        return result;
    }

} // namespace Ui::Accessibility