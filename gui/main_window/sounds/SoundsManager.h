#pragma once

#include "device_monitoring/interface/device_monitoring.h"

namespace Ui
{
    enum class SoundType
    {
        Unknown = -1,
        IncomingMail = 0,
        IncomingMessage,
        OutgoingMessage,
        StartPtt,
        PttLimit,
        RemovePtt,

        Size
    };

    struct PlayingData
    {
        PlayingData()
            : Source_(0)
            , Buffer_(0)
            , Id_(SoundType::Unknown)
        {
        }

        void init(const SoundType _sound);
        void setVolume(const SoundType _sound);
        void setBuffer(const QByteArray& data, qint64 freq, qint64 fmt)
        {
            setBuffer(data.data(), data.size(), freq, fmt);
        }
        void setBuffer(const char* data, size_t size, qint64 freq, qint64 fmt);
        std::chrono::milliseconds play();
        void pause();
        void stop();
        void clear();
        void free();
        void clearData();
        bool isEmpty() const;
        std::chrono::milliseconds calcDuration();
        openal::ALenum state() const;

        size_t totalLengthInSamples() const;
        size_t currentSampleOffset() const;
        void setCurrentSampleOffset(size_t _offset);

        openal::ALuint Source_;
        openal::ALuint Buffer_;
        SoundType Id_;
    };

    class SoundsManager : public QObject, device::DeviceMonitoringCallback
    {
        Q_OBJECT
Q_SIGNALS:
        void pttPaused(SoundType id, int sampleOffset, QPrivateSignal);
        void pttFinished(SoundType, bool ended, QPrivateSignal);
        void needUpdateDeviceTimer(QPrivateSignal);

        void deviceListChangedInternal(QPrivateSignal);

        void deviceListChanged(QPrivateSignal);


    public:
        SoundsManager();
        ~SoundsManager();

        std::chrono::milliseconds playSound(SoundType _type);

        SoundType playPtt(const QString& file, SoundType id, int& duration, double progress = 0);
        SoundType playPtt(const char* data, size_t size, qint64 freq, qint64 fmt, SoundType id, int& duration, size_t sampleOffset = 0);
        void stopPtt(SoundType id);
        void pausePtt(SoundType id);

        bool isPaused(SoundType id) const;

        size_t sampleOffset(SoundType id) const;
        bool setSampleOffset(SoundType id, size_t _offset);
        void setProgressOffset(SoundType id, double percent);

        void delayDeviceTimer();
        void sourcePlay(unsigned source);

        void callInProgress(bool value);

        void checkAudioDevice();

        void DeviceMonitoringListChanged() override;

    private Q_SLOTS:
        void timedOut();
        void checkPttState();
        void contactChanged(const QString&);
        void deviceTimeOut();

        void initOpenAl();
        void closeOpenAlDevice();
        void shutdownOpenAl();
        void updateDeviceTimer();

        void onDeviceListChanged();

    private:
        struct PttBuffer
        {
            QByteArray data;
            qint64 frequency = 0;
            qint64 format = 0;
        };
        std::optional<SoundType> checkPlayPtt(SoundType id, int& duration, const std::optional<size_t>& sampleOffset);
        std::optional<PttBuffer> getBuffer(const QString& file);
        SoundType playPttImpl(const char* data, size_t size, qint64 freq, qint64 fmt, int& duration, size_t sampleOffset);
        void initPlayingData(PlayingData& _data, const QString& _file, const SoundType& _sound);

        void initSounds();
        void deInitSounds();

        bool canPlaySound(SoundType _type);
        static std::string selectedDeviceName();

    private:
        bool CallInProgress_;
        bool CanPlayIncoming_;

        std::vector<PlayingData> sounds_;

        int AlId;
        openal::ALCdevice *AlAudioDevice_;
        openal::ALCcontext *AlAudioContext_;
        PlayingData CurPlay_;
        PlayingData PrevPlay_;
        bool AlInited_;

        QTimer* Timer_;
        QTimer* PttTimer_;
        QTimer* DeviceTimer_;

        std::unique_ptr<device::DeviceMonitoring> deviceMonitoring_;

        std::mutex mutex_;
    };

    SoundsManager* GetSoundsManager();
    void ResetSoundsManager();
}
