#include "stdafx.h"
#include "UserMiniProfile.h"

#include "utils/utils.h"
#include "fonts.h"
#include "styles/ThemeParameters.h"
#include "styles/StyleSheetContainer.h"
#include "styles/StyleSheetGenerator.h"
#include "main_window/history_control/MessageStyle.h"
#include "cache/avatars/AvatarStorage.h"
#include "utils/InterConnector.h"
#include "utils/features.h"
#include "controls/TextWidget.h"
#include "main_window/containers/LastseenContainer.h"
#include "main_window/containers/StatusContainer.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/containers/FriendlyContainer.h"
#include "statuses/StatusUtils.h"

namespace
{
    QPoint getAvatarPos() noexcept
    {
        return Utils::scale_value(QPoint(0, 4));
    }

    int getAvatarSize() noexcept
    {
        return Utils::scale_value(48);
    }

    int getStatusOffset() noexcept
    {
        return Utils::scale_value(4);
    }

    int getTextHorOffset() noexcept
    {
        return Utils::scale_value(8);
    }

    int getTextTopOffset() noexcept
    {
        return Utils::scale_value(4);
    }

    int getTextSpacing() noexcept
    {
        return Utils::scale_value(2);
    }

    int getVerOffset() noexcept
    {
        return Utils::scale_value(12);
    }

    QPoint getNamePos() noexcept
    {
        return QPoint(getAvatarSize() + getTextHorOffset(), Utils::scale_value(4));
    }

    QPoint getUnderNamePos() noexcept
    {
        return QPoint(getAvatarSize() + getTextHorOffset(), Utils::scale_value(24));
    }

    QPoint getMessagePos() noexcept
    {
        return QPoint(0, getAvatarSize() + getVerOffset());
    }

    int getButtonHeight() noexcept
    {
        return Utils::scale_value(32);
    }

    int getButtonOffset() noexcept
    {
        return Utils::scale_value(8);
    }

    QSize getArrowButtonPixmapSize() noexcept
    {
        return Utils::scale_value(QSize(20, 20));
    }

    QRect getClickArea(const QRect& _contentRect)
    {
        return QRect(_contentRect.topLeft(), QSize(_contentRect.width(), getAvatarSize()));
    }

    QFont getNameFont()
    {
        return Fonts::adjustedAppFont(Features::isAppsNavigationBarVisible() ? 15 : 16);
    }

    QFont getUnderNameFont()
    {
        return Fonts::adjustedAppFont(Features::isAppsNavigationBarVisible() ? 12 : 13);
    }

    QFont getDescriptionFont()
    {
        return Fonts::adjustedAppFont(Features::isAppsNavigationBarVisible() ? 14 : 15);
    }

    QFont getButtonFont()
    {
        return Fonts::adjustedAppFont(14, Fonts::FontWeight::SemiBold);
    }

