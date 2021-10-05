#include "stdafx.h"

#include "HistoryControl.h"
#include "HistoryControlPage.h"
#include "GalleryPage.h"

#include "../MainPage.h"
#include "../MainWindow.h"
#include "../ContactDialog.h"
#include "../Placeholders.h"
#include "../ConnectionWidget.h"
#include "../../controls/DialogButton.h"
#include "../../controls/CustomButton.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/UnknownsModel.h"
#include "../contact_list/RecentsModel.h"
#include "../containers/FriendlyContainer.h"
#include "../containers/LastseenContainer.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/log/log.h"
#include "../../styles/ThemeParameters.h"
#include "../../styles/ThemesContainer.h"
#include "../../utils/gui_metrics.h"
#include "../../utils/features.h"
#include "../../previewer/toast.h"
#include "feeds/FeedPage.h"
#include "../../app_config.h"

namespace
{
    constexpr auto maxPagesInDialogHistory = 1;

    int getPlaceholderHeight() noexcept
    {
        return Utils::scale_value(36);
    }

    QColor getTextColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID);
    }

    int bubbleLeftMarginForWidgetWithSpinner() noexcept { return Utils::scale_value(8); }

    constexpr int maxCallParticipantsLabelNumber() { return 30; }

    void setLabelTextColor(QLabel* _label, QColor _color)
    {
        QPalette pal;
        pal.setColor(QPalette::Window, Qt::transparent);
        pal.setColor(QPalette::WindowText, _color);
        _label->setPalette(pal);
    }
}

namespace Ui
{
    class BubblePlateWidget : public QWidget
    {
        static int shadowMargin() noexcept { return Utils::getShadowMargin(); }
        static int defaultLeftMargin() noexcept { return Utils::scale_value(12); }

    public:
        BubblePlateWidget(QWidget* _parent)
            : QWidget(_parent)
        {
            setLayout(Utils::emptyVLayout());
            layout()->setContentsMargins(defaultLeftMargin(), 0, Utils::scale_value(12), shadowMargin());
        }

        void setLeftContentsMargin(int _leftMarginScaled)
        {
            auto margins = layout()->contentsMargins();
            margins.setLeft(_leftMarginScaled);
            layout()->setContentsMargins(margins);
        }

        void restoreLeftContentsMargin()
        {
            setLeftContentsMargin(defaultLeftMargin());
        }

    protected:
        void paintEvent(QPaintEvent* _e) override
        {
            QPainterPath path;
            {
                const QRect r(0, 0, width(), height() - shadowMargin());
                const auto radius = Utils::scale_value(18);
                path.addRoundedRect(r, radius, radius);
            }

            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(Qt::NoPen);

            Utils::drawBubbleShadow(p, path);
            p.fillPath(path, Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        }
    };


    class SelectDialogWidget : public QLabel
    {
    public:
        SelectDialogWidget(QWidget* _parent)
            : QLabel(_parent)
        {
            setFixedHeight(getPlaceholderHeight());

            setTextColor(getTextColor());
            setFont(Fonts::appFontScaled(16, Fonts::FontWeight::Normal));
            setText(QT_TRANSLATE_NOOP("history", "Please select a chat to start messaging"));
        }

        void setTextColor(const QColor& _color)
        {
            setLabelTextColor(this, _color);
        }
    };


