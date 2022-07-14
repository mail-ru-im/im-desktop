#pragma once

#include "controls/TextUnit.h"
#include "types/message.h"

#include "InputWidgetUtils.h"
#include "FileToSend.h"
#include "InputWidgetState.h"

#include "main_window/EscapeCancellable.h"

namespace Emoji
{
    class EmojiCode;
}

namespace Data
{
    class SmartreplySuggest;
}

namespace ptt
{
    class AudioRecorder;
    class AudioRecorder2;
    struct StatInfo;
    enum class State2;
}

namespace Utils
{
    class CallLinkCreator;
    struct FileSharingId;
}

namespace Ui
{
    class HistoryControlPage;
    class ThreadFeedItem;
    class MentionCompleter;
    class SubmitButton;

    class HistoryTextEdit;

    class QuotesWidget;

    class InputPanelMain;
    class InputPanelDisabled;
    class InputPanelReadonly;
    class InputPanelPtt;
    class InputPanelMultiselect;

    class InputBgWidget;
    class BackgroundWidget;

    class PttLock;
    class SelectContactsWidget;

    enum class ReadonlyPanelState;
    enum class DisabledPanelState;
    enum class ClickType;
    enum class ConferenceType;
    enum class AttachMediaType;
    enum class ClosePage;

    enum class FileSource
    {
        Clip,
        CopyPaste
    };

    class InputWidget : public QWidget, public IEscapeCancellable
    {
        Q_OBJECT

    Q_SIGNALS:
        void mentionSignPressed(const QString& _dialogAimId);
        void smilesMenuSignal(const bool _fromKeyboard, QPrivateSignal) const;
        void sendMessage(const QString&);
        void editFocusIn();
        void editFocusOut(Qt::FocusReason _reason);
        void inputTyped();
        void needSuggets(const QString _forText, const QPoint _pos, QPrivateSignal) const;
        void hideSuggets();
        void viewChanged(InputView _view, QPrivateSignal) const;

        void quotesCountChanged(int _count, QPrivateSignal) const;
        void heightChanged();
        void startPttRecording();

    public:
        void quote(const Data::QuotesVec& _quotes);
        void insertEmoji(const Emoji::EmojiCode& _code, const QPoint _pt);
        void sendSticker(const Utils::FileSharingId& _stickerId);
        void onSmilesVisibilityChanged(const bool _isVisible);
        void insertMention(const QString& _aimId, const QString& _friendly);
        void mentionCompleterResults(int _count);
        void messageIdsFetched(const QString& _aimId, const Data::MessageBuddies&);

        void onHistoryReady(const QString& _contact);
        void onHistoryCleared(const QString& _contact);
        void onHistoryInsertedMessages(const QString& _contact);

        enum class CursorPolicy
        {
            Keep,
            Set,
        };
        void mentionOffer(const int _position, const CursorPolicy _policy);
        void requestMoreStickerSuggests();

    public Q_SLOTS:
        void clearInputText();
        void onMultiselectChanged();
        void onDraftVersionCancelled();
        void createTask(const Data::FString& _text, const Data::MentionMap& _mentions, const QString& _assignee, const bool _isThreadFeedMessage);

    private:
        enum class InstantEdit
        {
            No,
            Yes
        };

        void send();

    private Q_SLOTS:
        void textChanged();
        void editContentChanged();
        void selectionChanged();
        void cursorPositionChanged();
        void inputPositionChanged();
        void enterPressed();
        void clearQuotes();
        void clearFiles();
        void clearEdit();
        void clearMentions();
        void clear();

        void onQuotesCancel();
        void onEditCancel();
        void onStickersClicked(const bool _fromKeyboard);

        void statsMessageEnter();
        void statsMessageSend();

        void chatRoleChanged(const QString& _contact);

        void goToEditedMessage();

        void onSuggestTimeout();

        void onNickInserted(const int _atSignPos, const QString& _nick);
        void insertFiles();
        void onContactChanged(const QString& _contact);
        void ignoreListChanged();
        void onRecvPermitDeny();

        void onRequestedStickerSuggests(const QString& _contact, const QVector<Utils::FileSharingId>& _suggests);
        void onQuotesHeightChanged();

        void onDraft(const QString& _contact, const Data::Draft& _draft);
        void onDraftTimer();
        void onDraftVersionAccepted();

        void onFilesWidgetOpened(const QString& _contact);

