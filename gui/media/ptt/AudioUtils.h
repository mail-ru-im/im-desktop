#pragma once

Q_DECLARE_LOGGING_CATEGORY(pttLog)

namespace ptt
{
    struct Buffer
    {
        QByteArray d;
        size_t dataOffset = 0;
        int sampleRate = 0;
        int format = 0;
        int bitsPerSample = 0;
        int channelsCount = 0;

        size_t size() const noexcept
        {
            if (auto s = std::make_signed_t<size_t>(d.size()) - std::make_signed_t<size_t>(dataOffset); s < 0)
                return 0;
            else
                return size_t(s);
        }

        const char* data() const noexcept { return d.constData() + dataOffset; }

        bool isEmpty() const noexcept { return size() == 0; }
    };

    struct StatInfo
    {
        bool playBeforeSent = false;
        std::chrono::seconds duration = std::chrono::seconds::zero();
        std::string mode;
        std::string chatType;
    };


    std::chrono::milliseconds calculateDuration(size_t _dataSize, int _sampleRate, int _channelsCount, int _bitesPerSample) noexcept;

    size_t sizeForDuration(std::chrono::seconds seconds, int _sampleRate, int _channelsCount, int _bitesPerSample) noexcept;

    std::vector<std::byte> getWavHeaderData(size_t _dataSize, int _sampleRate, int _channelsCount, int _bitesPerSample) noexcept;
    size_t getWavHeaderSize() noexcept;

    constexpr inline std::chrono::seconds maxDuration() noexcept
    {
        return std::chrono::minutes(15);
    }

    constexpr inline std::chrono::seconds minDuration() noexcept
    {
        return std::chrono::seconds(1);
    }

    constexpr inline size_t sampleBlockSizeForHist() noexcept
    {
        return 1024;
    }

    constexpr int nomalizedMagnitudeDb() noexcept { return 70; }

    double getVolumeDefaultDevice();
}

Q_DECLARE_METATYPE(ptt::Buffer);
Q_DECLARE_METATYPE(ptt::StatInfo);