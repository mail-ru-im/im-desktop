#include "stdafx.h"

#include "SimpleSettingsRow.h"

#include "../../controls/TextUnit.h"
#include "../../controls/SimpleListWidget.h"
#include "../../utils/utils.h"
#include "../../fonts.h"
#include "styles/ThemeParameters.h"

namespace
{
    int iconWidth()
    {
        return Utils::scale_value(32);
    }

    int pixmapWidth()
    {
        return Utils::scale_value(20);
    }

    int settingRowHeight()
    {
        return Utils::scale_value(44);
    }

    int normalIconOffset()
    {
        return Utils::scale_value(12);
    }

    int textOffset()
    {
        return normalIconOffset() + iconWidth() + Utils::scale_value(12);
    }

    QColor normalTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
    }

    QColor activeTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
    }

    QColor normalIconColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
    }

    QColor activeIconColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_SELECTED);
    }

    QColor activeBackgroundColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_SELECTED);
    }

    QColor hoveredBackgroundColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
    }

    QFont getNameTextFont()
    {
        return Fonts::appFontScaled(15);
    }
}

namespace Ui
{
    SimpleSettingsRow::SimpleSettingsRow(const QString& _icon, const Styling::StyleVariable _bg, const QString& _name, QWidget* _parent)
        : SimpleListItem(_parent)
        , isSelected_(false)
        , isCompactMode_(false)
        , normalIcon_(Utils::renderSvg(_icon, { pixmapWidth(), pixmapWidth() }, normalIconColor()))
        , selectedIcon_(Utils::renderSvg(_icon, { pixmapWidth(), pixmapWidth() }, activeIconColor()))
        , name_(_name)
        , iconBg_(_bg)
    {
        setFixedHeight(settingRowHeight());
        nameTextUnit_ = TextRendering::MakeTextUnit(name_);
        nameTextUnit_->init(getNameTextFont(), normalTextColor());
    }

    SimpleSettingsRow::~SimpleSettingsRow()
    {
    }

    void SimpleSettingsRow::setSelected(bool _value)
    {
        if (isSelected_ != _value)
        {
            isSelected_ = _value;
            updateTextColor();
            update();
        }
    }

    bool SimpleSettingsRow::isSelected() const
    {
        return isSelected_;
    }

    void SimpleSettingsRow::setCompactMode(bool _value)
    {
        if (isCompactMode_ != _value)
        {
            isCompactMode_ = _value;
            update();
        }
    }

    bool SimpleSettingsRow::isCompactMode() const
    {
        return isCompactMode_;
    }

    void SimpleSettingsRow::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        if (isSelected())
            p.fillRect(rect(), activeBackgroundColor());
        else if (isHovered())
            p.fillRect(rect(), hoveredBackgroundColor());

        const auto iconW = iconWidth();
        const auto iconX = isCompactMode() ? (width() - iconW) / 2 : normalIconOffset();
        const auto iconY = (height() - iconW) / 2;

        p.setPen(Qt::NoPen);
        p.setBrush(Styling::getParameters().getColor(isSelected() ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : iconBg_));
        p.drawEllipse(iconX, iconY, iconW, iconW);

        const auto pmX = iconX + (iconW - pixmapWidth()) / 2;
        const auto pmY = iconY + (iconW - pixmapWidth()) / 2;
        p.drawPixmap(pmX, pmY, isSelected() ? selectedIcon_ : normalIcon_);

        if (!isCompactMode())
        {
            nameTextUnit_->setOffsets(textOffset(), height() / 2);
            nameTextUnit_->getHeight(width() - nameTextUnit_->horOffset());
            nameTextUnit_->draw(p, TextRendering::VerPosition::MIDDLE);
        }
    }

    void SimpleSettingsRow::updateTextColor()
    {
        if (nameTextUnit_)
            nameTextUnit_->setColor(isSelected() ? activeTextColor() : normalTextColor());
    }
}