        void forceUpdateView();

    public:
        InputWidget(const QString& _contact, QWidget* _parent, InputFeatures _features = defaultInputFeatures(), BackgroundWidget* _bg = nullptr);
        ~InputWidget();

        void setThreadParentChat(const QString& _aimId);
        void setFocusOnFirstFocusable();
        void setFocusOnInput();
        void setFocusOnEmoji();
        void setFocusOnAttach();

        void edit(const Data::MessageBuddySptr& _msg, MediaType _mediaType);

        Data::FString getInputText() const;

        int getQuotesCount() const;

        bool isInputEmpty() const;
        bool isEditing() const;
        bool isReplying() const;
        bool isDraftVersionVisible() const;
        bool isRecordingPtt() const;
        bool tryPlayPttRecord();
        bool tryPausePttRecord();

        bool isPttHold() const;
        bool isPttHoldByKeyboard() const;
        bool isPttHoldByMouse() const;

        void stopPttRecording();
        void startPttRecordingLock();
        void sendPtt();

        void closePttPanel();
        void dropReply();

        QPoint tooltipArrowPosition() const;
        QPoint suggestPosition() const;

        void updateStyle();

        void onAttachMedia(AttachMediaType _mediaType);

        void onAttachPhotoVideo();
        void onAttachFile();
        void onAttachCamera();
        void onAttachContact();
        void onAttachTask();
        void onAttachPtt();
        void onAttachPoll();
        void onAttachCallByLink();
        void onAttachWebinar();

        void updateBackground();
        bool canSetFocus() const;

        bool hasServerSuggests() const;
        bool isServerSuggest(const Utils::FileSharingId& _stickerId) const;
        void clearLastSuggests();

        bool hasSelection() const;

        void setHistoryControlPage(HistoryControlPage* _page);
        void setThreadFeedItem(ThreadFeedItem* _thread);
        void setMentionCompleter(MentionCompleter* _completer);
        void setParentForPopup(QWidget* _parent);

        void onPageLeave();

        void setActive(bool _active);
        bool isActive() const { return active_; }

        bool isTextEditInFocus() const;

    protected:
        void keyPressEvent(QKeyEvent * _e) override;
        void keyReleaseEvent(QKeyEvent * _e) override;
        void resizeEvent(QResizeEvent*) override;
        void showEvent(QShowEvent*) override;
        void childEvent(QChildEvent* _event) override;
        bool eventFilter(QObject* _obj, QEvent* _event) override;

    private:
        void setInitialState();

        void loadInputText();
        void setInputText(const Data::FString& _text, std::optional<int> _cursorPos = {});

        void updateBackgroundGeometry();

        void setEditView();

        bool shouldOfferMentions() const;
        void requestStickerSuggests();

        void pasteFromClipboard();
        void updateInputData();
        void pasteClipboardUrl(const QUrl& _url, bool _hadImage, const QMimeData* _mimeData);
        void pasteClipboardImage(QImage&& _image);
        void pasteClipboardBase64Image(QString&& _text);

        InputWidgetState& currentState();
        const InputWidgetState& currentState() const;
        InputView currentView() const;

        void updateMentionCompleterState(const bool _force = false);

        void switchToMainPanel(const StateTransition _transition);
        void switchToReadonlyPanel(const ReadonlyPanelState _state);
        void switchToDisabledPanel(const DisabledPanelState _state);
        void switchToPttPanel(const StateTransition _transition);
        void switchToMultiselectPanel();

        void setCurrentWidget(QWidget* _widget);

        void initSubmitButton();
        void onSubmitClicked(const ClickType _clickType);
        void onSubmitPressed();
        void onSubmitReleased();
        void onSubmitLongTapped();
        void onSubmitLongPressed();
        void onSubmitMovePressed();

        enum class SetHistoryPolicy
        {
            AddToHistory,
            DontAddToHistory,
        };
        void setView(InputView _view, const UpdateMode _updateMode = UpdateMode::IfChanged, const SetHistoryPolicy _historyPolicy = SetHistoryPolicy::AddToHistory);
        void switchToPreviousView();
        void addViewToHistory(const InputView _view);

        void updateSubmitButtonState(const UpdateMode _updateMode);
        void updateSubmitButtonPos();

        void sendFiles(const FileSource _source = FileSource::CopyPaste);

        void notifyDialogAboutSend();

        void sendStatsIfNeeded() const;

