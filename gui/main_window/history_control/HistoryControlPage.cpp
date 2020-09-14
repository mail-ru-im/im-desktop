#include "stdafx.h"
#include "HistoryControlPage.h"

#include "../common.shared/config/config.h"

#include "HistoryControl.h"
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
#include "selection_panel/SelectionPanel.h"
#include "ActiveCallPlate.h"
#include "../ContactDialog.h"
#include "../GroupChatOperations.h"
#include "../MainPage.h"
#include "../ReportWidget.h"
#include "../MainWindow.h"
#include "../ConnectionWidget.h"
#include "../contact_list/ChatContactsModel.h"
#include "../contact_list/RecentsTab.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/RecentsModel.h"
#include "../contact_list/UnknownsModel.h"
#include "../contact_list/IgnoreMembersModel.h"
#include "../contact_list/SelectionContactsForGroupChat.h"
#include "../contact_list/contact_profile.h"
#include "../contact_list/FavoritesUtils.h"
#include "../containers/FriendlyContainer.h"
#include "../containers/LastseenContainer.h"
#include "../sidebar/Sidebar.h"
#include "../sidebar/SidebarUtils.h"
#include "../input_widget/InputWidget.h"
#include "../../core_dispatcher.h"
#include "../../my_info.h"
#include "../../gui_settings.h"

#include "../../controls/ContextMenu.h"
#include "../../controls/LabelEx.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TooltipWidget.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/PictureWidget.h"
#include "../../controls/ClickWidget.h"
#include "../../controls/CustomButton.h"
#include "../../controls/StartCallButton.h"
#include "../../controls/GeneralDialog.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/Text2DocConverter.h"
#include "../../utils/utils.h"
#include "../../utils/log/log.h"
#include "../../utils/SChar.h"
#include "../../utils/stat_utils.h"
#include "../../utils/features.h"
#include "../../styles/ThemeParameters.h"
#include "../../styles/ThemesContainer.h"
#include "../../app_config.h"
#include "../proxy/AvatarStorageProxy.h"
#include "../proxy/FriendlyContaInerProxy.h"

#include "history/Heads.h"
#include "history/MessageReader.h"
#include "history/MessageBuilder.h"
#include "history/DateInserter.h"

#include "main_window/smartreply/SmartReplyWidget.h"
#include "main_window/smartreply/SmartReplyItem.h"
#include "main_window/smartreply/ShowHideButton.h"

#include "previewer/toast.h"

#include <boost/range/adaptor/reversed.hpp>

#ifdef __APPLE__
#include "../../utils/macos/mac_support.h"
#endif

Q_LOGGING_CATEGORY(historyPage, "historyPage")

namespace
{
    constexpr auto button_shift = 20;

    constexpr auto chatNamePaddingLeft = 16;

    bool isRemovableWidget(QWidget *w);
    bool isUpdateableWidget(QWidget *w);

    constexpr int scroll_by_key_delta = 20;

    QMap<QString, QVariant> makeData(const QString& _command, const QString& _aimId)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = _command;
        result[qsl("aimid")] = _aimId;
        return result;
    }

    constexpr std::chrono::milliseconds mentionCompleterTimeout = std::chrono::milliseconds(200);
    constexpr std::chrono::milliseconds typingInterval = std::chrono::seconds(3);
    constexpr std::chrono::milliseconds lookingInterval = std::chrono::seconds(10);
    constexpr std::chrono::milliseconds statusUpdateInterval = std::chrono::minutes(1);
    constexpr std::chrono::milliseconds placeholderAvatarTimeout = std::chrono::milliseconds(500);

#ifdef STRIP_VOIP
    constexpr bool voipEnabled = false;
#else
    constexpr bool voipEnabled = true;
