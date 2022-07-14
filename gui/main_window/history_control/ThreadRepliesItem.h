#pragma once

#include "HistoryControlPageItem.h"

namespace Ui
{
    class ClickableTextWidget;
    class TextWidget;

    class ThreadRepliesItem : public HistoryControlPageItem
    {
        Q_OBJECT

    public:
        ThreadRepliesItem(QWidget* _parent, const QString& _chatAimId = {});

        QString formatRecentsText() const override;
        MediaType getMediaType(MediaRequestMode _mode = MediaRequestMode::Chat) const override;

        void setThreadReplies(int _numNew, int _numOld);

        void updateFonts() override;

        bool isOutgoing() const override { return false; }
        int32_t getTime() const override { return -1; }

        void setMessageKey(const Logic::MessageKey& _key);

    Q_SIGNALS:
        void clicked(QPrivateSignal);

    protected:
        void setQuoteSelection() override {}
        void paintEvent(QPaintEvent* _event) override;
        bool canBeThreadParent() const override { return false; }
        bool supportsOverlays() const override { return false; }

    private:
        Styling::ThemeColorKey textColorKey() const;
        QColor backgroundColor() const;

    private Q_SLOTS:
        void onMoreClick();

    private:
        TextWidget* textWidget_;
        ClickableTextWidget* clickableText_ = nullptr;
        QWidget* content_;
    };
} // namespace Ui
