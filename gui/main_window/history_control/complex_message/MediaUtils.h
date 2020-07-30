#pragma once

#include "../../../namespaces.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace MediaUtils
{

    enum class MediaType
    {
        Image,
        Video,
        Gif,
        Sticker  // gif stickers are stickers too
    };

    enum class BackgroundMode
    {
        Auto,
        NoBackground
    };

    QSize getTargetSize(const QRect& _rect, const QPixmap& _pixmap, const QSize& _originalSize);
    void drawMediaInRect(QPainter& _p, const QRect& _rect, const QPixmap& _pixmap, const QSize& _originalSize, BackgroundMode _backgroundMode = BackgroundMode::Auto);

    struct MediaBlockParams
    {
        QSize mediaSize;
        int availableWidth;
        MediaType mediaType;
        int minBlockWidth = 0;
        bool isInsideQuote = false;
    };

    QSize calcMediaBlockSize(const MediaBlockParams& _params);

    QPixmap scaleMediaBlockPreview(const QPixmap& _pixmap);

    int mediaBlockMaxWidth(const QSize& _originalSize);

    bool isSmallMedia(const QSize& _mediaSize);
    bool isNarrowMedia(const QSize& _mediaSize);
    bool isWideMedia(const QSize& _mediaSize);

    bool isMediaSmallerThanBlock(const QSize& _blockSize, const QSize& _originalSize);
}

UI_COMPLEX_MESSAGE_NS_END
