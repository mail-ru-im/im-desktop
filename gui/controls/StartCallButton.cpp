#include "stdafx.h"
#include "StartCallButton.h"
#include "ContextMenu.h"
#include "../core_dispatcher.h"
#include "../utils/utils.h"
#include "../styles/ThemeParameters.h"
#include "../main_window/GroupChatOperations.h"

using namespace Ui;

namespace
{
    constexpr std::chrono::milliseconds animDuration() noexcept { return std::chrono::milliseconds(150); }

    auto getPhoneButtonSize(bool _isButton)
    {
        return _isButton ? Utils::scale_value(32) : Utils::scale_value(24);
    }

    auto getArrowButtonSize(bool _isButton)
    {
        return _isButton ? Utils::scale_value(20) : Utils::scale_value(12);
    }

    auto getCallButtonSize(bool _isButton)
    {
        return _isButton ? Utils::scale_value(QSize(76, 40)) : Utils::scale_value(QSize(36, 24));
    }

    auto getColor(Ui::CallButtonType _type, bool _isHovered, bool _isActive)
    {
        return (_type == Ui::CallButtonType::Button)
               ? Styling::StyleVariable::BASE_GLOBALWHITE
               : (_isActive ? Styling::StyleVariable::BASE_SECONDARY_ACTIVE : (_isHovered ? Styling::StyleVariable::BASE_SECONDARY_HOVER : Styling::StyleVariable::BASE_SECONDARY));
    }

    QPixmap getPhonePixmap(Ui::CallButtonType _type, bool _isHovered, bool _isActive)
    {
        auto makeBtn = [&_type](const auto _var)
        {
            const auto isButton = _type == Ui::CallButtonType::Button;
            const auto path = isButton ? qsl(":/sidebar_call") : qsl(":/phone_icon");
            const auto size = getPhoneButtonSize(isButton);
            return Utils::renderSvg(path, QSize(size,size), Styling::getParameters().getColor(_var));
        };
        return makeBtn(getColor(_type, _isHovered, _isActive));
    }

    QPixmap getArrowPixmap(Ui::CallButtonType _type, bool _isHovered, bool _isActive)
    {
        auto makeBtn = [&_type](const auto _var)
        {
            const auto isButton = _type == Ui::CallButtonType::Button;
            const auto size = getArrowButtonSize(isButton);
            const auto path = isButton ? qsl(":/controls/arrow_right") : qsl(":/arrow_small");
            auto pm =  Utils::renderSvg(path, QSize(size, size), Styling::getParameters().getColor(_var));
            if (isButton)
            {
                QTransform trans;
                trans.rotate(90);
                return pm.transformed(trans);
            }
            return pm;
        };
        return makeBtn(getColor(_type, _isHovered, _isActive));
    }

    auto getBgColor(bool _hovered, bool _pressed)
    {
        if (_hovered)
            return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_HOVER);
        else if (_pressed)
            return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_ACTIVE);

        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
    }
}

StartCallButton::StartCallButton(QWidget* _parent, CallButtonType _type)
    : ClickableWidget(_parent)
    , anim_(new QVariantAnimation(this))
    , currentAngle_(0.0)
    , type_(_type)
    , menu_(nullptr)
    , callLinkCreator_(nullptr)
    , doPreventNextMenu_(false)
{
    anim_->setDuration(animDuration().count());
    anim_->setEasingCurve(QEasingCurve::InOutSine);
    connect(anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
    {
        currentAngle_ = value.toDouble();
        update();
    });

    connect(this, &ClickableWidget::clicked, this, &StartCallButton::showContextMenu);
    if constexpr (platform::is_windows())
    {
        connect(this, &ClickableWidget::pressed, this, [this]()
        {
            if (menu_)
                doPreventNextMenu_ = true;
        });
    }

    setFixedSize(getCallButtonSize(isButton()));
    setTooltipText(QT_TRANSLATE_NOOP("tooltips", "Call"));
}

StartCallButton::~StartCallButton() = default;

