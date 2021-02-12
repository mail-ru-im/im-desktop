#include "stdafx.h"
#include "SelectionContactsForConference.h"

#include "../main_window/contact_list/ContactListWidget.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/contact_list/SearchWidget.h"
#include "../main_window/contact_list/ContactListItemDelegate.h"

#include "../main_window/containers/FriendlyContainer.h"
#include "../main_window/containers/LastseenContainer.h"
#include "../main_window/GroupChatOperations.h"

#include "../cache/avatars/AvatarStorage.h"
#include "../controls/GeneralDialog.h"
#include "../controls/CustomButton.h"
#include "../utils/InterConnector.h"
#include "../utils/features.h"
#include "../styles/ThemeParameters.h"
#include "statuses/StatusTooltip.h"

#include "CommonUI.h"

namespace
{
    constexpr int curMembersIdx = 0;

    QSize getArrowSize()
    {
        return QSize(16, 16);
    }

    int horizontalModeAvatarLeftMargin()
    {
        return Utils::scale_value(12);
    }
}

namespace Ui
{
    CurrentConferenceMembers::CurrentConferenceMembers(QWidget* _parent, Logic::ChatMembersModel* _membersModel, bool _chatRoomCall)
        : QWidget(_parent)
        , conferenceMembersModel_(_membersModel)
    {
        membersLabelHost_ = new ClickableWidget(this);
        membersLabelHost_->setContentsMargins(Utils::scale_value(16), Utils::scale_value(12), Utils::scale_value(20), Utils::scale_value(4));
        Testing::setAccessibleName(membersLabelHost_, qsl("AS SelectionContactsForConference membersLabelHost"));

        memberLabel_ = new QLabel(getLabelCaption(), membersLabelHost_);
        memberLabel_->setContentsMargins(0, 0, 0, 0);
        memberLabel_->setStyleSheet(u"color:" % Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_SOLID));
        memberLabel_->setFont(Fonts::appFontScaled(12, Fonts::FontWeight::SemiBold));
        Testing::setAccessibleName(memberLabel_, qsl("AS SelectionContactsForConference memberLabel"));

        memberArrowDown_ = new CustomButton(membersLabelHost_, qsl(":/controls/down_icon"), getArrowSize());
        memberArrowDown_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        Testing::setAccessibleName(memberArrowDown_, qsl("AS SelectionContactsForConference memberExpand"));
        Styling::Buttons::setButtonDefaultColors(memberArrowDown_);

        memberArrowUp_ = new CustomButton(membersLabelHost_, qsl(":/controls/top_icon"), getArrowSize());
        memberArrowUp_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        Testing::setAccessibleName(memberArrowUp_, qsl("AS SelectionContactsForConference memberFold"));
        Styling::Buttons::setButtonDefaultColors(memberArrowUp_);
        memberArrowUp_->hide();

        auto membersLabelLayout = Utils::emptyHLayout(membersLabelHost_);
        membersLabelLayout->addWidget(memberLabel_);
        membersLabelLayout->addSpacing(Utils::scale_value(6));
        membersLabelLayout->addWidget(memberArrowDown_, 0, Qt::AlignTop);
        membersLabelLayout->addWidget(memberArrowUp_, 0, Qt::AlignTop);
        membersLabelLayout->addStretch();

        auto clDelegate = new Logic::ContactListItemDelegate(this, Logic::MembersWidgetRegim::VIDEO_CONFERENCE, conferenceMembersModel_);
        conferenceContacts_ = CreateFocusableViewAndSetTrScrollBar(this);
        conferenceContacts_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        conferenceContacts_->setFrameShape(QFrame::NoFrame);
        conferenceContacts_->setSpacing(0);
        conferenceContacts_->setModelColumn(0);
        conferenceContacts_->setUniformItemSizes(false);
        conferenceContacts_->setBatchSize(50);
        conferenceContacts_->setStyleSheet(qsl("background: transparent;"));
        conferenceContacts_->setCursor(Qt::PointingHandCursor);
        conferenceContacts_->setMouseTracking(true);
        conferenceContacts_->setAcceptDrops(true);
        conferenceContacts_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        conferenceContacts_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        conferenceContacts_->setAutoScroll(false);
        conferenceContacts_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        conferenceContacts_->setAttribute(Qt::WA_MacShowFocusRect, false);
        conferenceContacts_->setModel(conferenceMembersModel_);
        conferenceContacts_->setItemDelegate(clDelegate);
        conferenceContacts_->setContentsMargins(0, 0, 0, 0);
        Testing::setAccessibleName(conferenceContacts_, qsl("AS SelectionContactsForConference conferenceContacts"));

