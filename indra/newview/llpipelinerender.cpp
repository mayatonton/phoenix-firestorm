/**
 * @file llpipelinerender.cpp
 * @brief Rendering pipeline: geometry render drivers (pure move from pipeline.cpp).
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

static inline void bindHighlightProgram(LLGLSLShader& program)
{
    if ((LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_INTERFACE) > 0))
    {
        program.bind();
        gGL.diffuseColor4f(1, 1, 1, 0.5f);
    }
}

static inline void unbindHighlightProgram(LLGLSLShader& program)
{
    if (LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_INTERFACE) > 0)
    {
        program.unbind();
    }
}

static const char* postDeferredPoolCheckpointLabel(U32 t)
{
    switch (t)
    {
        case LLDrawPool::POOL_ALPHA_PRE_WATER:       return "post:alpha_pre_water";
        case LLDrawPool::POOL_ALPHA_POST_WATER:      return "post:alpha_post_water";
        case LLDrawPool::POOL_WATER:                 return "post:water";
        case LLDrawPool::POOL_FULLBRIGHT:            return "post:fullbright";
        case LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK: return "post:fb_alpha_mask";
        case LLDrawPool::POOL_ALPHA_MASK:            return "post:alpha_mask";
        case LLDrawPool::POOL_GLTF_PBR:              return "post:gltf_pbr";
        case LLDrawPool::POOL_GLTF_PBR_ALPHA_MASK:   return "post:gltf_pbr_amask";
        default:                                     return "post:other";
    }
}

static U32 sIndicesDrawnCount = 0;

// <FS:AYAstorm r30 BD full port Phase 5 R3 (A4)>
// Cinematic mode 判定。R2 で導入した settings_cinematic_bd.xml overlay により
// mode 2 起動時に BD default 値が gSavedSettings に焼き込まれるため、render path
// 側は直接 gSavedSettings.getX() を呼べばよい (P5 step 5 paradigm shift で導入
// した getRenderCvar* helper は R3 で撤去済)。本 isCinematicMode のみ残し、
// Cinematic 専用機能 (Volumetric Lighting / Motion Blur / DoF chain 等) の
// gate 判定に使用する。
bool LLPipeline::isCinematicMode()
{
    static LLCachedControl<U32> aya_view_mode(gSavedSettings, "AYAVisualRealismEnabled", 1);
    return aya_view_mode() == 2;
}

void LLPipeline::renderHighlights()
{
    assertInitialized();

    // Draw 3D UI elements here (before we clear the Z buffer in POOL_HUD)
    // Render highlighted faces.
    LLGLSPipelineAlpha gls_pipeline_alpha;
    disableLights();

    if (hasRenderDebugFeatureMask(RENDER_DEBUG_FEATURE_SELECTED))
    {
        bindHighlightProgram(gHighlightProgram);

        if (sRenderHighlightTextureChannel == LLRender::DIFFUSE_MAP ||
            sRenderHighlightTextureChannel == LLRender::BASECOLOR_MAP ||
            sRenderHighlightTextureChannel == LLRender::METALLIC_ROUGHNESS_MAP ||
            sRenderHighlightTextureChannel == LLRender::GLTF_NORMAL_MAP ||
            sRenderHighlightTextureChannel == LLRender::EMISSIVE_MAP ||
            sRenderHighlightTextureChannel == LLRender::NUM_TEXTURE_CHANNELS)
        {
            static const LLColor4 highlight_selected_color(1.f, 1.f, 1.f, 0.5f);
            renderSelectedFaces(highlight_selected_color);
        }

        // Paint 'em red!
        static const LLColor4 highlight_face_color(1.f, 0.f, 0.f, 0.5f);
        for (auto facep : mHighlightFaces)
        {
            facep->renderSelected(LLViewerTexture::sNullImagep, highlight_face_color);
        }

        unbindHighlightProgram(gHighlightProgram);
    }

    // Contains a list of the faces of objects that are physical or
    // have touch-handlers.
    mHighlightFaces.clear();

    if (hasRenderDebugFeatureMask(RENDER_DEBUG_FEATURE_SELECTED))
    {
        if (sRenderHighlightTextureChannel == LLRender::NORMAL_MAP)
        {
            static const LLColor4 highlight_normal_color(1.0f, 0.5f, 0.5f, 0.5f);
            bindHighlightProgram(gHighlightNormalProgram);
            renderSelectedFaces(highlight_normal_color);
            unbindHighlightProgram(gHighlightNormalProgram);
        }
        else if (sRenderHighlightTextureChannel == LLRender::SPECULAR_MAP)
        {
            static const LLColor4 highlight_specular_color(0.0f, 0.3f, 1.0f, 0.8f);
            bindHighlightProgram(gHighlightSpecularProgram);
            renderSelectedFaces(highlight_specular_color);
            unbindHighlightProgram(gHighlightSpecularProgram);
        }
    }
}

// <AYAstorm r30 P2> Velocity pass (BD lineage). Bind mVelocityMap, clear, run
// each pool's renderMotionBlur(). Step 5 wires the display() callsite; until
// then this stays unreferenced. Pools that don't override the new virtuals
// (Step 4c per-pool override list) contribute zero passes — safe to call
// before any override exists, just produces a cleared RG16F target.
LLRecordPassContext LLPipeline::buildRecordPassContext()
{
    LLPipelineFrameContext& fc = LLPipelineFrameContext::getInstance();
    LLRecordPassContext ctx;
    ctx.shadowPass         = fc.isShadowPass();
    ctx.reflectionPass     = fc.isReflectionPass();
    ctx.impostorPass       = fc.isImpostorPass();
    ctx.hudPass            = fc.isHUDPass();
    ctx.underWater         = fc.isUnderWaterRendering();
    ctx.dofPass            = fc.isDoFPass();
    ctx.cullResult         = fc.getCullResult();
    ctx.activeRT           = fc.getActiveRT();
    ctx.avatarMinimumAlpha = fc.getAvatarMinimumAlpha();
    return ctx;
}

// </AYAstorm r30 P3>


void LLPipeline::renderGeomDeferred(LLCamera& camera, bool do_occlusion)
{
    LLAppViewer::instance()->pingMainloopTimeout("Pipeline:RenderGeomDeferred");
    const LLRecordPassContext ctx = buildRecordPassContext();

    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_GEOMETRY);
    LL_PROFILE_GPU_ZONE("renderGeomDeferred");
    LLVKLoader::gpuCheckpoint("renderGeomDeferred");

    llassert(!isFrameHUDPass());

    if (gUseWireframe)
    {
        LLGLState::setPolygonMode(GL_LINE);
    }

    if (&camera == LLViewerCamera::getInstance())
    {   // a bit hacky, this is the start of the main render frame, figure out delta between last modelview matrix and
        // current modelview matrix
        glm::mat4 last_modelview = get_last_modelview();
        glm::mat4 cur_modelview = get_current_modelview();

        // goal is to have a matrix here that goes from the last frame's camera space to the current frame's camera space
        glm::mat4 m = glm::inverse(last_modelview);  // last camera space to world space
        m = cur_modelview * m; // world space to camera space

        glm::mat4 n = glm::inverse(m);

        gGLDeltaModelView = m;
        gGLInverseDeltaModelView = n;
    }

    if (LLVKLoader::isVulkanInitialized())
    {
        LLVKLoader::GlobalF_PerProgramBind gf = {};
        gf.mirror_flag  = mHeroProbeManager.isMirrorPass() ? 1.f : 0.f;
        gf.clipPlane[0] = LLPipeline::sLastClipPlane.mV[0];
        gf.clipPlane[1] = LLPipeline::sLastClipPlane.mV[1];
        gf.clipPlane[2] = LLPipeline::sLastClipPlane.mV[2];
        gf.clipPlane[3] = LLPipeline::sLastClipPlane.mV[3];
        LLVKLoader::writeCurrentGlobalFUBO(gf);
    }

    bool occlude = getFrameCull()->getUseOcclusion() > 1 && do_occlusion && !LLGLSLShader::sProfileEnabled;

    setupHWLights();

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("deferred pools");

        LLGLEnable cull(GL_CULL_FACE);

        for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
        {
            LLDrawPool *poolp = *iter;
            if (hasRenderType(poolp->getType()))
            {
                poolp->prerender();
            }
        }

        LLVertexBuffer::unbind();


        if (LLViewerShaderMgr::instance()->mShaderLevel[LLViewerShaderMgr::SHADER_DEFERRED] > 1)
        {
            //update reflection probe uniform
            mReflectionMapManager.updateUniforms();
            mHeroProbeManager.updateUniforms();
        }

        U32 cur_type = 0;

        gGL.setColorMask(true, true);

        pool_set_t::iterator iter1 = mPools.begin();

        while ( iter1 != mPools.end() )
        {
            LLDrawPool *poolp = *iter1;

            cur_type = poolp->getType();

            if (occlude && cur_type >= LLDrawPool::POOL_GRASS)
            {
                llassert(!gCubeSnapshot); // never do occlusion culling on cube snapshots
                occlude = false;
                gGLLastMatrix = NULL;
                gGL.loadMatrix(gGLModelView);
                doOcclusion(camera);
            }

            pool_set_t::iterator iter2 = iter1;
            if (hasRenderType(poolp->getType()) && poolp->getNumDeferredPasses() > 0)
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("deferred pool render");

                gGLLastMatrix = NULL;
                gGL.loadMatrix(gGLModelView);

                const bool fam_track = !gCubeSnapshot && !gHeroProbeMirrorRender
                                       && cur_type < LLDrawPool::NUM_POOL_TYPES;
                const U64 fam_t0 = fam_track ? (U64)LLTimer::getTotalTime() : 0;
                const U32 fam_d0 = LLVertexBuffer::sVkDrawCallCount.load();

                for( S32 i = 0; i < poolp->getNumDeferredPasses(); i++ )
                {
                    LLVertexBuffer::unbind();
                    poolp->beginDeferredPass(ctx, i);
                    for (iter2 = iter1; iter2 != mPools.end(); iter2++)
                    {
                        LLDrawPool *p = *iter2;
                        if (p->getType() != cur_type)
                        {
                            break;
                        }

                        if ( !p->getSkipRenderFlag() ) { p->renderDeferred(ctx, i); }
                    }
                    poolp->endDeferredPass(ctx, i);
                    LLVertexBuffer::unbind();

                }

                if (fam_track)
                {
                    LLVKLoader::gVkPerf.fam_us[cur_type] += (U64)LLTimer::getTotalTime() - fam_t0;
                    LLVKLoader::gVkPerf.fam_draws[cur_type] += LLVertexBuffer::sVkDrawCallCount.load() - fam_d0;
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

        if (!gCubeSnapshot)
        {
            static U32 s_draw_total_history[64] = {};
            static U32 s_draw_history_pos       = 0;
            static U32 s_draw_history_filled    = 0;
            static U32 s_deferred_draws_prev    = 0;

            const U32 frame_total = LLVertexBuffer::sVkDrawCallCount - s_deferred_draws_prev;
            s_deferred_draws_prev = LLVertexBuffer::sVkDrawCallCount;

            U64 sum = 0;
            for (U32 i = 0; i < s_draw_history_filled; ++i)
            {
                sum += s_draw_total_history[i];
            }
            const U32 avg = s_draw_history_filled ? (U32)(sum / s_draw_history_filled) : 0;

            if (s_draw_history_filled >= 32 && avg > 100 && frame_total * 5 < avg &&
                getFrameCull()->getVisibleGroupsSize() > 500)
            {
                static U32 s_collapse_count = 0;
                ++s_collapse_count;
                if ((s_collapse_count & (s_collapse_count - 1)) == 0)
                {
                    LL_WARNS("RenderDrop") << "deferred draw collapse:"
                                           << " frame_draws=" << frame_total
                                           << " rolling_avg=" << avg
                                           << " vis_groups=" << getFrameCull()->getVisibleGroupsSize()
                                           << " (count=" << s_collapse_count << ")" << LL_ENDL;
                }
            }

            s_draw_total_history[s_draw_history_pos] = frame_total;
            s_draw_history_pos = (s_draw_history_pos + 1) & 63;
            if (s_draw_history_filled < 64)
            {
                ++s_draw_history_filled;
            }
        }

        gGLLastMatrix = NULL;
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.loadMatrix(gGLModelView);

        gGL.setColorMask(true, false);

    } // Tracy ZoneScoped

    if (gUseWireframe)
    {
        LLGLState::setPolygonMode(GL_FILL);
    }
}

// Render all of our geometry that's required after our deferred pass.
// This is gonna be stuff like alpha, water, etc.
void LLPipeline::renderGeomPostDeferred(LLCamera& camera)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LL_PROFILE_GPU_ZONE("renderGeomPostDeferred");
    const LLRecordPassContext ctx = buildRecordPassContext();

    LLVKLoader::gpuCheckpoint("renderGeomPostDeferred");

    if (gUseWireframe)
    {
        LLGLState::setPolygonMode(GL_LINE);
    }

    U32 cur_type = 0;

    LLGLEnable cull(GL_CULL_FACE);

    bool done_atmospherics = isFrameHUDPass(); //skip atmospherics on huds
    bool done_water_haze = done_atmospherics;
    bool done_water_exclusion = false;
    // <FS:AYAstorm bug fix> SSS dispatch を FB pool より前に動かすため独立 flag を導入。
    bool done_sss = isFrameHUDPass();
    // </FS:AYAstorm>

    // do water exclusion just before water pass.
    U32 water_exclusion_pass = LLDrawPool::POOL_WATEREXCLUSION;

    // do atmospheric haze just before post water alpha
    U32 atmospherics_pass = LLDrawPool::POOL_ALPHA_POST_WATER;

    if (isFrameUnderWaterRendering())
    { // if under water, do atmospherics just before the water pass
        atmospherics_pass = LLDrawPool::POOL_WATER;
    }

    // do water haze just before pre water alpha
    U32 water_haze_pass = LLDrawPool::POOL_ALPHA_PRE_WATER;

    // <FS:AYAstorm bug fix> SSS を FullBright/postDeferred より前で発火させて
    //   FB prim 越しに SSS pink が透ける bug を解消。scene color が FB で
    //   上書きされる前 (softenLight 直後の素 skin 色) を sample させる。
    U32 sss_pass = LLDrawPool::POOL_FULLBRIGHT;
    // </FS:AYAstorm>

    calcNearbyLights(camera);
    setupHWLights();

    gGL.setSceneBlendType(LLRender::BT_ALPHA);
    gGL.setColorMask(true, false);

    pool_set_t::iterator iter1 = mPools.begin();

    // turn off atmospherics and water haze for low detail reflection probe
    static LLCachedControl<S32> probe_level(gSavedSettings, "RenderReflectionProbeLevel", 0);
    bool low_detail_probe = probe_level == 0 && gCubeSnapshot;
    done_atmospherics = done_atmospherics || low_detail_probe;
    done_water_haze   = done_water_haze || low_detail_probe;
    // <FS:AYAstorm bug fix> 旧コードでは SSS が atmospherics と同 block にあったため
    //   low_detail_probe の done_atmospherics 経由で skip されていた。独立 dispatch に
    //   分離した本 fix でも cube snapshot 時の挙動を維持するため done_sss も同様に gate。
    done_sss = done_sss || low_detail_probe;
    // </FS:AYAstorm>

    while ( iter1 != mPools.end() )
    {
        LLDrawPool *poolp = *iter1;

        cur_type = poolp->getType();

        if (cur_type >= water_exclusion_pass && !done_water_exclusion)
        { // do water exclusion against depth buffer before rendering alpha
            doWaterExclusionMask();
            done_water_exclusion = true;
        }

        // <FS:AYAstorm bug fix> SSS を FullBright/postDeferred より前で発火。
        //   旧: atmospherics と同 block (POOL_ALPHA_POST_WATER) で発火、getFrameRT()->screen が
        //       FB で塗られた状態を sample → FB pixel に skin pink shadow が滲む bug。
        //   新: POOL_FULLBRIGHT 到達直前で発火、scene color は softenLight 直後の素 skin 色のまま、
        //       FB が後で覆い被さるので SSS は FB pixel に乗らない。
        //   r20 SSS dispatch 条件は元と同じ (mode > 0 && AYAR20AvatarSkinSSSEnabled)。
        if (cur_type >= sss_pass && !done_sss)
        {
            static LLCachedControl<U32>  aya_view_mode_sss(gSavedSettings, "AYAVisualRealismEnabled", 1);
            static LLCachedControl<bool> aya_r20_enabled_sss(gSavedSettings, "AYAR20AvatarSkinSSSEnabled", false);
            bool dispatch_r20 = (aya_view_mode_sss() > 0) && aya_r20_enabled_sss;
            if (dispatch_r20)
            {
                LLVKLoader::gpuCheckpoint("post:skin_sss");
                doSkinSSS();
            }
            done_sss = true;
        }
        // </FS:AYAstorm>

        if (cur_type >= atmospherics_pass && !done_atmospherics)
        { // do atmospherics against depth buffer before rendering alpha
            LLVKLoader::gpuCheckpoint("post:atmospherics");
            bool scene_depth_ok = doAtmospherics();
            done_atmospherics = true;
            // <FS:AYAstorm r30 BD改善> AYAstorm View は無条件、Cinematic は個別 InCinematic cvar で opt-in。
            //   各関数も自己 gate 済 (Phase 3.1 / r20 早期 return) だが call-site でも wrap して
            //   Cinematic OFF 時の関数 entry を無駄ゼロ化。
            //   r15 Godrays は atmospherics 後の light scattering なので atmospherics ブロックに残置。
            //   r20 SSS は本 fix で FB 前 dispatch に分離済 (上の sss_pass ブロック)。
            static LLCachedControl<U32>  aya_view_mode(gSavedSettings, "AYAVisualRealismEnabled", 1);
            static LLCachedControl<bool> aya_r15_in_cinematic(gSavedSettings, "AYAR15GodraysInCinematicEnabled", false);
            bool dispatch_r15 = ((aya_view_mode() == 1) || (aya_view_mode() == 2 && aya_r15_in_cinematic));
            if (dispatch_r15)
            {
                LLVKLoader::gpuCheckpoint("post:godrays");
                doGodrays(scene_depth_ok);
            }
            // </FS:AYAstorm>
        }

        if (cur_type >= water_haze_pass && !done_water_haze)
        { // do water haze against depth buffer before rendering alpha
            LLVKLoader::gpuCheckpoint("post:water_haze");
            doWaterHaze();
            done_water_haze = true;
        }

        pool_set_t::iterator iter2 = iter1;
        if (hasRenderType(poolp->getType()) && poolp->getNumPostDeferredPasses() > 0)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("deferred poolrender");
            LLVKLoader::gpuCheckpoint(postDeferredPoolCheckpointLabel(cur_type));

            gGLLastMatrix = NULL;
            gGL.loadMatrix(gGLModelView);

            const bool fam_track = !gCubeSnapshot && !gHeroProbeMirrorRender
                                   && cur_type < LLDrawPool::NUM_POOL_TYPES;
            const U64 fam_t0 = fam_track ? (U64)LLTimer::getTotalTime() : 0;
            const U32 fam_d0 = LLVertexBuffer::sVkDrawCallCount.load();

            for( S32 i = 0; i < poolp->getNumPostDeferredPasses(); i++ )
            {
                LLVertexBuffer::unbind();
                poolp->beginPostDeferredPass(ctx, i);
                for (iter2 = iter1; iter2 != mPools.end(); iter2++)
                {
                    LLDrawPool *p = *iter2;
                    if (p->getType() != cur_type)
                    {
                        break;
                    }

                    p->renderPostDeferred(ctx, i);
                }
                poolp->endPostDeferredPass(ctx, i);
                LLVertexBuffer::unbind();
            }

            if (fam_track)
            {
                LLVKLoader::gVkPerf.fam_us[cur_type] += (U64)LLTimer::getTotalTime() - fam_t0;
                LLVKLoader::gVkPerf.fam_draws[cur_type] += LLVertexBuffer::sVkDrawCallCount.load() - fam_d0;
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
    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.loadMatrix(gGLModelView);

    if (!gCubeSnapshot)
    {
        // debug displays
        renderHighlights();
        mHighlightFaces.clear();

        renderDebug();
    }

    if (gUseWireframe)
    {
        LLGLState::setPolygonMode(GL_FILL);
    }
}

void LLPipeline::addTrianglesDrawn(S32 index_count)
{
    sIndicesDrawnCount += index_count;
}

void LLPipeline::recordTrianglesDrawn()
{
    assertInitialized();
    U32 count = sIndicesDrawnCount / 3;
    sIndicesDrawnCount = 0;
    add(LLStatViewer::TRIANGLES_DRAWN, LLUnits::Triangles::fromValue(count));
}

void LLPipeline::renderObjects(const LLRecordPassContext& ctx, U32 type, bool texture, bool batch_texture, bool rigged)
{
    assertInitialized();
    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;

    if (rigged)
    {
        mSimplePool->pushRiggedBatches(ctx, type + 1, texture, batch_texture);
    }
    else
    {
        mSimplePool->pushBatches(ctx, type, texture, batch_texture);
    }

    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;
}

void LLPipeline::renderGLTFObjects(const LLRecordPassContext& ctx, U32 type, bool texture, bool rigged, bool scene_manager)
{
    assertInitialized();
    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;

    if (rigged)
    {
        mSimplePool->pushRiggedGLTFBatches(ctx, type + 1, texture);
    }
    else
    {
        mSimplePool->pushGLTFBatches(ctx, type, texture);
    }

    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;

    if (!scene_manager)
    {
        return;
    }

    if (!rigged)
    {
        LL::GLTFSceneManager::instance().renderOpaque(ctx);
    }
    else
    {
        LL::GLTFSceneManager::instance().render(ctx, true, true);
    }
}

// Currently only used for shadows -Cosmic,2023-04-19
void LLPipeline::renderAlphaObjects(const LLRecordPassContext& ctx, bool rigged, S32 gltf_mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    assertInitialized();
    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;
    U32 target_width = LLRenderTarget::sCurResX;
    U32 type = LLRenderPass::PASS_ALPHA;
    // for gDeferredShadowAlphaMaskProgram
    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin;
    // for gDeferredShadowGLTFAlphaBlendProgram
    const LLVOAvatar* lastAvatarGLTF = nullptr;
    U64 lastMeshIdGLTF = 0;
    bool skipLastSkinGLTF;

    LLVKBucket::forEachSource(ctx, type, [&](LLDrawInfo& params)
    {
        LLDrawInfo* pparams = &params;

        if (rigged != (pparams->mAvatar != nullptr))
        {
            // Pool contains both rigged and non-rigged DrawInfos. Only draw
            // the objects we're interested in in this pass.
            return;
        }

        const bool is_gltf = (pparams->mGLTFMaterial != nullptr);
        if ((gltf_mode == 1 && is_gltf) || (gltf_mode == 2 && !is_gltf))
        {
            return;
        }

        if (rigged)
        {
            if (pparams->mGLTFMaterial)
            {
                gDeferredShadowGLTFAlphaBlendProgram.bind(rigged);
                if (LLVKLoader::isVulkanInitialized())
                {
                    LLVKLoader::ShadowParams_PerShaderBind shadow_params = {};
                    shadow_params.shadow_target_width = (float)target_width;
                    LLVKLoader::writeCurrentShadowParamsUBO(shadow_params);
                }
                LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);
                LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(pparams->mGLTFMaterial->mBaseColor.mV[3]);
                LLRenderPass::pushRiggedGLTFBatch(ctx, *pparams, lastAvatarGLTF, lastMeshIdGLTF, skipLastSkinGLTF);
            }
            else
            {
                gDeferredShadowAlphaBlendProgram.bind(rigged);
                if (LLVKLoader::isVulkanInitialized())
                {
                    LLVKLoader::ShadowParams_PerShaderBind shadow_params = {};
                    shadow_params.shadow_target_width = (float)target_width;
                    LLVKLoader::writeCurrentShadowParamsUBO(shadow_params);
                }
                LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);
                LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(pparams->mObjectAlpha);
                if (mSimplePool->uploadMatrixPalette(pparams->mAvatar, pparams->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
                {
                    mSimplePool->pushBatch(*pparams, true, true);
                }
            }
        }
        else
        {
            if (pparams->mGLTFMaterial)
            {
                gDeferredShadowGLTFAlphaBlendProgram.bind(rigged);
                if (LLVKLoader::isVulkanInitialized())
                {
                    LLVKLoader::ShadowParams_PerShaderBind shadow_params = {};
                    shadow_params.shadow_target_width = (float)target_width;
                    LLVKLoader::writeCurrentShadowParamsUBO(shadow_params);
                }
                LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);
                LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(pparams->mGLTFMaterial->mBaseColor.mV[3]);
                LLRenderPass::pushGLTFBatch(ctx, *pparams);
            }
            else
            {
                gDeferredShadowAlphaBlendProgram.bind(rigged);
                if (LLVKLoader::isVulkanInitialized())
                {
                    LLVKLoader::ShadowParams_PerShaderBind shadow_params = {};
                    shadow_params.shadow_target_width = (float)target_width;
                    LLVKLoader::writeCurrentShadowParamsUBO(shadow_params);
                }
                LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);
                LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(pparams->mObjectAlpha);
                mSimplePool->pushBatch(*pparams, true, true);
            }
        }
    });

    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;
}

// Currently only used for shadows -Cosmic,2023-04-19
void LLPipeline::renderMaskedObjects(const LLRecordPassContext& ctx, U32 type, bool texture, bool batch_texture, bool rigged)
{
    assertInitialized();
    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;
    if (rigged)
    {
        mAlphaMaskPool->pushRiggedMaskBatches(ctx, type+1, texture, batch_texture);
    }
    else
    {
        mAlphaMaskPool->pushMaskBatches(ctx, type, texture, batch_texture);
    }
    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;
}

// Currently only used for shadows -Cosmic,2023-04-19
void LLPipeline::renderFullbrightMaskedObjects(const LLRecordPassContext& ctx, U32 type, bool texture, bool batch_texture, bool rigged)
{
    assertInitialized();
    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;
    if (rigged)
    {
        mFullbrightAlphaMaskPool->pushRiggedMaskBatches(ctx, type+1, texture, batch_texture);
    }
    else
    {
        mFullbrightAlphaMaskPool->pushMaskBatches(ctx, type, texture, batch_texture);
    }
    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;
}

void LLPipeline::renderAlphaObjectsMultiview(const LLRecordPassContext& ctx, bool rigged)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    assertInitialized();

    U32 target_width = LLRenderTarget::sCurResX;
    U32 type = LLRenderPass::PASS_ALPHA;

    {
        gDeferredShadowAlphaBlendMultiviewProgram.bind(rigged);
        if (LLVKLoader::isVulkanInitialized())
        {
            LLVKLoader::ShadowParams_PerShaderBind shadow_params = {};
            shadow_params.shadow_target_width = (float)target_width;
            LLVKLoader::writeCurrentShadowParamsUBO(shadow_params);
        }
        LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);
        LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(-1.f);
        gGL.loadMatrix(gGLModelView);
        gGLLastMatrix = NULL;

        bool mdi_ran = false;
        if (!rigged
            && LLVKBucket::isShadowMdiPass(type)
            && LLVKBucket::emitActive(type)
            && LLVKLoader::isIndirectDrawEnabled()
            && LLGLSLShader::sCurBoundShaderPtr != nullptr
            && LLGLSLShader::sCurBoundShaderPtr->mVkUsesHeapSet
            && LLGLSLShader::sCurBoundShaderPtr->mVkPerDrawSupplySlotComplete
            && !gSnapshot)
        {
            const std::vector<U64>* bits = LLVKBucket::currentVisBits(ctx);
            if (bits != nullptr)
            {
                U64 rec_n = 0;
                U64 dyn_n = 0;
                for (LLVKBucket::Bucket* bucket : LLVKBucket::bucketsForPass(type))
                {
                    mSimplePool->pushIndirectBucket(ctx, *bucket, *bits, true, false, &rec_n, &dyn_n);
                }
                LLVKLoader::gVkPerf.shamdi[3][0] += rec_n;
                LLVKLoader::gVkPerf.shamdi[3][1] += dyn_n;
                mdi_ran = true;
            }
        }

        const LLVOAvatar* lastAvatar = nullptr;
        U64 lastMeshId = 0;
        bool skipLastSkin = false;
        LLVKBucket::forEachSource(ctx, type, [&](LLDrawInfo& params)
        {
            LLDrawInfo* pparams = &params;
            if (rigged != (pparams->mAvatar != nullptr))
            {
                return;
            }
            if (pparams->mGLTFMaterial != nullptr)
            {
                return;
            }
            if (mdi_ran && pparams->mVkTplBucket != nullptr)
            {
                return;
            }
            if (rigged)
            {
                LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(pparams->mObjectAlpha);
                if (mSimplePool->uploadMatrixPalette(pparams->mAvatar, pparams->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
                {
                    mSimplePool->pushBatch(*pparams, true, true);
                }
            }
            else
            {
                ++LLVKLoader::gVkPerf.shamdi[3][2];
                mSimplePool->pushBatch(*pparams, true, true);
            }
        });
    }

    {
        gDeferredShadowGLTFAlphaBlendMultiviewProgram.bind(rigged);
        if (LLVKLoader::isVulkanInitialized())
        {
            LLVKLoader::ShadowParams_PerShaderBind shadow_params = {};
            shadow_params.shadow_target_width = (float)target_width;
            LLVKLoader::writeCurrentShadowParamsUBO(shadow_params);
        }
        LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);
        gGL.loadMatrix(gGLModelView);
        gGLLastMatrix = NULL;

        const LLVOAvatar* lastAvatarGLTF = nullptr;
        U64 lastMeshIdGLTF = 0;
        bool skipLastSkinGLTF = false;
        LLVKBucket::forEachSource(ctx, type, [&](LLDrawInfo& params)
        {
            LLDrawInfo* pparams = &params;
            if (rigged != (pparams->mAvatar != nullptr))
            {
                return;
            }
            if (pparams->mGLTFMaterial == nullptr)
            {
                return;
            }
            LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(pparams->mGLTFMaterial->mBaseColor.mV[3]);
            if (rigged)
            {
                LLRenderPass::pushRiggedGLTFBatch(ctx, *pparams, lastAvatarGLTF, lastMeshIdGLTF, skipLastSkinGLTF);
            }
            else
            {
                LLRenderPass::pushGLTFBatch(ctx, *pparams);
            }
        });
    }

    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;
}

void LLPipeline::renderHighlight(const LLViewerObject* obj, F32 fade)
{
    if (obj && obj->getVolume())
    {
        for (LLViewerObject::child_list_t::const_iterator iter = obj->getChildren().begin(); iter != obj->getChildren().end(); ++iter)
        {
            renderHighlight(*iter, fade);
        }

        LLDrawable* drawable = obj->mDrawable;
        if (drawable)
        {
            for (S32 i = 0; i < drawable->getNumFaces(); ++i)
            {
                LLFace* face = drawable->getFace(i);
                if (face)
                {
                    face->renderSelected(LLViewerTexture::sNullImagep, LLColor4(1,1,1,fade));
                }
            }
        }
    }
}

