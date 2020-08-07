#include "stdafx.h"

#include "InputWidget.h"
#include "Text2Symbol.h"
#include "HistoryUndoStack.h"
#include "HistoryTextEdit.h"
#include "SubmitButton.h"
#include "PttLock.h"
#include "InputBgWidget.h"
#include "AttachFilePopup.h"

#include "panels/QuotesWidget.h"
#include "panels/PanelMain.h"
#include "panels/PanelDisabled.h"
#include "panels/PanelReadonly.h"
#include "panels/PanelPtt.h"
#include "panels/PanelMultiselect.h"

#include "../ContactDialog.h"
#include "../history_control/MentionCompleter.h"
#include "../history_control/HistoryControlPage.h"
#include "../history_control/TransitProfileShareWidget.h"
#include "../history_control/MessageStyle.h"
#include "../history_control/SnippetCache.h"

#include "../../core_dispatcher.h"
#include "../../gui_settings.h"

#include "../../controls/GeneralDialog.h"
#include "../../controls/BackgroundWidget.h"
#include "../../controls/TooltipWidget.h"
#include "../../controls/textrendering/TextRenderingUtils.h"

#include "../../main_window/MainWindow.h"
#include "../../main_window/FilesWidget.h"
#include "../../main_window/GroupChatOperations.h"
#include "../../main_window/MainPage.h"
#include "../../main_window/PollWidget.h"
#include "../../main_window/contact_list/RecentsModel.h"
#include "../../main_window/contact_list/MentionModel.h"
#include "../../main_window/contact_list/IgnoreMembersModel.h"
#include "../../main_window/contact_list/ContactListModel.h"
#include "../../main_window/contact_list/ContactListUtils.h"
#include "../../main_window/contact_list/SelectionContactsForGroupChat.h"
#include "../../main_window/sounds/SoundsManager.h"
#include "../../main_window/smiles_menu/suggests_widget.h"

#include "../../main_window/containers/FriendlyContainer.h"
#include "../../main_window/containers/LastseenContainer.h"

#include "../../utils/gui_coll_helper.h"
#include "../../utils/utils.h"
#include "../../utils/UrlParser.h"
#include "../../utils/features.h"
#include "../../utils/stat_utils.h"

#include "../../styles/ThemeParameters.h"
#include "../../styles/ThemesContainer.h"
#include "../../cache/stickers/stickers.h"

#include "../../previewer/toast.h"

#include "media/ptt/AudioRecorder2.h"
#include "media/ptt/AudioUtils.h"
#include "media/permissions/MediaCapturePermissions.h"

#include "../../../common.shared/smartreply/smartreply_types.h"
#include "../../../common.shared/config/config.h"

namespace
{
    constexpr std::chrono::milliseconds viewTransitDuration() noexcept { return std::chrono::milliseconds(100); }
    constexpr std::chrono::milliseconds singleEmojiSuggestDuration() noexcept { return std::chrono::milliseconds(100); }
    constexpr auto symbolAt() noexcept { return u'@'; }

    constexpr double getBlurAlpha() noexcept
    {
        return 0.45;
    }

    void clearContactModel(Ui::SelectContactsWidget* _selectWidget)
    {
        Logic::getContactListModel()->clearCheckedItems();
        _selectWidget->rewindToTop();
    }

    void updateMainMenu()
    {
        if constexpr (platform::is_apple())
        {
            if (auto w = Utils::InterConnector::instance().getMainWindow())
                w->updateMainMenu();
        }
    };

    constexpr std::chrono::milliseconds getLoaderOverlayDelay()
    {
        return std::chrono::milliseconds(100);
    }

    QString getProfileUrl()
    {
        const auto useProfileAgent = config::get().is_on(config::features::profile_agent_as_domain_url);
        return useProfileAgent ? Features::getProfileDomainAgent() : Features::getProfileDomain();
    }
}

