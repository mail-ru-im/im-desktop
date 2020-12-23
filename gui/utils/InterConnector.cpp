#include "stdafx.h"

#include "../main_window/MainWindow.h"
#include "../main_window/MainPage.h"
#include "../main_window/ContactDialog.h"
#include "../main_window/history_control/HistoryControlPage.h"
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
        , currentElement_(MultiselectCurrentElement::Message)
        , currentMessage_(-1)
        , multiselectAnimation_(new QVariantAnimation(this))
    {
        multiselectAnimation_->setStartValue(0);
        multiselectAnimation_->setEndValue(100);
        multiselectAnimation_->setDuration(100);
        connect(multiselectAnimation_, &QVariantAnimation::valueChanged, this, &InterConnector::multiselectAnimationUpdate);
    }

    InterConnector::~InterConnector()
    {
        assert(disabledWidgets_.empty());
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
            return MainWindow_->getHistoryPage(aimId);

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

    void InterConnector::showSidebarWithParams(const QString &aimId, Ui::SidebarParams _params)
    {
        if (MainWindow_)
            MainWindow_->showSidebarWithParams(aimId, std::move(_params));
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

    void InterConnector::setMultiselect(bool _enable, const QString& _contact, bool _fromKeyboard)
    {
        const auto current = _contact.isEmpty() ? Logic::getContactListModel()->selectedContact() : _contact;
        if (current.isEmpty())
            return;

        auto prevState = false;
        if (auto iter = multiselectStates_.find(current); iter != multiselectStates_.end())
            prevState = iter->second;

        multiselectStates_[current] = _enable;

        if (!_enable)
        {
            currentElement_ = MultiselectCurrentElement::Message;
            currentMessage_ = -1;
        }


        if (prevState != _enable)
        {
            for (const auto& w : disabledWidgets_)
            {
                if (w.second == current)
                    w.first->setAttribute(Qt::WA_TransparentForMouseEvents, _enable);
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
        const auto current = _contact.isEmpty() ? Logic::getContactListModel()->selectedContact() : _contact;
        if (auto iter = multiselectStates_.find(current); iter != multiselectStates_.end())
            return iter->second;

        return false;
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
        if (currentElement_ == MultiselectCurrentElement::Cancel)
            setMultiselect(false);
        else if (currentElement_ == MultiselectCurrentElement::Delete)
            Q_EMIT multiselectDelete();
        else if (currentElement_ == MultiselectCurrentElement::Favorites)
            Q_EMIT multiselectFavorites();
        else if (currentElement_ == MultiselectCurrentElement::Forward)
            Q_EMIT multiselectForward(Logic::getContactListModel()->selectedContact());
        else if (currentElement_ == MultiselectCurrentElement::Copy)
            Q_EMIT multiselectCopy();
        else if (currentElement_ == MultiselectCurrentElement::Reply)
            Q_EMIT multiselectReply();
    }

    qint64 InterConnector::currentMultiselectMessage() const
    {
        return currentMessage_;
    }

    void InterConnector::setCurrentMultiselectMessage(qint64 _id)
    {
        currentMessage_ = _id;
        Q_EMIT multiSelectCurrentMessageChanged();
    }

    MultiselectCurrentElement InterConnector::currentMultiselectElement() const
    {
        return currentElement_;
    }

    double InterConnector::multiselectAnimationCurrent() const
    {
        //if (!isMultiselect())
          //  return 0.0;

        return multiselectAnimation_->currentValue().toDouble();
    }

    void InterConnector::disableInMultiselect(QWidget* _w, const QString& _aimid)
    {
        if (!_w)
            return;

        auto aimid = _aimid.isEmpty() ? Logic::getContactListModel()->selectedContact() : _aimid;
        disabledWidgets_[_w] = aimid;

        _w->setAttribute(Qt::WA_TransparentForMouseEvents, isMultiselect(aimid));
    }

    void InterConnector::detachFromMultiselect(QWidget* _w)
    {
        if (!_w)
            return;

        auto iter = disabledWidgets_.find(_w);
        if (iter != disabledWidgets_.end())
            disabledWidgets_.erase(iter);
        else
            assert(!"not found");
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

    bool InterConnector::isRecordingPtt() const
    {
        if (auto contactDialog = getContactDialog())
            return contactDialog->isRecordingPtt();
        return false;
    }

    void InterConnector::showAddMembersFailuresPopup(QString _chatAimId, std::map<core::add_member_failure, std::vector<QString>> _failures)
    {
        if (auto mp = getMainPage())
            mp->showAddMembersFailuresPopup(std::move(_chatAimId), std::move(_failures));
    }
}
