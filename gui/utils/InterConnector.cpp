#include "stdafx.h"

#include "../main_window/MainWindow.h"
#include "../main_window/AppsPage.h"
#include "../main_window/MainPage.h"
#ifdef HAS_WEB_ENGINE
#include "../main_window/WebAppPage.h"
#endif
#include "../main_window/ContactDialog.h"
#include "../main_window/history_control/HistoryControlPage.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../styles/ThemesContainer.h"
#include "../gui_settings.h"
#ifdef __APPLE__
#include "../utils/macos/mac_support.h"
#endif //__APPLE__

#include "InterConnector.h"
#include "features.h"

#include "../types/authorization.h"
#include "../core_dispatcher.h"

using HistoryPagePtr = QPointer<Ui::HistoryControlPage>;

namespace Utils
{
    InterConnector& InterConnector::instance()
    {
        static InterConnector instance = InterConnector();
        return instance;
    }

    InterConnector::InterConnector()
        : MainWindow_(nullptr)
        , dragOverlay_(false)
        , destroying_(false)
        , loginComplete_(false)
        , currentElement_(MultiselectCurrentElement::Message)
        , currentMessage_(-1)
        , multiselectAnimation_(new QVariantAnimation(this))
#ifdef HAS_WEB_ENGINE
        , localProfile_(nullptr)
#endif //HAS_WEB_ENGINE
    {
        multiselectAnimation_->setStartValue(0.);
        multiselectAnimation_->setEndValue(1.);
        multiselectAnimation_->setDuration(100);
        connect(multiselectAnimation_, &QVariantAnimation::valueChanged, this, &InterConnector::multiselectAnimationUpdate);
        connect(this, &InterConnector::omicronUpdated, this, &InterConnector::configUpdated);
        connect(this, &InterConnector::loginCompleted, this, &InterConnector::configUpdated, Qt::QueuedConnection);
        connect(this, &InterConnector::logout, this, [this] () {setLoginComplete(false);}, Qt::QueuedConnection);

        configUpdated();
    }

    InterConnector::~InterConnector()
    {
        im_assert(disabledWidgets_.empty());
    }

    void InterConnector::clearInternalCaches()
    {
        multiselectStates_.clear();
        historyPages_.clear();
    }

    void InterConnector::setMainWindow(Ui::MainWindow* window)
    {
        MainWindow_ = window;
    }

    void InterConnector::startDestroying()
    {
        destroying_ = true;
    }

    Ui::MainWindow* InterConnector::getMainWindow(bool _check_destroying) const
    {
        if (_check_destroying && destroying_)
            return nullptr;

        return MainWindow_;
    }

    void InterConnector::setQmlEngine(Qml::Engine* _engine)
    {
        qmlEngine_ = _engine;
    }

    Qml::Engine* InterConnector::qmlEngine() const
    {
        return qmlEngine_;
    }

    void InterConnector::setQmlRootModel(Qml::RootModel* _model)
    {
        qmlRootModel_ = _model;
    }

    Qml::RootModel* InterConnector::qmlRootModel() const
    {
        return qmlRootModel_;
    }

    Ui::HistoryControlPage* InterConnector::getHistoryPage(const QString& _aimId) const
    {
        return qobject_cast<Ui::HistoryControlPage*>(getPage(_aimId));
    }

    Ui::PageBase* InterConnector::getPage(const QString& _aimId) const
    {
        if (!_aimId.isEmpty())
        {
            if (auto it = historyPages_.find(_aimId); it != historyPages_.end())
                return it->second;
        }
        return nullptr;
    }

    void InterConnector::addPage(Ui::PageBase* _page)
    {
        if (_page)
            historyPages_[_page->aimId()] = _page;
    }

    void InterConnector::removePage(Ui::PageBase* _page)
    {
        if (_page)
            historyPages_.erase(_page->aimId());
    }

