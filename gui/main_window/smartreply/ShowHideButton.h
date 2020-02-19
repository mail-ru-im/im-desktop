#pragma once

#include "controls/ClickWidget.h"

namespace Ui
{
    class ShowHideButton : public ClickableWidget
    {
        Q_OBJECT

    public:
        ShowHideButton(QWidget* _parent);

        void updateBackgroundColor(const QColor& _wallpaperColor);

        enum class ArrowState
        {
            up,
            transit,
            down,
        };
        const ArrowState getArrowState() const noexcept { return state_; }
        void setArrowState(const ArrowState _state);

    protected:
        void paintEvent(QPaintEvent* _e) override;

    private:
        QColor getMouseStateColor() const;
        void updateTooltipText();
        const std::vector<QPoint>& getArrowLine() const;

    private:
        ArrowState state_ = ArrowState::down;
        QColor bgColor_;
    };
}