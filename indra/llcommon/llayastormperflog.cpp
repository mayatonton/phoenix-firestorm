/**
 * @file llayastormperflog.cpp
 * @brief AYAstorm CPU perf 章: zone 計測 CSV writer 実装.
 *
 * See llayastormperflog.h のコメントヘッダ。
 */
#include "linden_common.h"

#include "llayastormperflog.h"

#include "lltimer.h"

#include <fstream>
#include <mutex>

namespace
{
    bool          sEnabled = false;
    std::ofstream sFile;
    U64           sStartUs = 0;
    U64           sFrameN  = 0;
    std::mutex    sMutex;
}

void LLAyastormPerfLog::init(bool enable, const std::string& output_path)
{
    sEnabled = enable;
    if (!sEnabled)
    {
        return;
    }

    sFile.open(output_path.c_str(), std::ios::out | std::ios::trunc);
    if (!sFile.is_open())
    {
        LL_WARNS("AyastormPerfLog")
            << "Failed to open: " << output_path << LL_ENDL;
        sEnabled = false;
        return;
    }
    sFile << "frame,wall_ms,zone,dt_us\n";
    sFile.flush();
    sStartUs = totalTime();
    sFrameN  = 0;

    LL_INFOS("AyastormPerfLog")
        << "Perf log started: " << output_path << LL_ENDL;
}

void LLAyastormPerfLog::shutdown()
{
    std::lock_guard<std::mutex> lock(sMutex);
    if (sEnabled && sFile.is_open())
    {
        sFile.flush();
        sFile.close();
        LL_INFOS("AyastormPerfLog") << "Perf log closed." << LL_ENDL;
    }
    sEnabled = false;
}

void LLAyastormPerfLog::onFrameEnd()
{
    if (!sEnabled)
    {
        return;
    }
    ++sFrameN;
}

void LLAyastormPerfLog::recordSample(const char* zone, U64 dt_us)
{
    if (!sEnabled)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(sMutex);
    if (!sFile.is_open())
    {
        return;
    }
    const U64 wall_ms = (totalTime() - sStartUs) / 1000;
    sFile << sFrameN << ',' << wall_ms << ',' << zone << ',' << dt_us << '\n';
}

bool LLAyastormPerfLog::isEnabled()
{
    return sEnabled;
}

LLAyastormPerfZone::LLAyastormPerfZone(const char* zone_name)
    : mZoneName(zone_name)
    , mStartUs(0)
{
    if (LLAyastormPerfLog::isEnabled())
    {
        mStartUs = totalTime();
    }
}

LLAyastormPerfZone::~LLAyastormPerfZone()
{
    if (mStartUs != 0)
    {
        // <FS:AYAstorm> B1: U64 underflow ガード。zone scope が極短 (~0 us) で
        // totalTime() の clock 粒度・コア間スキューにより end < start を観測した場合、
        // 引き算が 2^64-1 にラップして sum/avg を破壊する。明示的に 0 へ clamp。
        const U64 now   = totalTime();
        const U64 dt_us = (now >= mStartUs) ? (now - mStartUs) : 0;
        // </FS:AYAstorm>
        LLAyastormPerfLog::recordSample(mZoneName, dt_us);
    }
}
