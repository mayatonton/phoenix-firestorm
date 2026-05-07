/**
 * @file llirloader.cpp
 * @brief AYAstorm r11 mini WAV (RIFF) reader for impulse-response files.
 *
 * Strict-by-design: rejects anything outside PCM16 / float32 stereo or mono.
 * Bundled IRs (P7c) live inside that envelope, so this loader's narrowness
 * doubles as a guard against feeding the convolver something it can't handle.
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

#include "linden_common.h"

#include "llirloader.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace
{
    // WAVE format tags we accept.
    constexpr uint16_t kWaveFormatPcm   = 0x0001;
    constexpr uint16_t kWaveFormatFloat = 0x0003;

    // FourCC helpers — RIFF stores tags as little-endian 4-byte ASCII.
    inline uint32_t fourcc(const char a, const char b, const char c, const char d)
    {
        return  (static_cast<uint32_t>(static_cast<uint8_t>(a)))
              | (static_cast<uint32_t>(static_cast<uint8_t>(b)) << 8)
              | (static_cast<uint32_t>(static_cast<uint8_t>(c)) << 16)
              | (static_cast<uint32_t>(static_cast<uint8_t>(d)) << 24);
    }

    inline uint16_t readU16LE(const uint8_t* p)
    {
        return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
    }

    inline uint32_t readU32LE(const uint8_t* p)
    {
        return  static_cast<uint32_t>(p[0])
              | (static_cast<uint32_t>(p[1]) << 8)
              | (static_cast<uint32_t>(p[2]) << 16)
              | (static_cast<uint32_t>(p[3]) << 24);
    }

    inline int16_t readS16LE(const uint8_t* p)
    {
        return static_cast<int16_t>(readU16LE(p));
    }

    inline F32 readF32LE(const uint8_t* p)
    {
        // Little-endian on every platform AYAstorm targets — direct memcpy is
        // fine; the reads above already assume LE.
        F32 v;
        std::memcpy(&v, p, sizeof(F32));
        return v;
    }

    // RAII wrapper so each early-return path closes the file.
    struct ScopedFile
    {
        std::FILE* fp { nullptr };
        ~ScopedFile() { if (fp) std::fclose(fp); }
    };
}

LLIRLoader::Result LLIRLoader::loadWav(const std::string& path, LLIRData& out)
{
    out.samples_l.clear();
    out.samples_r.clear();
    out.sample_rate     = 0;
    out.source_channels = 0;

    ScopedFile sf;
    sf.fp = std::fopen(path.c_str(), "rb");
    if (!sf.fp)
    {
        return Result::OpenFailed;
    }

    // RIFF header: "RIFF" <u32 size> "WAVE".
    uint8_t riff[12];
    if (std::fread(riff, 1, sizeof(riff), sf.fp) != sizeof(riff))
    {
        return Result::Truncated;
    }
    if (readU32LE(riff)     != fourcc('R','I','F','F') ||
        readU32LE(riff + 8) != fourcc('W','A','V','E'))
    {
        return Result::InvalidHeader;
    }

    // Walk chunks until we have both fmt and data.
    bool     have_fmt   = false;
    uint16_t fmt_tag    = 0;
    uint16_t channels   = 0;
    uint32_t sample_rate = 0;
    uint16_t bits       = 0;

    std::vector<uint8_t> data_bytes;

    while (true)
    {
        uint8_t hdr[8];
        size_t got = std::fread(hdr, 1, sizeof(hdr), sf.fp);
        if (got == 0)
        {
            break;  // clean EOF
        }
        if (got != sizeof(hdr))
        {
            return Result::Truncated;
        }
        const uint32_t chunk_id   = readU32LE(hdr);
        const uint32_t chunk_size = readU32LE(hdr + 4);

        if (chunk_id == fourcc('f','m','t',' '))
        {
            // We only need the first 16 bytes of fmt for PCM/float formats.
            if (chunk_size < 16)
            {
                return Result::InvalidHeader;
            }
            std::vector<uint8_t> fmt(chunk_size);
            if (std::fread(fmt.data(), 1, chunk_size, sf.fp) != chunk_size)
            {
                return Result::Truncated;
            }
            fmt_tag     = readU16LE(&fmt[0]);
            channels    = readU16LE(&fmt[2]);
            sample_rate = readU32LE(&fmt[4]);
            // bytes/sec @ 8, block_align @ 12 — unused; we re-derive.
            bits        = readU16LE(&fmt[14]);
            have_fmt    = true;
        }
        else if (chunk_id == fourcc('d','a','t','a'))
        {
            data_bytes.resize(chunk_size);
            if (chunk_size > 0 &&
                std::fread(data_bytes.data(), 1, chunk_size, sf.fp) != chunk_size)
            {
                return Result::Truncated;
            }
            // RIFF chunks are word-aligned; skip the pad byte if present.
            if (chunk_size & 1u)
            {
                std::fseek(sf.fp, 1, SEEK_CUR);
            }
            // We have the data chunk — everything past it is irrelevant for
            // an IR (no LIST/INFO/cue/etc. consumers here).
            break;
        }
        else
        {
            // Skip unknown chunks; honour the word-align pad.
            const long skip = static_cast<long>(chunk_size + (chunk_size & 1u));
            if (std::fseek(sf.fp, skip, SEEK_CUR) != 0)
            {
                return Result::Truncated;
            }
        }
    }

    if (!have_fmt || data_bytes.empty())
    {
        return Result::InvalidHeader;
    }
    if (fmt_tag != kWaveFormatPcm && fmt_tag != kWaveFormatFloat)
    {
        return Result::UnsupportedFormat;
    }
    if (channels != 1 && channels != 2)
    {
        return Result::UnsupportedChannels;
    }
    const bool is_float = (fmt_tag == kWaveFormatFloat);
    if (is_float && bits != 32)
    {
        return Result::UnsupportedBitDepth;
    }
    if (!is_float && bits != 16)
    {
        return Result::UnsupportedBitDepth;
    }

    const int    bytes_per_sample = bits / 8;
    const int    frame_bytes      = bytes_per_sample * channels;
    if (frame_bytes == 0 || (data_bytes.size() % frame_bytes) != 0)
    {
        return Result::Truncated;
    }
    const size_t num_frames = data_bytes.size() / frame_bytes;

    out.sample_rate     = static_cast<int>(sample_rate);
    out.source_channels = channels;
    out.samples_l.resize(num_frames);
    out.samples_r.resize(num_frames);

    const uint8_t* p = data_bytes.data();
    if (is_float)
    {
        if (channels == 1)
        {
            for (size_t i = 0; i < num_frames; ++i, p += 4)
            {
                const F32 v = readF32LE(p);
                out.samples_l[i] = v;
                out.samples_r[i] = v;
            }
        }
        else  // stereo
        {
            for (size_t i = 0; i < num_frames; ++i, p += 8)
            {
                out.samples_l[i] = readF32LE(p);
                out.samples_r[i] = readF32LE(p + 4);
            }
        }
    }
    else  // PCM16
    {
        constexpr F32 inv_scale = 1.f / 32768.f;
        if (channels == 1)
        {
            for (size_t i = 0; i < num_frames; ++i, p += 2)
            {
                const F32 v = static_cast<F32>(readS16LE(p)) * inv_scale;
                out.samples_l[i] = v;
                out.samples_r[i] = v;
            }
        }
        else  // stereo
        {
            for (size_t i = 0; i < num_frames; ++i, p += 4)
            {
                out.samples_l[i] = static_cast<F32>(readS16LE(p))     * inv_scale;
                out.samples_r[i] = static_cast<F32>(readS16LE(p + 2)) * inv_scale;
            }
        }
    }

    return Result::Ok;
}

const char* LLIRLoader::resultString(Result r)
{
    switch (r)
    {
    case Result::Ok:                   return "Ok";
    case Result::OpenFailed:           return "OpenFailed";
    case Result::InvalidHeader:        return "InvalidHeader";
    case Result::UnsupportedFormat:    return "UnsupportedFormat";
    case Result::UnsupportedChannels:  return "UnsupportedChannels";
    case Result::UnsupportedBitDepth:  return "UnsupportedBitDepth";
    case Result::Truncated:            return "Truncated";
    }
    return "Unknown";
}
