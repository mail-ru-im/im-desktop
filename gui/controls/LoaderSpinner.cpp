#include "stdafx.h"

#include "LoaderSpinner.h"
#include "CustomButton.h"
#include "../main_window/Placeholders.h"
#include "../styles/ThemeParameters.h"
#include "../utils/utils.h"

namespace
{
    auto getDefaultWidgetSize() noexcept
    {
        return Utils::scale_value(QSize(64, 64));
    }

    auto getRectRadius() noexcept
    {
        return Utils::fscale_value(8);
    }

    auto getShadowSize() noexcept
    {
        return QSize(0, Utils::getShadowMargin());
    }

    auto getCloseIconSize()
    {
        return QSize(12, 12);
    }

    auto getBgColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
    }
}

using namespace Ui;

LoaderSpinner::LoaderSpinner(QWidget* _parent, const QSize& _size, Options _opt)
    : QWidget(_parent)
    , button_(nullptr)
    , size_(_size)
    , options_(_opt)
{
    spinner_ = new Ui::RotatingSpinner(this);

    if (options_ & Option::Cancelable)
    {
        button_ = new CustomButton(this, qsl(":/controls/close_icon"), getCloseIconSize());
        Styling::Buttons::setButtonDefaultColors(button_);
        button_->setFixedSize(spinner_->size());

        connect(button_, &CustomButton::clicked, this, &LoaderSpinner::clicked);
    }

    if (!size_.isValid())
        size_ = getDefaultWidgetSize();

    setFixedSize(options_ & Option::Shadowed ? size_ + getShadowSize() : size_);
}

LoaderSpinner::~LoaderSpinner()
{
    stopAnimation();
}

void LoaderSpinner::startAnimation(const QColor& _spinnerColor, const QColor& _bgColor)
{
    spinner_->startAnimation(_spinnerColor, _bgColor);
}

void LoaderSpinner::stopAnimation()
{
    spinner_->stopAnimation();
}

void LoaderSpinner::paintEvent(QPaintEvent* _event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    const auto rect = QRectF(0, 0, size_.width(), size_.height());
    if (options_ & Option::Rounded)
        path.addRoundedRect(rect, getRectRadius(), getRectRadius());
    else
        path.addRect(rect);

    if (options_ & Option::Shadowed)
        Utils::drawBubbleShadow(p, path);

    p.fillPath(path, getBgColor());
}

void LoaderSpinner::resizeEvent(QResizeEvent* _event)
{
    size_ = _event->size();
    if (options_ & Option::Shadowed)
        size_ -= getShadowSize();

    const auto localPos = QPoint((size_.width() - spinner_->width()) / 2, (size_.height() - spinner_->height()) / 2);
    spinner_->move(localPos);
    if (button_)
        button_->move(localPos);

    update();

    QWidget::resizeEvent(_event);
}
