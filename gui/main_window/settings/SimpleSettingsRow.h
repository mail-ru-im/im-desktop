#pragma once

#include "controls/SimpleListWidget.h"
#include "../../styles/StyleVariable.h"
#include "utils/SvgUtils.h"

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class SimpleSettingsRow : public SimpleListItem
    {
        Q_OBJECT;

    public:
        explicit SimpleSettingsRow(const QString& _icon, const Styling::StyleVariable _bg, const QString& _name, QWidget* _parent = nullptr);
        ~SimpleSettingsRow();

        void setSelected(bool _value) override;
        bool isSelected() const override;

        void setCompactMode(bool _value) override;
        bool isCompactMode() const override;

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        void updateTextColor();

    private:
        Utils::StyledPixmap normalIcon_;
        Utils::StyledPixmap selectedIcon_;
        QString name_;
        std::unique_ptr<Ui::TextRendering::TextUnit> nameTextUnit_;
        Styling::StyleVariable iconBg_;
        bool isSelected_;
        bool isCompactMode_;
    };
}