#include "stdafx.h"
#include "FFMpegPlayer.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "main_window/sounds/SoundsManager.h"

#include "FrameRenderer.h"

#ifdef __WIN32__
#include "win32/WindowRenderer.h"
#endif //__WIN32__

Q_LOGGING_CATEGORY(ffmpegPlayer, "ffmpegPlayer")

namespace
{
    double currentTimeInSeconds()
    {
        return (double)ffmpeg::av_gettime() / 1000000.0;
    }

    constexpr bool isCompressedType(Ui::thread_message_type _type) noexcept
    {
        return _type == Ui::thread_message_type::tmt_get_next_video_frame;
    }

    void alignedImageBufferCleanupHandler(void* data)
    {
        auto buffer = static_cast<uchar*>(data);
        delete[] buffer;
    }

    QImage createAlignedImage(QSize size, const int _align)
    {
        const auto width = size.width();
        const auto height = size.height();
        const auto widthalign = _align / 4;
        const auto neededwidth = width + ((width % widthalign) ? (widthalign - (width % widthalign)) : 0);
        const auto bytesperline = neededwidth * 4;
        auto buffer = new uchar[bytesperline * height + _align];
        auto cleanupdata = static_cast<void*>(buffer);
        auto bufferval = reinterpret_cast<uintptr_t>(buffer);
        auto alignedbuffer = buffer + ((bufferval % _align) ? (_align - (bufferval % _align)) : 0);

        return QImage(alignedbuffer, width, height, bytesperline, QImage::Format_ARGB32, alignedImageBufferCleanupHandler, cleanupdata);
    }

    bool isAlignedImage(const QImage& image, const int _align)
    {
        return !(reinterpret_cast<uintptr_t>(image.constBits()) % _align) && !(image.bytesPerLine() % _align);
    }

    constexpr std::chrono::milliseconds onDataReadyTimeout() noexcept { return std::chrono::milliseconds(50); }
}

class AVCodecScope
{
public:
    AVCodecScope()
    {
        ffmpeg::av_register_all();
        ffmpeg::avcodec_register_all();
    }
    ~AVCodecScope() { }
} g_AVCodecScope;

struct RLottieCacheDisable
{
    RLottieCacheDisable()
    {
        rlottie::configureModelCacheSize(0);
    }
} g_LottieCacheDisabler;

namespace Ui
{
    constexpr int32_t maxQueueSize(1024 * 1024 * 15);
    constexpr int32_t minFramesCount(25);

    static ffmpeg::AVPacket flush_pkt_data;
    bool isFlushPacket(const ffmpeg::AVPacket& _packet)
    {
        return _packet.data == (uint8_t*)&flush_pkt_data;
    }

    static ffmpeg::AVPacket quit_pkt;
    bool isQuitPacket(const ffmpeg::AVPacket& _packet)
    {
        return _packet.data == (uint8_t*)&quit_pkt;
    }

    constexpr int64_t empty_pts = -1000000;

    bool ThreadMessagesQueue::getMessage(ThreadMessage& _message, std::function<bool()> _isQuit, int32_t _wait_timeout)
    {
        condition_.tryAcquire(1, _wait_timeout);
        decltype(messages_) tmpList;

        {
            std::scoped_lock lock(queue_mutex_);

            if (_isQuit() || messages_.empty())
                return false;

            tmpList.splice(tmpList.cbegin(), messages_, messages_.cbegin());
        }

        _message = std::move(tmpList.front());

        return true;
    }

    bool ThreadMessagesQueue::getAllMessages(ThreadMessageList& _messages, std::function<bool()> _isQuit, int32_t _wait_timeout)
    {
        condition_.tryAcquire(1, _wait_timeout);
        decltype(messages_) tmpList;
        {
            std::scoped_lock lock(queue_mutex_);

            if (_isQuit() || messages_.empty())
                return false;

            _messages = std::exchange(messages_, std::move(tmpList));
        }
        return true;
    }

    void ThreadMessagesQueue::pushMessage(const ThreadMessage& _message, bool _forward, bool _clear_others)
    {
        decltype(messages_) tmpList;
        tmpList.push_back(_message);

        const auto compressedType = isCompressedType(_message.message_);
        {
            std::scoped_lock lock(queue_mutex_);

            const auto skipMessage = compressedType && std::any_of(messages_.cbegin(), messages_.cend(),
                [&_message](const auto& x)
                {
                    return x.videoId_ == _message.videoId_ && x.message_ == _message.message_;
                });

            if (!skipMessage)
            {
                if (_clear_others)
                    messages_.remove_if([id = _message.videoId_](const auto& x) { return x.videoId_ == id; });

                messages_.splice(_forward ? messages_.cbegin() : messages_.cend(), tmpList, tmpList.cbegin());
            }
        }

        condition_.release(1);
    }

