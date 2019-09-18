#include "stdafx.h"

#include "ContactListItemDelegate.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../friendly/FriendlyContainer.h"
#include "../../controls/TextUnit.h"
#include "../../controls/CheckboxList.h"

#include "SearchMembersModel.h"
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
#include "ContactList.h"
#include "CommonChatsModel.h"
#include "SearchHighlight.h"

#include "styles/ThemeParameters.h"

namespace
{
    int nickPadding()
    {
        return Utils::scale_value(4);
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

        QColor overlayColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        overlayColor.setAlphaF(0.9);
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
            auto result = Utils::renderSvgScaled(qsl(":/controls/more_icon"), QSize(20, 20), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY)).transformed(trans);
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
        static const QPixmap add(Utils::renderSvgScaled(qsl(":/controls/add_icon"), QSize(20, 20), Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY)));
        static const QPixmap addSelected(Utils::renderSvgScaled(qsl(":/controls/add_icon"), QSize(20, 20), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE)));

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
        const auto cbSize = QSize(20, 20);
        const auto state = _visData.isChatMember_ ? CheckBox::Activity::DISABLED : (_visData.isCheckedBox_ ? CheckBox::Activity::ACTIVE : CheckBox::Activity::NORMAL);
        const auto& img =  Ui::CheckBox::getCheckBoxIcon(state);

        double ratio = Utils::scale_bitmap_ratio();
        _painter.drawPixmap(_contactList.itemWidth(), _contactList.getItemMiddleY() - (img.height() / 2. / ratio), img);

    }

    int Item::drawRemove(QPainter &_painter, bool _isSelected, ContactListParams& _contactList, const ViewParams& _viewParams)
    {
        Utils::PainterSaver p(_painter);
        static const QPixmap rem(Utils::renderSvgScaled(qsl(":/controls/close_icon"), QSize(12, 12), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY)));
        static const QPixmap remSelected(Utils::renderSvgScaled(qsl(":/controls/close_icon"), QSize(12, 12), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_ACTIVE)));

        const auto& remove_img = _isSelected ? remSelected : rem;

        const auto width = CorrectItemWidth(ItemWidth(false, false), _viewParams.fixedWidth_);
        static const auto buttonWidth = Utils::scale_value(32);

        auto& removeFrame = _contactList.removeContactFrame();
        removeFrame.setX(width - buttonWidth);
        removeFrame.setY(0);
        removeFrame.setWidth(buttonWidth);
        removeFrame.setHeight(_contactList.itemHeight());

        const double ratio = Utils::scale_bitmap_ratio();
        const int x = removeFrame.x() + ((buttonWidth - remove_img.width() / ratio) / 2);
        const int y = (_contactList.itemHeight() - remove_img.height() / ratio) / 2.;

        _painter.setRenderHint(QPainter::Antialiasing);
        _painter.setRenderHint(QPainter::SmoothPixmapTransform);

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

        const auto bgColor = Styling::getParameters().getColor( _isSelected? Styling::StyleVariable::TEXT_SOLID :
                            (_isMuted ? Styling::StyleVariable::BASE_TERTIARY
                            : Styling::StyleVariable::PRIMARY));

        const auto textColor = Styling::getParameters().getColor( _isSelected ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::TEXT_SOLID);
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
        else if (_type == Data::EMPTY_IGNORE_LIST)
        {
            color = Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
            font = contactList.emptyIgnoreListFont();
        }

        name_->init(font, color);
        name_->evaluateDesiredSize();
    }

    ServiceContact::~ServiceContact()
    {
    }

    void ServiceContact::paint(QPainter& _painter, const bool _isHovered, const bool _isSelected, const bool /*_isPressed*/, const int _curWidth)
    {
        Utils::PainterSaver p(_painter);

        auto contactList = GetContactListParams();
        auto x = contactList.itemHorPadding();
        auto height = contactList.serviceItemHeight();
        switch (type_)
        {
        case Data::EMPTY_IGNORE_LIST:
            height = contactList.itemHeight();
            x = contactList.getContactNameX();
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

        contactStatus_ = TextRendering::MakeTextUnit(QString());
        contactStatus_->init(Fonts::appFontScaled(13, Fonts::FontWeight::Normal), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));

        contactRole_ = TextRendering::MakeTextUnit(QString());
        contactRole_->init(Fonts::appFontScaled(13), Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
    }

    ContactItem::~ContactItem()
    {

    }

    void ContactItem::updateParams(const VisualDataBase& _item, const ViewParams& _viewParams)
    {
        const auto& contactList = GetContactListParams();

        if (_item.ContactName_ != item_.ContactName_ || _item.highlights_ != item_.highlights_)
        {
            QColor color;
            if (contactList.isCL())
            {
                const auto isMemberSelected = IsSelectMembers(_viewParams.regim_) && (_item.isChatMember_ || _item.isCheckedBox_);
                color = contactList.getNameFontColor(_item.IsSelected_, isMemberSelected);
            }
            else
            {
                color = contactList.getNameFontColor(_item.IsSelected_, false);
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

        if (!_item.nick_.isEmpty() && !_item.highlights_.empty())
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
            else if (_item.role_ == ql1s("admin") || _item.role_ == ql1s("moder"))
                contactRole_->setText(QT_TRANSLATE_NOOP("contactlist", "Admin"));
            else if (_item.role_ == ql1s("readonly"))
                contactRole_->setText(QT_TRANSLATE_NOOP("contactlist", "Read only"));
            else
                contactRole_->setText(QString());
        }

        if (_item.IsSelected_ != item_.IsSelected_)
        {
            const auto isMemberSelected = IsSelectMembers(_viewParams.regim_) && (_item.isChatMember_ || _item.isCheckedBox_);
            const auto color = contactList.getNameFontColor(_item.IsSelected_, isMemberSelected);
            contactName_->setColor(color);
        }

        if (_item.GetStatus() != item_.GetStatus() || _item.IsSelected_ != item_.IsSelected_)
        {
            const auto onlineColor = Styling::getParameters().getColor(_item.IsSelected_ ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::TEXT_PRIMARY);
            const auto seenColor = Styling::getParameters().getColor(_item.IsSelected_ ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::BASE_PRIMARY);

            contactStatus_->setText(_item.GetStatus(), _item.GetStatus() == QT_TRANSLATE_NOOP("state", "Online") ? onlineColor : seenColor);
        }

        if (_viewParams.regim_ == Logic::MembersWidgetRegim::COMMON_CHATS)
        {
            auto memebersStr = Utils::GetTranslator()->getNumberString(_item.membersCount_, QT_TRANSLATE_NOOP3("contactlist", "%1 member", "1"),
                                                                                            QT_TRANSLATE_NOOP3("contactlist", "%1 members", "2"),
                                                                                            QT_TRANSLATE_NOOP3("contactlist", "%1 members", "5"),
                                                                                            QT_TRANSLATE_NOOP3("contactlist", "%1 members", "21")).arg(_item.membersCount_);
            contactStatus_->setText(memebersStr, Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
        }

        item_ = _item;
        params_ = _viewParams;
    }

    void ContactItem::paint(QPainter& _painter, const bool, const bool, const bool, const int _curWidth)
    {
        auto contactList = GetContactListParams();

        Utils::PainterSaver p(_painter);

        _painter.setRenderHint(QPainter::Antialiasing);

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

            if (textWidth + nickWidth > maxWidth)
            {
                if (contactNick_)
                {
                    if (nickWidth <= maxWidth / 2)
                    {
                        contactName_->getHeight(maxWidth - nickWidth);
                        textWidth = contactName_->getLastLineWidth();
                    }
                    else
                    {
                        contactName_->getHeight(maxWidth / 2);
                        textWidth = contactName_->getLastLineWidth();

                        contactNick_->getHeight(maxWidth - textWidth - nickPadding());
                    }

                    contactNick_->setOffsets(nameX + textWidth + nickPadding(), _y);
                }
                else
                {
                    contactName_->getHeight(maxWidth);
                }
            }
            else if (contactNick_)
            {
                contactNick_->setOffsets(nameX + textWidth + nickPadding(), _y);
            }

            contactName_->setOffsets(nameX, _y);
            contactName_->draw(_painter, _center ? TextRendering::VerPosition::MIDDLE : TextRendering::VerPosition::TOP);

            if (contactNick_)
                contactNick_->draw(_painter, _center ? TextRendering::VerPosition::MIDDLE : TextRendering::VerPosition::TOP);

            if (drawStatus)
            {
                contactStatus_->elide(maxWidth, TextRendering::ELideType::FAST);
                contactStatus_->setOffsets(contactList.getContactNameX(params_.leftMargin_), _y + contactName_->cachedSize().height() + Utils::scale_value(8));
                contactStatus_->draw(_painter, TextRendering::VerPosition::MIDDLE);
            }

            contactRole_->setOffsets(_rightMargin - contactList.itemHorPadding() - Utils::scale_value(4) - contactRole_->cachedSize().width() - _role_right_offset, contactList.itemHeight() / 2);
            contactRole_->draw(_painter, TextRendering::VerPosition::MIDDLE);
        };

        const auto regim = params_.regim_;

        RenderMouseState(_painter, item_.IsHovered_, item_.IsSelected_, params_, contactList.itemHeight());

        if (IsSelectMembers(regim) || regim == Logic::MembersWidgetRegim::SHARE)
            drawCheckbox(_painter, item_, contactList);

        const auto avX = params_.pictOnly_
            ? _curWidth / 2 - contactList.getAvatarSize() / 2
            : contactList.getAvatarX() + params_.leftMargin_;
        const auto avY = contactList.getAvatarY();

        Utils::drawAvatarWithBadge(_painter, QPoint(avX, avY), item_.Avatar_, item_.isOfficial_, item_.isMuted_, item_.IsSelected_);

        if (params_.pictOnly_)
            return;

        auto rightMargin = 0;

        auto role_right_offset = 0;
        if (regim == Logic::MembersWidgetRegim::PENDING_MEMBERS)
        {
            rightMargin = drawRemove(_painter, false, contactList, params_);
            rightMargin = drawAddContact(_painter, rightMargin, false, contactList) - contactList.itemHorPadding();

            renderContactName(rightMargin, contactList.itemHeight() / 2, true, role_right_offset, false);
            return;
        }
        else if (item_.IsHovered_)
        {
            if (Logic::is_members_regim(regim))
            {
                rightMargin = drawRemove(_painter, false, contactList, params_);
                role_right_offset = Utils::scale_value(14);
            }
            else if (Logic::is_admin_members_regim(regim))
            {
                rightMargin = drawMore(_painter, contactList, params_);
                role_right_offset = Utils::scale_value(14);
            }
        }

        rightMargin = CorrectItemWidth(ItemWidth(params_), params_.fixedWidth_);
        rightMargin -= params_.rightMargin_;

        const auto isSelectRegim = IsSelectMembers(regim) || regim == Logic::MembersWidgetRegim::SHARE || regim == Logic::MembersWidgetRegim::SHARE_CONTACT;
        auto contactNameY = 0;
        if (regim == Logic::MembersWidgetRegim::COMMON_CHATS)
            contactNameY = platform::is_apple() ? Utils::scale_value(6) : Utils::scale_value(2);
        else
            contactNameY = isSelectRegim || !item_.HasStatus() ? contactList.itemHeight() / 2 : (platform::is_apple() ? Utils::scale_value(6) : Utils::scale_value(2));

        if (isSelectRegim)
            rightMargin -= Utils::scale_value(32);

        renderContactName(rightMargin, contactNameY, regim == Logic::MembersWidgetRegim::COMMON_CHATS ? false : (isSelectRegim || !item_.HasStatus()), role_right_offset, (!isSelectRegim && item_.HasStatus()) || regim == Logic::MembersWidgetRegim::COMMON_CHATS);
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

    GroupItem::~GroupItem()
    {

    }

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

    ServiceButton::~ServiceButton()
    {
    }

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

    ContactListItemDelegate::~ContactListItemDelegate()
    {
    }

    void ContactListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        auto contactType = Data::BASE;
        QString displayName, status, state, aimId;
        bool hasLastSeen = false, isChecked = false, isChatMember = false, isOfficial = false;
        int membersCount = 0;
        QDateTime lastSeen;
        QString iconId;
        QPixmap icon;
        QPixmap hoverIcon;
        QPixmap pressedIcon;
        Ui::highlightsV highlights;

        Data::Contact* contact_in_cl = nullptr;
        QString role;
        bool isCreator = false;

        if (index.data(Qt::DisplayRole).value<QWidget*>())
        {
            // If we use custom widget.
            return;
        }
        else if (auto searchRes = index.data(Qt::DisplayRole).value<Data::AbstractSearchResultSptr>())
        {
            aimId = searchRes->getAimId();
            highlights = searchRes->highlights_;

            //!! todo: Logic::SearchMembersModel temporarily for voip
            if (qobject_cast<const Logic::ChatMembersModel*>(index.model()) || qobject_cast<const Logic::SearchMembersModel*>(index.model()))
            {
                const auto& memberRes = std::static_pointer_cast<Data::SearchResultChatMember>(searchRes);
                isCreator = memberRes->isCreator();

                if (renderRole_)
                    role = memberRes->getRole();

                if (auto contact_item = Logic::getContactListModel()->getContactItem(aimId))
                    contact_in_cl = contact_item->Get();

                if (memberRes->getLastseen() != -1)
                {
                    hasLastSeen = true;
                    lastSeen = QDateTime::fromTime_t(memberRes->getLastseen());
                }
            }
            else if (qobject_cast<const Logic::SearchModel*>(index.model()))
            {
                if (auto contact_item = Logic::getContactListModel()->getContactItem(aimId))
                    contact_in_cl = contact_item->Get();
            }
            else if (qobject_cast<const Logic::CommonChatsSearchModel*>(index.model()))
            {
                const auto& inf = std::static_pointer_cast<Data::SearchResultCommonChat>(searchRes);
                membersCount = inf->info_.membersCount_;
            }
        }
        else if (qobject_cast<const Logic::CommonChatsModel*>(index.model()))
        {
            auto cont = index.data(Qt::DisplayRole).value<Data::CommonChatInfo>();
            aimId = cont.aimid_;
            displayName = cont.friendly_;
            membersCount = cont.membersCount_;
        }
        else
        {
            contact_in_cl = index.data(Qt::DisplayRole).value<Data::Contact*>();
        }

        bool serviceContact = false;
        if (contact_in_cl)
        {
            contactType = contact_in_cl->GetType();
            serviceContact = (contactType == Data::SERVICE_HEADER || contactType == Data::DROPDOWN_BUTTON);

            aimId = contact_in_cl->AimId_;

            if (aimId == Ui::MyInfo()->aimId() && hasLastSeen)
                status = Logic::GetFriendlyContainer()->getStatusString(lastSeen.toTime_t());
            else if (!contact_in_cl->Is_chat_ && !serviceContact)
                status = Logic::GetFriendlyContainer()->getStatusString(aimId);

            state = contact_in_cl->State_;
            if (!hasLastSeen)
            {
                hasLastSeen = contact_in_cl->HasLastSeen_;
                lastSeen = contact_in_cl->LastSeen_;
            }
            isChecked = contact_in_cl->IsChecked_;
            isOfficial = contact_in_cl->IsOfficial_;
            iconId = contact_in_cl->iconId_;
            icon = contact_in_cl->iconPixmap_;
            hoverIcon = contact_in_cl->iconHoverPixmap_;
            pressedIcon = contact_in_cl->iconPressedPixmap_;
        }
        else
        {
            if (hasLastSeen)
                status = Logic::GetFriendlyContainer()->getStatusString(lastSeen.toTime_t());

            isOfficial = Logic::GetFriendlyContainer()->getOfficial(aimId);
        }

        if (displayName.isEmpty())
        {
            if (contact_in_cl && serviceContact)
                displayName = contact_in_cl->GetDisplayName();
            else
                displayName = Logic::GetFriendlyContainer()->getFriendly(aimId);
        }

        if (!!chatMembersModel_)
            isChatMember = chatMembersModel_->contains(aimId);

        const auto isHeader = contactType == Data::SERVICE_HEADER;
        const bool isSelected = viewParams_.regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP && Logic::getContactListModel()->selectedContact() == aimId && !isHeader && !StateBlocked_;
        const bool isHovered = (option.state & QStyle::State_Selected) && !isHeader && !StateBlocked_;
        const bool isMouseOver = (option.state & QStyle::State_MouseOver);
        const bool isPressed = isHovered && isMouseOver && (QApplication::mouseButtons() & Qt::MouseButton::LeftButton);

        Utils::PainterSaver ps(*painter);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->translate(option.rect.topLeft());

        if (contactType != Data::BASE)
        {
            switch (contactType)
            {
            case Data::GROUP:
            {
                auto& item = items[aimId];
                if (!item)
                    item = std::make_unique<Ui::GroupItem>(displayName);

                item->paint(*painter);
                break;
            }
            case Data::SERVICE_HEADER:
            {
                auto& item = items[aimId];
                if (!item)
                    item = std::make_unique<Ui::ServiceContact>(displayName, contactType);

                item->paint(*painter);
                break;
            }
            case Data::DROPDOWN_BUTTON:
            {
                auto& item = items[aimId];
                if (!item)
                    item = std::make_unique<Ui::ServiceButton>(displayName, icon, hoverIcon, pressedIcon, option.rect.height());

                item->paint(*painter, isHovered && isMouseOver, false, isPressed);
                break;
            }
            default:
                break;
            }
        }
        else
        {
            const auto isMultichat = Logic::getContactListModel()->isChat(aimId);

            QString nick;
            if (!isMultichat)
                nick = Logic::GetFriendlyContainer()->getNick(aimId);

            auto isDefault = false;

            if (isSelected && (state == ql1s("online") || state == ql1s("mobile")))
                state += ql1s("_active");

            const auto &avatar = Logic::GetAvatarStorage()->GetRounded(
                aimId, displayName, Utils::scale_bitmap(Ui::GetContactListParams().getAvatarSize()),
                QString(),
                isDefault, false /* _regenerate */ ,
                Ui::GetContactListParams().isCL());

            Ui::VisualDataBase visData(
                aimId, *avatar, state, status, isHovered, isSelected, displayName, nick, highlights,
                hasLastSeen, lastSeen, isChecked, isChatMember, isOfficial, false /* draw last read */, QPixmap() /* last seen avatar*/,
                role, Logic::getRecentsModel()->getUnreadCount(aimId), Logic::getContactListModel()->isMuted(aimId), false, Logic::getRecentsModel()->getAttention(aimId), isCreator);

            visData.membersCount_ = membersCount;

            Ui::ViewParams viewParams(viewParams_.regim_, option.rect.width(), viewParams_.leftMargin_, viewParams_.rightMargin_, viewParams_.pictOnly_);
            viewParams.fixedWidth_ = viewParams_.fixedWidth_;

            auto& item = contactItems_[aimId];
            if (!item)
                item = std::make_unique<Ui::ContactItem>(membersView_);

            item->updateParams(visData, viewParams);
            item->paint(*painter, false, false, false, option.rect.width());
        }

        if (index == DragIndex_)
        {
            Ui::Item::drawContactsDragOverlay(*painter);
        }

        if (Q_UNLIKELY(Ui::GetAppConfig().IsShowMsgIdsEnabled()))
        {
            if (contactType == Data::BASE)
            {
                painter->setPen(Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
                painter->setFont(Fonts::appFontScaled(10, Fonts::FontWeight::SemiBold));

                const auto x = option.rect.width();
                const auto y = option.rect.height();
                const QStringList debugInfo =
                {
                    Logic::getContactListModel()->isChannel(aimId) ? qsl("Chan") : QString(),
                    Logic::getContactListModel()->contains(aimId)
                        ? qsl("oc:") % QString::number(Logic::getContactListModel()->getOutgoingCount(aimId))
                        : qsl("!CL"),
                    aimId,
                };
                Utils::drawText(*painter, QPointF(x, y), Qt::AlignRight | Qt::AlignBottom, debugInfo.join(ql1c(' ')));
            }
        }
    }

    QSize ContactListItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex &index) const
    {
        // For custom widget
        auto customWidget = index.data().value<QWidget*>();
        if (customWidget)
            return customWidget->isVisible() ? customWidget->sizeHint() : QSize();

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

    void ContactListItemDelegate::clearCache()
    {
        items.clear();
        contactItems_.clear();
    }

    void ContactListItemDelegate::setMembersView()
    {
        membersView_ = true;
    }
}
