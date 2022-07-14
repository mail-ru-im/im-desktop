#include "stdafx.h"

#include "Common.h"
#include "RecentsTab.h"
#include "ContactItem.h"
#include "ContactListModel.h"
#include "IgnoreMembersModel.h"
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
#include "../contact_list/ChatMembersModel.h"
#include "../contact_list/ServiceContacts.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../settings/SettingsTab.h"
#include "../../types/contact.h"
#include "../../types/typing.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/TooltipWidget.h"

#include "../containers/FriendlyContainer.h"
#include "../containers/InputStateContainer.h"

#include "../../controls/TransparentScrollBar.h"
#include "../../controls/CustomButton.h"
#include "../../controls/HorScrollableView.h"
#include "../../utils/utils.h"
#include "../../utils/MimeDataUtils.h"
#include "../../utils/async/AsyncTask.h"
#include "../../utils/stat_utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/features.h"
#include "../../utils/log/log.h"
#include "../../fonts.h"
#include "../Placeholders.h"
#include "../../utils/DragAndDropEventFilter.h"
#include "AddContactDialogs.h"
#include "FavoritesUtils.h"
#include "statuses/StatusTooltip.h"
#include "main_window/containers/StatusContainer.h"
#include "main_window/containers/LastseenContainer.h"

#include "styles/ThemeParameters.h"

#include "../common.shared/string_utils.h"
#include "../common.shared/config/config.h"
#include "IconsDelegate.h"

namespace
{
    constexpr int autoscroll_offset_recents = 68;
    constexpr int autoscroll_offset_cl = 44;
    constexpr int autoscroll_speed_pixels = 10;
    constexpr int autoscroll_timeout = 50;

    constexpr int RECENTS_HEIGHT = 68;

    QString getAimId(const QModelIndex& _index)
    {
        if (const auto model = qobject_cast<const Logic::RecentsModel*>(_index.model()))
        {
            return model->data(_index, Qt::DisplayRole).value<Data::DlgState>().AimId_;
        }
        else if (const auto model = qobject_cast<const Logic::ContactListWithHeaders*>(_index.model()))
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
            if (Logic::getContactListModel()->isDeleted(aimId) || Logic::getIgnoreModel()->contains(aimId) || Logic::GetLastseenContainer()->getLastSeen(aimId).isBlocked())
                return QString();

            const auto role = Logic::getContactListModel()->getYourRole(aimId);
            if (role != u"notamember" && role != u"readonly")
                return aimId;
        }
        return QString();
    }
}

