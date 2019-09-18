#pragma once

#include "MessageItemBase.h"

namespace HistoryControl
{
    typedef std::shared_ptr<class ChatEventInfo> ChatEventInfoSptr;
}

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class ChatEventItem : public MessageItemBase
    {
        Q_OBJECT

    public:
        ChatEventItem(const ::HistoryControl::ChatEventInfoSptr& _eventInfo, const qint64 _id, const qint64 _prevId);

        ChatEventItem(QWidget* _parent, const ::HistoryControl::ChatEventInfoSptr& eventInfo, const qint64 _id, const qint64 _prevId);

        ~ChatEventItem();

        void clearSelection() override;

        QString formatRecentsText() const override;

        MediaType getMediaType() const override;

        QSize sizeHint() const override;

        void setLastStatus(LastStatus _lastStatus) override;

        qint64 getId() const override;
        qint64 getPrevId() const override;

        void setQuoteSelection() override;

        void updateStyle() override;
        void updateFonts() override;

        void updateSize() override;

        bool isOutgoing() const override;
        int32_t getTime() const override;

        int bottomOffset() const override;

    private:

        QRect BubbleRect_;

        std::unique_ptr<TextRendering::TextUnit> TextWidget_;

        const ::HistoryControl::ChatEventInfoSptr EventInfo_;

        int height_;

        qint64 id_;
        qint64 prevId_;

        QPoint pressPos_;

        int32_t evaluateTextWidth(const int32_t _widgetWidth);

        virtual void paintEvent(QPaintEvent* _event) override;

        virtual void resizeEvent(QResizeEvent* _event) override;

        virtual void mousePressEvent(QMouseEvent* _event) override;

        virtual void mouseMoveEvent(QMouseEvent* _event) override;

        virtual void mouseReleaseEvent(QMouseEvent* _event) override;

        void updateSize(const QSize& _size);
    };

}
