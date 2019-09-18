#include "stdafx.h"

#include "InputBgWidget.h"
#include "controls/BackgroundWidget.h"
#include "utils/BlurPixmapTask.h"
#include "utils/utils.h"

namespace
{
    constexpr std::chrono::milliseconds resizeTimeout() noexcept { return std::chrono::milliseconds(10); }

    constexpr int getBlurRadius() noexcept
    {
        return 75;
    }
}

namespace Ui
{
    InputBgWidget::InputBgWidget(QWidget* _parent, BackgroundWidget* _bgWidget)
        : QWidget(_parent)
        , chatBg_(_bgWidget)
    {
        assert(_bgWidget);
        setAttribute(Qt::WA_TransparentForMouseEvents);

        resizeTimer_.setSingleShot(true);
        resizeTimer_.setInterval(resizeTimeout().count());
        connect(&resizeTimer_, &QTimer::timeout, this, &InputBgWidget::requestBg);

        connect(chatBg_, &BackgroundWidget::wallpaperChanged, this, &InputBgWidget::onWallpaperChanged);
    }

    void InputBgWidget::setOverlayColor(const QColor& _color)
    {
        assert(_color.alpha() != 255);
        assert(_color.isValid());
        overlay_ = _color;
        update();
    }

    void InputBgWidget::setBgVisible(const bool _isVisible)
    {
        if (isBgVisible_ == _isVisible)
            return;

        isBgVisible_ = _isVisible;
        if (!isBgVisible_)
            dropBg();

        update();
    }

    void InputBgWidget::forceUpdate()
    {
        requestedSize_ = QSize();
        requestedKey_ = -1;
        setBgVisible(!chatBg_->isFlatColor());
        requestBg();
    }

    void InputBgWidget::paintEvent(QPaintEvent*)
    {
        QPainter p(this);

        if (isBgVisible_ && !cachedBg_.isNull())
            p.drawPixmap(rect(), cachedBg_);

        p.fillRect(rect(), overlay_);
    }

    void InputBgWidget::resizeEvent(QResizeEvent* _e)
    {
        if (isVisible())
            resizeTimer_.start();
    }

    void InputBgWidget::showEvent(QShowEvent*)
    {
        if (cachedBg_.isNull())
            requestBg();
    }

    void InputBgWidget::dropBg()
    {
        cachedBg_ = QPixmap();
        requestedSize_ = QSize();
        update();
    }

    void InputBgWidget::requestBg()
    {
        if (!isBgVisible_ || chatBg_->isFlatColor())
            return;

        if (const auto newSize = size(); requestedSize_ != newSize && newSize.isValid())
        {
            requestedSize_ = newSize;

            QPixmap bg(newSize);
            {
                {
                    QPainter p(&bg);
                    const QRect clip(0, chatBg_->height() - height(), width(), height());
                    chatBg_->render(&p, QPoint(), QRegion(clip), QWidget::RenderFlags());
                }
                requestedKey_ = bg.cacheKey();

                auto task = new Utils::BlurPixmapTask(bg, getBlurRadius(), 1);
                connect(task, &Utils::BlurPixmapTask::blurred, this, &InputBgWidget::onBlurred);
                QThreadPool::globalInstance()->start(task);
            }
        }
    }

    void InputBgWidget::onBlurred(const QPixmap& _result, qint64 _srcCacheKey)
    {
        if (_srcCacheKey != requestedKey_)
            return;

        cachedBg_ = _result;
        requestedSize_ = QSize();
        requestedKey_ = -1;
        update();
    }

    void InputBgWidget::onWallpaperChanged()
    {
        dropBg();
        forceUpdate();
    }
}