#include "stdafx.h"

#include "MediaUtils.h"
#include "styles/ThemeParameters.h"
#include "../MessageStyle.h"
#include "utils/utils.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    int maxMediaBlockDimenson(MediaUtils::MediaType _mediaType)
    {
        switch (_mediaType)
        {
            case MediaUtils::MediaType::Image:
            case MediaUtils::MediaType::Gif:
            case MediaUtils::MediaType::Video:
                return MessageStyle::Preview::getImageWidthMax();

            case MediaUtils::MediaType::Sticker:
                return MessageStyle::Preview::getStickerMaxWidth();

            default:
                return 0;
        }
    }

    int mediaInsideQuoteDimension()
    {
        return Utils::scale_value(112);
    }

    int smallMediaBlockDimension(MediaUtils::MediaType _mediaType)
    {
        if (_mediaType == MediaUtils::MediaType::Video)
            return Utils::scale_value(154);
        else if (_mediaType == MediaUtils::MediaType::Sticker)
            return Utils::scale_value(56);
        else
            return Utils::scale_value(120);
    }

    int minMediaBlockWidth()
    {
        return Utils::scale_value(154);
    }

    int minMediaBlockHeight()
    {
        return Utils::scale_value(124);
    }

    int wideMediaHeight(int _availableWidth)
    {
        static const auto wideMediaHeightFor292 = minMediaBlockHeight();
        static const auto wideMediaHeightFor360 = Utils::scale_value(154);
        static const auto wideMediaWidthStart = Utils::scale_value(292);
        static const auto wideMediaWidthEnd = MessageStyle::Preview::getImageWidthMax();
        static const auto widthDiff = wideMediaWidthEnd - wideMediaWidthStart;
        static const auto heightDiff = wideMediaHeightFor360 - wideMediaHeightFor292;

        return wideMediaHeightFor292 + std::min(widthDiff, std::max(_availableWidth - wideMediaWidthStart, 0)) / static_cast<double>(widthDiff) * heightDiff;
    }

    int mediaCropThreshold() // if one of media dimensions is less than crop threshold it will be cropped
    {
        return MessageStyle::Preview::mediaCropThreshold();
    }

    QRect mediaSourceRect(const QSize& _mediaSize, const QSize& targetSize, const QSize& _originalSize)
    {
        const auto scaledSize = _mediaSize.scaled(targetSize, Qt::KeepAspectRatio);
        const auto needsCrop = !MediaUtils::isSmallMedia(_originalSize) && std::min(scaledSize.width(), scaledSize.height()) < mediaCropThreshold();

        if (!needsCrop)
            return QRect(QPoint(0,0), _mediaSize);

        auto horizontal = _mediaSize.width() > _mediaSize.height();

        const auto scaledWidth = horizontal ? targetSize.width() : mediaCropThreshold();
        const auto scaledHeight = horizontal ? mediaCropThreshold() : targetSize.height();

        auto scale = horizontal ? _mediaSize.height() / static_cast<double>(scaledHeight) : _mediaSize.width() / static_cast<double>(scaledWidth);

        const int sourceWidth = horizontal ? scaledWidth * scale : _mediaSize.width();
        const int sourceHeight = horizontal ? _mediaSize.height() : scaledHeight * scale;

        const int xOffset = (_mediaSize.width() - sourceWidth) / 2;
        const int yOffset = (_mediaSize.height() - sourceHeight) / 2;

        return QRect(xOffset, yOffset, sourceWidth , sourceHeight);
    }

    bool needsBackground(const QSize& _mediaSize)
    {
        return MediaUtils::isWideMedia(_mediaSize) || MediaUtils::isNarrowMedia(_mediaSize);
    }

    int calcMaxBlockWidth(const MediaUtils::MediaBlockParams& _params)
    {
        return std::min(_params.availableWidth, maxMediaBlockDimenson(_params.mediaType));
    }

    int calcMaxBlockHeight(const ComplexMessage::MediaUtils::MediaBlockParams& _params)
    {
        return (_params.isInsideQuote && _params.mediaType != MediaUtils::MediaType::Sticker) ? mediaInsideQuoteDimension() : maxMediaBlockDimenson(_params.mediaType);
    }

    int calcMinBlockWidth(const ComplexMessage::MediaUtils::MediaBlockParams& _params, const int _scaledWidth)
    {
        if (_params.isInsideQuote)
        {
            if (_params.mediaType == MediaUtils::MediaType::Sticker)
                return smallMediaBlockDimension(_params.mediaType);
            else
                return MediaUtils::isSmallMedia(_params.mediaSize) ? mediaInsideQuoteDimension() : _scaledWidth;
        }
        else
        {
            if (MediaUtils::isSmallMedia(_params.mediaSize))
                return smallMediaBlockDimension(_params.mediaType);
            else if (MediaUtils::isNarrowMedia(_params.mediaSize))
                return minMediaBlockWidth();
            else
                return _scaledWidth;
        }
    }

    int calcMinBlockHeight(const ComplexMessage::MediaUtils::MediaBlockParams& _params, const int _scaledHeight)
    {
        if (_params.isInsideQuote)
        {
            if (_params.mediaType == MediaUtils::MediaType::Sticker)
            {
                return smallMediaBlockDimension(_params.mediaType);
            }
            else
            {
                if (MediaUtils::isWideMedia(_params.mediaSize))
                    return mediaInsideQuoteDimension();
                else
                    return MediaUtils::isSmallMedia(_params.mediaSize) ? mediaInsideQuoteDimension() : _scaledHeight;
            }
        }
        else
        {
            if (MediaUtils::isSmallMedia(_params.mediaSize))
            {
                return smallMediaBlockDimension(_params.mediaType);
            }
            else if (MediaUtils::isWideMedia(_params.mediaSize))
            {
                return wideMediaHeight(_params.availableWidth);
            }
            else
            {
                switch (_params.mediaType)
                {
                    case MediaUtils::MediaType::Gif:
                    case MediaUtils::MediaType::Video:
                        return minMediaBlockHeight();
                    default:
                        return _scaledHeight;
                }
            }
        }
    }
}

