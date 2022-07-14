#pragma once

#include "controls/CustomButton.h"
#include "CircleHover.h"

namespace Ui
{
    class ButtonWithCircleHover : public CustomButton, CircleHoverAnimated
    {
        Q_OBJECT
    public:
        explicit ButtonWithCircleHover(QWidget* _parent, const QString& _svgName = QString(), const QSize& _iconSize = QSize(), const Styling::ColorParameter& _defaultColor = Styling::ColorParameter());
        ~ButtonWithCircleHover();

        void setCircleHover(std::unique_ptr<CircleHover>&& _) override;
        void enableCircleHover(bool _val) override;

        void setUnderLongPress(bool _val);

        void updateHover();
        void updateHover(bool _hover);
        void setRectExtention(const QMargins& _m);
        bool containsCursorUnder() const;

        void showToolTipForce();

    protected:
        void keyPressEvent(QKeyEvent* _event) override;
        void keyReleaseEvent(QKeyEvent* _event) override;
        void focusInEvent(QFocusEvent* _event) override;
        void focusOutEvent(QFocusEvent* _event) override;

    private:
        void updateHoverCircle(bool _hover);

        void showToolTip();
        void hideToolTip();

    private:
        std::unique_ptr<CircleHover> circleHover_;
        QTimer tooltipTimer_;
        QMargins extention_;
        bool enableCircleHover_ = false;
        bool underLongPress_ = false;
        bool enableTooltip_ = false;
        bool underMouse_ = false;
    };
}