    std::vector<HistoryPagePtr> InterConnector::getVisibleHistoryPages() const
    {
        std::vector<HistoryPagePtr> res;
        res.reserve(historyPages_.size());
        for (const auto& [_, page] : historyPages_)
        {
            auto historyPage = qobject_cast<Ui::HistoryControlPage*>(page);
            if (historyPage && historyPage->isPageOpen())
                res.push_back(historyPage);
        }

        return res;
    }

    bool InterConnector::isHistoryPageVisible(const QString& _aimId) const
    {
        if (_aimId.isEmpty())
            return false;

        const auto pages = getVisibleHistoryPages();
        return std::any_of(pages.begin(), pages.end(), [&_aimId](auto p) { return p->aimId() == _aimId; });
    }

    Ui::ContactDialog* InterConnector::getContactDialog() const
    {
        auto mainPage = getMessengerPage();
        if (!mainPage)
            return nullptr;

        return mainPage->getContactDialog();
    }

    Ui::AppsPage* InterConnector::getAppsPage() const
    {
        if (!MainWindow_)
            return nullptr;

        return MainWindow_->getAppsPage();
    }

    Ui::MainPage *InterConnector::getMessengerPage() const
    {
        if (!MainWindow_)
            return nullptr;

        return MainWindow_->getMessengerPage();
    }

#ifdef HAS_WEB_ENGINE
    Ui::WebAppPage* InterConnector::getTasksPage() const
    {
        if (!MainWindow_)
            return nullptr;

        return MainWindow_->getTasksPage();
    }
#endif

    int InterConnector::getHeaderHeight() const
    {
        if (auto w = getAppsPage())
            return w->getHeaderHeight();

        return 0;
    }

    bool InterConnector::isInBackground() const
    {
        if (MainWindow_)
            return !(MainWindow_->isActive());
        return false;
    }

    void InterConnector::showSidebar(const QString& _aimId)
    {
        auto appsPage = getAppsPage();
        if (appsPage->isMessengerPage())
        {
            if (auto page = getMessengerPage())
            {
                page->showSidebar(_aimId);
                Q_EMIT sidebarVisibilityChanged(true);
            }
        }
#ifdef HAS_WEB_ENGINE
        else if (appsPage->isTasksPage())
        {
            if (auto page = getTasksPage())
                page->showThreadInfo(_aimId);
        }
#endif
    }

    void InterConnector::showSidebarWithParams(const QString &_aimId, Ui::SidebarParams _params)
    {
        auto appsPage = getAppsPage();
        if (appsPage->isMessengerPage())
        {
            if (auto page = getMessengerPage())
            {
                page->showSidebarWithParams(_aimId, std::move(_params));
                Q_EMIT sidebarVisibilityChanged(true);
            }
        }
#ifdef HAS_WEB_ENGINE
        else if (appsPage->isTasksPage())
        {
            Q_UNUSED(_params);
            if (auto page = getTasksPage())
                page->showProfileFromThreadInfo(_aimId);
        }
#endif
    }

    void InterConnector::showMembersInSidebar(const QString& _aimId)
    {
        if (auto page = getMessengerPage())
            page->showMembersInSidebar(_aimId);
    }

    void InterConnector::setSidebarVisible(bool _visible)
    {
        Q_EMIT sidebarVisibilityChanged(_visible);
    }

    void InterConnector::navigateBackFromThreadInfo(const QString& _aimId)
    {
        auto appsPage = getAppsPage();
        if (appsPage->isMessengerPage())
        {
            if (auto page = getMessengerPage())
                page->closeThreadInfo(_aimId);
        }
#ifdef HAS_WEB_ENGINE
        else if (appsPage->isTasksPage())
        {
            if(auto page = getTasksPage())
                page->closeThreadInfo(_aimId);
        }
#endif
    }

    bool InterConnector::isSidebarVisible() const
    {
        if (auto page = getMessengerPage())
            return page->isSidebarVisible();

        return false;
    }

    void InterConnector::restoreSidebar()
    {
        if (auto page = getMessengerPage())
            page->restoreSidebar();
    }

    QString InterConnector::getSidebarAimid() const
    {
        if (auto page = getMessengerPage())
            return page->getSidebarAimid();
        return {};
    }

