#include "stdafx.h"

#include "HistoryControl.h"
#include "HistoryControlPage.h"

#include "../MainPage.h"
#include "../MainWindow.h"
#include "../Placeholders.h"
#include "../ConnectionWidget.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/UnknownsModel.h"
#include "../contact_list/RecentsModel.h"
#include "../containers/FriendlyContainer.h"
#include "../containers/LastseenContainer.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../controls/BackgroundWidget.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/log/log.h"
#include "../../styles/ThemeParameters.h"
#include "../../styles/ThemesContainer.h"
#include "../../utils/gui_metrics.h"

namespace
{
    constexpr std::chrono::milliseconds update_timer_timeout = std::chrono::minutes(1);
    constexpr auto maxPagesInDialogHistory = 1;

    int getPlaceholderHeight()
    {
        return Utils::scale_value(36);
    }

    QColor getTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
    }
}

namespace Ui
{
    SelectDialogWidget::SelectDialogWidget(QWidget* _parent)
        : QWidget(_parent)
        , connectionWidget_(new ConnectionWidget(this, getTextColor()))
        , selectDialogLabel_(new QLabel(this))
        , rootLayout_(new QHBoxLayout(this))
    {
        setFixedHeight(getPlaceholderHeight() + Utils::getShadowMargin() * 2);

        rootLayout_->setContentsMargins(Utils::scale_value(12), 0, Utils::scale_value(12), 0);

        setTextColor(getTextColor());

        selectDialogLabel_->setFont(Fonts::appFontScaled(16, Fonts::FontWeight::Normal));
        selectDialogLabel_->setText(QT_TRANSLATE_NOOP("history", "Please select a chat to start messaging"));

        Testing::setAccessibleName(connectionWidget_, qsl("AS HistoryPage connectionWidget"));
        rootLayout_->addWidget(connectionWidget_);
        Testing::setAccessibleName(selectDialogLabel_, qsl("AS HistoryPage selectDialogLabel"));
        rootLayout_->addWidget(selectDialogLabel_);

        connect(GetDispatcher(), &Ui::core_dispatcher::connectionStateChanged, this, &SelectDialogWidget::connectionStateChanged);
        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalWallpaperChanged, this, &SelectDialogWidget::onGlobalWallpaperChanged);

