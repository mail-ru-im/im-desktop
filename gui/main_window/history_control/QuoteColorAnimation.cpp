#include "stdafx.h"

#include "QuoteColorAnimation.h"
#include "../../styles/ThemeParameters.h"

QuoteColorAnimation::QuoteColorAnimation(QWidget* parent)
    : QObject(nullptr)
    , Widget_(parent)
    , Alpha_(255)
    , IsActive_(true)
    , bPlay_(false)
{
}

QColor QuoteColorAnimation::quoteColor() const
{
    return QuoteColor_;
}

void QuoteColorAnimation::setQuoteColor(QColor color)
{
    QuoteColor_ = std::move(color);
    Widget_->update();
}

void QuoteColorAnimation::setSemiTransparent()
{
    Alpha_ = 105;
}

static QByteArray quoteColorName()
{
    return QByteArrayLiteral("quoteColor");
}

void QuoteColorAnimation::startQuoteAnimation()
{
    if (!IsActive_ || bPlay_)
        return;

    bPlay_ = true;
    /// pause 0.25 sec
    /// color fader green->transparent
    /// pause 0.25 sec
    /// color fader transparent->green
    auto delay = new QPauseAnimation();
    delay->setDuration(250);
    delay->start(QAbstractAnimation::DeleteWhenStopped);

    connect(delay, &QPauseAnimation::finished, this, [this]()
    {
        auto quoteAnimationColor = getStartColor();

        auto anim = new QPropertyAnimation(this, quoteColorName());
        anim->setDuration(300);
        quoteAnimationColor.setAlpha(0);
        anim->setStartValue(quoteAnimationColor);
        quoteAnimationColor.setAlpha(Alpha_);
        anim->setEndValue(quoteAnimationColor);
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        connect(anim, &QPropertyAnimation::finished, this, [this]()
        {
            auto delay = new QPauseAnimation();
            delay->setDuration(150);
            delay->start(QAbstractAnimation::DeleteWhenStopped);

            connect(delay, &QPauseAnimation::finished, this, [this]()
            {
                auto quoteAnimationColor = getStartColor();

                auto anim = new QPropertyAnimation(this, quoteColorName());
                anim->setDuration(300);
                quoteAnimationColor.setAlpha(Alpha_);
                anim->setStartValue(quoteAnimationColor);
                quoteAnimationColor.setAlpha(0);
                anim->setEndValue(quoteAnimationColor);
                anim->start(QAbstractAnimation::DeleteWhenStopped);

                connect(anim, &QPropertyAnimation::finished, this, [this]()
                {
                    bPlay_ = false;
                    this->QuoteColor_ = QColor();
                    Widget_->update();
                });
            });
        });
    });
}

bool QuoteColorAnimation::isPlay() const
{
    return bPlay_;
}

void QuoteColorAnimation::deactivate()
{
    IsActive_ = false;
}

void QuoteColorAnimation::setAimId(const QString& _aimId)
{
    assert(!_aimId.isEmpty());
    aimId_ = _aimId;
}

QColor QuoteColorAnimation::getStartColor() const
{
    assert(!aimId_.isEmpty());
    return Styling::getParameters(aimId_).getColor(Styling::StyleVariable::PRIMARY);
}
