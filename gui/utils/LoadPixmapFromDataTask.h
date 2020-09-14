#pragma once

#include "utils.h"

namespace core
{
    struct istream;
}

namespace Utils
{
    class LoadPixmapFromDataTask
        : public QObject
        , public QRunnable
    {
        Q_OBJECT

    Q_SIGNALS:
        void loadedSignal(const QPixmap& pixmap);

    public:
        LoadPixmapFromDataTask(core::istream *stream, const QSize& _maxSize = Utils::getMaxImageSize());

        virtual ~LoadPixmapFromDataTask();

        void run() override;

    private:
        core::istream *Stream_;
        QSize maxSize_;
    };

    class LoadImageFromDataTask
        : public QObject
        , public QRunnable
    {
        Q_OBJECT

    Q_SIGNALS:
        void loaded(const QImage& _image);
        void loadingFailed();

    public:
        LoadImageFromDataTask(core::istream* _stream, const QSize& _maxSize = Utils::getMaxImageSize());

        virtual ~LoadImageFromDataTask();

        void run() override;

    private:
        core::istream* stream_;
        QSize maxSize_;
    };
}