    void ThreadMessagesQueue::clear()
    {
        decltype(messages_) tmpList;
        {
            std::scoped_lock lock(queue_mutex_);
            tmpList.splice(tmpList.cbegin(), messages_);
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // PacketQueue
    //////////////////////////////////////////////////////////////////////////
    PacketQueue::PacketQueue()
        :   packets_(0),
            size_(0)
    {
    }

    PacketQueue::~PacketQueue()
    {
        freeData();
    }

    void PacketQueue::freeData(std::function<void(ffmpeg::AVPacket& _packet)> _callback)
    {
        std::scoped_lock locker(mutex_);

        for (auto iter = list_.begin(); iter != list_.end(); ++iter)
        {
            if (_callback)
                _callback(*iter);

            if (iter->data && !isFlushPacket(*iter) && !isQuitPacket(*iter))
                ffmpeg::av_packet_unref(&(*iter));
        }

        list_.clear();
        size_ = 0;
        packets_ = 0;
    }

    void PacketQueue::push(ffmpeg::AVPacket* _packet)
    {
        decltype(list_) tmpList;
        tmpList.push_back(*_packet);
        {
            std::scoped_lock locker(mutex_);

            list_.splice(list_.end(), tmpList, tmpList.begin());

            ++packets_;

            if (!isFlushPacket(*_packet))
                size_ += _packet->size;
        }
    }

    bool PacketQueue::get(/*out*/ ffmpeg::AVPacket& _packet)
    {
        decltype(list_) tmpList;
        {
            std::scoped_lock locker(mutex_);

            if (list_.empty())
                return false;

            tmpList.splice(tmpList.begin(), list_, list_.begin());

            --packets_;
            size_ -= tmpList.front().size;
        }

        _packet = tmpList.front();

        return true;
    }

    int32_t PacketQueue::getSize() const
    {
        return size_;
    }

    int32_t PacketQueue::getPackets() const
    {
        return packets_;
    }


    MediaData::MediaData()
        : videoQueue_(QSharedPointer<PacketQueue>::create())
        , audioQueue_(QSharedPointer<PacketQueue>::create())
    {
    }


    //////////////////////////////////////////////////////////////////////////
    // VideoContext
    //////////////////////////////////////////////////////////////////////////
    VideoContext::VideoContext()
        : quit_(false)
        , curr_id_(0)
    {
        QObject::connect(this, &VideoContext::audioQuit, this, &VideoContext::onAudioQuit);
        QObject::connect(this, &VideoContext::videoQuit, this, &VideoContext::onVideoQuit);
        QObject::connect(this, &VideoContext::demuxQuit, this, &VideoContext::onDemuxQuit);
        QObject::connect(this, &VideoContext::streamsClosed, this, &VideoContext::onStreamsClosed);
    }

    VideoContext::~VideoContext() = default;

    void VideoContext::init(MediaData& _media)
    {
        ffmpeg::AVPacket flush_pkt_v, flush_pkt_a;
        initFlushPacket(flush_pkt_a);
        initFlushPacket(flush_pkt_v);

        pushVideoPacket(&flush_pkt_v, _media);
        pushAudioPacket(&flush_pkt_a, _media);
    }

    uint32_t VideoContext::addVideo(uint32_t _id)
    {
        const auto id = _id == 0 ? reserveId() : _id;
        {
            auto mediaData = std::make_shared<MediaData>();
            std::scoped_lock lock(mediaDataMutex_);
            mediaData_[id] = std::move(mediaData);
        }

        {
            std::scoped_lock lock(activeVideosMutex_);
            activeVideos_[id] = true;
        }

        return id;
    }

    uint32_t VideoContext::reserveId()
    {
        return ++curr_id_;
    }

    void VideoContext::deleteVideo(uint32_t _videoId)
    {
        {
            std::scoped_lock lock(mediaDataMutex_);
            mediaData_.erase(_videoId);
        }

        {
            std::scoped_lock lock(activeVideosMutex_);
            activeVideos_.erase(_videoId);
        }

        getMediaContainer()->stopMedia(_videoId);
    }

    ffmpeg::AVStream* VideoContext::openStream(int32_t _type, ffmpeg::AVFormatContext* _context)
    {
        ffmpeg::AVStream* stream = 0;

        im_assert(_context);

        int32_t streamIndex = ffmpeg::av_find_best_stream(_context, (ffmpeg::AVMediaType) _type, -1, -1, NULL, 0);
        if (streamIndex < 0)
        {
            return 0;
        }

        stream = _context->streams[streamIndex];

        ffmpeg::AVCodecContext* codecContext = stream->codec;

        // Find suitable codec
        ffmpeg::AVCodec* codec = ffmpeg::avcodec_find_decoder(codecContext->codec_id);

        if (!codec)
        {
            // Codec not found
            return 0;
        }

        if (avcodec_open2(codecContext, codec, 0) < 0)
        {
            // Failed to open codec
            return 0;
        }

        return stream;
    }

    void VideoContext::closeStream(ffmpeg::AVStream* _stream)
    {
        if (_stream && _stream->codec)
        {
            avcodec_close(_stream->codec);
        }
    }

    int32_t VideoContext::getVideoStreamIndex(MediaData& _media) const
    {
        return _media.videoStream_ ? _media.videoStream_->index : 0;
    }

    int32_t VideoContext::getAudioStreamIndex(MediaData& _media) const
    {
        if (_media.audioStream_)
            return _media.audioStream_->index;

        return -1;
    }

    bool VideoContext::isQuit() const
    {
        return quit_;
    }

    bool VideoContext::isVideoQuit(int _videoId) const
    {
        std::scoped_lock lock(activeVideosMutex_);

        const auto it = std::as_const(activeVideos_).find(_videoId);
        if (it == std::as_const(activeVideos_).end())
            return false;
        return !it->second;
    }

    void VideoContext::setVideoQuit(int _videoId)
    {
        std::scoped_lock lock(activeVideosMutex_);

        const auto it = activeVideos_.find(_videoId);
        if (it != activeVideos_.end())
            it->second = false;
    }

    void VideoContext::setQuit(bool _val)
    {
        quit_ = _val;
    }

    int32_t VideoContext::readAVPacket(/*OUT*/ffmpeg::AVPacket* _packet, MediaData& _media)
    {
        return ffmpeg::av_read_frame(_media.formatContext_, _packet);
    }

    int32_t VideoContext::readAVPacketPause(MediaData& _media)
    {
        return ffmpeg::av_read_pause(_media.formatContext_);
    }

    int32_t VideoContext::readAVPacketPlay(MediaData& _media)
    {
        return ffmpeg::av_read_play(_media.formatContext_);
    }

    bool VideoContext::isEof(int32_t _error, MediaData& _media)
    {
        return (_error == AVERROR_EOF || ffmpeg::avio_feof(_media.formatContext_->pb));
    }

    bool VideoContext::isStreamError(MediaData& _media)
    {
        if (_media.formatContext_->pb && _media.formatContext_->pb->error)
        {
            im_assert(false);
            return true;
        }

        return false;
    }

    void VideoContext::initFlushPacket(ffmpeg::AVPacket& _packet)
    {
        ffmpeg::av_init_packet(&_packet);

        _packet.data = (uint8_t*)&flush_pkt_data;

        _packet.dts = empty_pts;
        _packet.pts = empty_pts;
        _packet.size = 0;
    }

    bool VideoContext::getNextVideoFrame(/*OUT*/ffmpeg::AVFrame* _frame, ffmpeg::AVPacket* _packet, VideoData& _videoData, MediaData& _media, uint32_t _videoId)
    {
        if (!_media.videoStream_)
            return false;

        ffmpeg::AVCodecContext* videoCodecContext = _media.videoStream_->codec;

        while (!isVideoQuit(_videoId))
        {
            if (!_videoData.stream_finished_)
            {
                if (!_media.videoQueue_->get(*_packet))
                    continue;
            }

            if (isFlushPacket(*_packet))
            {
                if (_packet->dts != empty_pts)
                {
                    _media.seek_position_ = _packet->dts;

                    Q_EMIT seekedV(_videoId);
                }

                flushVideoBuffers(_media);

                continue;
            }

            if (isQuitPacket(*_packet))
                return false;

            if (!_packet->data)
                _videoData.stream_finished_ = true;

            int got_frame = 0;

            int len = ffmpeg::avcodec_decode_video2(videoCodecContext, _frame, &got_frame, _packet);

            if (_packet->data)
                ffmpeg::av_packet_unref(_packet);

            if (len < 0)
                return false;

            if (got_frame)
            {
                if (_media.seek_position_ > 0)
                {
                    if (_frame->pkt_dts < _media.seek_position_)
                    {
                        ffmpeg::av_frame_unref(_frame);
                        continue;
                    }
                }

                _media.seek_position_ = 0;

                return true;
            }
            else if (_videoData.stream_finished_)
            {
                _videoData.eof_ = true;

                return false;
            }
        }

        return false;
    }

    void pushNullPacket(PacketQueue& _queue, int32_t _streamIndex)
    {
        ffmpeg::AVPacket pkt1, *pkt = &pkt1;
        ffmpeg::av_init_packet(pkt);
        pkt->data = NULL;
        pkt->size = 0;
        pkt->stream_index = _streamIndex;

        _queue.push(pkt);
    }

    void VideoContext::pushVideoPacket(ffmpeg::AVPacket* _packet, MediaData& _media)
    {
        if (!_packet)
            return pushNullPacket(*(_media.videoQueue_), getVideoStreamIndex(_media));

        return _media.videoQueue_->push(_packet);
    }

    int32_t VideoContext::getVideoQueuePackets(MediaData& _media) const
    {
        return _media.videoQueue_->getPackets();
    }

    int32_t VideoContext::getVideoQueueSize(MediaData& _media) const
    {
        return _media.videoQueue_->getSize();
    }

    void VideoContext::pushAudioPacket(ffmpeg::AVPacket* _packet, MediaData& _media)
    {
        if (!_packet)
            return pushNullPacket((*_media.audioQueue_), getAudioStreamIndex(_media));

        return _media.audioQueue_->push(_packet);
    }

    int32_t VideoContext::getAudioQueuePackets(MediaData& _media) const
    {
        return _media.audioQueue_->getPackets();
    }

    int32_t VideoContext::getAudioQueueSize(MediaData& _media) const
    {
        return _media.audioQueue_->getSize();
    }

    void VideoContext::clearAudioQueue(MediaData& _media)
    {
        _media.audioQueue_->freeData();
    }

    void VideoContext::clearVideoQueue(MediaData& _media)
    {
        _media.videoQueue_->freeData();
    }

    void VideoContext::cleanupOnExit()
    {
        std::scoped_lock lock(mediaDataMutex_);

        for (const auto& [id, data] : mediaData_)
        {
            qCDebug(ffmpegPlayer) << "cleanup active media id = " << id;

            clearAudioQueue(*data);
            clearVideoQueue(*data);

            if (data->streamClosed_)
                continue;

            data->streamClosed_ = true;

            closeFile(*data);
        }
    }

    QImage VideoContext::getFirstFrame(const QString& _file)
    {
        static std::mutex firstFrameMutex_;
        std::scoped_lock lock(firstFrameMutex_);

        QImage result;

        ffmpeg::AVFormatContext* ctx = nullptr;
        ffmpeg::AVStream* videoStream = nullptr;
        ffmpeg::AVCodecContext* codecContext = nullptr;

        do
        {
            int32_t err = ffmpeg::avformat_open_input(&ctx, _file.toStdString().c_str(), 0, 0);
            if (err < 0)
                break;

            err = ffmpeg::avformat_find_stream_info(ctx, 0);
            if (err < 0)
                break;

            videoStream = openStream(ffmpeg::AVMEDIA_TYPE_VIDEO, ctx);
            if (!videoStream)
                break;

            codecContext = videoStream->codec;

            ffmpeg::AVDictionary* dictionary = videoStream->metadata;

            int32_t rotation = 0;

            if (dictionary)
            {
                ffmpeg::AVDictionaryEntry* entryRotate = ffmpeg::av_dict_get(dictionary, "rotate", 0, AV_DICT_IGNORE_SUFFIX);

                if (entryRotate && entryRotate->value && entryRotate->value[0] != '\0')
                {
                    rotation = QString::fromLatin1(entryRotate->value).toInt();
                }
            }

            int32_t width = codecContext->width;
            int32_t height = codecContext->height;
            int32_t videoStreamIndex = videoStream->index;

            ffmpeg::AVPacket packet;
            ffmpeg::AVFrame* frame = ffmpeg::av_frame_alloc();

            while (ffmpeg::av_read_frame(ctx, &packet) >= 0)
            {
                if (packet.stream_index == videoStreamIndex)
                {
                    int got_frame = 0;

                    int len = ffmpeg::avcodec_decode_video2(codecContext, frame, &got_frame, &packet);

                    if (len < 0)
                    {
                        ffmpeg::av_packet_unref(&packet);
                        break;
                    }

                    if (got_frame)
                    {
                        QSize scaledSize(width, height);

                        int align = 256;
                        while (1)
                        {
                            if (frame->linesize[0] % align == 0)
                                break;

                            align = align / 2;
                        }

                        ffmpeg::AVFrame* frameRGB = ffmpeg::av_frame_alloc();
                        int numBytes = ffmpeg::av_image_get_buffer_size(ffmpeg::AV_PIX_FMT_RGBA, scaledSize.width(), scaledSize.height(), align);
                        std::vector<uint8_t> scaledBuffer(numBytes);
                        av_image_fill_arrays(frameRGB->data, frameRGB->linesize, &scaledBuffer[0], ffmpeg::AV_PIX_FMT_RGBA, scaledSize.width(), scaledSize.height(), align);

                        ffmpeg::SwsContext* swsContext = nullptr;
                        swsContext = sws_getCachedContext(swsContext, frame->width, frame->height, ffmpeg::AVPixelFormat(frame->format), scaledSize.width(), scaledSize.height(), ffmpeg::AV_PIX_FMT_RGBA, SWS_POINT, 0, 0, 0);

                        ffmpeg::sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, frameRGB->data, frameRGB->linesize);

                        QImage lastFrame(scaledSize.width(), scaledSize.height(), QImage::Format_RGBA8888);

                        for (int32_t y = 0; y < scaledSize.height(); ++y)
                        {
                            memcpy(lastFrame.scanLine(y), frameRGB->data[0] + y * frameRGB->linesize[0], scaledSize.width() * 4);
                        }

                        if (rotation)
                        {
                            lastFrame = lastFrame.transformed(QTransform().rotate(rotation));
                        }

                        result = lastFrame;

                        ffmpeg::av_frame_unref(frameRGB);
                        ffmpeg::av_frame_free(&frameRGB);

                        sws_freeContext(swsContext);

                        ffmpeg::av_packet_unref(&packet);
                        break;
                    }
                }

                ffmpeg::av_packet_unref(&packet);
            }

            ffmpeg::av_frame_unref(frame);
            ffmpeg::av_frame_free(&frame);

        } while (false);

        if (videoStream)
            closeStream(videoStream);

        if (ctx)
            avformat_close_input(&ctx);

        return result;
    }

    int64_t VideoContext::getDuration(const QString& _file)
    {
        static std::mutex durationMutext;
        std::scoped_lock lock(durationMutext);

        int64_t result = -1;

        ffmpeg::AVFormatContext* ctx = nullptr;

        do
        {
            int32_t err = ffmpeg::avformat_open_input(&ctx, _file.toStdString().c_str(), 0, 0);
            if (err < 0)
                break;

            err = ffmpeg::avformat_find_stream_info(ctx, 0);
            if (err < 0)
                break;

            result = ctx->duration / AV_TIME_BASE;

        } while (false);

        if (ctx)
            avformat_close_input(&ctx);

        return result;
    }

    bool VideoContext::getGotAudio(const QString& _file)
    {
        static std::mutex gotAudioMutext;
        std::scoped_lock lock(gotAudioMutext);

        bool result = true;

        ffmpeg::AVFormatContext* ctx = nullptr;

        int32_t err = ffmpeg::avformat_open_input(&ctx, _file.toStdString().c_str(), 0, 0);
        if (err >= 0)
        {
            int32_t streamIndex = ffmpeg::av_find_best_stream(ctx, ffmpeg::AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
            result = streamIndex >= 0;
        }

        if (ctx)
            avformat_close_input(&ctx);

        return result;
    }

    bool VideoContext::openStreams(uint32_t _mediaId, const QString& _file, MediaData& _media)
    {
        //qDebug() << "openStreams " << _mediaId;
        int32_t err = 0;

        if (_media.isLottie_)
        {
            _media.lottieRenderer_ = LottieHandleCache::instance().getHandle(_file);
            if (!_media.lottieRenderer_)
            {
                Q_EMIT streamsOpenFailed(_mediaId);
                return false;
            }

            _media.duration_ = std::ceil(_media.lottieRenderer_->duration());
            _media.videoClock_ = 1. / _media.lottieRenderer_->frameRate();
            _media.frameLastDelay_ = _media.videoClock_;
            _media.size_ = _media.lottieRenderer_.originalSize();
            _media.scaledSize_ = _media.size_;
        }
        else
        {
            err = ffmpeg::avformat_open_input(&_media.formatContext_, _file.toStdString().c_str(), 0, 0);
            if (err < 0)
            {
                Q_EMIT streamsOpenFailed(_mediaId);
                return false;
            }

            // Retrieve stream information
            err = ffmpeg::avformat_find_stream_info(_media.formatContext_, 0);
            if (err < 0)
            {
                Q_EMIT streamsOpenFailed(_mediaId);
                return false;
            }

            // Open video and audio streams
            _media.videoStream_ = openStream(ffmpeg::AVMEDIA_TYPE_VIDEO, _media.formatContext_);

            if (!_media.videoStream_)
            {
                Q_EMIT streamsOpenFailed(_mediaId);
                return false;
            }

            _media.codecContext_ = _media.videoStream_->codec;

            _media.audioStream_ = openStream(ffmpeg::AVMEDIA_TYPE_AUDIO, _media.formatContext_);
        }

        Q_EMIT streamsOpened(_mediaId);
        return true;
    }

    bool VideoContext::openFile(MediaData& _media)
    {
        if (_media.isLottie_)
        {
            if (!_media.lottieRenderer_)
                return false;
        }
        else
        {
            // Create conversion context
            ffmpeg::AVCodecContext* videoCodecContext = _media.videoStream_->codec;

            ffmpeg::AVDictionary* dictionary = _media.videoStream_->metadata;

            if (dictionary)
            {
                ffmpeg::AVDictionaryEntry* entryRotate = ffmpeg::av_dict_get(dictionary, "rotate", 0, AV_DICT_IGNORE_SUFFIX);

                if (entryRotate && entryRotate->value && entryRotate->value[0] != '\0')
                {
                    _media.rotation_ = QString::fromLatin1(entryRotate->value).toInt();
                }
            }

            _media.size_ = { videoCodecContext->width, videoCodecContext->height };
            _media.duration_ = _media.formatContext_->duration / (AV_TIME_BASE / 1000);
            _media.scaledSize_ = _media.size_;
        }

        resetFrameTimer(_media);

        return true;
    }

    void VideoContext::closeFile(MediaData& _media)
    {
        if (_media.isLottie_)
        {
            _media.lottieRenderer_ = LottieHandle();
        }
        else
        {
            closeStream(_media.audioStream_);
            closeStream(_media.videoStream_);
            _media.audioStream_ = nullptr;
            _media.videoStream_ = nullptr;

            if (_media.formatContext_)
                avformat_close_input(&_media.formatContext_);
        }
    }

    int32_t VideoContext::getWidth(MediaData& _media) const
    {
        return _media.size_.width();
    }

    int32_t VideoContext::getHeight(MediaData& _media) const
    {
        return _media.size_.height();
    }

    int32_t VideoContext::getRotation(MediaData& _media) const
    {
        return _media.rotation_;
    }

    int64_t VideoContext::getDuration(MediaData& _media) const
    {
        return _media.duration_;
    }

    QSize VideoContext::getSourceSize(MediaData& _media) const
    {
        return _media.size_;
    }

    QSize VideoContext::getTargetSize(MediaData& _media) const
    {
        return _media.scaledSize_;
    }

    double VideoContext::getVideoTimebase(MediaData& _media)
    {
        return ffmpeg::av_q2d(_media.videoStream_->time_base);
    }

    double VideoContext::getAudioTimebase(MediaData& _media)
    {
        return ffmpeg::av_q2d(_media.audioStream_->time_base);
    }

    double VideoContext::synchronizeVideo(ffmpeg::AVFrame* _frame, double _pts, MediaData& _media)
    {
        ffmpeg::AVCodecContext* videoCodecContext = _media.videoStream_->codec;
        if (_pts > std::numeric_limits<double>::epsilon())
        {
            /* if we have pts, set video clock to it */
            _media.videoClock_ = _pts;
        }
        else
        {
            /* if we aren't given a pts, set it to the clock */
            _pts = _media.videoClock_;
        }
        /* update the video clock */
        double frameDelay = ffmpeg::av_q2d(videoCodecContext->time_base);
        /* if we are repeating a frame, adjust clock accordingly */

        frameDelay += _frame->repeat_pict * (frameDelay * 0.5);
        _media.videoClock_ += frameDelay;

        return _pts;
    }

    double VideoContext::computeDelay(double _picturePts, MediaData& _media)
    {
        if (_media.isLottie_)
        {
            const auto delay = std::max(_media.videoClock_, 0.01);
            _media.frameTimer_ += delay;
            _media.frameLastDelay_ = delay;
            return delay;
        }

        double delay = _picturePts - _media.frameLastPts_;

        if (std::exchange(_media.frameLastPtsReset_, false) || delay <= 0.0 || delay >= 10.0)
        {
            qCDebug(ffmpegPlayer) << "!!! delay incorrect" << delay << "->" << _media.frameLastDelay_;

            // Delay is incorrect or last frame pts was reset - use previous one
            delay = _media.frameLastDelay_;
        }

        // Save for next time
        _media.frameLastPts_ = _picturePts;
        _media.frameLastDelay_ = delay;
        _media.frameTimer_ += delay;

        qCDebug(ffmpegPlayer) << "media frame timer = " << int(_media.frameTimer_);

        double actual_delay = _media.frameTimer_ - currentTimeInSeconds();

        int64_t current_time = ffmpeg::av_gettime();

        qCDebug(ffmpegPlayer) << "actual delay = " << int(actual_delay * 1000.0);

        if (_media.syncWithAudio_)
        {
            int64_t timeDiff = current_time - _media.audioClockTime_;
            double timeDiff_d = ((double)timeDiff) / (double)1000000.0;
            double pts_audio = _media.audioClock_ + timeDiff_d;

            qCDebug(ffmpegPlayer) << "_picture_pts = " << int(_picturePts * 1000.0);
            qCDebug(ffmpegPlayer) << "pts_audio = " << int(pts_audio * 1000.0);

            double diff = _picturePts - pts_audio;

            if (fabs(diff) > 0.010)
            {
                double correction = std::min(fabs(actual_delay * 0.1), fabs(diff));
                if (diff < 0.0)
                    correction *= -1.0;

                actual_delay += correction;

                _media.frameTimer_ += correction;

                qCDebug(ffmpegPlayer) << "sync with audio diff = " << int(diff * 1000.0);
                qCDebug(ffmpegPlayer) << "actual delay after corrections = " << int(actual_delay * 1000.0);
            }
        }

        return std::max(actual_delay, 0.01);
    }

    bool VideoContext::initDecodeAudioData(MediaData& _media)
    {
        if (!_media.hasAudio())
            return true;

        _media.audioData_.frame_ = ffmpeg::av_frame_alloc();

        // Generate some AL Buffers for streaming
        openal::alGenBuffers(audio::num_buffers, _media.audioData_.uiBuffers_);

        // Generate a Source to playback the Buffers
        openal::alGenSources(1, &_media.audioData_.uiSource_);

        _media.audioData_.audioCodecContext_ = _media.audioStream_->codec;

        _media.audioData_.layout_ = _media.audioData_.audioCodecContext_->channel_layout;
        _media.audioData_.channels_ = _media.audioData_.audioCodecContext_->channels;
        _media.audioData_.freq_ = _media.audioData_.audioCodecContext_->sample_rate;

        if (_media.audioData_.layout_ == 0 && _media.audioData_.channels_ > 0)
        {
            if (_media.audioData_.channels_ == 1)
                _media.audioData_.layout_ = AV_CH_LAYOUT_MONO;
            else
                _media.audioData_.layout_ = AV_CH_LAYOUT_STEREO;
        }

        ffmpeg::AVSampleFormat inputFormat = _media.audioData_.audioCodecContext_->sample_fmt;

        switch (_media.audioData_.layout_)
        {
        case AV_CH_LAYOUT_MONO:
            switch (inputFormat)
            {
            case ffmpeg::AV_SAMPLE_FMT_U8:
            case ffmpeg::AV_SAMPLE_FMT_U8P:
                _media.audioData_.fmt_ = AL_FORMAT_MONO8;
                _media.audioData_.sampleSize_ = 1;
                break;

            case ffmpeg::AV_SAMPLE_FMT_S16:
            case ffmpeg::AV_SAMPLE_FMT_S16P:
                _media.audioData_.fmt_ = AL_FORMAT_MONO16;
                _media.audioData_.sampleSize_ = sizeof(uint16_t);
                break;

            default:
                _media.audioData_.sampleSize_ = -1;
                break;
            }
            break;

        case AV_CH_LAYOUT_STEREO:
            switch (inputFormat)
            {
            case ffmpeg::AV_SAMPLE_FMT_U8:
                _media.audioData_.fmt_ = AL_FORMAT_STEREO8;
                _media.audioData_.sampleSize_ = 2;
                break;

            case ffmpeg::AV_SAMPLE_FMT_S16:
                _media.audioData_.fmt_ = AL_FORMAT_STEREO16;
                _media.audioData_.sampleSize_ = 2 * sizeof(uint16_t);
                break;

            default:
                _media.audioData_.sampleSize_ = -1;
                break;
            }
            break;

        default:
            _media.audioData_.sampleSize_ = -1;
            break;
        }

        if (_media.audioData_.freq_ != 44100 && _media.audioData_.freq_ != 48000)
            _media.audioData_.sampleSize_ = -1;

        if (_media.audioData_.sampleSize_ < 0)
        {
            _media.audioData_.swrContext_ = ffmpeg::swr_alloc();
            if (!_media.audioData_.swrContext_)
                return false;

            int64_t src_ch_layout = _media.audioData_.layout_, dst_ch_layout = audio::outChannelLayout;
            _media.audioData_.srcRate_ = _media.audioData_.freq_;
            ffmpeg::AVSampleFormat src_sample_fmt = inputFormat, dst_sample_fmt = audio::outFormat;
            _media.audioData_.dstRate_ = (_media.audioData_.freq_ != 44100 && _media.audioData_.freq_ != 48000) ? audio::outFrequency : _media.audioData_.freq_;

            ffmpeg::av_opt_set_int(_media.audioData_.swrContext_, "in_channel_layout", src_ch_layout, 0);
            ffmpeg::av_opt_set_int(_media.audioData_.swrContext_, "in_sample_rate", _media.audioData_.srcRate_, 0);
            ffmpeg::av_opt_set_sample_fmt(_media.audioData_.swrContext_, "in_sample_fmt", src_sample_fmt, 0);
            ffmpeg::av_opt_set_int(_media.audioData_.swrContext_, "out_channel_layout", dst_ch_layout, 0);
            ffmpeg::av_opt_set_int(_media.audioData_.swrContext_, "out_sample_rate", _media.audioData_.dstRate_, 0);
            ffmpeg::av_opt_set_sample_fmt(_media.audioData_.swrContext_, "out_sample_fmt", dst_sample_fmt, 0);

            if (swr_init(_media.audioData_.swrContext_) < 0)
                return false;

            _media.audioData_.sampleSize_ = audio::outChannels * sizeof(uint16_t);
            _media.audioData_.freq_ = _media.audioData_.dstRate_;
            //!!!int64_t len_ = ffmpeg::av_rescale_rnd(Len_, DstRate_, SrcRate_, AV_ROUND_UP);
            _media.audioData_.fmt_ = AL_FORMAT_STEREO16;

            _media.audioData_.maxResampleSamples_ = ffmpeg::av_rescale_rnd(audio::blockSize / _media.audioData_.sampleSize_, _media.audioData_.dstRate_, _media.audioData_.srcRate_, ffmpeg::AV_ROUND_UP);
            if (ffmpeg::av_samples_alloc_array_and_samples(&_media.audioData_.outSamplesData_, 0, audio::outChannels, _media.audioData_.maxResampleSamples_, audio::outFormat, 0) < 0)
                return false;
        }

        return true;
    }

    void VideoContext::freeDecodeAudioData(MediaData& _media)
    {
        if (_media.audioData_.frame_)
            ffmpeg::av_frame_free(&_media.audioData_.frame_);

        if (_media.audioData_.swrContext_)
            ffmpeg::swr_free(&_media.audioData_.swrContext_);

        if (_media.audioData_.outSamplesData_)
        {
            ffmpeg::av_freep(&_media.audioData_.outSamplesData_[0]);
            ffmpeg::av_freep(&_media.audioData_.outSamplesData_);
        }

        openal::alDeleteSources(1, &_media.audioData_.uiSource_);
        openal::alDeleteBuffers(audio::num_buffers, _media.audioData_.uiBuffers_);
    }

    bool VideoContext::readFrameAudio(
        ffmpeg::AVPacket* _packet,
        AudioData& _audioData,
        openal::ALvoid** _frameData,
        openal::ALsizei& _frameDataSize,
        bool& _flush,
        int& _seekCount,
        MediaData& _media,
        uint32_t _videoId)
    {
        int64_t seek_position_audio = -1;

        for (;;)
        {
            if (isVideoQuit(_videoId))
                return false;

            if (!_media.audioQueue_->get(*_packet))
            {
                continue;
            }

            if (!_packet->data)
            {
                _audioData.stream_finished_ = true;
            }
            else if (isQuitPacket(*_packet))
            {
                return false;
            }
            else if (isFlushPacket(*_packet))
            {
                _flush = true;

                //qDebug() << "Audio thread: start seek";

                flushAudioBuffers(_media);

                if (_packet->pts != empty_pts)
                {
                    seek_position_audio = _packet->pts;

                    ++_seekCount;
                }

                continue;
            }

            int got_frame = 0;

            if (!_media.audioData_.audioCodecContext_)
                return false;

            int len = avcodec_decode_audio4(_media.audioData_.audioCodecContext_, _media.audioData_.frame_, &got_frame, _packet);

            if (_packet->data)
            {
                ffmpeg::av_packet_unref(_packet);
            }

            if (len < 0)
                return false;

            if (!got_frame)
            {
                if (_audioData.stream_finished_)
                {
                    _audioData.eof_ = true;

                    return false;
                }

                continue;
            }
            else
            {
                setStartTimeAudio(_media.audioData_.frame_->pkt_dts, _media);

                if (seek_position_audio > 0 && _media.audioData_.frame_->pkt_dts < seek_position_audio)
                {
                    ffmpeg::av_frame_unref(_media.audioData_.frame_);

                    continue;
                }

                seek_position_audio = 0;
            }

            if (_media.audioData_.outSamplesData_)
            {
                int64_t delay = ffmpeg::swr_get_delay(_media.audioData_.swrContext_, _media.audioData_.srcRate_);

                int64_t dstSamples = ffmpeg::av_rescale_rnd(delay + _media.audioData_.frame_->nb_samples, _media.audioData_.dstRate_, _media.audioData_.srcRate_, ffmpeg::AV_ROUND_UP);

                if (dstSamples > _media.audioData_.maxResampleSamples_)
                {
                    _media.audioData_.maxResampleSamples_ = dstSamples;
                    ffmpeg::av_free(_media.audioData_.outSamplesData_[0]);

                    if (ffmpeg::av_samples_alloc(_media.audioData_.outSamplesData_, 0, audio::outChannels, _media.audioData_.maxResampleSamples_, audio::outFormat, 1) < 0)
                    {
                        _media.audioData_.outSamplesData_[0] = 0;

                        return false;
                    }
                }

                int32_t res = 0;
                if ((res = ffmpeg::swr_convert(_media.audioData_.swrContext_, _media.audioData_.outSamplesData_, dstSamples, (const uint8_t**)_media.audioData_.frame_->extended_data, _media.audioData_.frame_->nb_samples)) < 0)
                    return false;

                qint32 resultLen = ffmpeg::av_samples_get_buffer_size(0, audio::outChannels, res, audio::outFormat, 1);

                *_frameData = _media.audioData_.outSamplesData_[0];
                _frameDataSize = resultLen;
            }
            else
            {
                *_frameData = _media.audioData_.frame_->extended_data[0];
                _frameDataSize = _media.audioData_.frame_->nb_samples * _media.audioData_.sampleSize_;
            }

            return true;
        }
    }

    static void stopPttIfNeeded(const MediaData& _media)
    {
        if (!_media.mute_ && _media.volume_ > 0)
            Q_EMIT Utils::InterConnector::instance().stopPttRecord();
    }

    bool VideoContext::playNextAudioFrame(
        ffmpeg::AVPacket* _packet,
        /*in out*/ AudioData& _audioData,
        bool& _flush,
        int& _seekCount,
        MediaData& _media,
        uint32_t _videoId)
    {
        qCDebug(ffmpegPlayer) << "playNextAudioFrame";

        Ui::GetSoundsManager()->checkAudioDevice();

        openal::ALint iBuffersProcessed = 0;

        openal::ALvoid* frameData = 0;
        openal::ALsizei frameDataSize = 0;

        // set volume
        openal::ALfloat volume = (_media.mute_ ? (0.0) : (float(_media.volume_) / 100.0));
        openal::alSourcef(_media.audioData_.uiSource_, AL_GAIN, volume);

        openal::ALint iState = 0;
        openal::ALint iQueuedBuffers = 0;

        if (!_media.audioData_.queueInited_)
        {
            for (int i = 0; i < audio::num_buffers; ++i)
            {
                _flush = false;
                if (!readFrameAudio(_packet, _audioData, &frameData, frameDataSize, _flush, _seekCount, _media, _videoId))
                    return false;

                if (_flush)
                {
                    flushBuffers(_media);
                    return true;
                }

                openal::alBufferData(_media.audioData_.uiBuffers_[i], _media.audioData_.fmt_, frameData, frameDataSize, _media.audioData_.freq_);
                openal::alSourceQueueBuffers(_media.audioData_.uiSource_, 1, &_media.audioData_.uiBuffers_[i]);

                _media.audio_queue_ptss_.push(_media.audioData_.frame_->pkt_pts);
            }

            _media.audioData_.queueInited_ = true;
        }

        openal::alGetSourcei(_media.audioData_.uiSource_, AL_BUFFERS_PROCESSED, &iBuffersProcessed);

        while (iBuffersProcessed)
        {
            _media.audioData_.uiBuffer_ = 0;
            openal::alSourceUnqueueBuffers(_media.audioData_.uiSource_, 1, &_media.audioData_.uiBuffer_);

            _flush = false;
            if (!readFrameAudio(_packet, _audioData, &frameData, frameDataSize, _flush, _seekCount, _media, _videoId))
                return false;

            if (_flush)
            {
                flushBuffers(_media);
                return true;
            }

            openal::alBufferData(_media.audioData_.uiBuffer_, _media.audioData_.fmt_, frameData, frameDataSize, _media.audioData_.freq_);
            openal::alSourceQueueBuffers(_media.audioData_.uiSource_, 1, &_media.audioData_.uiBuffer_);

            _media.audio_queue_ptss_.push(_media.audioData_.frame_->pkt_pts);

            iBuffersProcessed--;

            _media.audio_queue_ptss_.pop();
        }

        openal::ALfloat offset = 0;
        openal::alGetSourcef(_media.audioData_.uiSource_, AL_SEC_OFFSET, &offset);

        if (!_media.audio_queue_ptss_.empty())
        {
            double pts = _media.audio_queue_ptss_.front();
            if (pts == AV_NOPTS_VALUE)
            {
                pts = 0.0;
            }

            pts *= getAudioTimebase(_media);

            pts += offset;

            _audioData.offset_ = pts;

            qCDebug(ffmpegPlayer) << "_audioData.offset_ = " << _audioData.offset_;
        }
        else
        {
            qCDebug(ffmpegPlayer) << "_media.audio_queue_ptss_.empty() = true";
        }

        openal::alGetSourcei(_media.audioData_.uiSource_, AL_SOURCE_STATE, &iState);

        const auto time_now = std::chrono::system_clock::now();

        static auto lastDelayTime = time_now;

        if ((time_now - lastDelayTime) > std::chrono::milliseconds(1000))
        {
            lastDelayTime = time_now;

            Ui::GetSoundsManager()->delayDeviceTimer();
        }

        if (iState != AL_PLAYING)
        {
            // If there are Buffers in the Source Queue then the Source was starved of audio
            // data, so needs to be restarted (because there is more audio data to play)

            openal::alGetSourcei(_media.audioData_.uiSource_, AL_BUFFERS_QUEUED, &iQueuedBuffers);
            if (iQueuedBuffers)
            {
                stopPttIfNeeded(_media);
                Ui::GetSoundsManager()->sourcePlay(_media.audioData_.uiSource_);
            }
            else
            {
                // Finished playing
                return true;
            }
        }

        return true;
    }

    void VideoContext::flushBuffers(MediaData& _media)
    {
        openal::alSourceStop(_media.audioData_.uiSource_);

        openal::alDeleteSources(1, &_media.audioData_.uiSource_);
        openal::alDeleteBuffers(audio::num_buffers, _media.audioData_.uiBuffers_);

        openal::alGenBuffers(audio::num_buffers, _media.audioData_.uiBuffers_);
        openal::alGenSources(1, &_media.audioData_.uiSource_);

        _media.audioData_.queueInited_ = false;

        _media.audio_queue_ptss_ = {};
    }

    void VideoContext::cleanupAudioBuffers(MediaData& _media)
    {
        _media.audioData_.buffersProcessed_ = 0;
        openal::alGetSourcei(_media.audioData_.uiSource_, AL_BUFFERS_PROCESSED, &_media.audioData_.buffersProcessed_);

        while (_media.audioData_.buffersProcessed_)
        {
            _media.audioData_.uiBuffer_ = 0;
            openal::alSourceUnqueueBuffers(_media.audioData_.uiSource_, 1, &_media.audioData_.uiBuffer_);

            --_media.audioData_.buffersProcessed_;
        }
    }

    void VideoContext::suspendAudio(MediaData& _media)
    {
        openal::alSourcePause(_media.audioData_.uiSource_);
    }

    void VideoContext::stopAudio(MediaData& _media)
    {
        openal::alSourceStop(_media.audioData_.uiSource_);
    }

    void VideoContext::updateScaledVideoSize(uint32_t _videoId, const QSize& _sz)
    {
        ThreadMessage msg(_videoId, thread_message_type::tmt_update_scaled_size);

        msg.x_ = _sz.width();
        msg.y_ = _sz.height();

        if (auto mediaData = getMediaData(_videoId))
        {
            const auto rotation = getRotation(*mediaData);
            if (rotation == 90 || rotation == 270)
                std::swap(msg.x_, msg.y_);
        }

        postVideoThreadMessage(msg, false);
    }

    void VideoContext::postVideoThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others)
    {
        videoThreadMessagesQueue_.pushMessage(_message, _forward, _clear_others);
    }

    void VideoContext::postDemuxThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others)
    {
        demuxThreadMessageQueue_.pushMessage(_message, _forward, _clear_others);
    }

