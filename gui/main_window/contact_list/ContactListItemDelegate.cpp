#include "stdafx.h"

#include "../MainPage.h"
#include "ContactListItemDelegate.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../containers/FriendlyContainer.h"
#include "../containers/LastseenContainer.h"
#include "../proxy/AvatarStorageProxy.h"
#include "../proxy/FriendlyContaInerProxy.h"
#include "../../controls/TextUnit.h"
#include "../../controls/CheckboxList.h"

#include "SearchModel.h"
#include "../../types/contact.h"
#include "../../types/chat.h"
#include "../../utils/utils.h"
#include "../../utils/translator.h"
#include "../../my_info.h"
#include "../../app_config.h"
#include "../../fonts.h"
#include "ContactListModel.h"
#include "CustomAbstractListModel.h"
#include "ChatMembersModel.h"
#include "RecentsModel.h"
#include "RecentsTab.h"
#include "CommonChatsModel.h"
#include "SearchHighlight.h"
#include "FavoritesUtils.h"
#ifndef STRIP_VOIP
#include "voip/SelectionContactsForConference.h"
#endif

#include "styles/ThemeParameters.h"
#include "IconsManager.h"

namespace
{
    auto nickPadding() noexcept
    {
        return Utils::scale_value(4);
    }

    constexpr auto moreIconAngle() noexcept
    {
        return 90;
    }

    constexpr auto getCLItemOpacity() noexcept
    {
        return 0.2;
    }

    auto roleRightOffset() noexcept
    {
        return Utils::scale_value(14);
    }

    QPixmap makeIcon(const QString& _path, QSize _size, Styling::StyleVariable _var)
    {
        return Utils::renderSvgScaled(_path, _size, Styling::getParameters().getColor(_var));
    }

    auto contactNameOffset() noexcept
    {
        return platform::is_apple() ? Utils::scale_value(6) : Utils::scale_value(2);
    }

    constexpr auto removeIconSize() noexcept
    {
        return QSize(18, 18);
    }

    constexpr auto moreBtnSize() noexcept
    {
        return QSize(20, 20);
    }

    Fonts::FontWeight getNameWeight(bool _forMembersView) noexcept
    {
        const auto& contactList = Ui::GetContactListParams();
        return _forMembersView ? contactList.membersNameFontWeight() : contactList.contactNameFontWeight();
    }
}

namespace Ui
{
    int Item::drawMore(QPainter &_painter, Ui::ContactListParams& _contactList, const Ui::ViewParams& _viewParams)
    {
        const auto width = _viewParams.fixedWidth_;
        auto img = []()
        {
            QTransform trans;
            trans.rotate(moreIconAngle());
            auto result = makeIcon(qsl(":/controls/more_icon"), moreBtnSize(), Styling::StyleVariable::BASE_SECONDARY).transformed(trans);
            Utils::check_pixel_ratio(result);
            return result;
        };

        static auto optionsImg = img();
        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash())
            optionsImg = img();

        Utils::PainterSaver p(_painter);

        _painter.setRenderHint(QPainter::Antialiasing);
        _painter.setRenderHint(QPainter::SmoothPixmapTransform);

        double ratio = Utils::scale_bitmap_ratio();
        _painter.drawPixmap(
            Ui::CorrectItemWidth(Ui::ItemWidth(false, false), width)
            - _contactList.moreRightPadding() - (optionsImg.width() / ratio),
            _contactList.itemHeight() / 2 - (optionsImg.height() / 2. / ratio),
            optionsImg
        );

