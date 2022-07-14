#include "stdafx.h"

#include "FileSharingIcon.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../utils/StringComparator.h"
#include "../fonts.h"
#include "main_window/history_control/MessageStyle.h"

using namespace FileSharing;

namespace
{
    constexpr auto QT_ANGLE_MULT = 16;
    constexpr auto animDuration = std::chrono::milliseconds(700);

    constexpr QSize getButtonSize() noexcept
    {
        return { 20, 20 };
    }

    const QPixmap& getDownloadIcon()
    {
        static const auto download_icon = Utils::renderSvgScaled(qsl(":/filesharing/download_icon"), getButtonSize(), Qt::white);
        return download_icon;
    }

    const QPixmap& getCancelIcon()
    {
        static const auto cancel_icon = Utils::renderSvgScaled(qsl(":/filesharing/cancel_icon"), getButtonSize(), Qt::white);
        return cancel_icon;
    }

    QPixmap renderCircledIcon(const QString& _iconPath, QColor _iconColor, QColor _backgroundColor)
    {
        QPixmap pm(Utils::scale_bitmap(Ui::FileSharingIcon::getIconSize()));
        pm.fill(Qt::transparent);
        {
            QPainter p(&pm);
            p.setRenderHint(QPainter::Antialiasing);

            p.setBrush(_backgroundColor);
            p.setPen(Qt::transparent);
            p.drawEllipse(pm.rect());

            auto icon = Utils::renderSvg(_iconPath, Ui::FileSharingIcon::getIconSize(), _iconColor);
            icon.setDevicePixelRatio(1);

            p.drawPixmap(0, 0, icon);
        }
        Utils::check_pixel_ratio(pm);
        return pm;
    }

    QPixmap renderInfectedIcon(const QString& _iconPath, bool _isOutgoing)
    {
        return renderCircledIcon(_iconPath, Ui::MessageStyle::getInfectedFileIconColor(_isOutgoing), Ui::MessageStyle::getInfectedFileIconBackground(_isOutgoing));
    }

    QPixmap renderExtensionIcon(const QString& _extension, const QFont& _font, QColor _textColor, QColor _backgroundColor)
    {
        QPixmap pm(Utils::scale_bitmap(Ui::FileSharingIcon::getIconSize()));
        pm.fill(Qt::transparent);
        {
            QPainter p(&pm);
            p.setRenderHint(QPainter::Antialiasing);

            p.setBrush(_backgroundColor);
            p.setPen(Qt::transparent);
            p.drawEllipse(pm.rect());

            p.setPen(_textColor);
            p.setFont(_font);

            p.drawText(pm.rect(), Qt::AlignVCenter | Qt::AlignHCenter, _extension);
        }
        Utils::check_pixel_ratio(pm);
        return pm;
    }

    enum class PreviewIconType
    {
        SafeFile,
        InfectedOutgoing,
        InfectedIncoming
    };

    struct FileTypeInfo
    {
        QPixmap icon_;
        QPixmap disabledOutgoingIcon_;
        QPixmap disabledIncomingIcon_;
        QColor color_;

        FileTypeInfo() = default;

        FileTypeInfo(const QString& _iconPath, const QColor& _color)
            : icon_(renderCircledIcon(_iconPath, Qt::white, _color))
            , disabledOutgoingIcon_(renderInfectedIcon(_iconPath, true))
            , disabledIncomingIcon_(renderInfectedIcon(_iconPath, false))
            , color_(_color)
        {
        }

        FileTypeInfo(const QString& _extension, const QFont& _font, const QColor& _color)
            : icon_(renderExtensionIcon(_extension, _font, Qt::white, _color))
            , disabledOutgoingIcon_(renderExtensionIcon(_extension, _font, Ui::MessageStyle::getInfectedFileIconColor(true), Ui::MessageStyle::getInfectedFileIconBackground(true)))
            , disabledIncomingIcon_(renderExtensionIcon(_extension, _font, Ui::MessageStyle::getInfectedFileIconColor(false), Ui::MessageStyle::getInfectedFileIconBackground(false)))
            , color_(_color)
        {}