namespace MediaUtils
{

QSize getTargetSize(const QRect& _rect, const QPixmap& _pixmap, const QSize& _originalSize)
{

    const auto sourceRect = mediaSourceRect(_pixmap.size(), _rect.size(), _originalSize);
    const auto scaledSize = isSmallMedia(_originalSize) ? _originalSize : sourceRect.size().scaled(_rect.size(), Qt::KeepAspectRatio);

    const auto minWidth = std::min(_rect.width(), scaledSize.width());
    const auto minHeight = std::min(_rect.height(), scaledSize.height());

    const auto xOffset = (std::max(_rect.width(), scaledSize.width()) - minWidth) / 2;
    const auto yOffset = (std::max(_rect.height(), scaledSize.height()) - minHeight) / 2;

    const auto targetRect = QRect(_rect.left() + xOffset, _rect.top() + yOffset, minWidth, minHeight);

    return targetRect.size();
}

void drawMediaInRect(QPainter& _p, const QRect& _rect, const QPixmap& _pixmap, const QSize& _originalSize, BackgroundMode _backgroundMode)
{
    if (_pixmap.isNull())
        return;

    const auto sourceRect = mediaSourceRect(_pixmap.size(), _rect.size(), _originalSize);
    const auto scaledSize = isSmallMedia(_originalSize) ? _originalSize : sourceRect.size().scaled(_rect.size(), Qt::KeepAspectRatio);

    const auto minWidth = std::min(_rect.width(), scaledSize.width());
    const auto minHeight = std::min(_rect.height(), scaledSize.height());

    const auto xOffset = (std::max(_rect.width(), scaledSize.width()) - minWidth) / 2;
    const auto yOffset = (std::max(_rect.height(), scaledSize.height()) - minHeight) / 2;

    const auto targetRect = QRect(_rect.left() + xOffset, _rect.top() + yOffset, minWidth, minHeight);

    if (needsBackground(scaledSize) && _backgroundMode == BackgroundMode::Auto)
        _p.fillRect(_rect, Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_MEDIA));

