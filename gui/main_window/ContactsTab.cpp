#include "stdafx.h"

#include "ContactsTab.h"

#include "../common.shared/config/config.h"

#include "contact_list/ContactListWidget.h"
#include "contact_list/ContactListUtils.h"
#include "contact_list/ContactListTopPanel.h"
#include "contact_list/SearchWidget.h"
#include "contact_list/AddContactDialogs.h"
#include "../utils/DragAndDropEventFilter.h"
#include "MainPage.h"
#include "../utils/utils.h"
#include "../utils/MimeDataUtils.h"

#include "../types/contact.h"
#include "../types/search_result.h"
#include "contact_list/ContactListWithHeaders.h"
#include "contact_list/SearchModel.h"
#include "contact_list/ContactListModel.h"
#include "contact_list/IgnoreMembersModel.h"
#include "containers/LastseenContainer.h"

namespace
{
    QString getAimId(const QModelIndex& _index)
    {
        if (const auto model = qobject_cast<const Logic::ContactListWithHeaders*>(_index.model()))
        {
            if (const auto dlg = model->data(_index, Qt::DisplayRole).value<Data::Contact*>())
                return dlg->AimId_;
        }
        else if (const auto model = qobject_cast<const Logic::SearchModel*>(_index.model()))
        {
            if (const auto dlg = model->data(_index, Qt::DisplayRole).value<Data::AbstractSearchResultSptr>())
                return dlg->getAimId();
        }
        return QString();
    }

    QString contactForDrop(const QModelIndex& _index)
    {
        if (!_index.isValid())
            return QString();

        const auto model = qobject_cast<const Logic::CustomAbstractListModel*>(_index.model());
        if (!model || !model->isDropableItem(_index))
            return QString();

        if (const QString aimId = getAimId(_index); !aimId.isEmpty())
        {
            if (Logic::getContactListModel()->isDeleted(aimId) || Logic::getIgnoreModel()->contains(aimId) ||
                Logic::GetLastseenContainer()->getLastSeen(aimId).isBlocked())
                return QString();

            const auto role = Logic::getContactListModel()->getYourRole(aimId);
            if (role != u"notamember" && role != u"readonly")
                return aimId;
        }
        return QString();
    }

    int scrollStep() noexcept { return Utils::scale_value(10); }

    int autoScrollAreaHeight() noexcept { return Utils::scale_value(44); }

    constexpr std::chrono::microseconds scrollInterval { 50 };
} // namespace

namespace Ui
{
    ContactsTab::ContactsTab(QWidget* _parent)
        : QWidget(_parent)
        , state_(LeftPanelState::min)
        , header_(new ContactListTopPanel())
        , contactListWidget_(new ContactListWidget(this, Logic::MembersWidgetRegim::CONTACT_LIST_POPUP, nullptr, nullptr, nullptr, Logic::DrawIcons::NeedDrawIcons))
        , addButton_(config::get().is_on(config::features::add_contact) ? new HeaderTitleBarButton(this) : nullptr)
        , spacer_(new QWidget(this))
    {
        if (addButton_)
        {
            addButton_->setDefaultImage(qsl(":/header/add_user"), Styling::ColorParameter{}, QSize(24, 24));
            addButton_->setCustomToolTip(QT_TRANSLATE_NOOP("tab header", "Add contact"));
            header_->addButtonToRight(addButton_);
            Testing::setAccessibleName(addButton_, qsl("AS ContactsTab addContactButton"));

            connect(addButton_, &CustomButton::clicked,
                this, []() {
                Utils::showAddContactsDialog({ Utils::AddContactDialogs::Initiator::From::ScrContactTab });
            });
        }

        auto listEventFilter = new Utils::DragAndDropEventFilter(this);
        connect(listEventFilter, &Utils::DragAndDropEventFilter::dragPositionUpdate, this, &ContactsTab::onDragPositionUpdate);
        connect(listEventFilter, &Utils::DragAndDropEventFilter::dropMimeData, this, &ContactsTab::dropMimeData);
        contactListWidget_->installEventFilterToView(listEventFilter);

        scrollTimer_ = new QTimer(this);
        scrollTimer_->setInterval(scrollInterval.count());
        scrollTimer_->setSingleShot(false);
        connect(scrollTimer_, &QTimer::timeout, this, &ContactsTab::autoScroll);

        QObject::connect(contactListWidget_, &ContactListWidget::searchEnd, header_->getSearchWidget(), &SearchWidget::searchCompleted);
        contactListWidget_->connectSearchWidget(header_->getSearchWidget());
        Testing::setAccessibleName(contactListWidget_, qsl("AS ContactsTab contactListWidget"));
        QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setContactSearchFocus, this, [this]() {
            header_->getSearchWidget()->setFocus();
            Q_EMIT Utils::InterConnector::instance().hideContactListPlaceholder();
        });


