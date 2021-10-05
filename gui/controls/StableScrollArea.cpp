#include "stdafx.h"
#include "StableScrollArea.h"
#include "utils/utils.h"
#include "TransparentScrollBar.h"
#include "main_window/history_control/MessagesScrollbar.h"
#include "styles/ThemeParameters.h"

namespace
{
    int scrollToMargin() noexcept
    {
        return Utils::scale_value(100);
    }

    int scrollBarHandleWidth() noexcept
    {
        return Utils::scale_value(8);
    }

    constexpr auto scrollVisibilityInterval() noexcept
    {
        return std::chrono::milliseconds(1500);
    }
}

namespace Ui
{

class StableScrollArea_p
{
public:
    StableScrollArea_p(StableScrollArea* _q) : q(_q) {}
    int viewportTop()
    {
        if (!widget_)
            return 0;

        return -widget_->pos().y();
    }

    int mapToViewPort(int _y)
    {
        return _y - viewportTop();
    }

    int widgetPosFromViewport(QWidget* _w)
    {
        if (!widget_)
            return 0;

        return _w->mapTo(widget_, {0, 0}).y() - viewportTop() + _w->height();
    }

    int limitWidgetPos(int _y)
    {
        if (!widget_)
            return _y;

        return std::clamp(_y, -widget_->height() + q->height(), 0);
    }

    void limitCurrentPos()
    {
        if (widget_)
            widget_->move({0, limitWidgetPos(widget_->pos().y())});
    }

    void updateWidgetSize()
    {
        if (!widget_)
            return;

        const auto min = widget_->sizeHint();
        const auto vs = q->size();

        q->setUpdatesEnabled(false);
        widget_->resize(vs.expandedTo(min));

        if (anchor_)
        {
            const auto newAnchorPos = widgetPosFromViewport(anchor_);
            if (const auto diff = newAnchorPos - anchorPos_; diff != 0)
            {
                anchorPos_ += diff;
                if (!scrollLocked_)
                    scrollBy(diff);
            }
        }
        limitCurrentPos();
        q->setUpdatesEnabled(true);
        updateScrollbar();
    }

    void updateScrollbar()
    {
        QSignalBlocker sb(scrollbar_);
        auto g = q->geometry();
        g.setLeft(g.right() - scrollBarHandleWidth());
        scrollbar_->setGeometry(g);
        scrollbar_->setRange(0, widget_->height() - g.height());
        scrollbar_->setValue(viewportTop());
        scrollbar_->setPageStep(g.height());
    }

    void scrollBy(int _px) // scroll down by _px
    {
        if (!widget_)
            return;

        const auto y = widget_->pos().y();
        const auto newY = y - _px;
        const auto newYBounded = limitWidgetPos(newY);
        const auto scrolledBy = y - newYBounded;
        anchorPos_ -= scrolledBy;
        QSignalBlocker sb(scrollbar_);
        widget_->move({0, newYBounded});

        updateScrollbar();
        if (widget_->height() > q->height() && !scrollbar_->isVisible())
        {
            scrollbar_->show();
            scrollbar_->raise();
            scrollbarVisibilityTimer_->start();
        }
    }

    void scrollTo(int _y)
    {
        if (!widget_)
            return;

        scrollBy(_y + widget_->pos().y());
    }

    void initScrollBar()
    {
        scrollbar_ = new MessagesScrollbar(q);
        scrollbar_->setVisible(false);
        scrollbarVisibilityTimer_ = new QTimer(q);
        scrollbarVisibilityTimer_->setSingleShot(true);
        scrollbarVisibilityTimer_->setInterval(scrollVisibilityInterval());
        QObject::connect(scrollbarVisibilityTimer_, &QTimer::timeout, q, [this]()
        {
            if (!scrollbar_->isHovered())
                scrollbar_->fadeOut();
        });
        QObject::connect(scrollbar_, &MessagesScrollbar::hoverChanged, q, [this](bool _hovered)
        {
            if (widget_->height() > q->height())
                return;

            if (_hovered)
                scrollbar_->show();
            else if (!scrollbarVisibilityTimer_->isActive())
                scrollbarVisibilityTimer_->start();
        });
        QObject::connect(scrollbar_, &MessagesScrollbar::valueChanged, q, [this](int _value)
        {
            scrollTo(_value);
            Q_EMIT q->scrolled(viewportTop());
        });
    }

    void setScrollLocked(bool _locked)
    {
        scrollLocked_ = _locked;
    }

    QWidget* widget_ = nullptr;
    QWidget* anchor_ = nullptr;
    MessagesScrollbar* scrollbar_ = nullptr;
    QTimer* scrollbarVisibilityTimer_ = nullptr;
    int anchorPos_ = 0;
    StableScrollArea* q;
    bool scrollLocked_ = false;
};

StableScrollArea::StableScrollArea(QWidget* _parent)
    : QWidget(_parent)
    , d(std::make_unique<StableScrollArea_p>(this))
{
    d->initScrollBar();
}

StableScrollArea::~StableScrollArea() = default;

void StableScrollArea::setWidget(QWidget* _w)
{
    if (d->widget_)
        delete d->widget_;

    d->widget_ = _w;
    d->widget_->installEventFilter(this);
    _w->move(0, 0);
    _w->setParent(this);
    _w->resize(_w->sizeHint());
    _w->show();
}

QWidget* StableScrollArea::widget() const
{
    return d->widget_;
}

void StableScrollArea::setAnchor(QWidget* _w)
{
    if (_w && _w != d->anchor_)
        d->anchorPos_ = d->widgetPosFromViewport(_w);

    d->anchor_ = _w;
}

QWidget* StableScrollArea::anchor() const
{
    return d->anchor_;
}

void StableScrollArea::ensureWidgetVisible(QWidget* _w)
{
    const auto y = _w->mapTo(d->widget_, {0, 0}).y();
    d->scrollTo(y - scrollToMargin());
}

void StableScrollArea::ensureVisible(int _y)
{
    const auto top = d->viewportTop();
    const auto bottom = d->viewportTop() + height();
    const auto margin = scrollToMargin();
    if (_y < top + margin || _y > bottom - margin)
        d->scrollTo(_y - margin);
}

int StableScrollArea::widgetPosition() const
{
    return d->viewportTop();
}

void StableScrollArea::setScrollLocked(bool _locked)
{
    d->setScrollLocked(_locked);
}

void StableScrollArea::updateSize()
{
    d->updateWidgetSize();
}

bool StableScrollArea::event(QEvent* _e)
{
    if (_e->type() == QEvent::LayoutRequest)
        d->updateWidgetSize();

    return QWidget::event(_e);
}

bool StableScrollArea::eventFilter(QObject* _o, QEvent* _e)
{
    if (_o == d->widget_ && _e->type() == QEvent::Wheel)
    {
        auto we = static_cast<QWheelEvent*>(_e);
        d->scrollBy(-we->angleDelta().y());
        Q_EMIT scrolled(d->viewportTop());
    }
    return false;
}

void StableScrollArea::resizeEvent(QResizeEvent* _e)
{
    d->updateWidgetSize();
    QWidget::resizeEvent(_e);
}

}
