/**
 * @file llayastormperflog.h
 * @brief AYAstorm CPU perf 章: main thread / worker thread zone を CSV に記録する計測 module.
 *
 * 用途: docs/specs/ayastorm-cpu-perf/ §7-A の数字を実機計測で埋めるための
 * 計測 infra。`AYAPERF_ZONE("zoneName")` を hot path scope に挿入すると、
 * RAII で scope dt を CSV に append する。
 *
 * 出力形式: `frame, wall_ms, zone, dt_us` (header 1 行 + data 行)
 *
 * gate: cvar `AYAPerfLogEnabled` (U32, default 0)。OFF のとき RAII zone は
 * 即 return、disk I/O も file open も走らない。
 *
 * thread-safety: recordSample は internal mutex で保護。main + worker (例:
 * LLImageGLThread::syncToMainThread) 双方から呼ばれて良い。
 *
 * 配置: llcommon。lower layer (llrender / llcorehttp 等) からも include 可。
 * init() の path は caller (newview) が gDirUtilp で解決して渡す。
 */
#ifndef LL_AYASTORM_PERFLOG_H
#define LL_AYASTORM_PERFLOG_H

#include "stdtypes.h"

#include <string>

namespace LLAyastormPerfLog
{
    LL_COMMON_API void init(bool enable, const std::string& output_path);
    LL_COMMON_API void shutdown();
    LL_COMMON_API void onFrameEnd();
    LL_COMMON_API void recordSample(const char* zone, U64 dt_us);
    LL_COMMON_API bool isEnabled();
}

class LL_COMMON_API LLAyastormPerfZone
{
public:
    explicit LLAyastormPerfZone(const char* zone_name);
    ~LLAyastormPerfZone();
private:
    const char* mZoneName;
    U64         mStartUs;
};

#define AYAPERF_ZONE_CONCAT2(a, b) a##b
#define AYAPERF_ZONE_CONCAT(a, b)  AYAPERF_ZONE_CONCAT2(a, b)
#define AYAPERF_ZONE(name)         LLAyastormPerfZone AYAPERF_ZONE_CONCAT(_aya_perf_zone_, __LINE__)(name)

#endif // LL_AYASTORM_PERFLOG_H
