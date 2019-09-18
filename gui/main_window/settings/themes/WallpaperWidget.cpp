#include "stdafx.h"

#include "WallpaperWidget.h"

#include "../../../utils/utils.h"
#include "../../../styles/ThemeParameters.h"
#include "../../../styles/ThemesContainer.h"
#include "../../../fonts.h"

namespace
{
    QSize getButtonSize()
    {
        return Utils::scale_value(QSize(112, 112));
    }

    QFont getCaptionFont()
    {
        return Fonts::appFontScaled(14);
    }

    int getCornerRadius()
    {
        return Utils::scale_value(4);
    }

    QPainterPath getClipPath(const QRect& _rect)
    {
        QPainterPath p;
        p.addRoundedRect(_rect, getCornerRadius(), getCornerRadius());
        return p;
    }
}

namespace Ui
{
    WallpaperWidget::WallpaperWidget(QWidget* _parent, const Styling::WallpaperPtr _wallpaper)
        : ClickableWidget(_parent)
        , isSelected_(false)
        , wallpaper_(_wallpaper)
    {
        assert(_wallpaper);

        setFixedSize(getButtonSize());

        Utils::grabTouchWidget(this);

        connect(this, &ClickableWidget::clicked, this, [this]()
        {
            emit wallpaperClicked(getId(), QPrivateSignal());
        });

        connect(this, &ClickableWidget::hoverChanged, this, Utils::QOverload<>::of(&WallpaperWidget::update));
        connect(this, &ClickableWidget::pressChanged, this, Utils::QOverload<>::of(&WallpaperWidget::update));
    }

    const Styling::WallpaperId& WallpaperWidget::getId() const noexcept
    {
        return wallpaper_->getId();
    }

    void WallpaperWidget::onPreviewAwailable(const Styling::WallpaperId& _id)
    {
        if (pixmap_.isNull() && _id == wallpaper_->getId())
        {
            disconnect(&Styling::getThemesContainer(), &Styling::ThemesContainer::wallpaperPreviewAvailable, this, &WallpaperWidget::onPreviewAwailable);

            assert(!wallpaper_->getPreviewImage().isNull());
            if (const auto preview = wallpaper_->getPreviewImage(); !preview.isNull())
                setPreview(preview);
        }
    }

    void WallpaperWidget::setSelected(const bool _isSelected)
    {
        if (isSelected_ != _isSelected)
        {
            isSelected_ = _isSelected;
            update();
        }
    }

    void WallpaperWidget::setCaption(const QString& _capt, const QColor& _captColor)
    {
        caption_ = _capt;
        captionColor_ = _captColor;
        update();
    }

    void WallpaperWidget::dropCaption()
    {
        caption_.clear();
        update();
    }

    void WallpaperWidget::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setClipPath(clipPath_);

        if (!pixmap_.isNull())
        {
            auto imageRect = pixmap_.rect();
            imageRect.setSize(imageRect.size().scaled(rect().size(), Qt::KeepAspectRatioByExpanding));
            imageRect.moveCenter(rect().center());
            p.drawPixmap(imageRect, pixmap_);
        }
        else
        {
            p.fillRect(rect(), bgColor());
        }

        if (const auto t = tint(); t.isValid())
            p.fillRect(rect(), t);

        if (isSelected_)
        {
            static const auto mark = Utils::renderSvgLayered(
                qsl(":/background_apply_icon"),
                {
                    {qsl("bg"),   Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY) },
                    {qsl("tick"), Styling::getParameters().getColor(Styling::StyleVariable::TEXT_SOLID_PERMANENT) },
                }, Utils::scale_value(QSize(20, 20)));

            const auto padding = Utils::scale_value(8);
            const auto ratio = Utils::scale_bitmap_ratio();
            const auto x = width() - padding - mark.width() / ratio;
            const auto y = padding;

            p.drawPixmap(QPoint(x, y), mark);
        }

        if (!caption_.isEmpty() && captionColor_.isValid())
        {
            static const auto fnt = getCaptionFont();
            p.setFont(fnt);
            p.setPen(captionColor_);
            p.drawText(rect(), caption_, QTextOption(Qt::AlignCenter));
        }

        if constexpr (build::is_debug())
        {
            const QString text =
                getId().id_ % QChar::LineFeed %
                (wallpaper_->hasWallpaper() ? wallpaper_->getPreviewUrl() : wallpaper_->getBgColor().name()) % QChar::LineFeed %
                (wallpaper_->getTint().isValid() ? wallpaper_->getTint().name(QColor::HexArgb) : qsl("no tint"));

            p.setPen(wallpaper_->getColor(Styling::StyleVariable::BASE_GLOBALWHITE));
            p.drawText(rect().translated(Utils::scale_value(1), Utils::scale_value(1)), Qt::AlignHCenter | Qt::AlignBottom, text);

            p.setPen(wallpaper_->getColor(Styling::StyleVariable::TEXT_SOLID));
            p.drawText(rect(), Qt::AlignHCenter | Qt::AlignBottom, text);
        }
    }

    void WallpaperWidget::showEvent(QShowEvent*)
    {
        if (!isPreviewNeeded())
            return;

        if (const auto preview = wallpaper_->getPreviewImage(); !preview.isNull())
        {
            setPreview(preview);
        }
        else
        {
            connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::wallpaperPreviewAvailable, this, &WallpaperWidget::onPreviewAwailable);
            Styling::getThemesContainer().requestWallpaperPreview(wallpaper_);
        }
    }

    void WallpaperWidget::resizeEvent(QResizeEvent*)
    {
        clipPath_ = getClipPath(rect());
    }

    QColor WallpaperWidget::bgColor() const
    {
        return wallpaper_->getBgColor();
    }

    QColor WallpaperWidget::tint() const
    {
        if (!isSelected_)
        {
            if (isPressed())
                return QColor(0, 0, 0, 0.12 * 255);
            else if (isHovered())
                return QColor(0, 0, 0, 0.08 * 255);
        }

        return QColor(0, 0, 0, 0.03 * 255);
    }

    bool WallpaperWidget::isPreviewNeeded() const
    {
        return wallpaper_->hasWallpaper() && pixmap_.isNull();
    }

    void WallpaperWidget::setPreview(const QPixmap& _preview)
    {
        pixmap_ = _preview;
        update();
    }


    UserWallpaperWidget::UserWallpaperWidget(QWidget* _parent, const Styling::WallpaperPtr _wallpaper)
        : WallpaperWidget(_parent, _wallpaper)
    {
    }

    void UserWallpaperWidget::paintEvent(QPaintEvent* _e)
    {
        WallpaperWidget::paintEvent(_e);

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setClipPath(clipPath_);

        QPen pen(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), Utils::scale_value(2));

        static const auto dashes = []()
        {
            QVector<qreal> dashes;
            dashes << Utils::scale_value(2) << Utils::scale_value(4);
            return dashes;
        }();
        pen.setDashPattern(dashes);

        p.setPen(pen);
        p.drawRoundedRect(rect(), getCornerRadius(), getCornerRadius());
    }

    QColor UserWallpaperWidget::bgColor() const
    {
        const auto var = isPressed() ? Styling::StyleVariable::GHOST_ACCENT_ACTIVE
            : isHovered() ? Styling::StyleVariable::GHOST_ACCENT_HOVER
            : Styling::StyleVariable::GHOST_ACCENT;

        return Styling::getParameters().getColor(var);
    }

    QColor UserWallpaperWidget::tint() const
    {
        return QColor();
    }

    bool UserWallpaperWidget::isPreviewNeeded() const
    {
        return false;
    }
}