        std::optional<bool> needStartButton() const;

        enum class CheckOffer
        {
            No,
            Yes,
        };
        bool shouldOfferCompleter(const CheckOffer _check) const;

        enum class FileSelectionFilter
        {
            AllFiles,
            MediaOnly,
        };
        void selectFileToSend(const FileSelectionFilter _filter);
        void showFileDialog(const FileSource _source);

        InputStyleMode currentStyleMode() const;

        void updateInputHeight();
        void setInputHeight(const int _height);

        enum class PttMode
        {
            HoldByMouse,
            HoldByKeyboard,
            Lock,
        };
        void startPttRecord();

        void startPtt(PttMode _mode);
        void setPttMode(const std::optional<PttMode>& _mode);
        void sendPttImpl(ptt::StatInfo&&);
        std::optional<PttMode> getPttMode() const noexcept;

        void removePttRecord();

        void onPttReady(const QString& _file, std::chrono::seconds _duration, const ptt::StatInfo& _stat);
        void onPttRemoved();
        void onPttStateChanged(ptt::State2 _state);
        void disablePttCircleHover();

        void showQuotes();
        void hideQuotes();
        void setQuotesVisible(const bool _isVisible);

        QWidget* getCurrentPanel() const;

        void hideSmartreplies() const;

        void processUrls() const;

        void requestSmartrepliesForQuote() const;
        bool isBot() const;

        void createConference(ConferenceType _type);

        enum class SyncDraft
        {
            No,
            Yes
        };

        void saveDraft(SyncDraft _sync = SyncDraft::No, bool _textChanged = false);
        void clearDraft();
        void applyDraft(const Data::Draft& _draft);
        void startDraftTimer();
        void stopDraftTimer();
        void requestDraft();

        Data::MessageBuddy makeMessage(Data::FStringView _text) const;
        Data::MentionMap makeMentions(Data::FStringView _text) const;

        void transitionInit();
        void transitionCacheCurrent();
        void transitionAnimate(int _fromHeight, int _targetHeight);

        void activateParentLayout();

        bool showMentionCompleter(const QString& _initialPattern, const QPoint& _pos);
        void hideMentionCompleter();

        bool isSuggestsEnabled() const;
        bool isPttEnabled() const;

        void setHistoryControlPageToEdit();
        void setThreadFeedItemToEdit();

        void cancelDraftVersionByEdit();

        void createTaskWidget(const Data::FString& _text, const Data::QuotesVec& _quotes, const Data::MentionMap& _mentions, const QString& _assignee, const bool _isThreadFeedMessage);


    private:
        std::optional<PttMode> pttMode_;

        QString contact_;
        QString threadParentChat_;

        InputBgWidget* bgWidget_ = nullptr;

        QuotesWidget* quotesWidget_ = nullptr;
        QStackedWidget* stackWidget_ = nullptr;

        InputPanelMain* panelMain_ = nullptr;
        InputPanelDisabled* panelDisabled_ = nullptr;
        InputPanelReadonly* panelReadonly_ = nullptr;
#ifndef STRIP_AV_MEDIA
        InputPanelPtt* panelPtt_ = nullptr;
#endif // !STRIP_AV_MEDIA
        InputPanelMultiselect* panelMultiselect_ = nullptr;

        PttLock* pttLock_ = nullptr;

        HistoryTextEdit* textEdit_ = nullptr;
        SubmitButton* buttonSubmit_ = nullptr;

        InputStatePtr state_;

        QPoint suggestPos_;
        QPoint emojiPos_;
        QTimer* suggestTimer_ = nullptr;

        bool isExternalPaste_ = false;
        bool canLockPtt_ = false;

        SelectContactsWidget* selectContactsWidget_ = nullptr;

        QLabel* transitionLabel_ = nullptr;
        TransitionAnimation* transitionAnim_ = nullptr;

        bool setFocusToSubmit_ = false;

        bool suggestRequested_ = false;
        std::vector<Utils::FileSharingId> lastRequestedSuggests_;

        Utils::CallLinkCreator* callLinkCreator_ = nullptr;

        InputFeatures features_;

        HistoryControlPage* historyPage_ = nullptr;
        ThreadFeedItem* threadFeedItem_ = nullptr;
        MentionCompleter* mentionCompleter_ = nullptr;
        QTimer* draftTimer_ = nullptr;
        bool active_;
    };
}
