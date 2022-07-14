#include "stdafx.h"

#include "ThreadPage.h"
#include "main_window/history_control/HistoryControlPage.h"
#include "main_window/history_control/top_widget/ThreadHeaderPanel.h"
#include "main_window/contact_list/RecentsModel.h"
#include "main_window/contact_list/RecentsTab.h"
#include "main_window/contact_list/ContactListModel.h"
#include "main_window/contact_list/TopPanel.h"
#include "main_window/contact_list/SearchWidget.h"
#include "utils/InterConnector.h"
#include "utils/utils.h"
#include "controls/BackgroundWidget.h"
#include "types/message.h"
#include "core_dispatcher.h"
#include "app_config.h"

namespace
{
    int getVerSpacing() noexcept
    {
        return Utils::scale_value(8);
    }
}

namespace Ui
{
    ThreadPage::ThreadPage(QWidget* _parent)
        : SidebarPage(_parent)
        , stackedWidget_(new QStackedWidget(this))
        , bg_(new BackgroundWidget(this, false))
    {
        auto layout = Utils::emptyVLayout(this);
        layout->addWidget(stackedWidget_);
        stackedWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        bg_->setLayout(Utils::emptyVLayout());

        connect(GetDispatcher(), &core_dispatcher::dlgStates, this, &ThreadPage::dlgStates);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::scrollThreadToMsg, this, &ThreadPage::onScrollThreadToMsg);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::yourRoleChanged, this, &ThreadPage::onChatRoleChanged);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::startSearchInThread, this, &ThreadPage::onSearchStart);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::threadSearchClosed, this, &ThreadPage::onSearchEnd);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::threadCloseClicked, this, &ThreadPage::onBackClicked);
    }

    ThreadPage::~ThreadPage()
    {
        unloadPage();
    }

    void ThreadPage::initFor(const QString& _aimId, SidebarParams _params)
    {
        parentAimId_ = _params.parentPageId_;

        bg_->updateWallpaper(_aimId, true);

        stopUnloadTimer();

        const auto createNewPage = (page_ == nullptr);
        const auto dlgState = Logic::getRecentsModel()->getDlgState(_aimId);
        const qint64 lastReadMsg = dlgState.UnreadCount_ > 0 ? dlgState.YoursLastRead_ : -1;

        if (page_ && _aimId != page_->aimId())
            Q_EMIT Utils::InterConnector::instance().searchEnd();

        if (!page_)
        {
            page_ = new HistoryControlPage(_aimId, bg_, bg_);
            Testing::setAccessibleName(page_, qsl("AS Sidebar HistoryPage ") % _aimId);
            page_->setMinimumWidth(Sidebar::getDefaultWidth());

            bg_->layout()->addWidget(page_->getTopWidget());
            bg_->layout()->addWidget(page_);

            // workaround to force geometry resize (https://jira.mail.ru/browse/IMDESKTOP-16743)
            bg_->resize(size());
            bg_->grab();

            page_->setPinnedMessage(dlgState.pinnedMessage_);

            escCancel_->addChild(page_);
            stackedWidget_->insertWidget(static_cast<int>(Tab::Main), bg_);
        }

        if (stackedWidget_->currentIndex() == static_cast<int>(Tab::Search))
            changeTab(Tab::Main);

        page_->resetMessageHighlights();

        const auto scrollMode = hist::evaluateScrollMode(-1, lastReadMsg, createNewPage, false);
        if (scrollMode != hist::scroll_mode_type::none || createNewPage)
            page_->initFor(lastReadMsg, scrollMode, createNewPage ? HistoryControlPage::FirstInit::Yes : HistoryControlPage::FirstInit::No);

        page_->pageOpen();
        page_->updateItems();
        page_->open();

        isActive_ = true;
    }

    void ThreadPage::setFrameCountMode(FrameCountMode _mode)
    {
        mode_ = _mode;
        updateBackButton();
    }

    void ThreadPage::close()
    {
        if (page_)
        {
            page_->resetShiftingParams();
            page_->pageLeave();
            startUnloadTimer();
        }

        isActive_ = false;
    }

    void ThreadPage::onPageBecomeVisible()
    {
        if (page_)
            Q_EMIT Utils::InterConnector::instance().setFocusOnInput(page_->aimId());
    }

    bool ThreadPage::hasInputOrHistoryFocus() const
    {
        return page_ && page_->isInputOrHistoryInFocus();
    }

    bool ThreadPage::hasSearchFocus() const
    {
        return stackedWidget_->currentIndex() == static_cast<int>(Tab::Search) && topWidget_ && topWidget_->getSearchWidget()->hasFocus();
    }

    void ThreadPage::updateWidgetsTheme()
    {
        bg_->updateWallpaper(bg_->aimId(), true);
        if (page_)
            page_->updateWidgetsTheme();
    }

    void ThreadPage::dlgStates(const QVector<Data::DlgState>& _states)
    {
        if (page_)
        {
            for (const auto& st : _states)
            {
                if (st.AimId_ == page_->aimId())
                    page_->setPinnedMessage(st.pinnedMessage_);
            }
        }
    }

    void ThreadPage::startUnloadTimer()
    {
        if (!unloadTimer_)
        {
            unloadTimer_ = new QTimer(this);
            unloadTimer_->setSingleShot(true);
            connect(unloadTimer_, &QTimer::timeout, this, &ThreadPage::unloadPage);
        }

        unloadTimer_->start(std::max(GetAppConfig().CacheHistoryControlPagesFor(), std::chrono::seconds(1)));
    }

    void ThreadPage::stopUnloadTimer()
    {
        if (unloadTimer_)
            unloadTimer_->stop();
    }

    void ThreadPage::unloadPage()
    {
        if (page_)
        {
            escCancel_->removeChild(page_);
            page_->pageLeave();
            page_->deleteLater();
            page_ = nullptr;
        }
    }

    void ThreadPage::initSearch()
    {
        if (!searchWidget_)
        {
            searchWidget_ = new QWidget(this);

            auto searchLayout = Utils::emptyVLayout(searchWidget_);
            searchLayout->setSpacing(getVerSpacing());
            topWidget_ = new TopPanelWidget(this);
            recentsTab_ = new RecentsTab(searchWidget_);
            Testing::setAccessibleName(recentsTab_, qsl("AS ThreadSearchTab ") % page_->aimId());

            auto topSearchWidget = topWidget_->getSearchWidget();
            recentsTab_->connectSearchWidget(topSearchWidget);
            connect(topSearchWidget, &SearchWidget::escapePressed, this, &ThreadPage::onSearchEnd);
            connect(topWidget_, &TopPanelWidget::searchCancelled, this, &ThreadPage::onSearchEnd);
            disconnect(&Utils::InterConnector::instance(), &Utils::InterConnector::disableSearchInDialog, recentsTab_, nullptr);

            searchLayout->addWidget(topWidget_);
            searchLayout->addWidget(recentsTab_);
            stackedWidget_->insertWidget(static_cast<int>(Tab::Search), searchWidget_);
        }

        recentsTab_->setSearchMode(true);
        recentsTab_->setSearchInDialog(page_->aimId());
        recentsTab_->showSearch();
        recentsTab_->setCloseOnThreadResult(true);

        topWidget_->setRegime(TopPanelWidget::Regime::Search);
        topWidget_->getSearchWidget()->setFocus();
    }

    void ThreadPage::changeTab(Tab _tab)
    {
        if (_tab == Tab::Search)
            initSearch();

        stackedWidget_->setCurrentIndex(static_cast<int>(_tab));
        updateBackButton();
    }

    void ThreadPage::onSearchStart(const QString& _aimId)
    {
        if (page_ && page_->aimId() == _aimId)
            changeTab(Tab::Search);
    }

    void ThreadPage::onSearchEnd()
    {
        changeTab(Tab::Main);
        if (searchWidget_)
            topWidget_->getSearchWidget()->clearInput();
        page_->resetMessageHighlights();
        Q_EMIT Utils::InterConnector::instance().threadSearchButtonDeactivate();
    }

    void ThreadPage::updateBackButton()
    {
        if (auto header = findChild<ThreadHeaderPanel*>())
            header->updateCloseIcon(mode_);
    }

    void ThreadPage::onChatRoleChanged(const QString& _aimId)
    {
        if (isActive() && !parentAimId_.isEmpty() && _aimId == parentAimId_)
        {
            if (Logic::getContactListModel()->isReadonly(_aimId))
                Utils::InterConnector::instance().setSidebarVisible(false);
        }
    }

    void ThreadPage::onBackClicked(bool _searchActive)
    {
        if (_searchActive)
            changeTab(Tab::Search);
        else
            Utils::InterConnector::instance().setSidebarVisible(false);
    }

    void ThreadPage::onScrollThreadToMsg(const QString& _threadId, int64_t _msgId, const highlightsV& _highlights)
    {
        if (_threadId == page_->aimId())
        {
            bg_->updateWallpaper(_threadId, true);
            page_->scrollToMessage(_msgId);
            if (!_highlights.empty())
                page_->setHighlights(_highlights);
            QMetaObject::invokeMethod(this, [_threadId](){ Q_EMIT Utils::InterConnector::instance().setFocusOnInput(_threadId); }, Qt::QueuedConnection);
        }
    }
}
