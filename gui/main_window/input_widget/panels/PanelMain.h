#pragma once

#include "types/message.h"
#include "animation/animation.h"
#include "../InputWidgetUtils.h"

namespace Ui
{
    class EditMessageWidget;
    class HistoryTextEdit;
    class CustomButton;
    class SubmitButton;
    class AttachFileButton;
    class InputPanelMain;
    enum class ClickType;

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
        InputSide(QWidget* _parent);

        enum class SideState
        {
            Minimized,
            Normal
        };
        SideState getState() const noexcept;

        void resizeAnimated(const SideState _state);

    private:
        int getStateWidth(const SideState _state) const;

    private:
        anim::Animation anim_;
        SideState state_;
    };


    class InputPanelMain : public QWidget, public StyledInputElement
    {
        Q_OBJECT

    Q_SIGNALS:
        void editedMessageClicked(QPrivateSignal) const;
        void editCancelClicked(QPrivateSignal) const;
        void emojiButtonClicked(const bool _fromKeyboard, QPrivateSignal) const;

        void panelHeightChanged(const int _height, QPrivateSignal) const;

    public:
        InputPanelMain(QWidget* _parent, SubmitButton* _submit);
        ~InputPanelMain();

        void setSubmitButton(SubmitButton* _button);

        void showEdit(Data::MessageBuddySptr _message);
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

        void setSideStates(const InputSide::SideState _stateLeft, const InputSide::SideState _stateRight);
        void setLeftSideState(const InputSide::SideState _state);
        void setRightSideState(const InputSide::SideState _state);

        void editContentChanged();
        void scrollToBottomIfNeeded();

        void setButtonsTabOrder();

    protected:
        void updateStyleImpl(const InputStyleMode _mode) override;

    private:
        QWidget* topHost_;
        QWidget* currentTopWidget_;

        QWidget* bottomHost_;
        InputSide* leftSide_;
        InputSide* rightSide_;

        QWidget* emptyTop_;
        EditMessageWidget* edit_;

        TextEditViewport* editViewport_;
        HistoryTextEdit* textEdit_;

        AttachFileButton* buttonAttach_;
        CustomButton* buttonEmoji_;

        SubmitButton* buttonSubmit_;

        anim::Animation animResize_;
        int curEditHeight_;
    };
}