#pragma once

extern "C"
{
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavfilter/avfilter.h>
};

namespace Ui
{
    class BaseMpegLoader
    {
    public:

        BaseMpegLoader(const QString &file, bool isRc);
        ~BaseMpegLoader();

        bool open();
        void close();
        qint64 duration() const noexcept;
        qint32 frequency() const noexcept;

    private:
        bool open_file();
        static int read_rc(void* opaque, uint8_t* buf, int buf_size);
        static int64_t seek_rc(void* opaque, int64_t offset, int whence);

    protected:

        qint32 Freq_;
        qint64 Len_;
        AVFormatContext* FmtContext_;
        AVCodec* Codec_;
        qint32 StreamId_;
        QString File_;
        bool IsRc_;
        QFile Rc_;
        uchar* RcBuffer_;
        AVIOContext* RcContext_;
    };

    class MpegLoader : public BaseMpegLoader
    {
    public:

        MpegLoader(const QString &file, bool isRc);
        ~MpegLoader();

        bool open();
        qint32 format();
        int readMore(QByteArray &result, qint64 &samplesAdded);

    private:

        qint32 Fmt_;
        qint32 SrcRate_, DstRate_, MaxResampleSamples_, SampleSize_;
        uint8_t** OutSamplesData_;
        AVCodecContext* CodecContext_;
        AVPacket Avpkt_;
        AVSampleFormat InputFormat_;
        AVFrame* Frame_;
        SwrContext* SwrContext_;
    };
}
