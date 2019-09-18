#pragma once

#include <QWidget>
#include <QVector>
#include <QSize>

namespace Ui
{

class HorizontalImgList: public QWidget
{
    Q_OBJECT

public:
    using ImageType = QPixmap;

    struct ImageInfo
    {
        ImageInfo(const QSize& _size = QSize(), const ImageType& _defImg = ImageType(), const ImageType& _highlightedImg = ImageType())
            : defaultImage_(_defImg),
              highlightedImage_(_highlightedImg),
              size_(_size)
        {

        }

        ImageInfo(const ImageInfo &_info) = default;

        const ImageType defaultImage_;
        const ImageType highlightedImage_;
        QSize size_;
    };

    struct Options
    {
        bool highlightOnHover_ = true;
        int  hlOnHoverHigherThan_ = 0;

        bool rememberClickedHighlight_ = true;
        int hlClicked_ = -1;
    };

public:
    explicit HorizontalImgList(const Options& _options, QWidget *_parent = nullptr);

    void setIdenticalItems(const ImageInfo& _info, int _count);
    void setItemOffsets(const QMargins& _margins);

    int itemsCount() const;

    virtual QSize sizeHint() const override;

protected:
    virtual void paintEvent(QPaintEvent *_event) override;
    virtual void mouseMoveEvent(QMouseEvent *_event) override;
    virtual void mouseReleaseEvent(QMouseEvent *_event) override;
    virtual void leaveEvent(QEvent *) override;

Q_SIGNALS:
    void itemHovered(int _index);
    void itemClicked(int _index);
    void mouseLeft();

private Q_SLOTS:
    void onItemHovered(int _index);
    void onItemClicked(int _index);

private:
    int itemIndexForXPos(int _x);
    void highlightUserSelection(int _index);

private:
    struct ImageState
    {
        bool highlighted_ = false;

        int currentXPos_ = -1;
        int currentWidth_ = -1;
        int currentRightOffset_ = 0;

        bool hasX(int _x) const;
    };

private:
    QVector<ImageInfo> items_;
    QVector<ImageState> states_;
    QMargins itemOffsets_;
    Options options_;
};

}
