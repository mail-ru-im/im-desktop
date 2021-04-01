#include "stdafx.h"
#include "ContactDialog.h"

#include "MainWindow.h"
#include "MainPage.h"
#include "DragOverlayWindow.h"

#include "contact_list/ContactListModel.h"
#include "history_control/HistoryControl.h"
#include "history_control/HistoryControlPage.h"
#include "history_control/MessageStyle.h"
#include "input_widget/InputWidget.h"
#include "sidebar/Sidebar.h"
#include "smiles_menu/suggests_widget.h"
#include "smiles_menu/SmilesMenu.h"
#include "core_dispatcher.h"
#include "gui_settings.h"

#include "smartreply/SmartReplyForQuote.h"

#include "utils/gui_coll_helper.h"
#include "utils/InterConnector.h"
#include "utils/features.h"
#include "controls/textrendering/TextRenderingUtils.h"
#include "controls/TooltipWidget.h"
#include "styles/ThemeParameters.h"

namespace
{
    constexpr std::chrono::milliseconds overlayUpdateTimeout() noexcept { return std::chrono::milliseconds(500); }
}

namespace Ui
{
    ContactDialog::ContactDialog(QWidget* _parent)
        : BackgroundWidget(_parent)
        , historyControlWidget_(new HistoryControl(this))
        , inputWidget_(nullptr)
        , smilesMenu_(nullptr)
        , dragOverlayWindow_(nullptr)
        , topWidget_(nullptr)
        , suggestsWidget_(nullptr)
        , frameCountMode_(FrameCountMode::_1)
        , suggestWidgetShown_(false)
        , smartreplyForQuotePopup_(nullptr)
    {
        setFrameCountMode(frameCountMode_);
        setPalette(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        setAcceptDrops(true);

        auto layout = Utils::emptyVLayout(this);

        Testing::setAccessibleName(historyControlWidget_, qsl("AS Dialog historyControlWidget"));
        layout->addWidget(historyControlWidget_);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::dialogClosed, this, &ContactDialog::removeTopWidget);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::dialogClosed, this, &ContactDialog::hideSmartrepliesForQuoteForce);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::currentPageChanged, this, &ContactDialog::hideSuggest);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::currentPageChanged, this, &ContactDialog::hideSmartrepliesForQuoteForce);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectChanged, this, &ContactDialog::onMultiselectChanged);

        connect(this, &ContactDialog::contactSelected, historyControlWidget_, &HistoryControl::contactSelected);
        connect(historyControlWidget_, &HistoryControl::setTopWidget, this, &ContactDialog::insertTopWidget);
        connect(historyControlWidget_, &HistoryControl::clicked, this, &ContactDialog::historyControlClicked, Qt::QueuedConnection);
        connect(historyControlWidget_, &HistoryControl::needUpdateWallpaper, this, &BackgroundWidget::updateWallpaper);

        updateWallpaper(QString());

        overlayUpdateTimer_.setInterval(overlayUpdateTimeout());
        connect(&overlayUpdateTimer_, &QTimer::timeout, this, &ContactDialog::updateDragOverlay);

        connect(Utils::InterConnector::instance().getMainWindow(), &MainWindow::mouseReleased, this, [this]()
        {
            if (!rect().contains(mapFromGlobal(QCursor::pos())) && !isPttRecordingHold())
                Q_EMIT Utils::InterConnector::instance().stopPttRecord();
        });

        connect(Utils::InterConnector::instance().getMainWindow(), &MainWindow::mousePressed, this, [this]()
        {
            if (isPttRecordingHoldByKeyboard())
            {
                Q_EMIT Utils::InterConnector::instance().stopPttRecord();
            }
            else if (const auto buttons = qApp->mouseButtons(); buttons != Qt::MouseButtons(Qt::LeftButton))
            {
                if (!historyControlWidget_->hasMessageUnderCursor())
                    Q_EMIT Utils::InterConnector::instance().stopPttRecord();
            }
        });

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::smartreplyRequestedResult, this, &ContactDialog::onSmartrepliesForQuote);
    }

    ContactDialog::~ContactDialog() = default;

    void ContactDialog::onContactSelected(const QString& _aimId, qint64 _messageId, const highlightsV& _highlights, bool _ignoreScroll)
    {
        initInputWidget();
        hideSmartrepliesForQuoteForce();

        Q_EMIT contactSelected(_aimId, _messageId, _highlights, _ignoreScroll);

        // defer setting focus to ensure page is already opened
        QMetaObject::invokeMethod(this, &ContactDialog::setFocusOnInputWidget, Qt::QueuedConnection);
    }

    void ContactDialog::onContactSelectedToLastMessage(const QString& _aimId, qint64 _messageId)
    {
        initInputWidget();
        hideSmartrepliesForQuoteForce();

        Q_EMIT contactSelectedToLastMessage(_aimId, _messageId);

        // defer setting focus to ensure page is already opened
        QMetaObject::invokeMethod(this, &ContactDialog::setFocusOnInputWidget, Qt::QueuedConnection);
    }

    void ContactDialog::initTopWidget()
    {
        if (topWidget_)
            return;

        topWidget_ = new QStackedWidget(this);
        topWidget_->setFixedHeight(Utils::getTopPanelHeight());
        Testing::setAccessibleName(topWidget_, qsl("AS Dialog topWidget"));

        if (auto vLayout = qobject_cast<QVBoxLayout*>(layout()))
            vLayout->insertWidget(0, topWidget_);
    }

    void ContactDialog::initSmilesMenu()
    {
        if (smilesMenu_)
            return;

        smilesMenu_ = new Smiles::SmilesMenu(this);
        smilesMenu_->setCurrentHeight(0);
        smilesMenu_->setHorizontalMargins(Utils::scale_value(16));
        Testing::setAccessibleName(smilesMenu_, qsl("AS EmojiAndStickerPicker"));

        if (auto vLayout = qobject_cast<QVBoxLayout*>(layout()))
        {
            auto smileIdx = -1; // append to end
            if (inputWidget_)
                smileIdx = vLayout->indexOf(inputWidget_);
            vLayout->insertWidget(smileIdx, smilesMenu_);
        }

        connect(smilesMenu_, &Smiles::SmilesMenu::emojiSelected, this, &ContactDialog::emojiSelected);
        connect(smilesMenu_, &Smiles::SmilesMenu::scrolled, this, &ContactDialog::onSuggestHide, Qt::QueuedConnection);

        im_assert(inputWidget_);
        if (inputWidget_)
        {
            connect(smilesMenu_, &Smiles::SmilesMenu::visibilityChanged, inputWidget_, &InputWidget::onSmilesVisibilityChanged);
            connect(smilesMenu_, &Smiles::SmilesMenu::stickerSelected, inputWidget_, &InputWidget::sendSticker);
            connect(inputWidget_, &InputWidget::sendMessage, smilesMenu_, &Smiles::SmilesMenu::hideAnimated);
        }
    }

    void ContactDialog::initInputWidget()
    {
        if (inputWidget_)
            return;

        inputWidget_ = new InputWidget(this, this);

        connect(inputWidget_, &InputWidget::smilesMenuSignal, this, &ContactDialog::onSmilesMenu);
        connect(inputWidget_, &InputWidget::editFocusOut, this, &ContactDialog::onInputEditFocusOut);
        connect(inputWidget_, &InputWidget::sendMessage, this, &ContactDialog::onSendMessage);
        connect(inputWidget_, &InputWidget::inputTyped, this, &ContactDialog::inputTyped);
        connect(inputWidget_, &InputWidget::inputTyped, this, &ContactDialog::hideSmartrepliesForQuoteAnimated);
        connect(inputWidget_, &InputWidget::quotesAdded, this, &ContactDialog::onQuotesAdded);
        connect(inputWidget_, &InputWidget::quotesDropped, this, &ContactDialog::onQuotesDropped);
        connect(inputWidget_, &InputWidget::needSuggets, this, &ContactDialog::onSuggestShow);
        connect(inputWidget_, &InputWidget::hideSuggets, this, &ContactDialog::onSuggestHide);

        connect(this, &ContactDialog::contactSelected, inputWidget_, &InputWidget::contactSelected);
        connect(historyControlWidget_, &HistoryControl::needUpdateWallpaper, inputWidget_, &InputWidget::updateStyle);
        connect(historyControlWidget_, &HistoryControl::messageIdsFetched, inputWidget_, &InputWidget::messageIdsFetched);
        connect(historyControlWidget_, &HistoryControl::quote, inputWidget_, &InputWidget::quote);

        Testing::setAccessibleName(inputWidget_, qsl("AS ChatInput"));

        if (auto vLayout = qobject_cast<QVBoxLayout*>(layout()))
            vLayout->addWidget(inputWidget_);
    }

    void ContactDialog::updateDragOverlayGeometry()
    {
        if (dragOverlayWindow_)
        {
            const auto topH = topWidget_ ? topWidget_->height() : 0;
            const QSize overlaySize(width(), height() - topH);
            if (overlaySize.isValid())
            {
                dragOverlayWindow_->setFixedSize(overlaySize);
                dragOverlayWindow_->move(0, topH);
            }
        }
    }

    void ContactDialog::emojiSelected(const Emoji::EmojiCode& _code, const QPoint _pos)
    {
        if (inputWidget_)
            inputWidget_->insertEmoji(_code, _pos);
    }

    void ContactDialog::onSmilesMenu(const bool _fromKeyboard)
    {
        initSmilesMenu();

        smilesMenu_->showHideAnimated(_fromKeyboard);

        Q_EMIT Utils::InterConnector::instance().hideMentionCompleter();
        hideSuggest();
    }

    void ContactDialog::onInputEditFocusOut()
    {
        hideSmilesMenu();
    }

    void ContactDialog::hideSmilesMenu()
    {
        if (smilesMenu_)
            smilesMenu_->hideAnimated();

        hideSuggest();
    }

    void ContactDialog::updateDragOverlay()
    {
        if (!rect().contains(mapFromGlobal(QCursor::pos())))
            hideDragOverlay();
    }

    void ContactDialog::historyControlClicked()
    {
        Q_EMIT clicked();
    }

    void ContactDialog::onSendMessage(const QString& _contact)
    {
        historyControlWidget_->scrollHistoryToBottom(_contact);

        Q_EMIT sendMessage(_contact);
    }

    void ContactDialog::cancelSelection()
    {
        im_assert(historyControlWidget_);
        historyControlWidget_->cancelSelection();
    }

    void ContactDialog::hideInput()
    {
        overlayUpdateTimer_.stop();

        if (topWidget_)
            topWidget_->hide();

        if (inputWidget_)
            inputWidget_->hideAndClear();
    }

    void ContactDialog::showDragOverlay()
    {
        if (dragOverlayWindow_ && dragOverlayWindow_->isVisible())
            return;

        if (!dragOverlayWindow_)
            dragOverlayWindow_ = new DragOverlayWindow(this);

        updateDragOverlayGeometry();
        dragOverlayWindow_->show();

        overlayUpdateTimer_.start();
        Utils::InterConnector::instance().setDragOverlay(true);
    }

    void ContactDialog::hideDragOverlay()
    {
        if (dragOverlayWindow_)
            dragOverlayWindow_->hide();

        Utils::InterConnector::instance().setDragOverlay(false);
    }

    void ContactDialog::insertTopWidget(const QString& _aimId, QWidget* _widget)
    {
        initTopWidget();

        if (!topWidgetsCache_.contains(_aimId))
        {
            topWidgetsCache_.insert(_aimId, _widget);
            Testing::setAccessibleName(_widget, qsl("AS Dialog widget"));
            topWidget_->addWidget(_widget);
        }

        im_assert(_widget == topWidgetsCache_[_aimId]);

        topWidget_->setCurrentWidget(topWidgetsCache_[_aimId]);
        topWidget_->show();
    }

    void ContactDialog::removeTopWidget(const QString& _aimId)
    {
        if (!topWidget_)
            return;

        if (auto it = std::as_const(topWidgetsCache_).find(_aimId); it != std::as_const(topWidgetsCache_).end())
        {
            topWidget_->removeWidget(it.value());
            it.value()->deleteLater();
            topWidgetsCache_.remove(_aimId);
        }

        if (!topWidget_->currentWidget())
            topWidget_->hide();
    }

    void ContactDialog::dragEnterEvent(QDragEnterEvent *_e)
    {
        if (Logic::getContactListModel()->selectedContact().isEmpty() ||
                !(_e->mimeData() && (_e->mimeData()->hasUrls() || _e->mimeData()->hasImage())) ||
                _e->mimeData()->property("icq").toBool() || QApplication::activeModalWidget() != nullptr)
        {
            _e->setDropAction(Qt::IgnoreAction);
            return;
        }

        auto role = Logic::getContactListModel()->getYourRole(Logic::getContactListModel()->selectedContact());
        if (role == u"notamember" || role == u"readonly")
        {
            _e->setDropAction(Qt::IgnoreAction);
            return;
        }

        showDragOverlay();
        if (dragOverlayWindow_)
            dragOverlayWindow_->setModeByMimeData(_e->mimeData());

        _e->acceptProposedAction();
    }

    void ContactDialog::dragLeaveEvent(QDragLeaveEvent *_e)
    {
        _e->accept();
    }

    void ContactDialog::dragMoveEvent(QDragMoveEvent *_e)
    {
        if (topWidget_ && topWidget_->rect().contains(_e->pos()))
            hideDragOverlay();
        else
            showDragOverlay();

        _e->acceptProposedAction();
    }

    void ContactDialog::resizeEvent(QResizeEvent* _e)
    {
        BackgroundWidget::resizeEvent(_e);

        Q_EMIT resized();
        hideSuggest();

        const auto needUpdateInputBg =
            isVisible() &&
            inputWidget_ &&
            inputWidget_->isVisible() &&
            _e->oldSize().height() != height() &&
            !isFlatColor();
        if (needUpdateInputBg)
            inputWidget_->updateBackground();

        updateDragOverlayGeometry();

        if (smartreplyForQuotePopup_ && smartreplyForQuotePopup_->isTooltipVisible())
        {
            const auto params = getSmartreplyForQuoteParams();
            smartreplyForQuotePopup_->adjustTooltip(params.pos_, params.maxSize_, params.areaRect_);
        }
    }

    HistoryControlPage* ContactDialog::getHistoryPage(const QString& _aimId) const
    {
        return historyControlWidget_->getHistoryPage(_aimId);
    }

    Smiles::SmilesMenu* ContactDialog::getSmilesMenu() const
    {
        return smilesMenu_;
    }

    void ContactDialog::setFocusOnInputWidget()
    {
        if (canSetFocusOnInput())
            inputWidget_->setFocusOnInput();
    }

    void ContactDialog::setFocusOnInputFirstFocusable()
    {
        if (inputWidget_)
            inputWidget_->setFocusOnFirstFocusable();
    }

    bool ContactDialog::canSetFocusOnInput() const
    {
        if (inputWidget_ && !Logic::getContactListModel()->selectedContact().isEmpty())
            return inputWidget_->canSetFocus();

        return false;
    }

    Ui::InputWidget* ContactDialog::getInputWidget() const
    {
        return inputWidget_;
    }

    bool ContactDialog::inputHasSelection() const
    {
        return inputWidget_ && inputWidget_->hasSelection();
    }

    void ContactDialog::notifyApplicationWindowActive(const bool isActive)
    {
        if (historyControlWidget_)
            historyControlWidget_->notifyApplicationWindowActive(isActive);

        if (!isActive)
            Q_EMIT Utils::InterConnector::instance().stopPttRecord();
    }

    void ContactDialog::notifyUIActive(const bool _isActive)
    {
        if (historyControlWidget_)
            historyControlWidget_->notifyUIActive(_isActive);
    }

    void ContactDialog::inputTyped()
    {
        if (historyControlWidget_)
            historyControlWidget_->inputTyped();
    }

    void ContactDialog::onQuotesAdded()
    {
        if (auto page = getHistoryPage(Logic::getContactListModel()->selectedContact()))
            page->setSmartrepliesSemitransparent(true);
    }

    void ContactDialog::onQuotesDropped()
    {
        hideSmartrepliesForQuoteAnimated();
        if (auto page = getHistoryPage(Logic::getContactListModel()->selectedContact()))
            page->setSmartrepliesSemitransparent(false);
    }

    void ContactDialog::suggestedStickerSelected(const QString& _stickerId)
    {
        im_assert(inputWidget_);
        if (!inputWidget_)
            return;

        if (suggestsWidget_)
            suggestsWidget_->hide();

        if (!smilesMenu_)
            initSmilesMenu();
        smilesMenu_->addStickerToRecents(-1, _stickerId);

        inputWidget_->sendSticker(_stickerId);

        sendSuggestedStickerStats(_stickerId);

        inputWidget_->clearInputText();
    }

    void ContactDialog::onSmartrepliesForQuote(const std::vector<Data::SmartreplySuggest>& _suggests)
    {
        if (!inputWidget_ || !Features::isSmartreplyForQuoteEnabled() || !Utils::canShowSmartreplies(currentAimId()))
            return;

        if (_suggests.empty() || _suggests.front().getContact() != currentAimId())
            return;

        if (const auto& quotes = inputWidget_->getInputQuotes(); quotes.size() != 1 || _suggests.front().getMsgId() != quotes.front().msgId_)
            return;

        if (!smartreplyForQuotePopup_)
        {
            smartreplyForQuotePopup_ = new SmartReplyForQuote(this);
            connect(smartreplyForQuotePopup_, &SmartReplyForQuote::sendSmartreply, this, &ContactDialog::sendSmartreplyForQuote);
        }

        smartreplyForQuotePopup_->clear();
        smartreplyForQuotePopup_->setSmartReplies(_suggests);

        const auto params = getSmartreplyForQuoteParams();
        smartreplyForQuotePopup_->showAnimated(params.pos_, params.maxSize_, params.areaRect_);
    }

    void ContactDialog::onMultiselectChanged()
    {
        const auto& contact = Logic::getContactListModel()->selectedContact();
        const auto isMultiselect = Utils::InterConnector::instance().isMultiselect(contact);

        if (isMultiselect)
            hideSmartrepliesForQuoteAnimated();
    }

    void ContactDialog::onSuggestShow(const QString& _text, const QPoint& _pos)
    {
        if (!suggestsWidget_)
        {
            suggestsWidget_ = new Stickers::StickersSuggest(this);
            suggestsWidget_->setArrowVisible(true);
            connect(suggestsWidget_, &Stickers::StickersSuggest::stickerSelected, this, &ContactDialog::suggestedStickerSelected);
            connect(suggestsWidget_, &Stickers::StickersSuggest::scrolledToLastItem, inputWidget_, &InputWidget::requestMoreStickerSuggests);
        }
        const auto fromInput = inputWidget_ && QRect(inputWidget_->mapToGlobal(inputWidget_->rect().topLeft()), inputWidget_->size()).contains(_pos);
        const auto maxSize = QSize(!fromInput ? width() : std::min(width(), Tooltip::getMaxMentionTooltipWidth()), height());

        const auto pt = mapFromGlobal(_pos);
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
            suggestsWidget_->showAnimated(_text, pt, maxSize, areaRect);
            suggestWidgetShown_ = !suggestsWidget_->getStickers().empty();
        }
        else
        {
            suggestsWidget_->updateStickers(_text, pt, maxSize, areaRect);
        }

        core::stats::event_props_type props;
        const bool isEmoji = TextRendering::isEmoji(_text);

        props.push_back({ "type", isEmoji ? "emoji" : "word"});
        props.push_back({ "cont_server_suggest", inputWidget_->hasServerSuggests() ? "yes" : "no" });
        GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::stickers_suggests_appear_in_input, props);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_suggests_appear_in_input, props);
    }

    void ContactDialog::onSuggestHide()
    {
        hideSuggest();
        inputWidget_->clearLastSuggests();
        suggestWidgetShown_ = false;
    }

    bool ContactDialog::isSuggestVisible() const
    {
        return (suggestsWidget_ && suggestsWidget_->isTooltipVisible());
    }

    bool ContactDialog::isEditingMessage() const
    {
        return inputWidget_ && inputWidget_->isEditing();
    }

    bool ContactDialog::isShowingSmileMenu() const
    {
        return smilesMenu_ && !smilesMenu_->isHidden();
    }

    bool ContactDialog::isReplyingToMessage() const
    {
        return inputWidget_ && inputWidget_->isReplying();
    }

    bool ContactDialog::isRecordingPtt() const
    {
        return inputWidget_ && inputWidget_->isRecordingPtt();
    }

    bool ContactDialog::isPttRecordingHold() const
    {
        return inputWidget_ && inputWidget_->isPttHold();
    }

    bool ContactDialog::isPttRecordingHoldByKeyboard() const
    {
        return inputWidget_ && inputWidget_->isPttHoldByKeyboard();
    }

    bool ContactDialog::isPttRecordingHoldByMouse() const
    {
        return inputWidget_ && inputWidget_->isPttHoldByMouse();
    }

    bool ContactDialog::tryPlayPttRecord()
    {
        return inputWidget_ && inputWidget_->tryPlayPttRecord();
    }

    bool ContactDialog::tryPausePttRecord()
    {
        return inputWidget_ && inputWidget_->tryPausePttRecord();
    }

    void ContactDialog::closePttRecording()
    {
        if (inputWidget_ && !isPttRecordingHoldByMouse())
            inputWidget_->closePttPanel();
    }

    void ContactDialog::sendPttRecord()
    {
        if (inputWidget_ && !isPttRecordingHold())
            inputWidget_->sendPtt();
    }

    void ContactDialog::stopPttRecord()
    {
        if (inputWidget_ && !isPttRecordingHold())
            inputWidget_->stopPttRecording();
    }

    void ContactDialog::startPttRecordingLock()
    {
        if (inputWidget_ && !isPttRecordingHold())
            inputWidget_->startPttRecordingLock();
    }

    void ContactDialog::dropReply()
    {
        if (inputWidget_)
            inputWidget_->dropReply();
    }

    bool ContactDialog::isPasteEnabled() const
    {
        return !isRecordingPtt();
    }

    void ContactDialog::hideSuggest()
    {
        if (suggestsWidget_)
            suggestsWidget_->hideAnimated();
    }

    Stickers::StickersSuggest* ContactDialog::getStickersSuggests()
    {
        return suggestsWidget_;
    }

    void ContactDialog::setFrameCountMode(FrameCountMode _mode)
    {
        frameCountMode_ = _mode;
        historyControlWidget_->setFrameCountMode(frameCountMode_);
    }

    FrameCountMode ContactDialog::getFrameCountMode() const
    {
        return frameCountMode_;
    }

    const QString& ContactDialog::currentAimId() const
    {
        return historyControlWidget_->currentAimId();
    }

    void ContactDialog::hideSmartrepliesForQuoteAnimated()
    {
        if (smartreplyForQuotePopup_ && smartreplyForQuotePopup_->isTooltipVisible())
            smartreplyForQuotePopup_->hideAnimated();
    }

    void ContactDialog::hideSmartrepliesForQuoteForce()
    {
        if (smartreplyForQuotePopup_ && smartreplyForQuotePopup_->isTooltipVisible())
            smartreplyForQuotePopup_->hideForce();
    }

    void ContactDialog::sendSuggestedStickerStats(const QString& _stickerId)
    {
        const QString inputText = inputWidget_->getInputText();
        const bool isEmoji = TextRendering::isEmoji(inputText);
        const bool isServerSuggest = inputWidget_->isServerSuggest(_stickerId);
        const auto& currentStickers = getStickersSuggests()->getStickers();
        const auto stickerPos = std::find(currentStickers.begin(), currentStickers.end(), _stickerId);
        const auto posInStickers = stickerPos != currentStickers.end() ? std::distance(currentStickers.begin(), stickerPos) + 1 : -1;

        core::stats::event_props_type props;
        props.push_back({ "stickerId", _stickerId.toStdString() });
        props.push_back({ "number", std::to_string(posInStickers) });
        props.push_back({ "type", isEmoji ? "emoji" : "word" });
        props.push_back({ "suggest_type", isServerSuggest ? "server" : "client" });
        GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::stickers_suggested_sticker_sent, props);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_suggested_sticker_sent, props);

        if (isServerSuggest)
            GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::text_sticker_used);
    }

    ContactDialog::SmartreplyForQuoteParams ContactDialog::getSmartreplyForQuoteParams() const
    {
        const auto maxSize = QSize(std::min(width(), Tooltip::getMaxMentionTooltipWidth()), height());
        auto areaRect = rect();
        if (areaRect.width() > MessageStyle::getHistoryWidgetMaxWidth())
        {
            const auto center = areaRect.center();
            areaRect.setWidth(MessageStyle::getHistoryWidgetMaxWidth());
            areaRect.moveCenter(center);
        }

        im_assert(inputWidget_);

        const auto x = Utils::scale_value(52);
        const auto y = inputWidget_->pos().y() + Utils::scale_value(10);
        return { QPoint(x, y), areaRect, maxSize };
    }

    void ContactDialog::sendSmartreplyForQuote(const Data::SmartreplySuggest& _suggest)
    {
        if (!inputWidget_ || currentAimId().isEmpty())
            return;

        GetDispatcher()->sendSmartreplyToContact(currentAimId(), _suggest, inputWidget_->getInputQuotes());

        inputWidget_->dropReply();
        historyControlWidget_->scrollHistoryToBottom(currentAimId());
        Ui::MainPage::instance()->setFocusOnInput();
    }
}
