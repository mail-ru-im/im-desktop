#pragma once

#include "controls/TextUnit.h"

namespace Statuses
{

struct StatusesListItem : public QObject
{
    Q_OBJECT

public:
    StatusesListItem(QObject* _parent) : QObject(_parent) {}

    QRect rect_;
    QRect emojiRect_;
    QRect buttonRect_;
    QRect buttonPressRect_;
    QString description_;
    QString statusCode_;
    QImage emoji_;
    Ui::TextRendering::TextUnitPtr descriptionUnit_;
    Ui::TextRendering::TextUnitPtr durationUnit_;
};

class AccessibleStatusesListItem : public QAccessibleObject
{
public:
    AccessibleStatusesListItem(StatusesListItem* _item) : QAccessibleObject(_item), item_(_item) {}
    bool isValid() const override { return true; }
    QAccessibleInterface* parent() const override { return QAccessible::queryAccessibleInterface(item_->parent()); }
    int childCount() const override { return 0; }
    QAccessibleInterface* child(int index) const override { return nullptr; }
    int indexOfChild(const QAccessibleInterface* child) const override { return -1; }
    QRect rect() const override;
    QString text(QAccessible::Text t) const override;
    QAccessible::Role role() const override { return QAccessible::Role::Button; }
    QAccessible::State state() const override { return {}; }

private:
    StatusesListItem* item_;
};

}
