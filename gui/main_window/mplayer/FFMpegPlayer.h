#pragma once

#include "ffmpeg.h"
#include "LottieHandle.h"
#include "../../memory_stats/FFmpegPlayerMemMonitor.h"

Q_DECLARE_LOGGING_CATEGORY(ffmpegPlayer)

namespace Ui
{
    namespace audio
    {
        const int32_t num_buffers = 4;
        const int32_t outFrequency = 48000;
        const int32_t blockSize = 4096;
        const ffmpeg::AVSampleFormat outFormat = ffmpeg::AV_SAMPLE_FMT_S16;
        const int64_t outChannelLayout = AV_CH_LAYOUT_STEREO;
        const qint32 outChannels = 2;
    }

    enum class decode_thread_state
    {
        dts_none = 0,
        dts_playing = 1,
        dts_paused = 2,
        dts_end_of_media = 3,
        dts_seeking = 4,
        dts_failed = 5
    };

    //////////////////////////////////////////////////////////////////////////
    // ThreadMessage
    //////////////////////////////////////////////////////////////////////////
    enum class thread_message_type
    {
        tmt_unknown = 0,
        tmt_update_scaled_size = 1,
        tmt_quit = 3,
        tmt_set_volume = 4,
        tmt_set_mute = 5,
        tmt_pause = 6,
        tmt_play = 7,
        tmt_get_next_video_frame = 8,
        tmt_seek_position = 9,
        tmt_stream_seeked = 10,
        tmt_set_finished = 11,
        tmt_init = 12,
        tmt_open_streams = 13,
        tmt_close_streams = 14,
        tmt_wake_up = 15,
        tmt_get_first_frame = 16,
        tmt_reinit_audio = 17,
        tmt_empty_frame = 18,

        last = 19
    };

    struct ThreadMessage
    {
        thread_message_type message_ = thread_message_type::tmt_unknown;

        int32_t x_ = 0;
        int32_t y_ = 0;
        uint32_t videoId_ = std::numeric_limits<uint32_t>::max();
        QString str_;

        QImage emptyFrame_;

        ThreadMessage(uint32_t _videoId = std::numeric_limits<uint32_t>::max(), thread_message_type _message = thread_message_type::tmt_unknown)
            : message_(_message)
            , videoId_(_videoId)
        {
        }

        void setEmptyFrame(QImage&& _image)
        {
            emptyFrame_ = std::move(_image);
        }
    };
    using ThreadMessageList = std::list<ThreadMessage>;


    //////////////////////////////////////////////////////////////////////////
    // ThreadMessagesQueue
    //////////////////////////////////////////////////////////////////////////
    class ThreadMessagesQueue
    {
        std::mutex queue_mutex_;

        QSemaphore condition_;

        ThreadMessageList messages_;

    public:

        bool getMessage(ThreadMessage& _message, std::function<bool()> _isQuit, int32_t _waitTimeout);
        bool getAllMessages(ThreadMessageList& _messages, std::function<bool()> _isQuit, int32_t _waitTimeout);
        void pushMessage(const ThreadMessage& _message, bool _forward, bool _clearOthers);
        void clear();
    };


    //////////////////////////////////////////////////////////////////////////
    // PacketQueue
    //////////////////////////////////////////////////////////////////////////
    class PacketQueue
    {
        mutable std::mutex mutex_;

        std::list<ffmpeg::AVPacket> list_;

        std::atomic<int32_t> packets_;
        std::atomic<int32_t> size_;

    public:

        PacketQueue();
        ~PacketQueue();

        void freeData(std::function<void(ffmpeg::AVPacket& _packet)> _callback = {});

        void push(ffmpeg::AVPacket* _packet);
        bool get(ffmpeg::AVPacket& _packet);

        int32_t getSize() const;
        int32_t getPackets() const;
    };


    //////////////////////////////////////////////////////////////////////////
    // DecodeAudioData
    //////////////////////////////////////////////////////////////////////////
    struct DecodeAudioData
    {
        ffmpeg::AVFrame* frame_ = nullptr;

