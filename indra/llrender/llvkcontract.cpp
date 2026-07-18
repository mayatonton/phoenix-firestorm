#include "linden_common.h"
#include "llvkcontract.h"
#include "llerror.h"
#include "lltimer.h"

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <set>
#include <sstream>
#include <unordered_map>

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
    "stale_unrefreshed"
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

struct StaleEntry
{
    U32 objId = 0;
    const char* kind = "";
    U64 frame = 0;
    U8 stage = 0;
};

std::mutex sStaleMutex;
std::unordered_map<const void*, StaleEntry> sStalePending;
std::atomic<U64> sStalePendingCount{0};
std::atomic<U64> sStaleGapWin[4] = {};

std::string (*sDescribe)(const void*) = nullptr;
U64 (*sKey)(const void*) = nullptr;

thread_local const void* tCurDrawInfo = nullptr;
thread_local const char* tCurTag      = nullptr;
thread_local ECause      tLastCause   = C_UNKNOWN;
thread_local U32         tLastPokeSite = 0;

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

void setResolvers(std::string (*describe)(const void*), U64 (*key)(const void*))
{
    sDescribe = describe;
    sKey      = key;
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

void stalePend(const void* key, U32 obj_local_id, const char* kind)
{
    if (key == nullptr || !verboseEnabled())
    {
        return;
    }
    std::lock_guard<std::mutex> lock(sStaleMutex);
    StaleEntry e;
    e.objId = obj_local_id;
    e.kind = kind != nullptr ? kind : "";
    e.frame = sFrame.load(std::memory_order_relaxed);
    if (sStalePending.emplace(key, e).second)
    {
        sStalePendingCount.fetch_add(1, std::memory_order_relaxed);
    }
}

void staleResolve(const void* key)
{
    if (key == nullptr || sStalePendingCount.load(std::memory_order_relaxed) == 0 || !verboseEnabled())
    {
        return;
    }
    std::lock_guard<std::mutex> lock(sStaleMutex);
    auto it = sStalePending.find(key);
    if (it == sStalePending.end())
    {
        return;
    }
    U64 age = sFrame.load(std::memory_order_relaxed) - it->second.frame;
    U32 bin = age < 2 ? 0 : (age < 8 ? 1 : (age < 32 ? 2 : 3));
    sStaleGapWin[bin].fetch_add(1, std::memory_order_relaxed);
    sStalePending.erase(it);
    sStalePendingCount.fetch_sub(1, std::memory_order_relaxed);
}

void staleCancel(const void* key)
{
    if (key == nullptr || sStalePendingCount.load(std::memory_order_relaxed) == 0)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(sStaleMutex);
    if (sStalePending.erase(key) > 0)
    {
        sStalePendingCount.fetch_sub(1, std::memory_order_relaxed);
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
                               << " poke=" << tLastPokeSite
                               << " sn=" << sn << provenance() << LL_ENDL;
    }
}

void pokeSite(U32 id)
{
    tLastPokeSite = id;
}

void pokeClear()
{
    tLastPokeSite = 0;
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

    if (sStalePendingCount.load(std::memory_order_relaxed) != 0)
    {
        U64 frame = sFrame.load(std::memory_order_relaxed);
        std::lock_guard<std::mutex> lock(sStaleMutex);
        for (auto it = sStalePending.begin(); it != sStalePending.end(); )
        {
            StaleEntry& e = it->second;
            U64 age = frame - e.frame;
            if (age >= 10 && e.stage == 0)
            {
                e.stage = 1;
                sCauseWin[C_STALE_UNREFRESHED].fetch_add(1, std::memory_order_relaxed);
                sCauseTot[C_STALE_UNREFRESHED].fetch_add(1, std::memory_order_relaxed);
                U64 sn = 0;
                if (perShaderEscalate(C_STALE_UNREFRESHED, e.kind, sn))
                {
                    LL_WARNS("VKContract") << "VKC stale_unrefreshed kind=" << e.kind
                                           << " obj=" << e.objId
                                           << " age=" << age
                                           << " sn=" << sn << LL_ENDL;
                }
            }
            if (age >= 120 && e.stage == 1)
            {
                U64 sn = 0;
                if (perShaderEscalate(C_STALE_UNREFRESHED, "never", sn))
                {
                    LL_WARNS("VKContract") << "VKC stale_unrefreshed kind=" << e.kind
                                           << " obj=" << e.objId
                                           << " age=" << age << " dropped=1"
                                           << " sn=" << sn << LL_ENDL;
                }
                it = sStalePending.erase(it);
                sStalePendingCount.fetch_sub(1, std::memory_order_relaxed);
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
    if (any == 0 && skips == 0)
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

    U64 stale_win[4];
    U64 stale_any = 0;
    for (U32 i = 0; i < 4; ++i)
    {
        stale_win[i] = sStaleGapWin[i].exchange(0, std::memory_order_relaxed);
        stale_any += stale_win[i];
    }
    if (stale_any != 0)
    {
        os << " restage{<2=" << stale_win[0] << " <8=" << stale_win[1]
           << " <32=" << stale_win[2] << " 32+=" << stale_win[3] << '}';
    }

    LL_WARNS("VKContract") << os.str() << LL_ENDL;
}

}
