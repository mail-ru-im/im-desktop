#include "stdafx.h"
#include "MpegLoader.h"

extern "C"
{
    #include <libavutil/opt.h>
};


namespace
{
    const int OutFrequency = 48000;
    const int BlockSize = 4096;
}

namespace Ui
{
    BaseMpegLoader::BaseMpegLoader(const QString &file, bool isRc)
        : Freq_(OutFrequency)
        , Len_(0)
        , FmtContext_(nullptr)
        , Codec_(nullptr)
        , StreamId_(0)
        , File_(QDir::toNativeSeparators(file))
        , IsRc_(isRc)
        , RcBuffer_(nullptr)
        , RcContext_(nullptr)
    {
    }

    int BaseMpegLoader::read_rc(void* opaque, uint8_t* buf, int buf_size)
    {
        BaseMpegLoader *l = reinterpret_cast<BaseMpegLoader*>(opaque);
        return int(l->Rc_.read((char*)(buf), buf_size));
    }

    int64_t BaseMpegLoader::seek_rc(void* opaque, int64_t offset, int whence)
    {
        BaseMpegLoader *l = reinterpret_cast<BaseMpegLoader*>(opaque);

        switch (whence)
        {
            case SEEK_SET: return l->Rc_.seek(offset) ? l->Rc_.pos() : -1;
            case SEEK_CUR: return l->Rc_.seek(l->Rc_.pos() + offset) ? l->Rc_.pos() : -1;
            case SEEK_END: return l->Rc_.seek(l->Rc_.size() + offset) ? l->Rc_.pos() : -1;
        }

        return -1;
    }

    bool BaseMpegLoader::open_file()
    {
        Rc_.setFileName(File_);
        if (!Rc_.open(QIODevice::ReadOnly))
            return false;

        return true;
    }

    bool BaseMpegLoader::open()
    {
        int res = 0;
        if (FmtContext_)
            close();

        if (IsRc_)
        {
            if (!open_file())
                return false;

            FmtContext_ = avformat_alloc_context();
            RcBuffer_ = (uchar*)av_malloc(BlockSize);
            RcContext_ = avio_alloc_context(RcBuffer_, BlockSize, 0, reinterpret_cast<void*>(this), &BaseMpegLoader::read_rc, 0, &BaseMpegLoader::seek_rc);
            if (!FmtContext_ || !RcBuffer_ || !RcContext_)
            {
                close();
                return false;
            }
            FmtContext_->pb = RcContext_;
            if ((res = avformat_open_input(&FmtContext_, 0, 0, 0)) < 0)
                return false;
        }
        else if ((res = avformat_open_input(&FmtContext_,File_.toStdString().c_str(), 0, 0)) < 0)
        {
            return false;
        }

        if ((res = avformat_find_stream_info(FmtContext_, 0)) < 0)
            return false;

        StreamId_ = av_find_best_stream(FmtContext_, AVMEDIA_TYPE_AUDIO, -1, -1, &Codec_, 0);
        if (StreamId_ < 0)
            return false;

        Freq_ = FmtContext_->streams[StreamId_]->codec->sample_rate;
        if (FmtContext_->streams[StreamId_]->duration == AV_NOPTS_VALUE)
            Len_ = (FmtContext_->duration * Freq_) / AV_TIME_BASE;
        else
            Len_ = (FmtContext_->streams[StreamId_]->duration * Freq_ * FmtContext_->streams[StreamId_]->time_base.num) / FmtContext_->streams[StreamId_]->time_base.den;

        return true;
    }

    qint64 BaseMpegLoader::duration() const noexcept
    {
        return Len_;
    }

    qint32 BaseMpegLoader::frequency() const noexcept
    {
        return Freq_;
    }

    void BaseMpegLoader::close()
    {
        if (FmtContext_)
        {
            avformat_close_input(&FmtContext_);
        } else if (RcBuffer_)
            av_freep(&RcBuffer_);
        if (RcContext_)
        {
            av_freep(&RcContext_->buffer);
            av_freep(&RcContext_);
        }
    }

    BaseMpegLoader::~BaseMpegLoader()
    {
        close();
    }

    static const AVSampleFormat OutFormat = AV_SAMPLE_FMT_S16;
    static const int64_t OutChannelLayout = AV_CH_LAYOUT_STEREO;
    static const qint32 OutChannels = 2;

    MpegLoader::MpegLoader(const QString &file, bool isRc)
        : BaseMpegLoader(file, isRc)
        , Fmt_(AL_FORMAT_STEREO16)
        , SrcRate_(OutFrequency)
        , DstRate_(OutFrequency)
        , MaxResampleSamples_(1024)
        , SampleSize_(2 * sizeof(quint16))
        , OutSamplesData_(nullptr)
        , CodecContext_(nullptr)
        , Frame_(nullptr)
        , SwrContext_(nullptr)
    {
        Frame_ = av_frame_alloc();
    }

