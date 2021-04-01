#include "stdafx.h"
#include "SoundsManager.h"

#include "../../gui_settings.h"
#include "../contact_list/ContactListModel.h"
#include "../../utils/InterConnector.h"
#include "../../core_dispatcher.h"

#ifdef __APPLE__
#include "../../utils/macos/mac_support.h"
#endif

#include "MpegLoader.h"

Q_LOGGING_CATEGORY(soundManager, "soundManager")

namespace openal
{

#ifndef USE_SYSTEM_OPENAL
    #define AL_ALEXT_PROTOTYPES
    #include <AL/alext.h>
#endif
}

namespace
{
    constexpr int IncomingMessageInterval = 3000;
    constexpr int PttCheckInterval = 100;
    constexpr int DeviceCheckInterval = 60 * 1000;

    QString getFilePath(Ui::SoundsManager::Sound _s)
    {
        switch (_s)
        {
        case Ui::SoundsManager::Sound::IncomingMail:
            return qsl(":/sounds/mail");
        case Ui::SoundsManager::Sound::IncomingMessage:
            return qsl(":/sounds/incoming");
        case Ui::SoundsManager::Sound::OutgoingMessage:
            return qsl(":/sounds/outgoing");
        case Ui::SoundsManager::Sound::StartPtt:
            return qsl(":/sounds/ptt_start");
        case Ui::SoundsManager::Sound::PttLimit:
            return qsl(":/sounds/ptt_limit");
        case Ui::SoundsManager::Sound::RemovePtt:
            return qsl(":/sounds/ptt_remove");
        case Ui::SoundsManager::Sound::Size:
        default:
            Q_UNREACHABLE();
            return QString();
        }
    }
}

namespace Ui
{
    void PlayingData::init()
    {
        openal::alGenSources(1, &Source_);
        openal::alSourcef(Source_, AL_PITCH, 1.f);
        openal::alSourcef(Source_, AL_GAIN, 1.f);
        openal::alSource3f(Source_, AL_POSITION, 0, 0, 0);
        openal::alSource3f(Source_, AL_VELOCITY, 0, 0, 0);
        openal::alSourcei(Source_, AL_LOOPING, 0);
        openal::alGenBuffers(1, &Buffer_);
    }

    void PlayingData::setBuffer(const char* data, size_t size, qint64 freq, qint64 fmt)
    {
        openal::alBufferData(Buffer_, fmt, data, size, freq);
        openal::alSourcei(Source_, AL_BUFFER, Buffer_);
    }

    std::chrono::milliseconds PlayingData::play()
    {
        if (isEmpty())
            return std::chrono::milliseconds::zero();

        auto duration = calcDuration();
        GetSoundsManager()->sourcePlay(Source_);
        return duration;
    }

    void PlayingData::pause()
    {
        if (isEmpty())
            return;

        openal::alSourcePause(Source_);
    }

    void PlayingData::stop()
    {
        if (isEmpty())
            return;

        openal::alSourceStop(Source_);
        if (openal::alIsBuffer(Buffer_))
        {
            openal::alSourcei(Source_, AL_BUFFER, 0);
            openal::alDeleteBuffers(1, &Buffer_);
        }
    }

    void PlayingData::clear()
    {
        Buffer_ = 0;
        Source_ = 0;
        Id_ = -1;
    }

    void PlayingData::free()
    {
        if (!isEmpty())
        {
            stop();
            openal::alDeleteSources(1, &Source_);
            clear();
        }
    }

    void PlayingData::clearData()
    {
        openal::alSourceStop(Source_);
#ifndef USE_SYSTEM_OPENAL
        openal::ALuint buffer = 0;
        openal::alSourceUnqueueBuffers(Source_, 1, &buffer);
#endif
    }

    bool PlayingData::isEmpty() const
    {
        return Id_ == -1;
    }

