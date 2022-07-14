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
#include "panels/PanelMultiselect.h"
#include "panels/DraftVersionWidget.h"
#ifndef STRIP_AV_MEDIA
#include "panels/PanelPtt.h"
#endif // !STRIP_AV_MEDIA

#include "../history_control/MentionCompleter.h"
#include "../history_control/HistoryControlPage.h"
#include "../history_control/TransitProfileShareWidget.h"
#include "../history_control/MessageStyle.h"
#include "../history_control/SnippetCache.h"
#include "../history_control/history/MessageBuilder.h"
#include "../history_control/feeds/ThreadFeedItem.h"

#include "../../core_dispatcher.h"
#include "../../gui_settings.h"

#include "../../controls/GeneralDialog.h"
#include "../../controls/BackgroundWidget.h"
#include "../../controls/TooltipWidget.h"
#include "../../controls/CustomButton.h"
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
#include "../../main_window/smiles_menu/SuggestsWidget.h"
#include "../../main_window/containers/FriendlyContainer.h"
#include "../../main_window/containers/LastseenContainer.h"
#include "../../main_window/containers/InputStateContainer.h"
#include "../tasks/TaskEditWidget.h"
#include "../contact_list/ServiceContacts.h"

#include "../../utils/gui_coll_helper.h"
#include "../../utils/exif.h"
#include "../../utils/utils.h"
#include "../../utils/async/AsyncTask.h"
#include "../../utils/log/log.h"
#include "../../utils/UrlParser.h"
#include "../../utils/features.h"
#include "../../utils/stat_utils.h"

#include "../../styles/ThemeParameters.h"
#include "../../styles/ThemesContainer.h"
#include "../../cache/stickers/stickers.h"

#include "../../previewer/toast.h"

#ifndef STRIP_AV_MEDIA
#include "../../main_window/sounds/SoundsManager.h"
#include "media/ptt/AudioRecorder2.h"
#include "media/ptt/AudioUtils.h"
#endif // !STRIP_AV_MEDIA

#include "media/permissions/MediaCapturePermissions.h"

#include "../../../common.shared/smartreply/smartreply_types.h"
#include "../../../common.shared/config/config.h"
#include "../../../common.shared/string_utils.h"
#include "../../../common.shared/im_statistics.h"

namespace
{
    constexpr std::chrono::milliseconds viewTransitDuration() noexcept { return std::chrono::milliseconds(100); }
    constexpr std::chrono::milliseconds singleEmojiSuggestDuration() noexcept { return std::chrono::milliseconds(100); }
    constexpr std::chrono::milliseconds mentionCompleterTimeout() noexcept { return std::chrono::milliseconds(200); }

    constexpr auto symbolAt() noexcept { return u'@'; }

    constexpr double getBlurAlpha() noexcept { return 0.45; }

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

    int quotesLength(const Data::QuotesVec& _quotes)
    {
        auto res = 0;
        for (const auto& q : _quotes)
            res += q.text_.size();

        return res;
    }

    Data::FStringView getTextPart(Data::FStringView _text)
    {
        constexpr auto maxSize = Utils::getInputMaximumChars();
        decltype(maxSize) textSize = _text.size();
        const auto majorCurrentPiece = _text.left(std::min(maxSize, textSize));

        auto partSize = _text.size();

        if (partSize > maxSize)
            partSize = majorCurrentPiece.lastIndexOf(QChar::Space);

        if (partSize == -1 || partSize == 0)
            partSize = majorCurrentPiece.lastIndexOf(QChar::LineFeed);
        if (partSize == -1 || partSize == 0)
            partSize = std::min(textSize, maxSize);

        return majorCurrentPiece.left(partSize);
    }

    std::unordered_set<QStringView, Utils::QStringViewHasher> extractMentions(QStringView _text)
    {
        static constexpr QStringView marker = u"@[";

        std::unordered_set<QStringView, Utils::QStringViewHasher> mentions;
        auto i = 0;
        while (i < _text.size())
        {
            const auto startPos = _text.indexOf(marker, i);
            if (startPos == -1 || startPos == _text.size() - marker.size())
                break;

            const auto endPos = _text.indexOf(u']', startPos);
            if (endPos == -1)
                break;

            mentions.insert(_text.mid(startPos + marker.size(), endPos - startPos - marker.size()));
            i = endPos;
        }
        return mentions;
    }

    constexpr core::message_type getMediaType(Ui::MediaType _type) noexcept
    {
        switch (_type)
        {
        case Ui::MediaType::mediaTypeFsPhoto:
        case Ui::MediaType::mediaTypeFsVideo:
        case Ui::MediaType::mediaTypeFsGif:
            return core::message_type::file_sharing;
        default:
            break;
        }
        return core::message_type::base;
    }

    bool isDraftChanged(const Data::MessageBuddy& _l, const Data::MessageBuddy& _r)
    {
        if (_l.GetSourceText() != _r.GetSourceText())
            return true;

        if (_l.Quotes_.size() != _r.Quotes_.size())
            return !_l.GetSourceText().isEmpty(); // ignore quotes difference for drafts with empty texts

        for (auto i = 0; i < _l.Quotes_.size(); i++)
        {
            if (_l.Quotes_[i].text_.string() != _r.Quotes_[i].text_.string())
                return true;
        }
        return false;
    }

    auto bottomOffsetStickersSuggest() noexcept
    {
        return Utils::scale_value(10);
    }
}

namespace Ui
{
    InputWidget::InputWidget(const QString& _contact, QWidget* _parent, InputFeatures _features, BackgroundWidget* _bg)
        : QWidget(_parent)
        , IEscapeCancellable(this)
        , contact_(_contact)
        , bgWidget_(_bg ? new InputBgWidget(this, _bg) : nullptr)
        , stackWidget_(new QStackedWidget(this))
        , features_(_features)
    {
        im_assert(!contact_.isEmpty());

        installEventFilter(this);
        registerInputWidgetInstance(this);

        auto& instance = Logic::InputStateContainer::instance();
        state_ = (_features & InputFeature::LocalState) ? instance.createState(contact_) : instance.getOrCreateState(contact_);

        setStyleSheet(qsl("background-color: transparent"));

        auto host = new QWidget(this);
        host->setMaximumWidth(MessageStyle::getHistoryWidgetMaxWidth());

        auto rootLayout = Utils::emptyHLayout(this);
        rootLayout->addWidget(host);

        auto hostLayout = Utils::emptyVLayout(host);

        Testing::setAccessibleName(stackWidget_, qsl("AS ChatInput stackWidget"));
        hostLayout->addWidget(stackWidget_);

        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

        connect(Logic::getContactListModel(), &Logic::ContactListModel::yourRoleChanged, this, &InputWidget::chatRoleChanged);
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &InputWidget::onContactChanged);
        connect(Logic::getIgnoreModel(), &Logic::IgnoreMembersModel::changed, this, &InputWidget::ignoreListChanged);
        connect(GetDispatcher(), &core_dispatcher::recvPermitDeny, this, &InputWidget::onRecvPermitDeny);

        connect(GetDispatcher(), &core_dispatcher::stickersRequestedSuggestsResult, this, &InputWidget::onRequestedStickerSuggests);
        connect(GetDispatcher(), &core_dispatcher::draft, this, &InputWidget::onDraft);

