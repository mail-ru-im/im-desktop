#include "stdafx.h"

#include "../main_window/mplayer/FFMpegPlayer.h"

#include "LoadFirstFrameTask.h"

namespace Utils
{
    LoadFirstFrameTask::LoadFirstFrameTask(QString path)
        : Path_(std::move(path))
    {
        assert(!Path_.isEmpty());
        assert(QFile::exists(Path_));
    }

    LoadFirstFrameTask::~LoadFirstFrameTask()
    {
    }

    void LoadFirstFrameTask::run()
    {
        if (!QFile::exists(Path_))
        {
            emit loadedSignal(QPixmap(), QSize());
            return;
        }

        auto frame = Ui::VideoContext::getFirstFrame(Path_);
        const auto dur = Ui::VideoContext::getDuration(Path_);
        const auto audio = Ui::VideoContext::getGotAudio(Path_);

        assert(!frame.isNull());

        const auto frameSize = frame.size();
        emit loadedSignal(QPixmap::fromImage(std::move(frame)), frameSize);
        emit duration(dur);
        emit gotAudio(audio);
    }
}
