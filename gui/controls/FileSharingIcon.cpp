#include "stdafx.h"

#include "FileSharingIcon.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../fonts.h"

using namespace FileSharing;

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

    struct FileTypeInfo
    {
        QPixmap icon_;
        QColor color_;

        FileTypeInfo() = default;

        FileTypeInfo(const QString& _iconPath, const QColor& _color)
            : icon_(Utils::renderSvg(_iconPath, getIconSize()))
            , color_(_color)
        {
        }
    };

    using FileInfoArray = std::array<std::pair<FileType, FileTypeInfo>, static_cast<size_t>(FileType::max_val)>;
    static FileInfoArray getFileInfoArray()
    {
        FileInfoArray fileinfo =
        {
            std::pair(FileType::archive, FileTypeInfo(qsl(":/filesharing/archive"), QColor(qsl("#FFBB34")))),
            std::pair(FileType::xls, FileTypeInfo(qsl(":/filesharing/excel"), QColor(qsl("#1B7346")))),
            std::pair(FileType::html, FileTypeInfo(qsl(":/filesharing/html"), QColor(qsl("#8A6AB3")))),
            std::pair(FileType::keynote, FileTypeInfo(qsl(":/filesharing/keynote"), QColor(qsl("#4691EA")))),
            std::pair(FileType::numbers, FileTypeInfo(qsl(":/filesharing/numbers"), QColor(qsl("#23D129")))),
            std::pair(FileType::pages, FileTypeInfo(qsl(":/filesharing/pages"), QColor(qsl("#F59A36")))),
            std::pair(FileType::pdf, FileTypeInfo(qsl(":/filesharing/pdf"), QColor(qsl("#F96657")))),
            std::pair(FileType::ppt, FileTypeInfo(qsl(":/filesharing/powerpoint"), QColor(qsl("#D34829")))),
            std::pair(FileType::audio, FileTypeInfo(qsl(":/filesharing/sound"), QColor(qsl("#B684C3")))),
            std::pair(FileType::txt, FileTypeInfo(qsl(":/filesharing/text"), QColor(qsl("#1E93F1")))),
            std::pair(FileType::doc, FileTypeInfo(qsl(":/filesharing/word"), QColor(qsl("#23589B")))),
            std::pair(FileType::apk, FileTypeInfo(qsl(":/filesharing/other"), QColor(qsl("#A8ADB8")))),

            std::pair(FileType::unknown, FileTypeInfo(qsl(":/filesharing/other"), QColor(qsl("#A8ADB8")))),
        };

        if (!std::is_sorted(std::cbegin(fileinfo), std::cend(fileinfo), Utils::is_less_by_first<FileType, FileTypeInfo>))
            std::sort(std::begin(fileinfo), std::end(fileinfo), Utils::is_less_by_first<FileType, FileTypeInfo>);

        return fileinfo;
    }

    const FileTypeInfo& getTypeInfo(const FileType _type)
    {
        static const auto types = getFileInfoArray();
        return types[static_cast<size_t>(_type)].second;
    }

    QPixmap getUnknownPreviewIcon(const QString& _filename)
    {
        const auto ext = QFileInfo(_filename).suffix();
        const auto& otherIcon = getTypeInfo(FileType::unknown);

        if (ext.isEmpty() || ext.length() > 7)
            return otherIcon.icon_;

        static std::map<QString, std::optional<QPixmap>, StringComparator> extensions;
        auto& pixmap = extensions[ext];
        if (pixmap)
            return *pixmap;

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

        pixmap  = std::move(pm);
        return *pixmap;
    }

    QPixmap getPreviewIcon(const QString& _filename)
    {
        const auto t = getFSType(_filename);
        if (t == FileType::unknown || t == FileType::apk)
            return getUnknownPreviewIcon(_filename);

        return getTypeInfo(t).icon_;
    }

    QColor getProgressColor(const QString& _filename)
    {
        return getTypeInfo(getFSType(_filename)).color_;
    }
}

namespace Ui
{
    FileSharingIcon::FileSharingIcon(QWidget* _parent)
        : ClickableWidget(_parent)
        , iconColor_(getTypeInfo(FileType::unknown).color_)
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

        return getTypeInfo(FileType::unknown).icon_;
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
