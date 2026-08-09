#ifndef LL_LLVKCONTRACT_H
#define LL_LLVKCONTRACT_H

#include "stdtypes.h"
#include <atomic>
#include <string>

namespace LLVKContract
{

enum ECause : U32
{
    C_UNKNOWN = 0,
    C_VK_NOT_INIT,
    C_RECORD_JOB_PULL,
    C_NO_SHADER_OR_LAYOUT,
    C_NO_SAMPLER,
    C_UBO_COLLECT_OVERFLOW,
    C_ENSURE_SET_FAIL,
    C_REFRESH_NO_SHADER,
    C_REFRESH_PERPROGRAM_UBO,
    C_REFRESH_SHARED_UBO,
    C_AUTHORED_EMPTY,
    C_CMD_NULL,
    C_PIPELINE_NULL,
    C_FB_VIEW_DIFFUSE,
    C_FB_VIEW_AUX,
    C_FB_HEAP_DEFAULT,
    C_FLICKER,
    C_GEOAB_INPUT_DRIFT,
    C_GEOAB_KERNEL_MISMATCH,
    C_GEOAB_SRC_DRIFT,
    C_GEOAB_REF_FAIL,
    C_GEOAB_WORKER_SNAPSHOT,
    C_GEOAB_STAGE_DEGEN,
    C_GEOAB_STAGE_SKIP,
    C_UBO_SLICE_FAIL,
    C_PP_FALLBACK_LOSSY,
    C_UBO_OFFSET_STALE,
    C_UBO_CONTENT_STALE,
    C_MEMO_CROSS_CMD,
    C_DRAWDATA_SCRATCH_WRAP,
    C_DRAWDATA_EXHAUSTED,
    C_DRAWDATA_RACE,
    C_MEGA_RACE,
    C_ALLOC_NONCOHERENT,
    C_MV_STALE_VALUE,
    C_DRAWDATA_ID_MISMATCH,
    C_SIG_DIET_MISMATCH,
    C_PAR_MAIN_ONLY_WRITE,
    C_PAR_WORKER_FORBIDDEN,
    C_PAR_CONCURRENT,
    C_PAR_DEAD_ACCESS,
    C_SKIN_DRAW_NO_COMMIT,
    C_PASS_SCOPE_FAIL,
    C_PASS_REFUSED,
    C_MDI_STALE,
    C_MDI_OVERWRITE,
    C_MDI_HEAP_IDENTITY,
    C_MDI_RUNINV,
    C_MDI_GEOM,
    CAUSE_COUNT
};

bool verboseEnabled();

void setResolvers(std::string (*describe)(const void*), U64 (*key)(const void*));

void drawScopeBegin(const void* draw_info, const char* tag);
void drawScopeEnd();
const void* currentDrawInfo();
const char* currentDrawTag();

struct DrawScope
{
    DrawScope(const void* draw_info, const char* tag) { drawScopeBegin(draw_info, tag); }
    ~DrawScope() { drawScopeEnd(); }
};

void resolveBegin();
void cause(ECause c);
void causeIfNone(ECause c);
void causeNamed(ECause c, const std::string& shader_name);
void note(ECause c, const std::string& shader_name);
void noteDetail(ECause c, const char* key, const std::string& detail);
void noteFbSlot(const void* shader_key, const std::string& shader_name, U32 binding, const char* reason);
void noteFbNoView(const void* shader_key, const std::string& shader_name, U32 binding);
void drawSkipped(ECause fire_cause, const std::string& shader_name);
void drawFired();
void frameBegin();
void noteCorrectiveAction(const char* site, U64 state_fingerprint);

enum EVfy : U32
{
    VFY_BIND = 0,
    VFY_MV   = 1,
    VFY_DD   = 2,
    VFY_COUNT = 3
};
void vfyTick(U32 which);
void stashDrawDataID(U32 id);
void checkDrawDataIDAtFire(U32 actual);
void markPerDrawIDCommitted();
void checkPerDrawIDFreshnessAtFire(bool fired, bool uses_skin_set, const char* shader_name);

// MDI per-draw 供給検証器（恒久・docs/vknative_mdi_supply_verifier.md）
// α = DrawData 供給の stale/上書き（mdiAuthor/mdiReference）
// β = heap slot 実体の取り違え（呼び手が heap 照合し causeNamed(C_MDI_HEAP_IDENTITY)）
void mdiInit(U32 total_slots);
void mdiAuthor(U32 id, U64 content_hash, const void* src);
void mdiReference(U32 id, const void* src);

// slot[0..12]（GPU が実読する DrawData の全フィールド）の 64bit 指紋
inline U64 mdiHash(const U32* slots)
{
    U64 h = 1469598103934665603ULL;
    for (int i = 0; i < 13; ++i) { h ^= (U64)slots[i]; h *= 1099511628211ULL; }
    return h;
}

void parallelEpochBegin();
void parallelEpochEnd();
bool parallelEpochActive();
void markWorkerThread(bool is_worker);
bool isWorkerThread();
U32  threadTag();
U64  causeTotal(ECause c);
void runParallelSelfTest();

struct MainOnlyGuard
{
    MainOnlyGuard()
    {
        if (parallelEpochActive() && isWorkerThread())
        {
            cause(C_PAR_MAIN_ONLY_WRITE);
        }
    }
};

struct WorkerForbiddenGuard
{
    WorkerForbiddenGuard()
    {
        if (isWorkerThread())
        {
            cause(C_PAR_WORKER_FORBIDDEN);
        }
    }
};

struct ConcurrentEntryGuard
{
    std::atomic<U32>& mOwner;
    bool              mOwned;
    ConcurrentEntryGuard(std::atomic<U32>& owner)
        : mOwner(owner)
    {
        U32 expected = 0;
        mOwned = owner.compare_exchange_strong(expected, threadTag(), std::memory_order_acquire);
        if (!mOwned)
        {
            cause(C_PAR_CONCURRENT);
        }
    }
    ~ConcurrentEntryGuard()
    {
        if (mOwned)
        {
            mOwner.store(0, std::memory_order_release);
        }
    }
};

struct DeadObjectGuard
{
    DeadObjectGuard(bool is_dead)
    {
        if (isWorkerThread() && is_dead)
        {
            cause(C_PAR_DEAD_ACCESS);
        }
    }
};

}

#endif
