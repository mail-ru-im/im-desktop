#include "stdafx.h"

#include "utils.h"

#include "LoadPixmapFromFileTask.h"

namespace Utils
{
    LoadPixmapFromFileTask::LoadPixmapFromFileTask(const QString& path, const QSize& _maxSize)
        : path_(path)
        , maxSize_(_maxSize)
    {
        assert(!path_.isEmpty());
        assert(QFile::exists(path_));
    }

    LoadPixmapFromFileTask::~LoadPixmapFromFileTask()
    {
    }

    void LoadPixmapFromFileTask::run()
    {
        if (Q_UNLIKELY(!QCoreApplication::instance()))
            return;
        if (!QFile::exists(path_))
        {
            Q_EMIT loadedSignal(QPixmap(), QSize());
            return;
        }
        QPixmap preview;
        QSize originalImageSize;

        if (!Utils::loadPixmapScaled(path_, maxSize_, preview, originalImageSize, Utils::PanoramicCheck::no))
        {
            if (Q_LIKELY(QCoreApplication::instance()))
                Q_EMIT loadedSignal(QPixmap(), QSize());
            return;
        }

        assert(!preview.isNull());

        if (Q_LIKELY(QCoreApplication::instance()))
            Q_EMIT loadedSignal(preview, originalImageSize);
    }

    LoadImageFromFileTask::LoadImageFromFileTask(const QString& path, const QSize& _maxSize)
        : path_(path)
        , maxSize_(_maxSize)
    {
        assert(!path_.isEmpty());
        assert(QFile::exists(path_));
    }

    LoadImageFromFileTask::~LoadImageFromFileTask()
    {
    }

    void LoadImageFromFileTask::run()
    {
        if (!QFile::exists(path_))
        {
            Q_EMIT loadedSignal(QImage(), QSize());
            return;
        }

        QImage preview;
        QSize originalImageSize;

        Utils::loadImageScaled(path_, maxSize_, preview, originalImageSize, Utils::PanoramicCheck::no);

        assert(!preview.isNull());

        Q_EMIT loadedSignal(preview, originalImageSize);
    }
}