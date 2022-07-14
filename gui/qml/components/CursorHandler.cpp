#include "stdafx.h"
#include "CursorHandler.h"

CursorHandler::CursorHandler(QQuickItem* _parent)
    : QQuickItem(_parent)
{
    connect(this, &CursorHandler::cursorShapeChanged, this, &CursorHandler::updateCursor);
}

Qt::CursorShape CursorHandler::cursorShape() const
{
    return cursorShape_;
}

void CursorHandler::setCursorShape(Qt::CursorShape _cursorShape)
{
    if (_cursorShape != cursorShape_)
    {
        cursorShape_ = _cursorShape;
        Q_EMIT cursorShapeChanged();
    }
}

void CursorHandler::updateCursor()
{
    setCursor(cursorShape());
}
