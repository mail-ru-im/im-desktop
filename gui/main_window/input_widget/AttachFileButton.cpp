#include "stdafx.h"

#include "AttachFileButton.h"

#include "utils/utils.h"
#include "styles/ThemeParameters.h"

namespace
{
    constexpr std::chrono::milliseconds animDuration() noexcept { return std::chrono::milliseconds(150); }

    QSize buttonSize()
    {
        return Utils::scale_value(QSize(32, 32));
    }

    QPixmap getIcon(const Ui::InputStyleMode _mode, const bool _isHovered)
    {
        const auto make_icon = [](const Styling::ThemeColorKey& _key)
        {
            return Utils::StyledPixmap(qsl(":/input/expand_attach"), buttonSize(), _key);
        };

        if (_isHovered)
        {
            if (_mode == Ui::InputStyleMode::Default)
            {
                static auto pm = make_icon(Styling::InputButtons::Default::hoverColorKey());
                return pm.actualPixmap();
            }
            else
            {
                static auto pm = make_icon(Styling::InputButtons::Alternate::hoverColorKey());
                return pm.actualPixmap();
            }
        }
        else
        {
            if (_mode == Ui::InputStyleMode::Default)
            {
                static auto pm = make_icon(Styling::InputButtons::Default::defaultColorKey());
                return pm.actualPixmap();
            }
            else
            {
                static auto pm = make_icon(Styling::InputButtons::Alternate::defaultColorKey());
                return pm.actualPixmap();
            }
        }
    }
}

namespace Ui
{
    AttachFileButton::AttachFileButton(QWidget* _parent)
        : ClickableWidget(_parent)
        , anim_(new QVariantAnimation(this))
        , currentAngle_(0.0)
        , isActive_(false)
    {
        connect(this, &AttachFileButton::hoverChanged, this, qOverload<>(&AttachFileButton::update));

        anim_->setDuration(animDuration().count());
        anim_->setEasingCurve(QEasingCurve::InOutSine);
        connect(anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
        {
            currentAngle_ = value.toDouble();
            update();
        });
        connect(anim_, &QVariantAnimation::finished, this, [this]()
        {
            currentAngle_ = 0.0;
        });

        setFixedSize(buttonSize());

        setFocusPolicy(Qt::TabFocus);
        setFocusColor(focusColorPrimary());
        setTooltipText(QT_TRANSLATE_NOOP("tooltips", "Attach"));
    }

    void AttachFileButton::updateStyleImpl(const InputStyleMode)
    {
        update();
    }

    void AttachFileButton::setActive(const bool _isActive)
    {
        isActive_ = _isActive;
        update();
    }

    void AttachFileButton::paintEvent(QPaintEvent* _event)
    {
        ClickableWidget::paintEvent(_event);

        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        const auto cX = width() / 2;
        const auto cY = height() / 2;
        p.translate(cX, cY);
        p.rotate(currentAngle_);
        p.drawPixmap(-cX, -cY, getIcon(styleMode_, isHovered() || isActive()));
    }

    void AttachFileButton::onAttachVisibleChanged(const bool _isVisible)
    {
        setActive(_isVisible);

        if (_isVisible)
            rotate(RotateDirection::Left);
        else
            rotate(RotateDirection::Right);
    }

    void AttachFileButton::rotate(const RotateDirection _dir)
    {
        const auto mod = _dir == RotateDirection::Left ? -1 : 1;
        const auto endAngle = 90.0 * mod;

        anim_->stop();
        anim_->setStartValue(currentAngle_);
        anim_->setEndValue(endAngle);
        anim_->start();
    }
}