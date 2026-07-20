/**
 * @file llreloadqueue.cpp
 * @brief Single owner for deferred runtime shader / GL-buffer reloads
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llreloadqueue.h"

#include "llviewershadermgr.h"
#include "llviewerwindow.h"
#include "llviewerdisplay.h"
#include "llvkloader.h"
#include "pipeline.h"

U32 LLReloadQueue::sPending = 0;

void LLReloadQueue::request(U32 kinds)
{
    sPending |= kinds;
}

void LLReloadQueue::drain()
{
    bool rebuilt = false;

    if (sPending & RK_Shaders)
    {
        sPending &= ~RK_Shaders;
        LLViewerShaderMgr::instance()->setShaders();
        rebuilt = true;
    }

    if (gWindowResized)
    {
        LLPipeline::refreshCachedSettings();
        gPipeline.resizeScreenTexture();
        gResizeScreenTexture = false;
        gWindowResized       = false;
        rebuilt              = true;
    }

    if (sPending & RK_GLBuffers)
    {
        sPending &= ~RK_GLBuffers;
        if (gPipeline.isInit())
        {
            gPipeline.releaseGLBuffers();
            gPipeline.createGLBuffers();
        }
        rebuilt = true;
    }

    gViewerWindow->checkSettings();

    if (gResizeScreenTexture)
    {
        gPipeline.resizeScreenTexture();
        gResizeScreenTexture = false;
        rebuilt              = true;
    }

    if (gResizeShadowTexture)
    {
        gPipeline.resizeShadowTexture();
        rebuilt = true;
    }

    if (rebuilt)
    {
        ++LLVKLoader::gVkReloadEpoch;
    }
}
