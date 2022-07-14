#pragma once

#include "HistoryControlPageItem.h"

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class ServiceMessageItem : public HistoryControlPageItem
    {
        Q_OBJECT

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

        bool isFloating() const;

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

        QFont getFont();
        Styling::ThemeColorKey getColor();

    private:
        enum class Type
        {
            date,
            plateNew,
            overlay,
        } type_;

        QDate lastUpdated_;
        QDate date_;
        QString message_;
        std::unique_ptr<TextRendering::TextUnit> messageUnit_;

        QPainterPath bubble_;
        QRect bubbleGeometry_;
        bool isFloating_;
    };
}
