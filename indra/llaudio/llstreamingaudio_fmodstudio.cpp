/**
 * @file streamingaudio_fmodstudio.cpp
 * @brief LLStreamingAudio_FMODSTUDIO implementation
 *
 * $LicenseInfo:firstyear=2020&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2020, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llmath.h"

#include "fmodstudio/fmod.hpp"
#include "fmodstudio/fmod_errors.h"

#include "llstreamingaudio_fmodstudio.h"

#include <sstream>

inline bool Check_FMOD_Error(FMOD_RESULT result, const char *string)
{
    if (result == FMOD_OK)
        return false;
    LL_WARNS("AudioImpl") << string << " Error: " << FMOD_ErrorString(result) << LL_ENDL;
    return true;
}

static std::string stream_buffer_size_to_string(FMOD::System* system, const char* context)
{
    unsigned int file_buffer_size = 0;
    FMOD_TIMEUNIT file_buffer_size_type = FMOD_TIMEUNIT_RAWBYTES;
    if (Check_FMOD_Error(system->getStreamBufferSize(&file_buffer_size, &file_buffer_size_type),
                         "FMOD::System::getStreamBufferSize"))
    {
        return std::string("FMOD stream buffer size [") + context + "]: unavailable";
    }

    std::ostringstream stream;
    stream << "FMOD stream buffer size [" << context
           << "]: size=" << file_buffer_size
           << " type=" << static_cast<unsigned int>(file_buffer_size_type);
    return stream.str();
}

class LLAudioStreamManagerFMODSTUDIO
{
public:
    LLAudioStreamManagerFMODSTUDIO(FMOD::System *system, FMOD::ChannelGroup *group, const std::string& url);
    FMOD::Channel* startStream();
    bool stopStream(); // Returns true if the stream was successfully stopped.
    bool ready();

    const std::string& getURL()     { return mInternetStreamURL; }

    FMOD_RESULT getOpenState(FMOD_OPENSTATE& openstate, unsigned int* percentbuffered = NULL, bool* starving = NULL, bool* diskbusy = NULL);
protected:
    FMOD::System* mSystem;
    FMOD::ChannelGroup* mChannelGroup;
    FMOD::Channel* mStreamChannel;
    FMOD::Sound* mInternetStream;
    bool mReady;

    std::string mInternetStreamURL;
};



//---------------------------------------------------------------------------
// Internet Streaming
//---------------------------------------------------------------------------
LLStreamingAudio_FMODSTUDIO::LLStreamingAudio_FMODSTUDIO(FMOD::System *system) :
mSystem(system),
mCurrentInternetStreamp(NULL),
mStreamGroup(NULL),
mFMODInternetStreamChannelp(NULL),
mGain(1.0f),
mWasAlreadyPlaying(false)
{
    applyStreamBufferSize();

    Check_FMOD_Error(system->createChannelGroup("stream", &mStreamGroup), "FMOD::System::createChannelGroup");

    // AYAstorm: high-shelf EQ DSP attached to the stream group. Created in
    // DISABLED mode so quality=0 is bit-identical to upstream FS; quality=1
    // flips A_FILTER to HIGHSHELF to compensate the mp3 HF rolloff that makes
    // parcel music sound "veiled". Always attached so toggling is live.
    if (mStreamGroup &&
        !Check_FMOD_Error(mSystem->createDSPByType(FMOD_DSP_TYPE_MULTIBAND_EQ, &mStreamEqDsp),
                          "FMOD::System::createDSPByType(MULTIBAND_EQ)"))
    {
        // Band A: 6kHz presence shelf +4dB. Lower than the 8kHz air band so
        // the boost lands on instrument/vocal definition where 128 kbps mp3
        // thins them out, instead of just the very top octave. A single
        // gentle shelf produced the most reliable subjective improvement;
        // additional bands (low-mid dip, higher gain) were tried and rolled
        // back as either inaudible or fatigue-inducing.
        mStreamEqDsp->setParameterInt(FMOD_DSP_MULTIBAND_EQ_A_FILTER,
                                      FMOD_DSP_MULTIBAND_EQ_FILTER_DISABLED);
        mStreamEqDsp->setParameterFloat(FMOD_DSP_MULTIBAND_EQ_A_FREQUENCY, 6000.0f);
        mStreamEqDsp->setParameterFloat(FMOD_DSP_MULTIBAND_EQ_A_GAIN, 4.0f);
        Check_FMOD_Error(mStreamGroup->addDSP(0, mStreamEqDsp),
                         "FMOD::ChannelGroup::addDSP(stream EQ)");
    }
}

void LLStreamingAudio_FMODSTUDIO::applyStreamBufferSize()
{
    // FSParcelStreamQuality: original FS used estimated_bitrate=128 kbps × 10 s.
    // Higher-bitrate streams (256/320 kbps MP3, FLAC over HTTP) starve at 128
    // and trigger pause/resume cycles. AYAstorm path raises the hint to 320 to
    // keep ~10 s of buffer for typical high-bitrate sources.
    const U32 buffer_seconds = 10;
    const U32 estimated_bitrate = (mQuality == 1) ? 320u : 128u;
    const U32 stream_buffer_size = estimated_bitrate * buffer_seconds * 128 /*bytes/kbit*/;
    if (!Check_FMOD_Error(mSystem->setStreamBufferSize(stream_buffer_size, FMOD_TIMEUNIT_RAWBYTES),
                          "FMOD::System::setStreamBufferSize"))
    {
        LL_DEBUGS("AudioImpl") << "FSParcelStreamQuality buffer hint: quality=" << mQuality
                                << " estimated_bitrate_kbps=" << estimated_bitrate
                                << " seconds=" << buffer_seconds
                                << " requested_size=" << stream_buffer_size
                                << LL_ENDL;
        LL_DEBUGS("AudioImpl") << stream_buffer_size_to_string(mSystem, "after applyStreamBufferSize") << LL_ENDL;
    }
}

