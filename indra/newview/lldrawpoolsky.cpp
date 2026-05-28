/**
 * @file lldrawpoolsky.cpp
 * @brief LLDrawPoolSky class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "lldrawpoolsky.h"

#include "llvkloader.h" // <AYAstorm r41> sub-step 3.2 sky pool smoke-test draw

// DEPRECATED

LLDrawPoolSky::LLDrawPoolSky()
:   LLFacePool(POOL_SKY),
    mSkyTex(NULL),
    mShader(NULL)
{
}

void LLDrawPoolSky::prerender()
{
}

void LLDrawPoolSky::render(S32 pass)
{

}

void LLDrawPoolSky::renderSkyFace(U8 index)
{

}

void LLDrawPoolSky::endRenderPass( S32 pass )
{
}

// <AYAstorm r41> sub-step 3.2 (refine 2026-05-29): sky pool 1 draw smoke-test。
// LLVKLoader::recordSkySmokeDraw が fullscreen triangle で sky blue (0.4, 0.6, 0.9, 1.0)
// を出力、sub-doc 03 §3.1 sub-step 3.2 完遂 marker (Vulkan 経由 vkCmdDraw 投入 + validation 0 件)。
void LLDrawPoolSky::recordPoolDraws(VkCommandBuffer cmd_buf)
{
    static bool logged_once = false;
    if (!logged_once)
    {
        LL_INFOS("VkRecord") << "Sky pool recordPoolDraws hook fired (one-shot)" << LL_ENDL;
        logged_once = true;
    }
    LLVKLoader::recordSkySmokeDraw(cmd_buf);
}
// </AYAstorm r41>

