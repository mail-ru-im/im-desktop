#include "WindowController.h"
#include "styles/ThemeParameters.h"
#include "../main_window/contact_list/Common.h"

namespace Ui
{

    RubberBandWidget::RubberBandWidget(QWidget* _parent)
        : QRubberBand(QRubberBand::Rectangle)
    {
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAutoFillBackground(false);
    }

    void RubberBandWidget::paintEvent(QPaintEvent* _event)
    {
        QPainter painter(this);
        painter.setOpacity(0.6);
        painter.fillRect(rect().adjusted(15, 15, -15, -15), Styling::getParameters().getColor(Styling::StyleVariable::GHOST_PERCEPTIBLE));
    }

    void RubberBandWidget::resizeEvent(QResizeEvent* _event) { return QWidget::resizeEvent(_event); }
    void RubberBandWidget::moveEvent(QMoveEvent* _event) { return QWidget::moveEvent(_event); }

    class WindowControllerPrivate
    {
    public:
        QPoint dragPos_;
        QPointer<QWidget> target_;
        QPointer<QWidget> watched_;
        QObjectUniquePtr<QRubberBand> rubberBand_ = nullptr;
        WindowController::ResizeMode resizeMode_ = WindowController::ContinuousResize;
        WindowController::Options options_ = WindowController::Resizing;
        Qt::Edges mousePress_ = Qt::Edges{ 0 };
        Qt::Edges mouseMove_ = Qt::Edges{ 0 };
        int borderWidth_ = 5;
        bool cursorChanged_ = false;
        bool leftButtonPressed_ = false;
        bool dragStarted_ = false;
        bool enabled_ = true;

        void calculateCursorPosition(const QPoint& _pos, const QRect& _frameRect, Qt::Edges& _edge)
        {
            const int xLeft = _frameRect.left();
            const int xRight = _frameRect.right();
            const int yTop = _frameRect.top();
            const int yBottom = _frameRect.bottom();

            const bool onLeft = _pos.x() >= xLeft - borderWidth_ && _pos.x() <= xLeft + borderWidth_ &&
                _pos.y() <= yBottom - borderWidth_ && _pos.y() >= yTop + borderWidth_;

            const bool onRight = _pos.x() >= xRight - borderWidth_ && _pos.x() <= xRight &&
                _pos.y() >= yTop + borderWidth_ && _pos.y() <= yBottom - borderWidth_;

            const bool onBottom = _pos.x() >= xLeft + borderWidth_ && _pos.x() <= xRight - borderWidth_ &&
                _pos.y() >= yBottom - borderWidth_ && _pos.y() <= yBottom;

            const bool onTop = _pos.x() >= xLeft + borderWidth_ && _pos.x() <= xRight - borderWidth_ &&
                _pos.y() >= yTop && _pos.y() <= yTop + borderWidth_;

            const bool onBottomLeft = _pos.x() <= xLeft + borderWidth_ && _pos.x() >= xLeft &&
                _pos.y() <= yBottom && _pos.y() >= yBottom - borderWidth_;

            const bool onBottomRight = _pos.x() >= xRight - borderWidth_ && _pos.x() <= xRight &&
                _pos.y() >= yBottom - borderWidth_ && _pos.y() <= yBottom;

            const bool onTopRight = _pos.x() >= xRight - borderWidth_ && _pos.x() <= xRight &&
                _pos.y() >= yTop && _pos.y() <= yTop + borderWidth_;

            const bool onTopLeft = _pos.x() >= xLeft && _pos.x() <= xLeft + borderWidth_ &&
                _pos.y() >= yTop && _pos.y() <= yTop + borderWidth_;

            if (onLeft)
                _edge = Qt::LeftEdge;
            else if (onRight)
                _edge = Qt::RightEdge;
            else if (onBottom)
                _edge = Qt::BottomEdge;
            else if (onTop)
                _edge = Qt::TopEdge;
            else if (onBottomLeft)
                _edge = Qt::BottomEdge | Qt::LeftEdge;
            else if (onBottomRight)
                _edge = Qt::BottomEdge | Qt::RightEdge;
            else if (onTopRight)
                _edge = Qt::TopEdge | Qt::RightEdge;
            else if (onTopLeft)
                _edge = Qt::TopEdge | Qt::LeftEdge;
            else
                _edge = Qt::Edges{};
        }

