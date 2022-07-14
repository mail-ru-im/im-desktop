#include "stdafx.h"
#include "RecentItemDelegate.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../proxy/AvatarStorageProxy.h"

#include "ContactListModel.h"
#include "RecentsModel.h"
#include "UnknownsModel.h"
#include "../containers/FriendlyContainer.h"
#include "../containers/LastseenContainer.h"
#include "../proxy/FriendlyContaInerProxy.h"

#include "RecentsTab.h"
#include "../history_control/LastStatusAnimation.h"
#include "../mediatype.h"

#include "../../types/contact.h"
#include "../../utils/utils.h"
#include "../../utils/features.h"
#include "../../controls/textrendering/TextRenderingUtils.h"

#include "../history_control/HistoryControlPageItem.h"

#include "../../gui_settings.h"
#include "../../styles/ThemeParameters.h"
#include "../../styles/StyleVariable.h"

#include <boost/range/adaptor/reversed.hpp>
#include "main_window/LocalPIN.h"
#include "main_window/contact_list/FavoritesUtils.h"

#include "../../../common.shared/config/config.h"
#include "IconsDelegate.h"
#include "IconsManager.h"

namespace Ui
{
    //////////////////////////////////////////////////////////////////////////
    // RecentItemBase
    //////////////////////////////////////////////////////////////////////////

    int get_badge_size() noexcept
    {
        return Utils::scale_value(18);
    }

    int get_badge_mention_size() noexcept
    {
        return Utils::scale_value(16);
    }

    QSize getPinHeaderSize() noexcept
    {
        return Utils::scale_value(QSize(16, 16));
    }

    QSize getPinSize() noexcept
    {
        return Utils::scale_value(QSize(12, 12));
    }

    Utils::StyledPixmap getPin(const bool _selected, const QSize& _sz)
    {
        return Utils::StyledPixmap(qsl(":/pin_icon"), _sz, Styling::ThemeColorKey{ _selected ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::BASE_TERTIARY });
    }

