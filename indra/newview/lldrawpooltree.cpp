/**
 * @file lldrawpooltree.cpp
 * @brief LLDrawPoolTree class implementation
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

#include "lldrawpooltree.h"
#include "llvkloader.h" // <AYAstorm r41> sub-step 3.4-δ-2 placeholder pool draw helper

#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llvotree.h"
#include "pipeline.h"
#include "llviewercamera.h"
#include "llviewershadermgr.h"
#include "llrender.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llenvironment.h"

S32 LLDrawPoolTree::sDiffTex = 0;
static LLGLSLShader* shader = NULL;

LLDrawPoolTree::LLDrawPoolTree(LLViewerTexture *texturep) :
    LLFacePool(POOL_TREE),
    mTexturep(texturep)
{
    mTexturep->setAddressMode(LLTexUnit::TAM_WRAP);
}

//============================================
// deferred implementation
//============================================
void LLDrawPoolTree::beginDeferredPass(S32 pass)
{
    LL_RECORD_BLOCK_TIME(FTM_RENDER_TREES);

    shader = &gDeferredTreeProgram;
    shader->bind();
    shader->setMinimumAlpha(0.5f);
}

void LLDrawPoolTree::renderDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED;
    // <AYAstorm r41 PC-6ε-3> per-draw cadence flush (design 06b §4.1、AYA (B') 採用 2026-06-04 = 1 pool 1 site)
    LLVKLoader::flushDrawUbos();
    // </AYAstorm r41 PC-6ε-3>

    if (mDrawFace.empty())
    {
        return;
    }


// [SL:KB] - Patch: Render-TextureToggle (Catznip-4.0)
    if( (LLPipeline::sRenderTextures) )
        gGL.getTexUnit(sDiffTex)->bindFast( mTexturep );
    else
        gGL.getTexUnit(sDiffTex)->bindFast( LLViewerFetchedTexture::sDefaultDiffuseImagep );
// [/SL:KB]
//    gGL.getTexUnit(sDiffTex)->bindFast(mTexturep);
    mTexturep->addTextureStats(1024.f * 1024.f); // <=== keep Linden tree textures at full res

    for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
        iter != mDrawFace.end(); iter++)
    {
        LLFace* face = *iter;

        // <FS:AYA> [ParcelHide] Skip tree faces whose drawable
        // is in a parcel that should be hidden by the visitor's
        // ParcelHide setting or the parcel-owner [parcelhide:...]
        // tag (legacy [AYAstorm:...] also accepted). Trees live in
        // their own face pool (LLDrawPoolTree, not LLVOVolume's draw
        // map), so the rebuildGeom filter does not catch them.
        // shouldHideForOutsideParcel() applies the keep_avatars /
        // keep_own / HUD rules consistently with volume hiding.
        // renderShadow delegates to renderDeferred so the shadow pass
        // is also covered.
        LLDrawable* drawable = face ? face->getDrawable() : nullptr;
        if (drawable
            && LLPipeline::isParcelHideAlive(drawable))
        {
            continue;
        }
        // </FS:AYA>

        LLVertexBuffer* buff = face->getVertexBuffer();

        if (buff)
        {
            LLMatrix4* model_matrix = &(face->getDrawable()->getRegion()->mRenderMatrix);

            llassert(gGL.getMatrixMode() == LLRender::MM_MODELVIEW);
            LLRenderPass::applyModelMatrix(model_matrix);

            buff->setBuffer();
            buff->drawRange(LLRender::TRIANGLES, 0, buff->getNumVerts() - 1, buff->getNumIndices(), 0);
        }
    }
}

void LLDrawPoolTree::endDeferredPass(S32 pass)
{
    LL_RECORD_BLOCK_TIME(FTM_RENDER_TREES);

    shader->unbind();
}

//============================================
// shadow implementation
//============================================
void LLDrawPoolTree::beginShadowPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED;

    static LLCachedControl<F32> shadow_offset(gSavedSettings, "RenderDeferredTreeShadowOffset");
    static LLCachedControl<F32> shadow_bias(gSavedSettings, "RenderDeferredTreeShadowBias");
    glPolygonOffset(shadow_offset(), shadow_bias());

    LLEnvironment& environment = LLEnvironment::instance();

    gDeferredTreeShadowProgram.bind();
    gDeferredTreeShadowProgram.uniform1i(LLShaderMgr::SUN_UP_FACTOR, environment.getIsSunUp() ? 1 : 0);
    gDeferredTreeShadowProgram.setMinimumAlpha(0.5f);
}

void LLDrawPoolTree::renderShadow(S32 pass)
{
    renderDeferred(pass);
}

// <AYAstorm r30 P2> Motion blur / velocity pass (BD lineage, NiranV Dean,
// 995a1354d8). LGPL-2.1-only. Tree face-iter pattern: trees have no
// LLDrawInfo batches; use buff->drawRange directly. Trees don't move
// between frames, so feed per-region matrix as both LAST and CURRENT.

S32 LLDrawPoolTree::getNumMotionBlurPasses()
{
    return 1;
}

void LLDrawPoolTree::beginMotionBlurPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED;
    gVelocityProgram.bind();
    gVelocityProgram.uniformMatrix4fv(LLShaderMgr::LAST_MODELVIEW_MATRIX, 1, GL_FALSE, gGLLastModelView);
    gVelocityProgram.uniformMatrix4fv(LLShaderMgr::CURRENT_MODELVIEW_MATRIX, 1, GL_FALSE, gGLModelView);
    gVelocityProgram.uniform4f(LLShaderMgr::VIEWPORT, (F32)gGLViewport[0], (F32)gGLViewport[1], (F32)gGLViewport[2], (F32)gGLViewport[3]);
}

void LLDrawPoolTree::endMotionBlurPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED;
    gVelocityProgram.unbind();
}

void LLDrawPoolTree::renderMotionBlur(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED;
    LLGLEnable cull(GL_CULL_FACE);
    if (mDrawFace.empty())
        return;

    for (std::vector<LLFace*>::iterator iter = mDrawFace.begin(); iter != mDrawFace.end(); iter++)
    {
        LLFace* face = *iter;
        LLDrawable* drawable = face ? face->getDrawable() : nullptr;
        if (!drawable) continue;
        if (LLPipeline::isParcelHideAlive(drawable)) continue;

        LLVertexBuffer* buff = face->getVertexBuffer();
        if (!buff) continue;

        LLMatrix4* model_matrix = &(drawable->getRegion()->mRenderMatrix);
        llassert(gGL.getMatrixMode() == LLRender::MM_MODELVIEW);
        LLRenderPass::applyModelMatrix(model_matrix);
        LLGLSLShader::sCurBoundShaderPtr->uniformMatrix4fv(LLShaderMgr::CURRENT_OBJECT_MATRIX, 1, GL_FALSE, (GLfloat*)model_matrix->mMatrix);
        LLGLSLShader::sCurBoundShaderPtr->uniformMatrix4fv(LLShaderMgr::LAST_OBJECT_MATRIX, 1, GL_FALSE, (GLfloat*)model_matrix->mMatrix);

        buff->setBuffer();
        buff->drawRange(LLRender::TRIANGLES, 0, buff->getNumVerts() - 1, buff->getNumIndices(), 0);
    }
}
// </AYAstorm r30 P2>

void LLDrawPoolTree::endShadowPass(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED;

    // <FS:PP> Attempt to speed up things a little
    // glPolygonOffset(gSavedSettings.getF32("RenderDeferredSpotShadowOffset"),
    //                  gSavedSettings.getF32("RenderDeferredSpotShadowBias"));
    static LLCachedControl<F32> RenderDeferredSpotShadowOffset(gSavedSettings, "RenderDeferredSpotShadowOffset");
    static LLCachedControl<F32> RenderDeferredSpotShadowBias(gSavedSettings, "RenderDeferredSpotShadowBias");
    // <FS:AYAstorm r30 P5 step 5 pivot 2026-05-19> Cinematic 短絡撤去、user cvar 値を使う
    glPolygonOffset(RenderDeferredSpotShadowOffset, RenderDeferredSpotShadowBias);
    // </FS:AYAstorm>
    // </FS:PP>

    gDeferredTreeShadowProgram.unbind();
}

bool LLDrawPoolTree::verify() const
{
    return true;
}

LLViewerTexture *LLDrawPoolTree::getTexture()
{
    return mTexturep;
}

LLViewerTexture *LLDrawPoolTree::getDebugTexture()
{
    return mTexturep;
}


LLColor3 LLDrawPoolTree::getDebugColor() const
{
    return LLColor3(1.f, 0.f, 1.f);
}

// <AYAstorm r41> sub-step 2.2: empty Vulkan record hook. Stage 3 replaces the
// marker with PSO bind + vkCmdDraw* against cmd_buf.
void LLDrawPoolTree::recordPoolDraws(VkCommandBuffer cmd_buf)
{
    static bool logged_once = false;
    if (!logged_once)
    {
        LL_INFOS("VkRecord") << "Tree pool recordPoolDraws hook fired (one-shot)" << LL_ENDL;
        logged_once = true;
    }
    LLVKLoader::recordPlaceholderPoolDraw(cmd_buf);
}
// </AYAstorm r41>

