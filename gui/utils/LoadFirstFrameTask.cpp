#include "stdafx.h"

#include "../main_window/mplayer/FFMpegPlayer.h"

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
            Q_EMIT loadedSignal(QPixmap(), QSize());
            return;
        }

        auto frame = Ui::VideoContext::getFirstFrame(Path_);
        const auto dur = Ui::VideoContext::getDuration(Path_);
        const auto audio = Ui::VideoContext::getGotAudio(Path_);

        assert(!frame.isNull());

        const auto frameSize = frame.size();
        Q_EMIT loadedSignal(QPixmap::fromImage(std::move(frame)), frameSize);
        Q_EMIT duration(dur);
        Q_EMIT gotAudio(audio);
    }
}
