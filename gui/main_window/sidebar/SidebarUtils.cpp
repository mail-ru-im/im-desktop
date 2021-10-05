#include "stdafx.h"

#include "Sidebar.h"
#include "SidebarUtils.h"
#include "ImageVideoList.h"

#include "../../core_dispatcher.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/SwitcherCheckbox.h"
#include "../../controls/CheckboxList.h"
#include "controls/TransparentScrollBar.h"
#include "../../fonts.h"
#include "../../app_config.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/features.h"
#include "../../utils/stat_utils.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/FavoritesUtils.h"
#include "../../styles/ThemesContainer.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../main_window/containers/FriendlyContainer.h"
#include "../../main_window/containers/LastseenContainer.h"
#include "../../main_window/containers/StatusContainer.h"
#include "../../main_window/MainWindow.h"
#include "../../main_window/GroupChatOperations.h"
#include "../../main_window/contact_list/ChatMembersModel.h"
#include "../../main_window/contact_list/ContactListItemDelegate.h"
#include "../../main_window/contact_list/ContactListUtils.h"
#include "../../main_window/history_control/MessageStyle.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/GeneralDialog.h"
#include "../../controls/TooltipWidget.h"
#include "controls/BigEmojiWidget.h"
#include "statuses/StatusTooltip.h"

#include "../../utils/animations/WidgetFader.h"
#include "../../utils/Text2DocConverter.h"
#include "../../styles/ThemeParameters.h"
#include "previewer/toast.h"
#include "types/chat.h"

namespace
{
    constexpr int EDIT_AVATAR_SIZE = 100;
    constexpr int EDIT_FONT_SIZE = 16;
    constexpr int EDIT_HOR_OFFSET = 20;
    constexpr int EDIT_VER_OFFSET = 24;
    constexpr int EDIT_MAX_ENTER_HEIGHT = 100;

    constexpr auto HOR_OFFSET = 16;
    constexpr auto BUTTON_HOR_OFFSET = 20;
    constexpr auto AVATAR_NAME_OFFSET = 12;
    constexpr auto ICON_SIZE = 20;
    constexpr auto ICON_OFFSET = 4;
    constexpr auto AVATAR_INFO_HEIGHT = 84;
    constexpr auto INFO_HEADER_BOTTOM_MARGIN = 4;
    constexpr auto INFO_TEXT_BOTTOM_MARGIN = 12;
    constexpr auto BUTTON_HEIGHT = 36;
    constexpr auto COLORED_BUTTON_HEIGHT = 40;
    constexpr auto BUTTON_ICON_SIZE = 20;
    constexpr auto GALLERY_PREVIEW_SIZE = 52;
    constexpr auto GALLERY_PREVIEW_COUNT = 5;
    constexpr auto GALLERY_WIDGET_HEIGHT = 80;
    constexpr auto GALLERY_WIDGET_MARGIN_LEFT = HOR_OFFSET;
    constexpr auto GALLERY_WIDGET_MARGIN_TOP = 8;
    constexpr auto GALLERY_WIDGET_MARGIN_RIGHT = HOR_OFFSET;
    constexpr auto GALLERY_WIDGET_MARGIN_BOTTOM = 20;
    constexpr auto GALLERY_WIDGET_SPACING = 8;
    constexpr auto MEMBERS_LEFT_MARGIN = HOR_OFFSET;
    constexpr auto MEMBERS_RIGHT_MARGIN = HOR_OFFSET;
    constexpr auto MEMBERS_TOP_MARGIN = 6;
    constexpr auto MEMBERS_BOTTOM_MARGIN = 20;
    constexpr auto COLORED_BUTTON_TEXT_OFFSET = 8;
    constexpr auto COLORED_BUTTON_BOTTOM_OFFSET = 20;
    constexpr auto MAX_INFO_LINES_COUNT = 4;
    constexpr auto SPACER_HEIGHT = 8;
    constexpr auto SPACER_MARGIN = 8;
    constexpr auto CLICKABLE_AVATAR_INFO_OFFSET = 8;
    constexpr auto NAME_OFFSET = 2;
    constexpr auto MEMBERS_WIDGET_BOTTOM_OFFSET = 8;

    constexpr int BLOCK_AND_DELETE_HOR_OFFSET = 16;
    constexpr int BLOCK_AND_DELETE_TOP_OFFSET = 16;
    constexpr int BLOCK_AND_DELETE_ADD_OFFSET = 6;
    constexpr int BLOCK_AND_DELETE_BOTTOM_OFFSET = 16;

    QMap<QString, QVariant> makeData(const QString& _command, qint64 _msg, const QString& _link, const QString& _sender, time_t _time)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = _command;
        result[qsl("msg")] = _msg;
        result[qsl("link")] = _link;
        result[qsl("sender")] = _sender;
        result[qsl("time")] = (qlonglong)_time;
        return result;
    }

    int nameOffsetTop()
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(14);
        else
            return Utils::scale_value(12);
    }

    int statusTopOffset()
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(2);
        else
            return Utils::scale_value(4);
    }

    int botStatusTopOffset()
    {
        return Utils::scale_value(8);
    }

    int statusImageSize()
    {
        return Utils::scale_value(16);
    }

    int statusSideMargin()
    {
        return Utils::scale_value(8);
    }

    int statusImageSideMargin()
    {
        return Utils::scale_value(2);
    }

    QColor statusBackgroundColor(bool _hovered, bool _pressed)
    {
        if (_pressed)
            return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_ACTIVE);
        else if (_hovered)
            return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_HOVER);
        else
            return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
    }

    int statusBottomMargin()
    {
        return Utils::scale_value(12);
    }

    int statusPopupWidth()
    {
        return Utils::scale_value(356);
    }

    int statusPopupTextSideMargin()
    {
        return Utils::scale_value(16);
    }

    int statusPopupElapsedTextTopMargin()
    {
        return Utils::scale_value(6);
    }

    int statusPopupTextMaxWidth()
    {
        return statusPopupWidth() - 2 * statusPopupTextSideMargin();
    }

    int statusPopupButtonsGap()
    {
        return Utils::scale_value(8);
    }

    QColor statusPopupShadowColor()
    {
        return QColor(0, 0, 0, 255 * 0.3);
    }
}

namespace Ui
{
    SidebarButton::SidebarButton(QWidget* _parent)
        : QWidget(_parent)
        , textOffset_(0)
        , isHovered_(false)
        , isActive_(false)
        , isEnabled_(true)
    {
        text_ = TextRendering::MakeTextUnit(QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        counter_ = TextRendering::MakeTextUnit(QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);

        setCursor(Qt::PointingHandCursor);
    }

    void SidebarButton::setMargins(const QMargins& _margins)
    {
        if (margins_ == _margins)
            return;
        margins_ = _margins;
        update();
    }

    void SidebarButton::setTextOffset(int _textOffset)
    {
        if (textOffset_ == _textOffset)
            return;

        textOffset_ = _textOffset;
        update();
    }

    void SidebarButton::setIcon(const QPixmap& _icon)
    {
        icon_ = _icon;
        update();
    }

    void SidebarButton::initText(const QFont& _font, const QColor& _textColor, const QColor& _disabledColor)
    {
        text_->init(_font, _textColor, QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);

        textColor_ = _textColor;
        disabledColor_ = _disabledColor;
    }

    void SidebarButton::setText(const QString& _text)
    {
        if (label_ == _text)
            return;

        label_ = _text;
        text_->setText(_text);
        update();
    }

    void SidebarButton::initCounter(const QFont& _font, const QColor& _textColor)
    {
        counter_->init(_font, _textColor);
    }

    void SidebarButton::setCounter(int _count, bool _autoHide)
    {
        const QString text = QString::number(_count);
        if (counter_->getText() == text)
            return;

        counter_->setText(text);
        if (_autoHide)
            setVisible(_count > 0);
        update();
    }

    void SidebarButton::setColors(const QColor& _hover, const QColor& _active_)
    {
        hover_ = _hover;
        active_ = _active_;
        update();
    }

    void SidebarButton::setEnabled(bool _isEnabled)
    {
        if (isEnabled_ == _isEnabled)
            return;

        isEnabled_ = _isEnabled;
        setCursor(isEnabled_ ? Qt::PointingHandCursor : Qt::ArrowCursor);
        if (!_isEnabled)
        {
            isActive_ = false;
            isHovered_ = false;
        }

        text_->setColor(_isEnabled ? textColor_ : disabledColor_);

        update();
    }

    void SidebarButton::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);

        if (isEnabled_)
        {
            if (isActive() && active_.isValid())
                p.fillRect(rect(), active_);
            else if (isHovered() && hover_.isValid())
                p.fillRect(rect(), hover_);
        }

        if (!icon_.isNull())
            p.drawPixmap(margins_.left(), height() / 2 - icon_.height() / 2 / Utils::scale_bitmap(1), icon_);

        text_->setOffsets(margins_.left() + icon_.width() / Utils::scale_bitmap(1) + textOffset_, height() / 2);
        text_->draw(p, TextRendering::VerPosition::MIDDLE);

