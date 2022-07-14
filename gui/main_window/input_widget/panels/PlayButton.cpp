#include "stdafx.h"

#include "PlayButton.h"

#include "styles/ThemeParameters.h"
#include "utils/utils.h"
#include "../InputWidgetUtils.h"

namespace
{
    auto imageSize()
    {
        return QSize(20, 20);
    }

    auto buttonSize()
    {
        return QSize(32, 32);
    }

    auto focusColorGhostKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::GHOST_SECONDARY };
    }
}

namespace Ui
{
    PlayButton::PlayButton(QWidget* _parent)
        : ButtonWithCircleHover(_parent)
    {
        setFixedSize(Utils::scale_value(buttonSize()));
        setState(State::Stop);
        setFocusPolicy(Qt::TabFocus);
    }

    PlayButton::~PlayButton() = default;

    void PlayButton::setState(State _s)
    {
        if (_s != state_)
        {
            state_ = _s;
            updateIcon(_s);
            updateTooltipText(_s);
        }
    }

    PlayButton::State PlayButton::getState() const
    {
        return state_;
    }

    void PlayButton::updateIcon()
    {
        updateIcon(getState());
    }

    void PlayButton::updateTooltipText(State _s)
    {
        switch (_s)
        {
        case Ui::PlayButton::State::Stop:
            setCustomToolTip(QT_TRANSLATE_NOOP("input_widget", "Stop record"));
            break;
        case Ui::PlayButton::State::Play:
            setCustomToolTip(QT_TRANSLATE_NOOP("input_widget", "Play record"));
            break;
        case Ui::PlayButton::State::Pause:
            setCustomToolTip(QT_TRANSLATE_NOOP("input_widget", "Pause record"));
            break;
        case Ui::PlayButton::State::MaxValue:
            Q_UNREACHABLE();
        default:
            break;
        }
    }

    void PlayButton::updateIcon(State _s)
    {
        switch (_s)
        {
        case Ui::PlayButton::State::Stop:
            setDefaultImage(qsl(":/ptt/stop_record"), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE }, imageSize());
            setHoverColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE });
            setActiveColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE });
            setFocusColor(focusColorGhostKey());
            break;
        case Ui::PlayButton::State::Play:
            setDefaultImage(qsl(":/ptt/play_record"), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY }, imageSize());
            setHoverColor(Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY });
            setActiveColor(Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY });
            setFocusColor(focusColorPrimaryKey());
            break;
        case Ui::PlayButton::State::Pause:
            setDefaultImage(qsl(":/ptt/pause_record"), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY }, imageSize());
            setHoverColor(Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_HOVER });
            setActiveColor(Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_ACTIVE });
            setFocusColor(focusColorPrimaryKey());
            break;
        case Ui::PlayButton::State::MaxValue:
            Q_UNREACHABLE();
        default:
            break;
        }
    }
}