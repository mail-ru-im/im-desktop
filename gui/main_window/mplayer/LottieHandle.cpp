#include "stdafx.h"

#include "LottieHandle.h"
#include "utils/async/AsyncTask.h"

namespace
{
    constexpr std::chrono::milliseconds gcTimeout() { return std::chrono::seconds(1); }
}

namespace Ui
{
    LottieHandleCache& LottieHandleCache::instance()
    {
        static LottieHandleCache cache;
        return cache;
    }

    LottieHandle LottieHandleCache::getHandle(const QString& _path)
    {
        std::scoped_lock lock(mutex_);
        auto& ptr = nativePtrs_[_path];
        if (!ptr || !ptr->ptr_)
        {
            ptr = std::make_shared<LottieWrapper>();
            ptr->ptr_  = rlottie::Animation::loadFromFile(_path.toStdString());
        }

        im_assert(ptr);
        return LottieHandle(ptr);
    }

    bool LottieHandleCache::hasHandle(const QString& _path)
    {
        std::scoped_lock lock(mutex_);
        return nativePtrs_.find(_path) != nativePtrs_.end();
    }

    void LottieHandleCache::scheduleGC()
    {
        Async::runInMain([weakThis = QPointer(this)]()
        {
            if (weakThis)
            {
                auto& timer = weakThis->timer_;
                if (!timer)
                {
                    timer = new QTimer(weakThis);
                    timer->setSingleShot(true);
                    timer->setInterval(gcTimeout());
                    weakThis->connect(timer, &QTimer::timeout, weakThis, &LottieHandleCache::forceGC);
                }
                if (!timer->isActive())
                    timer->start();
            }
        });
    }

    void LottieHandleCache::forceGC()
    {
        decltype(nativePtrs_) tmp;
        {
            std::scoped_lock lock(mutex_);

            for (auto it = nativePtrs_.begin(); it != nativePtrs_.end();)
            {
                auto cur = it++;
                if (!cur->second || !cur->second->ptr_ || cur->second.unique())
                    tmp.insert(nativePtrs_.extract(cur));
            }
        }
    }

    QSize LottieHandle::originalSize() const
    {
        size_t w = 0;
        size_t h = 0;

        if (isValid())
            handle_->ptr_->size(w, h);

        return QSize((int)w, (int)h);
    }

    std::future<rlottie::Surface> LottieHandle::renderFrame(int _frameNo, rlottie::Surface _surface)
    {
#ifdef LOTTIE_FRAME_CACHE
        if (const auto& img = std::as_const(frames_[_frameNo]); !img.isNull() && img.size() == QSize(_surface.width(), _surface.height()))
        {
            std::packaged_task<rlottie::Surface()> render([&img, _surface = std::move(_surface)]()
            {
                memcpy(_surface.buffer(), img.bits(), _surface.bytesPerLine() * _surface.height());
                return _surface;
            });
            render();
            return render.get_future();
        }
#endif

        // rlottie is not designed to render several frames simultaneously in one handle
        if (isShared())
            return renderFrameSync(_frameNo, std::move(_surface));

        if (isValid())
            return handle_->ptr_->render(_frameNo, std::move(_surface));

        return {};
    }

    std::future<rlottie::Surface> LottieHandle::renderFrameSync(int _frameNo, rlottie::Surface _surface)
    {
        std::packaged_task<rlottie::Surface()> render([this, _frameNo, _surface = std::move(_surface)]()
        {
            if (isValid())
            {
                std::scoped_lock lock(handle_->mutex_);
                handle_->ptr_->renderSync(_frameNo, std::move(_surface));
            }
            return _surface;
        });
        render();
        return render.get_future();
    }

    QImage LottieHandle::renderFirstFrame(QSize _size)
    {
        if (!handle_)
            return QImage();

        QImage res(_size.isValid() ? _size : originalSize(), QImage::Format_ARGB32_Premultiplied);
        rlottie::Surface surface((uint32_t*)res.bits(), res.width(), res.height(), res.bytesPerLine());
        renderFrameSync(0, std::move(surface));
        return res;
    }

    void LottieHandle::setFrame(int _frameNo, const QImage& _frame)
    {
#ifdef LOTTIE_FRAME_CACHE
        if (_frameNo < 0 && _frameNo >= int(frames_.size()))
            return;

        if (auto& frame = frames_[_frameNo]; frame.isNull())
            frame = _frame;
#endif
    }

    LottieHandle::LottieHandle(wrapperSptr _handle)
        : handle_(std::move(_handle))
    {
#ifdef LOTTIE_FRAME_CACHE
        if (isValid())
            frames_.resize(handle_->ptr_->totalFrame());
#endif
    }

    LottieHandle::~LottieHandle()
    {
        if (isValid() && !isShared())
            LottieHandleCache::instance().scheduleGC();
    }
}