#pragma once

#include "controls/ClickWidget.h"
#include "../ButtonWithCircleHover.h"

namespace Ui
{
    class PlayButton : public ButtonWithCircleHover
    {
        Q_OBJECT

    public:

        enum class State
        {
            Stop,
            Play,
            Pause,

            MaxValue
        };

        explicit PlayButton(QWidget* _parent = nullptr);
        ~PlayButton();

        void setState(State _s);
        State getState() const;

    private:
        void updateIcon(State _s);
        void updateTooltipText(State _s);

    private:
        State state_ = State::MaxValue;
    };
}