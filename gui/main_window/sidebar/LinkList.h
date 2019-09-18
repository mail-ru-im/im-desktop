#pragma once
#include "GalleryList.h"
#include "SidebarUtils.h"
#include "../../types/message.h"
#include "../../types/link_metadata.h"

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class BaseLinkItem
    {
    public:
        virtual ~BaseLinkItem() = default;

        virtual void draw(QPainter& _p, const QRect& _rect) = 0;
        virtual int getHeight() const = 0;
        virtual void setWidth(int _width) {}
        virtual void setReqId(qint64 _id) {}
        virtual qint64 reqId() const { return -1; }

        virtual QString getLink() const { return QString(); }
        virtual qint64 getMsg() const { return 0; }
        virtual qint64 getSeq() const { return 0; }
        virtual QString sender() const { return QString(); }
        virtual time_t time() const { return 0; }

        virtual void setPreview(const QPixmap&) { }
        virtual bool isOverLink(const QPoint&, int) const { return false; }
        virtual bool isOverLink(const QPoint&) const { return false; }
        virtual void underline(bool _enabled) { }
        virtual bool isOverDate(const QPoint&) const { return false; }
        virtual void setDateState(bool _hover, bool _active) {}
        virtual void setTitle(const QString& _title) {}
        virtual void setDesc(const QString& _desc) {}
        virtual bool isOutgoing() const { return false; }

        virtual bool loaded() const { return false; }
        virtual void setMoreButtonState(const ButtonState& _state) {}
        virtual bool isOverMoreButton(const QPoint& _pos, int _h) const { return false; }
        virtual ButtonState moreButtonState() const { return ButtonState::NORMAL; }

        virtual bool isDateItem() const { return false; }
    };

    class DateLinkItem : public BaseLinkItem
    {
    public:
        DateLinkItem(time_t _time, qint64 _msg, qint64 _seq);

        virtual qint64 getMsg() const override;
        virtual qint64 getSeq() const override;
        virtual time_t time() const override;

        virtual void draw(QPainter& _p, const QRect& _rect) override;
        virtual int getHeight() const override;

        virtual bool isDateItem() const override { return true; }
    private:
        std::unique_ptr<Ui::TextRendering::TextUnit> date_;
        time_t time_;
        qint64 msg_;
        qint64 seq_;
    };

    class LinkItem : public BaseLinkItem
    {
    public:
        LinkItem(const QString& _url, const QString& _date, qint64 _msg, qint64 _seq, bool _outgoing, const QString& _sender, time_t _time, int _width);

        virtual void draw(QPainter& _p, const QRect& _rect) override;
        virtual int getHeight() const override;
        virtual void setWidth(int _width) override;
        virtual void setReqId(qint64 _id) override;
        virtual qint64 reqId() const override;

        virtual QString getLink() const override;
        virtual qint64 getMsg() const override;
        virtual qint64 getSeq() const override;
        virtual QString sender() const override;
        virtual time_t time() const override;

        virtual void setPreview(const QPixmap& _preview) override;

        virtual bool isOverLink(const QPoint& _pos, int _totalHeight) const override;
        virtual bool isOverLink(const QPoint& _pos) const override;
        virtual void underline(bool _enabled) override;

        virtual bool isOverDate(const QPoint& _pos) const override;
        virtual void setDateState(bool _hover, bool _active) override;

        virtual void setTitle(const QString& _title) override;
        virtual void setDesc(const QString& _desc) override;

        virtual bool isOutgoing() const override;

        virtual bool loaded() const override;
        virtual void setMoreButtonState(const ButtonState& _state) override;
        virtual bool isOverMoreButton(const QPoint& _pos, int _h) const override;
        virtual ButtonState moreButtonState() const override;

        virtual bool isDateItem() const override { return false; }

    private:
        QString url_;
        std::unique_ptr<Ui::TextRendering::TextUnit> title_;
        std::unique_ptr<Ui::TextRendering::TextUnit> desc_;
        std::unique_ptr<Ui::TextRendering::TextUnit> link_;
        std::unique_ptr<Ui::TextRendering::TextUnit> friedly_;
        std::unique_ptr<Ui::TextRendering::TextUnit> date_;
        QPixmap preview_;
        int height_;
        int width_;
        qint64 msg_;
        qint64 seq_;
        bool outgoing_;
        qint64 reqId_;
        QString sender_;
        time_t time_;
        bool loaded_;
        QPixmap more_;
        ButtonState moreState_;
    };

    class LinkList : public MediaContentWidget
    {
        Q_OBJECT
    public:
        LinkList(QWidget* _parent);

        virtual void initFor(const QString& _aimId) override;
        virtual void processItems(const QVector<Data::DialogGalleryEntry>& _entries) override;
        virtual void processUpdates(const QVector<Data::DialogGalleryEntry>& _entries) override;
        virtual void scrolled() override;

    protected:
        virtual void paintEvent(QPaintEvent*) override;
        virtual ItemData itemAt(const QPoint& _pos) override;
        virtual void resizeEvent(QResizeEvent* _event) override;
        virtual void mouseMoveEvent(QMouseEvent* _event) override;
        virtual void mousePressEvent(QMouseEvent* _event) override;
        virtual void mouseReleaseEvent(QMouseEvent* _event) override;
        virtual void leaveEvent(QEvent* _event) override;

    private Q_SLOTS:
        void onImageDownloaded(int64_t _seq, bool _success, const QPixmap& _image);
        void onMetaDownloaded(int64_t _seq, bool _success, const Data::LinkMetadata& _meta);
        void onTimer();

    private:
        void validateDates();

    private:
        std::vector<std::unique_ptr<BaseLinkItem>> Items_;
        std::vector<int> RequestIds_;
        QPoint pos_;
        std::vector<QString> RequestedUrls_;
        QTimer* timer_;
    };
}
