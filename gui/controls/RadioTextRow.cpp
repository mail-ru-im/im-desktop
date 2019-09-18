#include "stdafx.h"

#include "RadioTextRow.h"
#include "controls/TextUnit.h"

#include "../utils/utils.h"
#include "../fonts.h"
#include "../styles/ThemeParameters.h"

namespace
{
    int getDefaultHeight()
    {
        return Utils::scale_value(44);
    }

    QFont getNameTextFont()
    {
        return Fonts::appFontScaled(15);
    }

    int textHorOffset()
    {
        return Utils::scale_value(20);
    }

    int commentHorOffset()
    {
        return Utils::scale_value(4);
    }

    int circleOffset()
    {
        return Utils::scale_value(16);
    }

    int circleRadius()
    {
        return Utils::scale_value(10);
    }

    int internalCircleRadius()
    {
        return Utils::scale_value(4);
    }

    QColor activeCirleColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
    }

    QColor activeInternalCirleColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);
    }

    QColor cirleColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);
    }

    QColor internalCirleColor()
    {
        return cirleColor();
    }
}

namespace Ui
{
    RadioTextRow::RadioTextRow(const QString& _name, QWidget* _parent)
        : SimpleListItem(_parent)
        , name_(_name)
        , isSelected_(false)
    {
        setFixedHeight(getDefaultHeight());

        nameTextUnit_ = TextRendering::MakeTextUnit(name_);
        nameTextUnit_->init(getNameTextFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
    }

    RadioTextRow::~RadioTextRow()
    {
    }

    void RadioTextRow::setSelected(bool _value)
    {
        if (isSelected_ != _value)
        {
            isSelected_ = _value;
            update();
        }
    }

    bool RadioTextRow::isSelected() const
    {
        return isSelected_;
    }

    void RadioTextRow::setComment(const QString& _comment)
    {
        commentTextUnit_ = TextRendering::MakeTextUnit(_comment);
        commentTextUnit_->init(getNameTextFont(), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT);
    }

    void RadioTextRow::resetComment()
    {
        commentTextUnit_.reset();
    }

    void RadioTextRow::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        const auto r = rect();
        const static auto normalBackground = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
        const static auto hoveredBackground = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
        p.fillRect(r, isHovered() ? hoveredBackground : normalBackground);

        nameTextUnit_->setOffsets(textHorOffset(), r.height() / 2);
        nameTextUnit_->getHeight(r.width() - nameTextUnit_->horOffset() - 2 * circleOffset() - circleRadius());
        nameTextUnit_->draw(p, TextRendering::VerPosition::MIDDLE);

        if (commentTextUnit_)
        {
            commentTextUnit_->setOffsets(textHorOffset() + nameTextUnit_->getMaxLineWidth() + commentHorOffset(), r.height() / 2);
            commentTextUnit_->getHeight(r.width());
            commentTextUnit_->draw(p, TextRendering::VerPosition::MIDDLE);
            commentTextUnit_->setColor(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        }

        const auto circleCenter = QPointF(r.width() - circleOffset() - circleRadius(), r.height() / 2.0);
        Utils::PainterSaver ps(p);
        if (isSelected())
        {
            p.setPen(Qt::NoPen);
            p.setBrush(activeCirleColor());
            p.drawEllipse(circleCenter, circleRadius(), circleRadius());
            p.setBrush(activeInternalCirleColor());
            p.drawEllipse(circleCenter, internalCircleRadius(), internalCircleRadius());
        }
        else
        {
            const static QPen pen(cirleColor(), Utils::scale_value(1));
            p.setPen(pen);
            p.drawEllipse(circleCenter, circleRadius(), circleRadius());
            if (isHovered())
            {
                p.setBrush(internalCirleColor());
                p.drawEllipse(circleCenter, internalCircleRadius(), internalCircleRadius());
            }
        }
    }
}
