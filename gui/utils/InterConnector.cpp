#include "stdafx.h"

#include "../main_window/MainWindow.h"
#include "../main_window/MainPage.h"
#include "../main_window/contact_list/ContactListModel.h"
#ifdef __APPLE__
#include "../utils/macos/mac_support.h"
#endif //__APPLE__

#include "InterConnector.h"

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
    {
        //
    }

    InterConnector::~InterConnector()
    {
        //
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

    Ui::HistoryControlPage* InterConnector::getHistoryPage(const QString& aimId) const
    {
        if (MainWindow_)
        {
            return MainWindow_->getHistoryPage(aimId);
        }

        return nullptr;
    }

    Ui::ContactDialog* InterConnector::getContactDialog() const
    {
        auto mainPage = getMainPage();
        if (!mainPage)
            return nullptr;

        return mainPage->getContactDialog();
    }

    Ui::MainPage *InterConnector::getMainPage() const
    {
        if (!MainWindow_)
            return nullptr;

        return MainWindow_->getMainPage();
    }

    bool InterConnector::isInBackground() const
    {
        if (MainWindow_)
            return !(MainWindow_->isActive());
        return false;
    }

    void InterConnector::showSidebar(const QString& aimId)
    {
        if (MainWindow_)
            MainWindow_->showSidebar(aimId);
    }

    void InterConnector::showMembersInSidebar(const QString& aimId)
    {
        if (MainWindow_)
            MainWindow_->showMembersInSidebar(aimId);
    }

    void InterConnector::setSidebarVisible(bool _visible)
    {
        setSidebarVisible(SidebarVisibilityParams(_visible, false));
    }

    void InterConnector::setSidebarVisible(const SidebarVisibilityParams &_params)
    {
        if (MainWindow_)
            MainWindow_->setSidebarVisible(_params);
    }

    bool InterConnector::isSidebarVisible() const
    {
        if (MainWindow_)
            return MainWindow_->isSidebarVisible();

        return false;
    }

    void InterConnector::restoreSidebar()
    {
        if (MainWindow_)
            MainWindow_->restoreSidebar();
    }

    void InterConnector::setDragOverlay(bool enable)
    {
        dragOverlay_ = enable;
    }

    bool InterConnector::isDragOverlay() const
    {
        return dragOverlay_;
    }

    void InterConnector::setFocusOnInput()
    {
        if (MainWindow_)
            MainWindow_->setFocusOnInput();
    }

    void InterConnector::onSendMessage(const QString& contact)
    {
        if (MainWindow_)
            MainWindow_->onSendMessage(contact);
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
}
