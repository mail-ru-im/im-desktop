#include "stdafx.h"

#include "utils/utils.h"
#include "DraggableList.h"

namespace Ui
{

//////////////////////////////////////////////////////////////////////////
// DraggableList_p
//////////////////////////////////////////////////////////////////////////

class DraggableList_p
{
public:
    QBoxLayout* layout_;
    QPointer<QWidget> placeholder_;
    QPointer<QWidget> dragItem_;
};

//////////////////////////////////////////////////////////////////////////
// DraggableList
//////////////////////////////////////////////////////////////////////////

DraggableList::DraggableList(QWidget* _parent)
    : QWidget(_parent),
      d(std::make_unique<DraggableList_p>())
{
    d->layout_ = Utils::emptyVLayout(this);
}

DraggableList::~DraggableList()
{

}

void DraggableList::addItem(DraggableItem* _item)
{
    d->layout_->addWidget(_item);
    connect(_item, &DraggableItem::dragStarted, this, &DraggableList::onDragStarted);
    connect(_item, &DraggableItem::dragStopped, this, &DraggableList::onDragStopped);
    connect(_item, &DraggableItem::dragPositionChanged, this, &DraggableList::onDragPositionChanged);

    updateTabOrder();
}

void DraggableList::removeItem(DraggableItem* _item)
{
    d->layout_->removeWidget(_item);
}

QWidget* DraggableList::itemAt(int _index) const
{
    const auto itemsCount = d->layout_->count();
    if (_index >= 0 && _index < itemsCount)
        return d->layout_->itemAt(_index)->widget();

    return nullptr;
}

int DraggableList::indexOf(QWidget* _item) const
{
    return d->layout_->indexOf(_item);
}

int DraggableList::count() const
{
    return d->layout_->count();
}

QVariantList DraggableList::orderedData() const
{
    const auto itemsCount = d->layout_->count();

    QVariantList result;
    result.reserve(itemsCount);

    for (auto i = 0; i < itemsCount; ++i)
    {
        if (auto item = qobject_cast<DraggableItem*>(d->layout_->itemAt(i)->widget()))
            result.push_back(item->data());
    }

    return result;
}

void DraggableList::onDragStarted()
{
    if (auto w = qobject_cast<QWidget*>(sender()))
    {
        d->placeholder_ = new QWidget(this);
        d->placeholder_->setFixedSize(w->size());

        const auto index = d->layout_->indexOf(w);

        d->layout_->insertWidget(index, d->placeholder_);
        d->layout_->removeWidget(w);
        d->dragItem_ = w;
    }
}

void DraggableList::onDragStopped()
{
    const auto index = d->layout_->indexOf(d->placeholder_);

    d->layout_->insertWidget(index, d->dragItem_);
    d->layout_->removeWidget(d->placeholder_);

    delete d->placeholder_;

    updateTabOrder();
}

void DraggableList::onDragPositionChanged()
{
    if (auto w = qobject_cast<QWidget*>(sender()))
    {
        const auto dragItemRect = w->geometry();
        const auto center = dragItemRect.center();

        QWidget* widgetUnder = nullptr;
        auto index = -1;

        for (auto i = 0; i < d->layout_->count(); ++i)
        {
            auto item = d->layout_->itemAt(i);
            auto itemRect = item->geometry();

            const auto heightDiff = itemRect.height() - dragItemRect.height();

            if (heightDiff > 0)
                itemRect.adjust(0, heightDiff / 2, 0, -heightDiff / 2);

            if (itemRect.contains(center))
            {
                widgetUnder = item->widget();
                index = i;
                break;
            }
        }

        if (widgetUnder && widgetUnder != d->placeholder_)
        {
            d->layout_->removeWidget(d->placeholder_);
            d->layout_->insertWidget(index, d->placeholder_);
        }
    }
}

void DraggableList::updateTabOrder()
{
    auto prev = itemAt(0);

    if (prev && prev->focusProxy())
        prev = prev->focusProxy();

    for (auto i = 1; i < count(); i++)
    {
        auto current = itemAt(i);
        if (current->focusProxy())
            current = current->focusProxy();

        setTabOrder(prev, current);
        prev = current;
    }
}

//////////////////////////////////////////////////////////////////////////
// DraggableItem_p
//////////////////////////////////////////////////////////////////////////

class DraggableItem_p
{
public:
    QPoint pressPos_;

    int offset_ = 0;
    bool drag_ = false;
};

//////////////////////////////////////////////////////////////////////////
// DraggableItem
//////////////////////////////////////////////////////////////////////////

DraggableItem::DraggableItem(QWidget* _parent)
    : QFrame(_parent),
      d(std::make_unique<DraggableItem_p>())
{
}

DraggableItem::~DraggableItem()
{

}

void DraggableItem::onDragMove(const QPoint& _pos)
{
    if (!std::exchange(d->drag_, true))
    {
        Q_EMIT dragStarted();
        raise();
    }

    auto newPos = _pos;
    auto currentPos = pos();

    move(mapToParent(QPoint(currentPos.x(), newPos.y() - d->pressPos_.y())));

    Q_EMIT dragPositionChanged();
}

void DraggableItem::onDragStart(const QPoint& _pos)
{
    d->pressPos_ = _pos;
}

void DraggableItem::onDragStop()
{
    if (std::exchange(d->drag_, false))
        Q_EMIT dragStopped();
}

}
