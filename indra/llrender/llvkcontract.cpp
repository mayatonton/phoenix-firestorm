#include "linden_common.h"
#include "llvkcontract.h"
#include "llvkloader.h"
#include "llerror.h"
#include "lltimer.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace LLVKContract
{

namespace
{

const char* CAUSE_NAMES[CAUSE_COUNT] =
{
    "unknown",
    "vk_not_init",
    "record_job_pull",
    "no_shader_or_layout",
    "no_sampler",
    "ubo_collect_overflow",
    "ensure_set_fail",
    "refresh_no_shader",
    "refresh_perprogram_ubo",
    "refresh_shared_ubo",
    "authored_empty",
    "cmd_null",
    "pipeline_null",
    "fb_view_diffuse",
    "fb_view_aux",
    "fb_heap_default",
    "flicker",
    "geoab_input_drift",
    "geoab_kernel_mismatch",
    "geoab_source_drift",
    "geoab_ref_fail",
    "geoab_worker_snapshot",
    "geoab_stage_degenerate",
    "geoab_stage_skip",
    "ubo_slice_fail",
    "pp_fallback_lossy",
    "ubo_offset_stale",
    "ubo_content_stale",
    "memo_cross_cmd",
    "drawdata_scratch_wrap",
    "drawdata_exhausted",
    "drawdata_race",
    "mega_alloc_race",
    "alloc_noncoherent",
    "mv_stale_value",
    "drawdata_id_mismatch",
    "sig_diet_mismatch",
    "par_main_only_write",
    "par_worker_forbidden",
    "par_concurrent",
    "par_dead_access",
    "skin_draw_no_commit",
    "pass_scope_fail",
    "pass_refused"
};

std::string (*sDescribe)(const void*) = nullptr;
U64 (*sKey)(const void*) = nullptr;

std::atomic<bool> sParallelEpochActive{false};
std::atomic<U32>  sThreadTagCounter{1};
thread_local bool tIsWorkerThread = false;
thread_local U32  tThreadTag      = 0;

thread_local const void* tCurDrawInfo = nullptr;
thread_local const char* tCurTag      = nullptr;
thread_local ECause      tLastCause   = C_UNKNOWN;

std::atomic<U64> sCauseWin[CAUSE_COUNT] = {};
std::atomic<U64> sCauseTot[CAUSE_COUNT] = {};
std::atomic<U64> sSkipByCause[CAUSE_COUNT] = {};
std::atomic<U64> sSkipWin{0};
std::atomic<U64> sSkipTot{0};
std::atomic<U64> sFireWin{0};

std::atomic<U64> sFrame{0};
std::atomic<bool> sAnyWatched{false};
std::atomic<U64> sWatchBits[1024] = {};

std::mutex sFlickMutex;
struct FlickEntry
{
    U64 lastSkipFrame = 0;
    U64 lastFireFrame = 0;
    U64 flips         = 0;
    std::string lastDesc;
};
std::unordered_map<U64, FlickEntry> sFlickMap;

F64 sLastSummaryTime = 0.0;

bool pow2(U64 n)
{
    return n != 0 && (n & (n - 1)) == 0;
}

std::string provenance()
{
    std::ostringstream os;
    if (tCurTag != nullptr)
    {
        os << " pool=" << tCurTag;
    }
    if (tCurDrawInfo != nullptr && sDescribe != nullptr)
    {
        os << ' ' << sDescribe(tCurDrawInfo);
    }
    return os.str();
}

void watchBitSet(const void* p)
{
    U64 h = ((U64)(uintptr_t)p) >> 4;
    U32 bit = (U32)(h & 65535u);
    sWatchBits[bit >> 6].fetch_or(1ull << (bit & 63u), std::memory_order_relaxed);
    sAnyWatched.store(true, std::memory_order_relaxed);
}

bool watchBitTest(const void* p)
{
    U64 h = ((U64)(uintptr_t)p) >> 4;
    U32 bit = (U32)(h & 65535u);
    return (sWatchBits[bit >> 6].load(std::memory_order_relaxed) & (1ull << (bit & 63u))) != 0;
}

}

bool verboseEnabled()
{
    static const bool s_on = []() -> bool {
        const char* e = getenv("AYASTORM_VKC");
        return e != nullptr && strcmp(e, "0") != 0;
    }();
    return s_on;
}

void parallelEpochBegin()
{
    sParallelEpochActive.store(true, std::memory_order_release);
}

void parallelEpochEnd()
{
    sParallelEpochActive.store(false, std::memory_order_release);
}

bool parallelEpochActive()
{
    return sParallelEpochActive.load(std::memory_order_acquire);
}

void markWorkerThread(bool is_worker)
{
    tIsWorkerThread = is_worker;
}

bool isWorkerThread()
{
    return tIsWorkerThread;
}

U32 threadTag()
{
    if (tThreadTag == 0)
    {
        tThreadTag = sThreadTagCounter.fetch_add(1, std::memory_order_relaxed);
    }
    return tThreadTag;
}

U64 causeTotal(ECause c)
{
    return (c < CAUSE_COUNT) ? sCauseTot[c].load(std::memory_order_relaxed) : 0;
}

void runParallelSelfTest()
{
    LL_INFOS("ParSelfTest") << "=== parallel-safety detector self-test begin ===" << LL_ENDL;

    const U64 b_wf = causeTotal(C_PAR_WORKER_FORBIDDEN);
    const U64 b_mo = causeTotal(C_PAR_MAIN_ONLY_WRITE);
    const U64 b_da = causeTotal(C_PAR_DEAD_ACCESS);
    const U64 b_ce = causeTotal(C_PAR_CONCURRENT);

    std::thread w1([]()
    {
        markWorkerThread(true);
        parallelEpochBegin();
        { WorkerForbiddenGuard g; }
        { MainOnlyGuard g; }
        { DeadObjectGuard g(true); }
        parallelEpochEnd();
        markWorkerThread(false);
    });
    w1.join();

    std::atomic<U32> owner{0};
    {
        ConcurrentEntryGuard outer(owner);
        std::thread w2([&owner]()
        {
            markWorkerThread(true);
            { ConcurrentEntryGuard inner(owner); }
            markWorkerThread(false);
        });
        w2.join();
    }

    const U64 neg_before = causeTotal(C_PAR_WORKER_FORBIDDEN)
                         + causeTotal(C_PAR_MAIN_ONLY_WRITE)
                         + causeTotal(C_PAR_DEAD_ACCESS);
    { WorkerForbiddenGuard g; }
    { MainOnlyGuard g; }
    { DeadObjectGuard g(true); }
    const bool neg_ok = (causeTotal(C_PAR_WORKER_FORBIDDEN)
                       + causeTotal(C_PAR_MAIN_ONLY_WRITE)
                       + causeTotal(C_PAR_DEAD_ACCESS)) == neg_before;

    const U64 d_wf = causeTotal(C_PAR_WORKER_FORBIDDEN) - b_wf;
    const U64 d_mo = causeTotal(C_PAR_MAIN_ONLY_WRITE) - b_mo;
    const U64 d_da = causeTotal(C_PAR_DEAD_ACCESS) - b_da;
    const U64 d_ce = causeTotal(C_PAR_CONCURRENT) - b_ce;

    const bool pass = (d_wf >= 1) && (d_mo >= 1) && (d_da >= 1) && (d_ce >= 1) && neg_ok;
    LL_INFOS("ParSelfTest") << "WorkerForbidden=" << d_wf << " MainOnly=" << d_mo
                            << " DeadObject=" << d_da << " Concurrent=" << d_ce
                            << " negControl=" << (neg_ok ? "ok" : "FAIL")
                            << " => " << (pass ? "ALL PASS" : "FAIL") << LL_ENDL;
    LL_INFOS("ParSelfTest") << "=== parallel-safety detector self-test end ===" << LL_ENDL;
}

void setResolvers(std::string (*describe)(const void*), U64 (*key)(const void*))
{
    sDescribe = describe;
    sKey      = key;
}

namespace
{
bool perShaderEscalate(ECause c, const std::string& shader_name, U64& out_count);

struct FbSlotStat
{
    U64         count = 0;
    std::string shader;
    U32         binding = 0;
    const char* reason  = nullptr;
};
std::mutex sFbSlotMutex;
std::unordered_map<U64, FbSlotStat> sFbSlotWin;
}

void noteFbSlot(const void* shader_key, const std::string& shader_name, U32 binding, const char* reason)
{
    const U64 key = ((U64)(uintptr_t)shader_key * 0x100000001B3ull)
                    ^ ((U64)binding << 32)
                    ^ (U64)(uintptr_t)reason;
    std::lock_guard<std::mutex> lock(sFbSlotMutex);
    FbSlotStat& st = sFbSlotWin[key];
    if (st.count == 0)
    {
        st.shader  = shader_name;
        st.binding = binding;
        st.reason  = reason;
    }
    ++st.count;
}

void drawScopeBegin(const void* draw_info, const char* tag)
{
    tCurDrawInfo = draw_info;
    tCurTag      = tag;
}

void drawScopeEnd()
{
    tCurDrawInfo = nullptr;
    tCurTag      = nullptr;
}

const void* currentDrawInfo()
{
    return tCurDrawInfo;
}

const char* currentDrawTag()
{
    return tCurTag;
}

void resolveBegin()
{
    tLastCause = C_UNKNOWN;
}

void cause(ECause c)
{
    if (c >= CAUSE_COUNT)
    {
        return;
    }
    tLastCause = c;
    sCauseWin[c].fetch_add(1, std::memory_order_relaxed);
    sCauseTot[c].fetch_add(1, std::memory_order_relaxed);
}

void causeIfNone(ECause c)
{
    if (tLastCause == C_UNKNOWN)
    {
        cause(c);
    }
}

namespace
{
bool perShaderEscalate(ECause c, const std::string& shader_name, U64& out_count)
{
    static std::mutex sMapMutex;
    static std::unordered_map<std::string, U64> sCounts;
    std::string key = std::to_string((U32)c) + '|' + shader_name;
    std::lock_guard<std::mutex> lock(sMapMutex);
    U64 n = ++sCounts[key];
    out_count = n;
    return pow2(n);
}
}

void note(ECause c, const std::string& shader_name)
{
    if (c >= CAUSE_COUNT)
    {
        return;
    }
    sCauseWin[c].fetch_add(1, std::memory_order_relaxed);
    sCauseTot[c].fetch_add(1, std::memory_order_relaxed);
    if (!verboseEnabled())
    {
        return;
    }
    if (c == C_FB_VIEW_DIFFUSE || c == C_FB_HEAP_DEFAULT)
    {
        U64 sn = 0;
        if (perShaderEscalate(c, shader_name, sn))
        {
            LL_WARNS("VKContract") << "VKC " << CAUSE_NAMES[c]
                                   << " shader='" << shader_name << "'"
                                   << " sn=" << sn << provenance() << LL_ENDL;
        }
    }
    else
    {
        U64 n = sCauseTot[c].load(std::memory_order_relaxed);
        if (pow2(n))
        {
            LL_WARNS("VKContract") << "VKC " << CAUSE_NAMES[c]
                                   << " shader='" << shader_name << "'"
                                   << " n=" << n << provenance() << LL_ENDL;
        }
    }
}

void noteDetail(ECause c, const char* key, const std::string& detail)
{
    if (c >= CAUSE_COUNT)
    {
        return;
    }
    sCauseWin[c].fetch_add(1, std::memory_order_relaxed);
    sCauseTot[c].fetch_add(1, std::memory_order_relaxed);
    U64 sn = 0;
    if (perShaderEscalate(c, key != nullptr ? key : "", sn))
    {
        LL_WARNS("VKContract") << "VKC " << CAUSE_NAMES[c]
                               << ' ' << detail
                               << " sn=" << sn << LL_ENDL;
    }
}

namespace
{
std::atomic<U64> sVfyWin[VFY_COUNT] = {};
thread_local U32  tExpectedDrawDataID = 0;
thread_local bool tExpectedDDValid    = false;
thread_local bool tPerDrawIDCommitted = false;
}

void vfyTick(U32 which)
{
    if (which < VFY_COUNT)
    {
        sVfyWin[which].fetch_add(1, std::memory_order_relaxed);
    }
}

void stashDrawDataID(U32 id)
{
    tExpectedDrawDataID = id;
    tExpectedDDValid    = true;
}

void checkDrawDataIDAtFire(U32 actual)
{
    if (!verboseEnabled() || !tExpectedDDValid)
    {
        return;
    }
    vfyTick(VFY_DD);
    if (actual != tExpectedDrawDataID)
    {
        causeNamed(C_DRAWDATA_ID_MISMATCH,
                   std::to_string(actual) + "!=" + std::to_string(tExpectedDrawDataID));
    }
}

void markPerDrawIDCommitted()
{
    tPerDrawIDCommitted = true;
}

void checkPerDrawIDFreshnessAtFire(bool fired, bool uses_skin_set, const char* shader_name)
{
    if (!verboseEnabled())
    {
        return;
    }
    const bool committed = tPerDrawIDCommitted;
    tPerDrawIDCommitted = false;
    if (fired && uses_skin_set && !committed)
    {
        const std::string name = (shader_name != nullptr) ? shader_name : "";
        note(C_SKIN_DRAW_NO_COMMIT, name);
        noteFbSlot((const void*)(uintptr_t)std::hash<std::string>{}(name),
                   name, (U32)C_SKIN_DRAW_NO_COMMIT, "no_commit");
    }
}

void causeNamed(ECause c, const std::string& shader_name)
{
    if (c >= CAUSE_COUNT)
    {
        return;
    }
    cause(c);
    if (!verboseEnabled())
    {
        return;
    }
    U64 sn = 0;
    if (perShaderEscalate(c, shader_name, sn))
    {
        LL_WARNS("VKContract") << "VKC recovered cause=" << CAUSE_NAMES[c]
                               << " shader='" << shader_name << "'"
                               << " sn=" << sn << provenance() << LL_ENDL;
    }
}

void drawSkipped(ECause fire_cause, const std::string& shader_name)
{
    ECause c = fire_cause;
    if (c == C_UNKNOWN)
    {
        c = tLastCause;
    }
    else
    {
        sCauseWin[c].fetch_add(1, std::memory_order_relaxed);
        sCauseTot[c].fetch_add(1, std::memory_order_relaxed);
    }
    sSkipWin.fetch_add(1, std::memory_order_relaxed);
    sSkipTot.fetch_add(1, std::memory_order_relaxed);
    noteFbSlot((const void*)(uintptr_t)std::hash<std::string>{}(shader_name),
               shader_name, (U32)c, "skip");
    U64 n = sSkipByCause[c].fetch_add(1, std::memory_order_relaxed) + 1;
    if (verboseEnabled() && pow2(n))
    {
        LL_WARNS("VKContract") << "VKC skip cause=" << CAUSE_NAMES[c]
                               << " shader='" << shader_name << "'"
                               << " n=" << n << provenance() << LL_ENDL;
    }

    if (tCurDrawInfo != nullptr && sKey != nullptr)
    {
        U64 key = sKey(tCurDrawInfo);
        if (key != 0)
        {
            watchBitSet(tCurDrawInfo);
            U64 frame = sFrame.load(std::memory_order_relaxed);
            std::lock_guard<std::mutex> lock(sFlickMutex);
            FlickEntry& e = sFlickMap[key];
            if (e.lastFireFrame != 0 && frame >= e.lastFireFrame && frame - e.lastFireFrame <= 3)
            {
                ++e.flips;
                sCauseWin[C_FLICKER].fetch_add(1, std::memory_order_relaxed);
                U64 f = sCauseTot[C_FLICKER].fetch_add(1, std::memory_order_relaxed) + 1;
                if (verboseEnabled() && pow2(f))
                {
                    LL_WARNS("VKContract") << "VKC flicker fire/skip oscillation cause=" << CAUSE_NAMES[c]
                                           << " shader='" << shader_name << "'"
                                           << " flips=" << e.flips
                                           << " n=" << f << provenance() << LL_ENDL;
                }
            }
            e.lastSkipFrame = frame;
            if (sDescribe != nullptr && e.lastDesc.empty())
            {
                e.lastDesc = sDescribe(tCurDrawInfo);
            }
        }
    }
}

void drawFired()
{
    sFireWin.fetch_add(1, std::memory_order_relaxed);
    if (!sAnyWatched.load(std::memory_order_relaxed))
    {
        return;
    }
    if (tCurDrawInfo == nullptr || sKey == nullptr)
    {
        return;
    }
    if (!watchBitTest(tCurDrawInfo))
    {
        return;
    }
    U64 key = sKey(tCurDrawInfo);
    if (key == 0)
    {
        return;
    }
    U64 frame = sFrame.load(std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(sFlickMutex);
    auto it = sFlickMap.find(key);
    if (it == sFlickMap.end())
    {
        return;
    }
    FlickEntry& e = it->second;
    if (e.lastSkipFrame != 0 && frame >= e.lastSkipFrame && frame - e.lastSkipFrame <= 3)
    {
        ++e.flips;
        sCauseWin[C_FLICKER].fetch_add(1, std::memory_order_relaxed);
        U64 f = sCauseTot[C_FLICKER].fetch_add(1, std::memory_order_relaxed) + 1;
        if (verboseEnabled() && pow2(f))
        {
            LL_WARNS("VKContract") << "VKC flicker skip/fire oscillation"
                                   << " flips=" << e.flips
                                   << " n=" << f
                                   << (e.lastDesc.empty() ? provenance() : (" " + e.lastDesc))
                                   << LL_ENDL;
        }
    }
    e.lastFireFrame = frame;
}

void frameBegin()
{
    sFrame.fetch_add(1, std::memory_order_relaxed);

    F64 now = LLTimer::getElapsedSeconds();
    if (sLastSummaryTime == 0.0)
    {
        sLastSummaryTime = now;
        return;
    }
    if (now - sLastSummaryTime < 10.0)
    {
        return;
    }
    sLastSummaryTime = now;

    U64 win[CAUSE_COUNT];
    U64 any = 0;
    for (U32 i = 0; i < CAUSE_COUNT; ++i)
    {
        win[i] = sCauseWin[i].exchange(0, std::memory_order_relaxed);
        any += win[i];
    }
    U64 skips = sSkipWin.exchange(0, std::memory_order_relaxed);
    U64 fired = sFireWin.exchange(0, std::memory_order_relaxed);
    U64 vfy_win[VFY_COUNT];
    U64 vfy_any = 0;
    for (U32 i = 0; i < VFY_COUNT; ++i)
    {
        vfy_win[i] = sVfyWin[i].exchange(0, std::memory_order_relaxed);
        vfy_any += vfy_win[i];
    }
    if (any == 0 && skips == 0 && vfy_any == 0)
    {
        return;
    }
    std::ostringstream os;
    os << "VKC-SUM 10s skips=" << skips << " fired=" << fired << " cause{";
    bool first = true;
    for (U32 i = 0; i < CAUSE_COUNT; ++i)
    {
        if (win[i] == 0)
        {
            continue;
        }
        if (!first)
        {
            os << ' ';
        }
        os << CAUSE_NAMES[i] << '=' << win[i];
        first = false;
    }
    os << '}';
    os << " pfree=" << LLVKLoader::getPendingImageFreeCount()
       << " vkblk=" << LLVKLoader::getVmaTotalBlockCount();

    if (vfy_any != 0)
    {
        os << " vfy{bind=" << vfy_win[VFY_BIND]
           << " mv=" << vfy_win[VFY_MV]
           << " dd=" << vfy_win[VFY_DD] << '}';
    }

    {
        std::vector<FbSlotStat> slots;
        {
            std::lock_guard<std::mutex> lock(sFbSlotMutex);
            slots.reserve(sFbSlotWin.size());
            for (auto& kv : sFbSlotWin)
            {
                slots.push_back(kv.second);
            }
            sFbSlotWin.clear();
        }
        if (!slots.empty())
        {
            std::sort(slots.begin(), slots.end(),
                      [](const FbSlotStat& a, const FbSlotStat& b) { return a.count > b.count; });
            os << " fbslot{";
            const size_t top = slots.size() < 12 ? slots.size() : 12;
            bool first_slot = true;
            auto emit_slot = [&](const FbSlotStat& st)
            {
                if (!first_slot)
                {
                    os << ", ";
                }
                first_slot = false;
                os << st.shader << ":b" << st.binding << ':'
                   << (st.reason != nullptr ? st.reason : "?")
                   << '=' << st.count;
            };
            for (size_t i = 0; i < top; ++i)
            {
                emit_slot(slots[i]);
            }
            for (size_t i = top; i < slots.size(); ++i)
            {
                if (slots[i].reason != nullptr && std::strcmp(slots[i].reason, "skip") == 0)
                {
                    emit_slot(slots[i]);
                }
            }
            if (slots.size() > top)
            {
                os << " +" << (slots.size() - top);
            }
            os << '}';
        }
    }

    LL_WARNS("VKContract") << os.str() << LL_ENDL;
}

namespace
{
struct NoProgressStreak
{
    U64 fp    = 0;
    U32 count = 0;
};
std::mutex sNoProgressMutex;
std::unordered_map<std::string, NoProgressStreak> sNoProgressStreaks;
constexpr U32 NO_PROGRESS_THRESHOLD = 3;
}

void noteCorrectiveAction(const char* site, U64 state_fingerprint)
{
    std::lock_guard<std::mutex> lk(sNoProgressMutex);
    NoProgressStreak& s = sNoProgressStreaks[site];
    if (s.fp == state_fingerprint && s.count > 0)
    {
        ++s.count;
    }
    else
    {
        s.fp    = state_fingerprint;
        s.count = 1;
    }
    if (s.count == NO_PROGRESS_THRESHOLD ||
        (s.count > NO_PROGRESS_THRESHOLD && (s.count % 32) == 0))
    {
        LL_WARNS("VKContract") << "VKC no_progress site=" << site
                               << " fp=0x" << std::hex << state_fingerprint << std::dec
                               << " streak=" << s.count << LL_ENDL;
    }
}

}