        uint8_t** outSamplesData_ = nullptr;

        ffmpeg::SwrContext* swrContext_ = nullptr;

        openal::ALuint uiBuffers_[audio::num_buffers];
        openal::ALuint uiSource_ = 0;
        openal::ALuint uiBuffer_ = 0;
        openal::ALint buffersProcessed_ = 0;
        openal::ALint iState_ = 0;
        openal::ALint iQueuedBuffers_ = 0;

        int iloop_ = 0;

        uint64_t layout_ = 0;
        int32_t channels_ = 0;
        int32_t fmt_ = AL_FORMAT_STEREO16;
        int32_t sampleSize_ = 2 * sizeof(uint16_t);
        int32_t freq_ = 0;
        int32_t maxResampleSamples_ = 1024;
        int32_t srcRate_ = audio::outFrequency;
        int32_t dstRate_ = audio::outFrequency;

        bool queueInited_ = false;

        ffmpeg::AVCodecContext* audioCodecContext_ = nullptr;
        decode_thread_state state_ = decode_thread_state::dts_playing;
    };

    struct MediaData
    {
        bool syncWithAudio_ = false;

        ffmpeg::AVStream* videoStream_ = nullptr;
        ffmpeg::AVStream* audioStream_ = nullptr;
        ffmpeg::AVFormatContext* formatContext_ = nullptr;
        ffmpeg::AVCodecContext* codecContext_ = nullptr;

        QSharedPointer<PacketQueue> videoQueue_;
        QSharedPointer<PacketQueue> audioQueue_;

        bool needUpdateSwsContext_ = false;
        ffmpeg::SwsContext* swsContext_ = nullptr;
        std::vector<uint8_t> scaledBuffer_;
        ffmpeg::AVFrame* frameRGB_ = nullptr;
        DecodeAudioData audioData_;

        QSize size_;
        int32_t rotation_ = 0;
        int64_t duration_ = 0;
        int64_t seek_position_ = -1;

        QSize scaledSize_;

        double frameTimer_ = 0.0;
        double frameLastPts_ = 0.0;
        double frameLastDelay_ = 0.0;

        bool frameLastPtsReset_ = false;

        bool startTimeVideoSet_ = false;
        bool startTimeAudioSet_ = false;
        int64_t startTimeVideo_ = 0;
        int64_t startTimeAudio_ = 0;

        double videoClock_ = 0.0;

        double audioClock_ = 0.0;
        int64_t audioClockTime_ = 0;

        bool mute_ = true;
        int32_t volume_ = 100;

        bool audioQuitRecv_ = false;
        bool videoQuitRecv_ = false;
        bool demuxQuitRecv_ = false;
        bool streamClosed_ = false;

        std::queue<double> audio_queue_ptss_;

        bool isLottie_ = false;
        LottieHandle lottieRenderer_;

        MediaData();

        bool hasVideo() const { return !!videoStream_ || lottieRenderer_; }
        bool hasAudio() const { return !!audioStream_; }
    };

    struct VideoData
    {
        decode_thread_state current_state_ = decode_thread_state::dts_none;
        bool eof_ = false;
        bool stream_finished_ = false;
        std::list<QImage> emptyFrames_;
        std::weak_ptr<MediaData> media_;
    };

    struct AudioData
    {
        bool eof_ = false;
        bool stream_finished_ = false;
        double offset_ = 0.0;
        int64_t last_sync_time_ = 0;
        std::weak_ptr<MediaData> media_;
    };

    struct DemuxData
    {
        bool inited = false;
        decode_thread_state state = decode_thread_state::dts_playing;
        bool eof = false;
        int64_t seekPosition = -1;
        std::weak_ptr<MediaData> media_;
    };

    struct LottieRenderData
    {
        int videoId_ = -1;
        int frameNo_ = -1;
        QImage frame_;
        std::future<rlottie::Surface> fut_;
        std::weak_ptr<MediaData> media_;

        bool isValid() const noexcept { return videoId_ >= 0; }
    };

