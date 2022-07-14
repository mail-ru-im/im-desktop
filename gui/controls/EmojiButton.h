#pragma once

#include "cache/emoji/EmojiCode.h"

namespace Ui
{
    class EmojiButton : public QWidget
    {
        Q_OBJECT
    public:
        EmojiButton(QWidget* _parent);
        ~EmojiButton();

        enum class SkipAnimation { Yes, No };

        void setCurrent(const Emoji::EmojiCode& _code, SkipAnimation _skipAnimation = SkipAnimation::No);
        const QString& current() const;
        void reset();
        void setFixedSize(QSize _size);

    Q_SIGNALS:
        void clicked(QPrivateSignal);

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;

    private:
        std::unique_ptr<class EmojiButtonPrivate> d;
    };
}