        QPixmap operator[](PreviewIconType _type)
        {
            switch (_type)
            {
            case PreviewIconType::SafeFile:
                return icon_;
            case PreviewIconType::InfectedOutgoing:
                return disabledOutgoingIcon_;
            case PreviewIconType::InfectedIncoming:
                return disabledIncomingIcon_;
            default:
                im_assert(!"Wrong preview type");
                return icon_;
            }
        }
    };

    using FileInfoArray = std::array<std::pair<FileType, FileTypeInfo>, static_cast<size_t>(FileType::max_val)>;
    FileInfoArray getFileInfoArray()
    {
        FileInfoArray fileinfo =
        {
            std::pair(FileType::archive, FileTypeInfo(qsl(":/filesharing/archive"), QColor(u"#FFBB34"))),
            std::pair(FileType::xls, FileTypeInfo(qsl(":/filesharing/excel"), QColor(u"#1B7346"))),
            std::pair(FileType::html, FileTypeInfo(qsl(":/filesharing/html"), QColor(u"#8A6AB3"))),
            std::pair(FileType::keynote, FileTypeInfo(qsl(":/filesharing/keynote"), QColor(u"#4691EA"))),
            std::pair(FileType::numbers, FileTypeInfo(qsl(":/filesharing/numbers"), QColor(u"#23D129"))),
            std::pair(FileType::pages, FileTypeInfo(qsl(":/filesharing/pages"), QColor(u"#F59A36"))),
            std::pair(FileType::pdf, FileTypeInfo(qsl(":/filesharing/pdf"), QColor(u"#F96657"))),
            std::pair(FileType::ppt, FileTypeInfo(qsl(":/filesharing/powerpoint"), QColor(u"#D34829"))),
            std::pair(FileType::audio, FileTypeInfo(qsl(":/filesharing/sound"), QColor(u"#B684C3"))),
            std::pair(FileType::txt, FileTypeInfo(qsl(":/filesharing/text"), QColor(u"#1E93F1"))),
            std::pair(FileType::doc, FileTypeInfo(qsl(":/filesharing/word"), QColor(u"#23589B"))),
            std::pair(FileType::apk, FileTypeInfo(qsl(":/filesharing/other"), QColor(u"#A8ADB8"))),

            std::pair(FileType::unknown, FileTypeInfo(qsl(":/filesharing/other"), QColor(u"#A8ADB8"))),
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

    QPixmap getUnknownPreviewIcon(const QString& _filename, PreviewIconType _type)
    {
        const auto ext = QFileInfo(_filename).suffix();
        const auto& otherIcon = getTypeInfo(FileType::unknown);

        if (ext.isEmpty() || ext.length() > 7)
            return otherIcon.icon_;

        static std::map<QString, std::optional<FileTypeInfo>, Utils::StringComparator> extensions;
        auto& fileTypeInfo = extensions[ext];
        if (fileTypeInfo)
            return (*fileTypeInfo)[_type];

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

        fileTypeInfo = FileTypeInfo(ext.toUpper(), it->second, otherIcon.color_);

        return (*fileTypeInfo)[_type];
    }

    QPixmap getPreviewIcon(const QString& _filename)
    {
        const auto t = getFSType(_filename);
        if (t == FileType::unknown || t == FileType::apk)
            return getUnknownPreviewIcon(_filename, PreviewIconType::SafeFile);

        return getTypeInfo(t).icon_;
    }

    QPixmap getInfectedIcon(const QString& _filename, bool _isOutgoing)
    {
        const auto t = getFSType(_filename);

        if (t == FileType::unknown || t == FileType::apk)
            return getUnknownPreviewIcon(_filename, _isOutgoing ? PreviewIconType::InfectedOutgoing : PreviewIconType::InfectedIncoming);

        return _isOutgoing ? getTypeInfo(t).disabledOutgoingIcon_ : getTypeInfo(t).disabledIncomingIcon_;
    }

    QColor getProgressColor(const QString& _filename)
    {
        return getTypeInfo(getFSType(_filename)).color_;
    }

    const QPixmap& getBlockedIcon(bool _isOutgoing)
    {
        auto makeIcon = [](bool _isOutgoing)
        {
            QPixmap pm(Utils::scale_bitmap(Ui::FileSharingIcon::getIconSize()));
            pm.fill(Qt::transparent);
            {
                QPainter p(&pm);
                p.setRenderHint(QPainter::Antialiasing);

                p.setBrush(Ui::MessageStyle::getBlockedFileIconBackground(_isOutgoing));
                p.setPen(Qt::transparent);
                p.drawEllipse(pm.rect());

                auto icon = Utils::renderSvg(qsl(":/filesharing/blocked"), Ui::FileSharingIcon::getIconSize(), Ui::MessageStyle::getBlockedFileIconColor(_isOutgoing));
                icon.setDevicePixelRatio(1);

                p.drawPixmap(0, 0, icon);
            }
            Utils::check_pixel_ratio(pm);
            return pm;
        };

        static const auto in = makeIcon(false);
        static const auto out = makeIcon(true);
        return _isOutgoing ? out : in;
    }
}

namespace Ui
{
    FileSharingIcon::FileSharingIcon(bool _isOutgoing, QWidget* _parent)
        : ClickableWidget(_parent)
        , iconColor_(getTypeInfo(FileType::unknown).color_)
        , isOutgoing_(_isOutgoing)
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
        fileName_ = _fileName;
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

    QSize FileSharingIcon::getIconSize()
    {
        return Utils::scale_value(QSize(44, 44));
    }

    void FileSharingIcon::startAnimation()
    {
        if (!isAnimating())
        {
            initAnimation();
            anim_->start();
        }
    }

    void FileSharingIcon::stopAnimation()
    {
        if (anim_)
            anim_->stop();
    }

    bool FileSharingIcon::isAnimating() const
    {
        return anim_ && anim_->state() == QAbstractAnimation::State::Running;
    }

    void FileSharingIcon::onVisibilityChanged(const bool _isVisible)
    {
        if (_isVisible && anim_ && anim_->state() == QAbstractAnimation::Paused)
            anim_->resume();
        else if (!_isVisible && isAnimating())
            anim_->pause();
    }

    void FileSharingIcon::setBlocked(BlockReason _reason)
    {
        blockReason_ = _reason;
        switch (_reason)
        {
        case BlockReason::NoBlock:
            overrideIcon_ = {};
            break;
        case BlockReason::Antivirus:
            overrideIcon_ = getInfectedIcon(fileName_, isOutgoing_);
            break;
        case BlockReason::TrustCheck:
            overrideIcon_ = getBlockedIcon(isOutgoing_);
            break;
        default:
            im_assert(!"invalid reason");
            break;
        }
        update();
    }

    void FileSharingIcon::paintEvent(QPaintEvent*)
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

        if (!overrideIcon_.isNull())
        {
            if (BlockReason::TrustCheck == blockReason_ && themeChecker_.checkAndUpdateHash())
                overrideIcon_ = getBlockedIcon(isOutgoing_);
            drawIcon(overrideIcon_);
        }
        else if (!isFileDownloaded())
        {
            const auto animAngle = isAnimating() ? anim_->currentValue().toDouble() : 0.0;
            const auto baseAngle = (animAngle * QT_ANGLE_MULT);
            const auto progress = bytesTotal_ == 0 ? 0 : std::max((double)bytesCurrent_ / (double)bytesTotal_, 0.03);
            const auto progressAngle = (int)std::ceil(progress * 360 * QT_ANGLE_MULT);

            p.setBrush(iconColor_);
            p.setPen(Qt::NoPen);
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

    void FileSharingIcon::initAnimation()
    {
        if (anim_)
            return;

        anim_ = new QVariantAnimation(this);
        anim_->setStartValue(0.0);
        anim_->setEndValue(360.0);
        anim_->setDuration(animDuration.count());
        anim_->setLoopCount(-1);
        connect(anim_, &QVariantAnimation::valueChanged, this, qOverload<>(&ClickableWidget::update));
    }
}
