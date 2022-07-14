#include "stdafx.h"

#include "ContactListWidget.h"

#include "SearchModel.h"
#include "RecentsModel.h"
#include "ContactListModel.h"
#include "CustomAbstractListModel.h"
#include "ChatMembersModel.h"
#include "IgnoreMembersModel.h"
#include "CommonChatsModel.h"
#include "CountryListModel.h"
#include "CallsModel.h"
#include "DisallowedInvitersModel.h"

#include "ContactListItemDelegate.h"
#include "ContactListWithHeaders.h"
#include "SearchItemDelegate.h"
#include "CountryListItemDelegate.h"
#include "CallsSearchModel.h"
#include "CallItemDelegate.h"
#include "TaskAssigneeListDelegate.h"
#include "TaskAssigneeModel.h"
#include "ServiceContacts.h"

#include "ContactListUtils.h"
#include "FavoritesUtils.h"
#include "SearchWidget.h"
#include "../../app_config.h"
#include "../../controls/LabelEx.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/TooltipWidget.h"
#include "../../utils/SearchPatternHistory.h"
#include "../../utils/features.h"
#include "../../gui_settings.h"
#include "../Placeholders.h"
#include "../containers/FriendlyContainer.h"
#include "../proxy/FriendlyContaInerProxy.h"
#include "../history_control/VoipEventInfo.h"
#include "main_window/containers/StatusContainer.h"
#include "main_window/AppsPage.h"
#include "statuses/StatusTooltip.h"
#include "../containers/LastseenContainer.h"

#include "styles/ThemeParameters.h"
#include "styles/ThemesContainer.h"
#include "IconsDelegate.h"

namespace
{
    constexpr auto searchPreloadDistance = 0.75;
    constexpr auto scrollTimeout = std::chrono::milliseconds(300);

    int getMoreButtonWidth()
    {
        return Utils::scale_value(40);
    }

    QRect getDeleteSuggestBtnRect(const QRect& _itemRect)
    {
        const auto btnArea = Utils::scale_value(QSize(24, 24));
        return QRect(
            _itemRect.left() + _itemRect.width() - btnArea.width() - Utils::scale_value(12) + (btnArea.width() / 2 - Utils::scale_value(16 / 2)),
            _itemRect.top() + (_itemRect.height() - btnArea.height()) / 2,
            btnArea.width(),
            btnArea.height()
        );
    }

    bool isRegimWithFavorites(Logic::MembersWidgetRegim _regim)
    {
        static const auto regimsWithFavorites =
        {
            Logic::MembersWidgetRegim::SHARE,
            Logic::MembersWidgetRegim::SHARE_VIDEO_CONFERENCE,
            Logic::MembersWidgetRegim::CONTACT_LIST,
            Logic::MembersWidgetRegim::CONTACT_LIST_POPUP,
        };

        return std::any_of(regimsWithFavorites.begin(), regimsWithFavorites.end(), [_regim](const auto _r) { return _r == _regim;});
    }

    Logic::AbstractItemDelegateWithRegim* delegateFromRegim(QObject* parent,
        Logic::MembersWidgetRegim _regim,
        Logic::CustomAbstractListModel* _membersModel,
        Logic::DrawIcons _needDrawIcons = Logic::DrawIcons::NoNeedDrawIcons)
    {
        Logic::AbstractItemDelegateWithRegim* delegate = nullptr;

        if (_regim == Logic::MembersWidgetRegim::COUNTRY_LIST)
            delegate = new Logic::CountryListItemDelegate(parent);
        else if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST)
            delegate = new Logic::SearchItemDelegate(parent);
        else if (_regim == Logic::MembersWidgetRegim::CALLS_LIST)
            delegate = new Logic::CallItemDelegate(parent);
        else if (_regim == Logic::MembersWidgetRegim::TASK_ASSIGNEE)
            delegate = new Logic::TaskAssigneeListDelegate(parent);
        else
            delegate = new Logic::ContactListItemDelegate(parent, _regim, _membersModel, _needDrawIcons);

        delegate->setReplaceFavorites(isRegimWithFavorites(_regim));
        return delegate;
    }

    QAbstractItemModel* modelForRegim(Logic::MembersWidgetRegim _regim, Logic::AbstractSearchModel* _searchModel, Logic::CustomAbstractListModel* _membersModel,
                                      Logic::ContactListWithHeaders* _clModel, Logic::CommonChatsModel* _commonChatsModel, Logic::CallsModel* _callsModel)
    {
        if (Logic::is_members_regim(_regim) || Logic::is_select_chat_members_regim(_regim))
            return _membersModel;

        if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP)
            return _clModel;

        if (_regim == Logic::MembersWidgetRegim::CALLS_LIST)
            return _callsModel;

        if (_regim == Logic::MembersWidgetRegim::COMMON_CHATS)
            return _commonChatsModel;

        if (_regim == Logic::MembersWidgetRegim::COUNTRY_LIST)
            return Logic::getCountryModel();

        static const auto needSearchModel =
        {
            Logic::MembersWidgetRegim::SELECT_MEMBERS,
            Logic::MembersWidgetRegim::SHARE,
            Logic::MembersWidgetRegim::SHARE_CONTACT,
            Logic::MembersWidgetRegim::TASK_ASSIGNEE,
            Logic::MembersWidgetRegim::SHARE_VIDEO_CONFERENCE,
            Logic::MembersWidgetRegim::VIDEO_CONFERENCE,
            Logic::MembersWidgetRegim::CONTACT_LIST,
            Logic::MembersWidgetRegim::UNKNOWN,
            Logic::MembersWidgetRegim::DISALLOWED_INVITERS_ADD,
        };
        if (std::any_of(needSearchModel.begin(), needSearchModel.end(), [_regim](const auto _r) { return _r == _regim;}))
            return _searchModel;

        return Logic::getContactListModel();
    }

    bool isServerSearchSupported(Logic::MembersWidgetRegim _regim)
    {
        return Ui::GetAppConfig().IsServerSearchEnabled() && (_regim == Logic::MembersWidgetRegim::CONTACT_LIST
            || _regim == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP
            || (Features::isGlobalContactSearchAllowed() && Logic::isRegimWithGlobalContactSearch(_regim)));
    }

    Logic::AbstractSearchModel* searchModelForRegim(QObject* parent, Logic::MembersWidgetRegim _regim, Logic::CustomAbstractListModel* _membersModel)
    {
        if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            auto m = new Logic::SearchModel(parent);
            m->setServerSearchEnabled(isServerSearchSupported(_regim));
            m->setForceAddFavorites(true);
            return m;
        }

        if (_regim == Logic::MembersWidgetRegim::COUNTRY_LIST)
            return Logic::getCountryModel();

        if (_regim == Logic::MembersWidgetRegim::IGNORE_LIST)
            return Logic::getIgnoreModel();

        if (_regim == Logic::MembersWidgetRegim::CALLS_LIST)
            return new Logic::CallsSearchModel(parent);

        if (_regim == Logic::MembersWidgetRegim::TASK_ASSIGNEE)
            return new Logic::TaskAssigneeModel(parent);

        if (!Logic::is_members_regim(_regim) && !Logic::is_select_chat_members_regim(_regim))
        {
            const auto contactsOnly =
                _regim == Logic::MembersWidgetRegim::SHARE_CONTACT ||
                _regim == Logic::MembersWidgetRegim::DISALLOWED_INVITERS_ADD ||
                (_regim != Logic::MembersWidgetRegim::CONTACT_LIST_POPUP && !Logic::is_share_regims(_regim));

            const auto excludeChats =
                contactsOnly ? Logic::SearchDataSource::localAndServer
                    : (Features::isGlobalContactSearchAllowed() && Logic::is_share_regims(_regim)) ? Logic::SearchDataSource::server
                        : Logic::SearchDataSource::none;

            const auto shareOrTask = Logic::is_share_regims(_regim) || _regim == Logic::MembersWidgetRegim::SHARE_CONTACT || _regim == Logic::MembersWidgetRegim::TASK_ASSIGNEE;

            auto m = new Logic::SearchModel(parent);
            m->setSearchInDialogs(false);
            m->setExcludeChats(excludeChats);
            m->setHideReadonly(shareOrTask);
            m->setServerSearchEnabled(isServerSearchSupported(_regim));
            m->setCategoriesEnabled(_regim == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP);
            m->setSortByTime(shareOrTask);
            m->setFavoritesOnTop(Logic::is_share_regims(_regim));
            m->setForceAddFavorites(Logic::is_share_regims(_regim) || _regim == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP);
            return m;
        }

        return qobject_cast<Logic::ChatMembersModel*>(_membersModel);
    }

    QSize placeholderSize() noexcept
    {
        return Utils::scale_value(QSize(84, 84));
    }

    QPixmap noResultsPixmap()
    {
        return Utils::renderSvg(qsl(":/placeholders/empty_search"), placeholderSize(), Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
    }

    const QString& getGlobalSearchHeaderName()
    {
        static const QString name = qsl("globalSearchViewHeader");
        return name;
    }
}

