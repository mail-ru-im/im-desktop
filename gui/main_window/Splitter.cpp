#include "stdafx.h"

#include "private/qsplitter_p.h"

#include "Splitter.h"

#include "../styles/ThemeParameters.h"
#include "../utils/utils.h"

namespace Ui
{
    Splitter::Splitter(Qt::Orientation _o, QWidget* _parent)
        : QSplitter(_o, _parent)
    {
    }

    Splitter::~Splitter()
    {
    }

    QSplitterHandle* Splitter::createHandle()
    {
        return new SplitterHandle(orientation(), this);
    }

    QWidget* Splitter::replaceWidgetImpl(int _index, QWidget * _widget)
    {
        auto d = reinterpret_cast<QSplitterPrivate *>(qGetPtrHelper(d_ptr));
        if (!_widget) {
            qWarning("QSplitter::replaceWidget: Widget can't be null");
            return nullptr;
        }

        if (_index < 0 || _index >= d->list.count()) {
            qWarning("QSplitter::replaceWidget: Index %d out of range", _index);
            return nullptr;
        }

        QSplitterLayoutStruct *s = d->list.at(_index);
        QWidget *current = s->widget;
        if (current == _widget) {
            qWarning("QSplitter::replaceWidget: Trying to replace a widget with itself");
            return nullptr;
        }

        if (_widget->parentWidget() == this) {
            qWarning("QSplitter::replaceWidget: Trying to replace a widget with one of its siblings");
            return nullptr;
        }

        QBoolBlocker b(d->blockChildAdd);

        const QRect geom = current->geometry();
        const bool shouldShow = shouldShowWidget(current);

        s->widget = _widget;
        current->setParent(nullptr);
        _widget->setParent(this);

        // The splitter layout struct's geometry is already set and
        // should not change. Only set the geometry on the new widget
        _widget->setGeometry(geom);
        _widget->lower();
        _widget->setVisible(shouldShow);

        return current;
    }

    bool Splitter::shouldShowWidget(const QWidget* w) const
    {
        return isVisible() && !(w->isHidden() && w->testAttribute(Qt::WA_WState_ExplicitShowHide));
    }

    SplitterHandle::SplitterHandle(Qt::Orientation _o, QSplitter* _parent)
        : QSplitterHandle(_o, _parent)
    {
        setAutoFillBackground(true);
        updateStyle();
    }

    SplitterHandle::~SplitterHandle()
    {
    }

    void SplitterHandle::updateStyle()
    {
        Utils::updateBgColor(this, Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT));
    }

    void SplitterHandle::mouseMoveEvent(QMouseEvent* _event)
    {
        QSplitterHandle::mouseMoveEvent(_event);
        if (auto s = splitter(); s && (_event->buttons() & Qt::LeftButton))
        {
            auto pos = pick(parentWidget()->mapFromGlobal(_event->globalPos())) - mouseOffset_;

            if (s->isRightToLeft() && s->orientation() == Qt::Horizontal)
                pos = s->contentsRect().width() - pos;

            const auto index = s->indexOf(this);

            const auto prevIndex = index > 0 ? index - 1 : index;

            const auto res = pick(s->widget(prevIndex)->geometry().topLeft());

            if (auto splitter = qobject_cast<Splitter*>(s); splitter)
                splitter->movedImpl(pos, res, index);
        }
    }

    void SplitterHandle::mousePressEvent(QMouseEvent* _event)
    {
        if (_event->button() == Qt::LeftButton)
            mouseOffset_ = pick(_event->pos());

        QSplitterHandle::mousePressEvent(_event);
    }

    void SplitterHandle::mouseReleaseEvent(QMouseEvent* _event)
    {
        QSplitterHandle::mouseReleaseEvent(_event);
    }

    void SplitterHandle::paintEvent(QPaintEvent* _event)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    }

    int SplitterHandle::pick(QPoint _point) const noexcept
    {
        return orientation() == Qt::Horizontal ? _point.x() : _point.y();
    }
}
