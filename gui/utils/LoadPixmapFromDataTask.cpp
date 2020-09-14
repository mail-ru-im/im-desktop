#include "stdafx.h"

#include "../../corelib/collection_helper.h"

#include "../utils/utils.h"

#include "LoadPixmapFromDataTask.h"

namespace Utils
{
    LoadPixmapFromDataTask::LoadPixmapFromDataTask(core::istream *stream, const QSize& _maxSize)
        : Stream_(stream)
        , maxSize_(_maxSize)
    {
        assert(Stream_);

        Stream_->addref();
    }

    LoadPixmapFromDataTask::~LoadPixmapFromDataTask()
    {
        Stream_->release();
    }

    void LoadPixmapFromDataTask::run()
    {
        const auto size = Stream_->size();
        assert(size > 0);

        auto data = QByteArray::fromRawData((const char *)Stream_->read(size), (int)size);

        QPixmap preview;
        QSize originalSize;
        Utils::loadPixmapScaled(data, maxSize_, Out preview, Out originalSize);

        if (preview.isNull())
        {
            Q_EMIT loadedSignal(QPixmap());
            return;
        }

        Q_EMIT loadedSignal(preview);
    }

    LoadImageFromDataTask::LoadImageFromDataTask(core::istream* _stream, const QSize& _maxSize)
        : stream_(_stream),
          maxSize_(_maxSize)
    {
        assert(stream_);

        stream_->addref();
    }

    LoadImageFromDataTask::~LoadImageFromDataTask()
    {
        stream_->release();
    }

    void LoadImageFromDataTask::run()
    {
        const auto size = stream_->size();
        assert(size > 0);

        auto data = QByteArray::fromRawData((const char *)stream_->read(size), (int)size);

        QImage image;
        QSize originalSize;
        Utils::loadImageScaled(data, maxSize_, Out image, Out originalSize);

        if (!image.isNull())
            Q_EMIT loaded(image);
        else
            Q_EMIT loadingFailed();
    }

}
