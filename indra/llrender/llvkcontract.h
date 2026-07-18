#ifndef LL_LLVKCONTRACT_H
#define LL_LLVKCONTRACT_H

#include "stdtypes.h"
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

void drawScopeBegin(const void* draw_info, const char* tag);
void drawScopeEnd();

struct DrawScope
{
    DrawScope(const void* draw_info, const char* tag) { drawScopeBegin(draw_info, tag); }
    ~DrawScope() { drawScopeEnd(); }
};

void resolveBegin();
void cause(ECause c);
void causeIfNone(ECause c);
void causeNamed(ECause c, const std::string& shader_name);
void pokeSite(U32 id);
void pokeClear();
void note(ECause c, const std::string& shader_name);
void noteDetail(ECause c, const char* key, const std::string& detail);
void drawSkipped(ECause fire_cause, const std::string& shader_name);
void drawFired();
void frameBegin();

void sentinelEvict(U32 site, const void* drawable, U32 obj_local_id, U32 record_count, bool drawable_dead, bool eligible);
void sentinelRegister(const void* drawable);

}

#endif