namespace Ui
{
    ContactListWidget::ContactListWidget(QWidget* _parent,
        const Logic::MembersWidgetRegim& _regim,
        Logic::CustomAbstractListModel* _chatMembersModel,
        Logic::AbstractSearchModel* _searchModel,
        Logic::CommonChatsModel* _commonChatsModel,
        Logic::DrawIcons _needDrawIcons)
        : QWidget(_parent)
        , scrollStatsTimer_(new QTimer(this))
        , viewContainer_(new QWidget(this))
        , regim_(_regim)
        , chatMembersModel_(_chatMembersModel)
        , searchModel_(_searchModel)
        , commonChatsModel_(_commonChatsModel)
        , popupMenu_(nullptr)
        , personTooltip_(new Utils::PersonTooltip(this))
        , needDrawIcons_(_needDrawIcons)
    {
        initSearchModel(_searchModel);
        clModel_ = new Logic::ContactListWithHeaders(this, Logic::getContactListModel(), false);

        callsModel_ = Logic::GetCallsModel();

        layout_ = Utils::emptyVLayout(this);
        viewLayout_ = Utils::emptyVLayout(viewContainer_);

        view_ = CreateFocusableViewAndSetTrScrollBar(this);
        view_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        view_->setFrameShape(QFrame::NoFrame);
        view_->setSpacing(0);
        view_->setModelColumn(0);
        view_->setUniformItemSizes(false);

        view_->setStyleSheet(qsl("background: transparent;"));

        view_->setMouseTracking(true);
        view_->setAcceptDrops(true);
        view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        view_->setAutoScroll(false);
        view_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        view_->setSelectByMouseHover(true);
        view_->setResizeMode(QListView::Adjust);
        view_->setListViewGestureHandler(new ListViewGestureHandler(view_));

        view_->setAttribute(Qt::WA_MacShowFocusRect, false);
        view_->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
        view_->viewport()->grabGesture(Qt::TapAndHoldGesture);
        view_->viewport()->grabGesture(Qt::TapGesture);
        Utils::grabTouchWidget(view_->viewport(), true);

        Testing::setAccessibleName(view_, qsl("AS Search view"));
        viewLayout_->addWidget(view_);
        Testing::setAccessibleName(viewContainer_, qsl("AS Search viewContainer"));
        layout_->addWidget(viewContainer_);

        switchToInitial(true);

        scrollStatsTimer_->setSingleShot(true);
        scrollStatsTimer_->setInterval(scrollTimeout);
        connect(scrollStatsTimer_, &QTimer::timeout, this, &ContactListWidget::onMouseWheeledStats);

        connect(view_, &Ui::FocusableListView::activated, this, &ContactListWidget::searchClicked, Qt::QueuedConnection);
        connect(view_, &Ui::FocusableListView::clicked, this, &ContactListWidget::onItemClicked);
        connect(view_->getListViewGestureHandler(), &Ui::ListViewGestureHandler::tapGesture, this, [this](const auto& idx)
        {
            Q_EMIT onItemClicked(idx);
        }, Qt::QueuedConnection); // to avoid GeneralDialog closing in call log
        connect(view_, &Ui::FocusableListView::pressed, this, &ContactListWidget::onItemPressed);
        connect(view_->getListViewGestureHandler(), &Ui::ListViewGestureHandler::rightClick, this, [this](const auto& _idx)
        {
            onItemPressedImpl(_idx, Qt::MouseButtons(Qt::MouseButton::RightButton));
        });
        connect(view_, &Ui::FocusableListView::mousePosChanged, this, &ContactListWidget::onMouseMoved);
        connect(view_, &Ui::FocusableListView::wheeled, this, &ContactListWidget::onMouseWheeled);
        connect(view_->verticalScrollBar(), &QScrollBar::valueChanged, Logic::getContactListModel(), &Logic::ContactListModel::scrolled);
        connect(view_->verticalScrollBar(), &QScrollBar::valueChanged, this, &ContactListWidget::scrolled);
        connect(QScroller::scroller(view_->viewport()), &QScroller::stateChanged, this, &ContactListWidget::touchScrollStateChanged, Qt::QueuedConnection);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showContactListPlaceholder, this, &ContactListWidget::showPlaceholder);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideContactListPlaceholder, this, &ContactListWidget::hidePlaceholder);

        connect(this, &ContactListWidget::searchSuggestRemoved, this, &ContactListWidget::removeSuggestPattern);

        if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP)
        {
            connect(this, &ContactListWidget::itemSelected, this, [this](const QString& /*_aimid*/, qint64 _msgid)
            {
                Q_EMIT forceSearchClear(!getSearchInDialog() && _msgid == -1 && Ui::get_gui_settings()->get_value<bool>(settings_fast_drop_search_results, settings_fast_drop_search_default()));
            });
        }

        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalThemeChanged, this, &ContactListWidget::onThemeChanged);
    }

    ContactListWidget::~ContactListWidget() = default;

    void ContactListWidget::connectSearchWidget(SearchWidget* _widget)
    {
        connect(_widget, &SearchWidget::enterPressed, this, &ContactListWidget::searchResult);
        connect(_widget, &SearchWidget::upPressed, this, &ContactListWidget::searchUpPressed);
        connect(_widget, &SearchWidget::downPressed, this, &ContactListWidget::searchDownPressed);
        connect(_widget, &SearchWidget::search, this, &ContactListWidget::searchPatternChanged);
        connect(_widget, &SearchWidget::inputCleared, this, &ContactListWidget::onSearchInputCleared);

        connect(this, &ContactListWidget::searchSuggestSelected, _widget, &SearchWidget::setText);
        connect(this, &ContactListWidget::searchEnd, _widget, &SearchWidget::searchCompleted);
        connect(this, &ContactListWidget::forceSearchClear, _widget, &SearchWidget::clearSearch);
        connect(this, &ContactListWidget::moreClicked, _widget, [_widget]()
        {
            _widget->setNeedClear(false);
        });

        if (searchDialogContact_.isEmpty() || !Logic::getContactListModel()->isThread(searchDialogContact_))
        {
            connect(_widget, &SearchWidget::escapePressed, this, []()
            {
                Logic::updatePlaceholders({ Logic::Placeholder::Contacts });
            });
        }
    }

    void ContactListWidget::installEventFilterToView(QObject* _filter)
    {
        view_->viewport()->installEventFilter(_filter);
    }

    QWidget* ContactListWidget::indexWidget(int _index)
    {
        return view_->indexWidget(searchModel_->index(_index, 0));
    }

    void ContactListWidget::setIndexWidget(int index, QWidget* widget)
    {
        view_->setIndexWidget(searchModel_->index(index, 0), widget);
    }

    void ContactListWidget::setClDelegate(Logic::AbstractItemDelegateWithRegim* _delegate)
    {
        clDelegate_ = _delegate;
        view_->setItemDelegate(_delegate);
    }

    void ContactListWidget::setWidthForDelegate(int _width)
    {
        clDelegate_->setFixedWidth(_width);
    }

    void ContactListWidget::setDragIndexForDelegate(const QModelIndex& _index)
    {
        auto delegate = isSearchMode() ? searchDelegate_ : clDelegate_;
        delegate->setDragIndex(_index);
    }

    void ContactListWidget::setPictureOnlyForDelegate(bool _value)
    {
        if (auto d = qobject_cast<Logic::ContactListItemDelegate*>(clDelegate_))
            d->setPictOnlyView(_value);

        if (auto d = qobject_cast<Logic::CallItemDelegate*>(clDelegate_))
            d->setPictOnlyView(_value);

        if (contactsPlaceholder_)
            contactsPlaceholder_->setPictureOnlyView(_value);
    }

    void ContactListWidget::setEmptyLabelVisible(bool _isVisible)
    {
        if (_isVisible && !EmptyListLabel_)
        {
            EmptyListLabel_ = new EmptyListLabel(this, regim_);
            Testing::setAccessibleName(EmptyListLabel_, qsl("AS Search emptyListLabel"));
            layout_->addWidget(EmptyListLabel_);
        }
        if (EmptyListLabel_)
            EmptyListLabel_->setVisible(_isVisible);
        viewContainer_->setVisible(!_isVisible);
    }

    void ContactListWidget::setSearchInDialog(const QString& _contact, bool _switchModel)
    {
        if (regim_ != Logic::MembersWidgetRegim::CONTACT_LIST && regim_ != Logic::MembersWidgetRegim::UNKNOWN)
            return;

        if (searchDialogContact_ != _contact)
        {
            auto searchModel = qobject_cast<Logic::SearchModel*>(searchModel_);
            if (searchModel)
                searchModel->setSearchInSingleDialog(_contact);
        }

        searchDialogContact_ = _contact;

        const auto isThread = Logic::getContactListModel()->isThread(searchDialogContact_);
        if (getSearchInDialog() && !isThread)
        {
            if (!dialogSearchViewHeader_)
            {
                dialogSearchViewHeader_ = new DialogSearchViewHeader(this);
                Testing::setAccessibleName(dialogSearchViewHeader_, qsl("AS Search dialogSearchViewHeader"));
                layout_->insertWidget(0, dialogSearchViewHeader_);
                dialogSearchViewHeader_->hide();

                if (openedAs_ == PageOpenedAs::MainPage)
                    connect(dialogSearchViewHeader_, &DialogSearchViewHeader::contactFilterRemoved, this, &ContactListWidget::onDisableSearchInDialog);

                connect(dialogSearchViewHeader_,
                    &DialogSearchViewHeader::contactFilterRemoved,
                    this,
                    [aimId = _contact]() { Q_EMIT Utils::InterConnector::instance().contactFilterRemoved(aimId); });
            }

            QString name;
            if (_contact == ServiceContacts::getThreadsName())
                name = QT_TRANSLATE_NOOP("search", "Threads");
            else
                name = Logic::getFriendlyContainerProxy(Logic::FriendlyContainerProxy::ReplaceFavorites).getFriendly(_contact);
            dialogSearchViewHeader_->setChatNameFilter(name);
        }

        if (dialogSearchViewHeader_)
            dialogSearchViewHeader_->setVisible(getSearchInDialog() && !isThread);

        if (_switchModel)
            view_->setModel(modelForRegim(regim_, searchModel_, chatMembersModel_, clModel_, commonChatsModel_, callsModel_));

        if (searchDelegate_)
            searchDelegate_->setReplaceFavorites(_contact.isEmpty() || Favorites::isFavorites(_contact));
    }

    bool ContactListWidget::getSearchInDialog() const
    {
        return !searchDialogContact_.isEmpty();
    }

    const QString& ContactListWidget::getSearchInDialogContact() const
    {
        return searchDialogContact_;
    }

    bool ContactListWidget::isSearchMode() const
    {
        return !initial_;
    }

    QString ContactListWidget::getSelectedAimid() const
    {
        const QModelIndexList indexes = view_->selectionModel()->selectedIndexes();
        if (indexes.isEmpty())
            return QString();

        return Logic::aimIdFromIndex(indexes.first());
    }

    Logic::MembersWidgetRegim ContactListWidget::getRegim() const
    {
        return regim_;
    }

    void ContactListWidget::setRegim(const Logic::MembersWidgetRegim _regim)
    {
        if (regim_ == _regim)
            return;

        regim_ = _regim;
        update();
    }

    void ContactListWidget::enableCLDelegateOpacity(bool _enable)
    {
        clDelegate_->setOpacityEnabled(_enable);
    }

    bool ContactListWidget::isCLDelegateOpacityEnabled() const
    {
        return clDelegate_->isOpacityEnabled();
    }

    Logic::AbstractSearchModel* ContactListWidget::getSearchModel() const
    {
        return searchModel_;
    }

    FocusableListView* ContactListWidget::getView() const
    {
        return view_;
    }

    void ContactListWidget::showSearch()
    {
        switchToInitial(false);
    }

    void ContactListWidget::rewindToTop()
    {
        const auto model = qobject_cast<const Logic::SearchModel*>(view_->model());
        if (!model || !model->rowCount())
            return;

        QModelIndex i = model->index(0);
        if (const auto item = i.data().value<Data::AbstractSearchResultSptr>())
            scrollToItem(item->getAimId());
    }

    void ContactListWidget::searchResult()
    {
        if (isVisible())
            onItemClicked(view_->selectionModel()->currentIndex());
    }

    void ContactListWidget::searchUpPressed()
    {
        if (isVisible())
            searchUpOrDownPressed(true);
    }

    void ContactListWidget::searchDownPressed()
    {
        if (isVisible())
            searchUpOrDownPressed(false);
    }

    void ContactListWidget::selectionChanged(const QModelIndex & _current)
    {
        const auto aimid = Logic::aimIdFromIndex(_current);

        if (aimid.isEmpty())
            return;

        qint64 msgId = -1;
        highlightsV highlights;
        if (const auto searchModel = qobject_cast<const Logic::SearchModel*>(_current.model()))
        {
            const auto searchRes = _current.data().value<Data::AbstractSearchResultSptr>();
            if (searchRes && searchRes->isMessage())
            {
                msgId = searchRes->getMessageId();
                highlights = searchRes->highlights_;
            }
        }

        select(aimid, msgId, highlights, Logic::UpdateChatSelection::No, false, openedAs_);

        if (msgId == -1)
            Q_EMIT itemClicked(aimid);

        const auto index = view_->selectionModel()->currentIndex();
        if (!isSelectMembersRegim() && index.isValid())
            view_->selectionModel()->clear();

        view_->update(index);
    }

    void ContactListWidget::select(const QString& _aimId)
    {
        select(_aimId, -1, {}, Logic::UpdateChatSelection::No);
    }

    void ContactListWidget::select(const QString& _aimId, const qint64 _messageId, const highlightsV& _highlights, Logic::UpdateChatSelection _mode, bool _ignoreScroll, PageOpenedAs _openedAs)
    {
        const auto isSelectMembers = isSelectMembersRegim();
        const auto isThread = Logic::getContactListModel()->isThread(_aimId);
        const auto isTask = Logic::getContactListModel()->isTaskThread(_aimId);
        const auto isServiceContact = ServiceContacts::isServiceContact(Logic::getContactListModel()->selectedContact())
                || !Utils::InterConnector::instance().getAppsPage()->isMessengerPage();

        auto selectionAimId = _aimId;
        if (isThread)
        {
            if (isServiceContact || Logic::getContactListModel()->isTaskThread(_aimId))
                selectionAimId = Logic::getContactListModel()->selectedContact();
            else
                selectionAimId = Logic::getContactListModel()->getThreadParent(_aimId).value_or(QString());
        }

        if (selectionAimId.isEmpty() || isTask)
            selectionAimId = Logic::getContactListModel()->selectedContact();

        const auto parentMessageId = Logic::getContactListModel()->getThreadParentMessage(_aimId);
        const auto msgId = isThread && parentMessageId != -1 ? parentMessageId : _messageId;

        if (!isTask && !isSelectMembers && regim_ != Logic::MembersWidgetRegim::IGNORE_LIST && regim_ != Logic::MembersWidgetRegim::COUNTRY_LIST)
        {
            if (!isThread)
                Q_EMIT Utils::InterConnector::instance().threadSearchClosed();
            Utils::InterConnector::instance().openDialog(selectionAimId, msgId, regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP, _openedAs);
        }

        if (parentMessageId != -1 || _messageId == -1 || _mode == Logic::UpdateChatSelection::Yes)
            Q_EMIT changeSelected(_aimId);

        Q_EMIT itemSelected(selectionAimId, msgId, isThread ? _messageId : -1, _highlights, _ignoreScroll);

        if (isThread)
        {
            if (closeOnThreadResult_)
                Q_EMIT Utils::InterConnector::instance().searchEnd();
            else
                Q_EMIT Utils::InterConnector::instance().threadSearchClosed();

            Utils::InterConnector::instance().openThreadSearchResult(_aimId, _messageId, _highlights);
        }

        if (regim_ != Logic::MembersWidgetRegim::UNKNOWN && !isSelectMembers && _messageId == -1)
            Q_EMIT searchEnd();
    }

    void ContactListWidget::onItemClicked(const QModelIndex& _current)
    {
        const auto srchModel = qobject_cast<const Logic::SearchModel*>(_current.model());
        const auto callsModel = qobject_cast<const Logic::CallsModel*>(_current.model());
        const auto callsSearchModel = qobject_cast<const Logic::CallsSearchModel*>(_current.model());
        const auto changeSelection =
            !(QApplication::mouseButtons() & Qt::RightButton)
            && !(srchModel && srchModel->isServiceItem(_current));

        if (srchModel)
        {
            const auto searchRes = _current.data().value<Data::AbstractSearchResultSptr>();
            if (searchRes)
            {
                if (changeSelection)
                {
                    std::string statResType;
                    auto resChat = std::static_pointer_cast<Data::SearchResultChat>(searchRes);
                    if (searchRes->isSuggest())
                    {
                        const auto rect = view_->visualRect(_current);
                        const auto cursorPos = view_->mapFromGlobal(QCursor::pos());

                        if (getDeleteSuggestBtnRect(rect).contains(cursorPos))
                        {
                            Q_EMIT searchSuggestRemoved(srchModel->getSingleDialogAimId(), searchRes->getFriendlyName());
                        }
                        else
                        {
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_search_dialog_suggest_click);
                            Q_EMIT searchSuggestSelected(searchRes->getFriendlyName());
                        }
                    }
                    else if (searchRes->isMessage() || searchRes->isLocalResult_)
                    {
                        if (searchRes->isMessage() && searchRes->dialogSearchResult_)
                        {
                            if (const auto dialog = srchModel->getSingleDialogAimId(); !dialog.isEmpty())
                            {
                                if (const auto pattern = srchModel->getSearchPattern(); !pattern.isEmpty())
                                {
                                    static QString prevPattern;
                                    static QString prevDialog;
                                    if (pattern != prevPattern || dialog != prevDialog)
                                    {
                                        prevPattern = pattern;
                                        prevDialog = dialog;

                                        Logic::getLastSearchPatterns()->addPattern(pattern, dialog);
                                    }
                                }
                            }
                        }

                        statResType = searchRes->isMessage() ? "messages" : (searchRes->isChat() ? "own_chats" : "contacts");

                        if (openedAs_ == PageOpenedAs::FeedPage)
                        {
                            const auto aimId = Logic::aimIdFromIndex(_current);

                            qint64 msgId = -1;
                            highlightsV highlights;
                            if (const auto searchModel = qobject_cast<const Logic::SearchModel*>(_current.model()))
                            {
                                const auto searchRes = _current.data().value<Data::AbstractSearchResultSptr>();
                                if (searchRes && searchRes->isMessage())
                                {
                                    msgId = searchRes->getMessageId();
                                    highlights = searchRes->highlights_;
                                }
                            }

                            Q_EMIT Utils::InterConnector::instance().scrollToNewsFeedMesssage(aimId, msgId, highlights);
                        }
                        else
                        {
                            Q_EMIT Utils::InterConnector::instance().clearDialogHistory();
                            selectionChanged(_current);
                        }
                    }
                    else if (!searchRes->isLocalResult_ && Features::isGlobalContactSearchAllowed() && Logic::isRegimWithGlobalContactSearch(regim_))
                    {
                        statResType = "contacts";
                        Q_EMIT Utils::InterConnector::instance().clearDialogHistory();
                        selectionChanged(_current);
                    }
                    else
                    {
                        const auto aimid = searchRes->getAimId();
                        Utils::InterConnector::instance().openDialog(aimid, -1, false);
                        Q_EMIT itemSelected(aimid, -1, -1, {});

                        statResType = searchRes->isChat() ? "public_chats" : "contacts";
                    }

                    if (!statResType.empty())
                    {
                        if ((regim_ == Logic::MembersWidgetRegim::CONTACT_LIST || regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP) && !getSearchInDialog())
                            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::globalsearch_selected, {{ "type", statResType }});
                    }
                }
            }
        }
        else if (callsModel || callsSearchModel)
        {
            auto item = _current.data(Qt::DisplayRole).value<Data::CallInfoPtr>();
            if (item)
            {
                if (item->isService())
                {
                    if (item->getServiceAimid() == u"~group_call~")
                        Q_EMIT Utils::InterConnector::instance().createGroupCall();

                    if (item->getServiceAimid() == u"~call_by_link~")
                        Q_EMIT Utils::InterConnector::instance().createCallByLink();

                    if (item->getServiceAimid() == u"~webinar~")
                        Q_EMIT Utils::InterConnector::instance().createWebinar();
                }
                else
                {
                    Q_EMIT Utils::InterConnector::instance().callContact(item->getMembers(), item->getFriendly(), item->isVideo());
                }
            }
        }
        else if (regim_ == Logic::MembersWidgetRegim::DISALLOWED_INVITERS)
        {
            const auto aimid = Logic::aimIdFromIndex(_current);
            if (aimid.isEmpty())
                Q_EMIT Utils::InterConnector::instance().showAddToDisallowedInvitersDialog();
            else
                selectionChanged(_current);
        }
        else if (changeSelection)
        {
            if (regim_ != Logic::MembersWidgetRegim::COMMON_CHATS)
                 Q_EMIT Utils::InterConnector::instance().clearDialogHistory();

            selectionChanged(_current);
        }
    }

    void ContactListWidget::onItemPressed(const QModelIndex& _current)
    {
        onItemPressedImpl(_current, QApplication::mouseButtons());
    }

    void ContactListWidget::onItemPressedImpl(const QModelIndex& _current, Qt::MouseButtons _buttons)
    {
        if ((regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP || regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
            && (_buttons & Qt::RightButton))
        {
            const auto model = qobject_cast<const Logic::CustomAbstractListModel*>(_current.model());
            if (model && model->isServiceItem(_current))
                return;

            const auto searchModel = qobject_cast<const Logic::SearchModel*>(_current.model());
            if (searchModel)
            {
                const auto searchRes = _current.data().value<Data::AbstractSearchResultSptr>();
                if (searchRes)
                {
                    const auto cont = Logic::getContactListModel()->getContactItem(searchRes->getAimId());
                    if (cont)
                        showContactsPopupMenu(cont->get_aimid(), (cont->is_chat() || searchRes->isMessage()));
                    else
                        showContactsNotFromCLPopupMenu(searchRes->getAimId());
                }
            }
            else
            {
                const auto cont = _current.data(Qt::DisplayRole).value<Data::Contact*>();
                if (cont)
                    showContactsPopupMenu(cont->AimId_, cont->Is_chat_);
            }
        }

        if ((regim_ == Logic::MembersWidgetRegim::CALLS_LIST) && (_buttons & Qt::RightButton))
        {
            showCallsPopupMenu(_current.data().value<Data::CallInfoPtr>());
        }

        if (_current.isValid())
        {
            if (const auto aimId = Logic::aimIdFromIndex(_current); !aimId.isEmpty())
            {
                if (Utils::isChat(aimId))
                {
                    Q_EMIT selected(aimId);
                }
                else
                {
                    const auto rightX = width() - Utils::scale_value(8);
                    const auto removeX = rightX - Utils::scale_value(32);
                    const auto approveX = removeX - Utils::scale_value(32);

                    if (auto dlgt = qobject_cast<Logic::ContactListItemDelegate*>(clDelegate_))
                    {
                        const auto cursorPos = view_->mapFromGlobal(QCursor::pos());
                        const auto regim = dlgt->regim();
                        if (cursorPos.x() > removeX && cursorPos.x() <= rightX)
                        {
                            if (regim == Logic::MembersWidgetRegim::ADMIN_MEMBERS)
                            {
                                Q_EMIT moreClicked(aimId);
                            }
                            else if (regim == Logic::MembersWidgetRegim::PENDING_MEMBERS)
                            {
                                Q_EMIT approve(aimId, false);
                            }
                            else if (regim == Logic::MembersWidgetRegim::MEMBERS_LIST || regim == Logic::MembersWidgetRegim::YOUR_INVITES_LIST)
                            {
                                Q_EMIT removeClicked(aimId);
                            }
                            else
                            {
                                const auto searchRes = _current.data().value<Data::AbstractSearchResultSptr>();
                                if (searchRes && qobject_cast<const Logic::ChatMembersModel*>(_current.model()))
                                {
                                    const auto& memberRes = std::static_pointer_cast<Data::SearchResultChatMember>(searchRes);
                                    if (memberRes->canRemoveTill().isValid() && QDateTime::currentDateTime() < memberRes->canRemoveTill())
                                        Q_EMIT removeClicked(aimId);
                                    else
                                        Q_EMIT selected(aimId);
                                }
                                else
                                {
                                    Q_EMIT selected(aimId);
                                }
                            }
                        }
                        else if (regim == Logic::MembersWidgetRegim::PENDING_MEMBERS && cursorPos.x() >= approveX && cursorPos.x() <= removeX)
                        {
                            Q_EMIT approve(aimId, true);
                        }
                        else
                        {
                            Q_EMIT selected(aimId);
                        }
                    }
                }
            }
        }
    }

    void ContactListWidget::onMouseMoved(const QPoint& _pos, const QModelIndex& _index)
    {
        if (!_index.isValid())
            return;

        setKeyboardFocused(false);

        const auto searchModel = qobject_cast<const Logic::SearchModel*>(_index.model());
        auto searchDelegate = qobject_cast<Logic::SearchItemDelegate*>(clDelegate_);

        auto clDelegate = qobject_cast<Logic::ContactListItemDelegate*>(clDelegate_);
        if (clDelegate && searchModel)
            clDelegate->setModel(const_cast<Logic::SearchModel*>(searchModel));

        if (searchModel && searchDelegate)
        {
            searchDelegate->setModel(const_cast<Logic::SearchModel*>(searchModel));
            if (const auto item = _index.data().value<Data::AbstractSearchResultSptr>())
            {
                bool updated = false;
                if (item->isSuggest())
                {
                    const auto rect = view_->visualRect(_index);
                    if (getDeleteSuggestBtnRect(rect).contains(_pos))
                        updated = searchDelegate->setHoveredSuggetRemove(_index);
                    else
                        updated = searchDelegate->setHoveredSuggetRemove(QModelIndex());
                }
                else
                    updated = searchDelegate->setHoveredSuggetRemove(QModelIndex());

                if (updated)
                    view_->update();
            }
        }

        bool showMemberTooltip = false;
        const auto aimId = Logic::senderAimIdFromIndex(_index);
        const auto tooltipRect = getAvatarRect(_index);

        if ((!Favorites::isFavorites(aimId) || !isRegimWithFavorites(regim_)) && tooltipRect.contains(view_->mapFromGlobal(QCursor::pos())))
        {
            auto linkGlobalRect = QRect(view_->mapToGlobal(tooltipRect.topLeft()), tooltipRect.size());
            personTooltip_->show(Utils::PersonTooltipType::Person, aimId, linkGlobalRect, Tooltip::ArrowDirection::Down, Tooltip::ArrowPointPos::Top);
            showMemberTooltip = true;
        }

        if (Features::longPathTooltipsAllowed())
        {
            const auto aimId = Logic::senderAimIdFromIndex(_index);
            auto delegate = isSearchMode() ? searchDelegate_ : clDelegate_;
            auto cursorPos = _pos;
            if (!aimId.isEmpty() && delegate->needsTooltip(aimId, _index, _pos))
            {
                if (tooltipIndex_ != _index)
                {
                    tooltipIndex_ = _index;

                    const auto rect = view_->visualRect(_index);
                    auto ttRect = QRect(view_->mapToGlobal(rect.topLeft()), rect.size());
                    auto callItemDelegate = qobject_cast<Logic::CallItemDelegate*>(delegate);
                    const auto aimId = Logic::senderAimIdFromIndex(_index);
                    auto name = callItemDelegate ? callItemDelegate->getFriendly(_index) : Logic::GetFriendlyContainer()->getFriendly(aimId);
                    const auto direction = rect.y() < 0 ? Tooltip::ArrowDirection::Up : Tooltip::ArrowDirection::Down;
                    const auto arrowPosition = rect.y() < 0 ? Tooltip::ArrowPointPos::Bottom : Tooltip::ArrowPointPos::Top;
                    Data::DlgState state = Logic::getRecentsModel()->getDlgState(aimId);
                    Ui::ViewParams viewParams;
                    Logic::IconsDelegate* iconsDelegate = dynamic_cast<Logic::IconsDelegate*>(delegate);
                    const bool needDrawDraft = iconsDelegate ? iconsDelegate->needDrawDraft(viewParams, state): false;
                    auto draftIconRect = iconsDelegate ? iconsDelegate->getDraftIconRect() : QRect();
                    auto cursorInDraftRect = draftIconRect.contains(_pos);
                    draftIconRect.moveTop(ttRect.y());
                    draftIconRect.moveLeft(ttRect.x() + draftIconRect.x() - draftIconRect.width()/2);
                    const bool needShowDraftTooltip = needDrawDraft && cursorInDraftRect;
                    const auto draftMessageFormatted = makeDraftText(aimId);
                    personTooltip_->show(Utils::PersonTooltipType::Text, needShowDraftTooltip ? draftMessageFormatted : name, needShowDraftTooltip ? draftIconRect : ttRect, direction, arrowPosition, TextRendering::HorAligment::CENTER);
                }
            }
            else if (!showMemberTooltip)
            {
                hideTooltip();
            }
        }
    }

    void ContactListWidget::onMouseWheeled()
    {
        if (regim_ != Logic::MembersWidgetRegim::CONTACT_LIST || !getSearchInDialog() || !searchModel_->rowCount())
            return;

        std::string newWhere = searchModel_->getSearchPattern().isEmpty() ? "history" : "result";
        if (!scrollStatWhere_.empty() && scrollStatWhere_ != newWhere)
            onMouseWheeledStats();

        scrollStatWhere_ = std::move(newWhere);
        scrollStatsTimer_->start();
    }

    void ContactListWidget::onMouseWheeledStats()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_search_dialog_scroll, { { "where", std::exchange(scrollStatWhere_, {}) } });
    }

    void ContactListWidget::onSearchResults()
    {
        const auto model = qobject_cast<const Logic::AbstractSearchModel*>(view_->model());
        if (!model || !model->rowCount() || model->getSearchPattern().isEmpty())
            return;

        if (!ServiceContacts::isServiceContact(searchDialogContact_)
            && (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST || regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP))
        {
            if (const auto searchModel = qobject_cast<const Logic::SearchModel*>(model))
            {
                const auto headers = getCurrentCategories();
                if (std::any_of(headers.begin(), headers.end(), [](const auto& _p) { return _p.second.isValid(); }))
                {
                    if (!globalSearchViewHeader_)
                    {
                        globalSearchViewHeader_ = new GlobalSearchViewHeader(this);
                        globalSearchViewHeader_->setObjectName(getGlobalSearchHeaderName());
                        Testing::setAccessibleName(globalSearchViewHeader_, qsl("AS Search globalSearchViewHeader"));
                        globalSearchViewHeader_->hide();

                        if (regim_ != Logic::MembersWidgetRegim::CONTACT_LIST)
                            globalSearchViewHeader_->setCategoryVisible(SearchCategory::Messages, false);

                        connect(globalSearchViewHeader_, &GlobalSearchViewHeader::categorySelected, this, &ContactListWidget::scrollToCategory);
                    }

                    if (auto w = layout_->findChild<GlobalSearchViewHeader*>(getGlobalSearchHeaderName()))
                        layout_->removeWidget(w);

                    layout_->insertWidget(getSearchInDialog() && !Logic::getContactListModel()->isThread(searchDialogContact_) ? 1 : 0, globalSearchViewHeader_);

                    globalSearchViewHeader_->setCategoryVisible(SearchCategory::ContactsAndGroups, !getSearchInDialog());

                    auto threadsHeader = std::find_if(headers.begin(), headers.end(), [](const auto& _h) { return _h.first == SearchCategory::Threads; });
                    if (threadsHeader != headers.end())
                        globalSearchViewHeader_->setCategoryVisible(SearchCategory::Threads, threadsHeader->second.isValid() && getSearchInDialog());

                    for (const auto& [cat, ndx] : headers)
                        globalSearchViewHeader_->setCategoryVisible(cat, ndx.isValid());
                    globalSearchViewHeader_->show();

                    selectCurrentSearchCategory();
                }
                else if (globalSearchViewHeader_)
                {
                    globalSearchViewHeader_->hide();
                }
            }
        }

        const auto selectFirst = !searchResultsRcvdFirst_ && (!view_->selectionModel()->hasSelection() || view_->verticalScrollBar()->value() == 0);
        if (selectFirst)
        {
            QModelIndex i = model->index(0);

            while (model->isServiceItem(i))
            {
                i = model->index(i.row() + 1);
                if (!i.isValid())
                    return;
            }

            {
                QSignalBlocker sb(view_->selectionModel());
                view_->selectionModel()->setCurrentIndex(i, QItemSelectionModel::ClearAndSelect);
            }
            view_->update(i);
            view_->scrollTo(i);
        }
        searchResultsRcvdFirst_ = true;
    }

    void ContactListWidget::onSearchSuggests()
    {
        view_->updateSelectionUnderCursor();
    }

    void ContactListWidget::searchResults(const QModelIndex & _current, const QModelIndex &)
    {
        if (regim_ != Logic::MembersWidgetRegim::CONTACT_LIST)
            return;

        if (!_current.isValid())
        {
            Q_EMIT searchEnd();
            return;
        }

        if (qobject_cast<const Logic::SearchModel*>(_current.model()))
        {
            selectionChanged(_current);
            Q_EMIT searchEnd();

            if (auto aimid = Logic::aimIdFromIndex(_current); !aimid.isEmpty())
            {
                qint64 msgId = -1;
                highlightsV highlights;
                if (const auto searchModel = qobject_cast<const Logic::SearchModel*>(_current.model()))
                {
                    const auto searchRes = _current.data().value<Data::AbstractSearchResultSptr>();
                    if (searchRes && searchRes->isMessage())
                    {
                        msgId = searchRes->getMessageId();
                        highlights = searchRes->highlights_;
                    }
                }
                Q_EMIT itemSelected(aimid, msgId, -1, highlights);
            }
            return;
        }

        auto cont = _current.data().value<Data::Contact*>();
        if (!cont)
        {
            view_->clearSelection();
            view_->selectionModel()->clearCurrentIndex();
            return;
        }

        if (cont->GetType() != Data::GROUP)
            select(cont->AimId_);

        view_->clearSelection();
        view_->selectionModel()->clearCurrentIndex();

        Q_EMIT searchEnd();
    }

    void ContactListWidget::searchClicked(const QModelIndex& _current)
    {
        searchResults(_current, QModelIndex());
    }

    void ContactListWidget::showPopupMenu(QAction* _action)
    {
        Logic::showContactListPopup(_action);
    }

    void ContactListWidget::showContactsPopupMenu(const QString& aimId, bool _is_chat)
    {
        if (Logic::is_members_regim(regim_) || Logic::is_select_members_regim(regim_) || Favorites::isFavorites(aimId))
            return;

        if (!popupMenu_)
        {
            popupMenu_ = new ContextMenu(this);
            Testing::setAccessibleName(popupMenu_, qsl("AS Search view popupMenu"));
            connect(popupMenu_, &ContextMenu::triggered, this, &ContactListWidget::showPopupMenu);
        }
        else
        {
            popupMenu_->clear();
        }

        const Data::DlgState dlg = Logic::getRecentsModel()->getDlgState(aimId);

        Logic::addItemToContextMenu(popupMenu_, aimId, dlg);
    }

    void ContactListWidget::showCallsPopupMenu(Data::CallInfoPtr _call)
    {
        if (!_call || _call->isService())
            return;

        if (!popupMenu_)
        {
            popupMenu_ = new ContextMenu(this);
            connect(popupMenu_, &ContextMenu::triggered, this, &ContactListWidget::showPopupMenu);
        }
        else
        {
            popupMenu_->clear();
        }

        popupMenu_->addActionWithIcon(qsl(":/context_menu/delete"), QT_TRANSLATE_NOOP("context_menu", "Remove"), Logic::makeData(qsl("calls/remove"), _call));

        popupMenu_->popup(QCursor::pos());
    }


    void ContactListWidget::showContactsNotFromCLPopupMenu(const QString& _aimId)
    {
        if (Logic::is_members_regim(regim_) || Logic::is_select_members_regim(regim_) || Favorites::isFavorites(_aimId))
            return;

        if (!popupMenu_)
        {
            popupMenu_ = new ContextMenu(this);
            Testing::setAccessibleName(popupMenu_, qsl("AS Search view popupMenu"));
            connect(popupMenu_, &ContextMenu::triggered, this, &ContactListWidget::showPopupMenu);
        }
        else
        {
            popupMenu_->clear();
        }

        popupMenu_->addActionWithIcon(qsl(":/context_menu/profile"), QT_TRANSLATE_NOOP("context_menu", "Profile"), Logic::makeData(qsl("contacts/Profile"), _aimId));

        if (!Logic::getRecentsModel()->isUnimportant(_aimId))
        {
            if (Logic::getRecentsModel()->isFavorite(_aimId))
                popupMenu_->addActionWithIcon(qsl(":/context_menu/unpin"), QT_TRANSLATE_NOOP("context_menu", "Unpin"), Logic::makeData(qsl("recents/unfavorite"), _aimId));
            else
                popupMenu_->addActionWithIcon(qsl(":/context_menu/pin"), QT_TRANSLATE_NOOP("context_menu", "Pin"), Logic::makeData(qsl("recents/favorite"), _aimId));
        }

        if (!Logic::getRecentsModel()->isFavorite(_aimId))
        {
            if (Logic::getRecentsModel()->isUnimportant(_aimId))
                popupMenu_->addActionWithIcon(qsl(":/context_menu/unmark_unimportant"), QT_TRANSLATE_NOOP("context_menu", "Remove from Unimportant"), Logic::makeData(qsl("recents/remove_from_unimportant"), _aimId));
            else
                popupMenu_->addActionWithIcon(qsl(":/context_menu/mark_unimportant"), QT_TRANSLATE_NOOP("context_menu", "Move to Unimportant"), Logic::makeData(qsl("recents/mark_unimportant"), _aimId));
        }
        popupMenu_->popup(QCursor::pos());
    }

    void ContactListWidget::onSearchInputCleared()
    {
        if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST && getSearchInDialog())
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_search_dialog_cancel, { { "type", "inside" } });
    }

    void ContactListWidget::searchPatternChanged(const QString& _newPattern)
    {
        switchToInitial(_newPattern.isEmpty());

        const auto needSendStat =
            (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST || regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP)
            && searchModel_->getSearchPattern().isEmpty()
            && !QStringView(_newPattern).trimmed().isEmpty()
            && !getSearchInDialog();
        if (needSendStat)
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::globalsearch_start);

        searchResultsStatsSent_ = false;

        searchModel_->setServerSearchEnabled(isServerSearchSupported(regim_));
        searchModel_->setSearchPattern(_newPattern);

        view_->clearSelection();
    }

    void ContactListWidget::onDisableSearchInDialog()
    {
        if (globalSearchViewHeader_)
            globalSearchViewHeader_->hide();

        Q_EMIT Utils::InterConnector::instance().disableSearchInDialog();

        if (searchModel_)
        {
            if (!searchModel_->getSearchPattern().isEmpty())
                searchModel_->repeatSearch();
            else
                switchToInitial(true);
        }

        setSearchInDialog(QString(), false);
    }

    void ContactListWidget::touchScrollStateChanged(QScroller::State _state)
    {
        view_->blockSignals(_state != QScroller::Inactive);
        view_->selectionModel()->blockSignals(_state != QScroller::Inactive);
        if (!isSearchMode())
            view_->selectionModel()->setCurrentIndex(Logic::getContactListModel()->contactIndex(Logic::getContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
        clDelegate_->blockState(_state != QScroller::Inactive);
    }

    void ContactListWidget::scrollToCategory(const SearchCategory _category, bool _animated)
    {
        if (auto srchModel = qobject_cast<Logic::SearchModel*>(searchModel_))
            srchModel->setSearchFormat(_category == SearchCategory::Threads ? Logic::SearchFormat::Threads : Logic::SearchFormat::ContactsAndMessages);

        switch (_category)
        {
        case SearchCategory::ContactsAndGroups:
            scrollToItem(Logic::SearchModel::getContactsAndGroupsAimId());
            break;
        case SearchCategory::Messages:
            scrollToItem(Logic::SearchModel::getMessagesAimId(), _animated);
            break;
        case SearchCategory::Threads:
            scrollToItem(Logic::SearchModel::getThreadsAimId(), _animated);
            break;
        default:
            break;
        }
    }

    void ContactListWidget::scrollToItem(const QString& _aimId, bool _animated)
    {
        if (_aimId.isEmpty())
            return;

        const auto model = qobject_cast<const Logic::SearchModel*>(view_->model());
        if (!model || !model->rowCount())
            return;

        QModelIndex i = model->index(0);
        do
        {
            if (const auto item = i.data().value<Data::AbstractSearchResultSptr>())
            {
                if (item->getAimId() == _aimId)
                    break;
            }

            i = model->index(i.row() + 1);
        }
        while (i.isValid());

        if (i.isValid())
        {
            auto scrollbar = view_->verticalScrollBar();
            const auto curValue = scrollbar->value();
            auto endValue = curValue;

            setUpdatesEnabled(false);
            {
                QSignalBlocker sb(scrollbar);
                view_->scrollTo(i, QAbstractItemView::PositionAtTop);
                endValue = scrollbar->value();
                scrollbar->setValue(curValue);
            }
            setUpdatesEnabled(true);

            if (_animated)
            {
                ensureScrollToItemAnimationInitialized();
                scrollToItemAnim_->stop();
                scrollToItemAnim_->setStartValue(curValue);
                scrollToItemAnim_->setEndValue(endValue);
                scrollToItemAnim_->start();
            }
            else
            {
                view_->verticalScrollBar()->setValue(endValue);
            }
        }
    }

    void ContactListWidget::removeSuggestPattern(const QString& _contact, const QString& _pattern)
    {
        Logic::getLastSearchPatterns()->removePattern(_contact, _pattern);
        searchModel_->repeatSearch();

        view_->updateSelectionUnderCursor();
    }

    void ContactListWidget::showPlaceholder()
    {
        constexpr Logic::MembersWidgetRegim noNeedPlaceholder[] =
        {
            Logic::MembersWidgetRegim::IGNORE_LIST,
            Logic::MembersWidgetRegim::COUNTRY_LIST,
            Logic::MembersWidgetRegim::DISALLOWED_INVITERS_ADD,
            Logic::MembersWidgetRegim::DISALLOWED_INVITERS_ADD,
        };

        if (isSearchMode() || std::any_of(std::begin(noNeedPlaceholder), std::end(noNeedPlaceholder), [regim = regim_](auto r) { return r == regim; }))
            return;

        if (!contactsPlaceholder_)
        {
            contactsPlaceholder_ = new ContactsPlaceholder(this);
            Testing::setAccessibleName(contactsPlaceholder_, qsl("AS Search contactsPlaceholder"));
            viewLayout_->addWidget(contactsPlaceholder_);
        }
        else
        {
            contactsPlaceholder_->show();
        }

        view_->hide();
    }

    void ContactListWidget::hidePlaceholder()
    {
        if (contactsPlaceholder_)
            contactsPlaceholder_->hide();

        view_->show();
    }

    void ContactListWidget::showNoSearchResults()
    {
        if (isSearchMode() && searchModel_->rowCount() == 0)
        {
            viewContainer_->hide();

            if (noSearchResultsShown_)
                return;

            if (globalSearchViewHeader_)
                globalSearchViewHeader_->hide();

            noSearchResultsShown_ = true;
            if (!noSearchResults_)
            {
                viewContainer_->hide();
                noSearchResults_ = new QWidget(this);
                noSearchResults_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                {
                    auto mainLayout = Utils::emptyVLayout(noSearchResults_);
                    {
                        auto noSearchResultsWidget = new QWidget(noSearchResults_);
                        auto noSearchLayout = new QVBoxLayout(noSearchResultsWidget);
                        noSearchResultsWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noSearchLayout->setAlignment(Qt::AlignCenter);
                        noSearchLayout->setContentsMargins(0, Utils::scale_value(60), 0, 0);
                        noSearchLayout->setSpacing(Utils::scale_value(36));
                        {
                            noResultsPlaceholder_ = new QLabel(noSearchResultsWidget);

                            noResultsPlaceholder_->setPixmap(noResultsPixmap());
                            noResultsPlaceholder_->setFixedSize(placeholderSize());
                            Testing::setAccessibleName(noResultsPlaceholder_, qsl("AS Search noSearchResultsPlaceholder"));

                            auto hLayout = Utils::emptyHLayout();
                            hLayout->addWidget(noResultsPlaceholder_);
                            noSearchLayout->addLayout(hLayout);
                        }
                        {
                            auto noSearchResultsLabel = new LabelEx(noSearchResultsWidget);
                            noSearchResultsLabel->setColor(Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY });
                            noSearchResultsLabel->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold));
                            noSearchResultsLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                            noSearchResultsLabel->setAlignment(Qt::AlignCenter);
                            noSearchResultsLabel->setText(QT_TRANSLATE_NOOP("placeholders", "Nothing found"));
                            Testing::setAccessibleName(noSearchResultsLabel, qsl("AS Search noSearchResultsLabel"));
                            noSearchLayout->addWidget(noSearchResultsLabel);
                        }

                        Testing::setAccessibleName(noSearchResultsWidget, qsl("AS Search noSearchResultsWidget"));
                        mainLayout->addWidget(noSearchResultsWidget);
                        mainLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));
                    }
                }

                layout_->addWidget(noSearchResults_);
            }
            else
            {
                viewContainer_->hide();
                noSearchResults_->show();
            }
        }
    }

    void ContactListWidget::hideNoSearchResults()
    {
        if (noSearchResultsShown_)
        {
            noSearchResults_->hide();
            viewContainer_->show();
            noSearchResultsShown_ = false;
        }
    }

    void ContactListWidget::showSearchSpinner()
    {
        viewContainer_->hide();

        if (!searchSpinner_)
        {
            searchSpinner_ = new QWidget(this);
            searchSpinner_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            {
                auto spinner = new RotatingSpinner(searchSpinner_);
                spinner->startAnimation();
                Testing::setAccessibleName(spinner, qsl("AS Search searchSpinnerWidget"));

                auto mainLayout = Utils::emptyVLayout(searchSpinner_);
                mainLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
                mainLayout->addSpacing(Utils::scale_value(80));
                mainLayout->addWidget(spinner);
            }

            layout_->addWidget(searchSpinner_);
        }
    }

    void ContactListWidget::hideSearchSpinner()
    {
        if (searchSpinner_)
        {
            searchSpinner_->hide();
            layout_->removeWidget(searchSpinner_);
            searchSpinner_->deleteLater();
            searchSpinner_ = nullptr;
            viewContainer_->show();
            searchResultsRcvdFirst_ = false;
        }
    }

    void ContactListWidget::switchToInitial(bool _initial)
    {
        if (initial_ == _initial)
            return;

        const auto connectSearchDelegate = [this](Logic::AbstractItemDelegateWithRegim* _deleg)
        {
            if (auto srchDelegate = qobject_cast<Logic::SearchItemDelegate*>(_deleg))
            {
                connect(this, &ContactListWidget::itemSelected, srchDelegate, &Logic::SearchItemDelegate::onContactSelected);
                connect(this, &ContactListWidget::itemSelected, this, [this]() { view_->update(); });
                connect(this, &ContactListWidget::clearSearchSelection, srchDelegate, &Logic::SearchItemDelegate::clearSelection);
            }
        };

        initial_ = _initial;
        if (initial_)
        {
            if (clDelegate_ == nullptr)
            {
                auto deleg = delegateFromRegim(this, regim_, chatMembersModel_, needDrawIcons_);
                connectSearchDelegate(deleg);
                setClDelegate(deleg);
            }

            view_->setItemDelegate(clDelegate_);
            view_->setModel(modelForRegim(regim_, searchModel_, chatMembersModel_, clModel_, commonChatsModel_, callsModel_));

            if (globalSearchViewHeader_)
                globalSearchViewHeader_->hide();
        }
        else
        {
            if (!searchDelegate_)
            {
                if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP)
                {
                    searchDelegate_ = delegateFromRegim(this, Logic::MembersWidgetRegim::CONTACT_LIST, chatMembersModel_);
                    connectSearchDelegate(searchDelegate_);
                }
                else
                {
                    searchDelegate_ = clDelegate_;
                }
            }

            Q_EMIT clearSearchSelection();

            view_->setItemDelegate(searchDelegate_);
            view_->setModel(searchModel_);

            setSearchInDialog(searchDialogContact_);

            hidePlaceholder();
        }
    }

    void ContactListWidget::searchUpOrDownPressed(bool _isUpPressed)
    {
        auto model = qobject_cast<Logic::CustomAbstractListModel*>(view_->model());
        if (!model)
            return;

        const auto inc = _isUpPressed ? -1 : 1;

        const auto prevIndex = view_->selectionModel()->currentIndex();
        QModelIndex i = model->index(prevIndex.row() + inc);

        while (!model->isClickableItem(i))
        {
            i = model->index(i.row() + inc);
            if (!i.isValid())
                return;
        }

        {
            QSignalBlocker sb(view_->selectionModel());
            view_->selectionModel()->setCurrentIndex(i, QItemSelectionModel::ClearAndSelect);
        }

        setKeyboardFocused(true);

        view_->update(prevIndex);
        view_->update(i);
        view_->scrollTo(i);

        if (auto listWithScrollBar = dynamic_cast<ListViewWithTrScrollBar*>(view_))
            if (auto scrollBar = listWithScrollBar->getScrollBarV())
                scrollBar->fadeIn();
    }

    void ContactListWidget::setKeyboardFocused(bool _isFocused)
    {
        if (auto deleg = qobject_cast<Logic::AbstractItemDelegateWithRegim*>(view_->itemDelegate()))
        {
            if (deleg->isKeyboardFocused() != _isFocused)
            {
                deleg->setKeyboardFocus(_isFocused);

                if (const auto idx = view_->selectionModel()->currentIndex(); idx.isValid())
                    view_->update(idx);
            }
        }
    }

    void ContactListWidget::initSearchModel(Logic::AbstractSearchModel* _searchModel)
    {
        if (!_searchModel)
        {
            im_assert(!searchModel_);
            searchModel_ = searchModelForRegim(this, regim_, chatMembersModel_);
        }
        else
        {
            searchModel_ = _searchModel;
        }

        searchModel_->setSearchPattern(QString());

        connect(searchModel_, &Logic::AbstractSearchModel::results, this, &ContactListWidget::onSearchResults);
        connect(searchModel_, &Logic::SearchModel::showNoSearchResults, this, &ContactListWidget::showNoSearchResults);
        connect(searchModel_, &Logic::SearchModel::hideNoSearchResults, this, &ContactListWidget::hideNoSearchResults);
        connect(searchModel_, &Logic::SearchModel::showSearchSpinner, this, &ContactListWidget::showSearchSpinner);
        connect(searchModel_, &Logic::SearchModel::hideSearchSpinner, this, &ContactListWidget::hideSearchSpinner);

        connect(searchModel_, &Logic::AbstractSearchModel::results, this, &ContactListWidget::sendGlobalSearchResultStats);
        connect(searchModel_, &Logic::AbstractSearchModel::showNoSearchResults, this, &ContactListWidget::sendGlobalSearchResultStats);

        if (auto srchModel = qobject_cast<Logic::SearchModel*>(searchModel_))
        {
            connect(this, &ContactListWidget::searchEnd, srchModel, &Logic::SearchModel::endLocalSearch);

            connect(srchModel, &Logic::SearchModel::suggests, this, &ContactListWidget::onSearchSuggests);
        }
    }

    bool ContactListWidget::isSelectMembersRegim() const
    {
        return regim_ == Logic::MembersWidgetRegim::SELECT_MEMBERS
            || regim_ == Logic::MembersWidgetRegim::MEMBERS_LIST
            || regim_ == Logic::MembersWidgetRegim::VIDEO_CONFERENCE
            || regim_ == Logic::MembersWidgetRegim::SHARE
            || regim_ == Logic::MembersWidgetRegim::SHARE_CONTACT
            || regim_ == Logic::MembersWidgetRegim::TASK_ASSIGNEE
            || regim_ == Logic::MembersWidgetRegim::SHARE_VIDEO_CONFERENCE
            || regim_ == Logic::MembersWidgetRegim::SELECT_CHAT_MEMBERS;
    }

    void Ui::ContactListWidget::ensureScrollToItemAnimationInitialized()
    {
        if (scrollToItemAnim_)
            return;

        scrollToItemAnim_ = new QVariantAnimation(this);
        scrollToItemAnim_->setDuration(std::chrono::milliseconds(300).count());
        connect(scrollToItemAnim_, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
        {
            view_->verticalScrollBar()->setValue(value.toDouble());
        });
    }

    void ContactListWidget::selectCurrentSearchCategory()
    {
        if (!globalSearchViewHeader_ || !globalSearchViewHeader_->isVisible())
            return;

        auto srchModel = qobject_cast<Logic::SearchModel*>(searchModel_);
        if (!srchModel || srchModel->isSearchInThreads(true))
            return;

        if (auto index = view_->indexAt(QPoint(0, 0)); index.isValid())
        {
            auto headers = getCurrentCategories();
            headers.erase(std::remove_if(headers.begin(), headers.end(), [](const auto& _p) { return !_p.second.isValid(); }), headers.end());
            std::sort(headers.begin(), headers.end(), [](const auto& _p1, const auto& _p2) { return _p1.second.row() < _p2.second.row(); });

            const auto it = std::find_if(headers.rbegin(), headers.rend(), [&index](const auto& _p) { return index.row() >= _p.second.row(); });
            if (it != headers.rend())
                globalSearchViewHeader_->selectCategory(it->first);
        }
    }

    void ContactListWidget::sendGlobalSearchResultStats()
    {
        const auto inGlobalSearch = (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST || regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP) && !getSearchInDialog();
        if (!inGlobalSearch || searchResultsStatsSent_)
            return;

        const auto model = qobject_cast<const Logic::SearchModel*>(view_->model());
        if (!model || !model->isAllDataReceived() || model->getSearchPattern().isEmpty())
            return;

        int localContacts = 0;
        int serverContacts = 0;
        int localChats = 0;
        int serverChats = 0;
        const auto scopedExit = Utils::ScopeExitT([this, &localContacts, &serverContacts, &localChats, &serverChats]()
        {
            searchResultsStatsSent_ = true;

            core::stats::event_props_type props;
            props.push_back({ "public_chats", std::to_string(serverChats) });
            props.push_back({ "contacts", std::to_string(localContacts + serverContacts) });
            props.push_back({ "own_chats", std::to_string(localChats) });
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::globalsearch_result, props);
        });

        if (model->rowCount())
        {
            QModelIndex i = model->index(0);
            do
            {
                if (const auto item = i.data().value<Data::AbstractSearchResultSptr>())
                {
                    if (item->isChat())
                    {
                        if (item->isLocalResult_)
                            ++localChats;
                        else
                            ++serverChats;
                    }
                    else if (item->isContact())
                    {
                        if (item->isLocalResult_)
                            ++localContacts;
                        else
                            ++serverContacts;
                    }

                    if (item->isMessage())
                        break;
                }

                i = model->index(i.row() + 1);
            } while (i.isValid());
        }
    }

    ContactListWidget::SearchHeaders ContactListWidget::getCurrentCategories() const
    {
        ContactListWidget::SearchHeaders headers;

        if (const auto searchModel = qobject_cast<const Logic::SearchModel*>(searchModel_))
        {
            headers =
            {
                { SearchCategory::ContactsAndGroups, searchModel->contactIndex(Logic::SearchModel::getContactsAndGroupsAimId()) },
                { SearchCategory::Messages, searchModel->contactIndex(Logic::SearchModel::getMessagesAimId()) },
                { SearchCategory::Threads, searchModel->contactIndex(Logic::SearchModel::getThreadsAimId()) },
                { SearchCategory::SingleThread, searchModel->contactIndex(Logic::SearchModel::getSingleThreadAimId()) },
            };
        }

        return headers;
    }

    void ContactListWidget::hideTooltip()
    {
        tooltipIndex_ = {};
        personTooltip_->hide();
    }

    QRect ContactListWidget::getAvatarRect(const QModelIndex& _index) const
    {
        auto clDelegate = qobject_cast<Logic::ContactListItemDelegate*>(clDelegate_);
        auto isMessageItem = false;

        if (auto item = _index.data().value<Data::AbstractSearchResultSptr>())
            isMessageItem = item->isMessage();

        const auto itemRect = view_->visualRect(_index);
        const auto& params = isMessageItem ? GetRecentsParams() : GetContactListParams();
        const auto isPictureOnly = clDelegate ? clDelegate->isPictOnlyView() : false;
        const auto avatarX = isPictureOnly ? (itemRect.width() - params.getAvatarSize()) / 2 : params.getAvatarX();

        return QRect(itemRect.topLeft() + QPoint(avatarX, params.getAvatarY()), QSize(params.getAvatarSize(), params.getAvatarSize()));
    }

    void ContactListWidget::setPageOpenedAs(PageOpenedAs _openedAs)
    {
        openedAs_ = _openedAs;
    }

    void ContactListWidget::setCloseOnThreadResult(bool _val)
    {
        closeOnThreadResult_ = _val;
    }

    void ContactListWidget::scrolled(const int _value)
    {
        constexpr Logic::MembersWidgetRegim regims[] =
        {
            Logic::MembersWidgetRegim::CONTACT_LIST,
            Logic::MembersWidgetRegim::CONTACT_LIST_POPUP,
            Logic::MembersWidgetRegim::MEMBERS_LIST,
            Logic::MembersWidgetRegim::SELECT_CHAT_MEMBERS,
            Logic::MembersWidgetRegim::DISALLOWED_INVITERS,
        };
        if (std::none_of(std::begin(regims), std::end(regims), [regim = regim_](auto r){ return regim == r; }))
            return;

        selectCurrentSearchCategory();

        if (_value < view_->verticalScrollBar()->maximum() * searchPreloadDistance)
            return;

        if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            if (auto searchModel = qobject_cast<Logic::SearchModel*>(searchModel_))
                searchModel->requestMore();
        }
        else if (regim_ == Logic::MembersWidgetRegim::MEMBERS_LIST || regim_ == Logic::MembersWidgetRegim::SELECT_CHAT_MEMBERS)
        {
            if (auto chatMembersModel = qobject_cast<Logic::ChatMembersModel*>(chatMembersModel_))
            {
                if (chatMembersModel->isFullMembersListLoaded())
                    return;

                chatMembersModel->requestNextPage();
            }
        }
        else if (regim_ == Logic::MembersWidgetRegim::DISALLOWED_INVITERS)
        {
            if (auto model = qobject_cast<Logic::DisallowedInvitersModel*>(searchModel_))
                model->requestMore();
        }
    }

    void ContactListWidget::onThemeChanged()
    {
        if (noResultsPlaceholder_)
            noResultsPlaceholder_->setPixmap(noResultsPixmap());
    }

    void ContactListWidget::leaveEvent(QEvent*)
    {
        hideTooltip();
    }
}