    //////////////////////////////////////////////////////////////////////////
    // VideoContext
    //////////////////////////////////////////////////////////////////////////
    class VideoContext : public QObject
    {
        Q_OBJECT

        friend class Ui::FFmpegPlayerMemMonitor;

    Q_SIGNALS:

        void dataReady(uint32_t _videoId);

        void nextframeReady(uint32_t _videoId, const QImage& _frame, double _pts, bool _eof);
        void videoSizeChanged(QSize _sz);
        void streamsOpened(uint32_t _videoId);
        void streamsOpenFailed(uint32_t _videoId);
        void audioTime(uint32_t _videoId, int64_t _avtime, double _offset);

        void seekedV(uint32_t _videoId);
        void seekedA(uint32_t _videoId);

        void audioQuit(uint32_t _videoId);
        void videoQuit(uint32_t _videoId);
        void demuxQuit(uint32_t _videoId);
        void streamsClosed(uint32_t _videoId);

        void demuxInited(const uint32_t _videoId);

    public Q_SLOTS:

        void onAudioQuit(uint32_t _videoId);
        void onVideoQuit(uint32_t _videoId);
        void onDemuxQuit(uint32_t _videoId);
        void onStreamsClosed(uint32_t _videoId);

    public:
        ThreadMessagesQueue videoThreadMessagesQueue_;
    private:

        std::atomic<bool> quit_;
        uint32_t curr_id_;

        mutable std::unordered_map<uint32_t, std::shared_ptr<MediaData>> mediaData_;
        mutable std::mutex mediaDataMutex_;

        mutable std::unordered_map<uint32_t, bool> activeVideos_;
        mutable std::mutex activeVideosMutex_;

        ThreadMessagesQueue demuxThreadMessageQueue_;
        ThreadMessagesQueue audioThreadMessageQueue_;

    private:

        static ffmpeg::AVStream* openStream(int32_t _type, ffmpeg::AVFormatContext* _context);
        static void closeStream(ffmpeg::AVStream* _stream);
        void sendCloseStreams(uint32_t _videoId);

    public:

        VideoContext();
        virtual ~VideoContext();

        void init(MediaData& _media);
        uint32_t addVideo(uint32_t id = 0);
        void deleteVideo(uint32_t _videoId);
        uint32_t reserveId();

        int32_t getVideoStreamIndex(MediaData& _media) const;
        int32_t getAudioStreamIndex(MediaData& _media) const;

        int32_t readAVPacket(/*OUT*/ffmpeg::AVPacket* _packet, MediaData& _media);
        int32_t readAVPacketPause(MediaData& media);

        int32_t readAVPacketPlay(MediaData& _media);
        bool isEof(int32_t _error, MediaData& _media);
        bool isStreamError(MediaData& _media);

        bool getNextVideoFrame(/*OUT*/ffmpeg::AVFrame* _frame, ffmpeg::AVPacket* _packet, VideoData& _videoData, MediaData& _media, uint32_t _videoId);

        void pushVideoPacket(ffmpeg::AVPacket* _packet, MediaData& _media);
        int32_t getVideoQueuePackets(MediaData& _media) const;
        int32_t getVideoQueueSize(MediaData& _media) const;
        void pushAudioPacket(ffmpeg::AVPacket* _packet, MediaData& _media);
        int32_t getAudioQueuePackets(MediaData& _media) const;
        int32_t getAudioQueueSize(MediaData& _media) const;
        void clearAudioQueue(MediaData& _media);
        void clearVideoQueue(MediaData& _media);

        void cleanupOnExit();

        bool isQuit() const;
        void setQuit(bool _val = true);

        bool isVideoQuit(int _videoId) const;
        void setVideoQuit(int _videoId);

        bool openStreams(uint32_t _mediaId, const QString& _file, MediaData& _media);
        bool openFile(MediaData& _media);
        void closeFile(MediaData& _media);

        int32_t getWidth(MediaData& _media) const;
        int32_t getHeight(MediaData& _media) const;
        int32_t getRotation(MediaData& _media) const;
        int64_t getDuration(MediaData& _media) const;

