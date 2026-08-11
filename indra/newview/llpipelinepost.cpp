/**
 * @file llpipelinepost.cpp
 * @brief Rendering pipeline: post-process (pure move from pipeline.cpp).
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
#include "SMAAAreaTex.h"
#include "SMAASearchTex.h"
#include "llerror.h"
#ifndef LL_WINDOWS
#define A_GCC 1
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#if LL_LINUX
#pragma GCC diagnostic ignored "-Wrestrict"
#endif
#endif
#define A_CPU 1
#include "app_settings/shaders/class1/deferred/CASF.glsl" // This is also C++
// PCH 経由で Xlib (X11/Xlib.h:84) の `#define None 0L` が流入し、enum class member
// 等の `None` トークンを数値リテラル `0L` に置換してしまう。pipeline.cpp は X11
// API を直接呼ばないのでファイル冒頭で undef して局所的に無効化する。
// memory: project_linux_xlib_status_define_trap.md
#undef None

#include "llpipelineinternal.h"

static void notifyMotionBlurSkippedOnce(const std::string& message)
{
    static bool sNotified = false;
    if (sNotified)
    {
        return;
    }

    sNotified = true;

    LLSD args;
    args["MESSAGE"] = message;
    LLNotificationsUtil::add("ChatSystemMessageTip", args);
}

static void notifyDoFSkippedOnce(const std::string& message)
{
    static bool sNotified = false;
    if (sNotified)
    {
        return;
    }

    sNotified = true;

    LLSD args;
    args["MESSAGE"] = message;
    LLNotificationsUtil::add("ChatSystemMessageTip", args);
}

static LLTrace::BlockTimerStatHandle FTM_RENDER_BLOOM("Bloom");

void LLPipeline::renderGeomMotionBlur()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LL_PROFILE_GPU_ZONE("renderGeomMotionBlur");
    const LLRecordPassContext ctx = buildRecordPassContext();


    if (!mVelocityMap.isComplete())
    {
        return;
    }

    auto is_complete_with_rigged_variant = [](const LLGLSLShader& shader)
    {
        return shader.isComplete() &&
            (!shader.mRiggedVariant || shader.mRiggedVariant->isComplete());
    };

    if (!is_complete_with_rigged_variant(gVelocityProgram) ||
        !is_complete_with_rigged_variant(gVelocityAlphaProgram) ||
        !gAvatarVelocityProgram.isComplete())
    {
        LL_WARNS_ONCE("Pipeline") << "Skipping motion blur velocity pass because one or more velocity shaders failed to link." << LL_ENDL;
        notifyMotionBlurSkippedOnce("AYAstorm: Motion blur was disabled because a required velocity shader failed to load. Check AYAstorm.log for shader details.");
        return;
    }

    {
        LLRTScope rts(mVelocityMap, false, "geom_motionblur");
        if (rts)
        {
            mVelocityMap.clear(GL_COLOR_BUFFER_BIT);

            gGL.setColorMask(true, true);
            LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_LEQUAL);

            sVelocityRender = true;

            for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
            {
                LLDrawPool* poolp = *iter;
                S32 num_passes = poolp->getNumMotionBlurPasses();
                for (S32 i = 0; i < num_passes; ++i)
                {
                    poolp->beginMotionBlurPass(ctx, i);
                    poolp->renderMotionBlur(ctx, i);
                    poolp->endMotionBlurPass(ctx, i);
                }
            }

            sVelocityRender = false;
        }
    }
}

// <AYAstorm r30 P2 step 5b> Motion blur composite (BD lineage). Samples diffuseRect
// along the per-pixel velocity vector, 32-tap triangle-weighted. Strength = max blur
// length in pixels; 0 = effectively disabled (gated upstream).
void LLPipeline::renderMotionBlurComposite(LLRenderTarget* src, LLRenderTarget* dst)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    LL_PROFILE_GPU_ZONE("motion blur composite");

    if (!gDeferredMotionBlurProgram.isComplete())
    {
        LL_WARNS_ONCE("Pipeline") << "Skipping motion blur composite because the deferred motion blur shader failed to link." << LL_ENDL;
        notifyMotionBlurSkippedOnce("AYAstorm: Motion blur was disabled because the motion blur composite shader failed to load. Check AYAstorm.log for shader details.");
        return;
    }

    {
        LLRTScope rts(*dst, false, "motionblur_composite");
        if (rts)
        {
            gDeferredMotionBlurProgram.bind();


            gDeferredMotionBlurProgram.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, src);
            gDeferredMotionBlurProgram.bindTexture(LLShaderMgr::DEFERRED_VELOCITY, &mVelocityMap);

            static LLCachedControl<S32> blur_strength(gSavedSettings, "RenderMotionBlurStrength", 32);

            if (LLVKLoader::isVulkanInitialized()
                && gDeferredMotionBlurProgram.mVkPerProgramUBOMapped != nullptr
                && gDeferredMotionBlurProgram.mVkPerProgramUBOSize >= sizeof(LLVKLoader::MotionBlurF_PerProgramBind))
            {
                LLVKLoader::MotionBlurF_PerProgramBind ubo_data{};
                ubo_data._mbF_screen_res[0]        = (F32)src->getWidth();
                ubo_data._mbF_screen_res[1]        = (F32)src->getHeight();
                ubo_data._mbF_motion_blur_strength = (S32)blur_strength;
                ubo_data._motionBlurF_pad0         = 0;
                std::memcpy(gDeferredMotionBlurProgram.vkPerProgramBaseWritePtr(),
                            &ubo_data,
                            sizeof(ubo_data));
            }

            mScreenTriangleVB->setBuffer();
            mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

            gDeferredMotionBlurProgram.unbind();
        }
    }
}

void LLPipeline::compositeForwardFlip()
{
    if (!mForwardColor.wasWrittenThisFrame())
    {
        return;
    }
    if (!mForwardColor.isComplete() || !gAYAForwardFlipCompositeProgram.isComplete())
    {
        return;
    }

    LL_PROFILE_GPU_ZONE("aya forward flip composite");

    {
        LLRTScope rts(getFrameRT()->screen, false, "forward_flip");
        if (rts)
        {
            gGL.setColorMask(true, false);

            LLGLEnable blend_on(GL_BLEND);
            gGL.blendFunc(LLRender::BF_ONE, LLRender::BF_ONE_MINUS_SOURCE_ALPHA,
                          LLRender::BF_ONE, LLRender::BF_ONE_MINUS_SOURCE_ALPHA);

            gAYAForwardFlipCompositeProgram.bind();

            gAYAForwardFlipCompositeProgram.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, &mForwardColor, false, LLTexUnit::TFO_POINT);

            {
                LLGLDepthTest depth_test(GL_FALSE, GL_FALSE);
                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            }

            gAYAForwardFlipCompositeProgram.unbind();

            gGL.blendFunc(LLRender::BF_ONE, LLRender::BF_ZERO,
                          LLRender::BF_ONE, LLRender::BF_ZERO);
            gGL.setColorMask(true, true);
        }
    }
}

void LLPipeline::visualizeBuffers(LLRenderTarget* src, LLRenderTarget* dst, U32 bufferIndex)
{
    {
        LLRTScope rts(*dst, false, "visualize_buffers");
        if (rts)
        {
            gDeferredBufferVisualProgram.bind();
            gDeferredBufferVisualProgram.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, src, false, LLTexUnit::TFO_BILINEAR, bufferIndex);

            if (LLVKLoader::isVulkanInitialized()
                && gDeferredBufferVisualProgram.mVkPerProgramUBO != VK_NULL_HANDLE
                && gDeferredBufferVisualProgram.mVkPerProgramUBOMapped != nullptr)
            {
                LLVKLoader::PostVisualizeBuffers_PerProgramBind ubo_data = {};
                ubo_data.mipLevel = (RenderBufferVisualization != 4) ? 0.f : 8.f;
                std::memcpy(gDeferredBufferVisualProgram.vkPerProgramBaseWritePtr(), &ubo_data,
                            llmin((U32)sizeof(ubo_data), gDeferredBufferVisualProgram.mVkPerProgramUBOSize));
            }

            mScreenTriangleVB->setBuffer();
            mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            gDeferredBufferVisualProgram.unbind();
        }
    }
}

void LLPipeline::generateLuminance(LLRenderTarget* src, LLRenderTarget* dst)
{
    // luminance sample and mipmap generation
    {
        LL_PROFILE_GPU_ZONE("luminance sample");

        {
            LLRTScope rts(*dst, false, "luminance");
            if (rts)
            {
                LLGLDepthTest depth(GL_FALSE, GL_FALSE);

                gLuminanceProgram.bind();

                static LLCachedControl<F32> diffuse_luminance_scale(gSavedSettings, "RenderDiffuseLuminanceScale", 1.0f);

                S32 channel = 0;
                channel = gLuminanceProgram.enableTexture(LLShaderMgr::DEFERRED_DIFFUSE);
                if (channel > -1)
                {
                    src->bindTexture(0, channel, LLTexUnit::TFO_POINT);
                }

                channel = gLuminanceProgram.enableTexture(LLShaderMgr::DEFERRED_EMISSIVE);
                if (channel > -1)
                {
                    mGlow[1].bindTexture(0, channel);
                }

                channel = gLuminanceProgram.enableTexture(LLShaderMgr::NORMAL_MAP);
                if (channel > -1)
                {
                    // bind the normal map to get the environment mask
                    getFrameRT()->deferredScreen.bindTexture(2, channel, LLTexUnit::TFO_POINT);
                }

                if (LLVKLoader::isVulkanInitialized()
                    && gLuminanceProgram.mVkPerProgramUBO != VK_NULL_HANDLE
                    && gLuminanceProgram.mVkPerProgramUBOMapped != nullptr)
                {
                    LLVKLoader::LuminanceF_PerProgramBind ubo_data = {};
                    ubo_data.diffuse_luminance_scale = (F32)diffuse_luminance_scale;
                    std::memcpy(gLuminanceProgram.vkPerProgramBaseWritePtr(), &ubo_data,
                                llmin((U32)sizeof(ubo_data), gLuminanceProgram.mVkPerProgramUBOSize));
                }

                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            }
        }

        // note -- unbind AFTER the glGenerateMipMap so time in generatemipmap can be profiled under "Luminance"
        // also note -- keep an eye on the performance of glGenerateMipmap, might need to replace it with a mip generation shader
        gLuminanceProgram.unbind();
    }
}

void LLPipeline::generateExposure(LLRenderTarget* src, LLRenderTarget* dst, bool use_history) {
    // exposure sample
    {
        LL_PROFILE_GPU_ZONE("exposure sample");

        if (use_history)
        {
            // copy last frame's exposure into mLastExposure
            {
                LLRTScope rts(mLastExposure, false, "exposure_history");
                if (rts)
                {
                    gCopyProgram.bind();
                    gGL.getTexUnit(0)->bind(dst);

                    if (LLVKLoader::isVulkanInitialized())
                    {
                        dst->bindForShaderRead();
                    }

                    mScreenTriangleVB->setBuffer();
                    mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
                }
            }
        }

        {
        LLRTScope rts(*dst, false, "exposure");
        if (rts)
        {

        LLGLDepthTest depth(GL_FALSE, GL_FALSE);

        LLGLSLShader* shader;
        if (use_history)
        {
            shader = &gExposureProgram;
        }
        else
        {
            shader = &gExposureProgramNoFade;
        }

        shader->bind();

        S32 channel = shader->enableTexture(LLShaderMgr::DEFERRED_EMISSIVE);
        if (channel > -1)
        {
            src->bindTexture(0, channel, LLTexUnit::TFO_TRILINEAR);
        }

        if (use_history)
        {
            channel = shader->enableTexture(LLShaderMgr::EXPOSURE_MAP);
            if (channel > -1)
            {
                mLastExposure.bindTexture(0, channel);
            }
        }

        static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
        static LLCachedControl<bool> dynamic_exposure_enabled(gSavedSettings, "RenderDynamicExposureEnabled", true);
        static LLCachedControl<F32> dynamic_exposure_coefficient(gSavedSettings, "RenderDynamicExposureCoefficient", 0.175f);
        static LLCachedControl<F32> dynamic_exposure_speed_error(gSavedSettings, "RenderDynamicExposureSpeedError", 0.1f);
        static LLCachedControl<F32> dynamic_exposure_speed_target(gSavedSettings, "RenderDynamicExposureSpeedTarget", 2.f);

        LLSettingsSky::ptr_t sky = LLEnvironment::instance().getCurrentSky();

        F32 probe_ambiance = LLEnvironment::instance().getCurrentSky()->getReflectionProbeAmbiance(should_auto_adjust());

        F32 exp_min = 1.f;
        F32 exp_max = 1.f;

        static LLCachedControl<bool> use_exposure_sky_settings(gSavedSettings, "RenderUseExposureSkySettings", false);

        if (use_exposure_sky_settings)
        {
            if (dynamic_exposure_enabled)
            {
                exp_min = sky->getHDROffset(should_auto_adjust()) - sky->getHDRMin(should_auto_adjust());
                exp_max = sky->getHDROffset(should_auto_adjust()) + sky->getHDRMax(should_auto_adjust());
            }
            else
            {
                exp_min = sky->getHDROffset(should_auto_adjust());
                exp_max = sky->getHDROffset(should_auto_adjust());
            }
        }
        else if (dynamic_exposure_enabled)
        {
            if (probe_ambiance > 0.f)
            {
                F32 hdr_scale = sqrtf(LLEnvironment::instance().getCurrentSky()->getGamma()) * 2.f;

                if (hdr_scale > 1.f)
                {
                    exp_min = 1.f / hdr_scale;
                    exp_max = hdr_scale;
                }
            }
        }

        if (LLVKLoader::isVulkanInitialized()
            && shader->mVkPerProgramUBO != VK_NULL_HANDLE
            && shader->mVkPerProgramUBOMapped != nullptr)
        {
            LLVKLoader::ExposureF_PerProgramBind ubo_data = {};
            ubo_data.dynamic_exposure_params[0]  = (F32)dynamic_exposure_coefficient;
            ubo_data.dynamic_exposure_params[1]  = exp_min;
            ubo_data.dynamic_exposure_params[2]  = exp_max;
            ubo_data.dynamic_exposure_params[3]  = (F32)dynamic_exposure_speed_error;
            ubo_data.dynamic_exposure_params2[0] = sky->getHDROffset(should_auto_adjust());
            ubo_data.dynamic_exposure_params2[1] = exp_min;
            ubo_data.dynamic_exposure_params2[2] = exp_max;
            ubo_data.dynamic_exposure_params2[3] = (F32)dynamic_exposure_speed_target;
            ubo_data.dt                          = gFrameIntervalSeconds;
            std::memcpy(shader->vkPerProgramBaseWritePtr(), &ubo_data,
                        llmin((U32)sizeof(ubo_data), shader->mVkPerProgramUBOSize));
        }

        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

        if (use_history)
        {
            gGL.getTexUnit(channel)->unbind(mLastExposure.getUsage());
        }
        shader->unbind();
        }
        }
    }
}

void LLPipeline::tonemap(LLRenderTarget* src, LLRenderTarget* dst, bool gamma_correct)
{
    LL_PROFILE_GPU_ZONE("tonemap");

    {
    LLRTScope rts(*dst, false, "tonemap");
    if (rts)
    {
    LLVKLoader::gpuCheckpoint("tm:bind");
    // gamma correct lighting
    {
        static LLCachedControl<bool> buildNoPost(gSavedSettings, "RenderDisablePostProcessing", false);

        LLGLDepthTest depth(GL_FALSE, GL_FALSE);

        // Apply gamma correction to the frame here.

        static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);

        LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();

        static LLCachedControl<U32> tonemap_type_setting(gSavedSettings, "RenderTonemapType", 0U);

        bool no_post = gSnapshotNoPost
            || (psky->getReflectionProbeAmbiance(should_auto_adjust) == 0.f)
            || (buildNoPost && gFloaterTools && gFloaterTools->isAvailable());
        LLGLSLShader* shader = nullptr;
        if(gamma_correct)
        {
            bool legacy_gamma = psky->getReflectionProbeAmbiance(should_auto_adjust) == 0.f;
            if(legacy_gamma)
            {
                shader = no_post ? &gNoPostTonemapLegacyGammaCorrectProgram : &gDeferredPostTonemapLegacyGammaCorrectProgram;
            }
            else
            {
                shader = no_post ? &gNoPostTonemapGammaCorrectProgram : &gDeferredPostTonemapGammaCorrectProgram;
            }
        }
        else
        {
            shader = no_post ? &gNoPostTonemapProgram : &gDeferredPostTonemapProgram;
        }

        shader->bind();

        S32 channel = 0;

        shader->bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, src, false, LLTexUnit::TFO_POINT);
        LLVKLoader::gpuCheckpoint("tm:src_bound");

        shader->bindTexture(LLShaderMgr::EXPOSURE_MAP, &mExposureMap);
        LLVKLoader::gpuCheckpoint("tm:exp_bound");

        static LLCachedControl<F32> exposure(gSavedSettings, "RenderExposure", 1.f);

        F32 e = llclamp(exposure(), 0.5f, 4.f);

        F32 mix_val = psky->getTonemapMix(should_auto_adjust());

        if (LLVKLoader::isVulkanInitialized())
        {
            LLVKLoader::TonemapUtilF_PerProgramBind tu = {};
            tu.exposure     = e;
            tu.tonemap_mix  = mix_val;
            tu.tonemap_type = (S32)tonemap_type_setting;
            LLVKLoader::writeCurrentTonemapUtilFUBO(tu);
        }

        // <FS:AYAstorm r30 BD full port Phase 3.7 cat 01> AY r14+ color
        // correction uniform を Cinematic では BD parity NO-OP に固定。
        // shader 側は uniform 受領は必須 (BD でも tonemap shader が宣言)
        // のため値で no-op (saturation/contrast=1.0, brightness/temperature=0.0)。
        // spec §3.1 phase3.5-ay-only Category A。
        // Reload 3D LUT if setting changed
        {
            // <FS:AYAstorm r30 BD full port Phase 3.7 cat 01> Cinematic では
            // LUT name を空 string (= LUT load しない) に強制。
            const std::string lut_name = gSavedSettings.getString("RenderColorGradingLUTName");
            if (mColorGradingLUT.isNull() || lut_name != mColorGradingLUTName)
                loadColorGradingLUT(lut_name);
            // </FS:AYAstorm>
        }

        S32 lut_channel = shader->enableTexture(LLShaderMgr::COLOR_GRADING_LUT, LLTexUnit::TT_TEXTURE_3D);
        if (lut_channel > -1)
            gGL.getTexUnit(lut_channel)->bind(mColorGradingLUT);
        // </FS:AYAstorm>

        if (LLVKLoader::isVulkanInitialized()
            && shader->mVkPerProgramUBO != VK_NULL_HANDLE
            && shader->mVkPerProgramUBOMapped != nullptr)
        {
            LLVKLoader::PostTonemap_PerProgramBind ubo_data = {};
            ubo_data.color_saturation            = gSavedSettings.getF32("RenderColorSaturation");
            ubo_data.color_contrast              = gSavedSettings.getF32("RenderColorContrast");
            ubo_data.color_temperature           = gSavedSettings.getF32("RenderColorTemperature");
            ubo_data.color_brightness            = gSavedSettings.getF32("RenderColorBrightness");
            ubo_data.color_grading_lut_intensity = gSavedSettings.getF32("RenderColorGradingLUTIntensity");
            ubo_data.color_grading_lut_enabled   = mColorGradingLUTValid ? 1 : 0;
            ubo_data.gamma                       = gamma_correct ? (F32)psky->getGamma() : 0.f;
            std::memcpy(shader->vkPerProgramBaseWritePtr(), &ubo_data,
                        llmin((U32)sizeof(ubo_data), shader->mVkPerProgramUBOSize));
        }

        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
        LLVKLoader::gpuCheckpoint("tm:draw");

        if (lut_channel > -1)
            gGL.getTexUnit(lut_channel)->unbind(LLTexUnit::TT_TEXTURE_3D);
        gGL.getTexUnit(channel)->unbind(src->getUsage());
        shader->unbind();
    }
    }
    }
    LLVKLoader::gpuCheckpoint("tm:flush");
}

void LLPipeline::gammaCorrect(LLRenderTarget* src, LLRenderTarget* dst)
{
    LL_PROFILE_GPU_ZONE("gamma correct");

    {
    LLRTScope rts(*dst, false, "gamma_correct");
    if (rts)
    {
    // gamma correct lighting
    {
        LLGLDepthTest depth(GL_FALSE, GL_FALSE);

        static LLCachedControl<bool> buildNoPost(gSavedSettings, "RenderDisablePostProcessing", false);
        static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);

        LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();
        LLGLSLShader& shader = psky->getReflectionProbeAmbiance(should_auto_adjust) == 0.f ? gLegacyPostGammaCorrectProgram :
            gDeferredPostGammaCorrectProgram;

        shader.bind();

        if (LLVKLoader::isVulkanInitialized()
            && shader.mVkPerProgramUBOMapped != nullptr
            && shader.mVkPerProgramUBOSize >= sizeof(LLVKLoader::PostGammaCorrect_PerProgramBind))
        {
            LLVKLoader::PostGammaCorrect_PerProgramBind ubo_data{};
            ubo_data.gamma                  = (F32)psky->getGamma();
            ubo_data._postGammaCorrect_pad0 = 0.f;
            ubo_data._postGammaCorrect_pad1 = 0.f;
            ubo_data._postGammaCorrect_pad2 = 0.f;
            std::memcpy(shader.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
        }

        shader.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, src, false, LLTexUnit::TFO_POINT);

        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

        shader.unbind();
    }
    }
    }
}

void LLPipeline::copyScreenSpaceReflections(LLRenderTarget* src, LLRenderTarget* dst)
{

    if (RenderScreenSpaceReflections && !gCubeSnapshot)
    {
        LL_PROFILE_GPU_ZONE("ssr copy");
        LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_ALWAYS);

        LLRenderTarget& depth_src = getFrameRT()->deferredScreen;

        {
            LLRTScope rts(*dst, false, "ssr_copy");
            if (rts)
            {
                dst->clear();
                gCopyDepthProgram.bind();

                S32 diff_map = gCopyDepthProgram.getTextureChannel(LLShaderMgr::DIFFUSE_MAP);
                S32 depth_map = gCopyDepthProgram.getTextureChannel(LLShaderMgr::DEFERRED_DEPTH);

                gGL.getTexUnit(diff_map)->bind(src);
                gGL.getTexUnit(depth_map)->bind(&depth_src, true);

                if (LLVKLoader::isVulkanInitialized())
                {
                    src->bindForShaderRead();
                    depth_src.bindForShaderRead(0, true);
                }

                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            }
        }
    }
}

void LLPipeline::generateGlow(LLRenderTarget* src)
{
    LL_PROFILE_GPU_ZONE("glow generate");
    if (isFrameRenderingGlow())
    {
        {
        LLRTScope rts(mGlow[2], false, "glow_extract");
        if (rts)
        {
        mGlow[2].clear();

        gGlowExtractProgram.bind();
        F32 maxAlpha = RenderGlowMaxExtractAlpha;
        F32 warmthAmount = RenderGlowWarmthAmount;
        LLVector3 lumWeights = RenderGlowLumWeights;
        LLVector3 warmthWeights = RenderGlowWarmthWeights;

        if (RenderGlowNoise)
        {
            S32 channel = gGlowExtractProgram.enableTexture(LLShaderMgr::GLOW_NOISE_MAP);
            if (channel > -1)
            {
                gGL.getTexUnit(channel)->bind(mTrueNoiseMap);
                gGL.getTexUnit(channel)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
            }
        }

        {
            LLGLEnable blend_on(GL_BLEND);

            gGL.setSceneBlendType(LLRender::BT_ADD_WITH_ALPHA);

            gGlowExtractProgram.bindTexture(LLShaderMgr::DIFFUSE_MAP, src);

            gGL.color4f(1, 1, 1, 1);
            gPipeline.enableLightsFullbright();

            if (LLVKLoader::isVulkanInitialized()
                && gGlowExtractProgram.mVkPerProgramUBO != VK_NULL_HANDLE
                && gGlowExtractProgram.mVkPerProgramUBOMapped != nullptr)
            {
                src->bindForShaderRead();

                LLVKLoader::GlowExtract_PerProgramBind ubo_data = {};
                ubo_data.lumWeights[0]     = lumWeights.mV[0];
                ubo_data.lumWeights[1]     = lumWeights.mV[1];
                ubo_data.lumWeights[2]     = lumWeights.mV[2];
                ubo_data.minLuminance      = isCinematicMode() ? RenderGlowMinLuminance : 9999.f;
                ubo_data.warmthWeights[0]  = warmthWeights.mV[0];
                ubo_data.warmthWeights[1]  = warmthWeights.mV[1];
                ubo_data.warmthWeights[2]  = warmthWeights.mV[2];
                ubo_data.maxExtractAlpha   = maxAlpha;
                ubo_data.warmthAmount      = warmthAmount;
                ubo_data.screen_res[0]     = (F32)mGlow[2].getWidth();
                ubo_data.screen_res[1]     = (F32)mGlow[2].getHeight();

                memcpy(gGlowExtractProgram.vkPerProgramBaseWritePtr(), &ubo_data,
                       llmin((U32)sizeof(ubo_data), gGlowExtractProgram.mVkPerProgramUBOSize));
            }

            mScreenTriangleVB->setBuffer();
            mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
        }
        }
        }

        gGlowExtractProgram.unbind();

        // power of two between 1 and 1024
        U32 glowResPow = RenderGlowResolutionPow;
        const U32 glow_res = llmax(1, llmin(1024, 1 << glowResPow));

        S32 kernel = RenderGlowIterations * 2;
        F32 delta = RenderGlowWidth / glow_res;
        // Use half the glow width if we have the res set to less than 9 so that it looks
        // almost the same in either case.
        if (glowResPow < 9)
        {
            delta *= 0.5f;
        }
        F32 strength = RenderGlowStrength;

        gGlowProgram.bind();

        for (S32 i = 0; i < kernel; i++)
        {
            {
            LLRTScope rts(mGlow[i % 2], false, "glow_blur");
            if (rts)
            {
            mGlow[i % 2].clear();

            LLRenderTarget* glow_src = (i == 0) ? &mGlow[2] : &mGlow[(i - 1) % 2];

            if (i == 0)
            {
                gGlowProgram.bindTexture(LLShaderMgr::DIFFUSE_MAP, &mGlow[2]);
            }
            else
            {
                gGlowProgram.bindTexture(LLShaderMgr::DIFFUSE_MAP, &mGlow[(i - 1) % 2]);
            }

            F32 delta_x;
            F32 delta_y;
            if (i % 2 == 0)
            {
                delta_x = delta;
                delta_y = 0.f;
            }
            else
            {
                delta_x = 0.f;
                delta_y = delta;
            }
            if (LLVKLoader::isVulkanInitialized()
                && gGlowProgram.mVkPerProgramUBO != VK_NULL_HANDLE
                && gGlowProgram.mVkPerProgramUBOMapped != nullptr)
            {
                glow_src->bindForShaderRead();

                LLVKLoader::Glow_PerProgramBind ubo_data = {};
                ubo_data.glowDelta[0] = delta_x;
                ubo_data.glowDelta[1] = delta_y;
                ubo_data.glowStrength = strength;
                gGlowProgram.rotatePerProgramUBOSlot();
                memcpy(gGlowProgram.vkPerProgramActiveWritePtr(), &ubo_data, sizeof(ubo_data));
            }

            mScreenTriangleVB->setBuffer();
            mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            }
            }
        }

        gGlowProgram.unbind();

    }
    else // !isFrameRenderingGlow(), skip the glow ping-pong and just clear the result target
    {
        {
            LLRTScope rts(mGlow[1], false, "glow_clear");
            if (rts)
            {
                gGL.setClearColor(0.f, 0.f, 0.f, 0.f);
                mGlow[1].clear(GL_COLOR_BUFFER_BIT);
            }
        }
    }
}

void LLPipeline::applyCAS(LLRenderTarget* src, LLRenderTarget* dst)
{
    static LLCachedControl<F32> cas_sharpness(gSavedSettings, "RenderCASSharpness", 0.4f);
    LL_PROFILE_GPU_ZONE("cas");
    if (cas_sharpness == 0.0f || !gCASProgram.isComplete() || !gCASLegacyGammaProgram.isComplete())
    {
        gPipeline.copyRenderTarget(src, dst);
        return;
    }

    LLGLSLShader* sharpen_shader = &gCASProgram;
    static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);

    LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();
    bool legacy_gamma = psky->getReflectionProbeAmbiance(should_auto_adjust) == 0.f;
    if(legacy_gamma)
    {
        sharpen_shader = &gCASLegacyGammaProgram;
    }

    // Bind setup:
    {
    LLRTScope rts(*dst, false, "cas");
    if (rts)
    {
    LLVKLoader::gpuCheckpoint("cas:bind");

    sharpen_shader->bind();

    {
        varAU4(const0);
        varAU4(const1);
        CasSetup(const0, const1,
            cas_sharpness(),             // Sharpness tuning knob (0.0 to 1.0).
            (AF1)src->getWidth(), (AF1)src->getHeight(),  // Input size.
            (AF1)dst->getWidth(), (AF1)dst->getHeight()); // Output size.

        if (LLVKLoader::isVulkanInitialized()
            && sharpen_shader->mVkPerProgramUBO != VK_NULL_HANDLE
            && sharpen_shader->mVkPerProgramUBOMapped != nullptr)
        {
            src->bindForShaderRead();

            LLVKLoader::CasF_PerProgramBind ubo_data = {};
            ubo_data.out_screen_res_uniform[0] = (F32)dst->getWidth();
            ubo_data.out_screen_res_uniform[1] = (F32)dst->getHeight();
            ubo_data.cas_param_0_uniform[0]    = const0[0];
            ubo_data.cas_param_0_uniform[1]    = const0[1];
            ubo_data.cas_param_0_uniform[2]    = const0[2];
            ubo_data.cas_param_0_uniform[3]    = const0[3];
            ubo_data.cas_param_1_uniform[0]    = const1[0];
            ubo_data.cas_param_1_uniform[1]    = const1[1];
            ubo_data.cas_param_1_uniform[2]    = const1[2];
            ubo_data.cas_param_1_uniform[3]    = const1[3];
            ubo_data.gamma = (F32)psky->getGamma();
            memcpy(sharpen_shader->vkPerProgramBaseWritePtr(), &ubo_data,
                   llmin((U32)sizeof(ubo_data), sharpen_shader->mVkPerProgramUBOSize));
        }
    }

    sharpen_shader->bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, src, false, LLTexUnit::TFO_POINT);

    // Draw
    gPipeline.mScreenTriangleVB->setBuffer();
    gPipeline.mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
    LLVKLoader::gpuCheckpoint("cas:draw");

    sharpen_shader->unbind();

    }
    }
    LLVKLoader::gpuCheckpoint("cas:flush");
}

void LLPipeline::applyFXAA(LLRenderTarget* src, LLRenderTarget* dst)
{
    LL_PROFILE_GPU_ZONE("FXAA");
    {
        llassert(!gCubeSnapshot);
        bool multisample = RenderFSAAType == 1 && gFXAAProgram[0].isComplete() && mFXAAMap.isComplete();

        // Present everything.
        if (multisample)
        {
            LL_PROFILE_GPU_ZONE("aa");
            S32 width = dst->getWidth();
            S32 height = dst->getHeight();

            // bake out texture2D with RGBL for FXAA shader
            {
            LLRTScope rts(mFXAAMap, false, "fxaa_bake");
            if (rts)
            {
            mFXAAMap.clear(GL_COLOR_BUFFER_BIT);

            LLGLSLShader* shader = &gGlowCombineFXAAProgram;
            shader->bind();

            S32 channel = shader->enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, src->getUsage());
            if (channel > -1)
            {
                src->bindTexture(0, channel, LLTexUnit::TFO_BILINEAR);
            }

            if (LLVKLoader::isVulkanInitialized())
            {
                src->bindForShaderRead();
            }

            {
                LLGLDepthTest depth_test(GL_TRUE, GL_TRUE, GL_ALWAYS);
                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            }

            shader->disableTexture(LLShaderMgr::DEFERRED_DIFFUSE, src->getUsage());
            shader->unbind();
            }
            }

            {
            LLRTScope rts(*dst, false, "fxaa_present");
            if (rts)
            {

            static LLCachedControl<U32> aa_quality(gSavedSettings, "RenderFSAASamples", 0U);
            U32 fsaa_quality = std::clamp(aa_quality(), 0U, 3U);

            LLGLSLShader* shader = &gFXAAProgram[fsaa_quality];
            shader->bind();

            S32 channel = shader->enableTexture(LLShaderMgr::DIFFUSE_MAP, mFXAAMap.getUsage());
            if (channel > -1)
            {
                mFXAAMap.bindTexture(0, channel, LLTexUnit::TFO_BILINEAR);
            }

            gGLViewport[0] = gViewerWindow->getWorldViewRectRaw().mLeft;
            gGLViewport[1] = gViewerWindow->getWorldViewRectRaw().mBottom;
            gGLViewport[2] = gViewerWindow->getWorldViewRectRaw().getWidth();
            gGLViewport[3] = gViewerWindow->getWorldViewRectRaw().getHeight();

            llSetGLViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);

            F32 scale_x = (F32)width / mFXAAMap.getWidth();
            F32 scale_y = (F32)height / mFXAAMap.getHeight();
            if (LLVKLoader::isVulkanInitialized()
                && shader->mVkPerProgramUBOMapped != nullptr
                && shader->mVkPerProgramUBOSize >= sizeof(LLVKLoader::FxaaShared_PerProgramBind))
            {
                LLVKLoader::FxaaShared_PerProgramBind ubo_data{};
                ubo_data.tc_scale[0]         = scale_x;
                ubo_data.tc_scale[1]         = scale_y;
                ubo_data.rcp_screen_res[0]   = 1.f / width * scale_x;
                ubo_data.rcp_screen_res[1]   = 1.f / height * scale_y;
                ubo_data.rcp_frame_opt[0]    = -0.5f / width * scale_x;
                ubo_data.rcp_frame_opt[1]    = -0.5f / height * scale_y;
                ubo_data.rcp_frame_opt[2]    =  0.5f / width * scale_x;
                ubo_data.rcp_frame_opt[3]    =  0.5f / height * scale_y;
                ubo_data.rcp_frame_opt2[0]   = -2.f / width * scale_x;
                ubo_data.rcp_frame_opt2[1]   = -2.f / height * scale_y;
                ubo_data.rcp_frame_opt2[2]   =  2.f / width * scale_x;
                ubo_data.rcp_frame_opt2[3]   =  2.f / height * scale_y;
                std::memcpy(shader->vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
            }

            {
                LLGLDepthTest depth_test(GL_TRUE, GL_TRUE, GL_ALWAYS);
                S32 depth_channel = shader->getTextureChannel(LLShaderMgr::DEFERRED_DEPTH);
                gGL.getTexUnit(depth_channel)->bind(&getFrameRT()->deferredScreen, true);

                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            }

            shader->unbind();
            }
            }
        }
        else {
            copyRenderTarget(src, dst);
        }
    }
}

void LLPipeline::generateSMAABuffers(LLRenderTarget* src)
{
    llassert(!gCubeSnapshot);
    bool multisample = RenderFSAAType == 2 && gSMAAEdgeDetectProgram[0].isComplete() && mFXAAMap.isComplete() && mSMAABlendBuffer.isComplete();

    // Present everything.
    if (multisample)
    {
        LL_PROFILE_GPU_ZONE("SMAA Edge");
        static LLCachedControl<U32> aa_quality(gSavedSettings, "RenderFSAASamples", 0U);
        U32 fsaa_quality = std::clamp(aa_quality(), 0U, 3U);

        S32 width = src->getWidth();
        S32 height = src->getHeight();

        float rt_metrics[] = { 1.f / width, 1.f / height, (float)width, (float)height };

        LLGLDepthTest    depth(GL_FALSE, GL_FALSE);

        static LLCachedControl<bool> use_sample(gSavedSettings, "RenderSMAAUseSample", false);
        {
            // Bind setup:
            LLRenderTarget& dest = mFXAAMap;
            LLGLSLShader& edge_shader = gSMAAEdgeDetectProgram[fsaa_quality];

            {
            LLRTScope rts(dest, false, "smaa_edge");
            if (rts)
            {
            dest.clear(GL_COLOR_BUFFER_BIT);

            edge_shader.bind();
            if (LLVKLoader::isVulkanInitialized()
                && edge_shader.mVkPerProgramUBO != VK_NULL_HANDLE
                && edge_shader.mVkPerProgramUBOMapped != nullptr)
            {
                LLVKLoader::SMAA_PerProgramBind smaa_ubo = {};
                memcpy(smaa_ubo.SMAA_RT_METRICS, rt_metrics, sizeof(smaa_ubo.SMAA_RT_METRICS));
                memcpy(edge_shader.vkPerProgramBaseWritePtr(), &smaa_ubo, sizeof(smaa_ubo));
            }

            S32 channel = edge_shader.enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, src->getUsage());
            if (channel > -1)
            {
                if (!use_sample)
                {
                    src->bindTexture(0, channel, LLTexUnit::TFO_BILINEAR);
                }
                else
                {
                    gGL.getTexUnit(channel)->bind(mSMAASampleMap);
                    gGL.getTexUnit(channel)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
                }
                gGL.getTexUnit(channel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
            }

            mScreenTriangleVB->setBuffer();
            mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

            edge_shader.unbind();

            gGL.getTexUnit(channel)->unbindFast(LLTexUnit::TT_TEXTURE);
            }
            }
        }

        {
            // Bind setup:
            LLRenderTarget& dest = mSMAABlendBuffer;
            LLGLSLShader& blend_weights_shader = gSMAABlendWeightsProgram[fsaa_quality];

            {
            LLRTScope rts(dest, false, "smaa_blend_weights");
            if (rts)
            {
            dest.clear(GL_COLOR_BUFFER_BIT);

            blend_weights_shader.bind();
            if (LLVKLoader::isVulkanInitialized()
                && blend_weights_shader.mVkPerProgramUBO != VK_NULL_HANDLE
                && blend_weights_shader.mVkPerProgramUBOMapped != nullptr)
            {
                LLVKLoader::SMAA_PerProgramBind smaa_ubo = {};
                memcpy(smaa_ubo.SMAA_RT_METRICS, rt_metrics, sizeof(smaa_ubo.SMAA_RT_METRICS));
                memcpy(blend_weights_shader.vkPerProgramBaseWritePtr(), &smaa_ubo, sizeof(smaa_ubo));
            }

            if (LLVKLoader::isVulkanInitialized())
            {
                LLVKLoader::SMAABlendWeightsF_PerProgramBind smaa_data{};
                smaa_data.subsampleIndices[0] = 0.0f;
                smaa_data.subsampleIndices[1] = 0.0f;
                smaa_data.subsampleIndices[2] = 0.0f;
                smaa_data.subsampleIndices[3] = 0.0f;
                LLVKLoader::writeCurrentSMAABlendWeightsFUBO(smaa_data);
            }

            S32 edge_tex_channel = blend_weights_shader.enableTexture(LLShaderMgr::SMAA_EDGE_TEX, mFXAAMap.getUsage());
            if (edge_tex_channel > -1)
            {
                mFXAAMap.bindTexture(0, edge_tex_channel, LLTexUnit::TFO_BILINEAR);
                gGL.getTexUnit(edge_tex_channel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
            }
            S32 area_tex_channel = blend_weights_shader.enableTexture(LLShaderMgr::SMAA_AREA_TEX, LLTexUnit::TT_TEXTURE);
            if (area_tex_channel > -1)
            {
                gGL.getTexUnit(area_tex_channel)->bind(mSMAAAreaMap);
                gGL.getTexUnit(area_tex_channel)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
                gGL.getTexUnit(area_tex_channel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
            }
            S32 search_tex_channel = blend_weights_shader.enableTexture(LLShaderMgr::SMAA_SEARCH_TEX, LLTexUnit::TT_TEXTURE);
            if (search_tex_channel > -1)
            {
                gGL.getTexUnit(search_tex_channel)->bind(mSMAASearchMap);
                gGL.getTexUnit(search_tex_channel)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
                gGL.getTexUnit(search_tex_channel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
            }

            mScreenTriangleVB->setBuffer();
            mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            blend_weights_shader.unbind();
            gGL.getTexUnit(edge_tex_channel)->unbindFast(LLTexUnit::TT_TEXTURE);
            gGL.getTexUnit(area_tex_channel)->unbindFast(LLTexUnit::TT_TEXTURE);
            gGL.getTexUnit(search_tex_channel)->unbindFast(LLTexUnit::TT_TEXTURE);
            }
            }
        }
    }
}

void LLPipeline::applySMAA(LLRenderTarget* src, LLRenderTarget* dst)
{
    LL_PROFILE_GPU_ZONE("SMAA");
    llassert(!gCubeSnapshot);
    bool multisample = RenderFSAAType == 2 && gSMAAEdgeDetectProgram[0].isComplete() && mFXAAMap.isComplete() && mSMAABlendBuffer.isComplete();

    // Present everything.
    if (multisample)
    {
        static LLCachedControl<U32> aa_quality(gSavedSettings, "RenderFSAASamples", 0U);
        U32 fsaa_quality = std::clamp(aa_quality(), 0U, 3U);

        S32 width = src->getWidth();
        S32 height = src->getHeight();

        float rt_metrics[] = { 1.f / width, 1.f / height, (float)width, (float)height };

        LLGLDepthTest    depth(GL_FALSE, GL_FALSE);

        static LLCachedControl<bool> use_sample(gSavedSettings, "RenderSMAAUseSample", false);

        {
            // Bind setup:
            LLRenderTarget* bound_target = dst;
            LLGLSLShader& blend_shader = gSMAANeighborhoodBlendProgram[fsaa_quality];

            {
            LLRTScope rts(*bound_target, false, "smaa_neighborhood");
            if (rts)
            {
            bound_target->clear(GL_COLOR_BUFFER_BIT);

            blend_shader.bind();
            if (LLVKLoader::isVulkanInitialized()
                && blend_shader.mVkPerProgramUBO != VK_NULL_HANDLE
                && blend_shader.mVkPerProgramUBOMapped != nullptr)
            {
                LLVKLoader::SMAA_PerProgramBind smaa_ubo = {};
                memcpy(smaa_ubo.SMAA_RT_METRICS, rt_metrics, sizeof(smaa_ubo.SMAA_RT_METRICS));
                memcpy(blend_shader.vkPerProgramBaseWritePtr(), &smaa_ubo, sizeof(smaa_ubo));
            }

            S32 diffuse_channel = blend_shader.enableTexture(LLShaderMgr::DEFERRED_DIFFUSE);
            if(diffuse_channel > -1)
            {
                src->bindTexture(0, diffuse_channel, LLTexUnit::TFO_BILINEAR);
                gGL.getTexUnit(diffuse_channel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
            }

            S32 blend_channel = blend_shader.enableTexture(LLShaderMgr::SMAA_BLEND_TEX);
            if (blend_channel > -1)
            {
                mSMAABlendBuffer.bindTexture(0, blend_channel, LLTexUnit::TFO_BILINEAR);
            }

            mScreenTriangleVB->setBuffer();
            mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

            blend_shader.unbind();
            gGL.getTexUnit(diffuse_channel)->unbindFast(LLTexUnit::TT_TEXTURE);
            gGL.getTexUnit(blend_channel)->unbindFast(LLTexUnit::TT_TEXTURE);
            }
            }
        }
    }
    else
    {
        copyRenderTarget(src, dst);
    }
}

// <AYAstorm r30 P2 step 5c>
// SMAA T2x temporal resolve. Imported from BlackDragon Viewer 995a1354d8
// (LGPL-2.1-only), adapted: AYAstorm gates on mVelocityMap.isComplete()
// (Cinematic mode) and runs the resolve after applySMAA's spatial pass.
//
// Without Halton jitter (step 5d), the resolve simply blends frame N with
// frame N-1, which produces ghosting on motion but minor edge stabilization
// on the static parts of the scene. Step 5d adds the per-frame subpixel
// jitter that turns this into proper temporal anti-aliasing.
//
// History save uses copyRenderTarget on the *current SMAA'd input* (src),
// not the resolved output, so the next frame's resolve does a true 50/50
// blend between two jitter samples rather than exponential decay.
void LLPipeline::resolveSMAAT2x(LLRenderTarget* src, LLRenderTarget* dst)
{
    LL_PROFILE_GPU_ZONE("SMAA T2x Resolve");

    static LLCachedControl<U32> aa_quality(gSavedSettings, "RenderFSAASamples", 0U);
    U32 q = std::clamp(aa_quality(), 0U, 3U);

    {
    LLRTScope rts(*dst, false, "smaa_t2x_resolve");
    if (rts)
    {
    LLGLSLShader& shader = gSMAAResolveProgram[q];
    shader.bind();

    // Current SMAA'd frame goes to diffuseRect (DEFERRED_DIFFUSE), matching
    // our SMAAResolveF.glsl's "uniform sampler2D diffuseRect" declaration.
    // BD's variant uses a distinct SMAA_CURRENT_COLOR_TEX uniform name;
    // we reuse the existing DEFERRED_DIFFUSE slot to avoid widening
    // LLShaderMgr's reserved-uniform enum for a single binding.
    S32 cur_ch = shader.enableTexture(LLShaderMgr::DEFERRED_DIFFUSE);
    if (cur_ch > -1)
    {
        src->bindTexture(0, cur_ch, LLTexUnit::TFO_POINT);
    }

    S32 prev_ch = shader.enableTexture(LLShaderMgr::SMAA_PREVIOUS_COLOR_TEX);
    if (prev_ch > -1)
    {
        mSMAAHistory.bindTexture(0, prev_ch, LLTexUnit::TFO_POINT);
    }

    S32 vel_ch = shader.enableTexture(LLShaderMgr::SMAA_VELOCITY_TEX);
    if (vel_ch > -1)
    {
        mVelocityMap.bindTexture(0, vel_ch, LLTexUnit::TFO_BILINEAR);
    }

    mScreenTriangleVB->setBuffer();
    mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

    shader.unbind();
    }
    }

    // Save the current SMAA'd frame (not the resolved output) to history so
    // the next frame's resolve sees a true 50/50 blend between the two
    // jitter samples instead of exponential history decay.
    copyRenderTarget(src, &mSMAAHistory);
}

// </AYAstorm r30 P2 step 5c>

void LLPipeline::copyRenderTarget(LLRenderTarget* src, LLRenderTarget* dst)
{

    LL_PROFILE_GPU_ZONE("copyRenderTarget");
    {
    LLRTScope rts(*dst, false, "copy_rt");
    if (rts)
    {
    gDeferredPostNoDoFProgram.bind();

    gDeferredPostNoDoFProgram.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, src);
    gDeferredPostNoDoFProgram.bindTexture(LLShaderMgr::DEFERRED_DEPTH, &getFrameRT()->deferredScreen, true);

    // <AYAstorm r30 P4 step 4> BD chroma_str (vignette path runs when HAS_DOF_CHROMA==0)
    // <FS:AYAstorm r30 BD full port Phase 3.7 cat 01> Cinematic で chroma 完全 OFF
    // (BD_NOOP=0.0f)。spec §3.1 phase3.5-ay-only Category A。
    const F32 nodof_chroma_str = gSavedSettings.getF32("RenderChromaStrength");
    // </FS:AYAstorm>
    // </AYAstorm r30 P4 step 4>

    if (LLVKLoader::isVulkanInitialized()
        && gDeferredPostNoDoFProgram.mVkPerProgramUBO != VK_NULL_HANDLE
        && gDeferredPostNoDoFProgram.mVkPerProgramUBOMapped != nullptr)
    {
        LLVKLoader::PostNoDoFF_PerProgramBind ubo_data = {};
        ubo_data.screen_res[0] = (F32)src->getWidth();
        ubo_data.screen_res[1] = (F32)src->getHeight();
        ubo_data.chroma_str    = nodof_chroma_str;
        memcpy(gDeferredPostNoDoFProgram.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
    }

    {
        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
    }

    gDeferredPostNoDoFProgram.unbind();

    }
    }
}

void LLPipeline::blitScenePresentToSwapchain()
{
    LL_PROFILE_GPU_ZONE("blitScenePresentToSwapchain");

    gGLViewport[0] = gViewerWindow->getWorldViewRectRaw().mLeft;
    gGLViewport[1] = gViewerWindow->getWorldViewRectRaw().mBottom;
    gGLViewport[2] = gViewerWindow->getWorldViewRectRaw().getWidth();
    gGLViewport[3] = gViewerWindow->getWorldViewRectRaw().getHeight();

    LLVKLoader::beginSwapchainRendering();

    gCopyProgram.bind();
    gGL.getTexUnit(0)->bind(&mScenePresentRT[mScenePresentFront]);
    if (LLVKLoader::isVulkanInitialized())
    {
        mScenePresentRT[mScenePresentFront].bindForShaderRead();
    }

    {
        LLGLDepthTest depth_test(GL_TRUE, GL_TRUE, GL_ALWAYS);
        LLGLDisable blend_off(GL_BLEND);
        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
    }

    gCopyProgram.unbind();
}

void LLPipeline::combineGlow(LLRenderTarget* src, LLRenderTarget* dst)
{
    LL_PROFILE_GPU_ZONE("glow combine");

    // Go ahead and do our glow combine here in our destination.  We blit this later into the front buffer.
    {
    LLRTScope rts(*dst, false, "combine_glow");
    if (rts)
    {

    {

        gGlowCombineProgram.bind();

        gGlowCombineProgram.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, src);
        gGlowCombineProgram.bindTexture(LLShaderMgr::DEFERRED_EMISSIVE, &mGlow[1]);

        // <FS:AYAstorm:r30-bd-port> Phase 6 step 3: BD post FX (Cinematic only).
        // mode 0/1 では noop default (Greyscale=0 / Sepia=0 / NumColors=1) を送る。
        float glow_greyscale_str;
        float glow_sepia_str;
        float glow_num_colors;
        if (isCinematicMode())
        {
            glow_greyscale_str = RenderGreyscaleStrength;
            glow_sepia_str     = RenderSepiaStrength;
            glow_num_colors    = (GLfloat)RenderNumColors;
        }
        else
        {
            glow_greyscale_str = 0.0f;
            glow_sepia_str     = 0.0f;
            glow_num_colors    = 1.0f;
        }
        // </FS:AYAstorm:r30-bd-port>
        if (LLVKLoader::isVulkanInitialized())
        {
            LLVKLoader::GlowCombine_PerShaderBind ubo_data;
            ubo_data.greyscale_str = glow_greyscale_str;
            ubo_data.sepia_str     = glow_sepia_str;
            ubo_data.num_colors    = glow_num_colors;
            ubo_data._pad0         = 0.0f;
            LLVKLoader::writeCurrentGlowCombineUBO(ubo_data);
        }

        if (LLVKLoader::isVulkanInitialized())
        {
            src->bindForShaderRead();
            mGlow[1].bindForShaderRead();
        }

        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
    }

    }
    }
}

// <FS:Beq> updated Vignette code (based on original Exo Vignette)
bool LLPipeline::renderVignette(LLRenderTarget* src, LLRenderTarget* dst)
{
    if (RenderVignette.mV[0] > 0.f)
    {
        LL_PROFILE_GPU_ZONE("Vignette");
        LLRTScope rts(*dst, false, "vignette");
        if (rts)
        {
        LLGLSLShader *shader = &gPostVignetteProgram;

        // bind the progam and output to screentriangle VBO
        shader->bind();

        S32 channel = shader->enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, src->getUsage());
        if (channel > -1)
        {
            src->bindTexture(0, channel, LLTexUnit::TFO_POINT);
        }
        else
        {
            LL_ERRS("vignette") << "Failed to bind diffuse texture" << LL_ENDL;
        }

        if (LLVKLoader::isVulkanInitialized() && shader->mVkPerProgramUBO != VK_NULL_HANDLE
            && shader->mVkPerProgramUBOMapped != nullptr)
        {
            LLVKLoader::PostVignette_PerProgramBind ubo_data = {};
            ubo_data.screen_res[0]     = (F32)dst->getWidth();
            ubo_data.screen_res[1]     = (F32)dst->getHeight();
            ubo_data.vignette[0]       = RenderVignette.mV[0];
            ubo_data.vignette[1]       = RenderVignette.mV[1];
            ubo_data.vignette[2]       = RenderVignette.mV[2];
            memcpy(shader->vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
        }

        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

        shader->disableTexture(LLShaderMgr::DEFERRED_DIFFUSE, src->getUsage());
        shader->unbind();
        return true;
        }
        return false;
    }
    else
    {
        return false;
    }
}

// </FS:Beq>

void LLPipeline::renderDoF(LLRenderTarget* src, LLRenderTarget* dst)
{
    LL_PROFILE_GPU_ZONE("dof");
    {
        LLPipelineFrameContext::getInstance().setDoFPass( // <FS:Beq/> // FIRE-32023 Render focus point
            (RenderDepthOfFieldInEditMode || !LLToolMgr::getInstance()->inBuildMode()) &&
            RenderDepthOfField &&
            !gCubeSnapshot);

        gViewerWindow->setup3DViewport();

        if (isFrameDoFPass()) // <FS:Beq/> // FIRE-32023 Render focus point
        {
            if (!gDeferredCoFProgram.isComplete() ||
                !gDeferredPostProgram.isComplete() ||
                !gDeferredDoFCombineProgram.isComplete())
            {
                LL_WARNS_ONCE("Pipeline") << "Skipping depth of field because one or more DoF shaders failed to link." << LL_ENDL;
                notifyDoFSkippedOnce("AYAstorm: Depth of Field was disabled because a required post-processing shader failed to load. Check AYAstorm.log for shader details.");
                copyRenderTarget(src, dst);
                return;
            }

            LLGLDisable blend(GL_BLEND);

            // depth of field focal plane calculations
            static F32 current_distance = 16.f;
            static F32 start_distance = 16.f;
            static F32 transition_time = 1.f;

            LLVector3 focus_point;

            // <FS:Beq> FIRE-16728 focus point lock & free focus DoF - based on a feature developed by NiranV Dean
            
            if (LLPipeline::FSFocusPointLocked && !sLastFocusPoint.isExactlyZero())
            {
                focus_point = sLastFocusPoint;
            }
            else
            {
            // </FS:Beq>
            LLViewerObject* obj = LLViewerMediaFocus::getInstance()->getFocusedObject();
            if (obj && obj->mDrawable && obj->isSelected())
            { // focus on selected media object
                S32 face_idx = LLViewerMediaFocus::getInstance()->getFocusedFace();
                if (obj && obj->mDrawable)
                {
                    LLFace* face = obj->mDrawable->getFace(face_idx);
                    if (face)
                    {
                        focus_point = face->getPositionAgent();
                    }
                }
            }
            }// <FS:Beq/> support focus point lock

            if (focus_point.isExactlyZero())
            {
                if (LLViewerJoystick::getInstance()->getOverrideCamera() || LLPipeline::FSFocusPointFollowsPointer) // <FS:Beq/> FIRE-16728 Add free aim mouse and focus lock
                { // focus on point under cursor
                    focus_point.set(gDebugRaycastIntersection.getF32ptr());
                }
                else if (gAgentCamera.cameraMouselook())
                { // focus on point under mouselook crosshairs
                    LLVector4a result;
                    result.clear();

                    gViewerWindow->cursorIntersect(-1, -1, 512.f, nullptr, -1, false, false, true, true, nullptr, nullptr, nullptr, &result);

                    focus_point.set(result.getF32ptr());
                }
                else
                {
                    // focus on alt-zoom target
                    LLViewerRegion* region = gAgent.getRegion();
                    if (region)
                    {
                        focus_point = LLVector3(gAgentCamera.getFocusGlobal() - region->getOriginGlobal());
                    }
                }
            }

            // <FS:Beq> FIRE-16728 Add free aim mouse and focus lock
            sLastFocusPoint = focus_point;
            // </FS:Beq>
            LLVector3 eye = LLViewerCamera::getInstance()->getOrigin();
            F32 target_distance = 16.f;
            if (!focus_point.isExactlyZero())
            {
                target_distance = LLViewerCamera::getInstance()->getAtAxis() * (focus_point - eye);
            }

            if (transition_time >= 1.f && fabsf(current_distance - target_distance) / current_distance > 0.01f)
            { // large shift happened, interpolate smoothly to new target distance
                transition_time = 0.f;
                start_distance = current_distance;
            }
            else if (transition_time < 1.f)
            { // currently in a transition, continue interpolating
                transition_time += 1.f / CameraFocusTransitionTime * gFrameIntervalSeconds.value();
                transition_time = llmin(transition_time, 1.f);

                F32 t = cosf(transition_time * F_PI + F_PI) * 0.5f + 0.5f;
                current_distance = start_distance + (target_distance - start_distance) * t;
            }
            else
            { // small or no change, just snap to target distance
                current_distance = target_distance;
            }

            // convert to mm
            F32 subject_distance = current_distance * 1000.f;
            F32 fnumber = CameraFNumber;
            F32 default_focal_length = CameraFocalLength;

            F32 fov = LLViewerCamera::getInstance()->getView();

            const F32 default_fov = CameraFieldOfView * F_PI / 180.f;

            // F32 aspect_ratio = (F32) getFrameRT()->screen.getWidth()/(F32)getFrameRT()->screen.getHeight();

            F32 dv = 2.f * default_focal_length * tanf(default_fov / 2.f);

            F32 focal_length = dv / (2 * tanf(fov / 2.f));

            // F32 tan_pixel_angle = tanf(LLDrawable::sCurPixelAngle);

            // from wikipedia -- c = |s2-s1|/s2 * f^2/(N(S1-f))
            // where     N = fnumber
            //           s2 = dot distance
            //           s1 = subject distance
            //           f = focal length
            //

            F32 blur_constant = focal_length * focal_length / (fnumber * (subject_distance - focal_length));
            blur_constant /= 1000.f; // convert to meters for shader
            F32 magnification = focal_length / (subject_distance - focal_length);
            // <FS:Beq> FIRE-13989 DOF should be equivalent in all resolutions of the same rendered image
            F32 screen_to_target_scale_factor = (F32)gViewerWindow->getWindowHeightRaw()/dst->getHeight();
            F32 adj_COF = CameraMaxCoF / screen_to_target_scale_factor;
            // </FS:Beq>
            { // build diffuse+bloom+CoF
                LLRTScope rts(getFrameRT()->deferredLight, false, "dof_cof");
                if (rts)
                {
                gDeferredCoFProgram.bind();


                gDeferredCoFProgram.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, src, LLTexUnit::TFO_POINT);
                // <AYAstorm r30 P5 transparent-DoF L2-β> Bind the alpha-aware
                // depth buffer (opaque-only snapshot + cutoff-0.5 alpha
                // re-injection) instead of post-alpha deferredScreen.depth.
                // For an alpha BLEND hair pixel (alpha ≈ 0.2) the cutoff
                // discards the hair fragment so the snapshotted bg depth
                // survives → cofF blurs the bg; for an alpha BLEND window
                // grille (alpha ≈ 0.7) the cutoff passes so the grille z
                // is written and cofF treats it as subject. For pure opaque
                // pixels the two buffers carry the same z so behaviour is
                // unchanged. Falls back to deferredScreen.depth if the L2
                // RT is unavailable (e.g. probe paths).
                LLRenderTarget* cof_depth_src = mAYAAlphaDepth.isComplete() ? &mAYAAlphaDepth : &getFrameRT()->deferredScreen;
                gDeferredCoFProgram.bindTexture(LLShaderMgr::DEFERRED_DEPTH, cof_depth_src, true);
                // </AYAstorm r30 P5 transparent-DoF L2-β>

                // <FS:Beq> FIRE-13989 DOF should be equivalent in all resolutions of the same rendered image
                // gDeferredCoFProgram.uniform1f(LLShaderMgr::DOF_TAN_PIXEL_ANGLE, tanf(1.f / LLDrawable::sCurPixelAngle));
                // </FS:Beq>
                // <FS:Beq> FIRE-13989 DOF should be equivalent in all resolutions of the same rendered image
                // gDeferredCoFProgram.uniform1f(LLShaderMgr::DOF_MAX_COF, CameraMaxCoF);
                // divide by the screen->target ratio so tha a larger target (ratio < 1) has a higher MaxCoF value
                // </FS:Beq>

                if (LLVKLoader::isVulkanInitialized()
                    && gDeferredCoFProgram.mVkPerProgramUBOMapped != nullptr
                    && gDeferredCoFProgram.mVkPerProgramUBOSize >= sizeof(LLVKLoader::CofF_PerProgramBind))
                {
                    LLVKLoader::CofF_PerProgramBind ubo_data{};
                    ubo_data.focal_distance   = -subject_distance / 1000.f;
                    ubo_data.blur_constant    = blur_constant;
                    ubo_data.tan_pixel_angle  = tanf(1.f / LLDrawable::sCurPixelAngle) * screen_to_target_scale_factor;
                    ubo_data.magnification    = magnification;
                    ubo_data.max_cof          = adj_COF;
                    ubo_data._cofF_pad0       = 0.f;
                    ubo_data._cofF_pad1       = 0.f;
                    ubo_data._cofF_pad2       = 0.f;
                    std::memcpy(gDeferredCoFProgram.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
                }

                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
                gDeferredCoFProgram.unbind();
                }
            }

            U32 dof_width = (U32)(getFrameRT()->screen.getWidth() * CameraDoFResScale);
            U32 dof_height = (U32)(getFrameRT()->screen.getHeight() * CameraDoFResScale);

            { // perform DoF sampling at half-res (preserve alpha channel)
                LLRTScope rts(*src, false, "dof_sample");
                if (rts)
                {
                llSetGLViewport(0, 0, dof_width, dof_height);

                gGL.setColorMask(true, false);

                gDeferredPostProgram.bind();


                gDeferredPostProgram.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, &getFrameRT()->deferredLight, LLTexUnit::TFO_POINT);
                // <AYAstorm r30 P4 step 4> HQ DoF needs depthMap for the s.a <= depth*0.50 gate
                gDeferredPostProgram.bindTexture(LLShaderMgr::DEFERRED_DEPTH, &getFrameRT()->deferredScreen, true);
                // </AYAstorm r30 P4 step 4>

                // <FS:Beq> FIRE-13989 DOF should be equivalent in all resolutions of the same rendered image
                // gDeferredPostProgram.uniform1f(LLShaderMgr::DOF_MAX_COF, CameraMaxCoF);
                // </FS:Beq>

                // <AYAstorm r30 P4 step 4> BD chroma_str (HAS_DOF_CHROMA permutation, no-op otherwise)
                // <FS:AYAstorm r30 BD full port Phase 3.7 cat 01> Cinematic BD parity (0.0f)。spec §3.1。
                const F32 dof_chroma_str = gSavedSettings.getF32("RenderChromaStrength");
                // </FS:AYAstorm>
                // </AYAstorm r30 P4 step 4>

                if (LLVKLoader::isVulkanInitialized()
                    && gDeferredPostProgram.mVkPerProgramUBO != VK_NULL_HANDLE
                    && gDeferredPostProgram.mVkPerProgramUBOMapped != nullptr)
                {
                    LLVKLoader::PostF_PerProgramBind ubo_data = {};
                    ubo_data.screen_res[0] = (F32)dst->getWidth();
                    ubo_data.screen_res[1] = (F32)dst->getHeight();
                    ubo_data.max_cof       = adj_COF;
                    ubo_data.chroma_str    = dof_chroma_str;
                    memcpy(gDeferredPostProgram.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
                }

                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

                gDeferredPostProgram.unbind();

                gGL.setColorMask(true, true);
                }
            }

            { // combine result based on alpha

                LLRTScope rts(*dst, false, "dof_combine");
                if (rts)
                {
                llSetGLViewport(0, 0, dst->getWidth(), dst->getHeight());

                gDeferredDoFCombineProgram.bind();


                gDeferredDoFCombineProgram.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, src, LLTexUnit::TFO_POINT);
                gDeferredDoFCombineProgram.bindTexture(LLShaderMgr::DEFERRED_LIGHT, &getFrameRT()->deferredLight, LLTexUnit::TFO_POINT);

                // <FS:Beq> FIRE-13989 DOF should be equivalent in all resolutions of the same rendered image
                // gDeferredDoFCombineProgram.uniform1f(LLShaderMgr::DOF_MAX_COF, CameraMaxCoF);
                // </FS:Beq>

                if (LLVKLoader::isVulkanInitialized()
                    && gDeferredDoFCombineProgram.mVkPerProgramUBOMapped != nullptr
                    && gDeferredDoFCombineProgram.mVkPerProgramUBOSize >= sizeof(LLVKLoader::DofCombineF_PerProgramBind))
                {
                    LLVKLoader::DofCombineF_PerProgramBind ubo_data{};
                    ubo_data._dofC_screen_res[0] = (F32)dst->getWidth();
                    ubo_data._dofC_screen_res[1] = (F32)dst->getHeight();
                    ubo_data._dofC_pad0[0]       = 0.f;
                    ubo_data._dofC_pad0[1]       = 0.f;
                    ubo_data._dofC_max_cof       = adj_COF;
                    ubo_data._dofC_res_scale     = CameraDoFResScale;
                    ubo_data._dofC_dof_width     = (F32)(dof_width - 1) / (F32)src->getWidth();
                    ubo_data._dofC_dof_height    = (F32)(dof_height - 1) / (F32)src->getHeight();
                    std::memcpy(gDeferredDoFCombineProgram.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
                }

                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

                gDeferredDoFCombineProgram.unbind();

                }
            }
        }
        else
        {
            copyRenderTarget(src, dst);
        }
    }
}

void LLPipeline::renderFinalize()
{
    llassert(!gCubeSnapshot);
    LLVertexBuffer::unbind();

    assertInitialized();

    LL_RECORD_BLOCK_TIME(FTM_RENDER_BLOOM);
    LL_PROFILE_GPU_ZONE("renderFinalize");

    gGL.color4f(1, 1, 1, 1);
    LLGLDepthTest depth(GL_FALSE);
    LLGLDisable blend(GL_BLEND);
    LLGLDisable cull(GL_CULL_FACE);

    enableLightsFullbright();

    gGL.setColorMask(true, true);
    gGL.setClearColor(0, 0, 0, 0);

    LLVKLoader::gpuCheckpoint("fin:fwdflip");
    compositeForwardFlip();

    // <AYAstorm r30 P5 transparent-DoF C-(a) pre-tonemap composite>
    // Over-blend the linear premultiplied alpha plate (mAYAAlphaColor) onto
    if (mAYAAlphaColor.isComplete() && gAYAAlphaPlateCompositeProgram.isComplete())
    {
        LL_PROFILE_GPU_ZONE("aya plate pre-tonemap composite");
        LLVKLoader::gpuCheckpoint("fin:plate");
        {
        LLRTScope rts(getFrameRT()->screen, false, "finalize_composite");
        if (rts)
        {

        LLGLEnable blend_on(GL_BLEND);
        // RGB: premultiplied "over" composite onto opaque scene.
        //   dst.rgb = plate.rgb + screen.rgb * (1 - plate.a)
        // Alpha: attenuate dst.a by (1 - plate.a) to mimic the LMB-on-HUD
        // path's "glow suppression" blend (BF_ZERO, BF_ONE_MINUS_SOURCE_ALPHA)
        // which attenuates screen.a (scene glow channel consumed by
        // combineGlow) under alpha-covered pixels. Without attenuation
        // LMB-up shows opaque-light glow halos bleeding through hair /
        // clothing while LMB-on-HUD does not — visible path divergence.
        // This is an approximation: LMB-on-HUD attenuates per-draw with
        // emissive added between, whereas here we attenuate accumulated
        // screen.a (opaque_glow + sum(alpha_emissive)) once by accumulated
        // plate.a. For emissive-zero alpha BLEND (hair/clothing/glass, the
        // vast majority) the approximation matches LMB-on-HUD; for emissive
        // alpha BLEND (lanterns) we over-attenuate slightly. Acceptable.
        //   dst.a = screen.a * (1 - plate.a)
        gGL.blendFunc(LLRender::BF_ONE, LLRender::BF_ONE_MINUS_SOURCE_ALPHA,
                      LLRender::BF_ZERO, LLRender::BF_ONE_MINUS_SOURCE_ALPHA);

        gAYAAlphaPlateCompositeProgram.bind();


        gAYAAlphaPlateCompositeProgram.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, &mAYAAlphaColor, false, LLTexUnit::TFO_POINT);

        {
            LLGLDepthTest depth_test(GL_FALSE, GL_FALSE);
            mScreenTriangleVB->setBuffer();
            mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
        }

        gAYAAlphaPlateCompositeProgram.unbind();

        // Restore default blend func so subsequent passes (tonemap etc.)
        // aren't surprised.
        gGL.blendFunc(LLRender::BF_ONE, LLRender::BF_ZERO,
                      LLRender::BF_ONE, LLRender::BF_ZERO);

        }
        }
    }
    // </AYAstorm r30 P5 transparent-DoF C-(a) pre-tonemap composite>

    static LLCachedControl<bool> has_hdr(gSavedSettings, "RenderHDREnabled", true);
    bool hdr = gGLManager.mGLVersion > 4.05f && has_hdr();
    if (hdr)
    {
        LLVKLoader::gpuCheckpoint("fin:ssr_copy");
        copyScreenSpaceReflections(&getFrameRT()->screen, &mSceneMap);

        LLVKLoader::gpuCheckpoint("fin:luminance");
        generateLuminance(&getFrameRT()->screen, &mLuminanceMap);

        LLVKLoader::gpuCheckpoint("fin:exposure");
        generateExposure(&mLuminanceMap, &mExposureMap);

        static LLCachedControl<F32> cas_sharpness(gSavedSettings, "RenderCASSharpness", 0.4f);
        bool apply_cas = cas_sharpness != 0.0f && gCASProgram.isComplete() && gCASLegacyGammaProgram.isComplete();

        LLVKLoader::gpuCheckpoint("fin:tonemap");
        tonemap(&getFrameRT()->screen, apply_cas ? &getFrameRT()->deferredLight : &mPostPingMap, !apply_cas);

        if (apply_cas)
        {
            // Gamma Corrects
            LLVKLoader::gpuCheckpoint("fin:cas");
            applyCAS(&getFrameRT()->deferredLight, &mPostPingMap);
        }
    }
    else
    {
        LLVKLoader::gpuCheckpoint("fin:gamma");
        gammaCorrect(&getFrameRT()->screen, &mPostPingMap);
    }

    LLVertexBuffer::unbind();

    LLVKLoader::gpuCheckpoint("fin:glow");
    generateGlow(&mPostPingMap);

    LLRenderTarget* sourceBuffer = &mPostPingMap;
    LLRenderTarget* targetBuffer = &mPostPongMap;

    if (LLPipeline::isCinematicMode()
        && gSavedSettings.getBOOL("RenderVolumetricLighting")
        && !gCubeSnapshot)
    {
        LLVKLoader::gpuCheckpoint("fin:volumetric");
        renderVolumetric(sourceBuffer);
    }

    LLVKLoader::gpuCheckpoint("fin:combine_glow");
    combineGlow(sourceBuffer, targetBuffer);
    std::swap(sourceBuffer, targetBuffer);

    // <AYAstorm r30 P2 step 5b> Motion blur composite (Cinematic mode only — gated by
    // mVelocityMap.isComplete()). Reads diffuseRect + velocityMap, writes blurred image.
    static LLCachedControl<S32> motion_blur_strength(gSavedSettings, "RenderMotionBlurStrength", 32);
    // <FS:AYAstorm r30 P4> RenderMotionBlur (GUI checkbox) is the master gate.
    if (RenderMotionBlur && mVelocityMap.isComplete() && motion_blur_strength > 0 && !gCubeSnapshot)
    // </FS:AYAstorm r30 P4>
    {
        LLVKLoader::gpuCheckpoint("fin:motion_blur");
        renderMotionBlurComposite(sourceBuffer, targetBuffer);
        std::swap(sourceBuffer, targetBuffer);
    }
    // </AYAstorm r30 P2 step 5b>

    gGLViewport[0] = gViewerWindow->getWorldViewRectRaw().mLeft;
    gGLViewport[1] = gViewerWindow->getWorldViewRectRaw().mBottom;
    gGLViewport[2] = gViewerWindow->getWorldViewRectRaw().getWidth();
    gGLViewport[3] = gViewerWindow->getWorldViewRectRaw().getHeight();
    llSetGLViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);

    const bool dof_gate_pass =
        (RenderDepthOfFieldInEditMode || !LLToolMgr::getInstance()->inBuildMode()) &&
        RenderDepthOfField &&
        !gCubeSnapshot;

    if (dof_gate_pass)
    {
        LLVKLoader::gpuCheckpoint("fin:dof");
        renderDoF(sourceBuffer, targetBuffer);
        std::swap(sourceBuffer, targetBuffer);
    }

     // <AYAstorm r30 P2 step 5d> Default jitter off; only the SMAA T2x branch
     // below re-enables it. Without this clear, leaving SMAA mode would freeze
     // sT2xJitterEnabled=true and keep ghost-jittering the projection forever.
     sT2xJitterEnabled = false;
     // </AYAstorm r30 P2 step 5d>
     if (RenderFSAAType == 1)
    {
        LLVKLoader::gpuCheckpoint("fin:fxaa");
        applyFXAA(sourceBuffer, targetBuffer);
        std::swap(sourceBuffer, targetBuffer);
    }
    else if (RenderFSAAType == 2 || RenderFSAAType == 3)
    {
        LLVKLoader::gpuCheckpoint("fin:smaa");
        generateSMAABuffers(sourceBuffer);
        applySMAA(sourceBuffer, targetBuffer);
        std::swap(sourceBuffer, targetBuffer);

        // <AYAstorm r30 P2 step 5c+5d / cleanup A.2+A.5> SMAA T2x temporal resolve.
        // Selector is the single RenderFSAAType enum: 2 = plain SMAA (BD parity
        // default), 3 = SMAA + T2x (AY-only opt-in). The old RenderSMAAT2x cvar
        // was removed in cleanup A.5; FSAAType=3 is now the sole entry point.
        // sT2xJitterEnabled stays in lockstep with t2x_active so that
        // LLViewerCamera::setPerspective injects the ±0.25 px jitter only while
        // the resolve runs.
        bool t2x_active = (RenderFSAAType == 3) && mVelocityMap.isComplete() && mSMAAHistory.isComplete() && !gCubeSnapshot;
        sT2xJitterEnabled = t2x_active;
        if (t2x_active)
        {
            LLVKLoader::gpuCheckpoint("fin:smaa_t2x");
            resolveSMAAT2x(sourceBuffer, targetBuffer);
            std::swap(sourceBuffer, targetBuffer);
            mSMAAFrameIndex ^= 1;
        }
        // </AYAstorm r30 P2 step 5c+5d / cleanup A.2+A.5>
    }

    // <FS:Beq> Restore shader post proc for Vignette
    LLRenderTarget* auxActiveBuffer = sourceBuffer;
    LLRenderTarget* auxTargetBuffer = RenderFSAAType ? &getFrameRT()->screen : &mPostPingMap;
// [RLVa:KB] - @setsphere
    if (RlvActions::hasBehaviour(RLV_BHVR_SETSPHERE))
    {
        LLShaderEffectParams params(auxActiveBuffer, auxTargetBuffer, false);
        LLVfxManager::instance().runEffect(EVisualEffect::RlvSphere, &params);
        // flip the buffers round
        auxActiveBuffer = params.m_pDstBuffer;
        auxTargetBuffer = params.m_pSrcBuffer;
    }
// [/RLVa:KB]

    LLVKLoader::gpuCheckpoint("fin:vignette");
    if (renderVignette(auxActiveBuffer, auxTargetBuffer))
    {
        std::swap(auxActiveBuffer, auxTargetBuffer);
    };
    // </FS:Beq>
    // <FS:Beq> new shader for snapshot frame helper
    LLVKLoader::gpuCheckpoint("fin:snapframe");
    if (renderSnapshotFrame(auxActiveBuffer, auxTargetBuffer))
    {
        std::swap(auxActiveBuffer, auxTargetBuffer);
    };

    sourceBuffer = auxActiveBuffer;
    // </FS:Beq>
    mLastPresentedLdrRT = sourceBuffer;
    LLVKLoader::gpuCheckpoint("fin:bufviz");
    if (RenderBufferVisualization > -1)
    {
        switch (RenderBufferVisualization)
        {
        case 0:
        case 1:
        case 2:
        case 3:
            visualizeBuffers(&getFrameRT()->deferredScreen, sourceBuffer, RenderBufferVisualization);
            break;
        case 4:
            visualizeBuffers(&mLuminanceMap, sourceBuffer, 0);
            break;
        case 5:
        {
            if (RenderFSAAType > 0)
            {
                visualizeBuffers(&mFXAAMap, sourceBuffer, 0);
            }
            break;
        }
        case 6:
        {
            if (RenderFSAAType == 2)
            {
                visualizeBuffers(&mSMAABlendBuffer, sourceBuffer, 0);
            }
            break;
        }
        // <AYAstorm r30 P2> velocity buffer visualization
        case 7:
        {
            if (mVelocityMap.isComplete())
            {
                visualizeBuffers(&mVelocityMap, sourceBuffer, 0);
            }
            break;
        }
        // </AYAstorm r30 P2>
        default:
            break;
        }
    }

    // Present the screen target.

    LLVKLoader::gpuCheckpoint("fin:present_composite");
    if (mVkSnapshotRedirectTarget)
    {
        mVkSnapshotRedirectTarget->bindTarget();
    }
    else if (mScenePresentRedirect)
    {
        mScenePresentRedirect->bindTarget();
    }
    else
    {
        LLVKLoader::beginSwapchainRendering();
    }

    gDeferredPostNoDoFNoiseProgram.bind(); // Add noise as part of final render to screen pass to avoid damaging other post effects

    // Whatever is last in the above post processing chain should _always_ be rendered directly here.  If not, expect problems.
    gDeferredPostNoDoFNoiseProgram.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, sourceBuffer);
    gDeferredPostNoDoFNoiseProgram.bindTexture(LLShaderMgr::DEFERRED_DEPTH, &getFrameRT()->deferredScreen, true);

    // <AYAstorm r30 P4 step 4> BD chroma_str (vignette path runs when HAS_DOF_CHROMA==0)
    // <FS:AYAstorm r30 BD full port Phase 3.7 cat 01> Cinematic BD parity (0.0f)。spec §3.1。
    const F32 nodof_noise_chroma_str = gSavedSettings.getF32("RenderChromaStrength");
    // </FS:AYAstorm>
    // </AYAstorm r30 P4 step 4>

    if (LLVKLoader::isVulkanInitialized()
        && gDeferredPostNoDoFNoiseProgram.mVkPerProgramUBO != VK_NULL_HANDLE
        && gDeferredPostNoDoFNoiseProgram.mVkPerProgramUBOMapped != nullptr)
    {
        LLVKLoader::PostNoDoFF_PerProgramBind ubo_data = {};
        ubo_data.screen_res[0] = (F32)sourceBuffer->getWidth();
        ubo_data.screen_res[1] = (F32)sourceBuffer->getHeight();
        ubo_data.chroma_str    = nodof_noise_chroma_str;
        memcpy(gDeferredPostNoDoFNoiseProgram.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
    }

    {
        LLGLDepthTest depth_test(GL_TRUE, GL_TRUE, GL_ALWAYS);
        if (mVkSnapshotRedirectTarget || mScenePresentRedirect)
        {
            LLGLDisable snapshot_blend_off(GL_BLEND);
            mScreenTriangleVB->setBuffer();
            mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
        }
        else
        {
            mScreenTriangleVB->setBuffer();
            mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
        }
    }

    gDeferredPostNoDoFNoiseProgram.unbind();

    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    renderSnapshotGuidesOverlay(); // <FS:Beq/> Render snapshot guides as part of UI
    renderFocusPoint(); // <FS:Beq/> FIRE-32023 render focus point

    if (hasRenderDebugMask(LLPipeline::RENDER_DEBUG_PHYSICS_SHAPES))
    {
        renderPhysicsDisplay();
    }

    LLVertexBuffer::unbind();


    LLVKLoader::gpuCheckpoint("fin:end");

    // flush calls made to "addTrianglesDrawn" so far to stats machinery
    recordTrianglesDrawn();
}

