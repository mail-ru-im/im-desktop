#include "stdafx.h"

#include "Common.h"
#include "ContactList.h"
#include "ContactItem.h"
#include "ContactListModel.h"
#include "ContactListItemDelegate.h"
#include "ContactListWidget.h"
#include "ContactListUtils.h"
#include "ContactListWithHeaders.h"
#include "RecentsModel.h"
#include "RecentItemDelegate.h"
#include "RecentsView.h"
#include "UnknownsModel.h"
#include "UnknownItemDelegate.h"
#include "SearchModel.h"
#include "SearchWidget.h"
#include "../MainWindow.h"
#include "../ContactDialog.h"
#include "../contact_list/ChatMembersModel.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../settings/SettingsTab.h"
#include "../../types/contact.h"
#include "../../types/typing.h"
#include "../../controls/ContextMenu.h"

#include "../../controls/TransparentScrollBar.h"
#include "../../controls/CustomButton.h"
#include "../../controls/HorScrollableView.h"
#include "../../utils/utils.h"
#include "../../utils/stat_utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/features.h"
#include "../../fonts.h"
#include "../Placeholders.h"
#include "AddContactDialogs.h"

#include "../input_widget/InputWidget.h"

#include "styles/ThemeParameters.h"

namespace
{
    const int autoscroll_offset_recents = 68;
    const int autoscroll_offset_cl = 44;
    const int autoscroll_speed_pixels = 10;
    const int autoscroll_timeout = 50;

    const int LEFT_OFFSET = 16;
    const int BACK_HEIGHT = 12;
    const int RECENTS_HEIGHT = 68;

    constexpr std::chrono::milliseconds dragActivateDelay = std::chrono::milliseconds(500);

    QString contactForDrop(const QModelIndex& _index)
    {
        if (!_index.isValid())
            return QString();

        const auto model = qobject_cast<const Logic::CustomAbstractListModel*>(_index.model());
        if (!model || model->isServiceItem(_index))
            return QString();

        const auto dlg = model->data(_index, Qt::DisplayRole).value<Data::DlgState>();
        if (!dlg.AimId_.isEmpty())
        {
            const auto role = Logic::getContactListModel()->getYourRole(dlg.AimId_);
            if (role != ql1s("notamember") && role != ql1s("readonly"))
                return dlg.AimId_;
        }
        return QString();
    }
}

namespace Ui
{
    RCLEventFilter::RCLEventFilter(ContactList* _cl)
        : QObject(_cl)
        , cl_(_cl)
        , dragMouseoverTimer_(new QTimer(this))
    {
        dragMouseoverTimer_->setInterval(dragActivateDelay.count());
        dragMouseoverTimer_->setSingleShot(true);
        connect(dragMouseoverTimer_, &QTimer::timeout, this, &RCLEventFilter::onTimer);
    }

    void RCLEventFilter::onTimer()
    {
        Utils::InterConnector::instance().getMainWindow()->activate();
        Utils::InterConnector::instance().getMainWindow()->closeGallery();
    }