        const auto xPos =
            Ui::CorrectItemWidth(ItemWidth(_viewParams), width)
            - _contactList.moreRightPadding()
            - optionsImg.width();
        return xPos;
    }

    int Item::drawAddContact(QPainter &_painter, const int _rightMargin, const bool _isSelected, ContactListParams& _recentParams)
    {
        Utils::PainterSaver p(_painter);
        static QPixmap add, addSelected;

        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash() || add.isNull())
        {
            add = makeIcon(qsl(":/controls/add_icon"), QSize(20, 20), Styling::StyleVariable::PRIMARY);
            addSelected = makeIcon(qsl(":/controls/add_icon"), QSize(20, 20), Styling::StyleVariable::BASE_GLOBALWHITE);
        }

        const auto& img = _isSelected ? addSelected : add;

        double ratio = Utils::scale_bitmap_ratio();
        _recentParams.addContactFrame().setX(_rightMargin - (img.width() / ratio));
        _recentParams.addContactFrame().setY((_recentParams.itemHeight() / 2) - (img.height() / ratio / 2.));
        _recentParams.addContactFrame().setWidth(img.width() / ratio);
        _recentParams.addContactFrame().setHeight(img.height() / ratio);

        _painter.setRenderHint(QPainter::Antialiasing);
        _painter.setRenderHint(QPainter::SmoothPixmapTransform);
        _painter.drawPixmap(_recentParams.addContactFrame().x(), _recentParams.addContactFrame().y(), img);

        return _recentParams.addContactFrame().x();
    }

    QPixmap Item::getCheckBoxImage(const VisualDataBase& _visData)
    {
        using activity = CheckBox::Activity;
        const auto state = _visData.isChatMember_ ? activity::DISABLED : (_visData.isCheckedBox_ ? activity::ACTIVE : activity::NORMAL);
        return Ui::CheckBox::getCheckBoxIcon(state).actualPixmap();
    }

    double Item::getCheckBoxWidth(const VisualDataBase& _visData, const QPixmap* _img)
    {
        auto img = _img ? (*_img) : getCheckBoxImage(_visData);
        return img.height() / 2. / Utils::scale_bitmap_ratio();
    }

    void Item::fixMaxWidthIfCheckable(int& _maxWidth, const VisualDataBase& _visData, const Ui::ViewParams& _params) const
    {
        const auto regim = _params.regim_;
        if (IsSelectMembers(regim) || Logic::is_share_regims(regim))
            _maxWidth -= getCheckBoxWidth(_visData);
    }

    void Item::drawCheckbox(QPainter& _painter, const VisualDataBase& _visData, const ContactListParams& _contactList)
    {
        const auto img = getCheckBoxImage(_visData);
        _painter.drawPixmap(_contactList.itemWidth(), _contactList.getItemMiddleY() - getCheckBoxWidth(_visData, &img), img);
    }

    int Item::drawRemove(QPainter &_painter, bool _isSelected, ContactListParams& _contactList, const ViewParams& _viewParams)
    {
        static QPixmap rem, remSelected;
        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash() || rem.isNull())
        {
            rem = makeIcon(qsl(":/controls/close_icon"), removeIconSize(), Styling::StyleVariable::BASE_SECONDARY);
            remSelected = makeIcon(qsl(":/controls/close_icon"), removeIconSize(), Styling::StyleVariable::BASE_SECONDARY_ACTIVE);
        }

        const auto& remove_img = _isSelected ? remSelected : rem;

        const auto width = CorrectItemWidth(ItemWidth(false, false), _viewParams.fixedWidth_);
        static const auto buttonWidth = Utils::scale_value(32);

        auto& removeFrame = _contactList.removeContactFrame();
        removeFrame.setX(width - buttonWidth - Utils::scale_value(8));
        removeFrame.setY(0);
        removeFrame.setWidth(buttonWidth);
        removeFrame.setHeight(_contactList.itemHeight());

        const double ratio = Utils::scale_bitmap_ratio();
        const auto x = removeFrame.x() + ((buttonWidth - remove_img.width() / ratio) / 2);
        const auto y = (_contactList.itemHeight() - remove_img.height() / ratio) / 2.;

        Utils::PainterSaver p(_painter);
        _painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        _painter.drawPixmap(x, y, remove_img);

        return removeFrame.x();
    }

    int Item::drawUnreads(QPainter & _painter, const int _unreads, const bool _isMuted, const bool _isSelected, const bool _isHovered, const ViewParams & _viewParams, const ContactListParams & _clParams, const bool _isUnknownHeader)
    {
        auto width = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);

        if (_unreads <= 0)
            return width;

        static QFontMetrics m(_clParams.unreadsFont());

        const auto text = Utils::getUnreadsBadgeStr(_unreads);
        const auto unreadsRect = m.tightBoundingRect(text);
        const auto firstChar = text[0];
        const auto lastChar = text[text.size() - 1];
        const auto unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));
        const auto unreadsSize = Utils::scale_value(20);

        auto balloonWidth = unreadsWidth;
        if (text.length() > 1)
        {
            balloonWidth += (_clParams.unreadsPadding() * 2);
        }
        else
        {
            balloonWidth = unreadsSize;
        }

        auto unreadsX = width - _clParams.itemHorPadding() - balloonWidth;

        auto unreadsY = _isUnknownHeader ? _clParams.unknownsUnreadsY(_viewParams.pictOnly_) : (_clParams.itemHeight() - unreadsSize) / 2;

        if (_viewParams.pictOnly_)
        {
            unreadsX = _clParams.itemHorPadding();
            if (!_isUnknownHeader)
                unreadsY = (_clParams.itemHeight() - _clParams.getAvatarSize()) / 2;
        }
        else if (_viewParams.regim_ == Logic::MembersWidgetRegim::UNKNOWN)
        {
            unreadsX -= _clParams.removeSize().width();
        }

        auto borderColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
        if (_isHovered)
        {
            borderColor = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
        }
        else if (_isSelected)
        {
            borderColor = (_viewParams.pictOnly_) ? Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_SELECTED) : QColor();
        }

        const auto bgColor = Styling::getParameters().getColor( _isSelected ? Styling::StyleVariable::TEXT_SOLID :
                            (_isMuted ? Styling::StyleVariable::BASE_TERTIARY
                            : Styling::StyleVariable::PRIMARY));

        const auto textColor = Styling::getParameters().getColor( _isSelected ? Styling::StyleVariable::PRIMARY_SELECTED : Styling::StyleVariable::BASE_GLOBALWHITE);
        Utils::drawUnreads(&_painter, _clParams.unreadsFont(), bgColor, textColor, borderColor, _unreads, unreadsSize, unreadsX, unreadsY);

        return unreadsX;
    }

    ServiceContact::ServiceContact(const QString& _name, Data::ContactType _type)
        : name_ { TextRendering::MakeTextUnit(_name) }
        , type_(_type)
    {
        const auto& contactList = GetContactListParams();

        Styling::StyleVariable style = Styling::StyleVariable::INVALID;
        QFont font;
        if (_type == Data::SERVICE_HEADER)
        {
            style = Styling::StyleVariable::BASE_PRIMARY;
            font = contactList.getContactListHeadersFont();
        }
        else if (_type == Data::EMPTY_LIST)
        {
            style = Styling::StyleVariable::TEXT_SOLID;
            font = contactList.emptyIgnoreListFont();
        }

        name_->init({ font, Styling::ThemeColorKey{ style } });
        name_->evaluateDesiredSize();
    }

    ServiceContact::~ServiceContact() = default;

    void ServiceContact::paint(QPainter& _painter, const bool _isHovered, const bool _isSelected, const bool /*_isPressed*/, const int _curWidth)
    {
        Utils::PainterSaver p(_painter);

        const auto& contactList = GetContactListParams();
        auto x = contactList.itemHorPadding();
        auto height = contactList.serviceItemHeight();
        switch (type_)
        {
        case Data::EMPTY_LIST:
            height = contactList.itemHeight();
            x = (_curWidth - name_->cachedSize().width()) / 2;
            break;
        case Data::SERVICE_HEADER:
            height = contactList.serviceItemHeight();
            break;
        default:
            break;
        }

        RenderMouseState(_painter, _isHovered, _isSelected, QRect(0, 0, _curWidth, height));

        name_->setOffsets(x, height / 2);
        name_->draw(_painter, TextRendering::VerPosition::MIDDLE);
    }

    ContactItem::ContactItem(bool _membersView, Logic::DrawIcons _needDrawIcons)
        : needDrawIcons_(_needDrawIcons)
    {
        membersView_ = _membersView;

        const auto secondaryFontSize = platform::is_apple() ? 14 : 13;
        contactStatus_ = TextRendering::MakeTextUnit(QString());
        TextRendering::TextUnit::InitializeParameters params(Fonts::appFontScaled(secondaryFontSize, Fonts::FontWeight::Normal), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY });
        contactStatus_->init(params);

        contactRole_ = TextRendering::MakeTextUnit(QString());
        params.setFonts(Fonts::appFontScaled(secondaryFontSize));
        contactRole_->init(params);

        contactName_ = TextRendering::MakeTextUnit(QString(), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        params.setFonts(Fonts::appFont(GetContactListParams().contactNameFontSize(), getNameWeight(membersView_)));
        params.color_ = Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT };
        params.align_ = TextRendering::HorAligment::LEFT;
        params.maxLinesCount_ = 1;
        contactName_->init(params);
    }

    ContactItem::~ContactItem() = default;

    void ContactItem::updateParams(const VisualDataBase& _item, const ViewParams& _viewParams)
    {
        const auto& contactList = GetContactListParams();
        const auto isFavorites = _viewParams.replaceFavorites_ && Favorites::isFavorites(_item.AimId_);

        const auto getContactNameColor = [&_viewParams, &_item, &contactList, isFavorites](bool _canCheck = true)
        {
            const auto isMemberSelected = IsSelectMembers(_viewParams.regim_) && (_item.isChatMember_ || _item.isCheckedBox_);
            const auto isChecked = _canCheck && isMemberSelected;
            return contactList.getNameFontColor(_item.IsSelected_, isChecked, isFavorites);
        };

        if (_item.ContactName_ != item_.ContactName_ || _item.highlights_ != item_.highlights_ || _item.isCheckedBox_ != item_.isCheckedBox_)
        {
            contactName_ = createHightlightedText(
                _item.ContactName_,
                Fonts::appFont(contactList.contactNameFontSize(), getNameWeight(membersView_)),
                getContactNameColor(!Logic::is_select_chat_members_regim(_viewParams.regim_) && contactList.isCL()),
                Ui::getTextHighlightedColor(),
                1,
                _item.highlights_);
            contactName_->evaluateDesiredSize();
        }
        else if (_viewParams.fixedWidth_ != params_.fixedWidth_)
        {
            contactName_->evaluateDesiredSize();
        }

        if (!isFavorites && !_item.nick_.isEmpty() && !_item.highlights_.empty())
        {
            if (findNextHighlight(_item.nick_, _item.highlights_).indexStart_ != -1)
            {
                contactNick_ = createHightlightedText(
                    Utils::makeNick(_item.nick_),
                    Fonts::appFont(contactList.contactNameFontSize(), Fonts::FontWeight::Normal),
                    Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY },
                    Ui::getTextHighlightedColor(),
                    1,
                    _item.highlights_);

                contactNick_->evaluateDesiredSize();
            }
            else if (contactNick_)
            {
                contactNick_.reset();
            }
        }
        else if (contactNick_)
        {
            contactNick_.reset();
        }

        if (_item.role_ != item_.role_)
        {
            if (_item.isCreator_)
                contactRole_->setText(QT_TRANSLATE_NOOP("contactlist", "Creator"));
            else if (_item.role_ == u"admin" || _item.role_ == u"moder")
                contactRole_->setText(QT_TRANSLATE_NOOP("contactlist", "Admin"));
            else if (_item.role_ == u"readonly")
                contactRole_->setText(QT_TRANSLATE_NOOP("contactlist", "Read only"));
            else
                contactRole_->setText(QString());
        }

        if (_item.IsSelected_ != item_.IsSelected_)
            contactName_->setColor(getContactNameColor());

        if (_item.IsSelected_ != item_.IsSelected_ || _item.GetStatus() != item_.GetStatus())
        {
            const auto onlineColor = _item.IsSelected_ ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::TEXT_PRIMARY;
            const auto seenColor = _item.IsSelected_ ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::BASE_PRIMARY;
            contactStatus_->setText(_item.GetStatus(), Styling::ThemeColorKey{ _item.GetStatus() == Data::LastSeen::online().getStatusString() ? onlineColor : seenColor });
        }

        if (_viewParams.regim_ == Logic::MembersWidgetRegim::COMMON_CHATS)
        {
            const auto membersStr = Utils::getMembersString(_item.membersCount_, Logic::getContactListModel()->isChannel(_item.AimId_));
            contactStatus_->setText(membersStr, Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY });
        }

        item_ = _item;
        params_ = _viewParams;
    }

    bool ContactItem::needDrawUnreads(const Data::DlgState& _state) const
    {
        return needDrawIcons_ && _state.UnreadCount_ > 0;
    }

    bool ContactItem::needDrawMentions(const Data::DlgState& _state) const
    {
        return needDrawIcons_ && _state.unreadMentionsCount_ > 0 && !isUnknown();
    }

    bool ContactItem::needDrawAttention(const Data::DlgState& _state) const
    {
        return needDrawIcons_ && !needDrawUnreads(_state)  && _state.Attention_;
    }

    bool ContactItem::needDrawDraft(const Data::DlgState& _state) const
    {
        return needDrawIcons_ && _state.hasDraft();
    }

    bool ContactItem::isUnknown() const
    {
        return params_.regim_ == ::Logic::MembersWidgetRegim::UNKNOWN;
    }

    void ContactItem::fixMaxWidthIfIcons(int& _maxWidth) const
    {
        if (!needDrawIcons_)
            return;
        Data::DlgState state = Logic::getRecentsModel()->getDlgState(item_.AimId_);
        auto contactListParams = Ui::GetContactListParams();
        using im = Logic::IconsManager;

        if (needDrawUnreads(state))
        {
            _maxWidth -= im::getUnreadBubbleRightMarginPictOnly();
            _maxWidth -= im::getUnreadBalloonWidth(state.UnreadCount_);
        }

        if (needDrawAttention(state))
        {
            _maxWidth -= im::getUnreadBubbleRightMarginPictOnly();
            _maxWidth -= im::getAttentionSize().width();
        }

        if (needDrawMentions(state))
        {
            _maxWidth -= im::getUnreadBubbleRightMarginPictOnly();
            _maxWidth -= im::getMentionSize().width();
        }

        if (needDrawDraft(state))
            _maxWidth -= (im::getDraftSize().width() + contactListParams.official_hor_padding());

        if (contactListParams.isCL())
            _maxWidth -= contactListParams.itemContentPadding();

        if (!item_.role_.isEmpty())
            _maxWidth -= (contactRole_->cachedSize().width() + Utils::scale_value(8));

    }

    void ContactItem::paint(QPainter& _painter, const bool, const bool, const bool, const int _curWidth)
    {
        auto contactList = GetContactListParams();

        Utils::PainterSaver p(_painter);

        _painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

        auto renderContactName = [this, &_painter, contactList](int _rightMargin, int _y, bool _center, int _role_right_offset, bool drawStatus)
        {
            int maxWidth = _rightMargin - contactList.getContactNameX(params_.leftMargin_);
            fixMaxWidthIfCheckable(maxWidth, item_, params_);
            fixMaxWidthIfIcons(maxWidth);

            const auto contactRoleOffset = _role_right_offset + Utils::scale_value(4) + contactList.itemHorPadding();

            if (!contactRole_->isEmpty())
                maxWidth -= contactRole_->desiredWidth() + contactRoleOffset;
            else if (Logic::is_admin_members_regim(params_.regim_))
                maxWidth -= contactList.moreRightPadding() + (moreBtnSize().width() / Utils::scale_bitmap_ratio());

            const auto nameX = contactList.getContactNameX(params_.leftMargin_);

            int textWidth = contactName_->sourceTextWidth();
            int nickWidth = contactNick_ ? contactNick_->sourceTextWidth() + nickPadding() : 0;

            if (textWidth + nickWidth > maxWidth && contactNick_ && !contactNick_->isEmpty())
            {
                if (nickWidth <= maxWidth / 2)
                {
                    contactName_->elide(maxWidth - nickWidth);
                }
                else
                {
                    contactName_->elide(maxWidth / 2);
                    contactNick_->elide(maxWidth - contactName_->textWidth() - nickPadding());
                }
            }
            else
                contactName_->elide(maxWidth);

            contactName_->setOffsets(nameX, _y);
            contactName_->draw(_painter, _center ? TextRendering::VerPosition::MIDDLE : TextRendering::VerPosition::TOP);

            if (contactNick_)
            {
                contactNick_->setOffsets(nameX + contactName_->textWidth() + nickPadding(), _y);
                contactNick_->draw(_painter, _center ? TextRendering::VerPosition::MIDDLE : TextRendering::VerPosition::TOP);
            }

            if (drawStatus)
            {
                contactStatus_->elide(maxWidth);
                contactStatus_->setOffsets(contactList.getContactNameX(params_.leftMargin_), _y + contactName_->cachedSize().height() + Utils::scale_value(8));
                contactStatus_->draw(_painter, TextRendering::VerPosition::MIDDLE);
            }

            contactRole_->setOffsets(_rightMargin - contactRole_->cachedSize().width() - contactRoleOffset, contactList.itemHeight() / 2);
            contactRole_->draw(_painter, TextRendering::VerPosition::MIDDLE);
        };

        const auto regim = params_.regim_;

        RenderMouseState(_painter, item_.IsHovered_, item_.IsSelected_, params_, contactList.itemHeight());

        if (IsSelectMembers(regim) || Logic::is_share_regims(regim))
            drawCheckbox(_painter, item_, contactList);

        const auto avX = params_.pictOnly_
            ? _curWidth / 2 - contactList.getAvatarSize() / 2
            : contactList.getAvatarX() + params_.leftMargin_;
        const auto avY = contactList.getAvatarY();

        const auto isFavorites = Favorites::isFavorites(item_.AimId_);
        const auto statusBadge = isFavorites && params_.replaceFavorites_ ? QPixmap() : Utils::getStatusBadge(item_.AimId_, item_.Avatar_.width());

        if (isFavorites)
        {
            Utils::drawAvatarWithoutBadge(_painter, QPoint(avX, avY), item_.Avatar_, statusBadge);
        }
        else
        {
            Utils::StatusBadgeFlags statusFlags { Utils::StatusBadgeFlag::SmallOnline | Utils::StatusBadgeFlag::Small };
            statusFlags.setFlag(Utils::StatusBadgeFlag::Official, item_.isOfficial_);
            statusFlags.setFlag(Utils::StatusBadgeFlag::Muted, item_.isMuted_);
            statusFlags.setFlag(Utils::StatusBadgeFlag::Selected, item_.IsSelected_);
            statusFlags.setFlag(Utils::StatusBadgeFlag::Online, item_.IsOnline_);
            Utils::drawAvatarWithBadge(_painter, QPoint(avX, avY), item_.Avatar_, statusBadge, statusFlags);
        }

        if (params_.pictOnly_)
            return;

        auto rightMargin = 0;

        auto role_right_offset = 0;

        const auto isSelectRegim = IsSelectMembers(regim) || Logic::is_share_regims(regim) || regim == Logic::MembersWidgetRegim::SHARE_CONTACT || regim == Logic::MembersWidgetRegim::TASK_ASSIGNEE;
        const auto isCurVideoConf = regim == Logic::MembersWidgetRegim::CURRENT_VIDEO_CONFERENCE;
        auto contactNameY = 0;
        if (regim == Logic::MembersWidgetRegim::COMMON_CHATS || (item_.HasStatus() && !isFavorites && !isCurVideoConf))
            contactNameY = contactNameOffset();
        else
            contactNameY = contactList.itemHeight() / 2;

        const auto center = isFavorites || (regim != Logic::MembersWidgetRegim::COMMON_CHATS && !item_.HasStatus()) || isCurVideoConf;
        const auto drawStatus = !isFavorites && (item_.HasStatus() || regim == Logic::MembersWidgetRegim::COMMON_CHATS) && !isCurVideoConf;

        if (regim == Logic::MembersWidgetRegim::PENDING_MEMBERS || regim == Logic::MembersWidgetRegim::YOUR_INVITES_LIST || (!isFavorites && item_.IsHovered_ && isCurVideoConf))
        {
            rightMargin = drawRemove(_painter, false, contactList, params_);
            if (regim == Logic::MembersWidgetRegim::PENDING_MEMBERS)
                rightMargin = drawAddContact(_painter, rightMargin, false, contactList) - contactList.itemHorPadding();
            renderContactName(rightMargin, contactNameY, center, role_right_offset, drawStatus);
            return;
        }
        else if (item_.IsHovered_)
        {
            role_right_offset = roleRightOffset();
            if (Logic::is_admin_members_regim(regim))
                rightMargin = drawMore(_painter, contactList, params_);
            else if (Logic::is_members_regim(regim) || (item_.canRemoveTill_.isValid() && QDateTime::currentDateTime() < item_.canRemoveTill_))
                rightMargin = drawRemove(_painter, false, contactList, params_);
            else
                role_right_offset = 0;
        }

        rightMargin = CorrectItemWidth(ItemWidth(params_), params_.fixedWidth_);
        rightMargin -= params_.rightMargin_;

        if (isSelectRegim)
            rightMargin -= Utils::scale_value(32);
        renderContactName(rightMargin, contactNameY, center, role_right_offset, drawStatus);
    }

    bool ContactItem::needsTooltip() const
    {
        return contactName_ && contactName_->isElided();
    }

    GroupItem::GroupItem(const QString& _name)
    {
        const auto& contactList = GetContactListParams();

        groupName_ = TextRendering::MakeTextUnit(_name);
        groupName_->init({ contactList.groupFont(), contactList.groupColor() });
        groupName_->evaluateDesiredSize();
    }

    GroupItem::~GroupItem() = default;

    void GroupItem::paint(QPainter& _painter, const bool _isHovered /* = false */, const bool _isActive /* = false */, const bool /*_isPressed*/, const int _curWidth /* = 0 */)
    {
        Utils::PainterSaver p(_painter);

        auto contactList = GetContactListParams();
        groupName_->setOffsets(contactList.getContactNameX(), contactList.groupY());
        groupName_->draw(_painter, TextRendering::VerPosition::MIDDLE);
    }

    ServiceButton::ServiceButton(const QString& _name, const Utils::StyledPixmap& _icon, const Utils::StyledPixmap& _hoverIcon, const Utils::StyledPixmap& _pressedIcon, int _height)
        : icon_(_icon)
        , hoverIcon_(_hoverIcon)
        , pressedIcon_(_pressedIcon)
        , height_(_height)
    {
        const auto& contactList = GetContactListParams();

        name_ = TextRendering::MakeTextUnit(_name);

        name_->init({ contactList.getContactNameFont(Fonts::FontWeight::Normal), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } });
        name_->evaluateDesiredSize();
    }

    ServiceButton::~ServiceButton() = default;

    void ServiceButton::paint(QPainter& _painter, const bool _isHovered, const bool _isActive /* = false */, const bool _isPressed, const int _curWidth /* = 0 */)
    {
        Utils::PainterSaver p(_painter);
        RenderMouseState(_painter, _isHovered, false, QRect(0, 0, _curWidth, height_));

        const auto& contactList = GetContactListParams();
        const auto icon = (_isPressed ? pressedIcon_ : (_isHovered ? hoverIcon_ : icon_)).actualPixmap();

        const double ratio = Utils::scale_bitmap_ratio();
        const auto x = contactList.getAvatarX() + ((contactList.getAvatarSize() - icon.width() / ratio) / 2);
        const auto y = (height_ - icon.height() / ratio) / 2;
        _painter.drawPixmap(x, y, icon);

        name_->setOffsets(contactList.getContactNameX(), height_ / 2);
        name_->draw(_painter, TextRendering::VerPosition::MIDDLE);
    }
}

