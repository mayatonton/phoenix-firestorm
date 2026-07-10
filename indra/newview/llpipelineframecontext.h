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

class LLPipelineFrameContext
{
public:
    static LLPipelineFrameContext& getInstance();

    LLCullResult* getCullResult() const { return mCullResult; }
    void          setCullResult(LLCullResult* cull) { mCullResult = cull; }

    LLPipeline::RenderTargetPack* getActiveRT() const { return mActiveRT; }
    void                          setActiveRT(LLPipeline::RenderTargetPack* rt) { mActiveRT = rt; }

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

    bool isRenderingGlow()              const { return mRenderingGlow; }
    void setRenderingGlow(bool b)             { mRenderingGlow = b; }
    bool isRenderingDeferred()          const { return mRenderingDeferred; }
    void setRenderingDeferred(bool b)         { mRenderingDeferred = b; }
    bool isUnderWaterRendering()        const { return mUnderWaterRendering; }
    void setUnderWaterRendering(bool b)       { mUnderWaterRendering = b; }
    bool isReflectionProbesEnabled()    const { return mReflectionProbesEnabled; }
    void setReflectionProbesEnabled(bool b)   { mReflectionProbesEnabled = b; }

    LLViewerCamera::eCameraID getCurCameraID() const  { return LLViewerCamera::getCurCameraID(); }
    void                      setCurCameraID(LLViewerCamera::eCameraID id) { LLViewerCamera::setCurCameraID(id); }

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

private:
    LLPipelineFrameContext();
    ~LLPipelineFrameContext();
    LLPipelineFrameContext(const LLPipelineFrameContext&) = delete;
    LLPipelineFrameContext& operator=(const LLPipelineFrameContext&) = delete;

    LLCullResult*                 mCullResult;
    LLPipeline::RenderTargetPack* mActiveRT;

    bool mShadowPass;
    bool mReflectionPass;
    bool mImpostorPass;
    bool mHUDPass;
    bool mDoFPass;

    bool mRenderingGlow;
    bool mRenderingDeferred;
    bool mUnderWaterRendering;
    bool mReflectionProbesEnabled;
};

#endif // LL_LLPIPELINEFRAMECONTEXT_H
