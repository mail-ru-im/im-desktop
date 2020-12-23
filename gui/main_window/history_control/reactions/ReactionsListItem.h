#pragma once

#include "controls/TextUnit.h"

namespace Ui
{

class ReactionsListItem : public QObject
{
    Q_OBJECT

public:
    ReactionsListItem(QObject* _parent) : QObject(_parent) {}

    QRect rect_;
    TextRendering::TextUnitPtr nameUnit_;
    QPixmap avatar_;
    QRect avatarRect_;
    QRect emojiRect_;
    QRect buttonRect_;
    QString contact_;
    QString reaction_;
    double opacity_ = 1;
    bool buttonHovered_ = false;
    bool buttonPressed_ = false;
    bool hovered_ = false;
    bool pressed_ = false;
    bool myReaction_ = false;
};

class CancelReaction : public QObject
{
    Q_OBJECT

public:
    CancelReaction(ReactionsListItem* _item) : QObject(_item), item_(_item) {}

    ReactionsListItem* item_;
};

class AccessibleReactionsListItem : public QAccessibleObject
{
public:
    AccessibleReactionsListItem(ReactionsListItem* _item) : QAccessibleObject(_item), item_(_item), cancel_(new CancelReaction(_item)) {}
    bool isValid() const override { return true; }
    QAccessibleInterface* parent() const override { return QAccessible::queryAccessibleInterface(item_->parent()); }
    int childCount() const override;
    QAccessibleInterface* child(int _index) const override;
    int indexOfChild(const QAccessibleInterface* _child) const override;
    QRect rect() const override;
    QString text(QAccessible::Text _type) const override;
    QAccessible::Role role() const override { return QAccessible::Role::ListItem; }
    QAccessible::State state() const override { return {}; }

private:
    ReactionsListItem* item_;
    CancelReaction* cancel_;
};

class AccessibleCancelReaction : public QAccessibleObject
{
public:
    AccessibleCancelReaction(CancelReaction* _cancel) : QAccessibleObject(_cancel), cancel_(_cancel) {}
    bool isValid() const override { return true; }
    QAccessibleInterface* parent() const override { return QAccessible::queryAccessibleInterface(cancel_->parent()); }
    int childCount() const override { return 0; }
    QAccessibleInterface* child(int _index) const override { return nullptr; }
    int indexOfChild(const QAccessibleInterface* _child) const override { return -1; }
    QRect rect() const override;
    QString text(QAccessible::Text _type) const override;
    QAccessible::Role role() const override { return QAccessible::Role::Button; }
    QAccessible::State state() const override { return {}; }

private:
    CancelReaction* cancel_;
};

}
