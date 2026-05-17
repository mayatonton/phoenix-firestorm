/**
 * @file llpluginaudio.h
 * @brief Shared-memory audio ring used by media plugins.
 */

#ifndef LL_LLPLUGINAUDIO_H
#define LL_LLPLUGINAUDIO_H

#include <atomic>
#include <cstddef>
#include <cstdint>

static constexpr std::uint32_t LL_PLUGIN_AUDIO_RING_MAGIC = 0x41594141; // "AYAA"
static constexpr std::uint32_t LL_PLUGIN_AUDIO_RING_VERSION = 2;
static constexpr std::uint32_t LL_PLUGIN_AUDIO_RING_MAX_CHANNELS = 8;

static constexpr std::uint32_t ll_plugin_audio_ring_total_frames(std::uint32_t capacity_frames)
{
    return capacity_frames + 1;
}

static constexpr std::size_t ll_plugin_audio_ring_sample_count(std::uint32_t capacity_frames)
{
    return static_cast<std::size_t>(ll_plugin_audio_ring_total_frames(capacity_frames)) *
           static_cast<std::size_t>(LL_PLUGIN_AUDIO_RING_MAX_CHANNELS);
}

static constexpr std::size_t ll_plugin_audio_ring_sample_bytes(std::uint32_t capacity_frames)
{
    return ll_plugin_audio_ring_sample_count(capacity_frames) * sizeof(float);
}

static constexpr bool ll_plugin_audio_ring_supported_3d_channel_count(std::uint32_t channels)
{
    return channels == 1 || channels == 2 || channels == 6 || channels == 8;
}

enum class LLPluginAudioSixChannel : std::uint32_t
{
    FrontLeft = 0,
    FrontRight = 1,
    Center = 2,
    Lfe = 3,
    SurroundLeft = 4,
    SurroundRight = 5,
};

enum class LLPluginAudioEightChannel : std::uint32_t
{
    FrontLeft = 0,
    FrontRight = 1,
    Center = 2,
    Lfe = 3,
    SurroundLeft = 4,
    SurroundRight = 5,
    BackLeft = 6,
    BackRight = 7,
};

struct LLPluginAudioRingHeader
{
    std::uint32_t mMagic;
    std::uint32_t mVersion;
    std::uint32_t mHeaderSize;
    std::uint32_t mCapacityFrames;
    std::atomic<std::uint32_t> mSampleRate;
    std::atomic<std::uint32_t> mChannels;
    std::atomic<std::uint32_t> mBytesPerSample;
    std::atomic<std::uint32_t> mFormatSerial;
    std::atomic<std::uint32_t> mWriteFrame;
    std::atomic<std::uint32_t> mReadFrame;
    std::atomic<std::uint64_t> mTotalFramesWritten;
    std::atomic<std::uint64_t> mTotalFramesDropped;
};

static constexpr std::size_t ll_plugin_audio_ring_shared_memory_size(std::uint32_t capacity_frames)
{
    return sizeof(LLPluginAudioRingHeader) + ll_plugin_audio_ring_sample_bytes(capacity_frames);
}

#endif // LL_LLPLUGINAUDIO_H
