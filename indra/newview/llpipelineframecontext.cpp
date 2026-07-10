/**
* @file llpipelineframecontext.cpp
* @brief AYAstorm r41 LLPipeline frame context implementation
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

#include "llviewerprecompiledheaders.h"

#include "llpipelineframecontext.h"

LLPipelineFrameContext& LLPipelineFrameContext::getInstance()
{
    static LLPipelineFrameContext sInstance;
    return sInstance;
}

LLPipelineFrameContext::LLPipelineFrameContext()
    : mCullResult(nullptr)
    , mActiveRT(nullptr)
    , mShadowPass(false)
    , mReflectionPass(false)
    , mImpostorPass(false)
    , mHUDPass(false)
    , mDoFPass(false)
    , mRenderingGlow(false)
    , mRenderingDeferred(false)
    , mUnderWaterRendering(false)
    , mReflectionProbesEnabled(false)
{
}

LLPipelineFrameContext::~LLPipelineFrameContext()
{
}

LLPipelineFrameContext::ScopedActiveRT::ScopedActiveRT(LLPipeline::RenderTargetPack* new_rt)
    : mPrevRT(LLPipelineFrameContext::getInstance().getActiveRT())
{
    LLPipelineFrameContext::getInstance().setActiveRT(new_rt);
}

LLPipelineFrameContext::ScopedActiveRT::~ScopedActiveRT()
{
    LLPipelineFrameContext::getInstance().setActiveRT(mPrevRT);
}

LLPipelineFrameContext::ScopedRenderingGlow::ScopedRenderingGlow(bool new_val)
    : mPrev(LLPipelineFrameContext::getInstance().isRenderingGlow())
{
    LLPipelineFrameContext::getInstance().setRenderingGlow(new_val);
}

LLPipelineFrameContext::ScopedRenderingGlow::~ScopedRenderingGlow()
{
    LLPipelineFrameContext::getInstance().setRenderingGlow(mPrev);
}

LLPipelineFrameContext::ScopedCameraID::ScopedCameraID(LLViewerCamera::eCameraID new_id)
    : mPrev(LLPipelineFrameContext::getInstance().getCurCameraID())
{
    LLPipelineFrameContext::getInstance().setCurCameraID(new_id);
}

LLPipelineFrameContext::ScopedCameraID::~ScopedCameraID()
{
    LLPipelineFrameContext::getInstance().setCurCameraID(mPrev);
}
