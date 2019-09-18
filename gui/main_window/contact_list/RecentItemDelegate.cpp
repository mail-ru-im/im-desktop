#include "stdafx.h"
#include "RecentItemDelegate.h"
#include "../../cache/avatars/AvatarStorage.h"

#include "ContactListModel.h"
#include "RecentsModel.h"
#include "UnknownsModel.h"
#include "../friendly/FriendlyContainer.h"

#include "ContactList.h"
#include "../history_control/LastStatusAnimation.h"
#include "../mediatype.h"

#include "../../types/contact.h"
#include "../../utils/utils.h"

#include "../history_control/HistoryControlPageItem.h"

#include "../../gui_settings.h"
#include "../../styles/ThemeParameters.h"

#include <boost/range/adaptor/reversed.hpp>
#include "main_window/LocalPIN.h"

namespace Ui
{
    //////////////////////////////////////////////////////////////////////////
    // RecentItemBase
    //////////////////////////////////////////////////////////////////////////

    QSize getPinHeaderSize()
    {
        return Utils::scale_value(QSize(16, 16));
    }

    QSize getPinSize()
    {
        return Utils::scale_value(QSize(12, 12));
    }

    QPixmap getPin(const bool _selected, const QSize& _sz)
    {
        return Utils::renderSvg(qsl(":/pin_icon"), _sz, Styling::getParameters().getColor(_selected ? Styling::StyleVariable::TEXT_SOLID_PERMANENT: Styling::StyleVariable::BASE_TERTIARY));
    }

