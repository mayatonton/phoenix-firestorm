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
#include "llviewercamera.h"

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

    // sub-step 4.2 (a): 5 個 pass-specific bool flags. 入れ子発火可 (generateImpostor() 内
    // shadow / reflection / impostor 多重)、5 個独立 field + ScopedXxxPass RAII helper
    // (下方 nested class 群、4.1-β ScopedActiveRT 範式継承)。
    bool isShadowPass()                 const { return mShadowPass; }
    void setShadowPass(bool b)                { mShadowPass = b; }
    bool isReflectionPass()             const { return mReflectionPass; }
    void setReflectionPass(bool b)            { mReflectionPass = b; }
    bool isImpostorPass()               const { return mImpostorPass; }
    void setImpostorPass(bool b)              { mImpostorPass = b; }
    bool isHUDPass()                    const { return mHUDPass; }
    void setHUDPass(bool b)                   { mHUDPass = b; }
    bool isDoFPass()                    const { return mDoFPass; }
    void setDoFPass(bool b)                   { mDoFPass = b; }

    // sub-step 4.2 (b): 4 個 frame-global bool flags. frame entry で settle、
    // frame 終端まで persist (4.1-α/β 範式継承 = field + getter/setter のみ)。
    bool isRenderingGlow()              const { return mRenderingGlow; }
    void setRenderingGlow(bool b)             { mRenderingGlow = b; }
    bool isRenderingDeferred()          const { return mRenderingDeferred; }
    void setRenderingDeferred(bool b)         { mRenderingDeferred = b; }
    bool isUnderWaterRendering()        const { return mUnderWaterRendering; }
    void setUnderWaterRendering(bool b)       { mUnderWaterRendering = b; }
    bool isReflectionProbesEnabled()    const { return mReflectionProbesEnabled; }
    void setReflectionProbesEnabled(bool b)   { mReflectionProbesEnabled = b; }

    // sub-step 4.3-α: sCurCameraID accessor 配線 (LLViewerCamera::sCurCameraID への forward call)。
    // cross-class transient 状態、4.3-β 以降 per-pool draw で active camera 配線。
    LLViewerCamera::eCameraID getCurCameraID() const  { return LLViewerCamera::getCurCameraID(); }
    void                      setCurCameraID(LLViewerCamera::eCameraID id) { LLViewerCamera::setCurCameraID(id); }

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

    // sub-step 4.2 (a) ScopedXxxPass RAII helpers (4.1-β ScopedActiveRT 範式継承)
    class ScopedShadowPass
    {
    public:
        explicit ScopedShadowPass(bool new_val);
        ~ScopedShadowPass();
        ScopedShadowPass(const ScopedShadowPass&) = delete;
        ScopedShadowPass& operator=(const ScopedShadowPass&) = delete;
    private:
        bool mPrev;
    };

    class ScopedReflectionPass
    {
    public:
        explicit ScopedReflectionPass(bool new_val);
        ~ScopedReflectionPass();
        ScopedReflectionPass(const ScopedReflectionPass&) = delete;
        ScopedReflectionPass& operator=(const ScopedReflectionPass&) = delete;
    private:
        bool mPrev;
    };

    class ScopedImpostorPass
    {
    public:
        explicit ScopedImpostorPass(bool new_val);
        ~ScopedImpostorPass();
        ScopedImpostorPass(const ScopedImpostorPass&) = delete;
        ScopedImpostorPass& operator=(const ScopedImpostorPass&) = delete;
    private:
        bool mPrev;
    };

    class ScopedHUDPass
    {
    public:
        explicit ScopedHUDPass(bool new_val);
        ~ScopedHUDPass();
        ScopedHUDPass(const ScopedHUDPass&) = delete;
        ScopedHUDPass& operator=(const ScopedHUDPass&) = delete;
    private:
        bool mPrev;
    };

    class ScopedDoFPass
    {
    public:
        explicit ScopedDoFPass(bool new_val);
        ~ScopedDoFPass();
        ScopedDoFPass(const ScopedDoFPass&) = delete;
        ScopedDoFPass& operator=(const ScopedDoFPass&) = delete;
    private:
        bool mPrev;
    };

    // sub-step 4.2 (b): transient frame-global flip helper (llgltfmaterialpreviewmgr
    // preview render path で sRenderGlow を一時 false 化する SetTemporarily<bool>
    // を 1:1 置換、4.1-β ScopedActiveRT / 4.2 (a) ScopedXxxPass 範式継承)。
    class ScopedRenderingGlow
    {
    public:
        explicit ScopedRenderingGlow(bool new_val);
        ~ScopedRenderingGlow();
        ScopedRenderingGlow(const ScopedRenderingGlow&) = delete;
        ScopedRenderingGlow& operator=(const ScopedRenderingGlow&) = delete;
    private:
        bool mPrev;
    };

    // sub-step 4.3-α: sCurCameraID transient swap RAII (llviewerregion.cpp / pipeline.cpp
    // 既存手動 save/restore 2 件を 1:1 置換、4.2 ScopedXxxPass 範式継承)。
    class ScopedCameraID
    {
    public:
        explicit ScopedCameraID(LLViewerCamera::eCameraID new_id);
        ~ScopedCameraID();
        ScopedCameraID(const ScopedCameraID&) = delete;
        ScopedCameraID& operator=(const ScopedCameraID&) = delete;
    private:
        LLViewerCamera::eCameraID mPrev;
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

    // sub-step 4.2 (a): pass-specific 5 件 (入れ子発火可)
    bool mShadowPass;
    bool mReflectionPass;
    bool mImpostorPass;
    bool mHUDPass;
    bool mDoFPass;

    // sub-step 4.2 (b): frame-global 4 件 (settle 後 frame 終端まで persist)
    bool mRenderingGlow;
    bool mRenderingDeferred;
    bool mUnderWaterRendering;
    bool mReflectionProbesEnabled;
};

#endif // LL_LLPIPELINEFRAMECONTEXT_H
