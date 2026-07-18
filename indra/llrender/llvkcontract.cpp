#include "linden_common.h"
#include "llvkcontract.h"
#include "llerror.h"
#include "lltimer.h"

#include <atomic>
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
    "flicker"
};

std::string (*sDescribe)(const void*) = nullptr;
U64 (*sKey)(const void*) = nullptr;

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

void causeNamed(ECause c, const std::string& shader_name)
{
    if (c >= CAUSE_COUNT)
    {
        return;
    }
    cause(c);
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
    U64 n = sSkipByCause[c].fetch_add(1, std::memory_order_relaxed) + 1;
    if (pow2(n))
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
                if (pow2(f))
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
        if (pow2(f))
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
    LL_WARNS("VKContract") << os.str() << LL_ENDL;
}

}
