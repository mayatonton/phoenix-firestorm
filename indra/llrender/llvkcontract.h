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
    CAUSE_COUNT
};

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
void note(ECause c, const std::string& shader_name);
void drawSkipped(ECause fire_cause, const std::string& shader_name);
void drawFired();
void frameBegin();

}

#endif
