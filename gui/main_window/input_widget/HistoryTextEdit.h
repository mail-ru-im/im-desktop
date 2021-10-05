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
        void navigationKeyPressed(int _key, Qt::KeyboardModifiers _modifiers, QPrivateSignal) const;

    public:
        HistoryTextEdit(QWidget* _parent);
        void setPlaceholderTextEx(const QString& _text);
        void setPlaceholderAnimationEnabled(const bool _enabled);

        void updateLinkColor();

        Emoji::TextSymbolReplacer& getReplacer();
        HistoryUndoStack& getUndoRedoStack();

        bool tabWillFocusNext();

        void setMarginScaleCorrection(qreal _koef);
        qreal getMarginScaleCorrection() const;

        bool isEmpty() const;

        QWidget* setContentWidget(QWidget* _widget); // return previous widget
        void setCursorPaintHackEnabled(bool _enabled) { useCursorPaintHack_ = _enabled; }

        void registerTextChange();
        std::pair<QString, int> getLastWord();
        bool replaceEmoji();

    protected:
        void keyPressEvent(QKeyEvent*) override;
        void paintEvent(QPaintEvent* _event) override;
        bool focusNextPrevChild(bool _next) override;
        void contextMenuEvent(QContextMenuEvent *e) override;

    private:
        void checkMentionNeeded();
        void onEditContentChanged();

        void onIntermediateTextChange(const Data::FString& _string, int _cursor);

        bool isPreediting() const;

        bool passKeyEventToContentWidget(QKeyEvent* _e);
        bool passKeyEventToPopup(QKeyEvent* _e);
        bool processUndoRedo(QKeyEvent* _e);

        std::vector<QWidget*> getInputPopups() const;

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

        QTimer* keyPressTimer_;
        bool useCursorPaintHack_ = true;

        // use this value to correct the margin from the document layout in the multi-screen system
        qreal marginScaleCorrection_;

        QWidget* contentWidget_ = nullptr;
    };
}
