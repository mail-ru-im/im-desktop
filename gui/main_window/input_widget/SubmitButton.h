#pragma once

#include "controls/ClickWidget.h"
#include "animation/animation.h"
#include "CircleHover.h"
#include "InputWidgetUtils.h"

namespace Ui
{
    class CircleHover;

    class SubmitButton : public ClickableWidget, public StyledInputElement,  CircleHoverAnimated
    {
        Q_OBJECT

    Q_SIGNALS:
        void longTapped(QPrivateSignal);
        void mouseMovePressed(QPrivateSignal);

    public:
        SubmitButton(QWidget* _parent);
        ~SubmitButton();

        enum class State
        {
            Ptt,
            Send,
            Edit,
        };
        void setState(const State _state, const StateTransition _transition);
        State getState() const noexcept { return state_; }

        void setCircleHover(std::unique_ptr<CircleHover>&&) override;
        void enableCircleHover(bool _val) override;
        void setRectExtention(const QMargins& _m);
        bool containsCursorUnder() const;

    protected:
        void paintEvent(QPaintEvent*) override;
        void leaveEvent(QEvent* _e) override;
        void mousePressEvent(QMouseEvent* _e) override;
        void mouseReleaseEvent(QMouseEvent* _e) override;
        void mouseMoveEvent(QMouseEvent* _e) override;
        void focusInEvent(QFocusEvent* _event) override;
        void focusOutEvent(QFocusEvent* _event) override;

        void updateStyleImpl(const InputStyleMode _mode) override;
        void onTooltipTimer() override;
        void showToolTip() override;

    private:
        struct IconStates
        {
            QPixmap normal_;
            QPixmap hover_;
            QPixmap pressed_;
        };
        IconStates getStatePixmaps() const;

        void onMouseStateChanged();
        void onHoverChanged();
        void updateHoverCircle(bool _hover);

        QString getStateTooltip() const;

    private:
        State state_;

        IconStates currentIcon_;
        QPixmap nextIcon_;

        anim::Animation anim_;

        QTimer longTapTimer_;
        bool enableCircleHover_;
        std::unique_ptr<CircleHover> circleHover_;
        QMargins extention_;
        bool underMouse_;
    };
}