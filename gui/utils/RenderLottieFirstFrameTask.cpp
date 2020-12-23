#include "stdafx.h"

#include "RenderLottieFirstFrameTask.h"
#include "utils.h"
#ifndef STRIP_AV_MEDIA
#include "main_window/mplayer/LottieHandle.h"
#endif

namespace Utils
{
    RenderLottieFirstFrameTask::RenderLottieFirstFrameTask(const QString& _fileName, QSize _size)
        : size_(std::move(_size))
        , path_(_fileName)
    {
    }

    void RenderLottieFirstFrameTask::run()
    {
        QImage res;

#ifndef STRIP_AV_MEDIA
        if (auto handle = Ui::LottieHandleCache::instance().getHandle(path_))
        {
            res = handle.renderFirstFrame(Utils::scale_bitmap(size_));
            Utils::check_pixel_ratio(res);
        }
#endif

        Q_EMIT result(res, QPrivateSignal());
    }
}