        const auto connectionState = GetDispatcher()->getConnectionState();
        connectionStateChanged(connectionState);
    }

    void SelectDialogWidget::connectionStateChanged(const Ui::ConnectionState& _state)
    {
        rootLayout_->setContentsMargins(
            (_state == ConnectionState::stateOnline) ? Utils::scale_value(12) : Utils::scale_value(8),
            0,
            Utils::scale_value(12), 0);

        connectionWidget_->setVisible(_state != ConnectionState::stateOnline);
        selectDialogLabel_->setVisible(_state == ConnectionState::stateOnline);
    }

    void SelectDialogWidget::onGlobalWallpaperChanged()
    {
        const auto clr = getTextColor();
        setTextColor(clr);
        connectionWidget_->setTextColor(clr);
    }

    void SelectDialogWidget::setTextColor(const QColor& _color)
    {
        QPalette pal;
        pal.setColor(QPalette::Window, Qt::transparent);
        pal.setColor(QPalette::WindowText, _color);
        selectDialogLabel_->setPalette(pal);
    }

    void SelectDialogWidget::paintEvent(QPaintEvent* _e)
    {
        QPainterPath path;
        {
            const QRect r(0, (height() - getPlaceholderHeight()) / 2, width(), getPlaceholderHeight());
            const auto radius = r.height() / 2;
            path.addRoundedRect(r, radius, radius);
        }

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);

        Utils::drawBubbleShadow(p, path);
        p.fillPath(path, Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
    }



    class EmptyPage : public QWidget
    {
    public:
        explicit EmptyPage(QWidget* _parent = nullptr);

    private:
        QWidget* dialogWidget_;
    };

    EmptyPage::EmptyPage(QWidget *_parent)
        : QWidget(_parent)
        , dialogWidget_(nullptr)
    {
        auto layout = Utils::emptyHLayout(this);

        dialogWidget_ = new QWidget(this);

        auto dialogLayout = Utils::emptyHLayout(dialogWidget_);

        SelectDialogWidget* selectDialogWidget_ = new SelectDialogWidget(dialogWidget_);

        dialogLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

        Testing::setAccessibleName(selectDialogWidget_, qsl("AS HistoryPage selectDialogWidget"));
        dialogLayout->addWidget(selectDialogWidget_);
        dialogLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

        dialogWidget_->setLayout(dialogLayout);

        Testing::setAccessibleName(dialogWidget_, qsl("AS HistoryPage dialogWidget"));
        layout->addWidget(dialogWidget_);
    }

    HistoryControl::HistoryControl(QWidget* parent)
        : QWidget(parent)
        , timer_(new QTimer(this))
        , emptyPage_(new EmptyPage(this))
        , frameCountMode_(FrameCountMode::_1)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        auto layout = Utils::emptyVLayout(this);
        stackPages_ = new QStackedWidget(this);
        stackPages_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        Testing::setAccessibleName(emptyPage_, qsl("AS HistoryPage emptyPage"));
        stackPages_->addWidget(emptyPage_);
        Testing::setAccessibleName(stackPages_, qsl("AS HistoryPage stackPages"));
        layout->addWidget(stackPages_);

        updateEmptyPageWallpaper();

        connect(timer_, &QTimer::timeout, this, &HistoryControl::updatePages);
        timer_->setInterval(update_timer_timeout);
        timer_->setSingleShot(false);
        timer_->start();

        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalWallpaperChanged, this, &HistoryControl::onGlobalWallpaperChanged);
        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::contactWallpaperChanged, this, &HistoryControl::onContactWallpaperChanged);
        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::wallpaperImageAvailable, this, &HistoryControl::onWallpaperAvailable);

        connect(Logic::getContactListModel(), &Logic::ContactListModel::leave_dialog, this, &HistoryControl::leaveDialog);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, &HistoryControl::closeDialog);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::selectedContactChanged, this, &HistoryControl::onContactSelected);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::activeDialogHide, this, &HistoryControl::closeDialog);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dlgStates, this, &HistoryControl::dlgStates);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::currentPageChanged, this, &HistoryControl::mainPageChanged);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::addPageToDialogHistory, this, &HistoryControl::addPageToDialogHistory);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::clearDialogHistory, this, &HistoryControl::clearDialogHistory);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::headerBack, this, &HistoryControl::onBackToChats);

        connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStatesHandled, this, &HistoryControl::updateUnreads);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, &HistoryControl::updateUnreads);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::dlgStatesHandled, this, &HistoryControl::updateUnreads);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updatedMessages, this, &HistoryControl::updateUnreads);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &HistoryControl::updateUnreads);
    }

    HistoryControl::~HistoryControl() = default;

    void HistoryControl::cancelSelection()
    {
        for (const auto &page : std::as_const(pages_))
        {
            assert(page);
            page->cancelSelection();
        }
    }

    HistoryControlPage* HistoryControl::getHistoryPage(const QString& aimId) const
    {
        if (const auto iter = pages_.find(aimId); iter != pages_.end())
            return *iter;
        return nullptr;
    }

    bool HistoryControl::hasMessageUnderCursor() const
    {
        const auto page = getCurrentPage();
        return page && page->hasMessageUnderCursor();
    }

    void HistoryControl::notifyApplicationWindowActive(const bool isActive)
    {
        if (!isActive)
        {
            for (const auto &page : std::as_const(pages_))
                page->suspendVisisbleItems();
        }
        else
        {
            auto page = getCurrentPage();
            if (page)
                page->resumeVisibleItems();
        }

        auto page = getCurrentPage();
        if (page)
            page->notifyApplicationWindowActive(isActive);
    }

    void HistoryControl::notifyUIActive(const bool _isActive)
    {
        auto page = getCurrentPage();
        if (page)
            page->notifyUIActive(_isActive);
    }

    void HistoryControl::scrollHistoryToBottom(const QString& _contact) const
    {
        auto page = getHistoryPage(_contact);
        if (page)
            page->scrollToBottom();
    }

    void HistoryControl::mouseReleaseEvent(QMouseEvent *e)
    {
        QWidget::mouseReleaseEvent(e);

        if (getCurrentPage())
            Q_EMIT clicked();
    }

    void HistoryControl::updatePages()
    {
        const auto currentTime = QTime::currentTime();
        constexpr std::chrono::seconds time = std::chrono::minutes(5);
        for (auto iter = times_.begin(); iter != times_.end(); )
        {
            if (iter.value().secsTo(currentTime) >= time.count() && iter.key() != current_)
            {
                closeDialog(iter.key());
                iter = times_.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    void HistoryControl::contactSelected(const QString& _aimId, qint64 _messageId, const highlightsV& _highlights)
    {
        assert(!_aimId.isEmpty());

        const bool contactChanged = (_aimId != current_);

        const Data::DlgState dlgState = Logic::getRecentsModel()->getDlgState(_aimId);
        const qint64 lastReadMsg = dlgState.UnreadCount_ > 0 && dlgState.YoursLastRead_ < dlgState.LastMsgId_ ? dlgState.YoursLastRead_ : -1;
        auto scrollMode = hist::scroll_mode_type::none;
        if (_messageId == -1)
        {
            _messageId = lastReadMsg;
            if (_messageId != -1)
                scrollMode = hist::scroll_mode_type::unread;
        }
        else
        {
            scrollMode = hist::scroll_mode_type::search;
        }

        auto oldPage = getCurrentPage();
        if (oldPage)
        {
            const bool canScrollToBottom = scrollMode == hist::scroll_mode_type::unread;
            if (_messageId == -1 || canScrollToBottom)
            {
                if (oldPage->aimId() == _aimId)
                {
                    oldPage->scrollToBottom();

                    Utils::InterConnector::instance().setFocusOnInput();

                    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_scr_read_all_event, { { "Type", "Recents_Double_Click" } });

                    return;
                }

                if (!canScrollToBottom)
                    oldPage->updateState(true);
            }

            oldPage->resetShiftingParams();

            if (contactChanged)
                oldPage->pageLeave();
        }

        auto page = getHistoryPage(_aimId);
        const auto createNewPage = (page == nullptr);
        if (createNewPage)
        {
            auto newPage = new HistoryControlPage(this, _aimId);
            newPage->setHistoryControl(this);
            newPage->setPinnedMessage(dlgState.pinnedMessage_);

            page = *(pages_.insert(_aimId, newPage));
            connect(page, &HistoryControlPage::quote, this, &HistoryControl::quote);
            connect(page, &HistoryControlPage::switchToPrevDialog, this, &HistoryControl::switchToPrevDialogPage);
            connect(page, &HistoryControlPage::messageIdsFetched, this, [this](const QString& _aimId, const Data::MessageBuddies& _resolves)
            {
                Q_EMIT messageIdsFetched(_aimId, _resolves, QPrivateSignal());
            });

            Testing::setAccessibleName(page, qsl("AS HistoryPage ") % _aimId);
            stackPages_->addWidget(page);

            Logic::getContactListModel()->setCurrentCallbackHappened(newPage);
        }

        Q_EMIT Utils::InterConnector::instance().historyControlReady(_aimId, _messageId, dlgState, -1, createNewPage);

        page->resetMessageHighlights();
        if (!_highlights.empty())
            page->setHighlights(_highlights);

        if (scrollMode != hist::scroll_mode_type::none || createNewPage)
            page->initFor(_messageId, scrollMode, createNewPage ? HistoryControlPage::FirstInit::Yes : HistoryControlPage::FirstInit::No);

        if (createNewPage)
            statistic::getGuiMetrics().eventChatOpen(_aimId);

        const auto prevButtonVisible = !dialogHistory_.empty();
        for (const auto& p : std::as_const(pages_))
        {
            const auto isBackgroundPage = (p != page);
            if (isBackgroundPage)
                p->suspendVisisbleItems();

            page->setPrevChatButtonVisible(prevButtonVisible || (frameCountMode_ == FrameCountMode::_1));
            page->setOverlayTopWidgetVisible(frameCountMode_ == FrameCountMode::_1);
            page->setUnreadBadgeText(unreadText_);
        }

        if (contactChanged)
            Logic::GetLastseenContainer()->unsubscribe(current_);

        rememberCurrentDialogTime();
        current_ = _aimId;

        stackPages_->setCurrentWidget(page);
        page->updateState(false);
        page->open();
        Q_EMIT setTopWidget(current_, page->getTopWidget(), QPrivateSignal());

        const auto& tc = Styling::getThemesContainer();
        const auto newWpId = tc.getContactWallpaperId(page->aimId());
        if (!oldPage || (oldPage && tc.getContactWallpaperId(oldPage->aimId()) != newWpId))
            updateWallpaper(_aimId);

        if (contactChanged)
        {
            page->pageOpen();
            if (dlgState.Attention_)
                Logic::getRecentsModel()->setAttention(_aimId, false);
        }

        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();

        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        GetDispatcher()->post_message_to_core("dialogs/add", collection.get());
    }

    void HistoryControl::leaveDialog(const QString& _aimId)
    {
        auto iter_page = pages_.constFind(_aimId);
        if (iter_page == pages_.cend())
            return;

        if (current_ == _aimId)
        {
            iter_page.value()->pageLeave();
            switchToEmpty();
        }
    }

    void HistoryControl::switchToEmpty()
    {
        rememberCurrentDialogTime();

        current_.clear();
        window()->setFocus();
        stackPages_->setCurrentWidget(emptyPage_);
        updateEmptyPageWallpaper();
        MainPage::instance()->hideInput();
    }

    void HistoryControl::addPageToDialogHistory(const QString& _aimId)
    {
        if (dialogHistory_.empty() || dialogHistory_.back() != _aimId)
        {
            dialogHistory_.push_back(_aimId);
            Q_EMIT Utils::InterConnector::instance().pageAddedToDialogHistory();
        }

        while (dialogHistory_.size() > maxPagesInDialogHistory)
            dialogHistory_.pop_front();
    }

    void HistoryControl::clearDialogHistory()
    {
        dialogHistory_.clear();
    }

    void HistoryControl::switchToPrevDialogPage(const bool _calledFromKeyboard)
    {
        if (!dialogHistory_.empty())
        {
            const auto prev = dialogHistory_.back();
            dialogHistory_.pop_back();
            Logic::getContactListModel()->setCurrent(prev, -1, true);
        }
        else
        {
            Q_EMIT Utils::InterConnector::instance().closeLastDialog(_calledFromKeyboard);
        }

        if (dialogHistory_.empty())
        {
            Q_EMIT Utils::InterConnector::instance().noPagesInDialogHistory();
        }
    }

    void HistoryControl::dlgStates(const QVector<Data::DlgState>& _states)
    {
        for (const auto& dlg : _states)
        {
            auto iter_page = pages_.find(dlg.AimId_);
            if (iter_page == pages_.end())
                return;

            auto page = iter_page.value();
            page->setPinnedMessage(dlg.pinnedMessage_);
        }
    }

    void HistoryControl::closeDialog(const QString& _aimId)
    {
        auto iter_page = pages_.find(_aimId);
        if (iter_page == pages_.end())
            return;

        auto page = iter_page.value();

        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        GetDispatcher()->post_message_to_core("dialogs/remove", collection.get());

        const bool isCurrent = current_ == _aimId;
        if (isCurrent)
            switchToEmpty();

        stackPages_->removeWidget(page);

        page->pageLeave();
        page->deleteLater();
        pages_.erase(iter_page);

        Q_EMIT Utils::InterConnector::instance().dialogClosed(_aimId, isCurrent);
    }

    void HistoryControl::mainPageChanged()
    {
        if (auto page = getCurrentPage())
        {
            if (Utils::InterConnector::instance().getMainWindow()->isMainPage())
                page->pageOpen();
            else
                page->pageLeave();
        }
    }

    void HistoryControl::onContactSelected(const QString & _aimId)
    {
        auto page = getCurrentPage();
        if (page && _aimId.isEmpty())
        {
            switchToEmpty();
            page->pageLeave();
        }
    }

    void HistoryControl::onBackToChats()
    {
        auto page = getCurrentPage();
        if (page)
            page->updateState(false);
    }

    void HistoryControl::updateUnreads()
    {
        unreadText_ = Utils::getUnreadsBadgeStr(Logic::getUnknownsModel()->totalUnreads() + Logic::getRecentsModel()->totalUnreads());
        for (const auto& page : std::as_const(pages_))
            page->setUnreadBadgeText(unreadText_);
    }

    void HistoryControl::onGlobalWallpaperChanged()
    {
        if (const auto page = getCurrentPage())
        {
            updateWallpaper(page->aimId());
            page->updateWidgetsTheme();
        }
        else
        {
            updateEmptyPageWallpaper();
        }
    }

    void HistoryControl::onContactWallpaperChanged(const QString& _aimId)
    {
        if (const auto page = getCurrentPage(); page && page->aimId() == _aimId)
        {
            updateWallpaper(_aimId);
            page->updateWidgetsTheme();
        }
    }

    void HistoryControl::onWallpaperAvailable(const Styling::WallpaperId& _id)
    {
        if (const auto page = getCurrentPage())
        {
            if (const auto wpId = Styling::getThemesContainer().getContactWallpaperId(page->aimId()); wpId.isValid())
                updateWallpaper(page->aimId());
        }
        else
        {
            updateEmptyPageWallpaper();
        }
    }

    HistoryControlPage* HistoryControl::getCurrentPage() const
    {
        assert(stackPages_);

        return qobject_cast<HistoryControlPage*>(stackPages_->currentWidget());
    }

    void HistoryControl::updateWallpaper(const QString& _aimId) const
    {
        Q_EMIT needUpdateWallpaper(_aimId, QPrivateSignal());
    }

    void HistoryControl::updateEmptyPageWallpaper() const
    {
        updateWallpaper(QString());
    }

    void HistoryControl::rememberCurrentDialogTime()
    {
        if (!current_.isEmpty())
            times_[current_] = QTime::currentTime();
    }

    void HistoryControl::inputTyped()
    {
        auto page = getCurrentPage();
        assert(page);
        if (!page)
            return;

        page->inputTyped();
    }

    const QString& HistoryControl::currentAimId() const
    {
        return current_;
    }

    void HistoryControl::setFrameCountMode(FrameCountMode _mode)
    {
        if (frameCountMode_ != _mode)
        {
            frameCountMode_ = _mode;
            if (auto page = getCurrentPage())
            {
                page->setPrevChatButtonVisible(!dialogHistory_.empty() || (frameCountMode_ == FrameCountMode::_1));
                page->setOverlayTopWidgetVisible(frameCountMode_ == FrameCountMode::_1);
            }
        }
    }
}
