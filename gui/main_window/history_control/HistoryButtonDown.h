#pragma once

#include "../../namespaces.h"
#include "../../controls/ClickWidget.h"

// Round button in history pane with icon and counter

UI_NS_BEGIN

class HistoryButton final : public ClickableWidget
{
    Q_OBJECT

Q_SIGNALS:
    void sendWheelEvent(QWheelEvent* e);

protected:
    void paintEvent(QPaintEvent *_event) override;
    void wheelEvent(QWheelEvent * _event) override;

public:
    HistoryButton(QWidget* _parent, const QString& _imageName);

    void addCounter(const int _numToAdd);
    void setCounter(const int _numToSet);

    inline void resetCounter() { setCounter(0); }

    int getCounter() const;

    static const int buttonSize = 46;
    static const int bubbleSize = 38;

private:
    int counter_;
    QPixmap icon_;
    QPixmap iconHover_;
};

UI_NS_END