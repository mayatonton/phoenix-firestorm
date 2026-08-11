/**
 * @file llpipelineshadow.cpp
 * @brief Rendering pipeline: shadow passes (pure move from pipeline.cpp).
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "pipeline.h"
#include "llvkbucket.h"
#include <unordered_map>
#include <optional>
#include "llimagepng.h"
#include "llaudioengine.h" // For debugging.
#include "llocclusiongeometrymgr.h" // r13: OBB occlusion debug overlay.
#include "llerror.h"
#include "llfile.h"
#include "llviewercontrol.h"
#include "llfasttimer.h"
#include "llfontgl.h"
#include "llfontvertexbuffer.h"
#include "llnamevalue.h"
#include "llpointer.h"
#include "llprimitive.h"
#include "llvolume.h"
#include "material_codes.h"
#include "v3color.h"
#include "llui.h"
#include "llglheaders.h"
#include "llrender.h"
#include "llvkloader.h"
#include "llvkcontract.h"
#include <fstream>
#include "llvkuboreg.h"
#include "llstartup.h"
#include "llwindow.h"   // swapBuffers()
#include "llagent.h"
#include "llagentcamera.h"
#include "llappviewer.h"
#include "lltexturecache.h"
#include "lltexturefetch.h"
#include "llimageworker.h"
#include "lldrawable.h"
#include "lldrawpoolalpha.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolbump.h"
#include "lldrawpoolwlsky.h"
#include "lldrawpooltree.h"
#include "lldrawpoolwater.h"
#include "llface.h"
#include "llfeaturemanager.h"
#include "llfloatertelehub.h"
#include "llfloaterreg.h"
#include "llhudmanager.h"
#include "llhudnametag.h"
#include "llhudtext.h"
#include "lllightconstants.h"
#include "llmeshrepository.h"
#include "llvolumemgr.h"
#include "llpipelineframecontext.h"
#include "llpipelinelistener.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "lltracker.h"
#include "lltool.h"
#include "lltoolmgr.h"
#include "llviewercamera.h"
#include "llviewermediafocus.h"
#include "llviewertexturelist.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llparcel.h" // <FS:AYA> [ParcelHide-Tag] for LLParcel::getDesc()
#include "llviewerregion.h" // for audio debugging.
#include "llviewerwindow.h" // For getSpinAxis
#include "llvoavatarself.h"
#include "llviewerjointattachment.h"
#include "llvocache.h"
#include "llvosky.h"
#include "llvowlsky.h"
#include "llvotree.h"
#include "llvovolume.h"
#include "llvosurfacepatch.h"
#include "llvowater.h"
#include "llvotree.h"
#include "llvopartgroup.h"
#include "llworld.h"
#include "llcubemap.h"
#include "llviewershadermgr.h"
#include "llreloadqueue.h"
#include "llviewerstats.h"
#include "llviewerjoystick.h"
#include "llviewerdisplay.h"
#include "llspatialpartition.h"
#include "llmutelist.h"
#include "lltoolpie.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llpathinglib.h"
#include "llfloaterpathfindingconsole.h"
#include "llfloaterpathfindingcharacters.h"
#include "llfloatertools.h"
#include "llfloatersnapshot.h" // <FS:Beq/> for snapshotFrame
#include "llfloaterflickr.h" // <FS:Beq/> for snapshotFrame
#include "fsfloaterprimfeed.h" // <FS:Beq/> for snapshotFrame
#include "llsnapshotlivepreview.h" // <FS:Beq/> for snapshotFrame
#include "llpathfindingpathtool.h"
#include "llscenemonitor.h"
#include "llprogressview.h"
#include "llcleanup.h"
#include "gltfscenemanager.h"
#include "llvisualeffect.h"
#include "rlvactions.h"
#include "rlvlocks.h"
#include "llenvironment.h"
#include "llsettingsvo.h"
// PCH 経由で Xlib (X11/Xlib.h:84) の `#define None 0L` が流入し、enum class member
// 等の `None` トークンを数値リテラル `0L` に置換してしまう。pipeline.cpp は X11
// API を直接呼ばないのでファイル冒頭で undef して局所的に無効化する。
// memory: project_linux_xlib_status_define_trap.md
#undef None

#include "llerror.h"
#include "llpipelineinternal.h"

namespace
{
    bool shadowLayeredOracleEnabled()
    {
        static const bool s = []() -> bool {
            const char* e = getenv("AYASTORM_SHADOW_LAYERED_ORACLE");
            return (e != nullptr) && (atof(e) != 0.0);
        }();
        return s;
    }
}

glm::mat4 look(const LLVector3 pos, const LLVector3 dir, const LLVector3 up)
{
    LLVector3 dirN;
    LLVector3 upN;
    LLVector3 lftN;

    lftN = dir % up;
    lftN.normVec();

    upN = lftN % dir;
    upN.normVec();

    dirN = dir;
    dirN.normVec();

    F32 ret[16];
    ret[ 0] = lftN[0];
    ret[ 1] = upN[0];
    ret[ 2] = -dirN[0];
    ret[ 3] = 0.f;

    ret[ 4] = lftN[1];
    ret[ 5] = upN[1];
    ret[ 6] = -dirN[1];
    ret[ 7] = 0.f;

    ret[ 8] = lftN[2];
    ret[ 9] = upN[2];
    ret[10] = -dirN[2];
    ret[11] = 0.f;

    ret[12] = -(lftN*pos);
    ret[13] = -(upN*pos);
    ret[14] = dirN*pos;
    ret[15] = 1.f;

    return glm::make_mat4(ret);
}

static bool shadowMatricesFinite(const glm::mat4& view, const glm::mat4& proj)
{
    const F32* vp = glm::value_ptr(view);
    const F32* pp = glm::value_ptr(proj);
    for (U32 fi = 0; fi < 16u; ++fi)
    {
        if (!std::isfinite(vp[fi]) || !std::isfinite(pp[fi]))
        {
            return false;
        }
    }
    return true;
}

namespace
{
    struct ScopedShadowBatchCull
    {
        F32 mPrev;
        ScopedShadowBatchCull(F32 radius) : mPrev(LLRenderPass::sShadowBatchCullRadius)
        {
            LLRenderPass::sShadowBatchCullRadius = radius;
        }
        ~ScopedShadowBatchCull()
        {
            LLRenderPass::sShadowBatchCullRadius = mPrev;
        }
    };

    struct ScopedIdentityModelView
    {
        F32 mSaved[16];
        ScopedIdentityModelView()
        {
            memcpy(mSaved, gGLModelView, sizeof(mSaved));
            glm::mat4 id = glm::identity<glm::mat4>();
            memcpy(gGLModelView, glm::value_ptr(id), sizeof(mSaved));
            gGL.matrixMode(LLRender::MM_MODELVIEW);
            gGL.loadMatrix(glm::value_ptr(id));
            gGLLastMatrix = NULL;
        }
        ~ScopedIdentityModelView()
        {
            memcpy(gGLModelView, mSaved, sizeof(mSaved));
            gGLLastMatrix = NULL;
        }
    };
}

static bool sShadowAlphaMvEnabled()
{
    return LLVKLoader::isBindlessActiveVk();
}

class LLDisableOcclusionCulling
{
public:
    S32 mUseOcclusion;

    LLDisableOcclusionCulling()
    {
        mUseOcclusion = LLPipeline::sUseOcclusion;
        LLPipeline::sUseOcclusion = 0;
    }

    ~LLDisableOcclusionCulling()
    {
        LLPipeline::sUseOcclusion = mUseOcclusion;
    }
};

void LLPipeline::requestResizeShadowTexture()
{
    gResizeShadowTexture = true;
}

bool LLPipeline::resizeShadowTexture()
{
    releaseSunShadowTargets();
    releaseSpotShadowTargets();
    return allocateShadowBuffer(getFrameRT()->screen.getWidth(), getFrameRT()->screen.getHeight()); // <FS:Beq> revert and correct previous shadowres fix that leads to FPS drop (FIRE-3200)
}

bool LLPipeline::allocateShadowBuffer(U32 resX, U32 resY)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    S32 shadow_detail = RenderShadowDetail;

    F32 scale = gCubeSnapshot ? 1.0f : llmax(0.f, RenderShadowResolutionScale); // Don't scale probe shadow maps
    U32 sun_shadow_map_width = BlurHappySize(resX, scale);
    U32 sun_shadow_map_height = BlurHappySize(resY, scale);

    if (shadow_detail > 0)
    { //allocate a single 4-layer layered sun shadow map
        if (!getFrameRT()->sunShadowLayered.allocateLayeredDepth(sun_shadow_map_width,
                                                                 sun_shadow_map_height,
                                                                 LLPipeline::kSunShadowCount))
        {
            return false;
        }
    }
    else
    {
        releaseSunShadowTargets();
    }

    if (!gCubeSnapshot) // hack to not allocate spot shadow maps during ReflectionMapManager init
    {
        U32 width = (U32)(resX * scale);
        U32 height = width;

        if (shadow_detail > 1)
        { //allocate two spot shadow maps
            U32 spot_shadow_map_width = width;
            U32 spot_shadow_map_height = height;
            for (U32 i = 0; i < 2; i++)
            {
                if (!mSpotShadow[i].allocate(spot_shadow_map_width, spot_shadow_map_height, 0, true))
                {
                    return false;
                }
            }
        }
        else
        {
            releaseSpotShadowTargets();
        }
    }

    {
        static const bool s_res_oracle = []() -> bool {
            const char* e = getenv("AYASTORM_SHADOW_RES_ORACLE");
            return (e != nullptr) && (atof(e) != 0.0);
        }();
        if (s_res_oracle)
        {
            U32 mism = 0;
            if (shadow_detail > 0)
            {
                for (U32 i = 0; i < LLPipeline::kSunShadowCount; i++)
                {
                    LLRenderTarget* t = getSunShadowTarget(i);
                    if (!t || t->getWidth() != sun_shadow_map_width || t->getHeight() != sun_shadow_map_height)
                    {
                        ++mism;
                    }
                }
            }
            U32 spot_expected = (U32)(resX * scale);
            if (!gCubeSnapshot && shadow_detail > 1)
            {
                for (U32 i = 0; i < 2; i++)
                {
                    if (mSpotShadow[i].getWidth() != spot_expected || mSpotShadow[i].getHeight() != spot_expected)
                    {
                        ++mism;
                    }
                }
            }
            if (mism == 0)
            {
                LL_INFOS("ShadowResOracle") << "shadow_res_oracle detail=" << shadow_detail
                    << " cinematic=" << (isCinematicMode() ? 1 : 0)
                    << " sun=" << sun_shadow_map_width << "x" << sun_shadow_map_height
                    << " spot=" << spot_expected << " mismatch=0" << LL_ENDL;
            }
            else
            {
                LL_WARNS("ShadowResOracle") << "shadow_res_oracle detail=" << shadow_detail
                    << " cinematic=" << (isCinematicMode() ? 1 : 0)
                    << " sun=" << sun_shadow_map_width << "x" << sun_shadow_map_height
                    << " spot=" << spot_expected << " mismatch=" << mism << LL_ENDL;
            }
        }
    }


    // set up shadow map filtering and compare modes (single layered RT)
    if (shadow_detail > 0)
    {
        LLRenderTarget* shadow_target = getSunShadowTarget(0);
        if (shadow_target)
        {
            gGL.getTexUnit(0)->bind(shadow_target, true, /*depthLayer=*/0);
            gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
            gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);

            shadow_target->setUseDepthCompareSampler(true);
        }
    }

    if (shadow_detail > 1 && !gCubeSnapshot)
    {
        for (U32 i = 0; i < LLPipeline::kSpotShadowCount; i++)
        {
            LLRenderTarget* shadow_target = getSpotShadowTarget(i);
            if (shadow_target)
            {
                gGL.getTexUnit(0)->bind(shadow_target, true);
                gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
                gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);

                shadow_target->setUseDepthCompareSampler(true);
            }
        }
    }

    return true;
}

