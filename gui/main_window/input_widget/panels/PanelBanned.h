#pragma once

#include "../InputWidgetUtils.h"

namespace Ui
{
    class LabelEx;

    enum class BannedPanelState
    {
        Banned,
        Pending,
    };

    class InputPanelBanned : public QWidget, public StyledInputElement
    {
    public:
        InputPanelBanned(QWidget* _parent);
        void setState(const BannedPanelState _state);

    protected:
        void updateStyleImpl(const InputStyleMode _mode) override;

    private:
        void updateLabelColor();

    private:
        LabelEx* label_;
    };
}