        QSize getSourceSize(MediaData& _media) const;
        QSize getTargetSize(MediaData& _media) const;

        //don't call these functions in main thread
        static QImage getFirstFrame(const QString& _file);
        static int64_t getDuration(const QString& _file);
        static bool getGotAudio(const QString& _file);

        double getVideoTimebase(MediaData& _media);
        double getAudioTimebase(MediaData& _media);
        double synchronizeVideo(ffmpeg::AVFrame* _frame, double _pts, MediaData& _media);
        double computeDelay(double _picturePts, MediaData& _media);

        bool initDecodeAudioData(MediaData& _media);
        void freeDecodeAudioData(MediaData& _media);

        bool readFrameAudio(
            ffmpeg::AVPacket* _packet,
            AudioData& _audioData,
            openal::ALvoid** _frameData,
            openal::ALsizei& _frameDataSize,
            bool& _flush,
            int& _seekCount,
            MediaData& _media,
            uint32_t _videoId);

        bool playNextAudioFrame(
            ffmpeg::AVPacket* _packet,
            /*in out*/ AudioData& _audioData,
            bool& _flush,
            int& _seekCount,
            MediaData& _media,
            uint32_t _videoId);

        void flushBuffers(MediaData& _media);
        void cleanupAudioBuffers(MediaData& _media);
        void suspendAudio(MediaData& _media);
        void stopAudio(MediaData& _media);

        void updateScaledVideoSize(uint32_t _videoId, const QSize& _sz);

        void postVideoThreadMessage(const ThreadMessage& _message, bool _forward, bool _clearOthers = false);
        bool getVideoThreadMessage(ThreadMessage& _message, int32_t _waitTimeout);
        bool getAllVideoThreadMessages(ThreadMessageList& _messages, int32_t _waitTimeout);

        void postDemuxThreadMessage(const ThreadMessage& _message, bool _forward, bool _clearOthers = false);
        bool getDemuxThreadMessage(ThreadMessage& _message, int32_t _waitTimeout);

        void postAudioThreadMessage(const ThreadMessage& _message, bool _forward, bool _clearOthers = false);
        bool getAudioThreadMessage(ThreadMessage& _message, int32_t _waitTimeout);

        void clearMessageQueue();

        decode_thread_state getAudioThreadState(MediaData& _media) const;
        void setAudioThreadState(const decode_thread_state& _state, MediaData& _media);

        bool updateScaleContext(MediaData& _media, const QSize _sz);
        void freeScaleContext(MediaData& _media);

        void setVolume(int32_t _volume, MediaData& _media);
        void setMute(bool _mute, MediaData& _media);

        void resetFrameTimer(MediaData& _media);
        void resetLastFramePts(MediaData& _media);
        bool seekMs(uint32_t _videoId, int _tsms, MediaData& _media);
        bool seekFrame(uint32_t _videoId, int64_t _ts_video, int64_t _ts_audio, MediaData& _media);

        void flushVideoBuffers(MediaData& _media);
        void flushAudioBuffers(MediaData& _media);

        void resetVideoClock(MediaData& _media);
        void resetAudioClock(MediaData& _media);

        void setStartTimeVideo(const int64_t& _startTime, MediaData& _media);
        const int64_t& getStartTimeVideo(MediaData& _media) const;

        void setStartTimeAudio(const int64_t& _startTime, MediaData& _media);

        const int64_t& getStartTimeAudio(MediaData& _media) const;

        std::shared_ptr<MediaData> getMediaData(uint32_t _videoId) const;

        void initFlushPacket(ffmpeg::AVPacket& _packet);
    };


    //////////////////////////////////////////////////////////////////////////
    // DemuxThread
    //////////////////////////////////////////////////////////////////////////
    class DemuxThread : public QThread
    {
        VideoContext& ctx_;

    protected:

        virtual void run() override;

    public:

        explicit DemuxThread(VideoContext& _ctx);
    };


