#pragma once

class WheelInverter : public QQuickItem
{
    Q_OBJECT

public:
    WheelInverter(QQuickItem* _parent = nullptr);

protected:
    void wheelEvent(QWheelEvent* _event) override;
};
