#include "stdafx.h"

#include "AttachFileButton.h"

#include "utils/utils.h"
#include "utils/InterConnector.h"
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
        const auto make_icon = [](const QColor& _color)
        {
            return Utils::renderSvg(qsl(":/input/expand_attach"), buttonSize(), _color);
        };

        if (_isHovered)
        {
            if (_mode == Ui::InputStyleMode::Default)
            {
                static const auto pm = make_icon(Styling::InputButtons::Default::hoverColor());
                return pm;
            }
            else
            {
                static const auto pm = make_icon(Styling::InputButtons::Alternate::hoverColor());
                return pm;
            }
        }
        else
        {
            if (_mode == Ui::InputStyleMode::Default)
            {
                static const auto pm = make_icon(Styling::InputButtons::Default::defaultColor());
                return pm;
            }
            else
            {
                static const auto pm = make_icon(Styling::InputButtons::Alternate::defaultColor());
                return pm;
            }
        }
    }
}

namespace Ui
{
    AttachFileButton::AttachFileButton(QWidget* _parent)
        : ClickableWidget(_parent)
        , currentAngle_(0)
        , isActive_(false)
    {
        connect(this, &AttachFileButton::hoverChanged, this, Utils::QOverload<>::of(&AttachFileButton::update));
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::attachFilePopupVisiblityChanged, this, &AttachFileButton::onAttachVisibleChanged);

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
        const auto endAngle = 90 * mod;
        anim_.start(
            [this]() { currentAngle_ = anim_.current(); update(); },
            [this]() { currentAngle_ = 0; },
            currentAngle_,
            endAngle,
            animDuration().count(),
            anim::sineInOut
        );
    }
}