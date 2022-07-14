#include "stdafx.h"
#include "ShapedWidget.h"

namespace Ui
{
    class ShapedWidgetPrivate
    {
    public:
        ShapedWidget* q = nullptr;
        ShapedWidget::MaskingOptions options_ = ShapedWidget::TrackChildren | ShapedWidget::IgnoreFrame;

        ShapedWidgetPrivate(ShapedWidget* _q) : q(_q) {}

        QRegion frameMask() const
        {
            QRegion mask = q->rect();
            const int w = q->frameWidth();
            mask -= q->rect().adjusted(w, w, -w, -w);
            return mask;
        }

        QRegion regionMask() const
        {
            QRegion mask;
            if (!(options_ & ShapedWidget::IgnoreFrame) && q->frameWidth() != 0)
                mask = frameMask();

            const QList<QWidget*> children = q->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
            for (const auto w : children)
            {
                if (!w->isVisible())
                    continue;

                mask += widgetRegion(w);
            }
            return mask;
        }

        QRegion appendRegion(QWidget* _widget, const QRegion& _region) const
        {
            QRegion region = _region;
            return (region += widgetRegion(_widget));
        }

        QRegion removeRegion(QWidget* _widget, const QRegion& _region) const
        {
            QRegion region = _region;
            return (region -= widgetRegion(_widget));
        }

        QRegion widgetRegion(QWidget* _widget) const
        {
            QRegion r = _widget->mask();
            if ((options_ & ShapedWidget::IgnoreMasks) || r.isEmpty())
                return _widget->geometry();

            r.translate(_widget->x(), _widget->y());
            return r;
        }
    };

    ShapedWidget::ShapedWidget(QWidget* _parent, Qt::WindowFlags _flags)
        : QFrame(_parent, _flags)
        , d(std::make_unique<ShapedWidgetPrivate>(this))
    {
    }

    ShapedWidget::~ShapedWidget() = default;

    void ShapedWidget::setOptions(ShapedWidget::MaskingOptions _options)
    {
        if (d->options_ == _options)
            return;

        d->options_ = _options;
        updateMask();
    }

    ShapedWidget::MaskingOptions ShapedWidget::options() const
    {
        return d->options_;
    }

    bool ShapedWidget::eventFilter(QObject* _watched, QEvent* _event)
    {
        if ((d->options_ & TrackChildren) && _watched->isWidgetType() && _watched->parent() == this)
        {
            QWidget* widget = static_cast<QWidget*>(_watched);
            switch (_event->type())
            {
            case QEvent::ParentChange:
                if (!isAncestorOf(widget)) // we are not an ancestor of widget anymore
                    widget->removeEventFilter(this);
                break;
            case QEvent::Show:
                setMask(d->appendRegion(widget, mask()));
                break;
            case QEvent::Hide:
                setMask(d->removeRegion(widget, mask()));
                break;
            case QEvent::Move:
            case QEvent::Resize:
                _watched->event(_event);
                setMask(d->regionMask());
                return true;
            default:
                break;
            }
        }
        return QWidget::eventFilter(_watched, _event);
    }

    void ShapedWidget::paintEvent(QPaintEvent*)
    {
        if (frameShape() == QFrame::NoFrame)
            return;

        QStyleOptionFrame opt;
        initStyleOption(&opt);

        QStylePainter painter(this);
        painter.drawPrimitive(QStyle::PE_Frame, opt);
    }

    void ShapedWidget::resizeEvent(QResizeEvent* _event)
    {
        QFrame::resizeEvent(_event);
        updateMask();
    }

    void ShapedWidget::moveEvent(QMoveEvent* _event)
    {
        QFrame::moveEvent(_event);
        updateMask();
    }

    void ShapedWidget::childEvent(QChildEvent* _event)
    {
        QObject* object = _event->child();
        if ((d->options_ & TrackChildren) &&
            _event->added() &&
            object->isWidgetType() &&
            object->parent() == this)
        {
            object->removeEventFilter(this);
            object->installEventFilter(this);
        }
    }

    void ShapedWidget::updateMask()
    {
        setMask(d->regionMask());
    }

}