void LLStreamingAudio_FMODSTUDIO::applyStreamEq()
{
    if (!mStreamEqDsp) return;
    // quality=0: filter DISABLED -> DSP is a pass-through (bit-identical).
    // quality=1: HIGHSHELF +4dB above 6kHz to restore the presence band
    // typical 128 kbps mp3 streams thin out.
    const int filter_type = (mQuality == 1)
        ? static_cast<int>(FMOD_DSP_MULTIBAND_EQ_FILTER_HIGHSHELF)
        : static_cast<int>(FMOD_DSP_MULTIBAND_EQ_FILTER_DISABLED);
    mStreamEqDsp->setParameterInt(FMOD_DSP_MULTIBAND_EQ_A_FILTER, filter_type);
}

void LLStreamingAudio_FMODSTUDIO::setQuality(U32 quality)
{
    if (mQuality == quality) return;
    mQuality = quality;
    // Buffer hint reaches FMOD now but only takes effect on next createStream.
    applyStreamBufferSize();
    // EQ filter type swaps live on the existing DSP.
    applyStreamEq();
}

LLStreamingAudio_FMODSTUDIO::~LLStreamingAudio_FMODSTUDIO()
{
    stop();
    for (U32 i = 0; i < 100; ++i)
    {
        if (releaseDeadStreams())
            break;
        ms_sleep(10);
    }

    if (mStreamEqDsp)
    {
        if (mStreamGroup)
        {
            mStreamGroup->removeDSP(mStreamEqDsp);
        }
        mStreamEqDsp->release();
        mStreamEqDsp = nullptr;
    }
}

