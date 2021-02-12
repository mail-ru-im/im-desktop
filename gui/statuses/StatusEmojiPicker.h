#pragma once

#include "cache/emoji/EmojiCode.h"

namespace Statuses
{

    class StatusEmojiPicker_p;

    //////////////////////////////////////////////////////////////////////////
    // StatusEmojiPicker
    //////////////////////////////////////////////////////////////////////////

    class StatusEmojiPicker : public QWidget
    {
        Q_OBJECT
    public:
        StatusEmojiPicker(QWidget* _parent);
        ~StatusEmojiPicker();

    Q_SIGNALS:
        void emojiSelected(const Emoji::EmojiCode& _code, QPrivateSignal);

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        std::unique_ptr<StatusEmojiPicker_p> d;
    };


    class StatusEmojiPickerDialog_p;

    class StatusEmojiPickerDialog : public QWidget
    {
        Q_OBJECT
    public:
        StatusEmojiPickerDialog(QWidget* _emojiButton, QWidget* _parent);
        ~StatusEmojiPickerDialog();

    Q_SIGNALS:
        void emojiSelected(const Emoji::EmojiCode& _code, QPrivateSignal);

    protected:
        void mousePressEvent(QMouseEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private Q_SLOTS:
        void onWindowResized();

    private:
        std::unique_ptr<StatusEmojiPickerDialog_p> d;
    };


}
