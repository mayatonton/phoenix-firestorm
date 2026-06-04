/**
 * @file lldrawpool.cpp
 * @brief LLDrawPoolMaterials class implementation
 * @author Jonathan "Geenz" Goodman
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

#include "lldrawpoolwaterexclusion.h"
#include "llvkloader.h" // <AYAstorm r41> sub-step 3.4-δ-2 placeholder pool draw helper
#include "llviewershadermgr.h"
#include "pipeline.h"
#include "llglcommonfunc.h"
#include "llvoavatar.h"
#include "lldrawpoolwater.h"

LLDrawPoolWaterExclusion::LLDrawPoolWaterExclusion() : LLRenderPass(LLDrawPool::POOL_WATEREXCLUSION)
{
    LL_INFOS("DPInvisible") << "Creating water exclusion draw pool" << LL_ENDL;
}


void LLDrawPoolWaterExclusion::render(S32 pass)
{                                             // render invisiprims
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; // LL_RECORD_BLOCK_TIME(FTM_RENDER_INVISIBLE);
    // <AYAstorm r41 PC-6ε-3> per-draw cadence flush (design 06b §4.1、AYA (B') 採用 2026-06-04 = 1 pool 1 site)
    LLVKLoader::flushDrawUbos();
    // </AYAstorm r41 PC-6ε-3>

    if (gPipeline.shadersLoaded())
    {
        gDrawColorProgram.bind();
    }


    LLGLDepthTest depth(GL_TRUE);
    gDrawColorProgram.uniform4f(LLShaderMgr::DIFFUSE_COLOR, 1, 1, 1, 1);

    LLDrawPoolWater* pwaterpool = (LLDrawPoolWater*)gPipeline.getPool(LLDrawPool::POOL_WATER);
    if (pwaterpool)
    {
        // Just treat our water planes as double sided for the purposes of generating the exclusion mask.
        LLGLDisable cullface(GL_CULL_FACE);
        pwaterpool->pushWaterPlanes(0);

        // Take care of the edge water tiles.
        pwaterpool->pushWaterPlanes(1);
    }

    gDrawColorProgram.uniform4f(LLShaderMgr::DIFFUSE_COLOR, 0, 0, 0, 1);

    static LLStaticHashedString waterSign("waterSign");
    gDrawColorProgram.uniform1f(waterSign, 1.f);

    pushBatches(LLRenderPass::PASS_INVISIBLE, false, false);


    if (gPipeline.shadersLoaded())
    {
        gDrawColorProgram.unbind();
    }
}

// <AYAstorm r41> sub-step 2.1b: empty Vulkan record hook. Stage 3 replaces the
// marker with PSO bind + vkCmdDraw* against cmd_buf.
void LLDrawPoolWaterExclusion::recordPoolDraws(VkCommandBuffer cmd_buf)
{
    static bool logged_once = false;
    if (!logged_once)
    {
        LL_INFOS("VkRecord") << "WaterExclusion pool recordPoolDraws hook fired (one-shot)" << LL_ENDL;
        logged_once = true;
    }
    LLVKLoader::recordPlaceholderPoolDraw(cmd_buf);
}
// </AYAstorm r41>