#endif

    constexpr auto chatInfoMembersLimit = 5;

    bool isYouMember(std::shared_ptr<Data::ChatInfo> _info)
    {
        if (!_info)
            return false;

        return _info->YouMember_ || (!_info->YourRole_.isEmpty() && _info->YourRole_ != u"notamember" && _info->YourRole_ != u"pending");
    }

    bool isYouAdmin(std::shared_ptr<Data::ChatInfo> _info)
    {
        if (!_info)
            return false;

        return _info->YourRole_ == u"admin" || _info->Creator_ == Ui::MyInfo()->aimId();
    }

    bool isYouModer(std::shared_ptr<Data::ChatInfo> _info)
    {
        if (!_info)
            return false;

        return _info->YourRole_ == u"moder";
    }

    bool isYouAdminOrModer(std::shared_ptr<Data::ChatInfo> _info)
    {
        return isYouAdmin(_info) || isYouModer(_info);
    }

    int badgeTopOffset()
    {
        return Utils::scale_value(8);
    }

    int badgeLeftOffset()
    {
        return Utils::scale_value(28);
    }

    auto contactNameFont()
    {
        return Fonts::appFontScaled(16, platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::Normal);
    }

    auto contactStatusFont()
    {
        return Fonts::appFontScaled(14, Fonts::FontWeight::Normal);
    }

    Ui::InputWidget* getInputWidget()
    {
        if (auto contactDialog = Utils::InterConnector::instance().getContactDialog())
            return contactDialog->getInputWidget();
        return nullptr;
    }

    constexpr std::chrono::milliseconds getLoaderOverlayDelay() noexcept
    {
        return std::chrono::milliseconds(100);
    }

    auto getMinimalChatAreaWidthForVideoCallButton() noexcept
    {
        return Utils::scale_value(520);
    }

    constexpr auto getTopIconsSize() noexcept
    {
        return QSize(24, 24);
    }

    constexpr auto getTopButtonsSize() noexcept
    {
        return QSize(24, 24);
    }

    auto getTopButtonsLeftSpace() noexcept
    {
        return Utils::scale_value(12);
    }

    auto getTopButtonsSpacing() noexcept
    {
        return Utils::scale_value(16);
    }

    void sendSmartreplyStats(const Data::SmartreplySuggest& _suggest)
    {
        using namespace core::stats;

        im_stat_event_names imStatName = im_stat_event_names::min;
        stats_event_names statName = stats_event_names::min;
        event_props_type props;

        if (_suggest.isStickerType())
        {
            props.push_back({ "stickerId", _suggest.getData().toStdString() });
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

    TopWidget::TopWidget(QWidget* parent)
        : QStackedWidget(parent)
        , lastIndex_(0)
    {
        updateStyle();
    }

    void TopWidget::updateStyle()
    {
        bg_ = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
        border_ = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
        update();
    }

    void TopWidget::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        Utils::drawBackgroundWithBorders(p, rect(), bg_, border_, Qt::AlignBottom);
    }

    OverlayTopChatWidget::OverlayTopChatWidget(QWidget* _parent)
        : QWidget(_parent)
        , textUnit_(TextRendering::MakeTextUnit(QString(), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS))
    {
        setStyleSheet(qsl("background: transparent; border-style: none;"));
        setAttribute(Qt::WA_TransparentForMouseEvents);

        textUnit_->init(Fonts::appFontScaled(11), Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE), QColor(), QColor(), QColor(), TextRendering::HorAligment::CENTER);
    }

    OverlayTopChatWidget::~OverlayTopChatWidget()
    {
    }

    void OverlayTopChatWidget::setBadgeText(const QString& _text)
    {
        if (_text != text_)
        {
            text_ = _text;
            textUnit_->setText(text_);
            textUnit_->evaluateDesiredSize();
            update();
        }
    }

    void OverlayTopChatWidget::setPosition(const QPoint& _pos)
    {
        pos_ = _pos;
        update();
    }

    void OverlayTopChatWidget::paintEvent(QPaintEvent* _event)
    {
        if (!text_.isEmpty())
        {
            QPainter p(this);
            Utils::Badge::drawBadge(textUnit_, p, pos_.x(), pos_.y(), Utils::Badge::Color::Green);
        }
    }

    MessagesWidgetEventFilter::MessagesWidgetEventFilter(
        QWidget* _buttonsWidget,
        const QString& _contactName,
        ClickableTextWidget* _contactNameWidget,
        MessagesScrollArea* _scrollArea,
        ServiceMessageItem* _overlay,
        HistoryControlPage* _dialog,
        const QString& _aimId
    )
        : QObject(_dialog)
        , ButtonsWidget_(_buttonsWidget)
        , ScrollArea_(_scrollArea)
        , NewPlateShowed_(false)
        , ScrollDirectionDown_(false)
        , Dialog_(_dialog)
        , ContactNameWidget_(_contactNameWidget)
        , Overlay_(_overlay)
    {
        assert(ContactNameWidget_);
        assert(ScrollArea_);

        setContactName(_contactName.isEmpty() ? Logic::GetFriendlyContainer()->getFriendly(_aimId) : _contactName);

        ContactNameWidget_->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
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
            const bool isScrollAtBottom = ScrollArea_->isScrollAtBottom();

            const auto debugRectAvailable = GetAppConfig().IsShowMsgIdsEnabled();

            ScrollArea_->enumerateWidgets(
                [this, &firstVisibleId, &lastVisibleId, &currentPrevId, &newFound,
                &date, &dateVisible, isScrollAtBottom, debugRectAvailable, &prevItemId]
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

                auto msgItemBase = qobject_cast<Ui::HistoryControlPageItem*>(widget);

                if (!isVisible || widget->visibleRegion().isEmpty())
                {
                    return true;
                }

                if (!isScrollAtBottom)
                {
                    if (msgItemBase && !msgItemBase->isOutgoing())
                    {
                        const auto it = std::find(Dialog_->newMessageIds_.cbegin(), Dialog_->newMessageIds_.cend(), msgItemBase->getId());
                        if (it != Dialog_->newMessageIds_.cend())
                            Dialog_->newMessageIds_.erase(it);
                    }
                }

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
            bool applePageUp = (platform::is_apple() && keyEvent->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier) && keyEvent->key() == Qt::Key_Up);
            bool applePageDown = (platform::is_apple() && keyEvent->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier) && keyEvent->key() == Qt::Key_Down);
            bool applePageEnd = (platform::is_apple() && ((keyEvent->modifiers().testFlag(Qt::KeyboardModifier::MetaModifier) && keyEvent->key() == Qt::Key_Right) || keyEvent->key() == Qt::Key_End));
            if (keyEvent->matches(QKeySequence::Copy))
            {
                Dialog_->copy(QString());
                if (!Utils::InterConnector::instance().isMultiselect(Dialog_->aimId()))
                    Ui::MainPage::instance()->setFocusOnInput();
                return true;
            }
            else if (keyEvent->matches(QKeySequence::Paste) || keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
            {
                if (auto inputWidget = getInputWidget())
                    QApplication::sendEvent(inputWidget, _event);
            }
            if (keyEvent->key() == Qt::Key_Up && !applePageUp)
            {
                if (auto inputWidget = getInputWidget())
                {
                    if (inputWidget->isInputEmpty() && ScrollArea_->tryEditLastMessage())
                        return true;
                }

                ScrollArea_->scroll(ScrollDirection::UP, Utils::scale_value(scroll_by_key_delta));
            }
            else if (keyEvent->key() == Qt::Key_Down && !applePageDown)
            {
                ScrollArea_->scroll(ScrollDirection::DOWN, Utils::scale_value(scroll_by_key_delta));
            }
            else if ((keyEvent->modifiers() == Qt::CTRL && keyEvent->key() == Qt::Key_End) || applePageEnd)
            {
                Dialog_->scrollToBottom();
            }
            else if (keyEvent->key() == Qt::Key_PageUp || applePageUp)
            {
                ScrollArea_->scroll(ScrollDirection::UP, ScrollArea_->height());
            }
            else if (keyEvent->key() == Qt::Key_PageDown || applePageDown)
            {
                ScrollArea_->scroll(ScrollDirection::DOWN, ScrollArea_->height());
            }
        }

        return QObject::eventFilter(_obj, _event);
    }

    void MessagesWidgetEventFilter::setContactName(const QString& _contactName)
    {
        if (ContactName_ == _contactName)
            return;

        ContactName_ = _contactName;
        ContactNameWidget_->setText(_contactName);
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
            assert(!"unexpected state value");
            break;
        }

        return _oss;
    }

    HistoryControlPage::HistoryControlPage(QWidget* _parent, const QString& _aimId)
        : QWidget(_parent)
        , wallpaperId_(-1)
        , typingWidget_(new TypingWidget(this, _aimId))
        , smartreplyWidget_(nullptr)
        , smartreplyButton_(nullptr)
        , isContactStatusClickable_(false)
        , isMessagesRequestPostponed_(false)
        , isMessagesRequestPostponedDown_(false)
        , isPublicChat_(false)
        , chatMembersCount_(-1)
        , contactStatus_(nullptr)
        , messagesArea_(nullptr)
        , topWidgetLeftPadding_(nullptr)
        , prevChatButtonWidget_(nullptr)
        , searchButton_(nullptr)
        , addMemberButton_(nullptr)
        , callButton_(nullptr)
        , buttonsWidget_(nullptr)
        , aimId_(_aimId)
        , contactStatusTimer_(nullptr)
        , messagesOverlay_(new ServiceMessageItem(this, true))
        , state_(State::Idle)
        , topWidget_(nullptr)
        , overlayTopChatWidget_(nullptr)
        , overlayPendings_(nullptr)
        , mentionCompleter_(nullptr)
        , pinnedWidget_(new PinnedMessageWidget(this))
        , buttonDown_(nullptr)
        , buttonMentions_(nullptr)
        , buttonDir_(0)
        , buttonDownTime_(0)
        , isFetchBlocked_(false)
        , isPageOpen_(false)
        , fontsHaveChanged_(false)
        , bNewMessageForceShow_(false)
        , buttonDownCurrentTime_(0)
        , prevTypingTime_(0)
        , typedTimer_(new QTimer(this))
        , lookingTimer_(new QTimer(this))
        , mentionTimer_(new QTimer(this))
        , groupSubscribeTimer_(new QTimer(this))
        , chatscrBlockbarActionTracked_(false)
        , heads_(new Heads::HeadContainer(_aimId, this))
        , reader_(new hist::MessageReader(_aimId, this))
        , history_(new hist::History(_aimId, this))
        , control_(nullptr)
        , isAdminRole_(false)
        , isChatCreator_(false)
        , requestWithLoaderSequence_(-1)
        , avatar_(nullptr)
        , activeCallPlate_(new ActiveCallPlate(_aimId, this))
        , emptyArea_(nullptr)
        , chatPlaceholder_(nullptr)
        , chatPlaceholderTimer_(nullptr)
    {
        lookingId_ = QUuid::createUuid().toString();
        lookingId_ = lookingId_.mid(1, lookingId_.length() - 2);//remove braces

        auto layout = Utils::emptyVLayout(this);
        layout->addWidget(pinnedWidget_);
        layout->addWidget(activeCallPlate_);

        if (platform::is_apple())
        {
            auto scrollArea = new ScrollAreaContainer(this);
            auto viewport = scrollArea->viewport();

            messagesArea_ = new MessagesScrollArea(viewport, typingWidget_, new hist::DateInserter(_aimId, this), _aimId);
            connect(scrollArea, &ScrollAreaContainer::scrollViewport, messagesArea_, &MessagesScrollArea::scrollContent);

            auto areaLayout = Utils::emptyHLayout(viewport);
            areaLayout->addWidget(messagesArea_);
            layout->addWidget(scrollArea);
        }
        else
        {
            messagesArea_ = new MessagesScrollArea(this, typingWidget_, new hist::DateInserter(_aimId, this), _aimId);
            layout->addWidget(messagesArea_);
        }

        Testing::setAccessibleName(messagesArea_, qsl("AS HistoryPage messagesArea"));
        messagesArea_->getLayout()->setHeadContainer(heads_);
        messagesArea_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        messagesArea_->setFocusPolicy(Qt::ClickFocus);
        messagesArea_->installEventFilter(this);

        Testing::setAccessibleName(pinnedWidget_, qsl("AS HistoryPage pinnedWidget"));
        pinnedWidget_->hide();

        initButtonDown();
        initMentionsButton();
        initTopWidget();

        emptyArea_ = new QWidget(this);
        emptyArea_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        emptyArea_->hide();

        messagesOverlay_->setContact(_aimId);
        messagesOverlay_->raise();
        messagesOverlay_->setAttribute(Qt::WA_TransparentForMouseEvents);

        eventFilter_ = new MessagesWidgetEventFilter(
            buttonsWidget_,
            Logic::getFriendlyContainerProxy(Logic::FriendlyContainerProxy::ReplaceFavorites).getFriendly(aimId_),
            contactName_,
            messagesArea_,
            messagesOverlay_,
            this, aimId_);
        installEventFilter(eventFilter_);

        typingWidget_->setParent(messagesArea_);
        typingWidget_->hide();

        activeCallPlate_->hide();

        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalThemeChanged, this, &HistoryControlPage::onGlobalThemeChanged);

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
        connect(Logic::getContactListModel(), &Logic::ContactListModel::youRoleChanged, this, &HistoryControlPage::onChatRoleChanged);

        // QueuedConnection gives time to event loop to finish all message-related operations
        connect(this, &HistoryControlPage::postponeFetch, this, &HistoryControlPage::canFetchMore, Qt::QueuedConnection);

        connect(messagesArea_, &MessagesScrollArea::updateHistoryPosition, this, &HistoryControlPage::onUpdateHistoryPosition);
        connect(messagesArea_, &MessagesScrollArea::fetchRequestedEvent, this, &HistoryControlPage::onReachedFetchingDistance);
        connect(messagesArea_, &MessagesScrollArea::scrollMovedToBottom, this, &HistoryControlPage::scrollMovedToBottom);
        connect(messagesArea_, &MessagesScrollArea::itemRead, this, [this](qint64 _id, bool _visible)
        {
            if (control_ && control_->currentAimId() == aimId())
                reader_->onMessageItemRead(_id, _visible);
        });

        if (QApplication::clipboard()->supportsSelection())
        {
            connect(messagesArea_, &MessagesScrollArea::messagesSelected, this, [this]()
            {
                QApplication::clipboard()->setText(messagesArea_->getSelectedText(), QClipboard::Selection);
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
        groupSubscribeTimer_->setSingleShot(true);
        connect(groupSubscribeTimer_, &QTimer::timeout, this, &HistoryControlPage::groupSubscribe);

        if (auto contactDialog = Utils::InterConnector::instance().getContactDialog())
        {
            mentionCompleter_ = new MentionCompleter(contactDialog);
            if (auto inputWidget = contactDialog->getInputWidget())
            {
                mentionCompleter_->setFixedWidth(std::min(inputWidget->width(), Tooltip::getMaxMentionTooltipWidth()));
                connect(mentionCompleter_, &MentionCompleter::contactSelected, inputWidget, &InputWidget::insertMention);

                connect(contactDialog, &Ui::ContactDialog::resized, this, [this, inputWidget]()
                {
                    if (inputWidget && mentionCompleter_->isVisible())
                    {
                        inputWidget->updateGeometry();
                        positionMentionCompleter(std::move(inputWidget->tooltipArrowPosition()));
                    }
                });

                connect(inputWidget, &Ui::InputWidget::resized, this, [this, inputWidget]()
                {
                    if (inputWidget && mentionCompleter_->isVisible())
                    {
                        mentionCompleter_->setArrowPosition(std::move(inputWidget->tooltipArrowPosition()));
                    }
                });
            }
        }

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::hideMentionCompleter, mentionCompleter_, &MentionCompleter::hideAnimated);

        mentionCompleter_->setDialogAimId(aimId_);
        mentionCompleter_->hideAnimated();

        connect(mentionTimer_, &QTimer::timeout, this, [this]
        {
            updateMentionsButton();
            mentionTimer_->stop();
        });

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::connectionStateChanged, this, &HistoryControlPage::connectionStateChanged);
        if (Ui::GetDispatcher()->getConnectionState() != Ui::ConnectionState::stateOnline)
            connectionStateChanged(Ui::GetDispatcher()->getConnectionState());

        connect(Logic::GetFriendlyContainer(), &Logic::FriendlyContainer::friendlyChanged, this, [this](const QString& _aimid, const QString& _friendly)
        {
            if (_aimid == aimId_)
                updateName();
        });

        connect(Logic::GetLastseenContainer(), &Logic::LastseenContainer::lastseenChanged, this, &HistoryControlPage::lastseenChanged);

        if (Utils::isChat(aimId_))
        {
            connect(Logic::getContactListModel(), &Logic::ContactListModel::youRoleChanged, this, &HistoryControlPage::onRoleChanged);
            connectToMessageBuddies();
            GetDispatcher()->subscribeCallRoomInfo(aimId_);
        }

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::subscribeResult, this, &HistoryControlPage::groupSubscribeResult);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::addReactionPlateActivityChanged, this, &HistoryControlPage::onAddReactionPlateActivityChanged);
        connect(GetDispatcher(), &core_dispatcher::callRoomInfo, this, &HistoryControlPage::onCallRoomInfo);
    }

    void HistoryControlPage::initButtonDown()
    {
        buttonDown_ = new HistoryButton(this, qsl(":/history/to_bottom"));
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
        topWidget_ = new TopWidget(this);

        auto mainTopWidget = new QWidget(this);
        mainTopWidget->setFixedHeight(Utils::getTopPanelHeight());

        auto topLayout = Utils::emptyHLayout(mainTopWidget);
        topLayout->setContentsMargins(0, 0, Utils::scale_value(16), 0);

        {
            topWidgetLeftPadding_ = new QWidget(mainTopWidget);
            topWidgetLeftPadding_->setFixedWidth(Utils::scale_value(chatNamePaddingLeft));
            topWidgetLeftPadding_->setAttribute(Qt::WA_TransparentForMouseEvents);
            topLayout->addWidget(topWidgetLeftPadding_);

            prevChatButtonWidget_ = new QWidget(mainTopWidget);
            prevChatButtonWidget_->setFixedWidth(Utils::scale_value(getTopButtonsSize().width() + (12 * 2)));
            auto prevButtonLayout = Utils::emptyHLayout(prevChatButtonWidget_);

            auto prevChatButton = new CustomButton(prevChatButtonWidget_, qsl(":/controls/back_icon"), QSize(20, 20));
            Styling::Buttons::setButtonDefaultColors(prevChatButton);
            prevChatButton->setFixedSize(Utils::scale_value(getTopButtonsSize()));
            prevChatButton->setFocusPolicy(Qt::TabFocus);
            prevChatButton->setFocusColor(Styling::getParameters().getPrimaryTabFocusColor());
            connect(prevChatButton, &CustomButton::clicked, this, [this, prevChatButton]()
            {
                Q_EMIT switchToPrevDialog(prevChatButton->hasFocus(), QPrivateSignal());
            });

            Testing::setAccessibleName(prevChatButton, qsl("AS HistoryPage backButton"));
            prevButtonLayout->addWidget(prevChatButton);
            Testing::setAccessibleName(prevChatButtonWidget_, qsl("AS HistoryPage backButtonWidget"));
            topLayout->addWidget(prevChatButtonWidget_);
        }

        {
            const auto avatarSize = Utils::scale_value(32);
            avatar_ = new ContactAvatarWidget(mainTopWidget, aimId_, Logic::GetFriendlyContainer()->getFriendly(aimId_), avatarSize, true);
            avatar_->SetOffset(Utils::scale_value(1));
            avatar_->SetSmallBadge(true);
            avatar_->setCursor(Qt::PointingHandCursor);
            avatar_->setAvatarProxyFlags(Logic::AvatarStorageProxy::ReplaceFavorites);
            connect(avatar_, &ContactAvatarWidget::leftClicked, this, &HistoryControlPage::nameClicked);

            Testing::setAccessibleName(avatar_, qsl("AS HistoryPage dialogAvatar"));
            topLayout->addWidget(avatar_);

            topLayout->addSpacing(Utils::scale_value(8) - (avatar_->width() - avatarSize));
        }

        {
            contactWidget_ = new QWidget(mainTopWidget);
            contactWidget_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

            auto contactWidgetLayout = Utils::emptyVLayout(contactWidget_);

            contactName_ = new ClickableTextWidget(contactWidget_, contactNameFont(), Styling::StyleVariable::TEXT_SOLID);
            contactName_->setFixedHeight(Utils::scale_value(20));
            contactName_->setCursor(Qt::PointingHandCursor);
            connect(contactName_, &ClickableWidget::clicked, this, &HistoryControlPage::nameClicked);
            Testing::setAccessibleName(contactName_, qsl("AS HistoryPage dialogName"));
            contactWidgetLayout->addWidget(contactName_);

            contactStatus_ = new ClickableTextWidget(this, contactStatusFont(), Styling::StyleVariable::BASE_PRIMARY);
            contactStatus_->setFixedHeight(Utils::scale_value(16));
            Testing::setAccessibleName(contactStatus_, qsl("AS HistoryPage dialogStatus"));
            contactWidgetLayout->addWidget(contactStatus_);

            Testing::setAccessibleName(contactWidget_, qsl("AS HistoryPage dialogWidget"));
            topLayout->addWidget(contactWidget_);
        }

        {
            connectionWidget_ = new ConnectionWidget(mainTopWidget);
            connectionWidget_->setVisible(false);

            Testing::setAccessibleName(connectionWidget_, qsl("AS HistoryPage connectionWidget"));
            topLayout->addWidget(connectionWidget_);
        }

        {
            buttonsWidget_ = new QWidget(mainTopWidget);

            auto buttonsLayout = Utils::emptyHLayout(buttonsWidget_);
            buttonsLayout->addSpacing(getTopButtonsLeftSpace());
            buttonsLayout->setSpacing(getTopButtonsSpacing());

            const auto btn = [this](const QString& _iconName, const QSize& _iconSize, const QString& _tooltip, const QString& _accesibleName = QString())
            {
                auto button = new CustomButton(buttonsWidget_, _iconName, _iconSize);
                Styling::Buttons::setButtonDefaultColors(button);
                button->setFixedSize(Utils::scale_value(getTopButtonsSize()));
                button->setCustomToolTip(_tooltip);

                if (Testing::isEnabled() && !_accesibleName.isEmpty())
                    Testing::setAccessibleName(button, _accesibleName);

                return button;
            };

            searchButton_ = btn(qsl(":/search_icon"), getTopIconsSize(), QT_TRANSLATE_NOOP("tooltips", "Search for messages"), qsl("AS HistoryPage searchButton"));
            connect(searchButton_, &QPushButton::clicked, this, &HistoryControlPage::searchButtonClicked);
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::startSearchInDialog, searchButton_,
                [button = searchButton_, dialogAimId = aimId_](const QString& _searchedDialog)
            {
                if (dialogAimId == _searchedDialog)
                    button->setActive(true);
            });
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::searchClosed, searchButton_, [button = searchButton_]()
            {
                button->setActive(false);
            });
            connect(&Utils::InterConnector::instance(), &Utils::InterConnector::disableSearchInDialog, searchButton_, [button = searchButton_]()
            {
                button->setActive(false);
            });
            buttonsLayout->addWidget(searchButton_, 0, Qt::AlignRight);

            pendingButton_ = btn(qsl(":/pending_icon"), getTopIconsSize(), QT_TRANSLATE_NOOP("sidebar", "Waiting for approval"), qsl("AS HistoryPage pendingButton"));
            connect(pendingButton_, &QPushButton::clicked, this, &HistoryControlPage::pendingButtonClicked);
            buttonsLayout->addWidget(pendingButton_, 0, Qt::AlignRight);
            pendingButton_->hide();

            addMemberButton_ = btn(qsl(":/header/add_user"), getTopIconsSize(), QT_TRANSLATE_NOOP("tooltips", "Add member"), qsl("AS HistoryPage addMemberButton"));
            connect(addMemberButton_, &CustomButton::clicked, this, &HistoryControlPage::addMember);
            buttonsLayout->addWidget(addMemberButton_, 0, Qt::AlignRight);

            callButton_ = new StartCallButton(buttonsWidget_, CallButtonType::Standalone);
            callButton_->setAimId(aimId_);
            Testing::setAccessibleName(callButton_, qsl("AS HistoryPage callButton"));

            buttonsLayout->addWidget(callButton_, 0, Qt::AlignRight);

            auto moreButton = btn(qsl(":/about_icon_small"), getTopIconsSize(), QT_TRANSLATE_NOOP("tooltips", "Information"), qsl("AS HistoryPage moreButton"));
            connect(moreButton, &QPushButton::clicked, this, &HistoryControlPage::moreButtonClicked);
            buttonsLayout->addWidget(moreButton, 0, Qt::AlignRight);

            addMemberButton_->setVisible(Utils::isChat(aimId_));
            updateCallButtonsVisibility();

            Testing::setAccessibleName(buttonsWidget_, qsl("AS HistoryPage buttonsWidget"));
            topLayout->addWidget(buttonsWidget_, 0, Qt::AlignRight);
        }

        overlayTopChatWidget_ = new OverlayTopChatWidget(mainTopWidget);
        overlayPendings_ = new OverlayTopChatWidget(mainTopWidget);
        overlayPendings_->hide();

        setPrevChatButtonVisible(false);
        setContactStatusClickable(false);

        auto selectionPanel = new SelectionPanel(this);
        topWidget_->insertWidget(TopWidget::Main, mainTopWidget);
        topWidget_->insertWidget(TopWidget::Selection, selectionPanel);
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
            Q_EMIT Logic::getContactListModel()->select(aimId_, nextMention->Id_);
        }
        else
        {
            buttonMentions_->resetCounter();
            buttonMentions_->hide();
        }
    }

    void HistoryControlPage::updateMentionsButton()
    {
        const int32_t mentionsCount = reader_->getMentionsCount();

        buttonMentions_->setCounter(mentionsCount);

        const bool buttonMentionsVisible = !!mentionsCount;

        if (buttonMentions_->isVisible() != buttonMentionsVisible)
            buttonMentions_->setVisible(buttonMentionsVisible);
    }

    void HistoryControlPage::updateMentionsButtonDelayed()
    {
        if (mentionTimer_->isActive())
            mentionTimer_->stop();

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
        messagesArea_->enumerateWidgets([](QWidget *widget, const bool)
        {
            if (auto item = qobject_cast<Ui::HistoryControlPageItem*>(widget))
                item->updateStyle();
            return true;

        }, false);

        messagesOverlay_->updateStyle();
        typingWidget_->updateTheme();
        mentionCompleter_->updateStyle();

        updateSmartreplyStyle();

        if (chatPlaceholder_)
            chatPlaceholder_->updateStyle();

        Utils::ApplyStyle(this, Styling::getParameters(aimId()).getScrollBarQss());
    }

    void HistoryControlPage::updateTopPanelStyle()
    {
        topWidget_->updateStyle();

        const auto buttons = topWidget_->findChildren<CustomButton*>();
        for (auto btn : buttons)
            Styling::Buttons::setButtonDefaultColors(btn);

        contactName_->setColor(Styling::StyleVariable::TEXT_SOLID);
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
    }

    void HistoryControlPage::initFor(qint64 _id, hist::scroll_mode_type _type, FirstInit _firstInit)
    {
        assert(_type == hist::scroll_mode_type::none || _type == hist::scroll_mode_type::search || _type == hist::scroll_mode_type::unread);
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

    void HistoryControlPage::showStrangerIfNeeded()
    {
        if (!config::get().is_on((config::features::stranger_contacts)) || !Utils::isChat(aimId_))
            return;

        if (Logic::getRecentsModel()->isStranger(aimId_) && !pinnedWidget_->isStrangerVisible())
        {
            connect(pinnedWidget_, &PinnedMessageWidget::strangerBlockClicked, this, &HistoryControlPage::strangerBlock, Qt::UniqueConnection);
            connect(pinnedWidget_, &PinnedMessageWidget::strangerCloseClicked, this, &HistoryControlPage::strangerClose, Qt::UniqueConnection);
            pinnedWidget_->showStranger();
        }
        else if (!Logic::getRecentsModel()->isStranger(aimId_) && pinnedWidget_->isStrangerVisible())
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
        auto widget = messagesArea_->getItemByKey(_key);
        if (!widget)
        {
            //find in itemsData
            const auto it = std::find_if(itemsData_.begin(), itemsData_.end(), [&_key](const auto& x) { return _key == x.Key_; });
            if (it == itemsData_.end())
                return nullptr;

            widget = it->Widget_;
        }

        return widget;
    }

    QWidget* HistoryControlPage::extractWidgetByKey(const Logic::MessageKey & _key)
    {
        auto widget = messagesArea_->extractItemByKey(_key);
        if (!widget)
        {
            //find in itemsData
            const auto it = std::find_if(itemsData_.begin(), itemsData_.end(), [&_key](const auto& x) { return _key == x.Key_; });
            if (it == itemsData_.end())
                return nullptr;

            widget = it->Widget_;
            itemsData_.erase(it);
        }

        return widget;
    }

    HistoryControlPageItem* HistoryControlPage::getPageItemByKey(const Logic::MessageKey & _key) const
    {
        assert(!_key.isEmpty());

        if (auto widget = getWidgetByKey(_key))
            return qobject_cast<HistoryControlPageItem*>(widget);

        return nullptr;
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

    void HistoryControlPage::replaceExistingWidgetByKey(const Logic::MessageKey& _key, std::unique_ptr<QWidget> _widget)
    {
        assert(_key.hasId());
        assert(_widget);

        messagesArea_->replaceWidget(_key, std::move(_widget));
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
        auto result = Ui::BlockAndReport(aimId_, Logic::GetFriendlyContainer()->getFriendly(aimId_));
        if (result != BlockAndReportResult::CANCELED)
        {
            Logic::getContactListModel()->ignoreContact(aimId_, true);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_blockbar_action, { { "type", result == BlockAndReportResult::BLOCK ? "block" : "spam" },{ "chat_type", Utils::chatTypeByAimId(aimId_) } });
            chatscrBlockbarActionTracked_ = true;
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
        removeNewPlateItem();
        newPlateId_ = std::nullopt;
    }

    void HistoryControlPage::insertNewPlate()
    {
    }

    void HistoryControlPage::scrollTo(const Logic::MessageKey& key, hist::scroll_mode_type _scrollMode)
    {
        messagesArea_->scrollTo(key, _scrollMode);
    }

    void HistoryControlPage::updateItems()
    {
        messagesArea_->updateItems();
    }

    void HistoryControlPage::copy(const QString& _text)
    {
        MimeData::copyMimeData(*messagesArea_);
    }

    void HistoryControlPage::quoteText(const Data::QuotesVec& q)
    {
        const auto quotes = messagesArea_->getQuotes();
        messagesArea_->clearSelection();
        Q_EMIT quote(quotes.isEmpty() ? q : quotes);

        QTimer::singleShot(0, this, []()
        {
            if (auto inputWidget = getInputWidget(); inputWidget && inputWidget->getInputQuotes().size() != 1)
            {
                if (auto contactDialog = Utils::InterConnector::instance().getContactDialog())
                    contactDialog->hideSmartrepliesForQuoteAnimated();
            }
        });
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
        if (auto contactDialog = Utils::InterConnector::instance().getContactDialog())
            contactDialog->setFocusOnInputWidget();
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
        topWidget_->setCurrentIndex(isMultiselect ? TopWidget::Selection : TopWidget::Main);

        auto quotesCount = 0;
        if (auto inputWidget = getInputWidget())
            quotesCount = inputWidget->getQuotesCount();
        setSmartrepliesSemitransparent(isMultiselect || quotesCount > 0);

        if (isMultiselect)
            setFocusOnArea();

        Q_EMIT Utils::InterConnector::instance().updateSelectedCount();
    }

    void HistoryControlPage::multiselectDelete()
    {
        if (aimId_ != Logic::getContactListModel()->selectedContact())
            return;

        auto canDeleteForMe = 0, canDeleteForAll = 0;
        messagesArea_->countSelected(canDeleteForMe, canDeleteForAll);
        const auto showInfoText = Favorites::isFavorites(aimId_) ? Utils::DeleteMessagesWidget::ShowInfoText::No : Utils::DeleteMessagesWidget::ShowInfoText::Yes;
        auto w = new Utils::DeleteMessagesWidget(nullptr, canDeleteForAll > 0, showInfoText);
        w->setCheckBoxText(QT_TRANSLATE_NOOP("context_menu", "Delete for all"));
        w->setCheckBoxChecked(true);
        if (canDeleteForAll > 0 && showInfoText == Utils::DeleteMessagesWidget::ShowInfoText::Yes)
            w->setInfoText(QT_TRANSLATE_NOOP("delete_messages", "Messages will be deleted only for you"));

        GeneralDialog generalDialog(w, Utils::InterConnector::instance().getMainWindow());
        generalDialog.addLabel(QT_TRANSLATE_NOOP("delete_messages", "Delete messages"));
        generalDialog.addButtonsPair(QT_TRANSLATE_NOOP("delete_messages", "Cancel"), QT_TRANSLATE_NOOP("delete_messages", "OK"), true);
        if (generalDialog.showInCenter())
        {
            if (w->isChecked() && canDeleteForMe != canDeleteForAll)
            {
                const auto confirmed = Utils::GetConfirmationWithTwoButtons(
                    QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                    QT_TRANSLATE_NOOP("popup_window", "Yes"),
                    QT_TRANSLATE_NOOP("popup_window", "You can delete for all only your messages (%1 from %2). Are you sure you want to continue?").arg(canDeleteForAll).arg(canDeleteForMe),
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
            Utils::InterConnector::instance().setMultiselect(false);
        }
    }

    void HistoryControlPage::multiselectFavorites()
    {
        if (aimId_ != Logic::getContactListModel()->selectedContact())
            return;

        const auto quotes = messagesArea_->getQuotes(MessagesScrollArea::IsForward::Yes);
        addToFavorites(quotes);
        Utils::InterConnector::instance().setMultiselect(false);
    }

    void HistoryControlPage::multiselectCopy()
    {
        if (aimId_ != Logic::getContactListModel()->selectedContact())
            return;

        copy(QString());
        Utils::InterConnector::instance().setMultiselect(false);
    }

    void HistoryControlPage::multiselectReply()
    {
        if (aimId_ != Logic::getContactListModel()->selectedContact())
            return;

        quoteText({});
        Utils::InterConnector::instance().setMultiselect(false);
    }

    void HistoryControlPage::multiselectForward(const QString& _aimid)
    {
        if (aimId_ != _aimid)
            return;

        forwardText({});
    }

    void HistoryControlPage::edit(const int64_t _msgId, const QString& _internalId, const common::tools::patch_version& _patchVersion, const QString& _text, const Data::MentionMap& _mentions, const Data::QuotesVec& _quotes, qint32 _time, const Data::FilesPlaceholderMap& _files, MediaType _mediaType, bool instantEdit)
    {
        if (auto inputWidget = getInputWidget())
        {
            messagesArea_->clearSelection();
            inputWidget->edit(_msgId, _internalId, _patchVersion, _text, _mentions, _quotes, _time, _files, _mediaType, instantEdit);
        }
    }

    void HistoryControlPage::editWithCaption(const int64_t _msgId, const QString& _internalId, const common::tools::patch_version& _patchVersion, const QString& _url, const QString& _description, const Data::MentionMap& _mentions, const Data::QuotesVec& _quotes, qint32 _time, const Data::FilesPlaceholderMap& _files, MediaType _mediaType, bool instantEdit)
    {
        if (auto inputWidget = getInputWidget())
        {
            messagesArea_->clearSelection();
            inputWidget->editWithCaption(_msgId, _internalId, _patchVersion, _url, _description, _mentions, _quotes, _time, _files, _mediaType, instantEdit);
        }
    }

    void HistoryControlPage::pressedDestroyed()
    {
        messagesArea_->pressedDestroyed();
    }

    void HistoryControlPage::onItemLayoutChanged()
    {
        updateItems();
    }

    void HistoryControlPage::updateState(bool _close)
    {
        //const Data::DlgState state = getDlgState(aimId_, !_close);

        updateItems();
    }

    std::optional<qint64> HistoryControlPage::getNewPlateId() const
    {
        return newPlateId_;
    }

    static InsertHistMessagesParams::PositionWidget makeNewPlate(const QString& _aimId, qint64 _id, int _width, QWidget* _parent)
    {
        return { Logic::MessageKey(_id, Logic::control_type::ct_new_messages), hist::MessageBuilder::createNew(_aimId, _width, _parent) };
    }

    static InsertHistMessagesParams::WidgetsList makeWidgets(const QString& _aimId, const Data::MessageBuddies& _messages, int _width, Heads::HeadContainer* headsContainer, std::optional<qint64> _newPlateId, QWidget* _parent)
    {
        InsertHistMessagesParams::WidgetsList widgets;
        widgets.reserve(_messages.size() + (_newPlateId ? 1 : 0));
        for (const auto& msg : _messages)
        {
            auto item = hist::MessageBuilder::makePageItem(*msg, _width, _parent);
            if (item)
            {
                const auto itemId = item->getId();
                const auto heads = headsContainer->headsById();
                if (const auto it = heads.find(itemId); it != heads.end())
                {
                    item->setHeads(it.value());
                    item->updateSize();
                }
                widgets.emplace_back(msg->ToKey(), std::move(item));
            }
        }

        if (_newPlateId)
            widgets.push_back(makeNewPlate(_aimId, *_newPlateId, _width, _parent));

        return widgets;
    }

    void HistoryControlPage::sourceReadyHist(int64_t _mess_id, hist::scroll_mode_type _scrollMode)
    {
        if (_scrollMode == hist::scroll_mode_type::unread)
            newPlateId_ = _mess_id;
        else
            newPlateId_ = std::nullopt;

        const auto id = _mess_id > 0 ? std::make_optional(_mess_id) : std::nullopt;
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
        switchToInsertingState(__FUNCLINEA__);
        InsertHistMessagesParams params;
        params.isBackgroundMode = isInBackground();
        params.widgets = makeWidgets(aimId_, _buddies, width(), heads_, newPlateId_, messagesArea_);
        params.newPlateId = newPlateId_;
        params.updateExistingOnly = true;
        params.scrollOnInsert = std::make_optional(addReactionPlateActive_ ? ScrollOnInsert::None : ScrollOnInsert::ScrollToBottomIfNeeded);

        insertMessages(std::move(params));
        switchToIdleState(__FUNCLINEA__);

        if (!_buddies.isEmpty() && !messagesArea_->isViewportFull())
            fetch(hist::FetchDirection::toOlder);
    }

    void HistoryControlPage::insertedBuddies(const Data::MessageBuddies& _buddies)
    {
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

            assert(std::is_sorted(_buddies.begin(), _buddies.end(), [](const auto& l, const auto& r){ return l->Id_ < r->Id_; }));

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
                assert(!"can not connect to complexMessageItem");
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
                        assert(!"can not connect to complexMessageItem");
                }
                else if (auto childVoipItem = qobject_cast<const Ui::VoipEventItem*>(childWidget))
                {
                    if (!connectToVoipItem(childVoipItem))
                        assert(!"can not connect to VoipEventItem");
                }

                if (auto item = qobject_cast<const Ui::HistoryControlPageItem*>(childWidget))
                    connect(item, &HistoryControlPageItem::mention, this, &HistoryControlPage::mentionHeads);
            }
        }
    }

    bool HistoryControlPage::connectToVoipItem(const Ui::VoipEventItem* _item) const
    {
        const auto list = {
            connect(_item, &VoipEventItem::copy, this, &HistoryControlPage::copy),
            connect(_item, &VoipEventItem::quote, this, &HistoryControlPage::quoteText),
            connect(_item, &VoipEventItem::forward, this, &HistoryControlPage::forwardText),
            connect(_item, &VoipEventItem::addToFavorites, this, &HistoryControlPage::addToFavorites),
            connect(_item, &VoipEventItem::pressedDestroyed, this, &HistoryControlPage::pressedDestroyed)
        };
        return std::all_of(list.begin(), list.end(), [](const auto& x) { return x; });
    }

    void HistoryControlPage::connectToMessageBuddies()
    {
        connectMessageBuddies_ = connect(Ui::GetDispatcher(), &Ui::core_dispatcher::messageBuddies, this, &HistoryControlPage::onMessageBuddies, Qt::UniqueConnection);
    }

    bool HistoryControlPage::connectToComplexMessageItemImpl(const Ui::ComplexMessage::ComplexMessageItem* complexMessageItem) const
    {
        const auto list = {
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::copy, this, &HistoryControlPage::copy),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::quote, this, &HistoryControlPage::quoteText),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::forward, this, &HistoryControlPage::forwardText),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::avatarMenuRequest, this, &HistoryControlPage::avatarMenuRequest),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::pin, this, &HistoryControlPage::pin),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::edit, this, &HistoryControlPage::edit),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::editWithCaption, this, &HistoryControlPage::editWithCaption),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::needUpdateRecentsText, this, &HistoryControlPage::onNeedUpdateRecentsText),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::pressedDestroyed, this, &HistoryControlPage::pressedDestroyed),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::layoutChanged, this, &HistoryControlPage::onItemLayoutChanged),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::addToFavorites, this, &HistoryControlPage::addToFavorites),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::readOnlyUser, this, &HistoryControlPage::onReadOnlyUser),
            connect(complexMessageItem, &ComplexMessage::ComplexMessageItem::blockUser, this, &HistoryControlPage::onBlockUser),
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
        assert(result > WidgetRemovalResult::Min);
        assert(result < WidgetRemovalResult::Max);
    }

    void HistoryControlPage::insertMessages(InsertHistMessagesParams&& _params)
    {
        auto doInsertWidgets = [this, &_params]()
        {
            const auto isStranger = Logic::getRecentsModel()->isStranger(aimId_);
            InsertHistMessagesParams::WidgetsList insertWidgets;
            insertWidgets.reserve(_params.widgets.size());

            while (!_params.widgets.empty())
            {
                auto itemData = std::move(_params.widgets.back());
                _params.widgets.pop_back();

                if (isStranger)
                {
                    auto pageItem = qobject_cast<Ui::HistoryControlPageItem*>(itemData.second.get());
                    if (pageItem && pageItem->isOutgoing())
                    {
                        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_blockbar_action, { { "type", "send" },{ "chat_type", Utils::chatTypeByAimId(aimId_) } });
                        chatscrBlockbarActionTracked_ = true;
                    }
                }

                if (auto existing = getWidgetByKey(itemData.first))
                {
                    auto isNewItemChatEvent = qobject_cast<Ui::ChatEventItem*>(itemData.second.get());

                    if (!isUpdateableWidget(existing) && !isNewItemChatEvent)
                    {
                        auto messageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(existing);
                        auto newMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(itemData.second.get());

                        if (messageItem && newMessageItem)
                        {
                            messageItem->setDeliveredToServer(!itemData.first.isPending());
                            newMessageItem->setDeliveredToServer(!itemData.first.isPending());
                            //messageItem->updateWith(*newMessageItem);

                            connectToComplexMessageItem(newMessageItem);
                            messagesArea_->replaceWidget(itemData.first, std::move(itemData.second));
                            continue;
                        }

                        assert(false);
                        continue;
                    }

                    auto exisitingPageItem = qobject_cast<Ui::HistoryControlPageItem*>(existing);
                    auto newPageItem = qobject_cast<Ui::HistoryControlPageItem*>(itemData.second.get());

                    if (exisitingPageItem && newPageItem)
                    {
                        newPageItem->setHasAvatar(exisitingPageItem->hasAvatar());
                        newPageItem->setHasSenderName(exisitingPageItem->hasSenderName());
                    }

                    auto complexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(existing);
                    auto newComplexMessageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(itemData.second.get());

                    if (complexMessageItem && newComplexMessageItem && isUpdateableWidget(existing))
                    {
                        complexMessageItem->setDeliveredToServer(!itemData.first.isPending());
                        if (complexMessageItem->canBeUpdatedWith(*newComplexMessageItem))
                        {
                            complexMessageItem->updateWith(*newComplexMessageItem);
                        }
                        else
                        {
                            newComplexMessageItem->getUpdatableInfoFrom(*complexMessageItem);
                            connectToComplexMessageItem(newComplexMessageItem);
                            messagesArea_->replaceWidget(itemData.first, std::move(itemData.second));
                        }
                        continue;
                    }

                    auto chatEventItem = qobject_cast<Ui::ChatEventItem*>(existing);
                    auto newChatEventItem = qobject_cast<Ui::ChatEventItem*>(itemData.second.get());
                    if (chatEventItem && newChatEventItem)
                    {
                        connect(newPageItem, &HistoryControlPageItem::mention, this, &HistoryControlPage::mentionHeads);
                        messagesArea_->replaceWidget(itemData.first, std::move(itemData.second));
                        continue;
                    }

                    auto voipItem = qobject_cast<Ui::VoipEventItem*>(existing);
                    auto newVoipItem = qobject_cast<Ui::VoipEventItem*>(itemData.second.get());
                    if (voipItem && newVoipItem)
                    {
                        voipItem->updateWith(*newVoipItem);
                        auto w = messagesArea_->extractItemByKey(itemData.first);
                        messagesArea_->insertWidget(itemData.first, std::unique_ptr<QWidget>(w));
                        continue;
                    }

                    removeExistingWidgetByKey(itemData.first);
                }
                else if (_params.updateExistingOnly)
                {
                    if (itemData.first.getControlType() != Logic::control_type::ct_new_messages) // insert new plate anyway since we deleted it at start of this method
                        continue;
                }

                if (auto childVoipItem = qobject_cast<const Ui::VoipEventItem*>(itemData.second.get()))
                {
                    if (!connectToVoipItem(childVoipItem))
                        assert(!"can not connect to VoipEventItem");
                }
                else
                {
                    connectToComplexMessageItem(itemData.second.get());
                }

                if (auto item = qobject_cast<const Ui::HistoryControlPageItem*>(itemData.second.get()))
                    connect(item, &HistoryControlPageItem::mention, this, &HistoryControlPage::mentionHeads);

                insertWidgets.emplace_back(std::move(itemData));
            }

            return insertWidgets;
        };

        bool isInserted = false;
        {
            std::scoped_lock lock(*(messagesArea_->getLayout()));
            if (_params.removeOthers == RemoveOthers::Yes)
                messagesArea_->getLayout()->removeAllExcluded(_params);

            if (_params.newPlateId)
                removeNewPlateItem();

            _params.widgets = doInsertWidgets();

            isInserted = !_params.widgets.empty();
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

            assert(history_->isViewLocked());

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
        return std::stable_partition(first, last, [](const auto& x) { return x.getControlType() == Logic::control_type::ct_message; });
    }

    void HistoryControlPage::unloadTop(FromInserting _mode)
    {
        auto keys = messagesArea_->getKeysToUnloadTop();
        const auto begin = keys.begin();
        const auto it = messageKeysPartition(begin, keys.end());
        if (const auto messagesSize = std::distance(begin, it); messagesSize > 1)
        {
            std::sort(begin, it);
            const auto lastMessIt = std::prev(it);
            history_->setTopBound(*lastMessIt, _mode == FromInserting::Yes ? hist::History::CheckViewLock::No : hist::History::CheckViewLock::Yes);

            qCDebug(historyPage) << "unloadTop:";
            for (const auto& key : boost::make_iterator_range(begin, it))
                qCDebug(historyPage) << key.getId() << key.getInternalId();

            assert(std::prev(it)->getControlType() == Logic::control_type::ct_message);

            keys.erase(lastMessIt);
            cancelWidgetRequests(keys);
            removeWidgetByKeys(keys);
        }
    }

    void HistoryControlPage::unloadBottom(FromInserting _mode)
    {
        auto keys = messagesArea_->getKeysToUnloadBottom();
        const auto begin = keys.begin();
        const auto it = messageKeysPartition(begin, keys.end());
        if (const auto messagesSize = std::distance(begin, it); messagesSize > 1)
        {
            std::sort(begin, it, std::greater<>());
            const auto lastMessIt = std::prev(it);
            history_->setBottomBound(*lastMessIt, _mode == FromInserting::Yes ? hist::History::CheckViewLock::No : hist::History::CheckViewLock::Yes);
            qCDebug(historyPage) << "unloadBottom:";
            for (const auto& key : boost::make_iterator_range(begin, it))
                qCDebug(historyPage) << key.getId() << key.getInternalId();

            assert(std::prev(it)->getControlType() == Logic::control_type::ct_message);

            keys.erase(lastMessIt);
            cancelWidgetRequests(keys);
            removeWidgetByKeys(keys);
        }
    }

    void HistoryControlPage::updateMessageItems()
    {

    }

    void HistoryControlPage::removeNewPlateItem()
    {
        std::scoped_lock lock(*(messagesArea_->getLayout()));
        messagesArea_->getLayout()->removeItemsByType(Logic::control_type::ct_new_messages);
        messagesArea_->getLayout()->updateItemsProps();
        eventFilter_->resetNewPlate();
    }

    bool HistoryControlPage::touchScrollInProgress() const
    {
        return messagesArea_->touchScrollInProgress();
    }

    void HistoryControlPage::loadChatInfo()
    {
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
        const auto startTimer = [this]()
        {
            if (!contactStatusTimer_)
            {
                contactStatusTimer_ = new QTimer(this);
                contactStatusTimer_->setInterval(statusUpdateInterval);
                connect(contactStatusTimer_, &QTimer::timeout, this, &HistoryControlPage::initStatus);
            }
            if (!contactStatusTimer_->isActive())
                contactStatusTimer_->start();
        };

        const auto stopTimer = [this]()
        {
            if (contactStatusTimer_)
                contactStatusTimer_->stop();
        };

        if (Favorites::isFavorites(aimId_))
        {
            stopTimer();
            contactStatus_->hide();
        }
        else if (Utils::isChat(aimId_))
        {
            stopTimer();
            loadChatInfo();
        }
        else
        {
            const auto lastSeen = Logic::GetLastseenContainer()->getLastSeen(aimId_, Logic::LastSeenRequestType::Subscribe);

            if (!lastSeen.isValid() || lastSeen.isOnline() || lastSeen.isBlocked() || lastSeen.isAbsent() || lastSeen.isBot())
                stopTimer();
            else // offline
                startTimer();

            const auto statusText = lastSeen.getStatusString();
            if (!statusText.isEmpty())
            {
                contactStatus_->setText(statusText);
                contactStatus_->setColor(lastSeen.isOnline() ? Styling::StyleVariable::TEXT_PRIMARY : Styling::StyleVariable::BASE_PRIMARY);
            }
            contactStatus_->setVisible(!statusText.isEmpty());

            if (avatar_)
                avatar_->update();
        }
    }

    void HistoryControlPage::updateName()
    {
        updateName(Logic::getFriendlyContainerProxy(Logic::FriendlyContainerProxy::ReplaceFavorites).getFriendly(aimId_));
    }

    void Ui::HistoryControlPage::updateName(const QString& _friendly)
    {
        eventFilter_->setContactName(_friendly);
    }

    void HistoryControlPage::chatInfoFailed(qint64 _seq, core::group_chat_info_errors _errorCode, const QString& _aimId)
    {
        if (_aimId != aimId_)
            return;

        if (_errorCode == core::group_chat_info_errors::not_in_chat || _errorCode == core::group_chat_info_errors::blocked)
        {
            contactStatus_->setText(QT_TRANSLATE_NOOP("groupchats", "You are not a member of this chat"));
            Logic::getContactListModel()->setYourRole(aimId_, qsl("notamember"));
            setContactStatusClickable(false);
            addMemberButton_->hide();
            isMember_ = false;

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

        const auto youMember = isYouMember(_info);
        eventFilter_->setContactName(_info->Name_);
        setContactStatusClickable(!_info->Members_.isEmpty() && youMember);

        if (youMember && !isMember_)
            GetDispatcher()->subscribeCallRoomInfo(aimId_);

        isMember_ = youMember;

        QString stateText;
        if (youMember)
        {
            chatMembersCount_ = _info->MembersCount_;
            stateText = QString::number(_info->MembersCount_) % ql1c(' ')
                % Utils::GetTranslator()->getNumberString(
                    _info->MembersCount_,
                    QT_TRANSLATE_NOOP3("chat_page", "member", "1"),
                    QT_TRANSLATE_NOOP3("chat_page", "members", "2"),
                    QT_TRANSLATE_NOOP3("chat_page", "members", "5"),
                    QT_TRANSLATE_NOOP3("chat_page", "members", "21")
                );
        }
        else
        {
            stateText = QT_TRANSLATE_NOOP("groupchats", "You are not a member of this chat");
            chatMembersCount_ = 0;
        }
        contactStatus_->setText(stateText);
        contactStatus_->show();

        isPublicChat_ = _info->Public_;

        isChatCreator_ = isYouAdmin(_info);

        bool isAdmin = isYouAdminOrModer(_info);
        if (!isAdminRole_ && isAdmin)
            updateSendersInfo();

        if (!isAdmin && connectMessageBuddies_)
        {
            disconnect(connectMessageBuddies_);
            chatSenders_.clear();
        }

        isAdminRole_ = isAdmin;

        if (isAdmin)
        {
            const auto pendingCount = _info->PendingCount_;
            addMemberButton_->setVisible(pendingCount == 0);
            pendingButton_->setVisible(pendingCount != 0);
            overlayPendings_->resize(topWidget_->size());
            overlayPendings_->setVisible(pendingCount != 0);
            overlayPendings_->setBadgeText(Utils::getUnreadsBadgeStr(pendingCount));
        }
        else
        {
            pendingButton_->hide();
            overlayPendings_->hide();
            addMemberButton_->setVisible(youMember);
        }

        updateCallButtonsVisibility();
        updateChatPlaceholder();
        updateSpellCheckVisibility();

        if (isAdmin)
            updatePendingButtonPosition();

        if (youMember)
            Q_EMIT updateMembers();
    }

    void HistoryControlPage::contactChanged(const QString& _aimId)
    {
        if (_aimId != aimId_)
            return;

        if (Utils::isChat(aimId_))
            loadChatInfo();

        updateName();
    }

    void HistoryControlPage::onChatRoleChanged(const QString& _aimId)
    {
        if (_aimId != aimId_)
            return;

        if (Logic::getContactListModel()->isReadonly(aimId_))
            hideAndClearSmartreplies();

        updateSpellCheckVisibility();
    }

    void HistoryControlPage::editMembers()
    {
        Utils::InterConnector::instance().showMembersInSidebar(aimId_);
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
            if (!messagesArea_->isScrollAtBottom())
            {
                newMessageIds_.insert(newMessageIds_.end(), _ids.begin(), _ids.end());
                std::sort(newMessageIds_.begin(), newMessageIds_.end());
                newMessageIds_.erase(std::unique(newMessageIds_.begin(), newMessageIds_.end()), newMessageIds_.end());
            }

            // in background or after reconnecting mark messages by "new" plate
            if (isInBackground() || Ui::GetDispatcher()->getConnectionState() == Ui::ConnectionState::stateUpdating)
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
        if (auto inputWidget = getInputWidget())
            positionMentionCompleter(inputWidget->tooltipArrowPosition());

        const auto dlgstate = Logic::getRecentsModel()->getDlgState(aimId_);
        if (dlgstate.YoursLastRead_ == -1 && Logic::getRecentsModel()->isStranger(aimId_))
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_blockbar_event, { { "type", "appeared" }, { "chat_type", Utils::chatTypeByAimId(aimId_) } });

        initStatus();

        multiselectChanged();

        const auto newWallpaperId = Styling::getThemesContainer().getContactWallpaperId(aimId());
        const auto changed = (newWallpaperId.isValid() && wallpaperId_ != newWallpaperId) || (!newWallpaperId.isValid() && wallpaperId_.isValid());
        if (changed)
        {
            updateWidgetsTheme();
            wallpaperId_ = newWallpaperId;
            assert(wallpaperId_.isValid());
        }
        else
        {
            messagesOverlay_->update();
        }

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

        Q_EMIT Utils::InterConnector::instance().updateSelectedCount();
    }

    void HistoryControlPage::showMainTopPanel()
    {
        topWidget_->setCurrentIndex(TopWidget::Main);
    }

    const QString& HistoryControlPage::aimId() const
    {
        return aimId_;
    }

    void HistoryControlPage::cancelSelection()
    {
        assert(messagesArea_);
        messagesArea_->cancelSelection();
    }

    void HistoryControlPage::setHistoryControl(HistoryControl* _control)
    {
        control_ = _control;
    }

    void HistoryControlPage::deleted(const Data::MessageBuddies& _messages)
    {
        {
            std::scoped_lock lock(*(messagesArea_->getLayout()));
            for (const auto& msg : _messages)
                removeExistingWidgetByKey(msg->ToKey());

            if (messagesArea_->getLayout()->removeItemAtEnd(Logic::control_type::ct_new_messages))
                newPlateId_ = std::nullopt;

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
                    params.scrollOnInsert = std::make_optional(ScrollOnInsert::ScrollToBottomIfNeeded);

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
        newMessageIds_.clear();
        buttonDown_->resetCounter();
        scrollToBottom();
    }

    void HistoryControlPage::scrollMovedToBottom()
    {
        newMessageIds_.clear();
        buttonDown_->resetCounter();
    }

    void HistoryControlPage::autoScroll(bool _enabled)
    {
        if (!_enabled)
        {
            return;
        }
        newMessageIds_.clear();
        buttonDown_->resetCounter();
    }

    void HistoryControlPage::searchButtonClicked()
    {
        if (!searchButton_->isActive())
        {
            Q_EMIT Utils::InterConnector::instance().startSearchInDialog(aimId_);
        }
        else
        {
            Q_EMIT Utils::InterConnector::instance().searchEnd();
        }
    }

    void HistoryControlPage::pendingButtonClicked()
    {
        if (!Utils::InterConnector::instance().isSidebarVisible())
            Utils::InterConnector::instance().showSidebar(aimId_);

        Q_EMIT Utils::InterConnector::instance().showPendingMembers();
    }

    void HistoryControlPage::moreButtonClicked()
    {
        if (Utils::InterConnector::instance().isSidebarVisible())
        {
            Utils::InterConnector::instance().setSidebarVisible(Utils::SidebarVisibilityParams(false, true));
        }
        else
        {
            Utils::InterConnector::instance().showSidebar(aimId_);
        }

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::more_button_click);
    }

    void HistoryControlPage::focusOutEvent(QFocusEvent* _event)
    {
        QWidget::focusOutEvent(_event);
    }

    void HistoryControlPage::wheelEvent(QWheelEvent* _event)
    {
        if (!hasFocus())
        {
            return;
        }

        return QWidget::wheelEvent(_event);
    }

    void HistoryControlPage::addMember()
    {
        if (!Utils::isChat(aimId_))
            return;

        Logic::ChatContactsModel membersModel;
        membersModel.loadChatContacts(aimId_);

        auto selectMembersDialog = new SelectContactsWidget(&membersModel, Logic::MembersWidgetRegim::SELECT_MEMBERS,
            QT_TRANSLATE_NOOP("groupchats", "Add to chat"), QT_TRANSLATE_NOOP("popup_window", "Done"), this);

        connect(&membersModel, &Logic::ChatContactsModel::dataChanged, selectMembersDialog, &SelectContactsWidget::UpdateMembers);
        connect(this, &HistoryControlPage::updateMembers, &membersModel, [&membersModel, aimId = aimId_]() { membersModel.loadChatContacts(aimId); });

        const auto guard = QPointer(this);
        const auto res = selectMembersDialog->show();
        if (!guard)
        {
            Logic::getContactListModel()->removeTemporaryContactsFromModel();
            return;
        }

        if (res == QDialog::Accepted)
        {
            postAddChatMembersFromCLModelToCore(aimId_);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chats_add_member_dialog);
        }
        else
        {
            Logic::getContactListModel()->clearCheckedItems();
            Logic::getContactListModel()->removeTemporaryContactsFromModel();
        }

        selectMembersDialog->deleteLater();
    }

    void HistoryControlPage::renameContact()
    {
        QString result_chat_name;

        auto result = Utils::NameEditor(
            this,
            Logic::GetFriendlyContainer()->getFriendly(aimId_),
            QT_TRANSLATE_NOOP("popup_window", "Save"),
            QT_TRANSLATE_NOOP("popup_window", "Contact name"),
            result_chat_name);

        if (result && !result_chat_name.isEmpty())
        {
            Logic::getContactListModel()->renameContact(aimId_, result_chat_name);
        }
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
        assert(_state > State::Min);
        assert(_state < State::Max);
        assert(state_ > State::Min);
        assert(state_ < State::Max);

        __INFO(
            "smooth_scroll",
            "switching state\n"
            "    from=<" << state_ << ">\n"
            "    to=<" << _state << ">\n"
            "    where=<" << _dbgWhere << ">\n"
            "    items num=<" << itemsData_.size() << ">"
        );

        state_ = _state;
    }

    bool HistoryControlPage::isState(const State _state) const
    {
        assert(state_ > State::Min);
        assert(state_ < State::Max);
        assert(_state > State::Min);
        assert(_state < State::Max);

        return (state_ == _state);
    }

    bool HistoryControlPage::isStateFetching() const
    {
        return isState(State::Fetching);
    }

    bool HistoryControlPage::isStateIdle() const
    {
        return isState(State::Idle);
    }

    bool HistoryControlPage::isStateInserting() const
    {
        return isState(State::Inserting);
    }

    void HistoryControlPage::postponeMessagesRequest(const char *_dbgWhere, bool _isDown)
    {
        assert(isStateInserting());

        dbgWherePostponed_ = _dbgWhere;
        isMessagesRequestPostponed_ = true;
        isMessagesRequestPostponedDown_ = _isDown;
    }

    void HistoryControlPage::switchToIdleState(const char* _dbgWhere)
    {
        setState(State::Idle, _dbgWhere);
    }

    void HistoryControlPage::switchToInsertingState(const char* _dbgWhere)
    {
        setState(State::Inserting, _dbgWhere);
    }

    void HistoryControlPage::switchToFetchingState(const char* _dbgWhere)
    {
        setState(State::Fetching, _dbgWhere);
    }

    void HistoryControlPage::setContactStatusClickable(bool _isEnabled)
    {
        if (_isEnabled)
        {
            contactStatus_->setCursor(Qt::PointingHandCursor);
            connect(contactStatus_, &ClickableTextWidget::clicked, this, &HistoryControlPage::editMembers, Qt::UniqueConnection);
        }
        else
        {
            contactStatus_->setCursor(Qt::ArrowCursor);
            disconnect(contactStatus_, &ClickableTextWidget::clicked, this, &HistoryControlPage::editMembers);
        }
        isContactStatusClickable_ = _isEnabled;
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

        Ui::MainPage::instance()->setFocusOnInput();
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

    void HistoryControlPage::nameClicked()
    {
        if (Utils::InterConnector::instance().isSidebarVisible())
            Utils::InterConnector::instance().setSidebarVisible(false);
        else if (Utils::isChat(aimId_))
            Utils::InterConnector::instance().showSidebar(aimId_);
        else
            Q_EMIT Utils::InterConnector::instance().profileSettingsShow(aimId_);
    }

    void HistoryControlPage::showEvent(QShowEvent* _event)
    {
        showStrangerIfNeeded();
        updateFooterButtonsPositions();
        updateCallButtonsVisibility();

        QWidget::showEvent(_event);
    }

    void HistoryControlPage::resizeEvent(QResizeEvent * _event)
    {
        if (topWidget_)
        {
            // Fix pending badge counter position not being updated correctly after going fullscreen
            if (auto topWidgetRect = topWidget_->geometry(); topWidgetRect.width() != geometry().width())
            {
                topWidgetRect.setWidth(geometry().width());
                topWidget_->setGeometry(topWidgetRect);
            }
        }

        updateOverlaySizes();
        updateFooterButtonsPositions();
        overlayTopChatWidget_->resize(topWidget_->size());
        overlayTopChatWidget_->setPosition({ badgeLeftOffset(), badgeTopOffset() });

        updatePendingButtonPosition();
        updateCallButtonsVisibility();
        updatePlaceholderPosition();
    }

    bool HistoryControlPage::eventFilter(QObject* _object, QEvent* _event)
    {
        if (_object == messagesArea_ && _event->type() == QEvent::Resize)
            updateOverlaySizes();

        return QWidget::eventFilter(_object, _event);
    }

    void HistoryControlPage::updateFooterButtonsPositions()
    {
        const auto rect = getScrollAreaGeometry();
        const auto size = Utils::scale_value(HistoryButton::buttonSize);
        const auto x = Utils::scale_value(16 - (HistoryButton::buttonSize - HistoryButton::bubbleSize));
        auto y = Utils::scale_value(6);

        /// 0.35 is experimental offset to fully hided
        setButtonDownPositions(rect.width() - size - x, rect.bottom() + 1 - size - y, rect.bottom() + 0.35 * size - y);

        if (buttonDown_->isVisible())
            y += size;

        setButtonMentionPositions(rect.width() - size - x, rect.bottom() + 1 - size - y, rect.bottom() + 0.35 * size - y);
    }

    void HistoryControlPage::setButtonDownPositions(int x_showed, int y_showed, int y_hided)
    {
        buttonDownShowPosition_.setX(x_showed);
        buttonDownShowPosition_.setY(y_showed);

        buttonDownHidePosition_.setX(x_showed);
        buttonDownHidePosition_.setY(y_hided);

        if (buttonDown_->isVisible())
            buttonDown_->move(buttonDownShowPosition_);
    }

    void HistoryControlPage::setButtonMentionPositions(int x_showed, int y_showed, int /*y_hided*/)
    {
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
        if (chatPlaceholder_)
        {
            chatPlaceholder_->resize(chatPlaceholder_->sizeHint());
            chatPlaceholder_->move((width() - chatPlaceholder_->width()) / 2, (height() - chatPlaceholder_->height()) / 2);
        }
    }

    void HistoryControlPage::setUnreadBadgeText(const QString& _text)
    {
        overlayTopChatWidget_->setBadgeText(_text);
    }

    MentionCompleter* HistoryControlPage::getMentionCompleter()
    {
        return mentionCompleter_;
    }

    void HistoryControlPage::changeDlgState(const Data::DlgState& _dlgState)
    {
        if (_dlgState.AimId_ != aimId_)
            return;

        reader_->onReadAllMentionsLess(_dlgState.LastReadMention_, false);
        reader_->setDlgState(_dlgState);

        buttonDown_->setCounter(_dlgState.UnreadCount_);

        if (!messagesArea_->isScrollAtBottom() && !newPlateId_)
        {
            if (_dlgState.LastMsgId_ != _dlgState.YoursLastRead_)
                newPlateId_ = _dlgState.YoursLastRead_;
        }

        if (dlgState_.LastMsgId_ != _dlgState.LastMsgId_ && dlgState_.LastMsgId_ != -1 && (_dlgState.Outgoing_ || dlgState_.LastMsgId_ > _dlgState.LastMsgId_))
            hideAndClearSmartreplies();

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

        const auto heads = heads_->headsById();

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
            assert(itemId >= -1);

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
            assert(itemId >= -1);

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

        if (Utils::InterConnector::instance().isRecordingPtt())
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
            generalDialog.addButtonsPair(QT_TRANSLATE_NOOP("block_and_delete", "Cancel"), QT_TRANSLATE_NOOP("block_and_delete", "Yes"), true);
            confirmed = generalDialog.showInCenter();
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
        const auto confirmed = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            _readonly ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to ban user to write in this chat?") : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to allow user to write in this chat?"),
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
        if (auto contactDialog = Utils::InterConnector::instance().getContactDialog())
        {
            if (auto inputWidget = contactDialog->getInputWidget())
            {
                inputWidget->insertMention(_aimId, Logic::GetFriendlyContainer()->getFriendly(_aimId));
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mentions_inserted, { { "From", "Chat_AvatarRightClick" } });
            }
            contactDialog->setFocusOnInputWidget();
        }
    }

    bool HistoryControlPage::isPageOpen() const
    {
        return isPageOpen_;
    }

    bool HistoryControlPage::isInBackground() const
    {
        return !isPageOpen() || !Utils::InterConnector::instance().getMainWindow()->isUIActive();
    }

    void HistoryControlPage::updatePendingButtonPosition()
    {
        overlayPendings_->resize(topWidget_->size());
        auto pendingBadgeOffset = mapFromGlobal(pendingButton_->mapToGlobal(pendingButton_->rect().topRight())).x() - pendingButton_->width() / 2;
        overlayPendings_->setPosition({ pendingBadgeOffset, badgeTopOffset() });
    }

    void HistoryControlPage::updateCallButtonsVisibility()
    {
        bool isVisible = false;
        if constexpr (voipEnabled)
        {
            isVisible = GetDispatcher()->getConnectionState() == ConnectionState::stateOnline
                        && !Logic::getContactListModel()->isChannel(aimId_)
                        && !Logic::GetLastseenContainer()->isBot(aimId_)
                        && aimId_ != MyInfo()->aimId();

            if (isVisible && Utils::isChat(aimId_) && chatMembersCount_ < 2)
                isVisible = false;
        }
        callButton_->setVisible(isVisible);
    }

    void HistoryControlPage::sendBotCommand(const QString& _command)
    {
        if (!isPageOpen() || _command.isEmpty())
            return;

        if (Utils::isChat(aimId_) && (!Logic::getContactListModel()->contains(aimId_) || Logic::getContactListModel()->isReadonly(aimId_)))
            Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("toast", "You are not a member or banned to write in this group"));
        else
            GetDispatcher()->sendMessageToContact(aimId_, _command);
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
        if (auto contactDialog = Utils::InterConnector::instance().getContactDialog())
        {
            if (auto inputWidget = contactDialog->getInputWidget())
            {
                inputWidget->insertMention(_aimId, _friendly);
                contactDialog->setFocusOnInputWidget();
            }
        }
    }

    void HistoryControlPage::setRecvLastMessage(bool _value)
    {
        messagesArea_->setRecvLastMessage(_value);
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

    bool HistoryControlPage::containsWidgetWithKey(const Logic::MessageKey& _key) const
    {
        return getWidgetByKey(_key) != nullptr;
    }

    void HistoryControlPage::resumeVisibleItems()
    {
        assert(messagesArea_);
        if (messagesArea_)
        {
            messagesArea_->resumeVisibleItems();
        }
    }

    void HistoryControlPage::suspendVisisbleItems()
    {
        assert(messagesArea_);

        if (messagesArea_)
        {
            messagesArea_->suspendVisibleItems();
        }
    }

    void HistoryControlPage::scrollToBottomByButton()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_down_button);
        newMessageIds_.clear();
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
        const auto shift = Utils::scale_value(button_shift);
        if (offset - position > shift)
        {
            startShowButtonDown();
        }
        else
        {
            startHideButtonDown();
        }
    }

    void HistoryControlPage::startShowButtonDown()
    {
        if (buttonDir_ != -1)
        {
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


            buttonDown_->move(p);
            Testing::setAccessibleName(buttonDown_, qsl("AS HistoryPage buttonDown"));

            if (!(buttonDownTime_ > 0.f && buttonDownTime_ < 1.f))
            {
                if (buttonDir_ == 1)
                    buttonDown_->hide();

                updateFooterButtonsPositions();

                buttonDownTimer_->stop();
            }
        }
    }

    void HistoryControlPage::showNewMessageForce()
    {
        bNewMessageForceShow_ = true;
    }

    void HistoryControlPage::editBlockCanceled()
    {
        messagesArea_->enableViewportShifting(true);
        messagesArea_->resetShiftingParams();
    }

    void HistoryControlPage::showMentionCompleter(const QString& _initialPattern, const QPoint& _position)
    {
        positionMentionCompleter(_position);

        mentionCompleter_->setDialogAimId(aimId_);

        const auto cond = _initialPattern.isEmpty() ? Logic::MentionModel::SearchCondition::Force : Logic::MentionModel::SearchCondition::IfPatternChanged;
        mentionCompleter_->setSearchPattern(_initialPattern, cond);

        QTimer::singleShot(mentionCompleterTimeout, this, [this, _position]()
        {
            mentionCompleter_->showAnimated(_position);
        });
    }

    void HistoryControlPage::setPrevChatButtonVisible(const bool _visible)
    {
        prevChatButtonWidget_->setVisible(_visible);
        topWidgetLeftPadding_->setVisible(!_visible);
    }

    void HistoryControlPage::setOverlayTopWidgetVisible(const bool _visible)
    {
        overlayTopChatWidget_->setVisible(_visible);
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
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("stamp", Logic::getContactListModel()->getChatStamp(aimId_));
        Ui::GetDispatcher()->post_message_to_core("group/subscribe", collection.get());
    }

    void HistoryControlPage::cancelGroupSubscription()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("stamp", Logic::getContactListModel()->getChatStamp(aimId_));
        Ui::GetDispatcher()->post_message_to_core("group/cancel_subscription", collection.get());
    }

    void HistoryControlPage::groupSubscribeResult(int _error, int _resubscribeIn)
    {
        if (Logic::getContactListModel()->youAreNotAMember(aimId_))
        {
            if (!_error)
            {
                if (groupSubscribeTimer_->isActive() || !isPageOpen())
                    return;

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
    }

    void HistoryControlPage::pageOpen()
    {
        postTypingState(aimId_, core::typing_status::looking, lookingId_);

        if (Logic::getContactListModel()->youAreNotAMember(aimId_))
            groupSubscribe();

        lookingTimer_->start();

        connectionWidget_->resume();
        isPageOpen_ = true;
        reader_->setEnabled(true);
        messagesArea_->checkVisibilityForRead();
        updateFonts();
        updateCallButtonsVisibility();
    }

    void HistoryControlPage::pageLeave()
    {
        isPageOpen_ = false;
        reader_->setEnabled(false);
        typedTimer_->stop();
        lookingTimer_->stop();
        groupSubscribeTimer_->stop();

        postTypingState(aimId_, core::typing_status::none);

        if (Logic::getContactListModel()->youAreNotAMember(aimId_))
            cancelGroupSubscription();

        if (mentionCompleter_)
            mentionCompleter_->hideAnimated();

        const auto& dlg = Logic::getRecentsModel()->getDlgState(aimId_);

        if (Logic::getRecentsModel()->isStranger(aimId_) && !chatscrBlockbarActionTracked_)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_blockbar_action, { { "type", "ignor" },{ "chat_type", Utils::chatTypeByAimId(aimId_) } });

        qint64 msgid = dlg.YoursLastRead_;

        reader_->onReadAllMentionsLess(msgid, false);

        newPlateShowed();

        clearSelection();
        Utils::InterConnector::instance().setMultiselect(false, aimId_);

        if (hasMessages() && botParams_)
            botParams_ = {};

        connectionWidget_->suspend();
    }

    void HistoryControlPage::notifyApplicationWindowActive(const bool _active)
    {
    }

    void HistoryControlPage::notifyUIActive(const bool _active)
    {
        if (!Utils::InterConnector::instance().getMainWindow()->isMainPage())
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
            if (!pinnedWidget_->isStrangerVisible())
                pinnedWidget_->hide();

            pinnedWidget_->clear();
        }
        else
        {
            if (pinnedWidget_->setMessage(_msg))
                pinnedWidget_->showExpanded();
        }
    }

    void HistoryControlPage::connectionStateChanged(const Ui::ConnectionState& _state)
    {
        const auto isOnline = _state == Ui::ConnectionState::stateOnline;
        contactWidget_->setVisible(isOnline);
        connectionWidget_->setVisible(!isOnline);

        updateCallButtonsVisibility();
    }

    void HistoryControlPage::lastseenChanged(const QString& _aimid)
    {
        if (aimId_ == _aimid)
            initStatus();
    }

    void HistoryControlPage::onGlobalThemeChanged()
    {
        updateTopPanelStyle();
        pinnedWidget_->updateStyle();
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

        auto isAdmin = Logic::getContactListModel()->isYouAdmin(aimId_);
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

    void HistoryControlPage::onMessageBuddies(const Data::MessageBuddies& _buddies, const QString& _aimId, Ui::MessagesBuddiesOpt _option, bool _havePending, qint64 _seq, int64_t _last_msgid)
    {
        if (aimId_ != _aimId)
            return;

        bool isNewSenders = false;
        for (const auto& msg : _buddies)
        {
            if (msg && !msg->IsChatEvent() && !msg->IsOutgoing())
            {
                auto sender = Data::normalizeAimId(msg->Chat_ ? msg->GetChatSender() : msg->AimId_);
                const auto end = chatSenders_.end();
                auto it = std::find_if(chatSenders_.begin(), end, [&sender](const auto &x) { return x.first == sender; });
                if (it == end)
                {
                    chatSenders_.insert({ std::move(sender), nullptr });
                    isNewSenders = true;
                }
            }
        }

        if (isAdminRole_ && isNewSenders)
        {
            std::vector<QString> senders;
            senders.reserve(chatSenders_.size());
            for (const auto& [aimId, info] : chatSenders_)
            {
                if (!info)
                    senders.emplace_back(aimId);
            }

            if (!senders.empty())
                loadChatMembersInfo(senders);
        }
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
            if (Logic::getContactListModel()->selectedContact() == aimId_)
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

    QWidget* Ui::HistoryControlPage::getTopWidget() const
    {
        return topWidget_;
    }

    int Ui::HistoryControlPage::chatRoomCallParticipantsCount() const
    {
        return activeCallPlate_ ? activeCallPlate_->participantsCount() : 0;
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
            messagesArea_->showSmartReplies();

        showSmartrepliesButton();

        auto quotesCount = 0;
        if (auto inputWidget = getInputWidget())
            quotesCount = inputWidget->getInputQuotes().size();

        setSmartrepliesSemitransparent(Utils::InterConnector::instance().isMultiselect(aimId_) || quotesCount > 0);
    }

    void HistoryControlPage::hideSmartreplies()
    {
        messagesArea_->hideSmartReplies();

        if (smartreplyButton_)
            smartreplyButton_->setArrowState(ShowHideButton::ArrowState::up);

        if (auto contactDialog = Utils::InterConnector::instance().getContactDialog())
            contactDialog->hideSmartrepliesForQuoteAnimated();
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
            chatPlaceholder_ = new ChatPlaceholder(this, aimId_);
            connect(chatPlaceholder_, &ChatPlaceholder::resized, this, &HistoryControlPage::updatePlaceholderPosition);
        }
    }

    void HistoryControlPage::showChatPlaceholder(PlaceholderState _state)
    {
        initChatPlaceholder();

        chatPlaceholder_->setState(_state);

        chatPlaceholder_->show();
        chatPlaceholder_->raise();

        updatePlaceholderPosition();

        setEmptyAreaVisible(true);
    }

    void HistoryControlPage::hideChatPlaceholder()
    {
        if (chatPlaceholderTimer_)
        {
            chatPlaceholderTimer_->stop();
            chatPlaceholderTimer_->deleteLater();
            chatPlaceholderTimer_ = nullptr;
        }

        if (chatPlaceholder_)
        {
            chatPlaceholder_->deleteLater();
            chatPlaceholder_ = nullptr;

            scrollToBottom();
            fetch(hist::FetchDirection::toNewer);
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
        const auto dlg = dlgState_.AimId_.isEmpty() ? hist::getDlgState(aimId_) : dlgState_;
        assert(!dlg.AimId_.isEmpty());

        const auto hasHistory = dlg.HasLastMsgId();
        if (Utils::isChat(aimId_))
        {
            if (Logic::getContactListModel()->isYouBlocked(aimId_))
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
                        !(role == u"notamember" || role == u"pending" || Logic::getContactListModel()->youAreNotAMember(aimId_));
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

    void HistoryControlPage::setEmptyAreaVisible(bool _visible)
    {
        const auto msgAreaVisChanged = messagesArea_->isVisible() != !_visible;
        messagesArea_->setVisible(!_visible);
        emptyArea_->setVisible(_visible);

        if (msgAreaVisChanged)
            messagesArea_->invalidateLayout();
    }
}

namespace
{
    bool isRemovableWidget(QWidget *_w)
    {
        return true;
    }

    bool isUpdateableWidget(QWidget* _w)
    {
        if (const auto mi = qobject_cast<Ui::HistoryControlPageItem*>(_w))
            return mi->isUpdateable();

        return true;
    }
}
