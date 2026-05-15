/**
 * @file llpluginaudio.h
 * @brief Shared-memory audio ring used by media plugins.
 */

#ifndef LL_LLPLUGINAUDIO_H
#define LL_LLPLUGINAUDIO_H

#include <atomic>
#include <cstdint>

static constexpr std::uint32_t LL_PLUGIN_AUDIO_RING_MAGIC = 0x41594141; // "AYAA"
static constexpr std::uint32_t LL_PLUGIN_AUDIO_RING_VERSION = 2;
static constexpr std::uint32_t LL_PLUGIN_AUDIO_RING_MAX_CHANNELS = 8;

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

#endif // LL_LLPLUGINAUDIO_H
