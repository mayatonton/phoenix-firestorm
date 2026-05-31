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
    , mCurrentPass(PASS_NONE)
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
