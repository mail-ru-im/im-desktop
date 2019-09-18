#pragma once

#include "controls/CustomButton.h"
#include "CircleHover.h"

namespace Ui
{
    class ButtonWithCircleHover : public CustomButton, CircleHoverAnimated
    {
        Q_OBJECT
    public:
        explicit ButtonWithCircleHover(QWidget* _parent, const QString& _svgName = QString(), const QSize& _iconSize = QSize(), const QColor& _defaultColor = QColor());
        ~ButtonWithCircleHover();

        void setCircleHover(std::unique_ptr<CircleHover>&& _) override;
        void enableCircleHover(bool _val) override;

        void updateHover();
        void updateHover(bool _hover);
        void setRectExtention(const QMargins& _m);
        bool containsCursorUnder() const;

        void showToolTipForce();

    private:
        void updateHoverCircle(bool _hover);

        void showToolTip();
        void hideToolTip();

    private:
        bool enableCircleHover_ = false;
        bool enableTooltip_ = false;
        bool underMouse_ = false;
        std::unique_ptr<CircleHover> circleHover_;
        QTimer tooltipTimer_;
        QMargins extention_;
    };
}