    QString InterConnector::getSidebarSelectedText() const
    {
        if (auto page = getMessengerPage())
            return page->getSidebarSelectedText();
        return {};
    }

    bool InterConnector::isSidebarWithThreadPage() const
    {
        if (auto page = getMessengerPage())
            return page->isSidebarWithThreadPage();
        return false;
    }

    void InterConnector::setDragOverlay(bool enable)
    {
        dragOverlay_ = enable;
    }

    bool InterConnector::isDragOverlay() const
    {
        return dragOverlay_;
    }

    void InterConnector::registerKeyCombinationPress(QKeyEvent* event, qint64 time)
    {
        static std::unordered_set<KeyCombination> WatchedKeyCombinations = {
            KeyCombination::Ctrl_or_Cmd_AltShift,
        };

        for (auto keyComb : WatchedKeyCombinations)
        {
            if (Utils::keyEventCorrespondsToKeyComb(event, keyComb))
            {
                registerKeyCombinationPress(keyComb, time);
            }
        }
    }

    void InterConnector::registerKeyCombinationPress(KeyCombination keyComb, qint64 time)
    {
        keyCombinationPresses_[keyComb] = time;
    }

    qint64 InterConnector::lastKeyEventMsecsSinceEpoch(KeyCombination keyCombination) const
    {
        const auto it = keyCombinationPresses_.find(keyCombination);
        return it == keyCombinationPresses_.end() ? QDateTime::currentMSecsSinceEpoch()
                                                  : it->second;
    }