        counter_->setOffsets(width() - margins_.right() - counter_->cachedSize().width(), height() / 2);
        counter_->draw(p, TextRendering::VerPosition::MIDDLE);
    }

    void SidebarButton::resizeEvent(QResizeEvent* _event)
    {
        text_->getHeight(width() - margins_.left() - margins_.right() - icon_.width() - textOffset_ - counter_->cachedSize().width());
        QWidget::resizeEvent(_event);
    }

    void SidebarButton::mousePressEvent(QMouseEvent* _event)
    {
        if (isEnabled_)
        {
            clickedPoint_ = _event->pos();
            isActive_ = true;
            update();
        }
        QWidget::mousePressEvent(_event);
    }

    void SidebarButton::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (isEnabled_)
        {
            if (Utils::clicked(clickedPoint_, _event->pos()))
                Q_EMIT clicked(QPrivateSignal());
            isActive_ = false;
            update();
        }
        QWidget::mouseReleaseEvent(_event);
    }

    void SidebarButton::enterEvent(QEvent* _event)
    {
        if (isEnabled_)
        {
            isHovered_ = true;
            update();
        }
        QWidget::enterEvent(_event);
    }

    void SidebarButton::leaveEvent(QEvent* _event)
    {
        if (isEnabled_)
        {
            isHovered_ = false;
            update();
        }
        QWidget::leaveEvent(_event);
    }

    bool SidebarButton::isHovered() const
    {
        return isHovered_;
    }

    bool SidebarButton::isActive() const
    {
        return isActive_;
    }

    void SidebarButton::elideText(int _width)
    {
        text_->setText(label_);
        text_->elide(_width);
        text_->setText(text_->getText());
    }

    SidebarCheckboxButton::SidebarCheckboxButton(QWidget* _parent)
        : SidebarButton(_parent)
        , checkbox_(new SwitcherCheckbox(this))
    {
        checkbox_->setAttribute(Qt::WA_TransparentForMouseEvents);
        checkbox_->setFocusPolicy(Qt::NoFocus);
    }

    void SidebarCheckboxButton::setChecked(bool _checked)
    {
        if (checkbox_->isChecked() == _checked)
            return;

        checkbox_->setChecked(_checked);
        Q_EMIT checked(_checked, QPrivateSignal());
        update();
    }

    bool SidebarCheckboxButton::isChecked() const
    {
        return checkbox_->isChecked();
    }

    void SidebarCheckboxButton::setEnabled(bool _isEnabled)
    {
        SidebarButton::setEnabled(_isEnabled);
        checkbox_->setEnabled(_isEnabled);
    }

    void SidebarCheckboxButton::setCheckValidator(std::function<bool(bool)> _validatorFunc)
    {
        checkValidator_ = std::move(_validatorFunc);
    }

    void SidebarCheckboxButton::resizeEvent(QResizeEvent* _event)
    {
        checkbox_->move(width() - margins_.right() - checkbox_->width(), height() / 2 - checkbox_->height() / 2);
        elideText(width() - margins_.right() - margins_.left() - checkbox_->width()); // elide extra text
        SidebarButton::resizeEvent(_event);
    }

    void SidebarCheckboxButton::mousePressEvent(QMouseEvent* _event)
    {
        clickedPoint_ = _event->pos();
        SidebarButton::mousePressEvent(_event);
    }

    void SidebarCheckboxButton::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (Utils::clicked(clickedPoint_, _event->pos()))
        {
            if (isEnabled_)
            {
                const auto desired = !isChecked();
                const auto newValue = checkValidator_ ? checkValidator_(desired) : desired;
                setChecked(newValue);
            }
            else
            {
                Q_EMIT disabledClicked(QPrivateSignal());
            }
        }
        SidebarButton::mouseReleaseEvent(_event);
    }

    AvatarNameInfo::AvatarNameInfo(QWidget* _parent)
        : QWidget(_parent)
        , textOffset_(0)
        , animStep_(0)
        , avatarSize_(0)
        , defaultAvatar_(false)
        , clickable_(false)
        , hovered_(false)
        , nameOnly_(false)
        , statusPlate_(new StatusPlate(this))
        , tooltipTimer_(nullptr)
        , tooltipActivated_(false)
    {
        name_ = TextRendering::MakeTextUnit(QString(), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);
        info_ = TextRendering::MakeTextUnit(QString(), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS);

        connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &AvatarNameInfo::avatarChanged);
        connect(Logic::GetFriendlyContainer(), &Logic::FriendlyContainer::friendlyChanged, this, &AvatarNameInfo::friendlyChanged);
        connect(Logic::GetStatusContainer(), &Logic::StatusContainer::statusChanged, this, &AvatarNameInfo::statusChanged);
        connect(statusPlate_, &StatusPlate::update, this, qOverload<>(&AvatarNameInfo::update));

        setMouseTracking(true);

        setMinimumHeight(Utils::scale_value(AVATAR_INFO_HEIGHT));
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }

    void AvatarNameInfo::setMargins(const QMargins& _margins)
    {
        if (margins_ == _margins)
            return;
        margins_ = _margins;
        update();
    }

    void AvatarNameInfo::setTextOffset(int _textOffset)
    {
        if (textOffset_ == _textOffset)
            return;

        textOffset_ = _textOffset;
        update();
    }

    void AvatarNameInfo::initName(const QFont& _font, const QColor& _color)
    {
        name_->init(_font, _color);
    }

    void AvatarNameInfo::initInfo(const QFont& _font, const QColor& _color)
    {
        info_->init(_font, _color);
    }

    void AvatarNameInfo::setAimIdAndSize(const QString& _aimid, int _size, const QString& _friendly)
    {
        const QString friendlyName = _friendly.isEmpty() ? Logic::GetFriendlyContainer()->getFriendly(_aimid) : _friendly;
        if (avatarSize_ == _size && aimId_ == _aimid && friendlyName_ == friendlyName)
            return;

        aimId_ = _aimid;
        avatarSize_ = _size;
        friendlyName_ = friendlyName;
        name_->setText(friendlyName_);
        name_->elide(width() - margins_.left() - margins_.right() - avatarSize_ - textOffset_ - (clickable_ ? Utils::scale_value(ICON_SIZE + ICON_OFFSET) : 0));
        statusPlate_->setContactId(aimId_);
        setBadgeRect();
        loadAvatar();
        updateSize();

        Testing::setAccessibleName(this, qsl("AS Sidebar AvatarNameInfo ") + aimId_);
    }

    void AvatarNameInfo::setFrienlyAndSize(const QString& _friendly, int _size)
    {
        if (avatarSize_ == _size && friendlyName_ == _friendly)
            return;

        avatarSize_ = _size;
        friendlyName_ = _friendly;
        name_->setText(friendlyName_);
        name_->elide(width() - margins_.left() - margins_.right() - avatarSize_ - textOffset_ - (clickable_ ? Utils::scale_value(ICON_SIZE + ICON_OFFSET) : 0));

        avatar_ = Logic::GetAvatarStorage()->GetRounded(aimId_, friendlyName_, Utils::scale_bitmap(avatarSize_), defaultAvatar_, false, false);
        Utils::check_pixel_ratio(avatar_);
        setBadgeRect();
        update();
    }

    void AvatarNameInfo::setInfo(const QString& _info, const QColor& _color)
    {
        if (info_->getColor() == _color && info_->getText() == _info)
            return;

        info_->setText(_info, _color);
        name_->elide(width() - margins_.left() - margins_.right() - avatarSize_ - textOffset_ - (clickable_ ? Utils::scale_value(ICON_SIZE + ICON_OFFSET) : 0));
        updateSize();
        update();
    }

    const QString& AvatarNameInfo::getFriendly() const
    {
        return friendlyName_;
    }

    void AvatarNameInfo::makeClickable()
    {
        clickable_ = true;
        update();
    }

    void AvatarNameInfo::nameOnly()
    {
        nameOnly_ = true;
        update();
    }

    void AvatarNameInfo::stateChanged()
    {
        static constexpr std::chrono::milliseconds animationDuration = std::chrono::milliseconds(52);
        if (!isEnabled())
            timer_.start(int(animationDuration.count()), this);
        else
            timer_.stop();
    }

    QSize AvatarNameInfo::sizeHint() const
    {
        const auto statusTopLeft = statusPos();
        const auto statusBottom = statusTopLeft.y() + statusPlate_->height() + statusBottomMargin();
        const auto avatarHeight = Utils::scale_value(AVATAR_INFO_HEIGHT);
        return QSize(width(), std::max(statusBottom, avatarHeight));
    }

    QSize AvatarNameInfo::minimumSizeHint() const
    {
        return QSize(Utils::scale_value(AVATAR_INFO_HEIGHT), Utils::scale_value(AVATAR_INFO_HEIGHT));
    }

    void AvatarNameInfo::paintEnabled(QPainter& p)
    {
        p.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing | QPainter::TextAntialiasing);
        if (clickable_)
        {
            if (hovered_)
                p.fillRect(QRect(rect().x(), Utils::scale_value(CLICKABLE_AVATAR_INFO_OFFSET), width(), height() - Utils::scale_value(CLICKABLE_AVATAR_INFO_OFFSET) * 2), Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));

            static auto pixmap = Utils::mirrorPixmapHor(Utils::renderSvgScaled(qsl(":/controls/back_icon"), QSize(ICON_SIZE, ICON_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY)));
            p.drawPixmap(width() - Utils::scale_value(ICON_SIZE) - margins_.right(), height() / 2 - Utils::scale_value(ICON_SIZE) / 2, pixmap);
        }

        if (!avatar_.isNull())
            Utils::drawAvatarWithBadge(p, avatarTopLeft(), avatar_, aimId_, false, Utils::StatusBadgeState::BadgeOnly, false, false);

        const auto isBot = Logic::GetLastseenContainer()->isBot(aimId_);

        if (nameOnly_)
        {
            name_->setOffsets(margins_.left() + avatarSize_ + textOffset_, height() / 2);
        }
        if (!statusPlate_->isEmpty() && !isBot)
        {
            name_->setOffsets(margins_.left() + avatarSize_ + textOffset_, nameOffsetTop());
        }
        else
        {
            name_->setOffsets(margins_.left() + avatarSize_ + textOffset_, height() / 2 - name_->cachedSize().height() - Utils::scale_value(NAME_OFFSET) / 2);
        }

        name_->draw(p, nameOnly_ ? TextRendering::VerPosition::MIDDLE : TextRendering::VerPosition::TOP);

        if (!nameOnly_ && !isBot)
        {
            if (statusPlate_->isEmpty())
                info_->setOffsets(margins_.left() + avatarSize_ + textOffset_, height() / 2 + Utils::scale_value(NAME_OFFSET) / 2);
            else
                info_->setOffsets(margins_.left() + avatarSize_ + textOffset_, nameOffsetTop() + name_->cachedSize().height());

            info_->draw(p);
        }

        statusPlate_->draw(p);

        if (Q_UNLIKELY(Ui::GetAppConfig().IsShowMsgIdsEnabled()))
        {
            p.setPen(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
            p.setFont(Fonts::appFontScaled(10, Fonts::FontWeight::SemiBold));

            const auto x = width();
            const auto y = height();
            Utils::drawText(p, QPointF(x, y), Qt::AlignRight | Qt::AlignBottom, aimId_);
        }
    }

    void AvatarNameInfo::paintDisabled(QPainter& painter)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);

        QLinearGradient gradient(0, 0, width(), 0);
        gradient.setSpread(QGradient::RepeatSpread);
        gradient.setColorAt(0.0, Styling::getParameters().getColor(Styling::StyleVariable::BASE_LIGHT));
        gradient.setColorAt(animStep_ / 10.0, Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
        gradient.setColorAt(1.0, Styling::getParameters().getColor(Styling::StyleVariable::BASE_LIGHT));
        p.setBrush(gradient);

        const auto avatarPos = Utils::scale_value(16);
        const auto avatarSize = Utils::scale_value(52);
        p.drawEllipse(avatarPos, avatarPos, avatarSize, avatarSize);

        const auto lineX = avatarPos + avatarSize + Utils::scale_value(13);
        const auto radius = Utils::scale_value(2);

        p.drawRoundedRect(lineX, Utils::scale_value(25), Utils::scale_value(150), Utils::scale_value(12), radius, radius);
        p.drawRoundedRect(lineX, Utils::scale_value(47), Utils::scale_value(112), Utils::scale_value(8), radius, radius);
    }

    void AvatarNameInfo::timerEvent(QTimerEvent* _event)
    {
        if (_event->timerId() == timer_.timerId())
        {
            ++animStep_;
            animStep_ %= 10;
            update();
        }
        else
        {
            QWidget::timerEvent(_event);
        }
    }

    void AvatarNameInfo::changeEvent(QEvent* _event)
    {
        if (_event->type() == QEvent::EnabledChange)
            stateChanged();

        QWidget::changeEvent(_event);
    }

    void AvatarNameInfo::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        if (!isEnabled() && timer_.isActive())
            paintDisabled(p);
        else
            paintEnabled(p);
    }

    void AvatarNameInfo::resizeEvent(QResizeEvent* _event)
    {
        name_->elide(width() - margins_.left() - margins_.right() - avatarSize_ - textOffset_ - (clickable_ ? Utils::scale_value(ICON_SIZE + ICON_OFFSET): 0));
        info_->elide(width() - margins_.left() - margins_.right() - avatarSize_ - textOffset_);

        const auto pos = statusPos();
        statusPlate_->updateGeometry(pos, _event->size().width() -  pos.x() - Utils::scale_value(16));
        updateGeometry();
        QWidget::resizeEvent(_event);
    }

    void AvatarNameInfo::mousePressEvent(QMouseEvent* _event)
    {
        clicked_ = _event->pos();
        statusPlate_->onMousePress(_event->pos());

        QWidget::mousePressEvent(_event);
    }

    void AvatarNameInfo::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (Utils::clicked(clicked_, _event->pos()))
        {
            if (aimId_ == MyInfo()->aimId() && badgeRect_.isValid() && badgeRect_.contains(_event->pos()))
            {
                Q_EMIT badgeClicked(QPrivateSignal());
            }
            else
            {
                QRect r(margins_.left(), height() / 2 - avatarSize_ / 2, avatarSize_, avatarSize_);
                if (r.contains(_event->pos()) && !defaultAvatar_)
                    Q_EMIT avatarClicked(QPrivateSignal());

                if (clickable_)
                    Q_EMIT clicked(QPrivateSignal());
            }
        }

        statusPlate_->onMouseRelease(_event->pos());

        QWidget::mouseReleaseEvent(_event);
    }

    void AvatarNameInfo::mouseMoveEvent(QMouseEvent* _event)
    {
        QRect r(margins_.left(), height() / 2 - avatarSize_ / 2, avatarSize_, avatarSize_);
        if (r.contains(_event->pos()) && !defaultAvatar_ || (statusPlate_->rect().contains(_event->pos()) && !Logic::GetLastseenContainer()->isBot(aimId_)))
            setCursor(Qt::PointingHandCursor);
        else
            setCursor(Qt::ArrowCursor);

        const auto nameRect = getNameRect();
        if (Features::longPathTooltipsAllowed() && nameRect.contains(mapToGlobal(_event->pos())) && name_->isElided())
        {
            if (!isTooltipActivated())
                showTooltip();
        }
        else
        {
            hideTooltip();
        }

        statusPlate_->onMouseMove(_event->pos());

        QWidget::mouseMoveEvent(_event);
    }

    void AvatarNameInfo::enterEvent(QEvent* _event)
    {
        hovered_ = true;
        update();

        QWidget::enterEvent(_event);
    }

    void AvatarNameInfo::leaveEvent(QEvent* _event)
    {
        hovered_ = false;
        statusPlate_->onMouseLeave();

        hideTooltip();
        update();

        QWidget::leaveEvent(_event);
    }

    void AvatarNameInfo::avatarChanged(const QString& aimId)
    {
        if (aimId != aimId_ || aimId.isEmpty())
            return;

        loadAvatar();
    }

    void AvatarNameInfo::friendlyChanged(const QString& _aimId, const QString& _friendlyName)
    {
        if (aimId_ != _aimId)
            return;

        friendlyName_ = _friendlyName;
        name_->setText(friendlyName_);
        name_->elide(width() - margins_.left() - margins_.right() - avatarSize_ - textOffset_ - (clickable_ ? Utils::scale_value(ICON_SIZE + ICON_OFFSET) : 0));
        update();
    }

    void AvatarNameInfo::statusChanged(const QString& _aimid)
    {
        if (_aimid != aimId_)
            return;

        updateSize();
        update();
    }

    void AvatarNameInfo::loadAvatar()
    {
        avatar_ = Logic::GetAvatarStorage()->GetRounded(aimId_, friendlyName_, Utils::scale_bitmap(avatarSize_), defaultAvatar_, false, false);
        Utils::check_pixel_ratio(avatar_);
        update();
    }

    void AvatarNameInfo::setBadgeRect()
    {
        if (aimId_ != MyInfo()->aimId())
            return;
        const auto statusParams = Utils::getStatusBadgeParams(Utils::scale_bitmap(avatarSize_));
        if (statusParams.isValid())
            badgeRect_ = QRect(statusParams.offset_, QSize(avatarSize_, avatarSize_)).translated(margins_.left(), height() / 2 - avatarSize_ / 2);
    }

    void AvatarNameInfo::updateSize()
    {
        const auto statusTopLeft = statusPos();
        const auto statusBottom = statusTopLeft.y() + statusPlate_->height() + statusBottomMargin();
        const auto avatarHeight = Utils::scale_value(AVATAR_INFO_HEIGHT);

        setMinimumHeight(std::max(statusBottom, avatarHeight));

        statusPlate_->updateGeometry(statusTopLeft, width() -  statusTopLeft.x() - Utils::scale_value(16));
    }

    QPoint AvatarNameInfo::statusPos() const
    {
        const auto isBot = Logic::GetLastseenContainer()->isBot(aimId_);
        auto nameHeight = name_->cachedSize().height();
        auto infoHeight = info_->cachedSize().height();
        return { margins_.left() + avatarSize_ + textOffset_, nameOffsetTop() + nameHeight + (isBot ? botStatusTopOffset() : infoHeight + statusTopOffset()) };
    }

    QPoint AvatarNameInfo::avatarTopLeft() const
    {
        return QPoint(margins_.left(), height() / 2 - avatarSize_ / 2);
    }

    QRect AvatarNameInfo::getNameRect() const
    {
        const auto nameWidth = width() - margins_.left() - margins_.right() - avatarSize_ - textOffset_ - (clickable_ ? Utils::scale_value(ICON_SIZE + ICON_OFFSET) : 0);
        return QRect(mapToGlobal(mapFromParent(name_->offsets())), QSize(nameWidth, name_->cachedSize().height()));
    }

    bool AvatarNameInfo::isTooltipActivated() const
    {
        return tooltipActivated_;
    }

    void AvatarNameInfo::showTooltip()
    {
        if (!tooltipTimer_)
        {
            tooltipTimer_ = new QTimer(this);
            tooltipTimer_->setInterval(Tooltip::getDefaultShowDelay());
            tooltipTimer_->setSingleShot(true);
            connect(tooltipTimer_, &QTimer::timeout, this, [this]()
            {
                if (name_)
                    Tooltip::show(name_->getSourceText().string(), getNameRect(), {}, Tooltip::ArrowDirection::Auto, Tooltip::ArrowPointPos::Top, {}, Tooltip::TooltipMode::Multiline);
            });
        }

        tooltipTimer_->start();

        tooltipActivated_ = true;
    }

    void AvatarNameInfo::hideTooltip()
    {
        if (tooltipTimer_)
            tooltipTimer_->stop();

        Tooltip::hide();

        tooltipActivated_ = false;
    }

    StatusPlate::StatusPlate(QObject* _parent)
        : QObject(_parent)
    {
        text_ = TextRendering::MakeTextUnit(QString(), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        text_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1);
        connect(Logic::GetStatusContainer(), &Logic::StatusContainer::statusChanged, this, &StatusPlate::onStatusChanged);
    }

    StatusPlate::~StatusPlate() = default;

    void StatusPlate::setContactId(const QString& _id)
    {
        if (contactId_ == _id)
            return;

        contactId_ = _id;

        if (!Logic::GetLastseenContainer()->isBot(contactId_))
            status_ = Logic::GetStatusContainer()->getStatus(contactId_);
        else
            status_ = Statuses::Status(Emoji::EmojiCode::toQString(Emoji::EmojiCode(0x1F916)), QT_TRANSLATE_NOOP("status_plate", "Bot"));

        text_->setText(status_.getDescription());
        hoverEnabled_ = !Logic::GetLastseenContainer()->isBot(contactId_);
        updateGeometry(rect_.topLeft(), cachedAvailableWidth_);
    }

    void StatusPlate::draw(QPainter& _p)
    {
        if (status_.isEmpty())
            return;

        QPainterPath path;
        path.addRoundedRect(rect_, Utils::scale_value(10), Utils::scale_value(10));
        _p.fillPath(path, statusBackgroundColor(hovered_, pressed_));
        _p.drawImage(rect_.left() + statusSideMargin(), rect_.top() + (rect_.height() - statusImageSize()) / 2, status_.getImage(statusImageSize()));
        text_->draw(_p, TextRendering::VerPosition::MIDDLE);
    }

    int StatusPlate::height() const
    {
        return status_.isEmpty() ? 0 : Utils::scale_value(20);
    }

    QRect StatusPlate::rect() const
    {
        return rect_;
    }

    bool StatusPlate::isEmpty() const
    {
        return status_.isEmpty();
    }

    void StatusPlate::onMouseMove(const QPoint& _pos)
    {
        if (!hoverEnabled_)
            return;

        auto mouseOver = rect_.contains(_pos);
        if (mouseOver != std::exchange(hovered_, mouseOver))
            Q_EMIT update(QPrivateSignal());
    }

    void StatusPlate::onMousePress(const QPoint& _pos)
    {
        if (!hoverEnabled_)
            return;

        pressed_ = rect_.contains(_pos);
        Q_EMIT update(QPrivateSignal());
    }

    void StatusPlate::onMouseRelease(const QPoint& _pos)
    {
        if (pressed_ && hoverEnabled_ && rect_.contains(_pos))
        {
            GeneralDialog::Options opt;
            opt.fixedSize_ = false;
            GeneralDialog d(new StatusPopup(status_, nullptr), Utils::InterConnector::instance().getMainWindow(), opt);
            d.setTransparentBackground(true);
            d.setShadow(false);

            if (d.showInCenter())
            {
                if (auto mainWindow = Utils::InterConnector::instance().getMainWindow())
                    mainWindow->openStatusPicker();
            }
        }

        pressed_ = false;
        Q_EMIT update(QPrivateSignal());
    }

    void StatusPlate::onMouseLeave()
    {
        hovered_ = false;
        Q_EMIT update(QPrivateSignal());
    }

    void StatusPlate::onStatusChanged(const QString& _contactId)
    {
        if (_contactId != contactId_ || Logic::GetLastseenContainer()->isBot(contactId_))
            return;

        status_ = Logic::GetStatusContainer()->getStatus(contactId_);
        text_->setText(status_.getDescription());

        updateGeometry(rect_.topLeft(), cachedAvailableWidth_);
    }

    int StatusPlate::availableForText(int _availableWidth) const
    {
        return _availableWidth - statusImageSize() - 2 * statusSideMargin() - statusImageSideMargin();
    }

    void StatusPlate::updateTextGeometry(int _availableWidth)
    {
        text_->getHeight(availableForText(_availableWidth));
        text_->setOffsets(rect_.left() + statusImageSize() + statusImageSideMargin() + statusSideMargin(), rect_.center().y());
    }

    void StatusPlate::updateGeometry(const QPoint& _topLeft, int _availableWidth)
    {
        cachedAvailableWidth_ = _availableWidth;
        if (!status_.isEmpty())
        {
            rect_ = QRect(_topLeft, QSize(statusImageSize() + std::min(availableForText(_availableWidth),
                                                           text_->desiredWidth()) + 2 * statusSideMargin() + statusImageSideMargin(), Utils::scale_value(20)));
        }
        else
        {
            rect_ = QRect();
        }
        updateTextGeometry(_availableWidth);
    }

    StatusPopup::StatusPopup(const Statuses::Status& _status, QWidget* _parent)
        : QWidget(_parent),
          content_(new QWidget(this)),
          status_(_status),
          emoji_(new BigEmojiWidget(_status.toString(), Utils::scale_value(100), this)),
          text_(new TextWidget(_parent, _status.getDescription(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS)),
          duration_(new TextWidget(_parent, _status.getTimeString()))

    {
        const auto margin = Utils::scale_value(16);
        setContentsMargins(margin, 0, margin, margin); // add margins to have enough space for custom shadow

        auto layout = Utils::emptyVLayout(this);

        content_->setFixedWidth(statusPopupWidth());
        auto contentLayout = Utils::emptyVLayout(content_);
        contentLayout->addWidget(emoji_, 0, Qt::AlignHCenter);

        scrollArea_ = CreateScrollAreaAndSetTrScrollBarV(this);
        auto scrollAreaContent = new QWidget(scrollArea_);
        auto scrollAreaContentLayout = Utils::emptyVLayout(scrollAreaContent);
        scrollAreaContentLayout->addWidget(text_, 0, Qt::AlignHCenter);
        scrollAreaContentLayout->addSpacing(statusPopupElapsedTextTopMargin());
        scrollAreaContentLayout->addWidget(duration_, 0, Qt::AlignHCenter);

        text_->init(Fonts::appFontScaled(23), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
        text_->setMaxWidthAndResize(statusPopupTextMaxWidth());
        duration_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
        duration_->setMaxWidthAndResize(statusPopupTextMaxWidth());

        scrollArea_->setWidget(scrollAreaContent);
        scrollArea_->setWidgetResizable(true);
        scrollArea_->setFrameStyle(QFrame::NoFrame);
        scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea_->verticalScrollBar()->setStyleSheet(qsl("height: 0")); // fix for minimum height of scroll area
        scrollArea_->setStyleSheet(qsl("background: transparent;"));

        contentLayout->addSpacing(Utils::scale_value(16));
        contentLayout->addWidget(scrollArea_);
        contentLayout->addSpacing(Utils::scale_value(24));

        auto buttonsLayout = Utils::emptyHLayout();
        auto accept = new DialogButton(this, QT_TRANSLATE_NOOP("status_popup", "Select my status"), DialogButtonRole::CONFIRM);
        connect(accept, &DialogButton::clicked, this, []() { Q_EMIT Utils::InterConnector::instance().acceptGeneralDialog(); });

        accept->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        accept->adjustSize();
        auto cancel = new DialogButton(this, QT_TRANSLATE_NOOP("status_popup", "Cancel"), DialogButtonRole::CANCEL);
        connect(cancel, &DialogButton::clicked, this, []() { Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo()); });
        cancel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        cancel->adjustSize();

        buttonsLayout->addStretch();
        buttonsLayout->addWidget(cancel);
        buttonsLayout->addSpacing(statusPopupButtonsGap());
        buttonsLayout->addWidget(accept);
        buttonsLayout->addStretch();

        contentLayout->addLayout(buttonsLayout);
        contentLayout->addSpacing(Utils::scale_value(16));

        auto shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(16);
        shadow->setOffset(0, Utils::scale_value(1));
        shadow->setColor(statusPopupShadowColor());
        setGraphicsEffect(shadow);

        layout->addWidget(content_);
    }

    void StatusPopup::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        auto backgroundRect = content_->geometry();
        backgroundRect.setTop(emoji_->height() / 2);

        QPainterPath path;
        path.addRoundedRect(backgroundRect, Utils::scale_value(6), Utils::scale_value(6));
        p.fillPath(path, Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
    }

    TextLabel::TextLabel(QWidget* _parent, int _maxLinesCount)
        : QWidget(_parent)
        , maxLinesCount_(_maxLinesCount)
        , collapsed_(false)
        , copyable_(false)
        , buttonsVisible_(false)
        , onlyCopyButton_(false)
        , cursorForText_(false)
        , isTextField_(false)
        , menu_(nullptr)
    {
        text_ = TextRendering::MakeTextUnit(QString(), Data::MentionMap());
        collapsedText_ = TextRendering::MakeTextUnit(QString(), Data::MentionMap(), TextRendering::LinksVisible::SHOW_LINKS);
        readMore_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("sidebar", "Read more"));

        iconNormalColor_ = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);
        iconHoverColor_ = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER);
        iconPressedColor_ = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE);
        setMouseTracking(true);

        if (QApplication::clipboard()->supportsSelection())
        {
            connect(this, &TextLabel::selectionChanged, this, [this]()
            {
                QApplication::clipboard()->setText(getSelectedText(), QClipboard::Selection);
            });
        }
    }

    void TextLabel::setMargins(const QMargins& _margins)
    {
        if (margins_ == _margins)
            return;
        margins_ = _margins;
        setFixedHeight(std::max(0, text_->cachedSize().height()) + margins_.top() + margins_.bottom());
        update();
    }

    void TextLabel::init(const QFont& _font, const QColor& _color, const QColor& _linkColor)
    {
        text_->init(_font, _color, _linkColor, MessageStyle::getTextSelectionColor(), QColor(), textAlign_, -1, TextRendering::LineBreakType::PREFER_SPACES);
        collapsedText_->init(_font, _color, _linkColor, MessageStyle::getTextSelectionColor(), QColor(), textAlign_, maxLinesCount_, TextRendering::LineBreakType::PREFER_SPACES);
        readMore_->init(_font, _linkColor);
        readMore_->evaluateDesiredSize();

        update();
    }

    void TextLabel::selectText(const QPoint& _from, const QPoint& _to)
    {
        if (isTextField_)
        {
            const auto isSelectionTopToBottom = (_from.y() <= _to.y());
            const auto from = isSelectionTopToBottom ? _from : _to;
            const auto to = isSelectionTopToBottom ? _to : _from;
            if (collapsed_)
                collapsedText_->select(from, to);
            else
                text_->select(from, to);
            Q_EMIT selectionChanged(QPrivateSignal());
        }
        update();
    }

    void TextLabel::updateSize(bool _forceCollapsed)
    {
        auto w = width() - margins_.left() - margins_.right();
        if (buttonsVisible_)
        {
            collapsed_ = false;
            setFixedHeight(text_->cachedSize().height() + margins_.top() + margins_.bottom());
            return;
        }

        auto fullHeight = text_->getHeight(w);
        auto collapsedHeight = collapsedText_->getHeight(w);

        if (_forceCollapsed)
            collapsed_ = (fullHeight != collapsedHeight && collapsedText_->isElided());

        if (maxLinesCount_ == -1 || !collapsed_)
            setFixedHeight(fullHeight + margins_.top() + margins_.bottom());
        else
            setFixedHeight(collapsedHeight + readMore_->cachedSize().height() + margins_.top() + margins_.bottom());
    }

    void TextLabel::setText(const QString& _text, const QColor& _color)
    {
        if (text_->getText() == _text && text_->getColor() == _color)
            return;

        text_->setText(_text, _color);
        collapsedText_->setText(_text, _color);

        updateSize(true);
        update();
    }

    void TextLabel::setCursorForText()
    {
        cursorForText_ = true;
        update();
    }

    void TextLabel::unsetCursorForText()
    {
        cursorForText_ = false;
        update();
    }

    void TextLabel::setColor(const QColor& _color)
    {
        if (text_->getColor() == _color)
            return;

        text_->setColor(_color);
        update();
    }

    void TextLabel::setLinkColor(const QColor& _color)
    {
        text_->setLinkColor(_color);
        update();
    }

    void TextLabel::disableCommands()
    {
        text_->disableCommands();
        collapsedText_->disableCommands();
        update();
    }

    void TextLabel::clearMenuActions()
    {
        if (menu_)
        {
            menu_->deleteLater();
            menu_ = nullptr;
        }
    }

    void TextLabel::addMenuAction(const QString& _iconPath, const QString& _name, const QVariant& _data)
    {
        if (!menu_)
        {
            menu_ = new ContextMenu(this);
            connect(menu_, &ContextMenu::triggered, this, &TextLabel::menuAction);
        }

        menu_->addActionWithIcon(_iconPath, _name, _data);
    }

    void TextLabel::makeCopyable()
    {
        copyable_ = true;
    }

    void TextLabel::makeTextField()
    {
        makeCopyable();
        isTextField_ = true;
    }

    void TextLabel::showButtons()
    {
        buttonsVisible_ = true;
        updateSize();
        update();
    }

    void TextLabel::allowOnlyCopy()
    {
        onlyCopyButton_ = true;
        updateSize();
        update();
    }

    QString TextLabel::getText() const
    {
        return text_->getText();
    }

    void TextLabel::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        if (bgColor_.isValid())
            p.fillRect(rect(), bgColor_);

        if (buttonsVisible_)
        {
            auto w = width() - margins_.left() - margins_.right();
            if (rect().contains(mapFromGlobal(QCursor::pos())))
            {
                text_->elide(w - Utils::scale_value(ICON_SIZE) * (onlyCopyButton_ ? 1 : 3) - Utils::scale_value(COLORED_BUTTON_TEXT_OFFSET));

                auto makeIcon = [](const auto& _path, auto _color)
                {
                    return Utils::renderSvgScaled(_path, QSize(ICON_SIZE, ICON_SIZE), _color);
                };

                const auto copy = makeIcon(qsl(":/copy_icon"), iconNormalColor_);
                const auto copyHover = makeIcon(qsl(":/copy_icon"), iconHoverColor_);
                const auto copyActive = makeIcon(qsl(":/copy_icon"), iconPressedColor_);

                const auto copyRect = QRect(width() - Utils::scale_value(ICON_SIZE) * (onlyCopyButton_ ? 2 :4), margins_.top(), Utils::scale_value(ICON_SIZE), Utils::scale_value(ICON_SIZE));
                if (copyRect.contains(mapFromGlobal(QCursor::pos())))
                {
                    if (QApplication::mouseButtons() & Qt::LeftButton)
                        p.drawPixmap(copyRect.topLeft(), copyActive);
                    else
                        p.drawPixmap(copyRect.topLeft(), copyHover);
                }
                else
                {
                    p.drawPixmap(copyRect.topLeft(), copy);
                }

                if (!onlyCopyButton_)
                {
                    const auto share = makeIcon(qsl(":/share_icon"), iconNormalColor_);
                    const auto shareHover = makeIcon(qsl(":/share_icon"), iconHoverColor_);
                    const auto shareActive = makeIcon(qsl(":/share_icon"), iconPressedColor_);

                    const auto shareRect = QRect(width() - Utils::scale_value(ICON_SIZE) * 2, margins_.top(), Utils::scale_value(ICON_SIZE), Utils::scale_value(ICON_SIZE));


                    if (shareRect.contains(mapFromGlobal(QCursor::pos())))
                    {
                        if (QApplication::mouseButtons() & Qt::LeftButton)
                            p.drawPixmap(shareRect.topLeft(), shareActive);
                        else
                            p.drawPixmap(shareRect.topLeft(), shareHover);
                    }
                    else
                    {
                        p.drawPixmap(shareRect.topLeft(), share);
                    }
                }
            }
            else
            {
                text_->elide(w);
            }
        }

        text_->setOffsets(margins_.left(), margins_.top());
        collapsedText_->setOffsets(margins_.left(), margins_.top());
        if (maxLinesCount_ != -1 && collapsed_)
        {
            collapsedText_->draw(p);
            readMore_->setOffsets(margins_.left(), margins_.top() + collapsedText_->cachedSize().height());
            readMore_->draw(p);
        }
        else
        {
            text_->draw(p);
        }
    }

    void TextLabel::resizeEvent(QResizeEvent* _event)
    {
        updateSize();
        QWidget::resizeEvent(_event);
    }

    void TextLabel::mousePressEvent(QMouseEvent* _event)
    {
        if (_event->button() == Qt::LeftButton)
        {
            if (TripleClickTimer_ && TripleClickTimer_->isActive())
            {
                TripleClickTimer_->stop();
                if (collapsed_)
                {
                    collapsedText_->selectAll();
                    collapsedText_->fixSelection();
                }
                else
                {
                    text_->selectAll();
                    text_->fixSelection();
                }
            }
            else
            {
                clearSelection();
                selectFrom_ = _event->pos();
            }
        }
        else
        {
            selectFrom_ = QPoint();
            selectTo_ = QPoint();
        }
        update();
        QWidget::mousePressEvent(_event);
    }

    void TextLabel::mouseReleaseEvent(QMouseEvent* _event)
    {
        const auto pos = _event->pos();

        if (_event->button() == Qt::LeftButton)
        {
            if (Utils::clicked(selectFrom_, pos) || text_->isSelected() || (collapsed_ && collapsedText_->isSelected()))
            {
                selectFrom_ = QPoint();
                selectTo_ = QPoint();

                if (maxLinesCount_ != -1 && collapsed_ && readMore_->contains(pos))
                {
                    collapsed_ = false;
                    updateSize();
                    update();
                }
                else
                {
                    bool buttonWasClicked = false;
                    if (buttonsVisible_)
                    {
                        auto copyRect = QRect(width() - Utils::scale_value(ICON_SIZE) * (onlyCopyButton_ ? 2 : 4), margins_.top(), Utils::scale_value(ICON_SIZE), Utils::scale_value(ICON_SIZE));
                        if (copyRect.contains(pos) && !std::exchange(buttonWasClicked, true))
                            Q_EMIT copyClicked(text_->getSourceText().string(), QPrivateSignal());

                        auto shareRect = QRect(width() - Utils::scale_value(ICON_SIZE) * 2, margins_.top(), Utils::scale_value(ICON_SIZE), Utils::scale_value(ICON_SIZE));
                        if (!onlyCopyButton_ && shareRect.contains(pos) && !std::exchange(buttonWasClicked, true))
                            Q_EMIT shareClicked(QPrivateSignal());
                    }

                    if (!buttonWasClicked && (text_->contains(pos) || collapsedText_->contains(pos)))
                    {
                        const auto isTextOverLink = !text_->isSelected() && text_->isOverLink(pos);
                        const auto isCollapsedOverLink = collapsed_ && !collapsedText_->isSelected() && collapsedText_->isOverLink(pos);
                        if (isTextField_ && (isTextOverLink || isCollapsedOverLink))
                        {
                            if (collapsed_)
                                collapsedText_->clicked(pos);
                            else
                                text_->clicked(pos);
                        }
                        else
                        {
                            Q_EMIT textClicked(QPrivateSignal());
                        }
                    }
                }
            }
            else
            {
                selectTo_ = pos;
                if (isTextField_)
                {
                    selectText(selectFrom_, selectTo_);
                    selectFrom_ = QPoint();
                    selectTo_ = QPoint();
                }
            }
        }
        else if (_event->button() == Qt::RightButton)
        {
            ContextMenu* menu = nullptr;
            if (menu_)
            {
                menu = menu_;
            }
            else if (copyable_)
            {
                menu = new ContextMenu(this);

                QMap<QString, QVariant> data;
                data[qsl("command")] = qsl("copy");
                data[qsl("text")] = isTextField_ && text_->isSelected() ? text_->getSelectedText().string() : text_->getText();

                menu->addActionWithIcon(qsl(":/copy_icon"), QT_TRANSLATE_NOOP("sidebar", "Copy"), data);

                connect(menu, &ContextMenu::triggered, this, &TextLabel::menuAction);
                connect(menu, &ContextMenu::triggered, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
                connect(menu, &ContextMenu::aboutToHide, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
            }

            if (menu)
            {
                menu->showAtLeft(pos.x() >= width() / 2);
                menu->popup(QCursor::pos());
            }
        }

        update();

        QWidget::mouseReleaseEvent(_event);
    }

    void TextLabel::mouseMoveEvent(QMouseEvent* _event)
    {
        auto copyRect = QRect(width() - Utils::scale_value(ICON_SIZE) * (onlyCopyButton_ ? 2 :4), margins_.top(), Utils::scale_value(ICON_SIZE), Utils::scale_value(ICON_SIZE));
        auto shareRect = QRect(width() - Utils::scale_value(ICON_SIZE) * 2, margins_.top(), Utils::scale_value(ICON_SIZE), Utils::scale_value(ICON_SIZE));
        const auto pos = _event->pos();

        if (buttonsVisible_ && (copyRect.contains(_event->pos()) || shareRect.contains(pos)))
        {
            setCursor(Qt::PointingHandCursor);
            if (copyRect.contains(_event->pos()))
                Tooltip::show(QT_TRANSLATE_NOOP("sidebar", "Copy"), QRect(mapToGlobal(copyRect.topLeft()), mapToGlobal(copyRect.bottomRight())));
            else
                Tooltip::show(QT_TRANSLATE_NOOP("sidebar", "Share"), QRect(mapToGlobal(shareRect.topLeft()), mapToGlobal(shareRect.bottomRight())));
        }
        else
        {
            const auto isOnText = ((maxLinesCount_ != -1 && collapsed_ && readMore_->contains(pos))
                                   || (text_->contains(pos) && (buttonsVisible_ || cursorForText_))
                                   || (!collapsed_ && text_->isOverLink(pos))
                                   || (collapsed_ && collapsedText_->isOverLink(pos)));

            if (isTextField_ && !selectFrom_.isNull())
            {
                selectTo_ = pos;
                selectText(selectFrom_, selectTo_);
            }
            setCursor((isOnText && selectFrom_.isNull()) ? Qt::PointingHandCursor : Qt::ArrowCursor);
            Tooltip::hide();
        }

        update();

        QWidget::mouseMoveEvent(_event);
    }

    void TextLabel::mouseDoubleClickEvent(QMouseEvent* _event)
    {
        if (isTextField_ && _event->button() == Qt::LeftButton && !text_->isAllSelected())
        {
            const auto pos = _event->pos();
            selectFrom_ = pos;
            selectFrom_.rx() = (pos.x() != 0) ? 0 : pos.x();
            selectTo_ = pos;

            const auto tripleClick = [this](bool result)
            {
                if (result)
                {
                    if (!TripleClickTimer_)
                    {
                        TripleClickTimer_ = new QTimer(this);
                        TripleClickTimer_->setInterval(QApplication::doubleClickInterval());
                        TripleClickTimer_->setSingleShot(true);
                    }
                    TripleClickTimer_->start();
                }
            };
            if (collapsed_)
                collapsedText_->doubleClicked(pos, true, std::move(tripleClick));
            else
                text_->doubleClicked(pos, true, std::move(tripleClick));
            tripleClick(true);
        }

        update();
    }

    void TextLabel::enterEvent(QEvent* _event)
    {
        update();
        QWidget::enterEvent(_event);
    }

    void TextLabel::leaveEvent(QEvent* _event)
    {
        Tooltip::hide();
        update();
        QWidget::leaveEvent(_event);
    }

    void TextLabel::setIconColors(QColor _normal, QColor _hover, QColor _pressed)
    {
        iconNormalColor_ = _normal;
        iconHoverColor_ = _hover;
        iconPressedColor_ = _pressed;
    }

    QString TextLabel::getSelectedText() const
    {
        if (collapsed_)
            return collapsedText_->getSelectedText().string();
        return text_->getSelectedText().string();
    }

    void TextLabel::tryClearSelection(const QPoint& _pos)
    {
        if (!geometry().contains(_pos))
            clearSelection();
    }

    void TextLabel::clearSelection()
    {
        text_->releaseSelection();
        text_->clearSelection();
        collapsedText_->releaseSelection();
        collapsedText_->clearSelection();
        selectFrom_ = QPoint();
        selectTo_ = QPoint();
        update();
    }

    void InfoBlock::hide()
    {
        header_->hide();
        text_->hide();
    }

    void InfoBlock::show()
    {
        header_->show();
        text_->show();
    }

    void InfoBlock::setVisible(bool _value)
    {
        header_->setVisible(_value);
        text_->setVisible(_value);
    }

    void InfoBlock::setHeaderText(const QString& _text, const QColor& _color)
    {
        if (header_->getText() != _text)
            header_->setText(_text, _color);
    }

    void InfoBlock::setText(const QString& _text, const QColor& _color)
    {
        text_->setText(_text, _color);
    }

    void InfoBlock::setTextLinkColor(const QColor& _color)
    {
        text_->setLinkColor(_color);
    }

    void InfoBlock::setHeaderLinkColor(const QColor& _color)
    {
        header_->setLinkColor(_color);
    }

    void InfoBlock::disableCommandsInText()
    {
        if (text_)
            text_->disableCommands();
    }

    bool InfoBlock::isVisible() const
    {
        return header_ && header_->isVisible();
    }

    QString InfoBlock::getSelectedText() const
    {
        return text_->getSelectedText();
    }

    GalleryPreviewItem::GalleryPreviewItem(QWidget* _parent, const QString& _link, qint64 _msg, qint64 _seq, const QString& _aimId, const QString& _sender, bool _outgoing, time_t _time, int previewSize)
        : QWidget(_parent)
        , aimId_(_aimId)
    {
        item_ = new ImageVideoItem(this, _link, _msg, _seq, _outgoing, _sender, _time);
        item_->load(LoadType::Immediate);

        setFixedSize(previewSize, previewSize);
        connect(item_, &ImageVideoItem::needUpdate, this, &GalleryPreviewItem::updateWidget);
        setCursor(Qt::PointingHandCursor);
    }

    qint64 GalleryPreviewItem::msg()
    {
        return item_->getMsg();
    }

    qint64 GalleryPreviewItem::seq()
    {
        return item_->getSeq();
    }

    void GalleryPreviewItem::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        item_->draw(p);
    }

    void GalleryPreviewItem::resizeEvent(QResizeEvent* _event)
    {
        const auto& s = _event->size();
        item_->setRect(QRect(0, 0, s.width(), s.height()));
        QWidget::resizeEvent(_event);
    }

    void GalleryPreviewItem::mousePressEvent(QMouseEvent* _event)
    {
        pos_ = _event->pos();
        item_->setPressed(true);
        QWidget::mousePressEvent(_event);
    }

    void GalleryPreviewItem::enterEvent(QEvent* _e)
    {
        item_->setHovered(true);
        update();
        QWidget::enterEvent(_e);
    }

    void GalleryPreviewItem::leaveEvent(QEvent* _e)
    {
        item_->setHovered(false);
        item_->setPressed(false);
        update();
        QWidget::leaveEvent(_e);
    }

    void GalleryPreviewItem::mouseReleaseEvent(QMouseEvent* _event)
    {
        item_->setPressed(false);
        if (Utils::clicked(pos_, _event->pos()))
        {
            if (_event->button() == Qt::LeftButton)
            {
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profilescr_chatgallery_action, { { "category", "ChatInfo" }, { "chat_type", Utils::chatTypeByAimId(aimId_) } });
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_view, { { "chat_type", Utils::chatTypeByAimId(aimId_) }, { "from", "profile_carousel" }, { "media_type", item_->isVideo() ? "video" : (item_->isGif() ? "gif" : "photo") } });
                Utils::InterConnector::instance().openGallery(Utils::GalleryData(aimId_, item_->getLink(), item_->getMsg(), item_->sender(), item_->time()));
            }
            else if (_event->button() == Qt::RightButton)
            {
                auto menu = new ContextMenu(this);

                menu->addActionWithIcon(qsl(":/context_menu/goto"), QT_TRANSLATE_NOOP("gallery", "Go to message"), makeData(qsl("go_to"), item_->getMsg(), item_->getLink(), item_->sender(), item_->time()));
                menu->addActionWithIcon(qsl(":/context_menu/forward"), QT_TRANSLATE_NOOP("context_menu", "Forward"), makeData(qsl("forward"), item_->getMsg(), item_->getLink(), item_->sender(), item_->time()));

                if (!Favorites::isFavorites(aimId_))
                {
                    menu->addSeparator();
                    menu->addActionWithIcon(qsl(":/context_menu/favorites"), QT_TRANSLATE_NOOP("context_menu", "Add to favorites"), makeData(qsl("add_to_favorites"), item_->getMsg(), item_->getLink(), item_->sender(), item_->time()));
                }

                connect(menu, &ContextMenu::triggered, this, &GalleryPreviewItem::onMenuAction, Qt::QueuedConnection);
                connect(menu, &ContextMenu::triggered, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
                connect(menu, &ContextMenu::aboutToHide, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);

                menu->popup(mapToGlobal(_event->pos()));
            }
        }
        QWidget::mouseReleaseEvent(_event);
    }

    void GalleryPreviewItem::updateWidget(const QRect&)
    {
        update();
    }

    void GalleryPreviewItem::onMenuAction(QAction *_action)
    {
        const auto params = _action->data().toMap();
        const auto command = params[qsl("command")].toString();
        const auto link = params[qsl("link")].toString();
        const auto msg = params[qsl("msg")].toLongLong();
        const auto sender = params[qsl("sender")].toString();
        const auto time = params[qsl("time")].toLongLong();

        auto makeQuote = [this, &sender, &link, &msg, &time]()
        {
            Data::Quote quote;
            quote.chatId_ = aimId_;
            quote.senderId_ = sender;
            quote.text_ = link;
            quote.msgId_ = msg;
            quote.time_ = time;
            quote.senderFriendly_ = Logic::GetFriendlyContainer()->getFriendly(sender);
            return quote;
        };

        if (!msg)
            return;

        if (command == u"go_to")
        {
            Q_EMIT Logic::getContactListModel()->select(aimId_, msg, Logic::UpdateChatSelection::Yes);
        }
        if (command == u"forward")
        {
            Ui::forwardMessage({ makeQuote() }, false, QString(), QString(), !Favorites::isFavorites(aimId_));
        }
        else if (command == u"add_to_favorites")
        {
            Favorites::addToFavorites({ makeQuote() });
            Favorites::showSavedToast();
        }
        else if (command == u"copy_link")
        {
            if (!link.isEmpty())
                QApplication::clipboard()->setText(link);
        }
        else if (const auto is_shared = command == u"delete_all"; is_shared || command == u"delete")
        {
            QString text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete message?");

            auto confirm = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Yes"),
                text,
                QT_TRANSLATE_NOOP("popup_window", "Delete message"),
                nullptr
            );

            if (confirm)
                GetDispatcher()->deleteMessages(aimId_, { DeleteMessageInfo(msg, QString(), is_shared) });
        }
    }

    GalleryPreviewWidget::GalleryPreviewWidget(QWidget* _parent, int _previewSize, int _previewCount)
        : QWidget(_parent)
        , previewSize_(_previewSize)
        , previewCount_(_previewCount)
        , reqId_(-1)
    {
        layout_ = Utils::emptyHLayout(this);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryResult, this, &GalleryPreviewWidget::dialogGalleryResult);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryUpdate, this, &GalleryPreviewWidget::dialogGalleryUpdate);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryInit, this, &GalleryPreviewWidget::dialogGalleryInit);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryErased, this, &GalleryPreviewWidget::dialogGalleryErased);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryHolesDownloading, this, &GalleryPreviewWidget::dialogGalleryHolesDownloading);
    }

    void GalleryPreviewWidget::setMargins(const QMargins& _margins)
    {
        layout_->setContentsMargins(_margins);
    }

    void GalleryPreviewWidget::setSpacing(int _spacing)
    {
        layout_->setSpacing(_spacing);
    }

    void GalleryPreviewWidget::setAimId(const QString& _aimid)
    {
        if (aimId_ == _aimid)
            return;

        aimId_ = _aimid;
        clear();
        requestGallery();
    }

    void GalleryPreviewWidget::clear()
    {
        while (layout_->count() != 0)
        {
            auto i = layout_->takeAt(0);
            if (i->widget())
                i->widget()->deleteLater();

            delete i;
        }
    }

    void GalleryPreviewWidget::requestGallery()
    {
        reqId_ = Ui::GetDispatcher()->getDialogGallery(aimId_, { ql1s("image"), ql1s("video") }, 0, 0, previewCount_, false);
    }

    void GalleryPreviewWidget::dialogGalleryResult(const int64_t _seq, const QVector<Data::DialogGalleryEntry>& _entries, bool _exhausted)
    {
        if (_seq != reqId_)
            return;

        clear();

        auto count = 0;
        for (const auto& e : _entries)
        {
            if (count++ == previewCount_)
                break;

            auto w = new GalleryPreviewItem(this, e.url_, e.msg_id_, e.seq_, aimId_, e.sender_, e.outgoing_, e.time_, previewSize_);
            layout_->addWidget(w);
            w->show();
        }

        layout_->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        setVisible(!_entries.empty());
    }

    void GalleryPreviewWidget::dialogGalleryUpdate(const QString& _aimId, const QVector<Data::DialogGalleryEntry>& _entries)
    {
        if (_aimId != aimId_)
            return;

        if (layout_->isEmpty())
        {
            requestGallery();
            return;
        }

        auto del = false;
        for (const auto& e : _entries)
        {
            if (e.action_ == u"add" && e.type_ != u"image" && e.type_ != u"video")
                continue;

            for (auto i = 0; i < layout_->count(); ++i)
            {
                auto item = layout_->itemAt(i);
                auto w = item ? qobject_cast<GalleryPreviewItem*>(item->widget()) : nullptr;
                if (w)
                {
                    if (w->msg() == e.msg_id_ && e.action_ == u"del")
                    {
                        del = true;
                        break;
                    };

                    if (w->msg() > e.msg_id_ || (w->msg() == e.msg_id_ && w->seq() > e.seq_))
                        continue;

                    if (e.action_ == u"add")
                    {
                        layout_->insertWidget(i, new GalleryPreviewItem(this, e.url_, e.msg_id_, e.seq_, aimId_, e.sender_, e.outgoing_, e.time_, previewSize_));
                        if (layout_->count() >= previewCount_ + 2)
                        {
                            auto t = layout_->takeAt(previewCount_);
                            t->widget()->deleteLater();
                        }
                        break;
                    }
                }
            }
        }

        setVisible(layout_->count() > 1);

        if (del)
            reqId_ = Ui::GetDispatcher()->getDialogGallery(aimId_, { ql1s("image"), ql1s("video") }, 0, 0, previewCount_, true);
    }

    void GalleryPreviewWidget::dialogGalleryInit(const QString& _aimId)
    {
        if (aimId_ != _aimId)
            return;

        requestGallery();
    }

    void GalleryPreviewWidget::dialogGalleryErased(const QString& _aimId)
    {
        if (aimId_ != _aimId)
            return;

        requestGallery();
    }

    void GalleryPreviewWidget::dialogGalleryHolesDownloading(const QString& _aimId)
    {
        if (aimId_ != _aimId)
            return;

        if (layout_->count() < previewCount_ + 1)
            requestGallery();
    }

    MembersPlate::MembersPlate(QWidget* _parent)
        : QWidget(_parent)
    {
        members_ = TextRendering::MakeTextUnit(QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);
        search_ = TextRendering::MakeTextUnit(QString(), Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS);

        setMouseTracking(true);
    }

    void MembersPlate::setMargins(const QMargins& _margins)
    {
        if (margins_ == _margins)
            return;
        margins_ = _margins;
        update();
    }

    void MembersPlate::setMembersCount(int _count, bool _isChannel)
    {
        QString text;
        if (_isChannel)
        {
            text = Utils::GetTranslator()->getNumberString(_count,
                   QT_TRANSLATE_NOOP3("sidebar", "%1 SUBSCRIBER", "1"),
                   QT_TRANSLATE_NOOP3("sidebar", "%1 SUBSCRIBERS", "2"),
                   QT_TRANSLATE_NOOP3("sidebar", "%1 SUBSCRIBERS", "5"),
                   QT_TRANSLATE_NOOP3("sidebar", "%1 SUBSCRIBERS", "21")
                   ).arg(QString::number(_count));
        }
        else
        {
            text = Utils::GetTranslator()->getNumberString(_count,
                   QT_TRANSLATE_NOOP3("sidebar", "%1 MEMBER", "1"),
                   QT_TRANSLATE_NOOP3("sidebar", "%1 MEMBERS", "2"),
                   QT_TRANSLATE_NOOP3("sidebar", "%1 MEMBERS", "5"),
                   QT_TRANSLATE_NOOP3("sidebar", "%1 MEMBERS", "21")
                   ).arg(QString::number(_count));
        }

        members_->setText(text);

        setFixedHeight(members_->cachedSize().height() + margins_.top() + margins_.bottom());
        update();
    }

    void MembersPlate::initMembersLabel(const QFont& _font, const QColor& _color)
    {
        members_->init(_font, _color);
    }

    void MembersPlate::initSearchLabel(const QFont& _font, const QColor& _color)
    {
        search_->init(_font, _color);
        search_->setText(QT_TRANSLATE_NOOP("sidebar", "Search"));
    }

    void MembersPlate::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);

        members_->setOffsets(margins_.left(), margins_.top());
        members_->draw(p);

        search_->setOffsets(width() - margins_.right() - search_->cachedSize().width(), margins_.top());
        search_->draw(p);
    }

    void MembersPlate::mouseMoveEvent(QMouseEvent* _event)
    {
        setCursor(search_->contains(_event->pos()) ? Qt::PointingHandCursor : Qt::ArrowCursor);
        QWidget::mouseMoveEvent(_event);
    }

    void MembersPlate::mousePressEvent(QMouseEvent* _event)
    {
        clicked_ = _event->pos();
        QWidget::mousePressEvent(_event);
    }

    void MembersPlate::mouseReleaseEvent(QMouseEvent* _event)
    {
        auto pos = _event->pos();
        if (Utils::clicked(clicked_, pos) && search_->contains(pos))
            Q_EMIT searchClicked(QPrivateSignal());

        QWidget::mousePressEvent(_event);
    }

    MembersWidget::MembersWidget(QWidget* _parent, Logic::ChatMembersModel* _model, Logic::ContactListItemDelegate* _delegate, int _maxMembersCount)
        : QWidget(_parent)
        , model_(_model)
        , delegate_(_delegate)
        , params_(true)
        , maxMembersCount_(_maxMembersCount)
        , memberCount_(0)
        , hovered_(-1)
        , scrollArea_(nullptr)
        , tooltipTimer_(nullptr)
    {
        delegate_->setRenderRole(true);
        updateSize();
        connect(model_, &Logic::ChatMembersModel::dataChanged, this, &MembersWidget::dataChanged);
        setMouseTracking(true);
        setCursor(Qt::PointingHandCursor);

        connect(Logic::GetLastseenContainer(), &Logic::LastseenContainer::lastseenChanged, this, &Ui::MembersWidget::lastseenChanged);
    }

    void MembersWidget::clearCache()
    {
        delegate_->clearCache();
    }

    void Ui::MembersWidget::setScrollArea(QScrollArea* _scrollArea)
    {
        scrollArea_ = _scrollArea;
    }

    void MembersWidget::dataChanged(const QModelIndex&, const QModelIndex&, const QVector<int>&)
    {
        updateSize();
        update();
    }

    void MembersWidget::lastseenChanged(const QString& _aimid)
    {
        if (model_->contains(_aimid))
            update();
    }

    void MembersWidget::updateSize()
    {
        memberCount_ = std::min(model_->rowCount(), maxMembersCount_);
        setFixedHeight(params_.itemHeight() * memberCount_ + Utils::scale_value(MEMBERS_WIDGET_BOTTOM_OFFSET));
    }

    void MembersWidget::showTooltip(QString _text, QRect _rect, Tooltip::ArrowDirection _arrowDir, Tooltip::ArrowPointPos _arrowPos)
    {
        Tooltip::hide();

        if (!tooltipTimer_)
        {
            tooltipTimer_ = new QTimer(this);
            tooltipTimer_->setInterval(Tooltip::getDefaultShowDelay());
            tooltipTimer_->setSingleShot(true);
        }
        else
        {
            tooltipTimer_->stop();
            tooltipTimer_->disconnect(this);
        }

        connect(tooltipTimer_, &QTimer::timeout, this, [text = std::move(_text), _rect, _arrowDir, _arrowPos]()
        {
            Tooltip::show(text, _rect, {}, _arrowDir, _arrowPos, {}, Tooltip::TooltipMode::Multiline);
        });
        tooltipTimer_->start();
    }

    void MembersWidget::hideTooltip()
    {
        tooltipIndex_ = {};

        if (tooltipTimer_)
            tooltipTimer_->stop();

        Tooltip::hide();
    }

    void MembersWidget::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        memberCount_ = std::min(model_->rowCount(), maxMembersCount_);
        for (int i = 0; i < memberCount_; ++i)
        {
            QStyleOptionViewItem style;
            if (i == hovered_)
            {
                style.state |= QStyle::State_Selected;
            }
            delegate_->paint(&p, style, model_->index(i));
            p.translate(QPoint(0, params_.itemHeight()));
        }
    }

    void MembersWidget::resizeEvent(QResizeEvent* _event)
    {
        delegate_->setFixedWidth(width());
        QWidget::resizeEvent(_event);
    }

    void MembersWidget::mouseMoveEvent(QMouseEvent* _event)
    {
        hovered_ = -1;
        QRect hoveredRect;
        for (int i = 0; i < memberCount_; ++i)
        {
            const auto itemRect = QRect(0, i * params_.itemHeight(), width(), params_.itemHeight());
            if (itemRect.contains(_event->pos()))
            {
                hovered_ = i;
                hoveredRect = itemRect;
                break;
            }
        }

        if (Statuses::isStatusEnabled() && hovered_ != -1)
        {
            auto& params = GetContactListParams();
            const auto avatarRect = QRect(hoveredRect.topLeft() + QPoint(params.getAvatarX(), params.getAvatarY()), QSize(params.getAvatarSize(), params.getAvatarSize()));
            const auto aimId = model_->index(hovered_).data().value<Data::AbstractSearchResultSptr>()->getAimId();
            const auto muted = Logic::getContactListModel()->isMuted(aimId);

            if (!muted && avatarRect.contains(_event->pos()))
            {
                StatusTooltip::instance()->objectHovered([this, avatarRect]()
                {
                    return QRect(mapToGlobal(avatarRect.topLeft()), avatarRect.size());
                }, aimId, this);
            }
        }

        if (Features::longPathTooltipsAllowed() && hovered_ != -1)
        {
            const auto index = model_->index(hovered_);
            const auto aimId = index.data().value<Data::AbstractSearchResultSptr>()->getAimId();
            if (delegate_->needsTooltip(aimId, index))
            {
                if (tooltipIndex_ != index)
                {
                    tooltipIndex_ = index;

                    auto ttRect = QRect(mapToGlobal(hoveredRect.topLeft()), hoveredRect.size());
                    auto name = Logic::GetFriendlyContainer()->getFriendly(aimId);
                    const auto verticalPosition = scrollArea_ ? (mapTo(scrollArea_->viewport(), rect().topLeft()).y() + hoveredRect.y()) : 0;
                    const auto needBottom = verticalPosition < 0;
                    auto direction = needBottom ? Tooltip::ArrowDirection::Up : Tooltip::ArrowDirection::Down;
                    auto arrowPosition = needBottom ? Tooltip::ArrowPointPos::Bottom : Tooltip::ArrowPointPos::Top;
                    showTooltip(std::move(name), ttRect, direction, arrowPosition);
                }
            }
            else
            {
                hideTooltip();
            }
        }

        update();
        QWidget::mouseMoveEvent(_event);
    }

    void MembersWidget::mousePressEvent(QMouseEvent* _event)
    {
        clicked_ = _event->pos();
        QWidget::mousePressEvent(_event);
    }

    void MembersWidget::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (Utils::clicked(clicked_, _event->pos()))
        {
            for (int i = 0; i < memberCount_; ++i)
            {
                const auto itemRect = QRect(0, i * params_.itemHeight(), width(), params_.itemHeight());
                if (itemRect.contains(_event->pos()))
                {
                    auto index = model_->index(i);
                    if (index.isValid())
                    {
                        if (const auto aimId = Logic::aimIdFromIndex(index); !aimId.isEmpty())
                        {
                            const auto rSide = width() - Utils::scale_value(8);
                            auto butttonX = rSide - Utils::scale_value(32);
                            if (_event->pos().x() >= butttonX && _event->pos().x() <= rSide)
                            {
                                const auto searchRes = index.data().value<Data::AbstractSearchResultSptr>();
                                const auto& memberRes = std::static_pointer_cast<Data::SearchResultChatMember>(searchRes);
                                const auto canRemove = memberRes->canRemoveTill().isValid() && QDateTime::currentDateTime() < memberRes->canRemoveTill();

                                if (delegate_->regim() == Logic::MembersWidgetRegim::ADMIN_MEMBERS)
                                    Q_EMIT moreClicked(aimId, QPrivateSignal());
                                else if (delegate_->regim() == Logic::MembersWidgetRegim::MEMBERS_LIST || canRemove)
                                    Q_EMIT removeClicked(aimId, QPrivateSignal());
                                else
                                    Q_EMIT selected(aimId, QPrivateSignal());
                            }
                            else
                            {
                                Q_EMIT selected(aimId, QPrivateSignal());
                            }
                        }
                    }
                    break;
                }
            }
        }
        QWidget::mousePressEvent(_event);
    }

    void Ui::MembersWidget::wheelEvent(QWheelEvent* _event)
    {
        hideTooltip();
    }

    void MembersWidget::leaveEvent(QEvent* _event)
    {
        hovered_ = -1;
        hideTooltip();
        update();
    }

    ColoredButton::ColoredButton(QWidget* _parent)
        : QWidget(_parent)
        , isHovered_(false)
        , isActive_(false)
        , height_(Utils::scale_value(COLORED_BUTTON_HEIGHT))
        , textOffset_(0)
    {
        setMargins(Utils::scale_value(HOR_OFFSET), 0, Utils::scale_value(HOR_OFFSET), Utils::scale_value(COLORED_BUTTON_BOTTOM_OFFSET));
        text_ = TextRendering::MakeTextUnit(QString());
        setCursor(Qt::PointingHandCursor);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }

    void ColoredButton::setMargins(const QMargins& _margins)
    {
        if (margins_ == _margins)
            return;
        margins_ = _margins;
        setMinimumHeight(height_ + margins_.bottom() + margins_.top());
        updateGeometry();
    }

    void ColoredButton::setTextOffset(int _offset)
    {
        if (textOffset_ == _offset)
            return;

        textOffset_ = _offset;
        update();
    }

    void ColoredButton::updateTextOffset()
    {
        const auto iconWidth = icon_.width() / Utils::scale_bitmap_ratio();
        auto offset = Utils::scale_value(COLORED_BUTTON_TEXT_OFFSET);
        if (iconWidth != Utils::scale_value(BUTTON_ICON_SIZE))
            offset -= (iconWidth - BUTTON_ICON_SIZE) / 2;

        setTextOffset(offset);
    }

    void ColoredButton::setHeight(int _height)
    {
        if (height_ == _height)
            return;
        height_ = _height;
        setMinimumHeight(height_ + margins_.bottom() + margins_.top());
        updateGeometry();
    }

    void ColoredButton::initColors(const QColor& _base, const QColor& _hover, const QColor& _active)
    {
        base_ = _base;
        hover_ = _hover;
        active_ = _active;
    }

    void ColoredButton::setIcon(const QPixmap& _icon)
    {
        icon_ = _icon;
        updateTextOffset();
        update();
    }

    void ColoredButton::initText(const QFont& _font, const QColor& _color)
    {
        text_->init(_font, _color);
    }

    void ColoredButton::setText(const QString& _text)
    {
        if (text_->getText() == _text)
            return;

        text_->setText(_text);
    }

    void ColoredButton::makeRounded()
    {
        setFixedWidth(height_ + margins_.left() + margins_.right());
        updateGeometry();
    }

    QMargins ColoredButton::getMargins() const
    {
        return margins_;
    }

    QSize ColoredButton::sizeHint() const
    {
        return QSize(width(), height_ + margins_.bottom() + margins_.top());
    }

    QSize ColoredButton::minimumSizeHint() const
    {
        return QSize(height_ + margins_.left() + margins_.right(), height_ + margins_.bottom() + margins_.top());
    }

    void ColoredButton::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        if (!isEnabled())
        {
            p.fillRect(rect(), Qt::transparent);
            return;
        }
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);

        if (isActive_)
            p.setBrush(active_);
        else if (isHovered_)
            p.setBrush(hover_);
        else
            p.setBrush(base_);

        const auto btnRect = rect().marginsRemoved(margins_);
        const auto radius = height_ / 2;
        const auto iconSize = icon_.size() / Utils::scale_bitmap_ratio();
        p.drawRoundedRect(btnRect, radius, radius);

        if (text_->isEmpty())
        {
            p.drawPixmap(btnRect.left() + (btnRect.width() - iconSize.width()) / 2, btnRect.top() + (btnRect.height() - iconSize.height()) / 2, icon_);
        }
        else
        {
            const auto totalWidth = text_->cachedSize().width() + iconSize.width() + textOffset_;
            text_->setOffsets(btnRect.left() + (btnRect.width() - totalWidth) / 2 + iconSize.width() + textOffset_, btnRect.top() + btnRect.height() / 2);
            text_->draw(p, TextRendering::VerPosition::MIDDLE);

            p.drawPixmap(btnRect.left() + (btnRect.width() - totalWidth) / 2, btnRect.top() + (btnRect.height() - iconSize.height()) / 2, icon_);
        }
    }

    void ColoredButton::mousePressEvent(QMouseEvent* _event)
    {
        clicked_ = _event->pos();
        isActive_ = true;
        update();
        QWidget::mousePressEvent(_event);
    }

    void ColoredButton::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (Utils::clicked(clicked_, _event->pos()))
            Q_EMIT clicked(QPrivateSignal());

        isActive_ = false;
        update();
        QWidget::mouseReleaseEvent(_event);
    }

    void ColoredButton::enterEvent(QEvent* _event)
    {
        isHovered_ = true;
        update();
        QWidget::enterEvent(_event);
    }

    void ColoredButton::leaveEvent(QEvent* _event)
    {
        isHovered_ = false;
        update();
        QWidget::leaveEvent(_event);
    }

    EditWidget::EditWidget(QWidget* _parent)
        : QWidget(_parent)
        , avatar_(nullptr)
        , avatarSpacer_(nullptr)
        , name_(nullptr)
        , nameSpacer_(nullptr)
        , description_(nullptr)
        , descriptionSpacer_(nullptr)
        , rules_(nullptr)
        , rulesSpacer_(nullptr)
        , avatarChanged_(false)
        , nameOnly_(false)
    {
    }

    void EditWidget::init(const QString& _aimid, const QString& _name, const QString& _description, const QString& _rules)
    {
        auto layout = Utils::emptyVLayout(this);
        layout->addSpacerItem(new QSpacerItem(0, Utils::scale_value(EDIT_VER_OFFSET), QSizePolicy::Preferred, QSizePolicy::Fixed));
        {
            auto widget = new QWidget(this);
            Utils::emptyContentsMargins(widget);
            Utils::transparentBackgroundStylesheet(widget);
            auto hLayout = Utils::emptyHLayout(widget);
            hLayout->setAlignment(Qt::AlignHCenter);
            avatar_ = new ContactAvatarWidget(this, _aimid, _name, Utils::scale_value(EDIT_AVATAR_SIZE), true);
            Testing::setAccessibleName(avatar_, qsl("AS EditWidget avatar"));
            avatar_->SetMode(Ui::ContactAvatarWidget::Mode::ChangeAvatar);
            connect(avatar_, &Ui::ContactAvatarWidget::avatarDidEdit, this, [this]() { avatarChanged_ = true; });
            hLayout->addWidget(avatar_);
            layout->addWidget(widget);
        }

        avatarSpacer_ = new QWidget(this);
        Utils::transparentBackgroundStylesheet(avatarSpacer_);
        avatarSpacer_->setFixedHeight(EDIT_VER_OFFSET);
        layout->addWidget(avatarSpacer_);

        {
            auto widget = new QWidget(this);
            Utils::emptyContentsMargins(widget);
            Utils::transparentBackgroundStylesheet(widget);
            auto hLayout = Utils::emptyHLayout(widget);
            hLayout->setAlignment(Qt::AlignHCenter);
            name_ = new Ui::TextEditEx(this, Fonts::appFontScaled(EDIT_FONT_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), true, true);
            Testing::setAccessibleName(name_, qsl("AS EditWidget nickName"));
            Utils::ApplyStyle(name_, Styling::getParameters().getTextEditCommonQss(true));
            name_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
            name_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            name_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
            name_->setFrameStyle(QFrame::NoFrame);
            name_->document()->setDocumentMargin(0);
            name_->addSpace(Utils::scale_value(4));
            name_->setMaxHeight(Utils::scale_value(EDIT_MAX_ENTER_HEIGHT));
            Logic::Text4Edit(_name, *name_, Logic::Text2DocHtmlMode::Pass, false);
            name_->setPlaceholderText(QT_TRANSLATE_NOOP("sidebar", "Name"));

            hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(EDIT_HOR_OFFSET), 0, QSizePolicy::Fixed));
            hLayout->addWidget(name_);
            hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(EDIT_HOR_OFFSET), 0, QSizePolicy::Fixed));
            layout->addWidget(widget);
        }

        nameSpacer_ = new QWidget(this);
        Utils::transparentBackgroundStylesheet(nameSpacer_);
        nameSpacer_->setFixedHeight(EDIT_VER_OFFSET);
        layout->addWidget(nameSpacer_);

        {
            auto widget = new QWidget(this);
            Utils::emptyContentsMargins(widget);
            Utils::transparentBackgroundStylesheet(widget);
            auto hLayout = Utils::emptyHLayout(widget);
            hLayout->setAlignment(Qt::AlignHCenter);
            description_ = new Ui::TextEditEx(this, Fonts::appFontScaled(EDIT_FONT_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), true, true);
            Testing::setAccessibleName(description_, qsl("AS EditWidget description"));
            Utils::ApplyStyle(description_, Styling::getParameters().getTextEditCommonQss(true));
            description_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
            description_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            description_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
            description_->setEnterKeyPolicy(TextEditEx::EnterKeyPolicy::CatchNewLine);
            description_->setFrameStyle(QFrame::NoFrame);
            description_->document()->setDocumentMargin(0);
            description_->addSpace(Utils::scale_value(4));
            description_->setMaxHeight(Utils::scale_value(EDIT_MAX_ENTER_HEIGHT));
            Logic::Text4Edit(_description, *description_, Logic::Text2DocHtmlMode::Pass, false);
            description_->setPlaceholderText(QT_TRANSLATE_NOOP("sidebar", "Description"));
            hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(EDIT_HOR_OFFSET), 0, QSizePolicy::Fixed));
            hLayout->addWidget(description_);
            hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(EDIT_HOR_OFFSET), 0, QSizePolicy::Fixed));
            layout->addWidget(widget);
        }

        descriptionSpacer_ = new QWidget(this);
        Utils::transparentBackgroundStylesheet(descriptionSpacer_);
        descriptionSpacer_->setFixedHeight(EDIT_VER_OFFSET);
        layout->addWidget(descriptionSpacer_);

        {
            auto widget = new QWidget(this);
            Utils::emptyContentsMargins(widget);
            Utils::transparentBackgroundStylesheet(widget);
            auto hLayout = Utils::emptyHLayout(widget);
            hLayout->setAlignment(Qt::AlignHCenter);
            rules_ = new Ui::TextEditEx(this, Fonts::appFontScaled(EDIT_FONT_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), true, true);
            Testing::setAccessibleName(rules_, qsl("AS EditWidget rules"));
            Utils::ApplyStyle(rules_, Styling::getParameters().getTextEditCommonQss(true));
            rules_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
            rules_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            rules_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
            rules_->setEnterKeyPolicy(TextEditEx::EnterKeyPolicy::CatchNewLine);
            rules_->setFrameStyle(QFrame::NoFrame);
            rules_->document()->setDocumentMargin(0);
            rules_->addSpace(Utils::scale_value(4));
            rules_->setMaxHeight(Utils::scale_value(EDIT_MAX_ENTER_HEIGHT));
            Logic::Text4Edit(_rules, *rules_, Logic::Text2DocHtmlMode::Pass, false);
            rules_->setPlaceholderText(QT_TRANSLATE_NOOP("sidebar", "Rules"));
            hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(EDIT_HOR_OFFSET), 0, QSizePolicy::Fixed));
            hLayout->addWidget(rules_);
            hLayout->addSpacerItem(new QSpacerItem(Utils::scale_value(EDIT_HOR_OFFSET), 0, QSizePolicy::Fixed));
            layout->addWidget(widget);
        }

        rulesSpacer_ = new QWidget(this);
        Utils::transparentBackgroundStylesheet(rulesSpacer_);
        rulesSpacer_->setFixedHeight(EDIT_VER_OFFSET);
        layout->addWidget(rulesSpacer_);
    }

    QString EditWidget::name() const
    {
        return name_ ? name_->getPlainText() : QString();
    }

    QString EditWidget::desription() const
    {
        return description_ ? description_->getPlainText() : QString();
    }

    QString EditWidget::rules() const
    {
        return rules_ ? rules_->getPlainText() : QString();
    }

    bool EditWidget::avatarChanged() const
    {
        return avatarChanged_;
    }

    QPixmap EditWidget::getAvatar() const
    {
        return avatar_->croppedImage();
    }

    void EditWidget::nameOnly(bool _nameOnly)
    {
        if (nameOnly_ == _nameOnly)
            return;

        nameOnly_ = _nameOnly;

        avatar_->setVisible(!_nameOnly);
        avatarSpacer_->setVisible(!_nameOnly);
        nameSpacer_->setVisible(!_nameOnly);
        description_->setVisible(!_nameOnly);
        descriptionSpacer_->setVisible(!_nameOnly);
        rules_->setVisible(!_nameOnly);
        rulesSpacer_->setVisible(!_nameOnly);
    }

    void EditWidget::showEvent(QShowEvent* _event)
    {
        if (nameOnly_)
            name_->setFocus();
        else
            description_->setFocus();

        QWidget::showEvent(_event);
    }

    QPixmap editGroup(const QString& _aimid, QString& _name, QString& _description, QString& _rules)
    {
        auto w = new EditWidget(nullptr);
        w->init(_aimid, _name, _description, _rules);

        auto generalDialog = std::make_unique<GeneralDialog>(w, Utils::InterConnector::instance().getMainWindow());
        generalDialog->addButtonsPair(QT_TRANSLATE_NOOP("report_widget", "Cancel"), QT_TRANSLATE_NOOP("report_widget", "OK"), true);
        if (generalDialog->showInCenter())
        {
            _name = w->name();
            _description = w->desription();
            _rules = w->rules();
            if (w->avatarChanged())
                return w->getAvatar();
        }

        return QPixmap();
    }

    QString editName(const QString& _name)
    {
        auto w = new EditWidget(nullptr);
        w->init(QString(), _name, QString(), QString());
        w->nameOnly(true);

        auto generalDialog = std::make_unique<GeneralDialog>(w, Utils::InterConnector::instance().getMainWindow());
        generalDialog->addLabel(QT_TRANSLATE_NOOP("sidebar", "Contact name"));
        generalDialog->addButtonsPair(QT_TRANSLATE_NOOP("report_widget", "Cancel"), QT_TRANSLATE_NOOP("report_widget", "OK"), true);
        if (generalDialog->showInCenter())
            return w->name();

        return QString();
    }

    QByteArray avatarToByteArray(const QPixmap &_avatar)
    {
        auto avatar = _avatar;
        if (std::max(avatar.width(), avatar.height()) > 1024)
        {
            if (avatar.width() > avatar.height())
                avatar = avatar.scaledToWidth(1024, Qt::SmoothTransformation);
            else
                avatar = avatar.scaledToHeight(1024, Qt::SmoothTransformation);
        }

        QByteArray result;
        do
        {
            result.clear();
            QBuffer buffer(&result);
            avatar.save(&buffer, "png");
            avatar = avatar.scaled(avatar.size() / 2, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        } while (result.size() > 8 * 1024 * 1024);

        return result;
    }

    int getIconSize()
    {
        return Utils::scale_value(BUTTON_ICON_SIZE);
    }

    QString formatTimeStr(const QDateTime& _dt)
    {
        return _dt.toString(u"d MMM, ") % _dt.time().toString(u"hh:mm");
    }

    void setWidgetFading(QWidget* _target)
    {
        if (!_target)
            return;

        auto fader = new Utils::WidgetFader(_target);
        fader->setDuration(SidebarPage::kShowDuration);
        fader->setEventDirection(QEvent::Show, QPropertyAnimation::Forward);
    }

    AvatarNameInfo* addAvatarInfo(QWidget* _parent, QLayout* _layout)
    {
        auto w = new AvatarNameInfo(_parent);
        w->setMargins(Utils::scale_value(HOR_OFFSET), 0, Utils::scale_value(HOR_OFFSET), 0);
        w->setTextOffset(Utils::scale_value(AVATAR_NAME_OFFSET));
        w->initName(Fonts::appFontScaled(16, platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        w->initInfo(Fonts::appFontScaled(14), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        w->setMinimumHeight(Utils::scale_value(AVATAR_INFO_HEIGHT));

        if (_layout)
            _layout->addWidget(w);

        return w;
    }

    TextLabel* addLabel(const QString& _text, QWidget* _parent, QLayout* _layout, int _addLeftOffset, TextRendering::HorAligment _align, Fading _faded)
    {
        TextLabel* label = new TextLabel(_parent);
        if (_faded == Fading::On)
            setWidgetFading(label);

        label->setMargins(Utils::scale_value(HOR_OFFSET) + _addLeftOffset, 0, Utils::scale_value(HOR_OFFSET), 0);
        label->setTextAlign(_align);
        label->init(Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        label->setText(_text);
        if (_layout)
            _layout->addWidget(label);

        return label;
    }

    TextLabel* addText(const QString& _text, QWidget* _parent, QLayout* _layout, int _addLeftOffset, int _maxLineNumbers, Fading _faded)
    {
        TextLabel* label = new TextLabel(_parent, _maxLineNumbers == 0 ? MAX_INFO_LINES_COUNT : _maxLineNumbers);
        if (_faded == Fading::On)
            setWidgetFading(label);

        label->setMargins(Utils::scale_value(HOR_OFFSET) + _addLeftOffset, 0, Utils::scale_value(HOR_OFFSET), 0);
        label->init(Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
        label->setText(_text);
        if (_layout)
            _layout->addWidget(label);

        return label;
    }

    std::unique_ptr<InfoBlock> addInfoBlock(const QString& _header, const QString& _text, QWidget* _parent, QLayout* _layout, int _addLeftOffset, Fading _faded)
    {
        auto infoBlock = std::make_unique<InfoBlock>();
        infoBlock->header_ = addLabel(_header, _parent, _layout, _addLeftOffset, TextRendering::HorAligment::LEFT, _faded);
        {
            auto m = infoBlock->header_->getMargins();
            infoBlock->header_->setMargins(m.left(), m.top(), m.right(), Utils::scale_value(INFO_HEADER_BOTTOM_MARGIN));
        }
        infoBlock->text_ = addText(_text, _parent, _layout, _addLeftOffset, 0, _faded);
        {
            auto m = infoBlock->text_->getMargins();
            infoBlock->text_->setMargins(m.left(), m.top(), m.right(), Utils::scale_value(INFO_TEXT_BOTTOM_MARGIN));
        }
        return infoBlock;
    }

    QWidget* addSpacer(QWidget* _parent, QLayout* _layout, int _height)
    {
        auto w = new QWidget(_parent);
        w->setStyleSheet(qsl("background: transparent"));
        auto l = Utils::emptyVLayout(w);
        auto spacer = new QWidget(w);
        spacer->setStyleSheet(ql1s("background: %1").arg(Styling::getParameters().getColor(Styling::StyleVariable::BASE_LIGHT).name()));
        spacer->setFixedHeight(_height == -1 ? Utils::scale_value(SPACER_HEIGHT) : _height);
        l->addWidget(spacer);
        w->setFixedHeight(Utils::scale_value(SPACER_HEIGHT) + Utils::scale_value(SPACER_MARGIN) * 2);
        if (_layout)
            _layout->addWidget(w);

        return w;
    }

    SidebarButton* addButton(const QString& _icon, const QString& _text, QWidget* _parent, QLayout* _layout, const QString& _accName)
    {
        auto w = new SidebarButton(_parent);
        w->setFixedHeight(Utils::scale_value(BUTTON_HEIGHT));
        w->setMargins(Utils::scale_value(BUTTON_HOR_OFFSET), 0, Utils::scale_value(HOR_OFFSET), 0);
        w->setTextOffset(Utils::scale_value(HOR_OFFSET));
        w->setIcon(Utils::renderSvgScaled(_icon, QSize(BUTTON_ICON_SIZE, BUTTON_ICON_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY)));
        w->initText(Fonts::appFontScaledFixed(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        w->setText(_text);
        w->initCounter(Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        w->setColors(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));

        if (Testing::isEnabled() && !_accName.isEmpty())
            Testing::setAccessibleName(w, _accName);

        if (_layout)
            _layout->addWidget(w);

        return w;
    }

    SidebarCheckboxButton* addCheckbox(const QString& _icon, const QString& _text, QWidget* _parent, QLayout* _layout, Fading _faded)
    {
        auto w = new SidebarCheckboxButton(_parent);
        if (_faded == Fading::On)
            setWidgetFading(w);

        w->setFixedHeight(Utils::scale_value(BUTTON_HEIGHT));
        w->setMargins(Utils::scale_value(BUTTON_HOR_OFFSET), 0, Utils::scale_value(HOR_OFFSET), 0);
        if (!_icon.isEmpty())
        {
            w->setTextOffset(Utils::scale_value(HOR_OFFSET));
            w->setIcon(Utils::renderSvgScaled(_icon, QSize(BUTTON_ICON_SIZE, BUTTON_ICON_SIZE), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY)));
        }

        w->initText(Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        w->setText(_text);
        w->initCounter(Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        w->setColors(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));

        if (_layout)
            _layout->addWidget(w);

        return w;
    }

    GalleryPreviewWidget* addGalleryPrevieWidget(QWidget* _parent, QLayout* _layout)
    {
        auto w = new GalleryPreviewWidget(_parent, Utils::scale_value(GALLERY_PREVIEW_SIZE), GALLERY_PREVIEW_COUNT);
        w->setFixedHeight(Utils::scale_value(GALLERY_WIDGET_HEIGHT));
        w->setMargins(Utils::scale_value(GALLERY_WIDGET_MARGIN_LEFT), Utils::scale_value(GALLERY_WIDGET_MARGIN_TOP), Utils::scale_value(GALLERY_WIDGET_MARGIN_RIGHT), Utils::scale_value(GALLERY_WIDGET_MARGIN_BOTTOM));
        w->setSpacing(Utils::scale_value(GALLERY_WIDGET_SPACING));
        if (_layout)
            _layout->addWidget(w);

        return w;
    }

    MembersPlate* addMembersPlate(QWidget* _parent, QLayout* _layout)
    {
        auto w = new MembersPlate(_parent);
        w->setMargins(Utils::scale_value(MEMBERS_LEFT_MARGIN), Utils::scale_value(MEMBERS_TOP_MARGIN), Utils::scale_value(MEMBERS_RIGHT_MARGIN), Utils::scale_value(MEMBERS_BOTTOM_MARGIN));
        w->initMembersLabel(Fonts::appFontScaledFixed(12, Fonts::FontWeight::SemiBold), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        w->initSearchLabel(Fonts::appFontScaledFixed(12, Fonts::FontWeight::SemiBold), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
        if (_layout)
            _layout->addWidget(w);

        return w;
    }

    MembersWidget* addMembersWidget(Logic::ChatMembersModel* _model,  Logic::ContactListItemDelegate* _delegate, int _membersCount, QWidget* _parent, QLayout* _layout)
    {
        auto w = new MembersWidget(_parent, _model, _delegate, _membersCount);
        if (_layout)
            _layout->addWidget(w);

        return w;
    }

    ColoredButton* addColoredButton(const QString& _icon, const QString& _text, QWidget* _parent, QLayout* _layout, const QSize& _iconSize, Fading _faded)
    {
        auto w = new ColoredButton(_parent);
        if (_faded == Fading::On)
            setWidgetFading(w);

        w->setHeight(Utils::scale_value(COLORED_BUTTON_HEIGHT));
        w->setMargins(Utils::scale_value(HOR_OFFSET), 0, Utils::scale_value(HOR_OFFSET), Utils::scale_value(COLORED_BUTTON_BOTTOM_OFFSET));
        if (!_icon.isEmpty())
        {
            const auto size = _iconSize.isValid() ? _iconSize : QSize(BUTTON_ICON_SIZE, BUTTON_ICON_SIZE);
            w->setIcon(Utils::renderSvgScaled(_icon, size, Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE)));
        }

        w->initColors(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_HOVER), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_ACTIVE));
        w->initText(Fonts::appFontScaled(16), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        w->setText(_text);

        const auto diff = _iconSize.isValid() ? (_iconSize.width() - BUTTON_ICON_SIZE) / 2 : 0;
        if (diff != 0)
            w->setTextOffset(Utils::scale_value(COLORED_BUTTON_TEXT_OFFSET - diff));

        if (_layout)
            _layout->addWidget(w);

        return w;
    }

    GalleryPopup::GalleryPopup()
        : QWidget(nullptr, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
    {
        setAttribute(Qt::WA_TranslucentBackground);

        auto hL = Utils::emptyHLayout(this);
        auto galleryWidget = new QFrame(this);
        auto style = ql1s("background: %3; border: %1px solid %4; border-radius: %2px;");
        galleryWidget->setStyleSheet(style.arg(QString::number(Utils::scale_value(1)), QString::number(Utils::scale_value(4)),
            Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_GLOBALWHITE),
            Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_BRIGHT)));

        auto verLayout = Utils::emptyVLayout(galleryWidget);
        verLayout->setContentsMargins(0, Utils::scale_value(4), 0, Utils::scale_value(4));
        {
            galleryPhoto_ = addButton(qsl(":/background_icon"), QT_TRANSLATE_NOOP("sidebar", "Photo and video"), galleryWidget, verLayout);
            connect(galleryPhoto_, &SidebarButton::clicked, this, [this]() { Q_EMIT galleryPhotoClicked(QPrivateSignal()); });

            galleryVideo_ = addButton(qsl(":/video_icon"), QT_TRANSLATE_NOOP("sidebar", "Video"), galleryWidget, verLayout);
            connect(galleryVideo_, &SidebarButton::clicked, this, [this]() { Q_EMIT galleryVideoClicked(QPrivateSignal()); });

            galleryFiles_ = addButton(qsl(":/gallery/file_icon"), QT_TRANSLATE_NOOP("sidebar", "Files"), galleryWidget, verLayout);
            connect(galleryFiles_, &SidebarButton::clicked, this, [this]() { Q_EMIT galleryFilesClicked(QPrivateSignal()); });

            galleryLinks_ = addButton(qsl(":/copy_link_icon"), QT_TRANSLATE_NOOP("sidebar", "Links"), galleryWidget, verLayout);
            connect(galleryLinks_, &SidebarButton::clicked, this, [this]() { Q_EMIT galleryLinksClicked(QPrivateSignal()); });

            galleryPtt_ = addButton(qsl(":/gallery/micro_icon"), QT_TRANSLATE_NOOP("sidebar", "Voice messages"), galleryWidget, verLayout);
            connect(galleryPtt_, &SidebarButton::clicked, this, [this]() { Q_EMIT galleryPttClicked(QPrivateSignal()); });
        }
        hL->addWidget(galleryWidget);
    }

    void GalleryPopup::setCounters(int _photo, int _video, int _files, int _links, int _ptt)
    {
        galleryPhoto_->setCounter(_photo);
        galleryVideo_->setCounter(_video);
        galleryFiles_->setCounter(_files);
        galleryLinks_->setCounter(_links);
        galleryPtt_->setCounter(_ptt);
    }

    void GalleryPopup::setCounters(const Data::DialogGalleryState& _state)
    {
        setCounters(
            countForType(_state, MediaContentType::ImageVideo),
            countForType(_state, MediaContentType::Video),
            countForType(_state, MediaContentType::Files),
            countForType(_state, MediaContentType::Links),
            countForType(_state, MediaContentType::Voice)
        );
    }

    int GalleryPopup::horOffset()
    {
        return Utils::scale_value(8);
    }

    int GalleryPopup::verOffset()
    {
        return Utils::scale_value(52);
    }

    int GalleryPopup::itemsCount() const
    {
        const auto items = findChildren<SidebarButton*>();
        return std::count_if(items.begin(), items.end(), [this](auto _item) { return _item->isVisibleTo(this); });
    }

    BlockAndDeleteWidget::BlockAndDeleteWidget(QWidget* _parent, const QString& _friendly, const QString& _chatAimid)
        : QWidget(_parent)
        , removeMessages_(false)
    {
        checkbox_ = new Ui::CheckBox(this);
        checkbox_->setText(QT_TRANSLATE_NOOP("block_and_delete", "Delete messages"));
        checkbox_->setChecked(removeMessages_);
        checkbox_->move(Utils::scale_value(BLOCK_AND_DELETE_HOR_OFFSET), Utils::scale_value(BLOCK_AND_DELETE_TOP_OFFSET));

        const auto isChannel = Logic::getContactListModel()->isChannel(_chatAimid);

        label_ = Ui::TextRendering::MakeTextUnit(isChannel ? QT_TRANSLATE_NOOP("block_and_delete", "This member won't be able to join the channel again. You could also delete his messages")
            : QT_TRANSLATE_NOOP("block_and_delete", "This member won't be able to join the group again. You could also delete his messages"));
        label_->init(Fonts::appFontScaled(15), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
    }

    void BlockAndDeleteWidget::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        label_->setOffsets(Utils::scale_value(BLOCK_AND_DELETE_HOR_OFFSET), checkbox_->height() + Utils::scale_value(BLOCK_AND_DELETE_TOP_OFFSET + BLOCK_AND_DELETE_ADD_OFFSET));
        label_->draw(p);

        QWidget::paintEvent(_event);
    }

    bool BlockAndDeleteWidget::needToRemoveMessages() const
    {
        return checkbox_->isChecked();
    }

    void BlockAndDeleteWidget::resizeEvent(QResizeEvent* _event)
    {
        const auto maxWidth = _event->size().width() - Utils::scale_value(BLOCK_AND_DELETE_HOR_OFFSET * 2);
        checkbox_->setFixedWidth(maxWidth);
        setFixedHeight(label_->getHeight(maxWidth) + Utils::scale_value(BLOCK_AND_DELETE_TOP_OFFSET + BLOCK_AND_DELETE_BOTTOM_OFFSET + BLOCK_AND_DELETE_ADD_OFFSET) + checkbox_->height());
        QWidget::resizeEvent(_event);
    }

}


