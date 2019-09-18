#pragma once

#include "AudioUtils.h"

namespace ptt
{
    class AudioPlayerImpl;

    class AudioPlayer : public QObject
    {
        Q_OBJECT

    public:
        enum class State
        {
            Stopped,
            Paused,
            Playing
        };

    Q_SIGNALS:
        void samplePositionChanged(int, QPrivateSignal);
        void timePositionChanged(std::chrono::milliseconds, QPrivateSignal);
        void stateChanged(State, QPrivateSignal);

    public:
        explicit AudioPlayer(QObject* _parent);
        ~AudioPlayer();

        void setBuffer(const ptt::Buffer& _buffer);
        bool hasBuffer() const noexcept;
        void clear();

        std::chrono::milliseconds duration() const;

        void setSampleOffset(size_t _offset);
        size_t getCurrentSampleOffset() const;

        std::optional<std::chrono::milliseconds> timePosition(size_t _sampleOffset) const;

        void play();
        void pause();
        void stop();

    private:
        void onPositionTimer();

    private:
        std::unique_ptr<AudioPlayerImpl> impl_;

        QTimer* positionTimer_ = nullptr;
    };
}