    bool MpegLoader::open()
    {
        if (!BaseMpegLoader::open())
            return false;

        int res = 0;

        av_opt_set_int(FmtContext_->streams[StreamId_]->codec, "refcounted_frames", 1, 0);
        if ((res = avcodec_open2(FmtContext_->streams[StreamId_]->codec, Codec_, 0)) < 0)
            return false;

        CodecContext_ = FmtContext_->streams[StreamId_]->codec;

        uint64_t layout = CodecContext_->channel_layout;
        int channels = CodecContext_->channels;
        if (layout == 0 && channels > 0)
        {
            if (channels == 1)
                layout = AV_CH_LAYOUT_MONO;
            else
                layout = AV_CH_LAYOUT_STEREO;
        }
        InputFormat_ = CodecContext_->sample_fmt;
        switch (layout) {
        case AV_CH_LAYOUT_MONO:
            switch (InputFormat_)
            {
            case AV_SAMPLE_FMT_U8:
            case AV_SAMPLE_FMT_U8P:
                Fmt_ = AL_FORMAT_MONO8;
                SampleSize_ = 1;
                break;

            case AV_SAMPLE_FMT_S16:
            case AV_SAMPLE_FMT_S16P:
                Fmt_ = AL_FORMAT_MONO16;
                SampleSize_ = sizeof(quint16);
                break;

            default:
                SampleSize_ = -1;
                break;
            }
            break;

        case AV_CH_LAYOUT_STEREO:
            switch (InputFormat_)
            {
            case AV_SAMPLE_FMT_U8:
                Fmt_ = AL_FORMAT_STEREO8;
                SampleSize_ = 2;
                break;

            case AV_SAMPLE_FMT_S16:
                Fmt_ = AL_FORMAT_STEREO16;
                SampleSize_ = 2 * sizeof(quint16);
                break;

            default:
                SampleSize_ = -1;
                break;
            }
            break;

        default:
            SampleSize_ = -1;
            break;
        }

        if (Freq_ != 44100 && Freq_ != 48000)
            SampleSize_ = -1;

        if (SampleSize_ < 0)
        {
            SwrContext_ = swr_alloc();
            if (!SwrContext_)
                return false;

            int64_t src_ch_layout = layout, dst_ch_layout = OutChannelLayout;
            SrcRate_ = Freq_;
            AVSampleFormat src_sample_fmt = InputFormat_, dst_sample_fmt = OutFormat;
            DstRate_ = (Freq_ != 44100 && Freq_ != 48000) ? OutFrequency : Freq_;

            av_opt_set_int(SwrContext_, "in_channel_layout", src_ch_layout, 0);
            av_opt_set_int(SwrContext_, "in_sample_rate", SrcRate_, 0);
            av_opt_set_sample_fmt(SwrContext_, "in_sample_fmt", src_sample_fmt, 0);
            av_opt_set_int(SwrContext_, "out_channel_layout", dst_ch_layout, 0);
            av_opt_set_int(SwrContext_, "out_sample_rate", DstRate_, 0);
            av_opt_set_sample_fmt(SwrContext_, "out_sample_fmt", dst_sample_fmt, 0);

            if ((res = swr_init(SwrContext_)) < 0)
                return false;

            SampleSize_ = OutChannels * sizeof(short);
            Freq_ = DstRate_;
            Len_ = av_rescale_rnd(Len_, DstRate_, SrcRate_, AV_ROUND_UP);
            Fmt_ = AL_FORMAT_STEREO16;

            MaxResampleSamples_ = av_rescale_rnd(BlockSize / SampleSize_, DstRate_, SrcRate_, AV_ROUND_UP);
            if ((res = av_samples_alloc_array_and_samples(&OutSamplesData_, 0, OutChannels, MaxResampleSamples_, OutFormat, 0)) < 0)
                return false;
        }

        return true;
    }

    qint32 MpegLoader::format()
    {
        return Fmt_;
    }

    int MpegLoader::readMore(QByteArray &result, qint64 &samplesAdded)
    {
        int res;
        if ((res = av_read_frame(FmtContext_, &Avpkt_)) < 0)
            return -1;

        if (Avpkt_.stream_index == StreamId_)
        {
            av_frame_unref(Frame_);
            int got_frame = 0;
            if ((res = avcodec_decode_audio4(CodecContext_, Frame_, &got_frame, &Avpkt_)) < 0)
            {
                av_packet_unref(&Avpkt_);
                if (res == AVERROR_INVALIDDATA)
                    return 0;
                return -1;
            }

            if (got_frame)
            {
                if (OutSamplesData_)
                {
                    int64_t dstSamples = av_rescale_rnd(swr_get_delay(SwrContext_, SrcRate_) + Frame_->nb_samples, DstRate_, SrcRate_, AV_ROUND_UP);
                    if (dstSamples > MaxResampleSamples_)
                    {
                        MaxResampleSamples_ = dstSamples;
                        av_free(OutSamplesData_[0]);

                        if ((res = av_samples_alloc(OutSamplesData_, 0, OutChannels, MaxResampleSamples_, OutFormat, 1)) < 0)
                        {
                            OutSamplesData_[0] = 0;
                            av_packet_unref(&Avpkt_);
                            return -1;
                        }
                    }

                    if ((res = swr_convert(SwrContext_, OutSamplesData_, dstSamples, (const uint8_t**)Frame_->extended_data, Frame_->nb_samples)) < 0)
                    {
                        av_packet_unref(&Avpkt_);
                        return -1;
                    }

                    qint32 resultLen = av_samples_get_buffer_size(0, OutChannels, res, OutFormat, 1);
                    result.append((const char*)OutSamplesData_[0], resultLen);
                    samplesAdded += resultLen / SampleSize_;
                }
                else
                {
                    result.append((const char*)Frame_->extended_data[0], Frame_->nb_samples * SampleSize_);
                    samplesAdded += Frame_->nb_samples;
                }
            }
        }
        av_packet_unref(&Avpkt_);
        return 1;
    }

    MpegLoader::~MpegLoader()
    {
        if (CodecContext_)
            avcodec_close(CodecContext_);
        if (SwrContext_)
            swr_free(&SwrContext_);
        if (OutSamplesData_)
        {
            if (OutSamplesData_[0])
                av_freep(&OutSamplesData_[0]);
            av_freep(&OutSamplesData_);
        }
        av_frame_free(&Frame_);
    }
}
