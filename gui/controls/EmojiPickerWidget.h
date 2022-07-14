#pragma once
#include "cache/emoji/EmojiCode.h"
#include "EmojiPickerDialog.h"

namespace Ui
{
    //! Used in non-HistoryTextEdit inputs (custom status, file description, poll question)
    class EmojiPickerWidget : public QWidget
    {
        Q_OBJECT
    public:
        enum class ArrowDirection // excactly the opposite to EmojiPickerDialog::PopupDirection
        {
            Top = EmojiPickerDialog::PopupDirection::Bottom,
            Bottom = EmojiPickerDialog::PopupDirection::Top,
            Left = EmojiPickerDialog::PopupDirection::Right,
            Right = EmojiPickerDialog::PopupDirection::Left
        };

        EmojiPickerWidget(QWidget* _parent);
        ~EmojiPickerWidget();

        void setArrow(ArrowDirection _direction, int _offset);

        static int maxPickerHeight() noexcept;
        static int maxPickerWidth() noexcept;

    Q_SIGNALS:
        void emojiSelected(const Emoji::EmojiCode& _code, QPrivateSignal);

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        std::unique_ptr<class EmojiPickerPrivate> d;
    };
}
