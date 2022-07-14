#include "stdafx.h"
#include "WheelInverter.h"
#include "utils/utils.h"

WheelInverter::WheelInverter(QQuickItem* _parent)
    : QQuickItem(_parent)
{
}

void WheelInverter::wheelEvent(QWheelEvent* _event)
{
    const auto eventReceiver = parentItem();
    if (const auto delta = _event->angleDelta(); delta.x() == 0 && delta.y() != 0 && eventReceiver && delta.y() % Utils::defaultMouseWheelDelta() == 0)
    {
        const QPoint invertedAngleDelta { delta.y(), delta.x() };
        const auto editedEvent = new QWheelEvent(
            _event->position(), _event->globalPosition(), _event->pixelDelta(), invertedAngleDelta,
            _event->buttons(), _event->modifiers(), _event->phase(), _event->inverted());
        QCoreApplication::postEvent(eventReceiver, editedEvent);
    }
    else
    {
        QQuickItem::wheelEvent(_event);
    }
}
