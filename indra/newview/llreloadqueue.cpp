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

static constexpr U32 kRetryCooldownFrames = 30;

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
        gResizeScreenTexture = true;
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

    const U32 frame = LLVKLoader::getMonotonicFrameCount();

    static U32 s_main_last_fail   = 0;
    static U32 s_shadow_last_fail = 0;
    static U32 s_probe_last_fail  = 0;
    static int s_main_state       = -1;
    static int s_shadow_state     = -1;
    static int s_probe_state      = -1;

    {
        bool need     = gResizeScreenTexture || (gPipeline.shadersLoaded() && !gPipeline.mainChainComplete());
        bool cooldown = (s_main_last_fail == 0) || (frame - s_main_last_fail >= kRetryCooldownFrames);
        if (need && cooldown)
        {
            bool ok = gPipeline.resizeScreenTexture();
            gResizeScreenTexture = !ok;
            if (!ok)
            {
                s_main_last_fail = llmax(frame, 1u);
            }
            rebuilt = true;
        }
        int now = gPipeline.mainChainComplete() ? 1 : 0;
        if (s_main_state == 1 && now == 0)
        {
            LL_WARNS("Pipeline") << "render chain: main died (unrenderable)" << LL_ENDL;
        }
        else if (s_main_state == 0 && now == 1)
        {
            LL_INFOS("Pipeline") << "render chain: main recovered" << LL_ENDL;
        }
        s_main_state = now;
    }

    {
        bool shadow_pack_bad = (LLPipeline::RenderShadowDetail > 0) && !gPipeline.getSunShadowTarget(0)->isComplete();
        bool need            = gResizeShadowTexture || shadow_pack_bad;
        bool cooldown        = (s_shadow_last_fail == 0) || (frame - s_shadow_last_fail >= kRetryCooldownFrames);
        if (need && gPipeline.mainChainComplete() && cooldown)
        {
            bool ok = gPipeline.resizeShadowTexture();
            gResizeShadowTexture = !ok;
            if (!ok)
            {
                s_shadow_last_fail = llmax(frame, 1u);
            }
            rebuilt = true;
        }
        int now = ((LLPipeline::RenderShadowDetail <= 0) || gPipeline.getSunShadowTarget(0)->isComplete()) ? 1 : 0;
        if (s_shadow_state == 1 && now == 0)
        {
            LL_WARNS("Pipeline") << "render chain: shadow died" << LL_ENDL;
        }
        else if (s_shadow_state == 0 && now == 1)
        {
            LL_INFOS("Pipeline") << "render chain: shadow recovered" << LL_ENDL;
        }
        s_shadow_state = now;
    }

    {
        bool probe_bad = !gPipeline.probeChainComplete() || (LLPipeline::RenderMirrors && !gPipeline.heroChainComplete());
        bool cooldown  = (s_probe_last_fail == 0) || (frame - s_probe_last_fail >= kRetryCooldownFrames);
        if (gPipeline.shadersLoaded() && gPipeline.mainChainComplete() && probe_bad && cooldown)
        {
            bool ok = gPipeline.allocateProbeChains();
            if (!ok)
            {
                s_probe_last_fail = llmax(frame, 1u);
            }
            rebuilt = true;
        }
        int now = (gPipeline.probeChainComplete() && (!LLPipeline::RenderMirrors || gPipeline.heroChainComplete())) ? 1 : 0;
        if (s_probe_state == 1 && now == 0)
        {
            LL_WARNS("Pipeline") << "render chain: probe died" << LL_ENDL;
        }
        else if (s_probe_state == 0 && now == 1)
        {
            LL_INFOS("Pipeline") << "render chain: probe recovered" << LL_ENDL;
        }
        s_probe_state = now;
    }

    if (rebuilt)
    {
        ++LLVKLoader::gVkReloadEpoch;
    }
}
