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

#include "ContactListItemDelegate.h"
#include "ContactListWithHeaders.h"
#include "SearchItemDelegate.h"
#include "CountryListItemDelegate.h"

#include "ContactListUtils.h"
#include "SearchWidget.h"

#include "../../app_config.h"
#include "../../controls/LabelEx.h"
#include "../../controls/ContextMenu.h"
#include "../../utils/InterConnector.h"
#include "../../utils/SearchPatternHistory.h"
#include "../../utils/features.h"
#include "../Placeholders.h"
#include "../friendly/FriendlyContainer.h"

#include "styles/ThemeParameters.h"

namespace
{
    constexpr auto searchPreloadDistance = 0.75;
    constexpr auto scrollTimeout = std::chrono::milliseconds(300);

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

    Logic::AbstractItemDelegateWithRegim* delegateFromRegim(QObject* parent, Logic::MembersWidgetRegim _regim, Logic::CustomAbstractListModel* _membersModel)
    {
        if (_regim == Logic::MembersWidgetRegim::COUNTRY_LIST)
            return new Logic::CountryListItemDelegate(parent);

        if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST)
            return new Logic::SearchItemDelegate(parent);

        return new Logic::ContactListItemDelegate(parent, _regim, _membersModel);
    }

    QAbstractItemModel* modelForRegim(Logic::MembersWidgetRegim _regim, Logic::AbstractSearchModel* _searchModel, Logic::CustomAbstractListModel* _membersModel,
                                      Logic::ContactListWithHeaders* _clModel, Logic::CommonChatsModel* _commonChatsModel)
    {
        if (Logic::is_members_regim(_regim))
            return _membersModel;

        if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP)
            return _clModel;

        if (_regim == Logic::MembersWidgetRegim::COMMON_CHATS)
            return _commonChatsModel;

        if (_regim == Logic::MembersWidgetRegim::COUNTRY_LIST)
            return Logic::getCountryModel();

        static const auto needSearhModel =
        {
            Logic::MembersWidgetRegim::SELECT_MEMBERS,
            Logic::MembersWidgetRegim::SHARE,
            Logic::MembersWidgetRegim::SHARE_CONTACT,
            Logic::MembersWidgetRegim::VIDEO_CONFERENCE,
            Logic::MembersWidgetRegim::CONTACT_LIST,
            Logic::MembersWidgetRegim::UNKNOWN,
        };
        if (std::any_of(needSearhModel.begin(), needSearhModel.end(), [_regim](const auto _r) { return _r == _regim;}))
            return _searchModel;

        return Logic::getContactListModel();
    }

    bool isServerSearchSupported(Logic::MembersWidgetRegim _regim)
    {
        return Ui::GetAppConfig().IsServerSearchEnabled() &&
            (_regim == Logic::MembersWidgetRegim::CONTACT_LIST || _regim == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP);
    }

    Logic::AbstractSearchModel* searchModelForRegim(QObject* parent, Logic::MembersWidgetRegim _regim, Logic::CustomAbstractListModel* _membersModel)
    {
        if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            auto m = new Logic::SearchModel(parent);
            m->setServerSearchEnabled(isServerSearchSupported(_regim));
            return m;
        }

        if (_regim == Logic::MembersWidgetRegim::COUNTRY_LIST)
            return Logic::getCountryModel();

        if (_regim == Logic::MembersWidgetRegim::IGNORE_LIST)
            return Logic::getIgnoreModel();

        if (!Logic::is_members_regim(_regim))
        {
            const auto contactsOnly =
                    _regim == Logic::MembersWidgetRegim ::SHARE_CONTACT ||
                    (_regim != Logic::MembersWidgetRegim::SHARE &&
                    _regim != Logic::MembersWidgetRegim::CONTACT_LIST_POPUP);

            auto m = new Logic::SearchModel(parent);
            m->setSearchInDialogs(false);
            m->setExcludeChats(contactsOnly);
            m->setHideReadonly(_regim == Logic::MembersWidgetRegim::SHARE ||  _regim == Logic::MembersWidgetRegim::SHARE_CONTACT);
            m->setServerSearchEnabled(isServerSearchSupported(_regim));
            m->setCategoriesEnabled(_regim == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP);
            m->setSortByTime(_regim == Logic::MembersWidgetRegim::SHARE ||  _regim == Logic::MembersWidgetRegim::SHARE_CONTACT);
            return m;
        }

        return qobject_cast<Logic::ChatMembersModel*>(_membersModel);
    }
}

