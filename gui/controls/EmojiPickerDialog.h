#pragma once
#include "cache/emoji/EmojiCode.h"

namespace Ui
{
    class EmojiPickerDialog : public QWidget
    {
        Q_OBJECT
    public:
        enum PopupDirection
        {
            Top = 0x1,
            Bottom = 0x2,
            Left = 0x4,
            Right = 0x8
        };
        Q_DECLARE_FLAGS(PopupDirections, PopupDirection)

        EmojiPickerDialog(QWidget* _inputWidget, QWidget* _emojiButton, QWidget* _alignWidget);
        ~EmojiPickerDialog();

        void setAllowedDirections(PopupDirections _dir);
        PopupDirections allowedDirections() const;

    public Q_SLOTS:
        void popup();

    Q_SIGNALS:
        void emojiSelected(const Emoji::EmojiCode& _code, QPrivateSignal);

    protected:
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void closeEvent(QCloseEvent* _event) override;
        void focusOutEvent(QFocusEvent* _event) override;
        bool eventFilter(QObject* _object, QEvent* _event) override;

    private:
        void onWindowResized();

    private:
        std::unique_ptr<class EmojiPickerDialogPrivate> d;
    };
    Q_DECLARE_OPERATORS_FOR_FLAGS(Ui::EmojiPickerDialog::PopupDirections)
}
