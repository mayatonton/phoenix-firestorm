/**
 * @file llirloader.h
 * @brief AYAstorm r11 mini WAV (RIFF) reader for impulse-response files.
 *
 * P7b: minimal RIFF/WAVE loader that handles PCM16 + IEEE float32 in mono
 * or stereo. The output is two F32 channels of equal length (mono input is
 * duplicated to L=R) so callers can feed two LLPartitionedConvolver kernels
 * without further fan-out. Resampling is intentionally out of scope —
 * bundled IRs are pre-rendered at the viewer's mixing sample rate, and an
 * external IR with a different rate is rejected with a logged error.
 *
 * Not a general-purpose decoder: extensible (WAVEFORMATEXTENSIBLE) headers,
 * compressed formats, and 24-bit PCM are not handled here. The bundled IR
 * pipeline (P7c) commits files in canonical PCM16/FLOAT32 stereo so the
 * common path stays narrow.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Phoenix Firestorm Project, Inc.
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
 * $/LicenseInfo$
 */

#ifndef LL_IRLOADER_H
#define LL_IRLOADER_H

#include "stdtypes.h"

#include <string>
#include <vector>

struct LLIRData
{
    std::vector<F32> samples_l;   // L kernel (== samples_r when source is mono)
    std::vector<F32> samples_r;
    int sample_rate { 0 };
    int source_channels { 0 };    // 1 (mono) or 2 (stereo) of the input file
};

class LLIRLoader
{
public:
    enum class Result
    {
        Ok = 0,
        OpenFailed,
        InvalidHeader,
        UnsupportedFormat,
        UnsupportedChannels,
        UnsupportedBitDepth,
        Truncated,
    };

    // Synchronous read on the calling thread. Caller (= main thread) is
    // responsible for keeping the result alive long enough for the FFT prep
    // step (LLPartitionedConvolver::init) to consume it.
    static Result loadWav(const std::string& path, LLIRData& out);

    static const char* resultString(Result r);
};

#endif // LL_IRLOADER_H
