#pragma once

#include "GalleryList.h"
#include "types/images.h"
#include "types/link_metadata.h"
#include "previewer/Drawable.h"
#include "utils/medialoader.h"
#include <memory>

namespace Ui
{

//////////////////////////////////////////////////////////////////////////

class ImageVideoBlock;

class ImageVideoItemVisitor;

class ImageVideoList : public MediaContentWidget
{
    Q_OBJECT

public:
    ImageVideoList(QWidget* _parent, MediaContentWidget::Type _type, const std::string& _mediaType);
    ~ImageVideoList();

    void processItems(const QVector<Data::DialogGalleryEntry>& _entries) override;
    virtual void processUpdates(const QVector<Data::DialogGalleryEntry>& _entries) override;
    void clear();
    void initFor(const QString& _aimId) override;

    void scrolled() override;
    void accept(ImageVideoItemVisitor* _visitor);

    std::vector<std::unique_ptr<ImageVideoBlock>> & blocks() { return blocks_; }

    int topViewOffset() { return visibleRegion().boundingRect().height() / 4; }

    int bottomViewOffset() { return visibleRegion().boundingRect().height() / 2; }

signals:

    void runVisitor(ImageVideoList* _list);

protected:
    void paintEvent(QPaintEvent* _event) override;
    void resizeEvent(QResizeEvent* _event) override;
    void mouseMoveEvent(QMouseEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

    virtual void mousePressEvent(QMouseEvent* _event) override;
    virtual void mouseReleaseEvent(QMouseEvent* _event) override;

    ItemData itemAt(const QPoint &_pos) override;
    void onClicked(ItemData _data) override;

private Q_SLOTS:
    void onVisitorTimeout();

private:
    void addBlock(ImageVideoBlock* block);
    void updateHeight(bool _force = false);


    std::vector<std::unique_ptr<ImageVideoBlock>> blocks_;

    ImageVideoItemVisitor* visitor_;

    QTimer visitorTimer_;
    QTimer updateTimer_;
    QRegion updateRegion_;
    std::string mediaType_;
};


//////////////////////////////////////////////////////////////////////////

class ImageVideoItem;

class ImageVideoBlock : public QObject, public Drawable
{
    Q_OBJECT

public:
    ImageVideoBlock(QObject* _parent, const QString& _aimid);
    ~ImageVideoBlock();
    void setDate(const QDate& _date);
    QDate date();
    void addItem(const QString& _link, qint64 _msg, qint64 _seq, bool _outgoing, const QString& _sender, time_t _time);
    void removeItem(qint64 _msg, qint64 _seq);
    void removeItemByMsg(qint64 _msg);
    int count();

    int getHeight(int _width, bool _force = false);
    void setRect(const QRect& _rect) override;

    void drawInRect(QPainter &_p, const QRect& _rect);

    MediaContentWidget::ItemData itemAt(const QPoint& _pos);

    void onMousePress(const QPoint& _pos, const std::string& _mediaTYpe);
    void onMouseRelease(const QPoint& _pos);
    bool onMouseMove(const QPoint& _pos);
    void clearHovered();

    void accept(ImageVideoItemVisitor* _visitor);

    enum class ViewportChangeDirection
    {
        None,
        Up,   // viewport moved up
        Down  // viewport moved down
    };

    void unload(const QRect& _rect, ViewportChangeDirection _direction);
    void load(QRect _rect, ViewportChangeDirection _direction);

signals:
    void needUpdate(const QRect& _rect);

private:
    QRect itemRect(int _index);
    QRect dateRect();
    void updateItemsPositions();
    int calcItemSize(int _width);
    int calcRowSize(int _width);
    int calcRowSizeHelper(int _width, int _itemSize);
    int calcHeight();
    int hMargin();
    int vMargin();

    int spacing();

    std::unique_ptr<Label> dateLabel_;
    QDate date_;
    std::vector<std::unique_ptr<ImageVideoItem>> items_;
    int itemSize_;
    int cachedHeight_;
    QString aimid_;

    bool heightValid_ = false;
    bool positionsValid_ = false;
};

struct Preview
{
    QPixmap source_;
    QPixmap scaled_;
};

enum class LoadType {
    Immediate,
    Postponed
};

//////////////////////////////////////////////////////////////////////////

class ImageVideoItem : public QObject, public Drawable
{
    Q_OBJECT

public:
    ImageVideoItem(QObject* _parent, const QString& _link, qint64 _msg, qint64 _seq, bool _outgoing, const QString& _sender, time_t _time);
    ~ImageVideoItem();
    void load(LoadType _loadType = LoadType::Postponed);
    void unload();

    void setRect(const QRect& _rect) override { rect_ = _rect; }

    void draw(QPainter &_p) override;

    QString getLink() const { return link_; }
    qint64 getMsg() const { return msg_; }
    qint64 getSeq() const { return seq_; }
    bool isOutgoing() const { return outgoing_; }
    QString sender() const { return sender_; }
    time_t time() const { return time_; }

    bool isVideo() const { return loader_ && loader_->isVideo(); }
    bool isGif() const { return loader_ && loader_->isGif(); }

    void cancelLoading();

signals:
    void needUpdate(const QRect& _rect);

private Q_SLOTS:
    void onPreviewLoaded(const QPixmap& _preview);

private:
    static QPixmap cropPreview(const QPixmap& _source, const int _toSize);
    void updatePreview();
    void updateDurationLabel();

    std::unique_ptr<Utils::MediaLoader> loader_;
    qint64 msg_;
    qint64 seq_;

    QString link_;
    bool filesharing_;
    QSize cachedSize_;
    TextRendering::TextUnitPtr durationLabel_;
    bool outgoing_;
    QString sender_;
    time_t time_;

    std::unique_ptr<Preview> previewPtr_;

    QTimer loadDelayTimer_;
};

//////////////////////////////////////////////////////////////////////////

class ImageVideoItemVisitor : public QObject
{
    Q_OBJECT
public:
    ImageVideoItemVisitor(QObject* _parent = nullptr);

    void visit(ImageVideoList* _list);
    void visit(ImageVideoBlock* _block);

    void setForceLoad() { forceLoad_ = true; }

    void reset();

private:
    QRect visibleRect_;
    QRect prevVisibleRect_;

    int height_;
    int prevHeight_;

    bool forceLoad_;
};

//////////////////////////////////////////////////////////////////////////

class ResetPreviewTask: public QObject, public QRunnable
{
    Q_OBJECT
public:
    ResetPreviewTask(std::unique_ptr<Preview> _preview) : preview_(std::move(_preview)) { setAutoDelete(true); }

    void run();
private:
    std::unique_ptr<Preview> preview_;
};

}
