#pragma once

#include "device_monitoring/interface/device_monitoring.h"

namespace Ui
{
    struct PlayingData
    {
        PlayingData()
            : Source_(0)
            , Buffer_(0)
            , Id_(-1)
        {
        }

        void init();
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

        size_t currentSampleOffset() const;
        void setCurrentSampleOffset(size_t _offset);

        openal::ALuint Source_;
        openal::ALuint Buffer_;
        int Id_;
    };

    class SoundsManager : public QObject, device::DeviceMonitoringCallback
    {
        Q_OBJECT
Q_SIGNALS:
        void pttPaused(int id, int sampleOffset, QPrivateSignal);
        void pttFinished(int, bool ended, QPrivateSignal);
        void needUpdateDeviceTimer(QPrivateSignal);

        void deviceListChangedInternal(QPrivateSignal);

        void deviceListChanged(QPrivateSignal);

    public:
        enum class Sound
        {
            IncomingMail = 0,
            IncomingMessage,
            OutgoingMessage,
            StartPtt,
            PttLimit,
            RemovePtt,

            Size
        };

    public:
        SoundsManager();
        ~SoundsManager();

        std::chrono::milliseconds playSound(Sound _type);

        int playPtt(const QString& file, int id, int& duration);
        int playPtt(const char* data, size_t size, qint64 freq, qint64 fmt, int id, int& duration, size_t sampleOffset = 0);
        void stopPtt(int id);
        void pausePtt(int id);

        bool isPaused(int id) const;

        size_t sampleOffset(int id) const;
        bool setSampleOffset(int id, size_t _offset);

        void delayDeviceTimer();
        void sourcePlay(unsigned source);

        void callInProgress(bool value);

        void reinit();

        void DeviceMonitoringListChanged() override;
        void DeviceMonitoringBluetoothHeadsetChanged(bool _connected) override;

    private Q_SLOTS:
        void timedOut();
        void checkPttState();
        void contactChanged(const QString&);
        void deviceTimeOut();

        void initOpenAl();
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
        std::optional<int> checkPlayPtt(int id, int& duration, const std::optional<size_t>& sampleOffset);
        std::optional<PttBuffer> getBuffer(const QString& file);
        int playPttImpl(const char* data, size_t size, qint64 freq, qint64 fmt, int& duration, size_t sampleOffset);
        void initPlayingData(PlayingData& _data, const QString& _file);

        void initSounds();
        void deInitSounds();

        bool canPlaySound(Sound _type);

    private:
        bool CallInProgress_;
        bool CanPlayIncoming_;

        PlayingData Incoming_;
        PlayingData Outgoing_;
        PlayingData Mail_;

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

        std::string deviceName_;
    };

    SoundsManager* GetSoundsManager();
    void ResetSoundsManager();
    void ReinitSoundsManager();
}
