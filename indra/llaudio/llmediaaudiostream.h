/**
 * @file llmediaaudiostream.h
 * @brief FMOD 2D playback bridge for CEF/MOAP audio PCM.
 */

#ifndef LL_LLMEDIAAUDIOSTREAM_H
#define LL_LLMEDIAAUDIOSTREAM_H

#include "stdtypes.h"

#include "fmodstudio/fmod_common.h"

#include <atomic>

class LLAudioEngine;
struct LLPluginAudioRingHeader;

namespace FMOD
{
    class System;
    class Sound;
    class Channel;
}

class LLMediaAudioStream
{
public:
    LLMediaAudioStream();
    ~LLMediaAudioStream();

    void setRing(LLPluginAudioRingHeader* ring);
    void setVolume(F32 volume);
    void update(LLAudioEngine* engine);
    void stop();

private:
    bool start(LLAudioEngine* engine);
    U32 availableFrames() const;
    static FMOD_RESULT pcmReadCallback(FMOD_SOUND* sound, void* data, unsigned int datalen);
    FMOD_RESULT readPCM(void* data, unsigned int datalen);

private:
    LLPluginAudioRingHeader* mRing;
    FMOD::System* mSystem;
    FMOD::Sound* mSound;
    FMOD::Channel* mChannel;
    F32 mVolume;
    U32 mSampleRate;
    U32 mChannels;
    U32 mFormatSerial;
    bool mPlaybackStarted;
    std::atomic<bool> mNeedsPrebuffer;
    std::atomic<U64> mCallbackCount;
    std::atomic<U64> mFramesRequested;
    std::atomic<U64> mFramesRead;
    std::atomic<U64> mFramesSilenced;
    std::atomic<U32> mMinAvailableFrames;
    std::atomic<U32> mMaxAvailableFrames;
    U64 mLastLoggedCallbackCount;
    U64 mLastLoggedFramesRequested;
    U64 mLastLoggedFramesRead;
    U64 mLastLoggedFramesSilenced;
    U64 mLastLoggedFramesDropped;
    F64 mNextStatsLogTime;
};

#endif // LL_LLMEDIAAUDIOSTREAM_H
