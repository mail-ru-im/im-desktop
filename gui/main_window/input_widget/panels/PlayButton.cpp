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

    QColor focusColorGhost()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::GHOST_SECONDARY);
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
            setDefaultImage(qsl(":/ptt/stop_record"), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT), imageSize());
            setHoverColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
            setActiveColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT));
            setFocusColor(focusColorGhost());
            break;
        case Ui::PlayButton::State::Play:
            setDefaultImage(qsl(":/ptt/play_record"), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), imageSize());
            setHoverColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
            setActiveColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
            setFocusColor(focusColorPrimary());
            break;
        case Ui::PlayButton::State::Pause:
            setDefaultImage(qsl(":/ptt/pause_record"), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), imageSize());
            setHoverColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_HOVER));
            setActiveColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_ACTIVE));
            setFocusColor(focusColorPrimary());
            break;
        case Ui::PlayButton::State::MaxValue:
            Q_UNREACHABLE();
        default:
            break;
        }
    }
}