void LLPipeline::releaseShadowBuffers()
{
    releaseSunShadowTargets();
    releaseSpotShadowTargets();
}

void LLPipeline::releaseSunShadowTargets()
{
    getFrameRT()->sunShadowLayered.release();
}

void LLPipeline::releaseSpotShadowTargets()
{
    if (!gCubeSnapshot) // hack to avoid freeing spot shadows during ReflectionMapManager init
    {
        for (U32 i = 0; i < 2; i++)
        {
            mSpotShadow[i].release();
        }
    }
}

void LLPipeline::enableShadows(const bool enable_shadows)
{
    //should probably do something here to wrangle shadows....
}

void LLPipeline::renderGeomShadow(LLCamera& camera)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LL_PROFILE_GPU_ZONE("renderGeomShadow");
    const LLRecordPassContext ctx = buildRecordPassContext();

    U32 cur_type = 0;

    LLGLEnable cull(GL_CULL_FACE);

    LLVertexBuffer::unbind();

    pool_set_t::iterator iter1 = mPools.begin();

    while ( iter1 != mPools.end() )
    {
        LLDrawPool *poolp = *iter1;

        cur_type = poolp->getType();

        pool_set_t::iterator iter2 = iter1;
        if (hasRenderType(poolp->getType()) && poolp->getNumShadowPasses() > 0)
        {
            U32 geom_sec = LLVKLoader::VKPERF_SHSEC_OTHER;
            switch (cur_type)
            {
            case LLDrawPool::POOL_TERRAIN:    geom_sec = LLVKLoader::VKPERF_SHSEC_GEOM_TERRAIN; break;
            case LLDrawPool::POOL_TREE:       geom_sec = LLVKLoader::VKPERF_SHSEC_GEOM_TREE;    break;
            case LLDrawPool::POOL_AVATAR:
            case LLDrawPool::POOL_CONTROL_AV: geom_sec = LLVKLoader::VKPERF_SHSEC_GEOM_AVATAR;  break;
            default: break;
            }
            LLVKLoader::VkPerfShadowSectionScope geom_scope(geom_sec);

            poolp->prerender() ;

            gGLLastMatrix = NULL;
            gGL.loadMatrix(gGLModelView);

            for( S32 i = 0; i < poolp->getNumShadowPasses(); i++ )
            {
                LLVertexBuffer::unbind();
                poolp->beginShadowPass(ctx, i);
                for (iter2 = iter1; iter2 != mPools.end(); iter2++)
                {
                    LLDrawPool *p = *iter2;
                    if (p->getType() != cur_type)
                    {
                        break;
                    }

                    p->renderShadow(ctx, i);
                }
                poolp->endShadowPass(ctx, i);
                LLVertexBuffer::unbind();
            }
        }
        else
        {
            // Skip all pools of this type
            for (iter2 = iter1; iter2 != mPools.end(); iter2++)
            {
                LLDrawPool *p = *iter2;
                if (p->getType() != cur_type)
                {
                    break;
                }
            }
        }
        iter1 = iter2;
    }

    gGLLastMatrix = NULL;
    gGL.loadMatrix(gGLModelView);
}

void LLPipeline::bindShadowMaps(LLGLSLShader& shader)
{
    for (U32 i = 0; i < LLPipeline::kSunShadowCount; i++)
    {
        LLRenderTarget* shadow_target = getSunShadowTarget(i);
        if (shadow_target)
        {
            S32 channel = shader.enableTexture(LLShaderMgr::DEFERRED_SHADOW0 + i, LLTexUnit::TT_TEXTURE);
            if (channel > -1)
            {
                gGL.getTexUnit(channel)->bind(getSunShadowTarget(i), true, /*depthLayer=*/i);
            }
        }
    }

    for (U32 i = LLPipeline::kSunShadowCount; i < LLPipeline::kTotalShadowCount; i++)
    {
        S32 channel = shader.enableTexture(LLShaderMgr::DEFERRED_SHADOW0 + i);
        if (channel > -1)
        {
            LLRenderTarget* shadow_target = getSpotShadowTarget(i - LLPipeline::kSunShadowCount);
            if (shadow_target)
            {
                gGL.getTexUnit(channel)->bind(shadow_target, true);
            }
        }
    }

}

void LLPipeline::renderShadowOpaqueBucketizedMultiview(LLCamera& cam, LLCullResult& result)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LL_PROFILE_GPU_ZONE("renderShadowOpaqueBucketizedMultiview");
    LLVKLoader::gpuCheckpoint("renderShadowOpaqueBucketizedMultiview");

    LLPipelineFrameContext::getInstance().setShadowPass(true);
    const LLRecordPassContext ctx = buildRecordPassContext();

    LLVKLoader::VkPerfShadowCtxScope shadow_ctx_scope(LLVKLoader::VKPERF_SHCTX_MV);
    LLVKLoader::gVkPerfShadowMapIndex = 6u;

    U32 saved_occlusion = sUseOcclusion;
    sUseOcclusion = 0;

    LLGLEnable cull(GL_CULL_FACE);
    LLGLEnable clamp_depth(GL_DEPTH_CLAMP);
    LLGLDepthTest depth_test(GL_TRUE, GL_TRUE, GL_LESS);

    ScopedIdentityModelView mv_scope;

    LLVertexBuffer::unbind();
    for (int jj = 0; jj < 2; ++jj)
    {
        bool rigged = jj == 1;
        gDeferredShadowMultiviewProgram.bind(rigged);

        gGL.diffuseColor4f(1, 1, 1, 1);

        if (RenderShadowDetail <= 2)
        {
            gGL.setColorMask(false, false);
        }

        gGL.getTexUnit(0)->disable();

        {
            LLVKLoader::VkPerfShadowSectionScope sec_scope(LLVKLoader::VKPERF_SHSEC_OPAQUE + (rigged ? 1u : 0u));
            for (U32 ti = 0; ti < LLVKBucket::kOpaqueShadowPassCount; ++ti)
            {
                renderObjects(ctx, LLVKBucket::kOpaqueShadowPasses[ti], false, false, rigged);
            }
        }

        {
            LLVKLoader::VkPerfShadowSectionScope sec_scope(LLVKLoader::VKPERF_SHSEC_GLTF_PBR + (rigged ? 1u : 0u));
            renderGLTFObjects(ctx, LLRenderPass::PASS_GLTF_PBR, false, rigged);
        }

        gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
    }

    if (sShadowAlphaMvEnabled())
    {
        renderShadowAlphaMultiview(cam, result);
    }

    gGL.setColorMask(true, true);

    sUseOcclusion = saved_occlusion;
    LLPipelineFrameContext::getInstance().setShadowPass(false);
}

