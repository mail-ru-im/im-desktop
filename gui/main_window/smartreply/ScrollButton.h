#pragma once

#include "controls/ClickWidget.h"

namespace Ui
{
    class ScrollButton : public ClickableWidget
    {
        Q_OBJECT

    public:
        enum ButtonDirection
        {
            left,
            right,
        };

        ScrollButton(QWidget* _parent, const ButtonDirection _dir);

        void setBackgroundVisible(const bool _visible);

    protected:
        void paintEvent(QPaintEvent* _e) override;

    private:
        QPixmap getIcon() const;
        QColor getBubbleColor() const;

    private:
        ButtonDirection dir_;
        bool withBackground_;
        QPainterPath bubble_;
    };
}