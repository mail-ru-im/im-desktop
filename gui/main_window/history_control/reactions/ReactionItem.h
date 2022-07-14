#pragma once

#include "controls/TextUnit.h"

namespace Ui
{

class ReactionItem : public QObject
{
    Q_OBJECT

public:
    ReactionItem(QObject* _parent) : QObject(_parent) {}

    QRect rect_;
    QRect emojiRect_;
    QImage emoji_;
    bool mouseOver_ = false;
    QString reaction_;
    int64_t count_ = 0;
    int64_t msgId_ = 0;
    double opacity_ = 1;
};

class AccessibleReactionItem : public QAccessibleObject
{
public:
    AccessibleReactionItem(ReactionItem* _item) : QAccessibleObject(_item), item_(_item) {}
    bool isValid() const override { return true; }
    QAccessibleInterface* parent() const override { return QAccessible::queryAccessibleInterface(item_->parent()); }
    int childCount() const override { return 0; }
    QAccessibleInterface* child(int _index) const override { return nullptr; }
    int indexOfChild(const QAccessibleInterface* _child) const override { return -1; }
    QRect rect() const override;
    QString text(QAccessible::Text _type) const override;
    QAccessible::Role role() const override { return QAccessible::Role::Button; }
    QAccessible::State state() const override { return {}; }

private:
    ReactionItem* item_;
};

}