    bool VideoContext::getDemuxThreadMessage(ThreadMessage& _message, int32_t _waitTimeout)
    {
        return demuxThreadMessageQueue_.getMessage(_message, [this] {return isQuit(); }, _waitTimeout);
    }

    void VideoContext::postAudioThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others)
    {
        audioThreadMessageQueue_.pushMessage(_message, _forward, _clear_others);
    }

    bool VideoContext::getAudioThreadMessage(ThreadMessage& _message, int32_t _waitTimeout)
    {
        return audioThreadMessageQueue_.getMessage(_message, [this] {return isQuit(); }, _waitTimeout);
    }

    void VideoContext::clearMessageQueue()
    {
        videoThreadMessagesQueue_.clear();
        audioThreadMessageQueue_.clear();
        demuxThreadMessageQueue_.clear();
    }

    decode_thread_state VideoContext::getAudioThreadState(MediaData& _media) const
    {
        return _media.audioData_.state_;
    }

    void VideoContext::setAudioThreadState(const decode_thread_state& _state, MediaData& _media)
    {
        _media.audioData_.state_ = _state;
    }

    bool VideoContext::getVideoThreadMessage(ThreadMessage& _message, int32_t _waitTimeout)
    {
        return videoThreadMessagesQueue_.getMessage(_message, [this] {return isQuit(); }, _waitTimeout);
    }

    bool VideoContext::getAllVideoThreadMessages(ThreadMessageList& _messages, int32_t _waitTimeout)
    {
        return videoThreadMessagesQueue_.getAllMessages(_messages, [this] {return isQuit(); }, _waitTimeout);
    }

    bool VideoContext::updateScaleContext(MediaData& _media, const QSize _sz)
    {
        if (_media.scaledSize_ != _sz)
        {
            _media.scaledSize_ = _sz;
            _media.needUpdateSwsContext_ = true;

            Q_EMIT videoSizeChanged(_media.scaledSize_);
        }

        return true;
    }

    void VideoContext::freeScaleContext(MediaData& _media)
    {
        if (_media.frameRGB_)
        {
            ffmpeg::av_frame_unref(_media.frameRGB_);
            ffmpeg::av_frame_free(&_media.frameRGB_);
        }

        _media.frameRGB_ = nullptr;

        sws_freeContext(_media.swsContext_);
    }

    void VideoContext::setVolume(int32_t _volume, MediaData& _media)
    {
        _media.volume_ = _volume;
        stopPttIfNeeded(_media);
    }

    void VideoContext::setMute(bool _mute, MediaData& _media)
    {
        _media.mute_ = _mute;
        stopPttIfNeeded(_media);
    }

    void VideoContext::resetFrameTimer(MediaData& _media)
    {
        _media.frameTimer_ = currentTimeInSeconds();
    }

    void VideoContext::resetLastFramePts(MediaData& _media)
    {
        _media.frameLastPts_ = 0.0;
        _media.frameLastPtsReset_ = true;
    }

    bool VideoContext::seekMs(uint32_t _videoId, int _tsms, MediaData& _media)
    {
        if (_media.isLottie_)
        {
            Q_EMIT seekedA(_videoId);
            Q_EMIT seekedV(_videoId);
            return false;
        }

        auto calc_ts = [_tsms](ffmpeg::AVStream* _stream)
        {
            int64_t ts = 0;
            if (_stream)
            {
                ts = ffmpeg::av_rescale(_tsms, _stream->time_base.den, _stream->time_base.num);
                ts /= 1000;
            }
            return ts;
        };

        const auto ts_video = calc_ts(_media.videoStream_);
        const auto ts_audio = calc_ts(_media.audioStream_);
        return seekFrame(_videoId, ts_video, ts_audio, _media);
    }

    bool VideoContext::seekFrame(uint32_t _videoId, int64_t _ts_video, int64_t _ts_audio, MediaData& _media)
    {
        if (ffmpeg::avformat_seek_file(_media.formatContext_, _media.videoStream_->index, INT64_MIN
            , _media.startTimeVideo_ + _ts_video, INT64_MAX, 0) < 0)
        {
            return false;
        }

        _media.videoQueue_->freeData([_videoId, this](ffmpeg::AVPacket& _packet)
        {
            if (isFlushPacket(_packet))
            {
                qCDebug(ffmpegPlayer) << "Q_EMIT seekedV from clear";

                Q_EMIT seekedV(_videoId);
            }
        });

        _media.audioQueue_->freeData([_videoId, this](ffmpeg::AVPacket& _packet)
        {
            if (isFlushPacket(_packet))
            {
                qCDebug(ffmpegPlayer) << "Q_EMIT seekedA from clear";

                Q_EMIT seekedA(_videoId);
            }
        });

        ffmpeg::AVPacket flush_pkt_v;
        initFlushPacket(flush_pkt_v);
        flush_pkt_v.dts = (_media.startTimeVideo_ + _ts_video);

        pushVideoPacket(&flush_pkt_v, _media);

        if (_media.hasAudio())
        {
            ffmpeg::AVPacket flush_pkt_a;
            initFlushPacket(flush_pkt_a);
            flush_pkt_a.pts = (_media.startTimeAudio_ + _ts_audio);

            pushAudioPacket(&flush_pkt_a, _media);
        }
        else
        {
            Q_EMIT seekedA(_videoId);
        }

        return true;
    }

    void VideoContext::flushVideoBuffers(MediaData& _media)
    {
        if (_media.videoStream_)
        {
            ffmpeg::avcodec_flush_buffers(_media.videoStream_->codec);
        }
    }

    void VideoContext::flushAudioBuffers(MediaData& _media)
    {
        if (_media.audioStream_)
        {
            ffmpeg::avcodec_flush_buffers(_media.audioStream_->codec);
        }
    }

    void VideoContext::resetVideoClock(MediaData& _media)
    {
        _media.videoClock_ = 0.0;
    }

    void VideoContext::resetAudioClock(MediaData& _media)
    {
        _media.audioClock_ = 0.0;
    }

    void VideoContext::setStartTimeVideo(const int64_t& _startTime, MediaData& _media)
    {
        if (_media.startTimeVideoSet_)
            return;

        _media.startTimeVideoSet_ = true;
        _media.startTimeVideo_ = _startTime;
    }

    const int64_t& VideoContext::getStartTimeVideo(MediaData& _media) const
    {
        return _media.startTimeVideo_;
    }

    void VideoContext::setStartTimeAudio(const int64_t& _startTime, MediaData& _media)
    {
        if (_media.startTimeAudioSet_)
            return;

        _media.startTimeAudioSet_ = true;
        _media.startTimeAudio_ = _startTime;
    }

    const int64_t& VideoContext::getStartTimeAudio(MediaData& _media) const
    {
        return _media.startTimeAudio_;
    }


    std::shared_ptr<MediaData> VideoContext::getMediaData(uint32_t _videoId) const
    {
        std::scoped_lock lock(mediaDataMutex_);

        const auto it = std::as_const(mediaData_).find(_videoId);

        if (it == std::as_const(mediaData_).end())
            return nullptr;

        return it->second;
    }

    void VideoContext::onAudioQuit(uint32_t _videoId)
    {
        Utils::ensureMainThread();

        auto media_ptr = getMediaContainer()->ctx_.getMediaData(_videoId);
        if (!media_ptr)
            return;

        media_ptr->audioQuitRecv_ = true;

        sendCloseStreams(_videoId);
    }

    void VideoContext::onVideoQuit(uint32_t _videoId)
    {
        Utils::ensureMainThread();

        auto media_ptr = getMediaContainer()->ctx_.getMediaData(_videoId);
        if (!media_ptr)
            return;

        media_ptr->videoQuitRecv_ = true;

        sendCloseStreams(_videoId);
    }

    void VideoContext::onDemuxQuit(uint32_t _videoId)
    {
        Utils::ensureMainThread();

        auto media_ptr = getMediaContainer()->ctx_.getMediaData(_videoId);
        if (!media_ptr)
            return;

        media_ptr->demuxQuitRecv_ = true;

        sendCloseStreams(_videoId);
    }

    void VideoContext::onStreamsClosed(uint32_t _videoId)
    {
        Utils::ensureMainThread();
        deleteVideo(_videoId);
    }

    void VideoContext::sendCloseStreams(uint32_t _videoId)
    {
        auto media_ptr = getMediaContainer()->ctx_.getMediaData(_videoId);
        if (!media_ptr)
            return;

        if (media_ptr->demuxQuitRecv_ && media_ptr->audioQuitRecv_ && media_ptr->videoQuitRecv_)
        {
            getMediaContainer()->postDemuxThreadMessage(ThreadMessage(_videoId, thread_message_type::tmt_close_streams), true, true);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // DemuxThread
    //////////////////////////////////////////////////////////////////////////

    DemuxThread::DemuxThread(VideoContext& _ctx)
        : ctx_(_ctx)
    {
        setObjectName(qsl("Demux"));
    }

    void DemuxThread::run()
    {
        qCDebug(ffmpegPlayer) << "demux thread started";

        int32_t waitMsgTimeout = 0;
        ffmpeg::AVPacket packet;
        std::unordered_map<uint32_t, DemuxData> demuxData;
        ThreadMessage msg;

        while (!ctx_.isQuit())
        {
            decode_thread_state state = decode_thread_state::dts_none;
            int64_t seekPosition = -1;
            bool eof = false;

            if (ctx_.getDemuxThreadMessage(msg, waitMsgTimeout))
            {
                auto iterData = demuxData.find(msg.videoId_);
                bool videoDataFound = (iterData != demuxData.end());

                if (videoDataFound)
                {
                    state = iterData->second.state;
                    seekPosition = iterData->second.seekPosition;
                    eof = iterData->second.eof;
                }

                switch (msg.message_)
                {
                    case thread_message_type::tmt_init:
                    {
                        if (!videoDataFound)
                        {
                            auto emplace_result = demuxData.insert(std::make_pair(msg.videoId_, Ui::DemuxData()));
                            if (!emplace_result.second)
                            {
                                im_assert(false);
                                continue;
                            }

                            iterData = emplace_result.first;
                            iterData->second.media_ = ctx_.getMediaData(msg.videoId_);
                            videoDataFound = true;
                        }

                        Q_EMIT ctx_.demuxInited(msg.videoId_);
                        break;
                    }
                    case thread_message_type::tmt_open_streams:
                    {
                        qCDebug(ffmpegPlayer) << "open stream videoId = " << msg.videoId_;

                        auto media_ptr = ctx_.getMediaData(msg.videoId_);
                        if (!media_ptr || media_ptr->demuxQuitRecv_)
                            continue;

                        getMediaContainer()->ctx_.openStreams(msg.videoId_, msg.str_, *media_ptr);
                        continue;
                    }
                    case thread_message_type::tmt_close_streams:
                    {
                        qCDebug(ffmpegPlayer) << "close stream videoId = " << msg.videoId_;

                        auto media_ptr = ctx_.getMediaData(msg.videoId_);
                        if (!media_ptr || media_ptr->streamClosed_)
                            continue;

                        media_ptr->streamClosed_ = true;

                        ctx_.clearAudioQueue(*media_ptr);
                        ctx_.clearVideoQueue(*media_ptr);

                        getMediaContainer()->ctx_.closeFile(*media_ptr);

                        Q_EMIT ctx_.streamsClosed(msg.videoId_);
                        continue;
                    }
                    case thread_message_type::tmt_get_first_frame:
                    {
                        Q_EMIT getMediaContainer()->frameReady(ctx_.getFirstFrame(msg.str_), msg.str_);
                        continue;
                    }
                    case thread_message_type::tmt_quit:
                    {
                        qCDebug(ffmpegPlayer) << "demux quit videoId = " << msg.videoId_;
                        if (videoDataFound)
                        {
                            demuxData.erase(iterData);

                            iterData = demuxData.end();
                            videoDataFound = false;
                        }

                        auto media_ptr = ctx_.getMediaData(msg.videoId_);
                        if (!media_ptr)
                        {
                            Q_EMIT ctx_.demuxQuit(msg.videoId_);
                            continue;
                        }

                        ctx_.pushVideoPacket(&quit_pkt, *media_ptr);
                        ctx_.pushAudioPacket(&quit_pkt, *media_ptr);

                        Q_EMIT ctx_.demuxQuit(msg.videoId_);
                        break;
                    }
                    case thread_message_type::tmt_play:
                    {
                        if (videoDataFound)
                            state = decode_thread_state::dts_playing;

                        break;
                    }
                    case thread_message_type::tmt_pause:
                    {
                        if (videoDataFound)
                            state = decode_thread_state::dts_paused;

                        break;
                    }
                    case thread_message_type::tmt_seek_position:
                    {
                        if (videoDataFound)
                        {
                            if (seekPosition >= 0)
                            {
                                Q_EMIT ctx_.seekedV(msg.videoId_);
                                Q_EMIT ctx_.seekedA(msg.videoId_);
                            }

                            seekPosition = msg.x_;
                        }

                        break;
                    }
                    default:
                        continue;
                }

                if (videoDataFound)
                {
                    iterData->second.state = state;
                    iterData->second.seekPosition = seekPosition;
                }
            }

            waitMsgTimeout = demuxData.empty() ? 60000 : 10;

            for (auto& [videoId, data] : demuxData)
            {
                state = data.state;
                seekPosition = data.seekPosition;
                eof = data.eof;

                auto media_ptr = data.media_.lock();
                if (!media_ptr)
                    continue;

                auto& media = *media_ptr;

                if (data.inited && media.formatContext_)
                {
                    if (state == decode_thread_state::dts_paused)
                        ctx_.readAVPacketPause(media);
                    else
                        ctx_.readAVPacketPlay(media);
                }

                if (state == decode_thread_state::dts_paused)
                    continue;

                if (seekPosition >= 0)
                {
                    ctx_.seekMs(videoId, seekPosition, media);

                    seekPosition = -1;
                    data.seekPosition = seekPosition;
                    eof = false;
                    data.eof = eof;
                }

                if (
                    (
                        (ctx_.getAudioQueueSize(media) > maxQueueSize || !media.hasAudio())
                        && (ctx_.getVideoQueueSize(media) > maxQueueSize || !media.hasVideo())
                        )

                    ||
                    (
                        (ctx_.getAudioQueuePackets(media) > minFramesCount || !media.hasAudio())
                        && (ctx_.getVideoQueuePackets(media) > minFramesCount || !media.hasVideo())
                        )
                    )
                {
                    continue;
                }

                if (media.isLottie_)
                {
                    data.inited = true;
                    data.eof = false;

                    waitMsgTimeout = 1;

                    while (ctx_.getVideoQueuePackets(media) <= minFramesCount)
                        ctx_.pushVideoPacket(0, media);

                    Q_EMIT ctx_.dataReady(videoId);
                    continue;
                }

                int32_t readPacketError = ctx_.readAVPacket(&packet, media);

                if (readPacketError < 0)
                {
                    if (ctx_.isEof(readPacketError, media) && !eof)
                    {
                        if (media.hasAudio())
                            ctx_.pushAudioPacket(0, media);

                        ctx_.pushVideoPacket(0, media);

                        eof = true;
                    }

                    data.eof = eof;
                    // TODO : do smth with this error
                    if (ctx_.isStreamError(media))
                        continue;

                    continue;
                }
                else
                {
                    eof = false;
                }

                waitMsgTimeout = 1;
                data.eof = eof;

                int32_t videoStreamIndex = ctx_.getVideoStreamIndex(media);
                int32_t audioStreamIndex = ctx_.getAudioStreamIndex(media);

                if (packet.stream_index == videoStreamIndex)
                {
                    ctx_.pushVideoPacket(&packet, media);
                }
                else if (packet.stream_index == audioStreamIndex)
                {
                    ctx_.pushAudioPacket(&packet, media);
                }
                else
                {
                    av_packet_unref(&packet);
                }

                if (!data.inited)
                {
                    data.inited = true;

                    Q_EMIT ctx_.dataReady(videoId);
                }
            }
        }

        ctx_.cleanupOnExit();

        qCDebug(ffmpegPlayer) << "demux thread finished";
    }



    //////////////////////////////////////////////////////////////////////////
    // VideoDecodeThread
    //////////////////////////////////////////////////////////////////////////
    VideoDecodeThread::VideoDecodeThread(VideoContext& _ctx)
        : ctx_(_ctx)
    {
        setObjectName(qsl("VideoDecode"));
    }

    void VideoDecodeThread::run()
    {
        qCDebug(ffmpegPlayer) << "video decode thread start";

        ffmpeg::AVFrame* frame = ffmpeg::av_frame_alloc();

        std::unordered_map<uint32_t, VideoData> videoData;

        ThreadMessageList messages;

        std::vector<LottieRenderData> lottieData;
        lottieData.reserve(1000);

        int32_t waitMsgTimeout = 60000;

        ffmpeg::AVPacket av_packet;

        while (!ctx_.isQuit())
        {
            if (!ctx_.getAllVideoThreadMessages(messages, waitMsgTimeout))
                continue;

            qCDebug(ffmpegPlayer) << "video decode processing" << messages.size() << "messages";
            for (auto& msg : messages)
            {
                const auto videoId = msg.videoId_;
                auto it = videoData.find(videoId);
                if (it == videoData.end())
                {
                    if (msg.message_ == thread_message_type::tmt_init)
                    {
                        VideoData data;
                        data.current_state_ = decode_thread_state::dts_playing;
                        data.stream_finished_ = false;
                        data.eof_ = false;
                        data.media_ = ctx_.getMediaData(videoId);
                        it = videoData.insert({ videoId, std::move(data) }).first;
                    }
                    else if (msg.message_ == thread_message_type::tmt_quit)
                    {
                        Q_EMIT ctx_.videoQuit(videoId);
                        continue;
                    }
                    else
                    {
                        continue;
                    }
                }

                auto& data = it->second;
                if (processDataMessage(data, msg))
                    continue;

                auto media_ptr = data.media_.lock();
                if (!media_ptr)
                    continue;

                auto& media = *media_ptr;
                switch (msg.message_)
                {
                    case thread_message_type::tmt_quit:
                    {
                        videoData.erase(it);
                        ctx_.freeScaleContext(media);

                        Q_EMIT ctx_.videoQuit(videoId);
                        break;
                    }
                    case thread_message_type::tmt_update_scaled_size:
                    {
                        const QSize scaledSize = ctx_.getTargetSize(media);

                        QSize newSize(msg.x_, msg.y_);

                        if (scaledSize != newSize)
                            ctx_.updateScaleContext(media, newSize);
                        break;
                    }
                    case thread_message_type::tmt_get_next_video_frame:
                    {
                        if (data.current_state_ == decode_thread_state::dts_end_of_media || data.current_state_ == decode_thread_state::dts_failed)
                            break;

                        if (media.isLottie_)
                        {
                            auto rdata = renderLottieFrame(data, media, videoId);
                            if (rdata.isValid())
                                lottieData.emplace_back(std::move(rdata));
                        }
                        else
                        {
                            renderVideoFrame(data, media, frame, av_packet, videoId);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }

            for (auto& rdata : lottieData)
            {
                rdata.fut_.get();
                Q_EMIT ctx_.nextframeReady(rdata.videoId_, rdata.frame_, currentTimeInSeconds(), false);

                if (auto media = rdata.media_.lock())
                    media->lottieRenderer_.setFrame(rdata.frameNo_, rdata.frame_);
            }
            lottieData.clear();
        }

        for (const auto& [_, data] : videoData)
            if (auto media = data.media_.lock())
                ctx_.freeScaleContext(*media);

        ffmpeg::av_frame_unref(frame);
        ffmpeg::av_frame_free(&frame);

        qCDebug(ffmpegPlayer) << "video decode thread finished";
    }

    LottieRenderData VideoDecodeThread::renderLottieFrame(VideoData& _data, MediaData& _media, int32_t _videoId)
    {
        if (!_media.lottieRenderer_ || ctx_.isQuit() || ctx_.isVideoQuit(_videoId))
            return {};

        const auto startTimeSet = _media.startTimeVideoSet_;
        ctx_.setStartTimeVideo(currentTimeInSeconds(), _media);
        if (!startTimeSet)
            _media.frameTimer_ = _media.startTimeVideo_;

        const auto totalFrames = _media.lottieRenderer_->totalFrame();
        unsigned int frameNo = int((_media.frameTimer_ - _media.startTimeVideo_) / _media.videoClock_);
        _data.eof_ = false;

        if (frameNo < totalFrames)
        {
            qCDebug(ffmpegPlayer) << "lottie render" << frameNo << "/" << (totalFrames - 1);

            const auto scaledSize = ctx_.getTargetSize(_media);
            QImage lastFrame = getLastFrame(_data);
            if (lastFrame.isNull() || lastFrame.size() != scaledSize || !lastFrame.isDetached())
                lastFrame = QImage(scaledSize, QImage::Format_ARGB32_Premultiplied);

            LottieRenderData rdata;
            rdata.frame_ = std::move(lastFrame);
            rdata.videoId_ = _videoId;
            rdata.frameNo_ = frameNo;
            rdata.media_ = _data.media_;

            rlottie::Surface surface((uint32_t*)rdata.frame_.bits(), rdata.frame_.width(), rdata.frame_.height(), rdata.frame_.bytesPerLine());
            rdata.fut_ = _media.lottieRenderer_.renderFrame(frameNo, std::move(surface));
            return rdata;
        }
        else
        {
            qCDebug(ffmpegPlayer) << "lottie render EOF";

            _data.current_state_ = decode_thread_state::dts_end_of_media;
            _data.stream_finished_ = false;

            _media.startTimeVideoSet_ = false;

            Q_EMIT ctx_.nextframeReady(_videoId, QImage(), 0, true);
        }

        return {};
    }

    void VideoDecodeThread::renderVideoFrame(VideoData& _data, MediaData& _media, ffmpeg::AVFrame* _frame, ffmpeg::AVPacket& _av_packet, int32_t _videoId)
    {
        ffmpeg::av_frame_unref(_frame);

        _data.eof_ = false;

        if (ctx_.getNextVideoFrame(_frame, &_av_packet, _data, _media, _videoId))
        {
            ctx_.setStartTimeVideo(_frame->pkt_dts, _media);

            // Consider sync
            double pts = _frame->pkt_dts;
            if (pts == AV_NOPTS_VALUE)
                pts = _frame->pkt_pts;
            if (pts == AV_NOPTS_VALUE)
                pts = 0;

            pts *= ctx_.getVideoTimebase(_media);
            pts = ctx_.synchronizeVideo(_frame, pts, _media);

            if (ctx_.isQuit())
                return;

            auto scaledSize = ctx_.getTargetSize(_media);
            const auto sourceSize = ctx_.getSourceSize(_media);

            auto targetRatio = static_cast<double>(sourceSize.width()) / sourceSize.height();

            if (scaledSize.width() > scaledSize.height())
                scaledSize.rheight() = scaledSize.width() / targetRatio;
            else
                scaledSize.rwidth() = scaledSize.height() * targetRatio;

            scaledSize = scaledSize.boundedTo(sourceSize);

            const auto rotation = ctx_.getRotation(_media);

            if (scaledSize.width() == 0)
                scaledSize.setWidth(1);

            if (scaledSize.height() == 0)
                scaledSize.setHeight(1);

            auto lastFrame = getLastFrame(_data);

            int align = 256;
            if constexpr (platform::is_windows()) // spike for opengl
            {
                align = 4;
            }
            else // spike for osx
            {
                while (true)
                {
                    if (_frame->linesize[0] % align == 0)
                        break;

                    align = align / 2;
                }
            }

            if (lastFrame.isNull() || lastFrame.size() != scaledSize || !lastFrame.isDetached() || !isAlignedImage(lastFrame, align))
                lastFrame = createAlignedImage(scaledSize, align);

            if ((_media.needUpdateSwsContext_) || (_frame->format != -1 && _frame->format != _media.codecContext_->pix_fmt) || !_media.swsContext_)
            {
                _media.needUpdateSwsContext_ = false;
                _media.swsContext_ = sws_getCachedContext(
                    _media.swsContext_,
                    _frame->width,
                    _frame->height,
                    ffmpeg::AVPixelFormat(_frame->format), scaledSize.width(), scaledSize.height(), ffmpeg::AV_PIX_FMT_BGRA, SWS_POINT, 0, 0, 0);
            }

            uint8_t* toData[AV_NUM_DATA_POINTERS] = { lastFrame.bits(), nullptr };
            int toLinesize[AV_NUM_DATA_POINTERS] = { lastFrame.bytesPerLine(), 0 };
            ffmpeg::sws_scale(_media.swsContext_, _frame->data, _frame->linesize, 0, _frame->height, toData, toLinesize);

            if (rotation)
                lastFrame = lastFrame.transformed(QTransform().rotate(rotation));

            Q_EMIT ctx_.nextframeReady(_videoId, lastFrame, pts, false);
        }
        else if (_data.eof_)
        {
            _data.current_state_ = decode_thread_state::dts_end_of_media;
            _data.stream_finished_ = false;

            ctx_.resetVideoClock(_media);

            Q_EMIT ctx_.nextframeReady(_videoId, QImage(), 0, true);
        }
    }

    QImage VideoDecodeThread::getLastFrame(VideoData& _data) const
    {
        QImage lastFrame = _data.emptyFrames_.empty() ? QImage() : std::move(_data.emptyFrames_.front());
        if (!_data.emptyFrames_.empty())
            _data.emptyFrames_.pop_front();

        return lastFrame;
    }

    bool VideoDecodeThread::processDataMessage(VideoData& _data, ThreadMessage& _msg) const
    {
        switch (_msg.message_)
        {
            case thread_message_type::tmt_pause:
                if (_data.current_state_ != decode_thread_state::dts_failed)
                    _data.current_state_ = decode_thread_state::dts_paused;
                break;
            case thread_message_type::tmt_play:
                if (_data.current_state_ != decode_thread_state::dts_failed)
                    _data.current_state_ = decode_thread_state::dts_playing;
                break;
            case thread_message_type::tmt_seek_position:
                if (_data.current_state_ == decode_thread_state::dts_end_of_media)
                    _data.current_state_ = decode_thread_state::dts_playing;
                break;
            case thread_message_type::tmt_empty_frame:
                if (_data.emptyFrames_.size() < 5)
                    _data.emptyFrames_.emplace_back(std::move(_msg.emptyFrame_));
                break;
            default:
                return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    // AudioDecodeThread
    //////////////////////////////////////////////////////////////////////////
    AudioDecodeThread::AudioDecodeThread(VideoContext& _ctx)
        : ctx_(_ctx)
    {
        setObjectName(qsl("AudioDecode"));
    }

    void AudioDecodeThread::run()
    {
        qCDebug(ffmpegPlayer) << "audio decode thread start";

        std::unordered_map<uint32_t, AudioData> audioData;
        bool flush = false;

        ffmpeg::AVPacket packet;
        ThreadMessage msg;

        auto isPlaying = [](const auto& media_ptr)
        {
            return
                media_ptr &&
                media_ptr->hasAudio() &&
                media_ptr->audioData_.state_ == decode_thread_state::dts_playing;
        };

        while (!ctx_.isQuit())
        {
            const auto has_playing = std::any_of(audioData.cbegin(), audioData.cend(), [isPlaying](const auto& x)
            {
                return isPlaying(x.second.media_.lock());
            });

            if (!flush && ctx_.getAudioThreadMessage(msg, has_playing ? 10 : 60000))
            {
                const auto videoId = msg.videoId_;

                auto it = audioData.find(videoId);
                if (it == audioData.end())
                {
                    if (msg.message_ == thread_message_type::tmt_init)
                    {
                        auto media = ctx_.getMediaData(videoId);
                        if (!media || media->isLottie_ || !ctx_.initDecodeAudioData(*media))
                            continue;

                        AudioData data;
                        data.eof_ = false;
                        data.stream_finished_ = false;
                        data.media_ = media;
                        it = audioData.insert({ videoId, std::move(data) }).first;

                        ctx_.setMute(msg.x_, *media);
                        ctx_.setVolume(msg.y_, *media);
                    }
                    else if (msg.message_ == thread_message_type::tmt_quit)
                    {
                        Q_EMIT ctx_.audioQuit(videoId);
                        continue;
                    }
                    else
                    {
                        continue;
                    }
                }

                auto media_ptr = it->second.media_.lock();
                if (!media_ptr)
                    continue;

                auto& media = *media_ptr;

                switch (msg.message_)
                {
                    case thread_message_type::tmt_set_volume:
                    {
                        ctx_.setVolume(msg.x_, media);

                        break;
                    }
                    case thread_message_type::tmt_pause:
                    {
                        ctx_.setAudioThreadState(decode_thread_state::dts_paused, media);

                        ctx_.suspendAudio(media);

                        break;
                    }
                    case thread_message_type::tmt_play:
                    {
                        ctx_.setAudioThreadState(decode_thread_state::dts_playing, media);

                        break;
                    }
                    case thread_message_type::tmt_set_mute:
                    {
                        ctx_.setMute(msg.x_, media);

                        break;
                    }
                    case thread_message_type::tmt_quit:
                    {
                        ctx_.freeDecodeAudioData(media);

                        audioData.erase(it);

                        Q_EMIT ctx_.audioQuit(videoId);
                        break;
                    }
                    case thread_message_type::tmt_set_finished:
                    {
                        ctx_.setAudioThreadState(decode_thread_state::dts_end_of_media, media);

                        it->second.eof_ = true;

                        break;
                    }

                    case thread_message_type::tmt_reinit_audio:
                    {
                        ctx_.flushBuffers(media);
                        break;
                    }

                    default:
                        break;
                }
            }

            flush = false;

            if (ctx_.isQuit())
                break;

            for (auto& [videoId, data] : audioData)
            {
                data.eof_ = false;

                auto media_ptr = data.media_.lock();
                if (!isPlaying(media_ptr))
                    continue;

                auto& media = *media_ptr;
                bool local_flush = false;
                int seekCount = 0;

                ctx_.playNextAudioFrame(&packet, data, local_flush, seekCount, media, videoId);

                int64_t current_time = ffmpeg::av_gettime();

                constexpr int64_t sync_period = 1000 * 200; // 200 milliseconds

                if ((current_time - data.last_sync_time_) > sync_period)
                {
                    data.last_sync_time_ = current_time;

                    //qDebug() << "Audio thread: emit audioTime";
                    //qDebug() << "Audio thread: offset = " <<  data.offset_;

                    Q_EMIT ctx_.audioTime(videoId, current_time, data.offset_);
                }

                if (seekCount)
                {
                    for (int i = 0; i < seekCount; ++i)
                        Q_EMIT ctx_.seekedA(videoId);
                }

                flush |= local_flush;

                if (data.eof_)
                {
                    ctx_.setAudioThreadState(decode_thread_state::dts_end_of_media, media);

                    data.stream_finished_ = false;
                }
            }
        }

        for (const auto& [_, data] : audioData)
            if (auto media = data.media_.lock())
                ctx_.freeDecodeAudioData(*media);

        qCDebug(ffmpegPlayer) << "audio decode thread finish";
    }

    static int lockmgr(void **mtx, enum ffmpeg::AVLockOp op)
    {
        switch (op)
        {
            case ffmpeg::AV_LOCK_CREATE:
            {
                *mtx = new std::mutex();
                return 0;
            }
            case ffmpeg::AV_LOCK_OBTAIN:
            {
                ((std::mutex*) *mtx)->lock();
                return 0;
            }
            case ffmpeg::AV_LOCK_RELEASE:
            {
                ((std::mutex*) *mtx)->unlock();
                return 0;
            }
            case ffmpeg::AV_LOCK_DESTROY:
            {
                delete ((std::mutex*) *mtx);
                return 0;
            }
        }

        return 1;
    }

    const auto mouse_move_rate = std::chrono::milliseconds(200);

    PlayersList::PlayersList()
    {
        connect(&getMediaContainer()->ctx_, &VideoContext::nextframeReady, this, &PlayersList::onNextFrameReady);
    }

    PlayersList::~PlayersList() = default;

    PlayersList& PlayersList::getPlayersList()
    {
        static PlayersList players;

        return players;
    }

    void PlayersList::addPlayer(FFMpegPlayer* _player)
    {
        playersList_.push_back(_player);
    }

    void PlayersList::removePlayer(FFMpegPlayer* _player)
    {
        playersList_.erase(std::remove_if(playersList_.begin(), playersList_.end(), [_player](const auto& _val)
        {
            return (_val == _player);
        }), playersList_.end());
    }

    void PlayersList::onNextFrameReady(uint32_t _videoId, const QImage& _image, double _pts, bool _eof)
    {
        auto iterPlayer = std::find_if(playersList_.begin(), playersList_.end(), [_videoId](const auto& _player)
        {
            return (_videoId == _player->getMedia());
        });

        if (iterPlayer != playersList_.end())
            (*iterPlayer)->onNextFrameReady(_image, _pts, _eof);
    }


    //////////////////////////////////////////////////////////////////////////
    // FFMpegPlayer
    //////////////////////////////////////////////////////////////////////////
    FFMpegPlayer::FFMpegPlayer(QWidget* _parent, std::shared_ptr<FrameRenderer> _renderer, bool _continius)
        :   QObject(_parent),
            mediaId_(0),
            restoreVolume_(100),
            volume_(100),
            isFirstFrame_(true),
            updatePositonRate_(platform::is_apple() ? 1000 : 100),
            state_(decode_thread_state::dts_none),
            lastVideoPosition_(0),
            lastPostedPosition_(0),
            lastEmitMouseMove_(std::chrono::system_clock::now() - mouse_move_rate),
            stopped_(false),
            started_(false),
            continius_(_continius),
            replay_(false),
            mute_(false),
            dataReady_(false),
            pausedByUser_(false),
            seek_request_id_(0)
    {
        static auto is_init = false;

        if (!is_init && ffmpeg::av_lockmgr_register(lockmgr)) // TODO
        {
        }

        is_init = true;

        setRenderer(_renderer);

        timer_ = new QTimer(this);
        timer_->setTimerType(Qt::PreciseTimer);
        connect(timer_, &QTimer::timeout, this, &FFMpegPlayer::onTimer);

        connect(&getMediaContainer()->ctx_, &VideoContext::dataReady, this, &FFMpegPlayer::onDataReady);
        connect(&getMediaContainer()->ctx_, &VideoContext::streamsOpenFailed, this, &FFMpegPlayer::streamsOpenFailed);
        connect(&getMediaContainer()->ctx_, &VideoContext::audioTime, this, &FFMpegPlayer::onAudioTime);
        connect(&getMediaContainer()->ctx_, &VideoContext::seekedV, this, &FFMpegPlayer::seekedV);
        connect(&getMediaContainer()->ctx_, &VideoContext::seekedA, this, &FFMpegPlayer::seekedA);

        connect(GetSoundsManager(), &SoundsManager::deviceListChanged, this, &FFMpegPlayer::onDeviceListChanged);

        PlayersList::getPlayersList().addPlayer(this);
    }


    FFMpegPlayer::~FFMpegPlayer()
    {
        stop();

        PlayersList::getPlayersList().removePlayer(this);
    }

    void FFMpegPlayer::seeked(uint32_t _videoId, const bool _fromAudio)
    {
        if (_videoId != mediaId_)
            return;

        --seek_request_id_;

        qCDebug(ffmpegPlayer) << (_fromAudio ? "seekedA" : "seekedV") << " seek_request_id_ = " << seek_request_id_;

        if (seek_request_id_ < 0)
        {
            //im_assert(false);
            seek_request_id_ = 0;
        }

        if (seek_request_id_ == 0)
        {
            if (auto media = getMediaContainer()->ctx_.getMediaData(mediaId_))
                getMediaContainer()->resetLastFramePts(*media);
        }
    }

    void FFMpegPlayer::seekedV(uint32_t _videoId)
    {
        seeked(_videoId, false);
    }

    void FFMpegPlayer::seekedA(uint32_t _videoId)
    {
        seeked(_videoId, true);
    }

    void FFMpegPlayer::onDeviceListChanged()
    {
        const auto messsage = ThreadMessage(mediaId_, thread_message_type::tmt_reinit_audio);
        getMediaContainer()->postAudioThreadMessage(messsage, true);
    }

    void FFMpegPlayer::onNextFrameReady(const QImage& _image, double _pts, bool _eof)
    {
        if (!_eof)
        {
            if (state_ == decode_thread_state::dts_playing)
                decodedFrames_.emplace_back(_image, _pts);

            if (!firstFrame_)
            {
                firstFrame_ = std::make_unique<DecodedFrame>(_image, _pts);

                Q_EMIT firstFrameReady();
            }
        }
        else
        {
            decodedFrames_.emplace_back(_eof);

            getMediaContainer()->postAudioThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_set_finished), false);
        }
    }

    void FFMpegPlayer::onDataReady(uint32_t _videoId)
    {
        // TODO : use signal mapper
        if (_videoId != mediaId_)
            return;

        timer_->stop();

        Q_EMIT durationChanged(getDuration());
        dataReady_ = true;

        if (!getMediaContainer()->is_decods_inited_)
        {
            getMediaContainer()->VideoDecodeThreadStart(mediaId_);
            getMediaContainer()->AudioDecodeThreadStart(mediaId_);
            getMediaContainer()->is_decods_inited_ = true;

            timer_->start(onDataReadyTimeout());
        }
        else
        {
            timer_->start(0);
        }
    }

    void FFMpegPlayer::onAudioTime(uint32_t _videoId, int64_t _avtime, double _offset)
    {
        if (_videoId != mediaId_)
            return;

        if (seek_request_id_ > 0)
            return;

        auto media = getMediaContainer()->ctx_.getMediaData(mediaId_);
        if (!media)
            return;

        //qDebug() << "media->syncWithAudio_ = " << media->syncWithAudio_;

        media->syncWithAudio_ = true;
        media->audioClock_ = _offset;
        media->audioClockTime_ = _avtime;

        qCDebug(ffmpegPlayer) << "media->audioClock_" << media->audioClock_;
        qCDebug(ffmpegPlayer) << "media->audioClockTime_" << media->audioClockTime_;
    }

    uint32_t FFMpegPlayer::stop()
    {
        if (stopped_)
            return mediaId_;

        state_ = decode_thread_state::dts_none;
        timer_->stop();

        decodedFrames_.clear();

        if (!continius_)
            stopped_ = true;

        getMediaContainer()->ctx_.setVideoQuit(mediaId_);

        const auto messsage = ThreadMessage(mediaId_, thread_message_type::tmt_quit);
        getMediaContainer()->postDemuxThreadMessage(messsage, true, true);
        getMediaContainer()->postVideoThreadMessage(messsage, true, true);
        getMediaContainer()->postAudioThreadMessage(messsage, true, true);

        auto ret = mediaId_;
        mediaId_ = 0;

        return ret;
    }

    void FFMpegPlayer::updateVideoPosition(const DecodedFrame& _frame)
    {
        if (_frame.eof_)
        {
            auto media = getMediaContainer()->ctx_.getMediaData(mediaId_);
            if (!media)
                return;

            Q_EMIT positionChanged(getMediaContainer()->getDuration(*media));

            lastVideoPosition_ = 0;
            lastPostedPosition_ = 0;
        }
        else if (seek_request_id_ == 0)
        {
            qint64 videoClock = (_frame.pts_ * 1000);
            lastVideoPosition_ = videoClock;

            if ((lastVideoPosition_ - lastPostedPosition_) > updatePositonRate_)
            {
                lastPostedPosition_ = lastVideoPosition_;

                Q_EMIT positionChanged(lastVideoPosition_);
            }
        }
    }

    void FFMpegPlayer::onTimer()
    {
        qCDebug(ffmpegPlayer) << "real timer time = " << int((std::chrono::system_clock::now() - prev_frame_time_) / std::chrono::milliseconds(1));

        auto renderer = weak_renderer_.lock();

        if (!getStarted())
        {
            if (renderer)
                renderer->redraw();

            pause();
            return;
        }

        auto media = getMediaContainer()->ctx_.getMediaData(mediaId_);
        if (!media)
        {
            timer_->stop();
            return;
        }

        if (isFirstFrame_)
        {
            isFirstFrame_ = false;
            getMediaContainer()->resetFrameTimer(*media);
        }

        if (getMediaContainer()->isQuit(mediaId_))
        {
            timer_->stop();
            return;
        }

        if (state_ != decode_thread_state::dts_playing)
            return;

        if (decodedFrames_.empty())
        {
            if (getStarted())
            {
                const int interval = std::min(100., (media->frameLastDelay_ > 0. ? media->frameLastDelay_ : 0.1) * 1000.0 + 0.5);
                qCDebug(ffmpegPlayer) << "!!! decoded frames empty, timer restart" << interval << "id" << mediaId_;
                timer_->start(interval);
            }

            getMediaContainer()->postVideoThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_get_next_video_frame), false);
            return;
        }

        if (dataReady_)
        {
            Q_EMIT dataReady();
            dataReady_ = false;
        }

        auto& frame = decodedFrames_.front();

        updateVideoPosition(frame);

        if (frame.eof_)
        {
            decodedFrames_.pop_front();
            timer_->stop();


            if (continius_ && !replay_)
            {
                stop();
                loadFromQueue();
            }
            else
            {
                state_ = decode_thread_state::dts_end_of_media;
            }

            if (renderer)
            {
                if (firstFrame_ && !continius_)
                    renderer->updateFrame(firstFrame_->image_);

                renderer->redraw();
            }

//            // TODO : use another way
//            if (!continius_ || mediaId_ != 0)
//                play(true);

            if (replay_)
            {
                play(true);
            }
            else
            {
                stop();
                Q_EMIT mediaFinished();
            }

            return;
        }
        //qDebug() << "send "<< QTime::currentTime() <<"get next id onTimer2 " << mediaId_;
        getMediaContainer()->postVideoThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_get_next_video_frame), false);

        if (renderer)
            renderer->updateFrame(frame.image_);

        ThreadMessage msg(mediaId_, thread_message_type::tmt_empty_frame);
        msg.setEmptyFrame(std::exchange(frame.image_, {}));
        getMediaContainer()->postVideoThreadMessage(msg, false);

        prev_frame_time_ = std::chrono::system_clock::now();

        double delay = (seek_request_id_ ? 0.1 : getMediaContainer()->computeDelay(frame.pts_, *media));

        int timeout = (int)(delay * 1000.0 + 0.5);
        if (getStarted() && timeout != timer_->interval())
        {
            qCDebug(ffmpegPlayer) << "restart timer" << timer_->interval() << "->" << timeout;
            timer_->setInterval(timeout);
        }

        qCDebug(ffmpegPlayer) << "seek" << seek_request_id_ << "delay" << delay << "timeout is =" << timeout;

        decodedFrames_.pop_front();

        if (renderer)
            renderer->redraw();
    }

    bool FFMpegPlayer::openMedia(const QString& _mediaPath, bool _isLottie, bool _isLottieInstantPreview, uint32_t _id)
    {
        if (!continius_)
            stopped_ = false;

        uint32_t mediaId = getMediaContainer()->init(_id);

        openStreamsConnection_ = connect(&getMediaContainer()->ctx_, &VideoContext::streamsOpened, this, &FFMpegPlayer::onStreamsOpened);

        auto media = getMediaContainer()->ctx_.getMediaData(mediaId);
        if (!media)
            return false;

        media->isLottie_ = _isLottie;

        if (_isLottie && _isLottieInstantPreview)
            setPreview(LottieHandleCache::instance().getHandle(_mediaPath).renderFirstFrame(getVideoSize()));

        getMediaContainer()->DemuxThreadStart(mediaId);

        auto msg = ThreadMessage(mediaId, thread_message_type::tmt_open_streams);
        msg.str_ = _mediaPath;

        getMediaContainer()->postDemuxThreadMessage(msg, false);

        if (mediaId_ == 0 || !continius_)
        {
            mediaId_ = mediaId;
            Q_EMIT mediaChanged(mediaId_);
        }
        else
        {
            if (_id != 0)
                queuedMedia_.push_front(mediaId);
            else
                queuedMedia_.push_back(mediaId);
        }

        return true;// getMediaContainer()->openFile(_mediaPath, *media);
    }

    void FFMpegPlayer::onRendererSize(const QSize _sz)
    {
        getMediaContainer()->updateVideoScaleSize(getMedia(), Utils::scale_bitmap(_sz));
    }

    void FFMpegPlayer::onStreamsOpened(uint32_t _videoId)
    {
        if (_videoId != mediaId_)
        {
            if (!continius_)
                return;

            const auto queued = std::find(queuedMedia_.begin(), queuedMedia_.end(), _videoId);
            if (queued == queuedMedia_.end())
                return;
        }

        disconnect(openStreamsConnection_);

        auto media = getMediaContainer()->ctx_.getMediaData(_videoId);
        if (!media || !media->hasVideo())
            return;

        getMediaContainer()->openFile(*media);
        Q_EMIT fileLoaded();
    }

    void FFMpegPlayer::play(bool _init)
    {
        qCDebug(ffmpegPlayer) << "call play(bool _init = " << _init << ')';

        if (!_init)
        {
            if (auto renderer = weak_renderer_.lock())
                renderer->redraw();
            return;
        }

        auto media_ptr = getMediaContainer()->ctx_.getMediaData(mediaId_);

        if (!media_ptr)
            return;

        auto& media = *media_ptr;

        setStarted(_init);

        if (state_ == decode_thread_state::dts_none)
        {
            QObject::connect(&getMediaContainer()->ctx_, &VideoContext::demuxInited, this, [this](const uint32_t _videoId)
            {
                if (_videoId != mediaId_)
                    return;

                if (state_ == decode_thread_state::dts_playing || state_ == decode_thread_state::dts_paused)
                {
                    ThreadMessage msg(mediaId_, thread_message_type::tmt_init);
                    getMediaContainer()->postVideoThreadMessage(msg, false);

                    msg.x_ = mute_;
                    msg.y_ = volume_;
                    getMediaContainer()->postAudioThreadMessage(msg, false);

                    if (auto renderer = weak_renderer_.lock())
                        onRendererSize(renderer->getWidget()->size());

                    getMediaContainer()->postVideoThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_get_next_video_frame), false);

                    if (auto media_ptr = getMediaContainer()->ctx_.getMediaData(mediaId_))
                        getMediaContainer()->resetFrameTimer(*media_ptr);
                }
            });

            getMediaContainer()->DemuxThreadStart(mediaId_);
            getMediaContainer()->postDemuxThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_init), false);
        }
        else if (/*started_ || */state_ == decode_thread_state::dts_paused)
        {

            getMediaContainer()->postDemuxThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_play), false);
            getMediaContainer()->postVideoThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_play), false);
            getMediaContainer()->postAudioThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_play), false);

            timer_->start(0);

            getMediaContainer()->resetFrameTimer(media);
        }
        else if (state_ == decode_thread_state::dts_end_of_media)
        {
            setPosition(0);

            getMediaContainer()->postDemuxThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_play), false);
            getMediaContainer()->postVideoThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_play), false);
            getMediaContainer()->postAudioThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_play), false);

            state_ = decode_thread_state::dts_playing;

            getMediaContainer()->resetFrameTimer(media);

            timer_->start(0);
        }

        state_ = decode_thread_state::dts_playing;

        Q_EMIT played();
    }

    bool FFMpegPlayer::canPause() const
    {
        auto renderer = weak_renderer_.lock();
        return renderer && !renderer->isActiveImageNull();
    }

    void FFMpegPlayer::pause()
    {
        qCDebug(ffmpegPlayer) << "call pause()";

        if (state_ == decode_thread_state::dts_playing && canPause())
        {
            // don't pause demux thread fix for getNextFrame
            //getMediaContainer()->postDemuxThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_pause), false);
            getMediaContainer()->postVideoThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_pause), false);
            getMediaContainer()->postAudioThreadMessage(ThreadMessage(mediaId_, thread_message_type::tmt_pause), false);

            state_ = decode_thread_state::dts_paused;
        }

        timer_->stop();

        Q_EMIT paused();
    }

    void FFMpegPlayer::setPosition(int64_t _position)
    {
        ++seek_request_id_; // for video thread
        ++seek_request_id_; // for audio thread

        qCDebug(ffmpegPlayer) << "seek request id = " << seek_request_id_ << " position = " << _position << " videoId = " << mediaId_;

        auto media = getMediaContainer()->ctx_.getMediaData(mediaId_);
        if (!media)
            return;

        media->syncWithAudio_ = false;
        media->audioClock_ = 0.0;
        media->audioClockTime_ = 0;

        //qDebug() << "media->syncWithAudio_ = " << media->syncWithAudio_;

        decodedFrames_.clear();

        ThreadMessage msg(mediaId_, thread_message_type::tmt_seek_position);
        msg.x_ = (int32_t)_position;
        getMediaContainer()->postDemuxThreadMessage(msg, false);
        getMediaContainer()->postVideoThreadMessage(msg, false);
        getMediaContainer()->postAudioThreadMessage(msg, false);

        getMediaContainer()->resetFrameTimer(*media);
        getMediaContainer()->resetLastFramePts(*media);

        lastVideoPosition_ = _position;
        lastPostedPosition_ = _position;
    }

    void FFMpegPlayer::resetPosition()
    {
        lastVideoPosition_ = 0;
        lastPostedPosition_ = 0;
    }

    void FFMpegPlayer::setUpdatePositionRate(int _rate)
    {
        updatePositonRate_ = _rate;
    }

    void FFMpegPlayer::setVolume(int32_t _volume)
    {
        volume_ = _volume;

        ThreadMessage msg(mediaId_, thread_message_type::tmt_set_volume);
        msg.x_ = _volume;

        getMediaContainer()->postAudioThreadMessage(msg, false);
    }

    int32_t FFMpegPlayer::getVolume() const
    {
        return volume_;
    }

    void FFMpegPlayer::setMute(bool _mute)
    {
        ThreadMessage msg(mediaId_, thread_message_type::tmt_set_mute);
        msg.x_ = _mute;

        getMediaContainer()->postAudioThreadMessage(msg, false);

        mute_ = _mute;
    }

    bool FFMpegPlayer::isMute() const
    {
        return mute_;
    }

    QSize FFMpegPlayer::getVideoSize() const
    {
        auto media_ptr = getMediaContainer()->ctx_.getMediaData(mediaId_);
        if (!media_ptr)
            return QSize();

        auto& media = *media_ptr;

        auto size = getMediaContainer()->ctx_.getSourceSize(media);
        if (auto rot = getVideoRotation(); rot == 90 || rot == 270)
            return size.transposed();
        return size;
    }

    int32_t FFMpegPlayer::getVideoRotation() const
    {
        auto media_ptr = getMediaContainer()->ctx_.getMediaData(mediaId_);
        if (!media_ptr)
            return 0;

        return getMediaContainer()->getRotation(*media_ptr);
    }

    int64_t FFMpegPlayer::getDuration() const
    {
        if (auto media = getMediaContainer()->ctx_.getMediaData(mediaId_))
            return media->duration_;

        return -1;
    }

    bool FFMpegPlayer::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (auto renderer = weak_renderer_.lock())
        {
            QObject* objectRenderer = qobject_cast<QObject*>(renderer->getWidget());

            if (objectRenderer == _obj)
            {
                if (QEvent::Leave == _event->type())
                {
                    Q_EMIT mouseLeaveEvent(QPrivateSignal());
                }
                else if (QEvent::MouseMove == _event->type())
                {
                    const auto currentTime = std::chrono::system_clock::now();
                    if (currentTime - lastEmitMouseMove_ > mouse_move_rate)
                    {
                        lastEmitMouseMove_ = currentTime;

                        Q_EMIT mouseMoved(QPrivateSignal());
                    }
                }
            }
        }

        return QObject::eventFilter(_obj, _event);
    }

    void FFMpegPlayer::setPaused(bool _paused)
    {
        if (_paused)
        {
            pause();
        }
        else
        {
            play(false /* _init */);
        }
    }

    void FFMpegPlayer::setPausedByUser(const bool _paused)
    {
        pausedByUser_ = _paused;
    }

    bool FFMpegPlayer::isPausedByUser() const
    {
        return pausedByUser_;
    }

    QMovie::MovieState FFMpegPlayer::state() const
    {
        if (state_ == decode_thread_state::dts_playing)
            return QMovie::Running;
        else if (state_ == decode_thread_state::dts_paused)
            return QMovie::Paused;
        else
            return QMovie::NotRunning;
    }

    void FFMpegPlayer::setPreview(QPixmap _preview)
    {
        setPreview(_preview.toImage());
    }

    void FFMpegPlayer::setPreview(QImage _preview)
    {
        if (auto renderer = weak_renderer_.lock())
        {
            renderer->updateFrame(_preview);
            renderer->redraw();
        }
    }

    QPixmap FFMpegPlayer::getActiveImage() const
    {
        if (auto renderer = weak_renderer_.lock())
            return QPixmap::fromImage(renderer->getActiveImage());

        return QPixmap();
    }

    QImage FFMpegPlayer::getFirstFrame() const
    {
        return firstFrame_ ? firstFrame_->image_ : QImage();
    }

    bool FFMpegPlayer::getStarted() const
    {
        return started_;
    }

    void FFMpegPlayer::setStarted(bool _started)
    {
        started_ = _started;
    }

    void FFMpegPlayer::loadFromQueue()
    {
        if (queuedMedia_.empty())
        {
            Q_EMIT mediaChanged(-1);
            return;
        }

        mediaId_ = queuedMedia_.front();
        queuedMedia_.pop_front();

        Q_EMIT mediaChanged(mediaId_);
    }

    void FFMpegPlayer::removeFromQueue(uint32_t _media)
    {
        queuedMedia_.remove(_media);
    }

    void FFMpegPlayer::clearQueue()
    {
        for (auto i : queuedMedia_)
        {
            getMediaContainer()->ctx_.setVideoQuit(i);
            const auto message = ThreadMessage(i, thread_message_type::tmt_quit);
            getMediaContainer()->postDemuxThreadMessage(message, true, true);
            getMediaContainer()->postVideoThreadMessage(message, true, true);
            getMediaContainer()->postAudioThreadMessage(message, true, true);
        }
        queuedMedia_.clear();
    }

    bool FFMpegPlayer::queueIsEmpty() const
    {
        return queuedMedia_.empty();
    }

    int FFMpegPlayer::queueSize() const
    {
        return queuedMedia_.size();
    }

    uint32_t FFMpegPlayer::getLastMedia() const
    {
        return queuedMedia_.empty() ? mediaId_ : queuedMedia_.back();
    }

    uint32_t FFMpegPlayer::getMedia() const
    {
        return mediaId_;
    }

    uint32_t FFMpegPlayer::reserveId()
    {
        return getMediaContainer()->ctx_.reserveId();
    }

    void FFMpegPlayer::setReplay(bool _replay)
    {
        replay_ = _replay;
    }

    void FFMpegPlayer::setRestoreVolume(const int32_t _volume)
    {
        restoreVolume_ = volume_;
    }

    int32_t FFMpegPlayer::getRestoreVolume() const
    {
        return restoreVolume_;
    }

    void FFMpegPlayer::setRenderer(std::shared_ptr<FrameRenderer> _renderer)
    {
        _renderer->filterEvents(this);
        _renderer->setSizeCallback([this](auto _size) { onRendererSize(_size); });
        onRendererSize(_renderer->getWidget()->size());
        weak_renderer_ = std::move(_renderer);
    }

    //////////////////////////////////////////////////////////////////////////
    // MediaContainer
    //////////////////////////////////////////////////////////////////////////

    std::unique_ptr<MediaContainer> g_media_container;

    MediaContainer* getMediaContainer()
    {
        if (!g_media_container)
        {
            Utils::ensureMainThread();
            g_media_container = std::make_unique<MediaContainer>();
        }

        return g_media_container.get();
    }

    void ResetMediaContainer()
    {
        if (g_media_container)
            g_media_container.reset();
    }

    MediaContainer::MediaContainer()
        : is_decods_inited_(false)
        , is_demux_inited_(false)
        , demuxThread_(ctx_)
        , videoDecodeThread_(ctx_)
        , audioDecodeThread_(ctx_)
    {}

    MediaContainer::~MediaContainer()
    {
        Utils::ensureMainThread();
        if (ctx_.isQuit())
            return;

        setQuit();

        const auto message = ThreadMessage(0, thread_message_type::tmt_wake_up);
        postDemuxThreadMessage(message, true);
        postVideoThreadMessage(message, true);
        postAudioThreadMessage(message, true);

        VideoDecodeThreadWait();
        AudioDecodeThreadWait();
        DemuxThreadWait();

        ffmpeg::av_lockmgr_register(NULL);
        ffmpeg::avformat_network_deinit();
    }

    void MediaContainer::VideoDecodeThreadStart(uint32_t _mediaId)
    {
        videoDecodeThread_.start();
    }

    void MediaContainer::AudioDecodeThreadStart(uint32_t _mediaId)
    {
        audioDecodeThread_.start();
    }

    void MediaContainer::DemuxThreadStart(uint32_t _mediaId)
    {
        if (!is_demux_inited_)
        {
            demuxThread_.start();
            is_demux_inited_ = true;
        }
    }

    void MediaContainer::stopMedia(uint32_t _mediaId)
    {
        //qDebug() << "stop media " << _mediaId;

        if (active_video_ids_.count(_mediaId) == 0)
            return;

        active_video_ids_.erase(_mediaId);

        if (!active_video_ids_.empty())
            return;

        if (ctx_.isQuit())
            return;

        setQuit();

        postDemuxThreadMessage(ThreadMessage(_mediaId, thread_message_type::tmt_wake_up), true);
        postVideoThreadMessage(ThreadMessage(_mediaId, thread_message_type::tmt_wake_up), true);
        postAudioThreadMessage(ThreadMessage(_mediaId, thread_message_type::tmt_wake_up), true);

        //qDebug() << "before wait " << QTime::currentTime();
        VideoDecodeThreadWait();
        AudioDecodeThreadWait();
        DemuxThreadWait();

        clearMessageQueue();

        //qDebug() << "after wait " << QTime::currentTime();

        is_decods_inited_ = false;
        is_demux_inited_ = false;

        ffmpeg::av_lockmgr_register(NULL);
    }

    int32_t MediaContainer::getWidth(MediaData& _media) const
    {
        return ctx_.getWidth(_media);
    }

    int32_t MediaContainer::getHeight(MediaData& _media) const
    {
        return ctx_.getHeight(_media);
    }

    int32_t MediaContainer::getRotation(MediaData& _media) const
    {
        return ctx_.getRotation(_media);
    }

    int64_t MediaContainer::getDuration(MediaData& _media) const
    {
        return ctx_.getDuration(_media);
    }

    void MediaContainer::postVideoThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others)
    {
        ctx_.postVideoThreadMessage(_message, _forward, _clear_others);
    }

    void MediaContainer::updateVideoScaleSize(const uint32_t _mediaId, const QSize _sz)
    {
        ctx_.updateScaledVideoSize(_mediaId, _sz);
    }

    void MediaContainer::postDemuxThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others)
    {
        ctx_.postDemuxThreadMessage(_message, _forward, _clear_others);
    }

    void MediaContainer::postAudioThreadMessage(const ThreadMessage& _message, bool _forward, bool _clear_others)
    {
        ctx_.postAudioThreadMessage(_message, _forward, _clear_others);
    }

    void MediaContainer::clearMessageQueue()
    {
        ctx_.clearMessageQueue();
    }

    void MediaContainer::resetFrameTimer(MediaData& _media)
    {
        ctx_.resetFrameTimer(_media);
    }

    void MediaContainer::resetLastFramePts(MediaData& _media)
    {
        ctx_.resetLastFramePts(_media);
    }

    bool MediaContainer::isQuit(uint32_t _mediaId) const
    {
        return ctx_.isQuit();
    }

    void MediaContainer::setQuit(bool _val)
    {
        //qDebug() << "setQuit " << _val << " " << QTime::currentTime();
        ctx_.setQuit(_val);
    }

    bool MediaContainer::openFile(MediaData& _media)
    {
        return ctx_.openFile(_media);
    }

    void MediaContainer::closeFile(MediaData& _media)
    {
        ctx_.closeFile(_media);
    }

    double MediaContainer::computeDelay(double _picturePts, MediaData& _media)
    {
        return ctx_.computeDelay(_picturePts, _media);
    }

    uint32_t MediaContainer::init(uint32_t _id)
    {
        auto id = ctx_.addVideo(_id);

        if (active_video_ids_.empty())
        {
            ffmpeg::av_init_packet(&quit_pkt);
            quit_pkt.data = (uint8_t*)&quit_pkt;
        }

        auto media_ptr = getMediaContainer()->ctx_.getMediaData(id);
        if (media_ptr)
        {
            auto& media = *media_ptr;

            ctx_.init(media);

            if (active_video_ids_.empty())
            {
                setQuit(false);
                Ui::GetSoundsManager();
            }
            active_video_ids_.insert(id);
        }
        else
        {
            im_assert(!"MediaContainer: fail on init");
        }

        return id;
    }

    void MediaContainer::DemuxThreadWait()
    {
        demuxThread_.wait();
    }

    void MediaContainer::VideoDecodeThreadWait()
    {
        videoDecodeThread_.wait();
    }

    void MediaContainer::AudioDecodeThreadWait()
    {
        audioDecodeThread_.wait();
    }

    MediaContainer* getMediaPreview(const QString& _media)
    {
        Ui::ThreadMessage msg;

        msg.str_ = _media;
        msg.message_ = thread_message_type::tmt_get_first_frame;

        MediaContainer* container = getMediaContainer();

        container->postDemuxThreadMessage(msg, false);

        return container;
    }

}
