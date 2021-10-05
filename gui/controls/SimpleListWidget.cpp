#include "stdafx.h"

#include "SimpleListWidget.h"

#include "../utils/utils.h"

#include "../fonts.h"

namespace Ui
{
    SimpleListItem::SimpleListItem(QWidget* _parent)
        : QWidget(_parent)
        , hoverStateCursor_(Qt::PointingHandCursor)
    {
        setAttribute(Qt::WA_Hover);
        setMouseTracking(true);
    }

    SimpleListItem::~SimpleListItem()
    {
        unsetCursor();
    }

    void SimpleListItem::setSelected(bool)
    {
    }

    bool SimpleListItem::isSelected() const
    {
        return false;
    }

    void SimpleListItem::setCompactMode(bool)
    {
    }

    bool SimpleListItem::isCompactMode() const
    {
        return false;
    }

    void SimpleListItem::setPressed(bool _isPressed)
    {
        if (std::exchange(isPressed_, _isPressed) != _isPressed)
        {
            Q_EMIT pressChanged(isPressed_);
            update();
        }
    }

    bool SimpleListItem::isPressed() const noexcept
    {
        return isPressed_;
    }

    void SimpleListItem::setHovered(bool _isHovered)
    {
        if (std::exchange(isHovered_, _isHovered) != _isHovered)
        {
            setCursor(isHovered_ ? getHoverStateCursor() : Qt::ArrowCursor);
            Q_EMIT hoverChanged(isHovered_);
            update();
        }
    }

    bool SimpleListItem::isHovered() const noexcept
    {
        return isHovered_;
    }

    void SimpleListItem::setHoverStateCursor(Qt::CursorShape _value)
    {
        if (std::exchange(hoverStateCursor_, _value) != _value)
        {
            if (isHovered() || isPressed())
                update();
        }
    }

    Qt::CursorShape SimpleListItem::getHoverStateCursor() const noexcept
    {
        return hoverStateCursor_;
    }

    void SimpleListItem::enterEvent(QEvent* _event)
    {
        setHovered(true);
        QWidget::enterEvent(_event);
    }

    void SimpleListItem::leaveEvent(QEvent* _event)
    {
        setHovered(false);
        QWidget::leaveEvent(_event);
    }

    SimpleListWidget::SimpleListWidget(Qt::Orientation _o, QWidget* _parent)
        : QWidget(_parent)
    {
        if (_o == Qt::Horizontal)
            setLayout(Utils::emptyHLayout());
        else
            setLayout(Utils::emptyVLayout());
    }

    SimpleListWidget::~SimpleListWidget() = default;

    int SimpleListWidget::addItem(SimpleListItem* _tab)
    {
        items_.push_back(_tab);
        layout()->addWidget(_tab);
        return int(items_.size() - 1);
    }

    int SimpleListWidget::addItem(SimpleListItem* _tab, int _index)
    {
        if (auto boxLayout = qobject_cast<QBoxLayout*>(layout()))
        {
            if (_index >= 0 && static_cast<size_t>(_index) <= items_.size())
                items_.insert(std::next(items_.cbegin(), _index), _tab);
            else
                items_.push_back(_tab);

            boxLayout->insertWidget(_index, _tab);
            return _index;
        }
        return -1;
    }

    void SimpleListWidget::setCurrentIndex(int _index)
    {
        if (_index == currentIndex_)
            return;

        if (isValidIndex(_index))
        {
            clearSelection();
            currentIndex_ = _index;
            items_[currentIndex_]->setSelected(true);
        }
        else
        {
            currentIndex_ = -1;
            qWarning("ListWidget: invalid index");
        }
    }

    int SimpleListWidget::getCurrentIndex() const noexcept
    {
        return currentIndex_;
    }

    SimpleListItem* SimpleListWidget::itemAt(int _index) const
    {
        if (isValidIndex(_index))
            return items_[_index];
        return nullptr;
    }

    void SimpleListWidget::removeItemAt(int _index)
    {
        if (auto item = itemAt(_index))
        {
            item->hide();
            layout()->removeWidget(item);
            item->deleteLater();
            items_.erase(std::next(items_.begin(), _index));
        }
    }

    int SimpleListWidget::count() const noexcept
    {
        return int(items_.size());
    }

    void SimpleListWidget::clear()
    {
        while (auto item = layout()->takeAt(0))
        {
            item->widget()->deleteLater();
            delete item;
        }

        items_.clear();
    }

    void SimpleListWidget::clearSelection()
    {
        currentIndex_ = -1;

        for (auto item : items_)
            item->setSelected(false);
    }

    void SimpleListWidget::setSpacing(const int _spacing)
    {
        layout()->setSpacing(_spacing);
    }

    void SimpleListWidget::updateHoverState()
    {
        const auto pos = mapFromGlobal(QCursor::pos());
        for (auto item : items_)
            item->setHovered(item->geometry().contains(pos));
    }

    bool SimpleListWidget::isValidIndex(int _index) const noexcept
    {
        return _index >= 0 && _index < int(items_.size());
    }

    void SimpleListWidget::mousePressEvent(QMouseEvent* _event)
    {
        const auto itemIdx = indexOf([pos = _event->pos()](auto item)
        {
            return item->isVisible() && item->geometry().contains(pos);
        });
        if (auto item = itemAt(itemIdx))
        {
            const auto accepted = item->acceptedButtons();
            if (std::any_of(accepted.begin(), accepted.end(), [btn = _event->button()](auto b) { return b == btn; }))
                pressedIndex_ = { itemIdx,  _event->button() };
        }

        updatePressedState();

        QWidget::mousePressEvent(_event);
    }

    void SimpleListWidget::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (auto item = itemAt(pressedIndex_.index_))
        {
            if (item->isVisible() && item->geometry().contains(_event->pos()))
            {
                if (const auto btn = pressedIndex_.button_; btn == _event->button())
                {
                    if (btn == Qt::LeftButton)
                        Q_EMIT clicked(pressedIndex_.index_, QPrivateSignal());
                    else if (btn == Qt::RightButton)
                        Q_EMIT rightClicked(pressedIndex_.index_, QPrivateSignal());
                }
            }
        }

        pressedIndex_ = PressedIndex();
        updatePressedState();

        QWidget::mouseReleaseEvent(_event);
    }

    void SimpleListWidget::showEvent(QShowEvent* _event)
    {
        Q_EMIT shown(QPrivateSignal());
    }

    void SimpleListWidget::updatePressedState()
    {
        for (size_t i = 0; i < items_.size(); ++i)
            items_[i]->setPressed(int(i) == pressedIndex_.index_);
    }
}