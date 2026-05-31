/**
* @file llpipelineframecontext.h
* @brief AYAstorm r41 LLPipeline frame context — per-frame state aggregation (sub-doc 04)
*
* $LicenseInfo:firstyear=2026&license=viewerlgpl$
* AYAstorm Viewer Source Code
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
* $/LicenseInfo$
*/

#ifndef LL_LLPIPELINEFRAMECONTEXT_H
#define LL_LLPIPELINEFRAMECONTEXT_H

#include "pipeline.h"

class LLCullResult;

// AYAstorm r41 sub-doc 04 §3 LLPipelineFrameContext
// Per-frame state aggregation for Vulkan migration. sub-step 4.1-α scope =
// sCull aggregation + lifecycle 4 関数. Later sub-steps add mRT (4.1-β),
// 10 bool flags (4.2), sCurCameraID accessor + per-pool draw (4.3).
class LLPipelineFrameContext
{
public:
    enum EPassType
    {
        PASS_NONE,
        PASS_SHADOW,
        PASS_DEFERRED,
        PASS_FORWARD,
        PASS_POST
    };

    // Singleton access. sub-step 4.1 uses static accessor pattern; const-ref
    // passing through render path is a later sub-step (4.3+) decision.
    static LLPipelineFrameContext& getInstance();

    // Frame lifecycle (called from LLPipeline frame entry/exit hooks).
    void beginFrameContext();
    void endFrameContext();

    // Render-pass transition (shadow / deferred / forward / post).
    void beginPass(EPassType pass_type);
    void endPass();

    // sub-step 4.1-α: sCull aggregation.
    LLCullResult* getCullResult() const { return mCullResult; }
    void          setCullResult(LLCullResult* cull) { mCullResult = cull; }

    // sub-step 4.1-β: mRT aggregation. mRT は LLPipeline::init() + allocateScreenBufferInternal()
    // 内の transient juggling 経路のみで mutate、frame lifecycle では reset しない (既存挙動 1:1 維持)。
    LLPipeline::RenderTargetPack* getActiveRT() const { return mActiveRT; }
    void                          setActiveRT(LLPipeline::RenderTargetPack* rt) { mActiveRT = rt; }

    // sub-step 4.1-β: RAII helper for transient mRT juggling (replaces the
    // SetTemporarily<RenderTargetPack*> pattern used at llgltfmaterialpreviewmgr.cpp).
    class ScopedActiveRT
    {
    public:
        explicit ScopedActiveRT(LLPipeline::RenderTargetPack* new_rt);
        ~ScopedActiveRT();
        ScopedActiveRT(const ScopedActiveRT&) = delete;
        ScopedActiveRT& operator=(const ScopedActiveRT&) = delete;
    private:
        LLPipeline::RenderTargetPack* mPrevRT;
    };

    EPassType getCurrentPass() const { return mCurrentPass; }

private:
    LLPipelineFrameContext();
    ~LLPipelineFrameContext();
    LLPipelineFrameContext(const LLPipelineFrameContext&) = delete;
    LLPipelineFrameContext& operator=(const LLPipelineFrameContext&) = delete;

    LLCullResult*                 mCullResult;
    LLPipeline::RenderTargetPack* mActiveRT;
    EPassType                     mCurrentPass;
};

#endif // LL_LLPIPELINEFRAMECONTEXT_H