namespace Logic
{
    ContactListItemDelegate::ContactListItemDelegate(QObject* parent, int _regim, CustomAbstractListModel* chatMembersModel, DrawIcons _needDrawIcons)
        : AbstractItemDelegateWithRegim(parent)
        , StateBlocked_(false)
        , renderRole_(false)
        , membersView_(false)
        , chatMembersModel_(chatMembersModel)
        , needDrawIcons_(_needDrawIcons)
    {
        viewParams_.regim_ = _regim;
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStateChanged, this, &ContactListItemDelegate::dlgStateChanged, Qt::QueuedConnection);
    }

    void ContactListItemDelegate::dlgStateChanged(const Data::DlgState& _dlgState)
    {
        if (model_)
        {
            model_->onDataChanged(_dlgState.AimId_);
        }
        else
        {
            contactItems_.erase(_dlgState.AimId_);
            if (auto w = qobject_cast<QWidget*>(parent()))
                w->update();
        }
    }

    ContactListItemDelegate::~ContactListItemDelegate() = default;

    void ContactListItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        auto contactType = Data::BASE;
        QString displayName, aimId;
        bool isChecked = false, isChatMember = false, isOfficial = false;
        int membersCount = 0;
        Data::LastSeen memberModelLastseen;
        QString iconId;
        Utils::StyledPixmap icon;
        Utils::StyledPixmap hoverIcon;
        Utils::StyledPixmap pressedIcon;
        Ui::highlightsV highlights;

        Data::Contact* contact_in_cl = nullptr;
        QString role;
        bool isCreator = false;
        QDateTime canRemoveTill;

        if (_index.data(Qt::DisplayRole).value<QWidget*>()) // If we use custom widget.
        {
            return;
        }
        else if (auto searchRes = _index.data(Qt::DisplayRole).value<Data::AbstractSearchResultSptr>())
        {
            aimId = searchRes->getAimId();
            highlights = searchRes->highlights_;

            if (qobject_cast<const Logic::ChatMembersModel*>(_index.model()))
            {
                const auto& memberRes = std::static_pointer_cast<Data::SearchResultChatMember>(searchRes);
                isCreator = memberRes->isCreator();

                if (renderRole_)
                    role = memberRes->getRole();

                if (auto contact_item = Logic::getContactListModel()->getContactItem(aimId))
                    contact_in_cl = contact_item->Get();

                memberModelLastseen = memberRes->getLastseen();
                canRemoveTill = memberRes->canRemoveTill();
            }
            else if (qobject_cast<const Logic::SearchModel*>(_index.model()))
            {
                if (auto contact_item = Logic::getContactListModel()->getContactItem(aimId))
                    contact_in_cl = contact_item->Get();
            }
            else if (qobject_cast<const Logic::CommonChatsSearchModel*>(_index.model()))
            {
                const auto& inf = std::static_pointer_cast<Data::SearchResultCommonChat>(searchRes);
                membersCount = inf->info_.membersCount_;
            }
#ifndef STRIP_VOIP
            else if (auto model = qobject_cast<const Ui::ConferenceSearchModel*>(_index.model()))
            {
                isChecked = model->isCheckedItem(aimId);
            }
#endif
        }
        else if (qobject_cast<const Logic::CommonChatsModel*>(_index.model()))
        {
            auto cont = _index.data(Qt::DisplayRole).value<Data::CommonChatInfo>();
            aimId = cont.aimid_;
            displayName = cont.friendly_;
            membersCount = cont.membersCount_;
        }
        else
        {
            if (contact_in_cl = _index.data(Qt::DisplayRole).value<Data::Contact*>())
                aimId = contact_in_cl->AimId_;
        }

        auto lastSeen = Logic::GetLastseenContainer()->getLastSeen(aimId);
        if (!lastSeen.isValid())
            lastSeen = memberModelLastseen;

        bool serviceContact = false;
        const auto isSelectChatMembersRegim = Logic::is_select_chat_members_regim(viewParams_.regim_);
        if (contact_in_cl)
        {
            contactType = contact_in_cl->GetType();
            serviceContact = (contactType == Data::SERVICE_HEADER || contactType == Data::DROPDOWN_BUTTON);

            isChecked = contact_in_cl->IsChecked_;
            isOfficial = contact_in_cl->IsOfficial_;
            iconId = contact_in_cl->iconId_;
            icon = contact_in_cl->iconPixmap_;
            hoverIcon = contact_in_cl->iconHoverPixmap_;
            pressedIcon = contact_in_cl->iconPressedPixmap_;
        }
        else
        {
            isOfficial = Logic::GetFriendlyContainer()->getOfficial(aimId);
        }

        bool isOnline = (serviceContact || lastSeen.isBot()) ? false : lastSeen.isOnline();

        if (isSelectChatMembersRegim && chatMembersModel_)
            isChecked = chatMembersModel_->isCheckedItem(aimId);

        if (displayName.isEmpty())
        {
            if (contact_in_cl && serviceContact)
                displayName = contact_in_cl->GetDisplayName();
            else
                displayName = Logic::getFriendlyContainerProxy(friendlyProxyFlags()).getFriendly(aimId);
        }

        if (!isSelectChatMembersRegim)
        {
            if (!!chatMembersModel_)
                isChatMember = chatMembersModel_->contains(aimId);

            isChatMember |= Logic::is_select_members_regim(viewParams_.regim_) && aimId == Ui::MyInfo()->aimId();
        }
        const auto isHeader = contactType == Data::SERVICE_HEADER;
        const bool isSelected = (viewParams_.regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP || viewParams_.regim_ == Logic::MembersWidgetRegim::CONTACT_LIST) && Logic::getContactListModel()->selectedContact() == aimId && !isHeader && !StateBlocked_;
        const bool isHovered = (_option.state & QStyle::State_Selected) && !isHeader && !StateBlocked_;
        const bool isMouseOver = (_option.state & QStyle::State_MouseOver);
        const bool isPressed = isHovered && isMouseOver && (QApplication::mouseButtons() & Qt::MouseButton::LeftButton);

        Utils::PainterSaver ps(*_painter);
        _painter->setRenderHint(QPainter::Antialiasing);
        _painter->translate(_option.rect.topLeft());
        dp_.setX(_option.rect.topLeft().x());
        dp_.setY(_option.rect.topLeft().y());

        if (isOpacityEnabled() && (!isSelectChatMembersRegim || (isSelectChatMembersRegim && !isChecked)))
            _painter->setOpacity(getCLItemOpacity());

        if (contactType != Data::BASE)
        {
            switch (contactType)
            {
            case Data::GROUP:
            {
                auto& item = items_[aimId];
                if (!item)
                    item = std::make_unique<Ui::GroupItem>(displayName);

                item->paint(*_painter);
                break;
            }
            case Data::SERVICE_HEADER:
            {
                auto& item = items_[aimId];
                if (!item)
                    item = std::make_unique<Ui::ServiceContact>(displayName, contactType);

                item->paint(*_painter);
                break;
            }
            case Data::DROPDOWN_BUTTON:
            {
                auto& item = items_[aimId];
                if (!item)
                    item = std::make_unique<Ui::ServiceButton>(displayName, icon, hoverIcon, pressedIcon, _option.rect.height());

                item->paint(*_painter, isHovered && isMouseOver, false, isPressed);
                break;
            }
            default:
                break;
            }
        }
        else
        {
            QString nick;
            if (!Logic::getContactListModel()->isChat(aimId))
                nick = Logic::GetFriendlyContainer()->getNick(aimId);

            auto isDefault = false;
            const auto &avatar = Logic::getAvatarStorageProxy(avatarProxyFlags()).GetRounded(
                aimId, displayName, Utils::scale_bitmap(Ui::GetContactListParams().getAvatarSize()),
                isDefault, false /* _regenerate */ ,
                Ui::GetContactListParams().isCL());

            Ui::VisualDataBase visData(
                aimId, avatar, lastSeen.getStatusString(), isHovered, isSelected, displayName, nick, highlights,
                lastSeen, isChecked, isChatMember, isOfficial, false /* draw last read */, QPixmap() /* last seen avatar*/,
                role, Logic::getRecentsModel()->getUnreadCount(aimId), Logic::getContactListModel()->isMuted(aimId), false, Logic::getRecentsModel()->getAttention(aimId),
                isCreator, isOnline);

            visData.membersCount_ = membersCount;
            visData.canRemoveTill_ = canRemoveTill;

            Ui::ViewParams viewParams(viewParams_.regim_, _option.rect.width(), viewParams_.leftMargin_, viewParams_.rightMargin_, viewParams_.pictOnly_);
            viewParams.fixedWidth_ = viewParams_.fixedWidth_;
            viewParams.replaceFavorites_ = replaceFavorites_;

            auto& item = contactItems_[aimId];
            if (!item)
                item = std::make_unique<Ui::ContactItem>(membersView_, needDrawIcons_);

            item->updateParams(visData, viewParams);
            item->paint(*_painter, false, false, false, _option.rect.width());
        }

        if (Q_UNLIKELY(Ui::GetAppConfig().IsShowMsgIdsEnabled()))
        {
            if (contactType == Data::BASE)
            {
                _painter->setPen(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
                _painter->setFont(Fonts::appFontScaled(10, Fonts::FontWeight::SemiBold));

                const auto x = _option.rect.width();
                const auto y = _option.rect.height();
                const QStringList debugInfo =
                {
                    Logic::getContactListModel()->isChannel(aimId) ? qsl("Chan") : QString(),
                    Logic::getContactListModel()->contains(aimId)
                        ? u"oc:" % QString::number(Logic::getContactListModel()->getOutgoingCount(aimId))
                        : qsl("!CL"),
                    aimId,
                };
                Utils::drawText(*_painter, QPointF(x, y), Qt::AlignRight | Qt::AlignBottom, debugInfo.join(ql1c(' ')));
            }
        }

        if (needDrawIcons())
        {
            Data::DlgState state = Logic::getRecentsModel()->getDlgState(aimId);
            auto unreadsX = _option.rect.width();
            auto unreadsY = (Ui::GetContactListParams().itemHeight() - IconsManager::getUnreadsSize()) / 2;
            auto contactListParams = Ui::GetContactListParams();
            auto right_area_right = _option.rect.width() - contactListParams.itemHorPaddingRight();

            drawUnreads(_painter, state, isSelected, _option.rect, viewParams_, unreadsX, unreadsY);
            drawAttention(_painter, state, isSelected, _option.rect, viewParams_, unreadsX, unreadsY);
            drawMentions(_painter, state, isSelected, _option.rect, viewParams_, unreadsX, unreadsY);
            drawDraft(_painter, state, isSelected, _option.rect, viewParams_, right_area_right);
        }

        if (_index == DragIndex_)
            renderDragOverlay(*_painter, QRect { QPoint { 0, 0 }, _option.rect.size() }, viewParams_);
    }

    int ContactListItemDelegate::getUnknownUnreads(const QRect& _rect) const
    {
        return _rect.x() + _rect.width() - Logic::IconsManager::buttonWidth() - Logic::IconsManager::getUnknownRightOffset();
    }

    QSize ContactListItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex &index) const
    {
        // For custom widget
        if (auto customWidget = index.data().value<QWidget*>())
            return customWidget->sizeHint();

        const auto membersModel = qobject_cast<const Logic::ChatMembersModel*>(index.model());
        const auto width = viewParams_.fixedWidth_ == -1 ? Ui::GetContactListParams().itemWidth() : viewParams_.fixedWidth_;
        if (!membersModel)
        {
            const auto cont = index.data(Qt::DisplayRole).value<Data::Contact*>();
            if (cont && cont->GetType() != Data::BASE)
            {
                switch (cont->GetType())
                {
                case Data::SERVICE_HEADER:
                    return QSize(width, Ui::GetContactListParams().serviceItemHeight());
                case Data::DROPDOWN_BUTTON:
                    return QSize(width, Ui::GetContactListParams().dropdownButtonHeight());
                default:
                    break;
                }
            }
        }
        return QSize(width, Ui::GetContactListParams().itemHeight());
    }

    void ContactListItemDelegate::blockState(bool value)
    {
        StateBlocked_= value;
    }

    void ContactListItemDelegate::setDragIndex(const QModelIndex& index)
    {
        DragIndex_ = index;
    }

    void ContactListItemDelegate::setFixedWidth(int width)
    {
        viewParams_.fixedWidth_ = width;
    }

    void ContactListItemDelegate::setLeftMargin(int margin)
    {
        viewParams_.leftMargin_ = margin;
    }

    void ContactListItemDelegate::setRightMargin(int margin)
    {
        viewParams_.rightMargin_ = margin;
    }

    void ContactListItemDelegate::setRegim(int _regim)
    {
        viewParams_.regim_ = _regim;
    }

    int ContactListItemDelegate::regim() const
    {
        return viewParams_.regim_;
    }

    void ContactListItemDelegate::setRenderRole(bool render)
    {
        renderRole_ = render;
    }

    void ContactListItemDelegate::setPictOnlyView(bool _pictOnlyView)
    {
        viewParams_.pictOnly_ = _pictOnlyView;
    }

    bool ContactListItemDelegate::isPictOnlyView() const
    {
        return viewParams_.pictOnly_;
    }

    void ContactListItemDelegate::clearCache()
    {
        items_.clear();
        contactItems_.clear();
    }

    void ContactListItemDelegate::setMembersView()
    {
        membersView_ = true;
    }

    bool ContactListItemDelegate::needsTooltip(const QString& _aimId, const QModelIndex&, QPoint _posCursor) const
    {
        Data::DlgState state = Logic::getRecentsModel()->getDlgState(_aimId);
        if (state.hasDraft() && isInDraftIconRect(_posCursor) && (viewParams_.regim_ == Logic::MembersWidgetRegim::CONTACT_LIST))
            return true;

        const auto it = contactItems_.find(_aimId);
        return it != contactItems_.end() && it->second && it->second->needsTooltip();
    }

    bool ContactListItemDelegate::isUnknown() const
    {
        return viewParams_.regim_ == ::Logic::MembersWidgetRegim::UNKNOWN;
    }

    void ContactListItemDelegate::drawDraft(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _rightAreaRight) const
    {
        if (!needDrawDraft(_viewParams, _state) || !_painter)
            return;

       using im = Logic::IconsManager;

       QPixmap badge = im::getDraftIcon(_isSelected).actualPixmap();
        _rightAreaRight -= Utils::unscale_bitmap(badge.width());

        if (needDrawUnreads(_state))
        {
            _rightAreaRight -= im::getUnreadBubbleRightMarginPictOnly();
            _rightAreaRight -= im::getUnreadBalloonWidth(_state.UnreadCount_);
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

        const auto draftY = (_rect.height() - im::getDraftSize().height())/2;
        _painter->drawPixmap(_rightAreaRight, draftY, badge);

        draftIconRect_.setRect(_rightAreaRight + dp_.x(), draftY + dp_.y(), badge.width(), badge.height());
    }

    bool ContactListItemDelegate::needDrawUnreads(const Data::DlgState& _state) const
    {
        return needDrawIcons() && _state.UnreadCount_ > 0;
    }

    bool ContactListItemDelegate::needDrawMentions(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const
    {
        Q_UNUSED(_viewParams);
        return needDrawIcons() && _state.unreadMentionsCount_ > 0 && !isUnknown();
    }

    bool ContactListItemDelegate::needDrawAttention(const Data::DlgState& _state) const
    {
        return needDrawIcons() && !needDrawUnreads(_state)  && _state.Attention_;
    }

    bool ContactListItemDelegate::needDrawDraft(const Ui::ViewParams& _viewParams, const Data::DlgState& _state) const
    {
        Q_UNUSED(_viewParams);
        return needDrawIcons() && _state.hasDraft() && !isPictOnlyView();
    }

    bool ContactListItemDelegate::isUnknownAndNotPictOnly(const Ui::ViewParams& _viewParams) const
    {
        return (isUnknown() && !isPictOnlyView());
    }

    void ContactListItemDelegate::drawUnreads(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const
    {
        Q_UNUSED(_unreadsY);

        if (!needDrawUnreads(_state) || !_painter)
            return;

        auto contactListParams = Ui::GetContactListParams();

        using im = Logic::IconsManager;
        _unreadsX -= isPictOnlyView() ? im::getUnreadBubbleRightMarginPictOnly() : im::getUnreadBubbleRightMargin(true);
        _unreadsX -= im::getUnreadBalloonWidth(_state.UnreadCount_);

        using st = Styling::StyleVariable;
        std::function getColor = [_isSelected](st selectedColor, st color){ return Styling::getParameters().getColor(_isSelected ? selectedColor : color); };

        const auto muted_ = Logic::getContactListModel()->isMuted(_state.AimId_);
        const auto bgColor = getColor(st::TEXT_SOLID_PERMANENT, muted_ ? st::BASE_TERTIARY : st::PRIMARY);
        const auto textColor = getColor(st::PRIMARY_SELECTED, st::BASE_GLOBALWHITE);

        auto x = isUnknownAndNotPictOnly(_viewParams) ? getUnknownUnreads(_rect) : _rect.left() + _unreadsX;
        auto y = isUnknownAndNotPictOnly(_viewParams) ? _rect.top() + (contactListParams.itemHeight() - im::getUnreadBubbleHeight()) / 2 :
                                                (_rect.height() - im::getUnreadsSize()) / 2;

        Utils::drawUnreads(*(_painter), contactListParams.unreadsFont(), bgColor, textColor, _state.UnreadCount_, im::getUnreadBubbleHeight(), x, y);
    }

    void ContactListItemDelegate::drawMentions(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const
    {
        Q_UNUSED(_unreadsY);

        if (!needDrawMentions(_viewParams, _state) || !_painter)
            return;

        using im = Logic::IconsManager;
        if (_state.UnreadCount_)
            _unreadsX -= isPictOnlyView() ? im::getUnreadLeftPaddingPictOnly() : im::getUnreadLeftPadding();
        else
            _unreadsX -= isPictOnlyView() ? im::getUnreadBubbleRightMarginPictOnly() : im::getUnreadBubbleRightMargin(true);

        _unreadsX -= im::getMentionSize().width();

        _painter->drawPixmap(_rect.left() + _unreadsX, (_rect.height() - im::getMentionSize().height()) / 2, im::getMentionIcon(_isSelected).cachedPixmap());
    }

    void ContactListItemDelegate::drawAttention(QPainter* _painter, const Data::DlgState& _state, bool _isSelected, const QRect& _rect, const Ui::ViewParams& _viewParams, int& _unreadsX, int& _unreadsY) const
    {
        Q_UNUSED(_unreadsY);
        if (!needDrawAttention(_state) || !_painter)
            return;

        using im = Logic::IconsManager;
        _unreadsX -= isPictOnlyView() ? im::getUnreadBubbleRightMarginPictOnly() : im::getUnreadBubbleRightMargin(true);
        _unreadsX -= im::getAttentionSize().width();

        auto contactListParams = Ui::GetContactListParams();

        auto x = isUnknownAndNotPictOnly(_viewParams) ? getUnknownUnreads(_rect) : _rect.left() + _unreadsX;
        auto y = isUnknownAndNotPictOnly(_viewParams) ? _rect.top() + (contactListParams.itemHeight() - im::getUnreadBubbleHeight()) / 2 :
                                                (_rect.height() - im::getAttentionSize().height()) / 2;

        _painter->drawPixmap(x, y, im::getAttentionIcon(_isSelected).cachedPixmap());
    }

}
