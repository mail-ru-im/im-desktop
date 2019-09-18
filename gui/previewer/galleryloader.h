#pragma once

#include "contentloader.h"

#include "../types/chat.h"

namespace Utils {

class MediaLoader;

}

namespace Ui
{
    class DialogPlayer;
}

namespace Previewer
{

class GalleryItem;

class GalleryLoader : public ContentLoader
{
    Q_OBJECT
public:
    GalleryLoader(const QString& _aimId, const QString& _link, int64_t _msgId, Ui::DialogPlayer* _attachedPlayer = nullptr);

    void next() override;
    void prev() override ;

    bool hasNext() override;
    bool hasPrev() override;

    ContentItem* currentItem() override;
    virtual int currentIndex() const override;
    virtual int totalCount() const override;

    void cancelCurrentItemLoading() override;
    void startCurrentItemLoading() override;

    virtual bool navigationSupported() override { return true; }

    void clear();

public Q_SLOTS:
    void onItemLoaded(const QString &_link, int64_t _msgId);
    void onPreviewLoaded(const QString &_link, int64_t _msgId);
    void onItemError(const QString &_link, int64_t _msgId);
    void dialogGalleryResult(const int64_t _seq, const QVector<Data::DialogGalleryEntry>& _entries, bool _exhausted);
    void dialogGalleryResultByMsg(const int64_t _seq, const QVector<Data::DialogGalleryEntry>& _entries, int _index, int _total);
    void dialogGalleryState(const QString& _aimId, const Data::DialogGalleryState& _state);
    void dialogGalleryUpdate(const QString& _aimId, const QVector<Data::DialogGalleryEntry>& _entries);
    void dialogGalleryInit(const QString& _aimId);
    void dialogGalleryHoles(const QString& _aimId);
    void dialogGalleryIndex(const QString& _aimId, qint64 _msg, qint64 _seq, int _index, int _total);

private:

    enum class Direction { Previous, Next};

    void move(Direction _direction);

    void loadMore();

    int64_t loadItems(int64_t _after, int64_t _seq, int _count);

    void unload();

    size_t loadDistance() { return 10; }

    std::unique_ptr<GalleryItem> createItem(const QString& _link, qint64 _msg, qint64 _seq, time_t _time, const QString& _sender, const QString& _caption, Ui::DialogPlayer* _attachedPlayer = nullptr);

    bool indexValid();

    std::deque<std::unique_ptr<GalleryItem>> items_;
    QString aimId_;
    size_t current_;
    int64_t firstItemSeq_;
    int64_t frontLoadSeq_;
    int64_t backLoadSeq_;
    QString initialLink_;

    bool exhaustedFront_;
    bool exhaustedBack_;
    int index_;
    int total_;
};

class GalleryItem : public ContentItem
{
    Q_OBJECT
public:
    GalleryItem(const QString& _link, qint64 _msg, qint64 _seq, time_t _time, const QString& _sender, const QString& _aimid, const QString& _caption, Ui::DialogPlayer* _attachedPlayer = nullptr);

    ~GalleryItem();

    qint64 msg() override { return msg_; }
    qint64 seq() override { return seq_; }

    void load();

    void setMsg(qint64 _msg) { msg_ = _msg; }
    void setSeq(qint64 _seq) { seq_ = _seq; }
    void setTime(time_t _time) { time_ = _time; }
    void setSender(const QString& _sender) { sender_ = _sender; }
    void setCaption(const QString& _caption) { caption_ = _caption; }
    void setPath(const QString& _path) { path_ = _path; }

    QString link() const override { return link_; }
    QString path() const override { return path_; }

    QString fileName() const override;

    QPixmap preview() const override { return preview_; }
    QSize originSize() const override { return originSize_; }

    bool isVideo() const override;

    void loadFullMedia();
    void cancelLoading();

    void showMedia(ImageViewerWidget* _viewer) override;
    void showPreview(ImageViewerWidget* _viewer) override;

    void save(const QString& _path) override;

    void copyToClipboard() override;

    virtual time_t time() const override { return time_; }
    virtual QString sender() const override { return sender_; }
    virtual QString caption() const override { return caption_; }
    virtual QString aimId() const override { return aimid_; }

Q_SIGNALS:
    void loaded(const QString& _link, int64_t _msgId);
    void previewLoaded(const QString& _link, int64_t _msgId);
    void error(const QString& _link, int64_t _msgId);

private Q_SLOTS:
    void onFileLoaded(const QString& _path);
    void onPreviewLoaded(const QPixmap& _preview, const QSize& _originSize);
    void onItemError();

private:

    QString link_;
    QPixmap preview_;
    QString path_;
    QSize originSize_;
    std::unique_ptr<Utils::MediaLoader> loader_;
    qint64 msg_;
    qint64 seq_;
    time_t time_;
    QString sender_;
    QPointer<Ui::DialogPlayer> attachedPlayer_;
    bool attachedPlayerShown_;
    QString aimid_;
    QString caption_;
};


}