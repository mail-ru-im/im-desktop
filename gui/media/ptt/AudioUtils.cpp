#include "stdafx.h"
#include "AudioUtils.h"

Q_LOGGING_CATEGORY(pttLog, "ptt")

namespace
{
#pragma pack(push, 1)
// WAVE file header format
struct wav_header_t
{
    char            chunk_id[4];        // RIFF string
    unsigned int    chunk_size;         // overall size of file in bytes (36 + data_size)
    char            sub_chunk1_id[8];   // WAVEfmt string with trailing null char
    unsigned int    sub_chunk1_size;    // 16 for PCM.  This is the size of the rest of the Subchunk which follows this number.
    unsigned short  audio_format;       // format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
    unsigned short  num_channels;       // Mono = 1, Stereo = 2
    unsigned int    sample_rate;        // 8000, 16000, 44100, etc. (blocks per second)
    unsigned int    byte_rate;          // SampleRate * NumChannels * BitsPerSample/8
    unsigned short  block_align;        // NumChannels * BitsPerSample/8
    unsigned short  bits_per_sample;    // bits per sample, 8- 8bits, 16- 16 bits etc
    char            sub_chunk2_id[4];   // Contains the letters "data"
    unsigned int    sub_chunk2_size;    // NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
};
#pragma pack(pop)

wav_header_t get_wav_header(int size, int sampleRate, int channelsCount, int bitesPerSample)
{
    wav_header_t wh;
    // RIFF chunk
#ifdef _WIN32
    memcpy_s(wh.chunk_id, std::size(wh.chunk_id), "RIFF", std::strlen("RIFF"));
#else
    memcpy(wh.chunk_id, "RIFF", std::strlen("RIFF"));
#endif
    wh.chunk_size = 36 + size;

    // fmt sub-chunk (to be optimized)
#ifdef _WIN32
    memcpy_s(wh.sub_chunk1_id, std::size(wh.sub_chunk1_id), "WAVEfmt ", std::strlen("WAVEfmt "));
#else
    memcpy(wh.sub_chunk1_id, "WAVEfmt ", std::strlen("WAVEfmt "));
#endif
    wh.sub_chunk1_size = 16;
    wh.audio_format = 1;
    wh.num_channels = channelsCount;
    wh.sample_rate = sampleRate;
    wh.bits_per_sample = bitesPerSample;
    wh.block_align = wh.num_channels * wh.bits_per_sample / 8;
    wh.byte_rate = wh.sample_rate * wh.num_channels * wh.bits_per_sample / 8;

    // data sub-chunk
#ifdef _WIN32
    memcpy_s(wh.sub_chunk2_id, std::size(wh.sub_chunk2_id), "data", std::strlen("data"));
#else
    memcpy(wh.sub_chunk2_id, "data", std::strlen("data"));
#endif
    wh.sub_chunk2_size = size;
    return wh;
}
}

namespace ptt
{
    std::chrono::milliseconds calculateDuration(size_t _dataSize, int _sampleRate, int _channelsCount, int _bitesPerSample) noexcept
    {
        if (_dataSize == 0 || _sampleRate == 0 || _channelsCount == 0 || _bitesPerSample == 0)
            return std::chrono::milliseconds::zero();
        const auto duration = double(_dataSize) / (_sampleRate * _channelsCount * _bitesPerSample / 8) * 1000;
        return std::chrono::milliseconds(std::lround(duration));
    }

    size_t sizeForDuration(std::chrono::seconds seconds, int _sampleRate, int _channelsCount, int _bitesPerSample) noexcept
    {
        return seconds.count() * (_sampleRate * _channelsCount * _bitesPerSample / 8);
    }

    std::vector<std::byte> getWavHeaderData(size_t _dataSize, int _sampleRate, int _channelsCount, int _bitesPerSample) noexcept
    {
        wav_header_t h = get_wav_header(_dataSize, _sampleRate, _channelsCount, _bitesPerSample);

        return std::vector<std::byte>(reinterpret_cast<std::byte*>(&h), reinterpret_cast<std::byte*>(&h) + sizeof(wav_header_t));
    }

    size_t getWavHeaderSize() noexcept
    {
        return sizeof(wav_header_t);
    }
}
