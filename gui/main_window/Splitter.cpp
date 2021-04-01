#include "stdafx.h"

#include "Splitter.h"

#include "../styles/ThemeParameters.h"
#include "../utils/utils.h"

namespace Ui
{
    Splitter::Splitter(Qt::Orientation _o, QWidget* _parent)
        : QSplitter(_o, _parent)
    {
    }

    Splitter::~Splitter() = default;

    QSplitterHandle* Splitter::createHandle()
    {
        return new SplitterHandle(orientation(), this);
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

    SplitterHandle::~SplitterHandle() = default;

    void SplitterHandle::updateStyle()
    {
        Utils::updateBgColor(this, Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT));
    }

    void SplitterHandle::mouseMoveEvent(QMouseEvent* _event)
    {
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
        QSplitterHandle::mouseMoveEvent(_event);
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