void LLStreamingAudio_FMODSTUDIO::start(const std::string& url)
{
    //if (!mInited)
    //{
    //  LL_WARNS() << "startInternetStream before audio initialized" << LL_ENDL;
    //  return;
    //}

    // "stop" stream but don't clear url, etc. in case url == mInternetStreamURL
    stop();

    if (!url.empty())
    {
        if (mDeadStreams.empty())
        {
            LL_INFOS() << "Starting internet stream: " << url << LL_ENDL;
            mCurrentInternetStreamp = new LLAudioStreamManagerFMODSTUDIO(mSystem, mStreamGroup, url);
            mURL = url;
        }
        else
        {
            LL_INFOS() << "Deferring stream load until buffer release: " << url << LL_ENDL;
            mPendingURL = url;
        }
    }
    else
    {
        LL_INFOS() << "Set internet stream to null" << LL_ENDL;
        mURL.clear();
    }
}


void LLStreamingAudio_FMODSTUDIO::update()
{
    if (!releaseDeadStreams())
    {
        llassert_always(mCurrentInternetStreamp == NULL);
        return;
    }

    if (!mPendingURL.empty())
    {
        llassert_always(mCurrentInternetStreamp == NULL);
        LL_INFOS() << "Starting internet stream: " << mPendingURL << LL_ENDL;
        mCurrentInternetStreamp = new LLAudioStreamManagerFMODSTUDIO(mSystem, mStreamGroup, mPendingURL);
        mURL = mPendingURL;
        mPendingURL.clear();
    }

    // Don't do anything if there are no streams playing
    if (!mCurrentInternetStreamp)
    {
        return;
    }

    unsigned int progress;
    bool starving;
    bool diskbusy;
    FMOD_OPENSTATE open_state;

    if (Check_FMOD_Error(mCurrentInternetStreamp->getOpenState(open_state, &progress, &starving, &diskbusy), "FMOD::Sound::getOpenState"))
    {
        LL_WARNS() << "Internet stream openstate error: open_state = " << open_state << " - progress = " << progress << " - starving = " << starving << " - diskbusy = " << diskbusy << LL_ENDL;
        bool was_playing = mWasAlreadyPlaying;
        stop();
        // Try to restart previously playing stream on socket error
        if (open_state == FMOD_OPENSTATE_ERROR && was_playing)
        {
            LL_WARNS() << "Stream was playing before - trying to restart" << LL_ENDL;
            start(mURL);
        }
        return;
    }
    else if (open_state == FMOD_OPENSTATE_READY)
    {
        // Stream is live

        // start the stream if it's ready
        if (!mFMODInternetStreamChannelp &&
            (mFMODInternetStreamChannelp = mCurrentInternetStreamp->startStream()))
        {
            // Reset volume to previously set volume
            setGain(getGain());
            Check_FMOD_Error(mFMODInternetStreamChannelp->setPaused(false), "FMOD::Channel::setPaused");
            mWasAlreadyPlaying = true;
        }
    }
    else if (open_state == FMOD_OPENSTATE_PLAYING)
    {
        if (!mWasAlreadyPlaying)
        {
            mWasAlreadyPlaying = true;
        }
    }


    if (mFMODInternetStreamChannelp)
    {
        FMOD::Sound* sound = nullptr;

        if (!Check_FMOD_Error(mFMODInternetStreamChannelp->getCurrentSound(&sound), "FMOD::Channel::getCurrentSound") && sound)
        {
            FMOD_TAG tag;
            S32 tagcount, numtagsupdated;

            if (!Check_FMOD_Error(sound->getNumTags(&tagcount, &numtagsupdated), "FMOD::Sound::getNumTags") && numtagsupdated > 0)
            {
                LL_DEBUGS("StreamMetadata") << "Tag count: " << tagcount << "  Tags updated since last call: " << numtagsupdated << LL_ENDL;

                // <DKO> Stream metadata - originally by Shyotl Khur
                mMetadata.clear();
                // </DKO>
                for (S32 i = 0; i < tagcount; ++i)
                {
                    if (Check_FMOD_Error(sound->getTag(nullptr, i, &tag), "FMOD::Sound::getTag"))
                        continue;

                    // <FS:TJ> [FIRE-36403] Sometimes tag.data is nullptr and causes a crash
                    if (!tag.data)
                    {
                        LL_WARNS("StreamMetadata") << tag.name << ": " << "NULL data with datalen: " << tag.datalen << LL_ENDL;
                        continue;
                    }
                    // </FS:TJ>

                    LL_DEBUGS("StreamMetadata") << "Tag name: " << tag.name << " - Tag type: " << tag.type << " - Tag data type: " << tag.datatype << LL_ENDL;

                    std::string name = tag.name;
                    switch (tag.type)
                    {
                        case FMOD_TAGTYPE_ID3V2:
                        {
                            if (name == "TIT2")
                                name = "TITLE";
                            else if (name == "TPE1")
                                name = "ARTIST";
                            break;
                        }
                        case FMOD_TAGTYPE_ASF:
                        {
                            if (name == "Title")
                                name = "TITLE";
                            else if (name == "WM/AlbumArtist")
                                name = "ARTIST";
                            break;
                        }
                        case FMOD_TAGTYPE_VORBISCOMMENT:
                        {
                            if (name == "title")
                                name = "TITLE";
                            else if (name == "artist")
                                name = "ARTIST";
                            break;
                        }
                        case FMOD_TAGTYPE_FMOD:
                        {
                            if (name == "Sample Rate Change")
                            {
                                LL_INFOS() << "Stream forced changing sample rate to " << *((float*)tag.data) << LL_ENDL;
                                mFMODInternetStreamChannelp->setFrequency(*((float*)tag.data));
                            }
                            continue;
                        }
                        default:
                            break;
                    }

                    switch (tag.datatype)
                    {
                        case FMOD_TAGDATATYPE_INT:
                        {
                            mMetadata[name] = *(LLSD::Integer*)(tag.data);
                            LL_DEBUGS("StreamMetadata") << tag.name << ": " << *(int*)(tag.data) << LL_ENDL;
                            break;
                        }
                        case FMOD_TAGDATATYPE_FLOAT:
                        {
                            mMetadata[name] = *(LLSD::Real*)(tag.data);
                            LL_DEBUGS("StreamMetadata") << tag.name << ": " << *(float*)(tag.data) << LL_ENDL;
                            break;
                        }
                        case FMOD_TAGDATATYPE_STRING:
                        {
                            std::string out = rawstr_to_utf8(std::string((char*)tag.data, tag.datalen));
                            mMetadata[name] = out;
                            LL_DEBUGS("StreamMetadata") << tag.name << ": " << out << LL_ENDL;
                            break;
                        }
                        case FMOD_TAGDATATYPE_STRING_UTF8:
                        {
                            std::string out((char*)tag.data);
                            mMetadata[name] = out;
                            LL_DEBUGS("StreamMetadata") << tag.name << ": " << out << LL_ENDL;
                            break;
                        }
                        case FMOD_TAGDATATYPE_STRING_UTF16:
                        {
                            std::string out = utf16str_to_utf8str((U16*)tag.data, tag.datalen / 2);
                            mMetadata[name] = out;
                            LL_DEBUGS("StreamMetadata") << tag.name << ": " << out << LL_ENDL;
                            break;
                        }
                        case FMOD_TAGDATATYPE_STRING_UTF16BE:
                        {
                            // UTF-16 Big Endian encoded; swap high & low bytes first
                            U16* buffer = (U16*)tag.data;
                            for (U32 j = 0; j < tag.datalen / 2; ++j)
                                buffer[j] = (((buffer[j] & 0xff) << 8) | ((buffer[j] & 0xff00) >> 8));

                            std::string out = utf16str_to_utf8str((U16*)tag.data, tag.datalen / 2);
                            mMetadata[name] = out;
                            LL_DEBUGS("StreamMetadata") << name << ": " << out << LL_ENDL;
                            break;
                        }
                        default:
                            break;
                    }
                }

                mMetadataUpdateSignal(mMetadata);
            }

            if (starving)
            {
                bool paused = false;
                if (!Check_FMOD_Error(mFMODInternetStreamChannelp->getPaused(&paused), "FMOD:Channel::getPaused") && !paused)
                {
                    LL_INFOS() << "Stream starvation detected! Pausing stream until buffer nearly full." << LL_ENDL;
                    LL_INFOS() << "  (diskbusy=" << diskbusy << ")" << LL_ENDL;
                    LL_INFOS() << "  (progress=" << progress << ")" << LL_ENDL;
                    Check_FMOD_Error(mFMODInternetStreamChannelp->setPaused(true), "FMOD::Channel::setPaused");
                }
            }
            else if (progress > 80)
            {
                Check_FMOD_Error(mFMODInternetStreamChannelp->setPaused(false), "FMOD::Channel::setPaused");
            }
        }
    }
}

