#include "stdafx.h"

#include "Emoji.h"
#include "EmojiDb.h"

#include "../../utils/SChar.h"
#include "../../utils/utils.h"

#include "utils/features.h"

#if defined(__APPLE__)
#include "macos/Emoji_mac.h"
#endif

//#define DEBUG_EMOJI

namespace
{
    using namespace Emoji;

    std::unordered_map<int64_t, const QImage> EmojiCache_;

    std::unordered_map<QString, QString, Utils::QStringHasher> EmojiFilePathCache_;

    constexpr int64_t MakeCacheKey(int32_t _index, int32_t _sizePx) noexcept
    {
        return int64_t(_index) | (int64_t(_sizePx) << 32);
    }

    const std::vector<std::pair<int32_t, QString>>& appleEmojiSizes()
    {
        static const std::vector<std::pair<int32_t, QString>> sizes = { {104, qsl("104")} }; // only one size currently, but may change in the future
        return  sizes;
    }

    const std::pair<int32_t, QString>& getNearestAppleEmojiSize(const int32_t _size)
    {
        const auto& sizes = appleEmojiSizes();
        for (const auto& s : sizes)  // find first size greater than _size, if not found - use last, it is the biggest
        {
            if (s.first > _size)
                return s;
        }

        return sizes.back();
    }

    enum class EmojiType {Svg, Png};

    QString getEmojiFilePathKey(QLatin1String _base, const int32_t _size, EmojiType _type)
    {
        if (_type == EmojiType::Svg)
            return _base;
        else
            return _base % u'_' % getNearestAppleEmojiSize(_size).second;
    }

    QString getEmojiDefaultPath(EmojiType _type, const QString& _size = QString())
    {
        static const auto svgDefaultPath = qsl(":/emoji/default.svg");
        if (_type == EmojiType::Svg)
            return svgDefaultPath;
        else
            return u":/apple_emoji/default_" % _size % u".png";
    }

    QString getEmojiOneFullPath(QLatin1String _base)
    {
        QString path = u":/emoji/" % _base % u".svg";
        if (!QFileInfo::exists(path))
            path = getEmojiDefaultPath(EmojiType::Svg);
        return path;
    }

    QString getAppleEmojiFullPath(QLatin1String _base, const int32_t _size)
    {
        const auto size = getNearestAppleEmojiSize(_size).second;

        QString path = u":/apple_emoji/" % _base % u'_' % size % u".png";

        if (!QFileInfo::exists(path))
            path = getEmojiDefaultPath(EmojiType::Png, size);
        return path;
    }

    QString getEmojiFilePath(QLatin1String _base, const int32_t _size, EmojiType _type)
    {
        auto& path =  EmojiFilePathCache_[getEmojiFilePathKey(_base, _size, _type)];

        if (!path.isEmpty())
            return path;

        path = _type == EmojiType::Png ? getAppleEmojiFullPath(_base, _size) : getEmojiOneFullPath(_base);

        return path;
    }
}

namespace Emoji
{
    void InitializeSubsystem()
    {
        InitEmojiDb();
    }

    void Cleanup()
    {
        EmojiCache_.clear();
    }

    Emoji::EmojiSizePx getEmojiSize() noexcept
    {
        Emoji::EmojiSizePx emojiSize = Emoji::EmojiSizePx::_32;
        int scale = (int)(Utils::getScaleCoefficient() * 100.0);
        scale = Utils::scale_bitmap(scale);
        switch (scale)
        {
        case 100:
            emojiSize = Emoji::EmojiSizePx::_32;
            break;
        case 125:
            emojiSize = Emoji::EmojiSizePx::_40;
            break;
        case 150:
            emojiSize = Emoji::EmojiSizePx::_48;
            break;
        case 200:
            emojiSize = Emoji::EmojiSizePx::_64;
            break;
        case 250:
            emojiSize = Emoji::EmojiSizePx::_80;
            break;
        case 300:
            emojiSize = Emoji::EmojiSizePx::_96;
            break;
        default:
            assert(!"invalid scale");
        }

        return emojiSize;
    }

    QImage GetEmoji(const EmojiCode& _code, const EmojiSizePx _size)
    {
        assert(!_code.isNull());
        assert(_size >= EmojiSizePx::Min);
        assert(_size <= EmojiSizePx::Max);

        int32_t sizePx = (_size == EmojiSizePx::Auto) ? GetEmojiSizeForCurrentUiScale()
                                                      : int32_t(_size);
        return GetEmoji(_code, sizePx * Utils::scale_bitmap_ratio());
    }

#if defined(__APPLE__)
    static QImage getEmoji_mac(const EmojiCode& _code, int32_t _sizePx)
    {
        assert(!_code.isNull());
        const auto pixelRatio = qApp->primaryScreen()->devicePixelRatio();
        auto imageOut = mac::getEmoji(_code, _sizePx);
        imageOut.setDevicePixelRatio(pixelRatio);
        return imageOut;
    }

    static QImage getEmoji_qpainter(const EmojiCode& _code, int32_t _sizePx)
    {
        assert(!_code.isNull());
        const auto pixelRatio = qApp->devicePixelRatio();
        const QString s = EmojiCode::toQString(_code);

        const auto size = int(_sizePx * pixelRatio);
        QImage imageOut(size, size, QImage::Format_ARGB32_Premultiplied);
        imageOut.fill(Qt::transparent);
        imageOut.setDevicePixelRatio(pixelRatio);

        QPainter painter(&imageOut);
        const QFont font(qsl("AppleColorEmoji"), _sizePx - Utils::scale_value(6));
        painter.setFont(font);
        painter.drawText(QRect(0, 0, _sizePx, _sizePx), Qt::AlignCenter, s);

        imageOut.setDevicePixelRatio(pixelRatio);
        return imageOut;
    }
#endif

