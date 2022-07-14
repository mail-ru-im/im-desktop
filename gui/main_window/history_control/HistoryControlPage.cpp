#include "stdafx.h"
#include "HistoryControlPage.h"

#include "../common.shared/config/config.h"
#include "../common.shared/string_utils.h"

#include "MessageStyle.h"
#include "MessagesScrollArea.h"
#include "MessagesScrollAreaLayout.h"
#include "ServiceMessageItem.h"
#include "ChatEventItem.h"
#include "ChatEventInfo.h"
#include "VoipEventItem.h"
#include "MentionCompleter.h"
#include "HistoryButtonDown.h"
#include "PinnedMessageWidget.h"
#include "CloseChatWidget.h"
#include "TypingWidget.h"
#include "ChatPlaceholder.h"
#include "complex_message/ComplexMessageItem.h"
#include "complex_message/FileSharingUtils.h"
#include "top_widget/SelectionPanel.h"
#include "top_widget/DialogHeaderPanel.h"
#include "top_widget/ThreadHeaderPanel.h"
#include "top_widget/TopWidget.h"
#include "ActiveCallPlate.h"
#include "FileStatus.h"
#include "../ContactDialog.h"
#include "../GroupChatOperations.h"
#include "../MainPage.h"
#include "../ReportWidget.h"
#include "../MainWindow.h"
#include "../contact_list/RecentsTab.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/RecentsModel.h"
#include "../contact_list/FavoritesUtils.h"
#include "../containers/FriendlyContainer.h"
#include "../containers/InputStateContainer.h"
#include "../containers/ThreadSubContainer.h"
#include "../sidebar/Sidebar.h"
#include "../sidebar/SidebarUtils.h"
#include "../input_widget/InputWidget.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../contact_list/IgnoreMembersModel.h"

#include "../../controls/ContextMenu.h"
#include "../../controls/TooltipWidget.h"
#include "../../controls/GeneralDialog.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/MimeDataUtils.h"
#include "../../utils/log/log.h"
#include "../../utils/stat_utils.h"
#include "../../utils/features.h"
#include "../../styles/ThemeParameters.h"
#include "../../styles/ThemesContainer.h"
#include "../../app_config.h"

#include "history/Heads.h"
#include "history/MessageReader.h"
#include "history/MessageBuilder.h"
#include "history/DateInserter.h"

#include "main_window/smartreply/SmartReplyWidget.h"
#include "main_window/smartreply/SmartReplyItem.h"
#include "main_window/smartreply/ShowHideButton.h"
#include "main_window/smartreply/SmartReplyForQuote.h"
#include "main_window/DragOverlayWindow.h"
#include "main_window/smiles_menu/SmilesMenu.h"
#include "main_window/smiles_menu/SuggestsWidget.h"

#include "previewer/toast.h"

#include "controls/textrendering/TextRenderingUtils.h"

#include <boost/range/adaptor/reversed.hpp>
#include <main_window/containers/LastseenContainer.h>

#ifdef __APPLE__
#include "../../utils/macos/mac_support.h"
#endif

Q_LOGGING_CATEGORY(historyPage, "historyPage")

namespace
{
    constexpr auto buttonDownShift = 20;
    constexpr int scrollByKeyDelta = 20;
    constexpr auto chatInfoMembersLimit = 5;

    constexpr auto useScrollAreaContainer() noexcept { return platform::is_apple(); }

    bool isRemovableWidget(QWidget* _w)
    {
        return true;
    }

    bool isUpdateableWidget(QWidget* _w)
    {
        if (const auto mi = qobject_cast<Ui::HistoryControlPageItem*>(_w))
            return mi->isUpdateable();

        return true;
    }

    QMap<QString, QVariant> makeData(const QString& _command, const QString& _aimId)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = _command;
        result[qsl("aimid")] = _aimId;
        return result;
    }

    constexpr std::chrono::milliseconds typingInterval = std::chrono::seconds(3);
    constexpr std::chrono::milliseconds lookingInterval = std::chrono::seconds(10);
    constexpr std::chrono::milliseconds placeholderAvatarTimeout = std::chrono::milliseconds(500);

    constexpr std::chrono::milliseconds getLoaderOverlayDelay() noexcept { return std::chrono::milliseconds(200); }
    constexpr std::chrono::milliseconds overlayUpdateTimeout() noexcept { return std::chrono::milliseconds(500); }

    void sendSmartreplyStats(const Data::SmartreplySuggest& _suggest)
    {
        using namespace core::stats;

        im_stat_event_names imStatName = im_stat_event_names::min;
        stats_event_names statName = stats_event_names::min;
        event_props_type props;

        if (_suggest.isStickerType())
        {
            if (auto fileSharingId = std::get_if<Utils::FileSharingId>(&_suggest.getData()))
            {
                if (fileSharingId->sourceId_)
                    return;
                props.push_back({ "stickerId", fileSharingId->fileId_.toStdString() });
            }
            imStatName = im_stat_event_names::stickers_smartreply_sticker_sent;
            statName = stats_event_names::stickers_smartreply_sticker_sent;
        }
        else if (_suggest.isTextType())
        {
            imStatName = im_stat_event_names::smartreply_text_sent;
            statName = stats_event_names::smartreply_text_sent;
        }

        if (imStatName != im_stat_event_names::min)
        {
            Ui::GetDispatcher()->post_im_stats_to_core(imStatName, props);
            Ui::GetDispatcher()->post_stats_to_core(statName, props);
        }
    }

    constexpr Ui::PlaceholderState joinEventToPlaceholderState(core::chat_event_type _type) noexcept
    {
        switch (_type)
        {
        case core::chat_event_type::mchat_waiting_for_approve:
            return Ui::PlaceholderState::pending;
        case core::chat_event_type::mchat_joining_rejected:
            return Ui::PlaceholderState::rejected;
        case core::chat_event_type::mchat_joining_canceled:
            return Ui::PlaceholderState::canceled;

        default:
            return Ui::PlaceholderState::hidden;
        }
    }
}

namespace Ui
{
    enum class HistoryControlPage::WidgetRemovalResult
    {
        Min,

        Removed,
        NotFound,
        PersistentWidget,

        Max
    };

    MessagesWidgetEventFilter::MessagesWidgetEventFilter(
        MessagesScrollArea* _scrollArea,
        ServiceMessageItem* _overlay,
        HistoryControlPage* _dialog
    )
        : QObject(_dialog)
        , ScrollArea_(_scrollArea)
        , NewPlateShowed_(false)
        , ScrollDirectionDown_(false)
        , Dialog_(_dialog)
        , Overlay_(_overlay)
    {
        im_assert(ScrollArea_);
    }

    void MessagesWidgetEventFilter::resetNewPlate()
    {
        NewPlateShowed_ = false;
    }

