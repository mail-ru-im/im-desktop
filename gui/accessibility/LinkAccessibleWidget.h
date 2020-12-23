#pragma once

#include "controls/TextUnit.h"

namespace Ui
{
    namespace Accessibility
    {
        class AccessibleLinkWidget;
    }

    class LinkAccessibleWidget : public QWidget
    {
        Q_OBJECT

    public:
        LinkAccessibleWidget(QWidget* _parent) : QWidget(_parent) {}
        virtual const TextRendering::TextUnitPtr& getTextUnit() const = 0;

    private:
        friend class Ui::Accessibility::AccessibleLinkWidget;
    };

    namespace Accessibility
    {
        class LinkAccessibilityObject : public QObject
        {
            Q_OBJECT

        public:
            LinkAccessibilityObject(QObject* _parent, QRect _rect, int _idx)
                : QObject(_parent)
                , rect_(_rect)
                , idx_(_idx)
            {
            }

            QRect rect_;
            int idx_ = -1;
        };

        class AccessibleTextLink : public QAccessibleObject
        {
        public:
            AccessibleTextLink(LinkAccessibilityObject* _object) : QAccessibleObject(_object) {}

            QAccessibleInterface* parent() const override;
            bool isValid() const override { return true; }
            int childCount() const override { return 0; }
            QAccessibleInterface* child(int index) const override { return nullptr; }
            int indexOfChild(const QAccessibleInterface* child) const override { return -1; }
            QRect rect() const override;
            QString text(QAccessible::Text t) const override;
            QAccessible::Role role() const override { return QAccessible::Role::Link; }
            QAccessible::State state() const override { return {}; }
        };

        class AccessibleLinkWidget : public QAccessibleWidget
        {
        public:
            AccessibleLinkWidget(LinkAccessibleWidget* _widget);
            int childCount() const override;
            QAccessibleInterface* child(int index) const override;
            int indexOfChild(const QAccessibleInterface* child) const override;
            QString text(QAccessible::Text t) const override;

        private:
            LinkAccessibleWidget* getWidget() const;
            std::vector<QRect> getLinkRects() const;
        };
    }
}