    bool RCLEventFilter::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_event->type() == QEvent::Gesture)
        {
            QGestureEvent* guesture  = static_cast<QGestureEvent*>(_event);
            if (QGesture *tapandhold = guesture->gesture(Qt::TapAndHoldGesture))
            {
                if (tapandhold->hasHotSpot() && tapandhold->state() == Qt::GestureFinished)
                {
                    cl_->triggerTapAndHold(true);
                    guesture->accept(Qt::TapAndHoldGesture);
                }
            }
        }
        if (_event->type() == QEvent::DragEnter || _event->type() == QEvent::DragMove)
        {
            dragMouseoverTimer_->start();

            QDropEvent* de = static_cast<QDropEvent*>(_event);
            auto acceptDrop = de->mimeData() && (de->mimeData()->hasUrls() || Utils::isMimeDataWithImage(de->mimeData()));
            if (QApplication::activeModalWidget() == nullptr && acceptDrop)
            {
                de->acceptProposedAction();
                cl_->dragPositionUpdate(de->pos());
            }
            else
            {
                de->setDropAction(Qt::IgnoreAction);
            }
            return true;
        }
        if (_event->type() == QEvent::DragLeave)
        {
            dragMouseoverTimer_->stop();
            cl_->dragPositionUpdate(QPoint());
            return true;
        }
        if (_event->type() == QEvent::Drop)
        {
            dragMouseoverTimer_->stop();
            onTimer();

            QDropEvent* e = static_cast<QDropEvent*>(_event);
            cl_->dropMimeData(e->pos(), e->mimeData());
            e->acceptProposedAction();
            cl_->dragPositionUpdate(QPoint());
            return true;
        }

        if (_event->type() == QEvent::MouseButtonDblClick)
        {
            _event->ignore();
            return true;
        }

        if (_event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* e = static_cast<QMouseEvent*>(_event);
            if (e->button() == Qt::LeftButton)
            {
                cl_->triggerTapAndHold(false);
            }
        }

        return QObject::eventFilter(_obj, _event);
    }

    ContactList::ContactList(QWidget* _parent)
        : QWidget(_parent)
        , listEventFilter_(new RCLEventFilter(this))
        , recentsPlaceholder_(nullptr)
        , popupMenu_(nullptr)
        , recentsDelegate_(new Ui::RecentItemDelegate(this))
        , unknownsDelegate_(new Logic::UnknownItemDelegate(this))
        , clDelegate_(new Logic::ContactListItemDelegate(this, Logic::MembersWidgetRegim::CONTACT_LIST))
        , contactListWidget_(new ContactListWidget(this, Logic::MembersWidgetRegim::CONTACT_LIST, nullptr))
        , clWithHeaders_(new Logic::ContactListWithHeaders(this, Logic::getContactListModel()))
        , scrolledView_(nullptr)
        , scrollMultipler_(1)
        , transitionLabel_(nullptr)
        , transitionAnim_(nullptr)
        , currentTab_(RECENTS)
        , prevTab_(RECENTS)
        , tapAndHold_(false)
        , pictureOnlyView_(false)
        , nextSelectWithOffset_(false)
    {
        auto mainLayout = Utils::emptyVLayout(this);
        mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
        stackedWidget_ = new QStackedWidget(this);

        recentsPage_ = new QWidget();
        recentsLayout_ = Utils::emptyVLayout(recentsPage_);

        recentsView_ = RecentsView::create(recentsPage_);
        recentsContainer_ = new QWidget(this);
        recentsContainerLayout_ = Utils::emptyVLayout(recentsContainer_);

        connect(recentsView_->verticalScrollBar(), &QScrollBar::actionTriggered, this, &ContactList::recentsScrollActionTriggered, Qt::DirectConnection);

        Testing::setAccessibleName(recentsView_, qsl("AS recentsView_"));
        recentsContainerLayout_->addWidget(recentsView_);
        Testing::setAccessibleName(recentsContainer_, qsl("AS recentsContainer_"));
        recentsLayout_->addWidget(recentsContainer_);

        Testing::setAccessibleName(recentsPage_, qsl("AS recentsPage_"));
        stackedWidget_->addWidget(recentsPage_);

        Testing::setAccessibleName(contactListWidget_, qsl("AS contactListWidget_"));
        stackedWidget_->addWidget(contactListWidget_);
        Testing::setAccessibleName(stackedWidget_, qsl("AS stackedWidget_"));
        mainLayout->addWidget(stackedWidget_);

        stackedWidget_->setCurrentIndex(0);

        recentsView_->setAttribute(Qt::WA_MacShowFocusRect, false);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showRecentsPlaceholder, this, &ContactList::showNoRecentsYet);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideRecentsPlaceholder, this, &ContactList::hideNoRecentsYet);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::myProfileBack, this, &ContactList::myProfileBack);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::disableSearchInDialog, this, [this]()
        {
            setSearchMode(!contactListWidget_->getSearchModel()->getSearchPattern().isEmpty());
        });

        Utils::grabTouchWidget(recentsView_->viewport(), true);

        contactListWidget_->installEventFilterToView(listEventFilter_);
        recentsView_->viewport()->grabGesture(Qt::TapAndHoldGesture);
        recentsView_->viewport()->installEventFilter(listEventFilter_);

        connect(QScroller::scroller(recentsView_->viewport()), &QScroller::stateChanged, this, &ContactList::touchScrollStateChangedRecents, Qt::QueuedConnection);

        connect(Logic::getContactListModel(), &Logic::ContactListModel::select, this, [this](const QString& _aimId, const qint64 _msgId, const Logic::UpdateChatSelection _mode)
        {
            contactListWidget_->select(_aimId, _msgId, {}, _mode);
        });

        connect(contactListWidget_->getView(), &Ui::FocusableListView::clicked, this, &ContactList::statsCLItemPressed);
        connect(contactListWidget_, &ContactListWidget::itemClicked, recentsDelegate_, &Ui::RecentItemDelegate::onItemClicked);

        connect(this, &ContactList::groupClicked, Logic::getContactListModel(), &Logic::ContactListModel::groupClicked);

        Logic::getUnknownsModel(); // just initialization
        recentsView_->setModel(Logic::getRecentsModel());
        recentsView_->setItemDelegate(recentsDelegate_);
        connect(recentsView_, &Ui::ListViewWithTrScrollBar::pressed, this, &ContactList::itemPressed);
        connect(recentsView_, &Ui::ListViewWithTrScrollBar::clicked, this, &ContactList::itemClicked);
        connect(recentsView_, &Ui::ListViewWithTrScrollBar::clicked, this, &ContactList::statsRecentItemPressed);
        connect(recentsView_, &Ui::ListViewWithTrScrollBar::mousePosChanged, this, &ContactList::onMouseMoved);

        recentsView_->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);

        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updatedSize, this, [=]()
        {
            if (recentsView_ && isRecentsOpen())
                Logic::getRecentsModel()->refresh();
        });

        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updatedMessages, this, [=]()
        {
            if (recentsView_ && isRecentsOpen())
                Logic::getRecentsModel()->refreshUnknownMessages();
        });

        connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, [=]()
        {
            if (recentsView_ && isUnknownsOpen())
                emit Logic::getUnknownsModel()->refresh();
        });

        connect(Logic::getRecentsModel(), &Logic::RecentsModel::selectContact, contactListWidget_, Utils::QOverload<const QString&>::of(&ContactListWidget::select));
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::selectContact, contactListWidget_, Utils::QOverload<const QString&>::of(&ContactListWidget::select));

        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::dlgStateChanged, unknownsDelegate_, &Logic::UnknownItemDelegate::dlgStateChanged);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStateChanged, recentsDelegate_, &Ui::RecentItemDelegate::dlgStateChanged);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::refreshAll, unknownsDelegate_, &Logic::UnknownItemDelegate::refreshAll);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::refreshAll, recentsDelegate_, &Ui::RecentItemDelegate::refreshAll);

        if (contactListWidget_->getRegim() == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            connect(get_gui_settings(), &Ui::qt_gui_settings::received, this, &ContactList::guiSettingsChanged);
            guiSettingsChanged();
        }

        scrollTimer_ = new QTimer(this);
        scrollTimer_->setInterval(autoscroll_timeout);
        scrollTimer_->setSingleShot(false);
        connect(scrollTimer_, &QTimer::timeout, this, &ContactList::autoScroll);

        connect(GetDispatcher(), &core_dispatcher::messagesReceived, this, &ContactList::messagesReceived);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsGoSeeThem, this, &ContactList::switchToUnknowns);

        connect(contactListWidget_, &Ui::ContactListWidget::itemSelected, this, &ContactList::itemSelected);
        connect(contactListWidget_, &Ui::ContactListWidget::changeSelected, this, &ContactList::changeSelected);

        connect(Utils::InterConnector::instance().getMainWindow(), &MainWindow::upKeyPressed, this, &ContactList::searchUpPressed);
        connect(Utils::InterConnector::instance().getMainWindow(), &MainWindow::downKeyPressed, this, &ContactList::searchDownPressed);
        connect(Utils::InterConnector::instance().getMainWindow(), &MainWindow::enterKeyPressed, this, &ContactList::searchEnterPressed);

        connect(qApp, &QApplication::focusChanged, this, [this](QWidget* _old, QWidget* _new)
        {
            if (_new)
                dropKeyboardFocus();
        });
    }

    ContactList::~ContactList()
    {

    }

    void ContactList::setSearchMode(bool _search)
    {
        if (isSearchMode() == _search)
            return;

        if (_search)
        {
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_search);

            if (contactListWidget_->getSearchInDialog())
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_search_dialog_first_symb_typed);
        }

        if (_search)
        {
            stackedWidget_->setCurrentIndex(SEARCH);
            contactListWidget_->getSearchModel()->setFocus();
        }
        else
        {
            stackedWidget_->setCurrentIndex(currentTab_);
        }
    }

    bool ContactList::isSearchMode() const
    {
        return stackedWidget_->currentIndex() == SEARCH;
    }

    void ContactList::itemClicked(const QModelIndex& _current)
    {
        if (qobject_cast<const Logic::ContactListWithHeaders*>(_current.model()) || qobject_cast<const Logic::ContactListModel*>(_current.model()))
        {
            if (const auto cont = _current.data().value<Data::Contact*>())
            {
                if (cont->GetType() == Data::DROPDOWN_BUTTON)
                {
                    if (cont->AimId_ == ql1s("~addContact~"))
                    {
                        emit addContactClicked(QPrivateSignal());
                        Utils::showAddContactsDialog({ Utils::AddContactDialogs::Initiator::From::ConlistScr });
                    }
                    else if (cont->AimId_ == ql1s("~newGroupchat~"))
                    {
                        emit createGroupChatClicked();

                        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_createbutton_click);
                    }
                    else if (cont->AimId_ == ql1s("~newChannel~"))
                    {
                        emit createChannelClicked();
                    }
                    return;
                }
                else if (cont->GetType() == Data::GROUP)
                {
                    emit groupClicked(cont->GroupId_);
                    return;
                }
            }
        }

        const auto rcntModel = qobject_cast<const Logic::RecentsModel *>(_current.model());
        const auto isLeftButton = QApplication::mouseButtons() & Qt::LeftButton || QApplication::mouseButtons() == Qt::NoButton;
        if (isLeftButton && rcntModel && rcntModel->isServiceItem(_current))
        {
            if (rcntModel->isFavoritesGroupButton(_current))
                Logic::getRecentsModel()->toggleFavoritesVisible();
            else if (rcntModel->isUnimportantGroupButton(_current))
                Logic::getRecentsModel()->toggleUnimportantVisible();
            return;
        }

        const auto unkModel = qobject_cast<const Logic::UnknownsModel *>(_current.model());
        const auto changeSelection =
            !(QApplication::mouseButtons() & Qt::RightButton || tapAndHoldModifier())
            && !(unkModel && unkModel->isServiceItem(_current));

        if (changeSelection)
        {
            emit Utils::InterConnector::instance().clearDialogHistory();
            contactListWidget_->selectionChanged(_current);
        }
    }

    void ContactList::itemPressed(const QModelIndex& _current)
    {
        const auto rcntModel = qobject_cast<const Logic::RecentsModel *>(_current.model());
        const auto unkModel = qobject_cast<const Logic::UnknownsModel *>(_current.model());

        const auto isLeftButton = QApplication::mouseButtons() & Qt::LeftButton || QApplication::mouseButtons() == Qt::NoButton;
        if (isLeftButton)
        {
            if (unkModel && !unkModel->isServiceItem(_current))
            {
                const auto rect = recentsView_->visualRect(_current);
                const auto pos1 = recentsView_->mapFromGlobal(QCursor::pos());
                if (rect.contains(pos1))
                {
                    QPoint pos(pos1.x(), pos1.y() - rect.y());
                    if (const auto aimId = Logic::aimIdFromIndex(_current); !aimId.isEmpty())
                    {
                        if (unknownsDelegate_->isInRemoveContactFrame(pos))
                        {
                            Logic::getContactListModel()->removeContactFromCL(aimId);
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::unknowns_close);
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_unknown_senders_close);
                            return;
                        }
                    }
                }
            }
        }

        if (QApplication::mouseButtons() & Qt::RightButton || tapAndHoldModifier())
        {
            const auto model = qobject_cast<const Logic::CustomAbstractListModel*>(_current.model());
            if (model && model->isServiceItem(_current))
                return;

            triggerTapAndHold(false);

            if (rcntModel || unkModel)
            {
                showRecentsPopupMenu(_current);
            }
            else if (const auto srchModel = qobject_cast<const Logic::SearchModel *>(_current.model()))
            {
                const auto searchRes = _current.data().value<Data::AbstractSearchResultSptr>();
                if (searchRes && searchRes->isContact())
                {
                    const auto cont = Logic::getContactListModel()->getContactItem(searchRes->getAimId());
                    if (cont)
                        contactListWidget_->showContactsPopupMenu(cont->get_aimid(), cont->is_chat());
                }
            }
            else
            {
                auto cont = _current.data(Qt::DisplayRole).value<Data::Contact*>();
                if (cont)
                    contactListWidget_->showContactsPopupMenu(cont->AimId_, cont->Is_chat_);
            }
        }
        else if (QApplication::mouseButtons() & Qt::MiddleButton && rcntModel)
        {
            if (!rcntModel->isServiceItem(_current))
            {
                Data::DlgState dlg = _current.data(Qt::DisplayRole).value<Data::DlgState>();
                Logic::getRecentsModel()->hideChat(dlg.AimId_);
            }
        }
    }

    void ContactList::statsRecentItemPressed(const QModelIndex& _current)
    {
        auto model = qobject_cast<const Logic::CustomAbstractListModel*>(_current.model());
        if (!model || model->isServiceItem(_current))
            return;

        std::string from;

        if (isRecentsOpen())
        {
            from = "Recents";
        }
        else if (isUnknownsOpen())
        {
            from = "Unknowns";
        }
        else if (isCLWithHeadersOpen())
        {
            if (clWithHeaders_->isInTopContacts(_current))
                from = "Top";
            else
                from = "All_Contacts";
        }

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_chat, { { "From", std::move(from) } });
    }

    void ContactList::statsCLItemPressed(const QModelIndex& _current)
    {
        if (!isSearchMode())
            return;

        auto model = qobject_cast<const Logic::SearchModel*>(_current.model());
        if (!model || model->isServiceItem(_current))
            return;

        const auto searchRes = _current.data().value<Data::AbstractSearchResultSptr>();
        if (!searchRes)
            return;

        if (searchRes->isContact())
        {
            if (searchRes->isLocalResult_)
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_chat, { { "From", "Search_Local" } });
        }
        else if (searchRes->isMessage())
        {
            if (contactListWidget_->getSearchInDialog())
            {
                const auto formatRange = [](const int _number)
                {
                    std::string statNum;
                    if (_number < 6)
                        statNum = std::to_string(_number);
                    else if (_number < 10)
                        statNum = "6-9";
                    else if (_number < 50)
                        statNum = "10-49";
                    else if (_number < 101)
                        statNum = "50-100";
                    else
                        statNum = "more 100";

                    return statNum;
                };

                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_search_dialog_openmessage, { { "search_res_count", formatRange(model->getTotalMessagesCount()) } });

                auto hdrRow = 0;
                const auto messagesHdrNdx = model->contactIndex(Logic::SearchModel::getMessagesAimId());
                if (messagesHdrNdx.isValid())
                    hdrRow = messagesHdrNdx.row();
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_search_dialog_item_click, { { "position", formatRange(_current.row() - hdrRow) } });
            }
            else
            {
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_search_openmessage);
            }
        }
    }

    void ContactList::changeTab(CurrentTab _currTab, bool silent)
    {
        if (currentTab_ == _currTab)
            return;

        if (getPictureOnlyView() && (_currTab != RECENTS))
            return;

        if (_currTab != RECENTS)
        {
            emit Utils::InterConnector::instance().unknownsGoBack();
        }

        if (currentTab_ != _currTab)
        {
            prevTab_ = currentTab_;
            currentTab_ = _currTab;
            updateTabState();
        }
        else
        {
            if (isRecentsOpen())
                Logic::getRecentsModel()->sendLastRead();
            else if (isUnknownsOpen())
                Logic::getUnknownsModel()->sendLastRead();
        }

        if (!silent)
            emit tabChanged(currentTab_);
    }

    CurrentTab ContactList::currentTab() const
    {
        return (CurrentTab)currentTab_;
    }

    void ContactList::triggerTapAndHold(bool _value)
    {
        tapAndHold_ = _value;
        contactListWidget_->triggerTapAndHold(_value);
    }

    bool ContactList::tapAndHoldModifier() const
    {
        return tapAndHold_;
    }

    void ContactList::dragPositionUpdate(const QPoint& _pos, bool fromScroll)
    {
        int autoscroll_offset = Utils::scale_value(autoscroll_offset_cl);

        if (isSearchMode())
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
                contactListWidget_->setDragIndexForDelegate(QModelIndex());
            }

            scrolledView_ = contactListWidget_->getView();
        }
        else if (currentTab_ == RECENTS)
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
                index = recentsView_->indexAt(_pos);

            if (!contactForDrop(index).isEmpty())
            {
                if (recentsView_->itemDelegate() == recentsDelegate_)
                    recentsDelegate_->setDragIndex(index);
                else
                    unknownsDelegate_->setDragIndex(index);

                recentsView_->update(index);
            }
            else
            {
                recentsDelegate_->setDragIndex(QModelIndex());
            }

            scrolledView_ = recentsView_;
            autoscroll_offset = Utils::scale_value(autoscroll_offset_recents);
        }


        auto rTop = scrolledView_->rect();
        rTop.setBottomLeft(QPoint(rTop.x(), autoscroll_offset));

        auto rBottom = scrolledView_->rect();
        rBottom.setTopLeft(QPoint(rBottom.x(), rBottom.height() - autoscroll_offset));

        if (!_pos.isNull() && (rTop.contains(_pos) || rBottom.contains(_pos)))
        {
            scrollMultipler_ =  (rTop.contains(_pos)) ? 1 : -1;
            scrollTimer_->start();
        }
        else
        {
            scrollTimer_->stop();
        }

        if (!fromScroll)
            lastDragPos_ = _pos;

        scrolledView_->update();
    }

    void ContactList::dropMimeData(const QPoint& _pos, const QMimeData* _mimeData)
    {
        auto send = [](const QMimeData* _mimeData, const QString& aimId)
        {
            const auto& quotes = Utils::InterConnector::instance().getContactDialog()->getInputWidget()->getInputQuotes();
            bool mayQuotesSent = false;

            if (Utils::isMimeDataWithImage(_mimeData))
            {
                auto image = Utils::getImageFromMimeData(_mimeData);

                QByteArray imageData;
                QBuffer buffer(&imageData);
                buffer.open(QIODevice::WriteOnly);
                image.save(&buffer, "png");

                Ui::GetDispatcher()->uploadSharedFile(aimId, imageData, qsl(".png"), quotes);
                mayQuotesSent = true;
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_sendmedia_action, { { "chat_type", Utils::chatTypeByAimId(aimId) }, { "count", "single" }, { "type", "dndrecents"} });
                auto cd = Utils::InterConnector::instance().getContactDialog();
                if (cd)
                    cd->onSendMessage(aimId);
            }
            else
            {
                auto count = 0;
                auto sendQuotesOnce = true;
                for (const QUrl& url : _mimeData->urls())
                {
                    if (url.isLocalFile())
                    {
                        QFileInfo info(url.toLocalFile());
                        bool canDrop = !(info.isBundle() || info.isDir());
                        if (info.size() == 0)
                            canDrop = false;

                        if (canDrop)
                        {
                            Ui::GetDispatcher()->uploadSharedFile(aimId, url.toLocalFile(), sendQuotesOnce ? quotes : Data::QuotesVec());
                            sendQuotesOnce = false;
                            mayQuotesSent = true;
                            auto cd = Utils::InterConnector::instance().getContactDialog();
                            if (cd)
                                cd->onSendMessage(aimId);

                            ++count;
                        }
                    }
                    else if (url.isValid())
                    {
                        Ui::GetDispatcher()->sendMessageToContact(aimId, url.toString(), sendQuotesOnce ? quotes : Data::QuotesVec());
                        sendQuotesOnce = false;
                        mayQuotesSent = true;
                        if (auto cd = Utils::InterConnector::instance().getContactDialog())
                            cd->onSendMessage(aimId);
                    }
                }

                if (count > 0)
                    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_sendmedia_action, { { "chat_type", Utils::chatTypeByAimId(aimId) },{ "count", count > 1 ? "multi" : "single" },{ "type", "dndrecents" } });
            }

            if (mayQuotesSent && !quotes.isEmpty())
            {
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, { { "Quotes_MessagesCount", std::to_string(quotes.size()) } });
                emit Utils::InterConnector::instance().getContactDialog()->getInputWidget()->needClearQuotes();
            }

        };
        if (isSearchMode())
        {
            if (!_pos.isNull())
            {
                QModelIndex index = contactListWidget_->getView()->indexAt(_pos);
                const auto contact = contactForDrop(index);
                if (!contact.isEmpty())
                {
                    if (contact != Logic::getContactListModel()->selectedContact())
                        emit Logic::getContactListModel()->select(contact, -1);

                    send(_mimeData, contact);
                    contactListWidget_->getView()->update(index);
                }
            }
            contactListWidget_->setDragIndexForDelegate(QModelIndex());
        }
        else if (currentTab_ == RECENTS)
        {
            if (!_pos.isNull())
            {
                QModelIndex index = recentsView_->indexAt(_pos);
                const auto contact = contactForDrop(index);
                if (!contact.isEmpty())
                {
                    if (contact != Logic::getContactListModel()->selectedContact())
                        emit Logic::getContactListModel()->select(contact, -1);

                    send(_mimeData, contact);
                    recentsView_->update(index);
                }
            }
            recentsDelegate_->setDragIndex(QModelIndex());
            unknownsDelegate_->setDragIndex(QModelIndex());
        }
    }

    void ContactList::recentsClicked()
    {
        if (currentTab_ == RECENTS)
            emit Utils::InterConnector::instance().activateNextUnread();
        else
            switchToRecents();
    }

    void ContactList::switchToRecents()
    {
        auto rModel = Logic::getRecentsModel();
        if (recentsView_->model() != rModel)
        {
            if (isUnknownsOpen())
                Logic::getUnknownsModel()->markAllRead();

            recentsView_->setModel(rModel);
            recentsView_->setItemDelegate(recentsDelegate_);

            rModel->refresh();
            recentsView_->scrollToTop();
            recentsView_->update();
            contactListWidget_->setRegim(Logic::MembersWidgetRegim::CONTACT_LIST);

            if constexpr (platform::is_apple())
                emit Utils::InterConnector::instance().forceRefreshList(recentsView_->model(), true);
        }

        changeTab(RECENTS);

        Logic::updatePlaceholders();
    }

    void ContactList::switchToUnknowns()
    {
        if (isUnknownsOpen())
            return;

        recentsView_->setModel(Logic::getUnknownsModel());
        recentsView_->setItemDelegate(unknownsDelegate_);
        recentsView_->scrollToTop();
        recentsView_->update();
        contactListWidget_->setRegim(Logic::MembersWidgetRegim::UNKNOWN);

        if constexpr (platform::is_apple())
            emit Utils::InterConnector::instance().forceRefreshList(recentsView_->model(), true);

        const auto itemsCount = Logic::getUnknownsModel()->itemsCount();
        std::string itemsRange = "0";
        if (itemsCount == 1)
            itemsRange = "1";
        else if (itemsCount <= 10)
            itemsRange = "2-10";
        else if (itemsCount <= 50)
            itemsRange = "11-50";
        else if (itemsCount <= 100)
            itemsRange = "51-100";
        else if (itemsCount <= 200)
            itemsRange = "101-200";
        else
            itemsRange = "200+";

        core::stats::event_props_type props = {
            {"Chats", itemsRange},
            {"Counter", Logic::getUnknownsModel()->totalUnreads() != 0 ? "Yes" : "No" }
        };
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_unknown_senders, props);
    }

    void ContactList::updateTabState()
    {
        stackedWidget_->setCurrentIndex(currentTab_);

        emit Utils::InterConnector::instance().makeSearchWidgetVisible(true);
    }

    void ContactList::guiSettingsChanged()
    {
        currentTab_ = 0;
        updateTabState();
    }

    void ContactList::touchScrollStateChangedRecents(QScroller::State _state)
    {
        recentsView_->blockSignals(_state != QScroller::Inactive);
        recentsView_->selectionModel()->blockSignals(_state != QScroller::Inactive);

        const auto contact = Logic::getContactListModel()->selectedContact();
        if (isRecentsOpen())
        {
            recentsView_->selectionModel()->setCurrentIndex(Logic::getRecentsModel()->contactIndex(contact), QItemSelectionModel::ClearAndSelect);
            recentsDelegate_->blockState(_state != QScroller::Inactive);
        }
        else
        {
            recentsView_->selectionModel()->setCurrentIndex(Logic::getUnknownsModel()->contactIndex(contact), QItemSelectionModel::ClearAndSelect);
            unknownsDelegate_->blockState(_state != QScroller::Inactive);
        }
    }

    void ContactList::changeSelected(const QString& _aimId)
    {
        auto updateSelection = [this](auto _view, const QModelIndex& _index)
        {
            QSignalBlocker sb(_view);
            if (_index.isValid())
            {
                if (const auto cur = _view->selectionModel()->currentIndex(); cur.isValid())
                {
                    _view->selectionModel()->setCurrentIndex(_index, QItemSelectionModel::ClearAndSelect);

                    const auto inc = cur.row() < _index.row() ? 1 : -1;
                    if (const auto next = _index.model()->index(_index.row() + inc, 0); next.isValid() && (isKeyboardFocused() || nextSelectWithOffset_))
                        _view->scrollTo(next);
                    else
                        _view->scrollTo(_index);
                }
                else
                {
                    _view->selectionModel()->setCurrentIndex(_index, QItemSelectionModel::ClearAndSelect);
                    _view->scrollTo(_index);
                }
            }
            else
            {
                _view->selectionModel()->clearSelection();
            }
            _view->update();

            nextSelectWithOffset_ = false;
        };

        QModelIndex index = Logic::getRecentsModel()->contactIndex(_aimId);
        if (!index.isValid())
            index = Logic::getUnknownsModel()->contactIndex(_aimId);
        updateSelection(recentsView_, index);
    }

    void ContactList::select(const QString& _aimId)
    {
        contactListWidget_->select(_aimId);
    }

    void ContactList::scrollToItem(const QString& _aimId, QAbstractItemView::ScrollHint _hint)
    {
        if (isRecentsOpen())
        {
            if (const auto index = Logic::getRecentsModel()->contactIndex(_aimId); index.isValid())
                recentsView_->scrollTo(index, _hint);
        }
    }

    void ContactList::showRecentsPopupMenu(const QModelIndex& _current)
    {
        if (recentsView_->model() != Logic::getRecentsModel())
            return;

        if (!popupMenu_)
        {
            popupMenu_ = new ContextMenu(this);
            Testing::setAccessibleName(popupMenu_, qsl("AS contactlist popupMenu_"));
            connect(popupMenu_, &ContextMenu::triggered, this, &ContactList::showPopupMenu);
        }
        else
        {
            popupMenu_->clear();
        }

        const Data::DlgState dlg = _current.data(Qt::DisplayRole).value<Data::DlgState>();
        const QString& aimId = dlg.AimId_;

        if (dlg.UnreadCount_ != 0 || dlg.Attention_)
            popupMenu_->addActionWithIcon(qsl(":/context_menu/mark_read"), QT_TRANSLATE_NOOP("context_menu", "Mark as read"), Logic::makeData(qsl("recents/mark_read"), aimId));
        else if (!dlg.Attention_)
            popupMenu_->addActionWithIcon(qsl(":/context_menu/mark_unread"), QT_TRANSLATE_NOOP("context_menu", "Mark as unread"), Logic::makeData(qsl("recents/mark_unread"), aimId));

        if (!Logic::getRecentsModel()->isUnimportant(aimId))
        {
            if (Logic::getRecentsModel()->isFavorite(aimId))
                popupMenu_->addActionWithIcon(qsl(":/context_menu/unpin"), QT_TRANSLATE_NOOP("context_menu", "Unpin"), Logic::makeData(qsl("recents/unfavorite"), aimId));
            else
                popupMenu_->addActionWithIcon(qsl(":/context_menu/pin"), QT_TRANSLATE_NOOP("context_menu", "Pin"), Logic::makeData(qsl("recents/favorite"), aimId));
        }

        if (!Logic::getRecentsModel()->isFavorite(aimId))
        {
            if (Logic::getRecentsModel()->isUnimportant(aimId))
                popupMenu_->addActionWithIcon(qsl(":/context_menu/unmark_unimportant"), QT_TRANSLATE_NOOP("context_menu", "Remove from Unimportant"), Logic::makeData(qsl("recents/remove_from_unimportant"), aimId));
            else
                popupMenu_->addActionWithIcon(qsl(":/context_menu/mark_unimportant"), QT_TRANSLATE_NOOP("context_menu", "Move to Unimportant"), Logic::makeData(qsl("recents/mark_unimportant"), aimId));
        }

        if (Logic::getContactListModel()->isMuted(aimId))
            popupMenu_->addActionWithIcon(qsl(":/context_menu/mute_off"), QT_TRANSLATE_NOOP("context_menu", "Turn on notifications"), Logic::makeData(qsl("recents/unmute"), aimId));
        else
            popupMenu_->addActionWithIcon(qsl(":/context_menu/mute"), QT_TRANSLATE_NOOP("context_menu", "Turn off notifications"), Logic::makeData(qsl("recents/mute"), aimId));

        popupMenu_->addSeparator();

        popupMenu_->addActionWithIcon(qsl(":/clear_chat_icon"), QT_TRANSLATE_NOOP("context_menu", "Clear history"), Logic::makeData(qsl("recents/clear_history"), aimId));
        popupMenu_->addActionWithIcon(qsl(":/ignore_icon"), QT_TRANSLATE_NOOP("context_menu", "Block"), Logic::makeData(qsl("recents/ignore"), aimId));

        if (Logic::getContactListModel()->isChat(aimId))
        {
            popupMenu_->addActionWithIcon(qsl(":/alert_icon"), QT_TRANSLATE_NOOP("context_menu", "Report and block"), Logic::makeData(qsl("recents/report"), aimId));
            popupMenu_->addActionWithIcon(qsl(":/exit_icon"), QT_TRANSLATE_NOOP("context_menu", "Leave and delete"), Logic::makeData(qsl("contacts/remove"), aimId));
        }
        else if (Features::clRemoveContactsAllowed())
        {
            popupMenu_->addActionWithIcon(qsl(":/alert_icon"), QT_TRANSLATE_NOOP("context_menu", "Report"), Logic::makeData(qsl("recents/report"), aimId));
        }

        if (!Logic::getRecentsModel()->isFavorite(aimId) && !Logic::getRecentsModel()->isUnimportant(aimId))
        {
            if (Features::clRemoveContactsAllowed() && !Logic::getContactListModel()->isChat(aimId))
                popupMenu_->addActionWithIcon(qsl(":/context_menu/delete"), QT_TRANSLATE_NOOP("context_menu", "Remove"), Logic::makeData(qsl("recents/remove"), aimId));

            popupMenu_->addSeparator();
            popupMenu_->addActionWithIcon(qsl(":/context_menu/close"), QT_TRANSLATE_NOOP("context_menu", "Hide"), Logic::makeData(qsl("recents/close"), aimId));

        }

        popupMenu_->popup(QCursor::pos());
    }

    void ContactList::showPopupMenu(QAction* _action)
    {
        Logic::showContactListPopup(_action);
    }

    void ContactList::autoScroll()
    {
        if (scrolledView_)
        {
            scrolledView_->verticalScrollBar()->setValue(scrolledView_->verticalScrollBar()->value() - (Utils::scale_value(autoscroll_speed_pixels) * scrollMultipler_));
            dragPositionUpdate(lastDragPos_, true);
        }
    }

    void ContactList::showNoRecentsYet()
    {
        if (!recentsPlaceholder_)
        {
            recentsPlaceholder_ = new RecentsPlaceholder(recentsPage_);
            Testing::setAccessibleName(recentsPlaceholder_, qsl("AS recentsPlaceholder_"));
            recentsContainerLayout_->addWidget(recentsPlaceholder_);
        }
        recentsView_->hide();
        recentsPlaceholder_->show();
    }

    void ContactList::hideNoRecentsYet()
    {
        if (recentsPlaceholder_)
            recentsPlaceholder_->hide();

        recentsView_->show();
    }

    void ContactList::messagesReceived(const QString& _aimId, const QVector<QString>& _chatters)
    {
        auto contactItem = Logic::getContactListModel()->getContactItem(_aimId);
        if (!contactItem)
            return;

        QModelIndex updateIndex;
        if (isRecentsOpen())
        {
            if (contactItem->is_chat())
            {
                for (const auto& chatter : _chatters)
                    recentsDelegate_->removeTyping(Logic::TypingFires(_aimId, chatter, QString()));
            }
            else
            {
                recentsDelegate_->removeTyping(Logic::TypingFires(_aimId, QString(), QString()));
            }

            updateIndex = Logic::getRecentsModel()->contactIndex(_aimId);
        }
        else
        {
            updateIndex = Logic::getUnknownsModel()->contactIndex(_aimId);
        }

        if (updateIndex.isValid())
            recentsView_->update(updateIndex);
    }

    bool ContactList::getPictureOnlyView() const
    {
        return pictureOnlyView_;
    }

    void ContactList::setPictureOnlyView(bool _isPictureOnly)
    {
        if (pictureOnlyView_ == _isPictureOnly)
            return;

        pictureOnlyView_ = _isPictureOnly;
        recentsDelegate_->setPictOnlyView(pictureOnlyView_);
        unknownsDelegate_->setPictOnlyView(pictureOnlyView_);
        recentsView_->setFlow(QListView::TopToBottom);
        if (recentsPlaceholder_)
            recentsPlaceholder_->setPictureOnlyView(_isPictureOnly);

        Logic::getUnknownsModel()->setHeaderVisible(!pictureOnlyView_);

        if (pictureOnlyView_)
            setSearchMode(false);
    }

    void ContactList::setItemWidth(int _newWidth)
    {
        recentsDelegate_->setFixedWidth(_newWidth);
        unknownsDelegate_->setFixedWidth(_newWidth);
        contactListWidget_->setWidthForDelegate(_newWidth);
        clDelegate_->setFixedWidth(_newWidth);
    }

    QString ContactList::getSelectedAimid() const
    {
        return contactListWidget_->getSelectedAimid();
    }

    bool ContactList::isRecentsOpen() const
    {
        return recentsView_->model() == Logic::getRecentsModel();
    }

    bool ContactList::isUnknownsOpen() const
    {
        return recentsView_->model() == Logic::getUnknownsModel();
    }

    bool Ui::ContactList::isCLWithHeadersOpen() const
    {
        return recentsView_->model() == clWithHeaders_;
    }

    bool ContactList::isKeyboardFocused() const
    {
        if (auto deleg = qobject_cast<Logic::AbstractItemDelegateWithRegim*>(recentsView_->itemDelegate()))
            return deleg->isKeyboardFocused();

        return false;
    }

    void ContactList::dropKeyboardFocus()
    {
        if (isKeyboardFocused())
        {
            setKeyboardFocused(false);
            recentsView_->updateSelectionUnderCursor();
        }
    }

    ContactListWidget* ContactList::getContactListWidget() const
    {
        return contactListWidget_;
    }

    void ContactList::connectSearchWidget(SearchWidget* _searchWidget)
    {
        assert(_searchWidget);

        getContactListWidget()->connectSearchWidget(_searchWidget);

        connect(_searchWidget, &SearchWidget::enterPressed, this, &ContactList::searchEnterPressed);
        connect(_searchWidget, &SearchWidget::upPressed,    this, &ContactList::searchUpPressed);
        connect(_searchWidget, &SearchWidget::downPressed,  this, &ContactList::searchDownPressed);

        connect(_searchWidget, &SearchWidget::selectFirstInRecents, this, &ContactList::highlightFirstItem);
        connect(this, &ContactList::topItemUpPressed, _searchWidget, &SearchWidget::setTabFocus);

        connect(this, &ContactList::needClearSearch, _searchWidget, &SearchWidget::searchCompleted);

        connect(_searchWidget, &SearchWidget::search, this, [this](const QString& _searchPattern)
        {
            if (!getSearchInDialog())
                setSearchMode(!_searchPattern.isEmpty());
        });

        connect(this, &ContactList::itemSelected, _searchWidget, [this, _searchWidget](const QString&, qint64 _msgid)
        {
            const auto forceClear = _msgid == -1
                && !getSearchInDialog()
                && get_gui_settings()->get_value<bool>(settings_fast_drop_search_results, settings_fast_drop_search_default());
            _searchWidget->clearSearch(forceClear);
        });
    }

    void ContactList::resizeEvent(QResizeEvent* _event)
    {
        if (transitionAnim_)
            transitionAnim_->stop();
        if (transitionLabel_)
            transitionLabel_->hide();
    }

    void ContactList::setSearchInAllDialogs()
    {
        setSearchInDialog(QString());
    }

    void ContactList::setSearchInDialog(const QString& _contact)
    {
        contactListWidget_->setSearchInDialog(_contact);
    }

    bool ContactList::getSearchInDialog() const
    {
        if (!contactListWidget_)
            return false;

        return contactListWidget_->getSearchInDialog();
    }

    const QString& ContactList::getSearchInDialogContact() const
    {
        return contactListWidget_->getSearchInDialogContact();
    }

    void ContactList::showSearch()
    {
        contactListWidget_->showSearch();
    }

    void ContactList::dialogClosed(const QString& _aimid)
    {
        const auto searchModel = qobject_cast<Logic::SearchModel*>(contactListWidget_->getSearchModel());
        if (searchModel && searchModel->getSingleDialogAimId() == _aimid)
            emit contactListWidget_->searchEnd();
    }

    void ContactList::myProfileBack()
    {
        changeTab((CurrentTab)prevTab_);
    }

    void ContactList::recentsScrollActionTriggered(int value)
    {
        recentsView_->verticalScrollBar()->setSingleStep(Utils::scale_value(RECENTS_HEIGHT));
    }

    void Ui::ContactList::searchEnterPressed()
    {
        if (!isSearchMode())
            itemClicked(recentsView_->selectionModel()->currentIndex());
    }

    void Ui::ContactList::searchUpPressed()
    {
        if (!isSearchMode())
            searchUpOrDownPressed(true);
    }

    void Ui::ContactList::searchDownPressed()
    {
        if (!isSearchMode())
            searchUpOrDownPressed(false);
    }

    void ContactList::highlightContact(const QString& _aimId)
    {
        const auto model = qobject_cast<Logic::CustomAbstractListModel*>(recentsView_->model());
        if (!model)
            return;

        QModelIndex index;
        if (isRecentsOpen())
            index = Logic::getRecentsModel()->contactIndex(_aimId);
        else
            index = Logic::getUnknownsModel()->contactIndex(_aimId);

        if (!index.isValid())
            return;

        setKeyboardFocused(true);

        {
            QSignalBlocker sb(recentsView_->selectionModel());
            recentsView_->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
        }
        recentsView_->update();
        recentsView_->scrollTo(index);
    }

    void ContactList::setNextSelectWithOffset()
    {
        nextSelectWithOffset_ = true;
    }

    void ContactList::highlightFirstItem()
    {
        const auto model = qobject_cast<Logic::CustomAbstractListModel*>(recentsView_->model());
        if (!model)
            return;

        auto i = model->index(0);
        if (!i.isValid())
            return;

        while (model->isServiceItem(i))
        {
            i = model->index(i.row() + 1);
            if (!i.isValid())
                return;
        }

        auto prevIndex = recentsView_->selectionModel()->currentIndex();

        {
            QSignalBlocker sb(recentsView_->selectionModel());
            recentsView_->selectionModel()->setCurrentIndex(i, QItemSelectionModel::ClearAndSelect);
        }

        setKeyboardFocused(true);

        if (prevIndex != i)
            recentsView_->update(prevIndex);

        recentsView_->update(i);
        recentsView_->scrollToTop();
    }

    void ContactList::searchUpOrDownPressed(const bool _isUpPressed)
    {
        const auto model = qobject_cast<Logic::CustomAbstractListModel*>(recentsView_->model());
        if (!model)
            return;

        const auto prevIndex = recentsView_->selectionModel()->currentIndex();
        if (!prevIndex.isValid())
        {
            highlightFirstItem();
            return;
        }

        const auto inc = _isUpPressed ? -1 : 1;
        auto nextRow = prevIndex.row() + inc;
        if (nextRow >= model->rowCount())
            nextRow = 0;

        auto i = model->index(nextRow);

        while (model->isServiceItem(i))
        {
            i = model->index(i.row() + inc);
            if (!i.isValid())
            {
                if (_isUpPressed && prevIndex.isValid())
                    emit topItemUpPressed(QPrivateSignal());
                return;
            }
        }

        {
            QSignalBlocker sb(recentsView_->selectionModel());
            recentsView_->selectionModel()->setCurrentIndex(i, QItemSelectionModel::ClearAndSelect);
        }

        setKeyboardFocused(true);

        if (prevIndex != i)
            recentsView_->update(prevIndex);

        recentsView_->update(i);

        const auto add = i.row() > prevIndex.row() ? 1 : -1;
        if (const auto next = model->index(i.row() + add); next.isValid())
            recentsView_->scrollTo(next);
        else
            recentsView_->scrollTo(i);

        if (auto scrollBar = recentsView_->getScrollBarV())
            scrollBar->fadeIn();
    }

    void ContactList::setKeyboardFocused(const bool _isFocused)
    {
        if (auto deleg = qobject_cast<Logic::AbstractItemDelegateWithRegim*>(recentsView_->itemDelegate()))
        {
            if (deleg->isKeyboardFocused() != _isFocused)
            {
                deleg->setKeyboardFocus(_isFocused);

                if (const auto idx = recentsView_->selectionModel()->currentIndex(); idx.isValid())
                    recentsView_->update(idx);
            }
        }
    }

    void ContactList::onMouseMoved(const QPoint&, const QModelIndex&)
    {
        setKeyboardFocused(false);
    }

    void ContactList::switchToContactListWithHeaders(const SwichType _switchType)
    {
        if (isCLWithHeadersOpen())
            return;

        if (_switchType == SwichType::Animated)
        {
            if (!transitionLabel_ || !transitionAnim_)
            {
                transitionLabel_ = new QLabel(this);
                transitionLabel_->hide();
                transitionLabel_->setAttribute(Qt::WA_TranslucentBackground);
                transitionLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);

                auto effect = new QGraphicsOpacityEffect(transitionLabel_);
                effect->setOpacity(1.0);
                transitionLabel_->setGraphicsEffect(effect);

                transitionAnim_ = new QPropertyAnimation(effect, "opacity", transitionLabel_);
                transitionAnim_->setDuration(100);
                transitionAnim_->setStartValue(1.0);
                transitionAnim_->setEndValue(0.0);
                transitionAnim_->setEasingCurve(QEasingCurve::InCirc);

                connect(transitionAnim_, &QPropertyAnimation::finished, transitionLabel_, &QLabel::hide);
            }

            QPixmap pm(size());
            pm.fill(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
            render(&pm, QPoint(), QRegion(), DrawChildren);

            transitionLabel_->move(0, 0);
            transitionLabel_->setFixedSize(size());
            transitionLabel_->setPixmap(pm);
            transitionLabel_->raise();
            transitionLabel_->show();
        }

        recentsView_->setItemDelegate(clDelegate_);
        recentsView_->setModel(clWithHeaders_);
        recentsView_->scrollToTop();
        recentsView_->update();
        contactListWidget_->setRegim(Logic::MembersWidgetRegim::CONTACT_LIST);

        if (_switchType == SwichType::Animated && transitionAnim_)
            transitionAnim_->start();

        if (platform::is_apple())
            emit Utils::InterConnector::instance().forceRefreshList(recentsView_->model(), true);

        emit Utils::InterConnector::instance().hideRecentsPlaceholder();
    }
}
