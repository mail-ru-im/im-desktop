#pragma once

#include "controls/TextUnit.h"
#include "types/message.h"
#include "animation/animation.h"

#include "InputWidgetUtils.h"

namespace Emoji
{
    class EmojiCode;
}

namespace ptt
{
    class AudioRecorder;
    class AudioRecorder2;
    struct StatInfo;
}

namespace Ui
{
    class MentionCompleter;
    class SubmitButton;

    class HistoryTextEdit;

    class QuotesWidget;

    class InputPanelMain;
    class InputPanelBanned;
    class InputPanelReadonly;
    class InputPanelPtt;

    class InputBgWidget;
    class BackgroundWidget;

    class PttLock;
    class SelectContactsWidget;

    enum class ReadonlyPanelState;
    enum class BannedPanelState;
    enum class ClickType;

    enum class InputView
    {
        Default,
        Readonly,
        Banned,
        Edit,
        Ptt,
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

        QPixmap imageBuffer_;
        QString description_;
        QStringList filesToSend_;

        struct
        {
            Data::MessageBuddySptr message_;
            Data::QuotesVec quotes_;
            QString buffer_;
        } edit_;

        InputView view_ = InputView::Default;
        std::vector<InputView> viewHistory_;
        int mentionSignIndex_ = -1;

        void clear(const InputView _newView = InputView::Default)
        {
            quotes_.clear();
            mentions_.clear();
            imageBuffer_ = QPixmap();
            filesToSend_.clear();
            description_.clear();
            edit_.message_.reset();
            edit_.buffer_.clear();
            edit_.quotes_.clear();
            view_ = _newView;
            viewHistory_.clear();
            mentionSignIndex_ = -1;
        }

        bool isDisabled() const noexcept
        {
            return view_ == InputView::Banned || view_ == InputView::Readonly;
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
        void needSuggets(const QString _forText, const QPoint _pos);
        void hideSuggets();
        void resized();
        void needClearQuotes();

    public:
        void quote(const Data::QuotesVec& _quotes);
        void contactSelected(const QString& _contact);
        void insertEmoji(const Emoji::EmojiCode& _code, const QPoint _pt);
        void sendSticker(const int32_t _set_id, const QString& _sticker_id);
        void onSmilesVisibilityChanged(const bool _isVisible);
        void clearInputText();
        void insertMention(const QString& _aimId, const QString& _friendly);
        void messageIdsFetched(const QString& _aimId, const Data::MessageBuddies&);

        enum class CursorPolicy
        {
            Keep,
            Set,
        };
        void mentionOffer(const int _position, const CursorPolicy _policy);

    private Q_SLOTS:
        void textChanged();
        void editContentChanged();
        void selectionChanged();
        void cursorPositionChanged();
        void inputPositionChanged();
        void enterPressed();

        void send();
        void sendPiece(const QStringRef& _msg);

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
        void onRecvPermitDeny();

    public:
        InputWidget(QWidget* _parent, BackgroundWidget* _bg);
        ~InputWidget();

        void setFocusOnFirstFocusable();
        void setFocusOnInput();
        void setFocusOnEmoji();
        void setFocusOnAttach();

        void edit(
            const int64_t _msgId,
            const QString& _internalId,
            const common::tools::patch_version& _patchVersion,
            const QString& _text,
            const Data::MentionMap& _mentions,
            const Data::QuotesVec& _quotes,
            qint32 _time);

        void editWithCaption(
            const int64_t _msgId,
            const QString& _internalId,
            const common::tools::patch_version& _patchVersion,
            const QString& _url,
            const QString& _description);

        void loadInputTextFromCL();
        void setInputText(const QString& _text, int _pos = -1);
        QString getInputText() const;

        const Data::MentionMap& getInputMentions() const;
        const Data::QuotesVec& getInputQuotes() const;

        bool isInputEmpty() const;
        bool isEditing() const;
        bool isReplying() const;
        bool isRecordingPtt() const;
        bool isRecordingPtt(const QString& _contact) const;

        bool isPttHold() const;

        void stopPttRecording();
        void startPttRecordingLock();
        void startPttRecordingHold();
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

        QRect getAttachFileButtonRect() const;

        void updateBackground();
        bool canSetFocus() const;

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
        void checkSuggest();

        void pasteFromClipboard();

        InputWidgetState& currentState();
        const InputWidgetState& currentState() const;
        InputView currentView() const;
        InputView currentView(const QString& _contact) const;

        void updateMentionCompleterState(const bool _force = false);

        void switchToMainPanel(const StateTransition _transition);
        void switchToReadonlyPanel(const ReadonlyPanelState _state);
        void switchToBannedPanel(const BannedPanelState _state);
        void switchToPttPanel(const StateTransition _transition);

        void setCurrentWidget(QWidget* _widget);

        void initSubmitButton();
        void onSubmitClicked(const ClickType _clickType);
        void onSubmitPressed();
        void onSubmitReleased();
        void onSubmitLongTapped();
        void onSubmitMovePressed();

        enum class SetHistoryPolicy
        {
            AddToHistory,
            DontAddToHistory,
        };
        void setView(const InputView _view, const UpdateMode _updateMode = UpdateMode::IfChanged, const SetHistoryPolicy _historyPolicy = SetHistoryPolicy::AddToHistory);
        void switchToPreviousView();
        void addViewToHistory(const InputView _view);

        void updateSubmitButtonState(const UpdateMode _updateMode);
        void updateSubmitButtonPos();

        void sendFiles(const FileSource _source = FileSource::CopyPaste);

        void sendStatsIfNeeded() const;

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
            Hold,
            Lock,
        };
        void startPttRecord();

        void startPtt(PttMode _mode);
        void setPttMode(PttMode _mode);
        void sendPttImpl(ptt::StatInfo&&);
        std::optional<PttMode> getPttMode() const noexcept;

        void needToStopPtt();
        void removePttRecord();

        void onPttReady(const QString& _contact, const QString& _file, std::chrono::seconds _duration, const ptt::StatInfo& _stat);
        void onPttRemoved(const QString& _contact);

        void showQuotes();
        void hideQuotes();
        void setQuotesVisible(const bool _isVisible);

        QWidget* getCurrentPanel() const;

    private:
        std::optional<PttMode> pttMode_;

    private:
        QString contact_;

        InputBgWidget* bgWidget_;

        QuotesWidget* quotesWidget_;
        QStackedWidget* stackWidget_;

        InputPanelMain* panelMain_;
        InputPanelBanned* panelBanned_;
        InputPanelReadonly* panelReadonly_;
        InputPanelPtt* panelPtt_;

        PttLock* pttLock_;

        HistoryTextEdit* textEdit_;
        SubmitButton* buttonSubmit_;

        QPixmap bg_;

        std::map<QString, InputWidgetState> states_;

        QPoint suggestPos_;
        QTimer suggestTimer_;

        bool isExternalPaste_ = false;
        bool canLockPtt_ = false;

        SelectContactsWidget* selectContactsWidget_;

        QLabel* transitionLabel_;
        anim::Animation transitionAnim_;
        bool setFocusToSubmit_ = false;
    };
}
