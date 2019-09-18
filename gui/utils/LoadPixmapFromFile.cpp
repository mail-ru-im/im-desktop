#include "stdafx.h"

#include "utils.h"

#include "LoadPixmapFromFileTask.h"

namespace Utils
{
    LoadPixmapFromFileTask::LoadPixmapFromFileTask(const QString& path, const QSize& _maxSize)
        : Path_(path)
        , maxSize_(_maxSize)
    {
        assert(!Path_.isEmpty());
        assert(QFile::exists(Path_));
    }

    LoadPixmapFromFileTask::~LoadPixmapFromFileTask()
    {
    }

    void LoadPixmapFromFileTask::run()
    {
        if (!QFile::exists(Path_))
        {
            emit loadedSignal(QPixmap(), QSize());
            return;
        }

        QPixmap preview;
        QSize originalImageSize;

        Utils::loadPixmapScaled(Path_, maxSize_, preview, originalImageSize, Utils::PanoramicCheck::no);

        assert(!preview.isNull());

        emit loadedSignal(preview, originalImageSize);
    }
}