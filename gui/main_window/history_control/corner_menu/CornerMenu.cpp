#include "stdafx.h"

#include "CornerMenu.h"
#include "main_window/history_control/HistoryControlPageItem.h"
#include "main_window/history_control/MessageStyle.h"
#include "utils/graphicsEffects.h"
#include "styles/ThemeParameters.h"

namespace
{
    int getMargin()
    {
        return Utils::scale_value(2);
    }

    QSize getButtonSize()
    {
        return Utils::scale_value(QSize(24, 24));
    }

    QSize getIconSize()
    {
        return Utils::scale_value(QSize(20, 20));
    }

    int getMenuHeight()
    {
        return getButtonSize().height() + 2 * getMargin();
    }

    int getMenuWidth(int _buttonCount)
    {
        return getMargin() * (_buttonCount + 1) + getButtonSize().width() * _buttonCount;
    }

    QColor getBackgroundColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY);
    }

    QColor getHoverColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_BRIGHT);
    }

    const QPixmap& getIcon(const QString& _path, Styling::StyleVariable _color)
    {
        using IconColors = std::unordered_map<Styling::StyleVariable, QPixmap>;
        using IconCache = std::unordered_map<QString, IconColors>;
        static IconCache cache;

        auto& pm = cache[_path][_color];
        if (pm.isNull())
            pm = Utils::renderSvg(_path, getIconSize(), Styling::getParameters().getColor(_color));
        return pm;
    }


    enum class IconStyle
    {
        Path,
        Filled,
    };
    QStringView getIconPath(Ui::MenuButtonType _type, IconStyle _style)
    {
        switch (_type)
        {
        case Ui::MenuButtonType::Thread:
            return _style == IconStyle::Filled ? u":/thread_icon_filled" : u":/thread_icon";
        case Ui::MenuButtonType::Reaction:
            return _style == IconStyle::Filled ? u":/reaction_icon_filled" : u":/reaction_icon";
        default:
            break;
        }

        im_assert(!"unknown button");
        return {};
    }

    QStringView getAccessibleName(Ui::MenuButtonType _type)
    {
        switch (_type)
        {
        case Ui::MenuButtonType::Thread:
            return u"Thread";
        case Ui::MenuButtonType::Reaction:
            return u"Reaction";
        default:
            break;
        }

        im_assert(!"unknown button");
        return u"Unknown";
    }

    struct ButtonPredicate
    {
        ButtonPredicate(Ui::MenuButtonType _type) : type_(_type) {}
        Ui::MenuButtonType type_;

        bool operator()(Ui::SimpleListItem* _item)
        {
            if (auto btn = qobject_cast<Ui::CornerMenuButton*>(_item))
                return btn->getType() == type_;
            return false;
        }
    };
}

namespace Ui
{
    CornerMenuButton::CornerMenuButton(MenuButtonType _type, QWidget* _parent)
        : SimpleListItem(_parent)
        , type_(_type)
    {
        setFixedSize(getButtonSize());

        connect(this, &CornerMenuButton::hoverChanged, this, qOverload<>(&CornerMenuButton::update));
        connect(this, &CornerMenuButton::pressChanged, this, qOverload<>(&CornerMenuButton::update));
        connect(this, &CornerMenuButton::selectChanged, this, qOverload<>(&CornerMenuButton::update));
    }

    void CornerMenuButton::setIcon(QStringView _iconPath)
    {
        iconNormal_ = _iconPath.toString();
        update();
    }

    void CornerMenuButton::setActiveIcon(QStringView _iconPath)
    {
        iconActive_ = _iconPath.toString();
        update();
    }

    void CornerMenuButton::setSelected(bool _value)
    {
        if (std::exchange(isSelected_, _value) != _value)
        {
            Q_EMIT selectChanged(isSelected_);
            update();
        }
    }

