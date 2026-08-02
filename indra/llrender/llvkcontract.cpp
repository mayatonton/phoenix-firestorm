#include "linden_common.h"
#include "llvkcontract.h"
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
    "map_evict_unpaired",
    "map_evict_long",
    "map_evict_unpaired_hide",
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
    "list_drop_infrustum",
    "list_occl_drop",
    "list_resume",
    "list_absent_long",
    "uuid_absent",
    "uuid_fb_diffuse",
    "uuid_fb_aux",
    "sig_diet_mismatch",
    "par_main_only_write",
    "par_worker_forbidden",
    "par_concurrent",
    "par_dead_access",
    "skin_draw_no_commit"
};

const char* SITE_NAMES[SITE_COUNT] =
{
    "none",
    "strip_destroy",
    "strip_cleanup",
    "strip_delete_faces",
    "clear_group_dtor",
    "clear_rebuild_generic",
    "clear_last_element",
    "clear_zombie",
    "clear_destroy_gl",
    "clear_apply"
};

bool siteTerminal(U32 s)
{
    return s == SITE_STRIP_DESTROY
        || s == SITE_STRIP_CLEANUP
        || s == SITE_CLEAR_GROUP_DTOR
        || s == SITE_CLEAR_LAST_ELEMENT
        || s == SITE_CLEAR_ZOMBIE;
}

struct SentEntry
{
    U32 site = 0;
    U32 objId = 0;
    U32 records = 0;
    U64 frame = 0;
    U8 stage = 0;
    bool eligible = true;
};

std::mutex sSentMutex;
std::unordered_map<const void*, SentEntry> sSentPending;
std::atomic<U64> sSentPendingCount{0};
std::atomic<U64> sSiteWin[SITE_COUNT] = {};
std::atomic<U64> sGapWin[4] = {};


std::string (*sDescribe)(const void*) = nullptr;
U64 (*sKey)(const void*) = nullptr;
U32 (*sObjIdFn)(const void*) = nullptr;
U32 (*sPassBucketFn)() = nullptr;

std::atomic<U32> sWatchDynamicId{0};

bool watchPickMode()
{
    static const bool s_pick = []() -> bool {
        const char* e = getenv("AYASTORM_VKC_OBJ");
        return e != nullptr && strcmp(e, "pick") == 0;
    }();
    return s_pick;
}

U32 watchObjId()
{
    static const U32 s_id = []() -> U32 {
        const char* e = getenv("AYASTORM_VKC_OBJ");
        return e != nullptr ? (U32)strtoul(e, nullptr, 10) : 0u;
    }();
    if (s_id != 0)
    {
        return s_id;
    }
    return watchPickMode() ? sWatchDynamicId.load(std::memory_order_relaxed) : 0u;
}

std::atomic<U64> sWatchFrameFired{0};
std::atomic<U64> sWatchFrameCam{0};
U64 sWatchPrevCam = 0;
U64 sWatchFrames  = 0;
U64 sWatchZeroCam = 0;
U64 sWatchCamMin  = ~0ull;
U64 sWatchCamMax  = 0;
U64 sWatchTot     = 0;

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

void setObjIdResolver(U32 (*fn)(const void*))
{
    sObjIdFn = fn;
}

void setPassBucketResolver(U32 (*fn)())
{
    sPassBucketFn = fn;
}

bool watchPickModeEnabled()
{
    return watchPickMode();
}

namespace
{
std::mutex sWatchLocalsMutex;
std::unordered_set<U32> sWatchLocals;
std::atomic<bool> sWatchLocalsAny{false};
bool perShaderEscalate(ECause c, const std::string& shader_name, U64& out_count);

struct WatchStageEv
{
    const char* what = nullptr;
    U32 n = 0;
    U64 frame = 0;
};
struct WatchEvictEv
{
    U32 site = 0;
    U32 records = 0;
    U64 frame = 0;
};
std::unordered_map<U32, WatchStageEv> sWatchStage;
std::unordered_map<U32, WatchEvictEv> sWatchEvict;

struct FbSlotStat
{
    U64         count = 0;
    std::string shader;
    U32         binding = 0;
    const char* reason  = nullptr;
};
std::mutex sFbSlotMutex;
std::unordered_map<U64, FbSlotStat> sFbSlotWin;

bool watchContainsLocked(U32 localid)
{
    return sWatchLocals.find(localid) != sWatchLocals.end();
}
}

