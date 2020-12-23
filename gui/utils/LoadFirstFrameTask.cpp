#include "stdafx.h"

#ifndef  STRIP_AV_MEDIA
#include "../main_window/mplayer/FFMpegPlayer.h"
#endif // ! STRIP_AV_MEDIA

#include "LoadFirstFrameTask.h"

namespace Utils
{
    LoadFirstFrameTask::LoadFirstFrameTask(QString path)
        : Path_(std::move(path))
    {
        assert(!Path_.isEmpty());
        assert(QFileInfo::exists(Path_));
    }

    LoadFirstFrameTask::~LoadFirstFrameTask() = default;

    void LoadFirstFrameTask::run()
    {
        if (!QFileInfo::exists(Path_))
        {
            if (Q_LIKELY(QCoreApplication::instance()))
                Q_EMIT loadedSignal(QPixmap(), QSize());
            return;
        }
#ifndef  STRIP_AV_MEDIA
        auto frame = Ui::VideoContext::getFirstFrame(Path_);
        const auto dur = Ui::VideoContext::getDuration(Path_);
        const auto audio = Ui::VideoContext::getGotAudio(Path_);

        assert(!frame.isNull());

        const auto frameSize = frame.size();
        if (Q_LIKELY(QCoreApplication::instance()))
            Q_EMIT loadedSignal(QPixmap::fromImage(std::move(frame)), frameSize);
        Q_EMIT duration(dur);
        Q_EMIT gotAudio(audio);
#else
        if (Q_LIKELY(QCoreApplication::instance()))
            Q_EMIT loadedSignal(QPixmap(), QSize());
#endif // ! STRIP_AV_MEDIA
    }
}
