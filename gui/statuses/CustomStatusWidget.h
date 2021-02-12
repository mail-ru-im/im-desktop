#pragma once

#include "cache/emoji/EmojiCode.h"
#include "Status.h"

namespace Statuses
{

    class CustomStatusWidget_p;

    //////////////////////////////////////////////////////////////////////////
    // CustomStatusWidget
    //////////////////////////////////////////////////////////////////////////

    class CustomStatusWidget : public QWidget
    {
        Q_OBJECT
    public:
        CustomStatusWidget(QWidget* _parent);
        ~CustomStatusWidget();

        void setStatus(const Status& _status);

    Q_SIGNALS:
        void backClicked(QPrivateSignal);

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void showEvent(QShowEvent* _event) override;

    private Q_SLOTS:
        void onTextChanged();
        void onNextClicked();
        void tryToConfirmStatus();

    private:
        void confirmStatus();

        std::unique_ptr<CustomStatusWidget_p> d;
    };


    class CustomStatusEmojiWidget_p;

    //////////////////////////////////////////////////////////////////////////
    // CustomStatusEmojiWidget
    //////////////////////////////////////////////////////////////////////////

    class CustomStatusEmojiWidget : public QWidget
    {
        Q_OBJECT
    public:
        CustomStatusEmojiWidget(QWidget* _parent);
        ~CustomStatusEmojiWidget();

        enum class SkipAnimation { Yes, No };

        void setCurrent(const Emoji::EmojiCode& _code, SkipAnimation _skipAnimation = SkipAnimation::No);
        const QString& current() const;
        void reset();

    Q_SIGNALS:
        void clicked(QPrivateSignal);

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;

    private:
        std::unique_ptr<CustomStatusEmojiWidget_p> d;
    };
}
