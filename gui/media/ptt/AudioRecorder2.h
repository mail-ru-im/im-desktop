#pragma once

namespace ptt
{
    class AmplitudeCalc;
    struct Buffer;
    struct StatInfo;

    enum class State2
    {
        Stopped,
        Recording,
        Paused,
    };
    enum class Error2
    {
        DeviceInit,
        Convert,
        BufferOOM
    };

    class AudioRecorder2 : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void dataReady(const QByteArray&, const QString&, const ptt::StatInfo&, QPrivateSignal) const;
        void spectrum(const QVector<double>&, const QString&, QPrivateSignal) const;
        void durationChanged(std::chrono::milliseconds, const QString&, QPrivateSignal) const;
        void playPositionChanged(int, const QString&, QPrivateSignal) const;
        void stateChanged(ptt::State2, const QString&, QPrivateSignal) const;
        void error(ptt::Error2, const QString&, QPrivateSignal) const;
        void aacReady(const QString&, const QString&, std::chrono::seconds, const ptt::StatInfo& statInfo, QPrivateSignal) const;
        void pcmDataReady(const ptt::Buffer&, const QString&, QPrivateSignal) const;
        void limitReached(const QString&, QPrivateSignal) const;
        void tooShortRecord(const QString&, QPrivateSignal) const;

    Q_SIGNALS:
        // internals
        void recordS(QPrivateSignal);
        void pauseRecordS(QPrivateSignal);
        void stopS(QPrivateSignal);
        void playS(QPrivateSignal);
        void pausePlayS(QPrivateSignal);
        void getDataS(const ptt::StatInfo&, QPrivateSignal);
        void getDurationS(QPrivateSignal);
        void getPcmDataS(QPrivateSignal);
        void deviceListChangedS(QPrivateSignal);
        void clearS(QPrivateSignal);

    public:
        explicit AudioRecorder2(QObject* _parent, const QString& _contact, std::chrono::seconds _maxDuration, std::chrono::seconds _minDuration);
        ~AudioRecorder2();

        void record();
        void pauseRecord();
        void stop();
        void getData(ptt::StatInfo&&);
        void deviceListChanged();
        void getDuration();
        void getPcmData();
        void clear();

        static constexpr int rate() noexcept { return 16000; }
        static constexpr int format() noexcept { return AL_FORMAT_MONO16; }
        static constexpr int channelsCount() noexcept { return 1; }
        static constexpr int bitesPerSample() noexcept { return sizeof(int16_t) * 8; }

        static bool hasDevice(const char* deviceName = nullptr);

    private:
        void recordImpl();
        void pauseRecordImpl();
        void stopImpl();
        void getDataImpl(const ptt::StatInfo&);
        void getPcmDataImpl();
        void onDataReadyImpl(const QByteArray&, const QString&, const ptt::StatInfo&);
        void clearImpl();

        void setStateImpl(State2 _state);
        State2 getStateImpl() const;

        std::chrono::milliseconds currentDurationImpl() const;
        void setDurationImpl(std::chrono::milliseconds _duration);

        void onTimer();
        void resetBuffer();

        bool insertWavHeader();

        bool needReinit() const noexcept;

        void setReinit(bool _v);

        bool reInit();

        static constexpr bool isLoopRecordingAvailable() noexcept { return false; }

    private:
        const QString contact_; // TODO remove me
        const std::chrono::seconds maxDuration_;
        const std::chrono::seconds minDuration_;
        std::chrono::milliseconds currentDurationImpl_ = std::chrono::milliseconds::zero();

        QByteArray buffer_;

        State2 state_ = State2::Stopped;

        std::vector<std::byte> internalBuffer_;

        QTimer* timer_;

        openal::ALCdevice* device_ = nullptr;
        bool needReinitDevice_ = true;
        std::string deviceName_;
        std::unique_ptr<AmplitudeCalc> fft_;
    };
}