#include "stdafx.h"
#include "BackgroundWidget.h"

#include "../utils/utils.h"
#include "../main_window/MainPage.h"
#include "../styles/ThemeParameters.h"

Q_LOGGING_CATEGORY(bgWidget, "bgWidget")

namespace Ui
{
    BackgroundWidget::BackgroundWidget(QWidget* _parent)
        : QWidget(_parent)
        , tiling_(false)
    {
    }

    void BackgroundWidget::paintEvent(QPaintEvent *_e)
    {
        QPainter p(this);

        const auto view = viewRect_.isEmpty() ? rect() : viewRect_;

        if (bgColor_.isValid())
        {
            p.fillRect(rect(), bgColor_);
        }
        else if (!wallpaper_.isNull())
        {
            if (!tiling_)
            {
                auto imageRect = wallpaper_.rect();
                imageRect.setSize(imageRect.size().scaled(view.size(), Qt::KeepAspectRatioByExpanding));
                imageRect.moveCenter(view.center());

                p.setRenderHint(QPainter::SmoothPixmapTransform);
                p.drawPixmap(imageRect, wallpaper_);

                qCDebug(bgWidget) << QDateTime::currentDateTime().time() << imageRect << view << rect() << _e->rect();
            }
            else
            {
                p.fillRect(view, QBrush(wallpaper_));
            }
        }
    }

    void BackgroundWidget::setImage(const QPixmap& _pixmap, const bool _tiling)
    {
        if (!_pixmap.isNull() && _pixmap.cacheKey() != wallpaper_.cacheKey())
        {
            wallpaper_ = _pixmap;
            bgColor_ = QColor();
            tiling_ = _tiling;

            emit wallpaperChanged(QPrivateSignal());
            update();
        }
    }

    void BackgroundWidget::setColor(const QColor& _color)
    {
        assert(_color.isValid());
        assert(_color.alpha() == 255);
        bgColor_ = _color;
        wallpaper_ = QPixmap();
        tiling_ = false;

        emit wallpaperChanged(QPrivateSignal());
        update();
    }

    void BackgroundWidget::setViewRect(const QRect& _rect)
    {
        viewRect_ = _rect;

        emit wallpaperChanged(QPrivateSignal());
        update();
    }

    void BackgroundWidget::updateWallpaper(const QString& _aimId)
    {
        const auto p = Styling::getParameters(_aimId);
        if (p.isChatWallpaperPlainColor())
        {
            setColor(p.getChatWallpaperPlainColor());
        }
        else
        {
            if (p.isChatWallpaperImageLoaded())
            {
                setImage(p.getChatWallpaper(), p.isChatWallpaperTiled());
            }
            else
            {
                setColor(p.getChatWallpaperPlainColor());
                p.requestChatWallpaper();
            }
        }
    }

    bool BackgroundWidget::isFlatColor() const
    {
        return bgColor_.isValid();
    }
}