void LLStreamingAudio_FMODSTUDIO::stop()
{
    mPendingURL.clear();
    mWasAlreadyPlaying = false;

    if (mFMODInternetStreamChannelp)
    {
        Check_FMOD_Error(mFMODInternetStreamChannelp->setPaused(true), "FMOD::Channel::setPaused");
        Check_FMOD_Error(mFMODInternetStreamChannelp->setPriority(0), "FMOD::Channel::setPriority");
        mFMODInternetStreamChannelp = NULL;

        // <FS:Ansariel> Stream meta data display
        mMetadata.clear();
        mMetadataUpdateSignal(mMetadata);
        // </FS:Ansariel>
    }

    if (mCurrentInternetStreamp)
    {
        LL_INFOS() << "Stopping internet stream: " << mCurrentInternetStreamp->getURL() << LL_ENDL;
        if (mCurrentInternetStreamp->stopStream())
        {
            delete mCurrentInternetStreamp;
        }
        else
        {
            LL_WARNS() << "Pushing stream to dead list: " << mCurrentInternetStreamp->getURL() << LL_ENDL;
            mDeadStreams.push_back(mCurrentInternetStreamp);
        }
        mCurrentInternetStreamp = NULL;
    }
}

void LLStreamingAudio_FMODSTUDIO::pause(int pauseopt)
{
    if (pauseopt < 0)
    {
        pauseopt = mCurrentInternetStreamp ? 1 : 0;
    }

    if (pauseopt)
    {
        if (mCurrentInternetStreamp)
        {
            stop();
        }
    }
    else
    {
        start(getURL());
    }
}


