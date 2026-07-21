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
    C_MAP_EVICT_UNPAIRED,
    C_MAP_EVICT_LONG,
    C_MAP_EVICT_UNPAIRED_HIDE,
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
    C_LIST_DROP_INFRUSTUM,
    C_LIST_OCCL_DROP,
    C_LIST_RESUME,
    C_LIST_ABSENT_LONG,
    C_UUID_ABSENT,
    C_UUID_FB_DIFFUSE,
    C_UUID_FB_AUX,
    C_SIG_DIET_MISMATCH,
    C_PAR_MAIN_ONLY_WRITE,
    C_PAR_WORKER_FORBIDDEN,
    C_PAR_CONCURRENT,
    C_PAR_DEAD_ACCESS,
    CAUSE_COUNT
};

enum ESentinelSite : U32
{
    SITE_NONE = 0,
    SITE_STRIP_DESTROY,
    SITE_STRIP_CLEANUP,
    SITE_STRIP_DELETE_FACES,
    SITE_CLEAR_GROUP_DTOR,
    SITE_CLEAR_REBUILD_GENERIC,
    SITE_CLEAR_LAST_ELEMENT,
    SITE_CLEAR_ZOMBIE,
    SITE_CLEAR_DESTROY_GL,
    SITE_CLEAR_APPLY,
    SITE_COUNT
};

bool verboseEnabled();

void setResolvers(std::string (*describe)(const void*), U64 (*key)(const void*));
void setObjIdResolver(U32 (*fn)(const void*));
void setPassBucketResolver(U32 (*fn)());
void watchPickCandidate(U32 localid);
bool watchPickModeEnabled();
void watchAddLocal(U32 localid);
void watchFbProbe(bool diffuse, const char* reason);
void watchStageEvent(U32 localid, const char* what, U32 n = 0);
bool watchLastStage(U32 localid, const char*& what, U32& n, U64& age);
bool watchLastEvict(U32 localid, U32& site, U32& records, U64& age);
const char* sentinelSiteName(U32 site);

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
void drawSkipped(ECause fire_cause, const std::string& shader_name);
void drawFired();
void frameBegin();

void sentinelEvict(U32 site, const void* drawable, U32 obj_local_id, U32 record_count, bool drawable_dead, bool eligible);
void sentinelRegister(const void* drawable);

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