        QRect evaluateGeometry(const QPoint& _globalPos)
        {
            int left = target_->geometry().left();
            int top = target_->geometry().top();
            int right = target_->geometry().right();
            int bottom = target_->geometry().bottom();
            switch (mousePress_)
            {
            case Qt::TopEdge:
                top = _globalPos.y();
                break;
            case Qt::BottomEdge:
                bottom = _globalPos.y();
                break;
            case Qt::LeftEdge:
                left = _globalPos.x();
                break;
            case Qt::RightEdge:
                right = _globalPos.x();
                break;
            case Qt::TopEdge | Qt::LeftEdge:
                top = _globalPos.y();
                left = _globalPos.x();
                break;
            case Qt::TopEdge | Qt::RightEdge:
                right = _globalPos.x();
                top = _globalPos.y();
                break;
            case Qt::BottomEdge | Qt::LeftEdge:
                bottom = _globalPos.y();
                left = _globalPos.x();
                break;
            case Qt::BottomEdge | Qt::RightEdge:
                bottom = _globalPos.y();
                right = _globalPos.x();
                break;
            }
            return QRect(QPoint(left, top), QPoint(right, bottom));
        }

        QRect adjustRect(const QRect& _rect, QWidget* _widget) const
        {
            int left = _rect.left();
            int top = _rect.top();
            const int right = _rect.right();
            const int bottom = _rect.bottom();

            if (_rect.width() < _widget->minimumWidth())
                left = _widget->geometry().x();

            if (_rect.height() < _widget->minimumHeight())
                top = _widget->geometry().y();

            return QRect(QPoint(left, top), QPoint(right, bottom));
        }
    };


    WindowController::WindowController(QObject* _parent)
        : QObject(_parent)
        , d(std::make_unique<WindowControllerPrivate>())
    {
        d->rubberBand_.reset(createRubberBand());
    }

    WindowController::~WindowController() = default;

    void WindowController::setWidget(QWidget* _target, QWidget* _watched)
    {
        if (d->target_ == _target && d->watched_ == _watched)
            return;

        if (_watched == nullptr)
            _watched = _target;

        if (d->watched_)
            d->watched_->removeEventFilter(this);

        d->target_ = nullptr;
        if (!_target)
            return;

        d->watched_ = _watched;
        if (d->watched_)
            d->watched_->installEventFilter(this);

        if (!(_target->windowFlags() & Qt::FramelessWindowHint))
            d->options_ &= ~Resizing;

        d->target_ = _target;
        d->target_->setMouseTracking(true);
        d->target_->setAttribute(Qt::WA_Hover);
        d->rubberBand_->setMinimumSize(d->target_->minimumSize());
    }

    QWidget* WindowController::widget() const
    {
        return d->target_;
    }

    void WindowController::setEnabled(bool _on)
    {
        if (d->enabled_ == _on)
            return;
        d->enabled_ = _on;
        if (!d->enabled_)
            d->rubberBand_->hide();
        Q_EMIT enabledChanged(d->enabled_, QPrivateSignal{});
    }

    bool WindowController::isEnabled() const
    {
        return d->enabled_;
    }

    void WindowController::setOptions(WindowController::Options _options)
    {
        if (d->options_ == _options)
            return;

        d->options_ = _options;
        if (d->target_ && (d->options_ & RubberBand))
            d->rubberBand_->setMinimumSize(d->target_->minimumSize());

        if (d->target_ && !(d->target_->windowFlags() & Qt::FramelessWindowHint))
            d->options_ &= ~Resizing;

        Q_EMIT optionsChanged(d->options_, QPrivateSignal{});
    }

    WindowController::Options WindowController::options() const
    {
        return d->options_;
    }

    void WindowController::setResizeMode(ResizeMode _mode)
    {
        d->resizeMode_ = _mode;
    }

    WindowController::ResizeMode WindowController::resizeMode() const
    {
        return d->resizeMode_;
    }

    void WindowController::setBorderWidth(int _width)
    {
        if (_width == d->borderWidth_)
            return;

        d->borderWidth_ = _width;
        Q_EMIT borderWidthChanged(d->borderWidth_, QPrivateSignal{});
    }

    int WindowController::borderWidth() const
    {
        return d->borderWidth_;
    }

    bool WindowController::eventFilter(QObject* _watched, QEvent* _event)
    {
        if (!d->enabled_ || _watched != d->watched_)
            return QObject::eventFilter(_watched, _event);

        switch (_event->type())
        {
        case QEvent::HoverMove:
            return mouseHover(static_cast<QHoverEvent*>(_event));
        case QEvent::Leave:
            return mouseLeave(_event);
        case QEvent::MouseButtonPress:
            return mousePress(static_cast<QMouseEvent*>(_event));
        case QEvent::MouseMove:
            return mouseMove(static_cast<QMouseEvent*>(_event));
        case QEvent::MouseButtonRelease:
            return mouseRelease(static_cast<QMouseEvent*>(_event));
        case QEvent::Hide:
            d->leftButtonPressed_ = false;
            d->dragStarted_ = false;
            d->rubberBand_->hide();
            d->target_->unsetCursor();
            break;
        default:
            break;
        }
        return QObject::eventFilter(_watched, _event);
    }

