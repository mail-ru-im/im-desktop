#pragma once

#include "controls/TextUnit.h"
#include "types/message.h"

#include "InputWidgetUtils.h"
#include "FileToSend.h"

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
}

namespace Ui
{
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

    enum class InputView
    {
        Default,
        Readonly,
        Disabled,
        Edit,
        Ptt,
        Multiselect,
    };

    enum class FileSource
    {
        Clip,
        CopyPaste
    };

    struct InputWidgetState
    {
        Data::QuotesVec quotes_;
        Data::MentionMap mentions_;
        Data::FilesPlaceholderMap files_;

        FilesToSend filesToSend_;
        QString description_;

        struct
        {
            Data::MessageBuddySptr message_;
            QString buffer_;
            MediaType type_;
            QString editableText_;
        } edit_;

        bool canBeEmpty_ = false;
        bool sendAsFile_ = false;

        InputView view_ = InputView::Default;
        std::vector<InputView> viewHistory_;
        int mentionSignIndex_ = -1;

        bool msgHistoryReady_ = false;

        void clear(const InputView _newView = InputView::Default)
        {
            quotes_.clear();
            mentions_.clear();
            filesToSend_.clear();
            description_.clear();
            edit_.message_.reset();
            edit_.buffer_.clear();
            edit_.type_ = MediaType::noMedia;
            edit_.editableText_.clear();
            view_ = _newView;
            viewHistory_.clear();
            mentionSignIndex_ = -1;
            canBeEmpty_ = false;
            sendAsFile_ = false;
        }

        bool isDisabled() const noexcept
        {
            return view_ == InputView::Disabled || view_ == InputView::Readonly;
        }

        bool hasQuotes() const
        {
            return !quotes_.isEmpty();
        }
    };

    class InputWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void mentionSignPressed(const QString& _dialogAimId);
        void smilesMenuSignal(const bool _fromKeyboard, QPrivateSignal) const;
        void sendMessage(const QString&);
        void editFocusOut();
        void inputTyped();
        void needSuggets(const QString _forText, const QPoint _pos, QPrivateSignal) const;
        void hideSuggets();
        void resized();
        void needClearQuotes();
        void quotesDropped(QPrivateSignal) const;
        void quotesAdded(QPrivateSignal) const;

    public:
        void quote(const Data::QuotesVec& _quotes);
        void contactSelected(const QString& _contact);
        void insertEmoji(const Emoji::EmojiCode& _code, const QPoint _pt);
        void sendSticker(const QString& _stickerId);
        void onSmilesVisibilityChanged(const bool _isVisible);
        void insertMention(const QString& _aimId, const QString& _friendly);
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

        void hideAndRemoveDialog(const QString& _contact);

        void onSuggestTimeout();

        void onNickInserted(const int _atSignPos, const QString& _nick);
        void insertFiles();
        void onContactChanged(const QString& _contact);
        void ignoreListChanged();
        void multiselectChanged();
        void onRecvPermitDeny();

        void onRequestedStickerSuggests(const QVector<QString>& _suggests);
        void onQuotesHeightChanged();

    public:
        InputWidget(QWidget* _parent, BackgroundWidget* _bg);
        ~InputWidget();

        void setFocusOnFirstFocusable();
        void setFocusOnInput();
        void setFocusOnEmoji();
        void setFocusOnAttach();

        void edit(const Data::MessageBuddySptr& _msg, MediaType _mediaType);

        void loadInputText();
        void setInputText(const QString& _text, int _pos = -1);
        QString getInputText() const;

        const Data::MentionMap& getInputMentions() const;
        const Data::QuotesVec& getInputQuotes();
        const Data::FilesPlaceholderMap& getInputFiles() const;
        int getQuotesCount() const;

        bool isInputEmpty() const;
        bool isEditing() const;
        bool isReplying() const;
        bool isRecordingPtt() const;
        bool isRecordingPtt(const QString& _contact) const;
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

        void hideAndClear();
        QPoint tooltipArrowPosition() const;

        void updateStyle();

        void onAttachPhotoVideo();
        void onAttachFile();
        void onAttachCamera();
        void onAttachContact();
        void onAttachPtt();
        void onAttachPoll();
        void onAttachCallByLink();
        void onAttachWebinar();

        QRect getAttachFileButtonRect() const;

        void updateBackground();
        bool canSetFocus() const;

        bool hasServerSuggests() const;
        bool isServerSuggest(const QString& _stickerId) const;
        void clearLastSuggests();

        bool hasSelection() const;

    protected:
        void keyPressEvent(QKeyEvent * _e) override;
        void keyReleaseEvent(QKeyEvent * _e) override;
        void resizeEvent(QResizeEvent*) override;
        void showEvent(QShowEvent*) override;

    private:
        void updateBackgroundGeometry();

        void setEditView();

        bool isAllAttachmentsEmpty(const QString& _contact) const;

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
        InputView currentView(const QString& _contact) const;

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

        void notifyDialogAboutSend(const QString& _contact);

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

        void needToStopPtt();
        void removePttRecord();

        void onPttReady(const QString& _contact, const QString& _file, std::chrono::seconds _duration, const ptt::StatInfo& _stat);
        void onPttRemoved(const QString& _contact);
        void onPttStateChanged(const QString& _contact, ptt::State2 _state);
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

        Data::MessageBuddy makeMessage(QString _text) const;
        Data::MentionMap makeMentions(QStringView _text) const;

    private:
        std::optional<PttMode> pttMode_;

        QString contact_;

        InputBgWidget* bgWidget_;

        QuotesWidget* quotesWidget_;
        QStackedWidget* stackWidget_;

        InputPanelMain* panelMain_;
        InputPanelDisabled* panelDisabled_;
        InputPanelReadonly* panelReadonly_;
#ifndef STRIP_AV_MEDIA
        InputPanelPtt* panelPtt_;
#endif // !STRIP_AV_MEDIA
        InputPanelMultiselect* panelMultiselect_;

        PttLock* pttLock_;

        HistoryTextEdit* textEdit_;
        SubmitButton* buttonSubmit_;

        QPixmap bg_;

        std::map<QString, InputWidgetState> states_;

        QPoint suggestPos_;
        QTimer* suggestTimer_;

        bool isExternalPaste_ = false;
        bool canLockPtt_ = false;

        SelectContactsWidget* selectContactsWidget_;

        QLabel* transitionLabel_;
        struct TransitionAnimation : public QVariantAnimation
        {
            TransitionAnimation(QObject* parent = nullptr) : QVariantAnimation(parent) { }
            int currentHeight_;
            int targetHeight_;
            QGraphicsOpacityEffect* targetEffect_;
        } * transitionAnim_;

        bool setFocusToSubmit_ = false;

        bool suggestRequested_ = false;
        std::vector<QString> lastRequestedSuggests_;

        Utils::CallLinkCreator* callLinkCreator_;
    };
}
