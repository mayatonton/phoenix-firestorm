/**
 * @file lldrawpoolwlsky.cpp
 * @brief LLDrawPoolWLSky class implementation
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "lldrawpoolwlsky.h"

#include "llerror.h"
#include "llface.h"
#include "llimage.h"
#include "llrender.h"
#include "llvkloader.h"
#include "llimagegl.h"
#include "llenvironment.h"
#include "llglslshader.h"
#include "llgl.h"

#include "llviewerregion.h"
#include "llviewershadermgr.h"
#include "llviewercamera.h"
#include "pipeline.h"
#include "llpipelineframecontext.h"
#include "llsky.h"
#include "llvowlsky.h"
#include "llsettingsvo.h"
#include "llviewercontrol.h"

extern bool gCubeSnapshot;


static LLGLSLShader* cloud_shader = NULL;
static LLGLSLShader* sky_shader   = NULL;
static LLGLSLShader* sun_shader   = NULL;
static LLGLSLShader* moon_shader  = NULL;

static float sStarTime;

LLDrawPoolWLSky::LLDrawPoolWLSky(void) :
    LLDrawPool(POOL_WL_SKY)
{
}

LLDrawPoolWLSky::~LLDrawPoolWLSky()
{
}

LLViewerTexture *LLDrawPoolWLSky::getDebugTexture()
{
    return NULL;
}

void LLDrawPoolWLSky::beginDeferredPass(S32 pass)
{
    sky_shader = &gDeferredWLSkyProgram;
    cloud_shader = &gDeferredWLCloudProgram;

    sun_shader = &gDeferredWLSunProgram;

    moon_shader = &gDeferredWLMoonProgram;
}

void LLDrawPoolWLSky::endDeferredPass(S32 pass)
{
    sky_shader   = nullptr;
    cloud_shader = nullptr;
    sun_shader   = nullptr;
    moon_shader  = nullptr;

    // clear the depth buffer so haze shaders can use unwritten depth as a mask
    LLRenderTarget::clearBoundTarget(GL_DEPTH_BUFFER_BIT);
}

void LLDrawPoolWLSky::renderDome(const LLVector3& camPosLocal, F32 camHeightLocal, LLGLSLShader * shader) const
{
    llassert_always(NULL != shader);

    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();

    //chop off translation
    if (LLPipelineFrameContext::getInstance().isReflectionPass() && camPosLocal.mV[2] > 256.f)
    {
        gGL.translatef(camPosLocal.mV[0], camPosLocal.mV[1], 256.f-camPosLocal.mV[2]*0.5f);
    }
    else
    {
        gGL.translatef(camPosLocal.mV[0], camPosLocal.mV[1], camPosLocal.mV[2]);
    }


    // the windlight sky dome works most conveniently in a coordinate system
    // where Y is up, so permute our basis vectors accordingly.
    gGL.rotatef(120.f, 1.f / F_SQRT3, 1.f / F_SQRT3, 1.f / F_SQRT3);

    gGL.scalef(0.333f, 0.333f, 0.333f);

    gGL.translatef(0.f,-camHeightLocal, 0.f);

    // Draw WL Sky
    gSky.mVOWLSkyp->drawDome();

    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.popMatrix();
}

extern LLPointer<LLImageGL> gEXRImage;

static bool use_hdri_sky()
{
    static LLCachedControl<F32> hdri_split(gSavedSettings, "RenderHDRISplitScreen", 1.f);
    static LLCachedControl<bool> irradiance_only(gSavedSettings, "RenderHDRIIrradianceOnly", false);

    return gCubeSnapshot && (!irradiance_only || !gPipeline.mReflectionMapManager.isRadiancePass()) ? gEXRImage.notNull() : // always use HDRI for reflection probes when available
        gEXRImage.notNull() ? hdri_split > 0.f : // fallback to EEP sky when split screen is zero
        false; // no HDRI available, always use EEP sky

}

void LLDrawPoolWLSky::writeWindlightAtmosUBOs()
{
    if (!LLVKLoader::isVulkanInitialized())
    {
        return;
    }
    LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();
    if (!psky)
    {
        return;
    }

    LLVKLoader::WindlightAtmos_PerProgramBind ubo_data = {};

    LLColor3 sunlight     = psky->getSunlightColor();
    LLColor3 moonlight    = psky->getMoonlightColor();
    LLColor4 totalAmbient = psky->getTotalAmbient();
    LLColor3 blue_horizon = psky->getBlueHorizon();
    LLColor3 blue_density = psky->getBlueDensity();
    LLColor3 glow         = psky->getGlow();
    LLVector3 light_norm  = LLVector3(LLEnvironment::instance().getClampedLightNorm().mV);

    LLColor3 r17_sun_mod = LLSettingsVOSky::getR17SunModulator(light_norm, psky.get());
    sunlight.mV[0]     *= r17_sun_mod.mV[0];
    sunlight.mV[1]     *= r17_sun_mod.mV[1];
    sunlight.mV[2]     *= r17_sun_mod.mV[2];
    totalAmbient.mV[0] *= r17_sun_mod.mV[0];
    totalAmbient.mV[1] *= r17_sun_mod.mV[1];
    totalAmbient.mV[2] *= r17_sun_mod.mV[2];

    {
        bool irradiance_pass_aa = gCubeSnapshot && !gPipeline.mReflectionMapManager.isRadiancePass();
        static LLCachedControl<bool> should_auto_adjust_aa(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
        if (!irradiance_pass_aa && psky->getReflectionProbeAmbiance() == 0.f
            && psky->canAutoAdjust() && should_auto_adjust_aa())
        {
            static LLCachedControl<F32> auto_adjust_ambient_scale(gSavedSettings, "RenderSkyAutoAdjustAmbientScale", 0.75f);
            static LLCachedControl<F32> auto_adjust_blue_horizon_scale(gSavedSettings, "RenderSkyAutoAdjustBlueHorizonScale", 1.f);
            static LLCachedControl<F32> auto_adjust_blue_density_scale(gSavedSettings, "RenderSkyAutoAdjustBlueDensityScale", 1.f);
            static LLCachedControl<F32> auto_adjust_sun_color_scale(gSavedSettings, "RenderSkyAutoAdjustSunColorScale", 1.f);
            F32 amb_s = (F32)auto_adjust_ambient_scale;
            F32 sun_s = (F32)auto_adjust_sun_color_scale;
            F32 bh_s  = (F32)auto_adjust_blue_horizon_scale;
            F32 bd_s  = (F32)auto_adjust_blue_density_scale;
            totalAmbient.mV[0] *= amb_s; totalAmbient.mV[1] *= amb_s; totalAmbient.mV[2] *= amb_s;
            sunlight.mV[0]     *= sun_s; sunlight.mV[1]     *= sun_s; sunlight.mV[2]     *= sun_s;
            blue_horizon.mV[0] *= bh_s;  blue_horizon.mV[1] *= bh_s;  blue_horizon.mV[2] *= bh_s;
            blue_density.mV[0] *= bd_s;  blue_density.mV[1] *= bd_s;  blue_density.mV[2] *= bd_s;
        }
    }

    static LLCachedControl<bool> desaturate_irradiance(gSavedSettings, "RenderDesaturateIrradiance", true);
    if (desaturate_irradiance && gCubeSnapshot && !gPipeline.mReflectionMapManager.isRadiancePass())
    {
        F32 h, s, l;
        blue_horizon.calcHSL(&h, &s, &l);
        blue_horizon.mV[0] = blue_horizon.mV[1] = blue_horizon.mV[2] = l;
        blue_density.calcHSL(&h, &s, &l);
        blue_density.mV[0] = blue_density.mV[1] = blue_density.mV[2] = l;
    }

    if (gCubeSnapshot && !gPipeline.mReflectionMapManager.isRadiancePass())
    {
        totalAmbient.mV[0] = totalAmbient.mV[1] = totalAmbient.mV[2] = 0.f;
    }

    ubo_data.sunlight_color[0] = sunlight.mV[0];
    ubo_data.sunlight_color[1] = sunlight.mV[1];
    ubo_data.sunlight_color[2] = sunlight.mV[2];
    ubo_data.sun_up_factor     = psky->getIsSunUp() ? 1 : 0;

    ubo_data.moonlight_color[0] = moonlight.mV[0];
    ubo_data.moonlight_color[1] = moonlight.mV[1];
    ubo_data.moonlight_color[2] = moonlight.mV[2];
    static LLCachedControl<bool> should_auto_adjust_wl(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
    ubo_data.classic_mode_wl    = (psky->canAutoAdjust() && !should_auto_adjust_wl()) ? 1 : 0;

    ubo_data.ambient_color[0] = totalAmbient.mV[0];
    ubo_data.ambient_color[1] = totalAmbient.mV[1];
    ubo_data.ambient_color[2] = totalAmbient.mV[2];

    static LLCachedControl<U32> aya_visual_realism(gSavedSettings, "AYAVisualRealismEnabled", 1);
    ubo_data.aya_visual_realism_enabled = (aya_visual_realism() == 1) ? 1 : 0;

    ubo_data.blue_horizon[0] = blue_horizon.mV[0];
    ubo_data.blue_horizon[1] = blue_horizon.mV[1];
    ubo_data.blue_horizon[2] = blue_horizon.mV[2];

    static LLCachedControl<bool> aya_r14_vol(gSavedSettings, "AYAR14VolumetricAtmosphereInCinematicEnabled", false);
    bool r14_on = (aya_visual_realism() == 1) || (aya_visual_realism() == 2 && aya_r14_vol);
    ubo_data.aya_r14_volumetric_atmosphere_enabled = r14_on ? 1 : 0;

    ubo_data.blue_density[0] = blue_density.mV[0];
    ubo_data.blue_density[1] = blue_density.mV[1];
    ubo_data.blue_density[2] = blue_density.mV[2];

    static LLCachedControl<F32> aya_r14_strength(gSavedSettings, "AYAR14Strength", 1.0f);
    ubo_data.aya_r14_strength = llclamp((F32)aya_r14_strength, 0.f, 1.f);

    ubo_data.glow[0] = glow.mV[0];
    ubo_data.glow[1] = glow.mV[1];
    ubo_data.glow[2] = glow.mV[2];

    static LLCachedControl<F32> aya_r16_strength(gSavedSettings, "AYAR16AerialPerspectiveStrength", 1.0f);
    ubo_data.aya_r16_strength = llclamp((F32)aya_r16_strength, 0.f, 1.f);

    ubo_data.lightnorm[0] = light_norm.mV[0];
    ubo_data.lightnorm[1] = light_norm.mV[1];
    ubo_data.lightnorm[2] = light_norm.mV[2];

    static LLCachedControl<bool> aya_r16_aerial(gSavedSettings, "AYAR16AerialPerspectiveEnabled", true);
    static LLCachedControl<bool> aya_r16_in_cinematic(gSavedSettings, "AYAR16AerialPerspectiveInCinematicEnabled", false);
    bool r16_on = (aya_visual_realism() == 1 && aya_r16_aerial)
               || (aya_visual_realism() == 2 && aya_r16_in_cinematic);
    ubo_data.aya_r16_aerial_perspective_enabled = r16_on ? 1 : 0;

    ubo_data.haze_density         = (F32)psky->getHazeDensity();
    ubo_data.density_multiplier   = (F32)psky->getDensityMultiplier();
    ubo_data.distance_multiplier  = (F32)psky->getDistanceMultiplier();
    ubo_data.max_y                = (F32)psky->getMaxY();
    ubo_data.haze_horizon         = (F32)psky->getHazeHorizon();
    ubo_data.cloud_shadow         = (F32)psky->getCloudShadow();
    ubo_data.sun_moon_glow_factor = (F32)psky->getSunMoonGlowFactor();

    static LLCachedControl<bool> hdr(gSavedSettings, "RenderHDREnabled");
    static LLCachedControl<F32> sunlight_scale(gSavedSettings, "RenderSkySunlightScale", 1.5f);
    static LLCachedControl<F32> sunlight_hdr_scale(gSavedSettings, "RenderHDRSkySunlightScale", 1.5f);
    static LLCachedControl<F32> ambient_scale(gSavedSettings, "RenderSkyAmbientScale", 1.5f);
    ubo_data.sky_sunlight_scale = hdr ? (F32)sunlight_hdr_scale : (F32)sunlight_scale;
    ubo_data.sky_ambient_scale  = (F32)ambient_scale;

    LLVKLoader::writeCurrentWindlightAtmosUBO(ubo_data);

    LLVKLoader::WindlightHDR_PerProgramBind hdr_data = {};
    hdr_data.sky_hdr_scale = LLPipeline::sLastSkyHdrScale;
    LLVKLoader::writeCurrentWindlightHDRUBO(hdr_data);
}

void LLDrawPoolWLSky::renderSkyHazeDeferred(const LLVector3& camPosLocal, F32 camHeightLocal) const
{
    if (!gSky.mVOSkyp)
    {
        return;
    }

    LLVector3 const & origin = LLViewerCamera::getInstance()->getOrigin();

    if (gPipeline.canUseWindLightShaders() && gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SKY))
    {
        if (use_hdri_sky())
        {
            sky_shader = &gEnvironmentMapProgram;
            sky_shader->bind();
            sky_shader->rotatePerProgramUBOSlot();
            S32 idx = sky_shader->enableTexture(LLShaderMgr::ENVIRONMENT_MAP);
            if (idx > -1)
            {
                gGL.getTexUnit(idx)->bind(gEXRImage);
            }

            static LLCachedControl<F32> hdri_exposure(gSavedSettings, "RenderHDRIExposure", 0.0f);
            static LLCachedControl<F32> hdri_rotation(gSavedSettings, "RenderHDRIRotation", 0.f);
            static LLCachedControl<F32> hdri_split(gSavedSettings, "RenderHDRISplitScreen", 1.f);

            LLMatrix3 rot;
            rot.setRot(0.f, hdri_rotation*DEG_TO_RAD, 0.f);

            if (LLVKLoader::isVulkanInitialized()
                && sky_shader->mVkPerProgramUBO != VK_NULL_HANDLE
                && sky_shader->mVkPerProgramUBOMapped != nullptr)
            {
                U8* ubo_base = (U8*) sky_shader->mVkActivePerProgramUBOMapped;
                const F32 hdr_scale_val    = powf(2.f, hdri_exposure);
                const F32 split_screen_val = gCubeSnapshot ? 1.f : (F32) hdri_split;
                memcpy(ubo_base + 32, &hdr_scale_val,    sizeof(F32));
                memcpy(ubo_base + 36, &split_screen_val, sizeof(F32));
                for (S32 c = 0; c < 3; ++c)
                {
                    memcpy(ubo_base + 48 + c * 16, &rot.mMatrix[c][0], 3 * sizeof(F32));
                }
            }
        }
        else
        {
            sky_shader->bind();
            sky_shader->rotatePerProgramUBOSlot();
        }

        LLGLSPipelineDepthTestSkyBox sky(true, true);

        LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();

        LLDrawPoolWLSky::writeWindlightAtmosUBOs();

        LLViewerTexture* rainbow_tex = gSky.mVOSkyp->getRainbowTex();
        LLViewerTexture* halo_tex  = gSky.mVOSkyp->getHaloTex();

        sky_shader->bindTexture(LLShaderMgr::RAINBOW_MAP, rainbow_tex);
        sky_shader->bindTexture(LLShaderMgr::HALO_MAP,  halo_tex);

        F32 moisture_level  = (float)psky->getSkyMoistureLevel();
        F32 droplet_radius  = (float)psky->getSkyDropletRadius();
        F32 ice_level       = (float)psky->getSkyIceLevel();

        // hobble halos and rainbows when there's no light source to generate them
        if (!psky->getIsSunUp() && !psky->getIsMoonUp())
        {
            moisture_level = 0.0f;
            ice_level      = 0.0f;
        }

        if (LLVKLoader::isVulkanInitialized() && sky_shader->mVkPerProgramUBO != VK_NULL_HANDLE
            && sky_shader->mVkPerProgramUBOMapped != nullptr)
        {
            U8* ubo_base = (U8*) sky_shader->mVkActivePerProgramUBOMapped;
            const F32 cam_pos_local[3] = { 0.f, camHeightLocal, 0.f };
            const S32 cube_snap        = gCubeSnapshot ? 1 : 0;
            memcpy(ubo_base + 0,  cam_pos_local,   3 * sizeof(F32));
            memcpy(ubo_base + 12, &cube_snap,      sizeof(S32));
            memcpy(ubo_base + 16, &moisture_level, sizeof(F32));
            memcpy(ubo_base + 20, &droplet_radius, sizeof(F32));
            memcpy(ubo_base + 24, &ice_level,      sizeof(F32));
        }

        /// Render the skydome
        renderDome(origin, camHeightLocal, sky_shader);

        sky_shader->unbind();
    }
}

void LLDrawPoolWLSky::renderStarsDeferred(const LLVector3& camPosLocal) const
{
    if (!gSky.mVOSkyp || use_hdri_sky())
    {
        return;
    }

    LLGLSPipelineBlendSkyBox gls_sky(true, false);

    gGL.setSceneBlendType(LLRender::BT_ADD_WITH_ALPHA);

    F32 star_alpha = LLEnvironment::instance().getCurrentSky()->getStarBrightness() / 500.0f;

    // If start_brightness is not set, exit
    if(star_alpha < 0.001f)
    {
        LL_DEBUGS("SKY") << "star_brightness below threshold." << LL_ENDL;
        return;
    }

    gDeferredStarProgram.bind();

    LLViewerTexture* tex_a = gSky.mVOSkyp->getBloomTex();
    LLViewerTexture* tex_b = gSky.mVOSkyp->getBloomTexNext();

    F32 blend_factor = (F32)LLEnvironment::instance().getCurrentSky()->getBlendFactor();

    if (tex_a && (!tex_b || (tex_a == tex_b)))
    {
        // Bind current and next sun textures
        gGL.getTexUnit(0)->bind(tex_a);
        gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);
        blend_factor = 0;
    }
    else if (tex_b && !tex_a)
    {
        gGL.getTexUnit(0)->bind(tex_b);
        gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);
        blend_factor = 0;
    }
    else if (tex_b != tex_a)
    {
        gGL.getTexUnit(0)->bind(tex_a);
        gGL.getTexUnit(1)->bind(tex_b);
    }

    gGL.pushMatrix();
    gGL.translatef(camPosLocal.mV[0], camPosLocal.mV[1], camPosLocal.mV[2]);
    gGL.rotatef(gFrameTimeSeconds*0.01f, 0.f, 0.f, 1.f);
    if (LLPipelineFrameContext::getInstance().isReflectionPass())
    {
        star_alpha = 1.0f;
    }
    sStarTime = (F32)LLFrameTimer::getElapsedSeconds() * 0.5f;

    if (LLVKLoader::isVulkanInitialized())
    {
        LLVKLoader::StarTime_PerShaderBind star_time = {};
        star_time.time         = sStarTime;
        star_time.blend_factor = blend_factor;
        star_time.custom_alpha = star_alpha;
        LLVKLoader::writeCurrentStarTimeUBO(star_time);
    }

    gSky.mVOWLSkyp->drawStars();

    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);

    gDeferredStarProgram.unbind();

    gGL.popMatrix();
}

void LLDrawPoolWLSky::renderSkyCloudsDeferred(const LLVector3& camPosLocal, F32 camHeightLocal, LLGLSLShader* cloudshader) const
{
    if (use_hdri_sky())
    {
        return;
    }

    if (gPipeline.canUseWindLightShaders() && gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_CLOUDS) && gSky.mVOSkyp && gSky.mVOSkyp->getCloudNoiseTex())
    {
        LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();

        LLGLSPipelineBlendSkyBox pipeline(true, true);

        cloudshader->bind();

        LLPointer<LLViewerTexture> cloud_noise      = gSky.mVOSkyp->getCloudNoiseTex();
        LLPointer<LLViewerTexture> cloud_noise_next = gSky.mVOSkyp->getCloudNoiseTexNext();

        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);

        F32 cloud_variance = psky ? (F32)psky->getCloudVariance() : 0.0f;
        F32 blend_factor   = psky ? (F32)psky->getBlendFactor() : 0.0f;

        if (psky->getCloudScrollRate().isExactlyZero())
        {
            blend_factor = 0.f;
        }

        // if we even have sun disc textures to work with...
        if (cloud_noise || cloud_noise_next)
        {
            if (cloud_noise && (!cloud_noise_next || (cloud_noise == cloud_noise_next)))
            {
                // Bind current and next sun textures
                cloudshader->bindTexture(LLShaderMgr::CLOUD_NOISE_MAP, cloud_noise, LLTexUnit::TT_TEXTURE);
                blend_factor = 0;
            }
            else if (cloud_noise_next && !cloud_noise)
            {
                cloudshader->bindTexture(LLShaderMgr::CLOUD_NOISE_MAP, cloud_noise_next, LLTexUnit::TT_TEXTURE);
                blend_factor = 0;
            }
            else if (cloud_noise_next != cloud_noise)
            {
                cloudshader->bindTexture(LLShaderMgr::CLOUD_NOISE_MAP, cloud_noise, LLTexUnit::TT_TEXTURE);
                cloudshader->bindTexture(LLShaderMgr::CLOUD_NOISE_MAP_NEXT, cloud_noise_next, LLTexUnit::TT_TEXTURE);
            }
        }

        if (LLVKLoader::isVulkanInitialized() && cloudshader->mVkPerProgramUBO != VK_NULL_HANDLE
            && cloudshader->mVkPerProgramUBOMapped != nullptr)
        {
            LLVKLoader::Cloud_PerProgramBind ubo_data = {};

            ubo_data.camPosLocal[0] = 0.f;
            ubo_data.camPosLocal[1] = camHeightLocal;
            ubo_data.camPosLocal[2] = 0.f;

            LLVector3 cloud_light_norm = LLVector3(LLEnvironment::instance().getClampedLightNorm().mV);
            LLColor3  r17_sun_mod = LLSettingsVOSky::getR17SunModulator(cloud_light_norm, psky.get());
            LLColor3  cloud_color = psky ? psky->getCloudColor() : LLColor3(1.f, 1.f, 1.f);
            ubo_data.cloud_color[0] = cloud_color.mV[0] * r17_sun_mod.mV[0];
            ubo_data.cloud_color[1] = cloud_color.mV[1] * r17_sun_mod.mV[1];
            ubo_data.cloud_color[2] = cloud_color.mV[2] * r17_sun_mod.mV[2];

            ubo_data.cloud_scale_v  = psky ? (F32)psky->getCloudScale() : 1.0f;

            LLColor3  cpd1 = psky ? psky->getCloudPosDensity1() : LLColor3(0.f, 0.f, 0.f);
            LLVector4 vect_c_p_d1(cpd1.mV[0], cpd1.mV[1], cpd1.mV[2]);
            LLVector4 cloud_scroll(LLEnvironment::instance().getCloudScrollDelta());
            cloud_scroll[0] = -cloud_scroll[0];
            vect_c_p_d1 += cloud_scroll;
            ubo_data.cloud_pos_density1[0] = vect_c_p_d1.mV[0];
            ubo_data.cloud_pos_density1[1] = vect_c_p_d1.mV[1];
            ubo_data.cloud_pos_density1[2] = vect_c_p_d1.mV[2];

            LLColor3  cpd2 = psky ? psky->getCloudPosDensity2() : LLColor3(0.f, 0.f, 0.f);
            ubo_data.cloud_pos_density2[0] = cpd2.mV[0];
            ubo_data.cloud_pos_density2[1] = cpd2.mV[1];
            ubo_data.cloud_pos_density2[2] = cpd2.mV[2];

            ubo_data.blend_factor   = blend_factor;
            ubo_data.cloud_scale    = psky ? (F32)psky->getCloudScale() : 1.0f;
            ubo_data.cloud_variance = cloud_variance;

            static LLCachedControl<U32>  aya_master(gSavedSettings, "AYAVisualRealismEnabled", 1);
            static LLCachedControl<bool> aya_r18_cloud_vol(gSavedSettings, "AYAR18CloudVolumetricEnabled", true);
            static LLCachedControl<bool> aya_r18_in_cinematic(gSavedSettings, "AYAR18CloudVolumetricInCinematicEnabled", false);
            static LLCachedControl<F32>  aya_r18_strength(gSavedSettings, "AYAR18CloudVolumetricStrength", 1.0f);
            bool is_legacy_midday = (psky && psky->getAssetId() == LLEnvironment::KNOWN_SKY_LEGACY_MIDDAY);
            bool r18_on = ((aya_master() == 1 && aya_r18_cloud_vol)
                        || (aya_master() == 2 && aya_r18_in_cinematic)) && !is_legacy_midday;
            ubo_data.aya_r18_cloud_volumetric_enabled = r18_on ? 1 : 0;
            ubo_data.aya_r18_strength = llclamp((F32)aya_r18_strength, 0.f, 1.f);

            memcpy(cloudshader->mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
        }

        /// Render the skydome
        renderDome(camPosLocal, camHeightLocal, cloudshader);

        cloudshader->unbind();

        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);
    }
}

void LLDrawPoolWLSky::renderHeavenlyBodies()
{
    if (!gSky.mVOSkyp || use_hdri_sky()) return;

    LLGLSPipelineBlendSkyBox gls_skybox(true, true); // SL-14113 we need moon to write to depth to clip stars behind

    LLVector3 const & origin = LLViewerCamera::getInstance()->getOrigin();
    gGL.pushMatrix();
    gGL.translatef(origin.mV[0], origin.mV[1], origin.mV[2]);

    LLFace * face = gSky.mVOSkyp->mFace[LLVOSky::FACE_SUN];

    F32 blend_factor = (F32)LLEnvironment::instance().getCurrentSky()->getBlendFactor();
    bool can_use_vertex_shaders = gPipeline.shadersLoaded();
    bool can_use_windlight_shaders = gPipeline.canUseWindLightShaders();


    if (gSky.mVOSkyp->getSun().getDraw() && face && face->getGeomCount())
    {
        LLPointer<LLViewerTexture> tex_a = face->getTexture(LLRender::DIFFUSE_MAP);
        LLPointer<LLViewerTexture> tex_b = face->getTexture(LLRender::ALTERNATE_DIFFUSE_MAP);

        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);

        // if we even have sun disc textures to work with...
        if (tex_a || tex_b)
        {
            // if and only if we have a texture defined, render the sun disc
            if (can_use_vertex_shaders && can_use_windlight_shaders)
            {
                sun_shader->bind();

                if (tex_a && (!tex_b || (tex_a == tex_b)))
                {
                    // Bind current and next sun textures
                    sun_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, tex_a, LLTexUnit::TT_TEXTURE);
                    blend_factor = 0;
                }
                else if (tex_b && !tex_a)
                {
                    sun_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, tex_b, LLTexUnit::TT_TEXTURE);
                    blend_factor = 0;
                }
                else if (tex_b != tex_a)
                {
                    sun_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, tex_a, LLTexUnit::TT_TEXTURE);
                    sun_shader->bindTexture(LLShaderMgr::ALTERNATE_DIFFUSE_MAP, tex_b, LLTexUnit::TT_TEXTURE);
                }

                if (LLVKLoader::isVulkanInitialized() && sun_shader->mVkPerProgramUBO != VK_NULL_HANDLE
                    && sun_shader->mVkPerProgramUBOMapped != nullptr)
                {
                    LLVKLoader::SunDiscF_PerProgramBind ubo_data = {};
                    ubo_data.blend_factor = blend_factor;
                    memcpy(sun_shader->mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
                }

                face->renderIndexed();

                gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
                gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);

                sun_shader->unbind();
            }
        }
    }

    face = gSky.mVOSkyp->mFace[LLVOSky::FACE_MOON];

    if (gSky.mVOSkyp->getMoon().getDraw() && face && face->getTexture(LLRender::DIFFUSE_MAP) && face->getGeomCount() && moon_shader)
    {
        LLViewerTexture* tex_a = face->getTexture(LLRender::DIFFUSE_MAP);
        LLViewerTexture* tex_b = face->getTexture(LLRender::ALTERNATE_DIFFUSE_MAP);

        LLColor4 color(gSky.mVOSkyp->getMoon().getInterpColor());

        if (can_use_vertex_shaders && can_use_windlight_shaders && (tex_a || tex_b))
        {
            moon_shader->bind();

            if (tex_a && (!tex_b || (tex_a == tex_b)))
            {
                // Bind current and next sun textures
                moon_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, tex_a, LLTexUnit::TT_TEXTURE);
                //blend_factor = 0;
            }
            else if (tex_b && !tex_a)
            {
                moon_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, tex_b, LLTexUnit::TT_TEXTURE);
                //blend_factor = 0;
            }
            else if (tex_b != tex_a)
            {
                moon_shader->bindTexture(LLShaderMgr::DIFFUSE_MAP, tex_a, LLTexUnit::TT_TEXTURE);
                //moon_shader->bindTexture(LLShaderMgr::ALTERNATE_DIFFUSE_MAP, tex_b, LLTexUnit::TT_TEXTURE);
            }

            LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();

            F32 moon_brightness = (float)psky->getMoonBrightness();

            if (LLVKLoader::isVulkanInitialized() && moon_shader->mVkPerProgramUBO != VK_NULL_HANDLE
                && moon_shader->mVkPerProgramUBOMapped != nullptr)
            {
                LLVKLoader::MoonF_PerProgramBind ubo_data = {};
                ubo_data.color[0]      = color.mV[0];
                ubo_data.color[1]      = color.mV[1];
                ubo_data.color[2]      = color.mV[2];
                ubo_data.color[3]      = color.mV[3];
                const LLVector3 mdir   = psky->getMoonDirection();
                ubo_data.moon_dir[0]   = mdir.mV[0];
                ubo_data.moon_dir[1]   = mdir.mV[1];
                ubo_data.moon_dir[2]   = mdir.mV[2];
                ubo_data.moon_brightness = moon_brightness;
                memcpy(moon_shader->mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
            }

            face->renderIndexed();

            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
            gGL.getTexUnit(1)->unbind(LLTexUnit::TT_TEXTURE);

            moon_shader->unbind();
        }
    }

    gGL.popMatrix();
}

void LLDrawPoolWLSky::renderDeferred(S32 pass)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL; //LL_RECORD_BLOCK_TIME(FTM_RENDER_WL_SKY);
    if (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_SKY) || gSky.mVOSkyp.isNull())
    {
        return;
    }

    // TODO: remove gSky.mVOSkyp and fold sun/moon into LLVOWLSky
    gSky.mVOSkyp->updateGeometry(gSky.mVOSkyp->mDrawable);

    const F32 camHeightLocal = LLEnvironment::instance().getCamHeight();

    LLVector3 const & origin = LLViewerCamera::getInstance()->getOrigin();

    if (gPipeline.canUseWindLightShaders())
    {
        renderSkyHazeDeferred(origin, camHeightLocal);
        renderHeavenlyBodies();
        if (!gCubeSnapshot)
        {
            renderStarsDeferred(origin);
        }

        if (!gCubeSnapshot || gPipeline.mReflectionMapManager.isRadiancePass()) // don't draw clouds in irradiance maps to avoid popping
        {
            renderSkyCloudsDeferred(origin, camHeightLocal, cloud_shader);
        }
    }
}



LLViewerTexture* LLDrawPoolWLSky::getTexture()
{
    return NULL;
}

void LLDrawPoolWLSky::resetDrawOrders()
{
}

//static
void LLDrawPoolWLSky::cleanupGL()
{
}

//static
void LLDrawPoolWLSky::restoreGL()
{
}