    bool WindowController::mouseHover(QHoverEvent* _event)
    {
        if (d->target_)
            updateCursorShape(d->target_->mapToGlobal(_event->pos()));
        return false;
    }

    bool WindowController::mouseLeave(QEvent*)
    {
        if (!d->target_)
            return false;

        if (!d->leftButtonPressed_)
            d->target_->unsetCursor();
        return true;
    }

    bool WindowController::mousePress(QMouseEvent* _event)
    {
        if (!d->target_ || (_event->button() != Qt::LeftButton))
            return false;

        d->leftButtonPressed_ = true;
        d->calculateCursorPosition(_event->globalPos(), d->target_->frameGeometry(), d->mousePress_);
        if (d->options_ & Resizing && d->resizeMode_ == OnDemandResize)
            d->rubberBand_->setGeometry(d->target_->geometry());

        if (d->mousePress_ != Qt::Edges{})
        {
            d->rubberBand_->setGeometry(d->target_->geometry());
            if ((d->options_ & RubberBand) && !d->rubberBand_->isVisible())
                d->rubberBand_->show();
        }
        if ((d->options_ & Dragging) && d->target_->rect().marginsRemoved(QMargins(borderWidth(), borderWidth(), borderWidth(), borderWidth())).contains(_event->pos()))
        {
            d->dragStarted_ = true;
            d->dragPos_ = _event->globalPos();
        }

        if (!(d->options_ & Resizing))
            return ((d->options_ & RequireMouseEvents) ? false : d->dragStarted_);

        return d->mousePress_ != Qt::Edges{};
    }

    bool WindowController::mouseRelease(QMouseEvent* _event)
    {
        if (!d->target_)
            return false;

        if (d->leftButtonPressed_ && (d->options_ & Resizing) && d->resizeMode_ == OnDemandResize)
            d->target_->setGeometry(d->rubberBand_->geometry());

        d->leftButtonPressed_ = false;
        d->dragStarted_ = false;
        d->rubberBand_->hide();
        return false;
    }

    bool WindowController::mouseMove(QMouseEvent* _event)
    {
        if (!d->target_)
            return false;

        updateCursorShape(_event->globalPos());
        if (!d->leftButtonPressed_)
            return false;

        if (d->dragStarted_)
        {
            d->target_->move(d->target_->frameGeometry().topLeft() + (_event->globalPos() - d->dragPos_));
            d->dragPos_ = _event->globalPos();
        }

        if (!(d->options_ & Resizing))
            return ((d->options_ & RequireMouseEvents) ? false : d->dragStarted_);

        if (d->mousePress_ == Qt::Edges{})
            return false;

        const QRect rc = d->evaluateGeometry(_event->globalPos());
        if (d->rubberBand_->isVisible())
            d->rubberBand_->setGeometry(d->adjustRect(rc, d->rubberBand_.get()));

        if (d->resizeMode_ == ContinuousResize)
            d->target_->setGeometry(d->adjustRect(rc, d->target_));

        _event->ignore();
        return true;
    }

    void WindowController::updateCursorShape(const QPoint& _pos)
    {
        if (!d->target_)
            return;

        if (d->target_->isFullScreen() || d->target_->isMaximized())
        {
            if (d->cursorChanged_)
                d->target_->unsetCursor();
            return;
        }

        if (d->leftButtonPressed_ || d->borderWidth_ == 0)
            return;

        d->calculateCursorPosition(_pos, d->target_->frameGeometry(), d->mouseMove_);
        d->cursorChanged_ = true;
        if ((d->mouseMove_ == Qt::TopEdge) || (d->mouseMove_ == Qt::BottomEdge))
        {
            d->target_->setCursor(Qt::SizeVerCursor);
        }
        else if ((d->mouseMove_ == Qt::LeftEdge) || (d->mouseMove_ == Qt::RightEdge))
        {
            d->target_->setCursor(Qt::SizeHorCursor);
        }
        else if ((d->mouseMove_ == (Qt::TopEdge | Qt::LeftEdge)) || (d->mouseMove_ == (Qt::BottomEdge | Qt::RightEdge)))
        {
            d->target_->setCursor(Qt::SizeFDiagCursor);
        }
        else if ((d->mouseMove_ == (Qt::TopEdge | Qt::RightEdge)) || (d->mouseMove_ == (Qt::BottomEdge | Qt::LeftEdge)))
        {
            d->target_->setCursor(Qt::SizeBDiagCursor);
        }
        else if (d->cursorChanged_)
        {
            d->target_->unsetCursor();
            d->cursorChanged_ = false;
        }
    }

    QRubberBand* WindowController::createRubberBand() const
    {
        RubberBandWidget* rubberBand = new RubberBandWidget(nullptr);
        rubberBand->setContentsMargins(d->borderWidth_, d->borderWidth_, d->borderWidth_, d->borderWidth_);
        return rubberBand;
    }
}
