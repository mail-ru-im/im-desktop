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
        setAttribute(Qt::WA_Hover, true);
        setMouseTracking(true);
    }

    SimpleListItem::~SimpleListItem()
    {
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

    void SimpleListItem::setPressed(const bool _isPressed)
    {
        if (isPressed_ != _isPressed)
        {
            isPressed_ = _isPressed;
            update();
        }
    }

    bool SimpleListItem::isPressed() const noexcept
    {
        return isPressed_;
    }

    bool SimpleListItem::isHovered() const noexcept
    {
        return isHovered_;
    }

    void SimpleListItem::setHoverStateCursor(Qt::CursorShape _value)
    {
        if (hoverStateCursor_ != _value)
        {
            hoverStateCursor_ = _value;

            if (isHovered())
                update();
        }
    }

    Qt::CursorShape SimpleListItem::getHoverStateCursor() const noexcept
    {
        return hoverStateCursor_;
    }

    bool SimpleListItem::event(QEvent* _event)
    {
        switch (_event->type())
        {
        case QEvent::HoverEnter:
        {
            isHovered_ = true;
            setCursor(getHoverStateCursor());
            emit hoverChanged(isHovered_, QPrivateSignal());
        }
        break;
        case QEvent::HoverLeave:
        {
            isHovered_ = false;
            setCursor(Qt::ArrowCursor);
            emit hoverChanged(isHovered_, QPrivateSignal());
        }
        break;
        default:
            break;
        }

        return QWidget::event(_event);
    }

    SimpleListWidget::SimpleListWidget(Qt::Orientation _o, QWidget* _parent)
        : QWidget(_parent)
    {
        if (_o == Qt::Horizontal)
            setLayout(Utils::emptyHLayout());
        else
            setLayout(Utils::emptyVLayout());
    }

    SimpleListWidget::~SimpleListWidget()
    {
    }

    int SimpleListWidget::addItem(SimpleListItem* _tab)
    {
        items_.push_back(_tab);
        layout()->addWidget(_tab);
        return int(items_.size() - 1);
    }

    void SimpleListWidget::setCurrentIndex(int _index)
    {
        if (_index != currentIndex_)
        {
            if (isValidIndex(currentIndex_))
                items_[currentIndex_]->setSelected(false);

            if (isValidIndex(_index))
            {
                currentIndex_ = _index;
                items_[currentIndex_]->setSelected(true);
            }
            else
            {
                currentIndex_ = -1;
                qWarning("ListWidget: invalid index");
            }
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

    bool SimpleListWidget::isValidIndex(int _index) const noexcept
    {
        return _index >= 0 && _index < int(items_.size());
    }

    void SimpleListWidget::mousePressEvent(QMouseEvent* _event)
    {
        const auto pos = _event->pos();
        for (size_t i = 0; i < items_.size(); ++i)
        {
            if (items_[i]->isVisible() && items_[i]->geometry().contains(pos))
            {
                pressedIndex_ = { int(i),  _event->button() };
                break;
            }
        }
        updatePressedState();

        QWidget::mousePressEvent(_event);
    }

    void SimpleListWidget::mouseReleaseEvent(QMouseEvent* _event)
    {
        const auto pos = _event->pos();
        for (size_t i = 0; i < items_.size(); ++i)
        {
            if (items_[i]->isVisible() && items_[i]->geometry().contains(pos))
            {
                if (int(i) == pressedIndex_.index_)
                {
                    if ((pressedIndex_.button_ == Qt::LeftButton) && (_event->button() == Qt::LeftButton))
                        emit clicked(pressedIndex_.index_, QPrivateSignal());
                    else if ((pressedIndex_.button_ == Qt::RightButton) && (_event->button() == Qt::RightButton))
                        emit rightClicked(pressedIndex_.index_, QPrivateSignal());
                }

                break;
            }
        }

        pressedIndex_ = PressedIndex();
        updatePressedState();

        QWidget::mouseReleaseEvent(_event);
    }

    void SimpleListWidget::showEvent(QShowEvent* _event)
    {
        emit shown(QPrivateSignal());
    }

    void SimpleListWidget::updatePressedState()
    {
        for (size_t i = 0; i < items_.size(); ++i)
            items_[i]->setPressed(int(i) == pressedIndex_.index_);
    }
}