void StartCallButton::rotate(const RotateDirection _dir)
{
    const auto endAngle = 180.0 * (_dir == RotateDirection::Left ? -1 : 0);
    anim_->stop();
    anim_->setStartValue(currentAngle_);
    anim_->setEndValue(endAngle);
    anim_->start();
}

void StartCallButton::showContextMenu()
{
    if constexpr (platform::is_windows())
    {
        if (doPreventNextMenu_)
        {
            doPreventNextMenu_ = false;
            return;
        }
    }

    if (!menu_)
    {
        menu_ = new ContextMenu(parentWidget());
        Testing::setAccessibleName(menu_, qsl("AS StartCallButton menu"));
        QObject::connect(menu_, &ContextMenu::hidden, this, [this]()
        {
            setHovered(false);
            setPressed(false);
            rotate(Ui::StartCallButton::RotateDirection::Right);
            menu_->deleteLater();
        }, Qt::QueuedConnection);

        // This call must be put on event queue, otherwise after call list popup closed
        // CustomMenu which is already deleted gets mouseReleaseEvent and app crashes
        menu_->addActionWithIcon(qsl(":/phone_icon"), QT_TRANSLATE_NOOP("tooltips", "Audio call"), this, [this]()
        {
            QMetaObject::invokeMethod(this, &StartCallButton::startAudioCall, Qt::QueuedConnection);
        });
        menu_->addActionWithIcon(qsl(":/video_icon"), QT_TRANSLATE_NOOP("tooltips", "Video call"), this, [this]()
        {
            QMetaObject::invokeMethod(this, &StartCallButton::startVideoCall, Qt::QueuedConnection);
        });
        menu_->addActionWithIcon(qsl(":/copy_link_icon"), QT_TRANSLATE_NOOP("tooltips", "Link to call"), this, &StartCallButton::createCallLink);
        menu_->invertRight(true);

        rotate(Ui::StartCallButton::RotateDirection::Left);
        auto pos = mapToGlobal(mapFromParent(geometry().bottomRight()));
        pos.ry() += Utils::scale_value(8);
        menu_->popup(pos);
    }
}

void StartCallButton::paintEvent(QPaintEvent* _event)
{
    ClickableWidget::paintEvent(_event);

    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    if (isButton())
    {
        const auto bgRect = rect();
        const auto r = bgRect.height() / 2;
        p.setPen(Qt::NoPen);
        p.setBrush(getBgColor(isHovered(), isPressed()));
        p.drawRoundedRect(bgRect, r, r);
    }

    const auto arrowSize = getArrowButtonSize(isButton());
    const auto phoneSize = getPhoneButtonSize(isButton());
    const auto dX = (width() - phoneSize - arrowSize) / 2;
    const auto dYPhone = (height() - phoneSize) / 2;
    const auto dYArrow = (height() - arrowSize) / 2;

    const auto phonePixmap = getPhonePixmap(type_, isHovered(), isPressed());
    p.drawPixmap(dX, dYPhone, phonePixmap);

    {
        Utils::PainterSaver ps(p);
        p.translate(dX + phoneSize + arrowSize / 2, dYArrow + arrowSize / 2);
        p.rotate(currentAngle_);
        p.translate(-(dX + phoneSize + arrowSize / 2), -(dYArrow + arrowSize / 2));
        const auto arrowPixmap = getArrowPixmap(type_, isHovered(), isPressed());
        p.drawPixmap(dX + phoneSize, dYArrow, arrowPixmap);
    }
}

void StartCallButton::startAudioCall() const
{
    doVoipCall(aimId_, CallType::Audio, isButton() ? CallPlace::Profile : CallPlace::Chat);
}

void StartCallButton::startVideoCall() const
{
    doVoipCall(aimId_, CallType::Video, isButton() ? CallPlace::Profile : CallPlace::Chat);
}

void StartCallButton::createCallLink()
{
    if (!callLinkCreator_)
        callLinkCreator_ = new Utils::CallLinkCreator(isButton() ? Utils::CallLinkFrom::Profile : Utils::CallLinkFrom::Chat);
    callLinkCreator_->createCallLink(ConferenceType::Call, aimId_);
}
