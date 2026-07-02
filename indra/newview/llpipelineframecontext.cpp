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
    , mCurrentPass(PASS_NONE)
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

void LLPipelineFrameContext::beginFrameContext()
{
    mCurrentPass = PASS_NONE;
}

void LLPipelineFrameContext::endFrameContext()
{
    mCullResult  = nullptr;
    mCurrentPass = PASS_NONE;
}

void LLPipelineFrameContext::beginPass(EPassType pass_type)
{
    mCurrentPass = pass_type;
}

void LLPipelineFrameContext::endPass()
{
    mCurrentPass = PASS_NONE;
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

LLPipelineFrameContext::ScopedShadowPass::ScopedShadowPass(bool new_val)
    : mPrev(LLPipelineFrameContext::getInstance().isShadowPass())
{
    LLPipelineFrameContext::getInstance().setShadowPass(new_val);
}

LLPipelineFrameContext::ScopedShadowPass::~ScopedShadowPass()
{
    LLPipelineFrameContext::getInstance().setShadowPass(mPrev);
}

LLPipelineFrameContext::ScopedReflectionPass::ScopedReflectionPass(bool new_val)
    : mPrev(LLPipelineFrameContext::getInstance().isReflectionPass())
{
    LLPipelineFrameContext::getInstance().setReflectionPass(new_val);
}

LLPipelineFrameContext::ScopedReflectionPass::~ScopedReflectionPass()
{
    LLPipelineFrameContext::getInstance().setReflectionPass(mPrev);
}

LLPipelineFrameContext::ScopedImpostorPass::ScopedImpostorPass(bool new_val)
    : mPrev(LLPipelineFrameContext::getInstance().isImpostorPass())
{
    LLPipelineFrameContext::getInstance().setImpostorPass(new_val);
}

LLPipelineFrameContext::ScopedImpostorPass::~ScopedImpostorPass()
{
    LLPipelineFrameContext::getInstance().setImpostorPass(mPrev);
}

LLPipelineFrameContext::ScopedHUDPass::ScopedHUDPass(bool new_val)
    : mPrev(LLPipelineFrameContext::getInstance().isHUDPass())
{
    LLPipelineFrameContext::getInstance().setHUDPass(new_val);
}

LLPipelineFrameContext::ScopedHUDPass::~ScopedHUDPass()
{
    LLPipelineFrameContext::getInstance().setHUDPass(mPrev);
}

LLPipelineFrameContext::ScopedDoFPass::ScopedDoFPass(bool new_val)
    : mPrev(LLPipelineFrameContext::getInstance().isDoFPass())
{
    LLPipelineFrameContext::getInstance().setDoFPass(new_val);
}

LLPipelineFrameContext::ScopedDoFPass::~ScopedDoFPass()
{
    LLPipelineFrameContext::getInstance().setDoFPass(mPrev);
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
