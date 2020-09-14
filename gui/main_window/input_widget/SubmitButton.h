#pragma once

#include "controls/ClickWidget.h"
#include "CircleHover.h"
#include "InputWidgetUtils.h"

namespace Ui
{
    class CircleHover;

    class SubmitButton : public ClickableWidget, public StyledInputElement, CircleHoverAnimated
    {
        Q_OBJECT

    Q_SIGNALS:
        void longTapped(QPrivateSignal);
        void longPressed(QPrivateSignal);
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
        void keyPressEvent(QKeyEvent* _event) override;
        void keyReleaseEvent(QKeyEvent* _event) override;
        bool focusNextPrevChild(bool _next) override;

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

        void setIsUnderLongPress(bool _v);

    private:
        State state_ = State::Send;

        IconStates currentIcon_;
        QPixmap nextIcon_;

        QVariantAnimation* anim_;

        QTimer longTapTimer_;
        QTimer longPressTimer_;
        QTimer autoDelayTimer_;
        bool isWaitingLongPress_ = false;
        bool isUnderLongPress_ = false;

        bool enableCircleHover_ = false;
        std::unique_ptr<CircleHover> circleHover_;
        QMargins extention_;
        bool underMouse_ = false;
    };
}