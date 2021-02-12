#include "CursorEventFilter.h"

namespace Utils
{

class CursorEventFilterPrivate
{
public:
    QCursor cachedCursor_;
    QPointer<QWidget> widget_;
    QTimer* timer_;
    int delay_;
    bool enabled_;
    bool hidden_;
    bool custom_;

    CursorEventFilterPrivate(CursorEventFilter* _q)
        : widget_(nullptr)
        , timer_(new QTimer(_q))
        , delay_(5000)
        , enabled_(true)
        , hidden_(false)
        , custom_(false)
    {
        timer_->setSingleShot(true);
    }

    void showCursor()
    {
        timer_->stop();
        if (hidden_)
        {
            hidden_ = false;
            if (widget_)
            {
                if (widget_->cursor().shape() != Qt::BlankCursor) // someone already has changed the cursor shape
                    return;

                if (custom_)
                    widget_->setCursor(cachedCursor_);
                else
                    widget_->unsetCursor();
            }
        }

        if (widget_)
        {
            timer_->setSingleShot(true);
            timer_->start(delay_);
        }
    }

    void hideCursor()
    {
        if (hidden_)
            return;

        hidden_ = true;
        if (widget_)
        {
            custom_ = widget_->testAttribute(Qt::WA_SetCursor);
            if (custom_)
                cachedCursor_ = widget_->cursor();

            widget_->setCursor(Qt::BlankCursor);
        }
    }
};


CursorEventFilter::CursorEventFilter(QObject* _parent)
    : QObject(_parent)
    , d(new CursorEventFilterPrivate(this))
{
    connect(d->timer_, &QTimer::timeout, this, &CursorEventFilter::hideCursor);
}

CursorEventFilter::~CursorEventFilter() = default;

void CursorEventFilter::setWidget(QWidget* _widget)
{
    if (d->widget_ == _widget)
        return;

    if (d->widget_ != nullptr)
        d->widget_->removeEventFilter(this);

    if (auto area = qobject_cast<QAbstractScrollArea*>(d->widget_))
        d->widget_ = area->viewport();
    else
        d->widget_ = _widget;

    if (d->widget_)
    {
        d->widget_->installEventFilter(this);
        d->custom_ = d->widget_->testAttribute(Qt::WA_SetCursor);
        if (d->custom_)
            d->cachedCursor_ = d->widget_->cursor();
    }
}

QWidget *CursorEventFilter::widget() const
{
    return d->widget_;
}

void CursorEventFilter::setEnabled(bool _on)
{
    d->enabled_ = _on;
    if (!d->enabled_)
    {
        d->showCursor();
        d->timer_->stop();
    }
}

bool CursorEventFilter::isEnabled() const
{
    return d->enabled_;
}

void CursorEventFilter::setDelay(int _msec)
{
    d->delay_ = _msec;
}

int CursorEventFilter::delay() const
{
    return d->delay_;
}

void CursorEventFilter::showCursor()
{
    d->showCursor();
}

void CursorEventFilter::hideCursor()
{
    d->hideCursor();
}

bool CursorEventFilter::eventFilter(QObject* _watched, QEvent* _event)
{
    if (!d->enabled_)
        return QObject::eventFilter(_watched, _event);

    if (d->widget_ != _watched)
        return QObject::eventFilter(_watched, _event);

    switch(_event->type())
    {
    case QEvent::Leave:
    case QEvent::FocusOut:
    case QEvent::WindowDeactivate:
        showCursor();
        break;
    case QEvent::KeyPress:
    case QEvent::Enter:
    case QEvent::FocusIn:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::Show:
    case QEvent::Hide:
    case QEvent::Wheel:
        showCursor();
        break;
    case QEvent::CursorChange:
        Q_EMIT cursorChanged(d->widget_->cursor().shape() != Qt::BlankCursor);
        break;
    default:
        break;
    }
    return QObject::eventFilter(_watched, _event);
}

} // end namespace GalleryUtils
