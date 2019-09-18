#include "stdafx.h"

#include "FileSharingIcon.h"
#include "../utils/utils.h"
#include "../fonts.h"

namespace
{
    constexpr auto QT_ANGLE_MULT = 16;
    constexpr auto animDuration = std::chrono::milliseconds(700);

    QSize getIconSize()
    {
        return Utils::scale_value(QSize(44, 44));
    }

    QSize getButtonSize()
    {
        return QSize(20, 20);
    }

    QPixmap getDownloadIcon()
    {
        static const auto download_icon = Utils::renderSvgScaled(qsl(":/filesharing/download_icon"), getButtonSize(), Qt::white);
        return download_icon;
    }

    QPixmap getCancelIcon()
    {
        static const auto cancel_icon = Utils::renderSvgScaled(qsl(":/filesharing/cancel_icon"), getButtonSize(), Qt::white);
        return cancel_icon;
    }

    QString getExtension(const QString& _filename)
    {
        const auto extNdx = _filename.lastIndexOf(ql1c('.'));
        if (extNdx == -1 || extNdx + 1 == _filename.length())
            return QString();

        return _filename.mid(extNdx + 1).toLower();
    }

    enum class type
    {
        archive,
        xls,
        html,
        keynote,
        numbers,
        pages,
        pdf,
        ppt,
        audio,
        txt,
        doc,

        unknown,
    };

    struct FileTypeInfo
    {
        QPixmap icon_;
        QColor color_;

        FileTypeInfo(const QString& _iconPath, const QColor& _color)
            : icon_(Utils::renderSvg(_iconPath, getIconSize()))
            , color_(_color)
        {
        }
    };

    const FileTypeInfo& getTypeInfo(const type _type)
    {
        static const std::map<type, FileTypeInfo> types
        {
            { type::archive, FileTypeInfo(qsl(":/filesharing/archive"), QColor(qsl("#FFBB34"))) },
            { type::xls, FileTypeInfo(qsl(":/filesharing/excel"), QColor(qsl("#1B7346"))) },
            { type::html, FileTypeInfo(qsl(":/filesharing/html"), QColor(qsl("#8A6AB3"))) },
            { type::keynote, FileTypeInfo(qsl(":/filesharing/keynote"), QColor(qsl("#4691EA"))) },
            { type::numbers, FileTypeInfo(qsl(":/filesharing/numbers"), QColor(qsl("#23D129"))) },
            { type::pages, FileTypeInfo(qsl(":/filesharing/pages"), QColor(qsl("#F59A36"))) },
            { type::pdf, FileTypeInfo(qsl(":/filesharing/pdf"), QColor(qsl("#F96657"))) },
            { type::ppt, FileTypeInfo(qsl(":/filesharing/powerpoint"), QColor(qsl("#D34829"))) },
            { type::audio, FileTypeInfo(qsl(":/filesharing/sound"), QColor(qsl("#B684C3"))) },
            { type::txt, FileTypeInfo(qsl(":/filesharing/text"), QColor(qsl("#1E93F1"))) },
            { type::doc, FileTypeInfo(qsl(":/filesharing/word"), QColor(qsl("#23589B"))) },

            { type::unknown, FileTypeInfo(qsl(":/filesharing/other"), QColor(qsl("#A8ADB8"))) },
        };

        const auto it = types.find(_type);
        if (it == types.end())
            return types.find(type::unknown)->second;

        return it->second;
    }

