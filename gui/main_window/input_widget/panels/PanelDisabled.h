#pragma once

#include "../InputWidgetUtils.h"

namespace Ui
{
    class LabelEx;

    enum class DisabledPanelState
    {
        Banned,
        Pending,
        Empty,
    };

    class InputPanelDisabled : public QWidget, public StyledInputElement
    {
    public:
        InputPanelDisabled(QWidget* _parent);
        void setState(const DisabledPanelState _state);

    protected:
        void updateStyleImpl(const InputStyleMode _mode) override;

    private:
        void updateLabelColor();

    private:
        LabelEx* label_;
    };
}