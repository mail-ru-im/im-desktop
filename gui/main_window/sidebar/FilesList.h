#pragma once
#include "GalleryList.h"
#include "SidebarUtils.h"
#include "../../types/message.h"
#include "types/filesharing_meta.h"
#include "types/filesharing_download_result.h"
#include "../../animation/animation.h"

namespace Ui
{
    class FileSharingIcon;

    namespace TextRendering
    {
        class TextUnit;
    }

    class BaseFileItem
    {
    public:
        virtual ~BaseFileItem() = default;

        virtual void draw(QPainter& _p, const QRect& _rect, double _progress) = 0;
        virtual int getHeight() const = 0;
        virtual void setWidth(int _width) {}
        virtual void setReqId(qint64 _id) {}
        virtual qint64 reqId() const { return -1; }

        virtual qint64 getMsg() const { return 0; }
        virtual qint64 getSeq() const { return 0; }
        virtual QString getLink() const { return QString(); }
        virtual QString sender() const { return QString(); }
        virtual time_t time() const { return 0; }
        virtual qint64 size() const { return 0; }
        virtual qint64 lastModified() const { return 0; }

        virtual void setFilename(const QString& _name) { }
        virtual void setFilesize(qint64 _size) { }
        virtual void setLastModified(qint64 _lastModified) { }

        virtual bool isOverControl(const QPoint& _pos) { return false; }
        virtual bool isOverLink(const QPoint& _pos, const QPoint& _pos2) { return false; }
        virtual bool isOverIcon(const QPoint & _pos) { return false; }

        virtual void setDownloading(bool _downloading) {}
        virtual bool isDownloading() const { return false; }
        virtual void setProgress(qint64 _transferred, qint64 _total) {}
        virtual void setLocalPath(const QString& _path) {}
        virtual QString getLocalPath() const { return QString(); }

        virtual bool isOverDate(const QPoint&) const { return false; }
        virtual void setDateState(bool _hover, bool _active) {}

        virtual bool isOutgoing() const { return false; }
        virtual void setMoreButtonState(const ButtonState& _state) {}
        virtual bool isOverMoreButton(const QPoint& _pos, int _h) const { return false; }
        virtual ButtonState moreButtonState() const { return ButtonState::NORMAL; }

        virtual bool isDateItem() const { return false; }
    };

    class DateFileItem : public BaseFileItem
    {
    public:
        DateFileItem(time_t _time, qint64 _msg, qint64 _seq);

        virtual void draw(QPainter& _p, const QRect& _rect, double _progress) override;
        virtual int getHeight() const override;

        virtual qint64 getMsg() const override;
        virtual qint64 getSeq() const override;
        virtual time_t time() const override;

        virtual bool isDateItem() const override { return true; }

    private:
        std::unique_ptr<Ui::TextRendering::TextUnit> date_;
        time_t time_;
        qint64 msg_;
        qint64 seq_;
    };

    class FileItem : public BaseFileItem
    {
    public:
        FileItem(const QString& _link, const QString& _date, qint64 _msg, qint64 _seq, bool _outgoing, const QString& _sender, time_t _time, int _width);

        virtual void draw(QPainter& _p, const QRect& _rect, double _progress) override;
        virtual int getHeight() const override;
        virtual void setWidth(int _width) override;
        virtual void setReqId(qint64 _id) override;
        virtual qint64 reqId() const override;

        virtual qint64 getMsg() const override;
        virtual qint64 getSeq() const override;
        virtual QString getLink() const override;
        virtual QString sender() const override;
        virtual time_t time() const override;
        virtual qint64 size() const override;
        virtual qint64 lastModified() const override;

        virtual void setFilename(const QString& _name) override;
        virtual void setFilesize(qint64 _size) override;
        virtual void setLastModified(qint64 _lastModified) override;

        virtual bool isOverControl(const QPoint& _pos) override;
        virtual bool isOverLink(const QPoint& _pos, const QPoint& _pos2) override;
        virtual bool isOverIcon(const QPoint & _pos) override;

        virtual void setDownloading(bool _downloading) override;
        virtual bool isDownloading() const override;
        virtual void setProgress(qint64 _transferred, qint64 _total) override;
        virtual void setLocalPath(const QString& _path) override;
        virtual QString getLocalPath() const override;

        virtual bool isOverDate(const QPoint&) const override;
        virtual void setDateState(bool _hover, bool _active) override;

        virtual bool isOutgoing() const override;
        virtual void setMoreButtonState(const ButtonState& _state) override;
        virtual bool isOverMoreButton(const QPoint& _pos, int _h) const override;
        virtual ButtonState moreButtonState() const override;

        virtual bool isDateItem() const override { return false; }

    private:
        QString link_;
        std::unique_ptr<TextRendering::TextUnit> name_;
        std::unique_ptr<TextRendering::TextUnit> size_;
        std::unique_ptr<TextRendering::TextUnit> date_;
        std::unique_ptr<TextRendering::TextUnit> friedly_;
        std::unique_ptr<TextRendering::TextUnit> showInFolder_;
        int height_;
        int width_;
        qint64 msg_;
        qint64 seq_;

        QPixmap fileType_;
        QString localPath_;
        QString filename_;
        QPoint pos_;
        bool downloading_;
        qint64 transferred_;
        qint64 total_;
        bool outgoing_;
        qint64 reqId_;
        QString sender_;
        time_t time_;
        qint64 sizet_;
        qint64 lastModified_;
        QPixmap more_;
        ButtonState moreState_;
    };

    class FilesList : public MediaContentWidget
    {
        Q_OBJECT
    public:
        FilesList(QWidget* _parent);

        virtual void initFor(const QString& _aimId) override;
        virtual void processItems(const QVector<Data::DialogGalleryEntry>& _entries) override;
        virtual void processUpdates(const QVector<Data::DialogGalleryEntry>& _entries) override;

    protected:
        virtual void paintEvent(QPaintEvent*) override;
        virtual void resizeEvent(QResizeEvent*) override;
        virtual void mousePressEvent(QMouseEvent* _event) override;
        virtual void mouseReleaseEvent(QMouseEvent* _event) override;
        virtual void mouseMoveEvent(QMouseEvent* _event) override;
        virtual void leaveEvent(QEvent* _event) override;
        virtual ItemData itemAt(const QPoint& _pos) override;

    private Q_SLOTS:
        void fileSharingFileMetainfoDownloaded(qint64, const Data::FileSharingMeta& _meta);
        void fileDownloading(qint64 _seq, QString _rawUri, qint64 _bytesTransferred, qint64 _bytesTotal);
        void fileDownloaded(qint64 _seq, const Data::FileSharingDownloadResult& _result);
        void fileError(qint64 _seq, const QString& _rawUri, qint32 _errorCode);

    private:
        void validateDates();

    private:
        std::vector<std::unique_ptr<BaseFileItem>> Items_;
        std::vector<qint64> RequestIds_;
        std::vector<qint64> Downloading_;
        anim::Animation anim_;
    };
}