namespace Ui
{
    InputWidget::InputWidget(QWidget* _parent, BackgroundWidget* _bg)
        : QWidget(_parent)
        , bgWidget_(new InputBgWidget(this, _bg))
        , quotesWidget_(nullptr)
        , stackWidget_(new QStackedWidget(this))
        , panelMain_(nullptr)
        , panelDisabled_(nullptr)
        , panelReadonly_(nullptr)
        , panelPtt_(nullptr)
        , panelMultiselect_(nullptr)
        , pttLock_(nullptr)
        , textEdit_(nullptr)
        , buttonSubmit_(nullptr)
        , suggestTimer_(nullptr)
        , selectContactsWidget_(nullptr)
        , transitionLabel_(nullptr)
        , transitionAnim_(new QVariantAnimation(this))
        , callLinkCreator_(nullptr)
    {
        hide();

        auto host = new QWidget(this);
        host->setMaximumWidth(MessageStyle::getHistoryWidgetMaxWidth());

        auto rootLayout = Utils::emptyHLayout(this);
        rootLayout->addWidget(host);

        auto hostLayout = Utils::emptyVLayout(host);

        Testing::setAccessibleName(stackWidget_, qsl("AS ChatInput stackWidget"));
        hostLayout->addWidget(stackWidget_);

        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

        connect(Logic::getContactListModel(), &Logic::ContactListModel::youRoleChanged, this, &InputWidget::chatRoleChanged);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &InputWidget::onContactChanged);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, &InputWidget::hideAndRemoveDialog);
        connect(GetDispatcher(), &core_dispatcher::recvPermitDeny, this, &InputWidget::onRecvPermitDeny);
        connect(GetDispatcher(), &core_dispatcher::activeDialogHide, this, &InputWidget::hideAndRemoveDialog);
        connect(GetDispatcher(), &core_dispatcher::stickersRequestedSuggestsResult, this, &InputWidget::onRequestedStickerSuggests);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::cancelEditing, this, &InputWidget::onEditCancel);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::stopPttRecord, this, &InputWidget::needToStopPtt);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::clearInputText, this, &InputWidget::clearInputText);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::historyReady, this, &InputWidget::onHistoryReady);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::historyCleared, this, &InputWidget::onHistoryCleared);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::historyInsertedMessages, this, &InputWidget::onHistoryInsertedMessages);

        if (Features::isSuggestsEnabled())
        {
            suggestTimer_ = new QTimer(this);
            suggestTimer_->setSingleShot(true);
            suggestTimer_->setInterval(Features::getSuggestTimerInterval());
            connect(suggestTimer_, &QTimer::timeout, this, &InputWidget::onSuggestTimeout);
        }

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::applicationLocked, this, &InputWidget::needToStopPtt);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectChanged, this, &InputWidget::multiselectChanged);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectAnimationUpdate, this, &InputWidget::updateBackground);

        connect(this, &InputWidget::needClearQuotes, this, &InputWidget::onQuotesCancel);

        bgWidget_->lower();
        bgWidget_->forceUpdate();
    }

    InputWidget::~InputWidget() = default;

    bool InputWidget::isReplying() const
    {
        return quotesWidget_ && quotesWidget_->isVisibleTo(this);
    }

    bool InputWidget::isRecordingPtt() const
    {
        return currentView() == InputView::Ptt;
    }

    bool InputWidget::isRecordingPtt(const QString& _contact) const
    {
        return currentView(_contact) == InputView::Ptt;
    }

    bool InputWidget::tryPlayPttRecord()
    {
        return panelPtt_ && isRecordingPtt() && panelPtt_->tryPlay(contact_);
    }

    bool InputWidget::tryPausePttRecord()
    {
        return panelPtt_ && isRecordingPtt() && panelPtt_->tryPause(contact_);
    }

    bool InputWidget::isPttHold() const
    {
        return isPttHoldByKeyboard() || isPttHoldByMouse();
    }

    bool InputWidget::isPttHoldByKeyboard() const
    {
        return getPttMode() == PttMode::HoldByKeyboard;
    }

    bool InputWidget::isPttHoldByMouse() const
    {
        return getPttMode() == PttMode::HoldByMouse;
    }

    void InputWidget::closePttPanel()
    {
        removePttRecord();
        switchToPreviousView();
    }

    void InputWidget::dropReply()
    {
        if (isReplying())
            onQuotesCancel();
    }

    void InputWidget::pasteFromClipboard()
    {
        auto mimedata = QApplication::clipboard()->mimeData();
        if (!mimedata || Utils::haveText(mimedata))
        {
            auto& inputMentions = isEditing() ? currentState().edit_.message_->Mentions_ : currentState().mentions_;
            const auto& textEditMentions = textEdit_->getMentions();
            inputMentions.insert(textEditMentions.begin(), textEditMentions.end());

            auto& inputFiles = isEditing() ? currentState().edit_.message_->Files_ : currentState().files_;
            const auto& textEditFiles = textEdit_->getFiles();
            inputFiles.insert(textEditFiles.begin(), textEditFiles.end());

            return;
        }

        bool hasUrls = false;
        if (mimedata->hasUrls())
        {
            currentState().filesToSend_.clear();
            currentState().description_.clear();
            const QList<QUrl> urlList = mimedata->urls();
            for (const auto& url : urlList)
            {
                if (url.isLocalFile())
                {
                    const auto path = url.toLocalFile();
                    if (const auto fi = QFileInfo(path); fi.exists() && !fi.isBundle() && !fi.isDir())
                        currentState().filesToSend_.emplace_back(path);
                    hasUrls = true;
                }
            }
        }

        if (!hasUrls && mimedata->hasImage())
        {
            currentState().filesToSend_.clear();
            QPixmap pixmap;
#ifdef _WIN32
            if (OpenClipboard(nullptr))
            {
                HBITMAP hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
                pixmap = qt_pixmapFromWinHBITMAP(hBitmap);
                CloseClipboard();
            }
#else
            pixmap = qvariant_cast<QPixmap>(mimedata->imageData());
#endif
            if (!pixmap.isNull())
                currentState().filesToSend_.emplace_back(std::move(pixmap));
        }

        showFileDialog(FileSource::CopyPaste);
    }

    void InputWidget::keyPressEvent(QKeyEvent* _e)
    {
        if (_e->matches(QKeySequence::Paste) && !isRecordingPtt())
        {
            if (!isExternalPaste_)
            {
                pasteFromClipboard();
                if (textEdit_)
                    textEdit_->getReplacer().setAutoreplaceAvailable(Emoji::ReplaceAvailable::Unavailable);
            }
            else
            {
                isExternalPaste_ = false;
            }
        }
        else if (_e->key() == Qt::Key_Return || _e->key() == Qt::Key_Enter)
        {
            Qt::KeyboardModifiers modifiers = _e->modifiers();

            if (textEdit_ && textEdit_->isVisible() && textEdit_->catchEnter(modifiers))
                send();
        }
        else if (_e->matches(QKeySequence::Find))
        {
            Q_EMIT Utils::InterConnector::instance().setSearchFocus();
        }
        else if (_e->key() == Qt::Key_At || _e->text() == symbolAt())
        {
            if (shouldOfferCompleter(CheckOffer::Yes))
            {
                if (showMentionCompleter(QString(), tooltipArrowPosition()))
                    currentState().mentionSignIndex_ = textEdit_->textCursor().position() - 1;
            }
        }

        return QWidget::keyPressEvent(_e);
    }

    void InputWidget::keyReleaseEvent(QKeyEvent *_e)
    {
        if constexpr (platform::is_apple())
        {
            if (_e->modifiers() & Qt::AltModifier)
            {
                QTextCursor cursor = textEdit_->textCursor();
                QTextCursor::MoveMode selection = (_e->modifiers() & Qt::ShiftModifier) ?
                    QTextCursor::MoveMode::KeepAnchor :
                    QTextCursor::MoveMode::MoveAnchor;

                if (_e->key() == Qt::Key_Left)
                {
                    cursor.movePosition(QTextCursor::MoveOperation::PreviousWord, selection);
                    textEdit_->setTextCursor(cursor);
                }
                else if (_e->key() == Qt::Key_Right)
                {
                    auto prev = cursor.position();
                    cursor.movePosition(QTextCursor::MoveOperation::EndOfWord, selection);
                    if (prev == cursor.position())
                    {
                        cursor.movePosition(QTextCursor::MoveOperation::NextWord, selection);
                        cursor.movePosition(QTextCursor::MoveOperation::EndOfWord, selection);
                    }
                    textEdit_->setTextCursor(cursor);
                }
            }
        }

        QWidget::keyReleaseEvent(_e);
    }

    void InputWidget::editContentChanged()
    {
        if (contact_.isEmpty())
            return;

        const auto inputText = getInputText();
        const auto isEdit = isEditing();

        if (isEdit)
        {
            if (currentState().edit_.message_ && currentState().edit_.message_->GetType() == core::message_type::file_sharing)
                currentState().edit_.buffer_ = currentState().edit_.message_->GetText() % ql1c('\n') % inputText;
            else
                currentState().edit_.buffer_ = inputText;
        }
        else
            inputTexts_[contact_] = InputText(inputText, textEdit_->textCursor().position());

        const auto textIsEmpty = QStringView(inputText).trimmed().isEmpty();

        if (textIsEmpty && textEdit_->getUndoRedoStack().isEmpty())
        {
            currentState().mentionSignIndex_ = -1;

            if (isEdit)
            {
                if (currentState().edit_.message_)
                {
                    assert(currentState().edit_.message_);
                    currentState().edit_.message_->Mentions_.clear();
                }
            }
            else
            {
                currentState().mentions_.clear();
            }
        }

        updateSubmitButtonState(UpdateMode::IfChanged);
    }

    void InputWidget::selectionChanged()
    {
        if (textEdit_->selection().isEmpty())
            return;

        if (auto page = Utils::InterConnector::instance().getHistoryPage(contact_))
            page->clearSelection();
    }

    void InputWidget::onSuggestTimeout()
    {
        if (!Features::isSuggestsEnabled() || contact_.isEmpty())
            return;

        const auto text = getInputText();
        const auto words = text.splitRef(QChar::Space);
        const auto suggestText = text.toLower().trimmed();

        auto isShowingSmileMenu = false;
        if (auto contactDialog = Utils::InterConnector::instance().getContactDialog())
            isShowingSmileMenu = contactDialog->isShowingSmileMenu();
        if (!TextRendering::isEmoji(suggestText) || !isShowingSmileMenu)
            suggestPos_ = QPoint();

        if (suggestText.isEmpty() || suggestText.size() > Features::maxAllowedLocalSuggestChars()
            || words.size() > Features::maxAllowedLocalSuggestWords())
            return;

        Stickers::Suggest suggest;
        if (!Stickers::getSuggestWithSettings(suggestText, suggest))
        {
            if (suggestRequested_)
                return;
        }

        const auto suggestLimit = std::min(Tooltip::getMaxMentionTooltipWidth(), width())/Stickers::getStickerWidth();
        if (!suggestRequested_
            && suggest.size() < (unsigned)suggestLimit
            && words.size() <= Features::maxAllowedSuggestWords()
            && suggestText.size() <= Features::maxAllowedSuggestChars()
            && !suggestText.contains(symbolAt())
            )
        {
            requestStickerSuggests();
            Stickers::getSuggestWithSettings(suggestText, suggest);
        }

        if (suggest.empty())
            return;

        auto pos = suggestPos_;
        if (pos.isNull())
        {
            pos = textEdit_->mapToGlobal(textEdit_->pos());
            pos.setY(textEdit_->mapToGlobal(textEdit_->geometry().topLeft()).y() + Utils::scale_value(10));
            if (pos.y() % 2 != 0)
                pos.ry() += 1;

            if (textEdit_->document()->lineCount() == 1)
                pos.rx() += textEdit_->document()->idealWidth() / 2 - Tooltip::getDefaultArrowOffset();
            else
                pos.rx() = width() / 2 - Tooltip::getDefaultArrowOffset();
        }

        Q_EMIT needSuggets(suggestText, pos, QPrivateSignal());
    }

    void InputWidget::requestStickerSuggests()
    {
        if (!Features::isServerSuggestsEnabled() || contact_.isEmpty())
            return;

        const auto text = getInputText();

        if (!get_gui_settings()->get_value<bool>(settings_show_suggests_emoji, true) && TextRendering::isEmoji(text))
            return;
        else if (!get_gui_settings()->get_value<bool>(settings_show_suggests_words, true))
            return;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("text", text);
        collection.set_value_as_qstring("contact", contact_);

        Ui::GetDispatcher()->post_message_to_core("stickers/suggests/get", collection.get());
    }

    void InputWidget::quote(const Data::QuotesVec& _quotes)
    {
        if (_quotes.isEmpty() || currentState().isDisabled())
            return;

        Q_EMIT quotesAdded(QPrivateSignal());

        auto quotes = _quotes;
        auto isQuote = [](const auto& _quote) { return _quote.type_ == Data::Quote::quote && _quote.hasReply_; };
        const auto allQuotes = std::all_of(quotes.begin(), quotes.end(), isQuote);

        if (!allQuotes)
            quotes.erase(std::remove_if(quotes.begin(), quotes.end(), isQuote), quotes.end());

        currentState().quotes_ += quotes;
        currentState().canBeEmpty_ = true;

        if (currentView() == InputView::Multiselect)
            setView(InputView::Default, UpdateMode::Force, SetHistoryPolicy::DontAddToHistory);
        else
            showQuotes();

        requestSmartrepliesForQuote();

        setFocusOnInput();
    }

    void InputWidget::insertEmoji(const Emoji::EmojiCode& _code, const QPoint _pt)
    {
        if (isInputEmpty())
            suggestPos_ = _pt;

        textEdit_->insertEmoji(_code);

        setFocusOnInput();

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_emoji_added_in_input);
    }

    void InputWidget::sendSticker(const QString& _stickerId)
    {
        Stickers::lockStickerCache(_stickerId);

        Ui::gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", contact_);
        collection.set_value_as_bool("fs_sticker", true);
        collection.set_value_as_qstring("message", Stickers::getSendBaseUrl() % _stickerId);
        collection.set_value_as_bool("channel", Logic::getContactListModel()->isChannel(contact_));

        const auto isEdit = isEditing();
        const auto& quotes = getInputQuotes();

        if (!isEdit && !quotes.isEmpty())
        {
            Data::MentionMap quoteMentions;
            core::ifptr<core::iarray> quotesArray(collection->create_array());
            quotesArray->reserve(quotes.size());
            for (const auto& q : quotes)
            {
                core::ifptr<core::icollection> quoteCollection(collection->create_collection());
                q.serialize(quoteCollection.get());
                core::coll_helper coll(collection->create_collection(), true);
                core::ifptr<core::ivalue> val(collection->create_value());
                val->set_as_collection(quoteCollection.get());
                quotesArray->push_back(val.get());

                quoteMentions.insert(q.mentions_.begin(), q.mentions_.end());
            }
            collection.set_value_as_array("quotes", quotesArray.get());

            Data::serializeMentions(collection, quoteMentions);

            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_send_answer);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, { { "Quotes_MessagesCount", std::to_string(quotes.size()) } });

            clearQuotes();
        }

        GetDispatcher()->post_message_to_core("send_message", collection.get());

        if (!isEdit)
        {
            switchToPreviousView();
            sendStatsIfNeeded();
        }

        Q_EMIT sendMessage(contact_);
        hideSmartreplies();
    }

    void InputWidget::onSmilesVisibilityChanged(const bool _isVisible)
    {
        if (panelMain_)
            panelMain_->setEmojiButtonChecked(_isVisible);
    }

    void InputWidget::insertMention(const QString& _aimId, const QString& _friendly)
    {
        if (_aimId.isEmpty() || _friendly.isEmpty() || !textEdit_)
            return;

        auto& mentionMap = isEditing() ? currentState().edit_.message_->Mentions_ : currentState().mentions_;
        mentionMap.emplace(_aimId, _friendly);

        auto insertPos = -1;
        auto& mentionSignIndex = currentState().mentionSignIndex_;
        if (mentionSignIndex != -1)
        {
            QSignalBlocker sbe(textEdit_);
            QSignalBlocker sbd(textEdit_->document());

            insertPos = mentionSignIndex;

            auto cursor = textEdit_->textCursor();
            auto posShift = 0;

            if (const auto mc = getMentionCompleter())
                posShift = mc->getSearchPattern().size() + 1;
            else if (cursor.position() >= mentionSignIndex)
                posShift = cursor.position() - mentionSignIndex;

            cursor.setPosition(mentionSignIndex);
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, posShift);
            cursor.removeSelectedText();

            mentionSignIndex = -1;
        }

        if (insertPos != -1)
        {
            auto cursor = textEdit_->textCursor();
            cursor.setPosition(insertPos);
            textEdit_->setTextCursor(cursor);
        }

        textEdit_->getUndoRedoStack().push(getInputText());
        textEdit_->getUndoRedoStack().pop();
        textEdit_->insertMention(_aimId, _friendly);
    }

    void InputWidget::mentionOffer(const int _position, const CursorPolicy _policy)
    {
        currentState().mentionSignIndex_ = _position;

        if (shouldOfferCompleter(CheckOffer::No))
        {
            if (showMentionCompleter(QString(), tooltipArrowPosition()))
            {
                if (_policy == CursorPolicy::Set)
                    textEdit_->textCursor().setPosition(_position);
            }
        }
    }

    void InputWidget::messageIdsFetched(const QString& _aimId, const Data::MessageBuddies& _buddies)
    {
        if (auto& editedMessage = currentState().edit_.message_)
        {
            for (const auto& msg : _buddies)
            {
                if (editedMessage->InternalId_ == msg->InternalId_)
                {
                    editedMessage->Id_ = msg->Id_;
                    editedMessage->SetTime(msg->GetTime());
                    if (!editedMessage->GetDescription().isEmpty())
                        editedMessage->SetUrl(msg->GetUrl());
                    break;
                }
            }
        }
    }

    void InputWidget::onHistoryReady(const QString& _contact)
    {
        if (contact_ != _contact)
            return;

        currentState().msgHistoryReady_ = true;
        if (isBot())
            setView(InputView::Default, UpdateMode::Force, SetHistoryPolicy::DontAddToHistory);
    }

    void InputWidget::onHistoryCleared(const QString& _contact)
    {
        if (contact_ != _contact || !isBot())
            return;

        setView(InputView::Readonly, UpdateMode::Force, SetHistoryPolicy::DontAddToHistory);
    }

    void InputWidget::onHistoryInsertedMessages(const QString& _contact)
    {
        if (auto view = currentView(); view != InputView::Disabled && view != InputView::Readonly)
            return;

        if (contact_ != _contact || !isBot())
            return;

        setView(InputView::Default, UpdateMode::Force, SetHistoryPolicy::DontAddToHistory);
    }

    void InputWidget::cursorPositionChanged()
    {
        if (const auto currentText = getInputText(); !currentText.isEmpty())
        {
            if (currentText == textEdit_->getUndoRedoStack().top().Text())
                textEdit_->getUndoRedoStack().top().Cursor() = textEdit_->textCursor().position();
        }

        const auto mentionSignIndex = currentState().mentionSignIndex_;
        if (mentionSignIndex == -1)
            return;

        if (auto mc = getMentionCompleter())
        {
            const auto cursorPos = textEdit_->textCursor().position();

            if (mc->completerVisible())
            {
                if (cursorPos <= mentionSignIndex || !mc->itemCount())
                    mc->hideAnimated();
            }
            else if (cursorPos > mentionSignIndex)
            {
                mc->showAnimated(tooltipArrowPosition());
            }
        }
    }

    void InputWidget::inputPositionChanged()
    {
        if (currentView() == InputView::Edit)
            return;

        inputTexts_[contact_] = InputText(getInputText(), textEdit_->textCursor().position());
    }

    void InputWidget::enterPressed()
    {
        const auto mc = getMentionCompleter();
        if (mc && mc->completerVisible() && mc->hasSelectedItem())
            mc->insertCurrent();
        else
            send();
    }

    void InputWidget::send(InstantEdit _mode)
    {
        if (currentState().isDisabled())
            return;

        const auto& curState = currentState();
        const auto isEdit = isEditing() || _mode == InstantEdit::Yes;
        auto files = isEdit ? curState.edit_.message_->Files_ : curState.files_;

        const auto inputText = getInputText().trimmed();
        const auto hasMessage = curState.edit_.message_ != nullptr;
        const auto isMedia = hasMessage && !(curState.edit_.message_->GetType() == core::message_type::base);

        QString text;

        if (hasMessage && !isMedia && _mode == InstantEdit::No)
            text = inputText;
        else if (auto trimmedBufer = QStringView(curState.edit_.buffer_).trimmed(); trimmedBufer.isEmpty() && hasMessage)
            text = curState.edit_.message_->GetUrl();
        else
            text = (trimmedBufer.size() != curState.edit_.buffer_.size()) ? trimmedBufer.toString() : curState.edit_.buffer_;

        if (text.isEmpty())
            text = inputText;
        textEdit_->getUndoRedoStack().clear(HistoryUndoStack::ClearType::Full);

        if (hasMessage && isMedia && _mode == InstantEdit::No)
        {
            curState.edit_.message_->SetDescription(inputText);

            if (curState.edit_.message_->GetUrl().isEmpty() && !curState.edit_.message_->GetText().isEmpty())
                curState.edit_.message_->SetUrl(Utils::replaceFilesPlaceholders(curState.edit_.message_->GetText(), files));
        }

        if (isEdit && _mode == InstantEdit::No
            && (
                 !hasMessage
                 || (!curState.canBeEmpty_ && !inputText.isEmpty() && text == curState.edit_.message_->GetText())
                 || (curState.canBeEmpty_ && inputText.isEmpty() && inputText == currentState().edit_.editableText_)
                )
            )
        {
            clearInputText();
            clearEdit();
            switchToPreviousView();
            return;
        }

        text = Utils::replaceFilesPlaceholders(text, files);

        const auto& quotes = isEdit ? curState.edit_.quotes_ : curState.quotes_;

        if (!curState.canBeEmpty_ && text.isEmpty() && quotes.isEmpty())
            return;

        if (!isEdit)
            hideSmartreplies();

        size_t quotesLength = 0;
        for (const auto& q : quotes)
        {
            quotesLength += q.text_.size();
            files.insert(q.files_.begin(), q.files_.end());
        }

        if (quotesLength >= 0.9 * Utils::getInputMaximumChars() || (text.isEmpty() && curState.canBeEmpty_))
        {
            sendPiece({}, _mode);
            clearQuotes();
        }

        QStringRef textRef(&text);

        while (!textRef.isEmpty())
        {
            constexpr auto maxSize = Utils::getInputMaximumChars();
            const auto majorCurrentPiece = textRef.left(maxSize);

            auto spaceIdx = (static_cast<decltype(maxSize)>(textRef.size()) <= maxSize)
                            ? textRef.size()
                            : majorCurrentPiece.lastIndexOf(QChar::Space);

            if (spaceIdx == -1 || spaceIdx == 0)
                spaceIdx = majorCurrentPiece.lastIndexOf(QChar::LineFeed);
            if (spaceIdx == -1 || spaceIdx == 0)
                spaceIdx = std::min(static_cast<decltype(maxSize)>(textRef.size()), maxSize);

            sendPiece(majorCurrentPiece.left(spaceIdx), _mode);
            textRef = textRef.mid(spaceIdx);

            if (_mode == InstantEdit::No)
            {
                if (isEdit)
                    clearEdit();
                else
                    clearQuotes();
            }
        }

        if (_mode == InstantEdit::No)
            clearInputText();

        if (!isEdit)
        {
            GetSoundsManager()->playSound(SoundsManager::Sound::OutgoingMessage);
            Q_EMIT sendMessage(contact_);
        }

        if (_mode == InstantEdit::No)
            switchToPreviousView();

        if (!isEdit)
            sendStatsIfNeeded();
    }

    void InputWidget::sendPiece(const QStringRef& _msg, InstantEdit _mode)
    {
        const auto& curState = currentState();
        const auto isEdit = isEditing() || _mode == InstantEdit::Yes;
        if (isEdit)
        {
            assert(curState.edit_.message_);
            if (!curState.edit_.message_)
                return;
        }

        const auto& quotes = isEdit ? curState.edit_.quotes_ : curState.quotes_;
        const auto& files = isEdit ? curState.edit_.message_->Files_ : curState.files_;

        Ui::gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", contact_);
        collection.set_value_as_bool("channel", Logic::getContactListModel()->isChannel(contact_));

        std::optional<QString> msg;

        if (isEdit)
        {
            if (!curState.edit_.message_->GetDescription().isEmpty() && curState.edit_.message_->GetType() != core::message_type::base)
            {
                collection.set_value_as_qstring("description", curState.edit_.message_->GetDescription());
                msg = std::make_optional(curState.edit_.message_->GetUrl());
                collection.set_value_as_qstring("url", *msg);
            }
            else if (curState.edit_.message_->GetType() != core::message_type::base)
            {
                collection.set_value_as_qstring("description", _msg.mid(curState.edit_.message_->GetUrl().size() + 1));
            }
        }

        if (msg)
            collection.set_value_as_qstring("message", *msg);
        else
            collection.set_value_as_qstring("message", Utils::convertFilesPlaceholders(_msg, files));

        if (!quotes.isEmpty())
        {
            core::ifptr<core::iarray> quotesArray(collection->create_array());
            quotesArray->reserve(quotes.size());
            for (auto quote : quotes)
            {
                if (_msg != quote.description_ || _msg != quote.url_)
                {
                    quote.url_.clear();
                    quote.description_.clear();
                }
                core::ifptr<core::icollection> quoteCollection(collection->create_collection());
                quote.text_ = Utils::replaceFilesPlaceholders(quote.text_, quote.files_);
                quote.serialize(quoteCollection.get());
                core::ifptr<core::ivalue> val(collection->create_value());
                val->set_as_collection(quoteCollection.get());
                quotesArray->push_back(val.get());
            }
            collection.set_value_as_array("quotes", quotesArray.get());

            GetDispatcher()->post_stats_to_core(_msg.isEmpty() ? core::stats::stats_event_names::quotes_send_alone : core::stats::stats_event_names::quotes_send_answer);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, { { "Quotes_MessagesCount", std::to_string(quotes.size()) } });
        }

        auto& myMentions = isEdit ? curState.edit_.message_->Mentions_ : curState.mentions_;
        auto myMentionCount = myMentions.size();

        auto allMentions = isEdit ? curState.edit_.message_->Mentions_ : curState.mentions_; // copy
        for (const auto& q : quotes)
            allMentions.insert(q.mentions_.begin(), q.mentions_.end());

        if (!allMentions.empty())
        {
            QString textWithMentions;
            textWithMentions += _msg;
            for (const auto& q : quotes)
                textWithMentions.append(q.text_);

            for (auto it = allMentions.cbegin(); it != allMentions.cend();)
            {
                if (!textWithMentions.contains(u"@[" % it->first % u']'))
                {
                    if (myMentions.count(it->first))
                        --myMentionCount;

                    it = allMentions.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        if (!allMentions.empty())
        {
            core::ifptr<core::iarray> mentArray(collection->create_array());
            mentArray->reserve(allMentions.size());
            for (const auto&[aimid, friendly] : std::as_const(allMentions))
            {
                core::ifptr<core::icollection> mentCollection(collection->create_collection());
                Ui::gui_coll_helper coll(mentCollection.get(), false);
                coll.set_value_as_qstring("aimid", aimid);
                coll.set_value_as_qstring("friendly", friendly);

                core::ifptr<core::ivalue> val(collection->create_value());
                val->set_as_collection(mentCollection.get());
                mentArray->push_back(val.get());
            }
            collection.set_value_as_array("mentions", mentArray.get());

            if (myMentionCount)
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mentions_sent, { { "Mentions_Count", std::to_string(myMentionCount) } });
        }

        if (isEdit)
        {
            collection.set_value_as_int64("id", curState.edit_.message_->Id_);
            collection.set_value_as_qstring("internal_id", curState.edit_.message_->InternalId_);
            collection.set_value_as_string("update_patch_version", curState.edit_.message_->GetUpdatePatchVersion().as_string());
            collection.set_value_as_int("offline_version", curState.edit_.message_->GetUpdatePatchVersion().get_offline_version());

            if (curState.edit_.message_->Id_ > 0)
                collection.set_value_as_int64("msg_time", curState.edit_.message_->GetTime());
        }

        const auto coreMessage = isEdit ? std::string_view("update_message") : std::string_view("send_message");
        GetDispatcher()->post_message_to_core(coreMessage, collection.get());
    }

    void InputWidget::selectFileToSend(const FileSelectionFilter _filter)
    {
        QFileDialog dialog(this);
        dialog.setFileMode(QFileDialog::ExistingFiles);
        dialog.setViewMode(QFileDialog::Detail);
        dialog.setDirectory(get_gui_settings()->get_value<QString>(settings_upload_directory, QString()));

        if (_filter == FileSelectionFilter::MediaOnly)
        {
            QString extList;
            extList += ql1s("*.gif");
            extList += QChar::Space;

            for (const auto& range : { boost::make_iterator_range(Utils::getImageExtensions()), boost::make_iterator_range(Utils::getVideoExtensions()) })
                for (auto ext : range)
                    extList += u"*." % ext % QChar::Space;

            const auto list = QStringView(extList).chopped(1);

            dialog.setNameFilter(QT_TRANSLATE_NOOP("input_widget", "Photo or Video") % QChar::Space % u'(' % list % u')');
        }

        const auto res = dialog.exec();
        hideSmartreplies();

        if (!res)
            return;

        get_gui_settings()->set_value<QString>(settings_upload_directory, dialog.directory().absolutePath());

        const QStringList selectedFiles = dialog.selectedFiles();
        if (selectedFiles.isEmpty())
            return;

        clearFiles();

        auto& files = currentState().filesToSend_;
        files.reserve(selectedFiles.size());
        for (const auto& f : selectedFiles)
            files.emplace_back(f);

        showFileDialog(FileSource::Clip);
    }

    void InputWidget::showFileDialog(const FileSource _source)
    {
        if (auto& files = currentState().filesToSend_; !files.empty())
        {
            hideSmartreplies();

            const auto inputText = getInputText();
            const auto mentions = currentState().mentions_;
            if (!inputText.isEmpty())
                setInputText(QString());

            auto filesWidget = new FilesWidget(this, files);
            GeneralDialog generalDialog(filesWidget, Utils::InterConnector::instance().getMainWindow(), false, true, true, true, filesWidget->size());
            generalDialog.addButtonsPair(QT_TRANSLATE_NOOP("files_widget", "Cancel"), QT_TRANSLATE_NOOP("files_widget", "Send"), true);

            connect(filesWidget, &FilesWidget::setButtonActive, &generalDialog, &GeneralDialog::setButtonActive);

            filesWidget->setDescription(inputText, mentions);
            filesWidget->setFocusOnInput();

            if (generalDialog.showInCenter())
            {
                files = filesWidget->getFiles();
                currentState().mentions_ = filesWidget->getMentions();
                currentState().description_ = filesWidget->getDescription().trimmed();
                sendFiles(_source);
            }
            else if (!inputText.isEmpty())
            {
                currentState().mentions_ = mentions;
                setInputText(inputText);
            }
            clearFiles();
            setFocusOnInput();
        }
    }

    InputStyleMode InputWidget::currentStyleMode() const
    {
        const auto& contactWpId = Styling::getThemesContainer().getContactWallpaperId(contact_);
        const auto& themeDefWpId = Styling::getThemesContainer().getCurrentTheme()->getDefaultWallpaperId();
        return contactWpId == themeDefWpId ? InputStyleMode::Default : InputStyleMode::CustomBg;
    }

    void InputWidget::updateInputHeight()
    {
        if (auto panel = getCurrentPanel())
            setInputHeight(panel->height());
    }

    void InputWidget::setInputHeight(const int _height)
    {
        stackWidget_->setFixedHeight(_height);
        setFixedHeight(_height + (isReplying() ? quotesWidget_->height() : 0));

        updateGeometry();
        parentWidget()->layout()->activate();
    }

    void InputWidget::startPttRecord()
    {
        panelPtt_->record(contact_);
    }

    void InputWidget::stopPttRecording()
    {
        if (contact_.isEmpty())
        {
            panelPtt_->stopAll();
        }
        else
        {
            panelPtt_->stop(contact_);
            panelPtt_->enableCircleHover(contact_, false);
        }
        if (pttLock_)
            pttLock_->hideForce();
        buttonSubmit_->enableCircleHover(false);
    }

    void InputWidget::startPttRecordingLock()
    {
        startPtt(PttMode::Lock);
    }

    void InputWidget::sendPtt()
    {
        ptt::StatInfo stat;

        if (const auto mode = getPttMode(); mode)
            stat.mode = (*mode == PttMode::HoldByKeyboard || *mode == PttMode::HoldByMouse) ? "tap" : "fix";
        stat.chatType = getStatsChatType();

        stopPttRecording();
        sendPttImpl(std::move(stat));
        switchToPreviousView();
    }

    void InputWidget::sendPttImpl(ptt::StatInfo&& _statInfo)
    {
        panelPtt_->send(contact_, std::move(_statInfo));
    }

    void InputWidget::needToStopPtt()
    {
        if (panelPtt_)
            stopPttRecording();
    }

    void InputWidget::removePttRecord()
    {
        if (panelPtt_)
            panelPtt_->remove(contact_);
    }

    void InputWidget::onPttReady(const QString& _contact, const QString& _file, std::chrono::seconds _duration, const ptt::StatInfo& _statInfo)
    {
        const auto& quotes = getInputQuotes();

        GetDispatcher()->uploadSharedFile(_contact, _file, quotes, std::make_optional(_duration));

        if (_statInfo.playBeforeSent)
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_recordptt_action, { { "do", "play" } });

        core::stats::event_props_type props;
        if (!_statInfo.mode.empty())
            props.push_back({ "type", _statInfo.mode });
        props.push_back({ "duration", std::to_string(_statInfo.duration.count()) });
        props.push_back({ "chat_type", _statInfo.chatType });
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_sentptt_action, props);

        if (!quotes.isEmpty())
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, { { "Quotes_MessagesCount", std::to_string(quotes.size()) } });
            clearQuotes();
            switchToPreviousView();
        }

        notifyDialogAboutSend(_contact);
    }

    void InputWidget::onPttRemoved(const QString& _contact)
    {
        if (contact_ != _contact)
            return;

        switchToPreviousView();
        disablePttCircleHover();
    }

    void InputWidget::onPttStateChanged(const QString& _contact, ptt::State2 _state)
    {
        if (contact_ != _contact)
            return;

        if (_state == ptt::State2::Stopped)
            disablePttCircleHover();
    }

    void InputWidget::disablePttCircleHover()
    {
        buttonSubmit_->enableCircleHover(false);
        if (panelPtt_)
            panelPtt_->enableCircleHover(contact_, false);
    }

    void InputWidget::showQuotes()
    {
        if (!currentState().hasQuotes())
            return;

        if (!quotesWidget_)
        {
            quotesWidget_ = new QuotesWidget(this);
            quotesWidget_->updateStyle(currentStyleMode());

            Testing::setAccessibleName(quotesWidget_, qsl("AS ChatInput quotes"));

            connect(quotesWidget_, &QuotesWidget::cancelClicked, this, &InputWidget::onQuotesCancel);
            connect(quotesWidget_, &QuotesWidget::quoteCountChanged, this, &InputWidget::updateInputHeight);
            connect(quotesWidget_, &QuotesWidget::quoteHeightChanged, this, &InputWidget::onQuotesHeightChanged);

            if (auto vLayout = qobject_cast<QVBoxLayout*>(stackWidget_->parentWidget()->layout()))
                vLayout->insertWidget(0, quotesWidget_);
        }

        quotesWidget_->setQuotes(currentState().quotes_);
        setQuotesVisible(true);
    }

    void InputWidget::hideQuotes()
    {
        setQuotesVisible(false);
    }

    void InputWidget::setQuotesVisible(const bool _isVisible)
    {
        if (quotesWidget_ && quotesWidget_->isVisibleTo(this) != _isVisible)
        {
            quotesWidget_->setVisible(_isVisible);

            if (panelMain_)
                panelMain_->setUnderQuote(_isVisible);

            if (panelPtt_)
                panelPtt_->setUnderQuote(_isVisible);

            updateInputHeight();
            updateSubmitButtonState(UpdateMode::IfChanged);
        }
    }

    QWidget* InputWidget::getCurrentPanel() const
    {
        switch (currentView())
        {
        case InputView::Default:
        case InputView::Edit:
            return panelMain_;

        case InputView::Ptt:
            return panelPtt_;

        case InputView::Readonly:
            return panelReadonly_;

        case InputView::Disabled:
            return panelDisabled_;

        case InputView::Multiselect:
            return panelMultiselect_;

        default:
            break;
        }

        return nullptr;
    }

    void InputWidget::hideSmartreplies() const
    {
        if (auto page = Utils::InterConnector::instance().getHistoryPage(contact_))
            page->hideSmartreplies();
    }

    // check for urls in text and load meta to cache
    void InputWidget::processUrls() const
    {
        if (!config::get().is_on(config::features::snippet_in_chat))
            return;

        if (!Ui::get_gui_settings()->get_value<bool>(settings_show_video_and_images, true))
            return;

        Utils::UrlParser p;
        p.process(getInputText().trimmed());

        if (p.hasUrl())
        {
            if (auto url = p.getUrl(); !url.is_email() && !url.is_filesharing())
                SnippetCache::instance()->load(QString::fromStdString(url.url_));
        }
    }

    void InputWidget::requestSmartrepliesForQuote() const
    {
        const auto canRequest =
            Features::isSmartreplyForQuoteEnabled() &&
            Utils::canShowSmartreplies(contact_) &&
            get_gui_settings()->get_value<bool>(settings_show_smartreply, settings_show_smartreply_default()) &&
            getQuotesCount() == 1 &&
            currentState().quotes_.front().senderId_ != MyInfo()->aimId();

        if (!canRequest)
            return;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", contact_);
        collection.set_value_as_int64("msgid", currentState().quotes_.front().msgId_);

        const std::vector<core::smartreply::type> supportedTypes = { core::smartreply::type::sticker, core::smartreply::type::text };
        core::ifptr<core::iarray> values_array(collection->create_array(), true);
        values_array->reserve(supportedTypes.size());
        for (auto type : supportedTypes)
        {
            core::ifptr<core::ivalue> ival(collection->create_value(), true);
            ival->set_as_int((int)type);
            values_array->push_back(ival.get());
        }
        collection.set_value_as_array("types", values_array.get());

        Ui::GetDispatcher()->post_message_to_core("smartreply/get", collection.get());
    }

    bool InputWidget::isBot() const
    {
        return !contact_.isEmpty() && Logic::GetLastseenContainer()->isBot(contact_);
    }

    void InputWidget::createConference(ConferenceType _type)
    {
        if (!callLinkCreator_)
            callLinkCreator_ = new Utils::CallLinkCreator(Utils::CallLinkFrom::Input);
        callLinkCreator_->createCallLink(_type, contact_);
    }

    bool InputWidget::canSetFocus() const
    {
        return !contact_.isEmpty() && currentView() != InputView::Disabled;
    }

    bool InputWidget::hasServerSuggests() const
    {
        return !lastRequestedSuggests_.empty();
    }

    bool InputWidget::isServerSuggest(const QString& _stickerId) const
    {
        return std::any_of(lastRequestedSuggests_.begin(), lastRequestedSuggests_.end(), [&_stickerId](const auto& s) { return s == _stickerId; });
    }

    void InputWidget::contactSelected(const QString& _contact)
    {
        if (contact_ == _contact)
            return;

        needToStopPtt();

        contact_ = _contact;
        updateStyle();

        if (textEdit_)
        {
            textEdit_->setPlaceholderAnimationEnabled(false);
            textEdit_->getUndoRedoStack().clear(HistoryUndoStack::ClearType::Full);
        }

        if (Logic::getContactListModel()->isReadonly(_contact))
        {
            currentState().clear();
            setView(InputView::Readonly);
        }
        else if (currentState().isDisabled())
        {
            currentState().clear();
            setView(InputView::Default, UpdateMode::Force, SetHistoryPolicy::DontAddToHistory);
        }
        else
        {
            if (currentView() == InputView::Multiselect)
            {
                if (!Utils::InterConnector::instance().isMultiselect(contact_))
                    switchToPreviousView();
            }

            setView(currentView(), UpdateMode::Force, SetHistoryPolicy::DontAddToHistory);

            if (textEdit_)
            {
                textEdit_->getUndoRedoStack().push(getInputText());
                textEdit_->getReplacer().setAutoreplaceAvailable(Emoji::ReplaceAvailable::Unavailable);
            }
        }

        if (panelMultiselect_)
            panelMultiselect_->setContact(contact_);

        if (textEdit_)
            textEdit_->setPlaceholderAnimationEnabled(true);

        activateWindow();
    }

    void InputWidget::setFocusOnFirstFocusable()
    {
        const auto panel = getCurrentPanel();
        if (!panel || !canSetFocus())
            return;

        if (panel == panelMain_)
            panelMain_->setFocusOnAttach();
        else if (panel == panelReadonly_)
            panelReadonly_->setFocusOnButton();
        else if (panel == panelPtt_)
            panelPtt_->setFocusOnDelete();
    }

    void InputWidget::setFocusOnInput()
    {
        if (canSetFocus())
        {
            if (panelMain_ && getCurrentPanel() == panelMain_)
                panelMain_->setFocusOnInput();
            else
                setFocus();
        }
    }

    void InputWidget::setFocusOnEmoji()
    {
        if (canSetFocus() && panelMain_ && getCurrentPanel() == panelMain_)
            panelMain_->setFocusOnEmoji();
    }

    void InputWidget::setFocusOnAttach()
    {
        if (canSetFocus() && panelMain_ && getCurrentPanel() == panelMain_)
            panelMain_->setFocusOnAttach();
    }

    void InputWidget::onNickInserted(const int _atSignPos, const QString& _nick)
    {
        currentState().mentionSignIndex_ = _atSignPos;
        updateMentionCompleterState(true);
        showMentionCompleter(_nick, tooltipArrowPosition());
    }

    void InputWidget::insertFiles()
    {
        if (isRecordingPtt())
            return;
        isExternalPaste_ = true;
        pasteFromClipboard();
        textEdit_->getReplacer().setAutoreplaceAvailable(Emoji::ReplaceAvailable::Unavailable);
    }

    void InputWidget::onContactChanged(const QString& _contact)
    {
        if (contact_ != _contact)
            return;

        if (currentView() == InputView::Readonly)
        {
            if (Utils::isChat(contact_))
                chatRoleChanged(contact_);
            else if (!Logic::getIgnoreModel()->contains(contact_))
                setView(InputView::Default, UpdateMode::Force, SetHistoryPolicy::DontAddToHistory);
        }
    }

    void InputWidget::multiselectChanged()
    {
        if (Utils::InterConnector::instance().isMultiselect(contact_))
            setView(InputView::Multiselect, UpdateMode::Force, SetHistoryPolicy::AddToHistory);
        else if (currentView() == InputView::Multiselect)
            switchToPreviousView();

        updateBackground();
    }

    void InputWidget::onRecvPermitDeny()
    {
        onContactChanged(contact_);
    }

    static auto getMediaType(MediaType type)
    {
        switch (type)
        {
        case MediaType::mediaTypeFsPhoto:
        case MediaType::mediaTypeFsVideo:
        case MediaType::mediaTypeFsGif:
            return core::message_type::file_sharing;
        default:
            break;
        }
        return core::message_type::base;
    }

    void InputWidget::edit(const int64_t _msgId, const QString& _internalId, const common::tools::patch_version& _patchVersion, const QString& _text, const Data::MentionMap& _mentions, const Data::QuotesVec& _quotes, int32_t _time, const Data::FilesPlaceholderMap& _files, MediaType _mediaType, bool _instantEdit)
    {
        const auto mode = _instantEdit ? InstantEdit::Yes : InstantEdit::No;
        editImpl(_msgId, _internalId, _patchVersion, _quotes, _time, _files, _mediaType, mode, _text, _mentions, {}, {});
    }

    void InputWidget::editWithCaption(const int64_t _msgId, const QString& _internalId, const common::tools::patch_version& _patchVersion, const QString& _url, const QString& _description, const Data::MentionMap& _mentions, const Data::QuotesVec& _quotes, int32_t _time, const Data::FilesPlaceholderMap& _files, MediaType _mediaType, bool _instantEdit)
    {
        const auto mode = _instantEdit ? InstantEdit::Yes : InstantEdit::No;
        editImpl(_msgId, _internalId, _patchVersion, _quotes, _time, _files, _mediaType, mode, {}, _mentions, _url, _description);
    }

    void InputWidget::editImpl(
        const int64_t _msgId,
        const QString& _internalId,
        const common::tools::patch_version& _patchVersion,
        const Data::QuotesVec& _quotes,
        qint32 _time,
        const Data::FilesPlaceholderMap& _files,
        MediaType _mediaType,
        InstantEdit _mode,
        std::optional<QString> _text,
        std::optional<Data::MentionMap> _mentions,
        std::optional<QString> _url,
        std::optional<QString> _description)
    {
        if (_msgId == -1 && _internalId.isEmpty())
            return;

        auto copyState = _mode == InstantEdit::Yes ? std::make_optional(currentState()) : std::nullopt;

        Data::MessageBuddySptr message = std::make_shared<Data::MessageBuddy>();
        message->AimId_ = contact_;
        message->Id_ = _msgId;
        message->InternalId_ = _internalId;
        message->SetUpdatePatchVersion(_patchVersion);
        message->Files_ = _files;
        message->SetTime(_time);
        const auto type = getMediaType(_mediaType);
        message->SetType(type);

        if (_mentions)
            message->Mentions_ = std::move(*_mentions);

        if (_description && _url)
        {
            message->SetDescription(*_description);
            message->SetUrl(*_url);
        }
        else if (_text)
        {
            message->SetText(*_text);
        }
        currentState().edit_.message_ = std::move(message);
        currentState().edit_.quotes_ = _quotes;
        currentState().edit_.type_ = _mediaType;

        if (_description)
        {
            currentState().edit_.buffer_ = *_description;
            currentState().edit_.editableText_ = std::move(*_description);
            currentState().canBeEmpty_ = type != core::message_type::base || !_quotes.empty();
        }
        else if (_text)
        {
            currentState().edit_.buffer_ = (type == core::message_type::base) ? *_text : QString();
            currentState().edit_.editableText_ = *std::move(_text);
            if (_mode == InstantEdit::No)
            currentState().canBeEmpty_ = type != core::message_type::base || !_quotes.empty();
        }

        if (_mode == InstantEdit::Yes)
        {
            send(_mode);
            currentState() = std::move(*copyState);
        }
        else
        {
            panelMain_->forceSendButton(currentState().canBeEmpty_);
            setEditView();
        }
    }

    bool InputWidget::isAllAttachmentsEmpty(const QString& _contact) const
    {
        const auto it = states_.find(_contact);
        if (it == states_.end())
            return true;

        const auto& contactState = it->second;
        return contactState.quotes_.isEmpty() && contactState.filesToSend_.empty();
    }

    bool InputWidget::shouldOfferMentions() const
    {
        auto cursor = textEdit_->textCursor();
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);

        const auto text = cursor.selectedText();
        if (text.isEmpty())
            return true;

        const auto lastChar = text.at(0);
        if (lastChar.isSpace() || qsl(".,:;_-+=!?\"\'#").contains(lastChar)) // use QStringView since 5.14
            return true;

        return false;
    }

    InputWidgetState& InputWidget::currentState()
    {
        assert(!contact_.isEmpty());
        return states_[contact_];
    }

    const InputWidgetState& InputWidget::currentState() const
    {
        assert(!contact_.isEmpty());
        return states_.find(contact_)->second;
    }

    InputView InputWidget::currentView() const
    {
        return currentView(contact_);
    }

    InputView InputWidget::currentView(const QString& _contact) const
    {
        const auto it = states_.find(_contact);
        if (it == states_.end())
            return InputView::Default;

        return it->second.view_;
    }

    void InputWidget::updateMentionCompleterState(const bool _force)
    {
        auto& mentionSignIndex = currentState().mentionSignIndex_;
        if (mentionSignIndex != -1)
        {
            const auto text = textEdit_->document()->toPlainText();
            const auto noAtSign = text.length() <= mentionSignIndex || text.at(mentionSignIndex) != symbolAt();

            if (noAtSign)
            {
                mentionSignIndex = -1;
                Q_EMIT Utils::InterConnector::instance().hideMentionCompleter();
                return;
            }

            if (auto mc = getMentionCompleter())
            {
                auto cursor = textEdit_->textCursor();
                cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, cursor.position() - mentionSignIndex - 1);
                mc->setSearchPattern(cursor.selectedText(), _force ? Logic::MentionModel::SearchCondition::Force : Logic::MentionModel::SearchCondition::IfPatternChanged);
                mc->setArrowPosition(tooltipArrowPosition());
            }
        }
    }

    void InputWidget::switchToMainPanel(const StateTransition _transition)
    {
        initSubmitButton();
        if (!panelMain_)
        {
            panelMain_ = new InputPanelMain(this, buttonSubmit_);
            panelMain_->updateStyle(currentStyleMode());
            panelMain_->setUnderQuote(isReplying());

            Testing::setAccessibleName(panelMain_, qsl("AS ChatInput panelMain"));
            stackWidget_->addWidget(panelMain_);

            connect(panelMain_, &InputPanelMain::editCancelClicked, this, &InputWidget::onEditCancel);
            connect(panelMain_, &InputPanelMain::editedMessageClicked, this, &InputWidget::goToEditedMessage);
            connect(panelMain_, &InputPanelMain::emojiButtonClicked, this, &InputWidget::onStickersClicked);
            connect(panelMain_, &InputPanelMain::panelHeightChanged, this, [this](const int)
            {
                if (getCurrentPanel() == panelMain_)
                    updateInputHeight();
            });

            textEdit_ = panelMain_->getTextEdit();
            assert(textEdit_);
            connect(textEdit_->document(), &QTextDocument::contentsChanged, this, &InputWidget::editContentChanged, Qt::QueuedConnection);
            connect(textEdit_, &HistoryTextEdit::selectionChanged, this, &InputWidget::selectionChanged, Qt::QueuedConnection);
            connect(textEdit_, &HistoryTextEdit::cursorPositionChanged, this, &InputWidget::cursorPositionChanged);
            connect(textEdit_, &HistoryTextEdit::enter, this, &InputWidget::enterPressed);
            connect(textEdit_, &HistoryTextEdit::enter, this, &InputWidget::statsMessageEnter);
            connect(textEdit_, &HistoryTextEdit::textChanged, this, &InputWidget::textChanged, Qt::QueuedConnection);
            connect(textEdit_, &HistoryTextEdit::focusOut, this, &InputWidget::editFocusOut);
            connect(textEdit_, &HistoryTextEdit::cursorPositionChanged, this, &InputWidget::inputPositionChanged);
            connect(textEdit_, &HistoryTextEdit::nickInserted, this, &InputWidget::onNickInserted);
            connect(textEdit_, &HistoryTextEdit::insertFiles, this, &InputWidget::insertFiles);
            connect(textEdit_, &HistoryTextEdit::needsMentionComplete, this, [this](const auto _pos) { mentionOffer(_pos, CursorPolicy::Set); });
        }

        if (const auto cur = stackWidget_->currentWidget(); cur == nullptr || cur != panelMain_)
            panelMain_->recalcHeight();

        if (_transition != StateTransition::Animated)
            updateInputHeight();

        setCurrentWidget(panelMain_);
        setFocusOnInput();
    }

    void InputWidget::switchToReadonlyPanel(const ReadonlyPanelState _state)
    {
        if (!panelReadonly_)
        {
            panelReadonly_ = new InputPanelReadonly(this);
            panelReadonly_->updateStyle(currentStyleMode());

            Testing::setAccessibleName(panelReadonly_, qsl("AS ChatInput panelReadonly"));
            stackWidget_->addWidget(panelReadonly_);
        }

        panelReadonly_->setAimid(contact_);
        panelReadonly_->setState(_state);

        updateInputHeight();
        setCurrentWidget(panelReadonly_);
    }

    void InputWidget::switchToDisabledPanel(const DisabledPanelState _state)
    {
        if (!panelDisabled_)
        {
            panelDisabled_ = new InputPanelDisabled(this);
            panelDisabled_->updateStyle(currentStyleMode());

            Testing::setAccessibleName(panelDisabled_, qsl("AS ChatInput panelDisabled"));
            stackWidget_->addWidget(panelDisabled_);
        }

        panelDisabled_->setState(_state);

        updateInputHeight();
        setCurrentWidget(panelDisabled_);
    }

    void InputWidget::switchToPttPanel(const StateTransition _transition)
    {
        initSubmitButton();
        if (!panelPtt_)
        {
            panelPtt_ = new InputPanelPtt(this, buttonSubmit_);
            panelPtt_->updateStyle(currentStyleMode());

            Testing::setAccessibleName(panelPtt_, qsl("AS ChatInput panelPtt"));
            stackWidget_->addWidget(panelPtt_);

            connect(panelPtt_, &InputPanelPtt::pttReady, this, &InputWidget::onPttReady);
            connect(panelPtt_, &InputPanelPtt::pttRemoved, this, &InputWidget::onPttRemoved);
            connect(panelPtt_, &InputPanelPtt::stateChanged, this, &InputWidget::onPttStateChanged);
        }

        hideSmartreplies();

        panelPtt_->open(contact_);
        panelPtt_->setUnderQuote(isReplying());

        if (_transition == StateTransition::Force)
            updateInputHeight();

        if (setFocusToSubmit_)
            buttonSubmit_->setFocus(Qt::TabFocusReason);
        setFocusToSubmit_ = false;

        setCurrentWidget(panelPtt_);
    }

    void InputWidget::switchToMultiselectPanel()
    {
        if (!panelMultiselect_)
        {
            panelMultiselect_ = new InputPanelMultiselect(this);
            panelMultiselect_->updateStyle(currentStyleMode());
            panelMultiselect_->setContact(contact_);

            Testing::setAccessibleName(panelMultiselect_, qsl("AS ChatInput panelMultiselect"));
            stackWidget_->addWidget(panelMultiselect_);
        }

        setCurrentWidget(panelMultiselect_);
        updateInputHeight();
        panelMultiselect_->updateElements();
    }

    void InputWidget::setCurrentWidget(QWidget* _widget)
    {
        AttachFilePopup::hidePopup();

        assert(_widget);
        const auto prev = stackWidget_->currentWidget();
        if (_widget == prev)
            return;

        if (auto focused = qobject_cast<QWidget*>(qApp->focusObject()); focused && prev && prev->isAncestorOf(focused))
            focused->clearFocus();

        stackWidget_->setCurrentWidget(_widget);

        updateSubmitButtonPos();

        if (buttonSubmit_)
        {
            buttonSubmit_->setGraphicsEffect(nullptr);
            buttonSubmit_->setVisible(_widget == panelMain_ || _widget == panelPtt_);
            buttonSubmit_->raise();
        }
    }

    void InputWidget::initSubmitButton()
    {
        if (buttonSubmit_)
            return;

        buttonSubmit_ = new SubmitButton(this);
        buttonSubmit_->setFixedSize(Utils::scale_value(QSize(32, 32)));
        buttonSubmit_->updateStyle(currentStyleMode());
        buttonSubmit_->setState(SubmitButton::State::Ptt, StateTransition::Force);
        Testing::setAccessibleName(buttonSubmit_, qsl("AS ChatInput buttonSubmit"));
        connect(buttonSubmit_, &SubmitButton::clicked, this, &InputWidget::onSubmitClicked);
        connect(buttonSubmit_, &SubmitButton::pressed, this, &InputWidget::onSubmitPressed);
        connect(buttonSubmit_, &SubmitButton::released, this, &InputWidget::onSubmitReleased);
        connect(buttonSubmit_, &SubmitButton::longTapped, this, &InputWidget::onSubmitLongTapped);
        connect(buttonSubmit_, &SubmitButton::longPressed, this, &InputWidget::onSubmitLongPressed);
        connect(buttonSubmit_, &SubmitButton::mouseMovePressed, this, &InputWidget::onSubmitMovePressed);

        auto circleHover = std::make_unique<CircleHover>();
        circleHover->setColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY, 0.28));
        buttonSubmit_->setCircleHover(std::move(circleHover));
        buttonSubmit_->setRectExtention(QMargins(Utils::scale_value(8), 0, 0, 0));
    }

    void InputWidget::onSubmitClicked(const ClickType _clickType)
    {
        const auto view = currentView();
        if (view == InputView::Default || view == InputView::Edit)
        {
            Q_EMIT Utils::InterConnector::instance().hideMentionCompleter();
            if (view == InputView::Default && isInputEmpty() && !isReplying())
            {
                setFocusToSubmit_ = (_clickType == ClickType::ByKeyboard);
                startPtt(PttMode::Lock);
                sendStat(core::stats::stats_event_names::chatscr_openptt_action, "primary");
            }
            else
            {
                send();
                if (view != InputView::Edit)
                    statsMessageSend();
            }
        }
        else if (view == InputView::Ptt)
        {
            sendPtt();
        }

        if (view != InputView::Edit)
            hideSmartreplies();
    }

    void InputWidget::onSubmitPressed()
    {
    }

    void InputWidget::onSubmitReleased()
    {
        const auto scoped = Utils::ScopeExitT([this]()
        {
            disablePttCircleHover();
        });
        if (!panelPtt_)
            return;
        if (const auto view = currentView(); view == InputView::Ptt)
        {
            if (pttLock_)
                pttLock_->hideForce();
            if (pttMode_ != PttMode::HoldByMouse)
                return;
            if (std::exchange(canLockPtt_, false))
            {
                setPttMode(PttMode::Lock);
                return;
            }
            if (panelPtt_->removeIfUnderMouse(contact_))
                return;
            if (buttonSubmit_->containsCursorUnder())
            {
                sendPtt();
                return;
            }
            if (const auto pos = QCursor::pos(); rect().contains(mapFromGlobal(pos)))
                stopPttRecording();
        }
    }

    void InputWidget::onSubmitLongTapped()
    {
        if (const auto view = currentView(); view == InputView::Default && isInputEmpty())
        {
            Q_EMIT Utils::InterConnector::instance().hideMentionCompleter();

            if (buttonSubmit_)
                buttonSubmit_->setFocus();
            setFocusToSubmit_ = true;
            startPtt(PttMode::HoldByMouse);
        }
    }

    void InputWidget::onSubmitLongPressed()
    {
        if (const auto view = currentView(); view == InputView::Default && isInputEmpty())
        {
            Q_EMIT Utils::InterConnector::instance().hideMentionCompleter();

            startPtt(PttMode::HoldByKeyboard);
        }
    }

    void InputWidget::onSubmitMovePressed()
    {
        if (panelPtt_ && !contact_.isEmpty() && currentView() == InputView::Ptt)
        {
            if (pttMode_ != PttMode::HoldByMouse)
                return;
            panelPtt_->pressedMouseMove(contact_);

            if (!pttLock_)
                pttLock_ = new PttLock(qobject_cast<QWidget*>(parent()));

            const auto g = geometry();
            auto b = g.bottomLeft();
            b.rx() += g.width() / 2 - pttLock_->width() / 2;
            b.ry() -= (g.height() / 2 + pttLock_->height() / 2);
            pttLock_->setBottom(b);

            const bool newCanLocked = panelPtt_->canLock(contact_) && !(buttonSubmit_->containsCursorUnder());

            if (newCanLocked != canLockPtt_)
            {
                if (newCanLocked)
                    pttLock_->showAnimated();
                else
                    pttLock_->hideAnimated();

                canLockPtt_ = newCanLocked;
            }
        }
    }

    void InputWidget::startPtt(PttMode _mode)
    {
        if (contact_.isEmpty())
            return;
        auto startCallBack = [pThis = QPointer(this), _mode, contact = contact_](bool _allowed)
        {
            if (!_allowed)
                return;

            if (!pThis)
                return;

            if (contact != pThis->contact_)
                return;

            if (ptt::AudioRecorder2::hasDevice())
            {
                pThis->setView(InputView::Ptt);
                pThis->startPttRecord();
                pThis->setPttMode(_mode);
                pThis->updateSubmitButtonState(UpdateMode::IfChanged);
            }
            else
            {
                static bool isShow = false;
                if (!isShow)
                {
                    QScopedValueRollback scoped(isShow, true);
                    const auto res = Utils::GetConfirmationWithOneButton(QT_TRANSLATE_NOOP("input_widget", "OK"),
                        QT_TRANSLATE_NOOP("input_widget", "Could not detect a microphone. Please ensure that the device is available and enabled."),
                        QT_TRANSLATE_NOOP("input_widget", "Microphone not found"),
                        nullptr);
                    /*if (res)
                    {
                        if (const auto page = Utils::InterConnector::instance().getMainPage())
                            page->settingsTabActivate(Utils::CommonSettingsType::CommonSettingsType_VoiceVideo);
                    }*/
                }
            }
        };
        static bool isShow = false;
        const auto p = media::permissions::checkPermission(media::permissions::DeviceType::Microphone);
        if (p == media::permissions::Permission::Allowed)
        {
            startCallBack(true);
        }
        else if (p == media::permissions::Permission::NotDetermined)
        {
#ifdef __APPLE__

            if (!isShow)
            {
                buttonSubmit_->clearFocus();
                isShow = true;
                auto startCallBackEnsureMainThread = [startCallBack, pThis = QPointer(this)](bool _allowed)
                {
                    if (pThis)
                        QMetaObject::invokeMethod(pThis, [_allowed, startCallBack]() { startCallBack(_allowed); isShow = false; });
                };

                media::permissions::requestPermission(media::permissions::DeviceType::Microphone, startCallBackEnsureMainThread);
            }
#else
            assert(!"unimpl");
#endif //__APPLE__
        }
        else
        {
            if (!isShow)
            {
                QScopedValueRollback scoped(isShow, true);
                const auto res = Utils::GetConfirmationWithTwoButtons(QT_TRANSLATE_NOOP("input_widget", "Cancel"), QT_TRANSLATE_NOOP("input_widget", "Open settings"),
                    QT_TRANSLATE_NOOP("input_widget", "To record a voice message, you need to allow access to the microphone in the system settings"),
                    QT_TRANSLATE_NOOP("input_widget", "Microphone permissions"),
                    nullptr);
                if (res)
                    media::permissions::openPermissionSettings(media::permissions::DeviceType::Microphone);
            }
        }
    }

    void InputWidget::setPttMode(const std::optional<PttMode>& _mode)
    {
        pttMode_ = _mode;

        const auto isHoldByKeyboard = pttMode_ == PttMode::HoldByKeyboard;
        const auto isHoldMode = isHoldByKeyboard || pttMode_ == PttMode::HoldByMouse;

        if (buttonSubmit_)
            buttonSubmit_->enableCircleHover(isHoldMode);
        if (panelPtt_ && !contact_.isEmpty())
        {
            panelPtt_->setUnderLongPress(contact_, isHoldByKeyboard);
            panelPtt_->enableCircleHover(contact_, isHoldMode);
        }
    }

    std::optional<InputWidget::PttMode> InputWidget::getPttMode() const noexcept
    {
        return pttMode_;
    }

    void InputWidget::setView(InputView _view, const UpdateMode _updateMode, const SetHistoryPolicy _historyPolicy)
    {
        Q_EMIT Utils::InterConnector::instance().hideMentionCompleter();

        if (auto contactDialog = Utils::InterConnector::instance().getContactDialog())
            contactDialog->hideSmilesMenu();

        const auto curView = currentView();
        if (curView == _view && _updateMode != UpdateMode::Force)
            return;

        if (_historyPolicy == SetHistoryPolicy::AddToHistory && curView != _view)
            addViewToHistory(curView);

        const auto animatedTransition = curView != _view && (curView == InputView::Ptt || _view == InputView::Ptt);

        auto currentHeight = getDefaultInputHeight();
        if (auto panel = getCurrentPanel())
            currentHeight = panel->height();

        if (animatedTransition)
        {
            if (!transitionLabel_)
            {
                transitionLabel_ = new QLabel(this);
                transitionLabel_->setAttribute(Qt::WA_TranslucentBackground);
                transitionLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
                transitionLabel_->setGraphicsEffect(new QGraphicsOpacityEffect(transitionLabel_));
            }

            transitionLabel_->setGeometry(rect());

            QPixmap pm(size());
            pm.fill(Qt::transparent);
            render(&pm, QPoint(), QRegion(), DrawChildren);

            if (auto effect = qobject_cast<QGraphicsOpacityEffect*>(transitionLabel_->graphicsEffect()))
                effect->setOpacity(1.0);

            transitionLabel_->setPixmap(pm);
            transitionLabel_->raise();
            transitionLabel_->show();
        }

        if (_view != InputView::Multiselect && isBot())
        {
            if (const auto page = Utils::InterConnector::instance().getHistoryPage(contact_))
            {
                if (page->needStartButton() || !page->hasMessages())
                    _view = InputView::Readonly;
                else if (!currentState().msgHistoryReady_)
                    _view = InputView::Disabled;
            }
            else
            {
                _view = InputView::Disabled;
            }
        }

        currentState().view_ = _view;

        if (Logic::getIgnoreModel()->contains(contact_))
        {
            currentState().clear(InputView::Readonly);
            switchToReadonlyPanel(ReadonlyPanelState::Unblock);
        }
        else if (_view != InputView::Multiselect && Utils::isChat(contact_) && !Logic::getContactListModel()->hasContact(contact_))
        {
            currentState().clear(InputView::Readonly);
            switchToReadonlyPanel(ReadonlyPanelState::AddToContactList);
        }
        else if (_view == InputView::Readonly || _view == InputView::Disabled)
        {
            if (isBot())
            {
                if (_view == InputView::Disabled)
                {
                    currentState().clear(InputView::Disabled);
                    switchToDisabledPanel(DisabledPanelState::Empty);
                }
                else
                {
                    currentState().clear(InputView::Readonly);
                    switchToReadonlyPanel(ReadonlyPanelState::Start);
                }
            }
            else
            {
                const auto role = Logic::getContactListModel()->getYourRole(contact_);
                if (role == u"notamember")
                {
                    switchToReadonlyPanel(ReadonlyPanelState::DeleteAndLeave);
                }
                else if (role == u"pending")
                {
                    currentState().clear(InputView::Readonly);
                    switchToReadonlyPanel(ReadonlyPanelState::CancelJoin);
                }
                else if (Logic::getContactListModel()->isChannel(contact_))
                {
                    const auto state = Logic::getContactListModel()->isMuted(contact_) ? ReadonlyPanelState::EnableNotifications : ReadonlyPanelState::DisableNotifications;
                    switchToReadonlyPanel(state);
                }
                else
                {
                    currentState().clear(InputView::Disabled);
                    switchToDisabledPanel(DisabledPanelState::Banned);
                }
            }
        }
        else if (_view == InputView::Ptt)
        {
            switchToPttPanel(animatedTransition ? StateTransition::Animated : StateTransition::Force);
            updateSubmitButtonState(UpdateMode::IfChanged);
            updateMainMenu();
        }
        else if (_view == InputView::Multiselect)
        {
            switchToMultiselectPanel();
        }
        else
        {
            switchToMainPanel(animatedTransition ? StateTransition::Animated : StateTransition::Force);
            updateMainMenu();

            if (_view == InputView::Edit)
            {
                auto& currState = currentState();
                panelMain_->showEdit(currState.edit_.message_, currState.edit_.type_);

                assert(currState.edit_.message_);
                if (currState.edit_.message_)
                {
                    textEdit_->limitCharacters(Utils::getInputMaximumChars());
                    textEdit_->setMentions(currentState().edit_.message_->Mentions_);
                    textEdit_->setFiles(currentState().edit_.message_->Files_);

                    QString textToEdit = currentState().edit_.buffer_;
                    if (currState.edit_.type_ == MediaType::mediaTypeFsPhoto || currState.edit_.type_ == MediaType::mediaTypeFsVideo || currState.edit_.type_ == MediaType::mediaTypeFsGif)
                    {
                        auto textWithPlaceholder = currState.edit_.message_->GetText();
                        auto placeholderEnd = textWithPlaceholder.indexOf(ql1c(']'));
                        placeholderEnd = placeholderEnd == -1 ? 0 : placeholderEnd; // in case we remove placeholders (IMDESKTOP-12050)
                        const auto textFromMessage = textWithPlaceholder.mid(placeholderEnd + 1);
                        auto spaceIDX = textFromMessage.indexOf(ql1c('\n'));
                        if (spaceIDX == -1)
                            spaceIDX = textFromMessage.indexOf(ql1c(' '));

                        if (spaceIDX == -1)
                        {
                            textToEdit = currState.edit_.message_->GetDescription();
                        }
                        else
                        {
                            currState.edit_.message_->SetUrl(Utils::replaceFilesPlaceholders(textWithPlaceholder.left(placeholderEnd + 1), currentState().edit_.message_->Files_));
                            currState.edit_.message_->SetText(textFromMessage.left(spaceIDX + 1));
                            textToEdit = textFromMessage.mid(spaceIDX + 1);
                            currState.edit_.editableText_ = textToEdit;
                        }
                    }

                    setInputText(textToEdit);
                    textEdit_->getUndoRedoStack().push(getInputText());
                }
            }
            else
            {
                if (!isEditing() && !isReplying())
                    panelMain_->hidePopups();

                textEdit_->limitCharacters(-1);
                textEdit_->setMentions(currentState().mentions_);
                textEdit_->setFiles(currentState().files_);
                loadInputText();
            }

            updateSubmitButtonState(UpdateMode::IfChanged);
            setFocusOnInput();
        }

        if ((_view == InputView::Default || _view == InputView::Ptt) && currentState().hasQuotes())
            showQuotes();
        else
            hideQuotes();

        if (animatedTransition)
        {
            if (auto effect = qobject_cast<QGraphicsOpacityEffect*>(transitionLabel_->graphicsEffect()))
            {
                auto targetHeight = currentHeight;
                if (auto panel = getCurrentPanel())
                    targetHeight = panel->height();

                transitionAnim_->stop();
                transitionAnim_->disconnect(this);

                connect(transitionAnim_, &QVariantAnimation::valueChanged, this, [this, effect, targetHeight, diff = currentHeight - targetHeight]()
                {
                    const auto val = transitionAnim_->currentValue().toDouble();
                    effect->setOpacity(val);

                    if (diff != 0)
                        setInputHeight(targetHeight + val * diff);
                });
                connect(transitionAnim_, &QVariantAnimation::stateChanged, this, [this]()
                {
                    if (transitionAnim_->state() == QAbstractAnimation::Stopped)
                        transitionLabel_->hide();
                });

                transitionAnim_->setStartValue(1.0);
                transitionAnim_->setEndValue(0.0);
                transitionAnim_->setDuration(viewTransitDuration().count());
                transitionAnim_->setEasingCurve(QEasingCurve::InOutSine);
                transitionAnim_->start();

            }
        }

        if (_view != InputView::Ptt)
            setPttMode(std::nullopt);

        show();
        updateGeometry();
        parentWidget()->layout()->activate();
    }

    void InputWidget::switchToPreviousView()
    {
        auto& viewHistory = currentState().viewHistory_;
        if (viewHistory.empty())
        {
            setView(InputView::Default, UpdateMode::Force, SetHistoryPolicy::DontAddToHistory);
            return;
        }

        setView(viewHistory.back(), UpdateMode::Force, SetHistoryPolicy::DontAddToHistory);
        if (!viewHistory.empty())
            viewHistory.pop_back();
    }

    void InputWidget::addViewToHistory(const InputView _view)
    {
        auto& viewHistory = currentState().viewHistory_;
        viewHistory.erase(std::remove(viewHistory.begin(), viewHistory.end(), _view), viewHistory.end());
        viewHistory.push_back(_view);
    }

    void InputWidget::updateSubmitButtonState(const UpdateMode _updateMode)
    {
        if (!buttonSubmit_ || currentState().isDisabled())
            return;

        const auto transit = _updateMode == UpdateMode::IfChanged ? StateTransition::Animated : StateTransition::Force;
        const auto curView = currentView();

        if (curView == InputView::Edit)
            buttonSubmit_->setState(SubmitButton::State::Edit, transit);
        else if (curView == InputView::Ptt)
            buttonSubmit_->setState(SubmitButton::State::Send, transit);
        else if (isInputEmpty() && !isReplying())
            buttonSubmit_->setState(SubmitButton::State::Ptt, transit);
        else
            buttonSubmit_->setState(SubmitButton::State::Send, transit);
    }

    void InputWidget::updateSubmitButtonPos()
    {
        if (buttonSubmit_)
        {
            const auto diff = std::max(0, (width() - MessageStyle::getHistoryWidgetMaxWidth()) / 2);
            const auto x = width() - diff - getHorSpacer() - buttonSubmit_->width();
            const auto y = height() - buttonSubmit_->height() - (getDefaultInputHeight() - buttonSubmit_->height()) / 2;
            buttonSubmit_->move(x, y);
        }
    }

    void InputWidget::notifyDialogAboutSend(const QString& _contact)
    {
        if (const auto contactDialog = Utils::InterConnector::instance().getContactDialog(); contactDialog)
            contactDialog->onSendMessage(_contact);
    }

    void InputWidget::sendFiles(const FileSource _source)
    {
        const auto& files = currentState().filesToSend_;

        if (files.empty())
            return;

        bool mayQuotesSent = false;
        const auto& quotes = getInputQuotes();
        const auto amount = files.size();

        auto descriptionInside = false;
        if (amount == 1)
        {
            if (files.front().isFile())
            {
                const auto& fi = files.front().getFileInfo();
                descriptionInside = Utils::is_image_extension(fi.suffix()) || Utils::is_video_extension(fi.suffix());
            }
            else
            {
                descriptionInside = true;
            }
        }

        if (!currentState().description_.isEmpty() && !descriptionInside)
        {
            GetDispatcher()->sendMessageToContact(contact_, currentState().description_, quotes, getInputMentions());
            notifyDialogAboutSend(contact_);
            mayQuotesSent = true;
        }

        auto sendQuotesOnce = !mayQuotesSent;
        for (const auto& f : files)
        {
            if (f.getSize() == 0)
                continue;

            const auto& quotesToSend = sendQuotesOnce ? quotes : Data::QuotesVec();
            const auto& descToSend = descriptionInside ? currentState().description_ : QString();
            const auto& mentionsToSend = descriptionInside ? getInputMentions() : Data::MentionMap();

            if (f.isFile())
            {
                GetDispatcher()->uploadSharedFile(contact_, f.getFileInfo().absoluteFilePath(), quotesToSend, descToSend, mentionsToSend);
            }
            else
            {
                QByteArray array;
                QBuffer b(&array);
                b.open(QIODevice::WriteOnly);
                f.getPixmap().save(&b, "png");

                GetDispatcher()->uploadSharedFile(contact_, array, u".png", quotesToSend, descToSend, mentionsToSend);
            }

            notifyDialogAboutSend(contact_);
            if (sendQuotesOnce)
            {
                mayQuotesSent = true;
                sendQuotesOnce = false;
            }
        }

        if (amount > 0)
        {
            std::string from;
            switch (_source)
            {
            case FileSource::Clip:
                from = "attach";
                break;
            case FileSource::CopyPaste:
                from = "copypaste";
                break;
            }

            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_sendmedia_action, { { "chat_type", Utils::chatTypeByAimId(contact_) },{ "count", amount > 1 ? "multi" : "single" },{ "type", from } });
            if (!currentState().description_.isEmpty())
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_sendmediawcapt_action, { { "chat_type", Utils::chatTypeByAimId(contact_) },{ "count", amount > 1 ? "multi" : "single" },{ "type", from } });
        }

        if (mayQuotesSent && !quotes.isEmpty())
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, { { "Quotes_MessagesCount", std::to_string(quotes.size()) } });
            clearQuotes();
            switchToPreviousView();
        }

        sendStatsIfNeeded();
    }

    void InputWidget::textChanged()
    {
        if (!isVisible())
            return;

        updateMentionCompleterState();

        suggestRequested_ = false;

        if (suggestTimer_)
        {
            const auto text = getInputText();
            if (!text.contains(symbolAt()) && !text.contains(QChar::LineFeed))
            {
                const auto time = suggestPos_.isNull() ? Features::getSuggestTimerInterval() : singleEmojiSuggestDuration();
                suggestTimer_->start(time);
            }
        }
        Q_EMIT hideSuggets();

        processUrls();

        if (isInputEmpty())
            textEdit_->getReplacer().setAutoreplaceAvailable();

        RegisterTextChange(textEdit_);

        Q_EMIT inputTyped();

        AttachFilePopup::instance().setPersistent(false);
        AttachFilePopup::hidePopup();
    }

    void InputWidget::statsMessageEnter()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::message_enter_button);
    }

    void InputWidget::statsMessageSend()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::message_send_button);
    }

    void InputWidget::clearQuotes()
    {
        currentState().quotes_.clear();
        currentState().canBeEmpty_ = false;

        if (quotesWidget_)
            quotesWidget_->clearQuotes();
    }

    void InputWidget::clearFiles()
    {
        currentState().description_.clear();
        currentState().filesToSend_.clear();
    }

    void InputWidget::clearMentions()
    {
        currentState().mentions_.clear();
    }

    void InputWidget::clearEdit()
    {
        currentState().edit_.message_.reset();
        currentState().edit_.buffer_.clear();
        currentState().edit_.quotes_.clear();
        currentState().edit_.editableText_.clear();
        currentState().canBeEmpty_ = false;
        currentState().edit_.type_ = MediaType::noMedia;
    }

    void InputWidget::clearInputText()
    {
        setInputText(QString());
    }

    void InputWidget::loadInputText()
    {
        const auto& input = inputTexts_[contact_];
        setInputText(input.text_, input.cursorPos_);
    }

    void InputWidget::setInputText(const QString& _text, int _pos)
    {
        {
            QSignalBlocker sb(textEdit_);
            textEdit_->setPlainText(_text, false);
            if (_pos != -1)
            {
                auto cursor = textEdit_->textCursor();
                cursor.setPosition(_pos);
                textEdit_->setTextCursor(cursor);
            }
        }

        editContentChanged();

        updateMentionCompleterState(true);
        cursorPositionChanged();
    }

    QString InputWidget::getInputText() const
    {
        return textEdit_->getPlainText();
    }

    const Data::MentionMap& InputWidget::getInputMentions() const
    {
        return currentState().mentions_;
    }

    const Data::QuotesVec& Ui::InputWidget::getInputQuotes()
    {
        for (auto& quote : currentState().quotes_)
            quote.text_ = Utils::replaceFilesPlaceholders(quote.text_, quote.files_);
        return currentState().quotes_;
    }

    const Data::FilesPlaceholderMap & Ui::InputWidget::getInputFiles() const
    {
        return currentState().files_;
    }

    int InputWidget::getQuotesCount() const
    {
        if (!contact_.isEmpty())
            if (const auto it = states_.find(contact_); it != states_.end())
                return it->second.quotes_.size();
        return 0;
    }

    bool InputWidget::isInputEmpty() const
    {
        return currentView() == InputView::Default && getInputText().trimmed().isEmpty();
    }

    bool InputWidget::isEditing() const
    {
        return currentView() == InputView::Edit;
    }

    void InputWidget::hideAndClear()
    {
        Q_EMIT Utils::InterConnector::instance().hideMentionCompleter();

        if (auto contactDialog = Utils::InterConnector::instance().getContactDialog())
            contactDialog->hideSmilesMenu();

        hide();
        contact_.clear();
        if (pttLock_)
            pttLock_->hideForce();
    }

    void InputWidget::resizeEvent(QResizeEvent*)
    {
        if (transitionLabel_ && transitionLabel_->isVisible())
            transitionLabel_->move(0, height() - transitionLabel_->height());

        updateBackgroundGeometry();
        updateSubmitButtonPos();

        parentWidget()->layout()->activate();
        Q_EMIT resized();
    }

    void InputWidget::showEvent(QShowEvent *)
    {
        updateBackgroundGeometry();
    }

    QPoint InputWidget::tooltipArrowPosition() const
    {
        if (!textEdit_)
            return QPoint();

        auto cursor = textEdit_->textCursor();

        QChar currentChar;
        while (currentChar != symbolAt())
        {
            if (cursor.atStart())
                break;

            cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
            currentChar = *cursor.selectedText().cbegin();
        }

        if (currentChar != symbolAt())
            return QPoint();

        auto cursorPos = textEdit_->mapToGlobal(textEdit_->cursorRect().topLeft());

        QFontMetrics currentMetrics(textEdit_->font());
        cursorPos.rx() -= currentMetrics.horizontalAdvance(cursor.selectedText());

        return cursorPos;
    }

    void InputWidget::sendStatsIfNeeded() const
    {
        if (contact_ == Utils::getStickerBotAimId())
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_discover_sendbot_message);

        if (Logic::getRecentsModel()->isSuspicious(contact_))
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_unknown_message);
    }

    bool InputWidget::shouldOfferCompleter(const CheckOffer _check) const
    {
        const auto mc = getMentionCompleter();
        return
            (currentState().mentionSignIndex_ == -1 || (mc && !mc->completerVisible())) &&
            textEdit_ &&
            textEdit_->isVisible() &&
            textEdit_->isEnabled() &&
            (_check == CheckOffer::No || shouldOfferMentions());
    }

    void InputWidget::updateStyle()
    {
        const auto mode = currentStyleMode();

        const auto styleVar = mode == InputStyleMode::Default
            ? Styling::StyleVariable::CHAT_ENVIRONMENT
            : Styling::StyleVariable::BASE_GLOBALWHITE;
        auto color = Styling::getParameters().getColor(styleVar, getBlurAlpha());
        bgWidget_->setOverlayColor(color);

        const std::vector<StyledInputElement*> elems = {
            quotesWidget_,
            panelMain_,
            panelReadonly_,
            panelPtt_,
            panelDisabled_,
            buttonSubmit_,
            panelMultiselect_
        };

        for (auto e: elems)
            if (e)
                e->updateStyle(mode);
    }

    void InputWidget::updateBackgroundGeometry()
    {
        bgWidget_->setGeometry(rect());
        bgWidget_->update();
    }

    void InputWidget::onAttachPhotoVideo()
    {
        selectFileToSend(FileSelectionFilter::MediaOnly);
    }

    void InputWidget::onAttachFile()
    {
        selectFileToSend(FileSelectionFilter::AllFiles);
    }

    void InputWidget::onAttachCamera()
    {
        hideSmartreplies();
    }

    void InputWidget::onAttachContact()
    {
        hideSmartreplies();

        if (!selectContactsWidget_)
            selectContactsWidget_ = new SelectContactsWidget(nullptr, Logic::MembersWidgetRegim::SHARE_CONTACT, QT_TRANSLATE_NOOP("profilesharing", "Share contact"), QT_TRANSLATE_NOOP("popup_window", "Cancel"), Ui::MainPage::instance());

        if (selectContactsWidget_->show() == QDialog::Accepted)
        {
            const auto selectedContacts = Logic::getContactListModel()->getCheckedContacts();
            if (!selectedContacts.empty())
            {
                const auto& aimId = selectedContacts.front();

                const auto wid = new TransitProfileSharing(Utils::InterConnector::instance().getMainWindow(), aimId);

                connect(wid, &TransitProfileSharing::accepted, this, [this, aimId, wid](const QString& _nick, const bool _sharePhone)
                {
                    QString link;

                    const auto& quotes = getInputQuotes();

                    if (Features::isNicksEnabled() && !_nick.isEmpty() && !_sharePhone)
                    {
                        link = u"https://" % getProfileUrl() % u'/' % _nick;
                    }
                    else
                    {
                        if (const auto& phone = wid->getPhone(); _sharePhone && !phone.isEmpty())
                        {
                            Ui::sharePhone(Logic::GetFriendlyContainer()->getFriendly(aimId), phone, { contact_ }, aimId, quotes);
                            if (!quotes.isEmpty())
                            {
                                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, { { "Quotes_MessagesCount", std::to_string(quotes.size()) } });
                                Q_EMIT needClearQuotes();
                            }
                            sendShareStat(true);
                            clearContactModel(selectContactsWidget_);
                            wid->deleteLater();
                            notifyDialogAboutSend(contact_);
                            return;
                        }
                        else
                        {
                            Utils::UrlParser p;
                            p.process(aimId);
                            if (p.hasUrl() && p.getUrl().is_email())
                            {
                                link = u"https://" % Features::getProfileDomainAgent() % u'/' % aimId;
                            }
                            else if (Utils::isUin(aimId))
                            {
                                link = u"https://" % Features::getProfileDomain() % u'/' % aimId;
                            }
                            else
                            {
                                sendShareStat(false);
                                clearContactModel(selectContactsWidget_);
                                wid->deleteLater();
                                return;
                            }
                        }
                    }

                    Data::Quote q;
                    q.text_ = std::move(link);
                    q.type_ = Data::Quote::Type::link;

                    Ui::shareContact({ std::move(q) }, contact_, quotes);
                    if (!quotes.isEmpty())
                    {
                        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, { { "Quotes_MessagesCount", std::to_string(quotes.size()) } });
                        Q_EMIT needClearQuotes();
                    }
                    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sharingscr_choicecontact_action, { { "count", Utils::averageCount(1) } });
                    sendShareStat(true);
                    clearContactModel(selectContactsWidget_);
                    wid->deleteLater();
                    notifyDialogAboutSend(contact_);
                });

                connect(wid, &TransitProfileSharing::declined, this, [this, wid]()
                {
                    wid->deleteLater();
                    sendShareStat(false);
                    onAttachContact();
                });

                connect(wid, &TransitProfileSharing::openChat, this, [this, aimId]()
                {
                    clearContactModel(selectContactsWidget_);
                    Q_EMIT Utils::InterConnector::instance().openDialogOrProfileById(aimId);

                    if (auto mp = Utils::InterConnector::instance().getMainPage(); mp && mp->getFrameCount() != FrameCountMode::_1)
                        mp->showSidebar(aimId);

                    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sharecontactscr_contact_action, { {"chat_type", Ui::getStatsChatType() }, { "type", std::string("internal") }, { "status", std::string("not_sent") } });
                });

                wid->showProfile();
            }
        }
        else
        {
            selectContactsWidget_->close();
            clearContactModel(selectContactsWidget_);
        }
    }

    void InputWidget::onAttachPtt()
    {
        startPtt(PttMode::Lock);
        sendStat(core::stats::stats_event_names::chatscr_openptt_action, "plus");
    }

    void InputWidget::onAttachPoll()
    {
        hideSmartreplies();

        sendStat(core::stats::stats_event_names::polls_open, "plus");

        auto pollWidget = new Ui::PollWidget(contact_);
        auto generalDialog = std::make_unique<GeneralDialog>(pollWidget, Utils::InterConnector::instance().getMainWindow(), true);
        pollWidget->setFocus();
        pollWidget->setFocusOnQuestion();
        pollWidget->setInputData(getInputText().trimmed(), currentState().quotes_, currentState().mentions_);

        if (generalDialog->showInCenter())
        {
            clearInputText();
            hideQuotes();
            clear();
            notifyDialogAboutSend(contact_);
        }
    }

    void Ui::InputWidget::onAttachCallByLink()
    {
        hideSmartreplies();

        createConference(ConferenceType::Call);
    }

    void Ui::InputWidget::onAttachWebinar()
    {
        hideSmartreplies();

        createConference(ConferenceType::Webinar);
    }

    QRect InputWidget::getAttachFileButtonRect() const
    {
        assert(panelMain_);
        if (panelMain_)
            return panelMain_->getAttachFileButtonRect();

        return QRect();
    }

    void InputWidget::updateBackground()
    {
        bgWidget_->forceUpdate();
    }

    void InputWidget::clear()
    {
        clearQuotes();
        clearFiles();
        clearMentions();
        clearEdit();
    }

    void InputWidget::setEditView()
    {
        if (!currentState().edit_.message_)
            return;

        hideSmartreplies();

        setView(InputView::Edit, UpdateMode::Force);
    }

    void InputWidget::onEditCancel()
    {
        const auto currentContact = Logic::getContactListModel()->selectedContact();
        if (!currentContact.isEmpty())
        {
            if (auto page = Utils::InterConnector::instance().getHistoryPage(currentContact))
                page->editBlockCanceled();
        }

        clearEdit();

        switchToPreviousView();
    }

    void InputWidget::onStickersClicked(const bool _fromKeyboard)
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_switchtostickers_action);
        Q_EMIT smilesMenuSignal(_fromKeyboard, QPrivateSignal());
        hideSmartreplies();
    }

    void InputWidget::onQuotesCancel()
    {
        onQuotesHeightChanged();
        clearQuotes();
        hideQuotes();

        Q_EMIT quotesDropped(QPrivateSignal());
    }

    void InputWidget::chatRoleChanged(const QString& _contact)
    {
        if (contact_ != _contact)
            return;

        if (textEdit_)
            textEdit_->getUndoRedoStack().clear(HistoryUndoStack::ClearType::Full);

        if (Logic::getContactListModel()->isReadonly(_contact))
        {
            currentState().clear();
            setView(InputView::Readonly, UpdateMode::Force);
        }
        else if (currentState().isDisabled())
        {
            currentState().clear();
            setView(InputView::Default, UpdateMode::Force, SetHistoryPolicy::DontAddToHistory);
        }
    }

    void InputWidget::goToEditedMessage()
    {
        if (isEditing())
        {
            assert(currentState().edit_.message_);
            if (currentState().edit_.message_)
            {
                if (const auto msgId = currentState().edit_.message_->Id_; msgId != -1)
                    Logic::getContactListModel()->setCurrent(Logic::getContactListModel()->selectedContact(), msgId, true);
            }
        }
    }

    void InputWidget::hideAndRemoveDialog(const QString& _contact)
    {
        if (panelPtt_)
            panelPtt_->close(_contact);
        const auto it = states_.find(_contact);
        if (it == states_.end())
            return;

        if (contact_ == _contact)
            hideAndClear();

        states_.erase(it);
    }

    void InputWidget::onRequestedStickerSuggests(const QVector<QString>& _suggests)
    {
        if (_suggests.empty())
        {
            suggestRequested_ = false;
            return;
        }

        clearLastSuggests();

        if (const auto text = getInputText().toLower().trimmed(); !text.isEmpty())
        {
            const auto suggestType = TextRendering::isEmoji(text) ? Stickers::SuggestType::suggestEmoji : Stickers::SuggestType::suggestWord;

            for (const auto& stickerId : _suggests)
            {
                if (!isServerSuggest(stickerId))
                    lastRequestedSuggests_.push_back(stickerId);
                Ui::Stickers::addStickers({ stickerId }, text, suggestType);
            }

            suggestRequested_ = true;
            onSuggestTimeout();
            if (!lastRequestedSuggests_.empty())
                GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::text_sticker_suggested);
        }
    }

    void InputWidget::onQuotesHeightChanged()
    {
        if (auto page = Utils::InterConnector::instance().getHistoryPage(contact_))
            page->notifyQuotesResize();
    }

    void InputWidget::requestMoreStickerSuggests()
    {
        if (!suggestRequested_)
        {
            const auto inputText = getInputText();
            const auto words = inputText.splitRef(QChar::Space);
            const QString suggestText = inputText.toLower().trimmed();

            if (words.size() <= Features::maxAllowedSuggestWords() && suggestText.length() <= Features::maxAllowedSuggestChars() && !suggestText.contains(symbolAt()))
                requestStickerSuggests();
        }
    }

    void InputWidget::clearLastSuggests()
    {
        lastRequestedSuggests_.clear();
    }

    bool InputWidget::hasSelection() const
    {
        return !textEdit_->selection().isEmpty();
    }
}