        if (features_ & InputFeature::Suggests)
            connect(GetDispatcher(), &core_dispatcher::stickersRequestedSuggestsResult, this, &InputWidget::onRequestedStickerSuggests);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::stopPttRecord, this, &InputWidget::stopPttRecording);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::clearInputText, this, &InputWidget::clearInputText);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::historyReady, this, &InputWidget::onHistoryReady);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::historyCleared, this, &InputWidget::onHistoryCleared);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::historyInsertedMessages, this, &InputWidget::onHistoryInsertedMessages);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::applicationLocked, this, &InputWidget::stopPttRecording);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectAnimationUpdate, this, &InputWidget::updateBackground);

        connect(this, &InputWidget::quotesCountChanged, this, [this](int){ saveDraft(); }, Qt::QueuedConnection); // saveDraft after call finished

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::filesWidgetOpened, this, &InputWidget::onFilesWidgetOpened);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::inputTextUpdated, this, [this](const QString& _contact)
        {
            if (contact_ == _contact)
                loadInputText();
        });
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::reloadDraft, this, [this](const QString& _contact)
        {
            if (contact_ == _contact)
                requestDraft();
        });
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::clearInputQuotes, this, [this](const QString& _contact)
        {
            if (contact_ == _contact)
                onQuotesCancel();
        });

        connect(GetDispatcher(), &core_dispatcher::userInfo, this, [this](const int64_t, const QString& _aimId, const Data::UserInfo& _info)
        {
            if (_info.lastseen_.isBot() && _aimId == contact_)
            {
                if (const auto needButton = needStartButton(); needButton && *needButton)
                    forceUpdateView();
            }
        });

        if (bgWidget_)
        {
            bgWidget_->lower();
            bgWidget_->forceUpdate();
        }

        connect(qApp, &QApplication::focusChanged, this, [this](QWidget*, QWidget* _new)
        {
            if (_new && (isAncestorOf(_new) || _new == this))
                setActive(true);
        });

        setInitialState();

        connect(Logic::getContactListModel(), &Logic::ContactListModel::deletedUser, this, [this]()
        {
            // initalstate when add deleted contact
            setInitialState();
        });

        Testing::setAccessibleName(this, qsl("AS ChatInput InputWidget"));
    }

    InputWidget::~InputWidget()
    {
        deregisterInputWidgetInstance(this);
    }

    bool InputWidget::isReplying() const
    {
        return quotesWidget_ && quotesWidget_->isVisibleTo(this);
    }

    bool InputWidget::isDraftVersionVisible() const
    {
        return panelMain_ && panelMain_->isDraftVersionVisible();
    }

    bool InputWidget::isRecordingPtt() const
    {
        return currentView() == InputView::Ptt;
    }

    bool InputWidget::tryPlayPttRecord()
    {
#ifndef STRIP_AV_MEDIA
        return panelPtt_ && isRecordingPtt() && panelPtt_->tryPlay();
#else
        return false;
#endif // !STRIP_AV_MEDIA
    }

    bool InputWidget::tryPausePttRecord()
    {
#ifndef STRIP_AV_MEDIA
        return panelPtt_ && isRecordingPtt() && panelPtt_->tryPause();
#else
        return false;
#endif // !STRIP_AV_MEDIA
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

#ifndef STRIP_AV_MEDIA
        escCancel_->removeChild(panelPtt_);
#endif // !STRIP_AV_MEDIA
    }

    void InputWidget::dropReply()
    {
        if (isReplying())
            onQuotesCancel();
    }

    void InputWidget::pasteFromClipboard()
    {
        auto mimeData = QApplication::clipboard()->mimeData();

        if (!mimeData)
            return;

        if (Utils::haveText(mimeData)) // try text
        {
            QString text = mimeData->text();
            if (text.isEmpty())
                return;

            if (Utils::isBase64EncodedImage(text))
            {
                pasteClipboardBase64Image(std::move(text));
                showFileDialog(FileSource::CopyPaste);
            }
            updateInputData();
            return;
        }

        currentState().filesToSend_.clear();
        currentState().sendAsFile_ = false;
        if (mimeData->hasUrls()) // try URLs
        {
            bool hadImage = mimeData->hasImage();
            currentState().description_.clear();
            const QList<QUrl> urlList = mimeData->urls();
            for (const auto& url : urlList)
                pasteClipboardUrl(url, hadImage, mimeData);
        }

        // if no local URLs found, try images
        if (currentState().filesToSend_.empty() && mimeData->hasImage())
            pasteClipboardImage(mimeData->imageData().value<QImage>());

        showFileDialog(FileSource::CopyPaste);
    }

    void InputWidget::updateInputData()
    {
        if (!textEdit_)
            return;

        const auto& textEditMentions = textEdit_->getMentions();
        currentState().getMentionsRef().insert(textEditMentions.begin(), textEditMentions.end());

        const auto& textEditFiles = textEdit_->getFiles();
        currentState().getFilesRef().insert(textEditFiles.begin(), textEditFiles.end());
    }

    void InputWidget::pasteClipboardUrl(const QUrl& _url, bool _hadImage, const QMimeData* _mimeData)
    {
        if (!_url.isValid())
            return;

        if (_url.isLocalFile())
        {
            auto path = _url.toLocalFile();
            if (const auto fi = QFileInfo(path); fi.exists() && !fi.isBundle() && !fi.isDir())
            {
                const auto imageGif = qsl("image/gif");
                const bool isGif = _mimeData->hasFormat(imageGif) && _mimeData->data(imageGif) == path.toUtf8();
                currentState().filesToSend_.emplace_back(std::move(path), isGif);
            }
        }
        else
        {
            if (_hadImage)
                return;

            if (QString text = _url.toString(); Utils::isBase64EncodedImage(text))
                pasteClipboardBase64Image(std::move(text));
        }
    }

    void InputWidget::pasteClipboardImage(QImage&& _image)
    {
        if (QPixmap pixmap = QPixmap::fromImage(std::move(_image)); !pixmap.isNull())
            currentState().filesToSend_.emplace_back(std::move(pixmap));
    }

    void InputWidget::pasteClipboardBase64Image(QString&& _text)
    {
        QImage image;
        QSize size;

        clearInputText(); // it's important to clear text to avoid doubling Base64 data in input field
        if (Utils::loadBase64Image(std::move(_text).toLatin1(), QSize(), image, size))
            pasteClipboardImage(std::move(image));
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

    void InputWidget::keyReleaseEvent(QKeyEvent* _e)
    {
        if (!textEdit_)
            return QWidget::keyReleaseEvent(_e);

        if constexpr (platform::is_apple())
        {
            if (const auto mainPage = MainPage::instance(); mainPage && mainPage->isSemiWindowVisible() && !mainPage->isSidebarWithThreadPage())
                    return;

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

        auto inputText = getInputText();
        const auto textIsEmpty = inputText.isTrimmedEmpty();
        const auto isEdit = isEditing();

        if (!isEdit)
            startDraftTimer();

        currentState().setText(std::move(inputText), textEdit_->textCursor().position());

        if (textIsEmpty && textEdit_->getUndoRedoStack().isEmpty())
        {
            currentState().mentionSignIndex_ = -1;

            if (!isEditing())
                currentState().mentions_.clear();
        }

        updateSubmitButtonState(UpdateMode::IfChanged);
    }

    void InputWidget::selectionChanged()
    {
        if (textEdit_->selection().isEmpty())
            return;

        if (historyPage_)
            historyPage_->clearSelection();
    }

    void InputWidget::onSuggestTimeout()
    {
        if (!isSuggestsEnabled())
            return;

        const auto text = getInputText().string().trimmed();
        const auto words = text.splitRef(QChar::Space, Qt::SkipEmptyParts);
        const auto suggestText = text.toLower();

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

        Q_EMIT needSuggets(suggestText, suggestPosition(), QPrivateSignal());
    }

    void InputWidget::requestStickerSuggests()
    {
        if (!isSuggestsEnabled() || !Features::isServerSuggestsEnabled())
            return;

        const auto text = getInputText().string().trimmed();
        if (text.isEmpty())
            return;

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

        if (isEditing())
            onEditCancel();

        auto quotes = _quotes;
        auto isQuote = [](const auto& _quote) { return _quote.type_ == Data::Quote::Type::quote && _quote.hasReply_; };
        const auto allQuotes = std::all_of(quotes.begin(), quotes.end(), isQuote);

        if (!allQuotes)
            quotes.erase(std::remove_if(quotes.begin(), quotes.end(), isQuote), quotes.end());

        currentState().quotes_ += quotes;
        currentState().canBeEmpty_ = true;

        Q_EMIT quotesCountChanged(currentState().quotes_.size(), QPrivateSignal());

        if (currentView() == InputView::Multiselect)
            setView(InputView::Default, UpdateMode::Force, SetHistoryPolicy::DontAddToHistory);
        else
            showQuotes();

        requestSmartrepliesForQuote();
        startDraftTimer();
        cancelDraftVersionByEdit();
        setFocusOnInput();
    }

    void InputWidget::insertEmoji(const Emoji::EmojiCode& _code, const QPoint _pt)
    {
        if (isInputEmpty())
        {
            suggestPos_ = _pt;
            emojiPos_ = _pt;
        }

        textEdit_->insertEmoji(_code);

        setFocusOnInput();

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_emoji_added_in_input);
    }

    void InputWidget::sendSticker(const Utils::FileSharingId& _stickerId)
    {
        Stickers::lockStickerCache(_stickerId);

        Ui::gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", contact_);
        collection.set_value_as_bool("fs_sticker", true);
        collection.set_value_as_qstring("message", Stickers::getSendUrl(_stickerId));

        if (Logic::getContactListModel()->isChannel(contact_))
            collection.set_value_as_qstring("chat_sender", contact_);

        if (!isEditing())
        {
            if (const auto& quotes = currentState().getQuotes(); !quotes.isEmpty())
            {
                Data::serializeQuotes(collection, quotes, Data::ReplaceFilesPolicy::Replace);

                Data::MentionMap quoteMentions;
                for (const auto& q : quotes)
                    quoteMentions.insert(q.mentions_.begin(), q.mentions_.end());
                Data::serializeMentions(collection, quoteMentions);

                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_send_answer);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, { { "Quotes_MessagesCount", std::to_string(quotes.size()) } });

                onQuotesCancel();
            }
        }

        GetDispatcher()->post_message_to_core("send_message", collection.get());

        if (!isEditing())
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

        auto& mentionMap = currentState().getMentionsRef();
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

            if (mentionCompleter_)
                posShift = mentionCompleter_->getSearchPattern().size() + 1;
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

    void InputWidget::mentionCompleterResults(int _count)
    {
        if (_count > 0 && currentState().mentionSignIndex_ != -1)
            mentionCompleter_->showAnimated(tooltipArrowPosition());
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
        if (!isEditing())
            return;

        if (auto& editedMessage = currentState().edit_->message_)
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

        if (!mentionCompleter_)
            return;

        const auto cursorPos = textEdit_->textCursor().position();

        if (mentionCompleter_->completerVisible())
        {
            if (cursorPos <= mentionSignIndex || !mentionCompleter_->itemCount())
                mentionCompleter_->hideAnimated();
        }
        else if (cursorPos > mentionSignIndex && shouldOfferCompleter(CheckOffer::Yes))
        {
            mentionCompleter_->showAnimated(tooltipArrowPosition());
        }
    }

    void InputWidget::inputPositionChanged()
    {
        if (currentView() == InputView::Edit || contact_.isEmpty())
            return;

        currentState().text_.cursorPos_ = textEdit_->textCursor().position();
    }

    void InputWidget::enterPressed()
    {
        if (!isActive())
            return;
        if (mentionCompleter_ && mentionCompleter_->completerVisible() && mentionCompleter_->hasSelectedItem())
        {
            mentionCompleter_->insertCurrent();
        }
        else if (auto panel = getCurrentPanel())
        {
            if (panel == panelMain_)
                send();
#ifndef STRIP_AV_MEDIA
            else if (panel == panelPtt_)
                onSubmitClicked(ClickType::ByMouse);
#endif // !STRIP_AV_MEDIA
        }
    }

    void InputWidget::send()
    {
        const auto& curState = currentState();
        if (curState.isDisabled())
            return;

        const auto inputText = getInputText();
        const auto inputTextTrimmed = Data::FStringView(inputText).trimmed();
        const auto isEdit = isEditing();
        const auto isEditMedia = isEdit && !(curState.edit_->message_->GetType() == core::message_type::base);

        std::decay_t<decltype(inputText)> text;

        const auto editAddedDescription = isEditMedia && curState.edit_->message_->GetDescription().isEmpty();
        const auto editBuffer = isEdit ? Data::FStringView(curState.edit_->buffer_).trimmed() : Data::FStringView();

        if (isEdit && !isEditMedia || editAddedDescription)
            text = inputText;
        else if (isEdit && editBuffer.isEmpty())
            text = curState.edit_->message_->GetUrl();
        else
            text = editBuffer.toFString();

        if (text.isEmpty())
            text = inputTextTrimmed.toFString();
        textEdit_->getUndoRedoStack().clear(HistoryUndoStack::ClearType::Full);

        if (isEdit && isEditMedia)
        {
            curState.edit_->message_->SetDescription(inputTextTrimmed.toFString());

            if (curState.edit_->message_->GetUrl().isEmpty() && !curState.edit_->message_->GetText().isEmpty())
                curState.edit_->message_->SetUrl(Utils::replaceFilesPlaceholders(curState.edit_->message_->GetText(), curState.getFiles()));
        }

        const auto canBeAndEmpty = curState.canBeEmpty_ && inputTextTrimmed.isEmpty();
        if (isEdit && (
            (!canBeAndEmpty && text == curState.edit_->message_->GetSourceText()) ||
            (canBeAndEmpty && Data::FStringView(curState.edit_->editableText_).trimmed().isEmpty())))
        {
            clearInputText();
            clearEdit();
            textEdit_->clearCache();
            switchToPreviousView();
            return;
        }

        Utils::replaceFilesPlaceholders(text, curState.getFiles());

        const auto& quotes = curState.getQuotes();

        if (!curState.canBeEmpty_ && Data::FStringView(text).trimmed().isEmpty() && quotes.isEmpty())
            return;

        if (isEdit)
        {
            GetDispatcher()->updateMessage(makeMessage(text));
            clearEdit();
            textEdit_->clearCache();
        }
        else
        {
            hideSmartreplies();

            if (quotesLength(quotes) >= 0.9 * Utils::getInputMaximumChars() || (text.isEmpty() && curState.canBeEmpty_))
            {
                GetDispatcher()->sendMessage(makeMessage({}));
                clearQuotes();
            }

            auto textView = Data::FStringView(text);
            while (!textView.isEmpty())
            {
                auto part = getTextPart(textView);
                textView = textView.mid(part.size());

                SendMessageParams params;
                if (!currentState().draft_.isEmpty())
                {
                    params.draftDeleteTime_ = QDateTime::currentDateTime().toTime_t();
                    currentState().draft_ = Data::Draft();
                }

                GetDispatcher()->sendMessage(makeMessage(part), params);
                clearQuotes();
            }

            clearDraft();
        }

        if (!isEdit)
        {
#ifndef STRIP_AV_MEDIA
            GetSoundsManager()->playSound(SoundType::OutgoingMessage);
#endif // !STRIP_AV_MEDIA

            Q_EMIT sendMessage(contact_);
        }

        clearInputText();
        switchToPreviousView();
        textEdit_->clearCache();

        if (!isEdit)
            sendStatsIfNeeded();
    }

    void InputWidget::selectFileToSend(const FileSelectionFilter _filter)
    {
        QWidget* parent = this;
        if constexpr (platform::is_linux())
            parent = Utils::InterConnector::instance().getMainWindow();

        QFileDialog dialog(parent);
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

        currentState().sendAsFile_ = _filter == FileSelectionFilter::AllFiles;

        showFileDialog(FileSource::Clip);
    }

    void InputWidget::showFileDialog(const FileSource _source)
    {
        if (auto& files = currentState().filesToSend_; !files.empty())
        {
            hideSmartreplies();
            Q_EMIT hideSuggets();

            const auto inputText = getInputText();
            const auto mentions = currentState().mentions_;
            if (!inputText.isEmpty())
                onFilesWidgetOpened(contact_);

            const auto target = Logic::getContactListModel()->isThread(contact_) ? FilesWidget::Target::Thread : FilesWidget::Target::Chat;
            auto filesWidget = new FilesWidget(files, target, this);
            GeneralDialog::Options opt;
            opt.preferredWidth_ = filesWidget->width();
            GeneralDialog generalDialog(filesWidget, Utils::InterConnector::instance().getMainWindow(), opt);
            generalDialog.addButtonsPair(QT_TRANSLATE_NOOP("files_widget", "Cancel"), QT_TRANSLATE_NOOP("files_widget", "Send"));

            connect(filesWidget, &FilesWidget::setButtonActive, &generalDialog, &GeneralDialog::setButtonActive);

            filesWidget->setDescription(inputText, mentions);
            filesWidget->setFocusOnInput();
            generalDialog.setFocusPolicyButtons(Qt::TabFocus);

            if (generalDialog.execute())
            {
                files = filesWidget->getFiles();
                currentState().mentions_ = filesWidget->getMentions();
                currentState().description_ = filesWidget->getDescription();
                sendFiles(_source);
            }
            else
            {
                currentState().mentions_ = filesWidget->getMentions();
                setInputText(filesWidget->getDescription());
                saveDraft(SyncDraft::Yes);
            }
            clearFiles();
            setFocusOnInput();
        }
    }

    InputStyleMode InputWidget::currentStyleMode() const
    {
        const auto& contactWpId = Styling::getThemesContainer().getContactWallpaperId(bgWidget_ ? bgWidget_->aimId() : contact_);
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
        activateParentLayout();
        Q_EMIT heightChanged();
    }

    void InputWidget::startPttRecord()
    {
#ifndef STRIP_AV_MEDIA
        for (auto input : getInputWidgetInstances())
        {
            if (input != this)
                input->stopPttRecording();
        }

        panelPtt_->record();
        escCancel_->addChild(panelPtt_, [this]()
        {
            if (!isPttHoldByMouse())
                closePttPanel();
        });
#endif // !STRIP_AV_MEDIA
    }

    void InputWidget::stopPttRecording()
    {
#ifndef STRIP_AV_MEDIA
        if (panelPtt_)
        {
            panelPtt_->stop();
            panelPtt_->enableCircleHover(false);
        }

        if (pttLock_)
            pttLock_->hideForce();
#endif // !STRIP_AV_MEDIA

        if (buttonSubmit_)
            buttonSubmit_->enableCircleHover(false);
    }

    void InputWidget::startPttRecordingLock()
    {
        startPtt(PttMode::Lock);
    }

    void InputWidget::sendPtt()
    {
#ifndef STRIP_AV_MEDIA
        ptt::StatInfo stat;

        if (const auto mode = getPttMode(); mode)
            stat.mode = (*mode == PttMode::HoldByKeyboard || *mode == PttMode::HoldByMouse) ? "tap" : "fix";
        stat.chatType = getStatsChatType(contact_);

        stopPttRecording();
        sendPttImpl(std::move(stat));
#endif // !STRIP_AV_MEDIA
    }

    void InputWidget::sendPttImpl(ptt::StatInfo&& _statInfo)
    {
#ifndef STRIP_AV_MEDIA
        panelPtt_->send(std::move(_statInfo));
#endif // !STRIP_AV_MEDIA
    }

    void InputWidget::removePttRecord()
    {
#ifndef STRIP_AV_MEDIA
        if (panelPtt_)
            panelPtt_->remove();
#endif // !STRIP_AV_MEDIA
    }

    void InputWidget::onPttReady(const QString& _file, std::chrono::seconds _duration, const ptt::StatInfo& _statInfo)
    {
#ifndef STRIP_AV_MEDIA
        const auto& quotes = currentState().getQuotes();

        GetDispatcher()->uploadSharedFile(contact_, _file, quotes, std::make_optional(_duration));

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
            onQuotesCancel();
        }

        switchToPreviousView();
        notifyDialogAboutSend();
