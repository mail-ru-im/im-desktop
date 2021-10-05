#pragma once

#include "HistoryControlPageItem.h"

namespace Ui
{
    class service_message_item;
    class ClickableTextWidget;

    namespace TextRendering
    {
        class TextUnit;
    }

    class ServiceMessageItem : public HistoryControlPageItem
    {
        Q_OBJECT

    Q_SIGNALS:
        void clicked();

    public:
        ServiceMessageItem(QWidget* _parent, const bool _overlay = false);
        ~ServiceMessageItem();

        QString formatRecentsText() const override;
        MediaType getMediaType(MediaRequestMode _mode = MediaRequestMode::Chat) const override;

        void setMessage(const QString& _message);
        void setWidth(const int _width);
        void setDate(const QDate& _date);

        void setNew();
        bool isNew() const;

        bool isDate() const;
        bool isOverlay() const;

        void setThreadReplies(int _numNew, int _numOld);

        bool isFloating() const;

        void updateStyle() override;
        void updateFonts() override;

        bool isOutgoing() const override { return false; }
        int32_t getTime() const override { return -1; }

    protected:
        void setQuoteSelection() override {}
        void paintEvent(QPaintEvent* _event) override;
        bool canBeThreadParent() const override { return false; }
        bool supportsOverlays() const override { return false; }

    private:
        void updateDateTranslation();

        void drawDate(QPainter& _p);
        void drawPlateNew(QPainter& _p);
        void drawOverlay(QPainter& _p);
        void drawThreadReplies(QPainter& _p);

        QFont getFont();
        QColor getColor();

    private:
        enum class Type
        {
            date,
            plateNew,
            overlay,
            threadReplies,
        } type_;

        bool isFloating_;
        QDate lastUpdated_;
        QDate date_;
        QString message_;
        std::unique_ptr<TextRendering::TextUnit> messageUnit_;
        ClickableTextWidget* clickableText_ = nullptr;

        QPainterPath bubble_;
        QRect bubbleGeometry_;
    };
}
