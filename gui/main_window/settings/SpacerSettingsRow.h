#pragma once

#include "controls/SimpleListWidget.h"

namespace Ui
{
    class SpacerSettingsRow : public SimpleListItem
    {
    public:
        SpacerSettingsRow(QWidget* _parent = nullptr);

        void setSelected(bool _value) override;
        bool isSelected() const override;

    protected:
        void paintEvent(QPaintEvent*) override;
    };
}