    QPixmap getRemove(const bool _isSelected)
    {
        static const QPixmap rem(Utils::renderSvgScaled(qsl(":/ignore_icon"), QSize(20, 20), Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_ATTENTION)));
        static const QPixmap remSelected(Utils::renderSvgScaled(qsl(":/ignore_icon"), QSize(20, 20), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT)));

        return _isSelected ? remSelected : rem;
    }

    QPixmap getAdd(const bool _isSelected)
    {
        static const QPixmap add(Utils::renderSvgScaled(qsl(":/controls/add_icon"), QSize(20, 20), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY)));
        static const QPixmap addSelected(Utils::renderSvgScaled(qsl(":/controls/add_icon"), QSize(20, 20), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_ACTIVE)));

        return _isSelected ? addSelected : add;
    }

    int getPinPadding()
    {
        return Utils::scale_value(12);
    }

    int getUnreadsSize()
    {
        return Utils::scale_value(20);
    }

    QSize getContactsIconSize()
    {
        return QSize(20, 20);
    }

    QPixmap getContactsIcon()
    {
        return Utils::renderSvgScaled(qsl(":/contacts_icon"), getContactsIconSize());
    }

    QSize getAttentionSize()
    {
        return Utils::scale_value(QSize(20, 20));
    }

    QPixmap getAttentionIcon(const bool _selected)
    {
        const auto bg = _selected ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::PRIMARY;
        const auto star = _selected ? Styling::StyleVariable::PRIMARY : Styling::StyleVariable::TEXT_SOLID_PERMANENT;

        return Utils::renderSvgLayered(qsl(":/unread_mark_icon"),
            {
                { qsl("bg"), Styling::getParameters().getColor(bg) },
                { qsl("star"), Styling::getParameters().getColor(star) },
            });
    }

    QSize getMentionSize()
    {
        return Utils::scale_value(QSize(20, 20));
    }

    QPixmap getMentionIcon(const bool _selected, const QSize _size = QSize())
    {
        const auto bg = _selected ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::PRIMARY;
        const auto dog = _selected ? Styling::StyleVariable::PRIMARY : Styling::StyleVariable::TEXT_SOLID_PERMANENT;
        if (_size.isValid())
        {
            return Utils::renderSvgLayered(qsl(":/recent_mention_icon"),
                {
                    { qsl("bg"), Styling::getParameters().getColor(bg) },
                    { qsl("dog"), Styling::getParameters().getColor(dog) },
                }
            , _size);
        }

        return Utils::renderSvgLayered(qsl(":/recent_mention_icon"),
            {
                { qsl("bg"), Styling::getParameters().getColor(bg) },
                { qsl("dog"), Styling::getParameters().getColor(dog) },
            });
    }

    QPixmap getMailIcon(const int _size, const bool _isSelected)
    {
        return Utils::renderSvg(qsl(":/mail_icon"), QSize(_size, _size));
    }

    int getUnreadLeftPadding()
    {
        return Utils::scale_value(4);
    }

    int getUnreadLeftPaddingPictOnly()
    {
        return Utils::scale_value(2);
    }

    int getRigthLineCenter(const bool _compactMode)
    {
        return _compactMode ? Utils::scale_value(22) : Utils::scale_value(22);
    }

    int getUnreadBubbleWidthMin()
    {
        return Utils::scale_value(20);
    }

    int getUnreadBubbleHeight()
    {
        return Utils::scale_value(20);
    }

    int getUnreadBubbleRightMargin(const bool _compactMode)
    {
        return getRigthLineCenter(_compactMode) - (getUnreadBubbleWidthMin() / 2);
    }

    int getUnreadBubbleRightMarginPictOnly()
    {
        return Utils::scale_value(4);
    }

    int getUnreadBubbleMarginPictOnly()
    {
        return Utils::scale_value(4);
    }

    int getMessageY()
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(23);
        else
            return Utils::scale_value(28);
    }

    int getMultiChatMessageOffset()
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(1);
        else
            return 0;
    }

    int getUnreadY()
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(32);
        else
            return Utils::scale_value(36);
    }

    int getNameY(const bool _compactMode)
    {
        return _compactMode ? Utils::scale_value(11) : platform::is_apple() ? Utils::scale_value(6) : Utils::scale_value(8);
    }

    int getMessageLineSpacing()
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(1);
        else
            return Utils::scale_value(0/*-2*/);
    }

    QColor getNameColor(const bool _isSelected)
    {
        return Styling::getParameters().getColor(_isSelected ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::TEXT_SOLID);
    }

    Fonts::FontWeight getNameFontWeight()
    {
        return Fonts::FontWeight::Normal;
    }

    QColor getContactNameColor(const bool _isSelected)
    {
        return Styling::getParameters().getColor(_isSelected ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::TEXT_SOLID);
    }

    int getAlertAvatarSize()
    {
        return Utils::scale_value(60);
    }

    int getAlertAvatarX()
    {
        return Utils::scale_value(20);
    }

    int getMailAlertIconX()
    {
        return Utils::scale_value(20);
    }

    int getAlertMessageY()
    {
        return Utils::scale_value(40);
    }

    int getAlertMessageRightMargin()
    {
        return Utils::scale_value(20);
    }

    int getMailIconSize()
    {
        return Utils::scale_value(60);
    }

    int getAvatarIconBorderSize()
    {
        return Utils::scale_value(4);
    }

    int getMailAvatarIconSize()
    {
        return Utils::scale_value(20);
    }

    int getAvatarIconMargin()
    {
        return Utils::scale_value(12);
    }

    int getMentionAvatarIconSize()
    {
        return Utils::scale_value(20);
    }

    int getDndOverlaySize()
    {
        return Utils::scale_value(60);
    }

    QColor getAlertUnderlineColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
    }

    QColor getMessageColor(const bool _selected)
    {
        return Styling::getParameters().getColor(_selected ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::BASE_PRIMARY);
    }

    QColor getDateColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
    }

    QColor getServiceItemColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
    }

    int getServiceItemHorPadding()
    {
        return Utils::scale_value(12);
    }

    QColor getMediaIconColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY);
    }

    int getMediaTypeIconTopMargin(const bool _multichat)
    {
        if constexpr (platform::is_apple())
            return (_multichat ? Utils::scale_value(40) : Utils::scale_value(24));
        else
            return (_multichat ? Utils::scale_value(48) : Utils::scale_value(30));
    }

    int getMediaTypeIconTopMarginAlert(const bool _multichat)
    {
        return (_multichat ? Utils::scale_value(60) : Utils::scale_value(42));
    }

    const QSize getMediaIconSize()
    {
        return QSize(16, 16);
    }

    int get2LineMessageOffset()
    {
        return Utils::scale_value(18);
    }

    int getUnknownTextSize()
    {
        return 16;
    }

    QColor getUnknownTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);
    }

    int getUnknownHorPadding()
    {
        return Utils::scale_value(12);
    }

    int getMessageRightMargin()
    {
        return Utils::scale_value(4);
    }

    int getUnknownRightOffset()
    {
        return Utils::scale_value(40);
    }

    int getTimeYOffsetFromName()
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

    const QPixmap& getMediaTypePix(const MediaType _mediaType, const bool _isSelected)
    {
        switch (_mediaType)
        {
            case MediaType::mediaTypeSticker:
            {
                static QPixmap pix = Utils::renderSvgScaled(qsl(":/message_type_sticker_icon"), getMediaIconSize(), getMediaIconColor());
                static QPixmap pixSelected = Utils::renderSvgScaled(qsl(":/message_type_sticker_icon"), getMediaIconSize(), Qt::white);

                return (_isSelected ? pixSelected : pix);
            }
            case MediaType::mediaTypeVideo:
            {
                static QPixmap pix = Utils::renderSvgScaled(qsl(":/message_type_video_icon"), getMediaIconSize(), getMediaIconColor());
                static QPixmap pixSelected = Utils::renderSvgScaled(qsl(":/message_type_video_icon"), getMediaIconSize(), Qt::white);

                return (_isSelected ? pixSelected : pix);
            }
            case MediaType::mediaTypePhoto:
            {
                static QPixmap pix = Utils::renderSvgScaled(qsl(":/message_type_photo_icon"), getMediaIconSize(), getMediaIconColor());
                static QPixmap pixSelected = Utils::renderSvgScaled(qsl(":/message_type_photo_icon"), getMediaIconSize(), Qt::white);

                return (_isSelected ? pixSelected : pix);
            }
            case MediaType::mediaTypeGif:
            {
                static QPixmap pix = Utils::renderSvgScaled(qsl(":/message_type_video_icon"), getMediaIconSize(), getMediaIconColor());
                static QPixmap pixSelected = Utils::renderSvgScaled(qsl(":/message_type_video_icon"), getMediaIconSize(), Qt::white);

                return (_isSelected ? pixSelected : pix);
            }
            case MediaType::mediaTypePtt:
            {
                static QPixmap pix = Utils::renderSvgScaled(qsl(":/message_type_audio_icon"), getMediaIconSize(), getMediaIconColor());
                static QPixmap pixSelected = Utils::renderSvgScaled(qsl(":/message_type_audio_icon"), getMediaIconSize(), Qt::white);

                return (_isSelected ? pixSelected : pix);
            }
            case MediaType::mediaTypeVoip:
            {
                static QPixmap pix = Utils::renderSvgScaled(qsl(":/message_type_phone_icon"), getMediaIconSize(), getMediaIconColor());
                static QPixmap pixSelected = Utils::renderSvgScaled(qsl(":/message_type_phone_icon"), getMediaIconSize(), Qt::white);

                return (_isSelected ? pixSelected : pix);
            }
            case MediaType::mediaTypeFileSharing:
            {
                static QPixmap pix = Utils::renderSvgScaled(qsl(":/message_type_file_icon"), getMediaIconSize(), getMediaIconColor());
                static QPixmap pixSelected = Utils::renderSvgScaled(qsl(":/message_type_file_icon"), getMediaIconSize(), Qt::white);

                return (_isSelected ? pixSelected : pix);
            }
            case MediaType::mediaTypeContact:
            {
                static QPixmap pix = Utils::renderSvgScaled(qsl(":/message_type_contact_icon"), getMediaIconSize(), getMediaIconColor());
                static QPixmap pixSelected = Utils::renderSvgScaled(qsl(":/message_type_contact_icon"), getMediaIconSize(), Qt::white);

                return (_isSelected ? pixSelected : pix);
            }

            default:
            {
                assert(false);

                static QPixmap pix;
                return pix;
            }
        }
    }

    const QPixmap& getArrowIcon(const bool _rotated)
    {
        static QPixmap p_normal = Utils::renderSvgScaled(qsl(":/controls/down_icon"), QSize(12, 12), getServiceItemColor());
        static QPixmap p_mirrored = Utils::mirrorPixmapVer(p_normal);

        return (_rotated ? p_mirrored : p_normal);
    }

    void renderRecentsDragOverlay(QPainter &_painter, const QRect& _rect, const ViewParams& _viewParams)
    {
        const auto& contactListParams = Ui::GetRecentsParams();

        Utils::PainterSaver ps(_painter);

        _painter.setPen(Qt::NoPen);
        _painter.setRenderHint(QPainter::Antialiasing);

        auto width = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);

        QColor overlayColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        overlayColor.setAlphaF(0.98);
        _painter.fillRect(_rect.left(), _rect.top(), width, _rect.height(), QBrush(overlayColor));
        _painter.setBrush(QBrush(Styling::getParameters().getColor(Styling::StyleVariable::GHOST_ACCENT)));

        QPen pen(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), ContactListParams::dragOverlayBorderWidth(), Qt::SolidLine, Qt::RoundCap);
        _painter.setPen(pen);

        _painter.drawRoundedRect(
            _rect.left() + ContactListParams::dragOverlayPadding(),
            _rect.top() + ContactListParams::dragOverlayVerPadding(),
            width - contactListParams.itemHorPadding(),
            _rect.height() - ContactListParams::dragOverlayVerPadding(),
            ContactListParams::dragOverlayBorderRadius(),
            ContactListParams::dragOverlayBorderRadius()
        );


        if (_viewParams.pictOnly_)
        {
            static QPixmap fast_send_green = Utils::renderSvg(qsl(":/fast_send"), QSize(getDndOverlaySize(), getDndOverlaySize()), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_PRIMARY));
            _painter.drawPixmap(_rect.left() + _rect.width() / 2 - getDndOverlaySize() / 2, _rect.top() + _rect.height() / 2 - getDndOverlaySize() / 2, fast_send_green);
        }
        else
        {
            _painter.setFont(Fonts::appFontScaled(15));
            Utils::drawText(_painter, QPoint(_rect.width() / 2, _rect.y() + _rect.height() / 2), Qt::AlignCenter, QT_TRANSLATE_NOOP("files_widget", "Quick send"));
        }
    }

    RecentItemBase::RecentItemBase(const Data::DlgState& _state)
        : aimid_(_state.AimId_)
    {
    }

    RecentItemBase::~RecentItemBase()
    {
    }

    void RecentItemBase::draw(QPainter& _p, const QRect& _rect, const Ui::ViewParams& _viewParams, const bool _isSelected, const bool _isHovered, const bool _isDrag, const bool _isKeyboardFocused)
    {
    }

    void RecentItemBase::drawMouseState(QPainter& _p, const QRect& _rect, const bool _isHovered, const bool _isSelected, const bool _isKeyboardFocused)
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

    //////////////////////////////////////////////////////////////////////////
    // RecentItemService
    //////////////////////////////////////////////////////////////////////////
    RecentItemService::RecentItemService(const Data::DlgState& _state)
        : RecentItemBase(_state)
    {
        text_ = TextRendering::MakeTextUnit(_state.GetText());
        // FONT
        text_->init(Fonts::appFontScaled(11, Fonts::FontWeight::SemiBold), getServiceItemColor());
        text_->evaluateDesiredSize();

        if (_state.AimId_ == ql1s("~recents~"))
        {
            type_ = ServiceItemType::recents;
        }
        else if (_state.AimId_ == ql1s("~favorites~"))
        {
            type_ = ServiceItemType::favorites;
        }
        else if (_state.AimId_ == ql1s("~unknowns~"))
        {
            type_ = ServiceItemType::unknowns;
        }
    }

    RecentItemService::RecentItemService(const QString& _text)
        : RecentItemBase(Data::DlgState())
    {
        text_ = TextRendering::MakeTextUnit(_text);
        // FONT
        text_->init(Fonts::appFontScaled(11, Fonts::FontWeight::SemiBold), getServiceItemColor());
        text_->evaluateDesiredSize();
    }

    RecentItemService::~RecentItemService()
    {
    }

    void RecentItemService::draw(QPainter& _p, const QRect& _rect, const Ui::ViewParams& _viewParams, const bool _isSelected, const bool _isHovered, const bool _isDrag, const bool _isKeyboardFocused)
    {
        auto recentParams = GetRecentsParams(_viewParams.regim_);
        const auto ratio = Utils::scale_bitmap_ratio();
        const auto height = recentParams.serviceItemHeight();

        if (!text_)
        {
            assert(false);
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

             static QPixmap pixFavorites = getPin(false, getPinHeaderSize());
             static QPixmap pixRecents = Utils::loadPixmap(qsl(":/resources/icon_recents_100.png"));

             const auto& p = (type_ == ServiceItemType::favorites ? pixFavorites : pixRecents);

             int y = height / 2;
             int xp = ItemWidth(_viewParams) / 2 - (p.width() / 2. / ratio);
             int yp = height / 2 - (p.height() / 2. / ratio);
             _p.drawLine(0, y, xp - recentParams.serviceItemIconPadding(), y);
             _p.drawLine(xp + p.width() / ratio + recentParams.serviceItemIconPadding(), y, ItemWidth(_viewParams), y);
             _p.drawPixmap(xp, yp, p);
        }

        if (type_ == ServiceItemType::favorites && !_viewParams.pictOnly_)
        {
            const auto& p = getArrowIcon(Logic::getRecentsModel()->isFavoritesVisible());
            QPen line_pen;
            line_pen.setColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY));
            _p.setPen(line_pen);

            const int x = text_->cachedSize().width() + getServiceItemHorPadding() + recentParams.favoritesStatusPadding();
            int y = height / 2 - (p.height() / 2. / ratio);

            _p.drawPixmap(x, y, p);
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // RecentItemUnknowns
    //////////////////////////////////////////////////////////////////////////
    RecentItemUnknowns::RecentItemUnknowns(const Data::DlgState& _state)
        : RecentItemBase(_state)
        , compactMode_(!Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, true))
        , count_(Logic::getUnknownsModel()->totalUnreads())
    {
        text_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("contact_list", "New contacts"));
        // FONT
        text_->init(Fonts::appFontScaled(getUnknownTextSize()), getUnknownTextColor());
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

    RecentItemUnknowns::~RecentItemUnknowns()
    {
    }

    void RecentItemUnknowns::draw(QPainter& _p, const QRect& _rect, const Ui::ViewParams& _viewParams, const bool _isSelected, const bool _isHovered, const bool _isDrag, const bool _isKeyboardFocused)
    {
        const auto width = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);

        Utils::PainterSaver ps(_p);

        _p.setPen(Qt::NoPen);

        drawMouseState(_p, _rect, _isHovered, _isSelected);

        auto& contactListParams = compactMode_ ? Ui::GetContactListParams() : Ui::GetRecentsParams();

        static QPixmap pict = getContactsIcon();

        int offset_x = _viewParams.pictOnly_  ? (width - Utils::scale_value(getContactsIconSize().width())) / 2 : getUnknownHorPadding();

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
            {
                unreadsX = getUnknownHorPadding();
            }

            auto unreadsY = (contactListParams.unknownsItemHeight() - getUnreadsSize()) / 2;
            if (_viewParams.pictOnly_)
            {
                unreadsY = Utils::scale_value(4);
            }

            auto borderColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
            if (_isHovered)
            {
                borderColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
            }
            else if (_isSelected)
            {
                borderColor = (_viewParams.pictOnly_) ? Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY) : QColor();
            }

            const auto bgColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
            const auto textColor = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);

            Utils::drawUnreads(&_p, contactListParams.unreadsFont(), bgColor, textColor, borderColor, count_, getUnreadsSize(), _rect.left() + unreadsX, _rect.top() + unreadsY);
        }
    }

    LastStatus getLastStatus(const Data::DlgState& _state)
    {
        LastStatus st = LastStatus::None;

        if (_state.UnreadCount_ || _state.Attention_)
            return st;

        if (!_state.IsLastMessageDelivered_)
        {
            st = LastStatus::Pending;
        }
        else if (_state.LastMsgId_ <= _state.TheirsLastDelivered_)
        {
            st = LastStatus::DeliveredToPeer;
        }
        else
        {
            st = LastStatus::DeliveredToServer;
        }

        return st;
    }

    const QPixmap getStatePixmap(const LastStatus& _lastStatus, const bool _selected, const bool _multichat)
    {
        const auto normalClr = Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY);
        const auto selClr = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT);

        switch (_lastStatus)
        {
            case LastStatus::Pending:
            {
                return LastStatusAnimation::getPendingPixmap(_selected ? selClr : normalClr);
            }
            case LastStatus::DeliveredToServer:
            {
                if (_multichat)
                    return LastStatusAnimation::getDeliveredToClientPixmap(_selected ? selClr : normalClr);

                return LastStatusAnimation::getDeliveredToServerPixmap(_selected ? selClr : normalClr);
            }
            case LastStatus::DeliveredToPeer:
            {
                if (_multichat)
                    return LastStatusAnimation::getDeliveredToServerPixmap(_selected ? selClr : normalClr);

                return LastStatusAnimation::getDeliveredToClientPixmap(_selected ? selClr : normalClr);
            }

            case LastStatus::None:
            case LastStatus::Read:
                break;
        }

        return QPixmap();
    }

    QString getSender(const Data::DlgState& _state)
    {
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
        const bool _compactMode,
        const bool _hideMessage,
        const bool _showHeads
    )
        : RecentItemBase(_state)
        , multichat_(Logic::getContactListModel()->isChat(_state.AimId_))
        , displayName_(Utils::replaceLine(Logic::GetFriendlyContainer()->getFriendly(_state.AimId_)))
        , messageNameWidth_(-1)
        , compactMode_(_compactMode)
        , unreadCount_(_state.UnreadCount_)
        , attention_(_state.Attention_)
        , mention_(_state.unreadMentionsCount_ > 0)
        , prevWidthName_(-1)
        , prevWidthMessage_(-1)
        , muted_(Logic::getContactListModel()->isMuted(_state.AimId_))
        , pin_(_state.FavoriteTime_ != -1)
        , typersCount_(_state.typings_.size())
        , typer_(_state.getTypers())
        , mediaType_(_hideMessage ? MediaType::noMedia : _state.mediaType_)
        , readOnlyChat_(multichat_ && (_state.AimId_ == _state.senderAimId_))
    {
        const auto friendly = Logic::GetFriendlyContainer()->getFriendly2(_state.AimId_);
        displayName_ = Utils::replaceLine(friendly.name_);
        official_ = friendly.official_;
        const auto& contactListParams = compactMode_ ? Ui::GetContactListParams() : Ui::GetRecentsParams();
        name_ = TextRendering::MakeTextUnit(displayName_, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        // FONT
        name_->init(Fonts::appFont(contactListParams.contactNameFontSize(), contactListParams.contactNameFontWeight()),
            Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID),
            QColor(), QColor(), QColor(),
            TextRendering::HorAligment::LEFT,
            1
            );
        name_->evaluateDesiredSize();
        nameWidth_ = name_->desiredWidth();

        //////////////////////////////////////////////////////////////////////////
        // init message
        if (!compactMode_)
        {
            const int fontSize = contactListParams.messageFontSize();

            if (typersCount_)
            {
                static auto typingTextSingle = QT_TRANSLATE_NOOP("contact_list", "is typing...");
                static auto typingTextMulty = QT_TRANSLATE_NOOP("contact_list", "are typing...");
                const auto& typingText = ((typersCount_ > 1) ? typingTextMulty : typingTextSingle);

                messageShortName_ = TextRendering::MakeTextUnit((multichat_ ? (typer_ + QChar::LineFeed) : QString()), Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS,
                    Ui::TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS);

                // FONT
                messageShortName_->init(
                    Fonts::appFont(fontSize, getNameFontWeight()),
                    Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID),
                    QColor(), QColor(), QColor(),
                    TextRendering::HorAligment::LEFT,
                    2,
                    Ui::TextRendering::LineBreakType::PREFER_SPACES
                );

                messageShortName_->evaluateDesiredSize();
                messageShortName_->setLineSpacing(getMessageLineSpacing());
                messageNameWidth_ = messageShortName_->desiredWidth();

                auto secondPath1 = TextRendering::MakeTextUnit((multichat_ ? qsl(" ") : QString()) + typingText, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS, Ui::TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
                // FONT
                secondPath1->init(
                    Fonts::appFont(fontSize, Fonts::FontWeight::Normal),
                    Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID),
                    QColor(), QColor(), QColor(),
                    TextRendering::HorAligment::LEFT, 1, Ui::TextRendering::LineBreakType::PREFER_SPACES);

                messageShortName_->append(std::move(secondPath1));
                messageShortName_->evaluateDesiredSize();

                // control for long contact name
                messageLongName_ = TextRendering::MakeTextUnit((multichat_ ? (typer_ + QChar::LineFeed) : QString()), Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS,
                    Ui::TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS);
                // FONT
                messageLongName_->init(
                    Fonts::appFont(fontSize, Fonts::FontWeight::Normal),
                    Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID),
                    QColor(), QColor(), QColor(),
                    TextRendering::HorAligment::LEFT, 1, Ui::TextRendering::LineBreakType::PREFER_SPACES);
                messageLongName_->setLineSpacing(getMessageLineSpacing());

                auto secondPath2 = TextRendering::MakeTextUnit(typingText, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS, Ui::TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
                // FONT
                secondPath2->init(
                    Fonts::appFont(fontSize, Fonts::FontWeight::Normal),
                    Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID),
                    QColor(), QColor(), QColor(),
                    TextRendering::HorAligment::LEFT, 1, Ui::TextRendering::LineBreakType::PREFER_SPACES);

                messageLongName_->appendBlocks(std::move(secondPath2));
                messageLongName_->evaluateDesiredSize();
            }
            else
            {
                const QString messageText = _hideMessage ? QT_TRANSLATE_NOOP("notifications", "New message") : Utils::replaceLine(_state.GetText());

                if (!multichat_ || readOnlyChat_)
                {
                    messageShortName_ = TextRendering::MakeTextUnit(messageText, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS, Ui::TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
                    // FONT
                    messageShortName_->init(Fonts::appFont(fontSize, Fonts::FontWeight::Normal),
                                            Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY),
                                            QColor(), QColor(), QColor(),
                                            TextRendering::HorAligment::LEFT, 2,
                                            Ui::TextRendering::LineBreakType::PREFER_SPACES);

                    messageShortName_->markdown(Fonts::appFont(fontSize, Fonts::FontWeight::Normal),
                                                Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY),
                                                Ui::TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
                    messageShortName_->setLineSpacing(getMessageLineSpacing());
                }
                else
                {
                    // control for short contact name
                    QString sender = (_state.Outgoing_ ? QT_TRANSLATE_NOOP("contact_list", "Me") : getSender(_state));
                    if (!sender.isEmpty())
                    {
                        sender += qsl(": ");
                    }

                    if (mediaType_ == MediaType::noMedia)
                    {
                        messageShortName_ = TextRendering::MakeTextUnit(sender + QChar::LineFeed, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS,
                            Ui::TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS);
                        // FONT
                        messageShortName_->init(Fonts::appFont(fontSize, getNameFontWeight()),
                                                Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY),
                                                QColor(), QColor(), QColor(),
                                                TextRendering::HorAligment::LEFT, 2, Ui::TextRendering::LineBreakType::PREFER_SPACES);
                        messageShortName_->evaluateDesiredSize();
                        messageShortName_->setLineSpacing(getMessageLineSpacing());
                        messageNameWidth_ = messageShortName_->desiredWidth();

                        // FONT
                        auto secondPath1 = TextRendering::MakeTextUnit(messageText, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS, Ui::TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
                        secondPath1->init(Fonts::appFont(fontSize, Fonts::FontWeight::Normal),
                                            Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY),
                                            QColor(), QColor(), QColor(),
                                            TextRendering::HorAligment::LEFT, 1, Ui::TextRendering::LineBreakType::PREFER_SPACES);
                        secondPath1->markdown(Fonts::appFont(fontSize, Fonts::FontWeight::Normal),
                                              Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY),
                                              Ui::TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);

                        messageShortName_->append(std::move(secondPath1));
                        messageShortName_->evaluateDesiredSize();

                        // constrol for long contact name
                        messageLongName_ = TextRendering::MakeTextUnit(sender + QChar::LineFeed, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS,
                            Ui::TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS);

                        // FONT
                        messageLongName_->init(Fonts::appFont(fontSize, Fonts::FontWeight::Normal),
                            Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY),
                            QColor(), QColor(), QColor(),
                            TextRendering::HorAligment::LEFT, 1, Ui::TextRendering::LineBreakType::PREFER_SPACES);
                        messageLongName_->setLineSpacing(getMessageLineSpacing());

                        auto secondPath2 = TextRendering::MakeTextUnit(messageText, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS, Ui::TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
                        secondPath2->init(Fonts::appFont(fontSize, Fonts::FontWeight::Normal),
                            Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY),
                            QColor(), QColor(), QColor(),
                            TextRendering::HorAligment::LEFT, 1, Ui::TextRendering::LineBreakType::PREFER_SPACES);

                        secondPath2->markdown(Fonts::appFont(fontSize, Fonts::FontWeight::Normal),
                                              Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY),
                                              Ui::TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);

                        messageLongName_->appendBlocks(std::move(secondPath2));
                        messageLongName_->evaluateDesiredSize();
                    }
                    else
                    {
                        messageShortName_ = TextRendering::MakeTextUnit(sender, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS, Ui::TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS);
                        // FONT
                        messageShortName_->init(Fonts::appFont(fontSize, getNameFontWeight()),
                            Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY),
                            QColor(), QColor(), QColor(),
                            TextRendering::HorAligment::LEFT, 1, Ui::TextRendering::LineBreakType::PREFER_SPACES);

                        messageShortName_->evaluateDesiredSize();
                        messageNameWidth_ = messageShortName_->desiredWidth();

                        messageLongName_ = TextRendering::MakeTextUnit(messageText, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS, Ui::TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS);
                        // FONT
                        messageLongName_->init(Fonts::appFont(fontSize, Fonts::FontWeight::Normal),
                            Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY),
                            QColor(), QColor(), QColor(),
                            TextRendering::HorAligment::LEFT, 1, Ui::TextRendering::LineBreakType::PREFER_SPACES);
                        messageLongName_->markdown(Fonts::appFont(fontSize), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
                        messageLongName_->evaluateDesiredSize();
                    }
                }
            }
        }

        const auto timeStr = _state.Time_ > 0 ? ::Ui::FormatTime(QDateTime::fromTime_t(_state.Time_)) : QString();
        time_ = TextRendering::MakeTextUnit(timeStr, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);
        // FONT NOT FIXED
        time_->init(Fonts::appFont(Utils::scale_value(11), Fonts::FontWeight::Normal), Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY));
        time_->evaluateDesiredSize();

        //////////////////////////////////////////////////////////////////////////
        // init unread
        if (unreadCount_)
        {
            unreadBalloonWidth_ = Utils::getUnreadsBadgeSize(unreadCount_, getUnreadBubbleHeight()).width();
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
                    if (isLastRead && (_state.lastMessage_ && _state.lastMessage_->HasId()))
                    {
                        drawLastReadAvatar_ = true;
                    }
                    else
                    {
                        lastStatus_ = getLastStatus(_state);
                    }
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

    RecentItemRecent::~RecentItemRecent()
    {
    }

    void RecentItemRecent::draw(
        QPainter& _p,
        const QRect& _rect,
        const Ui::ViewParams& _viewParams,
        const bool _isSelected,
        const bool _isHovered,
        const bool _isDrag,
        const bool _isKeyboardFocused)
    {
        Utils::PainterSaver ps(_p);

        _p.setPen(Qt::NoPen);

        drawMouseState(_p, _rect, _isHovered, _isSelected, _isKeyboardFocused);

        const auto width = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);

        auto state = multichat_ ? QString() : Logic::getContactListModel()->getState(getAimid());
        if (_isSelected && (state == ql1s("online") || state == ql1s("mobile")))
            state += ql1s("_active");

        bool isDefaultAvatar = false;

        auto contactListParams = compactMode_ ? Ui::GetContactListParams() : Ui::GetRecentsParams();

        //////////////////////////////////////////////////////////////////////////
        // render avatar
        auto avatar = Logic::GetAvatarStorage()->GetRounded(getAimid(), displayName_, Utils::scale_bitmap(contactListParams.getAvatarSize()),
            state, isDefaultAvatar, false, compactMode_);

        const auto ratio = Utils::scale_bitmap_ratio();
        const auto avatarX = _rect.left() + (_viewParams.pictOnly_ ? (_rect.width() - avatar->width() / ratio) / 2 : contactListParams.getAvatarX());
        const auto avatarY = _rect.top() + (_rect.height() - avatar->height() / ratio) / 2;

        Utils::drawAvatarWithBadge(_p, QPoint(avatarX, avatarY), *avatar, official_, muted_, _isSelected);

        const auto isUnknown = (_viewParams.regim_ == ::Logic::MembersWidgetRegim::UNKNOWN);

        auto unknownUnreads = 0;
        if (isUnknown && !_viewParams.pictOnly_)
        {
            const auto& remove_img = getRemove(_isSelected);

            static const auto buttonWidth = Utils::scale_value(32);

            const int addX = (_rect.width() - buttonWidth);
            const int x = _rect.x() + addX;
            const int addY = (contactListParams.itemHeight() - remove_img.height() / ratio) / 2.;
            const int y = _rect.top() + addY;

            contactListParams.removeContactFrame().setX(addX);
            contactListParams.removeContactFrame().setY(addY);
            contactListParams.removeContactFrame().setWidth(remove_img.width());
            contactListParams.removeContactFrame().setHeight(remove_img.height());

            _p.drawPixmap(x, y, remove_img);

            unknownUnreads = x - getUnknownRightOffset();
        }

        auto unreadsY = (contactListParams.itemHeight() - getUnreadsSize()) / 2;

        if (_viewParams.pictOnly_)
            unreadsY = getUnreadBubbleMarginPictOnly();
        else if (!compactMode_)
            unreadsY = getUnreadY();

        auto unreadsX = width;

        //////////////////////////////////////////////////////////////////////////
        // draw unreads

        if (unreadCount_)
        {
            unreadsX -= _viewParams.pictOnly_ ? getUnreadBubbleRightMarginPictOnly() : getUnreadBubbleRightMargin(compactMode_);

            unreadsX -= unreadBalloonWidth_;

            const auto bgColor = Styling::getParameters().getColor( _isSelected ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : muted_ ? Styling::StyleVariable::BASE_TERTIARY : Styling::StyleVariable::PRIMARY);
            const auto textColor = Styling::getParameters().getColor( _isSelected ? Styling::StyleVariable::PRIMARY : Styling::StyleVariable::TEXT_SOLID_PERMANENT);

            auto x = (isUnknown && !_viewParams.pictOnly_) ? unknownUnreads : _rect.left() + unreadsX;
            auto y = (isUnknown && !_viewParams.pictOnly_) ? _rect.top() + (contactListParams.itemHeight() - getUnreadBubbleHeight()) / 2 : _rect.top() + unreadsY;

            Utils::drawUnreads(_p, contactListParams.unreadsFont(), bgColor, textColor, unreadCount_, getUnreadBubbleHeight(), x, y);
        }
        else if (attention_)
        {
            unreadsX -= _viewParams.pictOnly_ ? getUnreadBubbleRightMarginPictOnly() : getUnreadBubbleRightMargin(compactMode_);

            static QPixmap attentionPixNormal = getAttentionIcon(false);
            static QPixmap attentionPixSelected = getAttentionIcon(true);
            const auto& attentionPix = _isSelected ? attentionPixSelected : attentionPixNormal;

            unreadsX -= getAttentionSize().width();

            auto x = (isUnknown && !_viewParams.pictOnly_) ? unknownUnreads : _rect.left() + unreadsX;
            auto y = (isUnknown && !_viewParams.pictOnly_) ? _rect.top() + (contactListParams.itemHeight() - getUnreadBubbleHeight()) / 2 : _rect.top() + unreadsY;

            _p.drawPixmap(x, y, attentionPix);
        }

        //////////////////////////////////////////////////////////////////////////
        // draw mention
        if (mention_ && !isUnknown)
        {
            if (unreadCount_)
                unreadsX -= _viewParams.pictOnly_ ? getUnreadLeftPaddingPictOnly() : getUnreadLeftPadding();
            else
                unreadsX -= _viewParams.pictOnly_ ? getUnreadBubbleRightMarginPictOnly() : getUnreadBubbleRightMargin(compactMode_);

            unreadsX -= getMentionSize().width();

            static QPixmap pixNormal = getMentionIcon(false);
            static QPixmap pixSelected = getMentionIcon(true);
            QPixmap& pix = _isSelected ? pixSelected : pixNormal;

            _p.drawPixmap(_rect.left() + unreadsX, _rect.top() + unreadsY, pix);
        }

        //////////////////////////////////////////////////////////////////////////
        // draw status
        const int avatarRightMargin = (getRigthLineCenter(compactMode_) - (contactListParams.getLastReadAvatarSize()) / 2);

        if (!_viewParams.pictOnly_ && !isUnknown)
        {
            if (drawLastReadAvatar_)
            {
                bool isDefault = false;
                auto lastReadAvatar = Logic::GetAvatarStorage()->GetRounded(aimid_, displayName_,
                    Utils::scale_bitmap(contactListParams.getLastReadAvatarSize()), QString(), isDefault, false, false);

                const auto opacityOld = _p.opacity();

                if (!_isSelected)
                    _p.setOpacity(0.7);

                unreadsX -= avatarRightMargin;
                unreadsX -= Utils::unscale_bitmap(lastReadAvatar->width());

                const int y_offset = (getUnreadBubbleHeight() - Utils::unscale_bitmap(lastReadAvatar->height())) / 2;

                _p.drawPixmap(_rect.left() + unreadsX, _rect.top() + unreadsY + y_offset, *lastReadAvatar);

                _p.setOpacity(opacityOld);
            }
            else if (lastStatus_ != LastStatus::None)
            {
                if (const auto& pix = getStatePixmap(lastStatus_, _isSelected, multichat_); !pix.isNull())
                {
                    const int rightMargin = getRigthLineCenter(compactMode_) - (Utils::unscale_bitmap(pix.width()) / 2);

                    unreadsX -= rightMargin;
                    unreadsX -= Utils::unscale_bitmap(pix.width());

                    const int y_offset = (getUnreadBubbleHeight() - Utils::unscale_bitmap(pix.height())) / 2;

                    _p.drawPixmap(_rect.left() + unreadsX, _rect.top() + unreadsY + y_offset, pix);
                }
            }
            else if (!heads_.empty())
            {
                _p.setRenderHint(QPainter::HighQualityAntialiasing);

                unreadsX -= avatarRightMargin;

                for (const auto& _head : boost::adaptors::reverse(heads_))
                {
                    bool isDefault = false;
                    auto headAvatar = Logic::GetAvatarStorage()->GetRounded(_head.first, (_head.second.isEmpty() ? _head.first : _head.second),
                        Utils::scale_bitmap(contactListParams.getLastReadAvatarSize()), QString(), isDefault, false, false);

                    const auto opacityOld = _p.opacity();

                    _p.setPen(Qt::NoPen);
                    _p.setBrush(getHeadsBorderColor(_isSelected, _isHovered));

                    const int whiteAreaD = Utils::scale_value(16);

                    unreadsX -= whiteAreaD;

                    int y_offset = (getUnreadBubbleHeight() - Utils::unscale_bitmap(headAvatar->height())) / 2;

                    const int border = (whiteAreaD - Utils::unscale_bitmap(headAvatar->height())) / 2;
                    const int overlap = Utils::scale_value(4);

                    y_offset -= border;

                    _p.drawEllipse(_rect.left() + unreadsX, _rect.top() + unreadsY + y_offset, whiteAreaD, whiteAreaD);

                    if (!_isSelected)
                        _p.setOpacity(0.7);

                    _p.drawPixmap(_rect.left() + unreadsX + border, _rect.top() + unreadsY + y_offset + border, *headAvatar);

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

            static QPixmap badgeNormal = getPin(false, getPinSize());
            static QPixmap badgeSelected = getPin(true, getPinSize());
            const QPixmap& badge = _isSelected ? badgeSelected : badgeNormal;

            right_area_right -= Utils::unscale_bitmap(badge.width());

            _p.drawPixmap(right_area_right, badge_y + Utils::scale_value(1), badge);

            right_area_right -= contactListParams.official_hor_padding();
        }

        //////////////////////////////////////////////////////////////////////////
        // draw time
        if (!_viewParams.pictOnly_ && !compactMode_ && !isUnknown)
        {
            const int time_width = time_->cachedSize().width();

            time_->setColor(Styling::getParameters().getColor( _isSelected ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::BASE_PRIMARY));
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

            const QColor contactNameColor = getContactNameColor(_isSelected);

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
            const QPixmap& mediaPix = getMediaTypePix(mediaType_, _isSelected);

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
                messageShortName_->setColor(getNameColor(_isSelected));

                const int textOffset = Utils::scale_value(getMediaIconSize().width()) + Utils::scale_value(4);

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
                        textOffset = Utils::scale_value(getMediaIconSize().width()) + Utils::scale_value(4);
                    }

                    messageX += textOffset;

                    messageControl->setOffsets(messageX, messageY + Utils::text_sy());

                    messageMaxWidth -= textOffset;

                    const bool widthMessageChanged = (messageMaxWidth != prevWidthMessage_);

                    if (widthMessageChanged)
                    {
                        messageControl->getHeight(messageMaxWidth);
                    }

                    messageControl->setColor(getNameColor(_isSelected));
                    if (!_isSelected)
                        messageControl->setColorForAppended(getMessageColor(false));

                    messageControl->draw(_p);
                }
            }
        }

        prevWidthName_ = nameMaxWidth;
        prevWidthMessage_ = messageMaxWidth;

        if (_isDrag)
            renderRecentsDragOverlay(_p, _rect, _viewParams);
    }




    //////////////////////////////////////////////////////////////////////////
    // MessageAlertItem
    //////////////////////////////////////////////////////////////////////////
    MessageAlertItem::MessageAlertItem(const Data::DlgState& _state, const bool _compactMode)
        : RecentItemRecent(
            _state,
            _compactMode,
            get_gui_settings()->get_value<bool>(settings_hide_message_notification, false) || LocalPIN::instance()->locked(), false)
    {
        mention_ = _state.mentionAlert_;
    }

    MessageAlertItem::~MessageAlertItem()
    {
    }

    void MessageAlertItem::draw(
        QPainter& _p,
        const QRect& _rect,
        const Ui::ViewParams& _viewParams,
        const bool _isSelected,
        const bool _isHovered,
        const bool _isDrag,
        const bool _isKeyboardFocused)
    {
        Utils::PainterSaver ps(_p);

        _p.setPen(Qt::NoPen);

        drawMouseState(_p, _rect, _isHovered, _isSelected);

        const auto width = _rect.width();//CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);

        auto state = multichat_ ? QString() : Logic::getContactListModel()->getState(getAimid());
        if (_isSelected && (state == ql1s("online") || state == ql1s("mobile")))
            state += ql1s("_active");

        bool isDefaultAvatar = false;

        const auto& contactListParams = compactMode_ ? Ui::GetContactListParams() : Ui::GetRecentsParams();

        //////////////////////////////////////////////////////////////////////////
        // render avatar
        auto avatar = Logic::GetAvatarStorage()->GetRounded(getAimid(), displayName_, Utils::scale_bitmap(getAlertAvatarSize()),
            state, isDefaultAvatar, false, compactMode_);

        const auto ratio = Utils::scale_bitmap_ratio();
        const auto avatarX = _rect.left() + getAlertAvatarX();
        const auto avatarY = _rect.top() + (_rect.height() - avatar->height() / ratio) / 2;

        Utils::drawAvatarWithBadge(_p, QPoint(avatarX, avatarY), *avatar, !multichat_ && official_, !mention_ && !multichat_ && muted_, _isSelected);

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
        {
            name_->elide(nameMaxWidth);
        }

        name_->draw(_p);


        //////////////////////////////////////////////////////////////////////////
        // draw mediatype
        if (mediaType_ != MediaType::noMedia)
        {
            const QPixmap& mediaPix = getMediaTypePix(mediaType_, _isSelected);

            const int pixY = _rect.top() + getMediaTypeIconTopMarginAlert(multichat_ && !readOnlyChat_);

            _p.drawPixmap(contactNameX, pixY, mediaPix);
        }


        //////////////////////////////////////////////////////////////////////////
        // draw message
        if (mediaType_ != MediaType::noMedia && multichat_ && messageLongName_ && messageShortName_)
        {
            const auto messageY = _rect.top() + getAlertMessageY();

            const int iconOffset = Utils::scale_value(getMediaIconSize().width()) + Utils::scale_value(4);

            const bool widthMessageChanged = (messageMaxWidth != prevWidthMessage_);
            if (widthMessageChanged)
            {
                messageShortName_->elide(messageMaxWidth);
                messageLongName_->elide(messageMaxWidth - iconOffset);
            }

            messageShortName_->setOffsets(contactNameX, messageY + Utils::text_sy());
            messageShortName_->setColor(getNameColor(_isSelected));
            messageShortName_->draw(_p);

            contactNameX += iconOffset;

            messageLongName_->setOffsets(contactNameX, messageY + Utils::text_sy() + get2LineMessageOffset());
            messageLongName_->setColor(getMessageColor(_isSelected));
            messageLongName_->draw(_p);
        }
        else
        {
            const auto& messageControl = (messageNameWidth_ > messageMaxWidth) ? messageLongName_ : messageShortName_;
            assert(messageControl);
            if (messageControl)
            {
                if (mediaType_ != MediaType::noMedia)
                    contactNameX += Utils::scale_value(getMediaIconSize().width()) + Utils::scale_value(4);

                const auto messageY = _rect.top() + getAlertMessageY();

                messageControl->setOffsets(contactNameX, messageY + Utils::text_sy());

                const bool widthMessageChanged = (messageMaxWidth != prevWidthMessage_);

                if (widthMessageChanged)
                {
                    messageControl->getHeight(messageMaxWidth);
                }

                messageControl->setColor(getNameColor(_isSelected));
                if (!_isSelected)
                    messageControl->setColorForAppended(getMessageColor(false));

                messageControl->draw(_p);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // draw mention
        if (mention_)
        {
            static QPixmap mentionIcon = getMentionIcon(_isSelected, QSize(getMentionAvatarIconSize(), getMentionAvatarIconSize()));

            const auto iconX = _rect.left() + getAvatarIconMargin();
            const auto iconY = _rect.top() + getAvatarIconMargin();

            static QBrush hoverBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
            static QBrush normalBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
            _p.setPen(Qt::NoPen);
            _p.setBrush(_isHovered ? hoverBrush : normalBrush);
            _p.setRenderHint(QPainter::HighQualityAntialiasing);

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
                mentionIcon);
        }

        prevWidthName_ = nameMaxWidth;
        prevWidthMessage_ = messageMaxWidth;

        // draw underline
        static QColor underlineColor = getAlertUnderlineColor();

        QPen underlinePen(underlineColor);
        underlinePen.setWidth(Utils::scale_value(1));

        _p.setPen(underlineColor);

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
        name_->init(Fonts::appFont(contactListParams.contactNameFontSize(), Fonts::FontWeight::Normal), Qt::black);
        name_->evaluateDesiredSize();
        nameWidth_ = name_->desiredWidth();


        //////////////////////////////////////////////////////////////////////////
        // init message
        const int fontSize = contactListParams.messageFontSize();

        // control for short contact name
        QString sender = qsl("(") + _state.senderAimId_ + qsl(")");

        messageLongName_ = TextRendering::MakeTextUnit(sender + QChar::LineFeed, Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS,
            Ui::TextRendering::ProcessLineFeeds::KEEP_LINE_FEEDS);
        messageLongName_->init(Fonts::appFont(fontSize, Fonts::FontWeight::Normal), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1, Ui::TextRendering::LineBreakType::PREFER_SPACES);
        messageLongName_->setLineSpacing(getMessageLineSpacing());

        auto secondPath2 = TextRendering::MakeTextUnit(_state.GetText(), Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS, Ui::TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        secondPath2->init(Fonts::appFont(fontSize, Fonts::FontWeight::Normal), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), QColor(), QColor(), QColor(), TextRendering::HorAligment::LEFT, 1, Ui::TextRendering::LineBreakType::PREFER_SPACES);
        secondPath2->markdown(Fonts::appFont(fontSize, Fonts::FontWeight::Normal), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID), Ui::TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);

        messageLongName_->appendBlocks(std::move(secondPath2));
        messageLongName_->evaluateDesiredSize();
    }

    MailAlertItem::~MailAlertItem()
    {
    }

    void MailAlertItem::draw(
        QPainter& _p,
        const QRect& _rect,
        const Ui::ViewParams& _viewParams,
        const bool _isSelected,
        const bool _isHovered,
        const bool _isDrag,
        const bool _isKeyboardFocused)
    {
        const Utils::PainterSaver saver(_p);

        _p.setPen(Qt::NoPen);

        drawMouseState(_p, _rect, _isHovered, _isSelected);

        const auto width = _rect.width(); //CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);

        bool isDefaultAvatar = false;

        const auto& contactListParams = Ui::GetRecentsParams();

        //////////////////////////////////////////////////////////////////////////
        // render avatar
        auto avatar = Logic::GetAvatarStorage()->GetRounded(fromMail_, fromNick_, Utils::scale_bitmap(getAlertAvatarSize()),
            QString(), isDefaultAvatar, false, false);

        const auto ratio = Utils::scale_bitmap_ratio();
        const auto avatarX = _rect.left() + getAlertAvatarX();
        const auto avatarY = _rect.top() + (_rect.height() - avatar->height() / ratio) / 2;

        Utils::drawAvatarWithBadge(_p, QPoint(avatarX, avatarY), *avatar, fromMail_);

        static QPixmap mailIcon = getMailIcon(Utils::scale_bitmap(getMailAvatarIconSize()), _isSelected);

        const auto iconX = _rect.left() + getAvatarIconMargin();
        const auto iconY = _rect.top() + getAvatarIconMargin();

        static QBrush hoverBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));
        static QBrush normalBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        _p.setPen(Qt::NoPen);
        _p.setBrush(_isHovered ? hoverBrush : normalBrush);
        _p.setRenderHint(QPainter::HighQualityAntialiasing);

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

            messageControl->setColor(getNameColor(_isSelected));
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
        name_->init(Fonts::appFont(contactListParams.contactNameFontSize(), Fonts::FontWeight::Normal), Qt::black);
        name_->evaluateDesiredSize();
        nameWidth_ = name_->desiredWidth();
    }

    ComplexMailAlertItem::~ComplexMailAlertItem()
    {
    }

    void ComplexMailAlertItem::draw(
        QPainter& _p,
        const QRect& _rect,
        const Ui::ViewParams& _viewParams,
        const bool _isSelected,
        const bool _isHovered,
        const bool _isDrag,
        const bool _isKeyboardFocused)
    {
        static QPixmap mailIcon = getMailIcon(Utils::scale_bitmap(getMailIconSize()), _isSelected);
        assert(!mailIcon.isNull());

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

    RecentItemDelegate::~RecentItemDelegate()
    {
    }

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
            if (recentsModel->isRecentsHeader(_i) && !recentsModel->getFavoritesCount())
                return QSize();

            height = recentParams.serviceItemHeight();
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

    void RecentItemDelegate::addTyping(const Logic::TypingFires& _typing)
    {
        auto iter = std::find(typings_.begin(), typings_.end(), _typing);
        if (iter == typings_.end())
        {
            typings_.push_back(_typing);
        }
    }

    void RecentItemDelegate::removeTyping(const Logic::TypingFires& _typing)
    {
        auto iter = std::find(typings_.begin(), typings_.end(), _typing);
        if (iter != typings_.end())
        {
            typings_.erase(iter);
        }
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

    void RecentItemDelegate::onContactSelected(const QString& _aimId, qint64 _msgId, qint64)
    {
        selectedAimId_ = _aimId;
    }

    void RecentItemDelegate::onItemClicked(const QString & _aimId)
    {
        selectedAimId_ = _aimId;
    }

    std::unique_ptr<RecentItemBase> RecentItemDelegate::createItem(const Data::DlgState& _state)
    {
        if (_state.AimId_.startsWith(ql1c('~')) && _state.AimId_.endsWith(ql1c('~')))
        {
            if (_state.AimId_ == ql1s("~unknowns~"))
                return std::make_unique<RecentItemUnknowns>(_state);

            return std::make_unique<RecentItemService>(_state);
        }

        const bool compactMode = !Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, true);
        return std::make_unique<RecentItemRecent>(_state, compactMode, false, shouldDisplayHeads(_state));
    }

    bool RecentItemDelegate::shouldDisplayHeads(const Data::DlgState &_state)
    {
        if (!Ui::get_gui_settings()->get_value<bool>(settings_show_groupchat_heads, true))
            return false;

        if (_state.mediaType_ == Ui::MediaType::mediaTypeVoip)
            return _state.Outgoing_;

        return true;
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
        assert(unreadDigitsNumber >= 0);
        assert(unreadDigitsNumber <= 2);
    }

    bool RecentItemDelegate::ItemKey::operator < (const ItemKey &_key) const
    {
        if (IsSelected != _key.IsSelected)
        {
            return (IsSelected < _key.IsSelected);
        }

        if (IsHovered != _key.IsHovered)
        {
            return (IsHovered < _key.IsHovered);
        }

        if (UnreadDigitsNumber != _key.UnreadDigitsNumber)
        {
            return (UnreadDigitsNumber < _key.UnreadDigitsNumber);
        }

        return false;
    }
}
