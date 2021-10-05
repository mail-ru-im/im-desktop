#include "stdafx.h"

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

namespace
{
    auto nickPadding() noexcept
    {
        return Utils::scale_value(4);
    }

    constexpr auto getCLItemOpacity() noexcept
    {
        return 0.2;
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
}

namespace Ui
{
    void Item::drawContactsDragOverlay(QPainter &_painter)
    {
        auto contactList = Ui::GetContactListParams();
        Utils::PainterSaver ps(_painter);

        _painter.setPen(Qt::NoPen);
        _painter.setRenderHint(QPainter::Antialiasing);

        QColor overlayColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE, 0.9));
        _painter.fillRect(0, 0, Ui::ItemWidth(false, false), contactList.itemHeight(), QBrush(overlayColor));
        _painter.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::GHOST_ACCENT));
        QPen pen(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_SELECTED), contactList.dragOverlayBorderWidth(), Qt::DashLine, Qt::RoundCap);
        _painter.setPen(pen);
        _painter.drawRoundedRect(
            contactList.dragOverlayPadding(),
            contactList.dragOverlayVerPadding(),
            Ui::ItemWidth(false, false) - contactList.itemHorPadding() - Utils::scale_value(1),
            contactList.itemHeight() - contactList.dragOverlayVerPadding(),
            contactList.dragOverlayBorderRadius(),
            contactList.dragOverlayBorderRadius()
        );

        _painter.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        _painter.setFont(Fonts::appFontScaled(15));
        Utils::drawText(_painter, QPoint(Ui::ItemWidth(false, false) / 2, contactList.itemHeight() / 2), Qt::AlignCenter, QT_TRANSLATE_NOOP("files_widget", "Quick send"));
    }

    int Item::drawMore(QPainter &_painter, Ui::ContactListParams& _contactList, const Ui::ViewParams& _viewParams)
    {
        const auto width = _viewParams.fixedWidth_;
        auto img = []()
        {
            QTransform trans;
            trans.rotate(90);
            auto result = makeIcon(qsl(":/controls/more_icon"), QSize(20, 20), Styling::StyleVariable::BASE_SECONDARY).transformed(trans);
            Utils::check_pixel_ratio(result);
            return result;
        };

        static const auto optionsImg = img();

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
        static const QPixmap add(makeIcon(qsl(":/controls/add_icon"), QSize(20, 20), Styling::StyleVariable::PRIMARY));
        static const QPixmap addSelected(makeIcon(qsl(":/controls/add_icon"), QSize(20, 20), Styling::StyleVariable::BASE_GLOBALWHITE));

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

    void Item::drawCheckbox(QPainter &_painter, const VisualDataBase &_visData, const ContactListParams& _contactList)
    {
        const auto state = _visData.isChatMember_ ? CheckBox::Activity::DISABLED : (_visData.isCheckedBox_ ? CheckBox::Activity::ACTIVE : CheckBox::Activity::NORMAL);
        const auto& img =  Ui::CheckBox::getCheckBoxIcon(state);

        double ratio = Utils::scale_bitmap_ratio();
        _painter.drawPixmap(_contactList.itemWidth(), _contactList.getItemMiddleY() - (img.height() / 2. / ratio), img);

    }

    int Item::drawRemove(QPainter &_painter, bool _isSelected, ContactListParams& _contactList, const ViewParams& _viewParams)
    {
        static const QPixmap rem(makeIcon(qsl(":/controls/close_icon"), removeIconSize(), Styling::StyleVariable::BASE_SECONDARY));
        static const QPixmap remSelected(makeIcon(qsl(":/controls/close_icon"), removeIconSize(), Styling::StyleVariable::BASE_SECONDARY_ACTIVE));

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
        : type_(_type)
    {
        name_ = TextRendering::MakeTextUnit(_name);

        auto contactList = GetContactListParams();

        QColor color;
        QFont font;
        if (_type == Data::SERVICE_HEADER)
        {
            color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
            font = contactList.getContactListHeadersFont();
        }
        else if (_type == Data::EMPTY_LIST)
        {
            color = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
            font = contactList.emptyIgnoreListFont();
        }

        name_->init(font, color);
        name_->evaluateDesiredSize();
    }

    ServiceContact::~ServiceContact() = default;

    void ServiceContact::paint(QPainter& _painter, const bool _isHovered, const bool _isSelected, const bool /*_isPressed*/, const int _curWidth)
    {
        Utils::PainterSaver p(_painter);

        auto contactList = GetContactListParams();
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

    ContactItem::ContactItem(bool _membersView)
    {
        membersView_ = _membersView;

        contactName_ = TextRendering::MakeTextUnit(QString(), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);
        contactName_->init(Fonts::appFont(GetContactListParams().contactNameFontSize(), getNameWeight()),
                           Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT), QColor(), QColor(), QColor(), Ui::TextRendering::HorAligment::LEFT, 1);

        const auto secondaryFontSize = platform::is_apple() ? 14 : 13;
        contactStatus_ = TextRendering::MakeTextUnit(QString());
        contactStatus_->init(Fonts::appFontScaled(secondaryFontSize, Fonts::FontWeight::Normal), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));

        contactRole_ = TextRendering::MakeTextUnit(QString());
        contactRole_->init(Fonts::appFontScaled(secondaryFontSize), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
    }

    ContactItem::~ContactItem() = default;

    void ContactItem::updateParams(const VisualDataBase& _item, const ViewParams& _viewParams)
    {
        const auto& contactList = GetContactListParams();
        const auto isFavorites = _viewParams.replaceFavorites_ && Favorites::isFavorites(_item.AimId_);

        if (_item.ContactName_ != item_.ContactName_ || _item.highlights_ != item_.highlights_)
        {
            QColor color;
            if (!Logic::is_select_chat_members_regim(_viewParams.regim_) && contactList.isCL())
            {
                const auto isMemberSelected = IsSelectMembers(_viewParams.regim_) && (_item.isChatMember_ || _item.isCheckedBox_);
                color = contactList.getNameFontColor(_item.IsSelected_, isMemberSelected, isFavorites);
            }
            else
            {
                color = contactList.getNameFontColor(_item.IsSelected_, false, isFavorites);
            }

            contactName_ = createHightlightedText(
                _item.ContactName_,
                Fonts::appFont(contactList.contactNameFontSize(), getNameWeight()),
                color,
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
                    Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY),
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
        {
            const auto isMemberSelected = IsSelectMembers(_viewParams.regim_) && (_item.isChatMember_ || _item.isCheckedBox_);
            const auto color = contactList.getNameFontColor(_item.IsSelected_, isMemberSelected, isFavorites);
            contactName_->setColor(color);
        }

        if (_item.IsSelected_ != item_.IsSelected_ || _item.GetStatus() != item_.GetStatus())
        {
            const auto onlineColor = Styling::getParameters().getColor(_item.IsSelected_ ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::TEXT_PRIMARY);
            const auto seenColor = Styling::getParameters().getColor(_item.IsSelected_ ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::BASE_PRIMARY);

            contactStatus_->setText(_item.GetStatus(), _item.GetStatus() == Data::LastSeen::online().getStatusString() ? onlineColor : seenColor);
        }

        if (_viewParams.regim_ == Logic::MembersWidgetRegim::COMMON_CHATS)
        {
            auto membersStr = Utils::getMembersString(_item.membersCount_, Logic::getContactListModel()->isChannel(_item.AimId_));
            contactStatus_->setText(membersStr, Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        }

        item_ = _item;
        params_ = _viewParams;
    }

    void ContactItem::paint(QPainter& _painter, const bool, const bool, const bool, const int _curWidth)
    {
        auto contactList = GetContactListParams();

        Utils::PainterSaver p(_painter);

        _painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

        auto renderContactName = [this, &_painter, contactList](int _rightMargin, int _y, bool _center, int _role_right_offset, bool drawStatus)
        {
            int maxWidth = _rightMargin - contactList.getContactNameX(params_.leftMargin_);

            if (contactList.isCL())
                maxWidth -= contactList.itemContentPadding();
            if (!item_.role_.isEmpty())
                maxWidth -= (contactRole_->cachedSize().width() + Utils::scale_value(8) + _role_right_offset);

            const auto nameX = contactList.getContactNameX(params_.leftMargin_);

            int textWidth = contactName_->getLastLineWidth();
            int nickWidth = contactNick_ ? contactNick_->getLastLineWidth() + nickPadding() : 0;


            contactName_->evaluateDesiredSize();

            if (textWidth + nickWidth > maxWidth)
            {
                if (contactNick_)
                {
                    if (nickWidth <= maxWidth / 2)
                    {
                        contactName_->elide(maxWidth - nickWidth);
                        textWidth = contactName_->getLastLineWidth();
                    }
                    else
                    {
                        contactName_->elide(maxWidth / 2);
                        textWidth = contactName_->getLastLineWidth();

                        contactNick_->getHeight(maxWidth - textWidth - nickPadding());
                    }

                    contactNick_->setOffsets(nameX + textWidth + nickPadding(), _y);
                }
                else
                {
                    contactName_->elide(maxWidth);
                }
            }
            else if (contactNick_)
            {
                if (!contactName_->isElided())
                    contactName_->elide(maxWidth);
                contactNick_->setOffsets(nameX + textWidth + nickPadding(), _y);
            }
            else if (!contactName_->isElided())
            {
                contactName_->elide(maxWidth);
            }

            contactName_->setOffsets(nameX, _y);
            contactName_->draw(_painter, _center ? TextRendering::VerPosition::MIDDLE : TextRendering::VerPosition::TOP);

            if (contactNick_)
                contactNick_->draw(_painter, _center ? TextRendering::VerPosition::MIDDLE : TextRendering::VerPosition::TOP);

            if (drawStatus)
            {
                contactStatus_->elide(maxWidth);
                contactStatus_->setOffsets(contactList.getContactNameX(params_.leftMargin_), _y + contactName_->cachedSize().height() + Utils::scale_value(8));
                contactStatus_->draw(_painter, TextRendering::VerPosition::MIDDLE);
            }

            contactRole_->setOffsets(_rightMargin - contactList.itemHorPadding() - Utils::scale_value(4) - contactRole_->cachedSize().width() - _role_right_offset, contactList.itemHeight() / 2);
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
            Utils::drawAvatarWithoutBadge(_painter, QPoint(avX, avY), item_.Avatar_, statusBadge);
        else
            Utils::drawAvatarWithBadge(_painter, QPoint(avX, avY), item_.Avatar_, item_.isOfficial_, statusBadge, item_.isMuted_, item_.IsSelected_, item_.IsOnline_, true);

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
            if (Logic::is_admin_members_regim(regim))
            {
                rightMargin = drawMore(_painter, contactList, params_);
                role_right_offset = Utils::scale_value(14);
            }
            else if (Logic::is_members_regim(regim) || (item_.canRemoveTill_.isValid() && QDateTime::currentDateTime() < item_.canRemoveTill_))
            {
                rightMargin = drawRemove(_painter, false, contactList, params_);
                role_right_offset = Utils::scale_value(14);
            }
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

    Fonts::FontWeight ContactItem::getNameWeight() const
    {
        const auto& contactList = GetContactListParams();
        return membersView_ ? contactList.membersNameFontWeight() : contactList.contactNameFontWeight();
    }

    GroupItem::GroupItem(const QString& _name)
    {
        auto contactList = GetContactListParams();

        groupName_ = TextRendering::MakeTextUnit(_name);
        groupName_->init(contactList.groupFont(), contactList.groupColor());
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

    ServiceButton::ServiceButton(const QString& _name, const QPixmap& _icon, const QPixmap& _hoverIcon, const QPixmap& _pressedIcon, int _height)
        : icon_(_icon)
        , hoverIcon_(_hoverIcon)
        , pressedIcon_(_pressedIcon)
        , height_(_height)
    {
        auto contactList = GetContactListParams();

        name_ = TextRendering::MakeTextUnit(_name);

        name_->init(contactList.getContactNameFont(Fonts::FontWeight::Normal), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        name_->evaluateDesiredSize();
    }

    ServiceButton::~ServiceButton() = default;

    void ServiceButton::paint(QPainter& _painter, const bool _isHovered, const bool _isActive /* = false */, const bool _isPressed, const int _curWidth /* = 0 */)
    {
        Utils::PainterSaver p(_painter);
        RenderMouseState(_painter, _isHovered, false, QRect(0, 0, _curWidth, height_));

        auto contactList = GetContactListParams();
        const auto& icon = _isPressed ? pressedIcon_ : (_isHovered ? hoverIcon_ : icon_);

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
    ContactListItemDelegate::ContactListItemDelegate(QObject* parent, int _regim, CustomAbstractListModel* chatMembersModel)
        : AbstractItemDelegateWithRegim(parent)
        , StateBlocked_(false)
        , renderRole_(false)
        , membersView_(false)
        , chatMembersModel_(chatMembersModel)
    {
        viewParams_.regim_ = _regim;
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
        QPixmap icon;
        QPixmap hoverIcon;
        QPixmap pressedIcon;
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
                item = std::make_unique<Ui::ContactItem>(membersView_);

            item->updateParams(visData, viewParams);
            item->paint(*_painter, false, false, false, _option.rect.width());
        }

        if (_index == DragIndex_)
        {
            Ui::Item::drawContactsDragOverlay(*_painter);
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

    bool ContactListItemDelegate::needsTooltip(const QString& _aimId, const QModelIndex&, QPoint) const
    {
        const auto it = contactItems_.find(_aimId);
        return it != contactItems_.end() && it->second && it->second->needsTooltip();
    }
}
