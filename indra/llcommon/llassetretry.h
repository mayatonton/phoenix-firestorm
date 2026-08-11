#pragma once

#include "stdtypes.h"

#include <atomic>
#include <cstdlib>

constexpr U8 ASSET_RETRY_LIMIT = 8;
constexpr F32 ASSET_RETRY_DRAWN_CAP_SEC = 32.f;

inline std::atomic<U32> gAssetOracleTexStuck{0};
inline std::atomic<U32> gAssetOracleWearPending{0};
inline std::atomic<U32> gAssetOracleTexRecovered{0};
inline std::atomic<U32> gAssetOracleGltfRecovered{0};
inline std::atomic<U32> gAssetOracleWearRecovered{0};
inline std::atomic<U32> gAssetOracleMotionRecovered{0};
inline std::atomic<U32> gAssetOracleMeshRecovered{0};
inline std::atomic<U32> gAssetOracleSoundRecovered{0};
inline std::atomic<U32> gAssetOracleBakeRearmApplied{0};
inline std::atomic<U32> gAssetOracleEnvRecovered{0};
inline std::atomic<U32> gAssetOracleMatRecovered{0};
inline std::atomic<U32> gAssetOracleMeshKick{0};
inline std::atomic<U32> gAssetOracleGeoRepair{0};
inline std::atomic<U32> gAssetOracleLodPromote{0};

inline F32 assetRetryDelaySec(U8 attempt)
{
    static const F32 s_base = []() -> F32 {
        const char* e = getenv("AYASTORM_ASSET_RETRY_BASE_SEC");
        const F32 v = (e != nullptr) ? (F32)atof(e) : 0.f;
        return (v > 0.f) ? v : 4.f;
    }();
    const U8 shift = (attempt > 0) ? (U8)(attempt - 1) : 0;
    return s_base * (F32)(1u << shift);
}