void LLPipeline::renderShadowAlphaMultiview(LLCamera& shadow_cam, LLCullResult& result)
{
    const LLRecordPassContext ctx = buildRecordPassContext();
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LL_PROFILE_GPU_ZONE("renderShadowAlphaMultiview");

    U32 target_width = LLRenderTarget::sCurResX;

    for (int i = 0; i < 2; ++i)
    {
        bool rigged = i == 1;

        {
            LLVKLoader::VkPerfShadowSectionScope sec_scope(LLVKLoader::VKPERF_SHSEC_AMASK + (rigged ? 1u : 0u));
            gDeferredShadowAlphaMaskMultiviewProgram.bind(rigged);
            LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(-1.f);
            LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(-1.f);
            if (LLVKLoader::isVulkanInitialized())
            {
                LLVKLoader::ShadowParams_PerShaderBind shadow_params = {};
                shadow_params.shadow_target_width = (float)target_width;
                LLVKLoader::writeCurrentShadowParamsUBO(shadow_params);
            }
            renderMaskedObjects(ctx, LLRenderPass::PASS_ALPHA_MASK, true, true, rigged);
        }

        {
            LLVKLoader::VkPerfShadowSectionScope sec_scope(LLVKLoader::VKPERF_SHSEC_ABLEND + (rigged ? 1u : 0u));
            renderAlphaObjectsMultiview(ctx, rigged);
        }

        {
            LLVKLoader::VkPerfShadowSectionScope sec_scope(LLVKLoader::VKPERF_SHSEC_FBMASK + (rigged ? 1u : 0u));
            gDeferredShadowAlphaMaskMultiviewProgram.bind(rigged);
            LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(-1.f);
            LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(-1.f);
            if (LLVKLoader::isVulkanInitialized())
            {
                LLVKLoader::ShadowParams_PerShaderBind shadow_params = {};
                shadow_params.shadow_target_width = (float)target_width;
                LLVKLoader::writeCurrentShadowParamsUBO(shadow_params);
            }
            renderFullbrightMaskedObjects(ctx, LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, true, true, rigged);
        }

        {
            LLVKLoader::VkPerfShadowSectionScope sec_scope(LLVKLoader::VKPERF_SHSEC_GRASSMAT + (rigged ? 1u : 0u));
            gDeferredShadowAlphaMaskMultiviewProgram.bind(rigged);

            if (i == 0)
            {
                LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);
                renderObjects(ctx, LLRenderPass::PASS_GRASS, true, true);
            }

            LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(-1.f);
            LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(-1.f);
            renderMaskedObjects(ctx, LLRenderPass::PASS_NORMSPEC_MASK, true, false, rigged);
            renderMaskedObjects(ctx, LLRenderPass::PASS_MATERIAL_ALPHA_MASK, true, false, rigged);
            renderMaskedObjects(ctx, LLRenderPass::PASS_SPECMAP_MASK, true, false, rigged);
            renderMaskedObjects(ctx, LLRenderPass::PASS_NORMMAP_MASK, true, false, rigged);
        }
    }

    for (int i = 0; i < 2; ++i)
    {
        bool rigged = i == 1;
        LLVKLoader::VkPerfShadowSectionScope sec_scope(LLVKLoader::VKPERF_SHSEC_GLTF_AMASK + (rigged ? 1u : 0u));
        gDeferredShadowGLTFAlphaMaskMultiviewProgram.bind(rigged);
        LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);
        if (LLVKLoader::isVulkanInitialized())
        {
            LLVKLoader::ShadowParams_PerShaderBind shadow_params = {};
            shadow_params.shadow_target_width = (float)target_width;
            LLVKLoader::writeCurrentShadowParamsUBO(shadow_params);
        }

        gGL.loadMatrix(gGLModelView);
        gGLLastMatrix = NULL;

        U32 type = LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK;

        if (rigged)
        {
            mAlphaMaskPool->pushRiggedGLTFBatches(ctx, type + 1);
        }
        else
        {
            mAlphaMaskPool->pushGLTFBatches(ctx, type);
        }

        gGL.loadMatrix(gGLModelView);
        gGLLastMatrix = NULL;
    }
}

void LLPipeline::renderShadowOpaqueBucketized(LLCamera& cam, LLCullResult& result)
{
    const LLRecordPassContext ctx = buildRecordPassContext();
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LL_PROFILE_GPU_ZONE("renderShadowOpaqueBucketized");

    LLVertexBuffer::unbind();
    for (int jj = 0; jj < 2; ++jj)
    {
        bool rigged = jj == 1;
        gDeferredShadowProgram.bind(rigged);

        gGL.diffuseColor4f(1, 1, 1, 1);

        gGL.getTexUnit(0)->disable();

        {
            LLVKLoader::VkPerfShadowSectionScope sec_scope(LLVKLoader::VKPERF_SHSEC_OPAQUE + (rigged ? 1u : 0u));
            for (U32 ti = 0; ti < LLVKBucket::kOpaqueShadowPassCount; ++ti)
            {
                renderObjects(ctx, LLVKBucket::kOpaqueShadowPasses[ti], false, false, rigged);
            }
        }

        {
            LLVKLoader::VkPerfShadowSectionScope sec_scope(LLVKLoader::VKPERF_SHSEC_GLTF_PBR + (rigged ? 1u : 0u));
            renderGLTFObjects(ctx, LLRenderPass::PASS_GLTF_PBR, false, rigged);
        }

        gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
    }
}

