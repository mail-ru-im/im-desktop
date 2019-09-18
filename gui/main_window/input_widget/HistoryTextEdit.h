#pragma once

#include "HistoryUndoStack.h"
#include "Text2Emoji.h"
#include "controls/TextEditEx.h"
#include "animation/animation.h"

namespace Ui
{
    class HistoryTextEdit : public TextEditEx
    {
        Q_OBJECT

    Q_SIGNALS:
        void needsMentionComplete(const int _position, QPrivateSignal) const;

    public:
        HistoryTextEdit(QWidget* _parent);
        bool catchEnter(const int _modifiers) override;
        bool catchNewLine(const int _modifiers) override;
        void setPlaceholderTextEx(const QString& _text);
        void setPlaceholderAnimationEnabled(const bool _enabled);

        void updateLinkColor();

        Emoji::TextEmojiReplacer& getReplacer();
        HistoryUndoStack& getUndoRedoStack();

        bool tabWillFocusNext();

    protected:
        void keyPressEvent(QKeyEvent*) override;
        void paintEvent(QPaintEvent* _event) override;
        bool focusNextPrevChild(bool _next) override;

    private:
        void checkMentionNeeded();
        void onEditContentChanged();

    private:
        QString customPlaceholderText_;
        anim::Animation placeholderAnim_;
        anim::Animation placeholderOpacityAnim_;
        enum class AnimType
        {
            Appear,
            Disappear,
        };
        AnimType phAnimType_;
        bool isEmpty_;
        bool phAnimEnabled_;

        std::unique_ptr<HistoryUndoStack> stackRedo_;
        std::unique_ptr<Emoji::TextEmojiReplacer> emojiReplacer_;
    };
}