#include "stdafx.h"

#include "AudioPlayer.h"
#include "AudioUtils.h"

#include "main_window/sounds/SoundsManager.h"

namespace ptt
{
    class AudioPlayerImpl
    {
    public:
        AudioPlayerImpl(std::function<void(AudioPlayer::State)>&& _f) : setStateCallback_(std::move(_f))
        {
        }

        ~AudioPlayerImpl()
        {
            setStateCallback_ = {};
            stop();
        }

        void setBuffer(const Buffer& _buffer)
        {
            buffer_ = _buffer;
            if (pendingOptions_.waintingBufferForPlay)
                playImpl();
        }

        const auto& getBuffer() const noexcept { return buffer_; }

        bool hasBuffer() const noexcept
        {
            return buffer_.has_value();
        }

        void clearBuffer() noexcept
        {
            buffer_ = std::nullopt;
        }

        void play()
        {
            if (hasBuffer())
                playImpl();
            else
                pendingOptions_.waintingBufferForPlay = true;
        }

        void pause()
        {
            resetPendingOptions();
            Ui::GetSoundsManager()->pausePtt(id_);
            if (setStateCallback_)
                setStateCallback_(AudioPlayer::State::Paused);
        }

        void stop()
        {
            resetPendingOptions();
            Ui::GetSoundsManager()->stopPtt(id_);
            if (setStateCallback_)
                setStateCallback_(AudioPlayer::State::Stopped);
            clearId();
        }

        void setSampleOffset(size_t _offset)
        {
            if (id() != -1)
            {
                if (Ui::GetSoundsManager()->setSampleOffset(id(), _offset) && !Ui::GetSoundsManager()->isPaused(id()))
                    pendingOptions_.sampleOffset = 0;
                else
                    pendingOptions_.sampleOffset = _offset;
            }
            else
            {
                pendingOptions_.sampleOffset = _offset;
            }
        }

        size_t getCurrentSampleOffset() const
        {
            if (id() != -1)
                return Ui::GetSoundsManager()->sampleOffset(id());
            return pendingOptions_.sampleOffset;
        }

        int id() const noexcept { return id_; }

        void clearId() noexcept { id_ = -1; }

        bool hasPendingOptions() const noexcept { return !pendingOptions_.isEmpty(); }

        std::chrono::milliseconds duration() const noexcept { return hasBuffer() ? calculateDuration(buffer_->size(), buffer_->sampleRate, buffer_->channelsCount, buffer_->bitsPerSample) : std::chrono::milliseconds::zero(); }

    private:
        void playImpl()
        {
            auto copy = pendingOptions_;
            resetPendingOptions();
            int duration = 0;
            if (!buffer_->isEmpty())
            {
                id_ = Ui::GetSoundsManager()->playPtt(buffer_->data(), buffer_->size(), buffer_->sampleRate, buffer_->format, id_, duration, std::max(copy.sampleOffset, size_t(0)));
                if (setStateCallback_)
                    setStateCallback_(AudioPlayer::State::Playing);
            }
        }

        void resetPendingOptions() noexcept { pendingOptions_ = {}; }

    private:
        std::function<void(AudioPlayer::State)> setStateCallback_;
        std::optional<ptt::Buffer> buffer_;

        struct {
            bool waintingBufferForPlay = false;
            size_t sampleOffset = 0;
            bool isEmpty() const noexcept { return waintingBufferForPlay || sampleOffset > 0; }
        } pendingOptions_;

        int id_ = -1;
    };

    constexpr static std::chrono::milliseconds positionTimeout() noexcept { return std::chrono::milliseconds(100); }

    AudioPlayer::AudioPlayer(QObject* _parent)
        : QObject(_parent)
        , positionTimer_(new QTimer(this))
    {
        impl_ = std::make_unique<AudioPlayerImpl>([this](State s) { emit stateChanged(s, QPrivateSignal()); });

        QObject::connect(Ui::GetSoundsManager(), &Ui::SoundsManager::pttFinished, this, [this](int _id, bool _ended)
        {
            if (_id == impl_->id())
            {
                if (_ended)
                    stop();
                else
                    impl_->clearId();
            }
        });

        QObject::connect(Ui::GetSoundsManager(), &Ui::SoundsManager::pttPaused, this, [this](int _id, int sampleOffset)
        {
            if (_id == impl_->id())
            {
                pause();
                setSampleOffset(sampleOffset);
            }
        });

        positionTimer_->setInterval(positionTimeout().count());
        QObject::connect(positionTimer_, &QTimer::timeout, this, &AudioPlayer::onPositionTimer);
    }

    AudioPlayer::~AudioPlayer() = default;

    void AudioPlayer::setBuffer(const ptt::Buffer& _buffer)
    {
        impl_->setBuffer(_buffer);
    }

    void AudioPlayer::clear()
    {
        impl_->clearBuffer();
        impl_->clearId();
    }

    std::chrono::milliseconds AudioPlayer::duration() const
    {
        return impl_->duration();
    }

    bool AudioPlayer::hasBuffer() const noexcept
    {
        return impl_->hasBuffer();
    }

    void AudioPlayer::setSampleOffset(size_t _offset)
    {
        impl_->setSampleOffset(_offset);
        if (!positionTimer_->isActive())
            onPositionTimer();
    }

    size_t AudioPlayer::getCurrentSampleOffset() const
    {
        return impl_->getCurrentSampleOffset();
    }

    std::optional<std::chrono::milliseconds> AudioPlayer::timePosition(size_t _sampleOffset) const
    {
        if (const auto& buffer = impl_->getBuffer(); buffer)
            return calculateDuration(_sampleOffset * buffer->bitsPerSample / 8, buffer->sampleRate, buffer->channelsCount, buffer->bitsPerSample);
        return {};
    }

    void AudioPlayer::play()
    {
        impl_->play();
        positionTimer_->start();
    }

    void AudioPlayer::pause()
    {
        positionTimer_->stop();
        impl_->pause();
    }

    void AudioPlayer::stop()
    {
        positionTimer_->stop();
        impl_->stop();

        emit timePositionChanged(impl_->duration(), QPrivateSignal());
        emit samplePositionChanged(-1, QPrivateSignal());
    }

    void AudioPlayer::onPositionTimer()
    {
        const auto pos = impl_->getCurrentSampleOffset();
        if (const auto time = timePosition(pos); time)
            emit timePositionChanged(*time, QPrivateSignal());
        emit samplePositionChanged(pos, QPrivateSignal());
    }
}