void LLPipeline::renderShadow(const glm::mat4& view, const glm::mat4& proj, LLCamera& shadow_cam, LLCullResult& result, bool depth_clamp, bool render_opaque_bucketized, bool render_alpha)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE; //LL_RECORD_BLOCK_TIME(FTM_SHADOW_RENDER);
    LL_PROFILE_GPU_ZONE("renderShadow");
    LLVKLoader::gpuCheckpoint("renderShadow");

    if (!shadowMatricesFinite(view, proj))
    {
        return;
    }

    LLPipelineFrameContext::getInstance().setShadowPass(true);
    const LLRecordPassContext ctx = buildRecordPassContext();

    // disable occlusion culling during shadow render
    U32 saved_occlusion = sUseOcclusion;
    sUseOcclusion = 0;

    LLGLEnable cull(GL_CULL_FACE);

    //enable depth clamping if available
    LLGLEnable clamp_depth(depth_clamp ? GL_DEPTH_CLAMP : 0);

    LLGLDepthTest depth_test(GL_TRUE, GL_TRUE, GL_LESS);

    //generate shadow map
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.pushMatrix();
    gGL.loadMatrix(glm::value_ptr(proj));
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();
    gGL.loadMatrix(glm::value_ptr(view));

    gGLLastMatrix = NULL;

    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);


    struct CompareVertexBuffer
    {
        bool operator()(const LLDrawInfo* const& lhs, const LLDrawInfo* const& rhs)
        {
            return lhs->mVertexBuffer > rhs->mVertexBuffer;
        }
    };


    LLVertexBuffer::unbind();

    if (RenderShadowDetail <= 2)
    {
        gGL.setColorMask(false, false);
    }

    if (render_opaque_bucketized)
    {
        renderShadowOpaqueBucketized(shadow_cam, result);
    }

    if (LLPipeline::sUseOcclusion > 1)
    { // do occlusion culling against non-masked only to take advantage of hierarchical Z
        doOcclusion(shadow_cam);
    }


    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow geom");
        renderGeomShadow(shadow_cam);
    }

    if (render_alpha)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow alpha");
        LL_PROFILE_GPU_ZONE("shadow alpha");
        U32 target_width = LLRenderTarget::sCurResX;

        for (int i = 0; i < 2; ++i)
        {
            bool rigged = i == 1;

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow alpha masked");
                LL_PROFILE_GPU_ZONE("shadow alpha masked");
                LLVKLoader::VkPerfShadowSectionScope sec_scope(LLVKLoader::VKPERF_SHSEC_AMASK + (rigged ? 1u : 0u));
                gDeferredShadowAlphaMaskProgram.bind(rigged);
                LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);
                if (LLVKLoader::isVulkanInitialized())
                {
                    LLVKLoader::ShadowParams_PerShaderBind shadow_params = {};
                    shadow_params.shadow_target_width = (float)target_width;
                    LLVKLoader::writeCurrentShadowParamsUBO(shadow_params);
                }
                renderMaskedObjects(ctx, LLRenderPass::PASS_ALPHA_MASK, true, true, rigged);
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow alpha blend");
                LL_PROFILE_GPU_ZONE("shadow alpha blend");
                LLVKLoader::VkPerfShadowSectionScope sec_scope(LLVKLoader::VKPERF_SHSEC_ABLEND + (rigged ? 1u : 0u));
                renderAlphaObjects(ctx, rigged, 0);
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow fullbright alpha masked");
                LL_PROFILE_GPU_ZONE("shadow alpha masked");
                LLVKLoader::VkPerfShadowSectionScope sec_scope(LLVKLoader::VKPERF_SHSEC_FBMASK + (rigged ? 1u : 0u));
                gDeferredShadowFullbrightAlphaMaskProgram.bind(rigged);
                LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);
                if (LLVKLoader::isVulkanInitialized())
                {
                    LLVKLoader::ShadowParams_PerShaderBind shadow_params = {};
                    shadow_params.shadow_target_width = (float)target_width;
                    LLVKLoader::writeCurrentShadowParamsUBO(shadow_params);
                }
                renderFullbrightMaskedObjects(ctx, LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, true, true, rigged);
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow alpha grass");
                LL_PROFILE_GPU_ZONE("shadow alpha grass");
                LLVKLoader::VkPerfShadowSectionScope sec_scope(LLVKLoader::VKPERF_SHSEC_GRASSMAT + (rigged ? 1u : 0u));
                gDeferredTreeShadowProgram.bind(rigged);
                LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);

                if (i == 0)
                {
                    renderObjects(ctx, LLRenderPass::PASS_GRASS, true);
                }

                {
                    LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow alpha material");
                    LL_PROFILE_GPU_ZONE("shadow alpha material");
                    renderMaskedObjects(ctx, LLRenderPass::PASS_NORMSPEC_MASK, true, false, rigged);
                    renderMaskedObjects(ctx, LLRenderPass::PASS_MATERIAL_ALPHA_MASK, true, false, rigged);
                    renderMaskedObjects(ctx, LLRenderPass::PASS_SPECMAP_MASK, true, false, rigged);
                    renderMaskedObjects(ctx, LLRenderPass::PASS_NORMMAP_MASK, true, false, rigged);
                }
            }
        }

        for (int i = 0; i < 2; ++i)
        {
            bool rigged = i == 1;
            LLVKLoader::VkPerfShadowSectionScope sec_scope(LLVKLoader::VKPERF_SHSEC_GLTF_AMASK + (rigged ? 1u : 0u));
            gDeferredShadowGLTFAlphaMaskProgram.bind(rigged);
            LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);
            if (LLVKLoader::isVulkanInitialized())
            {
                LLVKLoader::ShadowParams_PerShaderBind shadow_params = {};
                shadow_params.shadow_target_width = (float)target_width;
                LLVKLoader::writeCurrentShadowParamsUBO(shadow_params);
            }

            gGL.loadMatrix(gGLModelView);
            gGLLastMatrix = NULL;

            U32 type = LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK;

            if (rigged)
            {
                mAlphaMaskPool->pushRiggedGLTFBatches(ctx, type + 1);
            }
            else
            {
                mAlphaMaskPool->pushGLTFBatches(ctx, type);
            }

            gGL.loadMatrix(gGLModelView);
            gGLLastMatrix = NULL;
        }
    }

    gGL.setColorMask(true, true);

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.popMatrix();
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.popMatrix();
    gGLLastMatrix = NULL;

    // reset occlusion culling flag
    sUseOcclusion = saved_occlusion;
    LLPipelineFrameContext::getInstance().setShadowPass(false);
}

LLRenderTarget* LLPipeline::getSunShadowTarget(U32 i)
{
    llassert(i < LLPipeline::kSunShadowCount);
    return &getFrameRT()->sunShadowLayered;
}

LLRenderTarget* LLPipeline::getSpotShadowTarget(U32 i)
{
    llassert(i < LLPipeline::kSpotShadowCount);
    return &mSpotShadow[i];
}

