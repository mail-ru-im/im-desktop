#pragma once

#include "HistoryUndoStack.h"
#include "Text2Symbol.h"
#include "controls/TextEditEx.h"

namespace Ui
{
    class HistoryTextEdit : public TextEditEx
    {
        Q_OBJECT

    Q_SIGNALS:
        void needsMentionComplete(const int _position, QPrivateSignal) const;

    public:
        HistoryTextEdit(QWidget* _parent);
        void setPlaceholderTextEx(const QString& _text);
        void setPlaceholderAnimationEnabled(const bool _enabled);

        void updateLinkColor();

        Emoji::TextSymbolReplacer& getReplacer();
        HistoryUndoStack& getUndoRedoStack();

        bool tabWillFocusNext();

    protected:
        void keyPressEvent(QKeyEvent*) override;
        void paintEvent(QPaintEvent* _event) override;
        bool focusNextPrevChild(bool _next) override;
        void contextMenuEvent(QContextMenuEvent *e) override;

    private:
        void checkMentionNeeded();
        void onEditContentChanged();

        bool isPreediting() const;

    private:
        QString customPlaceholderText_;
        QVariantAnimation* placeholderAnim_;
        QVariantAnimation* placeholderOpacityAnim_;
        enum class AnimType
        {
            Appear,
            Disappear,
        };
        AnimType phAnimType_;
        bool isEmpty_;
        bool isPreediting_;
        bool phAnimEnabled_;

        std::unique_ptr<HistoryUndoStack> stackRedo_;
        std::unique_ptr<Emoji::TextSymbolReplacer> emojiReplacer_;
    };
}