    static QImage getEmoji_svg(const QString& _fileName, int32_t _sizePx)
    {
        QImage image(_sizePx, _sizePx, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        if (Q_LIKELY(QFileInfo::exists(_fileName)))
        {
            QPainter painter(&image);
            QSvgRenderer renderer(_fileName);
            renderer.render(&painter);
            return image;
        }
        // TODO provide fallback
        return image;
    }

    static QImage getEmoji_png(const QString& _fileName, int32_t _sizePx)
    {
        QImage image(_sizePx, _sizePx, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);

        QImageReader reader(_fileName);
        reader.setScaledSize(QSize(_sizePx, _sizePx));
        reader.read(&image);

        return image;
    }

    static QImage getEmojiImage(QLatin1String _base, int32_t _sizePx)
    {
        const auto useAppleEmoji = Features::useAppleEmoji();

        const auto emojiFilePath = getEmojiFilePath(_base, _sizePx, useAppleEmoji ? EmojiType::Png : EmojiType::Svg);

        if (useAppleEmoji)
            return getEmoji_png(emojiFilePath, _sizePx);
        else
            return getEmoji_svg(emojiFilePath, _sizePx);
    }

    QImage GetEmoji(const EmojiCode& _code, int32_t _sizePx)
    {
        assert(_sizePx > 0);
        assert(!_code.isNull());

        const auto& info = GetEmojiInfoByCodepoint(_code);
        if (!info.isValid())
            return QImage();

        assert(info.Index_ >= 0);
        assert(info.FileName_.size() > 0);

        const auto key = MakeCacheKey(info.Index_, _sizePx);
        if (const auto cacheIter = std::as_const(EmojiCache_).find(key); cacheIter != std::as_const(EmojiCache_).end())
        {
            assert(!cacheIter->second.isNull());
            return cacheIter->second;
        }

#if defined(__APPLE__)
        auto image = useNativeEmoji() && mac::supportEmoji(_code) ? getEmoji_mac(_code, _sizePx) : getEmojiImage(info.FileName_, _sizePx);
#else
        auto image = getEmojiImage(info.FileName_, _sizePx);
#endif
        assert(!image.isNull());

#if defined(DEBUG_EMOJI)
        QImage filled(image.width(), image.height(), QImage::Format_ARGB32_Premultiplied);
        filled.fill(Qt::yellow);
        QPainter p(&filled);
        p.drawImage(filled.rect(), image);
        image = filled;
#endif //DEBUG_EMOJI

        EmojiCache_.insert({ key, image });

        return image;
    }

    QImage GetEmojiImage(const EmojiCode& _code, int32_t _sizePx)
    {
        if (const auto& info = GetEmojiInfoByCodepoint(_code); info.isValid())
            return getEmojiImage(info.FileName_, _sizePx);
        return QImage();
    }

    uint32_t readCodepoint(QStringView text, qsizetype& pos)
    {
        if (pos >= text.size())
            return 0;

        const QChar high = text.at(pos);
        ++pos;

        if (pos >= text.size() || !high.isHighSurrogate())
        {
            return high.unicode();
        }

        const QChar low = text.at(pos);

        if (!low.isLowSurrogate())
            return high.unicode();

        ++pos;

        return QChar::surrogateToUcs4(high, low);
    }

    EmojiCode getEmoji(QStringView _text, qsizetype& _pos)
    {
        const auto emojiMaxSize = EmojiCode::maxSize();
        if (_pos >= 0)
        {
            size_t i = 0;
            EmojiCode current;
            std::array<qsizetype, emojiMaxSize> prevPositions = {0};

            do
            {
                prevPositions[i] = _pos;
                if (const auto codePoint = readCodepoint(_text, _pos))
                    current = Emoji::EmojiCode::addCodePoint(current, codePoint);
                else
                    break;
            } while (++i < emojiMaxSize);

            while (i > 0)
            {
                if (Emoji::isEmoji(current))
                    return current;
                current = current.chopped(1);
                _pos = prevPositions[--i];
            }
            _pos = prevPositions.front();
        }
        return EmojiCode();
    }

    // This is meant to be used for "debug" purposes only
    const std::unordered_map<int64_t, const QImage> &GetEmojiCache()
    {
        return EmojiCache_;
    }

    bool isSupported(QLatin1String _filename)
    {
        const auto& [sizeAsInt, sizeAsStr] = appleEmojiSizes().front(); // get first size
        const auto type = Features::useAppleEmoji() ? EmojiType::Png : EmojiType::Svg;
        const auto path = getEmojiFilePath(_filename, sizeAsInt, type);

        return path != getEmojiDefaultPath(type, sizeAsStr);
    }

    int32_t GetEmojiSizeForCurrentUiScale()
    {
        constexpr std::pair<int32_t, int32_t> info[] =
        {
            { 100, 22 },
            { 125, 27 },
            { 150, 32 },
            { 200, 44 },
            { 250, 55 },
            { 300, 66 },
        };

        const auto it = std::find_if(std::begin(info), std::end(info), [k = int32_t(Utils::getScaleCoefficient() * 100)](auto a) { return a.first == k; });
        if (it != std::end(info))
        {
            assert(it->second > 0);
            return it->second;
        }

        assert(!"scale is unknown");
        return 0;
    }
}