    void InterConnector::setMultiselect(bool _enable, const QString& _contact, bool _fromKeyboard)
    {
        const auto current = _contact.isEmpty() ? Logic::getContactListModel()->selectedContact() : _contact;
        if (current.isEmpty())
            return;

        const auto appsPage = getAppsPage();
        const auto isAppPageVisible = appsPage && appsPage->isVisible();
        const auto mainPage = getMessengerPage();
        const bool infoOnlyVisible = isSidebarVisible() && mainPage && (!mainPage->isSideBarInSplitter() || mainPage->isOneFrameSidebar()) && !mainPage->isSidebarWithThreadPage();
        bool isTaskThreadOpened = false;
#ifdef HAS_WEB_ENGINE
        if (isAppPageVisible)
        {
            auto tasks = appsPage->tasksPage();
            isTaskThreadOpened = tasks && tasks->isSidebarVisible();
        }
#endif
        if ((isAppPageVisible && !appsPage->isMessengerPage() && !appsPage->isTasksPage() || (isAppPageVisible && appsPage->isTasksPage() && !isTaskThreadOpened)
            || (mainPage && !mainPage->isInRecentsTab() && !mainPage->isInContactsTab()) || infoOnlyVisible) && _enable)
        {
            return;
        }

        auto prevState = isMultiselect(current);

        if (_enable && !multiselectStates_.empty() && !prevState)
        {
            multiselectStates_.clear();
            Q_EMIT multiselectChanged();
        }

        if (_enable)
            multiselectStates_.insert(current);
        else
            multiselectStates_.erase(current);

        if (!_enable)
        {
            currentElement_ = MultiselectCurrentElement::Message;
            currentMessage_ = -1;
        }

        if (prevState != _enable)
        {
            for (const auto& [wdg, aimid] : disabledWidgets_)
            {
                if (wdg && aimid == current)
                    wdg->setAttribute(Qt::WA_TransparentForMouseEvents, _enable);
            }

            Q_EMIT multiselectChanged();
            if (_fromKeyboard)
            {
                clearPartialSelection(current);
                Q_EMIT multiSelectCurrentElementChanged();
            }

            multiselectAnimation_->stop();
            multiselectAnimation_->setDirection(_enable ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
            multiselectAnimation_->start();
        }
    }

    bool InterConnector::isMultiselect(const QString& _contact) const
    {
        return !_contact.isEmpty() && multiselectStates_.find(_contact) != multiselectStates_.end();
    }

    bool InterConnector::isMultiselect() const
    {
        return !multiselectStates_.empty();
    }

    void InterConnector::multiselectNextElementTab()
    {
        if (currentElement_ == MultiselectCurrentElement::Cancel)
            currentElement_ = MultiselectCurrentElement::Message;
        else if (currentElement_ == MultiselectCurrentElement::Message)
            currentElement_ = MultiselectCurrentElement::Delete;
        else
            currentElement_ = MultiselectCurrentElement::Cancel;

        Q_EMIT multiSelectCurrentElementChanged();
    }

    void InterConnector::multiselectNextElementRight()
    {
        if (currentElement_ == MultiselectCurrentElement::Delete)
            currentElement_ = MultiselectCurrentElement::Favorites;
        else if (currentElement_ == MultiselectCurrentElement::Favorites)
            currentElement_ = MultiselectCurrentElement::Copy;
        else if (currentElement_ == MultiselectCurrentElement::Copy)
            currentElement_ = MultiselectCurrentElement::Reply;
        else if (currentElement_ == MultiselectCurrentElement::Reply)
            currentElement_ = MultiselectCurrentElement::Forward;
        else if (currentElement_ == MultiselectCurrentElement::Forward)
            currentElement_ = MultiselectCurrentElement::Delete;
        else
            return;

        Q_EMIT multiSelectCurrentElementChanged();
    }

    void InterConnector::multiselectNextElementLeft()
    {
        if (currentElement_ == MultiselectCurrentElement::Delete)
            currentElement_ = MultiselectCurrentElement::Forward;
        else if (currentElement_ == MultiselectCurrentElement::Forward)
            currentElement_ = MultiselectCurrentElement::Reply;
        else if (currentElement_ == MultiselectCurrentElement::Reply)
            currentElement_ = MultiselectCurrentElement::Copy;
        else if (currentElement_ == MultiselectCurrentElement::Copy)
            currentElement_ = MultiselectCurrentElement::Favorites;
        else if (currentElement_ == MultiselectCurrentElement::Favorites)
            currentElement_ = MultiselectCurrentElement::Delete;
        else
            return;

        Q_EMIT multiSelectCurrentElementChanged();
    }

    void InterConnector::multiselectNextElementUp(bool _shift)
    {
        if (currentElement_ == MultiselectCurrentElement::Message)
            Q_EMIT multiSelectCurrentMessageUp(_shift);
    }

    void InterConnector::multiselectNextElementDown(bool _shift)
    {
        if (currentElement_ == MultiselectCurrentElement::Message)
            Q_EMIT multiSelectCurrentMessageDown(_shift);
    }

    void InterConnector::multiselectSpace()
    {
        if (currentElement_ == MultiselectCurrentElement::Message)
            Q_EMIT multiselectSpaceClicked();
    }

    void InterConnector::multiselectEnter()
    {
        const auto& contact = Logic::getContactListModel()->selectedContact();

        if (currentElement_ == MultiselectCurrentElement::Cancel)
            setMultiselect(false, contact);
        else if (currentElement_ == MultiselectCurrentElement::Delete)
            Q_EMIT multiselectDelete(contact);
        else if (currentElement_ == MultiselectCurrentElement::Favorites)
            Q_EMIT multiselectFavorites(contact);
        else if (currentElement_ == MultiselectCurrentElement::Forward)
            Q_EMIT multiselectForward(contact);
        else if (currentElement_ == MultiselectCurrentElement::Copy)
            Q_EMIT multiselectCopy(contact);
        else if (currentElement_ == MultiselectCurrentElement::Reply)
            Q_EMIT multiselectReply(contact);
    }

    qint64 InterConnector::currentMultiselectMessage() const
    {
        return currentMessage_;
    }

    void InterConnector::setCurrentMultiselectMessage(qint64 _id)
    {
        if (currentMessage_ != _id)
        {
            currentMessage_ = _id;
            Q_EMIT multiSelectCurrentMessageChanged();
        }
    }

    MultiselectCurrentElement InterConnector::currentMultiselectElement() const
    {
        return currentElement_;
    }

    double InterConnector::multiselectAnimationCurrent() const
    {
        return multiselectAnimation_->currentValue().toDouble();
    }

    void InterConnector::disableInMultiselect(QWidget* _w, const QString& _aimid)
    {
        im_assert(_w);
        im_assert(!_aimid.isEmpty());

        if (!_w || _aimid.isEmpty())
            return;

        disabledWidgets_[_w] = _aimid;
        _w->setAttribute(Qt::WA_TransparentForMouseEvents, isMultiselect(_aimid));
        connect(_w, &QWidget::destroyed, this, [this, _w]() { detachFromMultiselect(_w); });
    }

    void InterConnector::detachFromMultiselect(QWidget* _w)
    {
        if (!_w)
            return;

        auto iter = disabledWidgets_.find(_w);
        if (iter != disabledWidgets_.end())
            disabledWidgets_.erase(iter);
        else
            im_assert(!"not found");
    }

    void InterConnector::clearPartialSelection(const QString& _aimid)
    {
        if (auto page = getHistoryPage(_aimid))
            page->clearPartialSelection();
    }

    void InterConnector::openGallery(const Utils::GalleryData& _data)
    {
        auto mainWindow = getMainWindow();
        if (!mainWindow)
            return;

#ifdef __APPLE__
        QTimer::singleShot(50, [mainWindow, _data](){
#endif //__APPLE__
            mainWindow->openGallery(_data);
#ifdef __APPLE__
        });
#endif //__APPLE
    }

    bool InterConnector::isRecordingPtt(const QString& _aimId) const
    {
        if (auto page = getHistoryPage(_aimId))
            return page->isRecordingPtt();

        return false;
    }

    bool InterConnector::isRecordingPtt() const
    {
        const auto pages = getVisibleHistoryPages();
        return std::any_of(pages.begin(), pages.end(), [](auto p) { return p->isRecordingPtt(); });
    }

    void InterConnector::showChatMembersFailuresPopup(ChatMembersOperation _operation, QString _chatAimId, std::map<core::chat_member_failure, std::vector<QString>> _failures)
    {
        if (auto mp = getMessengerPage())
            mp->showChatMembersFailuresPopup(_operation, std::move(_chatAimId), std::move(_failures));
    }

    void InterConnector::connectTo(Ui::core_dispatcher* _dispatcher)
    {
        connect(_dispatcher, &Ui::core_dispatcher::authParamsChanged, this, &InterConnector::onAuthParamsChanged, Qt::UniqueConnection);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::externalUrlConfigUpdated, this, &InterConnector::configUpdated);
    }