namespace Ui
{
    ContactListWidget::ContactListWidget(QWidget* _parent, const Logic::MembersWidgetRegim& _regim, Logic::CustomAbstractListModel* _chatMembersModel,
                                         Logic::AbstractSearchModel* _searchModel, Logic::CommonChatsModel* _commonChatsModel)
        : QWidget(_parent)
        , emptyIgnoreListLabel_(nullptr)
        , dialogSearchViewHeader_(nullptr)
        , globalSearchViewHeader_(nullptr)
        , scrollStatsTimer_(new QTimer(this))
        , clDelegate_(nullptr)
        , searchDelegate_(nullptr)
        , contactsPlaceholder_(nullptr)
        , noSearchResults_(nullptr)
        , searchSpinner_(nullptr)
        , viewContainer_(new QWidget(this))
        , regim_(_regim)
        , chatMembersModel_(_chatMembersModel)
        , searchModel_(_searchModel)
        , commonChatsModel_(_commonChatsModel)
        , popupMenu_(nullptr)
        , noSearchResultsShown_(false)
        , searchResultsRcvdFirst_(false)
        , searchResultsStatsSent_(false)
        , initial_(false)
        , tapAndHold_(false)
    {
        initSearchModel(_searchModel);
        clModel_ = new Logic::ContactListWithHeaders(this, Logic::getContactListModel(), false);

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

        view_->setAttribute(Qt::WA_MacShowFocusRect, false);
        view_->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
        view_->viewport()->grabGesture(Qt::TapAndHoldGesture);
        Utils::grabTouchWidget(view_->viewport(), true);

        Testing::setAccessibleName(view_, qsl("AS search view_"));
        viewLayout_->addWidget(view_);
        Testing::setAccessibleName(viewContainer_, qsl("AS search viewContainer_"));
        layout_->addWidget(viewContainer_);

        emptyIgnoreListLabel_ = new EmptyIgnoreListLabel(this);
        emptyIgnoreListLabel_->setContentsMargins(0, 0, 0, 0);
        emptyIgnoreListLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        emptyIgnoreListLabel_->setFixedHeight(::Ui::GetContactListParams().itemHeight());
        Testing::setAccessibleName(emptyIgnoreListLabel_, qsl("AS emptyIgnoreListLabel_"));
        layout_->addWidget(emptyIgnoreListLabel_);
        emptyIgnoreListLabel_->setVisible(false);

        switchToInitial(true);

        scrollStatsTimer_->setSingleShot(true);
        scrollStatsTimer_->setInterval(scrollTimeout.count());
        connect(scrollStatsTimer_, &QTimer::timeout, this, &ContactListWidget::onMouseWheeledStats);

        connect(view_, &Ui::FocusableListView::activated, this, &ContactListWidget::searchClicked, Qt::QueuedConnection);
        connect(view_, &Ui::FocusableListView::clicked, this, &ContactListWidget::onItemClicked);
        connect(view_, &Ui::FocusableListView::pressed, this, &ContactListWidget::onItemPressed);
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
                emit forceSearchClear(!getSearchInDialog() && _msgid == -1);
            });
        }
    }

    ContactListWidget::~ContactListWidget()
    {

    }

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
        connect(_widget, &SearchWidget::escapePressed, this, []() {
            Logic::updatePlaceholders({ Logic::Placeholder::Contacts });
        });
    }

    void ContactListWidget::installEventFilterToView(QObject* _filter)
    {
        view_->viewport()->installEventFilter(_filter);
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
        clDelegate_->setDragIndex(_index);
    }

    void ContactListWidget::setPictureOnlyForDelegate(bool _value)
    {
        if (auto d = qobject_cast<Logic::ContactListItemDelegate*>(clDelegate_))
            d->setPictOnlyView(_value);

        if (contactsPlaceholder_)
            contactsPlaceholder_->setPictureOnlyView(_value);
    }

    void ContactListWidget::setEmptyIgnoreLabelVisible(bool _isVisible)
    {
        emptyIgnoreListLabel_->setVisible(_isVisible);
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

        if (getSearchInDialog())
        {
            if (!dialogSearchViewHeader_)
            {
                dialogSearchViewHeader_ = new DialogSearchViewHeader(this);
                Testing::setAccessibleName(dialogSearchViewHeader_, qsl("AS dialogSearchViewHeader_"));
                layout_->insertWidget(0, dialogSearchViewHeader_);
                dialogSearchViewHeader_->hide();

                connect(dialogSearchViewHeader_, &DialogSearchViewHeader::contactFilterRemoved, this, &ContactListWidget::onDisableSearchInDialog);
            }

            const auto name = Logic::GetFriendlyContainer()->getFriendly(_contact).toUpper();
            dialogSearchViewHeader_->setChatNameFilter(name);
        }

        if (globalSearchViewHeader_)
            globalSearchViewHeader_->hide();

        if (dialogSearchViewHeader_)
            dialogSearchViewHeader_->setVisible(getSearchInDialog());

        if (_switchModel)
            view_->setModel(modelForRegim(regim_, searchModel_, chatMembersModel_, clModel_, commonChatsModel_));
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

    Logic::AbstractSearchModel* ContactListWidget::getSearchModel() const
    {
        return searchModel_;
    }

    FocusableListView* ContactListWidget::getView() const
    {
        return view_;
    }

    void ContactListWidget::triggerTapAndHold(bool _value)
    {
        tapAndHold_ = _value;
    }

    bool ContactListWidget::tapAndHoldModifier() const
    {
        return tapAndHold_;
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
        const auto item = i.data().value<Data::AbstractSearchResultSptr>();
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

        select(aimid, msgId, highlights, Logic::UpdateChatSelection::No);

        if (msgId == -1)
            emit itemClicked(aimid);

        const auto index = view_->selectionModel()->currentIndex();
        if (!isSelectMembersRegim() && index.isValid())
            view_->selectionModel()->clear();

        view_->update(index);
    }

    void ContactListWidget::select(const QString& _aimId)
    {
        select(_aimId, -1, {}, Logic::UpdateChatSelection::No);
    }

    void ContactListWidget::select(const QString& _aimId, const qint64 _message_id, const highlightsV& _highlights, Logic::UpdateChatSelection _mode)
    {
        const auto isSelectMembers = isSelectMembersRegim();

        if (!isSelectMembers && regim_ != Logic::MembersWidgetRegim::IGNORE_LIST && regim_ != Logic::MembersWidgetRegim::COUNTRY_LIST)
            Logic::getContactListModel()->setCurrent(_aimId, _message_id, regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP);

        if (_message_id == -1 || _mode == Logic::UpdateChatSelection::Yes)
            emit changeSelected(_aimId);

        emit itemSelected(_aimId, _message_id, _highlights);

        if (regim_ != Logic::MembersWidgetRegim::UNKNOWN && !isSelectMembers && _message_id == -1)
            emit searchEnd();
    }

    void ContactListWidget::onItemClicked(const QModelIndex& _current)
    {
        const auto srchModel = qobject_cast<const Logic::SearchModel*>(_current.model());
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
                            emit searchSuggestRemoved(srchModel->getSingleDialogAimId(), searchRes->getFriendlyName());
                        }
                        else
                        {
                            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_search_dialog_suggest_click);
                            emit searchSuggestSelected(searchRes->getFriendlyName());
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

                        emit Utils::InterConnector::instance().clearDialogHistory();
                        selectionChanged(_current);
                    }
                    else if (searchRes->isChat() && (Features::forceShowChatPopup() || resChat->chatInfo_.ApprovedJoin_))
                    {
                        emit Utils::InterConnector::instance().liveChatSelected();
                        emit Utils::InterConnector::instance().showLiveChat(std::make_shared<Data::ChatInfo>(resChat->chatInfo_));
                        statResType = "public_chats";
                    }
                    else
                    {
                        const auto aimid = searchRes->getAimId();
                        Logic::getContactListModel()->setCurrent(aimid, -1, false);
                        emit itemSelected(aimid, -1, {});
                        statResType = "contacts";
                    }

                    if (!statResType.empty())
                    {
                        if ((regim_ == Logic::MembersWidgetRegim::CONTACT_LIST || regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP) && !getSearchInDialog())
                            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::globalsearch_selected, {{ "type", statResType }});
                    }
                }
            }
        }
        else if (changeSelection)
        {
            if (regim_ != Logic::MembersWidgetRegim::COMMON_CHATS)
                 emit Utils::InterConnector::instance().clearDialogHistory();

            selectionChanged(_current);
        }
    }

    void ContactListWidget::onItemPressed(const QModelIndex& _current)
    {
        if ((regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP || regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
                && (QApplication::mouseButtons() & Qt::RightButton || tapAndHoldModifier()))
        {
            const auto model = qobject_cast<const Logic::CustomAbstractListModel*>(_current.model());
            if (model && model->isServiceItem(_current))
                return;

            triggerTapAndHold(false);

            const auto srchModel = qobject_cast<const Logic::SearchModel*>(_current.model());
            if (srchModel)
            {
                const auto searchRes = _current.data().value<Data::AbstractSearchResultSptr>();
                if (searchRes && searchRes->isContact())
                {
                    const auto cont = Logic::getContactListModel()->getContactItem(searchRes->getAimId());
                    if (cont)
                        showContactsPopupMenu(cont->get_aimid(), cont->is_chat());
                }
            }
            else
            {
                const auto cont = _current.data(Qt::DisplayRole).value<Data::Contact*>();
                if (cont)
                    showContactsPopupMenu(cont->AimId_, cont->Is_chat_);
            }
        }

        if (_current.isValid())
        {
            if (const auto aimId = Logic::aimIdFromIndex(_current); !aimId.isEmpty())
            {
                if (Utils::isChat(aimId))
                {
                    emit selected(aimId);
                }
                else
                {
                    const auto removeX = width() - Utils::scale_value(32);
                    const auto approveX = width() - Utils::scale_value(64);

                    if (auto dlgt = qobject_cast<Logic::ContactListItemDelegate*>(clDelegate_))
                    {
                        const auto cursorPos = view_->mapFromGlobal(QCursor::pos());
                        const auto regim = dlgt->regim();
                        if (cursorPos.x() > removeX && cursorPos.x() <= width())
                        {
                            if (regim == Logic::MembersWidgetRegim::ADMIN_MEMBERS)
                                emit moreClicked(aimId);
                            else if (regim == Logic::MembersWidgetRegim::MEMBERS_LIST)
                                emit removeClicked(aimId);
                            else if (regim == Logic::MembersWidgetRegim::PENDING_MEMBERS)
                                emit approve(aimId, false);
                            else
                                emit selected(aimId);
                        }
                        else if (regim == Logic::MembersWidgetRegim::PENDING_MEMBERS && cursorPos.x() >= approveX && cursorPos.x() <= removeX)
                        {
                            emit approve(aimId, true);
                        }
                        else
                        {
                            emit selected(aimId);
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

        const auto srchModel = qobject_cast<const Logic::SearchModel*>(_index.model());
        auto srchDelegate = qobject_cast<Logic::SearchItemDelegate*>(clDelegate_);

        if (srchModel && srchDelegate)
        {
            if (const auto item = _index.data().value<Data::AbstractSearchResultSptr>())
            {
                bool updated = false;
                if (item->isSuggest())
                {
                    const auto rect = view_->visualRect(_index);
                    if (getDeleteSuggestBtnRect(rect).contains(_pos))
                        updated = srchDelegate->setHoveredSuggetRemove(_index);
                    else
                        updated = srchDelegate->setHoveredSuggetRemove(QModelIndex());
                }
                else
                    updated = srchDelegate->setHoveredSuggetRemove(QModelIndex());

                if (updated)
                    view_->update();
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

        if ((regim_ == Logic::MembersWidgetRegim::CONTACT_LIST || regim_ == Logic::MembersWidgetRegim::CONTACT_LIST_POPUP) && !getSearchInDialog())
        {
            if (const auto searchModel = qobject_cast<const Logic::SearchModel*>(model))
            {
                const auto headers = getCurrentCategories();
                if (std::any_of(headers.begin(), headers.end(), [](const auto& _p) { return _p.second.isValid(); }))
                {
                    if (!globalSearchViewHeader_)
                    {
                        globalSearchViewHeader_ = new GlobalSearchViewHeader(this);
                        Testing::setAccessibleName(globalSearchViewHeader_, qsl("AS globalSearchViewHeader_"));
                        layout_->insertWidget(0, globalSearchViewHeader_);
                        globalSearchViewHeader_->hide();

                        if (regim_ != Logic::MembersWidgetRegim::CONTACT_LIST)
                            globalSearchViewHeader_->setCategoryVisible(SearchCategory::Messages, false);

                        connect(globalSearchViewHeader_, &GlobalSearchViewHeader::categorySelected, this, &ContactListWidget::scrollToCategory);
                    }

                    if (searchModel->isAllDataReceived())
                    {
                        for (const auto&[cat, ndx] : headers)
                            globalSearchViewHeader_->setCategoryVisible(cat, ndx.isValid());
                    }

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
            emit searchEnd();
            return;
        }

        if (qobject_cast<const Logic::SearchModel*>(_current.model()))
        {
            selectionChanged(_current);
            emit searchEnd();

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
                emit itemSelected(aimid, msgId, highlights);
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

        emit searchEnd();
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
        if (Logic::is_members_regim(regim_) || Logic::is_select_members_regim(regim_))
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

        if (!_is_chat)
        {
#ifndef STRIP_VOIP
            popupMenu_->addActionWithIcon(qsl(":/context_menu/call"), QT_TRANSLATE_NOOP("context_menu", "Call"), Logic::makeData(qsl("contacts/call"), aimId));
#endif //STRIP_VOIP
            popupMenu_->addActionWithIcon(qsl(":/context_menu/profile"), QT_TRANSLATE_NOOP("context_menu", "Profile"), Logic::makeData(qsl("contacts/Profile"), aimId));

            popupMenu_->addSeparator();
        }

        popupMenu_->addActionWithIcon(qsl(":/context_menu/ignore"), QT_TRANSLATE_NOOP("context_menu", "Block"), Logic::makeData(qsl("contacts/ignore"), aimId));

        if (_is_chat || Features::clRemoveContactsAllowed())
        {

            if (_is_chat)
            {
                popupMenu_->addActionWithIcon(qsl(":/exit_icon"), QT_TRANSLATE_NOOP("context_menu", "Leave and delete"), Logic::makeData(qsl("contacts/remove"), aimId));
            }
            else
            {
                popupMenu_->addActionWithIcon(qsl(":/alert_icon"), QT_TRANSLATE_NOOP("context_menu", "Report"), Logic::makeData(qsl("contacts/spam"), aimId));
                popupMenu_->addActionWithIcon(qsl(":/context_menu/delete"), QT_TRANSLATE_NOOP("context_menu", "Remove"), Logic::makeData(qsl("recents/remove"), aimId));
            }
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
            && !_newPattern.trimmed().isEmpty()
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
        emit Utils::InterConnector::instance().disableSearchInDialog();

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

    void ContactListWidget::scrollToCategory(const SearchCategory _category)
    {
        switch (_category)
        {
        case SearchCategory::ContactsAndGroups:
            scrollToItem(Logic::SearchModel::getContactsAndGroupsAimId());
            break;
        case SearchCategory::Messages:
            scrollToItem(Logic::SearchModel::getMessagesAimId());
            break;

        default:
            break;
        }
    }

    void ContactListWidget::scrollToItem(const QString& _aimId)
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
            constexpr auto duration = std::chrono::milliseconds(300);

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

            scrollToItemAnim_.finish();
            scrollToItemAnim_.start([scrollbar, this]()
            {
                scrollbar->setValue(scrollToItemAnim_.current());
            }, curValue, endValue, duration.count());
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
        if (isSearchMode() || regim_ == Logic::MembersWidgetRegim::IGNORE_LIST || regim_ == Logic::MembersWidgetRegim::COUNTRY_LIST)
            return;

        if (!contactsPlaceholder_)
        {
            contactsPlaceholder_ = new ContactsPlaceholder(this);
            Testing::setAccessibleName(contactsPlaceholder_, qsl("AS search contactsPlaceholder_"));
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
                        const auto textColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY);
                        const auto imgColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);

                        auto noSearchResultsWidget = new QWidget(noSearchResults_);
                        auto noSearchLayout = new QVBoxLayout(noSearchResultsWidget);
                        noSearchResultsWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noSearchLayout->setAlignment(Qt::AlignCenter);
                        noSearchLayout->setContentsMargins(0, Utils::scale_value(60), 0, 0);
                        noSearchLayout->setSpacing(Utils::scale_value(36));
                        {
                            auto noResultsPlaceholder = new QLabel(noSearchResultsWidget);

                            const auto phSize = Utils::scale_value(QSize(84, 84));
                            noResultsPlaceholder->setPixmap(Utils::renderSvg(qsl(":/placeholders/empty_search"), phSize, imgColor));
                            noResultsPlaceholder->setFixedSize(phSize);
                            Testing::setAccessibleName(noResultsPlaceholder, qsl("AS noSearchResultsPlaceholder"));

                            auto hLayout = Utils::emptyHLayout();
                            hLayout->addWidget(noResultsPlaceholder);
                            noSearchLayout->addLayout(hLayout);
                        }
                        {
                            auto noSearchResultsLabel = new LabelEx(noSearchResultsWidget);
                            noSearchResultsLabel->setColor(textColor);
                            noSearchResultsLabel->setFont(Fonts::appFontScaled(14, Fonts::FontWeight::SemiBold));
                            noSearchResultsLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                            noSearchResultsLabel->setAlignment(Qt::AlignCenter);
                            noSearchResultsLabel->setText(QT_TRANSLATE_NOOP("placeholders", "Nothing found"));
                            Testing::setAccessibleName(noSearchResultsLabel, qsl("AS noSearchResultsLabel"));
                            noSearchLayout->addWidget(noSearchResultsLabel);
                        }

                        Testing::setAccessibleName(noSearchResultsWidget, qsl("AS noSearchResultsWidget"));
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
        if (!isVisible())
            return;

        viewContainer_->hide();

        if (!searchSpinner_)
        {
            searchSpinner_ = new QWidget(this);
            searchSpinner_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            {
                auto spinner = new RotatingSpinner(searchSpinner_);
                spinner->startAnimation();
                Testing::setAccessibleName(spinner, qsl("AS searchSpinnerWidget"));

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
                connect(this, &ContactListWidget::itemSelected, this, [this]() {view_->update(); });
                connect(this, &ContactListWidget::clearSearchSelection, srchDelegate, &Logic::SearchItemDelegate::clearSelection);
            }
        };

        initial_ = _initial;
        if (initial_)
        {
            if (clDelegate_ == nullptr)
            {
                auto deleg = delegateFromRegim(this, regim_, chatMembersModel_);
                connectSearchDelegate(deleg);
                setClDelegate(deleg);
            }

            view_->setItemDelegate(clDelegate_);
            view_->setModel(modelForRegim(regim_, searchModel_, chatMembersModel_, clModel_, commonChatsModel_));

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

            emit clearSearchSelection();

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
            assert(!searchModel_);
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
        return  regim_ == Logic::MembersWidgetRegim::SELECT_MEMBERS ||
            regim_ == Logic::MembersWidgetRegim::MEMBERS_LIST ||
            regim_ == Logic::MembersWidgetRegim::VIDEO_CONFERENCE ||
            regim_ == Logic::MembersWidgetRegim::SHARE ||
            regim_ == Logic::MembersWidgetRegim::SHARE_CONTACT;
    }

    void ContactListWidget::selectCurrentSearchCategory()
    {
        if (!globalSearchViewHeader_ || !globalSearchViewHeader_->isVisible())
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
            };
        }

        return headers;
    }

    void ContactListWidget::scrolled(const int _value)
    {
        if (regim_ != Logic::MembersWidgetRegim::CONTACT_LIST && regim_ != Logic::MembersWidgetRegim::CONTACT_LIST_POPUP
            && regim_ != Logic::MembersWidgetRegim::MEMBERS_LIST)
            return;

        selectCurrentSearchCategory();

        if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            if (auto searchModel = qobject_cast<Logic::SearchModel*>(searchModel_))
            {
                if (_value < view_->verticalScrollBar()->maximum() * searchPreloadDistance)
                    return;

                searchModel->requestMore();
            }
        }
        else if (regim_ == Logic::MembersWidgetRegim::MEMBERS_LIST)
        {
            if (auto chatMembersModel = qobject_cast<Logic::ChatMembersModel*>(chatMembersModel_))
            {
                if (chatMembersModel->isFullMembersListLoaded() || _value < view_->verticalScrollBar()->maximum() * searchPreloadDistance)
                    return;

                chatMembersModel->requestNextPage();
            }
        }
    }
}
