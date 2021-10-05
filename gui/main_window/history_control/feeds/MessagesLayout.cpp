#include "stdafx.h"
#include "MessagesLayout.h"
#include "../HistoryControlPageItem.h"
#include "../complex_message/ComplexMessageItem.h"

namespace Ui
{

class MessagesLayout_p
{
public:
    MessagesLayout_p(MessagesLayout* _q) : q(_q) {}
    void updateItemsWidth(int _width)
    {
        const auto m = q->contentsMargins();
        _width -= m.left() + m.right();
        for (auto item : items_)
        {
            if (auto w = item->widget(); w && w->width() != _width)
                w->setFixedWidth(_width);
        }
    }

    int updateItemsPositions()
    {
        if (items_.empty())
            return 0;

        const auto layoutMargins = q->contentsMargins();
        const auto geometry = q->geometry();
        auto topOffset = geometry.top() + layoutMargins.top();
        auto spacing = 0;
        for (auto item : items_)
        {
            const auto hint = item->sizeHint();
            auto leftOffset = layoutMargins.left();
            if (const auto w = item->widget())
                leftOffset += w->contentsMargins().left();
            item->setGeometry(QRect({leftOffset, topOffset + spacing}, hint));
            topOffset = item->geometry().bottom() + 1;
            spacing = spacing_;
        }
        return topOffset + layoutMargins.bottom() - geometry.top();
    }

    void setGeometryHelper(const QRect& _rect)
    {
        updateItemsWidth(_rect.width());
        minHeight_ = updateItemsPositions();
        dirty_ = false;
    }

    int calcMinHeight()
    {
        if (!dirty_)
            return minHeight_;

        auto height = 0;
        for (auto item : items_)
            height += item->sizeHint().height();

        auto m = q->contentsMargins();
        if (!items_.empty())
            height += (items_.size() - 1) * spacing_ + m.bottom() + m.top();

        dirty_ = false;

        return height;
    }

    std::deque<QLayoutItem*> items_;
    int minHeight_ = 0;
    int spacing_ = 0;
    bool dirty_ = true;
    MessagesLayout* q;
};

MessagesLayout::MessagesLayout(QWidget* _parent)
    : QLayout(_parent)
    , d(std::make_unique<MessagesLayout_p>(this))
{
    setContentsMargins({});
}

MessagesLayout::~MessagesLayout() = default;

void MessagesLayout::setSpacing(int _spacing)
{
    d->spacing_ = _spacing;
    d->dirty_ = true;
}

void MessagesLayout::clear()
{
    for (auto item : d->items_)
        delete item;

    d->items_.clear();
    d->dirty_ = true;
}

void MessagesLayout::insertSpace(int _height, int _pos)
{
    if (_pos == -1)
        _pos = d->items_.size();

    auto spacer = new QSpacerItem(0, _height, QSizePolicy::Expanding, QSizePolicy::Fixed);
    d->items_.insert(d->items_.begin() + _pos, spacer);
    d->dirty_ = true;
}

void MessagesLayout::insertWidget(QWidget* _w, int _pos)
{
    addChildWidget(_w);
    auto widgetItem = new QWidgetItem(_w);
    if (_pos == -1)
        _pos = d->items_.size();

    d->items_.insert(d->items_.begin() + _pos, widgetItem);
    d->dirty_ = true;
}

void MessagesLayout::replaceWidget(QWidget* _w, int _pos)
{
    if (_pos < 0 || _pos >= count())
        return;

    addChildWidget(_w);
    auto widgetItem = new QWidgetItem(_w);
    delete d->items_[_pos];
    d->items_[_pos] = widgetItem;
    d->dirty_ = true;
}

void MessagesLayout::setGeometry(const QRect& _rect)
{
    QLayout::setGeometry(_rect);
    d->setGeometryHelper(_rect);
}

QSize MessagesLayout::sizeHint() const
{
    if (d->dirty_)
        d->minHeight_ = d->calcMinHeight();

    return { geometry().width(), d->minHeight_ };
}

QSize MessagesLayout::minimumSize() const
{
    if (d->dirty_)
        d->minHeight_ = d->calcMinHeight();

    return {0, d->minHeight_};
}

void MessagesLayout::addItem(QLayoutItem* _item)
{
    d->items_.push_back(_item);
    d->dirty_ = true;
}

QLayoutItem* MessagesLayout::itemAt(int _index) const
{
    if (_index < 0 || _index >= count())
        return nullptr;
    return d->items_[_index];
}

QLayoutItem* MessagesLayout::takeAt(int _index)
{
    auto item = itemAt(_index);
    if (item)
    {
        d->items_.erase(std::next(d->items_.begin(), _index));
        d->dirty_ = true;
    }
    return item;
}

int MessagesLayout::count() const
{
    return d->items_.size();
}

void MessagesLayout::invalidate()
{
    d->dirty_ = true;
    QLayout::invalidate();
}

bool MessagesLayout::tryToEditLastMessage()
{
    for (auto i = d->items_.rbegin(); i != d->items_.rend(); ++i)
    {
        if (auto messageItem = qobject_cast<Ui::HistoryControlPageItem*>((*i)->widget()))
        {
            if (!messageItem->isOutgoing())
                continue;

            if (!messageItem->isEditable())
                return false;

            messageItem->callEditing();
            return true;
        }
    }

    return false;
}

void MessagesLayout::clearSelection()
{
    for (auto i = d->items_.rbegin(); i != d->items_.rend(); ++i)
    {
        if (auto messageItem = qobject_cast<Ui::HistoryControlPageItem*>((*i)->widget()))
            messageItem->clearSelection(false);
    }
}

bool MessagesLayout::selectByPos(const QPoint& _from, const QPoint& _to, const QPoint& _areaFrom, const QPoint& _areaTo)
{
    for (auto i = d->items_.rbegin(); i != d->items_.rend(); ++i)
    {
        auto w = (*i)->widget();
        if (w && w->rect().contains(w->mapFromGlobal(_from)))
        {
            if (auto messageItem = qobject_cast<Ui::HistoryControlPageItem*>(w))
            {
                messageItem->selectByPos(_from, _to, _areaFrom, _areaTo);
                return messageItem->isSelected();
            }
            break;
        }
    }

    return false;
}

Data::FString MessagesLayout::getSelectedText() const
{
    for (auto i = d->items_.rbegin(); i != d->items_.rend(); ++i)
    {
        if (const auto messageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>((*i)->widget()); messageItem && messageItem->isSelected())
            return messageItem->getSelectedText(false, Ui::ComplexMessage::ComplexMessageItem::TextFormat::Raw);
    }

    return {};
}

}