void LLPipeline::generateSunShadow(LLCamera& camera)
{
    if (!isFrameRenderingDeferred() || RenderShadowDetail <= 0)
    {
        return;
    }

    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE; //LL_RECORD_BLOCK_TIME(FTM_GEN_SUN_SHADOW);
    LL_PROFILE_GPU_ZONE("generateSunShadow");

    LLVKLoader::VkPerfPassScope perf_pass_scope(1);

    LLDisableOcclusionCulling no_occlusion;

    bool skip_avatar_update = false;
    if (!isAgentAvatarValid() || gAgentCamera.getCameraAnimating() || gAgentCamera.getCameraMode() != CAMERA_MODE_MOUSELOOK || !LLVOAvatar::sVisibleInFirstPerson)
    {
        skip_avatar_update = true;
    }

    if (!skip_avatar_update)
    {
        gAgentAvatarp->updateAttachmentVisibility(CAMERA_MODE_THIRD_PERSON);
    }

    glm::mat4 last_modelview = get_last_modelview();
    glm::mat4 last_projection = get_last_projection();

    pushRenderTypeMask();
    andRenderTypeMask(LLPipeline::RENDER_TYPE_SIMPLE,
                    LLPipeline::RENDER_TYPE_ALPHA,
                    LLPipeline::RENDER_TYPE_ALPHA_PRE_WATER,
                    LLPipeline::RENDER_TYPE_ALPHA_POST_WATER,
                    LLPipeline::RENDER_TYPE_GRASS,
                    LLPipeline::RENDER_TYPE_GLTF_PBR,
                    LLPipeline::RENDER_TYPE_FULLBRIGHT,
                    LLPipeline::RENDER_TYPE_BUMP,
                    LLPipeline::RENDER_TYPE_VOLUME,
                    LLPipeline::RENDER_TYPE_AVATAR,
                    LLPipeline::RENDER_TYPE_CONTROL_AV,
                    LLPipeline::RENDER_TYPE_TREE,
                    LLPipeline::RENDER_TYPE_TERRAIN,
                    LLPipeline::RENDER_TYPE_WATER,
                    LLPipeline::RENDER_TYPE_VOIDWATER,
                    LLPipeline::RENDER_TYPE_PASS_ALPHA,
                    LLPipeline::RENDER_TYPE_PASS_ALPHA_MASK,
                    LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_ALPHA_MASK,
                    LLPipeline::RENDER_TYPE_PASS_GRASS,
                    LLPipeline::RENDER_TYPE_PASS_SIMPLE,
                    LLPipeline::RENDER_TYPE_PASS_BUMP,
                    LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT,
                    LLPipeline::RENDER_TYPE_PASS_SHINY,
                    LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_SHINY,
                    LLPipeline::RENDER_TYPE_PASS_MATERIAL,
                    LLPipeline::RENDER_TYPE_PASS_MATERIAL_ALPHA,
                    LLPipeline::RENDER_TYPE_PASS_MATERIAL_ALPHA_MASK,
                    LLPipeline::RENDER_TYPE_PASS_MATERIAL_ALPHA_EMISSIVE,
                    LLPipeline::RENDER_TYPE_PASS_SPECMAP,
                    LLPipeline::RENDER_TYPE_PASS_SPECMAP_BLEND,
                    LLPipeline::RENDER_TYPE_PASS_SPECMAP_MASK,
                    LLPipeline::RENDER_TYPE_PASS_SPECMAP_EMISSIVE,
                    LLPipeline::RENDER_TYPE_PASS_NORMMAP,
                    LLPipeline::RENDER_TYPE_PASS_NORMMAP_BLEND,
                    LLPipeline::RENDER_TYPE_PASS_NORMMAP_MASK,
                    LLPipeline::RENDER_TYPE_PASS_NORMMAP_EMISSIVE,
                    LLPipeline::RENDER_TYPE_PASS_NORMSPEC,
                    LLPipeline::RENDER_TYPE_PASS_NORMSPEC_BLEND,
                    LLPipeline::RENDER_TYPE_PASS_NORMSPEC_MASK,
                    LLPipeline::RENDER_TYPE_PASS_NORMSPEC_EMISSIVE,
                    LLPipeline::RENDER_TYPE_PASS_ALPHA_MASK_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_ALPHA_MASK_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_SIMPLE_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_BUMP_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_SHINY_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_SHINY_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_MATERIAL_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_MATERIAL_ALPHA_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_MATERIAL_ALPHA_MASK_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_MATERIAL_ALPHA_EMISSIVE_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_SPECMAP_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_SPECMAP_BLEND_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_SPECMAP_MASK_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_SPECMAP_EMISSIVE_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_NORMMAP_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_NORMMAP_BLEND_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_NORMMAP_MASK_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_NORMMAP_EMISSIVE_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_NORMSPEC_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_NORMSPEC_BLEND_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_NORMSPEC_MASK_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_NORMSPEC_EMISSIVE_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_GLTF_PBR,
                    LLPipeline::RENDER_TYPE_PASS_GLTF_PBR_RIGGED,
                    LLPipeline::RENDER_TYPE_PASS_GLTF_PBR_ALPHA_MASK,
                    LLPipeline::RENDER_TYPE_PASS_GLTF_PBR_ALPHA_MASK_RIGGED,
                    END_RENDER_TYPES);

    gGL.setColorMask(false, false);

    LLEnvironment& environment = LLEnvironment::instance();

    //get sun view matrix

    //store current projection/modelview matrix
    glm::mat4 saved_proj = get_current_projection();
    glm::mat4 saved_view = get_current_modelview();
    glm::mat4 inv_view = glm::inverse(saved_view);

    glm::mat4 view[6];
    glm::mat4 proj[6];

    LLVector3 caster_dir(environment.getIsSunUp() ? mSunDir : mMoonDir);

    //put together a universal "near clip" plane for shadow frusta
    LLPlane shadow_near_clip;
    {
        LLVector3 p = camera.getOrigin(); // gAgent.getPositionAgent();
        p += caster_dir * RenderFarClip*2.f;
        shadow_near_clip.setVec(p, caster_dir);
    }

    LLVector3 lightDir = -caster_dir;
    lightDir.normVec();

    //create light space camera matrix
    LLVector3 at = lightDir;

    LLVector3 up = camera.getAtAxis();

    if (fabsf(up*lightDir) > 0.75f)
    {
        up = camera.getUpAxis();
    }

    up.normVec();
    at.normVec();


    LLCamera main_camera = camera;

    F32 near_clip = 0.f;
    {
        //get visible point cloud
        std::vector<LLVector3> fp;

        main_camera.calcAgentFrustumPlanes(main_camera.mAgentFrustum);

        LLVector3 min,max;
        getVisiblePointCloud(main_camera,min,max,fp);

        if (fp.empty())
        {
            if (!hasRenderDebugMask(RENDER_DEBUG_SHADOW_FRUSTA) && !gCubeSnapshot)
            {
                mShadowCamera[0] = main_camera;
                mShadowExtents[0][0] = min;
                mShadowExtents[0][1] = max;

                mShadowFrustPoints[0].clear();
                mShadowFrustPoints[1].clear();
                mShadowFrustPoints[2].clear();
                mShadowFrustPoints[3].clear();
            }
            popRenderTypeMask();

            if (!skip_avatar_update)
            {
                gAgentAvatarp->updateAttachmentVisibility(gAgentCamera.getCameraMode());
            }

            return;
        }

        //get good split distances for frustum
        for (U32 i = 0; i < fp.size(); ++i)
        {
            glm::vec3 v(fp[i]);
            v = mul_mat4_vec3(saved_view, v);
            fp[i] = LLVector3(v);
        }

        min = fp[0];
        max = fp[0];

        //get camera space bounding box
        for (U32 i = 1; i < fp.size(); ++i)
        {
            update_min_max(min, max, fp[i]);
        }

        near_clip    = llclamp(-max.mV[2], 0.01f, 4.0f);
        F32 far_clip = llclamp(-min.mV[2]*2.f, 16.0f, 512.0f);

        //far_clip = llmin(far_clip, 128.f);
        far_clip = llmin(far_clip, camera.getFar());

        F32 range = far_clip-near_clip;

        LLVector3 split_exp = RenderShadowSplitExponent;

        F32 da = 1.f-llmax( fabsf(lightDir*up), fabsf(lightDir*camera.getLeftAxis()) );

        da = powf(da, split_exp.mV[2]);

        F32 sxp = split_exp.mV[1] + (split_exp.mV[0]-split_exp.mV[1])*da;

        // <FS:AYAstorm r30 P4> RenderShadowAutomaticDistance toggles sun-angle-weighted
        // split distribution (ON) vs equal linear splits (OFF).
        // <FS:AYAstorm:r30-bd-port> Phase 6 step 1/2: Cinematic uses BD verbatim clip planes.
        if (isCinematicMode())
        {
            if (RenderShadowAutomaticDistance)
            {
                // BD Auto=ON: powf-weighted * fixed RenderShadowFarClip
                for (U32 i = 0; i < 4; ++i)
                {
                    F32 x = (F32)(i+1)/4.f;
                    x = powf(x, sxp);
                    mSunClipPlanes.mV[i] = near_clip + RenderShadowFarClip*x;
                }
            }
            else
            {
                // BD Auto=OFF: per-cascade cumulative RenderShadowFarClipVec
                F32 tot = 0.f;
                for (U32 i = 0; i < 4; ++i)
                {
                    mSunClipPlanes.mV[i] = near_clip + tot + RenderShadowFarClipVec[i];
                    tot += RenderShadowFarClipVec[i];
                }
            }
        }
        else
        {
        // </FS:AYAstorm:r30-bd-port>
        for (U32 i = 0; i < 4; ++i)
        {
            F32 x = (F32)(i+1)/4.f;
            if (RenderShadowAutomaticDistance)
            {
                x = powf(x, sxp);
            }
            mSunClipPlanes.mV[i] = near_clip+range*x;
        }
        // <FS:AYAstorm:r30-bd-port> Phase 6 step 1/2
        }
        // </FS:AYAstorm:r30-bd-port>
        // </FS:AYAstorm r30 P4>

        mSunClipPlanes.mV[0] *= 1.25f; //bump back first split for transition padding
    }

    if (gCubeSnapshot)
    { // stretch clip planes for reflection probe renders to reduce number of shadow passes
        mSunClipPlanes.mV[1] = mSunClipPlanes.mV[2];
        mSunClipPlanes.mV[2] = mSunClipPlanes.mV[3];
        mSunClipPlanes.mV[3] *= 1.5f;
    }


    // convenience array of 4 near clip plane distances
    F32 dist[] = { near_clip, mSunClipPlanes.mV[0], mSunClipPlanes.mV[1], mSunClipPlanes.mV[2], mSunClipPlanes.mV[3] };

    static LLCullResult spot_result[2];
    bool cascade_valid[4] = { false, false, false, false };

    static const F32 s_cull_texels = []() -> F32 {
        const char* e = getenv("AYASTORM_SHADOW_CULL_TEXELS");
        return (e != nullptr) ? (F32)atof(e) : 1.0f;
    }();
    F32 mv_batch_cull_radius = 0.f;
    F32 cascade_batch_cull_radius[4] = { 0.f, 0.f, 0.f, 0.f };
    LLCamera tight_shadow_cam[4];

    if (mSunDiffuse == LLColor4::black)
    { //sun diffuse is totally black shadows don't matter
        skipRenderingShadows();
    }
    else
    {
        for (S32 j = 0; j < (gCubeSnapshot ? 2 : 4); j++)
        {
            LLVKLoader::gVkPerfShadowMapIndex = (U32)j;

            if (!hasRenderDebugMask(RENDER_DEBUG_SHADOW_FRUSTA) && !gCubeSnapshot)
            {
                mShadowFrustPoints[j].clear();
            }

            LLViewerCamera::setCurCameraID((LLViewerCamera::eCameraID)(LLViewerCamera::CAMERA_SUN_SHADOW0+j));

            //restore render matrices
            set_current_modelview(saved_view);
            set_current_projection(saved_proj);

            LLVector3 eye = camera.getOrigin();
            llassert(eye.isFinite());

            //camera used for shadow cull/render
            LLCamera shadow_cam;

            //create world space camera frustum for this split
            shadow_cam = camera;
            shadow_cam.setFar(16.f);

            LLViewerCamera::updateFrustumPlanes(shadow_cam, false, false, true);

            LLVector3* frust = shadow_cam.mAgentFrustum;

            LLVector3 pn = shadow_cam.getAtAxis();

            LLVector3 min, max;

            //construct 8 corners of split frustum section
            for (U32 i = 0; i < 4; i++)
            {
                LLVector3 delta = frust[i+4]-eye;
                delta += (frust[i+4]-frust[(i+2)%4+4])*0.05f;
                delta.normVec();
                F32 dp = delta*pn;
                frust[i] = eye + (delta*dist[j]*0.75f)/dp;
                frust[i+4] = eye + (delta*dist[j+1]*1.25f)/dp;
            }

            shadow_cam.calcAgentFrustumPlanes(frust);
            shadow_cam.mFrustumCornerDist = 0.f;

            if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA) && !gCubeSnapshot)
            {
                mShadowCamera[j] = shadow_cam;
            }

            std::vector<LLVector3> fp;

            if (!gPipeline.getVisiblePointCloud(shadow_cam, min, max, fp, lightDir)
                || j > RenderShadowSplits)
            {
                //no possible shadow receivers
                if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA) && !gCubeSnapshot)
                {
                    mShadowExtents[j][0] = LLVector3();
                    mShadowExtents[j][1] = LLVector3();
                    mShadowCamera[j + LLPipeline::kSunShadowCount] = shadow_cam;
                }

                mShadowError.mV[j] = 0.f;
                mShadowFOV.mV[j] = 0.f;

                continue;
            }

            if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA) && !gCubeSnapshot)
            {
                mShadowExtents[j][0] = min;
                mShadowExtents[j][1] = max;
                mShadowFrustPoints[j] = fp;
            }


            //find a good origin for shadow projection
            LLVector3 origin;

            //get a temporary view projection
            view[j] = look(camera.getOrigin(), lightDir, -up);

            std::vector<LLVector3> wpf;

            for (U32 i = 0; i < fp.size(); i++)
            {
                glm::vec3 p(fp[i]);
                p = mul_mat4_vec3(view[j], p);
                wpf.push_back(LLVector3(p));
            }

            min = wpf[0];
            max = wpf[0];

            for (U32 i = 0; i < fp.size(); ++i)
            { //get AABB in camera space
                update_min_max(min, max, wpf[i]);
            }

            {
                const F32 split_span = llmax(max.mV[0]-min.mV[0], max.mV[2]-min.mV[2]);
                const F32 map_res = (F32)getFrameRT()->sunShadowLayered.getWidth();
                cascade_batch_cull_radius[j] = (split_span > 0.f && map_res > 0.f)
                                       ? (s_cull_texels * split_span / map_res) : 0.f;
            }

            // Construct a perspective transform with perspective along y-axis that contains
            // points in wpf
            //Known:
            // - far clip plane
            // - near clip plane
            // - points in frustum
            //Find:
            // - origin

            //get some "interesting" points of reference
            LLVector3 center = (min+max)*0.5f;
            LLVector3 size = (max-min)*0.5f;
            LLVector3 near_center = center;
            near_center.mV[1] += size.mV[1]*2.f;


            //put all points in wpf in quadrant 0, reletive to center of min/max
            //get the best fit line using least squares
            F32 bfm = 0.f;
            F32 bfb = 0.f;

            for (U32 i = 0; i < wpf.size(); ++i)
            {
                wpf[i] -= center;
                wpf[i].mV[0] = fabsf(wpf[i].mV[0]);
                wpf[i].mV[2] = fabsf(wpf[i].mV[2]);
            }

            if (!wpf.empty())
            {
                F32 sx = 0.f;
                F32 sx2 = 0.f;
                F32 sy = 0.f;
                F32 sxy = 0.f;

                for (U32 i = 0; i < wpf.size(); ++i)
                {
                    sx += wpf[i].mV[0];
                    sx2 += wpf[i].mV[0]*wpf[i].mV[0];
                    sy += wpf[i].mV[1];
                    sxy += wpf[i].mV[0]*wpf[i].mV[1];
                }

                bfm = (sy*sx-wpf.size()*sxy)/(sx*sx-wpf.size()*sx2);
                bfb = (sx*sxy-sy*sx2)/(sx*sx-bfm*sx2);
            }

            {
                // best fit line is y=bfm*x+bfb

                //find point that is furthest to the right of line
                F32 off_x = -1.f;
                LLVector3 lp;

                for (U32 i = 0; i < wpf.size(); ++i)
                {
                    //y = bfm*x+bfb
                    //x = (y-bfb)/bfm
                    F32 lx = (wpf[i].mV[1]-bfb)/bfm;

                    lx = wpf[i].mV[0]-lx;

                    if (off_x < lx)
                    {
                        off_x = lx;
                        lp = wpf[i];
                    }
                }

                //get line with slope bfm through lp
                // bfb = y-bfm*x
                bfb = lp.mV[1]-bfm*lp.mV[0];

                //calculate error
                mShadowError.mV[j] = 0.f;

                for (U32 i = 0; i < wpf.size(); ++i)
                {
                    F32 lx = (wpf[i].mV[1]-bfb)/bfm;
                    mShadowError.mV[j] += fabsf(wpf[i].mV[0]-lx);
                }

                mShadowError.mV[j] /= wpf.size();
                mShadowError.mV[j] /= size.mV[0];

                if (mShadowError.mV[j] > RenderShadowErrorCutoff)
                { //just use ortho projection
                    mShadowFOV.mV[j] = -1.f;
                    origin.clearVec();
                    proj[j] = glm::ortho(min.mV[0], max.mV[0],
                                        min.mV[1], max.mV[1],
                                        -max.mV[2], -min.mV[2]);
                }
                else
                {
                    //origin is where line x = 0;
                    origin.setVec(0,bfb,0);

                    F32 fovz = 1.f;
                    F32 fovx = 1.f;

                    LLVector3 zp;
                    LLVector3 xp;

                    for (U32 i = 0; i < wpf.size(); ++i)
                    {
                        LLVector3 atz = wpf[i]-origin;
                        atz.mV[0] = 0.f;
                        atz.normVec();
                        if (fovz > -atz.mV[1])
                        {
                            zp = wpf[i];
                            fovz = -atz.mV[1];
                        }

                        LLVector3 atx = wpf[i]-origin;
                        atx.mV[2] = 0.f;
                        atx.normVec();
                        if (fovx > -atx.mV[1])
                        {
                            fovx = -atx.mV[1];
                            xp = wpf[i];
                        }
                    }

                    fovx = acos(fovx);
                    fovz = acos(fovz);

                    F32 cutoff = llmin((F32) RenderShadowFOVCutoff, 1.4f);

                    mShadowFOV.mV[j] = fovx;

                    if (fovx < cutoff && fovz > cutoff)
                    {
                        //x is a good fit, but z is too big, move away from zp enough so that fovz matches cutoff
                        F32 d = zp.mV[2]/tan(cutoff);
                        F32 ny = zp.mV[1] + fabsf(d);

                        origin.mV[1] = ny;

                        fovz = 1.f;
                        fovx = 1.f;

                        for (U32 i = 0; i < wpf.size(); ++i)
                        {
                            LLVector3 atz = wpf[i]-origin;
                            atz.mV[0] = 0.f;
                            atz.normVec();
                            fovz = llmin(fovz, -atz.mV[1]);

                            LLVector3 atx = wpf[i]-origin;
                            atx.mV[2] = 0.f;
                            atx.normVec();
                            fovx = llmin(fovx, -atx.mV[1]);
                        }

                        fovx = acos(fovx);
                        fovz = acos(fovz);

                        mShadowFOV.mV[j] = cutoff;
                    }


                    origin += center;

                    F32 ynear = -(max.mV[1]-origin.mV[1]);
                    F32 yfar = -(min.mV[1]-origin.mV[1]);

                    if (ynear < 0.1f) //keep a sensible near clip plane
                    {
                        F32 diff = 0.1f-ynear;
                        origin.mV[1] += diff;
                        ynear += diff;
                        yfar += diff;
                    }

                    if (fovx > cutoff)
                    { //just use ortho projection
                        origin.clearVec();
                        mShadowError.mV[j] = -1.f;
                        proj[j] = glm::ortho(min.mV[0], max.mV[0],
                                min.mV[1], max.mV[1],
                                -max.mV[2], -min.mV[2]);
                    }
                    else
                    {
                        //get perspective projection
                        view[j] = glm::inverse(view[j]);
                        //llassert(origin.isFinite());

                        glm::vec3 origin_agent(origin);

                        //translate view to origin
                        origin_agent = mul_mat4_vec3(view[j], origin_agent);

                        eye = LLVector3(origin_agent);
                        //llassert(eye.isFinite());
                        if (!hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA) && !gCubeSnapshot)
                        {
                            mShadowFrustOrigin[j] = eye;
                        }

                        view[j] = look(LLVector3(origin_agent), lightDir, -up);

                        F32 fx = 1.f/tanf(fovx);
                        F32 fz = 1.f/tanf(fovz);

                        proj[j] = glm::mat4(-fx, 0, 0, 0,
                            0, (yfar + ynear) / (ynear - yfar), 0, -1.0f,
                            0, 0, -fz, 0,
                            0, (2.f * yfar * ynear) / (ynear - yfar), 0, 0);
                    }
                }
            }

            //shadow_cam.setFar(128.f);
            shadow_cam.setOriginAndLookAt(eye, up, center);

            shadow_cam.setOrigin(0,0,0);

            set_current_modelview(view[j]);
            set_current_projection(proj[j]);

            LLViewerCamera::updateFrustumPlanes(shadow_cam, false, false, true);

            //shadow_cam.ignoreAgentFrustumPlane(LLCamera::AGENT_PLANE_NEAR);
            shadow_cam.getAgentPlane(LLCamera::AGENT_PLANE_NEAR).set(shadow_near_clip);

            set_current_modelview(view[j]);
            set_current_projection(proj[j]);

            set_last_modelview(mShadowModelview[j]);
            set_last_projection(mShadowProjection[j]);

            if (shadowMatricesFinite(view[j], proj[j]))
            {
                mShadowModelview[j] = view[j];
                mShadowProjection[j] = proj[j];
                mSunShadowMatrix[j] = sGlNdcToSampleBias*proj[j]*view[j]*inv_view;
            }


            cascade_valid[j] = shadowMatricesFinite(view[j], proj[j]);

            tight_shadow_cam[j] = shadow_cam;

            if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA) && !gCubeSnapshot)
            {
                mShadowCamera[j + LLPipeline::kSunShadowCount] = shadow_cam;
            }
        }

        if (LLVKLoader::isMultiviewEnabled())
        {
        LLVKLoader::ShadowViewProj_PerPass vp = {};
        for (int j = 0; j < 4; ++j)
        {
            if (!cascade_valid[j])
            {
                glm::mat4 m(0.0f);
                m[3][3] = -1.0f - (F32)j;
                memcpy(vp.shadow_viewproj[j], glm::value_ptr(m), sizeof(vp.shadow_viewproj[j]));
            }
            else
            {
                glm::mat4 z = glm::identity<glm::mat4>();
                z[2][2] = 0.5f;
                z[3][2] = 0.5f;
                glm::mat4 m = z * proj[j] * view[j];
                memcpy(vp.shadow_viewproj[j], glm::value_ptr(m), sizeof(vp.shadow_viewproj[j]));
            }
        }
        LLVKLoader::writeCurrentShadowViewProjUBO(vp);

        bool union_valid = false;
        int first_valid = -1;
        for (int j = 0; j < 4; ++j)
        {
            if (cascade_valid[j])
            {
                union_valid = true;
                if (first_valid < 0) { first_valid = j; }
            }
        }

        LLCamera union_cam = camera;
        if (union_valid)
        {
            glm::mat4 lbasis = view[first_valid];
            glm::mat4 linv   = glm::inverse(lbasis);
            LLVector3 bmin, bmax;
            bool first_pt = true;
            for (int j = 0; j < 4; ++j)
            {
                if (!cascade_valid[j])
                {
                    continue;
                }
                for (int c = 0; c < 8; ++c)
                {
                    glm::vec3 p = mul_mat4_vec3(lbasis, glm::vec3(tight_shadow_cam[j].mAgentFrustum[c]));
                    if (first_pt)
                    {
                        bmin.setVec(p.x, p.y, p.z);
                        bmax.setVec(p.x, p.y, p.z);
                        first_pt = false;
                    }
                    else
                    {
                        bmin.mV[0] = llmin(bmin.mV[0], p.x); bmin.mV[1] = llmin(bmin.mV[1], p.y); bmin.mV[2] = llmin(bmin.mV[2], p.z);
                        bmax.mV[0] = llmax(bmax.mV[0], p.x); bmax.mV[1] = llmax(bmax.mV[1], p.y); bmax.mV[2] = llmax(bmax.mV[2], p.z);
                    }
                }
            }

            glm::vec3 near_ref = mul_mat4_vec3(lbasis, glm::vec3(camera.getOrigin() + caster_dir * RenderFarClip * 2.f));
            bmax.mV[2] = llmax(bmax.mV[2], near_ref.z);

            LLVector3* frust = union_cam.mAgentFrustum;
            const F32 xs[4] = { bmin.mV[0], bmax.mV[0], bmax.mV[0], bmin.mV[0] };
            const F32 ys[4] = { bmin.mV[1], bmin.mV[1], bmax.mV[1], bmax.mV[1] };
            for (int i = 0; i < 4; ++i)
            {
                glm::vec3 pn = mul_mat4_vec3(linv, glm::vec3(xs[i], ys[i], bmax.mV[2]));
                glm::vec3 pf = mul_mat4_vec3(linv, glm::vec3(xs[i], ys[i], bmin.mV[2]));
                frust[i].setVec(pn.x, pn.y, pn.z);
                frust[i + 4].setVec(pf.x, pf.y, pf.z);
            }
            union_cam.calcAgentFrustumPlanes(frust);
            union_cam.setOrigin(0, 0, 0);
            union_cam.mFrustumCornerDist = 0.f;
        }
        static LLCullResult union_result;

        for (int j = 0; j < 4; ++j)
        {
            if (cascade_valid[j] && cascade_batch_cull_radius[j] > 0.f)
            {
                mv_batch_cull_radius = (mv_batch_cull_radius > 0.f)
                    ? llmin(mv_batch_cull_radius, cascade_batch_cull_radius[j])
                    : cascade_batch_cull_radius[j];
            }
        }

        if (union_valid)
        {
            LLPipelineFrameContext::getInstance().setShadowPass(true);
            ScopedShadowBatchCull cull_scope(mv_batch_cull_radius);
            updateCull(union_cam, union_result);
            stateSort(union_cam, union_result);
            LLPipelineFrameContext::getInstance().setShadowPass(false);
        }
        LLRenderPass::freezeAuthorShadowSources(LLPipelineFrameContext::getInstance().getCullResult());
        LLVKLoader::setShadowRecordPhase(true);

        U32 mv_view_mask = 0;
        VkImageView mv_depth_view = VK_NULL_HANDLE;
        {
        LLRTScope rts(getFrameRT()->sunShadowLayered, DEPTH_ARRAY_TAG, "shadow_mv_array");
        if (rts)
        {
        getFrameRT()->sunShadowLayered.getViewport(gGLViewport);
        if (union_valid)
        {
            ScopedShadowBatchCull cull_scope(mv_batch_cull_radius);
            renderShadowOpaqueBucketizedMultiview(union_cam, union_result);
        }

        mv_view_mask = LLVKLoader::currentRenderViewMask();
        mv_depth_view = LLVKLoader::currentRenderDepthView();
        }
        }

        for (int j = 0; j < 4; ++j)
        {
            if (!cascade_valid[j])
            {
                continue;
            }
            {
            LLRTScope rts(getFrameRT()->sunShadowLayered, DEPTH_LAYER_TAG, j, false, "shadow_layer");
            if (rts)
            {
            getFrameRT()->sunShadowLayered.getViewport(gGLViewport);
            set_current_modelview(view[j]);
            set_current_projection(proj[j]);
            {
                LLVKLoader::VkPerfShadowCtxScope shadow_ctx_scope(LLVKLoader::VKPERF_SHCTX_REST);
                LLVKLoader::gVkPerfShadowMapIndex = (U32)j;
                ScopedShadowBatchCull cull_scope(cascade_batch_cull_radius[j]);
                renderShadow(view[j], proj[j], tight_shadow_cam[j], union_result, true, false, !sShadowAlphaMvEnabled());
            }
            }
            }
        }
        LLVKLoader::setShadowRecordPhase(false);
        getFrameRT()->sunShadowLayered.bindForShaderRead(0, true);

        if (shadowLayeredOracleEnabled() && !gCubeSnapshot && RenderShadowDetail > 0)
        {
            const VkImageView arr = getFrameRT()->sunShadowLayered.getVkDepthArrayView();
            bool vp_ok = true;
            for (int a = 0; a < 4 && vp_ok; a++)
            {
                for (int k = 0; k < 16; k++)
                {
                    if (!std::isfinite(vp.shadow_viewproj[a][k]))
                    {
                        vp_ok = false;
                    }
                }
                for (int b = a + 1; b < 4 && vp_ok; b++)
                {
                    if (memcmp(vp.shadow_viewproj[a], vp.shadow_viewproj[b], 64) == 0)
                    {
                        vp_ok = false;
                    }
                }
            }
            const bool ok = LLVKLoader::isMultiviewEnabled() &&
                            (mv_view_mask == 0xFu) && (mv_depth_view == arr) &&
                            (arr != VK_NULL_HANDLE) && vp_ok;
            if (ok)
            {
                LL_INFOS("ShadowMultiviewOracle") << "shadow_mv_oracle view_mask=0x" << std::hex << mv_view_mask
                    << std::dec << " depth_array_ok=1 viewproj_distinct=1 end_to_end=1" << LL_ENDL;
            }
            else
            {
                LL_WARNS("ShadowMultiviewOracle") << "shadow_mv_oracle FAIL view_mask=0x" << std::hex << mv_view_mask
                    << std::dec << " depth_match=" << (mv_depth_view == arr) << " vp_ok=" << vp_ok
                    << " enabled=" << LLVKLoader::isMultiviewEnabled() << LL_ENDL;
            }
        }
        }
        else
        {
            static LLCullResult sun_result[4];
            {
                LLPipelineFrameContext::getInstance().setShadowPass(true);
                for (int j = 0; j < 4; ++j)
                {
                    if (!cascade_valid[j]) { continue; }
                    set_current_modelview(view[j]);
                    set_current_projection(proj[j]);
                    ScopedShadowBatchCull cull_scope(cascade_batch_cull_radius[j]);
                    updateCull(tight_shadow_cam[j], sun_result[j]);
                    stateSort(tight_shadow_cam[j], sun_result[j]);
                }
                LLPipelineFrameContext::getInstance().setShadowPass(false);
            }
            LLRenderPass::freezeAuthorShadowSources(LLPipelineFrameContext::getInstance().getCullResult());
            LLVKLoader::setShadowRecordPhase(true);
            for (int j = 0; j < 4; ++j)
            {
                if (!cascade_valid[j]) { continue; }
                {
                LLRTScope rts(getFrameRT()->sunShadowLayered, DEPTH_LAYER_TAG, j, true, "shadow_layer_fb");
                if (rts)
                {
                getFrameRT()->sunShadowLayered.getViewport(gGLViewport);
                set_current_modelview(view[j]);
                set_current_projection(proj[j]);
                {
                    LLVKLoader::VkPerfShadowCtxScope shadow_ctx_scope(LLVKLoader::VKPERF_SHCTX_FALLBACK);
                    LLVKLoader::gVkPerfShadowMapIndex = (U32)j;
                    ScopedShadowBatchCull cull_scope(cascade_batch_cull_radius[j]);
                    renderShadow(view[j], proj[j], tight_shadow_cam[j], sun_result[j], true, true);
                }
                }
                }
            }
            LLVKLoader::setShadowRecordPhase(false);
            getFrameRT()->sunShadowLayered.bindForShaderRead(0, true);
        }
    }

    //hack to disable projector shadows
    bool gen_shadow = RenderShadowDetail > 1;

    if (gen_shadow)
    {
        if (!gCubeSnapshot) //skip updating spot shadow maps during cubemap updates
        {
            LLTrace::CountStatHandle<>* velocity_stat = LLViewerCamera::getVelocityStat();
            F32 fade_amt = gFrameIntervalSeconds.value()
                * (F32)llmax(LLTrace::get_frame_recording().getLastRecording().getSum(*velocity_stat) / LLTrace::get_frame_recording().getLastRecording().getDuration().value(), 1.0);

            // should never happen
            llassert(mTargetShadowSpotLight[0] != mTargetShadowSpotLight[1] || mTargetShadowSpotLight[0].isNull());

            //update shadow targets
            for (U32 i = 0; i < 2; i++)
            { //for each current shadow
                LLViewerCamera::setCurCameraID((LLViewerCamera::eCameraID)(LLViewerCamera::CAMERA_SPOT_SHADOW0 + i));

                if (mShadowSpotLight[i].notNull() &&
                    (mShadowSpotLight[i] == mTargetShadowSpotLight[0] ||
                        mShadowSpotLight[i] == mTargetShadowSpotLight[1]))
                { //keep this spotlight
                    mSpotLightFade[i] = llmin(mSpotLightFade[i] + fade_amt, 1.f);
                }
                else
                { //fade out this light
                    mSpotLightFade[i] = llmax(mSpotLightFade[i] - fade_amt, 0.f);

                    if (mSpotLightFade[i] == 0.f || mShadowSpotLight[i].isNull())
                    { //faded out, grab one of the pending spots (whichever one isn't already taken)
                        if (mTargetShadowSpotLight[0] != mShadowSpotLight[(i + 1) % 2])
                        {
                            mShadowSpotLight[i] = mTargetShadowSpotLight[0];
                        }
                        else
                        {
                            mShadowSpotLight[i] = mTargetShadowSpotLight[1];
                        }
                    }
                }
            }
        }

        // this should never happen
        llassert(mShadowSpotLight[0] != mShadowSpotLight[1] || mShadowSpotLight[0].isNull());

        for (S32 i = 0; i < 2; i++)
        {
            set_current_modelview(saved_view);
            set_current_projection(saved_proj);

            if (mShadowSpotLight[i].isNull())
            {
                continue;
            }

            LLVOVolume* volume = mShadowSpotLight[i]->getVOVolume();

            if (!volume)
            {
                mShadowSpotLight[i] = NULL;
                continue;
            }

            LLDrawable* drawable = mShadowSpotLight[i];

            LLVector3 params = volume->getSpotLightParams();
            F32 fov = params.mV[0];

            //get agent->light space matrix (modelview)
            LLVector3 center = drawable->getPositionAgent();
            LLQuaternion quat = volume->getRenderRotation();

            //get near clip plane
            LLVector3 scale = volume->getScale();
            LLVector3 at_axis(0, 0, -scale.mV[2] * 0.5f);
            at_axis *= quat;

            LLVector3 np = center + at_axis;
            at_axis.normVec();

            //get origin that has given fov for plane np, at_axis, and given scale
            F32 dist = (scale.mV[1] * 0.5f) / tanf(fov * 0.5f);

            LLVector3 origin = np - at_axis * dist;

            LLMatrix4 mat(quat, LLVector4(origin, 1.f));

            view[i + 4] = glm::make_mat4((F32*)mat.mMatrix);

            view[i + 4] = glm::inverse(view[i + 4]);

            //get perspective matrix
            F32 near_clip = dist + 0.01f;
            F32 width = scale.mV[VX];
            F32 height = scale.mV[VY];
            F32 far_clip = dist + volume->getLightRadius() * 1.5f;

            F32 fovy = fov; // radians
            F32 aspect = width / height;

            proj[i + 4] = glm::perspective(fovy, aspect, near_clip, far_clip);

            set_current_modelview(view[i + 4]);
            set_current_projection(proj[i + 4]);

            const bool spot_mats_finite = shadowMatricesFinite(view[i + 4], proj[i + 4]);
            if (spot_mats_finite)
            {
                mSunShadowMatrix[i + 4] = sGlNdcToSampleBias * proj[i + 4] * view[i + 4] * inv_view;
            }

            set_last_modelview(mShadowModelview[i + 4]);
            set_last_projection(mShadowProjection[i + 4]);

            if (spot_mats_finite)
            {
                mShadowModelview[i + 4] = view[i + 4];
                mShadowProjection[i + 4] = proj[i + 4];
            }

            if (!gCubeSnapshot) //skip updating spot shadow maps during cubemap updates
            {
                LLCamera shadow_cam = camera;
                shadow_cam.setFar(far_clip);
                shadow_cam.setOrigin(origin);

                LLViewerCamera::updateFrustumPlanes(shadow_cam, false, false, true);

                LLViewerCamera::setCurCameraID((LLViewerCamera::eCameraID)(LLViewerCamera::CAMERA_SPOT_SHADOW0 + i));

                LLVKLoader::gVkPerfShadowMapIndex = 4u + (U32)i;

                {
                    LLPipelineFrameContext::getInstance().setShadowPass(true);
                    updateCull(shadow_cam, spot_result[i]);
                    stateSort(shadow_cam, spot_result[i]);
                    LLPipelineFrameContext::getInstance().setShadowPass(false);
                }
                LLRenderPass::freezeAuthorShadowSources(LLPipelineFrameContext::getInstance().getCullResult());
                LLVKLoader::setShadowRecordPhase(true);

                LLRenderTarget& spot_rt = mSpotShadow[i];

                {
                LLRTScope rts(spot_rt, false, "spot_shadow");
                if (rts)
                {
                RenderSpotLight = drawable;
                spot_rt.getViewport(gGLViewport);
                spot_rt.clear();

                {
                    LLVKLoader::VkPerfShadowCtxScope shadow_ctx_scope(LLVKLoader::VKPERF_SHCTX_SPOT);
                    renderShadow(view[i + 4], proj[i + 4], shadow_cam, spot_result[i], false);
                }

                RenderSpotLight = nullptr;
                }
                }
                LLVKLoader::setShadowRecordPhase(false);
                spot_rt.bindForShaderRead(0, true);
            }
        }
    }
    else
    { //no spotlight shadows
        mShadowSpotLight[0] = mShadowSpotLight[1] = NULL;
    }

    if (!CameraOffset)
    {
        set_current_modelview(saved_view);
        set_current_projection(saved_proj);
    }
    else
    {
        set_current_modelview(view[1]);
        set_current_projection(proj[1]);
        gGL.loadMatrix(glm::value_ptr(view[1]));
        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.loadMatrix(glm::value_ptr(proj[1]));
        gGL.matrixMode(LLRender::MM_MODELVIEW);
    }
    gGL.setColorMask(true, true);

    set_last_modelview(last_modelview);
    set_last_projection(last_projection);

    popRenderTypeMask();

    if (!skip_avatar_update)
    {
        gAgentAvatarp->updateAttachmentVisibility(gAgentCamera.getCameraMode());
    }
}

void LLPipeline::skipRenderingShadows()
{
    LLGLDepthTest depth(GL_TRUE);

    for (S32 j = 0; j < 4; j++)
    {
        {
            LLRTScope rts(getFrameRT()->sunShadowLayered, DEPTH_LAYER_TAG, j, false, "skip_shadow");
            if (rts)
            {
                getFrameRT()->sunShadowLayered.clear();
            }
        }
        getFrameRT()->sunShadowLayered.bindForShaderRead(0, true);
    }
}

void LLPipeline::handleShadowDetailChanged()
{
    // <FS:AYAstorm r30 BD full port Phase 3.4> Cinematic では effective 値が BD default に固定
    if (RenderShadowDetail > gSavedSettings.getS32("RenderShadowDetail"))
    // </FS:AYAstorm>
    {
        skipRenderingShadows();
    }
    // else <FS:Beq/> Ghosting fix for Whirly to try. just remove this for now.
    {
        LLReloadQueue::request(LLReloadQueue::RK_Shaders);
    }
}