#endif // !STRIP_AV_MEDIA
    }

    void InputWidget::onPttRemoved()
    {
        switchToPreviousView();
        disablePttCircleHover();
    }

    void InputWidget::onPttStateChanged(ptt::State2 _state)
    {
#ifndef STRIP_AV_MEDIA
        if (_state == ptt::State2::Stopped)
            disablePttCircleHover();
#endif // !STRIP_AV_MEDIA
    }

    void InputWidget::disablePttCircleHover()
    {
#ifndef STRIP_AV_MEDIA
        if (buttonSubmit_)
            buttonSubmit_->enableCircleHover(false);
        if (panelPtt_)
            panelPtt_->enableCircleHover(false);
#endif // !STRIP_AV_MEDIA
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
        if (isReplying() != _isVisible)
        {
            quotesWidget_->setVisible(_isVisible);

            if (_isVisible)
                escCancel_->addChild(quotesWidget_, [this]() { dropReply(); });
            else
                escCancel_->removeChild(quotesWidget_);

            if (panelMain_)
                panelMain_->setUnderQuote(_isVisible);
#ifndef STRIP_AV_MEDIA
            if (panelPtt_)
                panelPtt_->setUnderQuote(_isVisible);
#endif // !STRIP_AV_MEDIA

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
#ifndef STRIP_AV_MEDIA
            return panelPtt_;
#else
            return nullptr;
#endif // !STRIP_AV_MEDIA

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
        if (historyPage_)
            historyPage_->hideSmartreplies();
    }

    // check for urls in text and load meta to cache
    void InputWidget::processUrls() const
    {
        if (!config::get().is_on(config::features::snippet_in_chat))
            return;

        if (!Ui::get_gui_settings()->get_value<bool>(settings_show_video_and_images, true))
            return;

        Utils::UrlParser p;
        p.process(getInputText().string().trimmed());

        if (p.hasUrl())
        {
            if (!p.isEmail() && !p.isFileSharing())
                SnippetCache::instance()->load(p.formattedUrl());
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

    void InputWidget::saveDraft(SyncDraft _sync, bool _textChanged)
    {
        if (!currentState().draftLoaded_ || isEditing() || contact_.isEmpty() || panelMain_ && panelMain_->isDraftVersionVisible())
            return;

        auto msg = makeMessage(getInputText());
        const auto currentDraft = currentState().draft_;
        currentState().draft_.message_ = msg;

        if (msg.GetSourceText().isEmpty()) // do not save quotes without a message
            msg.Quotes_.clear();

        const auto sync = _sync == SyncDraft::Yes;
        auto changed = isDraftChanged(currentDraft.message_, msg) || _textChanged;
        if (changed || sync && (!currentState().draft_.synced_ && !currentState().draft_.isEmpty()))
        {
            if (changed)
                currentState().draft_.localTimestamp_ = QDateTime::currentMSecsSinceEpoch();

            currentState().draft_.synced_ = sync;
            GetDispatcher()->setDraft(msg, currentState().draft_.localTimestamp_, sync);
        }
    }

    void InputWidget::clearDraft()
    {
        Data::MessageBuddy msg;
        msg.AimId_ = contact_;
        currentState().draft_.message_ = msg;
        currentState().draft_.localTimestamp_ = QDateTime::currentMSecsSinceEpoch();
        GetDispatcher()->setDraft(msg, currentState().draft_.localTimestamp_);
    }

    void InputWidget::applyDraft(const Data::Draft& _draft)
    {
        const auto& draftText = _draft.message_.GetSourceText();

        currentState().mentions_ = _draft.message_.Mentions_;
        currentState().files_ = _draft.message_.Files_;
        currentState().draft_ = _draft;
        if (textEdit_)
        {
            textEdit_->setMentions(currentState().mentions_);
            textEdit_->setFiles(currentState().files_);
        }

        clearQuotes();
        auto quotes = _draft.message_.Quotes_;

        for (auto& q : quotes)
        {
            Data::MessageBuddy msg;
            msg.poll_ = q.poll_;
            msg.sharedContact_ = q.sharedContact_;
            msg.Files_ = q.files_;
            msg.geo_ = q.geo_;
            msg.SetText(q.text_);
            msg.AimId_ = MyInfo()->aimId();

            q.mediaType_ = hist::MessageBuilder::formatRecentsText(msg).mediaType_;
            auto mentions = extractMentions(q.text_.string());
            for (auto& mention : mentions)
            {
                const auto mentionStr = mention.toString();
                if (auto it = _draft.message_.Mentions_.find(mentionStr); it != _draft.message_.Mentions_.end())
                    q.mentions_[mentionStr] = it->second;
            }
        }
        quote(quotes);

        if (textEdit_ && (!draftText.isEmpty() || !textEdit_->isEmpty()))
        {
            textEdit_->setPlaceholderAnimationEnabled(false);
            setInputText(draftText);
            textEdit_->registerTextChange();
            textEdit_->setPlaceholderAnimationEnabled(true);
        }

        if (!currentState().hasQuotes())
            hideQuotes();
    }

    void InputWidget::startDraftTimer()
    {
        if (!Features::isDraftEnabled())
            return;

        if (!draftTimer_)
        {
            draftTimer_ = new QTimer(this);
            draftTimer_->setSingleShot(true);
            connect(draftTimer_, &QTimer::timeout, this, &InputWidget::onDraftTimer);
        }

        draftTimer_->start(Features::draftTimeout());
    }

    void InputWidget::stopDraftTimer()
    {
        if (draftTimer_)
            draftTimer_->stop();
    }

    void InputWidget::requestDraft()
    {
        currentState().draftLoaded_ = false;
        GetDispatcher()->getDraft(contact_);
    }

    Data::MessageBuddy InputWidget::makeMessage(Data::FStringView _text) const
    {
        Data::MessageBuddy buddy;

        if (isEditing())
        {
            buddy = *currentState().edit_->message_;
        }
        else
        {
            buddy.Quotes_.reserve(currentState().quotes_.size());
            for (auto quote : currentState().quotes_)
            {
                if (_text != quote.description_.string() || _text != quote.url_)
                {
                    quote.url_.clear();
                    quote.description_.clear();
                }
                Utils::replaceFilesPlaceholders(quote.text_, quote.files_);
                buddy.Quotes_.push_back(std::move(quote));
            }

            buddy.AimId_ = contact_;
            if (Logic::getContactListModel()->isChannel(contact_))
                buddy.SetChatSender(contact_);
        }

        buddy.Mentions_ = makeMentions(_text);

        if (buddy.GetDescription().isEmpty())
            buddy.SetText(_text.toFString());
        else
            buddy.SetDescription(_text.toFString());

        return buddy;
    }

    Data::MentionMap InputWidget::makeMentions(Data::FStringView _text) const
    {
        std::unordered_set<QStringView, Utils::QStringViewHasher> messageMentions;
        messageMentions.merge(extractMentions(_text.string()));

        const auto& quotes = currentState().getQuotes();
        for (const auto& quote : quotes)
            messageMentions.merge(extractMentions(quote.text_.string()));

        auto mentions = currentState().getMentions(); // copy
        auto it = mentions.begin();
        while (it != mentions.end()) // remove unnecessary mentions
        {
            if (messageMentions.count(it->first) == 0)
                it = mentions.erase(it);
            else
                ++it;
        }

        for (const auto& quote : quotes)
            mentions.insert(quote.mentions_.begin(), quote.mentions_.end()); // add mentions from quotes

        return mentions;
    }

    void InputWidget::transitionInit()
    {
        if (!transitionAnim_)
        {
            transitionAnim_ = new TransitionAnimation(this);
            transitionAnim_->setStartValue(1.0);
            transitionAnim_->setEndValue(0.0);
            transitionAnim_->setDuration(viewTransitDuration().count());
            transitionAnim_->setEasingCurve(QEasingCurve::InOutSine);
            connect(transitionAnim_, &TransitionAnimation::valueChanged, this, [this](const QVariant& value)
            {
                const auto val = value.toDouble();
                if (auto e = transitionAnim_->getEffect())
                    e->setOpacity(val);

                const auto currentHeight = transitionAnim_->getCurrentHeight();
                const auto targetHeight = transitionAnim_->getTargetHeight();
                if (currentHeight != targetHeight)
                    setInputHeight(targetHeight + val * (currentHeight - targetHeight));
            });
        }

        if (!transitionLabel_)
        {
            transitionLabel_ = new QLabel(this);
            transitionLabel_->setAttribute(Qt::WA_TranslucentBackground);
            transitionLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
            transitionLabel_->setGraphicsEffect(new QGraphicsOpacityEffect(transitionLabel_));

            connect(transitionAnim_, &TransitionAnimation::finished, transitionLabel_, &QLabel::hide);
        }
    }

    void InputWidget::transitionCacheCurrent()
    {
        transitionInit();

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

    void InputWidget::transitionAnimate(int _fromHeight, int _targetHeight)
    {
        transitionInit();

        if (auto effect = qobject_cast<QGraphicsOpacityEffect*>(transitionLabel_->graphicsEffect()))
        {
            transitionAnim_->stop();
            transitionAnim_->setCurrentHeight(_fromHeight);
            transitionAnim_->setTargetHeight(_targetHeight);
            transitionAnim_->setEffect(effect);
            transitionAnim_->start();
        }
    }

    void InputWidget::activateParentLayout()
    {
        if (auto wdg = parentWidget(); wdg && wdg->layout())
            wdg->layout()->activate();
    }

    bool InputWidget::showMentionCompleter(const QString& _initialPattern, const QPoint& _pos)
    {
        if (historyPage_)
            historyPage_->positionMentionCompleter(_pos);

        if (threadFeedItem_)
            threadFeedItem_->positionMentionCompleter(_pos);

        if (mentionCompleter_)
        {
            mentionCompleter_->setDialogAimId(contact_);
            const auto cond = _initialPattern.isEmpty()
                ? Logic::MentionModel::SearchCondition::Force
                : Logic::MentionModel::SearchCondition::IfPatternChanged;
            mentionCompleter_->setSearchPattern(_initialPattern, cond);

            QTimer::singleShot(mentionCompleterTimeout(), mentionCompleter_, [completer = mentionCompleter_, _pos]()
            {
                completer->showAnimated(_pos);
            });

            return true;
        }

        return false;
    }

    void InputWidget::hideMentionCompleter()
    {
        if (mentionCompleter_)
            mentionCompleter_->hideAnimated();
    }

    bool InputWidget::isSuggestsEnabled() const
    {
        return (features_ & InputFeature::Suggests) && Features::isSuggestsEnabled();
    }

    bool InputWidget::isPttEnabled() const
    {
        return features_ & InputFeature::Ptt;
    }

    void InputWidget::setHistoryControlPageToEdit()
    {
        if (historyPage_ && textEdit_)
        {
            auto oldWidget = textEdit_->setContentWidget(historyPage_);
            if (oldWidget)
                textEdit_->disconnect(oldWidget);
            connect(textEdit_, &HistoryTextEdit::navigationKeyPressed, historyPage_, &HistoryControlPage::onNavigationKeyPressed);
        }
    }

    void InputWidget::setThreadFeedItemToEdit()
    {
        if (threadFeedItem_ && textEdit_)
        {
            if (auto oldWidget = textEdit_->setContentWidget(threadFeedItem_); oldWidget)
                textEdit_->disconnect(oldWidget);

            connect(textEdit_, &HistoryTextEdit::navigationKeyPressed, threadFeedItem_, &ThreadFeedItem::onNavigationKeyPressed);
        }
    }

    void InputWidget::cancelDraftVersionByEdit()
    {
        if (panelMain_->isDraftVersionVisible())
        {
            panelMain_->hidePopups();
            currentState().queuedDraft_.reset();
        }
    }

    bool InputWidget::canSetFocus() const
    {
        return currentView() != InputView::Disabled;
    }

    bool InputWidget::hasServerSuggests() const
    {
        return !lastRequestedSuggests_.empty();
    }

    bool InputWidget::isServerSuggest(const Utils::FileSharingId& _stickerId) const
    {
        return std::any_of(lastRequestedSuggests_.begin(), lastRequestedSuggests_.end(), [&_stickerId](const auto& s) { return s == _stickerId; });
    }

    void InputWidget::setInitialState()
    {
        updateStyle();

        requestDraft();

        if (textEdit_)
        {
            textEdit_->setPlaceholderAnimationEnabled(false);
            textEdit_->getUndoRedoStack().clear(HistoryUndoStack::ClearType::Full);
        }

        if (Logic::getContactListModel()->isReadonly(contact_))
        {
            currentState().clear();
            setView(InputView::Readonly);
        }
        else if (Logic::getContactListModel()->isDeleted(contact_))
        {
            currentState().clear();
            setView(InputView::Disabled);
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

            forceUpdateView();

            if (textEdit_)
            {
                textEdit_->getUndoRedoStack().push(getInputText());
                textEdit_->getReplacer().setAutoreplaceAvailable(Emoji::ReplaceAvailable::Unavailable);
            }
        }

        if (textEdit_)
            textEdit_->setPlaceholderAnimationEnabled(true);

        if (const auto mainPage = MainPage::instance(); mainPage && !mainPage->isSidebarWithThreadPage())
            setView(InputView::Default, UpdateMode::Force, SetHistoryPolicy::DontAddToHistory);

        activateWindow();
    }

    void InputWidget::setThreadParentChat(const QString& _aimId)
    {
        threadParentChat_ = _aimId;
        chatRoleChanged(threadParentChat_);
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
#ifndef STRIP_AV_MEDIA
        else if (panel == panelPtt_)
            panelPtt_->setFocusOnDelete();
#endif // !STRIP_AV_MEDIA
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
        if (shouldOfferCompleter(CheckOffer::No))
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

    void InputWidget::ignoreListChanged()
    {
        if (currentView() == InputView::Readonly)
        {
            if (!Logic::getIgnoreModel()->contains(contact_))
                setView(InputView::Default, UpdateMode::Force, SetHistoryPolicy::DontAddToHistory);
        }
        else
        {
            if (Logic::getIgnoreModel()->contains(contact_))
                setView(InputView::Readonly, UpdateMode::Force, SetHistoryPolicy::DontAddToHistory);
        }
    }

    void InputWidget::onMultiselectChanged()
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

    void InputWidget::edit(const Data::MessageBuddySptr& _msg, MediaType _mediaType)
    {
        if (_msg->Id_ == -1 && _msg->InternalId_.isEmpty())
            return;

        _msg->SetType(getMediaType(_mediaType));
        currentState().startEdit(_msg);
        currentState().edit_->type_ = _mediaType;

        Data::FString text;
        if (!_msg->GetDescription().isEmpty())
            text = _msg->GetDescription();
        else if (_msg->GetType() == core::message_type::base)
            text = _msg->GetSourceText();

        currentState().edit_->buffer_ = text;
        currentState().edit_->editableText_ = std::move(text);
        currentState().canBeEmpty_ = _msg->GetType() != core::message_type::base || !_msg->Quotes_.empty();
        stopDraftTimer();

        panelMain_->forceSendButton(currentState().canBeEmpty_);

        dropReply();
        setEditView();
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
        static constexpr auto delims = QStringView(u".,:;_-+=!?\"\'#");
        if (lastChar.isSpace() || delims.contains(lastChar))
            return true;

        return false;
    }

    InputWidgetState& InputWidget::currentState()
    {
        return *state_;
    }

    const InputWidgetState& InputWidget::currentState() const
    {
        return *state_;
    }

    InputView InputWidget::currentView() const
    {
        return currentState().view_;
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
                hideMentionCompleter();
                return;
            }

            if (mentionCompleter_)
            {
                auto cursor = textEdit_->textCursor();
                cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, cursor.position() - mentionSignIndex - 1);

                const auto cond = _force
                    ? Logic::MentionModel::SearchCondition::Force
                    : Logic::MentionModel::SearchCondition::IfPatternChanged;
                mentionCompleter_->setSearchPattern(cursor.selectedText(), cond);
                mentionCompleter_->setArrowPosition(tooltipArrowPosition());
            }
        }
    }

    void InputWidget::switchToMainPanel(const StateTransition _transition)
    {
        initSubmitButton();
        if (!panelMain_)
        {
            panelMain_ = new InputPanelMain(this, buttonSubmit_, features_);
            panelMain_->updateStyle(currentStyleMode());

            Testing::setAccessibleName(panelMain_, qsl("AS ChatInput panelMain"));
            stackWidget_->addWidget(panelMain_);

            connect(panelMain_, &InputPanelMain::editCancelClicked, this, &InputWidget::onEditCancel);
            connect(panelMain_, &InputPanelMain::editedMessageClicked, this, &InputWidget::goToEditedMessage);
            connect(panelMain_, &InputPanelMain::emojiButtonClicked, this, &InputWidget::onStickersClicked);
            connect(panelMain_, &InputPanelMain::attachMedia, this, &InputWidget::onAttachMedia);
            connect(panelMain_, &InputPanelMain::panelHeightChanged, this, [this](const int)
            {
                if (getCurrentPanel() == panelMain_)
                    updateInputHeight();
            });
            connect(panelMain_, &InputPanelMain::draftVersionAccepted, this, &InputWidget::onDraftVersionAccepted);
            connect(panelMain_, &InputPanelMain::draftVersionCancelled, this, &InputWidget::onDraftVersionCancelled);

            textEdit_ = panelMain_->getTextEdit();
            im_assert(textEdit_);
            connect(textEdit_->document(), &QTextDocument::contentsChanged, this, &InputWidget::editContentChanged, Qt::QueuedConnection);
            connect(textEdit_, &HistoryTextEdit::clicked, this, [this]() { setActive(true); });
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

            setHistoryControlPageToEdit();
            setThreadFeedItemToEdit();

            escCancel_->addChild(panelMain_);
        }

        panelMain_->setUnderQuote(isReplying());

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
            panelReadonly_ = new InputPanelReadonly(this, contact_);
            panelReadonly_->updateStyle(currentStyleMode());

            Testing::setAccessibleName(panelReadonly_, qsl("AS ChatInput panelReadonly"));
            stackWidget_->addWidget(panelReadonly_);
        }

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
#ifndef STRIP_AV_MEDIA
        initSubmitButton();
        if (!panelPtt_)
        {
            panelPtt_ = new InputPanelPtt(contact_, this, buttonSubmit_);
            panelPtt_->updateStyle(currentStyleMode());

            Testing::setAccessibleName(panelPtt_, qsl("AS ChatInput panelPtt"));
            stackWidget_->addWidget(panelPtt_);

            connect(panelPtt_, &InputPanelPtt::pttReady, this, &InputWidget::onPttReady);
            connect(panelPtt_, &InputPanelPtt::pttRemoved, this, &InputWidget::onPttRemoved);
            connect(panelPtt_, &InputPanelPtt::stateChanged, this, &InputWidget::onPttStateChanged);
        }

        hideSmartreplies();

        panelPtt_->setUnderQuote(isReplying());

        if (_transition == StateTransition::Force)
            updateInputHeight();

        if (setFocusToSubmit_ && buttonSubmit_)
            buttonSubmit_->setFocus(Qt::TabFocusReason);
        setFocusToSubmit_ = false;

        setCurrentWidget(panelPtt_);
#endif // !STRIP_AV_MEDIA
    }

    void InputWidget::switchToMultiselectPanel()
    {
        if (!panelMultiselect_)
        {
            panelMultiselect_ = new InputPanelMultiselect(this, contact_);
            panelMultiselect_->updateStyle(currentStyleMode());

            Testing::setAccessibleName(panelMultiselect_, qsl("AS ChatInput panelMultiselect"));
            stackWidget_->addWidget(panelMultiselect_);
        }

        panelMultiselect_->updateElements();
        setCurrentWidget(panelMultiselect_);
        updateInputHeight();
    }

    void InputWidget::setCurrentWidget(QWidget* _widget)
    {
        if (panelMain_)
            panelMain_->hideAttachPopup();

        im_assert(_widget);
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

            const bool visible =
#ifndef STRIP_AV_MEDIA
                _widget == panelPtt_ ||
#endif // !STRIP_AV_MEDIA
                _widget == panelMain_;

            buttonSubmit_->setVisible(visible);
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
        connect(buttonSubmit_, &SubmitButton::clickedWithButton, this, &InputWidget::onSubmitClicked);
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
            hideMentionCompleter();
            if (isPttEnabled() && isInputEmpty() && !isReplying())
            {
                setFocusToSubmit_ = (_clickType == ClickType::ByKeyboard);
                startPtt(PttMode::Lock);
                sendStat(contact_, core::stats::stats_event_names::chatscr_openptt_action, "primary");
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
#ifndef STRIP_AV_MEDIA
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
            if (panelPtt_->removeIfUnderMouse())
                return;
            if (buttonSubmit_ && buttonSubmit_->containsCursorUnder())
            {
                sendPtt();
                return;
            }
            if (const auto pos = QCursor::pos(); rect().contains(mapFromGlobal(pos)))
                stopPttRecording();
        }
#endif // !STRIP_AV_MEDIA
    }

    void InputWidget::onSubmitLongTapped()
    {
        if (isInputEmpty())
        {
            hideMentionCompleter();

            if (buttonSubmit_)
                buttonSubmit_->setFocus();
            setFocusToSubmit_ = true;
            startPtt(PttMode::HoldByMouse);
        }
    }

    void InputWidget::onSubmitLongPressed()
    {
        if (isInputEmpty())
        {
            hideMentionCompleter();

            startPtt(PttMode::HoldByKeyboard);
        }
    }

    void InputWidget::onSubmitMovePressed()
    {
#ifndef STRIP_AV_MEDIA
        if (panelPtt_ && currentView() == InputView::Ptt)
        {
            if (pttMode_ != PttMode::HoldByMouse)
                return;
            panelPtt_->pressedMouseMove();

            if (!pttLock_)
                pttLock_ = new PttLock(qobject_cast<QWidget*>(parent()));

            const auto g = geometry();
            auto b = g.bottomLeft();
            b.rx() += g.width() / 2 - pttLock_->width() / 2;
            b.ry() -= (g.height() / 2 + pttLock_->height() / 2);
            pttLock_->setBottom(b);

            const bool newCanLocked = panelPtt_->canLock() && !(buttonSubmit_ && buttonSubmit_->containsCursorUnder());

            if (newCanLocked != canLockPtt_)
            {
                if (newCanLocked)
                    pttLock_->showAnimated();
                else
                    pttLock_->hideAnimated();

                canLockPtt_ = newCanLocked;
            }
        }
#endif // !STRIP_AV_MEDIA
    }

    void InputWidget::startPtt(PttMode _mode)
    {
        if (!isPttEnabled())
            return;

        auto startCallBack = [pThis = QPointer(this), _mode](bool _allowed)
        {
            if (!_allowed)
                return;

            if (!pThis)
                return;

#ifndef STRIP_AV_MEDIA
            if (ptt::AudioRecorder2::hasDevice())
            {
                pThis->setView(InputView::Ptt);
                pThis->startPttRecord();
                pThis->setPttMode(_mode);
                pThis->updateSubmitButtonState(UpdateMode::IfChanged);
                pThis->setFocus();
                Q_EMIT pThis->startPttRecording();
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
#else
            Q_UNUSED(_mode)
#endif // !STRIP_AV_MEDIA
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
                if (buttonSubmit_)
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
            im_assert(!"unimpl");
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
#ifndef STRIP_AV_MEDIA
        pttMode_ = _mode;

        const auto isHoldByKeyboard = pttMode_ == PttMode::HoldByKeyboard;
        const auto isHoldMode = isHoldByKeyboard || pttMode_ == PttMode::HoldByMouse;

        if (buttonSubmit_)
            buttonSubmit_->enableCircleHover(isHoldMode);
        if (panelPtt_)
        {
            panelPtt_->setUnderLongPress(isHoldByKeyboard);
            panelPtt_->enableCircleHover(isHoldMode);
        }
#endif // !STRIP_AV_MEDIA
    }

    std::optional<InputWidget::PttMode> InputWidget::getPttMode() const noexcept
    {
        return pttMode_;
    }

    void InputWidget::setView(InputView _view, const UpdateMode _updateMode, const SetHistoryPolicy _historyPolicy)
    {
        hideMentionCompleter();

        const auto curView = currentView();
        if (curView == _view && _updateMode != UpdateMode::Force)
            return;

        if (_historyPolicy == SetHistoryPolicy::AddToHistory && curView != _view)
            addViewToHistory(curView);

        auto currentHeight = getDefaultInputHeight();
        if (auto panel = getCurrentPanel())
            currentHeight = panel->height();

        const auto animatedTransition = curView != _view && (curView == InputView::Ptt || _view == InputView::Ptt);
        if (animatedTransition)
            transitionCacheCurrent();

        if (_view != InputView::Multiselect && isBot())
        {
            if (const auto needButton = needStartButton())
            {
                if (*needButton)
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
        const auto isThread = Logic::getContactListModel()->isThread(contact_);
        const auto role = Logic::getContactListModel()->getYourRole(contact_);
        const auto hasInContactList = Logic::getContactListModel()->hasContact(contact_);
        const auto isRoleKnownAndMember = role == u"member" || role == u"readonly";
        if (isRoleKnownAndMember && !hasInContactList)
            Log::write_network_log(su::concat("contact-list-debug role is known and ", role.toStdString(), " and hasContact() is false\r\n"));

        if (!isThread && Logic::getIgnoreModel()->contains(contact_))
        {
            currentState().clear(InputView::Readonly);
            switchToReadonlyPanel(ReadonlyPanelState::Unblock);
        }
        else if (_view != InputView::Multiselect && !isThread && Utils::isChat(contact_) && !hasInContactList && !isRoleKnownAndMember)
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
                else if (Logic::getContactListModel()->isDeleted(contact_))
                {
                    currentState().clear(InputView::Disabled);
                    switchToDisabledPanel(DisabledPanelState::Deleted);
                }
                else if (Logic::getContactListModel()->isThread(contact_))
                {
                    currentState().clear(InputView::Disabled);
                    switchToDisabledPanel(DisabledPanelState::Empty);
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
                if (isEditing())
                {
                    auto& editData = currentState().edit_;
                    auto& editedMessage = editData->message_;
                    panelMain_->showEdit(editedMessage, editData->type_);

                    textEdit_->limitCharacters(Utils::getInputMaximumChars());
                    textEdit_->setMentions(editedMessage->Mentions_);
                    textEdit_->setFiles(editedMessage->Files_);

                    auto textToEdit = editData->buffer_;
                    if (editData->type_ == MediaType::mediaTypeFsPhoto || editData->type_ == MediaType::mediaTypeFsVideo || editData->type_ == MediaType::mediaTypeFsGif)
                    {
                        auto textWithPlaceholder = editedMessage->GetSourceText();
                        auto placeholderEnd = textWithPlaceholder.indexOf(u']');
                        placeholderEnd = placeholderEnd == -1 ? 0 : placeholderEnd; // in case we remove placeholders (IMDESKTOP-12050)
                        const auto textFromMessage = Data::FStringView(textWithPlaceholder, placeholderEnd + 1);
                        auto spaceIDX = textFromMessage.indexOf(u'\n');
                        if (spaceIDX == -1)
                            spaceIDX = textFromMessage.indexOf(u' ');

                        if (spaceIDX == -1)
                        {
                            textToEdit = editedMessage->GetDescription();
                        }
                        else
                        {
                            editedMessage->SetUrl(Utils::replaceFilesPlaceholders(
                                textWithPlaceholder.string().left(placeholderEnd + 1),
                                editedMessage->Files_));
                            editedMessage->SetText(textFromMessage.left(spaceIDX + 1));
                            textToEdit = textFromMessage.mid(spaceIDX + 1).toFString();
                            editData->editableText_ = textToEdit;
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

                startDraftTimer();
                applyDraft(currentState().draft_);
                if (currentState().queuedDraft_)
                    onDraft(contact_, *currentState().queuedDraft_);
            }

            updateSubmitButtonState(UpdateMode::IfChanged);
            setFocusOnInput();
        }

        if (!currentState().hasQuotes() || (_view != InputView::Default && _view != InputView::Ptt))
            hideQuotes();

        if (animatedTransition)
        {
            auto targetHeight = currentHeight;
            if (auto panel = getCurrentPanel())
                targetHeight = panel->height();

            transitionAnimate(currentHeight, targetHeight);
        }

        if (_view != InputView::Ptt)
            setPttMode({});

        updateGeometry();
        activateParentLayout();

        Q_EMIT viewChanged(_view, QPrivateSignal());
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

        if (currentView() == InputView::Edit)
            buttonSubmit_->setState(SubmitButton::State::Edit, transit);
        else if (currentView() == InputView::Ptt)
            buttonSubmit_->setState(SubmitButton::State::Send, transit);
        else if (isPttEnabled() && isInputEmpty() && !isReplying())
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

    void InputWidget::notifyDialogAboutSend()
    {
        Q_EMIT Utils::InterConnector::instance().messageSent(contact_);
    }

    void InputWidget::sendFiles(const FileSource _source)
    {
        const auto& files = currentState().filesToSend_;

        if (files.empty())
            return;

        bool mayQuotesSent = false;
        const auto& quotes = currentState().getQuotes();
        const auto amount = files.size();

        const bool descriptionInside = (amount == 1);

        if (!currentState().description_.isEmpty() && !descriptionInside)
        {
            GetDispatcher()->sendMessageToContact(contact_, currentState().description_, quotes, currentState().mentions_);
            notifyDialogAboutSend();
            mayQuotesSent = true;
        }

        auto sendQuotesOnce = !mayQuotesSent;
        bool alreadySentWebp = false;
        for (const auto& f : files)
        {
            if (f.getSize() == 0)
                continue;

            const auto& quotesToSend = sendQuotesOnce ? quotes : Data::QuotesVec();
            const auto& fileUploadDescription = descriptionInside ? currentState().description_ : Data::FString();
            const auto& mentionsToSend = descriptionInside ? currentState().mentions_ : Data::MentionMap();
            const auto isWebpScreenshot = Features::isWebpScreenshotEnabled() && f.canConvertToWebp();

            if (f.isGifFile())
            {
                QFile file(f.getFileInfo().absoluteFilePath());
                if (file.open(QIODevice::ReadOnly))
                    GetDispatcher()->uploadSharedFile(contact_, file.readAll(), u".gif", quotesToSend, fileUploadDescription, mentionsToSend);
            }
            else if (f.isFile() && (currentState().sendAsFile_ || alreadySentWebp || !isWebpScreenshot))
            {
                GetDispatcher()->uploadSharedFile(contact_, f.getFileInfo().absoluteFilePath(), quotesToSend, fileUploadDescription, mentionsToSend, currentState().sendAsFile_);
            }
            else
            {
                if (isWebpScreenshot)
                    alreadySentWebp = true;
                Async::runAsync([f, contact_ = contact_, quotesToSend, fileUploadDescription, mentionsToSend, isWebpScreenshot]() mutable
                {
                    auto array = FileToSend::loadImageData(f, isWebpScreenshot ? FileToSend::Format::webp : FileToSend::Format::png);
                    if (array.isEmpty())
                        return;

                    Async::runInMain([array = std::move(array), contact_ = std::move(contact_), quotesToSend = std::move(quotesToSend), descToSend = std::move(fileUploadDescription), mentionsToSend = std::move(mentionsToSend), isWebpScreenshot]()
                    {
                        GetDispatcher()->uploadSharedFile(contact_, array, isWebpScreenshot ? u".webp" : u".png", quotesToSend, descToSend, mentionsToSend);
                    });
                });
            }

            notifyDialogAboutSend();
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
            onQuotesCancel();
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

        if (isSuggestsEnabled())
        {
            const auto text = getInputText();
            if (!text.contains(symbolAt()) && !text.contains(QChar::LineFeed))
            {
                if (!suggestTimer_)
                {
                    suggestTimer_ = new QTimer(this);
                    suggestTimer_->setSingleShot(true);
                    connect(suggestTimer_, &QTimer::timeout, this, &InputWidget::onSuggestTimeout);
                }

                const auto isShowingSmileMenu = getWidgetIfNot<Smiles::SmilesMenu>(historyPage_, &Smiles::SmilesMenu::isHidden) != nullptr;
                qsizetype emojiPos = 0;
                auto suggestText = text.string().toLower();
                const auto isEmoji = TextRendering::isEmoji(suggestText, emojiPos) && emojiPos == suggestText.size();

                if (!isEmoji || !isShowingSmileMenu)
                    suggestPos_ = QPoint();
                else
                    suggestPos_ = emojiPos_;

                const auto time = suggestPos_.isNull() ? Features::getSuggestTimerInterval() : singleEmojiSuggestDuration();
                suggestTimer_->start(time);
            }
        }

        Q_EMIT hideSuggets();

        processUrls();

        if (isInputEmpty())
            textEdit_->getReplacer().setAutoreplaceAvailable();

        const auto cursor = textEdit_->textCursor();
        const auto block = cursor.block();
        const auto bf = Logic::TextBlockFormat(block.blockFormat());
        if (core::data::format_type::ordered_list != bf.type() || !block.text().isEmpty())
            textEdit_->registerTextChange();

        const bool isTextChanged = getInputText() != currentState().draft_.message_.getFormattedText();
        const auto syncDraft = textEdit_ && textEdit_->isEmpty() ? SyncDraft::Yes : SyncDraft::No;
        startDraftTimer();
        saveDraft(syncDraft, isTextChanged);
        cancelDraftVersionByEdit();

        Q_EMIT inputTyped();
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
        currentState().sendAsFile_ = false;
    }

    void InputWidget::clearMentions()
    {
        currentState().mentions_.clear();
    }

    void InputWidget::clearEdit()
    {
        currentState().edit_.reset();
        currentState().canBeEmpty_ = false;
    }

    void InputWidget::clearInputText()
    {
        setInputText({});
    }

    void InputWidget::loadInputText()
    {
        im_assert(!contact_.isEmpty());
        if (contact_.isEmpty())
            return;

        setInputText(state_->text_.text_, state_->text_.cursorPos_);
    }

    void InputWidget::setInputText(const Data::FString& _text, std::optional<int> _cursorPos)
    {
        if (!textEdit_)
            return;

        {
            QSignalBlocker sb(textEdit_);
            textEdit_->setFormattedText(_text);
            if (_cursorPos && *_cursorPos < _text.size())
            {
                auto cursor = textEdit_->textCursor();
                cursor.setPosition(std::clamp(*_cursorPos, 0, textEdit_->toPlainText().size()));
                textEdit_->setTextCursor(cursor);
            }
        }

        editContentChanged();

        updateMentionCompleterState(true);
        cursorPositionChanged();
    }

    Data::FString InputWidget::getInputText() const
    {
        if (!textEdit_)
            return {};

        if (Features::isFormattingInInputEnabled())
            return textEdit_->getText();
        else
            return textEdit_->getPlainText();
    }

    int InputWidget::getQuotesCount() const
    {
        return currentState().quotes_.size();
    }

    bool InputWidget::isInputEmpty() const
    {
        return currentView() == InputView::Default && textEdit_ && textEdit_->isEmptyOrHasOnlyWhitespaces();
    }

    bool InputWidget::isEditing() const
    {
        return currentState().isEditing();
    }

    void InputWidget::resizeEvent(QResizeEvent*)
    {
        if (transitionLabel_ && transitionLabel_->isVisible())
            transitionLabel_->move(0, height() - transitionLabel_->height());

        updateBackgroundGeometry();
        updateSubmitButtonPos();

        activateParentLayout();

        if (mentionCompleter_ && mentionCompleter_->isVisible())
            mentionCompleter_->setArrowPosition(tooltipArrowPosition());
    }

    void InputWidget::showEvent(QShowEvent *)
    {
        updateBackgroundGeometry();
    }

    void InputWidget::childEvent(QChildEvent* _event)
    {
        auto listen = [this](QWidget* _w)
        {
            if (auto clickable = qobject_cast<ClickableWidget*>(_w))
            {
                connect(clickable, &ClickableWidget::clicked, this, [this]()
                {
                    setActive(true);
                    setFocus();

                    if (const auto input = qobject_cast<InputWidget*>(this); input && input->isRecordingPtt())
                        Utils::InterConnector::instance().getMainWindow()->setLastPttInput(input);
                }, Qt::UniqueConnection);
            }
            if (auto btn = qobject_cast<CustomButton*>(_w))
            {
                connect(btn, &CustomButton::clicked, this, [this]()
                {
                    setActive(true);
                    setFocus();
                }, Qt::UniqueConnection);
                connect(btn, &CustomButton::clickedWithButtons, this, [this]()
                {
                    setActive(true);
                    setFocus();
                }, Qt::UniqueConnection);
            }
            installEventFilter(_w);
        };

        if (_event->type() == QEvent::ChildAdded)
        {
            if (auto child = qobject_cast<QWidget*>(_event->child()))
            {
                QTimer::singleShot(0, [child = QPointer(child), listen]()
                {
                    if (child)
                    {
                        listen(child);

                        const auto grandChildren = child->findChildren<QWidget*>();
                        for (auto c : grandChildren)
                            listen(c);
                    }
                });
            }
        }
    }

    bool InputWidget::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_event->type() == QEvent::ChildAdded)
            childEvent(static_cast<QChildEvent*>(_event));
        else if (_event->type() == QEvent::KeyPress || _event->type() == QEvent::MouseButtonPress)
        {
            const auto mouseEvent = dynamic_cast<QMouseEvent*>(_event);
            if (mouseEvent && geometry().contains(mouseEvent->pos()))
            {
                setActive(true);
                setFocus();
            }
        }

        return QWidget::eventFilter(_obj, _event);
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

    QPoint InputWidget::suggestPosition() const
    {
        auto pos = suggestPos_;
        if (pos.isNull())
        {
            const auto addPosX = [](int _width) 
            {
                return _width / 2 - Tooltip::getDefaultArrowOffset();
            };
            pos = textEdit_->mapToGlobal(textEdit_->pos());
            pos.setY(textEdit_->mapToGlobal(textEdit_->geometry().topLeft()).y() + bottomOffsetStickersSuggest());
            if (pos.y() % 2 != 0)
                pos.ry() += 1;

            if (textEdit_->document()->lineCount() == 1)
                pos.rx() += addPosX(textEdit_->document()->idealWidth());
            else
                pos.rx() = addPosX(width());
        }
        return pos;
    }

    void InputWidget::sendStatsIfNeeded() const
    {
        if (contact_ == Utils::getStickerBotAimId())
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_discover_sendbot_message);

        if (Logic::getRecentsModel()->isSuspicious(contact_))
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chat_unknown_message);
    }

    std::optional<bool> InputWidget::needStartButton() const
    {
        if (historyPage_)
            return historyPage_->needStartButton() || !historyPage_->hasMessages();

        return {};
    }

    bool InputWidget::shouldOfferCompleter(const CheckOffer _check) const
    {
        const auto completerVisible = mentionCompleter_ && mentionCompleter_->completerVisible();

        return
            (currentState().mentionSignIndex_ == -1 || completerVisible) &&
            textEdit_ &&
            textEdit_->isVisible() &&
            textEdit_->isEnabled() &&
            !textEdit_->isCursorInCode() &&
            (_check == CheckOffer::No || shouldOfferMentions());
    }

    void InputWidget::updateStyle()
    {
        const auto mode = currentStyleMode();

        if (bgWidget_)
            bgWidget_->setOverlayColor(mode == InputStyleMode::CustomBg ? Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE, getBlurAlpha()) : QColor());

        StyledInputElement* elems[] = {
            quotesWidget_,
            panelMain_,
            panelReadonly_,
#ifndef STRIP_AV_MEDIA
            panelPtt_,
#endif // !STRIP_AV_MEDIA
            panelDisabled_,
            buttonSubmit_,
            panelMultiselect_
        };

        for (auto e: elems)
            if (e)
                e->updateStyle(mode);
    }

    void InputWidget::onAttachMedia(AttachMediaType _mediaType)
    {
        switch (_mediaType)
        {
        case AttachMediaType::photoVideo:
            onAttachPhotoVideo();
            break;
        case AttachMediaType::file:
            onAttachFile();
            break;
        case AttachMediaType::camera:
            onAttachCamera();
            break;
        case AttachMediaType::contact:
            onAttachContact();
            break;
        case AttachMediaType::poll:
            onAttachPoll();
            break;
        case AttachMediaType::task:
            onAttachTask();
            break;
        case AttachMediaType::ptt:
            onAttachPtt();
            break;
        case AttachMediaType::geo:
            break;
        case AttachMediaType::callLink:
            onAttachCallByLink();
            break;
        case AttachMediaType::webinar:
            onAttachWebinar();
            break;

        default:
            break;
        }

        sendAttachStat(contact_, _mediaType);
    }

    void InputWidget::updateBackgroundGeometry()
    {
        if (bgWidget_)
        {
            bgWidget_->setGeometry(rect());
            bgWidget_->update();
        }
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
        {
            SelectContactsWidgetOptions opt;
            opt.selectForThread_ = Logic::getContactListModel()->isThread(contact_);
            selectContactsWidget_ = new SelectContactsWidget(
                nullptr,
                Logic::MembersWidgetRegim::SHARE_CONTACT,
                QT_TRANSLATE_NOOP("profilesharing", "Share contact"),
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                Ui::MainPage::instance(),
                opt);
        }

        if (selectContactsWidget_->show() == QDialog::Accepted)
        {
            const auto selectedContacts = Logic::getContactListModel()->getCheckedContacts();
            if (!selectedContacts.empty())
            {
                const auto& aimId = selectedContacts.front();

                const auto wid = new TransitProfileSharing(Utils::InterConnector::instance().getMainWindow(), aimId, contact_);

                connect(wid, &TransitProfileSharing::accepted, this, [this, aimId, wid](const QString& _nick, const bool _sharePhone)
                {
                    bool sendStat = true;
                    auto scopeGuard = qScopeGuard([this, wid, &sendStat]()
                    {
                        clearContactModel(selectContactsWidget_);
                        notifyDialogAboutSend();
                        sendShareStat(contact_, sendStat);
                        wid->deleteLater();
                    });

                    QString link = u"https://" % Utils::getDomainUrl() % u'/';

                    const auto& quotes = currentState().getQuotes();

                    if (Features::isNicksEnabled() && !_nick.isEmpty() && !_sharePhone)
                    {
                        link += _nick;
                    }
                    else
                    {
                        if (const auto& phone = wid->getPhone(); _sharePhone && !phone.isEmpty())
                        {
                            Ui::sharePhone(Logic::GetFriendlyContainer()->getFriendly(aimId), phone, { contact_ }, aimId, quotes);
                            if (!quotes.isEmpty())
                            {
                                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, { { "Quotes_MessagesCount", std::to_string(quotes.size()) } });
                                onQuotesCancel();
                            }
                            return;
                        }
                        else if (Utils::isUin(aimId))
                        {
                            link += aimId;
                        }
                        else
                        {
                            Utils::UrlParser p;
                            p.process(aimId);
                            if (p.hasUrl() && p.isEmail())
                            {
                                link += aimId;
                            }
                            else
                            {
                                sendStat = false;
                                return;
                            }
                        }
                    }

                    Data::Quote q;
                    q.text_ = link;
                    q.type_ = Data::Quote::Type::link;

                    Ui::shareContact({ std::move(q) }, contact_, quotes);
                    if (!quotes.isEmpty())
                    {
                        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::quotes_messagescount, { { "Quotes_MessagesCount", std::to_string(quotes.size()) } });
                        onQuotesCancel();
                    }
                    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sharingscr_choicecontact_action, { { "count", Utils::averageCount(1) } });
                });

                connect(wid, &TransitProfileSharing::declined, this, [this, wid]()
                {
                    wid->deleteLater();
                    sendShareStat(contact_, false);
                    onAttachContact();
                });

                connect(wid, &TransitProfileSharing::openChat, this, [this, aimId]()
                {
                    clearContactModel(selectContactsWidget_);
                    Q_EMIT Utils::InterConnector::instance().openDialogOrProfileById(aimId);

                    if (auto mp = Utils::InterConnector::instance().getMessengerPage(); mp && mp->getFrameCount() != FrameCountMode::_1)
                        mp->showSidebar(aimId);

                    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sharecontactscr_contact_action,
                        { {"chat_type", Ui::getStatsChatType(contact_) }, { "type", std::string("internal") }, { "status", std::string("not_sent") } });
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

    void InputWidget::createTaskWidget(const Data::FString& _text, const Data::QuotesVec& _quotes, const Data::MentionMap& _mentions, const QString& _assignee, const bool _isThreadFeedMessage)
    {
        auto taskWidget = new Ui::TaskEditWidget(contact_, _assignee);

        GeneralDialog::Options opt;
        opt.ignoreKeyPressEvents_ = true;
        opt.threadBadge_ = Logic::getContactListModel()->isThread(contact_);
        GeneralDialog gd(taskWidget, Utils::InterConnector::instance().getMainWindow(), opt);
        taskWidget->setFocusOnTaskTitleInput();
        taskWidget->setInputData(_text, _quotes, _mentions);
        gd.addLabel(taskWidget->getHeader(), Qt::AlignCenter);

        clearInputText();
        if (!_text.isEmpty())
            saveDraft(SyncDraft::Yes);

        if (gd.execute())
        {
            if (_isThreadFeedMessage)
            {
                if (auto page = Utils::InterConnector::instance().getPage(ServiceContacts::getThreadsName()))
                    page->scrollToInput(contact_);
            }
            else
            {
                if (auto page = Utils::InterConnector::instance().getHistoryPage(contact_))
                    page->scrollToBottom();
            }

            hideQuotes();
            clear();
            notifyDialogAboutSend();
        }
        else
        {
            setInputText(taskWidget->getInputText(), taskWidget->getCursorPos());
            currentState().mentions_ = _mentions;
            saveDraft(SyncDraft::Yes);
        }
    }

    void InputWidget::onAttachTask()
    {
        hideSmartreplies();
        Q_EMIT hideSuggets();

        auto text = getInputText();
        Utils::convertMentions(text, currentState().mentions_);
        createTaskWidget(text, currentState().quotes_, currentState().mentions_, contact_, false);
    }

    void InputWidget::onAttachPtt()
    {
        Q_EMIT hideSuggets();
        startPtt(PttMode::Lock);
        sendStat(contact_, core::stats::stats_event_names::chatscr_openptt_action, "plus");
    }

    void InputWidget::onAttachPoll()
    {
        hideSmartreplies();
        Q_EMIT hideSuggets();

        sendStat(contact_, core::stats::stats_event_names::polls_open, "plus");

        const auto inputText = getInputText();
        const auto mentions = currentState().mentions_;
        clearInputText();

        auto pollWidget = new Ui::PollWidget(contact_);
        GeneralDialog::Options opt;
        opt.ignoreKeyPressEvents_ = true;
        GeneralDialog generalDialog(pollWidget, Utils::InterConnector::instance().getMainWindow(), opt);

        pollWidget->setFocusOnQuestion();
        pollWidget->setInputData(inputText, currentState().quotes_, currentState().mentions_);

        clearInputText();

        if (generalDialog.execute())
        {
            hideQuotes();
            clear();
            notifyDialogAboutSend();
        }
        else
        {
            setInputText(pollWidget->getInputText(), pollWidget->getCursorPos());
            currentState().mentions_ = mentions;
            saveDraft(SyncDraft::Yes);
        }
    }

    void InputWidget::onAttachCallByLink()
    {
        hideSmartreplies();
        createConference(ConferenceType::Call);
    }

    void InputWidget::onAttachWebinar()
    {
        hideSmartreplies();
        createConference(ConferenceType::Webinar);
    }

    void InputWidget::updateBackground()
    {
        if (bgWidget_ && isVisibleTo(parentWidget()))
            bgWidget_->forceUpdate();
    }

    void InputWidget::clear()
    {
        textEdit_->clearCache();
        clearQuotes();
        clearFiles();
        clearMentions();
        clearEdit();
        textChanged();
    }

    void InputWidget::setEditView()
    {
        if (!isEditing())
            return;

        hideSmartreplies();

        setView(InputView::Edit, UpdateMode::Force);
    }

    void InputWidget::onEditCancel()
    {
        if (historyPage_)
            historyPage_->editBlockCanceled();

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
        saveDraft();
        startDraftTimer();

        Q_EMIT quotesCountChanged(currentState().quotes_.size(), QPrivateSignal());
    }

    void InputWidget::chatRoleChanged(const QString& _contact)
    {
        const auto chatId = threadParentChat_.isEmpty() ? contact_ : threadParentChat_;
        if (chatId != _contact)
            return;

        auto clear = [this]()
        {
            if (textEdit_)
                textEdit_->getUndoRedoStack().clear(HistoryUndoStack::ClearType::Full);
            currentState().clear();
        };

        const auto areYouForbiddenToWrite = threadParentChat_.isEmpty()
            ? Logic::getContactListModel()->isReadonly(chatId)
            : !Logic::getContactListModel()->areYouAllowedToWriteInThreads(chatId);

        if (areYouForbiddenToWrite)
        {
            clear();
            setView(InputView::Readonly, UpdateMode::Force);
        }
        else if (currentState().isDisabled())
        {
            clear();
            setView(InputView::Default, UpdateMode::Force, SetHistoryPolicy::DontAddToHistory);
        }
    }

    void InputWidget::goToEditedMessage()
    {
        if (isEditing())
        {
            if (const auto msgId = currentState().edit_->message_->Id_; msgId != -1)
                Utils::InterConnector::instance().openDialog(currentState().edit_->message_->AimId_, msgId);
        }
    }

    void InputWidget::onRequestedStickerSuggests(const QString& _contact, const QVector<Utils::FileSharingId>& _suggests)
    {
        if (!isSuggestsEnabled() || _contact != contact_ || !textEdit_->hasFocus())
            return;

        if (_suggests.empty())
        {
            suggestRequested_ = false;
            return;
        }

        clearLastSuggests();

        if (const auto text = getInputText().string().trimmed().toLower(); !text.isEmpty())
        {
            const auto suggestType = TextRendering::isEmoji(text) ? Stickers::SuggestType::suggestEmoji : Stickers::SuggestType::suggestWord;

            for (const auto& stickerId : _suggests)
            {
                if (!isServerSuggest(stickerId))
                    lastRequestedSuggests_.push_back(stickerId);
                Ui::Stickers::addStickers({ stickerId }, text, suggestType);
            }

            suggestRequested_ = true;
            if (!suggestTimer_->isActive())
                onSuggestTimeout();
            if (!lastRequestedSuggests_.empty())
                GetDispatcher()->post_im_stats_to_core(core::stats::im_stat_event_names::text_sticker_suggested);
        }
    }

    void InputWidget::onQuotesHeightChanged()
    {
        if (historyPage_)
            historyPage_->notifyQuotesResize();
    }

    void InputWidget::forceUpdateView()
    {
        setView(currentView(), UpdateMode::Force, SetHistoryPolicy::DontAddToHistory);
    }

    void InputWidget::onDraft(const QString& _contact, const Data::Draft& _draft)
    {
        if (contact_ != _contact)
            return;

        const auto loaded = currentState().draftLoaded_;
        const auto openedDialog = (historyPage_ && historyPage_->isPageOpen()) || (threadFeedItem_ && threadFeedItem_->isInputActive());
        const auto draftIsEmpty = _draft.message_.GetSourceText().isEmpty();

        if (_draft.localTimestamp_ > currentState().draft_.localTimestamp_ || !currentState().draftLoaded_)
        {
            if (isEditing())
            {
                if (currentState().draftLoaded_ && openedDialog)
                    currentState().queuedDraft_ = _draft;
                else
                    currentState().draft_ = _draft;
            }
            else if (!loaded || !panelMain_ || !openedDialog || draftIsEmpty)
            {
                applyDraft(_draft);
                currentState().draftLoaded_ = true;

                if(panelMain_ && draftIsEmpty)
                    panelMain_->cancelDraft();
            }
            else if (!draftIsEmpty)
            {
                hideQuotes();
                currentState().draft_.synced_ = false;
                currentState().queuedDraft_ = _draft;
                panelMain_->showDraftVersion(_draft);
            }
            else if (panelMain_->isDraftVersionVisible())
            {
                panelMain_->hidePopups();
                applyDraft(currentState().draft_);
                currentState().queuedDraft_.reset();
            }
        }
    }

    void InputWidget::onDraftTimer()
    {
        textEdit_->clearCache();
        saveDraft(SyncDraft::Yes);
    }

    void InputWidget::onDraftVersionAccepted()
    {
        if (currentState().queuedDraft_)
        {
            if (!isEditing() && !currentState().queuedDraft_->isEmpty())
                applyDraft(*currentState().queuedDraft_);
            else
                currentState().draft_ = *currentState().queuedDraft_;

            currentState().queuedDraft_.reset();
        }
    }

    void InputWidget::onFilesWidgetOpened(const QString& _contact)
    {
        if (_contact != contact_)
            return;

        clearInputText();
        textEdit_->clearCache();
        saveDraft(SyncDraft::Yes);
    }

    void InputWidget::onDraftVersionCancelled()
    {
        panelMain_->hidePopups();
        currentState().draft_.localTimestamp_ = QDateTime::currentMSecsSinceEpoch();
        GetDispatcher()->setDraft(currentState().draft_.message_, currentState().draft_.localTimestamp_, true);
        applyDraft(currentState().draft_);
        currentState().queuedDraft_.reset();
    }

    void InputWidget::createTask(const Data::FString& _text, const Data::MentionMap& _mentions, const QString& _assignee, const bool _isThreadFeedMessage)
    {
        auto text = getInputText();
        text += _text;

        Data::MentionMap mentions = currentState().mentions_;
        mentions.insert(_mentions.begin(), _mentions.end());
        Utils::convertMentions(text, mentions);

        createTaskWidget(text, currentState().quotes_, mentions, _assignee, _isThreadFeedMessage);
    }

    void InputWidget::requestMoreStickerSuggests()
    {
        if (!suggestRequested_)
        {
            const auto inputText = getInputText().string();
            const auto words = inputText.splitRef(QChar::Space, Qt::SkipEmptyParts);
            const auto suggestText = QStringView(inputText).trimmed();

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
        return textEdit_ && !textEdit_->selection().isEmpty();
    }

    void InputWidget::setHistoryControlPage(HistoryControlPage* _page)
    {
        historyPage_ = _page;
        setHistoryControlPageToEdit();
    }

    void InputWidget::setThreadFeedItem(ThreadFeedItem* _thread)
    {
        threadFeedItem_ = _thread;
        setThreadFeedItemToEdit();
    }

    void InputWidget::setMentionCompleter(MentionCompleter* _completer)
    {
        if (auto oldCompleter = std::exchange(mentionCompleter_, _completer))
            disconnect(oldCompleter);

        if (mentionCompleter_)
        {
            connect(mentionCompleter_, &MentionCompleter::contactSelected, this, &InputWidget::insertMention);
            connect(mentionCompleter_, &MentionCompleter::results, this, &InputWidget::mentionCompleterResults);
        }
    }

    void InputWidget::setParentForPopup(QWidget* _parent)
    {
        if (panelMain_)
            panelMain_->setParentForPopup(_parent);
    }

    void InputWidget::onPageLeave()
    {
        if(textEdit_)
          textEdit_->clearCache();

        saveDraft(SyncDraft::Yes);
        stopDraftTimer();
        onDraftVersionAccepted();
    }

    void InputWidget::setActive(bool _active)
    {
        if (_active)
        {
            setGraphicsEffect({});

            for (auto input : getInputWidgetInstances())
            {
                if (input != this)
                    input->setActive(false);
            }
            Q_EMIT editFocusIn();
        }
        else if (currentView() != InputView::Disabled)
        {
            if (auto effect = qobject_cast<QGraphicsOpacityEffect*>(graphicsEffect()); !effect)
            {
                effect = new QGraphicsOpacityEffect(this);
                effect->setOpacity(0.5);
                setGraphicsEffect(effect);
                Q_EMIT editFocusOut(Qt::OtherFocusReason);
            }
        }
        active_ = _active;
    }

    bool InputWidget::isTextEditInFocus() const
    {
        if (textEdit_)
            return textEdit_->hasFocus();

        return false;
    }
}
