/**
 * @file llmediaaudiostream.cpp
 * @brief FMOD 2D playback bridge for CEF/MOAP audio PCM.
 */

#include "linden_common.h"

#include "llmediaaudiostream.h"

#include "llaudioengine.h"
#include "llaudioengine_fmodstudio.h"
#include "llpluginaudio.h"
#include "lltimer.h"

#include "fmodstudio/fmod.hpp"
#include "fmodstudio/fmod_errors.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace
{
    constexpr U32 MEDIA_AUDIO_PREBUFFER_FRAMES = 4096;

    bool checkFmod(FMOD_RESULT result, const char* what)
    {
        if (result == FMOD_OK)
        {
            return false;
        }
        LL_WARNS("AYAMediaAudio") << what << " error: " << FMOD_ErrorString(result) << LL_ENDL;
        return true;
    }
}

LLMediaAudioStream::LLMediaAudioStream()
:   mRing(nullptr),
    mSystem(nullptr),
    mSound(nullptr),
    mChannel(nullptr),
    mVolume(1.f),
    mSampleRate(0),
    mChannels(0),
    mFormatSerial(0),
    mPlaybackStarted(false),
    mNeedsPrebuffer(false),
    mCallbackCount(0),
    mFramesRequested(0),
    mFramesRead(0),
    mFramesSilenced(0),
    mMinAvailableFrames(std::numeric_limits<U32>::max()),
    mMaxAvailableFrames(0),
    mLastLoggedCallbackCount(0),
    mLastLoggedFramesRequested(0),
    mLastLoggedFramesRead(0),
    mLastLoggedFramesSilenced(0),
    mLastLoggedFramesDropped(0),
    mNextStatsLogTime(0.0)
{
}

LLMediaAudioStream::~LLMediaAudioStream()
{
    stop();
}

void LLMediaAudioStream::setRing(LLPluginAudioRingHeader* ring)
{
    if (mRing != ring)
    {
        stop();
        mRing = ring;
    }
}

void LLMediaAudioStream::setVolume(F32 volume)
{
    mVolume = llclamp(volume, 0.f, 1.f);
    if (mChannel)
    {
        checkFmod(mChannel->setVolume(mVolume), "FMOD::Channel::setVolume(media)");
    }
}

void LLMediaAudioStream::update(LLAudioEngine* engine)
{
    if (!mRing ||
        mRing->mMagic != LL_PLUGIN_AUDIO_RING_MAGIC ||
        mRing->mVersion != LL_PLUGIN_AUDIO_RING_VERSION ||
        mRing->mSampleRate.load(std::memory_order_acquire) == 0 ||
        mRing->mChannels.load(std::memory_order_acquire) == 0)
    {
        stop();
        return;
    }

    const U32 ring_sample_rate = mRing->mSampleRate.load(std::memory_order_acquire);
    const U32 ring_channels = llclamp((U32)mRing->mChannels.load(std::memory_order_acquire),
                                      1u,
                                      LL_PLUGIN_AUDIO_RING_MAX_CHANNELS);
    const U32 ring_format_serial = mRing->mFormatSerial.load(std::memory_order_acquire);

    if (!mChannel)
    {
        start(engine);
        return;
    }

    if (ring_sample_rate != mSampleRate ||
        ring_channels != mChannels ||
        ring_format_serial != mFormatSerial)
    {
        stop();
        start(engine);
        return;
    }

    bool playing = false;
    if (checkFmod(mChannel->isPlaying(&playing), "FMOD::Channel::isPlaying(media)") || !playing)
    {
        stop();
        return;
    }

    if (mNeedsPrebuffer.exchange(false, std::memory_order_acq_rel))
    {
        checkFmod(mChannel->setPaused(true), "FMOD::Channel::setPaused(media prebuffer)");
        mPlaybackStarted = false;
    }

    if (!mPlaybackStarted)
    {
        if (availableFrames() >= MEDIA_AUDIO_PREBUFFER_FRAMES)
        {
            checkFmod(mChannel->setPaused(false), "FMOD::Channel::setPaused(media start)");
            mPlaybackStarted = true;
        }
        return;
    }
}

void LLMediaAudioStream::stop()
{
    if (mChannel)
    {
        mChannel->stop();
        mChannel = nullptr;
    }
    if (mSound)
    {
        mSound->release();
        mSound = nullptr;
    }
    mSystem = nullptr;
    mSampleRate = 0;
    mChannels = 0;
    mFormatSerial = 0;
    mPlaybackStarted = false;
    mNeedsPrebuffer.store(false, std::memory_order_relaxed);
    mCallbackCount.store(0, std::memory_order_relaxed);
    mFramesRequested.store(0, std::memory_order_relaxed);
    mFramesRead.store(0, std::memory_order_relaxed);
    mFramesSilenced.store(0, std::memory_order_relaxed);
    mMinAvailableFrames.store(std::numeric_limits<U32>::max(), std::memory_order_relaxed);
    mMaxAvailableFrames.store(0, std::memory_order_relaxed);
    mLastLoggedCallbackCount = 0;
    mLastLoggedFramesRequested = 0;
    mLastLoggedFramesRead = 0;
    mLastLoggedFramesSilenced = 0;
    mLastLoggedFramesDropped = 0;
    mNextStatsLogTime = 0.0;
}