    type getType(const QString& _filename)
    {
        static const std::map<QString, type, StringComparator> knownTypes
        {
            { qsl("zip"), type::archive },
            { qsl("rar"), type::archive },
            { qsl("tar"), type::archive },
            { qsl("xz"),  type::archive },
            { qsl("gz"),  type::archive },
            { qsl("7z"),  type::archive },
            { qsl("bz2"),  type::archive },

            { qsl("xls"),  type::xls },
            { qsl("xlsx"), type::xls },
            { qsl("xlsm"), type::xls },
            { qsl("csv"),  type::xls },

            { qsl("html"), type::html },
            { qsl("htm"),  type::html },

            { qsl("key"),  type::keynote },

            { qsl("numbers"),  type::numbers },

            { qsl("pages"),  type::pages },

            { qsl("pdf"),  type::pdf },

            { qsl("ppt"),  type::ppt },
            { qsl("pptx"), type::ppt },

            { qsl("wav"),  type::audio },
            { qsl("mp3"),  type::audio },
            { qsl("ogg"),  type::audio },
            { qsl("flac"), type::audio },
            { qsl("aac"),  type::audio },
            { qsl("m4a"),  type::audio },
            { qsl("aiff"), type::audio },
            { qsl("ape"),  type::audio },
            { qsl("midi"),  type::audio },

            { qsl("log"), type::txt },
            { qsl("txt"), type::txt },
            { qsl("md"), type::txt },

            { qsl("doc"),  type::doc },
            { qsl("docx"), type::doc },
            { qsl("rtf"),  type::doc },
        };

        if (const auto ext = getExtension(_filename); !ext.isEmpty())
        {
            const auto it = knownTypes.find(ext);
            if (it != knownTypes.end())
                return it->second;
        }

        return type::unknown;
    }

    QPixmap getUnknownPreviewIcon(const QString& _filename)
    {
        const auto ext = getExtension(_filename);
        const auto& otherIcon = getTypeInfo(type::unknown);

        if (ext.isEmpty() || ext.length() > 7)
            return otherIcon.icon_;

        static std::map<QString, QPixmap, StringComparator> extensions;
        if (const auto it = extensions.find(ext); it != extensions.end())
            return it->second;

        static const std::map<int, QFont> fontSizes = []()
        {
            const std::map<std::vector<int>, int> sizes =
            {
                { { 1, 2, 3, 4, 5 }, 12 },
                { { 6 },             9  },
                { { 7 },             8  },
            };

            std::map<int, QFont> res;
            for (const auto& [vec, size] : sizes)
                for (const auto s : vec)
                    res[s] = Fonts::appFontScaled(Utils::scale_bitmap(size), Fonts::FontFamily::ROUNDED_MPLUS);
            return res;
        }();

        const auto it = fontSizes.find(ext.length());
        if (it == fontSizes.end())
            return otherIcon.icon_;

        QPixmap pm(Utils::scale_bitmap(getIconSize()));
        pm.fill(Qt::transparent);
        {
            QPainter p(&pm);
            p.setRenderHint(QPainter::Antialiasing);

            p.setBrush(otherIcon.color_);
            p.setPen(Qt::transparent);
            p.drawEllipse(pm.rect());

            p.setPen(Qt::white);
            p.setFont(it->second);

            p.drawText(pm.rect(), Qt::AlignVCenter | Qt::AlignHCenter, ext.toUpper());
        }
        Utils::check_pixel_ratio(pm);

        extensions[ext] = pm;
        return pm;
    }

    QPixmap getPreviewIcon(const QString& _filename)
    {
        const auto t = getType(_filename);
        if (t == type::unknown)
            return getUnknownPreviewIcon(_filename);

        return getTypeInfo(t).icon_;
    }

    QColor getProgressColor(const QString& _filename)
    {
        return getTypeInfo(getType(_filename)).color_;
    }
}

namespace Ui
{
    FileSharingIcon::FileSharingIcon(QWidget* _parent)
        : ClickableWidget(_parent)
        , iconColor_(getTypeInfo(type::unknown).color_)
        , bytesTotal_(0)
        , bytesCurrent_(0)
    {
        setFixedSize(getIconSize());
    }

    FileSharingIcon::~FileSharingIcon()
    {
        stopAnimation();
    }

    void FileSharingIcon::setBytes(const int64_t _bytesTotal, const int64_t _bytesCurrent)
    {
        setTotalBytes(_bytesTotal);
        setCurrentBytes(_bytesCurrent);
    }

