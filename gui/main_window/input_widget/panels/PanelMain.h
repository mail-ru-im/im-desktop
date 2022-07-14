#pragma once

#include "types/message.h"
#include "../InputWidgetUtils.h"

#include "main_window/EscapeCancellable.h"

namespace Ui
{
    class EditMessageWidget;
    class HistoryTextEdit;
    class CustomButton;
    class SubmitButton;
    class AttachFileButton;
    class AttachFilePopup;
    class InputPanelMain;
    class DraftVersionWidget;
    enum class ClickType;
    enum class AttachMediaType;

    class TextEditViewport : public QWidget
    {
        Q_OBJECT

    public:
        TextEditViewport(QWidget* _parent, InputPanelMain* _panelMain);
        void adjustHeight(const int _curEditHeight);

    protected:
        void resizeEvent(QResizeEvent* _event) override;

    private:
        int topOffset() const;
        int minHeight() const;
        int maxHeight() const;
        void adjustTextEditPos();

    private:
        HistoryTextEdit* textEdit_;
        InputPanelMain* panelMain_;
    };

    class InputSide : public QWidget
    {
        Q_OBJECT

    public:
        enum class SideState
        {
            Minimized,
            Normal
        };

        InputSide(QWidget* _parent, SideState _initialState = SideState::Normal);

        SideState getState() const noexcept;

        void resizeAnimated(const SideState _state);

    private:
        int getStateWidth(const SideState _state) const;
        void initAnimation();

    private:
        QVariantAnimation* anim_ = nullptr;
        SideState state_ = SideState::Normal;
    };


    class InputPanelMain : public QWidget, public StyledInputElement, public IEscapeCancellable
    {
        Q_OBJECT

    Q_SIGNALS:
        void editedMessageClicked(QPrivateSignal) const;
        void editCancelClicked(QPrivateSignal) const;
        void emojiButtonClicked(const bool _fromKeyboard, QPrivateSignal) const;
        void attachMedia(AttachMediaType _mediaType, QPrivateSignal) const;
        void draftVersionAccepted(QPrivateSignal);
        void draftVersionCancelled(QPrivateSignal);

        void panelHeightChanged(const int _height, QPrivateSignal) const;

    public:
        InputPanelMain(QWidget* _parent, SubmitButton* _submit, InputFeatures _features);
        ~InputPanelMain();

        void setSubmitButton(SubmitButton* _button);

        void showEdit(Data::MessageBuddySptr _message, const MediaType _type);
        void cancelEdit();
        void showDraftVersion(const Data::Draft& _draft);
        void cancelDraft();
        void hidePopups();

        void setFocusOnInput();
        void setFocusOnEmoji();
        void setFocusOnAttach();

        void setEmojiButtonChecked(const bool _checked);

        HistoryTextEdit* getTextEdit() const;
        void recalcHeight();

        int viewportMinHeight() const;
        int viewportMaxHeight() const;

        QRect getAttachFileButtonRect() const;

        void setUnderQuote(const bool _underQuote);

        void forceSendButton(const bool _value);

        void hideAttachPopup();

        bool isDraftVersionVisible() const;

        void setParentForPopup(QWidget* _parent);

    private:
        void onAttachClicked(const ClickType _type);

        void initEdit();

        void onTextEditClicked();

        enum class ResizeCondition
        {
            IfChanged,
            Force
        };
        void resizeAnimated(const int _height, const ResizeCondition _condition = ResizeCondition::IfChanged);

        void setEditHeight(const int _newHeight);
        void onDocumentSizeChanged(const QSizeF& _newSize);
        void onDocumentContentsChanged();
        void updateTextEditViewportMargins();

        void setCurrentTopWidget(QWidget* _widget);

        void setLeftSideState(InputSide::SideState _state);
        void setRightSideState(InputSide::SideState _state);
        void checkHideSubmit();

        void editContentChanged();
        void scrollToBottomIfNeeded();

        void setButtonsTabOrder();

        void initResizeAnimation();
        void initAttachPopup();

        bool isAttachEnabled() const;
        bool isPttEnabled() const;

    protected:
        void updateStyleImpl(const InputStyleMode _mode) override;
        void hideEvent(QHideEvent* _e) override;

    private:
        QWidget* topHost_ = nullptr;
        QWidget* currentTopWidget_ = nullptr;

        QWidget* bottomHost_ = nullptr;
        InputSide* leftSide_ = nullptr;
        InputSide* rightSide_ = nullptr;

        QWidget* emptyTop_ = nullptr;
        EditMessageWidget* edit_ = nullptr;
        DraftVersionWidget* draftVersionWidget_ = nullptr;

        TextEditViewport* editViewport_ = nullptr;
        HistoryTextEdit* textEdit_ = nullptr;

        AttachFileButton* buttonAttach_ = nullptr;
        CustomButton* buttonEmoji_ = nullptr;

        SubmitButton* buttonSubmit_ = nullptr;

        QVariantAnimation* animResize_ = nullptr;
        int curEditHeight_ = 0;
        bool forceSendButton_ = false;
        InputFeatures features_;

        AttachFilePopup* attachPopup_ = nullptr;
        QWidget* parentForPopup_ = nullptr;
    };
}