    _p.drawPixmap(targetRect, _pixmap, sourceRect);
}

QSize calcMediaBlockSize(const MediaBlockParams& _params)
{
    auto blockSize = _params.mediaSize;

    const auto maxBlockSize = QSize(calcMaxBlockWidth(_params), calcMaxBlockHeight(_params));

    const auto widthExceedsMax = _params.mediaSize.width() > maxBlockSize.width();
    const auto heightExceedsMax = _params.mediaSize.height() > maxBlockSize.height();

    if (widthExceedsMax || heightExceedsMax)
        blockSize.scale(maxBlockSize, Qt::KeepAspectRatio);

    const auto minBlockWidth = std::max(calcMinBlockWidth(_params, blockSize.width()), _params.minBlockWidth);
    const auto minBlockHeight = calcMinBlockHeight(_params, blockSize.height());

    blockSize.rwidth() = std::max(blockSize.width(), minBlockWidth);
    blockSize.rheight() = std::max(blockSize.rheight(), minBlockHeight);

    return blockSize;
}

bool isSmallMedia(const QSize& _mediaSize)
{
    return std::max(_mediaSize.width(), _mediaSize.height()) < Utils::scale_value(104);
}

bool isNarrowMedia(const QSize& _mediaSize)
{
    if (_mediaSize.isEmpty())
        return false;

    return (_mediaSize.width() / static_cast<double>(_mediaSize.height())) < 9. / 21.;
}

bool isWideMedia(const QSize &_mediaSize)
{
    if (_mediaSize.isEmpty())
        return false;

    return (_mediaSize.width() / static_cast<double>(_mediaSize.height())) > 21. / 9.;
}

QPixmap scaleMediaBlockPreview(const QPixmap& _pixmap)
{
    const auto maxBlockDimension = MessageStyle::Preview::getImageWidthMax();
    const auto maxBlockSize = Utils::scale_bitmap(QSize(maxBlockDimension, maxBlockDimension));

    const auto widthExceedsMax = _pixmap.width() > maxBlockSize.width();
    const auto heightExceedsMax = _pixmap.height() > maxBlockSize.height();

    const auto skipScale = isWideMedia(_pixmap.size()) || isNarrowMedia(_pixmap.size()) || isSmallMedia(_pixmap.size());

    if ((widthExceedsMax || heightExceedsMax) && !skipScale)
        return _pixmap.scaled(maxBlockSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    return _pixmap;
}

int mediaBlockMaxWidth(const QSize& _originalSize)
{
    const auto scaledSize = _originalSize.scaled(MessageStyle::Preview::getImageWidthMax(), MessageStyle::Preview::getImageHeightMax(), Qt::KeepAspectRatio);

    if (isWideMedia(_originalSize) || isNarrowMedia(_originalSize))
        return MessageStyle::Preview::getImageWidthMax();
    else if (std::min(_originalSize.width(), scaledSize.width()) < MessageStyle::Preview::mediaBlockWidthStretchThreshold())
        return MessageStyle::Preview::mediaBlockWidthStretchThreshold();
    else
        return std::min(MessageStyle::Preview::getImageWidthMax(), _originalSize.width());
}

bool isMediaSmallerThanBlock(const QSize& _blockSize, const QSize& _originalSize)
{
    const auto maxBlockDimension = MessageStyle::Preview::getImageWidthMax();
    const auto size = _originalSize.boundedTo(_originalSize.scaled(QSize(maxBlockDimension, maxBlockDimension), Qt::KeepAspectRatio));
    return size.width() < _blockSize.width() || size.height() < _blockSize.height();
}

}

UI_COMPLEX_MESSAGE_NS_END