bool LLMediaAudioStream::start(LLAudioEngine* engine)
{
    LLAudioEngine_FMODSTUDIO* fmod_engine = dynamic_cast<LLAudioEngine_FMODSTUDIO*>(engine);
    if (!fmod_engine || !mRing)
    {
        return false;
    }

    mSystem = fmod_engine->getSystem();
    if (!mSystem)
    {
        return false;
    }

    mSampleRate = mRing->mSampleRate.load(std::memory_order_acquire);
    mChannels = llclamp((U32)mRing->mChannels.load(std::memory_order_acquire),
                        1u,
                        LL_PLUGIN_AUDIO_RING_MAX_CHANNELS);
    mFormatSerial = mRing->mFormatSerial.load(std::memory_order_acquire);

    FMOD_CREATESOUNDEXINFO ex;
    std::memset(&ex, 0, sizeof(ex));
    ex.cbsize = sizeof(ex);
    ex.defaultfrequency = (int)mSampleRate;
    ex.numchannels = (int)mChannels;
    ex.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    ex.decodebuffersize = 4096;
    // Length is in bytes. Keep this consistent with the existing 3D Stream
    // OPENUSER paths so FMOD has a less aggressive loop boundary.
    ex.length = mSampleRate * mChannels * sizeof(float) * 5;
    ex.pcmreadcallback = &LLMediaAudioStream::pcmReadCallback;
    ex.userdata = this;

    const FMOD_MODE mode = FMOD_OPENUSER | FMOD_CREATESTREAM | FMOD_LOOP_NORMAL | FMOD_2D;
    if (checkFmod(mSystem->createSound(nullptr, mode, &ex, &mSound),
                  "FMOD::System::createSound(media OPENUSER)"))
    {
        mSound = nullptr;
        return false;
    }

    if (checkFmod(mSystem->playSound(mSound, nullptr, true, &mChannel),
                  "FMOD::System::playSound(media)"))
    {
        stop();
        return false;
    }

    setVolume(mVolume);
    mPlaybackStarted = false;
    mNeedsPrebuffer.store(false, std::memory_order_relaxed);

    return true;
}

U32 LLMediaAudioStream::availableFrames() const
{
    if (!mRing)
    {
        return 0;
    }

    const U32 capacity = mRing->mCapacityFrames;
    if (capacity == 0)
    {
        return 0;
    }

    const U32 total = capacity + 1;
    const U32 read = mRing->mReadFrame.load(std::memory_order_acquire);
    const U32 write = mRing->mWriteFrame.load(std::memory_order_acquire);
    return (write >= read) ? (write - read) : (total - read + write);
}

FMOD_RESULT LLMediaAudioStream::pcmReadCallback(FMOD_SOUND* sound, void* data, unsigned int datalen)
{
    void* userdata = nullptr;
    if (sound && FMOD_Sound_GetUserData(sound, &userdata) == FMOD_OK && userdata)
    {
        return static_cast<LLMediaAudioStream*>(userdata)->readPCM(data, datalen);
    }
    std::memset(data, 0, datalen);
    return FMOD_OK;
}

FMOD_RESULT LLMediaAudioStream::readPCM(void* data, unsigned int datalen)
{
    if (!mRing || mChannels == 0)
    {
        std::memset(data, 0, datalen);
        return FMOD_OK;
    }

    const U32 ring_sample_rate = mRing->mSampleRate.load(std::memory_order_acquire);
    const U32 ring_channels = llclamp((U32)mRing->mChannels.load(std::memory_order_acquire),
                                      1u,
                                      LL_PLUGIN_AUDIO_RING_MAX_CHANNELS);
    const U32 ring_format_serial = mRing->mFormatSerial.load(std::memory_order_acquire);
    if (ring_sample_rate != mSampleRate ||
        ring_channels != mChannels ||
        ring_format_serial != mFormatSerial)
    {
        std::memset(data, 0, datalen);
        mNeedsPrebuffer.store(true, std::memory_order_release);
        return FMOD_OK;
    }

    float* dst = static_cast<float*>(data);
    const U32 request_frames = datalen / (mChannels * sizeof(float));
    const U32 capacity = mRing->mCapacityFrames;
    const U32 total = capacity + 1;
    U32 read = mRing->mReadFrame.load(std::memory_order_relaxed);
    const U32 write = mRing->mWriteFrame.load(std::memory_order_acquire);
    U32 available = (write >= read) ? (write - read) : (total - read + write);
    const bool underflow = available < request_frames;
    const U32 frames_to_read = underflow ? 0 : request_frames;

    U32 min_available = mMinAvailableFrames.load(std::memory_order_relaxed);
    while (available < min_available &&
           !mMinAvailableFrames.compare_exchange_weak(min_available, available, std::memory_order_relaxed))
    {
    }
    U32 max_available = mMaxAvailableFrames.load(std::memory_order_relaxed);
    while (available > max_available &&
           !mMaxAvailableFrames.compare_exchange_weak(max_available, available, std::memory_order_relaxed))
    {
    }

    mCallbackCount.fetch_add(1, std::memory_order_relaxed);
    mFramesRequested.fetch_add(request_frames, std::memory_order_relaxed);
    mFramesRead.fetch_add(frames_to_read, std::memory_order_relaxed);

    const float* samples = reinterpret_cast<const float*>(
        reinterpret_cast<const unsigned char*>(mRing) + mRing->mHeaderSize);

    for (U32 frame = 0; frame < frames_to_read; ++frame)
    {
        const float* src = samples + ((size_t)read * LL_PLUGIN_AUDIO_RING_MAX_CHANNELS);
        for (U32 ch = 0; ch < mChannels; ++ch)
        {
            *dst++ = src[ch];
        }
        read = (read + 1) % total;
    }

    if (frames_to_read < request_frames)
    {
        const size_t missing_samples = (request_frames - frames_to_read) * mChannels;
        std::memset(dst, 0, missing_samples * sizeof(float));
        mFramesSilenced.fetch_add(request_frames - frames_to_read, std::memory_order_relaxed);
        mNeedsPrebuffer.store(true, std::memory_order_release);
    }

    mRing->mReadFrame.store(read, std::memory_order_release);
    return FMOD_OK;
}