    //////////////////////////////////////////////////////////////////////////
    // VideoDecodeThread
    //////////////////////////////////////////////////////////////////////////
    class VideoDecodeThread : public QThread
    {

        VideoContext& ctx_;

    protected:

        virtual void run() override;

        LottieRenderData renderLottieFrame(VideoData& _data, MediaData& _media, int32_t _videoId);
        void renderVideoFrame(VideoData& _data, MediaData& _media, ffmpeg::AVFrame* _frame, ffmpeg::AVPacket& _av_packet, int32_t _videoId);
        QImage getLastFrame(VideoData& _data) const;

        bool processDataMessage(VideoData& _data, ThreadMessage& _msg) const;

    public:

        explicit VideoDecodeThread(VideoContext& _ctx);
    };



    //////////////////////////////////////////////////////////////////////////
    // AudioDecodeThread
    //////////////////////////////////////////////////////////////////////////
    class AudioDecodeThread : public QThread
    {
        VideoContext& ctx_;

    protected:

        virtual void run() override;

    public:

        explicit AudioDecodeThread(VideoContext& _ctx);
    };

    class MediaContainer : public QObject
    {
        Q_OBJECT

    public:

        MediaContainer();
        ~MediaContainer();

    Q_SIGNALS:

        void frameReady(const QImage& _frame, const QString& _file);

    public:

        void VideoDecodeThreadStart(uint32_t _mediaId);
        void AudioDecodeThreadStart(uint32_t _mediaId);
        void DemuxThreadStart(uint32_t _mediaId);

        int32_t getWidth(MediaData& _media) const;
        int32_t getHeight(MediaData& _media) const;
        int32_t getRotation(MediaData& _media) const;
        int64_t getDuration(MediaData& _media) const;

        void postVideoThreadMessage(const ThreadMessage& _message, bool _forward, bool _clearOthers = false);
        void postDemuxThreadMessage(const ThreadMessage& _message, bool _forward, bool _clearOthers = false);
        void postAudioThreadMessage(const ThreadMessage& _message, bool _forward, bool _clearOthers = false);

        void clearMessageQueue();

        void resetFrameTimer(MediaData& _media);
        void resetLastFramePts(MediaData& _media);

        bool isQuit(uint32_t _mediaId) const;
        void setQuit(bool _val = true);

        bool openFile(MediaData& _media);
        void closeFile(MediaData& _media);

        double computeDelay(double _picturePts, MediaData& _media);

        uint32_t init(uint32_t _id = 0);

        void stopMedia(uint32_t _mediaId);
        void updateVideoScaleSize(const uint32_t _mediaId, const QSize _sz);

        VideoContext ctx_;

        bool is_decods_inited_;

    private:

        bool is_demux_inited_;
        std::unordered_set<uint32_t> active_video_ids_;

        DemuxThread demuxThread_;
        VideoDecodeThread videoDecodeThread_;
        AudioDecodeThread audioDecodeThread_;

        void DemuxThreadWait();
        void VideoDecodeThreadWait();
        void AudioDecodeThreadWait();
    };

    MediaContainer* getMediaContainer();

    void ResetMediaContainer();

    MediaContainer* getMediaPreview(const QString& _media);


    class FrameRenderer;

    //////////////////////////////////////////////////////////////////////////
    // FFMpegPlayer
    //////////////////////////////////////////////////////////////////////////
    class FFMpegPlayer : public QObject
    {
        Q_OBJECT

        friend class Ui::FFmpegPlayerMemMonitor;

        uint32_t mediaId_;

        int32_t restoreVolume_;
        int32_t volume_;

        std::list<uint32_t> queuedMedia_;

        QTimer* timer_;

        std::weak_ptr<FrameRenderer> weak_renderer_;

        bool isFirstFrame_;

        int updatePositonRate_;

        struct DecodedFrame
        {
            QImage image_;

            double pts_;

            bool eof_;

            DecodedFrame(const QImage& _image, const double _pts) : image_(_image), pts_(_pts), eof_(false) {}
            DecodedFrame(bool _eof) : pts_(0.0), eof_(_eof) {}
        };

