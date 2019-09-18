#include "stdafx.h"
#include "SelectionContactsForConference.h"

#include "../main_window/contact_list/ContactList.h"
#include "../main_window/contact_list/ContactListUtils.h"
#include "../main_window/contact_list/ContactListWidget.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/contact_list/ChatMembersModel.h"
#include "../main_window/contact_list/SearchWidget.h"
#include "../main_window/contact_list/ContactListItemDelegate.h"
#include "../main_window/contact_list/Common.h"
#include "../main_window/contact_list/SearchMembersModel.h"
#include "../main_window/contact_list/SearchModel.h"

#include "../main_window/friendly/FriendlyContainer.h"
#include "../main_window/GroupChatOperations.h"
#include "../main_window/sidebar/SidebarUtils.h"

#include "../../core/Voip/VoipManagerDefines.h"

#include "../core_dispatcher.h"
#include "../my_info.h"

#include "../cache/avatars/AvatarStorage.h"
#include "../controls/GeneralDialog.h"
#include "../controls/CustomButton.h"
#include "../controls/ClickWidget.h"
#include "../utils/InterConnector.h"
#include "../styles/ThemeParameters.h"

#include "CommonUI.h"

namespace
{
    QSize getArrowSize()
    {
        return QSize(16, 16);
    }
}

namespace Ui
{
    SelectionContactsForConference* SelectionContactsForConference::currentVisibleInstance_ = nullptr;

    SelectionContactsForConference::SelectionContactsForConference(
        Logic::ChatMembersModel* _conferenceMembersModel,
        Logic::ChatMembersModel* _otherContactsModel,
        const QString& _labelText,
        QWidget* _parent,
        ConferenceSearchMember& usersSearchModel,
        bool _handleKeyPressEvents)
        : SelectContactsWidget(
            _conferenceMembersModel,
            Logic::MembersWidgetRegim::VIDEO_CONFERENCE,
            _labelText,
            QString(),
            _parent,
            _handleKeyPressEvents,
            &usersSearchModel),
        conferenceMembersModel_(_conferenceMembersModel),
        videoConferenceMaxUsers_(-1)
    {
        initCurrentMembersWidget();

        contactList_->setIndexWidget(0, curMembersHost_);
        usersSearchModel.setWidget(curMembersHost_);

        mainDialog_->addButtonsPair(QT_TRANSLATE_NOOP("popup_window", "CANCEL"), QT_TRANSLATE_NOOP("popup_window", "ADD"), true);

        connect(conferenceContacts_, &FocusableListView::clicked, this, &SelectionContactsForConference::itemClicked, Qt::QueuedConnection);

        // Update other contacts list.
        connect(_otherContactsModel, &Logic::ChatMembersModel::dataChanged, this, [this]() { searchModel_->repeatSearch(); });

        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallNameChanged, this, &SelectionContactsForConference::onVoipCallNameChanged);
        connect(&Ui::GetDispatcher()->getVoipController(), &voip_proxy::VoipController::onVoipCallDestroyed, this, &SelectionContactsForConference::onVoipCallDestroyed);

        connect(membersLabelHost_, &ClickableWidget::clicked, this, &SelectionContactsForConference::clickConferenceMembers);
        connect(memberArrowDown_, &CustomButton::clicked, this, &SelectionContactsForConference::clickConferenceMembers);
        connect(memberArrowUp_,   &CustomButton::clicked, this, &SelectionContactsForConference::clickConferenceMembers);