        auto l = Utils::emptyVLayout(this);
        spacer_->setFixedHeight(Utils::scale_value(8));
        l->addWidget(header_);
        l->addWidget(spacer_);
        l->addWidget(contactListWidget_);
    }

    void ContactsTab::setState(const LeftPanelState _state)
    {
        if (state_ != _state)
        {
            state_ = _state;
            const bool isCompact = state_ == LeftPanelState::picture_only;
            header_->setState(state_);

            header_->setVisible(!isCompact);
            spacer_->setVisible(!isCompact);

            contactListWidget_->setPictureOnlyForDelegate(isCompact);
        }
    }

    void ContactsTab::setClWidth(int _width) { contactListWidget_->setWidthForDelegate(_width); }

    ContactListWidget* ContactsTab::getContactListWidget() const { return contactListWidget_; }

    SearchWidget* ContactsTab::getSearchWidget() const { return header_->getSearchWidget(); }

    void ContactsTab::dragPositionUpdate(QPoint _pos, bool _fromScroll)
    {
        QModelIndex index;
        if (!_pos.isNull())
            index = contactListWidget_->getView()->indexAt(_pos);

        if (!contactForDrop(index).isEmpty())
        {
            contactListWidget_->setDragIndexForDelegate(index);

            contactListWidget_->getView()->update(index);
        }
        else
        {
            contactListWidget_->setDragIndexForDelegate({});
        }

        auto scrolledView = contactListWidget_->getView();
        const int scrollAreaHeight = autoScrollAreaHeight();
        const QRect scrollViewRect = scrolledView->rect();
        const QRect rTop { scrollViewRect.topLeft(), QSize { scrollViewRect.width(), scrollAreaHeight } };
        const QRect rBottom { scrollViewRect.bottomLeft() - QPoint { 0, scrollAreaHeight - 1 }, QSize { scrollViewRect.width(), scrollAreaHeight } };

        if (!_pos.isNull() && (rTop.contains(_pos) || rBottom.contains(_pos)))
        {
            scrollMultipler_ = (rTop.contains(_pos)) ? 1 : -1;
            scrollTimer_->start();
        }
        else
        {
            scrollTimer_->stop();
        }

        if (!_fromScroll)
            lastDragPos_ = _pos;

        scrolledView->update();
    }

    void ContactsTab::onDragPositionUpdate(QPoint _pos) { dragPositionUpdate(_pos, false); }

    void ContactsTab::dropMimeData(QPoint _pos, const QMimeData* _mimeData)
    {
        if (!_pos.isNull())
        {
            const QModelIndex index = contactListWidget_->getView()->indexAt(_pos);
            if (const QString contact = contactForDrop(index); !contact.isEmpty())
            {
                if (contact != Logic::getContactListModel()->selectedContact())
                    Q_EMIT Logic::getContactListModel()->select(contact, -1);

                MimeData::sendToChat(_mimeData, contact);
                contactListWidget_->getView()->update(index);
            }
        }
        contactListWidget_->setDragIndexForDelegate({});
    }

    void ContactsTab::autoScroll()
    {
        auto scrollBar = contactListWidget_->getView()->verticalScrollBar();
        scrollBar->setValue(scrollBar->value() - (scrollStep() * scrollMultipler_));
        dragPositionUpdate(lastDragPos_, true);
    }
} // namespace Ui