    bool MessagesWidgetEventFilter::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_event->type() == QEvent::Paint)
        {
            QDate date;
            auto newFound = false;
            auto dateVisible = true;
            qint64 firstVisibleId = -1;
            qint64 lastVisibleId = -1;
            qint64 currentPrevId = -1;
            qint64 prevItemId = -1;

            const auto debugRectAvailable = GetAppConfig().IsShowMsgIdsEnabled();

            ScrollArea_->enumerateWidgets(
                [this, &firstVisibleId, &lastVisibleId, &currentPrevId, &newFound,
                &date, &dateVisible, debugRectAvailable, &prevItemId]
            (QWidget *widget, const bool isVisible)
            {
                auto checkHole = [&currentPrevId, &prevItemId, dialog = Dialog_](auto id, auto prevId)
                {
                    bool result = currentPrevId > 0 && id > 0 && currentPrevId != id;
                    if (result && dialog->history_)
                        result = dialog->history_->isVisibleHoleBetween(id, prevItemId);

                    currentPrevId = prevId;
                    prevItemId = id;
                    return result;
                };

                auto processHole = [debugRectAvailable, dialog = Dialog_](auto widget)
                {
                    if (debugRectAvailable)
                        dialog->drawDebugRectForHole(widget);
                };

                if (!isVisible || widget->visibleRegion().isEmpty())
                    return true;

                if (auto complexMsgItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(widget))
                {
                    const auto id = complexMsgItem->getId();
                    const auto prevId = complexMsgItem->getPrevId();

                    if (debugRectAvailable && checkHole(id, prevId))
                        processHole(widget);

                    if (firstVisibleId == -1)
                    {
                        date = complexMsgItem->getDate();
                        firstVisibleId = id;
                    }

                    lastVisibleId = id;

                    return true;
                }

                if (auto chatEventItem = qobject_cast<Ui::ChatEventItem*>(widget))
                {
                    const auto id = chatEventItem->getId();
                    const auto prevId = chatEventItem->getPrevId();

                    if (debugRectAvailable && checkHole(id, prevId))
                        processHole(widget);

                    return true;
                }

                if (auto voipItem = qobject_cast<Ui::VoipEventItem*>(widget))
                {
                    const auto id = voipItem->getId();
                    const auto prevId = voipItem->getPrevId();

                    if (debugRectAvailable && checkHole(id, prevId))
                        processHole(widget);

                    return true;
                }

                if (auto serviceItem = qobject_cast<Ui::ServiceMessageItem*>(widget))
                {
                    if (serviceItem->isNew())
                    {
                        newFound = true;
                        NewPlateShowed_ = true;
                    }
                    else
                    {
                        dateVisible = false;
                    }
                }

                return true;
            },
                false
                );

            if (!newFound && NewPlateShowed_)
            {
                Dialog_->newPlateShowed();
                NewPlateShowed_ = false;
            }

            bool visible = date.isValid();

            if (date != Date_)
            {
                Date_ = date;
                Overlay_->setDate(Date_);
                Overlay_->adjustSize();
            }

            const auto isOverlayVisible = (dateVisible && visible);
            Overlay_->setAttribute(Qt::WA_WState_Hidden, !isOverlayVisible);
            Overlay_->setAttribute(Qt::WA_WState_Visible, isOverlayVisible);
        }
        else if (_event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(_event);
            if (keyEvent->matches(QKeySequence::Copy))
            {
                if (Utils::InterConnector::instance().isSidebarVisible() && !Dialog_->inputWidget_->hasSelection())
                {
                    const auto text = Utils::InterConnector::instance().getSidebarSelectedText();
                    if (!text.isEmpty())
                    {
                        QApplication::clipboard()->setText(text);
                        Utils::showCopiedToast();
                    }
                    else
                    {
                        Dialog_->copy();
                    }
                }
                else
                {
                    Dialog_->copy();
                }

                if (!Utils::InterConnector::instance().isMultiselect(Dialog_->aimId()))
                    Dialog_->setFocusOnInputWidget();
                return true;
            }
        }

        return QObject::eventFilter(_obj, _event);
    }

    enum class HistoryControlPage::State
    {
        Min,

        Idle,
        Fetching,
        Inserting,

        Max
    };

    QTextStream& operator<<(QTextStream& _oss, const HistoryControlPage::State _arg)
    {
        switch (_arg)
        {
        case HistoryControlPage::State::Idle: _oss << ql1s("IDLE"); break;
        case HistoryControlPage::State::Fetching: _oss << ql1s("FETCHING"); break;
        case HistoryControlPage::State::Inserting: _oss << ql1s("INSERTING"); break;

        default:
            im_assert(!"unexpected state value");
            break;
        }

        return _oss;
    }

    HistoryControlPage::HistoryControlPage(const QString& _aimId, QWidget* _parent, BackgroundWidget* _bgWidget)
        : PageBase(_parent)
        , wallpaperId_(-1)
        , themeChecker_(_aimId)
        , typingWidget_(new TypingWidget(this, _aimId))
        , aimId_(_aimId)
        , messagesOverlay_(new ServiceMessageItem(this, true))
        , state_(State::Idle)
        , mentionCompleter_(new MentionCompleter(this))
        , pinnedWidget_(new PinnedMessageWidget(this))
        , typedTimer_(new QTimer(this))
        , lookingTimer_(new QTimer(this))
        , mentionTimer_(nullptr)
        , lookingId_(QUuid::createUuid().toString(QUuid::WithoutBraces))
        , heads_(new Heads::HeadContainer(_aimId, this))
        , reader_(new hist::MessageReader(_aimId, this))
        , history_(new hist::History(_aimId, this))
        , activeCallPlate_(new ActiveCallPlate(_aimId, this))
    {
        auto layout = Utils::emptyVLayout(this);
        layout->addWidget(pinnedWidget_);
        layout->addWidget(activeCallPlate_);

        messagesArea_ = new MessagesScrollArea(this, typingWidget_, new hist::DateInserter(_aimId, this), _aimId);
        Testing::setAccessibleName(messagesArea_, qsl("AS HistoryPage messagesArea"));
        messagesArea_->getLayout()->setHeadContainer(heads_);
        messagesArea_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        messagesArea_->setFocusPolicy(Qt::ClickFocus);
        messagesArea_->installEventFilter(this);

        if constexpr (useScrollAreaContainer())
        {
            auto scrollArea = new ScrollAreaContainer(this);
            connect(scrollArea, &ScrollAreaContainer::scrollViewport, messagesArea_, &MessagesScrollArea::scrollContent);

            auto areaLayout = Utils::emptyHLayout(scrollArea->viewport());
            areaLayout->addWidget(messagesArea_);
            layout->addWidget(scrollArea);
        }
        else
        {
            layout->addWidget(messagesArea_);
        }

        initButtonDown();
        initMentionsButton();
        initTopWidget();

        inputWidget_ = new InputWidget(_aimId,  this, defaultInputFeatures(), _bgWidget);
        if (const auto parentChat = Logic::getContactListModel()->getThreadParent(aimId_))
        {
            inputWidget_->setThreadParentChat(*parentChat);
            Ui::GetDispatcher()->getChatInfo(*parentChat, Logic::getContactListModel()->getChatStamp(*parentChat), 0);
        }

        inputWidget_->setHistoryControlPage(this);
        inputWidget_->setMentionCompleter(mentionCompleter_);
        Testing::setAccessibleName(inputWidget_, qsl("AS ChatInput"));
        layout->addWidget(inputWidget_);

        const auto mainWindow = Utils::InterConnector::instance().getMainWindow();
        connect(inputWidget_, &InputWidget::startPttRecording, mainWindow, [mainWindow, this]()
        {
            mainWindow->setLastPttInput(inputWidget_);
        });
        connect(inputWidget_, &InputWidget::destroyed, mainWindow, [mainWindow]() {mainWindow->setLastPttInput(nullptr);});

        connect(this, &HistoryControlPage::quote, inputWidget_, &InputWidget::quote);
        connect(this, &HistoryControlPage::createTask, inputWidget_, &InputWidget::createTask);
        connect(this, &HistoryControlPage::messageIdsFetched, inputWidget_, &InputWidget::messageIdsFetched);
        connect(inputWidget_, &InputWidget::inputTyped, this, &HistoryControlPage::inputTyped);
        connect(inputWidget_, &InputWidget::needSuggets, this, [this](const QString& _text, const QPoint& _pos)
                {
                    needShowSuggests_ = true;
                    onSuggestShow(_text, _pos);
                });
        connect(inputWidget_, &InputWidget::hideSuggets, this, [this]()
                {
                    needShowSuggests_ = false;
                    onSuggestHide();
                });
        connect(inputWidget_, &InputWidget::sendMessage, this, [this]()
                {
                    scrollToBottom();
                    hideSuggest();
                    needShowSuggests_ = false;
                });
        connect(inputWidget_, &InputWidget::viewChanged, this, [this]()
                {
                    if (suggestWidgetShown_)
                        return;
                    hideSuggest();
                });
        connect(inputWidget_, &InputWidget::smilesMenuSignal, this, [this](bool _fromKeyboard)
        {
            showHideStickerPicker(_fromKeyboard ? ShowHideInput::Keyboard : ShowHideInput::Mouse);
        }, Qt::QueuedConnection);
        connect(inputWidget_, &InputWidget::quotesCountChanged, this, [this](int _count)
        {
            const auto hasQuotes = _count > 0;
            setSmartrepliesSemitransparent(hasQuotes);
            if (!hasQuotes)
                hideSmartrepliesForQuoteAnimated();
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectChanged, inputWidget_, &InputWidget::onMultiselectChanged);

        connect(GetDispatcher(), &core_dispatcher::smartreplyRequestedResult, this, &HistoryControlPage::onSmartrepliesForQuote);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::currentPageChanged, this, &HistoryControlPage::hideSmartrepliesForQuoteForce);

        Testing::setAccessibleName(pinnedWidget_, qsl("AS HistoryPage pinnedWidget"));
        pinnedWidget_->hide();

        messagesOverlay_->setContact(_aimId);
        messagesOverlay_->raise();
        messagesOverlay_->setAttribute(Qt::WA_TransparentForMouseEvents);

        eventFilter_ = new MessagesWidgetEventFilter(messagesArea_, messagesOverlay_, this);
        installEventFilter(eventFilter_);

        typingWidget_->setParent(messagesArea_);
        typingWidget_->hide();

        activeCallPlate_->hide();

        connect(history_, &hist::History::readyInit, this, &HistoryControlPage::sourceReadyHist);
        connect(history_, &hist::History::updatedBuddies, this, &HistoryControlPage::updatedBuddies);
        connect(history_, &hist::History::insertedBuddies, this, &HistoryControlPage::insertedBuddies);
        connect(history_, &hist::History::dlgStateBuddies, this, &HistoryControlPage::insertedBuddies);
        connect(history_, &hist::History::canFetch, this, &HistoryControlPage::canFetchMore);
        connect(history_, &hist::History::deletedBuddies, this, &HistoryControlPage::deleted);
        connect(history_, &hist::History::pendingResolves, this, &HistoryControlPage::pendingResolves);
        connect(history_, &hist::History::newMessagesReceived, this, &HistoryControlPage::onNewMessagesReceived);
        connect(history_, &hist::History::clearAll, this, &HistoryControlPage::clearAll);
        connect(history_, &hist::History::emptyHistory, this, &HistoryControlPage::emptyHistory);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::searchClosed, this, [this]()
        {
            if (isPageOpen())
                resetMessageHighlights();
        });
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::chatEvents, this, &HistoryControlPage::updateChatInfo);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::chatFontParamsChanged, this, &HistoryControlPage::onFontParamsChanged);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::loaderOverlayCancelled, this, [this]()
        {
            requestWithLoaderSequence_ = -1;
        });

        connect(get_gui_settings(), &qt_gui_settings::changed, this, [this](const QString& _key)
        {
            if (_key == ql1s(settings_show_smartreply))
            {
                if (get_gui_settings()->get_value<bool>(settings_show_smartreply, settings_show_smartreply_default()))
                    showSmartreplies();
                else
                    hideSmartreplies();
            }
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::sendBotCommand, this, &HistoryControlPage::sendBotCommand);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::startBot, this, &HistoryControlPage::startBot);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectChanged, this, &HistoryControlPage::multiselectChanged);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectDelete, this, &HistoryControlPage::multiselectDelete);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectFavorites, this, &HistoryControlPage::multiselectFavorites);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectCopy, this, &HistoryControlPage::multiselectCopy);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectReply, this, &HistoryControlPage::multiselectReply);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectForward, this, &HistoryControlPage::multiselectForward);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::openThread, this, &HistoryControlPage::openThread);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::startPttRecord, this, &HistoryControlPage::startPttRecording);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setFocusOnInput, this, [this](const QString& _contact)
        {
            if (isPageOpen() && (aimId_ == _contact || (_contact.isEmpty() && aimId_ == Logic::getContactListModel()->selectedContact())))
                setFocusOnInputWidget();
        });

        connect(history_, &hist::History::updateLastSeen, this, &HistoryControlPage::changeDlgState);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dlgStates, this, [this](const QVector<Data::DlgState>& _states)
        {
            for (const auto& state : _states)
            {
                if (state.AimId_ == aimId_)
                    changeDlgState(state);
            }
        });

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::pendingListResult, this, &HistoryControlPage::loadChatInfo);

        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &HistoryControlPage::contactChanged);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::yourRoleChanged, this, &HistoryControlPage::onChatRoleChanged);

        // QueuedConnection gives time to event loop to finish all message-related operations
        connect(this, &HistoryControlPage::postponeFetch, this, &HistoryControlPage::canFetchMore, Qt::QueuedConnection);

        connect(messagesArea_, &MessagesScrollArea::updateHistoryPosition, this, &HistoryControlPage::onUpdateHistoryPosition);
        connect(messagesArea_, &MessagesScrollArea::fetchRequestedEvent, this, &HistoryControlPage::onReachedFetchingDistance);
        connect(messagesArea_, &MessagesScrollArea::scrollMovedToBottom, this, &HistoryControlPage::scrollMovedToBottom);
        connect(messagesArea_, &MessagesScrollArea::itemRead, this, [this](qint64 _id, bool _visible)
        {
            if (isPageOpen())
                reader_->onMessageItemRead(_id, _visible);
        });

        if (QApplication::clipboard()->supportsSelection())
        {
            connect(messagesArea_, &MessagesScrollArea::messagesSelected, this, [this]()
            {
                QApplication::clipboard()->setText(messagesArea_->getSelectedText().string(), QClipboard::Selection);
            });
        }

        connect(this, &HistoryControlPage::needRemove, this, &HistoryControlPage::removeWidget);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfo, this, &HistoryControlPage::chatInfo);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatInfoFailed, this, &HistoryControlPage::chatInfoFailed);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::chatMemberInfo, this, &HistoryControlPage::onChatMemberInfo);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::smartreplyInstantSuggests, this, &HistoryControlPage::onSmartreplies);

        connect(heads_, &Heads::HeadContainer::headChanged, this, &HistoryControlPage::chatHeads);
        connect(heads_, &Heads::HeadContainer::hide, this, &HistoryControlPage::hideHeads);

        typedTimer_->setInterval(typingInterval);
        connect(typedTimer_, &QTimer::timeout, this, &HistoryControlPage::onTypingTimer);
        lookingTimer_->setInterval(lookingInterval);
        connect(lookingTimer_, &QTimer::timeout, this, &HistoryControlPage::onLookingTimer);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideMentionCompleter, this, &HistoryControlPage::hideMentionCompleter);

        connect(mentionCompleter_, &MentionCompleter::visibilityChanged, this, &HistoryControlPage::onMentionCompleterVisibilityChanged);
        connect(inputWidget_, &InputWidget::editFocusOut, this, &HistoryControlPage::hideMentionCompleter);
        mentionCompleter_->setFixedWidth(std::min(inputWidget_->width(), Tooltip::getMaxMentionTooltipWidth()));
        mentionCompleter_->setDialogAimId(aimId_);
        mentionCompleter_->hideAnimated();

        if (Utils::isChat(aimId_))
        {
            connect(Logic::getContactListModel(), &Logic::ContactListModel::yourRoleChanged, this, &HistoryControlPage::onRoleChanged);
            connectToMessageBuddies();

            if (!isThread())
                GetDispatcher()->subscribeCallRoomInfo(aimId_);
        }

        connect(GetDispatcher(), &core_dispatcher::messageThreadAdded, this, &HistoryControlPage::onThreadAdded);
        connect(GetDispatcher(), &Ui::core_dispatcher::subscribeResult, this, &HistoryControlPage::groupSubscribeResult);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::addReactionPlateActivityChanged, this, &HistoryControlPage::onAddReactionPlateActivityChanged);
        connect(GetDispatcher(), &core_dispatcher::callRoomInfo, this, &HistoryControlPage::onCallRoomInfo);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::externalUrlConfigUpdated, this, &HistoryControlPage::showStatusBannerIfNeeded);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &HistoryControlPage::showStatusBannerIfNeeded);
        connect(Logic::GetStatusContainer(), &Logic::StatusContainer::statusChanged, this, [this](const QString& _aimid)
        {
            if (_aimid == aimId_)
                showStatusBannerIfNeeded();
        });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::messageSent, this, [this](const QString& _aimid)
        {
            if (_aimid == aimId_)
                scrollToBottom();
        });

        connect(Utils::InterConnector::instance().getMainWindow(), &MainWindow::mouseReleased, this, [this]()
        {
            if (isPageOpen() && !rect().contains(mapFromGlobal(QCursor::pos())) && !isPttRecordingHold())
                stopPttRecording();
        });

        connect(Utils::InterConnector::instance().getMainWindow(), &MainWindow::mousePressed, this, [this]()
        {
            if (!isPageOpen())
                return;

            if (isPttRecordingHoldByKeyboard())
            {
                stopPttRecording();
            }
            else if (const auto buttons = qApp->mouseButtons(); buttons != Qt::MouseButtons(Qt::LeftButton))
            {
                if (!hasMessageUnderCursor())
                    stopPttRecording();
            }
        });

        escCancel_->addChild(inputWidget_);

        setAcceptDrops(true);
        Utils::InterConnector::instance().addPage(this);
    }

    void HistoryControlPage::initButtonDown()
    {
        if (buttonDown_)
            return;

        buttonDown_ = new HistoryButton(this, qsl(":/history/to_bottom"));
        Testing::setAccessibleName(buttonDown_, qsl("AS HistoryPage buttonDown"));
        buttonDown_->move(-HistoryButton::buttonSize, -HistoryButton::buttonSize);
        buttonDown_->hide();

        connect(buttonDown_, &HistoryButton::clicked, messagesArea_, &MessagesScrollArea::buttonDownClicked);
        connect(buttonDown_, &HistoryButton::clicked, this, &HistoryControlPage::scrollToBottomByButton);
        connect(buttonDown_, &HistoryButton::sendWheelEvent, messagesArea_, &MessagesScrollArea::onWheelEvent);

        buttonDownTimer_ = new QTimer(this);
        buttonDownTimer_->setInterval(16);
        connect(buttonDownTimer_, &QTimer::timeout, this, &HistoryControlPage::onButtonDownMove);
    }

    void HistoryControlPage::initMentionsButton()
    {
        if (buttonMentions_)
            return;

        buttonMentions_ = new HistoryButton(this, qsl(":/history/mention"));
        buttonMentions_->move(-HistoryButton::buttonSize, -HistoryButton::buttonSize);
        buttonMentions_->hide();

        connect(buttonMentions_, &HistoryButton::clicked, this, &HistoryControlPage::onButtonMentionsClicked);
        connect(buttonMentions_, &HistoryButton::sendWheelEvent, messagesArea_, &MessagesScrollArea::onWheelEvent);

        connect(GetDispatcher(), &core_dispatcher::mentionMe, this, &HistoryControlPage::mentionMe);
        connect(reader_, &hist::MessageReader::mentionRead, this, &HistoryControlPage::onMentionRead);
    }

    void HistoryControlPage::initTopWidget()
    {
        QWidget* mainPanel;

        if (!isThread())
        {
            header_ = new DialogHeaderPanel(aimId(), this);
            connect(header_, &DialogHeaderPanel::switchToPrevDialog, this, &HistoryControlPage::switchToPrevDialog);
            mainPanel = header_;
        }
        else
        {
            mainPanel = new ThreadHeaderPanel(aimId_, this);
        }

        topWidget_ = new TopWidget(this);
        topWidget_->insertWidget(TopWidget::Main, mainPanel);
        topWidget_->insertWidget(TopWidget::Selection, new SelectionPanel(aimId_, this));
        topWidget_->setCurrentIndex(TopWidget::Main);
    }

    void HistoryControlPage::onMentionRead(const qint64 _messageId)
    {
        updateMentionsButton();
    }

    void HistoryControlPage::onNeedUpdateRecentsText()
    {
        const auto item = qobject_cast<Ui::HistoryControlPageItem*>(sender());
        if (!item || item->getId() == -1 || item->getId() < Logic::getRecentsModel()->getLastMsgId(item->getContact()))
            return;

        Logic::Recents::FriendlyItemText frText{ item->getId() , item->formatRecentsText() };

        Logic::getRecentsModel()->setItemFriendlyText(item->getContact(), frText);
    }

    void HistoryControlPage::hideHeads()
    {
        if (!Utils::isChat(aimId_))
            return;

        messagesArea_->enumerateWidgets([](QWidget* _item, const bool)
        {
            auto pageItem = qobject_cast<Ui::HistoryControlPageItem*>(_item);
            if (!pageItem)
            {
                return true;
            }

            pageItem->setHeads({});
            return true;
        }, false);
    }

    void HistoryControlPage::onButtonMentionsClicked()
    {
        if (const auto nextMention = reader_->getNextUnreadMention())
        {
            const auto key = nextMention->ToKey();
            if (messagesArea_->getItemByKey(key) != nullptr)
            {
                scrollTo(key, hist::scroll_mode_type::search);
            }
            else if (Logic::getContactListModel()->isThread(aimId_))
            {
                history_->scrollToBottom();

                if (messagesArea_->getItemByKey(key) != nullptr)
                    scrollTo(key, hist::scroll_mode_type::search);
            }
            else
            {
                Q_EMIT Logic::getContactListModel()->select(aimId_, nextMention->Id_);
            }
        }
        else if (buttonMentions_)
        {
            buttonMentions_->resetCounter();
            buttonMentions_->hide();
        }
    }

    void HistoryControlPage::updateMentionsButton()
    {
        const int32_t mentionsCount = reader_->getMentionsCount();

        if (mentionsCount)
            initMentionsButton();

        if (buttonMentions_)
        {
            buttonMentions_->setCounter(mentionsCount);
            buttonMentions_->setVisible(!!mentionsCount);
        }
    }

    void HistoryControlPage::updateMentionsButtonDelayed()
    {
        if (!mentionTimer_)
        {
            mentionTimer_ = new QTimer(this);
            mentionTimer_->setSingleShot(true);
            connect(mentionTimer_, &QTimer::timeout, this, &HistoryControlPage::updateMentionsButton);
        }
        mentionTimer_->start(500);
    }

    void HistoryControlPage::updateOverlaySizes()
    {
        const auto areaRect = getScrollAreaGeometry();
        messagesOverlay_->setFixedWidth(width());
        messagesOverlay_->move(areaRect.topLeft());
    }

    void HistoryControlPage::mentionMe(const QString& _contact, Data::MessageBuddySptr _mention)
    {
        if (aimId_ != _contact)
            return;

        updateMentionsButtonDelayed();
    }

    void HistoryControlPage::updateWidgetsTheme()
    {
        if (themeChecker_.checkAndUpdateHash())
            updateWidgetsThemeImpl();
    }

    void HistoryControlPage::updateTopPanelStyle()
    {
        if (topWidget_)
            topWidget_->updateStyle();

        initStatus();
    }

    void HistoryControlPage::updateSmartreplyStyle()
    {
        if (!smartreplyWidget_ && !smartreplyButton_)
            return;

        const auto p = Styling::getParameters(aimId());
        const auto srwBg = p.isChatWallpaperPlainColor() ? p.getChatWallpaperPlainColor() : QColor();

        if (smartreplyWidget_)
            smartreplyWidget_->setBackgroundColor(srwBg);
        if (smartreplyButton_)
            smartreplyButton_->updateBackgroundColor(srwBg);
    }

    HistoryControlPage::~HistoryControlPage()
    {
        mentionCompleter_->deleteLater();
        mentionCompleter_ = nullptr;

        GetDispatcher()->unsubscribeCallRoomInfo(aimId_);
        GetDispatcher()->setDialogOpen(aimId_, false);
        Utils::InterConnector::instance().removePage(this);
    }

    void HistoryControlPage::initFor(qint64 _id, hist::scroll_mode_type _type, FirstInit _firstInit)
    {
        im_assert(_type == hist::scroll_mode_type::none || _type == hist::scroll_mode_type::search || _type == hist::scroll_mode_type::unread);

        if (_id > 0 && _type == hist::scroll_mode_type::search)
            Log::write_network_log(su::concat("message-transition", " init for <", aimId_.toStdString(), "> ", std::to_string(_id), "\r\n"));

        initForTimer_.start();

        history_->initFor(_id, _type);

        if (_firstInit == FirstInit::Yes)
            reader_->init();
    }

    void HistoryControlPage::resetMessageHighlights()
    {
        highlights_.clear();

        messagesArea_->enumerateWidgets([](QWidget *widget, const bool)
        {
            if (auto item = qobject_cast<Ui::HistoryControlPageItem*>(widget))
                item->resetHighlight();
            return true;

        }, false);
    }

    void HistoryControlPage::setHighlights(const highlightsV& _highlights)
    {
        highlights_ = _highlights;
    }

    void HistoryControlPage::showStatusBannerIfNeeded()
    {
        if (Utils::isChat(aimId_) || aimId() == MyInfo()->aimId())
            return;

        const auto needBanner = Logic::GetStatusContainer()->isStatusBannerNeeded(aimId());
        const auto visible = pinnedWidget_->isBannerVisible();
        if (needBanner && !visible)
            pinnedWidget_->showStatusBanner();
        else if (!needBanner && visible)
            pinnedWidget_->hide();
    }

    void HistoryControlPage::showStrangerIfNeeded()
    {
        if (!config::get().is_on((config::features::stranger_contacts)) || !Utils::isChat(aimId_))
            return;

        const auto isStranger = Logic::getRecentsModel()->isStranger(aimId_);
        const auto visible = pinnedWidget_->isStrangerVisible();
        if (isStranger && !visible)
        {
            connect(pinnedWidget_, &PinnedMessageWidget::strangerBlockClicked, this, &HistoryControlPage::strangerBlock, Qt::UniqueConnection);
            connect(pinnedWidget_, &PinnedMessageWidget::strangerCloseClicked, this, &HistoryControlPage::strangerClose, Qt::UniqueConnection);
            pinnedWidget_->showStranger();
        }
        else if (!isStranger && visible)
        {
            pinnedWidget_->hide();
        }
    }

    bool HistoryControlPage::isScrolling() const
    {
        return !messagesArea_->isScrollAtBottom();
    }

    QWidget* HistoryControlPage::getWidgetByKey(const Logic::MessageKey& _key) const
    {
        im_assert(_key.hasId());
        return messagesArea_->getItemByKey(_key);
    }

    HistoryControlPage::WidgetRemovalResult HistoryControlPage::removeExistingWidgetByKey(const Logic::MessageKey& _key)
    {
        const auto widget = getWidgetByKey(_key);
        if (!widget)
            return WidgetRemovalResult::NotFound;

        if (!isRemovableWidget(widget))
            return WidgetRemovalResult::PersistentWidget;

        messagesArea_->removeWidget(widget);

        clearSmartrepliesForMessage(_key.getId());

        return WidgetRemovalResult::Removed;
    }

    void HistoryControlPage::cancelWidgetRequests(const QVector<Logic::MessageKey>& _keys)
    {
        messagesArea_->cancelWidgetRequests(_keys);
    }

    void HistoryControlPage::removeWidgetByKeys(const QVector<Logic::MessageKey>& _keys)
    {
        messagesArea_->removeWidgets(_keys);
    }

    void HistoryControlPage::strangerClose()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", aimId_);
        Ui::GetDispatcher()->post_message_to_core("dlg_state/close_stranger", collection.get());
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_blockbar_action, { { "type", "cancel" },{ "chat_type", Utils::chatTypeByAimId(aimId_) } });
        chatscrBlockbarActionTracked_ = true;

        if (activeCallPlate_ && activeCallPlate_->participantsCount() > 0)
            showActiveCallPlate();
    }

    void HistoryControlPage::strangerBlock()
    {
        if (Features::isGroupInvitesBlacklistEnabled() && Utils::isChat(aimId_))
        {
            const auto text = Logic::getContactListModel()->isChannel(aimId_)
                              ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to erase history and leave channel?")
                              : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to erase history and leave chat?");

            const auto confirmed = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Leave"),
                text,
                QT_TRANSLATE_NOOP("sidebar", "Leave and delete"),
                nullptr);

            if (!confirmed)
                return;

            chatscrBlockbarActionTracked_ = true;

            Logic::getContactListModel()->removeContactFromCL(aimId_);

            if (!chatInviter_.isEmpty())
            {
                QTimer::singleShot(0, [inviter = chatInviter_]()
                {
                    auto w = new MultipleOptionsWidget(nullptr,
                        { {qsl(":/ignore_icon"), QT_TRANSLATE_NOOP("popup_widget", "Disallow")},
                            {qsl(":/settings/general"), QT_TRANSLATE_NOOP("popup_widget", "Manage who can add me")} });
                    Ui::GeneralDialog generalDialog(w, Utils::InterConnector::instance().getMainWindow());
                    generalDialog.addLabel(QT_TRANSLATE_NOOP("popup_widget", "Disallow %1 to add you to add me to groups?").arg(Logic::GetFriendlyContainer()->getFriendly(inviter)));
                    generalDialog.addCancelButton(QT_TRANSLATE_NOOP("report_widget", "Cancel"), true);

                    if (generalDialog.execute())
                    {
                        if (w->selectedIndex() == 0)
                            GetDispatcher()->addToInvitersBlacklist({ inviter });
                        else if (auto mainPage = MainPage::instance())
                            mainPage->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_Security);
                    }
                });
            }
        }
        else
        {
            const auto result = Ui::BlockAndReport(aimId_, Logic::GetFriendlyContainer()->getFriendly(aimId_));
            if (result != BlockAndReportResult::CANCELED)
            {
                Logic::getContactListModel()->ignoreContact(aimId_, true);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_blockbar_action, { { "type", result == BlockAndReportResult::BLOCK ? "block" : "spam" },{ "chat_type", Utils::chatTypeByAimId(aimId_) } });
                chatscrBlockbarActionTracked_ = true;
            }
        }
    }

    void HistoryControlPage::authBlockContact(const QString& _aimId)
    {
        Ui::ReportContact(_aimId, Logic::GetFriendlyContainer()->getFriendly(_aimId));
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_contact, { { "From", "Auth_Widget" } });
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignore_contact, { { "From", "Auth_Widget" } });

        Q_EMIT Utils::InterConnector::instance().profileSettingsBack();
    }

    void HistoryControlPage::authDeleteContact(const QString& _aimId)
    {
        Logic::getContactListModel()->removeContactFromCL(_aimId);

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        Ui::GetDispatcher()->post_message_to_core("dialogs/hide", collection.get());

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::delete_contact, { { "From", "Auth_Widget" } });
    }

    void HistoryControlPage::newPlateShowed()
    {
        newPlateId_ = std::nullopt;
    }

    void HistoryControlPage::insertNewPlate()
    {
    }

    void HistoryControlPage::scrollToMessage(int64_t _msgId)
    {
        initFor(_msgId, hist::scroll_mode_type::search, PageBase::FirstInit::No);
        QMetaObject::invokeMethod(this, &HistoryControlPage::setFocusOnInputWidget, Qt::QueuedConnection);
    }

    void HistoryControlPage::scrollTo(const Logic::MessageKey& key, hist::scroll_mode_type _scrollMode)
    {
        messagesArea_->scrollTo(key, _scrollMode);
    }

    void HistoryControlPage::updateItems()
    {
        messagesArea_->updateItems();
    }

    void HistoryControlPage::copy()
    {
        MimeData::copyMimeData(*messagesArea_);
    }

    void HistoryControlPage::quoteText(const Data::QuotesVec& q)
    {
        const auto quotes = messagesArea_->getQuotes();
        messagesArea_->clearSelection();
        Q_EMIT quote(quotes.isEmpty() ? q : quotes);

        QMetaObject::invokeMethod(this, [this]()
        {
            if (inputWidget_->getQuotesCount() != 1)
                hideSmartrepliesForQuoteAnimated();
        }, Qt::QueuedConnection);
    }

    void HistoryControlPage::forwardText(const Data::QuotesVec& q)
    {
        Q_EMIT Utils::InterConnector::instance().stopPttRecord();
        const auto quotes = messagesArea_->getQuotes(MessagesScrollArea::IsForward::Yes);

        const auto& quotesToSend = quotes.isEmpty() ? q : quotes;

        // disable author setting for single favorites messages which are not forwards
        const auto disableAuthor = Favorites::isFavorites(aimId_) && quotesToSend.size() == 1 && Favorites::isFavorites(quotesToSend.front().senderId_);

        if (forwardMessage(quotesToSend, true, QString(), QString(), !disableAuthor) != 0)
        {
            messagesArea_->clearSelection();
            Utils::InterConnector::instance().setMultiselect(false, aimId_);
        }

        setFocusOnInputWidget();
    }

    void HistoryControlPage::addToFavorites(const Data::QuotesVec& _quotes)
    {
        Favorites::addToFavorites(_quotes);
        Favorites::showSavedToast(_quotes.size());
    }

    void HistoryControlPage::pin(const QString& _chatId, const int64_t _msgId, const bool _isUnpin)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", _chatId);
        collection.set_value_as_int64("msgId", _msgId);
        collection.set_value_as_bool("unpin", _isUnpin);
        Ui::GetDispatcher()->post_message_to_core("chats/message/pin", collection.get());
    }

    void HistoryControlPage::multiselectChanged()
    {
        const auto isMultiselect = Utils::InterConnector::instance().isMultiselect(aimId_);
        if (topWidget_)
            topWidget_->setCurrentIndex(isMultiselect ? TopWidget::Selection : TopWidget::Main);

        setSmartrepliesSemitransparent(isMultiselect || inputWidget_->getQuotesCount() > 0);

        if (isMultiselect)
        {
            hideSmartrepliesForQuoteAnimated();
            setFocusOnArea();
        }

        Q_EMIT Utils::InterConnector::instance().updateSelectedCount(aimId_);
    }

    void HistoryControlPage::multiselectDelete(const QString& _aimId)
    {
        if (!isPageOpen() || aimId_ != _aimId)
            return;

        if (!Logic::getContactListModel()->isThread(aimId_))
        {
            auto canDeleteForMe = 0, canDeleteForAll = 0;
            messagesArea_->countSelected(canDeleteForMe, canDeleteForAll);
            const auto showInfoText =
                Favorites::isFavorites(aimId_) ? Utils::DeleteMessagesWidget::ShowInfoText::No : Utils::DeleteMessagesWidget::ShowInfoText::Yes;
            auto w = new Utils::DeleteMessagesWidget(nullptr, canDeleteForAll > 0, showInfoText);
            w->setCheckBoxText(QT_TRANSLATE_NOOP("context_menu", "Delete for all"));
            w->setCheckBoxChecked(true);
            if (canDeleteForAll > 0 && showInfoText == Utils::DeleteMessagesWidget::ShowInfoText::Yes)
                w->setInfoText(QT_TRANSLATE_NOOP("delete_messages", "Messages will be deleted only for you"));

            GeneralDialog generalDialog(w, Utils::InterConnector::instance().getMainWindow());
            generalDialog.addLabel(QT_TRANSLATE_NOOP("delete_messages", "Delete messages"));
            generalDialog.addButtonsPair(QT_TRANSLATE_NOOP("delete_messages", "Cancel"), QT_TRANSLATE_NOOP("delete_messages", "OK"));
            if (generalDialog.execute())
            {
                if (w->isChecked() && canDeleteForMe != canDeleteForAll)
                {
                    const auto confirmed = Utils::GetConfirmationWithTwoButtons(QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                        QT_TRANSLATE_NOOP("popup_window", "Yes"),
                        QT_TRANSLATE_NOOP("popup_window", "You can delete for all only your messages (%1 from %2). Are you sure you want to continue?")
                            .arg(canDeleteForAll)
                            .arg(canDeleteForMe),
                        QT_TRANSLATE_NOOP("delete_messages", "Delete messages"),
                        nullptr);

                    if (!confirmed)
                        return;
                }

                auto messagesToDelete = messagesArea_->getSelectedMessagesForDelete();
                if (!w->isChecked())
                {
                    for (auto& m : messagesToDelete)
                        m.ForAll_ = false;
                }

                Ui::GetDispatcher()->deleteMessages(aimId_, messagesToDelete);
                Utils::InterConnector::instance().setMultiselect(false, aimId_);
            }
        }
        else
        {
            const auto guard = QPointer(this);

            auto confirm = Utils::GetConfirmationWithTwoButtons(QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Yes"),
                QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete message?"),
                QT_TRANSLATE_NOOP("popup_window", "Delete message"),
                nullptr);

            if (!guard)
                return;

            if (confirm)
            {

                Ui::GetDispatcher()->deleteMessages(aimId_, messagesArea_->getSelectedMessagesForDelete());
                Utils::InterConnector::instance().setMultiselect(false, aimId_);
            }
        }
    }

    void HistoryControlPage::multiselectFavorites(const QString& _aimId)
    {
        if (!isPageOpen() || aimId_ != _aimId)
            return;

        const auto quotes = messagesArea_->getQuotes(MessagesScrollArea::IsForward::Yes);
        addToFavorites(quotes);
        Utils::InterConnector::instance().setMultiselect(false, aimId_);
    }

    void HistoryControlPage::multiselectCopy(const QString& _aimId)
    {
        if (!isPageOpen() || aimId_ != _aimId)
            return;

        copy();
        Utils::InterConnector::instance().setMultiselect(false, aimId_);
    }

    void HistoryControlPage::multiselectReply(const QString& _aimId)
    {
        if (!isPageOpen() || aimId_ != _aimId)
            return;

        quoteText({});
        Utils::InterConnector::instance().setMultiselect(false, aimId_);
    }

    void HistoryControlPage::multiselectForward(const QString& _aimId)
    {
        if (!isPageOpen() || aimId_ != _aimId)
            return;

        forwardText({});
    }

    void HistoryControlPage::edit(const Data::MessageBuddySptr& _msg, MediaType _mediaType)
    {
        messagesArea_->clearSelection();
        inputWidget_->edit(_msg, _mediaType);
    }

    void HistoryControlPage::onItemLayoutChanged()
    {
        updateItems();
    }

    std::optional<qint64> HistoryControlPage::getNewPlateId() const
    {
        return newPlateId_;
    }

    static InsertHistMessagesParams::PositionWidget makeNewPlate(const QString& _aimId, qint64 _id, int _width, QWidget* _parent)
    {
        return { Logic::MessageKey(_id, Logic::ControlType::NewMessages), hist::MessageBuilder::createNew(_aimId, _width, _parent) };
    }

    static InsertHistMessagesParams::WidgetsList makeWidgets(const QString& _aimId, const Data::MessageBuddies& _messages, int _width, Heads::HeadContainer* headsContainer, std::optional<qint64> _newPlateId, QWidget* _parent)
    {
        const auto& heads = headsContainer->headsById();

        InsertHistMessagesParams::WidgetsList widgets;
        widgets.reserve(_messages.size() + (_newPlateId ? 1 : 0));
        for (const auto& msg : _messages)
        {
            if (auto item = hist::MessageBuilder::makePageItem(*msg, _width, _parent))
            {
                if (const auto it = heads.find(item->getId()); it != heads.end())
                    item->setHeads(it.value());
                widgets.emplace_back(msg->ToKey(), std::move(item));
            }
        }

        if (_newPlateId)
            widgets.push_back(makeNewPlate(_aimId, *_newPlateId, _width, _parent));

        return widgets;
    }

    void HistoryControlPage::sourceReadyHist(int64_t _messId, hist::scroll_mode_type _scrollMode)
    {
        if (_scrollMode == hist::scroll_mode_type::unread)
            newPlateId_ = _messId;
        else
            newPlateId_ = std::nullopt;

        if (_messId > 0 && _scrollMode == hist::scroll_mode_type::search)
            Log::write_network_log(su::concat("message-transition", " history is ready <", aimId_.toStdString(), "> ", std::to_string(_messId),
                "\ncompleted in ", std::to_string(initForTimer_.elapsed()), " ms\r\n"));

        const auto id = _messId > 0 ? std::make_optional(_messId) : std::nullopt;
        const auto messages = history_->getInitMessages(id);

        switchToInsertingState(__FUNCLINEA__);

        InsertHistMessagesParams params;
        params.scrollToMesssageId = id;
        params.scrollMode = _scrollMode;
        params.removeOthers = RemoveOthers::Yes;
        if (!id)
            params.scrollOnInsert = std::make_optional(ScrollOnInsert::ForceScrollToBottom);

        params.widgets = makeWidgets(aimId_, messages, width(), heads_, newPlateId_, messagesArea_);
        params.newPlateId = newPlateId_;
        params.highlights_ = std::move(highlights_);

        resetMessageHighlights();

        insertMessages(std::move(params));
        switchToIdleState(__FUNCLINEA__);
        setHighlights({});

        updateChatPlaceholder();

        if (!messages.isEmpty() && !messagesArea_->isViewportFull())
            fetch(hist::FetchDirection::toOlder);

        Q_EMIT Utils::InterConnector::instance().historyReady(aimId_);
    }

    void HistoryControlPage::updatedBuddies(const Data::MessageBuddies& _buddies)
    {
        updatePinMessage(_buddies);

        switchToInsertingState(__FUNCLINEA__);
        InsertHistMessagesParams params;
        params.isBackgroundMode = isInBackground();
        params.widgets = makeWidgets(aimId_, _buddies, width(), heads_, newPlateId_, messagesArea_);
        params.newPlateId = newPlateId_;
        params.updateExistingOnly = std::none_of(_buddies.begin(), _buddies.end(), [](const auto& _buddy) { return _buddy->IsRestoredPatch(); });
        params.scrollOnInsert = std::make_optional(addReactionPlateActive_ ? ScrollOnInsert::None : ScrollOnInsert::ScrollToBottomIfNeeded);

        insertMessages(std::move(params));
        switchToIdleState(__FUNCLINEA__);

        if (!_buddies.isEmpty() && !messagesArea_->isViewportFull())
            fetch(hist::FetchDirection::toOlder);
    }

    void HistoryControlPage::insertedBuddies(const Data::MessageBuddies& _buddies)
    {
        updatePinMessage(_buddies);

        switchToInsertingState(__FUNCLINEA__);
        InsertHistMessagesParams params;
        params.isBackgroundMode = isInBackground() || Ui::GetDispatcher()->getConnectionState() == Ui::ConnectionState::stateUpdating;
        params.widgets = makeWidgets(aimId_, _buddies, width(), heads_, newPlateId_, messagesArea_);
        params.newPlateId = newPlateId_;
        params.updateExistingOnly = false;
        params.forceUpdateItems = true;
        params.scrollOnInsert = std::make_optional(addReactionPlateActive_ ? ScrollOnInsert::None : ScrollOnInsert::ScrollToBottomIfNeeded);

        if (params.isBackgroundMode)
        {
            params.lastReadMessageId = hist::getDlgState(aimId_).YoursLastRead_;
        }
        else if (newPlateId_)
        {
            params.scrollMode = hist::scroll_mode_type::unread;

            im_assert(std::is_sorted(_buddies.begin(), _buddies.end(), [](const auto& l, const auto& r){ return l->Id_ < r->Id_; }));

            if (history_->lastMessageId() > _buddies.back()->Id_ && newPlateId_ < _buddies.front()->Id_)
                params.scrollToMesssageId = newPlateId_;
        }

        insertMessages(std::move(params));

        unloadAfterInserting();

        switchToIdleState(__FUNCLINEA__);
    }

    void HistoryControlPage::connectToComplexMessageItem(const QWidget* _widget) const
    {
        auto complexMessageItem = qobject_cast<const Ui::ComplexMessage::ComplexMessageItem*>(_widget);
        if (complexMessageItem)
        {
            if (!connectToComplexMessageItemImpl(complexMessageItem))
                im_assert(!"can not connect to complexMessageItem");

            connectToPageItem(complexMessageItem);
        }
        else if (auto layout = _widget->layout())
        {
            int index = 0;
            while (auto child = layout->itemAt(index++))
            {
                auto childWidget = child->widget();
                if (auto childComplexMessageItem = qobject_cast<const Ui::ComplexMessage::ComplexMessageItem*>(childWidget))
                {
                    if (!connectToComplexMessageItemImpl(childComplexMessageItem))
                        im_assert(!"can not connect to complexMessageItem");
                }
                else if (auto childVoipItem = qobject_cast<const Ui::VoipEventItem*>(childWidget))
                {
                    if (!connectToVoipItem(childVoipItem))
                        im_assert(!"can not connect to VoipEventItem");
                }

                connectToPageItem(qobject_cast<Ui::HistoryControlPageItem*>(childWidget));
            }
        }
    }

    bool HistoryControlPage::connectToVoipItem(const Ui::VoipEventItem* _item) const
    {
        const auto list = {
            connect(_item, &VoipEventItem::copy, this, &HistoryControlPage::copy),
            connect(_item, &VoipEventItem::quote, this, &HistoryControlPage::quoteText),
            connect(_item, &VoipEventItem::forward, this, &HistoryControlPage::forwardText),
            connect(_item, &VoipEventItem::addToFavorites, this, &HistoryControlPage::addToFavorites)
        };
        return std::all_of(list.begin(), list.end(), [](const auto& x) { return x; });
    }

    void HistoryControlPage::connectToPageItem(const HistoryControlPageItem* _item) const
    {
        if (!_item)
            return;

        connect(_item, &HistoryControlPageItem::mention, this, &HistoryControlPage::mentionHeads, Qt::UniqueConnection);
    }

    void HistoryControlPage::connectToMessageBuddies()
    {
        connectMessageBuddies_ = connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messageBuddies, this, &HistoryControlPage::onMessageBuddies, Qt::UniqueConnection);
    }

    bool HistoryControlPage::connectToComplexMessageItemImpl(const Ui::ComplexMessage::ComplexMessageItem* complexMessageItem) const
    {
        const auto list = {
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::copy, this, &HistoryControlPage::copy, Qt::UniqueConnection),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::quote, this, &HistoryControlPage::quoteText, Qt::UniqueConnection),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::createTask, this, &HistoryControlPage::onCreateTask, Qt::UniqueConnection),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::forward, this, &HistoryControlPage::forwardText, Qt::UniqueConnection),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::avatarMenuRequest, this, &HistoryControlPage::avatarMenuRequest, Qt::UniqueConnection),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::pin, this, &HistoryControlPage::pin, Qt::UniqueConnection),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::edit, this, &HistoryControlPage::edit, Qt::UniqueConnection),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::needUpdateRecentsText, this, &HistoryControlPage::onNeedUpdateRecentsText, Qt::UniqueConnection),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::layoutChanged, this, &HistoryControlPage::onItemLayoutChanged, Qt::UniqueConnection),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::addToFavorites, this, &HistoryControlPage::addToFavorites, Qt::QueuedConnection),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::readOnlyUser, this, &HistoryControlPage::onReadOnlyUser, Qt::UniqueConnection),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::blockUser, this, &HistoryControlPage::onBlockUser, Qt::UniqueConnection),
        };
        return std::all_of(list.begin(), list.end(), [](const auto& x) { return x; });
    }

    void HistoryControlPage::removeWidget(const Logic::MessageKey& _key)
    {
        /*if (isScrolling())
        {
            Q_EMIT needRemove(_key);
            return;
        }
        */

        __TRACE(
            "history_control",
            "requested to remove the widget\n"
            "	key=<" << _key.getId() << ";" << _key.getInternalId() << ">");

        const auto result = removeExistingWidgetByKey(_key);
        im_assert(result > WidgetRemovalResult::Min);
        im_assert(result < WidgetRemovalResult::Max);
    }

    void HistoryControlPage::insertMessages(InsertHistMessagesParams&& _params)
    {
        QVector<Logic::MessageKey> toRemove;
        toRemove.reserve(_params.widgets.size());

        auto doInsertWidgets = [this, &_params, &toRemove]()
        {
            InsertHistMessagesParams::WidgetsList insertWidgets;
            insertWidgets.reserve(_params.widgets.size());

            const auto isStranger = Logic::getRecentsModel()->isStranger(aimId_);

            while (!_params.widgets.empty())
            {
                auto data = std::move(_params.widgets.back());
                _params.widgets.pop_back();

                auto& key = data.first;
                auto& widget = data.second;

                auto insert = [this, &insertWidgets, &key, &widget]()
                {
                    connectToPageItem(qobject_cast<HistoryControlPageItem*>(widget.get()));
                    insertWidgets.emplace_back(std::move(key), std::move(widget));
                };
                auto replace = [&toRemove, &key, &insert]()
                {
                    toRemove.push_back(key);
                    insert();
                };

                auto newPageItem = qobject_cast<Ui::HistoryControlPageItem*>(widget.get());

                if (isStranger)
                {
                    if (newPageItem && newPageItem->isOutgoing())
                    {
                        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_blockbar_action, { { "type", "send" },{ "chat_type", Utils::chatTypeByAimId(aimId_) } });
                        chatscrBlockbarActionTracked_ = true;
                    }
                }

                if (auto existing = getWidgetByKey(key))
                {
                    if (key.getControlType() == Logic::ControlType::NewMessages)
                        continue;

                    auto exisitingPageItem = qobject_cast<Ui::HistoryControlPageItem*>(existing);
                    if (exisitingPageItem && newPageItem)
                    {
                        newPageItem->setHasAvatar(exisitingPageItem->hasAvatar());
                        newPageItem->setHasSenderName(exisitingPageItem->hasSenderName());
                    }

                    auto complexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(existing);
                    auto newComplexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(widget.get());

                    auto newChatEventItem = qobject_cast<Ui::ChatEventItem*>(widget.get());
                    const auto isUpdateable = isUpdateableWidget(existing);

                    if (!newChatEventItem && !isUpdateable)
                    {
                        if (newComplexMessageItem)
                        {
                            newComplexMessageItem->setDeliveredToServer(!key.isPending());

                            connectToComplexMessageItem(newComplexMessageItem);
                            replace();
                            continue;
                        }

                        im_assert(false);
                        continue;
                    }

                    if (complexMessageItem && newComplexMessageItem && isUpdateable)
                    {
                        complexMessageItem->setDeliveredToServer(!key.isPending());

                        if (complexMessageItem->canBeUpdatedWith(*newComplexMessageItem))
                        {
                            complexMessageItem->updateWith(*newComplexMessageItem);
                            messagesArea_->updateItemKey(key);

                            newComplexMessageItem->setThreadId({});
                        }
                        else
                        {
                            newComplexMessageItem->getUpdatableInfoFrom(*complexMessageItem);
                            connectToComplexMessageItem(newComplexMessageItem);

                            replace();
                        }

                        continue;
                    }

                    auto chatEventItem = qobject_cast<Ui::ChatEventItem*>(existing);
                    if (chatEventItem && newChatEventItem)
                    {
                        replace();
                        continue;
                    }

                    auto voipItem = qobject_cast<Ui::VoipEventItem*>(existing);
                    auto newVoipItem = qobject_cast<Ui::VoipEventItem*>(widget.get());
                    if (voipItem && newVoipItem)
                    {
                        voipItem->updateWith(*newVoipItem);
                        auto w = messagesArea_->extractItemByKey(key);
                        messagesArea_->insertWidget(key, std::unique_ptr<QWidget>(w));
                        continue;
                    }

                    removeExistingWidgetByKey(key);
                }
                else if (_params.updateExistingOnly)
                {
                    if (key.getControlType() != Logic::ControlType::NewMessages) // insert new plate anyway since we deleted it at start of this method
                        continue;
                }

                if (auto childVoipItem = qobject_cast<const Ui::VoipEventItem*>(widget.get()))
                {
                    if (!connectToVoipItem(childVoipItem))
                        im_assert(!"can not connect to VoipEventItem");
                }
                else
                {
                    connectToComplexMessageItem(widget.get());
                }

                insert();
            }

            return insertWidgets;
        };

        bool isInserted = false;
        {
            std::scoped_lock lock(*(messagesArea_->getLayout()));
            if (_params.removeOthers == RemoveOthers::Yes)
                messagesArea_->getLayout()->removeAllExcluded(_params);

            _params.widgets = doInsertWidgets();

            isInserted = !_params.widgets.empty();

            messagesArea_->removeWidgets(toRemove);
            messagesArea_->insertWidgets(std::move(_params));
        }

        if (isInserted)
        {
            Q_EMIT Utils::InterConnector::instance().historyInsertedMessages(aimId_);
            updateChatPlaceholder();
        }
    }

    void HistoryControlPage::unloadAfterInserting()
    {
        const auto canUnloadTop = !history_->hasFetchRequest(hist::FetchDirection::toOlder);
        const auto canUnloadBottom = !history_->hasFetchRequest(hist::FetchDirection::toNewer);

        if (canUnloadTop || canUnloadBottom)
        {
            std::scoped_lock lock(*(messagesArea_->getLayout()));

            im_assert(history_->isViewLocked());

            if (messagesArea_->isScrollAtBottom())
            {
                if (canUnloadTop)
                    unloadTop(FromInserting::Yes);
            }
            else
            {
                if (canUnloadTop)
                    unloadTop(FromInserting::Yes);
                if (canUnloadBottom)
                    unloadBottom(FromInserting::Yes);
            }
        }
    }

    template<typename T>
    static T messageKeysPartition(T first, T last)
    {
        return std::stable_partition(first, last, [](const auto& x) { return x.getControlType() == Logic::ControlType::Message; });
    }

    void HistoryControlPage::unload(FromInserting _mode, UnloadDirection _direction)
    {
        auto keys = _direction == UnloadDirection::TOP ? messagesArea_->getKeysToUnloadTop() : messagesArea_->getKeysToUnloadBottom();
        const auto begin = keys.begin();
        const auto it = messageKeysPartition(begin, keys.end());
        if (const auto messagesSize = std::distance(begin, it); messagesSize > 1)
        {
            if (_direction == UnloadDirection::TOP)
                std::sort(begin, it);
            else
                std::sort(begin, it, std::greater<>());

            const auto lastMessIt = std::prev(it);
            if (_direction == UnloadDirection::TOP)
                history_->setTopBound(*lastMessIt, _mode == FromInserting::Yes ? hist::History::CheckViewLock::No : hist::History::CheckViewLock::Yes);
            else
                history_->setBottomBound(*lastMessIt, _mode == FromInserting::Yes ? hist::History::CheckViewLock::No : hist::History::CheckViewLock::Yes);

            qCDebug(historyPage) << (_direction == UnloadDirection::TOP ? "unloadTop:" : "unloadBottom:");
            for (const auto& key : boost::make_iterator_range(begin, it))
                qCDebug(historyPage) << key.getId() << key.getInternalId();

            keys.erase(lastMessIt);
            keys.erase(std::remove_if(keys.begin(), keys.end(), [](const auto& iter) { return iter.getControlType() == Logic::ControlType::NewMessages; }), keys.end());
            cancelWidgetRequests(keys);
            removeWidgetByKeys(keys);
        }
    }

    void HistoryControlPage::unloadTop(FromInserting _mode)
    {
        unload(_mode, UnloadDirection::TOP);
    }

    void HistoryControlPage::unloadBottom(FromInserting _mode)
    {
        unload(_mode, UnloadDirection::BOTTOM);
    }

    void HistoryControlPage::removeNewPlateItem()
    {
        std::scoped_lock lock(*(messagesArea_->getLayout()));
        messagesArea_->getLayout()->removeItemsByType(Logic::ControlType::NewMessages);
        messagesArea_->getLayout()->updateItemsProps();
        eventFilter_->resetNewPlate();
    }

    bool HistoryControlPage::hasNewPlate()
    {
        std::scoped_lock lock(*(messagesArea_->getLayout()));
        return messagesArea_->getLayout()->hasItemsOfType(Logic::ControlType::NewMessages);
    }

    void HistoryControlPage::loadChatInfo()
    {
        if (isThread())
            return;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("aimid", aimId_);
        const auto stamp = Logic::getContactListModel()->getChatStamp(aimId_);
        if (!stamp.isEmpty())
            collection.set_value_as_qstring("stamp", stamp);
        collection.set_value_as_int("limit", chatInfoMembersLimit);
        Ui::GetDispatcher()->post_message_to_core("chats/info/get", collection.get());
    }

    qint64 Ui::HistoryControlPage::loadChatMembersInfo(const std::vector<QString>& _members)
    {
        return Ui::GetDispatcher()->loadChatMembersInfo(aimId_, _members);
    }

    void HistoryControlPage::initStatus()
    {
        if (Utils::isChat(aimId_))
            loadChatInfo();

        if (header_)
            header_->initStatus();
    }

    void HistoryControlPage::chatInfoFailed(qint64 _seq, core::group_chat_info_errors _errorCode, const QString& _aimId)
    {
        if (_aimId != aimId_)
            return;

        if (isThread())
        {
            isMember_ = true;
        }
        else if (_errorCode == core::group_chat_info_errors::not_in_chat || _errorCode == core::group_chat_info_errors::blocked)
        {
            Logic::getContactListModel()->setYourRole(aimId_, qsl("notamember"));
            isMember_ = false;
            if (header_)
                header_->updateFromChatInfo({});

            updateChatPlaceholder();
        }
    }

    void HistoryControlPage::chatInfo(qint64 _seq, const std::shared_ptr<Data::ChatInfo>& _info, const int _requestMembersLimit)
    {
        if (_info->AimId_ != aimId_)
            return;

        __INFO(
            "chat_info",
            "incoming chat info event\n"
            "    contact=<" << aimId_ << ">\n"
            "    your_role=<" << _info->YourRole_ << ">"
        );

        const auto youMember = Data::ChatInfo::areYouMember(_info);

        if (youMember && !isMember_)
            GetDispatcher()->subscribeCallRoomInfo(aimId_);

        isMember_ = youMember;
        isChatCreator_ = Data::ChatInfo::areYouAdmin(_info);
        chatInviter_ = _info->Inviter_;

        const bool isAdmin = Data::ChatInfo::areYouAdminOrModer(_info);
        if (!isAdminRole_ && isAdmin)
            updateSendersInfo();

        if (!isAdmin && connectMessageBuddies_)
        {
            disconnect(connectMessageBuddies_);
            chatSenders_.clear();
        }

        isAdminRole_ = isAdmin;

        if (header_)
            header_->updateFromChatInfo(_info);

        updateChatPlaceholder();
        updateSpellCheckVisibility();
        updateFileStatuses();
    }

    void HistoryControlPage::contactChanged(const QString& _aimId)
    {
        if (_aimId != aimId_)
            return;

        if (Utils::isChat(aimId_))
            loadChatInfo();

        if (header_)
            header_->updateName();
    }

    void HistoryControlPage::onChatRoleChanged(const QString& _aimId)
    {
        if (_aimId != aimId_)
            return;

        if (Logic::getContactListModel()->isReadonly(aimId_))
            hideAndClearSmartreplies();

        updateSpellCheckVisibility();
    }

    void HistoryControlPage::onNewMessageAdded(const QString& _aimId)
    {
        if (_aimId != aimId_)
            return;

        messagesArea_->enableViewportShifting(true);
    }

    void HistoryControlPage::onNewMessagesReceived(const QVector<qint64>& _ids)
    {
        if (!_ids.isEmpty())
        {
            // in background or after reconnecting mark messages by "new" plate
            if ((!isPageOpen() || !hasNewPlate()) && (isInBackground() || Ui::GetDispatcher()->getConnectionState() == Ui::ConnectionState::stateUpdating))
            {
                const Data::DlgState state = hist::getDlgState(aimId_);
                if (!(state.LastMsgId_ == state.YoursLastRead_ && *std::max_element(_ids.begin(), _ids.end()) <= state.LastMsgId_))
                    newPlateId_ = state.YoursLastRead_;
            }
        }
    }

    void HistoryControlPage::open()
    {
        mentionCompleter_->setDialogAimId(aimId_);
        positionMentionCompleter(inputWidget_->tooltipArrowPosition());

        const auto dlgstate = Logic::getRecentsModel()->getDlgState(aimId_);
        if (dlgstate.YoursLastRead_ == -1 && Logic::getRecentsModel()->isStranger(aimId_))
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_blockbar_event, { { "type", "appeared" }, { "chat_type", Utils::chatTypeByAimId(aimId_) } });

        initStatus();

        multiselectChanged();

        const auto& newWallpaperId = Styling::getThemesContainer().getContactWallpaperId(aimId());
        const bool changed = (newWallpaperId.isValid() && wallpaperId_ != newWallpaperId) || (!newWallpaperId.isValid() && wallpaperId_.isValid());

        if (changed)
        {
            wallpaperId_ = newWallpaperId;
            im_assert(wallpaperId_.isValid());
        }

        if (themeChecker_.checkAndUpdateHash() || changed)
            updateWidgetsThemeImpl();
        else
            messagesOverlay_->update();

        updateFonts();

        updateOverlaySizes();

        resumeVisibleItems();

        updateMentionsButtonDelayed();
        updatePlaceholderPosition();

        if (smartreplyWidget_ && Utils::canShowSmartreplies(aimId_))
        {
            smartreplyWidget_->scrollToNewest();

            if (smartreplyWidget_->itemCount() > 0 && !isSmartrepliesVisible())
                showSmartreplies();
        }

        messagesArea_->invalidateLayout();

        Q_EMIT Utils::InterConnector::instance().updateSelectedCount(aimId_);

        GetDispatcher()->setDialogOpen(aimId_, true);
    }

    const QString& HistoryControlPage::aimId() const
    {
        return aimId_;
    }

    void HistoryControlPage::cancelSelection()
    {
        im_assert(messagesArea_);
        messagesArea_->cancelSelection();
    }

    void HistoryControlPage::deleted(const Data::MessageBuddies& _messages)
    {
        {
            std::scoped_lock lock(*(messagesArea_->getLayout()));
            for (const auto& msg : _messages)
                removeExistingWidgetByKey(msg->ToKey());

            if (messagesArea_->getLayout()->removeItemAtEnd(Logic::ControlType::NewMessages) || !messagesArea_->getLayout()->hasItemsOfType(Logic::ControlType::NewMessages))
                newPlateId_ = std::nullopt;

            reader_->deleted(_messages);

            // update dates: updatedBuddies with empty list
            updatedBuddies({});
        }

        if (!messagesArea_->isViewportFull())
            fetch(hist::FetchDirection::toOlder);

        if (!hasMessages())
        {
            Q_EMIT Utils::InterConnector::instance().historyCleared(aimId_);
            updateChatPlaceholder();
        }
    }

    void HistoryControlPage::pendingResolves(const Data::MessageBuddies& _inView, const Data::MessageBuddies& _all)
    {
        Q_EMIT messageIdsFetched(aimId_, _all, QPrivateSignal());

        switchToInsertingState(__FUNCLINEA__);
        InsertHistMessagesParams params;
        params.isBackgroundMode = isInBackground();
        params.widgets = makeWidgets(aimId_, _inView, width(), heads_, newPlateId_, messagesArea_);
        params.newPlateId = newPlateId_;
        params.updateExistingOnly = false;
        params.scrollOnInsert = std::make_optional(ScrollOnInsert::ScrollToBottomIfNeeded);

        insertMessages(std::move(params));

        switchToIdleState(__FUNCLINEA__);
    }

    void HistoryControlPage::clearAll()
    {
        switchToInsertingState(__FUNCLINEA__);
        newPlateId_ = std::nullopt;
        std::scoped_lock lock(*(messagesArea_->getLayout()));

        messagesArea_->removeAll();

        setHighlights({});

        switchToIdleState(__FUNCLINEA__);
    }

    void HistoryControlPage::emptyHistory()
    {
        if (Favorites::isFavorites(aimId_))
            GetDispatcher()->sendMessageToContact(aimId_, Favorites::firstMessageText());
        else
            updateChatPlaceholder();
    }

    void HistoryControlPage::fetch(hist::FetchDirection _direction)
    {
        if (history_->canViewFetch() && isStateIdle())
        {
            switchToInsertingState(__FUNCLINEA__);
            const auto buddies = history_->fetchBuddies(_direction);
            if (!buddies.isEmpty())
            {
                std::scoped_lock lock(*(messagesArea_->getLayout()));

                InsertHistMessagesParams params;
                params.widgets = makeWidgets(aimId_, buddies, width(), heads_, newPlateId_, messagesArea_);
                params.newPlateId = newPlateId_;

                params.updateExistingOnly = false;
                if (_direction == hist::FetchDirection::toOlder)
                {
                    params.scrollOnInsert = std::make_optional(ScrollOnInsert::ScrollToBottomIfNeeded);
                }
                else if (params.newPlateId)
                {
                    params.scrollMode = hist::scroll_mode_type::unread;
                    params.lastReadMessageId = *newPlateId_;
                }

                insertMessages(std::move(params));

                if (_direction == hist::FetchDirection::toOlder)
                {
                    if (history_->canUnload(hist::UnloadDirection::toBottom))
                        unloadBottom();
                }
                else
                {
                    if (history_->canUnload(hist::UnloadDirection::toTop))
                        unloadTop();
                }

                changeDlgState(dlgState_);
            }

            switchToIdleState(__FUNCLINEA__);

            if (!buddies.isEmpty() && !messagesArea_->isViewportFull())
                Q_EMIT postponeFetch(_direction, QPrivateSignal());
        }
        else
        {
            Q_EMIT postponeFetch(_direction, QPrivateSignal());
        }
    }

    void HistoryControlPage::downPressed()
    {
        buttonDown_->resetCounter();
        scrollToBottom();
    }

    void HistoryControlPage::scrollMovedToBottom()
    {
        buttonDown_->resetCounter();
    }

    void HistoryControlPage::autoScroll(bool _enabled)
    {
        if (!_enabled)
            return;

        buttonDown_->resetCounter();
    }

    void HistoryControlPage::wheelEvent(QWheelEvent* _event)
    {
        if (!hasFocus())
            return;

        return PageBase::wheelEvent(_event);
    }

    void Ui::HistoryControlPage::updateSendersInfo()
    {
        if (chatSenders_.empty())
            return;

        std::vector<QString> senders;
        senders.reserve(chatSenders_.size());
        for (const auto&[aimId, _] : chatSenders_)
            senders.emplace_back(aimId);

        loadChatMembersInfo(senders);
    }

    void HistoryControlPage::setState(const State _state, const char* _dbgWhere)
    {
        im_assert(_state > State::Min);
        im_assert(_state < State::Max);
        im_assert(state_ > State::Min);
        im_assert(state_ < State::Max);

        __INFO(
            "smooth_scroll",
            "switching state\n"
            "    from=<" << state_ << ">\n"
            "    to=<" << _state << ">\n"
            "    where=<" << _dbgWhere << ">\n"
        );

        state_ = _state;
    }

    bool HistoryControlPage::isState(const State _state) const
    {
        im_assert(state_ > State::Min);
        im_assert(state_ < State::Max);
        im_assert(_state > State::Min);
        im_assert(_state < State::Max);

        return (state_ == _state);
    }

    bool HistoryControlPage::isStateIdle() const
    {
        return isState(State::Idle);
    }

    void HistoryControlPage::switchToIdleState(const char* _dbgWhere)
    {
        setState(State::Idle, _dbgWhere);
    }

    void HistoryControlPage::switchToInsertingState(const char* _dbgWhere)
    {
        setState(State::Inserting, _dbgWhere);
    }

    void HistoryControlPage::showAvatarMenu(const QString& _aimId)
    {
        auto menu = new ContextMenu(this);
        menu->addActionWithIcon(qsl(":/context_menu/mention"), QT_TRANSLATE_NOOP("context_menu", "Mention"), makeData(qsl("mention"), _aimId));

        if (isAdminRole_)
        {
            if (auto sender = chatSenders_.find(_aimId); sender != chatSenders_.end())
            {
                if (const auto& cont = sender->second)
                {
                    if (cont->Role_ != u"admin" && isChatCreator_)
                    {
                        if (cont->Role_ == u"moder")
                            menu->addActionWithIcon(qsl(":/context_menu/admin_off"), QT_TRANSLATE_NOOP("sidebar", "Revoke admin role"), makeData(qsl("revoke_admin"), _aimId));
                        else
                            menu->addActionWithIcon(qsl(":/context_menu/admin"), QT_TRANSLATE_NOOP("sidebar", "Assign admin role"), makeData(qsl("make_admin"), _aimId));
                    }

                    if (cont->Role_ == u"member")
                        menu->addActionWithIcon(qsl(":/context_menu/readonly"), QT_TRANSLATE_NOOP("sidebar", "Ban to write"), makeData(qsl("make_readonly"), _aimId));
                    else if (cont->Role_ == u"readonly")
                        menu->addActionWithIcon(qsl(":/context_menu/readonly_off"), QT_TRANSLATE_NOOP("sidebar", "Allow to write"), makeData(qsl("revoke_readonly"), _aimId));

                    const bool myInfo = MyInfo()->aimId() == _aimId;
                    if (!myInfo)
                        menu->addActionWithIcon(qsl(":/context_menu/profile"), QT_TRANSLATE_NOOP("sidebar", "Profile"), makeData(qsl("profile"), _aimId));
                    if (cont->Role_ != u"admin" && cont->Role_ != u"moder")
                        menu->addActionWithIcon(qsl(":/context_menu/block"), QT_TRANSLATE_NOOP("sidebar", "Delete from group"), makeData(qsl("block"), _aimId));

                    if (!myInfo && Features::clRemoveContactsAllowed())
                        menu->addActionWithIcon(qsl(":/context_menu/spam"), QT_TRANSLATE_NOOP("sidebar", "Report"), makeData(qsl("spam"), _aimId));
                }
            }
        }

        connect(menu, &ContextMenu::triggered, this, &HistoryControlPage::avatarMenu, Qt::QueuedConnection);
        connect(menu, &ContextMenu::triggered, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
        connect(menu, &ContextMenu::aboutToHide, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
        menu->popup(QCursor::pos());
    }

    QRect HistoryControlPage::getScrollAreaGeometry() const
    {
        if constexpr (platform::is_apple())
            return QRect(mapFromGlobal(messagesArea_->mapToGlobal(messagesArea_->pos())), messagesArea_->size());

        return messagesArea_->geometry();
    }

    void HistoryControlPage::onSmartreplies(const std::vector<Data::SmartreplySuggest>& _suggests)
    {
        if (_suggests.empty() || _suggests.front().getContact() != aimId_)
            return;

        if (dlgState_.AimId_.isEmpty())
            changeDlgState(hist::getDlgState(aimId_));

        if (dlgState_.AimId_.isEmpty() || (dlgState_.Outgoing_ && dlgState_.LastMsgId_ > _suggests.front().getMsgId()))
            return;

        if (!Utils::canShowSmartreplies(aimId_))
            return;

        if (!smartreplyWidget_)
        {
            smartreplyWidget_ = new SmartReplyWidget(messagesArea_, true);
            smartreplyWidget_->hide();
            updateSmartreplyStyle();
            messagesArea_->setSmartreplyWidget(smartreplyWidget_);

            connect(smartreplyWidget_, &SmartReplyWidget::needHide, this, &HistoryControlPage::onSmartreplyHide);
            connect(smartreplyWidget_, &SmartReplyWidget::sendSmartreply, this, &HistoryControlPage::sendSmartreply);
        }

        if (!smartreplyWidget_->isUnderMouse())
        {
            if (smartreplyWidget_->addItems(_suggests))
            {
                showSmartreplies();

                if (std::any_of(_suggests.begin(), _suggests.end(), [](const auto _s) { return _s.isStickerType(); }))
                {
                    GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::stickers_smartreply_appear_in_input);
                    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_smartreply_appear_in_input);
                }

                if (std::any_of(_suggests.begin(), _suggests.end(), [](const auto _s) { return _s.isTextType(); }))
                {
                    GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::smartreply_text_appear_in_input);
                    GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::smartreply_text_appear_in_input);
                }
            }
        }
    }

    void HistoryControlPage::hideAndClearSmartreplies()
    {
        if (smartreplyWidget_)
        {
            if (isSmartrepliesVisible())
            {
                smartreplyWidget_->markItemsToDeletion();

                if (smartreplyWidget_->isUnderMouse() && !Logic::getContactListModel()->isReadonly(aimId_))
                    smartreplyWidget_->setNeedHideOnLeave();
                else
                    hideSmartreplies();
            }
            else
            {
                smartreplyWidget_->clearItems();
            }
            hideSmartrepliesButton();
        }

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", aimId());
        Ui::GetDispatcher()->post_message_to_core("smartreply/suggests/clearall", collection.get());
    }

    void HistoryControlPage::clearSmartrepliesForMessage(const qint64 _msgId)
    {
        if (!smartreplyWidget_)
            return;

        if (dlgState_.LastMsgId_ != -1 && dlgState_.LastMsgId_ == _msgId)
        {
            hideAndClearSmartreplies();
        }
        else
        {
            smartreplyWidget_->clearItemsForMessage(_msgId);
            if (smartreplyWidget_->itemCount() == 0)
                hideSmartreplies();

            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", aimId());
            collection.set_value_as_int64("msgId", _msgId);
            Ui::GetDispatcher()->post_message_to_core("smartreply/suggests/clear", collection.get());
        }
    }

    void HistoryControlPage::onSmartreplyButtonClicked()
    {
        const auto val = get_gui_settings()->get_value<bool>(settings_show_smartreply, settings_show_smartreply_default());
        if (val && !isSmartrepliesVisible())
            showSmartreplies();
        else
            get_gui_settings()->set_value<bool>(settings_show_smartreply, !val);
    }

    void HistoryControlPage::onSmartreplyHide()
    {
        hideSmartrepliesButton();
    }

    void HistoryControlPage::sendSmartreply(const Data::SmartreplySuggest& _suggest)
    {
        if (!isPageOpen())
            return;

        GetDispatcher()->sendSmartreplyToContact(aimId_, _suggest);
        sendSmartreplyStats(_suggest);

        setFocusOnInputWidget();
    }

    void HistoryControlPage::showActiveCallPlate()
    {
        if (!activeCallPlate_->isVisible() && pinnedWidget_->isVisible())
            pinnedWidget_->showCollapsed();

        activeCallPlate_->show();
    }

    void HistoryControlPage::updateChatInfo(const QString& _aimId, const QVector<::HistoryControl::ChatEventInfoSptr>& _events)
    {
        if (_aimId != aimId_)
            return;

        bool allSendersUpdated = false;
        if (isAdminRole_ && std::any_of(_events.begin(), _events.end(), [](const auto& event) { return event->eventType() == core::chat_event_type::generic; }))
        {
            updateSendersInfo();
            allSendersUpdated = true;
        }

        auto isMemberRelatedEvent = [](const auto& event)
        {
            using namespace core;
            constexpr chat_event_type list[]
                = { chat_event_type::mchat_add_members, chat_event_type::mchat_invite, chat_event_type::mchat_leave, chat_event_type::mchat_del_members, chat_event_type::mchat_kicked };

            return std::any_of(std::begin(list), std::end(list), [&event](auto x) { return x == event->eventType(); });
        };

        if (std::any_of(_events.begin(), _events.end(), isMemberRelatedEvent))
        {
            loadChatInfo();

            if (isAdminRole_ && !allSendersUpdated)
            {
                std::vector<QString> members;
                members.reserve(chatSenders_.size());
                for (const auto& [sender, _] : chatSenders_)
                {
                    auto containsSender = [sender=sender](const auto& event)
                    {
                        return event->getMembers().contains(sender);
                    };

                    if (std::any_of(_events.begin(), _events.end(), containsSender))
                        members.emplace_back(sender);
                }

                if (!members.empty())
                    loadChatMembersInfo(members);
            }
        }
    }

    void HistoryControlPage::onReachedFetchingDistance(bool _isMoveToBottomIfNeed)
    {
        __INFO(
            "smooth_scroll",
            "initiating messages preloading..."
        );

        //requestMoreMessagesAsync(__FUNCLINEA__, _isMoveToBottomIfNeed);

        fetch(_isMoveToBottomIfNeed ? hist::FetchDirection::toOlder : hist::FetchDirection::toNewer);
    }

    void HistoryControlPage::canFetchMore(hist::FetchDirection _direction)
    {
        if (_direction == hist::FetchDirection::toNewer)
        {
            const bool needFetchMoreToBottom = messagesArea_->needFetchMoreToBottom();
            if (needFetchMoreToBottom/* && !history_->hasFetchRequest(_direction)*/)
                fetch(_direction);
        }
        else
        {
            const bool needFetchMoreToTop = messagesArea_->needFetchMoreToTop();
            if (needFetchMoreToTop/* && !history_->hasFetchRequest(_direction)*/)
                fetch(_direction);
        }
    }

    void HistoryControlPage::showEvent(QShowEvent* _event)
    {
        if (Utils::isChat(aimId()))
            showStrangerIfNeeded();
        else
            showStatusBannerIfNeeded();

        updateFooterButtonsPositions();
        updateCallButtonsVisibility();

        PageBase::showEvent(_event);
    }

    void HistoryControlPage::resizeEvent(QResizeEvent * _event)
    {
        if (topWidget_ && header_)
            header_->resize(topWidget_->size());

        updateOverlaySizes();
        updateFooterButtonsPositions();

        if (mentionCompleter_->isVisible())
        {
            inputWidget_->updateGeometry();
            positionMentionCompleter(inputWidget_->tooltipArrowPosition());
        }

        if (smartreplyForQuotePopup_ && smartreplyForQuotePopup_->isTooltipVisible())
        {
            const auto params = getSmartreplyForQuoteParams();
            smartreplyForQuotePopup_->adjustTooltip(params.pos_, params.maxSize_, params.areaRect_);
        }

        updatePlaceholderPosition();
        updateDragOverlayGeometry();

        if (suggestWidgetShown_)
            showStickersSuggest();
    }

    bool HistoryControlPage::eventFilter(QObject* _object, QEvent* _event)
    {
        if (_event->type() == QEvent::Resize)
        {
            if (_object == messagesArea_)
            {
                updateOverlaySizes();
                updateFooterButtonsPositions();
            }
            else if (emptyArea_ && _object == emptyArea_)
            {
                updatePlaceholderPosition();
            }
        }

        return PageBase::eventFilter(_object, _event);
    }

    void HistoryControlPage::dragEnterEvent(QDragEnterEvent* _event)
    {
        const auto ignoreDrop =
            !(_event->mimeData() && (_event->mimeData()->hasUrls() || _event->mimeData()->hasImage())) ||
            _event->mimeData()->property(mimetype_marker()).toBool() ||
            QApplication::activeModalWidget() != nullptr ||
            Logic::getContactListModel()->isReadonly(aimId_) ||
            Logic::getContactListModel()->isDeleted(aimId_) ||
            Logic::GetLastseenContainer()->getLastSeen(aimId_).isBlocked() ||
            Logic::getIgnoreModel()->contains(aimId_);

        if (ignoreDrop)
        {
            _event->setDropAction(Qt::IgnoreAction);
            return;
        }

        showDragOverlay();
        if (dragOverlayWindow_)
            dragOverlayWindow_->setModeByMimeData(_event->mimeData());

        _event->acceptProposedAction();
    }

    void HistoryControlPage::dragLeaveEvent(QDragLeaveEvent* _event)
    {
        _event->accept();
    }

    void HistoryControlPage::dragMoveEvent(QDragMoveEvent* _event)
    {
        showDragOverlay();
        _event->acceptProposedAction();
    }

    void HistoryControlPage::updateFooterButtonsPositions()
    {
        const auto rect = getScrollAreaGeometry();
        const auto size = Utils::scale_value(HistoryButton::buttonSize);
        const auto x = Utils::scale_value(16 - (HistoryButton::buttonSize - HistoryButton::bubbleSize));
        auto y = Utils::scale_value(6);

        /// 0.35 is experimental offset to fully hided
        setButtonDownPositions(rect.width() - size - x, rect.bottom() + 1 - size - y, rect.bottom() + 0.35 * size - y);

        if (buttonDown_ && buttonDown_->isVisible())
            y += size;

        setButtonMentionPositions(rect.width() - size - x, rect.bottom() + 1 - size - y, rect.bottom() + 0.35 * size - y);
    }

    void HistoryControlPage::setButtonDownPositions(int x_showed, int y_showed, int y_hided)
    {
        buttonDownShowPosition_.setX(x_showed);
        buttonDownShowPosition_.setY(y_showed);

        buttonDownHidePosition_.setX(x_showed);
        buttonDownHidePosition_.setY(y_hided);

        if (buttonDown_ && buttonDown_->isVisible())
            buttonDown_->move(buttonDownShowPosition_);
    }

    void HistoryControlPage::setButtonMentionPositions(int x_showed, int y_showed, int /*y_hided*/)
    {
        if (buttonMentions_)
            buttonMentions_->move(x_showed, y_showed);
    }

    void HistoryControlPage::positionMentionCompleter(const QPoint& _atPos)
    {
        mentionCompleter_->setFixedWidth(std::min(width(), Tooltip::getMaxMentionTooltipWidth()));
        mentionCompleter_->recalcHeight();

        const auto x = (width() - mentionCompleter_->width()) / 2;
        const auto y = mentionCompleter_->parentWidget()->mapFromGlobal(_atPos).y() - mentionCompleter_->height() + Tooltip::getArrowHeight();
        mentionCompleter_->move(x, y);
    }

    void HistoryControlPage::updatePlaceholderPosition()
    {
        if (chatPlaceholder_ && emptyArea_)
        {
            chatPlaceholder_->resize(chatPlaceholder_->sizeHint());
            chatPlaceholder_->move((emptyArea_->width() - chatPlaceholder_->width()) / 2, (emptyArea_->height() - chatPlaceholder_->height()) / 2);
        }
    }

    void HistoryControlPage::setUnreadBadgeText(const QString& _text)
    {
        if (header_)
            header_->setUnreadBadgeText(_text);
    }

    MentionCompleter* HistoryControlPage::getMentionCompleter()
    {
        return mentionCompleter_;
    }

    void Ui::HistoryControlPage::updatePinMessage(const Data::MessageBuddies& _buddies)
    {
        for (auto& msg : _buddies)
        {
            if (dlgState_.pinnedMessage_ && dlgState_.pinnedMessage_->Id_ == msg->Id_)
                setPinnedMessage(msg);
        }
    }

    void HistoryControlPage::changeDlgState(const Data::DlgState& _dlgState)
    {
        if (_dlgState.AimId_ != aimId_)
            return;

        if (!_dlgState.pinnedMessage_ || !dlgState_.pinnedMessage_ || dlgState_.pinnedMessage_->Id_ != _dlgState.pinnedMessage_->Id_)
            setPinnedMessage(_dlgState.pinnedMessage_);

        reader_->onReadAllMentionsLess(_dlgState.LastReadMention_, false);
        reader_->setDlgState(_dlgState);

        if (_dlgState.UnreadCount_ && !buttonDown_)
            initButtonDown();

        if (buttonDown_)
            buttonDown_->setCounter(_dlgState.UnreadCount_);

        if (dlgState_.LastMsgId_ != _dlgState.LastMsgId_ && dlgState_.LastMsgId_ != -1 && (_dlgState.Outgoing_ || dlgState_.LastMsgId_ > _dlgState.LastMsgId_))
            hideAndClearSmartreplies();

        if (!isPageOpen() && dlgState_.YoursLastRead_ != _dlgState.YoursLastRead_ && _dlgState.UnreadCount_ == 0)
            removeNewPlateItem();

        if (Utils::isChat(aimId_))
            changeDlgStateChat(_dlgState);
        else
            changeDlgStateContact(_dlgState);

        showStrangerIfNeeded();
    }

    void HistoryControlPage::changeDlgStateChat(const Data::DlgState& _dlgState)
    {
        if (_dlgState.AimId_ != aimId_)
            return;

        const auto membersVersionChanged = dlgState_.membersVersion_ != _dlgState.membersVersion_;
        const auto infoVersionChanged = dlgState_.infoVersion_ != _dlgState.infoVersion_;

        if (infoVersionChanged || membersVersionChanged)
            loadChatInfo();

        dlgState_ = _dlgState;

        bool incomingFound = false;

        const auto& heads = heads_->headsById();

        const qint64 maxRead = heads.isEmpty() ? -1 : heads.lastKey();

        messagesArea_->enumerateWidgets([&incomingFound, maxRead](QWidget* _item, const bool)
        {
            if (qobject_cast<Ui::ServiceMessageItem*>(_item))
                return true;

            auto pageItem = qobject_cast<Ui::HistoryControlPageItem*>(_item);
            if (!pageItem)
                return true;

            auto isOutgoing = false;
            if (auto complexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_item))
            {
                isOutgoing = complexMessageItem->isOutgoingPosition();
            }
            else if (auto voipMessageItem = qobject_cast<Ui::VoipEventItem*>(_item))
            {
                isOutgoing = voipMessageItem->isOutgoingPosition();
            }
            else if (auto chatEvent = qobject_cast<Ui::ChatEventItem*>(_item))
            {
                chatEvent->setLastStatus(LastStatus::None);
                return true;
            }

            if (!isOutgoing)
                incomingFound = true;

            if (incomingFound)
            {
                pageItem->setLastStatus(LastStatus::None);
                return true;
            }

            const auto itemId = pageItem->getId();
            im_assert(itemId >= -1);

            const bool itemHasId = itemId != -1;

            const auto isItemDeliveredToPeer = itemHasId && (maxRead == -1 || itemId > maxRead);

            if (isItemDeliveredToPeer)
            {
                pageItem->setLastStatus(LastStatus::DeliveredToPeer);
                return true;
            }

            if (isOutgoing && !itemHasId)
            {
                pageItem->setLastStatus(LastStatus::Pending);
                return true;
            }

            pageItem->setLastStatus(LastStatus::None);
            return true;

        }, false);

        if (auto state = updateChatPlaceholder(); state != PlaceholderState::hidden)
        {
            if (state == PlaceholderState::rejected || state == PlaceholderState::pending || state == PlaceholderState::canceled)
            {
                if (dlgState_.HasLastMsgId())
                    reader_->onMessageItemRead(dlgState_.LastMsgId_, true);
            }
        }
    }

    void HistoryControlPage::changeDlgStateContact(const Data::DlgState& _dlgState)
    {
        if (_dlgState.AimId_ != aimId_)
            return;

        dlgState_ = _dlgState;

        Ui::HistoryControlPageItem* lastReadItem = nullptr;

        messagesArea_->enumerateWidgets([&_dlgState, &lastReadItem](QWidget* _item, const bool)
        {
            if (qobject_cast<Ui::ServiceMessageItem*>(_item) /*|| qobject_cast<Ui::ChatEventItem*>(_item)*/)
            {
                return true;
            }

            auto pageItem = qobject_cast<Ui::HistoryControlPageItem*>(_item);
            if (!pageItem)
                return true;

            auto isOutgoing = false;


            if (auto complexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(_item))
                isOutgoing = complexMessageItem->isOutgoing();
            else if (auto voipMessageItem = qobject_cast<Ui::VoipEventItem*>(_item))
                isOutgoing = voipMessageItem->isOutgoing();

            const auto itemId = pageItem->getId();
            im_assert(itemId >= -1);

            const bool isChatEvent = !!qobject_cast<Ui::ChatEventItem*>(_item);
            const bool itemHasId = itemId != -1;
            const bool isItemRead = itemHasId && (itemId <= _dlgState.TheirsLastRead_);

            const bool markItemAsLastRead = (isOutgoing && isItemRead) || (!isOutgoing && !isChatEvent);
            if (!lastReadItem && markItemAsLastRead)
            {
                lastReadItem = pageItem;
                return true;
            }

            const auto isItemDeliveredToPeer = itemHasId && (itemId <= _dlgState.TheirsLastDelivered_) && itemId > _dlgState.TheirsLastRead_;
            if (isOutgoing && isItemDeliveredToPeer)
            {
                pageItem->setLastStatus(LastStatus::DeliveredToPeer);
                return true;
            }

            const auto markItemAsLastDeliveredToServer = isOutgoing && itemHasId && itemId > _dlgState.TheirsLastRead_;
            if (markItemAsLastDeliveredToServer && !lastReadItem)
            {
                pageItem->setLastStatus(LastStatus::DeliveredToServer);
                return true;
            }

            if (isOutgoing && !itemHasId)
            {
                pageItem->setLastStatus(LastStatus::Pending);
                return true;
            }

            pageItem->setLastStatus(LastStatus::None);
            return true;

        }, false);

        if (lastReadItem)
            lastReadItem->setLastStatus(LastStatus::Read);
    }

    void HistoryControlPage::avatarMenuRequest(const QString& _aimId)
    {
        if (aimId_ == _aimId && Utils::isChat(aimId_))
            return;

        if (isRecordingPtt())
            return;

        if (isAdminRole_ && chatSenders_.find(aimId_) == chatSenders_.end())
        {
            requestWithLoaderAimId_ = _aimId;
            requestWithLoaderSequence_ = loadChatMembersInfo({ _aimId });

            QTimer::singleShot(getLoaderOverlayDelay(), this, [this, seq = requestWithLoaderSequence_]()
            {
                if (requestWithLoaderSequence_ == seq)
                    Q_EMIT Utils::InterConnector::instance().loaderOverlayShow();
            });
        }
        else
        {
            showAvatarMenu(_aimId);
        }
    }

    void HistoryControlPage::avatarMenu(QAction* _action)
    {
        const auto params = _action->data().toMap();
        const auto command = params[qsl("command")].toString();
        const auto memberAimId = params[qsl("aimid")].toString();

        bool needUpdateInfo = false;
        if (command == u"mention")
        {
            mention(memberAimId);
        }
        else if (command == u"profile")
        {
            Utils::InterConnector::instance().showSidebar(memberAimId);
        }
        else if (command == u"spam")
        {
            const auto friendly = Logic::GetFriendlyContainer()->getFriendly(memberAimId);
            if (Ui::ReportContact(memberAimId, friendly))
            {
                Logic::getContactListModel()->removeContactFromCL(memberAimId);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_contact, { { "From", "ChatAvatar_Menu" } });
            }
        }
        else if (command == u"block")
        {
            needUpdateInfo = blockUser(memberAimId, true);
        }
        else if (command == u"make_admin")
        {
            needUpdateInfo = changeRole(memberAimId, true);
        }
        else if (command == u"make_readonly")
        {
            needUpdateInfo = readonly(memberAimId, true);
        }
        else if (command == u"revoke_readonly")
        {
            needUpdateInfo = readonly(memberAimId, false);
        }
        else if (command == u"revoke_admin")
        {
            needUpdateInfo = changeRole(memberAimId, false);
        }

        if (needUpdateInfo) // update info for clicked member
            loadChatMembersInfo({ memberAimId });
    }

    bool HistoryControlPage::blockUser(const QString& _aimId, bool _blockUser)
    {
        bool confirmed = false, removeMessages = false;
        if (_blockUser)
        {
            auto w = new Ui::BlockAndDeleteWidget(nullptr, Logic::GetFriendlyContainer()->getFriendly(_aimId), aimId_);
            GeneralDialog generalDialog(w, Utils::InterConnector::instance().getMainWindow());
            generalDialog.addLabel(QT_TRANSLATE_NOOP("block_and_delete", "Delete?"));
            generalDialog.addButtonsPair(QT_TRANSLATE_NOOP("block_and_delete", "Cancel"), QT_TRANSLATE_NOOP("block_and_delete", "Yes"));
            confirmed = generalDialog.execute();
            removeMessages = w->needToRemoveMessages();
        }
        else
        {
            confirmed = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Yes"),
                QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to unblock user?"),
                Logic::GetFriendlyContainer()->getFriendly(_aimId),
                nullptr);
        }

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", aimId_);
            collection.set_value_as_qstring("contact", _aimId);
            collection.set_value_as_bool("block", _blockUser);
            collection.set_value_as_bool("remove_messages", removeMessages);
            Ui::GetDispatcher()->post_message_to_core("chats/block", collection.get());
            chatSenders_.erase(_aimId);
        }

        return confirmed;
    }

    bool HistoryControlPage::changeRole(const QString& _aimId, bool _moder)
    {
        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            _moder ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to make user admin in this chat?") : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to revoke admin role?"),
            Logic::GetFriendlyContainer()->getFriendly(_aimId),
            nullptr);

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", aimId_);
            collection.set_value_as_qstring("contact", _aimId);
            const auto isChannel = Logic::getContactListModel()->isChannel(aimId_);
            collection.set_value_as_qstring("role", _moder ? u"moder" : (isChannel ? u"readonly" : u"member"));
            Ui::GetDispatcher()->post_message_to_core("chats/role/set", collection.get());
        }

        return confirmed;
    }

    bool HistoryControlPage::readonly(const QString& _aimId, bool _readonly)
    {
        const auto action = _readonly ? Utils::ROAction::Ban : Utils::ROAction::Allow;
        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            Utils::getReadOnlyString(action, Logic::getContactListModel()->isChannel(_aimId)),
            Logic::GetFriendlyContainer()->getFriendly(_aimId),
            nullptr);

        if (confirmed)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("aimid", aimId_);
            collection.set_value_as_qstring("contact", _aimId);
            collection.set_value_as_qstring("role", _readonly ? u"readonly" : u"member");
            Ui::GetDispatcher()->post_message_to_core("chats/role/set", collection.get());
        }

        return confirmed;
    }


    void HistoryControlPage::mention(const QString& _aimId)
    {
        inputWidget_->insertMention(_aimId, Logic::GetFriendlyContainer()->getFriendly(_aimId));
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mentions_inserted, { { "From", "Chat_AvatarRightClick" } });

        setFocusOnInputWidget();
    }

    bool HistoryControlPage::isPageOpen() const
    {
        return isPageOpen_;
    }

    bool HistoryControlPage::isInBackground() const
    {
        return !isPageOpen() || !Utils::InterConnector::instance().getMainWindow()->isUIActive();
    }

    void HistoryControlPage::updateCallButtonsVisibility()
    {
        if (header_)
            header_->updateCallButtonsVisibility();
    }

    void HistoryControlPage::sendBotCommand(const QString& _command)
    {
        if (!isPageOpen() || _command.isEmpty() || !underMouse())
            return;

        const auto isChat = Utils::isChat(aimId_);
        const auto canSend = !isChat || isThread() || (Logic::getContactListModel()->contains(aimId_) && !Logic::getContactListModel()->isReadonly(aimId_));

        if (canSend)
        {
            GetDispatcher()->sendMessageToContact(aimId_, _command);
            inputWidget_->setActive(true);
        }
        else
        {
            Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("toast", "You are not a member or banned to write in this group"));
        }
    }

    void HistoryControlPage::startBot()
    {
        if (!isPageOpen())
            return;

        if (const auto nick = Logic::GetFriendlyContainer()->getNick(aimId_); !nick.isEmpty())
        {
            gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("nick", nick);
            collection.set_value_as_qstring("params", botParams_.value_or(QString()));
            Ui::GetDispatcher()->post_message_to_core("bot/start", collection.get());

            botParams_ = {};
        }
    }

    void HistoryControlPage::mentionHeads(const QString& _aimId, const QString& _friendly)
    {
        inputWidget_->insertMention(_aimId, _friendly);
        setFocusOnInputWidget();
    }

    void HistoryControlPage::chatHeads(const Data::ChatHeads& _heads)
    {
        if (_heads.aimId_ != aimId_)
            return;

        const auto reset = _heads.resetState_;
        if (reset)
        {
            messagesArea_->enumerateWidgets([](QWidget* _item, const bool)
            {
                auto pageItem = qobject_cast<Ui::HistoryControlPageItem*>(_item);
                if (!pageItem)
                    return true;

                pageItem->setHeads({});
                return true;
            }, false);
        }
        else
        {
            auto iter = _heads.heads_.begin();
            while (iter != _heads.heads_.end())
            {
                messagesArea_->enumerateWidgets([iter](QWidget* _item, const bool)
                {
                    auto pageItem = qobject_cast<Ui::HistoryControlPageItem*>(_item);
                    if (!pageItem)
                        return true;

                    if (pageItem->getId() == iter.key() && pageItem->areTheSameHeads(iter.value()))
                        return true;

                    pageItem->removeHeads(iter.value());
                    return true;
                }, false);

                ++iter;
            }
        }

        auto iter = _heads.heads_.begin();
        while (iter != _heads.heads_.end())
        {
            messagesArea_->enumerateWidgets([iter, reset](QWidget* _item, const bool)
            {
                auto pageItem = qobject_cast<Ui::HistoryControlPageItem*>(_item);
                if (!pageItem)
                    return true;

                if (pageItem->getId() != iter.key())
                    return true;

                if (reset)
                {
                    pageItem->setHeads(iter.value());
                }
                else
                {
                    if (pageItem->areTheSameHeads(iter.value()))
                        return true;

                    pageItem->addHeads(iter.value());
                }

                return true;
            }, false);

            ++iter;
        }

        changeDlgStateChat(dlgState_);
    }

    bool HistoryControlPage::contains(const QString& _aimId) const
    {
        return messagesArea_->contains(_aimId);
    }

    void HistoryControlPage::resumeVisibleItems()
    {
        im_assert(messagesArea_);
        if (messagesArea_)
            messagesArea_->resumeVisibleItems();
    }

    void HistoryControlPage::suspendVisisbleItems()
    {
        im_assert(messagesArea_);

        if (messagesArea_)
            messagesArea_->suspendVisibleItems();
    }

    void HistoryControlPage::setPrevChatButtonVisible(bool _visible)
    {
        if (header_)
            header_->setPrevChatButtonVisible(_visible);
    }

    void HistoryControlPage::setOverlayTopWidgetVisible(bool _visible)
    {
        if (header_)
            header_->setOverlayTopWidgetVisible(_visible);
    }

    void HistoryControlPage::scrollToBottomByButton()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_down_button);
        if (buttonDown_)
            buttonDown_->resetCounter();
        scrollToBottom();
    }

    void HistoryControlPage::scrollToBottom()
    {
        history_->scrollToBottom();
        messagesArea_->scrollToBottom();

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_scr_read_all_event, { { "Type", "Scroll_Button_Click" } });
    }

    void HistoryControlPage::onUpdateHistoryPosition(int32_t position, int32_t offset)
    {
        if (offset - position > Utils::scale_value(buttonDownShift))
            startShowButtonDown();
        else
            startHideButtonDown();
    }

    void HistoryControlPage::startShowButtonDown()
    {
        if (buttonDir_ != -1)
        {
            initButtonDown();

            buttonDir_ = -1;
            buttonDown_->show();

            buttonDownCurrentTime_ = QDateTime::currentMSecsSinceEpoch();

            buttonDownTimer_->stop();
            buttonDownTimer_->start();
        }
    }

    void HistoryControlPage::startHideButtonDown()
    {
        if (buttonDir_ != 1)
        {
            initButtonDown();

            buttonDir_ = 1;
            buttonDownCurrentTime_ = QDateTime::currentMSecsSinceEpoch();

            buttonDownTimer_->stop();
            buttonDownTimer_->start();

        }
    }

    void HistoryControlPage::onButtonDownMove()
    {
        if (buttonDir_ != 0)
        {
            const qint64 cur = QDateTime::currentMSecsSinceEpoch();
            const qint64 dt = cur - buttonDownCurrentTime_;
            buttonDownCurrentTime_ = cur;

            /// 16 ms = 1... 32ms = 2..
            buttonDownTime_ += (dt / 16.f) * buttonDir_ * 1.f / 15.f;
            buttonDownTime_ = std::clamp(buttonDownTime_, 0.f, 1.f);

            /// interpolation
            static const QEasingCurve curve = QEasingCurve::InSine;
            const auto val = curve.valueForProgress(buttonDownTime_);
            const QPoint p = buttonDownShowPosition_ + val * (buttonDownHidePosition_ - buttonDownShowPosition_);

            initButtonDown();
            buttonDown_->move(p);

            if (!(buttonDownTime_ > 0.f && buttonDownTime_ < 1.f))
            {
                if (buttonDir_ == 1)
                    buttonDown_->hide();

                updateFooterButtonsPositions();

                buttonDownTimer_->stop();
            }
        }
    }

    void HistoryControlPage::editBlockCanceled()
    {
        messagesArea_->enableViewportShifting(true);
        messagesArea_->resetShiftingParams();
    }

    void HistoryControlPage::onWallpaperChanged(const QString& _aimId)
    {
        if (_aimId == aimId_)
            inputWidget_->updateStyle();
    }

    static void postTypingState(const QString& _contact, const core::typing_status _state, const QString& _id = QString())
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

        collection.set_value_as_qstring("contact", _contact);
        collection.set_value_as_int("status", (int32_t)_state);
        collection.set_value_as_qstring("id", _id);

        Ui::GetDispatcher()->post_message_to_core("message/typing", collection.get());
    }

    void HistoryControlPage::onTypingTimer()
    {
        postTypingState(aimId_, core::typing_status::typed);

        prevTypingTime_ = 0;

        typedTimer_->stop();
    }

    void HistoryControlPage::onLookingTimer()
    {
        postTypingState(aimId_, core::typing_status::looking, lookingId_);
    }

    void HistoryControlPage::groupSubscribe()
    {
        GetDispatcher()->groupSubscribe(aimId_, Logic::getContactListModel()->getChatStamp(aimId_));
    }

    void HistoryControlPage::cancelGroupSubscription()
    {
        GetDispatcher()->cancelGroupSubscription(aimId_, Logic::getContactListModel()->getChatStamp(aimId_));
    }

    void HistoryControlPage::groupSubscribeResult(const int64_t _seq, int _error, int _resubscribeIn)
    {
        if (groupSubscribeNeeded())
        {
            if (!_error)
            {
                if ((groupSubscribeTimer_ && groupSubscribeTimer_->isActive()) || !isPageOpen())
                    return;

                if (!groupSubscribeTimer_)
                {
                    groupSubscribeTimer_ = new QTimer(this);
                    groupSubscribeTimer_->setSingleShot(true);
                    connect(groupSubscribeTimer_, &QTimer::timeout, this, &HistoryControlPage::groupSubscribe);
                }
                groupSubscribeTimer_->start(std::chrono::seconds(_resubscribeIn));
            }
        }
    }

    void HistoryControlPage::inputTyped()
    {
        typedTimer_->start();

        const qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

        if ((currentTime - prevTypingTime_) >= 1000)
        {
            postTypingState(aimId_, core::typing_status::typing);

            lookingTimer_->start();

            prevTypingTime_ = currentTime;
        }

        hideSmartrepliesForQuoteAnimated();
    }

    void HistoryControlPage::pageOpen()
    {
        if (isPageOpen_)
            return;

        postTypingState(aimId_, core::typing_status::looking, lookingId_);

        if (groupSubscribeNeeded())
            groupSubscribe();

        lookingTimer_->start();

        if (header_)
            header_->resumeConnectionWidget();

        isPageOpen_ = true;
        reader_->setEnabled(true);
        messagesArea_->checkVisibilityForRead();
        updateFonts();
        updateCallButtonsVisibility();

        // defer setting focus to ensure page is fully opened
        QMetaObject::invokeMethod(this, &HistoryControlPage::setFocusOnInputWidget, Qt::QueuedConnection);
    }

    void HistoryControlPage::pageLeaveStuff()
    {
        if (!isPageOpen_)
            return;

        hideStickersAndSuggests();

        isPageOpen_ = false;
        reader_->setEnabled(false);
        typedTimer_->stop();
        lookingTimer_->stop();
        if (groupSubscribeTimer_)
            groupSubscribeTimer_->stop();

        postTypingState(aimId_, core::typing_status::none);

        if (Logic::getContactListModel()->areYouNotAMember(aimId_))
            cancelGroupSubscription();

        hideDragOverlay();
        hideMentionCompleter();

        const auto& dlg = Logic::getRecentsModel()->getDlgState(aimId_);

        if (Logic::getRecentsModel()->isStranger(aimId_) && !chatscrBlockbarActionTracked_)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_blockbar_action, { { "type", "ignor" },{ "chat_type", Utils::chatTypeByAimId(aimId_) } });

        qint64 msgid = dlg.YoursLastRead_;

        reader_->onReadAllMentionsLess(msgid, false);

        newPlateShowed();

        clearSelection();
        clearPttProgress();
        Utils::InterConnector::instance().setMultiselect(false, aimId_);

        if (hasMessages() && botParams_)
            botParams_ = {};

        if (header_)
            header_->suspendConnectionWidget();

        inputWidget_->onPageLeave();
        hideSmartreplies();
        hideSuggest();
    }

    void HistoryControlPage::pageLeave()
    {
        pageLeaveStuff();
        removeNewPlateItem();
    }

    void HistoryControlPage::mainPageChanged()
    {
        pageLeaveStuff();
    }

    void HistoryControlPage::notifyApplicationWindowActive(const bool _active)
    {
    }

    void HistoryControlPage::notifyUIActive(const bool _active)
    {
        if (!Utils::InterConnector::instance().getMainWindow()->isMessengerPageContactDialog())
            return;

        lookingTimer_->stop();

        if (_active)
        {
            postTypingState(aimId_, core::typing_status::looking, lookingId_);

            lookingTimer_->start();

            resumeVisibleItems();
        }
        else
        {
            typedTimer_->stop();

            postTypingState(aimId_, core::typing_status::none);
        }
    }

    void HistoryControlPage::setPinnedMessage(Data::MessageBuddySptr _msg)
    {
        if (!_msg)
        {
            if (!pinnedWidget_->isStrangerVisible() && !pinnedWidget_->isBannerVisible())
                pinnedWidget_->hide();

            pinnedWidget_->clear();
        }
        else
        {
            if (pinnedWidget_->setMessage(std::move(_msg)))
                pinnedWidget_->showExpanded();
        }
    }

    void HistoryControlPage::onFontParamsChanged()
    {
        fontsHaveChanged_ = true;

        // if open - update, else - defer update till page opens
        if (isPageOpen())
            updateFonts();
    }

    void Ui::HistoryControlPage::onRoleChanged(const QString& _aimId)
    {
        if (aimId_ != _aimId)
            return;

        auto isAdmin = Logic::getContactListModel()->areYouAdmin(aimId_);
        if (isAdmin && !connectMessageBuddies_)
        {
            connectToMessageBuddies();
        }
        else if(!isAdmin && connectMessageBuddies_)
        {
            disconnect(connectMessageBuddies_);
            chatSenders_.clear();
        }
    }

    void Ui::HistoryControlPage::onAddReactionPlateActivityChanged(const QString& _contact, bool _active)
    {
        if (_contact == aimId_)
            addReactionPlateActive_ = _active;
    }

    void Ui::HistoryControlPage::onCallRoomInfo(const QString& _roomId, int64_t _membersCount, bool _failed)
    {
        if (_roomId != aimId_)
            return;

        if (activeCallPlate_)
        {
            activeCallPlate_->setParticipantsCount(_failed ? 0 : _membersCount);

            if (!_failed && _membersCount > 0 && !pinnedWidget_->isStrangerVisible())
                showActiveCallPlate();
            else
                activeCallPlate_->hide();
        }
    }

    void Ui::HistoryControlPage::updateSpellCheckVisibility()
    {
        if (Features::isSpellCheckEnabled())
        {
            messagesArea_->enumerateWidgets([](QWidget* _item, const bool)
            {
                if (auto pageItem = qobject_cast<Ui::HistoryControlPageItem*>(_item))
                    pageItem->setSpellErrorsVisible(pageItem->isEditable());

                return true;
            }, false);
        }
    }

    void HistoryControlPage::updateFileStatuses()
    {
        if (Logic::getContactListModel()->isTrustRequired(aimId_))
        {
            messagesArea_->enumerateWidgets([](QWidget* _item, const bool)
            {
                if (auto msg = qobject_cast<ComplexMessage::ComplexMessageItem*>(_item))
                    msg->markTrustRequired();

                return true;
            }, false);
        }
    }

    void HistoryControlPage::onThreadAdded(int64_t _seq, const Data::MessageParentTopic& _parentTopic, const QString& _threadId, int _error)
    {
        if (requestWithLoaderSequence_ != _seq)
            return;

        requestWithLoaderSequence_ = -1;
        Q_EMIT Utils::InterConnector::instance().loaderOverlayHide();

        im_assert(_parentTopic.chat_ == aimId());
        if (!_error && !_threadId.isEmpty())
        {
            Logic::getContactListModel()->markAsThread(_threadId, &_parentTopic);
            Logic::ThreadSubContainer::instance().subscribe(_threadId);
            Utils::InterConnector::instance().openThread(_threadId, _parentTopic.msgId_, aimId_);
        }
        else
        {
            Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("toast", "Error occurred while opening the replies"));
        }
    }

    void HistoryControlPage::openThread(const QString& _threadId, int64_t _msgId, const QString& _fromPage)
    {
        if (_fromPage == aimId_ && _threadId.isEmpty())
        {
            Data::MessageParentTopic topic;
            topic.chat_ = aimId();
            topic.msgId_ = _msgId;
            requestWithLoaderSequence_ = GetDispatcher()->addThread(topic);

            QTimer::singleShot(getLoaderOverlayDelay(), this, [this, seq = requestWithLoaderSequence_]()
            {
                if (requestWithLoaderSequence_ == seq)
                    Q_EMIT Utils::InterConnector::instance().loaderOverlayShow();
            });
        }
    }

    void HistoryControlPage::startPttRecording()
    {
        if (isVisible() && !isRecordingPtt() && isInputWidgetInFocus())
            startPttRecordingLock();
    }

    void HistoryControlPage::onMessageBuddies(const Data::MessageBuddies& _buddies, const QString& _aimId, Ui::MessagesBuddiesOpt _option, bool _havePending, qint64 _seq, int64_t _last_msgid)
    {
        if (aimId_ != _aimId)
            return;

        std::vector<QString> newSenders;
        newSenders.reserve(_buddies.size());

        for (const auto& msg : _buddies)
        {
            if (!msg || msg->IsChatEvent())
                continue;

            if (!msg->IsOutgoing())
            {
                auto sender = Data::normalizeAimId(msg->Chat_ ? msg->GetChatSender() : msg->AimId_);
                const auto [it, inserted] = chatSenders_.insert({ sender, nullptr });
                if (!it->second && inserted)
                    newSenders.emplace_back(std::move(sender));
            }
        }

        if (isAdminRole_ && !newSenders.empty())
            loadChatMembersInfo(newSenders);
    }

    void HistoryControlPage::onChatMemberInfo(const qint64 _seq, const std::shared_ptr<Data::ChatMembersPage>& _info)
    {
        bool isShowAvatarMenu = false;
        if (requestWithLoaderSequence_ == _seq)
        {
            requestWithLoaderSequence_ = -1;
            isShowAvatarMenu = true;
            Q_EMIT Utils::InterConnector::instance().loaderOverlayHide();
        }

        if (_info && aimId_ == _info->AimId_)
        {
            for (const auto& member : _info->Members_)
                chatSenders_[member.AimId_] = std::make_unique<Data::ChatMemberInfo>(member);
        }

        if (isShowAvatarMenu)
        {
            if (isPageOpen())
                showAvatarMenu(requestWithLoaderAimId_);
        }
    }

    void HistoryControlPage::onReadOnlyUser(const QString& _aimId, bool _isReadOnly)
    {
        if (readonly(_aimId, _isReadOnly))
            loadChatMembersInfo({ _aimId });
    }

    void HistoryControlPage::onBlockUser(const QString& _aimId, bool _isBlock)
    {
        if (blockUser(_aimId, _isBlock))
            loadChatMembersInfo({ _aimId });
    }

    void HistoryControlPage::onCreateTask(const Data::FString& _text, const Data::MentionMap& _mentions, const QString& _assignee, const bool _isThreadFeedMessage)
    {
        Q_EMIT  createTask(_text, _mentions, _assignee, _isThreadFeedMessage);
    }

    void HistoryControlPage::clearSelection()
    {
        messagesArea_->clearSelection();
    }

    void HistoryControlPage::resetShiftingParams()
    {
        messagesArea_->resetShiftingParams();
    }

    void HistoryControlPage::drawDebugRectForHole(QWidget* w)
    {
        QPainter p(this);
        p.fillRect(w->geometry(), Qt::red);
    }

    void HistoryControlPage::updateFonts()
    {
        if (!fontsHaveChanged_)
            return;

        messagesArea_->enumerateWidgets([](QWidget *widget, const bool)
        {
            if (auto item = qobject_cast<Ui::HistoryControlPageItem*>(widget))
                item->updateFonts();
            return true;

        }, false);

        messagesOverlay_->updateFonts();

        if (chatPlaceholder_)
            chatPlaceholder_->updateStyle();

        fontsHaveChanged_ = false;
    }

    QWidget* HistoryControlPage::getTopWidget() const
    {
        return topWidget_;
    }

    int HistoryControlPage::chatRoomCallParticipantsCount() const
    {
        return activeCallPlate_ ? activeCallPlate_->participantsCount() : 0;
    }

    void Ui::HistoryControlPage::clearPttProgress()
    {
        messagesArea_->clearPttProgress();
    }

    Ui::MessagesScrollArea* Ui::HistoryControlPage::scrollArea() const
    {
        return messagesArea_;
    }

    bool HistoryControlPage::hasMessageUnderCursor() const
    {
        auto isMessageBubble = [](QObject* w)
        {
            while (w)
            {
                if (qobject_cast<ComplexMessage::ComplexMessageItem*>(w))
                    return true;
                w = w->parent();
            }
            return false;
        };

        return isMessageBubble(qApp->widgetAt(QCursor::pos()));
    }

    int HistoryControlPage::getMessagesAreaHeight() const
    {
        return messagesArea_->height();
    }

    void HistoryControlPage::setFocusOnArea()
    {
        messagesArea_->setFocus();
    }

    void HistoryControlPage::clearPartialSelection()
    {
        messagesArea_->clearPartialSelection();
    }

    void HistoryControlPage::showSmartreplies()
    {
        if (!Utils::canShowSmartreplies(aimId_))
            return;

        if (!smartreplyWidget_ || smartreplyWidget_->itemCount() == 0)
            return;

        if (get_gui_settings()->get_value<bool>(settings_show_smartreply, settings_show_smartreply_default()))
        {
            messagesArea_->showSmartReplies();
            escCancel_->addChild(smartreplyWidget_, [this]() { hideSmartreplies(); });
        }

        showSmartrepliesButton();
        setSmartrepliesSemitransparent(inputWidget_->getQuotesCount() > 0 || Utils::InterConnector::instance().isMultiselect(aimId_));
    }

    void HistoryControlPage::hideSmartreplies()
    {
        messagesArea_->hideSmartReplies();
        escCancel_->removeChild(smartreplyWidget_);

        if (smartreplyButton_)
            smartreplyButton_->setArrowState(ShowHideButton::ArrowState::up);

        hideSmartrepliesForQuoteAnimated();
    }

    bool HistoryControlPage::isSmartrepliesVisible() const
    {
        return smartreplyWidget_ && smartreplyWidget_->isVisibleTo(smartreplyWidget_->parentWidget());
    }

    void HistoryControlPage::showSmartrepliesButton()
    {
        if (!smartreplyButton_)
        {
            smartreplyButton_ = new ShowHideButton(messagesArea_);
            updateSmartreplyStyle();
            messagesArea_->setSmartreplyButton(smartreplyButton_);

            connect(smartreplyButton_, &ShowHideButton::clicked, this, &HistoryControlPage::onSmartreplyButtonClicked);
        }
        smartreplyButton_->show();
        smartreplyButton_->setEnabled(true);
        smartreplyButton_->setArrowState(isSmartrepliesVisible() ? ShowHideButton::ArrowState::down : ShowHideButton::ArrowState::up);
    }

    void HistoryControlPage::hideSmartrepliesButton()
    {
        if (smartreplyButton_)
            smartreplyButton_->hide();
    }

    void HistoryControlPage::setSmartrepliesSemitransparent(bool _semi)
    {
        messagesArea_->setSmartrepliesSemitransparent(_semi);
    }

    void HistoryControlPage::notifyQuotesResize()
    {
        messagesArea_->notifyQuotesResize();
    }

    bool HistoryControlPage::hasMessages() const noexcept
    {
        return messagesArea_->hasItems();
    }

    void HistoryControlPage::setBotParameters(std::optional<QString> _parameters)
    {
        botParams_ = std::move(_parameters);
    }

    void HistoryControlPage::initChatPlaceholder()
    {
        if (!chatPlaceholder_)
        {
            initEmptyArea();
            chatPlaceholder_ = new ChatPlaceholder(emptyArea_, aimId_);
            connect(chatPlaceholder_, &ChatPlaceholder::resized, this, &HistoryControlPage::updatePlaceholderPosition);
        }
    }

    void HistoryControlPage::showChatPlaceholder(PlaceholderState _state)
    {
        initChatPlaceholder();

        chatPlaceholder_->setState(_state);

        chatPlaceholder_->show();
        chatPlaceholder_->raise();

        setEmptyAreaVisible(true);
        // queue to ensure that empty area is shown and resized properly
        QMetaObject::invokeMethod(this, &HistoryControlPage::updatePlaceholderPosition, Qt::QueuedConnection);
    }

    void HistoryControlPage::hideChatPlaceholder()
    {
        if (chatPlaceholderTimer_)
        {
            chatPlaceholderTimer_->stop();
            chatPlaceholderTimer_->deleteLater();
        }

        if (chatPlaceholder_)
        {
            chatPlaceholder_->deleteLater();
            chatPlaceholder_ = nullptr;
        }

        setEmptyAreaVisible(false);
    }

    PlaceholderState HistoryControlPage::updateChatPlaceholder()
    {
        const auto state = getPlaceholderState();
        if (state != PlaceholderState::hidden)
        {
            if (state == PlaceholderState::avatar)
            {
                if (!chatPlaceholderTimer_)
                {
                    chatPlaceholderTimer_ = new QTimer(this);
                    chatPlaceholderTimer_->setSingleShot(true);
                    chatPlaceholderTimer_->setInterval(placeholderAvatarTimeout);
                    connect(chatPlaceholderTimer_, &QTimer::timeout, this, [this]()
                    {
                        if (!hasMessages())
                            showChatPlaceholder(PlaceholderState::avatar);
                        else
                            hideChatPlaceholder();
                    });
                }

                if (!chatPlaceholderTimer_->isActive())
                {
                    initChatPlaceholder();
                    chatPlaceholder_->hide();
                    chatPlaceholderTimer_->start();
                }
            }
            else
            {
                if (chatPlaceholderTimer_)
                    chatPlaceholderTimer_->stop();

                showChatPlaceholder(state);
            }
        }
        else
        {
            hideChatPlaceholder();
        }

        return state;
    }

    PlaceholderState HistoryControlPage::getPlaceholderState() const
    {
        if (isThread())
            return PlaceholderState::hidden;

        const auto dlg = dlgState_.AimId_.isEmpty() ? hist::getDlgState(aimId_) : dlgState_;

        const auto hasHistory = dlg.HasLastMsgId();
        if (Utils::isChat(aimId_))
        {
            if (Logic::getContactListModel()->areYouBlocked(aimId_))
                return hasHistory ? PlaceholderState::hidden : PlaceholderState::avatar;

            if (hasHistory)
            {
                auto isMember = false;
                if (Logic::getContactListModel()->contains(aimId_))
                {
                    const auto role = Logic::getContactListModel()->getYourRole(aimId_);
                    isMember =
                        role == u"readonly" ||
                        role == u"member" ||
                        !(role == u"notamember" || role == u"pending" || Logic::getContactListModel()->areYouNotAMember(aimId_));
                }

                if (!isMember)
                {
                    const auto joinModEventState = [&dlg]()
                    {
                        auto msg = dlg.lastMessage_;
                        if (msg && msg->IsChatEvent())
                            if (auto event = msg->GetChatEvent(); event && event->isForMe())
                                return joinEventToPlaceholderState(event->eventType());
                        return PlaceholderState::hidden;
                    }();

                    if (joinModEventState != PlaceholderState::hidden)
                        return joinModEventState;
                    else if (Logic::getContactListModel()->isJoinModeration(aimId_))
                        return PlaceholderState::needJoin;
                    else if (!Logic::getContactListModel()->isPublic(aimId_))
                        return PlaceholderState::privateChat;
                }
            }
            else
            {
                if (!Logic::getContactListModel()->hasContact(aimId_))
                {
                    if (Logic::getContactListModel()->isJoinModeration(aimId_))
                        return PlaceholderState::needJoin;
                    else if (!Logic::getContactListModel()->isPublic(aimId_))
                        return PlaceholderState::privateChat;
                }
                return PlaceholderState::avatar;
            }
        }
        else
        {
            if (!hasHistory)
                return PlaceholderState::avatar;
        }

        return PlaceholderState::hidden;
    }

    void HistoryControlPage::initEmptyArea()
    {
        if (emptyArea_)
            return;

        emptyArea_ = new QWidget(this);
        emptyArea_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        emptyArea_->installEventFilter(this);
        if (auto pageLayout = qobject_cast<QVBoxLayout*>(layout()))
            pageLayout->insertWidget(pageLayout->indexOf(inputWidget_), emptyArea_);
    }

    void HistoryControlPage::setEmptyAreaVisible(bool _visible)
    {
        const auto messagesVisible = !_visible;
        const auto msgAreaVisChanged = messagesArea_->isVisibleTo(this) != messagesVisible;

        if constexpr (useScrollAreaContainer())
        {
            auto container = findChild<ScrollAreaContainer*>({}, Qt::FindDirectChildrenOnly);
            im_assert(container);
            if (container)
                container->setVisible(messagesVisible);
        }
        else
        {
            messagesArea_->setVisible(messagesVisible);
        }

        if (_visible)
            initEmptyArea();

        if (emptyArea_)
            emptyArea_->setVisible(_visible);

        if (msgAreaVisChanged)
        {
            if (messagesVisible)
            {
                scrollToBottom();
                fetch(hist::FetchDirection::toNewer);
            }

            messagesArea_->invalidateLayout();
        }
    }

    void HistoryControlPage::setFocusOnInputWidget()
    {
        if (GeneralDialog::isActive())
            return;

        if (inputWidget_->canSetFocus())
            inputWidget_->setFocusOnInput();
        else
            setFocusOnArea();
    }

    void HistoryControlPage::hideStickersAndSuggests(bool _animated)
    {
        if (stickerPicker_)
            _animated ? stickerPicker_->hideAnimated() : stickerPicker_->hide();
        hideSuggest(_animated);
        onSuggestHide();
    }

    void HistoryControlPage::initStickerPicker()
    {
        if (stickerPicker_)
            return;

        stickerPicker_ = new Smiles::SmilesMenu(this, aimId_);

        stickerPicker_->setCurrentHeight(0);
        stickerPicker_->setHorizontalMargins(Utils::scale_value(16));
        stickerPicker_->setAddButtonVisible(!isThread());
        Testing::setAccessibleName(stickerPicker_, qsl("AS EmojiAndStickerPicker"));

        if (auto pageLayout = qobject_cast<QVBoxLayout*>(layout()))
            pageLayout->insertWidget(pageLayout->indexOf(inputWidget_), stickerPicker_);

        connect(stickerPicker_, &Smiles::SmilesMenu::emojiSelected, inputWidget_, &InputWidget::insertEmoji);
        connect(stickerPicker_, &Smiles::SmilesMenu::scrolled, this, &HistoryControlPage::onSuggestHide);
        connect(stickerPicker_, &Smiles::SmilesMenu::visibilityChanged, this, &HistoryControlPage::onStickerPickerVisibilityChanged);
        connect(stickerPicker_, &Smiles::SmilesMenu::visibilityChanged, inputWidget_, &InputWidget::onSmilesVisibilityChanged);
        connect(stickerPicker_, &Smiles::SmilesMenu::stickerSelected, inputWidget_, &InputWidget::sendSticker);
        connect(inputWidget_, &InputWidget::sendMessage, this, [this](){ hideStickersAndSuggests();} );
        connect(
            inputWidget_,
            &InputWidget::hideSuggets,
            this,
            [this]()
            {
                hideSuggest();
                onSuggestHide();
            });
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showStickersStore, this, [this](){ hideStickersAndSuggests(false);});
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::sidebarVisibilityChanged, this, [this](bool _visible){ if (_visible) hideStickersAndSuggests(); });
        connect(inputWidget_, &InputWidget::editFocusOut, this, [this](Qt::FocusReason _reason)
        {
            if (qApp->mouseButtons() == Qt::RightButton || _reason == Qt::PopupFocusReason)
                return;
            if (_reason == Qt::MouseFocusReason)
                hideStickersAndSuggests();
        });


        connect(inputWidget_, &InputWidget::viewChanged, this, [this](){ hideStickersAndSuggests(); });

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::showStickersPicker, this, [this]()
        {
            if (!isPageOpen())
                showHideStickerPicker(ShowHideInput::Mouse, ShowHideSource::MenuButton);
        });
    }

    void HistoryControlPage::showHideStickerPicker(ShowHideInput _input, ShowHideSource _source)
    {
        initStickerPicker();

        stickerPicker_->setPreviewAreaAdditions({ 0, 0, 0, inputWidget_->height() });

        if (_source == ShowHideSource::MenuButton)
        {
            if (stickerPicker_->isHidden())
                stickerPicker_->showAnimated();
        }
        else
        {
            stickerPicker_->showHideAnimated(_input == ShowHideInput::Keyboard);
        }

        hideMentionCompleter();
        onSuggestHide();
    }

    void HistoryControlPage::onStickerPickerVisibilityChanged(bool _visible)
    {
        if (_visible)
        {
            escCancel_->addChild(stickerPicker_, [this]()
            {
                stickerPicker_->hideAnimated();
                inputWidget_->setFocusOnEmoji();
            });
        }
        else
        {
            escCancel_->removeChild(stickerPicker_);
        }
    }

    void HistoryControlPage::onSmartrepliesForQuote(const std::vector<Data::SmartreplySuggest>& _suggests)
    {
        if (!Features::isSmartreplyForQuoteEnabled() || !Utils::canShowSmartreplies(aimId()))
            return;

        if (_suggests.empty() || _suggests.front().getContact() != aimId())
            return;

        const auto& quotes = Logic::InputStateContainer::instance().getState(aimId())->getQuotes();
        if (quotes.size() != 1 || _suggests.front().getMsgId() != quotes.front().msgId_)
            return;

        if (!smartreplyForQuotePopup_)
        {
            smartreplyForQuotePopup_ = new SmartReplyForQuote(this);
            connect(smartreplyForQuotePopup_, &SmartReplyForQuote::sendSmartreply, this, &HistoryControlPage::sendSmartreplyForQuote);
        }

        smartreplyForQuotePopup_->clear();
        smartreplyForQuotePopup_->setSmartReplies(_suggests);

        const auto params = getSmartreplyForQuoteParams();
        smartreplyForQuotePopup_->showAnimated(params.pos_, params.maxSize_, params.areaRect_);
        escCancel_->addChild(smartreplyForQuotePopup_, [this]() { hideSmartrepliesForQuoteAnimated(); });
    }

    void HistoryControlPage::hideSmartrepliesForQuoteAnimated()
    {
        if (smartreplyForQuotePopup_ && smartreplyForQuotePopup_->isTooltipVisible())
            smartreplyForQuotePopup_->hideAnimated();
        escCancel_->removeChild(smartreplyForQuotePopup_);
    }

    void HistoryControlPage::hideSmartrepliesForQuoteForce()
    {
        if (smartreplyForQuotePopup_ && smartreplyForQuotePopup_->isTooltipVisible())
            smartreplyForQuotePopup_->hideForce();
        escCancel_->removeChild(smartreplyForQuotePopup_);
    }

    HistoryControlPage::SmartreplyForQuoteParams HistoryControlPage::getSmartreplyForQuoteParams() const
    {
        const auto maxSize = QSize(std::min(width(), Tooltip::getMaxMentionTooltipWidth()), height());
        auto areaRect = rect();
        if (areaRect.width() > MessageStyle::getHistoryWidgetMaxWidth())
        {
            const auto center = areaRect.center();
            areaRect.setWidth(MessageStyle::getHistoryWidgetMaxWidth());
            areaRect.moveCenter(center);
        }

        const auto x = Utils::scale_value(52);
        const auto y = inputWidget_->pos().y() + Utils::scale_value(10);
        return { QPoint(x, y), areaRect, maxSize };
    }

    void HistoryControlPage::sendSmartreplyForQuote(const Data::SmartreplySuggest& _suggest)
    {
        const auto& quotes = Logic::InputStateContainer::instance().getState(aimId())->getQuotes();
        GetDispatcher()->sendSmartreplyToContact(aimId(), _suggest, quotes);

        inputWidget_->dropReply();
        scrollToBottom();
        setFocusOnInputWidget();
    }

    void HistoryControlPage::onSuggestShow(const QString& _text, const QPoint& _pos)
    {
        lastShownSuggests_.text_ = _text;
        showStickersSuggest();

        core::stats::event_props_type props;
        const bool isEmoji = TextRendering::isEmoji(_text);

        props.push_back({ "type", isEmoji ? "emoji" : "word" });
        props.push_back({ "cont_server_suggest", inputWidget_->hasServerSuggests() ? "yes" : "no" });
        GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::stickers_suggests_appear_in_input, props);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_suggests_appear_in_input, props);
    }

    void HistoryControlPage::onSuggestHide()
    {
        hideSuggest();
        inputWidget_->clearLastSuggests();
        suggestWidgetShown_ = false;
    }

    void HistoryControlPage::onSuggestedStickerSelected(const Utils::FileSharingId& _stickerId)
    {
        hideSuggest();

        initStickerPicker();
        stickerPicker_->addStickerToRecents(-1, _stickerId);

        inputWidget_->sendSticker(_stickerId);

        sendSuggestedStickerStats(_stickerId);

        inputWidget_->clearInputText();
    }

    void HistoryControlPage::hideSuggest(bool _animated)
    {
        if (suggestsWidget_)
        {
            _animated ? suggestsWidget_->hideAnimated() : suggestsWidget_->hide();
            escCancel_->removeChild(suggestsWidget_);
        }
    }

    void HistoryControlPage::sendSuggestedStickerStats(const Utils::FileSharingId& _stickerId)
    {
        if (!suggestsWidget_ || _stickerId.fileId_.isEmpty())
            return;

        const auto inputText = inputWidget_->getInputText().string();
        const bool isEmoji = TextRendering::isEmoji(inputText);
        const bool isServerSuggest = inputWidget_->isServerSuggest(_stickerId);
        const auto& currentStickers = suggestsWidget_->getStickers();
        const auto stickerPos = std::find(currentStickers.begin(), currentStickers.end(), _stickerId);
        const auto posInStickers = stickerPos != currentStickers.end() ? std::distance(currentStickers.begin(), stickerPos) + 1 : -1;

        core::stats::event_props_type props;
        props.push_back({ "stickerId", _stickerId.fileId_.toStdString() });
        props.push_back({ "number", std::to_string(posInStickers) });
        props.push_back({ "type", isEmoji ? "emoji" : "word" });
        props.push_back({ "suggest_type", isServerSuggest ? "server" : "client" });
        GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::stickers_suggested_sticker_sent, props);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_suggested_sticker_sent, props);

        if (isServerSuggest)
            GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::text_sticker_used);
    }

    void HistoryControlPage::hideMentionCompleter()
    {
        if (mentionCompleter_)
            mentionCompleter_->hideAnimated();
    }

    void HistoryControlPage::onMentionCompleterVisibilityChanged(bool _visible)
    {
        if (_visible)
            escCancel_->addChild(mentionCompleter_, [this]() { hideMentionCompleter(); });
        else
            escCancel_->removeChild(mentionCompleter_);
    }

    void Ui::HistoryControlPage::updateWidgetsThemeImpl()
    {
        typingWidget_->updateTheme();
        mentionCompleter_->updateStyle();

        updateSmartreplyStyle();

        if (chatPlaceholder_)
            chatPlaceholder_->updateStyle();

        updateTopPanelStyle();
        pinnedWidget_->updateStyle();
        inputWidget_->updateStyle();
        inputWidget_->updateBackground();
    }

    bool HistoryControlPage::groupSubscribeNeeded() const
    {
        return Utils::isChat(aimId_) && (isThread() || Logic::getContactListModel()->areYouNotAMember(aimId_));
    }

    void HistoryControlPage::onNavigationKeyPressed(int _key, Qt::KeyboardModifiers _modifiers)
    {
        const bool applePageUp = isApplePageUp(_key, _modifiers);
        const bool applePageDown = isApplePageDown(_key, _modifiers);

        if (_key == Qt::Key_Up && !applePageUp)
        {
            if (inputWidget_->isInputEmpty() && messagesArea_->tryEditLastMessage())
                return;

            messagesArea_->scroll(ScrollDirection::UP, Utils::scale_value(scrollByKeyDelta));
        }
        else if (_key == Qt::Key_Down && !applePageDown)
        {
            messagesArea_->scroll(ScrollDirection::DOWN, Utils::scale_value(scrollByKeyDelta));
        }
        else if (isCtrlEnd(_key, _modifiers))
        {
            messagesArea_->scrollToBottom();
        }
        else if (_key == Qt::Key_PageUp || applePageUp)
        {
            messagesArea_->scroll(ScrollDirection::UP, messagesArea_->height());
        }
        else if (_key == Qt::Key_PageDown || applePageDown)
        {
            messagesArea_->scroll(ScrollDirection::DOWN, messagesArea_->height());
        }
    }

    int HistoryControlPage::getInputHeight() const
    {
        return inputWidget_->height();
    }

    bool HistoryControlPage::isInputWidgetActive() const
    {
        return isPageOpen() && (inputWidget_->isRecordingPtt() || inputWidget_->isReplying() || !inputWidget_->isInputEmpty());
    }

    bool HistoryControlPage::canSetFocusOnInput() const
    {
        return isPageOpen() && inputWidget_->canSetFocus();
    }

    void HistoryControlPage::setFocusOnInputFirstFocusable()
    {
        if (isPageOpen())
            inputWidget_->setFocusOnFirstFocusable();
    }

    bool HistoryControlPage::isRecordingPtt() const
    {
        return isPageOpen() && inputWidget_->isRecordingPtt();
    }

    bool HistoryControlPage::isPttRecordingHold() const
    {
        return isPageOpen() && inputWidget_->isPttHold();
    }

    bool HistoryControlPage::isPttRecordingHoldByKeyboard() const
    {
        return isPageOpen() && inputWidget_->isPttHoldByKeyboard();
    }

    bool HistoryControlPage::isPttRecordingHoldByMouse() const
    {
        return isPageOpen() && inputWidget_->isPttHoldByMouse();
    }

    bool HistoryControlPage::tryPlayPttRecord()
    {
        return inputWidget_->tryPlayPttRecord();
    }

    bool HistoryControlPage::tryPausePttRecord()
    {
        return inputWidget_->tryPausePttRecord();
    }

    void HistoryControlPage::closePttRecording()
    {
        if (!isPttRecordingHoldByMouse())
            inputWidget_->closePttPanel();
    }

    void HistoryControlPage::sendPttRecord()
    {
        if (!isPttRecordingHold())
            inputWidget_->sendPtt();
    }

    bool HistoryControlPage::isInputActive() const
    {
        return inputWidget_->isActive();
    }

    void HistoryControlPage::startPttRecordingLock()
    {
        if (!isPttRecordingHold())
            inputWidget_->startPttRecordingLock();
    }

    void HistoryControlPage::stopPttRecording()
    {
        inputWidget_->stopPttRecording();
    }

    void HistoryControlPage::updateDragOverlayGeometry()
    {
        if (dragOverlayWindow_)
            dragOverlayWindow_->setGeometry(rect());
    }

    void HistoryControlPage::updateDragOverlay()
    {
        if (!rect().contains(mapFromGlobal(QCursor::pos())))
            hideDragOverlay();
    }

    bool HistoryControlPage::needShowSuggests()
    {
        return suggestsWidget_ && needShowSuggests_;
    }

    void HistoryControlPage::showDragOverlay()
    {
        if (dragOverlayWindow_ && dragOverlayWindow_->isVisible())
            return;

        if (!dragOverlayWindow_)
            dragOverlayWindow_ = new DragOverlayWindow(aimId_, this);

        updateDragOverlayGeometry();
        dragOverlayWindow_->show();

        if (!overlayUpdateTimer_)
        {
            overlayUpdateTimer_ = new QTimer(this);
            connect(overlayUpdateTimer_, &QTimer::timeout, this, &HistoryControlPage::updateDragOverlay);
            connect(
                dragOverlayWindow_,
                &DragOverlayWindow::sentFilesOpened,
                this,
                [this]()
                {
                    needShowSuggests_ = false;
                    hideSuggest(false);
                    onSuggestHide();
                });
            connect(
                dragOverlayWindow_,
                &DragOverlayWindow::onQuickSent,
                this,
                [this]()
                {
                    onHideDragOverlay();
                    if (needShowSuggests())
                        update();
                });
        }
        onShowDragOverlay();

        overlayUpdateTimer_->start(overlayUpdateTimeout());

        Utils::InterConnector::instance().setDragOverlay(true);
    }

    void HistoryControlPage::onShowDragOverlay()
    {
        if (!suggestsWidget_)
            return;

        needShowSuggests_ = suggestWidgetShown_;
        suggestsWidget_->cancelAnimation();
        suggestsWidget_->hide();
        hideSuggest(false);
        onSuggestHide();
        suggestWidgetShown_ = needShowSuggests_;
    }

    void HistoryControlPage::onHideDragOverlay()
    {
        if (!needShowSuggests())
            return;
        suggestsWidget_->cancelAnimation();
        suggestsWidget_->show();
        suggestWidgetShown_ = false;
        onSuggestShow(lastShownSuggests_.text_, lastShownSuggests_.pos_);
        suggestWidgetShown_ = true;
    }

    void HistoryControlPage::hideDragOverlay()
    {
        if (!dragOverlayWindow_)
            return;
        if (dragOverlayWindow_)
            dragOverlayWindow_->hide();

        if (overlayUpdateTimer_)
            overlayUpdateTimer_->stop();

        Utils::InterConnector::instance().setDragOverlay(false);
        onHideDragOverlay();
    }

    bool HistoryControlPage::isThread() const
    {
        return Logic::getContactListModel()->isThread(aimId_);
    }

    void HistoryControlPage::showStickersSuggest()
    {
        if (!suggestsWidget_)
        {
            suggestsWidget_ = new Stickers::StickersSuggest(this);
            suggestsWidget_->setArrowVisible(true);
            connect(suggestsWidget_, &Stickers::StickersSuggest::stickerSelected, this, &HistoryControlPage::onSuggestedStickerSelected);
            connect(suggestsWidget_, &Stickers::StickersSuggest::scrolledToLastItem, inputWidget_, &InputWidget::requestMoreStickerSuggests);
        }
        
        lastShownSuggests_.pos_ = inputWidget_->suggestPosition();

        const auto fromInput = inputWidget_&& QRect(inputWidget_->mapToGlobal(inputWidget_->rect().topLeft()), inputWidget_->size()).contains(lastShownSuggests_.pos_);
        const auto maxSize = QSize(!fromInput ? width() : std::min(width(), Tooltip::getMaxMentionTooltipWidth()), height());

        const auto pt = mapFromGlobal(lastShownSuggests_.pos_);
        suggestsWidget_->setNeedScrollToTop(!inputWidget_->hasServerSuggests());

        auto areaRect = rect();
        if (areaRect.width() > MessageStyle::getHistoryWidgetMaxWidth())
        {
            const auto center = areaRect.center();
            areaRect.setWidth(MessageStyle::getHistoryWidgetMaxWidth());

            if (fromInput)
            {
                areaRect.moveCenter(center);
            }
            else
            {
                if (pt.x() < width() / 3)
                    areaRect.moveLeft(0);
                else if (pt.x() >= (width() * 2) / 3)
                    areaRect.moveRight(width());
                else
                    areaRect.moveCenter(center);
            }
        }

        if (!suggestWidgetShown_)
        {
            suggestsWidget_->showAnimated(lastShownSuggests_.text_, pt, maxSize, areaRect);
            escCancel_->addChild(suggestsWidget_, [this]() { hideSuggest(); });
            suggestWidgetShown_ = !suggestsWidget_->getStickers().empty();
        }
        else
        {
            suggestsWidget_->updateStickers(lastShownSuggests_.text_, pt, maxSize, areaRect);
        }
    }

    bool HistoryControlPage::isInputWidgetInFocus() const
    {
        return inputWidget_->isTextEditInFocus();
    }

    bool HistoryControlPage::isInputOrHistoryInFocus() const
    {
        return isInputWidgetInFocus() || Utils::InterConnector::instance().isMultiselect(aimId_);
    }
}
