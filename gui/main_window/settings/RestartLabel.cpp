#include "stdafx.h"

#include "RestartLabel.h"
#include "../../controls/TextUnit.h"
#include "../../utils/utils.h"
#include "../../fonts.h"
#include "../../styles/ThemeParameters.h"

namespace
{
    QFont getTextFont()
    {
        return Fonts::appFontScaled(14);
    }

    int textHorPadding()
    {
        return Utils::scale_value(16);
    }

    int borderRadius()
    {
        return Utils::scale_value(8);
    }
}

namespace Ui
{
    RestartLabel::RestartLabel(QWidget* _parent)
        : QWidget(_parent)
    {
        textUnit_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("settings", "Application restart required"));
        textUnit_->init(getTextFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT);
        setFixedHeight(Utils::scale_value(38));
    }

    RestartLabel::~RestartLabel()
    {
    }

    void RestartLabel::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);

        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        const auto r = rect();

        const auto textHeight = textUnit_->getHeight(r.width() - 2 * textHorPadding());

        {
            const static auto background = Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_INFO);
            Utils::PainterSaver saver(p);
            p.setPen(Qt::NoPen);
            p.setBrush(background);
            p.drawRoundedRect(r, borderRadius(), borderRadius());
        }

        textUnit_->setOffsets(textHorPadding(), r.height() / 2);
        textUnit_->draw(p, TextRendering::VerPosition::MIDDLE);
    }
}