    QPixmap getRemove(const bool _isSelected)
    {
        static auto rem = Utils::StyledPixmap::scaled(qsl(":/ignore_icon"), QSize(20, 20), Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION });
        static auto remSelected = Utils::StyledPixmap::scaled(qsl(":/ignore_icon"), QSize(20, 20), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT });

        return (_isSelected ? remSelected : rem).actualPixmap();
    }

    QPixmap getAdd(const bool _isSelected)
    {
        static auto add = Utils::StyledPixmap::scaled(qsl(":/controls/add_icon"), QSize(20, 20), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY });
        static auto addSelected = Utils::StyledPixmap::scaled(qsl(":/controls/add_icon"), QSize(20, 20), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY_ACTIVE });

        return (_isSelected ? addSelected : add).actualPixmap();
    }

    int getPinPadding() noexcept
    {
        return Utils::scale_value(12);
    }

    int getUnreadsSize() noexcept
    {
        return Utils::scale_value(20);
    }

    constexpr QSize getContactsIconSize() noexcept
    {
        return QSize(20, 20);
    }

    QPixmap getContactsIcon()
    {
        return Utils::renderSvgScaled(qsl(":/contacts_icon"), getContactsIconSize());
    }

    QPixmap getMailIcon(const int _size, const bool _isSelected)
    {
        return Utils::renderSvg(qsl(":/mail_icon"), QSize(_size, _size));
    }

    int getUnreadBubbleMarginPictOnly() noexcept
    {
        return Utils::scale_value(4);
    }

    int getMessageY() noexcept
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(23);
        else
            return Utils::scale_value(28);
    }

    int getMultiChatMessageOffset() noexcept
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(1);
        else
            return 0;
    }

    int getUnreadY() noexcept
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(32);
        else
            return Utils::scale_value(36);
    }

    int getNameY(const bool _compactMode) noexcept
    {
        return _compactMode ? Utils::scale_value(13) : platform::is_apple() ? Utils::scale_value(6) : Utils::scale_value(8);
    }

    int getMessageLineSpacing() noexcept
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(1);
        else
            return Utils::scale_value(0/*-2*/);
    }

    auto senderColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
    }

    auto draftColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY };
    }

    auto getNameColor(bool _isSelected, bool _isDraft)
    {
        if (_isSelected)
            return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT };
        return _isDraft ? draftColor() : senderColor();
    }

    constexpr Fonts::FontWeight getNameFontWeight(bool _isDraft) noexcept
    {
        return _isDraft ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal;
    }

    auto getContactNameColor(const bool _isSelected, const bool _isFavorites = false)
    {
        if (_isSelected)
            return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT };
        else if (_isFavorites)
            return Favorites::nameColor();

        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
    }

    int getAlertAvatarSize() noexcept
    {
        return Utils::scale_value(60);
    }

    int getAlertAvatarX() noexcept
    {
        return Utils::scale_value(20);
    }

    int getMailAlertIconX() noexcept
    {
        return Utils::scale_value(20);
    }

    int getAlertMessageY() noexcept
    {
        return Utils::scale_value(40);
    }

    int getAlertMessageRightMargin() noexcept
    {
        return Utils::scale_value(20);
    }

    int getMailIconSize() noexcept
    {
        return Utils::scale_value(60);
    }

    int getAvatarIconBorderSize() noexcept
    {
        return Utils::scale_value(4);
    }

    int getMailAvatarIconSize() noexcept
    {
        return Utils::scale_value(20);
    }

    int getAvatarIconMargin() noexcept
    {
        return Utils::scale_value(12);
    }

    int getMentionAvatarIconSize() noexcept
    {
        return Utils::scale_value(20);
    }

    QColor getAlertUnderlineColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
    }

    auto getMessageColor(bool _selected)
    {
        return Styling::ThemeColorKey{ _selected ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::BASE_PRIMARY };
    }

    QColor getDateColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
    }

    auto getServiceItemColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
    }

    int getServiceItemHorPadding() noexcept
    {
        return Utils::scale_value(12);
    }

    int getMediaTypeIconTopMargin(const bool _multichat) noexcept
    {
        if constexpr (platform::is_apple())
            return (_multichat ? Utils::scale_value(42) : Utils::scale_value(26));
        else
            return (_multichat ? Utils::scale_value(48) : Utils::scale_value(30));
    }

    int getMediaTypeIconTopMarginAlert(const bool _multichat) noexcept
    {
        return (_multichat ? Utils::scale_value(60) : Utils::scale_value(42));
    }

    int get2LineMessageOffset() noexcept
    {
        return Utils::scale_value(18);
    }

    constexpr int getUnknownTextSize() noexcept
    {
        return 16;
    }

    auto getUnknownTextColor()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_SECONDARY };
    }

    int getUnknownHorPadding() noexcept
    {
        return Utils::scale_value(12);
    }

    int getMessageRightMargin() noexcept
    {
        return Utils::scale_value(4);
    }

    int getTimeYOffsetFromName() noexcept
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(3);
        else
            return Utils::scale_value(4);
    }

    // COLOR
    QColor getHeadsBorderColor(const bool _selected, const bool _hovered)
    {
        if (_selected)
            return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_SELECTED);
        else if (_hovered)
            return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
        else
            return Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
    }

    const QPixmap& getArrowIcon(const bool _rotated)
    {
        static QPixmap normal, mirrored;

        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash() || normal.isNull())
        {
            normal = Utils::renderSvgScaled(qsl(":/controls/down_icon"), QSize(12, 12), Styling::getColor(getServiceItemColor()));
            mirrored = Utils::mirrorPixmapVer(normal);
        }

        return _rotated ? mirrored : normal;
    }

    auto getUnreadsCountersBackgroundColorForServiceItemKey(bool _isSelected)
    {
        return Styling::ThemeColorKey{ _isSelected ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::PRIMARY_BRIGHT };
    }

    QColor getUnreadsCountersBackgroundColorForServiceItem(bool _isSelected)
    {
        return Styling::getColor(getUnreadsCountersBackgroundColorForServiceItemKey(_isSelected));
    }

    auto getUnreadsCounterForegroundColorServiceItemKey(bool _isSelected)
    {
        return Styling::ThemeColorKey{ _isSelected ? Styling::StyleVariable::PRIMARY_SELECTED : Styling::StyleVariable::TEXT_PRIMARY };
    }

    QColor getUnreadsCounterForegroundColorServiceItem(bool _isSelected)
    {
        return Styling::getColor(getUnreadsCounterForegroundColorServiceItemKey(_isSelected));
    }

    Utils::LayeredPixmap getMentionIconServiceItem(bool _isSelected, const QSize _size = QSize())
    {
        const auto backgroundColor = getUnreadsCountersBackgroundColorForServiceItemKey(_isSelected);
        const auto foregroundColor = getUnreadsCounterForegroundColorServiceItemKey(_isSelected);

        const auto svgLayers = Utils::ColorParameterLayers {
            { qsl("bg"), backgroundColor },
            { qsl("dog"), foregroundColor },
        };

        return Utils::LayeredPixmap(qsl(":/recent_mention_icon"), svgLayers, _size.isValid() ? _size : QSize());
    }

    bool isPinnedServiceItem(const Data::DlgState& _state)
    {
        return _state.pinnedServiceItemType() != Data::DlgState::PinnedServiceItemType::NonPinnedItem;
    }

    auto pinnedServiceItemIconCircleColor(bool _selected)
    {
        return _selected ? Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_BRIGHT) : Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
    }

    bool isMessageAlertTextHidden()
    {
        if (Features::hideMessageInfoEnabled() || Features::hideMessageTextEnabled() || LocalPIN::instance()->locked())
            return true;

        return get_gui_settings()->get_value<bool>(settings_hide_message_notification, false);
    }

    bool isMessageAlertSenderHidden()
    {
        if (Features::hideMessageInfoEnabled())
            return true;
        return get_gui_settings()->get_value<bool>(settings_hide_sender_notification, false);
    }

    RecentItemBase::RecentItemBase(const Data::DlgState& _state)
        : aimid_(_state.AimId_)
    {
    }

    RecentItemBase::~RecentItemBase() = default;

    void RecentItemBase::draw(QPainter& _p, const QRect& _rect, const Ui::ViewParams& _viewParams, const bool _isSelected, const bool _isHovered, const bool _isDrag, const bool _isKeyboardFocused) const
    {
    }

    void RecentItemBase::drawMouseState(QPainter& _p, const QRect& _rect, const bool _isHovered, const bool _isSelected, const bool _isKeyboardFocused) const
    {
        if (!_isHovered && !_isSelected)
            return;

        QColor color;
        if (_isSelected)
        {
            color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_SELECTED);
        }
        else if (_isHovered)
        {
            if (_isKeyboardFocused)
                color = Styling::getParameters().getPrimaryTabFocusColor();
            else
                color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
        }

        if (color.isValid())
            _p.fillRect(_rect, color);
    }

    const QString& RecentItemBase::getAimid() const
    {
        return aimid_;
    }

    PinnedServiceItem::PinnedServiceItem(const Data::DlgState& _state)
        : RecentItemBase(_state)
        , type_(_state.pinnedServiceItemType())
        , unreadCount_(_state.UnreadCount_)
        , unreadMentionsCount_(_state.unreadMentionsCount_)
    {
        im_assert(type_ != Data::DlgState::PinnedServiceItemType::NonPinnedItem);
        if (type_ == Data::DlgState::PinnedServiceItemType::NonPinnedItem)
            type_ = Data::DlgState::PinnedServiceItemType::Favorites;
        text_ = TextRendering::MakeTextUnit(Data::DlgState::PinnedServiceItemType::Favorites == _state.pinnedServiceItemType() ? QT_TRANSLATE_NOOP("favorites", "Favorites") : _state.GetText());
        const auto& contactListParams = Ui::GetContactListParams();
        TextRendering::TextUnit::InitializeParameters params{ Fonts::appFont(contactListParams.contactNameFontSize(), contactListParams.contactNameFontWeight()), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } };
        params.maxLinesCount_ = 1;
        text_->init(params);
        text_->evaluateDesiredSize();
        text_->setOffsets(contactListParams.getContactNameX(), contactListParams.itemHeight() / 2);

        connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStateChanged, this,[](const Data::DlgState& _dlgState)
        {
            Q_UNUSED(_dlgState);
            const auto model = Logic::getRecentsModel();
            const auto index = model->index(0);
            Q_EMIT model->dataChanged(index, index);
        });
    }

    PinnedServiceItem::~PinnedServiceItem() = default;

    bool PinnedServiceItem::needDrawDraft(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const
    {
        bool hasDraft = false;
        if (type_ == Data::DlgState::PinnedServiceItemType::Favorites)
            hasDraft = _state.hasDraft();
        return hasDraft && !_viewParams.pictOnly_;
    }

    bool PinnedServiceItem::needDrawUnreads(const Data::DlgState& _state) const
    {
        return unreadCount_;
    }

    bool PinnedServiceItem::needDrawMentions(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const
    {
        return unreadMentionsCount_;
    }

    void PinnedServiceItem::drawDraft(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX) const
    {
        if (!needDrawDraft(_viewParams, _state) || !_painter)
            return;

        const auto draftSize = Logic::IconsManager::getDraftSize();
        const QPixmap badge = Logic::IconsManager::getDraftIcon(_isSelected).actualPixmap();
        const auto& contactListParams = Ui::GetContactListParams();
        auto unreadsY = (contactListParams.itemHeight() - getUnreadsSize()) / 2;

        if (needDrawUnreads(_state))
            _unreadsX -= Logic::IconsManager::getUnreadBalloonWidth(unreadCount_);
        if (needDrawMentions(_viewParams, _state))
            _unreadsX -= Logic::IconsManager::getUnreadLeftPadding();
        _unreadsX -= draftSize.width();

        _painter->drawPixmap(_rect.left() + _unreadsX, (contactListParams.itemHeight() - draftSize.height()) / 2, badge);

        draftIconRect_ = QRect(_rect.left() + _unreadsX, unreadsY, badge.width(), badge.height());
    }

    void PinnedServiceItem::drawUnreads(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const
    {
        if (!needDrawUnreads(_state) || !_painter)
            return;

        _unreadsX -= Logic::IconsManager::getUnreadBalloonWidth(unreadCount_);
        const auto& contactListParams = Ui::GetContactListParams();

        Utils::drawUnreads((*_painter), contactListParams.unreadsFont(),
                           getUnreadsCountersBackgroundColorForServiceItem(_isSelected),
                           getUnreadsCounterForegroundColorServiceItem(_isSelected), unreadCount_, Logic::IconsManager::getUnreadBubbleHeight(),
                           _rect.left() + _unreadsX, _unreadsY);

    }

    void PinnedServiceItem::drawMentions(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const
    {
        if (!needDrawMentions(_viewParams, _state) || !_painter)
            return;

        if (needDrawUnreads(_state))
            _unreadsX -= Logic::IconsManager::getUnreadLeftPadding();
        _unreadsX -= Logic::IconsManager::getMentionSize().width();

        _painter->drawPixmap(_rect.left() + _unreadsX, _unreadsY, getMentionIconServiceItem(_isSelected).actualPixmap());
    }

    void PinnedServiceItem::draw(QPainter& _p, const QRect& _rect, const Ui::ViewParams& _viewParams, const bool _isSelected, const bool _isHovered, const bool _isDrag, const bool _isKeyboardFocused) const
    {
        Utils::PainterSaver ps(_p);
        _p.setClipRect(_rect);

        _p.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

        _p.setPen(Qt::NoPen);

        drawMouseState(_p, _rect, _isHovered, _isSelected, _isKeyboardFocused);

        _p.translate(_rect.topLeft());

        const auto& contactListParams = Ui::GetContactListParams();

        const auto circleSize = contactListParams.getAvatarSize();
        _p.setBrush(pinnedServiceItemIconCircleColor(_isSelected));
        auto iconLeftOffset = contactListParams.itemHorPadding();
        auto iconTopOffset = contactListParams.getAvatarY();
        auto iconPadding = (contactListParams.getAvatarSize() - Logic::IconsManager::pinnedServiceItemIconSize().width()) / 2;

        _p.drawEllipse(iconLeftOffset, iconTopOffset, circleSize, circleSize);
        _p.drawPixmap(iconLeftOffset + iconPadding, iconTopOffset + iconPadding, Logic::IconsManager::pinnedServiceItemPixmap(type_, _isSelected).cachedPixmap());

        text_->setColor(getContactNameColor(_isSelected));
        text_->draw(_p, Ui::TextRendering::VerPosition::MIDDLE);

        const auto unreadBalloonWidth = unreadCount_ > 0 ? Utils::getUnreadsBadgeSize(unreadCount_,
                                                                                      Logic::IconsManager::getUnreadBubbleHeight()).width()
                                                         : 0;
        auto unreadsX = _rect.width();
        unreadsX -= Logic::IconsManager::getUnreadBubbleRightMargin(true);

        auto unreadsY = (contactListParams.itemHeight() - getUnreadsSize()) / 2;
        const auto &state = Logic::getRecentsModel()->getDlgState(aimid_);

        drawUnreads(&_p, state, _isSelected, _rect, _viewParams, unreadsX, unreadsY);
        drawMentions(&_p, state, _isSelected, _rect, _viewParams, unreadsX, unreadsY);
        drawDraft(&_p, state, _isSelected, _rect, _viewParams, unreadsX);

        if (_isDrag)
            renderDragOverlay(_p, _rect, _viewParams);
    }

    bool PinnedServiceItem::needsTooltip(QPoint _posCursor) const
    {
        Ui::ViewParams viewParams;
        viewParams.pictOnly_ = false;
        const auto& state = Logic::getRecentsModel()->getDlgState(aimid_);
        return needDrawDraft(viewParams, state) && isInDraftIconRect(_posCursor);
    }

    //////////////////////////////////////////////////////////////////////////
    // RecentItemService
    //////////////////////////////////////////////////////////////////////////
    RecentItemService::RecentItemService(const Data::DlgState& _state)
        : RecentItemBase(_state)
    {
        text_ = TextRendering::MakeTextUnit(_state.GetText());
        // FONT
        text_->init({ Fonts::appFontScaled(11, Fonts::FontWeight::SemiBold), getServiceItemColor() });
        text_->evaluateDesiredSize();

        if (_state.AimId_ == u"~recents~")
            type_ = ServiceItemType::recents;
        else if (_state.AimId_ == u"~pinned~")
            type_ = ServiceItemType::pinned;
        else if (_state.AimId_ == u"~unimportant~")
            type_ = ServiceItemType::unimportant;
        else if (_state.AimId_ == u"~unknowns~")
            type_ = ServiceItemType::unknowns;

        badgeTextUnit_ = TextRendering::MakeTextUnit(QString());
        TextRendering::TextUnit::InitializeParameters params{ Fonts::appFontScaled(11, platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE } };
        params.align_ = TextRendering::HorAligment::CENTER;
        badgeTextUnit_->init(params);
        badgeTextUnit_->evaluateDesiredSize();
    }

    RecentItemService::RecentItemService(const QString& _text)
        : RecentItemBase(Data::DlgState())
    {
        text_ = TextRendering::MakeTextUnit(_text);
        // FONT
        text_->init({ Fonts::appFontScaled(11, Fonts::FontWeight::SemiBold), getServiceItemColor() });
        text_->evaluateDesiredSize();
    }

    RecentItemService::~RecentItemService() = default;

    void RecentItemService::draw(QPainter& _p, const QRect& _rect, const Ui::ViewParams& _viewParams, const bool _isSelected, const bool _isHovered, const bool _isDrag, const bool _isKeyboardFocused) const
    {
        auto recentParams = GetRecentsParams(_viewParams.regim_);
        const auto ratio = Utils::scale_bitmap_ratio();
        const auto height = recentParams.serviceItemHeight();

        if (!text_)
        {
            im_assert(false);
            return;
        }

        Utils::PainterSaver ps(_p);
        _p.translate(_rect.topLeft());

        if (!_viewParams.pictOnly_)
        {
            text_->setOffsets(getServiceItemHorPadding(), height / 2);
            text_->draw(_p, Ui::TextRendering::VerPosition::MIDDLE);
        }
        else
        {
            QPen line_pen;
            line_pen.setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY));
            _p.setPen(line_pen);

            static Utils::StyledPixmap pixPinned = getPin(false, getPinHeaderSize());
            static Utils::StyledPixmap pixUnimportant = Utils::StyledPixmap(qsl(":/unimportant_icon"), getPinHeaderSize(), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_TERTIARY });
            static Utils::StyledPixmap pixRecents = Utils::StyledPixmap(qsl(":/resources/icon_recents.svg"), getPinHeaderSize());

            QPixmap p = pixRecents.actualPixmap();
            if (type_ == ServiceItemType::pinned)
                p = pixPinned.actualPixmap();
            else if (type_ == ServiceItemType::unimportant)
                p = pixUnimportant.actualPixmap();

            int y = height / 2;
            int xp = ItemWidth(_viewParams) / 2 - (p.width() / 2. / ratio);
            int yp = height / 2 - (p.height() / 2. / ratio);
            _p.drawLine(0, y, xp - recentParams.serviceItemIconPadding(), y);
            _p.drawLine(xp + p.width() / ratio + recentParams.serviceItemIconPadding(), y, ItemWidth(_viewParams), y);
            _p.drawPixmap(xp, yp, p);
        }

        if ((type_ == ServiceItemType::pinned || type_ == ServiceItemType::unimportant) && !_viewParams.pictOnly_)
        {
            bool visible = false;
            if (type_ == ServiceItemType::pinned)
                visible = Logic::getRecentsModel()->isPinnedVisible();
            else
                visible = Logic::getRecentsModel()->isUnimportantVisible();

            const auto& p = getArrowIcon(visible);
            QPen line_pen;
            line_pen.setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY));
            _p.setPen(line_pen);

            int x = text_->cachedSize().width() + getServiceItemHorPadding() + recentParams.pinnedStatusPadding();
            int y = height / 2 - (p.height() / 2. / ratio);

            _p.drawPixmap(x, y, p);

            if (!visible && badgeTextUnit_)
            {
                x = _rect.width() - getUnknownHorPadding() - getUnreadsSize() / 2 + get_badge_size() / 2;
                auto offset = 0;
                auto unreads = type_ == ServiceItemType::pinned ? Logic::getRecentsModel()->pinnedUnreads() : Logic::getRecentsModel()->unimportantUnreads();

                const auto hasMentionsInPinned = Logic::getRecentsModel()->hasMentionsInPinned();
                const auto hasUnimportantMentions = Logic::getRecentsModel()->hasMentionsInUnimportant();
                const auto mutedPinnedMentions = Logic::getRecentsModel()->getMutedPinnedWithMentions();
                const auto mutedUnimportantMentions = Logic::getRecentsModel()->getMutedPinnedWithMentions();

                if (unreads != 0)
                {
                    if (type_ == ServiceItemType::pinned)
                        unreads += mutedPinnedMentions;
                    else
                        unreads += mutedUnimportantMentions;

                    badgeTextUnit_->setText(Utils::getUnreadsBadgeStr(unreads));
                    offset += Utils::Badge::drawBadgeRight(badgeTextUnit_, _p, x, y + p.height() / 2. / ratio - get_badge_size() / 2, Utils::Badge::Color::Green);
                    offset += recentParams.pinnedStatusPadding();
                }
                else if (((type_ == ServiceItemType::pinned && Logic::getRecentsModel()->hasAttentionPinned()) || Logic::getRecentsModel()->hasAttentionUnimportant()) &&
                         (!hasMentionsInPinned && !hasUnimportantMentions))
                {
                    static auto icon = Utils::LayeredPixmap(qsl(":/tab/attention"),
                        {
                            { qsl("border"), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE } },
                            { qsl("bg"), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY } },
                            { qsl("star"), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE } },
                        },
                        { get_badge_size(), get_badge_size() });
                    _p.drawPixmap(x - get_badge_size(), y + p.height() / 2. / ratio - get_badge_size() / 2, icon.actualPixmap());
                }

                if ((type_ == ServiceItemType::pinned && hasMentionsInPinned) || hasUnimportantMentions)
                {
                    if (unreads == 0)
                    {
                        badgeTextUnit_->setText(Utils::getUnreadsBadgeStr(type_ == ServiceItemType::pinned ? mutedPinnedMentions : mutedUnimportantMentions));
                        offset += Utils::Badge::drawBadgeRight(badgeTextUnit_, _p, x, y + p.height() / 2. / ratio - get_badge_size() / 2, Utils::Badge::Color::Gray);
                        offset += recentParams.pinnedStatusPadding();
                    }

                    offset += get_badge_size();
                    static auto icon = Logic::IconsManager::getMentionIcon(false, { get_badge_mention_size(), get_badge_mention_size() });
                    _p.drawPixmap(x - offset, y + p.height() / 2. / ratio - get_badge_size() / 2 + Utils::scale_value(1), icon.actualPixmap());
                }
            }
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // RecentItemUnknowns
    //////////////////////////////////////////////////////////////////////////
    RecentItemUnknowns::RecentItemUnknowns(const Data::DlgState& _state)
        : RecentItemBase(_state)
        , compactMode_(!Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, !config::get().is_on(config::features::compact_mode_by_default)))
        , count_(Logic::getUnknownsModel()->totalUnreads())
    {
        text_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("contact_list", "New contacts"));
        // FONT
        text_->init({ Fonts::appFontScaled(getUnknownTextSize()), getUnknownTextColor() });
        text_->evaluateDesiredSize();

        if (count_)
        {
            const auto& contactListParams = compactMode_ ? Ui::GetContactListParams() : Ui::GetRecentsParams();

            static QFontMetrics m(contactListParams.unreadsFont());

            const auto text = Utils::getUnreadsBadgeStr(count_);
            const auto unreadsRect = m.tightBoundingRect(text);
            const auto firstChar = text[0];
            const auto lastChar = text[text.size() - 1];
            const auto unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));

            unreadBalloonWidth_ = unreadsWidth;
            if (text.length() > 1)
            {
                unreadBalloonWidth_ += (contactListParams.unreadsPadding() * 2);
            }
            else
            {
                unreadBalloonWidth_ = getUnreadsSize();
            }
        }
    }

    RecentItemUnknowns::~RecentItemUnknowns() = default;

    void RecentItemUnknowns::draw(QPainter& _p, const QRect& _rect, const Ui::ViewParams& _viewParams, const bool _isSelected, const bool _isHovered, const bool _isDrag, const bool _isKeyboardFocused) const
    {
        const auto width = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);

        Utils::PainterSaver ps(_p);

        _p.setPen(Qt::NoPen);

        drawMouseState(_p, _rect, _isHovered, _isSelected);

        auto& contactListParams = compactMode_ ? Ui::GetContactListParams() : Ui::GetRecentsParams();

        static QPixmap pict = getContactsIcon();

        int offset_x = _viewParams.pictOnly_ ? (width - Utils::scale_value(getContactsIconSize().width())) / 2 : getUnknownHorPadding();

        _p.drawPixmap(_rect.left() + offset_x, _rect.top() + (_rect.height() - Utils::scale_value(getContactsIconSize().height())) / 2, pict);

        offset_x += Utils::unscale_bitmap(pict.width()) + getUnknownHorPadding();

        if (!_viewParams.pictOnly_)
        {
            text_->setOffsets(_rect.left() + offset_x, _rect.top() + contactListParams.unknownsItemHeight() / 2);
            text_->draw(_p, ::Ui::TextRendering::VerPosition::MIDDLE);
        }

        if (count_)
        {
            _p.setPen(Qt::NoPen);
            _p.setRenderHint(QPainter::Antialiasing);
            _p.setBrush(Qt::NoBrush);

            auto unreadsX = width - getUnknownHorPadding() - unreadBalloonWidth_;
            if (_viewParams.pictOnly_)
                unreadsX = getUnknownHorPadding();

            auto unreadsY = (contactListParams.unknownsItemHeight() - getUnreadsSize()) / 2;
            if (_viewParams.pictOnly_)
                unreadsY = Utils::scale_value(4);

            auto borderColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
            if (_isHovered)
                borderColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
            else if (_isSelected)
                borderColor = (_viewParams.pictOnly_) ? Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY) : QColor();

            const auto bgColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
            const auto textColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);

            Utils::drawUnreads(&_p, contactListParams.unreadsFont(), bgColor, textColor, borderColor, count_, getUnreadsSize(), _rect.left() + unreadsX, _rect.top() + unreadsY);
        }
    }

    LastStatus getLastStatus(const Data::DlgState& _state)
    {
        LastStatus st = LastStatus::None;

        if (_state.UnreadCount_ || _state.Attention_)
            return st;

        if (!_state.IsLastMessageDelivered_)
            st = LastStatus::Pending;
        else if (_state.LastMsgId_ <= _state.TheirsLastDelivered_)
            st = LastStatus::DeliveredToPeer;
        else
            st = LastStatus::DeliveredToServer;

        return st;
    }

    QPixmap getStatePixmap(const LastStatus& _lastStatus, const bool _selected, const bool _multichat)
    {
        const auto normalClr = Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY);
        const auto selClr = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);

        switch (_lastStatus)
        {
            case LastStatus::Pending:
                return LastStatusAnimation::getPendingPixmap(_selected ? selClr : normalClr);
            case LastStatus::DeliveredToServer:
                if (_multichat)
                    return LastStatusAnimation::getDeliveredToClientPixmap(_selected ? selClr : normalClr);
                return LastStatusAnimation::getDeliveredToServerPixmap(_selected ? selClr : normalClr);
            case LastStatus::DeliveredToPeer:
                if (_multichat)
                    return LastStatusAnimation::getDeliveredToServerPixmap(_selected ? selClr : normalClr);
                return LastStatusAnimation::getDeliveredToClientPixmap(_selected ? selClr : normalClr);

            case LastStatus::None:
            case LastStatus::Read:
                break;
        }

        return QPixmap();
    }

    QString getSender(const Data::DlgState& _state, bool _outgoing = false)
    {
        if (_state.lastMessage_ && _state.lastMessage_->GetChatEvent()) // do not show sender for chat event
            return QString();

        if (_outgoing)
            return QT_TRANSLATE_NOOP("contact_list", "Me");

        QString sender = _state.senderNick_;

        if (!_state.senderAimId_.isEmpty())
        {
            const auto displayName = Logic::GetFriendlyContainer()->getFriendly(_state.senderAimId_);
            if (!displayName.isEmpty() && displayName != _state.senderAimId_)
                sender = displayName;
        }

        sender = Utils::replaceLine(sender);

        const auto tokens = sender.split(ql1c(' '));

        if (!tokens.isEmpty())
            sender = tokens.at(0);

        return sender;
    }

    //////////////////////////////////////////////////////////////////////////
    // RecentItemRecent
    //////////////////////////////////////////////////////////////////////////
    RecentItemRecent::RecentItemRecent(
        const Data::DlgState& _state,
        bool _compactMode,
        bool _hideSender,
        bool _hideMessage,
        bool _showHeads,
        AlertType _alertType,
        bool _isNotification
    )
        : RecentItemBase(_state)
        , multichat_(Logic::getContactListModel()->isChat(_state.AimId_))
        , displayName_(Utils::replaceLine(Logic::getFriendlyContainerProxy(Logic::FriendlyContainerProxy::ReplaceFavorites).getFriendly(_state.AimId_)))
        , messageNameWidth_(-1)
        , compactMode_(_compactMode)
        , unreadCount_(_state.UnreadCount_)
        , attention_(_state.Attention_)
        , mention_(_state.unreadMentionsCount_ > 0)
        , prevWidthName_(-1)
        , prevWidthMessage_(-1)
        , muted_(Logic::getContactListModel()->isMuted(_state.AimId_))
        , online_(!Favorites::isFavorites(_state.AimId_) && Logic::GetLastseenContainer()->isOnline(_state.AimId_))
        , pin_(_state.PinnedTime_ != -1)
        , typersCount_(_state.typings_.size())
        , typer_(_state.getTypers())
        , mediaType_((_hideMessage || _state.hasDraft()) ? MediaType::noMedia : _state.mediaType_)
        , readOnlyChat_(multichat_ && (_state.AimId_ == _state.senderAimId_))
    {
        official_ = false;

        displayName_ = formatNotificationTitle(_state, _alertType, !_hideSender);
        if (!_hideSender)
        {
            const auto friendly = Logic::getFriendlyContainerProxy(Logic::FriendlyContainerProxy::ReplaceFavorites).getFriendly2(_state.AimId_);
            official_ = friendly.official_;
        }

        const auto& contactListParams = compactMode_ ? Ui::GetContactListParams() : Ui::GetRecentsParams();
        name_ = TextRendering::MakeTextUnit(displayName_, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        TextRendering::TextUnit::InitializeParameters params{ Fonts::appFont(contactListParams.contactNameFontSize(), contactListParams.contactNameFontWeight()), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } };
        params.maxLinesCount_ = 1;
        name_->init(params);
        name_->evaluateDesiredSize();
        nameWidth_ = name_->desiredWidth();

        //////////////////////////////////////////////////////////////////////////
        // init message
        draft_ = _state.hasDraft();
        if (!compactMode_)
        {
            const auto nameColor = getNameColor(false, draft_ && !typersCount_);
            const int fontSize = contactListParams.messageFontSize();
            const auto font = Fonts::appFontScaled(fontSize, Fonts::FontWeight::Normal);

            auto createUnit = [&nameColor]
            (const QString& _text, const QFont& _font, TextRendering::ProcessLineFeeds _processFeeds, int _lineCount = 1)
            {
                auto name = TextRendering::MakeTextUnit(_text, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, _processFeeds);
                TextRendering::TextUnit::InitializeParameters params{ _font, nameColor };
                params.maxLinesCount_ = _lineCount;
                params.lineBreak_ = TextRendering::LineBreakType::PREFER_SPACES;
                name->init(params);
                return name;
            };

            if (typersCount_)
            {
                static auto typingTextSingle = QT_TRANSLATE_NOOP("contact_list", "is typing...");
                static auto typingTextMulty = QT_TRANSLATE_NOOP("contact_list", "are typing...");
                const auto& typingText = ((typersCount_ > 1) ? typingTextMulty : typingTextSingle);

                const QString nameText = multichat_ ? typer_ % QChar::LineFeed : QString();

                messageShortName_ = createUnit(nameText, Fonts::appFontScaled(fontSize, getNameFontWeight(false)), TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS, 2);
                messageShortName_->evaluateDesiredSize();
                messageShortName_->setLineSpacing(getMessageLineSpacing());
                messageNameWidth_ = messageShortName_->desiredWidth();

                auto secondPath1 = createUnit((multichat_ ? TextRendering::spaceAsString() : QString()) + typingText, font, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
                messageShortName_->append(std::move(secondPath1));
                messageShortName_->evaluateDesiredSize();

                // control for long contact name
                messageLongName_ = createUnit(nameText, font, TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS);
                messageLongName_->setLineSpacing(getMessageLineSpacing());

                auto secondPath2 = createUnit(typingText, font, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
                messageLongName_->appendBlocks(std::move(secondPath2));
                messageLongName_->evaluateDesiredSize();
            }
            else
            {
                QString messageText;
                if (_isNotification)
                    messageText = formatNotificationText(_state, _alertType, !_hideSender, !_hideMessage, draft_);
                else
                    messageText = _hideMessage ?
                        QT_TRANSLATE_NOOP("notifications", "New message") :
                        Utils::replaceLine(draft_ ? _state.draftText_ : _state.GetText());

                if (!draft_ && (!multichat_ || readOnlyChat_))
                {
                    messageShortName_ = createUnit(messageText, font, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS, 2);
                    messageShortName_->setLineSpacing(getMessageLineSpacing());
                }
                else
                {
                    // control for short contact name
                    QString sender;
                    if (draft_)
                        sender = QT_TRANSLATE_NOOP("contact_list", "Draft");
                    else
                        sender = _isNotification ? formatNotificationSubtitle(_state, _alertType, !_hideSender) : getSender(_state, _state.Outgoing_);

                    if (!sender.isEmpty() && !_isNotification)
                        sender += u": ";

                    if (mediaType_ == MediaType::noMedia)
                        sender += QChar::LineFeed;

                    messageShortName_ = createUnit(sender, Fonts::appFontScaled(fontSize, getNameFontWeight(draft_)), TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS, 2);
                    messageShortName_->evaluateDesiredSize();

                    if (_isNotification)
                    {
                        messageNameWidth_ = messageShortName_->desiredWidth();

                        messageLongName_ = createUnit(messageText, font, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS, 1);
                    }
                    else if (mediaType_ == MediaType::noMedia)
                    {
                        messageShortName_->setLineSpacing(getMessageLineSpacing());
                        messageNameWidth_ = messageShortName_->desiredWidth();

                        messageShortName_->append(createUnit(messageText, font, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS));
                        messageShortName_->evaluateDesiredSize();

                        // control for long contact name
                        messageLongName_ = createUnit(sender, font, TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS);
                        messageLongName_->setLineSpacing(getMessageLineSpacing());

                        messageLongName_->appendBlocks(createUnit(messageText, font, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS));
                    }
                    else
                    {
                        messageNameWidth_ = messageShortName_->desiredWidth();

                        messageLongName_ = createUnit(messageText, font, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS, 1);
                    }

                    messageLongName_->evaluateDesiredSize();
                }
            }
        }

        const auto timeStr = _state.Time_ > 0 ? ::Ui::FormatTime(QDateTime::fromSecsSinceEpoch(_state.Time_)) : QString();
        time_ = TextRendering::MakeTextUnit(timeStr, {}, Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);
        constexpr auto timeFontSize = platform::is_apple() ? 12 : 11;
        time_->init({ Fonts::appFontScaledFixed(timeFontSize, Fonts::FontWeight::Normal), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_TERTIARY } });
        time_->evaluateDesiredSize();

        //////////////////////////////////////////////////////////////////////////
        // init unread
        if (unreadCount_)
        {
            unreadBalloonWidth_ = Utils::getUnreadsBadgeSize(unreadCount_, Logic::IconsManager::getUnreadBubbleHeight()).width();
        }

        //////////////////////////////////////////////////////////////////////////
        // init last read avatar
        drawLastReadAvatar_ = false;
        lastStatus_ = LastStatus::None;

        if (!unreadCount_)
        {
            const bool isLastRead = !_state.Attention_ && (_state.LastMsgId_ >= 0 && _state.TheirsLastRead_ > 0 && _state.LastMsgId_ <= _state.TheirsLastRead_);

            if (!multichat_)
            {
                if (_state.Outgoing_)
                {
                    if (isLastRead && (_state.lastMessage_ && _state.lastMessage_->HasId()) && !Favorites::isFavorites(aimid_))
                        drawLastReadAvatar_ = true;
                    else
                        lastStatus_ = getLastStatus(_state);
                }
            }
            else if (!_state.Attention_)
            {
                if (_showHeads && !_state.heads_.empty() && (_state.lastMessage_ && _state.lastMessage_->HasId()))
                {
                    for (const auto& _head : _state.heads_)
                    {
                        heads_.push_back(_head);
                        if (heads_.size() >= 3)
                            break;
                    }
                }
                else if (_state.Outgoing_)
                {
                    lastStatus_ = getLastStatus(_state);
                }
            }
        }
    }

    RecentItemRecent::~RecentItemRecent() = default;

    bool RecentItemRecent::needsTooltip(QPoint _posCursor) const
    {
        return (draft_ && isInDraftIconRect(_posCursor)) || (name_->isElided() && (!_posCursor.isNull() ? name_->contains(_posCursor) : true));
    }

    void RecentItemRecent::fixMaxWidthIfIcons(const Ui::ViewParams& _viewParams, int& _maxWidth) const
    {
        Data::DlgState state = Logic::getRecentsModel()->getDlgState(getAimid());
        if (!needDrawDraft(_viewParams, state))
            return;
        auto contactListParams = compactMode_ ? Ui::GetContactListParams() : Ui::GetRecentsParams();
        _maxWidth -= (Logic::IconsManager::getDraftSize().width() + contactListParams.official_hor_padding());
    }

    bool RecentItemRecent::isUnknown(const Ui::ViewParams& _viewParams) const
    {
        return _viewParams.regim_ == ::Logic::MembersWidgetRegim::UNKNOWN;
    }

    bool RecentItemRecent::isPictOnly(const Ui::ViewParams& _viewParams) const
    {
        return _viewParams.pictOnly_;
    }

    bool RecentItemRecent::isUnknownAndNotPictOnly(const Ui::ViewParams& _viewParams) const
    {
        return (isUnknown(_viewParams) && !isPictOnly(_viewParams));
    }

    bool RecentItemRecent::needDrawUnreads(const Data::DlgState& _state) const
    {
        return unreadCount_;
    }

    bool RecentItemRecent::needDrawMentions(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const
    {
        Q_UNUSED(_state);
        return (mention_ && !isUnknown(_viewParams));
    }

    bool RecentItemRecent::needDrawAttention(const Data::DlgState& _state) const
    {
        Q_UNUSED(_state);
        return (!unreadCount_ && attention_);
    }

    bool RecentItemRecent::needDrawDraft(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const
    {
        return (_state.hasDraft() && !isPictOnly(_viewParams) && compactMode_);
    }

    void RecentItemRecent::drawDraft(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _rightAreaRight) const
    {
        if (!needDrawDraft(_viewParams, _state) || !_painter)
            return;

        auto contactListParams = compactMode_ ? Ui::GetContactListParams() : Ui::GetRecentsParams();

        using im = Logic::IconsManager;

        const QPixmap badge = im::getDraftIcon(_isSelected).actualPixmap();
        _rightAreaRight -= Utils::unscale_bitmap(badge.width());

        if (needDrawUnreads(_state))
        {
            _rightAreaRight -= im::getUnreadBubbleRightMarginPictOnly();
            _rightAreaRight -= unreadBalloonWidth_;
        }

        if (needDrawAttention(_state))
        {
            _rightAreaRight -= im::getUnreadBubbleRightMarginPictOnly();
            _rightAreaRight -= im::getAttentionSize().width();
        }

        if (needDrawMentions(_viewParams, _state))
        {
            _rightAreaRight -= im::getUnreadBubbleRightMarginPictOnly();
            _rightAreaRight -= im::getMentionSize().width();
        }
        const auto draftY = _rect.top() + (contactListParams.itemHeight() - Logic::IconsManager::getDraftSize().height())/2;
        _painter->drawPixmap(_rightAreaRight, draftY, badge);

        draftIconRect_.setRect(_rightAreaRight, draftY, badge.width(), badge.height());

        _rightAreaRight -= contactListParams.official_hor_padding();

    }

    void RecentItemRecent::drawUnreads(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const
    {
        if (!needDrawUnreads(_state) || !_painter)
            return;

        auto contactListParams = compactMode_ ? Ui::GetContactListParams() : Ui::GetRecentsParams();

        using im = Logic::IconsManager;
        _unreadsX -= isPictOnly(_viewParams) ? im::getUnreadBubbleRightMarginPictOnly() : im::getUnreadBubbleRightMargin(compactMode_);
        _unreadsX -= unreadBalloonWidth_;

        using st = Styling::StyleVariable;
        std::function getColor = [_isSelected](st selectedColor, st color){ return Styling::getParameters().getColor(_isSelected ? selectedColor : color); };

        const auto bgColor = getColor(st::TEXT_SOLID_PERMANENT, muted_ ? st::BASE_TERTIARY : st::PRIMARY);
        const auto textColor = getColor(st::PRIMARY_SELECTED, st::BASE_GLOBALWHITE);

        auto x = isUnknownAndNotPictOnly(_viewParams) ? getUnknownUnreads(_viewParams, _rect) : _rect.left() + _unreadsX;
        auto y = isUnknownAndNotPictOnly(_viewParams) ? _rect.top() + (contactListParams.itemHeight() - im::getUnreadBubbleHeight()) / 2 : _rect.top() + _unreadsY;

        Utils::drawUnreads(*(_painter), contactListParams.unreadsFont(), bgColor, textColor, unreadCount_, im::getUnreadBubbleHeight(), x, y);
    }

    void RecentItemRecent::drawMentions(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const
    {
        if (!needDrawMentions(_viewParams, _state) || !_painter)
            return;

        using im = Logic::IconsManager;
        if (unreadCount_)
            _unreadsX -= isPictOnly(_viewParams) ? im::getUnreadLeftPaddingPictOnly() : im::getUnreadLeftPadding();
        else
            _unreadsX -= isPictOnly(_viewParams) ? im::getUnreadBubbleRightMarginPictOnly() : im::getUnreadBubbleRightMargin(compactMode_);

        _unreadsX -= im::getMentionSize().width();

        _painter->drawPixmap(_rect.left() + _unreadsX, _rect.top() + _unreadsY, im::getMentionIcon(_isSelected).cachedPixmap());
    }

    void RecentItemRecent::drawAttention(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const
    {
        if (!needDrawAttention(_state) || !_painter)
            return;

        auto contactListParams = compactMode_ ? Ui::GetContactListParams() : Ui::GetRecentsParams();

        using im = Logic::IconsManager;
        _unreadsX -= isPictOnly(_viewParams) ? im::getUnreadBubbleRightMarginPictOnly() : im::getUnreadBubbleRightMargin(compactMode_);
        _unreadsX -= im::getAttentionSize().width();

        auto x = isUnknownAndNotPictOnly(_viewParams) ? getUnknownUnreads(_viewParams, _rect) : _rect.left() + _unreadsX;
        auto y = isUnknownAndNotPictOnly(_viewParams) ? _rect.top() + (contactListParams.itemHeight() - im::getUnreadBubbleHeight()) / 2 : _rect.top() + _unreadsY;

        _painter->drawPixmap(x, y, im::getAttentionIcon(_isSelected).cachedPixmap());
    }

    int  RecentItemRecent::getUnknownUnreads(const Ui::ViewParams& _viewParams, const QRect& _rect) const
    {
        auto unknownUnreads = 0;
        if (!isUnknownAndNotPictOnly(_viewParams))
            return 0;
        const int addX = (_rect.width() - Logic::IconsManager::buttonWidth());
        const int x = _rect.x() + addX;
        unknownUnreads = x - Logic::IconsManager::getUnknownRightOffset();
        return unknownUnreads;
    }

    void RecentItemRecent::drawRemoveIcon(QPainter& _p, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams) const
    {
        if (!isUnknownAndNotPictOnly(_viewParams))
            return;
        const auto ratio = Utils::scale_bitmap_ratio();
        const auto remove_img = getRemove(_isSelected);

        auto contactListParams = compactMode_ ? Ui::GetContactListParams() : Ui::GetRecentsParams();

        const int addX = (_rect.width() - Logic::IconsManager::buttonWidth());
        const int x = _rect.x() + addX;
        const int addY = (contactListParams.itemHeight() - remove_img.height() / ratio) / 2.;
        const int y = _rect.top() + addY;

        contactListParams.removeContactFrame().setX(addX);
        contactListParams.removeContactFrame().setY(addY);
        contactListParams.removeContactFrame().setWidth(remove_img.width());
        contactListParams.removeContactFrame().setHeight(remove_img.height());

        _p.drawPixmap(x, y, remove_img);
    }

    void RecentItemRecent::draw(
        QPainter& _p,
        const QRect& _rect,
        const Ui::ViewParams& _viewParams,
        const bool _isSelected,
        const bool _isHovered,
        const bool _isDrag,
        const bool _isKeyboardFocused) const
    {
        Utils::PainterSaver ps(_p);
        _p.setClipRect(_rect);

        _p.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

        _p.setPen(Qt::NoPen);

        drawMouseState(_p, _rect, _isHovered, _isSelected, _isKeyboardFocused);

        const auto width = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);

        bool isDefaultAvatar = false;

        auto contactListParams = compactMode_ ? Ui::GetContactListParams() : Ui::GetRecentsParams();

        //////////////////////////////////////////////////////////////////////////
        // render avatar
        auto avatar = Logic::getAvatarStorageProxy(Logic::AvatarStorageProxy::ReplaceFavorites).GetRounded(
                    getAimid(), displayName_, Utils::scale_bitmap(contactListParams.getAvatarSize()), isDefaultAvatar, false, compactMode_);

        const auto ratio = Utils::scale_bitmap_ratio();
        const auto avatarX = _rect.left() + (_viewParams.pictOnly_ ? (_rect.width() - avatar.width() / ratio) / 2 : contactListParams.getAvatarX());
        const auto avatarY = _rect.top() + (_rect.height() - avatar.height() / ratio) / 2;

        const auto isFavorites = Favorites::isFavorites(getAimid());

        const auto statusBadge = isFavorites ? QPixmap() : Utils::getStatusBadge(getAimid(), avatar.width());

        Utils::StatusBadgeFlags statusFlags { Utils::StatusBadgeFlag::Small };
        statusFlags.setFlag(Utils::StatusBadgeFlag::Official, official_);
        statusFlags.setFlag(Utils::StatusBadgeFlag::Muted, muted_);
        statusFlags.setFlag(Utils::StatusBadgeFlag::Selected, _isSelected);
        statusFlags.setFlag(Utils::StatusBadgeFlag::Online, online_);
        statusFlags.setFlag(Utils::StatusBadgeFlag::SmallOnline, compactMode_);
        Utils::drawAvatarWithBadge(_p, QPoint(avatarX, avatarY), avatar, statusBadge, statusFlags);

        const auto isUnknown = (_viewParams.regim_ == ::Logic::MembersWidgetRegim::UNKNOWN);
        drawRemoveIcon(_p, _isSelected, _rect, _viewParams);

        auto unreadsY = (contactListParams.itemHeight() - getUnreadsSize()) / 2;

        if (_viewParams.pictOnly_)
            unreadsY = getUnreadBubbleMarginPictOnly();
        else if (!compactMode_)
            unreadsY = getUnreadY();

        auto unreadsX = width;

        //////////////////////////////////////////////////////////////////////////
        // draw unreads
        Data::DlgState state = Logic::getRecentsModel()->getDlgState(getAimid());
        drawUnreads(&_p, state, _isSelected, _rect, _viewParams, unreadsX, unreadsY);
        drawAttention(&_p, state, _isSelected, _rect, _viewParams, unreadsX, unreadsY);
        drawMentions(&_p, state, _isSelected, _rect, _viewParams, unreadsX, unreadsY);

        //////////////////////////////////////////////////////////////////////////
        // draw status
        using im = Logic::IconsManager;
        const int avatarRightMargin = (im::getRigthLineCenter(compactMode_) - (contactListParams.getLastReadAvatarSize()) / 2);

        if (!_viewParams.pictOnly_ && !isUnknown && !draft_)
        {
            if (drawLastReadAvatar_)
            {
                bool isDefault = false;
                auto lastReadAvatar = Logic::GetAvatarStorage()->GetRounded(aimid_, displayName_,
                    Utils::scale_bitmap(contactListParams.getLastReadAvatarSize()), isDefault, false, false);

                const auto opacityOld = _p.opacity();

                if (!_isSelected)
                    _p.setOpacity(0.7);

                unreadsX -= avatarRightMargin;
                unreadsX -= Utils::unscale_bitmap(lastReadAvatar.width());

                const int y_offset = (im::getUnreadBubbleHeight() - Utils::unscale_bitmap(lastReadAvatar.height())) / 2;

                _p.drawPixmap(_rect.left() + unreadsX, _rect.top() + unreadsY + y_offset, lastReadAvatar);

                _p.setOpacity(opacityOld);
            }
            else if (lastStatus_ != LastStatus::None)
            {
                if (const auto& pix = getStatePixmap(lastStatus_, _isSelected, multichat_); !pix.isNull())
                {
                    const int rightMargin = im::getRigthLineCenter(compactMode_) - (Utils::unscale_bitmap(pix.width()) / 2);

                    unreadsX -= rightMargin;
                    unreadsX -= Utils::unscale_bitmap(pix.width());

                    const int y_offset = (im::getUnreadBubbleHeight() - Utils::unscale_bitmap(pix.height())) / 2;

                    _p.drawPixmap(_rect.left() + unreadsX, _rect.top() + unreadsY + y_offset, pix);
                }
            }
            else if (!heads_.empty())
            {
                _p.setRenderHint(QPainter::Antialiasing);

                unreadsX -= avatarRightMargin;

                for (const auto& _head : boost::adaptors::reverse(heads_))
                {
                    bool isDefault = false;
                    auto headAvatar = Logic::GetAvatarStorage()->GetRounded(_head.first, (_head.second.isEmpty() ? _head.first : _head.second),
                        Utils::scale_bitmap(contactListParams.getLastReadAvatarSize()), isDefault, false, false);

                    const auto opacityOld = _p.opacity();

                    _p.setPen(Qt::NoPen);
                    _p.setBrush(getHeadsBorderColor(_isSelected, _isHovered));

                    const int whiteAreaD = Utils::scale_value(16);

                    unreadsX -= whiteAreaD;

                    int y_offset = (im::getUnreadBubbleHeight() - Utils::unscale_bitmap(headAvatar.height())) / 2;

                    const int border = (whiteAreaD - Utils::unscale_bitmap(headAvatar.height())) / 2;
                    const int overlap = Utils::scale_value(4);

                    y_offset -= border;

                    _p.drawEllipse(_rect.left() + unreadsX, _rect.top() + unreadsY + y_offset, whiteAreaD, whiteAreaD);

                    if (!_isSelected)
                        _p.setOpacity(0.7);

                    _p.drawPixmap(_rect.left() + unreadsX + border, _rect.top() + unreadsY + y_offset + border, headAvatar);

                    _p.setOpacity(opacityOld);

                    unreadsX += overlap;
                }
            }
        }

        if ((width - unreadsX) < contactListParams.itemHorPaddingRight())
            unreadsX = width - contactListParams.itemHorPaddingRight();
        else
            unreadsX -= getMessageRightMargin();

        //////////////////////////////////////////////////////////////////////////
        // render contact name
        auto unknownUnreads = getUnknownUnreads(_viewParams, _rect);
        const int rightMarginName = isUnknown ? unknownUnreads : width - contactListParams.itemHorPaddingRight();
        const int rightMarginMessage = isUnknown ? unknownUnreads : unreadsX;

        int nameMaxWidth = (rightMarginName - contactListParams.messageX());
        int messageMaxWidth = (rightMarginMessage - contactListParams.messageX());

        const auto contactNameY = _rect.top() + getNameY(compactMode_);
        const int badge_y = contactNameY + Utils::scale_value(4);

        int right_area_right = width - contactListParams.itemHorPaddingRight();

        //////////////////////////////////////////////////////////////////////////
        // draw pin
        if (pin_ && !_viewParams.pictOnly_ && !compactMode_)
        {
            nameMaxWidth -= (getPinSize().width() + contactListParams.official_hor_padding());

            const QPixmap badge = getPin(_isSelected, getPinSize()).actualPixmap();

            right_area_right -= Utils::unscale_bitmap(badge.width());

            _p.drawPixmap(right_area_right, badge_y + Utils::scale_value(1), badge);

            right_area_right -= contactListParams.official_hor_padding();
        }

        //////////////////////////////////////////////////////////////////////////
        drawDraft(&_p, state, _isSelected, _rect, _viewParams, right_area_right);
        fixMaxWidthIfIcons(_viewParams, nameMaxWidth);
        //////////////////////////////////////////////////////////////////////////
        // draw time
        if (!_viewParams.pictOnly_ && !compactMode_ && !isUnknown && !draft_)
        {
            const int time_width = time_->cachedSize().width();

            time_->setColor(Styling::ThemeColorKey{ _isSelected ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::BASE_PRIMARY });
            time_->setOffsets(right_area_right - time_width, contactNameY + getTimeYOffsetFromName() + Utils::text_sy());
            time_->draw(_p);

            nameMaxWidth -= (time_width + contactListParams.official_hor_padding());
        }

        const auto contactNameX = _rect.left() + contactListParams.getContactNameX();

        if (!_viewParams.pictOnly_)
        {
            if (compactMode_)
            {
                nameMaxWidth -= (width - rightMarginMessage);
                nameMaxWidth += contactListParams.itemHorPaddingRight();
            }

            const bool widthNameChanged = (nameMaxWidth != prevWidthName_);

            const auto contactNameColor = getContactNameColor(_isSelected, isFavorites);

            name_->setColor(contactNameColor);
            name_->setOffsets(contactNameX, contactNameY + Utils::text_sy());

            int currentNameWidth = nameWidth_;
            if (nameWidth_ > nameMaxWidth)
                currentNameWidth = nameMaxWidth;

            if (widthNameChanged)
                name_->elide(nameMaxWidth);

            name_->draw(_p);
        }

        //////////////////////////////////////////////////////////////////////////
        // draw mediatype
        if (mediaType_ != MediaType::noMedia && !compactMode_ && !_viewParams.pictOnly_ && !typersCount_)
        {
            const QPixmap mediaPix = Logic::IconsManager::getMediaTypePix(mediaType_, _isSelected);

            const int pixY = _rect.top() + getMediaTypeIconTopMargin(multichat_ && !readOnlyChat_);

            _p.drawPixmap(contactNameX, pixY, mediaPix);
        }

        //////////////////////////////////////////////////////////////////////////
        // draw message
        if (!compactMode_ && !_viewParams.pictOnly_ && messageShortName_)
        {
            if (mediaType_ != MediaType::noMedia && multichat_ && messageLongName_ && !typersCount_)
            {
                int messageY = _rect.top() + getMessageY();
                int messageX = _rect.left() + contactListParams.messageX();

                messageShortName_->setOffsets(messageX, messageY + Utils::text_sy() + getMultiChatMessageOffset());
                messageShortName_->setColor(getNameColor(_isSelected, draft_ && !typersCount_));

                const int textOffset = Utils::scale_value(Logic::IconsManager::getMediaIconSize().width()) + Utils::scale_value(4);

                messageX += textOffset;
                messageY += get2LineMessageOffset() - getMultiChatMessageOffset();

                messageMaxWidth -= textOffset;
                const bool widthMessageChanged = (messageMaxWidth != prevWidthMessage_);
                if (widthMessageChanged)
                {
                    messageShortName_->elide(messageMaxWidth + textOffset);
                    messageLongName_->elide(messageMaxWidth);
                }

                messageLongName_->setOffsets(messageX, messageY + Utils::text_sy());
                messageLongName_->setColor(getMessageColor(_isSelected));

                messageShortName_->draw(_p);
                messageLongName_->draw(_p);
            }
            else
            {
                const auto& messageControl = (messageNameWidth_ > messageMaxWidth) ? messageLongName_ : messageShortName_;
                if (messageControl)
                {
                    const int messageY = _rect.top() + getMessageY();

                    int messageX = _rect.left() + contactListParams.messageX();

                    int textOffset = 0;

                    if (mediaType_ != MediaType::noMedia && !typersCount_)
                    {
                        textOffset = Utils::scale_value(Logic::IconsManager::getMediaIconSize().width()) + Utils::scale_value(4);
                    }

                    messageX += textOffset;

                    messageControl->setOffsets(messageX, messageY + Utils::text_sy());

                    messageMaxWidth -= textOffset;

                    const bool widthMessageChanged = (messageMaxWidth != prevWidthMessage_);

                    if (widthMessageChanged)
                    {
                        messageControl->getHeight(messageMaxWidth);
                    }

                    messageControl->setColor(getNameColor(_isSelected, draft_ && !typersCount_));
                    if (!_isSelected)
                        messageControl->setColorForAppended(getMessageColor(false));

                    messageControl->draw(_p);
                }
            }
        }

        prevWidthName_ = nameMaxWidth;
        prevWidthMessage_ = messageMaxWidth;

        if (_isDrag)
            renderDragOverlay(_p, _rect, _viewParams);
    }

    //////////////////////////////////////////////////////////////////////////
    // MessageAlertItem
    //////////////////////////////////////////////////////////////////////////
    MessageAlertItem::MessageAlertItem(const Data::DlgState& _state, const bool _compactMode, AlertType _alertType)
        : RecentItemRecent(
            _state,
            _compactMode,
            isMessageAlertSenderHidden(),
            isMessageAlertTextHidden(),
            false,
            _alertType,
            true)
    {
        mention_ = _state.mentionAlert_;

        const int scale = Utils::scale_bitmap(getAlertAvatarSize());
        const QSize size(scale, scale);
        cachedLogo_ = QImage(qsl(":/logo/logo")).scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    MessageAlertItem::~MessageAlertItem() = default;

    void MessageAlertItem::draw(
        QPainter& _p,
        const QRect& _rect,
        const Ui::ViewParams& _viewParams,
        const bool _isSelected,
        const bool _isHovered,
        const bool _isDrag,
        const bool _isKeyboardFocused) const
    {
        Utils::PainterSaver ps(_p);

        _p.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

        _p.setPen(Qt::NoPen);

        drawMouseState(_p, _rect, _isHovered, _isSelected);

        const auto width = _rect.width();

        bool isDefaultAvatar = false;

        const auto& contactListParams = compactMode_ ? Ui::GetContactListParams() : Ui::GetRecentsParams();

        //////////////////////////////////////////////////////////////////////////
        // render avatar
        int avatarX = 0, avatarY = 0;
        if (isMessageAlertSenderHidden())
        {
            const int scale = Utils::scale_bitmap(getAlertAvatarSize());
            const auto ratio = Utils::scale_bitmap_ratio();

            const QSize size(scale, scale);
            avatarX = _rect.left() + getAlertAvatarX();
            avatarY = _rect.top() + (_rect.height() - size.height() / ratio) / 2;
            _p.drawImage(QPoint(avatarX, avatarY), cachedLogo_);
        }
        else
        {
            auto avatar = Logic::GetAvatarStorage()->GetRounded(getAimid(), displayName_, Utils::scale_bitmap(getAlertAvatarSize()),
                isDefaultAvatar, false, compactMode_);

            const auto isOnline = Logic::GetLastseenContainer()->isOnline(getAimid());
            const auto statusBadge = Utils::getStatusBadge(getAimid(), avatar.width());
            const auto ratio = Utils::scale_bitmap_ratio();

            avatarX = _rect.left() + getAlertAvatarX();
            avatarY = _rect.top() + (_rect.height() - avatar.height() / ratio) / 2;

            Utils::StatusBadgeFlags statusFlags { Utils::StatusBadgeFlag::Small };
            statusFlags.setFlag(Utils::StatusBadgeFlag::Official, !multichat_ && official_);
            statusFlags.setFlag(Utils::StatusBadgeFlag::Muted, !mention_ && !multichat_ && muted_);
            statusFlags.setFlag(Utils::StatusBadgeFlag::Selected, _isSelected);
            statusFlags.setFlag(Utils::StatusBadgeFlag::Online, isOnline);
            Utils::drawAvatarWithBadge(_p, QPoint(avatarX, avatarY), avatar, statusBadge, statusFlags);
        }

        //////////////////////////////////////////////////////////////////////////
        // render contact name
        const int rightMarginName = width - contactListParams.itemHorPadding() - Utils::scale_value(32) /*close button*/;
        const int rightMarginMessage = width - getAlertMessageRightMargin();
        auto contactNameX = _rect.left() + getAlertAvatarX() + getAlertAvatarSize() + getAlertAvatarX();

        int nameMaxWidth = (rightMarginName - contactNameX);
        int messageMaxWidth = (rightMarginMessage - contactNameX);

        const auto contactNameY = avatarY;

        const bool widthNameChanged = (nameMaxWidth != prevWidthName_);

        name_->setColor(getContactNameColor(_isSelected));
        name_->setOffsets(contactNameX, contactNameY + Utils::text_sy());

        int currentNameWidth = nameWidth_;
        if (nameWidth_ > nameMaxWidth)
            currentNameWidth = nameMaxWidth;

        if (widthNameChanged)
            name_->elide(nameMaxWidth);

        name_->draw(_p);


        //////////////////////////////////////////////////////////////////////////
        // draw mediatype
        if (mediaType_ != MediaType::noMedia)
        {
            const QPixmap mediaPix = Logic::IconsManager::getMediaTypePix(mediaType_, _isSelected);

            const int pixY = _rect.top() + getMediaTypeIconTopMarginAlert(multichat_ && !readOnlyChat_);

            _p.drawPixmap(contactNameX, pixY, mediaPix);
        }

        //////////////////////////////////////////////////////////////////////////
        // draw message

        const auto messageY = _rect.top() + getAlertMessageY();

        const int iconOffset = Utils::scale_value(Logic::IconsManager::getMediaIconSize().width()) + Utils::scale_value(4);

        const bool widthMessageChanged = (messageMaxWidth != prevWidthMessage_);
        if (widthMessageChanged)
            messageShortName_->elide(messageMaxWidth);

        int yOffset = messageY + Utils::text_sy();
        if (mediaType_ != MediaType::noMedia && !messageLongName_)
            contactNameX += iconOffset;

        messageShortName_->setColor(messageLongName_ ? getNameColor(_isSelected, draft_ && !typersCount_) : getMessageColor(_isSelected));
        messageShortName_->setOffsets(contactNameX, yOffset);
        messageShortName_->draw(_p);

        if (messageLongName_)
        {
            if (!messageShortName_->getText().isEmpty())
                yOffset += get2LineMessageOffset();

            if (mediaType_ != MediaType::noMedia)
                contactNameX += iconOffset;

            messageLongName_->elide(messageMaxWidth - iconOffset);
            messageLongName_->setOffsets(contactNameX, yOffset);
            messageLongName_->setColor(getMessageColor(_isSelected));
            messageLongName_->draw(_p);
        }

        //////////////////////////////////////////////////////////////////////////
        // draw mention
        if (mention_)
        {
            static auto mentionIcon = Logic::IconsManager::getMentionIcon(_isSelected, QSize(getMentionAvatarIconSize(), getMentionAvatarIconSize()));

            const auto iconX = _rect.left() + getAvatarIconMargin();
            const auto iconY = _rect.top() + getAvatarIconMargin();

            QBrush hoverBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
            QBrush normalBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
            _p.setPen(Qt::NoPen);
            _p.setBrush(_isHovered ? hoverBrush : normalBrush);
            _p.setRenderHint(QPainter::Antialiasing);

            _p.drawEllipse(
                iconX,
                iconY,
                getMentionAvatarIconSize() + 2 * getAvatarIconBorderSize(),
                getMentionAvatarIconSize() + 2 * getAvatarIconBorderSize());

            _p.drawPixmap(
                iconX + getAvatarIconBorderSize(),
                iconY + getAvatarIconBorderSize(),
                Utils::unscale_bitmap(getMentionAvatarIconSize()),
                Utils::unscale_bitmap(getMentionAvatarIconSize()),
                mentionIcon.actualPixmap());
        }

        prevWidthName_ = nameMaxWidth;
        prevWidthMessage_ = messageMaxWidth;

        // draw underline
        QPen underlinePen(getAlertUnderlineColor());
        underlinePen.setWidth(Utils::scale_value(1));

        _p.setPen(underlinePen);
        _p.drawLine(_rect.bottomLeft(), _rect.bottomRight());
    }



    //////////////////////////////////////////////////////////////////////////
    // MailAlertItem
    //////////////////////////////////////////////////////////////////////////
    MailAlertItem::MailAlertItem(const Data::DlgState& _state)
        : RecentItemBase(_state)
        , fromNick_(_state.senderNick_)
        , fromMail_(_state.senderAimId_)
    {
        const auto& contactListParams = Ui::GetRecentsParams();

        name_ = TextRendering::MakeTextUnit(Utils::replaceLine(_state.senderNick_), Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);
        TextRendering::TextUnit::InitializeParameters params{ Fonts::appFont(contactListParams.contactNameFontSize(), Fonts::FontWeight::Normal), Styling::ColorParameter{ Qt::black } };
        name_->init(params);
        name_->evaluateDesiredSize();
        nameWidth_ = name_->desiredWidth();


        //////////////////////////////////////////////////////////////////////////
        // init message
        const int fontSize = contactListParams.messageFontSize();

        // control for short contact name

        messageLongName_ = TextRendering::MakeTextUnit(u"(" % _state.senderAimId_ % u")" % QChar::LineFeed, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS,
            Ui::TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS);
        params.setFonts(Fonts::appFontScaled(fontSize, Fonts::FontWeight::Normal));
        params.color_ = Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
        params.maxLinesCount_ = 1;
        params.lineBreak_ = Ui::TextRendering::LineBreakType::PREFER_SPACES;
        messageLongName_->init(params);
        messageLongName_->setLineSpacing(getMessageLineSpacing());

        auto secondPath2 = TextRendering::MakeTextUnit(_state.GetText(), Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS, Ui::TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        secondPath2->init(params);

        messageLongName_->appendBlocks(std::move(secondPath2));
        messageLongName_->evaluateDesiredSize();
    }

    MailAlertItem::~MailAlertItem() = default;

    void MailAlertItem::draw(
        QPainter& _p,
        const QRect& _rect,
        const Ui::ViewParams& _viewParams,
        const bool _isSelected,
        const bool _isHovered,
        const bool _isDrag,
        const bool _isKeyboardFocused) const
    {
        const Utils::PainterSaver saver(_p);

        _p.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

        _p.setPen(Qt::NoPen);

        drawMouseState(_p, _rect, _isHovered, _isSelected);

        const auto width = _rect.width(); //CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);

        bool isDefaultAvatar = false;

        const auto& contactListParams = Ui::GetRecentsParams();

        //////////////////////////////////////////////////////////////////////////
        // render avatar
        auto avatar = Logic::GetAvatarStorage()->GetRounded(fromMail_, fromNick_, Utils::scale_bitmap(getAlertAvatarSize()),
            isDefaultAvatar, false, false);

        const auto ratio = Utils::scale_bitmap_ratio();
        const auto avatarX = _rect.left() + getAlertAvatarX();
        const auto avatarY = _rect.top() + (_rect.height() - avatar.height() / ratio) / 2;

        Utils::drawAvatarWithoutBadge(_p, QPoint(avatarX, avatarY), avatar);

        static QPixmap mailIcon = getMailIcon(Utils::scale_bitmap(getMailAvatarIconSize()), _isSelected);

        const auto iconX = _rect.left() + getAvatarIconMargin();
        const auto iconY = _rect.top() + getAvatarIconMargin();

        QBrush hoverBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
        QBrush normalBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        _p.setPen(Qt::NoPen);
        _p.setBrush(_isHovered ? hoverBrush : normalBrush);
        _p.setRenderHint(QPainter::Antialiasing);

        _p.drawEllipse(
            iconX,
            iconY,
            getMailAvatarIconSize() + 2 * getAvatarIconBorderSize(),
            getMailAvatarIconSize() + 2 * getAvatarIconBorderSize());

        _p.drawPixmap(
            iconX + getAvatarIconBorderSize(),
            iconY + getAvatarIconBorderSize(),
            Utils::unscale_bitmap(getMailAvatarIconSize()),
            Utils::unscale_bitmap(getMailAvatarIconSize()),
            mailIcon);

        //////////////////////////////////////////////////////////////////////////
        // render contact name
        const int rightMarginName = width - contactListParams.itemHorPadding() - Utils::scale_value(32) /*close button*/;
        const int rightMarginMessage = width - getAlertMessageRightMargin();
        const auto contactNameX = _rect.left() + getAlertAvatarX() + getAlertAvatarSize() + getAlertAvatarX();

        int nameMaxWidth = (rightMarginName - contactNameX);
        int messageMaxWidth = (rightMarginMessage - contactNameX);

        const auto contactNameY = avatarY;

        name_->setColor(getContactNameColor(_isSelected));
        name_->setOffsets(contactNameX, contactNameY + Utils::text_sy());

        int currentNameWidth = nameWidth_;
        if (nameWidth_ > nameMaxWidth)
            currentNameWidth = nameMaxWidth;

        name_->elide(nameMaxWidth);
        name_->draw(_p);

        const auto& messageControl = messageLongName_;
        if (messageControl)
        {
            const auto messageY = _rect.top() + getAlertMessageY();

            messageControl->setOffsets(contactNameX, messageY + Utils::text_sy());

            messageControl->getHeight(messageMaxWidth);

            messageControl->setColor(getNameColor(_isSelected, false));
            if (!_isSelected)
                messageControl->setColorForAppended(getMessageColor(false));

            messageControl->draw(_p);
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // ComplexMailAlertItem
    //////////////////////////////////////////////////////////////////////////
    ComplexMailAlertItem::ComplexMailAlertItem(const Data::DlgState& _state)
        : RecentItemBase(_state)
    {
        const auto& contactListParams = Ui::GetRecentsParams();

        name_ = TextRendering::MakeTextUnit(_state.GetText(), Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);
        name_->init({ Fonts::appFont(contactListParams.contactNameFontSize(), Fonts::FontWeight::Normal), Styling::ColorParameter{ Qt::black } });
        name_->evaluateDesiredSize();
        nameWidth_ = name_->desiredWidth();
    }

    ComplexMailAlertItem::~ComplexMailAlertItem() = default;

    void ComplexMailAlertItem::draw(
        QPainter& _p,
        const QRect& _rect,
        const Ui::ViewParams& _viewParams,
        const bool _isSelected,
        const bool _isHovered,
        const bool _isDrag,
        const bool _isKeyboardFocused) const
    {
        static QPixmap mailIcon = getMailIcon(Utils::scale_bitmap(getMailIconSize()), _isSelected);
        im_assert(!mailIcon.isNull());

        const auto iconX = _rect.left() + getMailAlertIconX();
        const auto iconY = _rect.top() + (_rect.height() - getMailIconSize()) / 2;
        _p.drawPixmap(iconX, iconY, mailIcon);

        const auto& contactListParams = Ui::GetRecentsParams();

        const int rightMarginName = /*CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_)*/ _rect.width() - contactListParams.itemHorPadding();

        const auto contactNameX = iconX + Utils::unscale_bitmap(mailIcon.width()) + Utils::scale_value(16);

        int nameMaxWidth = rightMarginName - contactNameX;

        const auto contactNameY = _rect.center().y();

        name_->setColor(getContactNameColor(_isSelected));
        name_->setOffsets(contactNameX, contactNameY + Utils::text_sy());

        name_->elide(nameMaxWidth);
        name_->draw(_p, ::Ui::TextRendering::VerPosition::MIDDLE);
    }


    //////////////////////////////////////////////////////////////////////////
    // RecentItemDelegate
    //////////////////////////////////////////////////////////////////////////
    RecentItemDelegate::RecentItemDelegate(QObject* parent)
        : AbstractItemDelegateWithRegim(parent)
        , stateBlocked_(false)
    {
        connect(Logic::getContactListModel(), &Logic::ContactListModel::select, this, &RecentItemDelegate::onContactSelected);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::selectedContactChanged, this, &RecentItemDelegate::onItemClicked);
        connect(Logic::GetFriendlyContainer(), &Logic::FriendlyContainer::friendlyChanged, this, &RecentItemDelegate::onFriendlyChanged);
    }

    RecentItemDelegate::~RecentItemDelegate() = default;

    void RecentItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        const Data::DlgState dlg = _index.data(Qt::DisplayRole).value<Data::DlgState>();
        if (dlg.AimId_.isEmpty())
            return;

        const bool isDrag = (_index == dragIndex_);

        auto& item = items_[dlg.AimId_];
        if (!item)
            item = createItem(dlg);

        const bool isSelected = (dlg.AimId_ == selectedAimId_);
        const bool isHovered = (_option.state & QStyle::State_Selected) && !stateBlocked_ && !isSelected && !dragIndex_.isValid();

        item->draw(*_painter, _option.rect, viewParams_, isSelected, isHovered, isDrag, isKeyboardFocused());
    }

    QSize RecentItemDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _i) const
    {
        auto& recentParams = Ui::GetRecentsParams(viewParams_.regim_);

        auto height = recentParams.itemHeight();

        const auto recentsModel = Logic::getRecentsModel();
        if (recentsModel->isServiceItem(_i))
        {
            height = recentParams.serviceItemHeight();
        }
        else if (recentsModel->isPinnedServiceItem(_i))
        {
            const auto& contactListParams = Ui::GetContactListParams();
            height = contactListParams.itemHeight();
        }

        return QSize(_option.rect.width(), height);
    }

    QSize RecentItemDelegate::sizeHintForAlert() const
    {
        return QSize(Ui::GetRecentsParams(viewParams_.regim_).itemWidth(), Ui::GetRecentsParams(viewParams_.regim_).itemHeight());
    }

    void RecentItemDelegate::blockState(bool value)
    {
        stateBlocked_ = value;
    }

    void RecentItemDelegate::setDragIndex(const QModelIndex& index)
    {
        dragIndex_ = index;
    }

    void RecentItemDelegate::setPictOnlyView(bool _pictOnlyView)
    {
        viewParams_.pictOnly_ = _pictOnlyView;
    }

    bool RecentItemDelegate::getPictOnlyView() const
    {
        return viewParams_.pictOnly_;
    }

    void RecentItemDelegate::setFixedWidth(int _newWidth)
    {
        viewParams_.fixedWidth_ = _newWidth;
    }

    void RecentItemDelegate::setRegim(int _regim)
    {
        viewParams_.regim_ = _regim;
    }

    void RecentItemDelegate::onContactSelected(const QString& _aimId)
    {
        selectedAimId_ = _aimId;
    }

    void RecentItemDelegate::onItemClicked(const QString& _aimId)
    {
        selectedAimId_ = _aimId;
    }

    std::unique_ptr<RecentItemBase> RecentItemDelegate::createItem(const Data::DlgState& _state)
    {
        if (_state.AimId_.startsWith(u'~') && _state.AimId_.endsWith(u'~'))
        {
            if (_state.AimId_ == u"~unknowns~")
                return std::make_unique<RecentItemUnknowns>(_state);
            if (isPinnedServiceItem(_state))
                return std::make_unique<PinnedServiceItem>(_state);

            return std::make_unique<RecentItemService>(_state);
        }
        if (Features::isRecentsPinnedItemsEnabled() && Favorites::isFavorites(_state.AimId_))
            return std::make_unique<PinnedServiceItem>(_state);

        const bool compactMode = !Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, !config::get().is_on(config::features::compact_mode_by_default));
        return std::make_unique<RecentItemRecent>(_state, compactMode, false, false, shouldDisplayHeads(_state));
    }

    bool RecentItemDelegate::shouldDisplayHeads(const Data::DlgState& _state)
    {
        if (!Ui::get_gui_settings()->get_value<bool>(settings_show_groupchat_heads, true))
            return false;

        if (_state.mediaType_ == Ui::MediaType::mediaTypeVoip)
            return _state.Outgoing_;

        return true;
    }

    bool RecentItemDelegate::needsTooltip(const QString& _aimId, const QModelIndex&, QPoint _posCursor) const
    {
        const auto it = items_.find(_aimId);
        return it != items_.end() && it->second && it->second->needsTooltip(std::move(_posCursor));
    }

    QRect RecentItemDelegate::getDraftIconRectWrapper(const QString& _aimId, const QModelIndex&, QPoint _posCursor) const
    {
        const auto it = items_.find(_aimId);
        if (it != items_.end() && it->second)
            return it->second->getDraftIconRectWrapper();
        return {};
    }

    void RecentItemDelegate::onFriendlyChanged(const QString& _aimId, const QString& /*_friendly*/)
    {
        removeItem(_aimId);
    }

    void RecentItemDelegate::removeItem(const QString& _aimId)
    {
        if (const auto iter = std::as_const(items_).find(_aimId); iter != std::as_const(items_).end())
            items_.erase(iter);
    }

    void RecentItemDelegate::dlgStateChanged(const Data::DlgState& _dlgState)
    {
        removeItem(_dlgState.AimId_);
    }

    void RecentItemDelegate::refreshAll()
    {
        items_.clear();
    }

    RecentItemDelegate::ItemKey::ItemKey(const bool isSelected, const bool isHovered, const int unreadDigitsNumber)
        : IsSelected(isSelected)
        , IsHovered(isHovered)
        , UnreadDigitsNumber(unreadDigitsNumber)
    {
        im_assert(unreadDigitsNumber >= 0);
        im_assert(unreadDigitsNumber <= 2);
    }

    bool RecentItemDelegate::ItemKey::operator<(const ItemKey& _key) const
    {
        if (IsSelected != _key.IsSelected)
            return (IsSelected < _key.IsSelected);

        if (IsHovered != _key.IsHovered)
            return (IsHovered < _key.IsHovered);

        if (UnreadDigitsNumber != _key.UnreadDigitsNumber)
            return (UnreadDigitsNumber < _key.UnreadDigitsNumber);

        return false;
    }
}
