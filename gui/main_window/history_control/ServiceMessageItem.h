#pragma once

#include "MessageItemBase.h"

namespace Ui
{
    class service_message_item;

    namespace TextRendering
    {
        class TextUnit;
    }

    class ServiceMessageItem : public MessageItemBase
    {
        Q_OBJECT

    public:
        ServiceMessageItem(QWidget* _parent, const bool _overlay = false);
        ~ServiceMessageItem();

        QString formatRecentsText() const override;
        MediaType getMediaType() const override;

        void setMessage(const QString& _message);
        void setWidth(const int _width);
        void setDate(const QDate& _date);

        void setNew();
        bool isNew() const;

        bool isFloating() const;

        void updateStyle() override;
        void updateFonts() override;

        bool isOutgoing() const override { return false; }
        int32_t getTime() const override { return -1; }

    protected:
        void setQuoteSelection() override;
        void paintEvent(QPaintEvent* _event) override;

    private:
        void updateDateTranslation();

        void drawDate(QPainter& _p);
        void drawPlateNew(QPainter& _p);
        void drawOverlay(QPainter& _p);

        QFont getFont();
        QColor getColor();

    private:
        enum type
        {
            typeDate = 0,
            typePlateNew = 1,
            typeOverlay = 2

        } type_;

        bool isFloating_;
        QDate lastUpdated_;
        QDate date_;
        QString message_;
        std::unique_ptr<TextRendering::TextUnit> messageUnit_;

        QPainterPath bubble_;
        QRect bubbleGeometry_;
    };
}