void watchAddLocal(U32 localid)
{
    if (localid == 0)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(sWatchLocalsMutex);
    sWatchLocals.insert(localid);
    sWatchLocalsAny.store(true, std::memory_order_relaxed);
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

void watchFbProbe(bool diffuse, const char* reason)
{
    if (!sWatchLocalsAny.load(std::memory_order_relaxed)
        || tCurDrawInfo == nullptr || sObjIdFn == nullptr)
    {
        return;
    }
    const U32 id = sObjIdFn(tCurDrawInfo);
    {
        std::lock_guard<std::mutex> lock(sWatchLocalsMutex);
        if (sWatchLocals.find(id) == sWatchLocals.end())
        {
            return;
        }
    }
    const ECause c = diffuse ? C_UUID_FB_DIFFUSE : C_UUID_FB_AUX;
    sCauseWin[c].fetch_add(1, std::memory_order_relaxed);
    sCauseTot[c].fetch_add(1, std::memory_order_relaxed);
    if (!verboseEnabled())
    {
        return;
    }
    U64 sn = 0;
    if (perShaderEscalate(c, std::to_string(id) + '|' + (reason != nullptr ? reason : "?"), sn))
    {
        LL_WARNS("VKContract") << "VKC-UUID fb " << (diffuse ? "diffuse" : "aux")
                               << " obj=" << id
                               << " reason=" << (reason != nullptr ? reason : "?")
                               << " sn=" << sn << provenance() << LL_ENDL;
    }
}

void watchStageEvent(U32 localid, const char* what, U32 n)
{
    if (localid == 0 || what == nullptr
        || !sWatchLocalsAny.load(std::memory_order_relaxed))
    {
        return;
    }
    std::lock_guard<std::mutex> lock(sWatchLocalsMutex);
    if (!watchContainsLocked(localid))
    {
        return;
    }
    WatchStageEv& ev = sWatchStage[localid];
    ev.what  = what;
    ev.n     = n;
    ev.frame = sFrame.load(std::memory_order_relaxed);
}

bool watchLastStage(U32 localid, const char*& what, U32& n, U64& age)
{
    std::lock_guard<std::mutex> lock(sWatchLocalsMutex);
    auto it = sWatchStage.find(localid);
    if (it == sWatchStage.end())
    {
        return false;
    }
    what = it->second.what;
    n    = it->second.n;
    age  = sFrame.load(std::memory_order_relaxed) - it->second.frame;
    return true;
}

bool watchLastEvict(U32 localid, U32& site, U32& records, U64& age)
{
    std::lock_guard<std::mutex> lock(sWatchLocalsMutex);
    auto it = sWatchEvict.find(localid);
    if (it == sWatchEvict.end())
    {
        return false;
    }
    site    = it->second.site;
    records = it->second.records;
    age     = sFrame.load(std::memory_order_relaxed) - it->second.frame;
    return true;
}

const char* sentinelSiteName(U32 site)
{
    switch (site)
    {
        case SITE_NONE:                  return "none";
        case SITE_STRIP_DESTROY:         return "strip_destroy";
        case SITE_STRIP_CLEANUP:         return "strip_cleanup";
        case SITE_STRIP_DELETE_FACES:    return "strip_delete_faces";
        case SITE_CLEAR_GROUP_DTOR:      return "group_dtor";
        case SITE_CLEAR_REBUILD_GENERIC: return "rebuild_generic";
        case SITE_CLEAR_LAST_ELEMENT:    return "last_element";
        case SITE_CLEAR_ZOMBIE:          return "zombie";
        case SITE_CLEAR_DESTROY_GL:      return "destroy_gl";
        case SITE_CLEAR_APPLY:           return "apply";
        default:                         return "?";
    }
}

void watchPickCandidate(U32 localid)
{
    if (!watchPickMode() || localid == 0)
    {
        return;
    }
    const U32 prev = sWatchDynamicId.exchange(localid, std::memory_order_relaxed);
    if (prev != localid)
    {
        LL_WARNS("VKContract") << "VKC-OBJ watch locked obj=" << localid
                               << (prev != 0 ? " (switched)" : "") << LL_ENDL;
    }
}

namespace
{
std::unordered_map<U32, U32> sWatchFires;
}

void drawScopeBegin(const void* draw_info, const char* tag)
{
    tCurDrawInfo = draw_info;
    tCurTag      = tag;
    if (draw_info != nullptr && sObjIdFn != nullptr
        && sWatchLocalsAny.load(std::memory_order_relaxed))
    {
        const U32 id = sObjIdFn(draw_info);
        if (id != 0)
        {
            std::lock_guard<std::mutex> lock(sWatchLocalsMutex);
            if (watchContainsLocked(id))
            {
                ++sWatchFires[id];
            }
        }
    }
}

U32 watchTakeFires(U32 localid)
{
    std::lock_guard<std::mutex> lock(sWatchLocalsMutex);
    auto it = sWatchFires.find(localid);
    if (it == sWatchFires.end())
    {
        return 0;
    }
    const U32 n = it->second;
    it->second = 0;
    return n;
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

void sentinelEvict(U32 site, const void* drawable, U32 obj_local_id, U32 record_count, bool drawable_dead, bool eligible)
{
    if (drawable == nullptr || site >= SITE_COUNT || !verboseEnabled())
    {
        return;
    }
    if (obj_local_id != 0 && sWatchLocalsAny.load(std::memory_order_relaxed))
    {
        std::lock_guard<std::mutex> wlock(sWatchLocalsMutex);
        if (watchContainsLocked(obj_local_id))
        {
            WatchEvictEv& ev = sWatchEvict[obj_local_id];
            ev.site    = site;
            ev.records = record_count;
            ev.frame   = sFrame.load(std::memory_order_relaxed);
        }
    }
    std::lock_guard<std::mutex> lock(sSentMutex);
    if (siteTerminal(site) || drawable_dead)
    {
        if (sSentPending.erase(drawable) > 0)
        {
            sSentPendingCount.fetch_sub(1, std::memory_order_relaxed);
        }
        return;
    }
    SentEntry e;
    e.site = site;
    e.objId = obj_local_id;
    e.records = record_count;
    e.frame = sFrame.load(std::memory_order_relaxed);
    e.eligible = eligible;
    if (sSentPending.emplace(drawable, e).second)
    {
        sSentPendingCount.fetch_add(1, std::memory_order_relaxed);
    }
}

void sentinelRegister(const void* drawable)
{
    if (drawable == nullptr || sSentPendingCount.load(std::memory_order_relaxed) == 0 || !verboseEnabled())
    {
        return;
    }
    std::lock_guard<std::mutex> lock(sSentMutex);
    auto it = sSentPending.find(drawable);
    if (it == sSentPending.end())
    {
        return;
    }
    U64 gap = sFrame.load(std::memory_order_relaxed) - it->second.frame;
    sGapWin[gap > 3 ? 3 : gap].fetch_add(1, std::memory_order_relaxed);
    sSentPending.erase(it);
    sSentPendingCount.fetch_sub(1, std::memory_order_relaxed);
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

    {
        const U32 wid = watchObjId();
        if (wid != 0 && tCurDrawInfo != nullptr && sObjIdFn != nullptr
            && sObjIdFn(tCurDrawInfo) == wid && verboseEnabled())
        {
            LL_WARNS("VKContract") << "VKC-OBJ skip cause=" << CAUSE_NAMES[c]
                                   << " shader='" << shader_name << "'"
                                   << provenance() << LL_ENDL;
        }
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
    {
        const U32 wid = watchObjId();
        if (wid != 0 && tCurDrawInfo != nullptr && sObjIdFn != nullptr
            && sObjIdFn(tCurDrawInfo) == wid)
        {
            sWatchFrameFired.fetch_add(1, std::memory_order_relaxed);
            if (sPassBucketFn == nullptr || sPassBucketFn() == 0)
            {
                sWatchFrameCam.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }
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

    {
        const U32 wid = watchObjId();
        if (wid != 0)
        {
            const U64 cam = sWatchFrameCam.exchange(0, std::memory_order_relaxed);
            const U64 tot = sWatchFrameFired.exchange(0, std::memory_order_relaxed);
            ++sWatchFrames;
            sWatchTot += tot;
            if (cam == 0)
            {
                ++sWatchZeroCam;
            }
            if (cam < sWatchCamMin)
            {
                sWatchCamMin = cam;
            }
            if (cam > sWatchCamMax)
            {
                sWatchCamMax = cam;
            }
            if (verboseEnabled())
            {
                if (cam == 0 && sWatchPrevCam > 0)
                {
                    LL_WARNS("VKContract") << "VKC-OBJ camera fire dropped obj=" << wid
                                           << " frame=" << sFrame.load(std::memory_order_relaxed)
                                           << " prev=" << sWatchPrevCam << LL_ENDL;
                }
                else if (cam > 0 && sWatchPrevCam == 0 && sWatchFrames > 1)
                {
                    LL_WARNS("VKContract") << "VKC-OBJ camera fire resumed obj=" << wid
                                           << " frame=" << sFrame.load(std::memory_order_relaxed)
                                           << " count=" << cam << LL_ENDL;
                }
            }
            sWatchPrevCam = cam;
        }
    }

    if (sSentPendingCount.load(std::memory_order_relaxed) != 0)
    {
        U64 frame = sFrame.load(std::memory_order_relaxed);
        std::lock_guard<std::mutex> lock(sSentMutex);
        for (auto it = sSentPending.begin(); it != sSentPending.end(); )
        {
            SentEntry& e = it->second;
            U64 age = frame - e.frame;
            if (age >= 1 && e.stage == 0)
            {
                e.stage = 1;
                if (!e.eligible)
                {
                    sCauseWin[C_MAP_EVICT_UNPAIRED_HIDE].fetch_add(1, std::memory_order_relaxed);
                    sCauseTot[C_MAP_EVICT_UNPAIRED_HIDE].fetch_add(1, std::memory_order_relaxed);
                    it = sSentPending.erase(it);
                    sSentPendingCount.fetch_sub(1, std::memory_order_relaxed);
                    continue;
                }
                sCauseWin[C_MAP_EVICT_UNPAIRED].fetch_add(1, std::memory_order_relaxed);
                sCauseTot[C_MAP_EVICT_UNPAIRED].fetch_add(1, std::memory_order_relaxed);
                sSiteWin[e.site].fetch_add(1, std::memory_order_relaxed);
                U64 sn = 0;
                if (perShaderEscalate(C_MAP_EVICT_UNPAIRED, SITE_NAMES[e.site], sn))
                {
                    LL_WARNS("VKContract") << "VKC map_evict_unpaired site=" << SITE_NAMES[e.site]
                                           << " obj=" << e.objId
                                           << " recs=" << e.records
                                           << " sn=" << sn << LL_ENDL;
                }
            }
            if (age >= 4 && e.stage == 1)
            {
                sCauseWin[C_MAP_EVICT_LONG].fetch_add(1, std::memory_order_relaxed);
                sCauseTot[C_MAP_EVICT_LONG].fetch_add(1, std::memory_order_relaxed);
                U64 sn = 0;
                if (perShaderEscalate(C_MAP_EVICT_LONG, SITE_NAMES[e.site], sn))
                {
                    LL_WARNS("VKContract") << "VKC map_evict_long site=" << SITE_NAMES[e.site]
                                           << " obj=" << e.objId
                                           << " recs=" << e.records
                                           << " sn=" << sn << LL_ENDL;
                }
                it = sSentPending.erase(it);
                sSentPendingCount.fetch_sub(1, std::memory_order_relaxed);
                continue;
            }
            ++it;
        }
    }

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
    if (any == 0 && skips == 0 && vfy_any == 0 && watchObjId() == 0)
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

    U64 site_win[SITE_COUNT];
    U64 site_any = 0;
    for (U32 i = 0; i < SITE_COUNT; ++i)
    {
        site_win[i] = sSiteWin[i].exchange(0, std::memory_order_relaxed);
        site_any += site_win[i];
    }
    U64 gap_win[4];
    U64 gap_any = 0;
    for (U32 i = 0; i < 4; ++i)
    {
        gap_win[i] = sGapWin[i].exchange(0, std::memory_order_relaxed);
        gap_any += gap_win[i];
    }
    if (site_any != 0)
    {
        os << " evict{";
        bool sfirst = true;
        for (U32 i = 0; i < SITE_COUNT; ++i)
        {
            if (site_win[i] == 0)
            {
                continue;
            }
            if (!sfirst)
            {
                os << ' ';
            }
            os << SITE_NAMES[i] << '=' << site_win[i];
            sfirst = false;
        }
        os << '}';
    }
    if (gap_any != 0)
    {
        os << " gap{0=" << gap_win[0] << " 1=" << gap_win[1]
           << " 2=" << gap_win[2] << " 3+=" << gap_win[3] << '}';
    }

    if (vfy_any != 0)
    {
        os << " vfy{bind=" << vfy_win[VFY_BIND]
           << " mv=" << vfy_win[VFY_MV]
           << " dd=" << vfy_win[VFY_DD] << '}';
    }

    {
        const U32 wid = watchObjId();
        if (wid != 0 && sWatchFrames != 0)
        {
            os << " watch{obj=" << wid
               << " frames=" << sWatchFrames
               << " zerocam=" << sWatchZeroCam
               << " cam=" << (sWatchCamMin == ~0ull ? 0 : sWatchCamMin) << ".." << sWatchCamMax
               << " tot=" << sWatchTot << '}';
            sWatchFrames  = 0;
            sWatchZeroCam = 0;
            sWatchCamMin  = ~0ull;
            sWatchCamMax  = 0;
            sWatchTot     = 0;
        }
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
