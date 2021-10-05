#include "stdafx.h"
#include "BackgroundWidget.h"

#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../main_window/MainPage.h"
#include "../styles/ThemeParameters.h"

Q_LOGGING_CATEGORY(bgWidget, "bgWidget")

namespace Ui
{
    BackgroundWidget::BackgroundWidget(QWidget* _parent, bool _isMultiselectEnabled)
        : QWidget(_parent)
        , tiling_(false)
        , isMultiselectEnabled_(_isMultiselectEnabled)
    {
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectChanged, this, qOverload<>(&BackgroundWidget::update));
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::multiselectAnimationUpdate, this, qOverload<>(&BackgroundWidget::update));
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

        if (isMultiselectEnabled_)
        {
            double current = Utils::InterConnector::instance().multiselectAnimationCurrent() / 100.0;
            const auto color = Styling::getParameters().getColor(Styling::StyleVariable::CHAT_ENVIRONMENT, 0.85 * current);
            p.fillRect(rect(), color);
        }
    }

    void BackgroundWidget::setImage(const QPixmap& _pixmap, const bool _tiling)
    {
        if (!_pixmap.isNull() && _pixmap.cacheKey() != wallpaper_.cacheKey())
        {
            wallpaper_ = _pixmap;
            bgColor_ = QColor();
            tiling_ = _tiling;

            Q_EMIT wallpaperChanged(QPrivateSignal());
            update();
        }
    }

    void BackgroundWidget::setColor(const QColor& _color)
    {
        im_assert(_color.isValid());
        im_assert(_color.alpha() == 255);
        bgColor_ = _color;
        wallpaper_ = QPixmap();
        tiling_ = false;

        Q_EMIT wallpaperChanged(QPrivateSignal());
        update();
    }

    void BackgroundWidget::setViewRect(const QRect& _rect)
    {
        viewRect_ = _rect;

        Q_EMIT wallpaperChanged(QPrivateSignal());
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

        aimId_ = _aimId;
    }

    bool BackgroundWidget::isFlatColor() const
    {
        return bgColor_.isValid();
    }
}
