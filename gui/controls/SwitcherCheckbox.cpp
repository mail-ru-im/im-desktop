#include "stdafx.h"

#include "SwitcherCheckbox.h"
#include "../utils/utils.h"
#include "../styles/ThemeParameters.h"

namespace
{
    QSize getSize()
    {
        return Utils::scale_value(QSize(36, 20));
    }

    const QPixmap& getOnIcon()
    {
        static const auto pm = Utils::renderSvg(qsl(":/controls/switch_on_icon"), getSize(), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
        return pm;
    }

    const QPixmap& getOffIcon()
    {
        static const auto pm = Utils::renderSvg(qsl(":/controls/switch_off_icon"), getSize(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY));
        return pm;
    }

}

namespace Ui
{
    SwitcherCheckbox::SwitcherCheckbox(QWidget* _parent)
        : QCheckBox(_parent)
    {
        setFixedSize(getSize());
        setCursor(Qt::PointingHandCursor);

        connect(this, &SwitcherCheckbox::toggled, this, Utils::QOverload<>::of(&SwitcherCheckbox::update));
    }

    void SwitcherCheckbox::paintEvent(QPaintEvent*)
    {
        const auto& pm = isChecked() ? getOnIcon() : getOffIcon();
        const auto x = (width() - pm.width() / Utils::scale_bitmap_ratio()) / 2;
        const auto y = (height() - pm.height() / Utils::scale_bitmap_ratio()) / 2;

        QPainter p(this);
        p.drawPixmap(x, y, pm);
    }

    bool SwitcherCheckbox::hitButton(const QPoint&) const
    {
        return true;
    }

}