    void FileSharingIcon::setTotalBytes(const int64_t _bytesTotal)
    {
        if (bytesTotal_ != _bytesTotal)
        {
            bytesTotal_ = _bytesTotal;
            update();
        }
    }

    void FileSharingIcon::setCurrentBytes(const int64_t _bytesCurrent)
    {
        if (bytesCurrent_ != _bytesCurrent)
        {
            bytesCurrent_ = _bytesCurrent;
            update();
        }
    }

    void FileSharingIcon::setDownloaded()
    {
        setBytes(1, 1);
    }

    void FileSharingIcon::reset()
    {
        setBytes(0, 0);
    }

    void FileSharingIcon::setFilename(const QString& _fileName)
    {
        iconColor_ = getProgressColor(_fileName);
        fileType_ = getPreviewIcon(_fileName);
        update();
    }

    QPixmap FileSharingIcon::getIcon() const
    {
        if (!fileType_.isNull())
            return fileType_;

        return getTypeInfo(type::unknown).icon_;
    }

    QPixmap FileSharingIcon::getIcon(const QString& _fileName)
    {
        return getPreviewIcon(_fileName);
    }

    QColor FileSharingIcon::getFillColor(const QString& _fileName)
    {
        return getProgressColor(_fileName);
    }

    void FileSharingIcon::startAnimation()
    {
        if (!anim_.isRunning())
            anim_.start([this]() { update(); }, 0.0, 360.0, animDuration.count(), anim::linear, -1);
    }

    void FileSharingIcon::stopAnimation()
    {
        anim_.finish();
    }

    bool FileSharingIcon::isAnimating() const
    {
        return anim_.state() != anim::State::Stopped;
    }

    void FileSharingIcon::onVisibilityChanged(const bool _isVisible)
    {
        if (_isVisible)
            anim_.resume();
        else
            anim_.pause();
    }

    void FileSharingIcon::paintEvent(QPaintEvent* event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const auto drawIcon = [&p, this](const QPixmap& _pm)
        {
            if (!_pm.isNull())
            {
                const QPoint pos((width() - _pm.width() / Utils::scale_bitmap_ratio()) / 2, (height() - _pm.height() / Utils::scale_bitmap_ratio()) / 2);
                p.drawPixmap(pos, _pm);
            }
        };

        if (!isFileDownloaded())
        {
            const auto animAngle = anim_.isRunning() ? anim_.current() : 0.0;
            const auto baseAngle = (animAngle * QT_ANGLE_MULT);
            const auto progress = bytesTotal_ == 0 ? 0 : std::max((double)bytesCurrent_ / (double)bytesTotal_, 0.03);
            const auto progressAngle = (int)std::ceil(progress * 360 * QT_ANGLE_MULT);

            p.setBrush(iconColor_);
            p.setPen(Qt::transparent);
            p.drawEllipse(rect());

            if (isFileDownloading())
            {
                static const QPen pen(Qt::white, Utils::scale_value(2));
                p.setPen(pen);

                static const auto r2 = rect().adjusted(Utils::scale_value(4), Utils::scale_value(4), -Utils::scale_value(4), -Utils::scale_value(4));
                p.drawArc(r2, -baseAngle, -progressAngle);

                drawIcon(getCancelIcon());
            }
            else
            {
                drawIcon(getDownloadIcon());
            }
        }
        else
        {
            drawIcon(fileType_);
        }

        if (isHovered())
        {
            static const auto clipPath = [this]()
            {
                QPainterPath path;
                path.addEllipse(rect());
                return path;
            }();
            p.setClipPath(clipPath);
            p.fillRect(rect(), QColor(0, 0, 0, 0.08 * 255));
        }
    }

    bool FileSharingIcon::isFileDownloading() const
    {
        return bytesTotal_ > 0 && bytesCurrent_ < bytesTotal_;
    }

    bool FileSharingIcon::isFileDownloaded() const
    {
        return bytesTotal_ > 0 && bytesCurrent_ >= bytesTotal_;
    }
}