    std::chrono::milliseconds PlayingData::calcDuration()
    {
        openal::ALint sizeInBytes;
        openal::ALint channels;
        openal::ALint bits;
        openal::ALint frequency;

        openal::alGetBufferi(Buffer_, AL_SIZE, &sizeInBytes);
        openal::alGetBufferi(Buffer_, AL_CHANNELS, &channels);
        openal::alGetBufferi(Buffer_, AL_BITS, &bits);
        openal::alGetBufferi(Buffer_, AL_FREQUENCY, &frequency);
        if (channels > 0 && bits > 0 && frequency > 0)
        {
            const auto lengthInSamples = sizeInBytes * 8 / (channels * bits);
            return std::chrono::milliseconds(size_t(((float)lengthInSamples / (float)frequency) * 1000));
        }
        return {};
    }

    openal::ALenum PlayingData::state() const
    {
        openal::ALenum state = AL_NONE;
        if (!isEmpty())
        {
            openal::alGetSourcei(Source_, AL_SOURCE_STATE, &state);
        }
        return state;
    }

    size_t PlayingData::totalLengthInSamples() const
    {
        openal::ALint sizeInBytes;
        openal::ALint channels;
        openal::ALint bits;

        openal::alGetBufferi(Buffer_, AL_SIZE, &sizeInBytes);
        openal::alGetBufferi(Buffer_, AL_CHANNELS, &channels);
        openal::alGetBufferi(Buffer_, AL_BITS, &bits);
        if (channels > 0 && bits > 0)
            return sizeInBytes * 8 / (channels * bits);
        return 0;
    }

    size_t PlayingData::currentSampleOffset() const
    {
        openal::ALint sampleOffset;
        openal::alGetSourcei(Source_, AL_SAMPLE_OFFSET, &sampleOffset);
        return size_t(sampleOffset);
    }

    void PlayingData::setCurrentSampleOffset(size_t _offset)
    {
        openal::alSourcei(Source_, AL_SAMPLE_OFFSET, openal::ALint(_offset));
    }

    SoundsManager::SoundsManager()
        : QObject(nullptr)
        , CallInProgress_(false)
        , CanPlayIncoming_(true)
        , AlId(-1)
        , AlAudioDevice_(nullptr)
        , AlAudioContext_(nullptr)
        , AlInited_(false)
        , Timer_(new QTimer(this))
        , PttTimer_(new QTimer(this))
        , DeviceTimer_(new QTimer(this))
    {
        sounds_.resize(static_cast<size_t>(Sound::Size));

        Timer_->setInterval(IncomingMessageInterval);
        Timer_->setSingleShot(true);
        connect(Timer_, &QTimer::timeout, this, &SoundsManager::timedOut);

        PttTimer_->setInterval(PttCheckInterval);
        PttTimer_->setSingleShot(true);
        connect(PttTimer_, &QTimer::timeout, this, &SoundsManager::checkPttState);

        DeviceTimer_->setInterval(DeviceCheckInterval);
        DeviceTimer_->setSingleShot(true);
        connect(DeviceTimer_, &QTimer::timeout, this, &SoundsManager::deviceTimeOut);

        connect(this, &SoundsManager::needUpdateDeviceTimer, this, &SoundsManager::updateDeviceTimer);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::historyControlReady, this, &SoundsManager::contactChanged);

        // since deviceListChangedInternal is in main thread now, postpone reinit openal
        connect(this, &SoundsManager::deviceListChangedInternal, this, &SoundsManager::onDeviceListChanged, Qt::QueuedConnection);

