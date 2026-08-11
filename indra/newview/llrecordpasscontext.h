/**
* @file llrecordpasscontext.h
* @brief Record-phase pass context passed by value through the render call graph
*
* $LicenseInfo:firstyear=2026&license=viewerlgpl$
* AYAstorm Viewer Source Code
* Copyright (C) 2025-2026 Ishikawa AYA (github: mayatonton, mayatonton1994@gmail.com)
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
* $/LicenseInfo$
*/

#ifndef LL_LLRECORDPASSCONTEXT_H
#define LL_LLRECORDPASSCONTEXT_H

#include "stdtypes.h"

class LLCullResult;
class LLRenderTargetPack;

struct LLRecordPassContext
{
    bool shadowPass     = false;
    bool reflectionPass = false;
    bool impostorPass   = false;
    bool hudPass        = false;
    bool underWater     = false;
    bool dofPass        = false;
    LLCullResult*        cullResult = nullptr;
    LLRenderTargetPack*  activeRT   = nullptr;
    F32  avatarMinimumAlpha = 0.2f;
};

#endif
