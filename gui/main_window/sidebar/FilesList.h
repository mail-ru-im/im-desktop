#pragma once
#include "GalleryList.h"
#include "SidebarUtils.h"
#include "../../types/message.h"
#include "types/filesharing_meta.h"
#include "types/filesharing_download_result.h"
#include "../history_control/FileStatus.h"
#include "../history_control/complex_message/FileSharingUtils.h"

namespace Ui
{
    class FileSharingIcon;

    namespace TextRendering
    {
        class TextUnit;
    }

    class BaseFileItem : public SidebarListItem
    {
    public:
        BaseFileItem(QObject* _parent = nullptr) : SidebarListItem(_parent) {}
        virtual ~BaseFileItem() = default;

        virtual void draw(QPainter& _p, const QRect& _rect, double _progress) = 0;
        virtual int getHeight() const = 0;
        virtual void setWidth(int _width) {}
        virtual void setReqId(qint64 _id) {}
        virtual qint64 reqId() const { return -1; }

        virtual qint64 getMsg() const { return 0; }
        virtual qint64 getSeq() const { return 0; }
        virtual QString getLink() const { return QString(); }
        virtual QString getFilename() const { return QString(); }
        virtual QString sender() const { return QString(); }
        virtual time_t time() const { return 0; }
        virtual qint64 size() const { return 0; }
        virtual qint64 lastModified() const { return 0; }
        virtual const Utils::FileSharingId& getFileSharingId() const;
        virtual bool isAntivirusAllowed() const { return true; }

        virtual void setFilename(const QString& _name) {}
        virtual void setFilesize(qint64 _size) {}
        virtual void setLastModified(qint64 _lastModified) {}
        virtual void setMetaInfo(const Data::FileSharingMeta& _meta) {}

        virtual bool isOverControl(const QPoint& _pos) { return false; }
        virtual bool isOverLink(const QPoint& _pos) { return false; }
        virtual bool isOverIcon(const QPoint& _pos) { return false; }
        virtual bool isOverFilename(const QPoint& _pos) const { return false; }
        virtual bool isOverStatus(const QPoint& _pos) const { return false; }

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

        virtual bool needsTooltip() const { return false; }
        virtual QRect getTooltipRect() const { return QRect(); }

        virtual FileStatus& getFileStatus() { static FileStatus fs; return fs; }

        virtual QRect getMoreButtonRect() const { return {}; }
        virtual QRect getStatusRect() const { return {}; }
    };

    class DateFileItem : public BaseFileItem
    {
    public:
        DateFileItem(time_t _time, qint64 _msg, qint64 _seq);

        void draw(QPainter& _p, const QRect& _rect, double _progress) override;
        int getHeight() const override;

        qint64 getMsg() const override;
        qint64 getSeq() const override;
        time_t time() const override;

        bool isDateItem() const override { return true; }

    private:
        std::unique_ptr<Ui::TextRendering::TextUnit> date_;
        time_t time_;
        qint64 msg_;
        qint64 seq_;
    };

    class FileItem : public BaseFileItem
    {
        Q_OBJECT
    public:
        FileItem(const QString& _link, const QString& _date, qint64 _msg, qint64 _seq, bool _outgoing, const QString& _sender, time_t _time, int _width, QWidget* _parent);

        void draw(QPainter& _p, const QRect& _rect, double _progress) override;
        int getHeight() const override;
        void setWidth(int _width) override;
        void setReqId(qint64 _id) override;
        qint64 reqId() const override;

        qint64 getMsg() const override;
        qint64 getSeq() const override;
        QString getLink() const override;
        QString getFilename() const override;
        QString sender() const override;
        time_t time() const override;
        qint64 size() const override;
        qint64 lastModified() const override;
        const Utils::FileSharingId& getFileSharingId() const override;
        bool isAntivirusAllowed() const override;

        void setFilename(const QString& _name) override;
        void setFilesize(qint64 _size) override;
        void setLastModified(qint64 _lastModified) override;

        void setMetaInfo(const Data::FileSharingMeta& _meta) override;