namespace Ui
{
    RecentsTab::RecentsTab(QWidget* _parent)
        : QWidget(_parent)
        , recentsPlaceholder_(nullptr)
        , popupMenu_(nullptr)
        , recentsDelegate_(new Ui::RecentItemDelegate(this))
        , unknownsDelegate_(new Logic::UnknownItemDelegate(this))
        , clDelegate_(new Logic::ContactListItemDelegate(this, Logic::MembersWidgetRegim::CONTACT_LIST, nullptr, Logic::NeedDrawIcons))
        , contactListWidget_(new ContactListWidget(this, Logic::MembersWidgetRegim::CONTACT_LIST, nullptr))
        , clWithHeaders_(new Logic::ContactListWithHeaders(this, Logic::getContactListModel()))
        , scrolledView_(nullptr)
        , scrollMultipler_(1)
        , transitionLabel_(nullptr)
        , transitionAnim_(nullptr)
        , tooltipTimer_(nullptr)
        , currentTab_(RECENTS)
        , prevTab_(RECENTS)
        , openedAs_(PageOpenedAs::MainPage)
        , pictureOnlyView_(false)
        , nextSelectWithOffset_(false)
        , personTooltip_(new Utils::PersonTooltip(this))
    {
        clDelegate_->setReplaceFavorites(true);

        auto mainLayout = Utils::emptyVLayout(this);
        mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
        stackedWidget_ = new QStackedWidget(this);

        recentsPage_ = new QWidget();
        recentsLayout_ = Utils::emptyVLayout(recentsPage_);

        recentsView_ = new RecentsView();
        recentsContainer_ = new QWidget();
        recentsContainerLayout_ = Utils::emptyVLayout(recentsContainer_);

        connect(recentsView_->verticalScrollBar(), &QScrollBar::actionTriggered, this, &RecentsTab::recentsScrollActionTriggered, Qt::DirectConnection);

        Testing::setAccessibleName(recentsView_, qsl("AS RecentsTab recentsView"));
        recentsContainerLayout_->addWidget(recentsView_);
        Testing::setAccessibleName(recentsContainer_, qsl("AS RecentsTab recentsContainer"));
        recentsLayout_->addWidget(recentsContainer_);

        Testing::setAccessibleName(recentsPage_, qsl("AS RecentsTab recentsPage"));
        stackedWidget_->addWidget(recentsPage_);

        Testing::setAccessibleName(contactListWidget_, qsl("AS RecentsTab"));
        stackedWidget_->addWidget(contactListWidget_);
        Testing::setAccessibleName(stackedWidget_, qsl("AS RecentsTab stackedWidget"));
        mainLayout->addWidget(stackedWidget_);

        stackedWidget_->setCurrentIndex(RECENTS);

        recentsView_->setAttribute(Qt::WA_MacShowFocusRect, false);
        recentsView_->setListViewGestureHandler(new ListViewGestureHandler(recentsView_));

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showRecentsPlaceholder, this, &RecentsTab::showNoRecentsYet);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideRecentsPlaceholder, this, &RecentsTab::hideNoRecentsYet);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::myProfileBack, this, &RecentsTab::myProfileBack);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::disableSearchInDialog, this, [this]()
        {
            setSearchMode(!contactListWidget_->getSearchModel()->getSearchPattern().isEmpty());
        });

        Utils::grabTouchWidget(recentsView_->viewport(), true);

        auto listEventFilter = new Utils::DragAndDropEventFilter(this);
        connect(listEventFilter, &Utils::DragAndDropEventFilter::dragPositionUpdate, this, &RecentsTab::onDragPositionUpdate);
        connect(listEventFilter, &Utils::DragAndDropEventFilter::dropMimeData, this, &RecentsTab::dropMimeData);
        contactListWidget_->installEventFilterToView(listEventFilter);
        recentsView_->viewport()->grabGesture(Qt::TapAndHoldGesture);
        recentsView_->viewport()->grabGesture(Qt::TapGesture);
        recentsView_->viewport()->installEventFilter(listEventFilter);

        connect(QScroller::scroller(recentsView_->viewport()), &QScroller::stateChanged, this, &RecentsTab::touchScrollStateChangedRecents, Qt::QueuedConnection);

        connect(Logic::getContactListModel(), &Logic::ContactListModel::select, this, [this](const QString& _aimId, const qint64 _msgId, const Logic::UpdateChatSelection _mode, bool _ignoreScroll, PageOpenedAs _openedAs)
        {
            if (_msgId > 0)
                Log::write_network_log(su::concat("message-transition", " jump to <", _aimId.toStdString(), "> ", std::to_string(_msgId), "\r\n"));

            contactListWidget_->select(_aimId, _msgId, {}, _mode, _ignoreScroll, _openedAs);
        });

        connect(contactListWidget_->getView(), &Ui::FocusableListView::clicked, this, &RecentsTab::statsCLItemPressed);
        connect(contactListWidget_, &ContactListWidget::itemClicked, recentsDelegate_, &Ui::RecentItemDelegate::onItemClicked);

        connect(this, &RecentsTab::groupClicked, Logic::getContactListModel(), &Logic::ContactListModel::groupClicked);

        Logic::getUnknownsModel(); // just initialization
        recentsView_->setModel(Logic::getRecentsModel());
        recentsView_->setItemDelegate(recentsDelegate_);
        connect(recentsView_, &Ui::ListViewWithTrScrollBar::pressed, this, &RecentsTab::itemPressed);
        connect(recentsView_, &Ui::ListViewWithTrScrollBar::clicked, this, &RecentsTab::itemClicked);
        connect(recentsView_->getListViewGestureHandler(), &Ui::ListViewGestureHandler::tapGesture, this, &RecentsTab::itemClicked);
        connect(recentsView_->getListViewGestureHandler(), &Ui::ListViewGestureHandler::rightClick, this, [this](const auto& _idx)
        {
            itemPressedImpl(_idx, Qt::MouseButtons(Qt::MouseButton::RightButton));
        });
        connect(recentsView_, &Ui::ListViewWithTrScrollBar::clicked, this, &RecentsTab::statsRecentItemPressed);
        connect(recentsView_, &Ui::ListViewWithTrScrollBar::mousePosChanged, this, &RecentsTab::onMouseMoved);

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
                Q_EMIT Logic::getUnknownsModel()->refresh();
        });

        connect(Logic::getRecentsModel(), &Logic::RecentsModel::selectContact, contactListWidget_, qOverload<const QString&>(&ContactListWidget::select));
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::selectContact, contactListWidget_, qOverload<const QString&>(&ContactListWidget::select));

        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::dlgStateChanged, unknownsDelegate_, &Logic::UnknownItemDelegate::dlgStateChanged);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStateChanged, recentsDelegate_, &Ui::RecentItemDelegate::dlgStateChanged);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::refreshAll, unknownsDelegate_, &Logic::UnknownItemDelegate::refreshAll);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::refreshAll, recentsDelegate_, &Ui::RecentItemDelegate::refreshAll);

        if (contactListWidget_->getRegim() == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            connect(get_gui_settings(), &Ui::qt_gui_settings::received, this, &RecentsTab::guiSettingsChanged);
            guiSettingsChanged();
        }

        scrollTimer_ = new QTimer(this);
        scrollTimer_->setInterval(autoscroll_timeout);
        scrollTimer_->setSingleShot(false);
        connect(scrollTimer_, &QTimer::timeout, this, &RecentsTab::autoScroll);

        connect(GetDispatcher(), &core_dispatcher::messagesReceived, this, &RecentsTab::messagesReceived);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsGoSeeThem, this, &RecentsTab::switchToUnknowns);

        connect(contactListWidget_, &Ui::ContactListWidget::itemSelected, this, &RecentsTab::itemSelected);
        connect(contactListWidget_, &Ui::ContactListWidget::changeSelected, this, &RecentsTab::changeSelected);

        connect(Utils::InterConnector::instance().getMainWindow(), &MainWindow::upKeyPressed, this, &RecentsTab::searchUpPressed);
        connect(Utils::InterConnector::instance().getMainWindow(), &MainWindow::downKeyPressed, this, &RecentsTab::searchDownPressed);
        connect(Utils::InterConnector::instance().getMainWindow(), &MainWindow::enterKeyPressed, this, &RecentsTab::searchEnterPressed);

        connect(qApp, &QApplication::focusChanged, this, [this](QWidget* _old, QWidget* _new)
        {
            if (_new)
                dropKeyboardFocus();
        });
    }

    RecentsTab::~RecentsTab() = default;

    void RecentsTab::setSearchMode(bool _search)
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

    bool RecentsTab::isSearchMode() const
    {
        return stackedWidget_->currentIndex() == SEARCH;
    }

    void RecentsTab::itemClicked(const QModelIndex& _current)
    {
        StatusTooltip::instance()->hide();

        if (qobject_cast<const Logic::ContactListWithHeaders*>(_current.model()) || qobject_cast<const Logic::ContactListModel*>(_current.model()))
        {
            if (const auto cont = _current.data().value<Data::Contact*>())
            {
                if (cont->GetType() == Data::DROPDOWN_BUTTON)
                {
                    if (cont->AimId_ == u"~addContact~")
                    {
                        Q_EMIT addContactClicked(QPrivateSignal());
                        Utils::showAddContactsDialog({ Utils::AddContactDialogs::Initiator::From::ConlistScr });
                    }
                    else if (cont->AimId_ == u"~newGroupchat~")
                    {
                        Q_EMIT createGroupChatClicked();

                        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_createbutton_click);
                    }
                    else if (cont->AimId_ == u"~newChannel~")
                    {
                        Q_EMIT createChannelClicked();
                    }
                    return;
                }
                else if (cont->GetType() == Data::GROUP)
                {
                    Q_EMIT groupClicked(cont->GroupId_);
                    return;
                }
            }
        }

        const auto rcntModel = qobject_cast<const Logic::RecentsModel *>(_current.model());
        const auto isLeftButton = QApplication::mouseButtons() & Qt::LeftButton || QApplication::mouseButtons() == Qt::NoButton;
        if (isLeftButton && rcntModel)
        {
            if (rcntModel->isServiceItem(_current))
            {
                if (rcntModel->isPinnedGroupButton(_current))
                    Logic::getRecentsModel()->togglePinnedVisible();
                else if (rcntModel->isUnimportantGroupButton(_current))
                    Logic::getRecentsModel()->toggleUnimportantVisible();
                return;
            }
        }

        const auto unkModel = qobject_cast<const Logic::UnknownsModel *>(_current.model());
        const auto changeSelection =
            !(QApplication::mouseButtons() & Qt::RightButton)
            && !(unkModel && unkModel->isServiceItem(_current));

        if (changeSelection)
        {
            Q_EMIT Utils::InterConnector::instance().clearDialogHistory();

            if (isOverUnknownsCloseButton(_current))
                return;

            contactListWidget_->selectionChanged(_current);
        }
    }

    void RecentsTab::itemPressed(const QModelIndex& _current)
    {
        itemPressedImpl(_current, QApplication::mouseButtons());
    }

    void RecentsTab::itemPressedImpl(const QModelIndex& _current, Qt::MouseButtons _buttons)
    {
        const auto rcntModel = qobject_cast<const Logic::RecentsModel*>(_current.model());
        const auto unkModel = qobject_cast<const Logic::UnknownsModel*>(_current.model());

        const auto isLeftButton = _buttons & Qt::LeftButton || _buttons == Qt::NoButton;
        if (isLeftButton && isOverUnknownsCloseButton(_current))
        {
            if (const auto aimId = Logic::aimIdFromIndex(_current); !aimId.isEmpty())
            {
                Logic::getContactListModel()->removeContactFromCL(aimId);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::unknowns_close);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_unknown_senders_close);
                return;
            }
        }

        if (_buttons & Qt::RightButton)
        {
            const auto model = qobject_cast<const Logic::CustomAbstractListModel*>(_current.model());
            if (model && model->isServiceItem(_current))
                return;
            if (rcntModel && rcntModel->isPinnedServiceItem(_current))
                return;

            if (rcntModel || unkModel)
            {
                showRecentsPopupMenu(_current);
            }
            else if (const auto srchModel = qobject_cast<const Logic::SearchModel*>(_current.model()))
            {
                const auto searchRes = _current.data().value<Data::AbstractSearchResultSptr>();
                if (searchRes && searchRes->isContact())
                {
                    if (const auto cont = Logic::getContactListModel()->getContactItem(searchRes->getAimId()))
                        contactListWidget_->showContactsPopupMenu(cont->get_aimid(), cont->is_chat());
                }
            }
            else
            {
                if (const auto cont = _current.data(Qt::DisplayRole).value<Data::Contact*>())
                    contactListWidget_->showContactsPopupMenu(cont->AimId_, cont->Is_chat_);
            }

            StatusTooltip::instance()->hide();
        }
        else if (_buttons & Qt::MiddleButton && rcntModel)
        {
            if (!rcntModel->isServiceItem(_current))
            {
                Data::DlgState dlg = _current.data(Qt::DisplayRole).value<Data::DlgState>();
                if (const auto id = dlg.AimId_; !id.isEmpty() && !ServiceContacts::isServiceContact(id))
                    Logic::getRecentsModel()->hideChat(id);
            }
        }
    }

    void RecentsTab::statsRecentItemPressed(const QModelIndex& _current)
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

    void RecentsTab::statsCLItemPressed(const QModelIndex& _current)
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

    void RecentsTab::changeTab(CurrentTab _currTab, bool silent)
    {
        if (currentTab_ == _currTab)
            return;

        if (getPictureOnlyView() && (_currTab != RECENTS))
            return;

        if (_currTab != RECENTS)
        {
            Q_EMIT Utils::InterConnector::instance().unknownsGoBack();
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
            Q_EMIT tabChanged(currentTab_);
    }

    CurrentTab RecentsTab::currentTab() const
    {
        return (CurrentTab)currentTab_;
    }

    void RecentsTab::dragPositionUpdate(QPoint _pos, bool _fromScroll)
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
                contactListWidget_->setDragIndexForDelegate({});
            }

            scrolledView_ = contactListWidget_->getView();
        }
        else if (currentTab_ == RECENTS)
        {
            QModelIndex index;
            if (!_pos.isNull())
                index = recentsView_->indexAt(_pos);

            if (isCLWithHeadersOpen())
            {
                if (!contactForDrop(index).isEmpty())
                    clDelegate_->setDragIndex(index);
                else
                    clDelegate_->setDragIndex({});
            }
            else
            {
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
                    recentsDelegate_->setDragIndex({});
                    unknownsDelegate_->setDragIndex({});
                }
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
            scrollMultipler_ =  rTop.contains(_pos) ? 1 : -1;
            scrollTimer_->start();
        }
        else
        {
            scrollTimer_->stop();
        }

        if (!_fromScroll)
            lastDragPos_ = _pos;

        scrolledView_->update();
    }

    void RecentsTab::dropMimeData(QPoint _pos, const QMimeData* _mimeData)
    {
        if (isSearchMode())
        {
            if (!_pos.isNull())
            {
                const QModelIndex index = contactListWidget_->getView()->indexAt(_pos);
                const QString aimId = contactForDrop(index);
                if (!aimId.isEmpty())
                {
                    if (aimId != Logic::getContactListModel()->selectedContact())
                        Q_EMIT Logic::getContactListModel()->select(aimId, -1);

                    MimeData::sendToChat(_mimeData, aimId);
                    contactListWidget_->getView()->update(index);
                }
            }
            contactListWidget_->setDragIndexForDelegate({});
        }
        else if (currentTab_ == RECENTS)
        {
            if (!_pos.isNull())
            {
                const QModelIndex index = recentsView_->indexAt(_pos);
                if (const QString aimId = contactForDrop(index); !aimId.isEmpty())
                {
                    if (aimId != Logic::getContactListModel()->selectedContact())
                        Q_EMIT Logic::getContactListModel()->select(aimId, -1);

                    MimeData::sendToChat(_mimeData, aimId);
                    recentsView_->update(index);
                }
            }
            recentsDelegate_->setDragIndex({});
            unknownsDelegate_->setDragIndex({});
            clDelegate_->setDragIndex({});
        }
    }

    void RecentsTab::recentsClicked()
    {
        if (currentTab_ == RECENTS)
            Q_EMIT Utils::InterConnector::instance().activateNextUnread();
        else
            switchToRecents();
    }

    void RecentsTab::switchToRecents(const QString& _contactToSelect)
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
                Q_EMIT Utils::InterConnector::instance().forceRefreshList(recentsView_->model(), true);
        }

        changeTab(RECENTS);

        if (!_contactToSelect.isEmpty())
        {
            const auto index = rModel->contactIndex(_contactToSelect);

            {
                QSignalBlocker sb(recentsView_->selectionModel());
                recentsView_->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
            }

            setKeyboardFocused(true);
            recentsView_->update(index);
        }

        Logic::updatePlaceholders();
    }

    void RecentsTab::switchToUnknowns()
    {
        if (isUnknownsOpen())
            return;

        recentsView_->setModel(Logic::getUnknownsModel());
        recentsView_->setItemDelegate(unknownsDelegate_);
        recentsView_->scrollToTop();
        recentsView_->update();
        contactListWidget_->setRegim(Logic::MembersWidgetRegim::UNKNOWN);

        if constexpr (platform::is_apple())
            Q_EMIT Utils::InterConnector::instance().forceRefreshList(recentsView_->model(), true);

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

    void RecentsTab::updateTabState()
    {
        stackedWidget_->setCurrentIndex(currentTab_);

        Q_EMIT Utils::InterConnector::instance().makeSearchWidgetVisible(true);
    }

    void RecentsTab::guiSettingsChanged()
    {
        currentTab_ = 0;
        updateTabState();
    }

    void RecentsTab::touchScrollStateChangedRecents(QScroller::State _state)
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

    void RecentsTab::changeSelected(const QString& _aimId)
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

    void RecentsTab::select(const QString& _aimId)
    {
        contactListWidget_->select(_aimId);
    }

    void RecentsTab::scrollToItem(const QString& _aimId, QAbstractItemView::ScrollHint _hint)
    {
        if (isRecentsOpen())
        {
            if (const auto index = Logic::getRecentsModel()->contactIndex(_aimId); index.isValid())
                recentsView_->scrollTo(index, _hint);
        }
    }

    void RecentsTab::showRecentsPopupMenu(const QModelIndex& _current)
    {
        if (recentsView_->model() != Logic::getRecentsModel())
            return;

        if (popupMenu_)
            return;
        popupMenu_ = new ContextMenu(this);
        Testing::setAccessibleName(popupMenu_, qsl("AS RecentsTab popupMenu"));
        popupMenu_->setAttribute(Qt::WA_DeleteOnClose);
        connect(popupMenu_, &ContextMenu::triggered, this, &RecentsTab::showPopupMenu);

        const Data::DlgState dlg = _current.data(Qt::DisplayRole).value<Data::DlgState>();
        const QString& aimId = dlg.AimId_;

        Logic::addItemToContextMenu(popupMenu_, aimId, dlg);
    }

    void RecentsTab::showPopupMenu(QAction* _action)
    {
        Logic::showContactListPopup(_action);
    }

    void RecentsTab::autoScroll()
    {
        if (scrolledView_)
        {
            scrolledView_->verticalScrollBar()->setValue(scrolledView_->verticalScrollBar()->value() - (Utils::scale_value(autoscroll_speed_pixels) * scrollMultipler_));
            dragPositionUpdate(lastDragPos_, true);
        }
    }

    void RecentsTab::showNoRecentsYet()
    {
        if (!recentsPlaceholder_)
        {
            recentsPlaceholder_ = new RecentsPlaceholder(recentsPage_);
            Testing::setAccessibleName(recentsPlaceholder_, qsl("AS RecentsTab placeholder"));
            recentsContainerLayout_->addWidget(recentsPlaceholder_);
        }
        recentsView_->hide();
        recentsPlaceholder_->show();
    }

    void RecentsTab::hideNoRecentsYet()
    {
        if (recentsPlaceholder_)
            recentsPlaceholder_->hide();

        recentsView_->show();
    }

    void RecentsTab::messagesReceived(const QString& _aimId, const QVector<QString>& _chatters)
    {
        auto contactItem = Logic::getContactListModel()->getContactItem(_aimId);
        if (!contactItem)
            return;

        QModelIndex updateIndex;
        if (isRecentsOpen())
            updateIndex = Logic::getRecentsModel()->contactIndex(_aimId);
        else
            updateIndex = Logic::getUnknownsModel()->contactIndex(_aimId);

        if (updateIndex.isValid())
            recentsView_->update(updateIndex);
    }

    bool RecentsTab::getPictureOnlyView() const
    {
        return pictureOnlyView_;
    }

    void RecentsTab::setPictureOnlyView(bool _isPictureOnly)
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

    void RecentsTab::setItemWidth(int _newWidth)
    {
        recentsDelegate_->setFixedWidth(_newWidth);
        unknownsDelegate_->setFixedWidth(_newWidth);
        contactListWidget_->setWidthForDelegate(_newWidth);
        clDelegate_->setFixedWidth(_newWidth);
    }

    QString RecentsTab::getSelectedAimid() const
    {
        return contactListWidget_->getSelectedAimid();
    }

    bool RecentsTab::isRecentsOpen() const
    {
        return recentsView_->model() == Logic::getRecentsModel();
    }

    bool RecentsTab::isUnknownsOpen() const
    {
        return recentsView_->model() == Logic::getUnknownsModel();
    }

    bool RecentsTab::isCLWithHeadersOpen() const
    {
        return recentsView_->model() == clWithHeaders_;
    }

    bool RecentsTab::isKeyboardFocused() const
    {
        if (auto deleg = qobject_cast<Logic::AbstractItemDelegateWithRegim*>(recentsView_->itemDelegate()))
            return deleg->isKeyboardFocused();

        return false;
    }

    void RecentsTab::dropKeyboardFocus()
    {
        if (isKeyboardFocused())
        {
            setKeyboardFocused(false);
            recentsView_->updateSelectionUnderCursor();
        }
    }

    ContactListWidget* RecentsTab::getContactListWidget() const
    {
        return contactListWidget_;
    }

    void RecentsTab::connectSearchWidget(SearchWidget* _searchWidget)
    {
        im_assert(_searchWidget);

        getContactListWidget()->connectSearchWidget(_searchWidget);

        connect(_searchWidget, &SearchWidget::enterPressed, this, &RecentsTab::searchEnterPressed);
        connect(_searchWidget, &SearchWidget::upPressed,    this, &RecentsTab::searchUpPressed);
        connect(_searchWidget, &SearchWidget::downPressed,  this, &RecentsTab::searchDownPressed);

        connect(_searchWidget, &SearchWidget::selectFirstInRecents, this, &RecentsTab::highlightFirstItem);
        connect(this, &RecentsTab::topItemUpPressed, _searchWidget, &SearchWidget::setTabFocus);

        connect(this, &RecentsTab::needClearSearch, _searchWidget, &SearchWidget::searchCompleted);

        connect(_searchWidget, &SearchWidget::search, this, [this](const QString& _searchPattern)
        {
            if (!getSearchInDialog())
                setSearchMode(!_searchPattern.isEmpty());
        });

        connect(this, &RecentsTab::itemSelected, _searchWidget, [this, _searchWidget](const QString&, qint64 _msgid)
        {
            const auto forceClear = _msgid == -1
                && !getSearchInDialog()
                && get_gui_settings()->get_value<bool>(settings_fast_drop_search_results, settings_fast_drop_search_default());
            _searchWidget->clearSearch(forceClear);
        });
    }

    void RecentsTab::resizeEvent(QResizeEvent* _event)
    {
        if (transitionAnim_)
            transitionAnim_->stop();
        if (transitionLabel_)
            transitionLabel_->hide();
    }

    void RecentsTab::leaveEvent(QEvent*)
    {
        hideTooltip();
    }

    void RecentsTab::setSearchInAllDialogs()
    {
        setSearchInDialog(QString());
    }

    void RecentsTab::setSearchInDialog(const QString& _contact)
    {
        contactListWidget_->setSearchInDialog(_contact);
    }

    bool RecentsTab::getSearchInDialog() const
    {
        if (!contactListWidget_)
            return false;

        return contactListWidget_->getSearchInDialog();
    }

    const QString& RecentsTab::getSearchInDialogContact() const
    {
        return contactListWidget_->getSearchInDialogContact();
    }

    void RecentsTab::showSearch()
    {
        contactListWidget_->showSearch();
    }

    void RecentsTab::dialogClosed(const QString& _aimid)
    {
        const auto searchModel = qobject_cast<Logic::SearchModel*>(contactListWidget_->getSearchModel());
        if (searchModel && searchModel->getSingleDialogAimId() == _aimid)
            Q_EMIT contactListWidget_->searchEnd();
    }

    void RecentsTab::myProfileBack()
    {
        changeTab((CurrentTab)prevTab_);
    }

    void RecentsTab::recentsScrollActionTriggered(int value)
    {
        recentsView_->verticalScrollBar()->setSingleStep(Utils::scale_value(RECENTS_HEIGHT));
    }

    void Ui::RecentsTab::searchEnterPressed()
    {
        if (!isSearchMode())
            itemClicked(recentsView_->selectionModel()->currentIndex());
    }

    void Ui::RecentsTab::searchUpPressed()
    {
        if (!isSearchMode())
            searchUpOrDownPressed(true);
    }

    void Ui::RecentsTab::searchDownPressed()
    {
        if (!isSearchMode())
            searchUpOrDownPressed(false);
    }

    void RecentsTab::highlightContact(const QString& _aimId)
    {
        const auto model = qobject_cast<Logic::CustomAbstractListModel*>(recentsView_->model());
        if (!model)
            return;

        const auto index = isRecentsOpen()
            ? Logic::getRecentsModel()->contactIndex(_aimId)
            : Logic::getUnknownsModel()->contactIndex(_aimId);
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

    void RecentsTab::setNextSelectWithOffset()
    {
        nextSelectWithOffset_ = true;
    }

    void RecentsTab::highlightFirstItem()
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

    void RecentsTab::searchUpOrDownPressed(const bool _isUpPressed)
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
                    Q_EMIT topItemUpPressed(QPrivateSignal());
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

    void RecentsTab::setKeyboardFocused(const bool _isFocused)
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

    void RecentsTab::onMouseMoved(const QPoint& _pos, const QModelIndex& _index)
    {
        setKeyboardFocused(false);

        const auto model = qobject_cast<Logic::CustomAbstractListModel*>(recentsView_->model());
        if (!model || (model && model->isServiceItem(_index)))
            return;

        const auto aimId = Logic::aimIdFromIndex(_index);
        const auto avatarRect = getAvatarRect(_index);
        const auto muted = Logic::getContactListModel()->isMuted(aimId);
        if (avatarRect.contains(_pos) && !Favorites::isFavorites(aimId))
        {
            const auto localPos = recentsView_->mapFromGlobal(QCursor::pos());
            const auto index = recentsView_->indexAt(localPos);
            if (!Features::isAppsNavigationBarVisible())
            {
                if (Statuses::isStatusEnabled() && !muted)
                {
                    StatusTooltip::instance()->objectHovered([this, aimId, index]()
                    {
                        if (aimId != Logic::aimIdFromIndex(index) || popupMenu_ && popupMenu_->isVisible())
                            return QRect();

                        const auto avatarRect = getAvatarRect(index);
                        return QRect(recentsView_->mapToGlobal(avatarRect.topLeft()), avatarRect.size());
                    }, aimId, recentsView_->viewport(), recentsView_);
                }
            }
            else if (aimId == Logic::aimIdFromIndex(index) && !(popupMenu_ && popupMenu_->isVisible()))
            {
                const auto tooltipRect = getAvatarRect(index);
                const auto linkGlobalRect = QRect(recentsView_->mapToGlobal(tooltipRect.topLeft()), tooltipRect.size());
                personTooltip_->show(Utils::PersonTooltipType::Person, aimId, linkGlobalRect, Tooltip::ArrowDirection::Down, Tooltip::ArrowPointPos::Top);
                return;
            }
        }
        else
        {
            hideTooltip();
            personTooltip_->hide();
        }

        if (Features::longPathTooltipsAllowed())
        {
            const auto aimId = Logic::senderAimIdFromIndex(_index);
            const auto isCompactMode = !get_gui_settings()->get_value<bool>(settings_show_last_message, !config::get().is_on(config::features::compact_mode_by_default));
            const auto posCursor = (pictureOnlyView_) ? QPoint() : _pos;
            const auto needTooltip = isUnknownsOpen() ? unknownsDelegate_->needsTooltip(aimId, _index, _pos) :
                                                        (isCLWithHeadersOpen() ? clDelegate_->needsTooltip(aimId, _index, _pos) :
                                                                               recentsDelegate_->needsTooltip(aimId, _index, _pos));
            if (needTooltip)
            {
                if (tooltipIndex_ != _index)
                {
                    tooltipIndex_ = _index;

                    const auto aimId = Logic::senderAimIdFromIndex(_index);
                    const auto rect = recentsView_->visualRect(_index);
                    auto ttRect = QRect(recentsView_->mapToGlobal(rect.topLeft()), rect.size());
                    auto name = Logic::GetFriendlyContainer()->getFriendly(aimId);
                    const auto direction = rect.y() < 0 ? Tooltip::ArrowDirection::Up : Tooltip::ArrowDirection::Down;
                    const auto arrowPosition = rect.y() < 0 ? Tooltip::ArrowPointPos::Bottom : Tooltip::ArrowPointPos::Top;
                    const bool hasDraft = Logic::getRecentsModel()->getDlgState(aimId).hasDraft();
                    auto draftIconRect = isUnknownsOpen() ? unknownsDelegate_->getDraftIconRectWrapper(aimId, _index, posCursor) :
                                                            (isCLWithHeadersOpen() ? clDelegate_->getDraftIconRect() :
                                                                                    recentsDelegate_->getDraftIconRectWrapper(aimId, _index, posCursor) );
                    auto cursorInDraftRect = draftIconRect.contains(posCursor);
                    draftIconRect.moveTop(ttRect.y());
                    draftIconRect.moveLeft(ttRect.x() + draftIconRect.x() - draftIconRect.width()/2);
                    const bool needShowDraftTooltip = hasDraft && cursorInDraftRect && (isCompactMode || isUnknownsOpen() || isCLWithHeadersOpen() || Favorites::isFavorites(aimId));
                    const auto draftMessageFormatted = makeDraftText(aimId);
                    showTooltip(needShowDraftTooltip ? draftMessageFormatted : name, needShowDraftTooltip ? draftIconRect : ttRect, direction, arrowPosition);
                }
            }
            else
            {
                hideTooltip();
            }
        }
    }

    void RecentsTab::showTooltip(const Data::FString& _text, const QRect& _rect, Tooltip::ArrowDirection _arrowDir, Tooltip::ArrowPointPos _arrowPos)
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

        connect(tooltipTimer_, &QTimer::timeout, this, [text = _text, _rect, _arrowDir, _arrowPos]()
        {
            Tooltip::show(text, _rect, {}, _arrowDir, _arrowPos, TextRendering::HorAligment::CENTER, {}, Tooltip::TooltipMode::Multiline);
        });
        tooltipTimer_->start();
    }

    void RecentsTab::hideTooltip()
    {
        tooltipIndex_ = {};

        if (tooltipTimer_)
            tooltipTimer_->stop();

        Tooltip::hide();
    }

    bool Ui::RecentsTab::isOverUnknownsCloseButton(const QModelIndex& _current) const
    {
        const auto unkModel = qobject_cast<const Logic::UnknownsModel*>(_current.model());
        if (!unkModel || unkModel->isServiceItem(_current))
            return false;

        const auto rect = recentsView_->visualRect(_current);
        const auto cursorPos = recentsView_->mapFromGlobal(QCursor::pos());
        if (!rect.contains(cursorPos))
            return false;

        QPoint pos(cursorPos.x(), cursorPos.y() - rect.y());
        const auto aimId = Logic::aimIdFromIndex(_current);
        return !aimId.isEmpty() && unknownsDelegate_->isInRemoveContactFrame(pos);
    }

    void RecentsTab::switchToContactListWithHeaders(const SwichType _switchType)
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
            Q_EMIT Utils::InterConnector::instance().forceRefreshList(recentsView_->model(), true);

        Q_EMIT Utils::InterConnector::instance().hideRecentsPlaceholder();
    }

    QRect RecentsTab::getAvatarRect(const QModelIndex& _index) const
    {
        const auto itemRect = recentsView_->visualRect(_index);
        const auto isCompact = !get_gui_settings()->get_value<bool>(settings_show_last_message, !config::get().is_on(config::features::compact_mode_by_default));

        const auto& params = isCompact || isCLWithHeadersOpen() ? Ui::GetContactListParams() : Ui::GetRecentsParams();

        const auto avatarSize = params.getAvatarSize();
        const auto avatarX = pictureOnlyView_ ? (itemRect.width() - avatarSize) / 2 : params.getAvatarX();

        return QRect(itemRect.topLeft() + QPoint(avatarX, params.getAvatarY()), QSize(avatarSize, avatarSize));
    }

    QRect RecentsTab::getContactFriendlyRect(const QModelIndex& _index) const
    {
        const auto itemRect = recentsView_->visualRect(_index);
        const auto isCompact = !get_gui_settings()->get_value<bool>(settings_show_last_message, !config::get().is_on(config::features::compact_mode_by_default));

        const auto& params = isCompact || isCLWithHeadersOpen() ? Ui::GetContactListParams() : Ui::GetRecentsParams();

        return QRect(itemRect.topLeft() + QPoint(params.getContactNameX(), params.getContactNameYTop()),
            QSize(itemRect.width() - params.getAvatarSize(), params.contactNameHeight()));
    }

    void RecentsTab::setPageOpenedAs(PageOpenedAs _openedAs)
    {
        openedAs_ = _openedAs;
        contactListWidget_->setPageOpenedAs(_openedAs);
    }

    void RecentsTab::setCloseOnThreadResult(bool _val)
    {
        contactListWidget_->setCloseOnThreadResult(_val);
    }

    void RecentsTab::onDragPositionUpdate(QPoint _pos)
    {
        dragPositionUpdate(_pos, false);
    }

    void RecentsTab::scrollToTop()
    {
        if (isRecentsOpen())
            recentsView_->scrollToTop();
    }
}