        std::unique_ptr<DecodedFrame> firstFrame_;

        std::list<DecodedFrame> decodedFrames_;

        decode_thread_state state_;

        qint64 lastVideoPosition_;
        qint64 lastPostedPosition_;

        std::chrono::system_clock::time_point prev_frame_time_;

        std::chrono::system_clock::time_point lastEmitMouseMove_;

        QMetaObject::Connection openStreamsConnection_;

        bool stopped_;
        bool started_;
        bool continius_;
        bool replay_;
        bool mute_;
        bool dataReady_;
        bool pausedByUser_;

        int seek_request_id_;

    private:

        void updateVideoPosition(const DecodedFrame& _frame);
        bool canPause() const;

    Q_SIGNALS:

        void durationChanged(qint64 _duration);
        void positionChanged(qint64 _position);
        void mediaFinished();
        void mouseMoved(QPrivateSignal);
        void mouseLeaveEvent(QPrivateSignal);
        void fileLoaded();
        void mediaChanged(qint32);
        void dataReady();
        void streamsOpenFailed(uint32_t _videoId);
        void paused();
        void played();
        void firstFrameReady();

    private Q_SLOTS:

        void onTimer();


        void onStreamsOpened(uint32_t _videoId);
        void onRendererSize(const QSize _sz);
        void onAudioTime(uint32_t _videoId, int64_t _avtime, double _offset);
        void onDataReady(uint32_t _videoId);
        void seekedV(uint32_t _videoId);
        void seekedA(uint32_t _videoId);

        void onDeviceListChanged();

    public Q_SLOTS:

        uint32_t stop();

    public:

        void onNextFrameReady(const QImage& _image, double _pts, bool _eof);

        FFMpegPlayer(QWidget* _parent, std::shared_ptr<FrameRenderer> _renderer, bool _continius = false);
        virtual ~FFMpegPlayer();

        bool openMedia(const QString& _mediaPath, bool _isLottie = false, bool _isLottieInstantPreview = false, uint32_t id = 0);

        void play(bool _init);
        void pause();

        void setPaused(const bool _paused);
        void setPausedByUser(const bool _paused);
        bool isPausedByUser() const;

        void setPosition(int64_t _position);
        void resetPosition();
        void setUpdatePositionRate(int _rate);

        void setVolume(int32_t _volume);
        int32_t getVolume() const;

        void setMute(bool _mute);
        bool isMute() const;

        QSize getVideoSize() const;
        int32_t getVideoRotation() const;
        int64_t getDuration() const;

        QMovie::MovieState state() const;

        void setPreview(QPixmap _preview);
        void setPreview(QImage _preview);
        QPixmap getActiveImage() const;
        QImage getFirstFrame() const;

        bool getStarted() const;
        void setStarted(bool _started);

        void loadFromQueue();
        void removeFromQueue(uint32_t _media);
        void clearQueue();
        bool queueIsEmpty() const;
        int queueSize() const;

        uint32_t getLastMedia() const;
        uint32_t getMedia() const;

        uint32_t reserveId();

        void setReplay(bool _replay);

        void setRestoreVolume(const int32_t _volume);
        int32_t getRestoreVolume() const;

        void setRenderer(std::shared_ptr<FrameRenderer> _renderer);

    protected:

        virtual bool eventFilter(QObject* _obj, QEvent* _event) override;

        void seeked(uint32_t _videoId, const bool _fromAudio);
    };

    class PlayersList : public QObject
    {
        Q_OBJECT

    public:
        PlayersList();
        virtual ~PlayersList();

        void addPlayer(FFMpegPlayer* _player);
        void removePlayer(FFMpegPlayer* _player);

        static PlayersList& getPlayersList();

    public Q_SLOTS:

        void onNextFrameReady(uint32_t _videoId, const QImage& _image, double _pts, bool _eof);

    private:

        std::vector<FFMpegPlayer*> playersList_;
    };
}