        clickConferenceMembers();
        updateMemberList();
    }

    void SelectionContactsForConference::itemClicked(const QModelIndex& _current)
    {
        if (!(QApplication::mouseButtons() & Qt::RightButton /*|| tapAndHoldModifier()*/))
        {
            if (const auto aimid = Logic::aimIdFromIndex(_current); !aimid.isEmpty())
                deleteMemberDialog(conferenceMembersModel_, QString(), aimid, regim_, this);
        }
    }

    void SelectionContactsForConference::onVoipCallNameChanged(const voip_manager::ContactsList& _contacts)
    {
        updateMemberList();
    }

    void SelectionContactsForConference::updateMemberList()
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

        chatInfo->MembersCount_ = currentCallContacts.size();

        conferenceMembersModel_->updateInfo(chatInfo);

        updateConferenceListSize();

        updateMaxSelectedCount();
        UpdateMembers();
        conferenceContacts_->update();
    }

    void SelectionContactsForConference::updateConferenceListSize()
    {
        if (conferenceContacts_->flow() == QListView::TopToBottom)
        {
            conferenceContacts_->setFixedHeight(::Ui::GetContactListParams().itemHeight() * conferenceMembersModel_->rowCount());
        }
        else
        {
            // All in one row.
            conferenceContacts_->setFixedHeight(::Ui::GetContactListParams().itemHeight());
        }

        // Force update first element in list view.
        conferenceContacts_->update(conferenceMembersModel_->index(0));
    }

    void SelectionContactsForConference::onVoipCallDestroyed(const voip_manager::ContactEx& _contactEx)
    {
        if (!_contactEx.incoming && _contactEx.connection_count <= 1)
        {
            reject();
        }
    }

    // Set maximum restriction for selected item count. Used for video conference.
    void SelectionContactsForConference::setMaximumSelectedCount(int number)
    {
        videoConferenceMaxUsers_ = number;
        updateMaxSelectedCount();
    }

    void SelectionContactsForConference::updateMaxSelectedCount()
    {
        // -1 is your own contact.
        if (videoConferenceMaxUsers_ >= 0)
        {
            memberLabel_->setText(QT_TRANSLATE_NOOP("voip_pages", "MEMBERS") % ql1s(" (") % QString::number(conferenceMembersModel_->getMembersCount()) % ql1c('/') % QString::number(videoConferenceMaxUsers_ - 1) % ql1c(')'));
            SelectContactsWidget::setMaximumSelectedCount(qMax(videoConferenceMaxUsers_ - conferenceMembersModel_->getMembersCount() - 1, 0));
        }
    }

    void SelectionContactsForConference::clickConferenceMembers()
    {
        // TODO: change
        if (conferenceContacts_->flow() == QListView::TopToBottom)
        {
            conferenceContacts_->setFlow(QListView::LeftToRight);
            conferenceContacts_->setItemDelegate(new ContactListItemHorizontalDelegate(this));

            memberArrowDown_->show();
            memberArrowUp_->hide();
        }
        else
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

    void SelectionContactsForConference::initCurrentMembersWidget()
    {
        curMembersHost_ = new QWidget(this);

        membersLabelHost_ = new ClickableWidget(curMembersHost_);
        membersLabelHost_->setContentsMargins(Utils::scale_value(16), Utils::scale_value(12), Utils::scale_value(20), Utils::scale_value(4));
        Testing::setAccessibleName(membersLabelHost_, qsl("AS scfc membersLabelHost"));

        memberLabel_ = new QLabel(QT_TRANSLATE_NOOP("voip_pages", "MEMBERS"), membersLabelHost_);
        memberLabel_->setContentsMargins(0, 0, 0, 0);
        QPalette p;
        p.setColor(QPalette::Text, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        memberLabel_->setPalette(p);
        memberLabel_->setFont(Fonts::appFontScaled(12, Fonts::FontWeight::SemiBold));
        Testing::setAccessibleName(memberLabel_, qsl("AS scfc memberLabel_"));

        memberArrowDown_ = new CustomButton(membersLabelHost_, qsl(":/controls/down_icon"), getArrowSize());
        memberArrowDown_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        Testing::setAccessibleName(memberArrowDown_, qsl("AS scfc memberArrowDown_"));
        Styling::Buttons::setButtonDefaultColors(memberArrowDown_);

        memberArrowUp_ = new CustomButton(membersLabelHost_, qsl(":/controls/top_icon"), getArrowSize());
        memberArrowUp_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        Testing::setAccessibleName(memberArrowUp_, qsl("AS scfc memberArrowUp_"));
        Styling::Buttons::setButtonDefaultColors(memberArrowUp_);
        memberArrowUp_->hide();

        auto membersLabelLayout = Utils::emptyHLayout(membersLabelHost_);
        membersLabelLayout->addWidget(memberLabel_);
        membersLabelLayout->addSpacing(Utils::scale_value(6));
        membersLabelLayout->addWidget(memberArrowDown_, 0, Qt::AlignTop);
        membersLabelLayout->addWidget(memberArrowUp_, 0, Qt::AlignTop);
        membersLabelLayout->addStretch();

        auto clDelegate = new Logic::ContactListItemDelegate(this, Logic::MembersWidgetRegim::VIDEO_CONFERENCE, conferenceMembersModel_);
        conferenceContacts_ = CreateFocusableViewAndSetTrScrollBar(curMembersHost_);
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
        Testing::setAccessibleName(conferenceContacts_, qsl("AS scfc conferenceContacts_"));

        auto othersLabel = new QLabel(QT_TRANSLATE_NOOP("voip_pages", "OTHERS"), curMembersHost_);
        othersLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        othersLabel->setContentsMargins(Utils::scale_value(16), Utils::scale_value(12), Utils::scale_value(20), Utils::scale_value(4));
        othersLabel->setMinimumHeight(2 * Utils::scale_value(12) + Utils::scale_value(4)); //Qt ignores our Margins if zoom is 200%. This line fix this problem.
        QPalette pal;
        pal.setColor(QPalette::Text, Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID));
        othersLabel->setPalette(pal);
        othersLabel->setFont(Fonts::appFontScaled(12, Fonts::FontWeight::SemiBold));
        Testing::setAccessibleName(othersLabel, qsl("AS scfc othersLabel"));

        auto verLayout = Utils::emptyVLayout(curMembersHost_);
        verLayout->addWidget(membersLabelHost_);
        verLayout->addWidget(conferenceContacts_);
        verLayout->addWidget(othersLabel);
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
        {
            currentVisibleInstance->mainDialog_->reject();
        }

        res = SelectContactsWidget::show();

        if (currentVisibleInstance_ == this)
        {
            currentVisibleInstance_ = nullptr;
        }

        return res;
    }


    ContactsForVideoConference::ContactsForVideoConference(QObject* parent, const Logic::ChatMembersModel& videoConferenceModel)
        : Logic::ChatMembersModel(parent), videoConferenceModel_(videoConferenceModel)
        , searchModel_(new Logic::SearchModel(this))
    {
        searchModel_->setSearchInDialogs(false);
        searchModel_->setExcludeChats(true);
        searchModel_->setHideReadonly(false);
        searchModel_->setCategoriesEnabled(false);
        searchModel_->setServerSearchEnabled(false);

        connect(&videoConferenceModel_, &Logic::ChatMembersModel::dataChanged, this, &ContactsForVideoConference::updateMemberList);
        updateMemberList();
    }

    void ContactsForVideoConference::updateMemberList()
    {
        if (allMembers_.Members_.empty())
        {
            // Fetch all members first.
            connection_ = connect(searchModel_, &QAbstractItemModel::dataChanged, this, &ContactsForVideoConference::allMembersDataChanged);
            searchModel_->setSearchPattern(QString());
        }
        else
        {
            auto chatInfo = std::make_shared<Data::ChatInfo>();
            chatInfo->Members_.reserve(allMembers_.Members_.size());

            for (const auto& memberInfo : allMembers_.Members_)
            {
                if (!videoConferenceModel_.contains(memberInfo.AimId_))
                    chatInfo->Members_.push_back(memberInfo);
            }
            chatInfo->MembersCount_ = chatInfo->Members_.size();

            updateInfo(chatInfo);
        }
    }

    void ContactsForVideoConference::allMembersDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
    {
        allMembers_.Members_.clear();

        int nCount = 0;
        for (int i = 0; i < searchModel_->rowCount(); i++)
        {
            const auto dataContact = searchModel_->index(i).data().value<Data::AbstractSearchResultSptr>();
            if (dataContact)
            {
                Data::ChatMemberInfo memberInfo;
                memberInfo.AimId_ = dataContact->getAimId();
                allMembers_.Members_.push_back(memberInfo);
                nCount++;
            }
        }

        allMembers_.MembersCount_ = nCount;

        if (allMembers_.MembersCount_ > 0)
        {
            updateMemberList();
            disconnect(connection_);
        }
    }

    ContactListItemHorizontalDelegate::ContactListItemHorizontalDelegate(QObject* parent)
        : AbstractItemDelegateWithRegim(parent)
    {
    }

    ContactListItemHorizontalDelegate::~ContactListItemHorizontalDelegate()
    {
    }

    void ContactListItemHorizontalDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        bool isOfficial = false;
        bool isMuted = false;

        const auto aimId = Logic::aimIdFromIndex(index);
        if (!aimId.isEmpty())
        {
            isOfficial = Logic::GetFriendlyContainer()->getOfficial(aimId);

            if (aimId != Ui::MyInfo()->aimId())
                isMuted = Logic::getContactListModel()->isMuted(aimId);
        }

        auto isDefault = false;
        const auto &avatar = Logic::GetAvatarStorage()->GetRounded(
            aimId,
            Logic::GetFriendlyContainer()->getFriendly(aimId),
            Utils::scale_bitmap(::Ui::GetContactListParams().getAvatarSize()),
            QString(),
            isDefault,
            false /* _regenerate */,
            ::Ui::GetContactListParams().isCL());

        const auto contactList = ::Ui::GetContactListParams();
        const QPoint pos(Utils::scale_value(16), contactList.getAvatarY());
        {
            Utils::PainterSaver ps(*painter);
            painter->translate(option.rect.topLeft());

            Utils::drawAvatarWithBadge(*painter, pos, *avatar, isOfficial, isMuted, false);
        }
    }

    QSize ContactListItemHorizontalDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex &index) const
    {
        return QSize(::Ui::GetContactListParams().getAvatarSize() + Utils::scale_value(16), ::Ui::GetContactListParams().getAvatarSize());
    }

    void ContactListItemHorizontalDelegate::setFixedWidth(int width)
    {
        viewParams_.fixedWidth_ = width;
    }

    void ContactListItemHorizontalDelegate::setLeftMargin(int margin)
    {
        viewParams_.leftMargin_ = margin;
    }

    void ContactListItemHorizontalDelegate::setRightMargin(int margin)
    {
        viewParams_.rightMargin_ = margin;
    }

    void ContactListItemHorizontalDelegate::setRegim(int _regim)
    {
        viewParams_.regim_ = _regim;
    }


    ConferenceSearchMember::ConferenceSearchMember() : Logic::SearchMembersModel(nullptr), firstElement_(nullptr), bSearchMode_(false) {}

    int ConferenceSearchMember::rowCount(const QModelIndex& _parent) const
    {
        return Logic::SearchMembersModel::rowCount(_parent) + 1;
    }

    QVariant ConferenceSearchMember::data(const QModelIndex& _index, int _role) const
    {
        if (!_index.isValid())
            return QVariant();

        const int cur = _index.row();
        if (cur == 0)
        {
            if (_role != Qt::DisplayRole && !Testing::isAccessibleRole(_role))
                return QVariant();

            if (firstElement_)
                firstElement_->setVisible(!bSearchMode_);

            return QVariant::fromValue(firstElement_);
        }

        if (cur >= rowCount(_index))
            return QVariant();

        const QModelIndex id = index(cur - 1);
        return Logic::SearchMembersModel::data(id, _role);
    }

    void ConferenceSearchMember::setWidget(QWidget* widget)
    {
        firstElement_ = widget;
    }

    void ConferenceSearchMember::setSearchPattern(const QString& _search)
    {
        bSearchMode_ = !_search.isEmpty();

        ::Logic::SearchMembersModel::setSearchPattern(_search);
    }
}

