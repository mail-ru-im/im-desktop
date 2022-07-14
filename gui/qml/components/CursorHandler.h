#pragma once

class CursorHandler : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(Qt::CursorShape cursorShape READ cursorShape WRITE setCursorShape NOTIFY cursorShapeChanged)

public:
    CursorHandler(QQuickItem* _parent = nullptr);
    Qt::CursorShape cursorShape() const;
    void setCursorShape(Qt::CursorShape _cursorShape);

Q_SIGNALS:
    void cursorShapeChanged();

private Q_SLOTS:
    void updateCursor();

private:
    Qt::CursorShape cursorShape_ = {};
};