// A stream is "playing" if it has been requested to start.  That
// doesn't necessarily mean audio is coming out of the speakers.
int LLStreamingAudio_FMODSTUDIO::isPlaying()
{
    if (mCurrentInternetStreamp)
    {
        return 1; // Active and playing
    }
    else if (!mURL.empty() || !mPendingURL.empty())
    {
        return 2; // "Paused"
    }
    else
    {
        return 0;
    }
}


F32 LLStreamingAudio_FMODSTUDIO::getGain()
{
    return mGain;
}


std::string LLStreamingAudio_FMODSTUDIO::getURL()
{
    return mURL;
}


void LLStreamingAudio_FMODSTUDIO::setGain(F32 vol)
{
    mGain = vol;

    if (mFMODInternetStreamChannelp)
    {
        Check_FMOD_Error(mFMODInternetStreamChannelp->setVolume(vol * vol), "FMOD::Channel::setVolume");
    }
}

///////////////////////////////////////////////////////
// manager of possibly-multiple internet audio streams

LLAudioStreamManagerFMODSTUDIO::LLAudioStreamManagerFMODSTUDIO(FMOD::System *system, FMOD::ChannelGroup *group, const std::string& url) :
mSystem(system),
mChannelGroup(group),
mStreamChannel(NULL),
mInternetStream(NULL),
mReady(false)
{
    mInternetStreamURL = url;

    LL_DEBUGS("AudioImpl") << stream_buffer_size_to_string(mSystem, "before createStream") << LL_ENDL;
    FMOD_RESULT result = mSystem->createStream(url.c_str(), FMOD_2D | FMOD_NONBLOCKING | FMOD_IGNORETAGS, 0, &mInternetStream);

    if (result != FMOD_OK)
    {
        LL_WARNS() << "Couldn't open fmod stream, error "
            << FMOD_ErrorString(result)
            << LL_ENDL;
        mReady = false;
        return;
    }

    mReady = true;
}

