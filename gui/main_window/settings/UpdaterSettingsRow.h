#pragma once

#include "controls/SimpleListWidget.h"

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class UpdatesWidget;

    class UpdaterSettingsRow : public SimpleListItem
    {
        Q_OBJECT;

    public:
        explicit UpdaterSettingsRow(QWidget* _parent = nullptr);
        ~UpdaterSettingsRow();

        void setSelected(bool _value) override;
        bool isSelected() const override;

        void setCompactMode(bool _value) override;
        bool isCompactMode() const override;

        void setUpdateReady(bool _value);

    private:
        bool isCompactMode_;
        bool isUpdateReady_;
        UpdatesWidget* updatesWidget_;
    };
}