        bool isOverControl(const QPoint& _pos) override;
        bool isOverLink(const QPoint& _pos) override;
        bool isOverIcon(const QPoint& _pos) override;
        bool isOverFilename(const QPoint& _pos) const override;
        bool isOverStatus(const QPoint& _pos) const override;

        void setDownloading(bool _downloading) override;
        bool isDownloading() const override;
        void setProgress(qint64 _transferred, qint64 _total) override;
        void setLocalPath(const QString& _path) override;
        QString getLocalPath() const override;

        bool isOverDate(const QPoint&) const override;
        void setDateState(bool _hover, bool _active) override;

        bool isOutgoing() const override;
        void setMoreButtonState(const ButtonState& _state) override;
        bool isOverMoreButton(const QPoint& _pos, int _h) const override;
        ButtonState moreButtonState() const override;

        bool isDateItem() const override { return false; }

        bool needsTooltip() const override;
        QRect getTooltipRect() const override;

        FileStatus& getFileStatus() override { return status_; }

        QRect getMoreButtonRect() const override;
        QRect getStatusRect() const override;

        void processMetaInfo(const Utils::FileSharingId& _fsId, const Data::FileSharingMeta& _meta);
        void processMetaInfoError(const Utils::FileSharingId& _fsId, qint32 _errorCode) {}
    private Q_SLOTS:
        void onAntivirusCheckResult(const Utils::FileSharingId& _fsId, core::antivirus::check::result _result);

    private:
        void elide(const std::unique_ptr<TextRendering::TextUnit>& _unit, int _addedWidth = 0);

    private:
        QString link_;
        Utils::FileSharingId fileSharingId_;
        std::unique_ptr<TextRendering::TextUnit> name_;
        std::unique_ptr<TextRendering::TextUnit> size_;
        std::unique_ptr<TextRendering::TextUnit> date_;
        std::unique_ptr<TextRendering::TextUnit> friendly_;
        std::unique_ptr<TextRendering::TextUnit> showInFolder_;
        int height_ = 0;
        int width_;
        qint64 msg_;
        qint64 seq_;

        QPixmap fileType_;
        QString localPath_;
        QString filename_;
        QPoint pos_;
        bool downloading_ = false;
        qint64 transferred_ = 0;
        qint64 total_ = 0;
        bool outgoing_;
        qint64 reqId_ = -1;
        QString sender_;
        time_t time_;
        qint64 sizet_ = 0;
        qint64 lastModified_ = 0;
        ButtonState moreState_;
        FileStatus status_;
        QPointer<QWidget> parent_;
    };

    class FilesList : public MediaContentWidget
    {
        Q_OBJECT
    public:
        FilesList(QWidget* _parent);

        void initFor(const QString& _aimId) override;
        void processItems(const QVector<Data::DialogGalleryEntry>& _entries) override;
        void processUpdates(const QVector<Data::DialogGalleryEntry>& _entries) override;
        void scrolled() override;

    protected:
        void paintEvent(QPaintEvent*) override;
        void resizeEvent(QResizeEvent*) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void leaveEvent(QEvent* _event) override;
        ItemData itemAt(const QPoint& _pos) override;

    private Q_SLOTS:
        void fileDownloading(qint64 _seq, QString _rawUri, qint64 _bytesTransferred, qint64 _bytesTotal);
        void fileDownloaded(qint64 _seq, const Data::FileSharingDownloadResult& _result);
        void fileError(qint64 _seq, const QString& _rawUri, qint32 _errorCode);

    private:
        void validateDates();
        void updateTooltip(const std::shared_ptr<BaseFileItem>& _item, const QPoint& _pos, int _h);

        void startDataTransferTimeoutTimer(qint64 _seq);
        void stopDataTransferTimeoutTimer(qint64 _seq);

    private:
        std::vector<std::shared_ptr<BaseFileItem>> Items_;
        std::vector<qint64> Downloading_;
        QVariantAnimation* anim_;
        std::vector<std::pair<qint64, QTimer*>> dataTransferTimeoutList_;
    };
} // namespace Ui