    class StartCallDialogWidget : public QWidget
    {
    public:
        QLabel* tittle_;
        static QSize widgetSize() noexcept { return Utils::scale_value(QSize(310, 208)); }
        static QMargins widgetMargins() noexcept { return Utils::scale_value(QMargins(16, 16, 16, 20 - 8)); }
        static QSize createCallButtonSize() noexcept { return Utils::scale_value(QSize(124, 24 + 16)); }
        static QColor tittleColor() { return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID); }
        static QColor textColor() { return Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY_HOVER); }
        static QFont titleFont() { return Fonts::appFontScaled(15, Fonts::FontWeight::Medium); }
        static QFont textFont() { return Fonts::appFontScaled(15); }
        static QFont buttonTextFont() { return Fonts::appFontScaled(13); }
        static QColor buttonTextColor() { return Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT); }
        static QColor buttonBackgroundColor() { return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY); }
        static int textToButtonSpacing() noexcept { return Utils::scale_value(12 - 8); }
        static int tittleToTextSpacing() noexcept { return Utils::scale_value(8); }

    public:
        StartCallDialogWidget(QWidget* _parent)
            : QWidget(_parent)
            , tittle_(new QLabel(this))
        {
            setFixedSize(widgetSize());
            auto rootLayout = Utils::emptyVLayout(this);

            auto addListItemLabel = [this, rootLayout](const QString& _text)
            {
                auto label = new QLabel(qsl("\u2022  %1").arg(_text), this);
                label->setFont(textFont());
                setLabelTextColor(label, textColor());
                rootLayout->addWidget(label);
            };

            auto button = new RoundButton(this, Utils::scale_value(12));
            button->setText(QT_TRANSLATE_NOOP("history", "Create a call"));
            Testing::setAccessibleName(button, qsl("AS HistoryPage startCallButton"));
            button->setFont(buttonTextFont());
            button->setFixedSize(createCallButtonSize());
            button->setTextColor(buttonTextColor());
            button->setColors(buttonBackgroundColor());
            connect(button, &RoundButton::clicked, this, []()
            {
                Q_EMIT Utils::InterConnector::instance().createGroupCall();
            });

            tittle_->setText(QT_TRANSLATE_NOOP("history", "Advantages of calls in Myteam"));
            tittle_->setAlignment(Qt::AlignCenter);
            tittle_->setFont(titleFont());
            setLabelTextColor(tittle_, tittleColor());

            rootLayout->addWidget(tittle_);
            rootLayout->addSpacing(tittleToTextSpacing());
            addListItemLabel(QT_TRANSLATE_NOOP("history", "Group calls up to %1 participants").arg(maxCallParticipantsLabelNumber()));
            addListItemLabel(QT_TRANSLATE_NOOP("history", "No time limit"));
            addListItemLabel(QT_TRANSLATE_NOOP("history", "Return to call in chat in one click"));
            addListItemLabel(QT_TRANSLATE_NOOP("history", "Status is set automatically during call"));
            rootLayout->addSpacing(textToButtonSpacing());
            rootLayout->addWidget(button);
            rootLayout->setAlignment(button, Qt::AlignHCenter);
            rootLayout->setContentsMargins(widgetMargins());
        }

        void setTextColor(const QColor& _color)
        {
            setLabelTextColor(tittle_, _color);
        }
    };

    EmptyConnectionInfoPage::EmptyConnectionInfoPage(QWidget *_parent)
        : BackgroundWidget(_parent)
        , dialogWidget_(new QWidget(this))
        , bubblePlate_(new BubblePlateWidget(dialogWidget_))
        , connectionWidget_(new ConnectionWidget(bubblePlate_, getTextColor()))
        , selectDialogWidget_(new SelectDialogWidget(bubblePlate_))
        , startCallDialogWidget_(new StartCallDialogWidget(bubblePlate_))
    {
        Testing::setAccessibleName(dialogWidget_, qsl("AS HistoryPage dialogWidget"));
        Testing::setAccessibleName(bubblePlate_, qsl("AS HistoryPage bubblePlate"));
        Testing::setAccessibleName(selectDialogWidget_, qsl("AS HistoryPage selectDialogWidget"));
        Testing::setAccessibleName(startCallDialogWidget_, qsl("AS HistoryPage startCallDialogWidget"));

        connectionWidget_->setFixedHeight(getPlaceholderHeight());

        auto rootLayout = Utils::emptyVLayout(this);
        rootLayout->addWidget(dialogWidget_);

        auto dialogLayout = Utils::emptyVLayout(dialogWidget_);
        dialogLayout->addWidget(bubblePlate_);
        dialogLayout->setAlignment(bubblePlate_, Qt::AlignCenter);

        bubblePlate_->layout()->addWidget(selectDialogWidget_);
        bubblePlate_->layout()->addWidget(connectionWidget_);
        bubblePlate_->layout()->addWidget(startCallDialogWidget_);
        bubblePlate_->layout()->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);

        updateWallpaper({});
        connectionState_ = GetDispatcher()->getConnectionState();
        pageType_ = OnlineWidgetType::Chat;
        updateCentralWidget();

        connect(GetDispatcher(), &core_dispatcher::connectionStateChanged, this, &EmptyConnectionInfoPage::connectionStateChanged);
        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalWallpaperChanged, this, &EmptyConnectionInfoPage::onGlobalWallpaperChanged);
    }

    void EmptyConnectionInfoPage::setPageType(OnlineWidgetType _pageType)
    {
        if (pageType_ != _pageType)
        {
            pageType_ = _pageType;
            updateCentralWidget();
        }
    }

    void EmptyConnectionInfoPage::updateCentralWidget()
    {
        const auto isOnline = connectionState_ == ConnectionState::stateOnline;

        if (isOnline)
            bubblePlate_->restoreLeftContentsMargin();
        else
            bubblePlate_->setLeftContentsMargin(bubbleLeftMarginForWidgetWithSpinner());

        connectionWidget_->setVisible(!isOnline);
        selectDialogWidget_->setVisible(isOnline && pageType_ == OnlineWidgetType::Chat);
        startCallDialogWidget_->setVisible(isOnline && pageType_ == OnlineWidgetType::Calls);
    }

    void EmptyConnectionInfoPage::connectionStateChanged(const ConnectionState& _state)
    {
        if (_state != connectionState_)
        {
            connectionState_ = _state;
            updateCentralWidget();
        }
    }

    void EmptyConnectionInfoPage::onGlobalWallpaperChanged()
    {
        const auto clr = getTextColor();
        selectDialogWidget_->setTextColor(clr);
        connectionWidget_->setTextColor(clr);
        startCallDialogWidget_->setTextColor(clr);
    }

    HistoryControl::HistoryControl(QWidget* parent)
        : QWidget(parent)
        , IEscapeCancellable(this)
        , timer_(new QTimer(this))
        , emptyPage_(new EmptyConnectionInfoPage(this))
        , frameCountMode_(FrameCountMode::_1)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        emptyPage_->setPageType(EmptyConnectionInfoPage::OnlineWidgetType::Chat);

        auto layout = Utils::emptyVLayout(this);
        stackPages_ = new QStackedWidget(this);
        stackPages_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        Testing::setAccessibleName(emptyPage_, qsl("AS HistoryPage emptyPage"));
        stackPages_->addWidget(emptyPage_);

        Testing::setAccessibleName(stackPages_, qsl("AS HistoryPage stackPages"));
        layout->addWidget(stackPages_);

        updateEmptyPageWallpaper();

        connect(timer_, &QTimer::timeout, this, &HistoryControl::updatePages);
        timer_->setInterval(std::max(GetAppConfig().CacheHistoryControlPagesCheckInterval(), std::chrono::seconds(1)));
        timer_->setSingleShot(false);
        timer_->start();

        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalWallpaperChanged, this, &HistoryControl::onGlobalWallpaperChanged);
        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::contactWallpaperChanged, this, &HistoryControl::onContactWallpaperChanged);
        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::wallpaperImageAvailable, this, &HistoryControl::onWallpaperAvailable);

        connect(Logic::getContactListModel(), &Logic::ContactListModel::leave_dialog, this, &HistoryControl::leaveDialog);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, &HistoryControl::closeDialog);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::selectedContactChanged, this, &HistoryControl::onContactSelected);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::activeDialogHide, this, &HistoryControl::onActiveDialogHide);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::currentPageChanged, this, &HistoryControl::mainPageChanged);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::addPageToDialogHistory, this, &HistoryControl::addPageToDialogHistory);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::clearDialogHistory, this, &HistoryControl::clearDialogHistory);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::headerBack, this, &HistoryControl::onBackToChats);

        connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStatesHandled, this, &HistoryControl::updateUnreads);
        connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, &HistoryControl::updateUnreads);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::dlgStatesHandled, this, &HistoryControl::updateUnreads);
        connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updatedMessages, this, &HistoryControl::updateUnreads);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &HistoryControl::updateUnreads);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::suggestNotifyUser, this, &HistoryControl::suggestNotifyUser);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::openDialogGallery, this, &HistoryControl::openGallery);
    }

    HistoryControl::~HistoryControl() = default;

    void HistoryControl::cancelSelection()
    {
        for (const auto &page : std::as_const(pages_))
        {
            im_assert(page);
            page->cancelSelection();
        }
    }

    HistoryControlPage* HistoryControl::getHistoryPage(const QString& _aimId) const
    {
        return qobject_cast<HistoryControlPage*>(getPage(_aimId));
    }

    PageBase* HistoryControl::getPage(const QString& _aimId) const
    {
        auto it = pages_.find(_aimId);
        if (it != pages_.end())
            return *it;
        return nullptr;
    }

    bool HistoryControl::hasMessageUnderCursor() const
    {
        const auto page = getCurrentHistoryPage();
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
            auto page = getCurrentHistoryPage();
            if (page)
                page->resumeVisibleItems();
        }

        auto page = getCurrentHistoryPage();
        if (page)
            page->notifyApplicationWindowActive(isActive);
    }

    void HistoryControl::notifyUIActive(const bool _isActive)
    {
        auto page = getCurrentHistoryPage();
        if (page)
            page->notifyUIActive(_isActive);
    }

    void HistoryControl::scrollHistoryToBottom(const QString& _contact) const
    {
        auto page = getHistoryPage(_contact);
        if (page)
            page->scrollToBottom();
    }

    void HistoryControl::updatePages()
    {
        const auto currentTime = QTime::currentTime();
        std::chrono::seconds time = std::max(GetAppConfig().CacheHistoryControlPagesFor(), std::chrono::seconds(1));
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

    void HistoryControl::contactSelected(const QString& _aimId, qint64 _messageId, const highlightsV& _highlights, bool _ignoreScroll)
    {
        im_assert(!_aimId.isEmpty());

        const bool contactChanged = (_aimId != current_);

        const Data::DlgState dlgState = Logic::getRecentsModel()->getDlgState(_aimId);
        const qint64 lastReadMsg = dlgState.UnreadCount_ > 0 && dlgState.YoursLastRead_ < dlgState.LastMsgId_ ? dlgState.YoursLastRead_ : -1;

        auto oldPage = getCurrentPage();
        auto oldHistoryPage = getCurrentHistoryPage();


        auto page = getPage(_aimId);
        const auto createNewPage = (page == nullptr);

        const auto scrollMode = hist::evaluateScrollMode(_messageId, lastReadMsg, createNewPage, _ignoreScroll);

        if (_messageId == -1)
            _messageId = lastReadMsg;

        if (oldHistoryPage)
        {
            const bool canScrollToBottom = scrollMode == hist::scroll_mode_type::unread;
            if (_messageId == -1 || canScrollToBottom)
            {
                if (!contactChanged)
                {
                    oldHistoryPage->scrollToBottom();
                    Q_EMIT Utils::InterConnector::instance().setFocusOnInput(current_);
                    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_scr_read_all_event, { { "Type", "Recents_Double_Click" } });
                    return;
                }

                if (!canScrollToBottom)
                    oldHistoryPage->updateItems();
            }

            oldHistoryPage->resetShiftingParams();
        }

        if (contactChanged && oldPage)
            oldPage->pageLeave();

        if (createNewPage)
        {
            page = createPage(_aimId);

            pages_[_aimId] = page;
            stackPages_->addWidget(page);

            Logic::getContactListModel()->setCurrentCallbackHappened(page);
        }

        Q_EMIT Utils::InterConnector::instance().historyControlReady(_aimId, _messageId, dlgState, -1, createNewPage);

        if (!_highlights.empty())
            page->setHighlights(_highlights);
        else if (!oldPage || !GalleryPage::isGalleryAimId(oldPage->aimId()))
            page->resetMessageHighlights();

        if (scrollMode != hist::scroll_mode_type::none || createNewPage)
            page->initFor(_messageId, scrollMode, createNewPage ? HistoryControlPage::FirstInit::Yes : HistoryControlPage::FirstInit::No);

        if (createNewPage)
            statistic::getGuiMetrics().eventChatOpen(_aimId);

        const auto prevButtonVisible = !dialogHistory_.empty() && dialogHistory_.back() != _aimId;
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
        lastInitedParams_.aimId_ = _aimId;
        lastInitedParams_.messageId_ = _messageId;

        stackPages_->setCurrentWidget(page);

        page->pageOpen();
        page->updateItems();
        page->open();
        Q_EMIT setTopWidget(current_, page->getTopWidget(), QPrivateSignal());

        const auto& tc = Styling::getThemesContainer();
        const auto newWpId = tc.getContactWallpaperId(page->aimId());
        if (!oldPage || (oldPage && tc.getContactWallpaperId(oldPage->aimId()) != newWpId))
            updateWallpaper(_aimId);

        if (contactChanged && dlgState.Attention_)
            Logic::getRecentsModel()->setAttention(_aimId, false);

        Utils::InterConnector::instance().getMainWindow()->updateMainMenu();

        escCancel_->removeChild(oldPage);
        page->setCancelCallback([]() { Utils::InterConnector::instance().closeAndHighlightDialog(); });
        escCancel_->addChild(page);
    }

    void HistoryControl::leaveDialog(const QString& _aimId)
    {
        if (!GalleryPage::isGalleryAimId(_aimId))
            leaveDialog(GalleryPage::createGalleryAimId(_aimId));

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
        escCancel_->removeChild(getCurrentPage());

        rememberCurrentDialogTime();

        current_.clear();
        lastInitedParams_.clear();
        window()->setFocus();
        stackPages_->setCurrentWidget(emptyPage_);
        emptyPage_->updateGeometry();
        updateEmptyPageWallpaper();
        MainPage::instance()->onSwitchedToEmpty();
    }

    void HistoryControl::addPageToDialogHistory(const QString& _aimId)
    {
        if (_aimId.isEmpty())
            return;

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
        QString prev;
        if (!dialogHistory_.empty())
        {
            prev = std::move(dialogHistory_.back());
            dialogHistory_.pop_back();
        }

        if (prev.isEmpty() || prev != currentAimId())
        {
            qint64 msgId = -1;
            if (GalleryPage::isGalleryAimId(currentAimId()) && lastInitedParams_.aimId_ == prev)
                msgId = lastInitedParams_.messageId_;
            Utils::InterConnector::instance().openDialog(prev, msgId, true, {}, true);
        }
        else
        {
            Q_EMIT Utils::InterConnector::instance().closeLastDialog(_calledFromKeyboard);
            suspendBackgroundPages();
        }

        if (dialogHistory_.empty())
            Q_EMIT Utils::InterConnector::instance().noPagesInDialogHistory();
    }

    void HistoryControl::closeDialog(const QString& _aimId)
    {
        if (!GalleryPage::isGalleryAimId(_aimId))
            closeDialog(GalleryPage::createGalleryAimId(_aimId));

        const auto iter_page = pages_.find(_aimId);
        if (iter_page == pages_.end())
            return;

        const bool isCurrent = current_ == _aimId;
        if (isCurrent)
            switchToEmpty();

        auto page = iter_page.value();
        stackPages_->removeWidget(page);

        page->pageLeave();
        page->deleteLater();
        pages_.erase(iter_page);

        Q_EMIT Utils::InterConnector::instance().dialogClosed(_aimId, isCurrent);
    }

    void HistoryControl::mainPageChanged()
    {
        if (auto page = getCurrentHistoryPage())
        {
            if (Utils::InterConnector::instance().getMainWindow()->isMessengerPageContactDialog())
            {
                page->pageOpen();
            }
            else
            {
                page->pageLeave();
            }
        }
    }

    void HistoryControl::onContactSelected(const QString & _aimId)
    {
        if (auto page = getCurrentPage(); page && _aimId.isEmpty())
        {
            switchToEmpty();
            page->pageLeave();
        }
    }

    void HistoryControl::onBackToChats()
    {
        if (auto page = getCurrentHistoryPage())
            page->updateItems();
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
        if (const auto page = getCurrentHistoryPage(); page && page->aimId() == _aimId)
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

    void HistoryControl::onActiveDialogHide(const QString& _aimId, Ui::ClosePage _closePage)
    {
        if (_closePage == ClosePage::Yes)
            closeDialog(_aimId);
    }

    HistoryControlPage* HistoryControl::getCurrentHistoryPage() const
    {
        im_assert(stackPages_);
        return qobject_cast<HistoryControlPage*>(stackPages_->currentWidget());
    }

    PageBase* HistoryControl::getCurrentPage() const
    {
        im_assert(stackPages_);
        return qobject_cast<PageBase*>(stackPages_->currentWidget());
    }

    void HistoryControl::updateWallpaper(const QString& _aimId) const
    {
        Q_EMIT needUpdateWallpaper(_aimId, QPrivateSignal());
    }

    void HistoryControl::updateEmptyPageWallpaper() const
    {
        updateWallpaper({});
    }

    void HistoryControl::rememberCurrentDialogTime()
    {
        if (!current_.isEmpty())
            times_[current_] = QTime::currentTime();
    }

    void HistoryControl::suggestNotifyUser(const QString& _contact, const QString& _smsContext)
    {
        if (_contact != currentAimId() || !Features::isInviteBySmsEnabled())
            return;

        const auto friendlyName = Logic::GetFriendlyContainer()->getFriendly(_contact);
        const auto isConfirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "No"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            QT_TRANSLATE_NOOP("popup_window", "%1 hasn't been online for a long time and won't read the message likely. Send free sms to suggest to come back to ICQ?")
                .arg(friendlyName),
            QString(),
            nullptr
        );

        const auto sendNotifySms = [&_smsContext]()
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("smsNotifyContext", _smsContext);
            Ui::GetDispatcher()->post_message_to_core("send_notify_sms", collection.get());
        };

        const auto sendStatistics = [](bool _confirm)
        {
            Ui::GetDispatcher()->post_stats_to_core(
                core::stats::stats_event_names::chatscr_invite_by_sms,
                {
                    { "type", _confirm ? "yes" : "no" },
                });
        };

        sendStatistics(isConfirmed);
        if (isConfirmed)
        {
            sendNotifySms();
            Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("chat_page", "Message sent"));
        }
    }

    void HistoryControl::suspendBackgroundPages()
    {
        for (const auto& p : std::as_const(pages_))
        {
            if (current_.isEmpty() || p->aimId() != current_)
                p->suspendVisisbleItems();
        }
    }

    void HistoryControl::openGallery(const QString& _contact)
    {
        const auto galleryAimId = GalleryPage::createGalleryAimId(_contact);
        auto page = getPage(galleryAimId);
        if (!page)
        {
            page = new GalleryPage(_contact);
            connect(page, &PageBase::switchToPrevDialog, this, &HistoryControl::switchToPrevDialogPage);

            pages_[galleryAimId] = page;
            stackPages_->addWidget(page);
        }

        if (auto oldPage = getCurrentHistoryPage())
        {
            addPageToDialogHistory(oldPage->aimId());
            oldPage->pageLeave();
        }

        stackPages_->setCurrentWidget(page);

        page->pageOpen();

        escCancel_->removeChild(getCurrentPage());
        page->setCancelCallback([this]() { switchToPrevDialogPage(false); });
        escCancel_->addChild(page);

        rememberCurrentDialogTime();
        current_ = galleryAimId;

        suspendBackgroundPages();

        Q_EMIT setTopWidget(current_, page->getTopWidget(), QPrivateSignal());
        Utils::InterConnector::instance().setSidebarVisible(false);
        window()->setFocus();
    }

    PageBase* HistoryControl::createPage(const QString& _aimId)
    {
        PageBase* page;
        if (_aimId == qsl("~threads~"))
        {
            page = new FeedPage(_aimId, this);
        }
        else
        {
            auto historyPage = new HistoryControlPage(_aimId, this, Utils::InterConnector::instance().getContactDialog());
            connect(this, &HistoryControl::needUpdateWallpaper, historyPage, &HistoryControlPage::onWallpaperChanged);
            Testing::setAccessibleName(historyPage, qsl("AS HistoryPage ") % _aimId);
            page = historyPage;
        }

        connect(page, &PageBase::switchToPrevDialog, this, &HistoryControl::switchToPrevDialogPage);

        return page;
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
                const auto hasDialogHistory = !dialogHistory_.empty() && dialogHistory_.back() != page->aimId();
                page->setPrevChatButtonVisible(hasDialogHistory || (frameCountMode_ == FrameCountMode::_1));
                page->setOverlayTopWidgetVisible(frameCountMode_ == FrameCountMode::_1);
            }
        }
    }
}