    auto getNameColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
    }

    auto getUnderNameColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_PASTEL };
    }

    auto getStatusColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
    }

    auto getDescriptionColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
    }

    auto avatarOverlayColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::GHOST_PRIMARY);
    }

    enum class ButtonState
    {
        Normal,
        Hovered,
        Active
    };

    Styling::ThemeColorKey getButtonBackgroundColor(ButtonState _state, bool _outgoing)
    {
        if (_outgoing)
        {
            switch (_state)
            {
            case ButtonState::Hovered:
                return Styling::ThemeColorKey { Styling::StyleVariable::PRIMARY_BRIGHT_HOVER };
            case ButtonState::Active:
                return Styling::ThemeColorKey { Styling::StyleVariable::PRIMARY_BRIGHT_ACTIVE };
            case ButtonState::Normal:
            default:
                return Styling::ThemeColorKey { Styling::StyleVariable::PRIMARY_BRIGHT };
            }
        }

        switch (_state)
        {
        case ButtonState::Hovered:
            return Styling::ThemeColorKey { Styling::StyleVariable::BASE_BRIGHT_HOVER };
        case ButtonState::Active:
            return Styling::ThemeColorKey { Styling::StyleVariable::BASE_BRIGHT_ACTIVE };
        case ButtonState::Normal:
        default:
            return Styling::ThemeColorKey { Styling::StyleVariable::BASE_BRIGHT };
        }
    }

    auto getButtonTextColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_INVERSE };
    }

    Styling::ThemeColorKey getSeparatorThemeKey() noexcept { return Styling::ThemeColorKey { Styling::StyleVariable::PRIMARY_PASTEL, 0.2 };}

    const QPixmap& getArrowButtonPixmap(bool _hovered, bool _active, bool _outgoing)
    {
        auto getPixmap = [](const Styling::StyleVariable _var) -> QPixmap
        {
            return Utils::mirrorPixmapHor(Utils::renderSvg(qsl(":/controls/back_icon"), getArrowButtonPixmapSize(), Styling::getParameters().getColor(_var)));
        };

        static QPixmap pixmap, pixmapHovered, pixmapActive;
        static QPixmap pixmapOutgoing, pixmapHoveredOutgoing, pixmapActiveOutgoing;

        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash() || pixmap.isNull())
        {
            pixmap = getPixmap(Styling::StyleVariable::BASE_SECONDARY);
            pixmapHovered = getPixmap(Styling::StyleVariable::BASE_SECONDARY_HOVER);
            pixmapActive = getPixmap(Styling::StyleVariable::BASE_SECONDARY_ACTIVE);
            pixmapOutgoing = getPixmap(Styling::StyleVariable::PRIMARY_PASTEL);
            pixmapHoveredOutgoing = getPixmap(Styling::StyleVariable::TEXT_PRIMARY_HOVER);
            pixmapActiveOutgoing = getPixmap(Styling::StyleVariable::TEXT_PRIMARY_ACTIVE);
        }

        if (_outgoing)
        {
            if (_active)
                return pixmapActiveOutgoing;
            else if (_hovered)
                return pixmapHoveredOutgoing;
            return pixmapOutgoing;
        }
        else
        {
            if (_active)
                return pixmapActive;
            else if (_hovered)
                return pixmapHovered;
            return pixmap;
        }
    }

    auto separatorStyleSheet()
    {
        return std::make_unique<Styling::ArrayStyleSheetGenerator>(ql1s("background: %1;"), std::vector<Styling::ThemeColorKey>{ getSeparatorThemeKey() });
    }

    QMargins getNameWidgetMarigns(bool _centered = false)
    {
        static const auto leftMargin = getAvatarSize() + getTextHorOffset();
        static const auto rightMargin = getArrowButtonPixmapSize().width() + getTextHorOffset();
        if (_centered)
            return QMargins(leftMargin, 0, rightMargin, 0);
        return QMargins(leftMargin, getTextTopOffset(), rightMargin, 0);
    }
}

namespace Ui
{
    UserMiniProfile::UserMiniProfile(QWidget* _parent, const QString _aimId, MiniProfileFlags _flags)
        : QWidget(_parent)
        , aimId_(_aimId)
        , separatorWidget_(nullptr)
        , separator_(nullptr)
        , flags_(_flags)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMouseTracking(true);