        deviceMonitoring_ = device::DeviceMonitoringModule::CreateDeviceMonitoring();
        if (deviceMonitoring_)
        {
            deviceMonitoring_->RegisterCaptureDeviceInfoObserver(*this);
            deviceMonitoring_->Start();
        }
    }

    SoundsManager::~SoundsManager()
    {
        if (deviceMonitoring_)
        {
            deviceMonitoring_->DeregisterCaptureDeviceInfoObserver();
            deviceMonitoring_->Stop();
        }
        CurPlay_.free();
        PrevPlay_.free();
        for (auto& s : sounds_)
            s.free();

        {
            std::scoped_lock lock(mutex_);
            if (AlInited_)
                shutdownOpenAl();
        }
    }

    std::chrono::milliseconds SoundsManager::playSound(Sound _type)
    {
        if (!canPlaySound(_type))
            return std::chrono::milliseconds::zero();

        checkAudioDevice();

        switch (_type)
        {
        case Ui::SoundsManager::Sound::IncomingMail:
        case Ui::SoundsManager::Sound::IncomingMessage:
            CanPlayIncoming_ = false;
            break;
        case Ui::SoundsManager::Sound::OutgoingMessage:
            break;
        case Ui::SoundsManager::Sound::StartPtt:
            break;
        case Ui::SoundsManager::Sound::PttLimit:
            break;
        case Ui::SoundsManager::Sound::RemovePtt:
            break;
        case Ui::SoundsManager::Sound::Size:
        default:
            Q_UNREACHABLE();
            return std::chrono::milliseconds::zero();
        }

        auto duration = sounds_[static_cast<size_t>(_type)].play();
        Timer_->start();
        return duration;
    }

    void SoundsManager::timedOut()
    {
        CanPlayIncoming_ = true;
        for (auto& s : sounds_)
            s.clearData();
    }

    void SoundsManager::updateDeviceTimer()
    {
        if (!DeviceTimer_->isActive() || DeviceTimer_->remainingTime() < DeviceTimer_->interval() / 2)
            DeviceTimer_->start();
    }

    void SoundsManager::onDeviceListChanged()
    {
        {
            std::scoped_lock lock(mutex_);
            if (AlInited_)
                shutdownOpenAl();
        }
        Q_EMIT deviceListChanged(QPrivateSignal());
    }

    std::optional<int> SoundsManager::checkPlayPtt(int id, int &duration, const std::optional<size_t>& _sampleOffsett)
    {
        checkAudioDevice();

        Q_EMIT Utils::InterConnector::instance().stopPttRecord();
        if (!CurPlay_.isEmpty())
        {
            if (!PrevPlay_.isEmpty())
            {
                if (PrevPlay_.state() == AL_PAUSED && PrevPlay_.Id_ == id)
                {
                    if (CurPlay_.state() == AL_PLAYING)
                    {
                        CurPlay_.pause();
                        Q_EMIT pttPaused(CurPlay_.Id_, CurPlay_.currentSampleOffset(), QPrivateSignal());
                    }

                    using std::swap;
                    swap(PrevPlay_, CurPlay_);

                    if (_sampleOffsett)
                        setSampleOffset(CurPlay_.Id_, *_sampleOffsett);
                    duration = CurPlay_.play().count();
                    PttTimer_->start();
                    return CurPlay_.Id_;
                }
            }
            if (CurPlay_.state() == AL_PLAYING)
            {
                if (!PrevPlay_.isEmpty())
                {
                    PrevPlay_.pause();
                    Q_EMIT pttPaused(PrevPlay_.Id_, PrevPlay_.currentSampleOffset(), QPrivateSignal());
                }

                CurPlay_.pause();
                Q_EMIT pttPaused(CurPlay_.Id_, CurPlay_.currentSampleOffset(), QPrivateSignal());
                using std::swap;
                swap(PrevPlay_, CurPlay_);
            }
            else if (CurPlay_.state() == AL_PAUSED)
            {
                if (CurPlay_.Id_ == id)
                {
                    if (_sampleOffsett)
                        setSampleOffset(CurPlay_.Id_, *_sampleOffsett);
                    duration = CurPlay_.play().count();
                    PttTimer_->start();
                    return CurPlay_.Id_;
                }

                if (!PrevPlay_.isEmpty())
                {
                    PrevPlay_.pause();
                    Q_EMIT pttPaused(PrevPlay_.Id_, PrevPlay_.currentSampleOffset(), QPrivateSignal());
                }

                using std::swap;
                swap(PrevPlay_, CurPlay_);
            }
            else if (CurPlay_.state() == AL_STOPPED)
            {
                Q_EMIT pttFinished(CurPlay_.Id_, false, QPrivateSignal());
            }
            CurPlay_.free();
        }

        CurPlay_.init();
        return std::nullopt;
    }

    std::optional<SoundsManager::PttBuffer> SoundsManager::getBuffer(const QString& file)
    {
        MpegLoader l(file, true);
        if (!l.open())
            return std::nullopt;

        QByteArray result;
        qint64 samplesAdded = 0, frequency = l.frequency(), format = l.format();
        while (1)
        {
            int res = l.readMore(result, samplesAdded);
            if (res < 0)
                break;
        }
        return PttBuffer{ std::move(result), frequency, format };
    }

    int SoundsManager::playPttImpl(const char* data, size_t size, qint64 freq, qint64 fmt, int& duration, size_t sampleOffset)
    {
        CurPlay_.setBuffer(data, size, freq, fmt);

        CurPlay_.Id_ = ++AlId;
        setSampleOffset(CurPlay_.Id_, sampleOffset);
        duration = CurPlay_.play().count();
        PttTimer_->start();
        return CurPlay_.Id_;
    }

    void SoundsManager::initPlayingData(PlayingData& _data, const QString& _file)
    {
        im_assert(AlInited_);

        if (!_data.isEmpty())
            return;

        _data.init();

        const auto buffer = getBuffer(_file);
        if (!buffer)
            return;

        _data.setBuffer((*buffer).data, (*buffer).frequency, (*buffer).format);

        if (int err = openal::alGetError(); err == AL_NO_ERROR)
            _data.Id_ = ++AlId;
    }

    void SoundsManager::initSounds()
    {
        for (size_t i = 0; i < sounds_.size(); ++i)
            initPlayingData(sounds_[i], getFilePath(static_cast<Sound>(i)));
    }

    void SoundsManager::deInitSounds()
    {
        for (auto& s : sounds_)
        {
            s.stop();
            s.free();
            s.clear();
        }
    }

    bool SoundsManager::canPlaySound(Sound _type)
    {
        if (CallInProgress_ || CurPlay_.state() == AL_PLAYING)
            return false;

#ifdef __APPLE__
        if (MacSupport::isDoNotDisturbOn())
            return false;
#endif
        switch (_type)
        {
        case Ui::SoundsManager::Sound::IncomingMail:
        case Ui::SoundsManager::Sound::IncomingMessage:
            return CanPlayIncoming_ && get_gui_settings()->get_value<bool>(settings_sounds_enabled, true);
        case Ui::SoundsManager::Sound::OutgoingMessage:
            return get_gui_settings()->get_value(settings_outgoing_message_sound_enabled, false);
        case Ui::SoundsManager::Sound::StartPtt:
        case Ui::SoundsManager::Sound::PttLimit:
        case Ui::SoundsManager::Sound::RemovePtt:
            return get_gui_settings()->get_value<bool>(settings_sounds_enabled, true);
        case Ui::SoundsManager::Sound::Size:
        default:
            Q_UNREACHABLE();
            return true;
        }
    }

    void SoundsManager::checkPttState()
    {
        if (CurPlay_.Source_ != 0)
        {
            openal::ALenum state;
            openal::alGetSourcei(CurPlay_.Source_, AL_SOURCE_STATE, &state);
            if (state == AL_PLAYING || state == AL_INITIAL)
            {
                PttTimer_->start();
            }
            else if (state == AL_PAUSED)
            {
                Q_EMIT pttPaused(CurPlay_.Id_, CurPlay_.currentSampleOffset(), QPrivateSignal());
            }
            else if (state == AL_STOPPED)
            {
                Q_EMIT pttFinished(CurPlay_.Id_, true, QPrivateSignal());
                if (!PrevPlay_.isEmpty())
                {
                    CurPlay_.stop();
                    CurPlay_.clear();
                    CurPlay_ = PrevPlay_;
                    PrevPlay_.clear();
                }
            }
        }
    }

    void SoundsManager::contactChanged(const QString&)
    {
        if (CurPlay_.state() == AL_PLAYING)
        {
            CurPlay_.stop();
            Q_EMIT pttPaused(CurPlay_.Id_, CurPlay_.currentSampleOffset(), QPrivateSignal());
        }
    }

    void SoundsManager::deviceTimeOut()
    {
        if (CurPlay_.state() == AL_PLAYING || Utils::InterConnector::instance().isRecordingPtt())
            return updateDeviceTimer();

#ifndef USE_SYSTEM_OPENAL
        openal::alcDevicePauseSOFT(AlAudioDevice_);
#else
        {
            std::scoped_lock lock(mutex_);
            if (AlInited_)
                shutdownOpenAl();
        }
#endif
    }

    int SoundsManager::playPtt(const QString& file, int id, int& duration, double _progress)
    {
        const auto sampleOffset = _progress * CurPlay_.totalLengthInSamples();
        if (auto res = checkPlayPtt(id, duration, sampleOffset); res)
            return *res;

        if (auto res = getBuffer(file); res)
        {
            const auto& data = (*res).data;
            return playPttImpl(data.constData(), data.size(), (*res).frequency, (*res).format, duration, sampleOffset);
        }
        return -1;
    }

    int SoundsManager::playPtt(const char* data, size_t size, qint64 freq, qint64 fmt, int id, int& duration, size_t sampleOffset)
    {
        if (auto res = checkPlayPtt(id, duration, sampleOffset); res)
            return *res;

        return playPttImpl(data, size, freq, fmt, duration, sampleOffset);
    }

    void SoundsManager::stopPtt(int id)
    {
        if (CurPlay_.Id_ == id)
            CurPlay_.stop();
    }

    void SoundsManager::pausePtt(int id)
    {
        if (CurPlay_.Id_ == id)
            CurPlay_.pause();
    }

    bool SoundsManager::isPaused(int id) const
    {
        if (CurPlay_.Id_ == id)
            return CurPlay_.state() == AL_PAUSED;
        return false;
    }

    size_t SoundsManager::sampleOffset(int id) const
    {
        if (CurPlay_.Id_ == id)
            return CurPlay_.currentSampleOffset();
        return 0;
    }

    bool SoundsManager::setSampleOffset(int id, size_t _offset)
    {
        if (CurPlay_.Id_ == id)
        {
            CurPlay_.setCurrentSampleOffset(_offset);
            return true;
        }
        return false;
    }

    void SoundsManager::setProgressOffset(int id, double percent)
    {
        if (CurPlay_.Id_ == id)
            CurPlay_.setCurrentSampleOffset(size_t(CurPlay_.totalLengthInSamples() * percent));
    }

    void SoundsManager::delayDeviceTimer()
    {
        Q_EMIT needUpdateDeviceTimer(QPrivateSignal());
    }

    void SoundsManager::sourcePlay(unsigned source)
    {
#ifndef USE_SYSTEM_OPENAL
        openal::alcDeviceResumeSOFT(AlAudioDevice_);
#else
        checkAudioDevice();
#endif
        openal::alSourcePlay(source);

        delayDeviceTimer();
    }

    void SoundsManager::callInProgress(bool value)
    {
        CallInProgress_ = value;
        if (value)
            shutdownOpenAl();
    }

    void SoundsManager::checkAudioDevice()
    {
        std::scoped_lock lock(mutex_);
        if (!AlInited_)
        {
            initOpenAl();
        }
    }

    void SoundsManager::DeviceMonitoringListChanged()
    {
        Q_EMIT deviceListChangedInternal(QPrivateSignal());
    }

    void SoundsManager::initOpenAl()
    {
        if (AlInited_)
            return;

        std::string deviceName = selectedDeviceName();
        if (!deviceName.empty())
        {
            AlAudioDevice_ = openal::alcOpenDevice(deviceName.c_str());
        }
        if (!AlAudioDevice_)
        {
            AlAudioDevice_ = openal::alcOpenDevice(nullptr);
            if (!AlAudioDevice_)
            {
                qCDebug(soundManager) << "could not create default playback device";
                return;
            }
        }

        AlAudioContext_ = openal::alcCreateContext(AlAudioDevice_, nullptr);
        openal::alcMakeContextCurrent(AlAudioContext_);
        if (auto errCode = openal::alcGetError(AlAudioDevice_); errCode != ALC_NO_ERROR)
        {
            qCDebug(soundManager) << "audio context error:" << errCode << openal::alcGetString(AlAudioDevice_, errCode);
            closeOpenAlDevice();
            return;
        }

        openal::ALfloat v[] = { 0.f, 0.f, -1.f, 0.f, 1.f, 0.f };
        openal::alListener3f(AL_POSITION, 0.f, 0.f, 0.f);
        openal::alListener3f(AL_VELOCITY, 0.f, 0.f, 0.f);
        openal::alListenerfv(AL_ORIENTATION, v);

        openal::alDistanceModel(AL_NONE);

        AlInited_ = true;
        QMetaObject::invokeMethod(this, &SoundsManager::updateDeviceTimer);

        qCDebug(soundManager) << "inited OpenAl device:" << AlAudioDevice_ << AlAudioContext_;

        initSounds();

    }

    void SoundsManager::closeOpenAlDevice()
    {
        qCDebug(soundManager) << "closed OpenAl device:" << AlAudioDevice_ << AlAudioContext_;

        if (AlAudioContext_)
        {
            openal::alcMakeContextCurrent(nullptr);
            openal::alcDestroyContext(AlAudioContext_);
            AlAudioContext_ = nullptr;
        }

        if (AlAudioDevice_)
        {
            openal::alcCloseDevice(AlAudioDevice_);
            AlAudioDevice_ = nullptr;
        }
    }

    void SoundsManager::shutdownOpenAl()
    {
        Q_EMIT pttPaused(CurPlay_.Id_, CurPlay_.currentSampleOffset(), QPrivateSignal());
        Q_EMIT pttFinished(CurPlay_.Id_, false, QPrivateSignal());
        Q_EMIT pttPaused(PrevPlay_.Id_, CurPlay_.currentSampleOffset(), QPrivateSignal());
        Q_EMIT pttFinished(PrevPlay_.Id_, false, QPrivateSignal());

        PrevPlay_.stop();
        PrevPlay_.free();
        PrevPlay_.clear();

        CurPlay_.stop();
        CurPlay_.free();
        CurPlay_.clear();

        deInitSounds();

        closeOpenAlDevice();

        AlInited_ = false;
    }

    std::unique_ptr<SoundsManager> g_sounds_manager;

    SoundsManager* GetSoundsManager()
    {
        if (!g_sounds_manager)
            g_sounds_manager = std::make_unique<SoundsManager>();

        return g_sounds_manager.get();
    }

    void ResetSoundsManager()
    {
        if (g_sounds_manager)
            g_sounds_manager.reset();
    }


    std::string SoundsManager::selectedDeviceName()
    {
#ifndef STRIP_VOIP
        auto d = Ui::GetDispatcher()->getVoipController().activeDevice(voip_proxy::EvoipDevTypes::kvoipDevTypeAudioPlayback);
        if (!d)
            return {};
        std::string_view deviceName((*d).uid);

        //macos OpenAl Enumeration Extension  bug: return only one default device
        const openal::ALCchar* device = openal::alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
        const openal::ALCchar* next = device + 1;
        size_t len = 0;
        while (device && *device != '\0' && next && *next != '\0') {
            len = strlen(device);
            auto alDeviceName = std::string_view(device, len);
            if (alDeviceName == deviceName)
                return std::string(alDeviceName);
            device += (len + 1);
            next += (len + 2);
        }
#endif
        return {};
    }
}
