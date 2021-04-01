#include "stdafx.h"

#include "LinkAccessibleWidget.h"
#include "utils/utils.h"

namespace Ui::Accessibility
{
    QAccessibleInterface* AccessibleTextLink::parent() const
    {
        QAccessibleInterface* result = nullptr;
        if (auto obj = object())
            result = QAccessible::queryAccessibleInterface(obj->parent());
        im_assert(result);
        return result;
    }

    QRect AccessibleTextLink::rect() const
    {
        if (auto link = qobject_cast<LinkAccessibilityObject*>(object()))
        {
            if (auto wdg = qobject_cast<QWidget*>(link->parent()))
                return link->rect_.translated(wdg->mapToGlobal({}));
        }

        return {};
    }

    QString AccessibleTextLink::text(QAccessible::Text t) const
    {
        auto linkObject = qobject_cast<LinkAccessibilityObject*>(object());
        im_assert(linkObject);
        return QAccessible::Text::Name == t ? u"AS TextUnit link" % QString::number(linkObject->idx_) : QString();
    }

    AccessibleLinkWidget::AccessibleLinkWidget(LinkAccessibleWidget* _widget)
        : QAccessibleWidget(_widget)
    {
        im_assert(_widget->getTextUnit());
    }

    int AccessibleLinkWidget::childCount() const
    {
        return getLinkRects().size();
    }

    QAccessibleInterface* AccessibleLinkWidget::child(int index) const
    {
        const auto linkRects = getLinkRects();
        if (index > -1 && index < static_cast<int>(linkRects.size()))
        {
            const auto linkObjects = getWidget()->findChildren<LinkAccessibilityObject*>();
            const auto it = std::find_if(linkObjects.begin(), linkObjects.end(), [index](auto obj) { return obj->idx_ == index; });

            if (it == linkObjects.end())
            {
                auto obj = new LinkAccessibilityObject(getWidget(), linkRects[index], index);
                return QAccessible::queryAccessibleInterface(obj);
            }

            (*it)->rect_ = linkRects[index];
            return QAccessible::queryAccessibleInterface(*it);
        }
        return nullptr;
    }

    int AccessibleLinkWidget::indexOfChild(const QAccessibleInterface* child) const
    {
        const auto linkObj = qobject_cast<LinkAccessibilityObject*>(child->object());
        const auto linkRects = getLinkRects();

        return Utils::indexOf(linkRects.cbegin(), linkRects.cend(), linkObj->rect_);
    }

    QString AccessibleLinkWidget::text(QAccessible::Text t) const
    {
        if (t == QAccessible::Text::Name)
            return getWidget()->accessibleName();

        return {};
    }

    LinkAccessibleWidget* AccessibleLinkWidget::getWidget() const
    {
        if (auto w = qobject_cast<Ui::LinkAccessibleWidget*>(object()))
            return w;

        im_assert(false);
        return nullptr;
    }

    std::vector<QRect> AccessibleLinkWidget::getLinkRects() const
    {
        return getWidget()->getTextUnit()->getLinkRects();
    }
}