FMOD::Channel *LLAudioStreamManagerFMODSTUDIO::startStream()
{
    // We need a live and opened stream before we try and play it.
    FMOD_OPENSTATE open_state;
    if (!mInternetStream || Check_FMOD_Error(getOpenState(open_state), "FMOD::Sound::getOpenState") || open_state != FMOD_OPENSTATE_READY)
    {
        LL_WARNS() << "No internet stream to start playing!" << LL_ENDL;
        return NULL;
    }

    if (mStreamChannel)
        return mStreamChannel;  //Already have a channel for this stream.

    Check_FMOD_Error(mSystem->playSound(mInternetStream, mChannelGroup, true, &mStreamChannel), "FMOD::System::playSound");
    return mStreamChannel;
}

bool LLAudioStreamManagerFMODSTUDIO::stopStream()
{
    if (mInternetStream)
    {
        bool close = true;
        FMOD_OPENSTATE open_state;
        if (!Check_FMOD_Error(getOpenState(open_state), "FMOD::Sound::getOpenState"))
        {
            switch (open_state)
            {
            case FMOD_OPENSTATE_CONNECTING:
                close = false;
                break;
            default:
                close = true;
            }
        }

        if (close && !Check_FMOD_Error(mInternetStream->release(), "FMOD::Sound::release"))
        {
            mStreamChannel = NULL;
            mInternetStream = NULL;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return true;
    }
}

FMOD_RESULT LLAudioStreamManagerFMODSTUDIO::getOpenState(FMOD_OPENSTATE& state, unsigned int* percentbuffered, bool* starving, bool* diskbusy)
{
    if (!mInternetStream)
        return FMOD_ERR_INVALID_HANDLE;
    FMOD_RESULT result = mInternetStream->getOpenState(&state, percentbuffered, starving, diskbusy);
    Check_FMOD_Error(result, "FMOD::Sound::getOpenState");
    return result;
}

void LLStreamingAudio_FMODSTUDIO::setBufferSizes(U32 streambuffertime, U32 decodebuffertime)
{
    const U32 stream_buffer_size = streambuffertime / 1000 * 128 * 128;
    if (Check_FMOD_Error(mSystem->setStreamBufferSize(stream_buffer_size, FMOD_TIMEUNIT_RAWBYTES), "FMOD::System::setStreamBufferSize"))
        return;
    LL_DEBUGS("AudioImpl") << "setBufferSizes stream buffer hint: streambuffertime_ms=" << streambuffertime
                            << " decodebuffertime_ms=" << decodebuffertime
                            << " requested_size=" << stream_buffer_size
                            << LL_ENDL;
    LL_DEBUGS("AudioImpl") << stream_buffer_size_to_string(mSystem, "after setBufferSizes") << LL_ENDL;

    FMOD_ADVANCEDSETTINGS settings;
    memset(&settings, 0, sizeof(settings));
    settings.cbSize = sizeof(settings);
    settings.defaultDecodeBufferSize = decodebuffertime;//ms
    Check_FMOD_Error(mSystem->setAdvancedSettings(&settings), "FMOD::System::setAdvancedSettings");

    if (mQuality == 1)
    {
        applyStreamBufferSize();
    }
}

bool LLStreamingAudio_FMODSTUDIO::releaseDeadStreams()
{
    // Kill dead internet streams, if possible
    std::list<LLAudioStreamManagerFMODSTUDIO *>::iterator iter;
    for (iter = mDeadStreams.begin(); iter != mDeadStreams.end();)
    {
        LLAudioStreamManagerFMODSTUDIO *streamp = *iter;
        if (streamp->stopStream())
        {
            LL_INFOS() << "Closed dead stream" << LL_ENDL;
            delete streamp;
            iter = mDeadStreams.erase(iter);
        }
        else
        {
            iter++;
        }
    }

    return mDeadStreams.empty();
}