    std::pair<QUrl, bool> InterConnector::signUrl(const QString& _miniappId, QUrl _url) const
    {
        bool success = false;
        if (isMiniAppAuthParamsValid(_miniappId) && _url.isValid())
        {
            QUrlQuery query;
            query.addQueryItem(qsl("aimsid"), authParams_->getMiniAppAimsid(_miniappId));
            _url = Utils::addQueryItemsToUrl(std::move(_url), std::move(query), Utils::AddQueryItemPolicy::ReplaceIfItemExists);
            success = true;
        }

        return { _url, success };
    }

    QString InterConnector::aimSid() const
    {
        return authParams_->getAimsid();
    }

    bool InterConnector::isAuthParamsValid() const
    {
        return authParams_ && !authParams_->isEmpty();
    }

    bool InterConnector::isMiniAppAuthParamsValid(const QString& _miniappId, bool _canUseMainAimisid) const
    {
        return authParams_ && !authParams_->getMiniAppAimsid(_miniappId, _canUseMainAimisid).isEmpty();
    }

    void InterConnector::onAuthParamsChanged(const Data::AuthParams& _params)
    {
        const auto prevParams = std::exchange(authParams_, std::make_unique<Data::AuthParams>(_params));
        const auto aimsidChanged = !prevParams || !prevParams->isTheSameMiniApps(*authParams_) || authParams_->isMiniAppsEmpty();

        Q_EMIT authParamsChanged(aimsidChanged);
    }

