#pragma once

#include "controls/SimpleListWidget.h"

namespace Styling
{
    enum class StyleVariable;
}

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
        bool isSelected_;
        bool isCompactMode_;
        const QPixmap normalIcon_;
        const QPixmap selectedIcon_;
        QString name_;
        std::unique_ptr<Ui::TextRendering::TextUnit> nameTextUnit_;
        Styling::StyleVariable iconBg_;
    };
}