        auto othersLabel = new QLabel(_chatRoomCall ? QT_TRANSLATE_NOOP("voip_pages", "ALL GROUP MEMBERS") : QT_TRANSLATE_NOOP("voip_pages", "OTHERS"), this);
        othersLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        othersLabel->setContentsMargins(Utils::scale_value(16), Utils::scale_value(12), Utils::scale_value(20), Utils::scale_value(4));
        othersLabel->setMinimumHeight(2 * Utils::scale_value(12) + Utils::scale_value(4)); //Qt ignores our Margins if zoom is 200%. This line fix this problem.
        othersLabel->setStyleSheet(u"color:" % Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_SOLID));
        othersLabel->setFont(Fonts::appFontScaled(12, Fonts::FontWeight::SemiBold));
        Testing::setAccessibleName(othersLabel, qsl("AS SelectionContactsForConference othersLabel"));

        auto verLayout = Utils::emptyVLayout(this);
        verLayout->addWidget(membersLabelHost_);
        verLayout->addWidget(conferenceContacts_);
        verLayout->addWidget(othersLabel);

        onMembersLabelClicked();

        connect(conferenceContacts_, &FocusableListView::clicked, this, &CurrentConferenceMembers::itemClicked);

        connect(membersLabelHost_, &ClickableWidget::clicked, this, &CurrentConferenceMembers::onMembersLabelClicked);
        connect(memberArrowDown_, &CustomButton::clicked, this, &CurrentConferenceMembers::onMembersLabelClicked);
        connect(memberArrowUp_,   &CustomButton::clicked, this, &CurrentConferenceMembers::onMembersLabelClicked);

        connect(conferenceMembersModel_, &Logic::ChatMembersModel::dataChanged, this, [this](){ memberLabel_->setText(getLabelCaption()); });
        connect(conferenceContacts_, &ListViewWithTrScrollBar::mousePosChanged, this, &CurrentConferenceMembers::onMouseMoved);
    }

    void CurrentConferenceMembers::updateConferenceListSize()
    {
        const auto count = conferenceContacts_->flow() == QListView::TopToBottom ? conferenceMembersModel_->rowCount() : 1;
        const auto newHeight = ::Ui::GetContactListParams().itemHeight() * count;
        if (conferenceContacts_->height() != newHeight)
        {
            conferenceContacts_->setFixedHeight(newHeight);

            Q_EMIT sizeChanged(QPrivateSignal());
            update();
        }
    }

    void CurrentConferenceMembers::resizeEvent(QResizeEvent*)
    {
        Q_EMIT sizeChanged(QPrivateSignal());
    }

    void CurrentConferenceMembers::onMouseMoved(const QPoint& _pos, const QModelIndex& _index)
    {
        const auto avatarRect = getAvatarRect(_index);
        const auto aimId = Logic::aimIdFromIndex(_index);
        if (Statuses::isStatusEnabled() && avatarRect.contains(_pos))
        {
            StatusTooltip::instance()->objectHovered([this, aimId]()
            {
                auto index = conferenceContacts_->indexAt(conferenceContacts_->mapFromGlobal(QCursor::pos()));
                if (Logic::aimIdFromIndex(index) != aimId)
                    return QRect();

                const auto avatarRect = getAvatarRect(index);
                return QRect(conferenceContacts_->mapToGlobal(avatarRect.topLeft()), avatarRect.size());
            }, aimId, conferenceContacts_->viewport());
        }
    }

    void CurrentConferenceMembers::onMembersLabelClicked()
    {
        if (conferenceContacts_->flow() == QListView::TopToBottom)
        {
            conferenceContacts_->setFlow(QListView::LeftToRight);
            conferenceContacts_->setItemDelegate(new ContactListItemHorizontalDelegate(this));

            memberArrowDown_->show();
            memberArrowUp_->hide();
        } else
        {
            conferenceContacts_->setFlow(QListView::TopToBottom);
            auto deleg = new Logic::ContactListItemDelegate(this, Logic::MembersWidgetRegim::VIDEO_CONFERENCE, conferenceMembersModel_);
            deleg->setFixedWidth(conferenceContacts_->width());
            conferenceContacts_->setItemDelegate(deleg);

            memberArrowDown_->hide();
            memberArrowUp_->show();
        }

        updateConferenceListSize();
    }

    void CurrentConferenceMembers::itemClicked(const QModelIndex& _current)
    {
        if (!(QApplication::mouseButtons() & Qt::RightButton))
        {
            if (const auto aimid = Logic::aimIdFromIndex(_current); !aimid.isEmpty())
                Q_EMIT memberClicked(aimid, QPrivateSignal());
        }
    }

    QString CurrentConferenceMembers::getLabelCaption() const
    {
        QString res = QT_TRANSLATE_NOOP("voip_pages", "CALL MEMBERS");

        const auto currentCount = conferenceMembersModel_->getMembersCount();
        res = res % u" (" % QString::number(currentCount) % u'/' % QString::number(GetDispatcher()->getVoipController().maxVideoConferenceMembers()) % u')';

        return res;
    }

    QRect CurrentConferenceMembers::getAvatarRect(const QModelIndex& _index)
    {
        const auto itemRect = conferenceContacts_->visualRect(_index);
        const auto& params = GetContactListParams();
        const auto avatarX = conferenceContacts_->flow() == QListView::LeftToRight ? horizontalModeAvatarLeftMargin() : params.getAvatarX();
        return QRect(itemRect.topLeft() + QPoint(avatarX, params.getAvatarY()), QSize(params.getAvatarSize(), params.getAvatarSize()));
    }

    SelectionContactsForConference* SelectionContactsForConference::currentVisibleInstance_ = nullptr;

    SelectionContactsForConference::SelectionContactsForConference(
        Logic::ChatMembersModel* _conferenceMembersModel,
        const QString& _labelText,
        QWidget* _parent,
        bool _chatRoomCall,
        SelectContactsWidgetOptions _options)
        : SelectContactsWidget(
            _conferenceMembersModel,
            Logic::MembersWidgetRegim::VIDEO_CONFERENCE,
            _labelText,
            QString(),
            _parent,
            _options)
        , conferenceMembersModel_(_conferenceMembersModel)
        , videoConferenceMaxUsers_(-1)
        , chatRoomCall_(_chatRoomCall)
    {
        if (auto model = qobject_cast<ConferenceSearchModel*>(_options.searchModel_))
        {
            model->setView(contactList_);
            model->setDialog(this);
            model->setSearchPattern(QString());
        }

        auto buttonPair = mainDialog_->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "Cancel"), QT_TRANSLATE_NOOP("popup_window", "Add"), true);
        acceptButton_ = buttonPair.first;
        cancelButton_ = buttonPair.second;
        focusWidget_[acceptButton_] = FocusPosition::Accept;
        focusWidget_[cancelButton_] = FocusPosition::Cancel;

        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallNameChanged, this, &SelectionContactsForConference::onVoipCallNameChanged);
        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallDestroyed, this, &SelectionContactsForConference::onVoipCallDestroyed);

        searchWidget_->setTempPlaceholderText(QT_TRANSLATE_NOOP("popup_window", "Global people search"));

        updateMemberList();
    }

    void SelectionContactsForConference::onCurrentMemberClicked(const QString& _aimid)
    {
        if (!_aimid.isEmpty() && _aimid != MyInfo()->aimId())
            deleteMemberDialog(conferenceMembersModel_, QString(), _aimid, regim_, this);
    }

    void SelectionContactsForConference::onVoipCallNameChanged(const voip_manager::ContactsList& _contacts)
    {
        updateMemberList();
    }

    void SelectionContactsForConference::updateMemberList()
    {
        if (conferenceMembersModel_)
        {
            auto chatInfo = std::make_shared<Data::ChatInfo>();

            const auto& currentCallContacts = Ui::GetDispatcher()->getVoipController().currentCallContacts();
            chatInfo->Members_.reserve(currentCallContacts.size());
            for (const voip_manager::Contact& contact : currentCallContacts)
            {
                Data::ChatMemberInfo memberInfo;
                memberInfo.AimId_ = QString::fromStdString(contact.contact);
                chatInfo->Members_.push_back(std::move(memberInfo));
            }

            Data::ChatMemberInfo myInfo;
            myInfo.AimId_ = MyInfo()->aimId();
            chatInfo->Members_.push_back(std::move(myInfo));

            chatInfo->MembersCount_ = currentCallContacts.size() + 1;

            conferenceMembersModel_->updateInfo(chatInfo);

            if (curMembers_)
                curMembers_->updateConferenceListSize();
        }

        updateMaxSelectedCount();
        UpdateMembers();
    }

    void SelectionContactsForConference::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
    {
        if (_contactEx.current_call)
            reject();
    }

    // Set maximum restriction for selected item count. Used for video conference.
    void SelectionContactsForConference::setMaximumSelectedCount(int _number)
    {
        videoConferenceMaxUsers_ = _number;
        updateMaxSelectedCount();
    }

    void SelectionContactsForConference::updateMaxSelectedCount()
    {
        if (conferenceMembersModel_)
        {
            if (videoConferenceMaxUsers_ >= 0)
                SelectContactsWidget::setMaximumSelectedCount(qMax(videoConferenceMaxUsers_ - conferenceMembersModel_->getMembersCount(), 0));
        }
        else
        {
            SelectContactsWidget::setMaximumSelectedCount(videoConferenceMaxUsers_);
        }
    }

    void SelectionContactsForConference::updateSize()
    {
        if (parentWidget())
            mainDialog_->updateSize();
    }

    bool SelectionContactsForConference::show()
    {
        bool res = false;
        auto currentVisibleInstance = currentVisibleInstance_;
        currentVisibleInstance_ = this;

        if (currentVisibleInstance)
            currentVisibleInstance->mainDialog_->reject();

        res = SelectContactsWidget::show();

        if (currentVisibleInstance_ == this)
            currentVisibleInstance_ = nullptr;
        return res;
    }

    QPointer<CurrentConferenceMembers> SelectionContactsForConference::createCurMembersPtr()
    {
        if (!curMembers_)
        {
            curMembers_ = new CurrentConferenceMembers(this, conferenceMembersModel_, chatRoomCall_);
            curMembers_->updateConferenceListSize();
            connect(curMembers_, &CurrentConferenceMembers::memberClicked, this, &SelectionContactsForConference::onCurrentMemberClicked);
        }
        return curMembers_;
    }

    void SelectionContactsForConference::clearCurMembersPtr()
    {
        curMembers_.clear();
    }


    ContactListItemHorizontalDelegate::ContactListItemHorizontalDelegate(QObject* _parent)
        : AbstractItemDelegateWithRegim(_parent)
    {
    }

    void ContactListItemHorizontalDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        bool isOfficial = false, isMuted = false, isOnline = false  ;

        const auto aimId = Logic::aimIdFromIndex(_index);
        if (!aimId.isEmpty())
        {
            isOfficial = Logic::GetFriendlyContainer()->getOfficial(aimId);
            if (aimId != Ui::MyInfo()->aimId())
                isMuted = Logic::getContactListModel()->isMuted(aimId);
            isOnline = Logic::GetLastseenContainer()->isOnline(aimId);
        }

        auto isDefault = false;
        const auto &avatar = Logic::GetAvatarStorage()->GetRounded(
            aimId,
            Logic::GetFriendlyContainer()->getFriendly(aimId),
            Utils::scale_bitmap(::Ui::GetContactListParams().getAvatarSize()),
            isDefault,
            false /* _regenerate */,
            ::Ui::GetContactListParams().isCL());

        const auto contactList = ::Ui::GetContactListParams();
        const QPoint pos(Utils::scale_value(16), contactList.getAvatarY());
        {
            Utils::PainterSaver ps(*_painter);
            _painter->setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing | QPainter::TextAntialiasing);
            auto topLeft = _option.rect.topLeft();
            topLeft.rx() -= Utils::scale_value(4);
            _painter->translate(topLeft);

            Utils::drawAvatarWithBadge(*_painter, pos, avatar, isOfficial, Utils::getStatusBadge(aimId, avatar.width()), isMuted, false, isOnline, true);
        }
    }

    QSize ContactListItemHorizontalDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
    {
        return QSize(GetContactListParams().getAvatarSize() + Utils::scale_value(16), GetContactListParams().getAvatarSize());
    }

    void ContactListItemHorizontalDelegate::setFixedWidth(int _width)
    {
        viewParams_.fixedWidth_ = _width;
    }

    void ContactListItemHorizontalDelegate::setLeftMargin(int _margin)
    {
        viewParams_.leftMargin_ = _margin;
    }

    void ContactListItemHorizontalDelegate::setRightMargin(int _margin)
    {
        viewParams_.rightMargin_ = _margin;
    }

    void ContactListItemHorizontalDelegate::setRegim(int _regim)
    {
        viewParams_.regim_ = _regim;
    }

    ConferenceSearchModel::ConferenceSearchModel(Logic::AbstractSearchModel* _sourceModel)
        : sourceModel_(_sourceModel)
    {
        connect(sourceModel_, &AbstractSearchModel::dataChanged, this, &ConferenceSearchModel::onSourceDataChanged);
    }

    int ConferenceSearchModel::rowCount(const QModelIndex& _index) const
    {
        return sourceModel_->rowCount(_index) + indexShift();
    }

    QVariant ConferenceSearchModel::data(const QModelIndex& _index, int _role) const
    {
        if (!_index.isValid() || (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role)))
            return QVariant();

        const auto cur = _index.row();

        if (cur == curMembersIdx && !inSearchMode_)
            return QVariant::fromValue((QWidget*)membersWidget_);

        if (cur >= rowCount(_index))
            return QVariant();

        const QModelIndex index = sourceModel_->index(cur - indexShift());
        return sourceModel_->data(index, _role);
    }

    void ConferenceSearchModel::setSearchPattern(const QString& _pattern)
    {
        inSearchMode_ = !_pattern.isEmpty();

        if (view_)
        {
            if (inSearchMode_)
            {
                membersWidget_.clear();
                dialog_->clearCurMembersPtr();
                view_->setIndexWidget(curMembersIdx, nullptr);
            }
            else if (!view_->indexWidget(curMembersIdx))
            {
                membersWidget_ = dialog_->createCurMembersPtr();
                connect(membersWidget_, &CurrentConferenceMembers::sizeChanged, this, [this]()
                {
                    const auto idx = index(curMembersIdx);
                    Q_EMIT dataChanged(idx, idx);
                });

                view_->setIndexWidget(curMembersIdx, membersWidget_);
            }
        }

        sourceModel_->setSearchPattern(_pattern);
    }

    bool ConferenceSearchModel::isCheckedItem(const QString& _name) const
    {
        auto chatModel = qobject_cast<Logic::ChatMembersModel*>(sourceModel_);
        if (chatModel)
            return chatModel->isCheckedItem(_name);
        else
            return Logic::getContactListModel()->isCheckedItem(_name);
    }

    void ConferenceSearchModel::setCheckedItem(const QString& _name, bool _checked)
    {
        auto chatModel = qobject_cast<Logic::ChatMembersModel*>(sourceModel_);
        if (chatModel)
            chatModel->setCheckedItem(_name, _checked);
        else
            Logic::getContactListModel()->setCheckedItem(_name, _checked);
    }

    int ConferenceSearchModel::getCheckedItemsCount() const
    {
        auto chatModel = qobject_cast<Logic::ChatMembersModel*>(sourceModel_);
        if (chatModel)
            return chatModel->getCheckedItemsCount();

        return Logic::getContactListModel()->getCheckedItemsCount();
    }

    void ConferenceSearchModel::addTemporarySearchData(const QString& _aimid)
    {
        if (!qobject_cast<Logic::ChatMembersModel*>(sourceModel_) && !Logic::getContactListModel()->contains(_aimid))
        {
            sourceModel_->addTemporarySearchData(_aimid);
            Logic::getContactListModel()->add(_aimid, Logic::GetFriendlyContainer()->getFriendly(_aimid), true);
        }
    }

    void ConferenceSearchModel::onSourceDataChanged(const QModelIndex& _topLeft, const QModelIndex& _bottomRight)
    {
        Q_EMIT dataChanged(mapFromSource(_topLeft), mapFromSource(_bottomRight));
    }

    QModelIndex ConferenceSearchModel::mapFromSource(const QModelIndex& _sourceIndex) const
    {
        if (!_sourceIndex.isValid())
            return QModelIndex();

        return index(_sourceIndex.row() + indexShift());
    }

}
