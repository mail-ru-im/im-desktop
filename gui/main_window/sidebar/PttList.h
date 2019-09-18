#pragma once
#include "GalleryList.h"
#include "SidebarUtils.h"
#include "../../types/message.h"
#include "types/filesharing_download_result.h"
#include "../../animation/animation.h"

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class BasePttItem
    {
    public:
        virtual ~BasePttItem() = default;

        virtual void draw(QPainter& _p, const QRect& _rect) = 0;
        virtual int getHeight() const = 0;
        virtual void setWidth(int _width) {}
        virtual void setReqId(qint64 _reqId) {}
        virtual qint64 reqId() const { return -1; }

        virtual qint64 getMsg() const { return 0; }
        virtual qint64 getSeq() const { return 0; }
        virtual QString getLink() const { return QString(); }
        virtual QString getLocalPath() const { return QString(); }
        virtual bool isPlaying() const { return false; }
        virtual bool hasText() const { return false; }
        virtual QString sender() const { return QString(); }
        virtual time_t time() const { return 0; }

        virtual void setProgress(int _progress) {}
        virtual void setPlaying(bool _playing) {}
        virtual void setLocalPath(const QString& _localPath) {}
        virtual void setPause(bool _pause) {}
        virtual void toggleTextVisible() {}
        virtual void setText(const QString& _text) {}

        virtual bool playClicked(const QPoint&) const { return false; }
        virtual bool pauseClicked(const QPoint&) const { return false; }
        virtual bool textClicked(const QPoint&) const { return false; }

        virtual void setPlayState(const ButtonState& _state) {}
        virtual void setTextState(const ButtonState& _state) {}

        virtual bool isOverDate(const QPoint&) const { return false; }
        virtual void setDateState(bool _hover, bool _active) {}

        virtual bool isOutgoing() const { return false; }

        virtual void setMoreButtonState(const ButtonState& _state) {}
        virtual bool isOverMoreButton(const QPoint& _pos, int _h) const { return false; }
        virtual ButtonState moreButtonState() const { return ButtonState::NORMAL; }

        virtual bool isDateItem() const { return false; }
    };

    class DatePttItem : public BasePttItem
    {
    public:
        DatePttItem(time_t _time, qint64 _msg, qint64 _seq);

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

    class PttItem : public BasePttItem
    {
    public:
        PttItem(const QString& _link, const QString& _date, qint64 _msg, qint64 _seq, int _width, const QString& _sender, bool _outgoing, time_t _time);

        virtual void draw(QPainter& _p, const QRect& _rect) override;
        virtual int getHeight() const override;
        virtual void setWidth(int _width) override;
        virtual void setReqId(qint64 _reqId) override;
        virtual qint64 reqId() const override;

        virtual qint64 getMsg() const override;
        virtual qint64 getSeq() const override;
        virtual QString getLink() const override;
        virtual QString getLocalPath() const override;
        virtual bool isPlaying() const override;
        virtual bool hasText() const override;
        virtual QString sender() const override;
        virtual time_t time() const override;

        virtual void setProgress(int _progress) override;
        virtual void setPlaying(bool _playing) override;
        virtual void setLocalPath(const QString& _localPath) override;
        virtual void setPause(bool _pause) override;
        virtual void toggleTextVisible() override;
        virtual void setText(const QString& _text) override;

        virtual bool playClicked(const QPoint&) const override;
        virtual bool pauseClicked(const QPoint&) const override;
        virtual bool textClicked(const QPoint&) const override;

        virtual void setPlayState(const ButtonState& _state) override;
        virtual void setTextState(const ButtonState& _state) override;

        virtual bool isOverDate(const QPoint&) const override;
        virtual void setDateState(bool _hover, bool _active) override;

        virtual bool isOutgoing() const override;

        virtual void setMoreButtonState(const ButtonState& _state) override;
        virtual bool isOverMoreButton(const QPoint& _pos, int _h) const override;
        virtual ButtonState moreButtonState() const override;

        virtual bool isDateItem() const override { return false; }

    private:
        QString link_;
        std::unique_ptr<Ui::TextRendering::TextUnit> name_;
        std::unique_ptr<Ui::TextRendering::TextUnit> time_;
        std::unique_ptr<Ui::TextRendering::TextUnit> date_;
        std::unique_ptr<Ui::TextRendering::TextUnit> textControl_;
        int height_;
        int width_;
        qint64 msg_;
        qint64 seq_;
        int progress_;
        bool playing_;
        bool pause_;
        bool textVisible_;
        ButtonState playState_;
        ButtonState textState_;

        QString localPath_;
        QString text_;
        QPixmap playIcon_;
        QPixmap textIcon_;
        bool outgoing_;
        qint64 reqId_;
        QString sender_;
        time_t timet_;

        QPixmap more_;
        ButtonState moreState_;
    };

    class PttList : public MediaContentWidget
    {
        Q_OBJECT
    public:
        PttList(QWidget* _parent);

        virtual void initFor(const QString& _aimId) override;
        virtual void processItems(const QVector<Data::DialogGalleryEntry>& _entries) override;
        virtual void processUpdates(const QVector<Data::DialogGalleryEntry>& _entries) override;
        virtual void markClosed() override;

    protected:
        virtual void mousePressEvent(QMouseEvent* _event) override;
        virtual void mouseReleaseEvent(QMouseEvent* _event) override;
        virtual void mouseMoveEvent(QMouseEvent* _event) override;
        virtual void resizeEvent(QResizeEvent* _event) override;
        virtual void paintEvent(QPaintEvent*) override;
        virtual void leaveEvent(QEvent* _event) override;

        virtual ItemData itemAt(const QPoint& _pos) override;

    private Q_SLOTS:
        void onFileDownloaded(qint64, const Data::FileSharingDownloadResult& _result);
        void onPttText(qint64 _seq, int _error, QString _text, int _comeback);
        void finishPtt(int);

    private:
        void play(std::unique_ptr<BasePttItem>& _item);
        void invalidateHeight();
        void validateDates();

        void onPttFinished(int _id, bool _byPlay);
        void onPttPaused(int _id);

    private:
        std::vector<std::unique_ptr<BasePttItem>> Items_;
        std::vector<qint64> RequestIds_;
        QPoint pos_;
        int playingId_;
        std::pair<qint64, qint64> playingIndex_;
        int lastProgress_;
        anim::Animation animation_;
    };
}
