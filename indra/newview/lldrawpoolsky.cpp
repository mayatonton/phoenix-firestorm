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

// <AYAstorm r41> sub-step 3.4-δ-1 (sub-doc 03 §3.1.4、旧 3.2 sky-smoke 由来):
// LLVKLoader::recordPlaceholderPoolDraw が PSO bind + set=0/1 + push constant 64 B identity +
// vkCmdDraw(3, 1, 0, 0) で fullscreen triangle + sky blue (0.4, 0.6, 0.9, 1.0) を投入。
// 他 11 pool は δ-2 で同 helper 呼出を hook body に投入予定。
void LLDrawPoolSky::recordPoolDraws(VkCommandBuffer cmd_buf)
{
    static bool logged_once = false;
    if (!logged_once)
    {
        LL_INFOS("VkRecord") << "Sky pool recordPoolDraws hook fired (one-shot)" << LL_ENDL;
        logged_once = true;
    }
    LLVKLoader::recordPlaceholderPoolDraw(cmd_buf);
}
// </AYAstorm r41>

