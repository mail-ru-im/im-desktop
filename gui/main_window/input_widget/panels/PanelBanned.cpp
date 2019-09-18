#include "stdafx.h"

#include "PanelBanned.h"

#include "controls/LabelEx.h"
#include "utils/utils.h"
#include "styles/ThemeParameters.h"
#include "fonts.h"

namespace Ui
{
    InputPanelBanned::InputPanelBanned(QWidget* _parent)
        : QWidget(_parent)
    {
        setFixedHeight(getDefaultInputHeight());

        label_ = new LabelEx(this);
        label_->setFont(Fonts::appFontScaled(14));
        label_->setAlignment(Qt::AlignCenter);
        label_->setWordWrap(true);
        label_->setTextInteractionFlags(Qt::NoTextInteraction);
        Testing::setAccessibleName(label_, qsl("AS inputwidget label"));

        auto l = Utils::emptyVLayout(this);
        l->addWidget(label_);

        setState(BannedPanelState::Banned);
        updateLabelColor();
    }

    void InputPanelBanned::setState(const BannedPanelState _state)
    {
        switch (_state)
        {
        case BannedPanelState::Banned:
            label_->setText(QT_TRANSLATE_NOOP("input_widget", "You was banned to write in this group"));
            break;

        case BannedPanelState::Pending:
            label_->setText(QT_TRANSLATE_NOOP("input_widget", "The join request has been sent to administrator"));
            break;

        default:
            assert(!"unknown state");
            break;
        }
    }

    void InputPanelBanned::updateStyleImpl(const InputStyleMode)
    {
        updateLabelColor();
    }

    void InputPanelBanned::updateLabelColor()
    {
        const auto var = styleMode_ == InputStyleMode::Default
            ? Styling::StyleVariable::BASE_PRIMARY
            : Styling::StyleVariable::GHOST_PRIMARY_INVERSE;
        label_->setColor(Styling::getParameters().getColor(var));
    }
}