    void CornerMenuButton::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        const auto isActive = isHovered() || isPressed() || isSelected();
        const auto clr = isActive ? Styling::StyleVariable::PRIMARY : Styling::StyleVariable::BASE_SECONDARY;
        const auto& iconPath = isActive ? iconActive_ : iconNormal_;
        const auto& icon = getIcon(iconPath, clr);
        const auto x = (width() - Utils::unscale_bitmap(icon.width())) / 2;
        const auto y = (height() - Utils::unscale_bitmap(icon.height())) / 2;
        p.drawPixmap(x, y, icon);
    }

    CornerMenuContainer::CornerMenuContainer(QWidget* _parent)
        : QWidget(_parent)
    {
        auto shadowEffect = new QGraphicsDropShadowEffect(this);
        shadowEffect->setBlurRadius(3);
        shadowEffect->setOffset(0, Utils::scale_value(1));
        shadowEffect->setColor(QColor(0, 0, 0, 255 * 0.04));
        setGraphicsEffect(shadowEffect);

        setFixedHeight(getMenuHeight());
    }

    void CornerMenuContainer::paintEvent(QPaintEvent* _e)
    {
        const auto radius = Utils::scale_value(8);

        QPainterPath path;
        path.addRoundedRect(rect(), radius, radius);

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setClipPath(path);
        p.fillRect(rect(), getBackgroundColor());

        p.setOpacity(0.1);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(Qt::black, Utils::scale_value(1)));
        p.drawPath(path);
    }

    CornerMenu::CornerMenu(QWidget* _parent)
        : QWidget(_parent)
        , opacityEffect_(new Utils::OpacityEffect(this))
        , listWidget_(new SimpleListWidget(Qt::Horizontal))
    {
        setMouseTracking(true);
        setFixedHeight(getMenuHeight() + shadowOffset() * 2);

        setGraphicsEffect(opacityEffect_);
        initAnimation();

        auto container = new CornerMenuContainer(this);
        auto containerLayout = Utils::emptyVLayout(container);
        containerLayout->setContentsMargins({ getMargin(), getMargin(), getMargin(), getMargin() });
        containerLayout->addWidget(listWidget_);

        auto rootLayout = Utils::emptyVLayout(this);
        rootLayout->setAlignment(Qt::AlignTop);
        rootLayout->setContentsMargins({ shadowOffset(), shadowOffset(), shadowOffset(), shadowOffset() });
        rootLayout->addWidget(container);

        listWidget_->setSpacing(getMargin());
        connect(listWidget_, &SimpleListWidget::clicked, this, &CornerMenu::onItemClicked);
    }

    void CornerMenu::clear()
    {
        listWidget_->clear();
    }

    void CornerMenu::addButton(MenuButtonType _button)
    {
        addButton(_button, getIconPath(_button, IconStyle::Path), getIconPath(_button, IconStyle::Filled));
    }

    void CornerMenu::addButton(MenuButtonType _type, QStringView _iconNormal, QStringView _iconActive)
    {
        if (hasButtonOfType(_type))
            return;

        auto item = new CornerMenuButton(_type, this);
        item->setIcon(_iconNormal);
        item->setActiveIcon(_iconActive);

        Testing::setAccessibleName(item, u"AS CornerMenu " % getAccessibleName(_type));

        listWidget_->addItem(item);
        setFixedWidth(getMenuWidth(listWidget_->count()) + shadowOffset() * 2);
    }

    QRect CornerMenu::getButtonRect(MenuButtonType _type) const
    {
        if (auto button = getButton(_type))
        {
            auto g = button->geometry();
            g.moveTopLeft(button->mapToGlobal({}));
            return g;
        }

        return QRect();
    }

    void CornerMenu::showAnimated()
    {
        if (isVisible() || !hasButtons())
            return;

        listWidget_->clearSelection();

        animate(QAbstractAnimation::Forward);
        show();
        raise();
    }

    void CornerMenu::hideAnimated()
    {
        if (!isVisible() || forceVisible_)
            return;

        animate(QAbstractAnimation::Backward);
    }

    void CornerMenu::forceHide()
    {
        hide();
        Q_EMIT hidden(QPrivateSignal());
    }

    void CornerMenu::setForceVisible(bool _forceVisible)
    {
        forceVisible_ = _forceVisible;
        if (forceVisible_)
            showAnimated();
    }

    void CornerMenu::setButtonSelected(MenuButtonType _type, bool _selected)
    {
        listWidget_->clearSelection();

        if (_selected)
        {
            const auto idx = listWidget_->indexOf(ButtonPredicate(_type));
            if (listWidget_->isValidIndex(idx))
                listWidget_->setCurrentIndex(idx);
        }
    }

    int CornerMenu::buttonsCount() const
    {
        return listWidget_->count();
    }

    bool CornerMenu::hasButtons() const
    {
        return buttonsCount() > 0;
    }

    bool CornerMenu::hasButtonOfType(MenuButtonType _type) const
    {
        return getButton(_type) != nullptr;
    }

    void CornerMenu::updateHoverState()
    {
        listWidget_->updateHoverState();
    }

    QSize CornerMenu::calcSize(int _buttonCount)
    {
        return { getMenuWidth(_buttonCount), getMenuHeight() };
    }

    int CornerMenu::shadowOffset()
    {
        return Utils::scale_value(4);
    }

    void CornerMenu::initAnimation()
    {
        if (animation_)
            return;

        animation_ = new QVariantAnimation(this);
        animation_->setStartValue(0.0);
        animation_->setEndValue(1.0);
        animation_->setEasingCurve(QEasingCurve::InOutSine);
        animation_->setDuration(MessageStyle::getHiddenControlsAnimationTime().count());
        connect(animation_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
        {
            opacityEffect_->setOpacity(value.toDouble());
        });
        connect(animation_, &QVariantAnimation::finished, this, [this]()
        {
            if (animation_->direction() == QAbstractAnimation::Backward)
                forceHide();
        });
    }

    void CornerMenu::animate(QAbstractAnimation::Direction _dir)
    {
        const auto isRunning = animation_->state() == QAbstractAnimation::Running;
        if (isRunning && animation_->direction() == _dir)
            return;

        const auto cur = isRunning ? animation_->currentTime() : 0.;
        animation_->stop();
        animation_->setCurrentTime(animation_->totalDuration() - cur);
        animation_->setDirection(_dir);
        animation_->start();
    }

    void CornerMenu::onItemClicked(int _idx)
    {
        if (!listWidget_->isValidIndex(_idx))
            return;

        if (auto btn = qobject_cast<CornerMenuButton*>(listWidget_->itemAt(_idx)))
            Q_EMIT buttonClicked(btn->getType(), QPrivateSignal());
    }

    void CornerMenu::enterEvent(QEvent* _e)
    {
        if (animation_->state() == QAbstractAnimation::Running)
        {
            animation_->stop();
            opacityEffect_->setOpacity(1.0);
        }

        QWidget::enterEvent(_e);
    }

    void CornerMenu::leaveEvent(QEvent* _e)
    {
        Q_EMIT mouseLeft(QPrivateSignal());
        QWidget::leaveEvent(_e);
    }

    void CornerMenu::mousePressEvent(QMouseEvent* _e)
    {
        _e->accept();
    }

    void CornerMenu::mouseReleaseEvent(QMouseEvent* _e)
    {
        _e->accept();
    }

    void CornerMenu::wheelEvent(QWheelEvent* _e)
    {
        Q_EMIT mouseLeft(QPrivateSignal());
        QWidget::wheelEvent(_e);
    }

    CornerMenuButton* CornerMenu::getButton(MenuButtonType _type) const
    {
        const auto idx = listWidget_->indexOf(ButtonPredicate(_type));
        const auto item = listWidget_->itemAt(idx);
        return qobject_cast<CornerMenuButton*>(item);
    }
}