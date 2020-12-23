#pragma once

#include "rlottie.h"
#include "utils/utils.h"
#include "memory_stats/FFmpegPlayerMemMonitor.h"

namespace Ui
{
    struct LottieWrapper
    {
        std::mutex mutex_;
        std::shared_ptr<rlottie::Animation> ptr_;
    };
    using wrapperSptr = std::shared_ptr<LottieWrapper>;

    class LottieHandle
    {
    public:
        QSize originalSize() const;

        std::future<rlottie::Surface> renderFrame(int _frameNo, rlottie::Surface _surface);
        std::future<rlottie::Surface> renderFrameSync(int _frameNo, rlottie::Surface _surface);
        QImage renderFirstFrame(QSize _size);

        void setFrame(int _frameNo, const QImage& _frame);

        bool isShared() const noexcept
        {
            return handle_.use_count() > 2; // 1 ours + 1 in cache
        }

        bool isValid() const noexcept
        {
            return handle_ && handle_->ptr_;
        }

        operator bool() const noexcept
        {
            return isValid();
        }

        rlottie::Animation* operator->() const noexcept
        {
            return isValid() ? handle_->ptr_.get() : nullptr;
        }

        LottieHandle(wrapperSptr _handle);
        ~LottieHandle();

        LottieHandle() = default;
        LottieHandle(LottieHandle&&) = default;
        LottieHandle(const LottieHandle&) = default;
        LottieHandle& operator=(LottieHandle&& other) = default;
        LottieHandle& operator=(const LottieHandle& other) = default;

    private:
        wrapperSptr handle_;

#ifdef LOTTIE_FRAME_CACHE
        std::vector<QImage> frames_;
#endif

        friend class Ui::FFmpegPlayerMemMonitor;
    };

    class LottieHandleCache : public QObject
    {
        Q_OBJECT

        friend class Ui::FFmpegPlayerMemMonitor;

    public:
        static LottieHandleCache& instance();
        LottieHandleCache(LottieHandleCache const&) = delete;
        LottieHandleCache& operator=(LottieHandleCache const&) = delete;

        LottieHandle getHandle(const QString& _path);
        bool hasHandle(const QString& _path);
        void scheduleGC();
        void forceGC();

    private:
        LottieHandleCache() = default;

    private:
        std::unordered_map<QString, wrapperSptr, Utils::QStringHasher> nativePtrs_;
        QTimer* timer_ = nullptr;
        std::mutex mutex_;
    };
}