    void InterConnector::openDialog(const QString& _aimId, qint64 _id, bool _select, Ui::PageOpenedAs _openedAs, std::function<void(Ui::PageBase*)> _getPageCallback, bool _ignoreScroll)
    {
        if (!Utils::InterConnector::instance().getMainWindow()->isWebAppTasksPage() && _openedAs == Ui::PageOpenedAs::MainPage)
            getMainWindow()->showMessengerPage();

        if (_aimId.isEmpty())
        {
            Logic::getContactListModel()->setCurrent({});
            if (_openedAs == Ui::PageOpenedAs::ReopenedMainPage)
            {
                if (auto messenger = getMainWindow()->getMessengerPage())
                    messenger->scrollRecentsToTop();
            }
            return;
        }

        const auto pages = getVisibleHistoryPages();
        const auto it = std::find_if(pages.begin(), pages.end(), [&_aimId](auto p) { return p->aimId() == _aimId; });
        if (it != pages.end())
        {
            if (_openedAs == Ui::PageOpenedAs::MainPage || _openedAs == Ui::PageOpenedAs::FeedPageScrollable)
                (*it)->scrollToMessage(_id);
            if (_getPageCallback)
                _getPageCallback(*it);

            Q_EMIT Utils::InterConnector::instance().setFocusOnInput(_aimId);
        }
        else if (!Logic::getContactListModel()->isThread(_aimId))
        {
            const auto openedAsMain = _openedAs == Ui::PageOpenedAs::MainPage || _openedAs == Ui::PageOpenedAs::ReopenedMainPage;
            Logic::getContactListModel()->setCurrent(_aimId, _id, _select, openedAsMain ? Ui::PageOpenedAs::MainPage : Ui::PageOpenedAs::FeedPage, std::move(_getPageCallback), _ignoreScroll);
        }
        else
        {
            im_assert(false);
        }
    }

    void InterConnector::closeDialog()
    {
        updateAppsPageReopenPage({});
        openDialog({});
    }

    void InterConnector::openThreadSearchResult(const QString& _aimId, const qint64 _messageId, const Ui::highlightsV& _highlights)
    {
        const auto& appsPage = getAppsPage();
        if (appsPage->isMessengerPage())
        {
            if (const auto parentAimId = Logic::getContactListModel()->getThreadParent(_aimId).value_or(QString()); !parentAimId.isEmpty())
            {
                if (auto page = getHistoryPage(parentAimId))
                    page->resetMessageHighlights();
            }
            Q_EMIT openThread(_aimId, _messageId, Logic::getContactListModel()->getThreadParent(_aimId).value_or(QString()));
        }
#ifdef HAS_WEB_ENGINE
        else if (appsPage->isTasksPage())
        {
            if (auto page = getTasksPage())
                page->showInfo(_aimId, Ui::SidebarContentType::thread);
        }
#endif
        Q_EMIT scrollThreadToMsg(_aimId, _messageId, _highlights);
    }

    bool InterConnector::areBotCommandsDisabled(const QString& _aimid)
    {
        return botCommandsDisabledChats_.contains(_aimid);
    }

    void InterConnector::closeAndHighlightDialog()
    {
        if (auto mp = getMessengerPage())
            mp->closeAndHighlightDialog();
    }

    void InterConnector::setLoginComplete(bool _lc)
    {
        loginComplete_ = _lc;
    }

    bool InterConnector::getLoginComplete()
    {
        return loginComplete_;
    }

#ifdef HAS_WEB_ENGINE
    QWebEngineProfile* InterConnector::getLocalProfile()
    {
        if (!localProfile_)
            localProfile_ = new QWebEngineProfile(qsl("local"), getAppsPage());

        return localProfile_;
    }
#endif//HAS_WEB_ENGINE

    void InterConnector::configUpdated()
    {
        const auto newChats = Features::getBotCommandsDisabledChats();
        if (newChats != botCommandsDisabledChats_)
        {
            botCommandsDisabledChats_ = newChats;
            Q_EMIT botCommandsDisabledChatsUpdated();
        }

        if (getLoginComplete() && Styling::getThemesAreLoaded())
            Styling::getThemesContainer().chooseAndSetTheme();

    }
}

