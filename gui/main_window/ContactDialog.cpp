#include "stdafx.h"
#include "ContactDialog.h"

#include "MainWindow.h"
#include "MainPage.h"
#include "DragOverlayWindow.h"

#include "contact_list/ContactListModel.h"
#include "history_control/HistoryControl.h"
#include "history_control/MessageStyle.h"
#include "input_widget/InputWidget.h"
#include "sidebar/Sidebar.h"
#include "smiles_menu/suggests_widget.h"
#include "smiles_menu/SmilesMenu.h"
#include "core_dispatcher.h"
#include "gui_settings.h"

#include "utils/gui_coll_helper.h"
#include "utils/InterConnector.h"
#include "controls/TextRendering.h"
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
    {
        setFrameCountMode(frameCountMode_);
        setPalette(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
        setAcceptDrops(true);

        auto layout = Utils::emptyVLayout(this);

        Testing::setAccessibleName(historyControlWidget_, qsl("AS historyControlWidget_"));
        layout->addWidget(historyControlWidget_);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::dialogClosed, this, &ContactDialog::removeTopWidget);
        connect(this, &ContactDialog::contactSelected, historyControlWidget_, &HistoryControl::contactSelected);
        connect(historyControlWidget_, &HistoryControl::setTopWidget, this, &ContactDialog::insertTopWidget);
        connect(historyControlWidget_, &HistoryControl::clicked, this, &ContactDialog::historyControlClicked, Qt::QueuedConnection);
        connect(historyControlWidget_, &HistoryControl::needUpdateWallpaper, this, &BackgroundWidget::updateWallpaper);

        updateWallpaper(QString());

        overlayUpdateTimer_.setInterval(overlayUpdateTimeout().count());
        connect(&overlayUpdateTimer_, &QTimer::timeout, this, &ContactDialog::updateDragOverlay);

        connect(Utils::InterConnector::instance().getMainWindow(), &MainWindow::mouseReleased, this, [this]()
        {
            if (!rect().contains(mapFromGlobal(QCursor::pos())) && !isPttRecordingHold())
                emit Utils::InterConnector::instance().stopPttRecord();
        });

        connect(Utils::InterConnector::instance().getMainWindow(), &MainWindow::mousePressed, this, [this]()
        {
            if (const auto buttons = qApp->mouseButtons(); buttons != Qt::MouseButtons(Qt::LeftButton))
            {
                if (!historyControlWidget_->hasMessageUnderCursor())
                    emit Utils::InterConnector::instance().stopPttRecord();
            }
        });
    }

    ContactDialog::~ContactDialog() = default;

    void ContactDialog::onContactSelected(const QString& _aimId, qint64 _messageId, qint64 _quoteId)
    {
        initInputWidget();
        emit contactSelected(_aimId, _messageId, _quoteId);
    }

    void ContactDialog::onContactSelectedToLastMessage(const QString& _aimId, qint64 _messageId)
    {
        initInputWidget();
        emit contactSelectedToLastMessage(_aimId, _messageId);
    }

    void ContactDialog::initTopWidget()
    {
        if (topWidget_)
            return;

        topWidget_ = new QStackedWidget(this);
        topWidget_->setFixedHeight(Utils::getTopPanelHeight());
        Testing::setAccessibleName(topWidget_, qsl("AS topWidget_"));

        if (auto vLayout = qobject_cast<QVBoxLayout*>(layout()))
            vLayout->insertWidget(0, topWidget_);
    }

    void ContactDialog::initSmilesMenu()
    {
        if (smilesMenu_)
            return;

        smilesMenu_ = new Smiles::SmilesMenu(this);
        smilesMenu_->setFixedHeight(0);
        Testing::setAccessibleName(smilesMenu_, qsl("AS smilesMenu_"));

        if (auto vLayout = qobject_cast<QVBoxLayout*>(layout()))
        {
            auto smileIdx = -1; // append to end
            if (inputWidget_)
                smileIdx = vLayout->indexOf(inputWidget_);
            vLayout->insertWidget(smileIdx, smilesMenu_);
        }

        connect(smilesMenu_, &Smiles::SmilesMenu::emojiSelected, this, &ContactDialog::emojiSelected);
        connect(smilesMenu_, &Smiles::SmilesMenu::scrolled, this, &ContactDialog::onSuggestHide, Qt::QueuedConnection);

        assert(inputWidget_);
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
        connect(inputWidget_, &InputWidget::needSuggets, this, &ContactDialog::onSuggestShow);
        connect(inputWidget_, &InputWidget::hideSuggets, this, &ContactDialog::onSuggestHide);

        connect(this, &ContactDialog::contactSelected, inputWidget_, &InputWidget::contactSelected);
        connect(historyControlWidget_, &HistoryControl::needUpdateWallpaper, inputWidget_, &InputWidget::updateStyle);
        connect(historyControlWidget_, &HistoryControl::messageIdsFetched, inputWidget_, &InputWidget::messageIdsFetched);
        connect(historyControlWidget_, &HistoryControl::quote, inputWidget_, &InputWidget::quote);

        Testing::setAccessibleName(inputWidget_, qsl("AS inputWidget_"));

        if (auto vLayout = qobject_cast<QVBoxLayout*>(layout()))
            vLayout->addWidget(inputWidget_);

        initSmilesMenu();
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

        emit Utils::InterConnector::instance().hideMentionCompleter();
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
        emit clicked();
    }

    void ContactDialog::onSendMessage(const QString& _contact)
    {
        historyControlWidget_->scrollHistoryToBottom(_contact);

        emit sendMessage(_contact);
    }

    void ContactDialog::cancelSelection()
    {
        assert(historyControlWidget_);
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
            Testing::setAccessibleName(_widget, qsl("AS cd _widget"));
            topWidget_->addWidget(_widget);
        }

        topWidget_->setCurrentWidget(topWidgetsCache_[_aimId]);
        topWidget_->show();
    }

    void ContactDialog::removeTopWidget(const QString& _aimId)
    {
        if (!topWidget_)
            return;

        if (topWidgetsCache_.contains(_aimId))
        {
            topWidget_->removeWidget(topWidgetsCache_[_aimId]);
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
        if (role == ql1s("notamember") || role == ql1s("readonly"))
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

        emit resized();
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
    }

    HistoryControlPage* ContactDialog::getHistoryPage(const QString& _aimId) const
    {
        return historyControlWidget_->getHistoryPage(_aimId);
    }

    Smiles::SmilesMenu* ContactDialog::getSmilesMenu() const
    {
        assert(smilesMenu_);
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

    void ContactDialog::notifyApplicationWindowActive(const bool isActive)
    {
        if (historyControlWidget_)
        {
            historyControlWidget_->notifyApplicationWindowActive(isActive);
        }

        if (!isActive)
            emit Utils::InterConnector::instance().stopPttRecord();
    }

    void ContactDialog::notifyUIActive(const bool _isActive)
    {
        if (historyControlWidget_)
        {
            historyControlWidget_->notifyUIActive(_isActive);
        }
    }

    void ContactDialog::inputTyped()
    {
        historyControlWidget_->inputTyped();
    }

    void ContactDialog::suggestedStickerSelected(qint32 _setId, const QString& _stickerId)
    {
        assert(inputWidget_);
        assert(smilesMenu_);
        if (!inputWidget_)
            return;

        if (suggestsWidget_)
            suggestsWidget_->hide();

        if (smilesMenu_)
            smilesMenu_->addStickerToRecents(_setId, _stickerId);

        inputWidget_->sendSticker(_setId, _stickerId);
        inputWidget_->clearInputText();

        const QString inputText = inputWidget_->getInputText();
        const bool isEmoji = TextRendering::isEmoji(QStringRef(&inputText));

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_suggested_sticker_sent, {
            { "searchlength", isEmoji ? std::string("1") : std::to_string(inputText.length()) },
            { "searchstring", inputText.toStdString() } });
    }

    void ContactDialog::onSuggestShow(const QString _text, const QPoint _pos)
    {
        if (!suggestsWidget_)
        {
            suggestsWidget_ = new Stickers::StickersSuggest(this);
            connect(suggestsWidget_, &Stickers::StickersSuggest::stickerSelected, this, &ContactDialog::suggestedStickerSelected);
        }
        const auto fromInput = inputWidget_ && QRect(inputWidget_->mapToGlobal(inputWidget_->rect().topLeft()), inputWidget_->size()).contains(_pos);
        const auto maxSize = QSize(!fromInput ? width() : std::min(width(), Tooltip::getMaxMentionTooltipWidth()), height());

        auto areaRect = fromInput ? rect() : QRect();
        if (areaRect.width() > MessageStyle::getHistoryWidgetMaxWidth())
        {
            const auto center = areaRect.center();
            areaRect.setWidth(MessageStyle::getHistoryWidgetMaxWidth());
            areaRect.moveCenter(center);
        }
        const auto pt = mapFromGlobal(_pos);
        suggestsWidget_->showAnimated(_text, pt, maxSize, areaRect);

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_suggests_appear_in_input);
    }

    void ContactDialog::onSuggestHide()
    {
        hideSuggest();
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

    void ContactDialog::closePttRecording()
    {
        if (inputWidget_ && !isPttRecordingHold())
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

    void ContactDialog::startPttRecordingHold()
    {
        if (inputWidget_)
            inputWidget_->startPttRecordingHold();
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
}
