#include "stdafx.h"

#include "ThreadPage.h"
#include "main_window/history_control/HistoryControlPage.h"
#include "main_window/history_control/top_widget/ThreadHeaderPanel.h"
#include "main_window/contact_list/RecentsModel.h"
#include "main_window/contact_list/ContactListModel.h"
#include "utils/InterConnector.h"
#include "utils/utils.h"
#include "controls/BackgroundWidget.h"
#include "types/message.h"
#include "core_dispatcher.h"
#include "app_config.h"

namespace Ui
{
    ThreadPage::ThreadPage(QWidget* _parent)
        : SidebarPage(_parent)
        , bg_(new BackgroundWidget(this, false))
    {
        auto layout = Utils::emptyVLayout(this);
        layout->addWidget(bg_);

        bg_->setLayout(Utils::emptyVLayout());

        connect(GetDispatcher(), &core_dispatcher::dlgStates, this, &ThreadPage::dlgStates);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::scrollThreadToMsg, this, &ThreadPage::onScrollThreadToMsg);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::yourRoleChanged, this, &ThreadPage::onChatRoleChanged);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::scrollThreadToMsg, this, &ThreadPage::onScrollThreadToMsg);
    }

    ThreadPage::~ThreadPage()
    {
        unloadPage();
    }

    void ThreadPage::initFor(const QString& _aimId, SidebarParams _params)
    {
        parentAimId_ = _params.parentPageId_;
        bg_->updateWallpaper(parentAimId_);

        stopUnloadTimer();

        const auto createNewPage = (page_ == nullptr);
        const auto dlgState = Logic::getRecentsModel()->getDlgState(_aimId);
        const qint64 lastReadMsg = dlgState.UnreadCount_ > 0 ? dlgState.YoursLastRead_ : -1;

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
        }

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
        if (auto header = findChild<ThreadHeaderPanel*>())
            header->updateCloseIcon(_mode);
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
            Utils::InterConnector::instance().setFocusOnInput(page_->aimId());
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

    void ThreadPage::onChatRoleChanged(const QString& _aimId)
    {
        if (isActive() && !parentAimId_.isEmpty() && _aimId == parentAimId_)
        {
            if (Logic::getContactListModel()->isReadonly(_aimId))
                Utils::InterConnector::instance().setSidebarVisible(false);
        }
    }

    void ThreadPage::onScrollThreadToMsg(const QString& _threadId, int64_t _msgId)
    {
        if (_threadId == page_->aimId())
            page_->scrollToMessage(_msgId);
    }
}