        nameUnit_ = new TextWidget(this, QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        statusUnit_ = new TextWidget(this, QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        statusUnit_->setFixedHeight(getButtonHeight());
        lastSeenUnit_ = new TextWidget(this, QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        statusLifeTimeUnit_ = new TextWidget(this, QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        underNameUnit_ = new TextWidget(this, QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        descriptionUnit_ = new TextWidget(this, QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        for (auto w : { nameUnit_, underNameUnit_, descriptionUnit_ })
            w->setAttribute(Qt::WA_TransparentForMouseEvents);

        quoteMarginWidget_ = new QWidget(this);
        quoteMarginWidget_->setFixedSize(1, MessageStyle::Files::getVerMargin());
        quoteMarginWidget_->hide();

        auto mainLayout = Utils::emptyVLayout(this);

        nameWidget_ = new QWidget(this);
        nameWidget_->setMouseTracking(true);
        nameWidget_->setContentsMargins(getNameWidgetMarigns());
        auto nameWidgetLayout = Utils::emptyVLayout(nameWidget_);
        nameWidgetLayout->setAlignment(Qt::AlignTop);
        nameWidgetLayout->setSpacing(getTextSpacing());
        nameWidgetLayout->addWidget(nameUnit_);
        nameWidgetLayout->addWidget(underNameUnit_);
        nameWidgetLayout->addWidget(lastSeenUnit_);
        nameWidgetLayout->addWidget(statusUnit_);
        nameWidgetLayout->addWidget(statusLifeTimeUnit_);

        if (isExtended())
        {
            separatorWidget_ = new QWidget(this);
            separatorWidget_->setContentsMargins(0, getVerOffset(), 0, 0);
            separatorWidget_->setFixedHeight(getVerOffset() + Utils::scale_value(1));
            separatorWidget_->setAttribute(Qt::WA_TransparentForMouseEvents);
            auto separatorLayout = Utils::emptyVLayout(separatorWidget_);
            separator_ = new QWidget(this);
            separator_->setFixedHeight(Utils::scale_value(1));
            Styling::setStyleSheet(separator_, separatorStyleSheet(), Styling::StyleSheetPolicy::UseSetStyleSheet);
            separatorLayout->addWidget(separator_);
        }

        mainLayout->setAlignment(Qt::AlignTop);
        mainLayout->addWidget(quoteMarginWidget_);
        mainLayout->addWidget(nameWidget_);
        if (separatorWidget_)
            mainLayout->addWidget(separatorWidget_);
        mainLayout->addSpacing(getVerOffset());
        mainLayout->addWidget(descriptionUnit_);

        connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &UserMiniProfile::onAvatarChanged);
    }

    void UserMiniProfile::updateStatusText()
    {
        if (!Statuses::isStatusEnabled())
        {
            statusUnit_->setVisible(false);
            lastSeenUnit_->setVisible(false);
            statusLifeTimeUnit_->setVisible(false);
            return;
        }

        QString status;
        if (!Logic::GetLastseenContainer()->isBot(aimId_))
            status = Logic::GetStatusContainer()->getStatus(aimId_).getDescription();

        if (!status.isEmpty() && status != Logic::GetStatusContainer()->getStatus(aimId_).emptyDescription())
        {
            Utils::elideText(status, getDesiredWidth());
            statusUnit_->setText(status);
            statusUnit_->elide(getDesiredWidth());
            statusUnit_->setVisible(!status.isEmpty());

            const auto lastSeen = Logic::GetLastseenContainer()->getLastSeen(aimId_);
            lastSeenUnit_->setText(lastSeen.getStatusString());
            lastSeenUnit_->setVisible(!lastSeenUnit_->getText().isEmpty());
            statusLifeTimeUnit_->setText(Logic::GetStatusContainer()->getStatus(aimId_).getTimeString());
            statusLifeTimeUnit_->setVisible(!statusLifeTimeUnit_->getText().isEmpty());
        }
        else
        {
            statusUnit_->setVisible(false);
            lastSeenUnit_->setVisible(false);
            statusLifeTimeUnit_->setVisible(false);
        }
    }

    void UserMiniProfile::init(const QString& _sn, const QString& _name, const QString& _underName, const QString& _description, const QString& _buttonText)
    {
        loaded_ = true;
        aimId_ = _sn;
        name_ = _name;
        if (isExtended() && !Utils::isChat(aimId_) && !Logic::GetLastseenContainer()->isBot(aimId_))
            name_ = name_.replace(ql1c(' '), ql1c('\n'));

        nameUnit_->setText(name_);
        updateStatusText();
        underNameText_ = _underName;
        underNameUnit_->setText(_underName);
        descriptionUnit_->setText(_description);

        if (!isExtended() && !(flags_ & MiniProfileFlag::isTooltip))
        {
            button_ = std::make_unique<BLabel>();
            auto buttonUnit = TextRendering::MakeTextUnit(_buttonText);
            button_->setTextUnit(std::move(buttonUnit));
            button_->background_ = getButtonBackgroundColor(ButtonState::Normal, useAccentColors());
            button_->hoveredBackground_ = getButtonBackgroundColor(ButtonState::Hovered, useAccentColors());
            button_->pressedBackground_ = getButtonBackgroundColor(ButtonState::Active, useAccentColors());
            button_->setBorderRadius(Utils::scale_value(8));
            button_->setVerticalPosition(TextRendering::VerPosition::MIDDLE);
            button_->setYOffset(getButtonHeight() / 2);
        }

        initTextUnits();
        updateSize();
        loadAvatar();
    }

    void UserMiniProfile::updateFlags(bool _isStandalone, bool _isInQuote)
    {
        flags_.setFlag(MiniProfileFlag::isStandalone, _isStandalone);
        flags_.setFlag(MiniProfileFlag::isInsideQuote, _isInQuote);
    }

    void UserMiniProfile::initTextUnits()
    {
        TextRendering::TextUnit::InitializeParameters params{ getNameFont(), getNameColor() };
        params.maxLinesCount_ = Utils::isChat(aimId_) || !isExtended() ? 1 : -1;
        nameUnit_->init(params);
        params.setFonts(getUnderNameFont());
        params.color_ = getStatusColor();
        statusUnit_->init(params);
        lastSeenUnit_->init(params);
        params.color_ = getUnderNameColor();
        params.maxLinesCount_ = 1;
        underNameUnit_->init(params);
        statusLifeTimeUnit_->init(params);
        if (button_)
        {
            params.setFonts(getButtonFont());
            params.color_ = getButtonTextColor();
            params.align_ = TextRendering::HorAligment::CENTER;
            button_->initTextUnit(params);
        }
        params.setFonts(getDescriptionFont());
        params.color_ = getDescriptionColor();
        params.lineBreak_ = TextRendering::LineBreakType::PREFER_SPACES;
        params.align_ = TextRendering::HorAligment::LEFT;
        params.maxLinesCount_ = isExtended() ? -1 : 2;
        params.selectionColor_ = MessageStyle::getTextSelectionColorKey();
        descriptionUnit_->init(params);
    }

    void UserMiniProfile::updateSize(int _width)
    {
        if (_width != -1)
            setFixedWidth(_width);
        else
            setFixedWidth(getDesiredWidth());

        const auto maxWidth = width() - getAvatarSize() - getTextHorOffset() - (hasArrow() ? getArrowButtonPixmapSize().width() + getTextHorOffset() : 0);
        nameUnit_->setMaxWidthAndResize(maxWidth);
        statusUnit_->setMaxWidthAndResize(maxWidth);
        lastSeenUnit_->setMaxWidthAndResize(maxWidth);
        statusLifeTimeUnit_->setMaxWidthAndResize(maxWidth);
        underNameUnit_->setMaxWidthAndResize(maxWidth);
        descriptionUnit_->setMaxWidthAndResize(width());

        const auto margins = nameWidget_->contentsMargins();
        auto statusHeight = 0;
        if (Statuses::isStatusEnabled() && !Logic::GetStatusContainer()->getStatus(aimId_).isEmpty())
        {
            statusHeight += statusUnit_->height();
            statusHeight += lastSeenUnit_->height();
            statusHeight += statusLifeTimeUnit_->height();
            statusHeight += margins.top() + margins.bottom();
        }
        nameWidget_->setFixedSize(width(), std::max(nameUnit_->height() + underNameUnit_->height() + statusHeight + margins.top() + margins.bottom() + getTextSpacing(),
                                                    getAvatarSize() + getStatusOffset() + getAvatarPos().y()));
        const auto centerNames = loaded_ && descriptionUnit_->isEmpty();
        nameWidget_->setContentsMargins(getNameWidgetMarigns(centerNames));
        if (auto layout = nameWidget_->layout())
            layout->setAlignment(centerNames ? Qt::AlignVCenter : Qt::AlignTop);

        auto height = std::max(nameWidget_->height(), getAvatarSize());

        if (quoteMarginWidget_ && quoteMarginWidget_->isVisible())
            height += quoteMarginWidget_->height();

        if (loaded_ && !descriptionUnit_->isEmpty())
        {
            if (separatorWidget_)
                height += separatorWidget_->height() + getVerOffset();
            height += descriptionUnit_->height();
        }

        if (!isInsideQuote() && button_)
            height += getButtonOffset() + getButtonHeight() + getVerOffset();

        if (!isStandalone())
            height += MessageStyle::Files::getVerMargin() * 2;

        setFixedHeight(height);
    }

    void UserMiniProfile::loadAvatar()
    {
        auto isDefault = false;
        avatar_ = Logic::GetAvatarStorage()->GetRounded(aimId_, name_, Utils::scale_bitmap(getAvatarSize()), Out isDefault, false, false);
    }

    void UserMiniProfile::onAvatarChanged(const QString& _aimId)
    {
        if (aimId_ != _aimId || aimId_.isEmpty())
            return;

        loadAvatar();
        update();
    }

    void UserMiniProfile::mouseMoveEvent(QMouseEvent* _e)
    {
        if (loaded_)
        {
            const auto pos = _e->pos();
            auto clickAreaHovered = getClickArea().contains(pos);
            auto buttonHovered = button_ && button_->rect().contains(pos);
            auto isButtonHoveredNow = button_ && button_->hovered();

            auto needUpdate = buttonHovered != isButtonHoveredNow || clickAreaHovered != clickAreaHovered_;

            if (button_)
                button_->setHovered(buttonHovered);

            clickAreaHovered_ = clickAreaHovered;

            if (Features::longPathTooltipsAllowed() && nameUnit_->isElided() && nameUnit_->geometry().contains(pos))
                Q_EMIT showNameTooltip(nameUnit_->getText(), nameUnit_->geometry());
            else
                Q_EMIT hideNameTooltip();

            setCursor(clickAreaHovered_ || (button_ && button_->hovered()) ? Qt::PointingHandCursor : Qt::ArrowCursor);

            if (needUpdate)
                update();
        }
        QWidget::mouseMoveEvent(_e);
    }

    void UserMiniProfile::updateFonts()
    {
        if (loaded_)
            initTextUnits();
    }

    QRect UserMiniProfile::getClickArea() const
    {
        return QRect(0, 0, width(), std::max(getAvatarSize() + getAvatarPos().y(), nameWidget_->height()));
    }

    void UserMiniProfile::pressed(const QPoint& _p, bool _hasMultiselect)
    {
        clickAreaPressed_ = getClickArea().contains(_p);

        if (button_ && !_hasMultiselect)
            button_->setPressed(button_->rect().contains(_p));

        update();
    }

    bool UserMiniProfile::clicked(const QPoint& _p, bool _hasMultiselect)
    {
        auto clickHandled = false;

        if (getClickArea().contains(_p) && !_hasMultiselect)
        {
            Q_EMIT onClickAreaClicked();
            clickHandled = true;
        }

        if (button_ && button_->rect().contains(_p) && !_hasMultiselect)
        {
            Q_EMIT onButtonClicked();
            clickHandled = true;
        }

        update();

        return clickHandled;
    }

    void UserMiniProfile::mouseReleaseEvent(QMouseEvent* _event)
    {
        clickAreaPressed_ = false;

        if (button_)
            button_->setPressed(false);

        update();
        QWidget::mouseReleaseEvent(_event);
    }

    void UserMiniProfile::leaveEvent(QEvent* _event)
    {
        if (button_)
            button_->setHovered(false);

        clickAreaHovered_ = false;

        setCursor(Qt::ArrowCursor);
        Q_EMIT hideNameTooltip();

        update();
        QWidget::leaveEvent(_event);
    }

    void UserMiniProfile::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);
        auto contentSize = size();
        const auto messagePos = getMessagePos();

        quoteMarginWidget_->setVisible(!isStandalone());
        if (separatorWidget_)
            separatorWidget_->setVisible(loaded_ && !descriptionUnit_->isEmpty());
        if (!isStandalone())
        {
            p.setTransform(QTransform().translate(0, MessageStyle::Files::getVerMargin()));
            contentSize.setWidth(width());
            contentSize.setHeight(contentSize.height() - MessageStyle::Files::getVerMargin() * 2);
        }

        if (loaded_)
        {
            if (!isInsideQuote() && button_)
            {
                button_->setRect(QRect(messagePos.x(), descriptionUnit_->geometry().bottom() + getButtonOffset(), contentSize.width(), getButtonHeight()));
                button_->draw(p);
            }

            if (hasArrow())
            {
                const auto pixmapSize = getArrowButtonPixmapSize();
                p.drawPixmap(contentSize.width() - pixmapSize.width(), getTextTopOffset() + getTextSpacing(), getArrowButtonPixmap(clickAreaHovered_, clickAreaPressed_, useAccentColors()));
            }

            const auto statusBadge = Utils::getStatusBadge(aimId_, getAvatarSize());
            Utils::StatusBadgeFlags statusFlags;
            const auto isMuted = Logic::getContactListModel()->isMuted(aimId_);
            const auto isOfficial = !isMuted && Logic::GetFriendlyContainer()->getOfficial(aimId_);
            const auto isOnline = Logic::GetLastseenContainer()->isOnline(aimId_);

            statusFlags.setFlag(Utils::StatusBadgeFlag::Official, isOfficial);
            statusFlags.setFlag(Utils::StatusBadgeFlag::Muted, isMuted);
            statusFlags.setFlag(Utils::StatusBadgeFlag::Online, isOnline);

            Utils::drawAvatarWithBadge(p, QPoint(getAvatarPos().x(), getAvatarPos().y()), avatar_, statusBadge, statusFlags);
       }
        else
        {
            QPainterPath path;

            const auto namePos = getNamePos();
            const auto underNamePos = getUnderNamePos();
            const auto textBlockHeight = Utils::scale_value(18);

            path.addRect(namePos.x(), namePos.y(), Utils::scale_value(100), textBlockHeight);
            path.addRect(underNamePos.x(), underNamePos.y(), Utils::scale_value(60), textBlockHeight);
            path.addRect(messagePos.x(), messagePos.y(), contentSize.width() - Utils::scale_value(40), textBlockHeight);
            path.addRect(QRect(messagePos.x(), messagePos.y() + textBlockHeight + getButtonOffset(), contentSize.width(), getButtonHeight()));

            path.addEllipse(getAvatarPos().x(), getAvatarPos().y(), getAvatarSize(), getAvatarSize());
            p.fillPath(path, Styling::getParameters().getColor(Styling::StyleVariable::GHOST_QUATERNARY));
        }
    }

    bool UserMiniProfile::useAccentColors() const
    {
        return (flags_ & MiniProfileFlag::isOutgoing);
    }

    bool UserMiniProfile::isStandalone() const
    {
        return (flags_ & MiniProfileFlag::isStandalone);
    }

    bool UserMiniProfile::isInsideQuote() const
    {
        return (flags_ & MiniProfileFlag::isInsideQuote);
    }

    bool UserMiniProfile::hasArrow() const
    {
        return !(flags_ & MiniProfileFlag::isTooltip);
    }

    bool UserMiniProfile::isExtended() const
    {
        return Features::isAppsNavigationBarVisible();
    }

    int UserMiniProfile::getMaximumWidth() const
    {
        auto maxUnitLength = std::max(nameUnit_->getDesiredWidth(), underNameUnit_->getDesiredWidth());
        maxUnitLength = std::max(statusUnit_->getDesiredWidth(), maxUnitLength);
        maxUnitLength = std::max(lastSeenUnit_->getDesiredWidth(), maxUnitLength);
        maxUnitLength = std::max(statusLifeTimeUnit_->getDesiredWidth(), maxUnitLength);
        maxUnitLength = std::max(descriptionUnit_->getDesiredWidth(), maxUnitLength);
        return maxUnitLength + getAvatarSize() + getTextHorOffset();
    }

    int UserMiniProfile::getDesiredWidth()
    {
        auto nameWidgetDesiredWidth = std::min(std::max(nameUnit_->getDesiredWidth(), underNameUnit_->getDesiredWidth()) + getAvatarSize() + getTextHorOffset(), getMaximumWidth());
        if (hasArrow())
            nameWidgetDesiredWidth += getTextHorOffset() + getArrowButtonPixmapSize().width();
        nameWidget_->setFixedWidth(nameWidgetDesiredWidth);

        return std::min(std::max(nameWidgetDesiredWidth, descriptionUnit_->isEmpty() ? 0 : descriptionUnit_->getDesiredWidth()), getMaximumWidth());
    }

    void UserMiniProfile::selectDescription(QPoint _from, QPoint _to)
    {
        const auto localFrom = descriptionUnit_->mapFromGlobal(_from);
        const auto localTo = descriptionUnit_->mapFromGlobal(_to);
        descriptionUnit_->select(localFrom, localTo);
    }

    Data::FString UserMiniProfile::getSelectedDescription()
    {
        return descriptionUnit_->getSelectedText(TextRendering::TextType::SOURCE);
    }

    bool UserMiniProfile::isSelected()
    {
        return descriptionUnit_->isSelected();
    }

    void UserMiniProfile::clearSelection()
    {
        descriptionUnit_->clearSelection();
    }

    void UserMiniProfile::releaseSelection()
    {
        descriptionUnit_->releaseSelection();
    }

    void UserMiniProfile::doubleClicked(const QPoint& _p, std::function<void(bool)> _callback)
    {
        const auto offset = quoteMarginWidget_->height() + nameWidget_->height() + separatorWidget_->height() + getVerOffset();
        const auto mappedPoint = _p - QPoint(0, offset);
        descriptionUnit_->doubleClicked(mappedPoint, true, _callback);
    }
}
