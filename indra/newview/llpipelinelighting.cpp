/**
 * @file llpipelinelighting.cpp
 * @brief Rendering pipeline: deferred lighting (pure move from pipeline.cpp).
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

static void notifySSAOShadowSkippedOnce(const std::string& message)
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

static F32 calc_light_dist(LLVOVolume* light, const LLVector3& cam_pos, F32 max_dist)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    F32 inten = light->getLightIntensity();
    if (inten < .001f)
    {
        return max_dist;
    }
    bool selected = light->isSelected();
    if (selected)
    {
        return 0.f; // selected lights get highest priority
    }
    F32 radius = light->getLightRadius();
    F32 dist = dist_vec(light->getRenderPosition(), cam_pos);
    dist = llmax(dist - radius, 0.f);
    if (light->mDrawable.notNull() && light->mDrawable->isState(LLDrawable::ACTIVE))
    {
        // moving lights get a little higher priority (too much causes artifacts)
        dist = llmax(dist - radius * 0.25f, 0.f);
    }
    return dist;
}

void LLPipeline::removeMutedAVsLights(LLVOAvatar* muted_avatar)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    light_set_t::iterator iter = gPipeline.mNearbyLights.begin();
    while (iter != gPipeline.mNearbyLights.end())
    {
        const LLViewerObject* vobj = iter->drawable->getVObj();
        if (vobj
            && vobj->getAvatar()
            && vobj->isAttachment()
            && vobj->getAvatar() == muted_avatar)
        {
            gPipeline.mLights.erase(iter->drawable);
            iter = gPipeline.mNearbyLights.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

// </AYAstorm r30 P2>

// <AYAstorm r30 P3 step 4> Volumetric Lighting (godrays) — BD lineage 995a1354d8.
// Adds shadow-accumulated god rays from the sun direction to the tonemapped
// color buffer. Atmosphere + shadow uniforms (sun_dir, blue_density,
// haze_density, sunlight_color, shadowMap[0..3], shadowMatrix[0..3]) are
// auto-bound by bindDeferredShader() via the calculatesAtmospherics /
// hasAtmospherics / hasShadows feature flags set in llviewershadermgr.cpp.
// Alpha channel left untouched (setColorMask(true, false)) to honor the
// AYAstorm visual-realism alpha-protect rule. Caller pong-chains the result.
void LLPipeline::renderVolumetric(LLRenderTarget* src)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    LL_PROFILE_GPU_ZONE("volumetric light");

    {
        LLRTScope rts(*src, false, "volumetric");
        if (rts)
        {
            llSetGLViewport(0, 0, src->getWidth(), src->getHeight());

            gGL.setColorMask(true, false);

            LLGLEnable blend(GL_BLEND);
            gGL.blendFunc(LLRender::BF_ONE, LLRender::BF_ONE, LLRender::BF_ONE, LLRender::BF_ONE);

            bindDeferredShader(gVolumetricLightProgram);

            static LLCachedControl<U32> godray_res(gSavedSettings, "RenderVolumetricLightingResolution", 16);
            static LLCachedControl<F32> godray_mult(gSavedSettings, "RenderVolumetricLightingMultiplier", 4.0f);
            static LLCachedControl<F32> falloff_mult(gSavedSettings, "RenderVolumetricLightingFalloffMultiplier", 2.0f);

            if (LLVKLoader::isVulkanInitialized()
                && gVolumetricLightProgram.mVkPerProgramUBOMapped != nullptr
                && gVolumetricLightProgram.mVkPerProgramUBOSize >= sizeof(LLVKLoader::VolumetricLightF_PerProgramBind))
            {
                LLVKLoader::VolumetricLightF_PerProgramBind ubo_data{};
                ubo_data.godray_res             = (S32)godray_res;
                ubo_data.godray_multiplier      = (F32)godray_mult;
                ubo_data.falloff_multiplier     = (F32)falloff_mult;
                ubo_data._volumetricLightF_pad0 = 0.f;
                std::memcpy(gVolumetricLightProgram.vkPerProgramBaseWritePtr(),
                            &ubo_data,
                            sizeof(ubo_data));
            }

            mScreenTriangleVB->setBuffer();
            mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

            unbindDeferredShader(gVolumetricLightProgram);
        }
    }

    gGL.setColorMask(true, true);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);
}

// Once-per-frame setup of hardware lights,
// including sun/moon, avatar backlight, and up to 6 local lights

void LLPipeline::setupAvatarLights(bool for_edit)
{
    assertInitialized();

    LLEnvironment& environment = LLEnvironment::instance();
    LLSettingsSky::ptr_t psky = environment.getCurrentSky();

    bool sun_up = environment.getIsSunUp();


    if (for_edit)
    {
        LLColor4 diffuse(1.f, 1.f, 1.f, 0.f);
        LLVector4 light_pos_cam(-8.f, 0.25f, 10.f, 0.f);  // w==0 => directional light
        LLMatrix4 camera_mat = LLViewerCamera::getInstance()->getModelview();
        LLMatrix4 camera_rot(camera_mat.getMat3());
        camera_rot.invert();
        LLVector4 light_pos = light_pos_cam * camera_rot;

        light_pos.normalize();

        LLLightState* light = gGL.getLight(1);

        mHWLightColors[1] = diffuse;

        light->setDiffuse(diffuse);
        light->setAmbient(LLColor4::black);
        light->setSpecular(LLColor4::black);
        light->setPosition(light_pos);
        light->setConstantAttenuation(1.f);
        light->setLinearAttenuation(0.f);
        light->setQuadraticAttenuation(0.f);
        light->setSpotExponent(0.f);
        light->setSpotCutoff(180.f);
    }
    else if (gAvatarBacklight)
    {
        LLVector3 light_dir = sun_up ? LLVector3(mSunDir) : LLVector3(mMoonDir);
        LLVector3 opposite_pos = -light_dir;
        LLVector3 orthog_light_pos = light_dir % LLVector3::z_axis;
        LLVector4 backlight_pos = LLVector4(lerp(opposite_pos, orthog_light_pos, 0.3f), 0.0f);
        backlight_pos.normalize();

        LLColor4 light_diffuse = sun_up ? mSunDiffuse : mMoonDiffuse;

        LLColor4 backlight_diffuse(1.f - light_diffuse.mV[VRED], 1.f - light_diffuse.mV[VGREEN], 1.f - light_diffuse.mV[VBLUE], 1.f);
        F32 max_component = 0.001f;
        for (S32 i = 0; i < 3; i++)
        {
            if (backlight_diffuse.mV[i] > max_component)
            {
                max_component = backlight_diffuse.mV[i];
            }
        }
        F32 backlight_mag;
        if (LLEnvironment::instance().getIsSunUp())
        {
            backlight_mag = BACKLIGHT_DAY_MAGNITUDE_OBJECT;
        }
        else
        {
            backlight_mag = BACKLIGHT_NIGHT_MAGNITUDE_OBJECT;
        }
        backlight_diffuse *= backlight_mag / max_component;

        mHWLightColors[1] = backlight_diffuse;

        LLLightState* light = gGL.getLight(1);

        light->setPosition(backlight_pos);
        light->setDiffuse(backlight_diffuse);
        light->setAmbient(LLColor4::black);
        light->setSpecular(LLColor4::black);
        light->setConstantAttenuation(1.f);
        light->setLinearAttenuation(0.f);
        light->setQuadraticAttenuation(0.f);
        light->setSpotExponent(0.f);
        light->setSpotCutoff(180.f);
    }
    else
    {
        LLLightState* light = gGL.getLight(1);

        mHWLightColors[1] = LLColor4::black;

        light->setDiffuse(LLColor4::black);
        light->setAmbient(LLColor4::black);
        light->setSpecular(LLColor4::black);
    }
}

void LLPipeline::calcNearbyLights(LLCamera& camera)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    assertInitialized();

    if (isFrameReflectionPass() || gCubeSnapshot || isFrameHUDPass() || LLApp::isExiting())
    {
        return;
    }

    static LLCachedControl<S32> local_light_count(gSavedSettings, "RenderLocalLightCount", 256);

    if (local_light_count >= 1)
    {
        // mNearbyLight (and all light_set_t's) are sorted such that
        // begin() == the closest light and rbegin() == the farthest light
        const S32 MAX_LOCAL_LIGHTS = 6;
        LLVector3 cam_pos = camera.getOrigin();

        F32 max_dist;
        if (isFrameRenderingDeferred())
        {
            max_dist = RenderFarClip;
        }
        else
        {
            max_dist = llmin(RenderFarClip, LIGHT_MAX_RADIUS * 4.f);
        }

        // UPDATE THE EXISTING NEARBY LIGHTS
        light_set_t cur_nearby_lights;
        for (light_set_t::iterator iter = mNearbyLights.begin();
            iter != mNearbyLights.end(); iter++)
        {
            const Light* light = &(*iter);
            LLDrawable* drawable = light->drawable;
            const LLViewerObject *vobj = light->drawable->getVObj();
            if(vobj && vobj->getAvatar()
               && (vobj->getAvatar()->isTooComplex() || vobj->getAvatar()->isInMuteList() || vobj->getAvatar()->isTooSlow())
               )
            {
                drawable->clearState(LLDrawable::NEARBY_LIGHT);
                continue;
            }

            LLVOVolume* volight = drawable->getVOVolume();
            if (!volight || !drawable->isState(LLDrawable::LIGHT))
            {
                drawable->clearState(LLDrawable::NEARBY_LIGHT);
                continue;
            }
            if (light->fade <= -LIGHT_FADE_TIME)
            {
                drawable->clearState(LLDrawable::NEARBY_LIGHT);
                continue;
            }
            // <FS:AYAstorm:r30-bd-port> Phase 6 step 2: Cinematic uses BD per-owner light split.
            if (isCinematicMode())
            {
                if (volight->isAttachment())
                {
                    LLVOAvatar* av = volight->getAvatar();
                    if ((!sRenderOtherAttachedLights && (av != gAgentAvatarp))
                        || (!sRenderOwnAttachedLights && (av == gAgentAvatarp)))
                    {
                        drawable->clearState(LLDrawable::NEARBY_LIGHT);
                        continue;
                    }
                }
                else if (!sRenderDeferredLights)
                {
                    drawable->clearState(LLDrawable::NEARBY_LIGHT);
                    continue;
                }
            }
            else
            // </FS:AYAstorm:r30-bd-port>
            if (!sRenderAttachedLights && volight && volight->isAttachment())
            {
                drawable->clearState(LLDrawable::NEARBY_LIGHT);
                continue;
            }

            F32 dist = calc_light_dist(volight, cam_pos, max_dist);
            F32 fade = light->fade;
            // actual fade gets decreased/increased by setupHWLights
            // light->fade value is 'time'.
            // >=0 and light will become visible as value increases
            // <0 and light will fade out
            if (dist < max_dist)
            {
                if (fade < 0)
                {
                    // mark light to fade in
                    // if fade was -LIGHT_FADE_TIME - it was fully invisible
                    // if fade -0 - it was fully visible
                    // visibility goes up from 0 to LIGHT_FADE_TIME.
                    fade += LIGHT_FADE_TIME;
                }
            }
            else
            {
                // mark light to fade out
                // visibility goes down from -0 to -LIGHT_FADE_TIME.
                if (fade >= LIGHT_FADE_TIME)
                {
                    fade = -0.0001f; // was fully visible
                }
                else if (fade >= 0)
                {
                    // 0.75 visible light should stay 0.75 visible, but should reverse direction
                    fade -= LIGHT_FADE_TIME;
                }
            }
            cur_nearby_lights.insert(Light(drawable, dist, fade));
        }
        mNearbyLights = cur_nearby_lights;

        // FIND NEW LIGHTS THAT ARE IN RANGE
        light_set_t new_nearby_lights;
        for (LLDrawable::ordered_drawable_set_t::iterator iter = mLights.begin();
             iter != mLights.end(); ++iter)
        {
            LLDrawable* drawable = *iter;
            LLVOVolume* light = drawable->getVOVolume();
            if (!light || drawable->isState(LLDrawable::NEARBY_LIGHT))
            {
                continue;
            }
            if (light->isHUDAttachment())
            {
                continue; // no lighting from HUD objects
            }
            // <FS:AYAstorm:r30-bd-port> Phase 6 step 2
            if (isCinematicMode())
            {
                if (light->isAttachment())
                {
                    LLVOAvatar* av_bd = light->getAvatar();
                    if ((!sRenderOtherAttachedLights && (av_bd != gAgentAvatarp))
                        || (!sRenderOwnAttachedLights && (av_bd == gAgentAvatarp)))
                    {
                        continue;
                    }
                }
                else if (!sRenderDeferredLights)
                {
                    continue;
                }
            }
            else
            // </FS:AYAstorm:r30-bd-port>
            if (!sRenderAttachedLights && light && light->isAttachment())
            {
                continue;
            }
            LLVOAvatar * av = light->getAvatar();
            if (av && (av->isTooComplex() || av->isInMuteList() || av->isTooSlow()))
            {
                // avatars that are already in the list will be removed by removeMutedAVsLights
                continue;
            }
            F32 dist = calc_light_dist(light, cam_pos, max_dist);
            if (dist >= max_dist)
            {
                continue;
            }
            new_nearby_lights.insert(Light(drawable, dist, 0.f));
            if (!isFrameRenderingDeferred() && new_nearby_lights.size() > (U32)MAX_LOCAL_LIGHTS)
            {
                new_nearby_lights.erase(--new_nearby_lights.end());
                const Light& last = *new_nearby_lights.rbegin();
                max_dist = last.dist;
            }
        }

        // INSERT ANY NEW LIGHTS
        for (light_set_t::iterator iter = new_nearby_lights.begin();
             iter != new_nearby_lights.end(); iter++)
        {
            const Light* light = &(*iter);
            if (isFrameRenderingDeferred() || mNearbyLights.size() < (U32)MAX_LOCAL_LIGHTS)
            {
                mNearbyLights.insert(*light);
                ((LLDrawable*) light->drawable)->setState(LLDrawable::NEARBY_LIGHT);
            }
            else
            {
                // crazy cast so that we can overwrite the fade value
                // even though gcc enforces sets as const
                // (fade value doesn't affect sort so this is safe)
                Light* farthest_light = (const_cast<Light*>(&(*(mNearbyLights.rbegin()))));
                if (light->dist < farthest_light->dist)
                {
                    // mark light to fade out
                    // visibility goes down from -0 to -LIGHT_FADE_TIME.
                    //
                    // This is a mess, but for now it needs to be in sync
                    // with fade code above. Ex: code above detects distance < max,
                    // sets fade time to positive, this code then detects closer
                    // lights and sets fade time negative, fully compensating
                    // for the code above
                    if (farthest_light->fade >= LIGHT_FADE_TIME)
                    {
                        farthest_light->fade = -0.0001f; // was fully visible
                    }
                    else if (farthest_light->fade >= 0)
                    {
                        farthest_light->fade -= LIGHT_FADE_TIME;
                    }
                }
                else
                {
                    break; // none of the other lights are closer
                }
            }
        }

        //mark nearby lights not-removable.
        for (light_set_t::iterator iter = mNearbyLights.begin();
             iter != mNearbyLights.end(); iter++)
        {
            const Light* light = &(*iter);
            ((LLViewerOctreeEntryData*) light->drawable)->setVisible();
        }
    }
}

void LLPipeline::setupHWLights()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    assertInitialized();

    if (isFrameHUDPass())
    {
        return;
    }

    F32 light_scale = 1.f;

    if (gCubeSnapshot)
    { //darken local lights when probe ambiance is above 1
        light_scale = mReflectionMapManager.mLightScale;
    }


    LLEnvironment& environment = LLEnvironment::instance();
    LLSettingsSky::ptr_t psky = environment.getCurrentSky();

    // <FS:AYA r17> Color Temperature (revert: P1.a 復活): 太陽 elevation から派生する Kelvin modulator を
    //   scene path (LLPipeline::mSunDiffuse / mHWLightColors[0] / setAmbientLightColor) に適用。
    //   sky path (skyV.glsl uniform) は llsettingsvo.cpp::applySpecial で同じ helper を呼ぶため、
    //   scene と sky で同色温度が適用される設計。
    //   「昼間(レガシー)」(KNOWN_SKY_LEGACY_MIDDAY) では helper 内で asset UUID 一致 pinpoint
    //   除外で no-op に落ち、PBR 前 noon 再現 preset の意図を歪めない。
    LLVector3 r17_lightnorm(environment.getClampedLightNorm().mV);
    LLColor3  r17_sun_mod = LLSettingsVOSky::getR17SunModulator(r17_lightnorm, psky.get());
    // </FS:AYA>

    // Ambient
    LLColor4 ambient = psky->getTotalAmbient();
    // <FS:AYA r17> ambient も連動して朝青/夕橙シフト
    ambient.mV[0] *= r17_sun_mod.mV[0];
    ambient.mV[1] *= r17_sun_mod.mV[1];
    ambient.mV[2] *= r17_sun_mod.mV[2];
    // </FS:AYA>

    gGL.setAmbientLightColor(ambient);

    bool sun_up  = environment.getIsSunUp();
    bool moon_up = environment.getIsMoonUp();

    // Light 0 = Sun or Moon (All objects)
    {
        LLVector4 sun_dir(environment.getSunDirection(), 0.0f);
        LLVector4 moon_dir(environment.getMoonDirection(), 0.0f);

        mSunDir.setVec(sun_dir);
        mMoonDir.setVec(moon_dir);

        mSunDiffuse.setVec(psky->getSunlightColor() * r17_sun_mod);  // <FS:AYA r17>
        mMoonDiffuse.setVec(psky->getMoonlightColor());

        F32 max_color = llmax(mSunDiffuse.mV[0], mSunDiffuse.mV[1], mSunDiffuse.mV[2]);
        if (max_color > 1.f)
        {
            mSunDiffuse *= 1.f/max_color;
        }
        mSunDiffuse.clamp();

        max_color = llmax(mMoonDiffuse.mV[0], mMoonDiffuse.mV[1], mMoonDiffuse.mV[2]);
        if (max_color > 1.f)
        {
            mMoonDiffuse *= 1.f/max_color;
        }
        mMoonDiffuse.clamp();

        // prevent underlighting from having neither lightsource facing us
        if (!sun_up && !moon_up)
        {
            mSunDiffuse.setVec(LLColor4(0.0, 0.0, 0.0, 1.0));
            mMoonDiffuse.setVec(LLColor4(0.0, 0.0, 0.0, 1.0));
            mSunDir.setVec(LLVector4(0.0, 1.0, 0.0, 0.0));
            mMoonDir.setVec(LLVector4(0.0, 1.0, 0.0, 0.0));
        }

        LLVector4 light_dir = sun_up ? mSunDir : mMoonDir;

        mHWLightColors[0] = sun_up ? mSunDiffuse : mMoonDiffuse;

        LLLightState* light = gGL.getLight(0);
        light->setPosition(light_dir);

        light->setSunPrimary(sun_up);
        light->setDiffuse(mHWLightColors[0]);
        light->setDiffuseB(mMoonDiffuse);
        light->setAmbient(psky->getTotalAmbient());
        light->setSpecular(LLColor4::black);
        light->setConstantAttenuation(1.f);
        light->setLinearAttenuation(0.f);
        light->setQuadraticAttenuation(0.f);
        light->setSpotExponent(0.f);
        light->setSpotCutoff(180.f);
    }

    // Light 1 = Backlight (for avatars)
    // (set by enableLightsAvatar)

    S32 cur_light = 2;

    // Nearby lights = LIGHT 2-7

    mLightMovingMask = 0;

    static LLCachedControl<S32> local_light_count(gSavedSettings, "RenderLocalLightCount", 256);

    if (local_light_count >= 1)
    {
        for (light_set_t::iterator iter = mNearbyLights.begin();
             iter != mNearbyLights.end(); ++iter)
        {
            LLDrawable* drawable = iter->drawable;
            LLVOVolume* light = drawable->getVOVolume();
            if (!light)
            {
                continue;
            }

            // <FS:AYAstorm:r30-bd-port> Phase 6 step 2
            if (isCinematicMode())
            {
                if (light->isAttachment())
                {
                    LLVOAvatar* av_bd = light->getAvatar();
                    if ((!sRenderOtherAttachedLights && (av_bd != gAgentAvatarp))
                        || (!sRenderOwnAttachedLights && (av_bd == gAgentAvatarp)))
                    {
                        continue;
                    }
                }
                else if (!sRenderDeferredLights)
                {
                    continue;
                }
            }
            else
            // </FS:AYAstorm:r30-bd-port>
            if (light->isAttachment())
            {
                if (!sRenderAttachedLights)
                {
                    continue;
                }
            }

            if (drawable->isState(LLDrawable::ACTIVE))
            {
                mLightMovingMask |= (1<<cur_light);
            }

            //send linear light color to shader
            LLColor4  light_color = light->getLightLinearColor() * light_scale;
            light_color.mV[3] = 0.0f;

            F32 fade = iter->fade;
            if (fade < LIGHT_FADE_TIME)
            {
                // fade in/out light
                if (fade >= 0.f)
                {
                    fade = fade / LIGHT_FADE_TIME;
                    ((Light*) (&(*iter)))->fade += gFrameIntervalSeconds.value();
                }
                else
                {
                    fade = 1.f + fade / LIGHT_FADE_TIME;
                    ((Light*) (&(*iter)))->fade -= gFrameIntervalSeconds.value();
                }
                fade = llclamp(fade,0.f,1.f);
                light_color *= fade;
            }

            if (light_color.magVecSquared() < 0.001f)
            {
                continue;
            }

            LLVector3 light_pos(light->getRenderPosition());
            LLVector4 light_pos_gl(light_pos, 1.0f);

            F32 adjusted_radius = light->getLightRadius() * (isFrameRenderingDeferred() ? 1.5f : 1.0f);
            if (adjusted_radius <= 0.001f)
            {
                continue;
            }

            F32 x = (3.f * (1.f + (light->getLightFalloff() * 2.0f)));  // why this magic?  probably trying to match a historic behavior.
            F32 linatten = x / adjusted_radius;                         // % of brightness at radius

            mHWLightColors[cur_light] = light_color;
            LLLightState* light_state = gGL.getLight(cur_light);

            light_state->setPosition(light_pos_gl);
            light_state->setDiffuse(light_color);
            light_state->setAmbient(LLColor4::black);
            light_state->setConstantAttenuation(0.f);
            light_state->setSize(light->getLightRadius() * 1.5f);
            light_state->setFalloff(light->getLightFalloff(DEFERRED_LIGHT_FALLOFF));

            if (isFrameRenderingDeferred())
            {
                light_state->setLinearAttenuation(linatten);
                light_state->setQuadraticAttenuation(light->getLightFalloff(DEFERRED_LIGHT_FALLOFF) + 1.f); // get falloff to match for forward deferred rendering lights
            }
            else
            {
                light_state->setLinearAttenuation(linatten);
                light_state->setQuadraticAttenuation(0.f);
            }


            if (light->isLightSpotlight() // directional (spot-)light
                && (isFrameRenderingDeferred() || RenderSpotLightsInNondeferred)) // these are only rendered as GL spotlights if we're in deferred rendering mode *or* the setting forces them on
            {
                LLQuaternion quat = light->getRenderRotation();
                LLVector3 at_axis(0,0,-1); // this matches deferred rendering's object light direction
                at_axis *= quat;

                light_state->setSpotDirection(at_axis);
                light_state->setSpotCutoff(90.f);
                light_state->setSpotExponent(2.f);

                LLVector3 spotParams = light->getSpotLightParams();

                const LLColor4 specular(0.f, 0.f, 0.f, spotParams[2]);
                light_state->setSpecular(specular);
            }
            else // omnidirectional (point) light
            {
                light_state->setSpotExponent(0.f);
                light_state->setSpotCutoff(180.f);

                // we use specular.z = 1.0 as a cheap hack for the shaders to know that this is omnidirectional rather than a spotlight
                const LLColor4 specular(0.f, 0.f, 1.f, 0.f);
                light_state->setSpecular(specular);
            }
            cur_light++;
            if (cur_light >= 8)
            {
                break; // safety
            }
        }
    }
    for ( ; cur_light < 8 ; cur_light++)
    {
        mHWLightColors[cur_light] = LLColor4::black;
        LLLightState* light = gGL.getLight(cur_light);
        light->setSunPrimary(true);
        light->setDiffuse(LLColor4::black);
        light->setAmbient(LLColor4::black);
        light->setSpecular(LLColor4::black);
    }

    // Bookmark comment to allow searching for mSpecialRenderMode == 3 (avatar edit mode),
    // prev site of forward (non-deferred) character light injection, removed by SL-13522 09/20

    // Init GL state
    for (S32 i = 0; i < 8; ++i)
    {
        gGL.getLight(i)->disable();
    }
    mLightMask = 0;
}

void LLPipeline::enableLights(U32 mask)
{
    assertInitialized();

    if (mLightMask != mask)
    {
        if (mask)
        {
            for (S32 i=0; i<8; i++)
            {
                LLLightState* light = gGL.getLight(i);
                if (mask & (1<<i))
                {
                    light->enable();
                    light->setDiffuse(mHWLightColors[i]);
                }
                else
                {
                    light->disable();
                    light->setDiffuse(LLColor4::black);
                }
            }
        }
        mLightMask = mask;
    }
}

void LLPipeline::enableLightsDynamic()
{
    assertInitialized();
    U32 mask = 0xff & (~2); // Local lights
    enableLights(mask);

    if (isAgentAvatarValid())
    {
        if (gAgentAvatarp->mSpecialRenderMode == 0) // normal
        {
            gPipeline.enableLightsAvatar();
        }
        else if (gAgentAvatarp->mSpecialRenderMode == 2)  // anim preview
        {
            gPipeline.enableLightsAvatarEdit(LLColor4(0.7f, 0.6f, 0.3f, 1.f));
        }
    }
}

void LLPipeline::enableLightsAvatar()
{
    U32 mask = 0xff; // All lights
    setupAvatarLights(false);
    enableLights(mask);
}

void LLPipeline::enableLightsPreview()
{
    disableLights();

    LLColor4 ambient = PreviewAmbientColor;
    gGL.setAmbientLightColor(ambient);

    LLColor4 diffuse0 = PreviewDiffuse0;
    LLColor4 specular0 = PreviewSpecular0;
    LLColor4 diffuse1 = PreviewDiffuse1;
    LLColor4 specular1 = PreviewSpecular1;
    LLColor4 diffuse2 = PreviewDiffuse2;
    LLColor4 specular2 = PreviewSpecular2;

    LLVector3 dir0 = PreviewDirection0;
    LLVector3 dir1 = PreviewDirection1;
    LLVector3 dir2 = PreviewDirection2;

    dir0.normVec();
    dir1.normVec();
    dir2.normVec();

    LLVector4 light_pos(dir0, 0.0f);

    LLLightState* light = gGL.getLight(1);

    light->enable();
    light->setPosition(light_pos);
    light->setDiffuse(diffuse0);
    light->setAmbient(ambient);
    light->setSpecular(specular0);
    light->setSpotExponent(0.f);
    light->setSpotCutoff(180.f);

    light_pos = LLVector4(dir1, 0.f);

    light = gGL.getLight(2);
    light->enable();
    light->setPosition(light_pos);
    light->setDiffuse(diffuse1);
    light->setAmbient(ambient);
    light->setSpecular(specular1);
    light->setSpotExponent(0.f);
    light->setSpotCutoff(180.f);

    light_pos = LLVector4(dir2, 0.f);
    light = gGL.getLight(3);
    light->enable();
    light->setPosition(light_pos);
    light->setDiffuse(diffuse2);
    light->setAmbient(ambient);
    light->setSpecular(specular2);
    light->setSpotExponent(0.f);
    light->setSpotCutoff(180.f);
}

void LLPipeline::enableLightsAvatarEdit(const LLColor4& color)
{
    U32 mask = 0x2002; // Avatar backlight only, set ambient
    setupAvatarLights(true);
    enableLights(mask);

    gGL.setAmbientLightColor(color);
}

void LLPipeline::enableLightsFullbright()
{
    assertInitialized();
    U32 mask = 0x1000; // Non-0 mask, set ambient
    enableLights(mask);
}

void LLPipeline::disableLights()
{
    enableLights(0); // no lighting (full bright)
}

//////////////////////////////
//
// Macros, functions, and inline methods from other classes
//
//

void LLPipeline::setLight(LLDrawable *drawablep, bool is_light)
{
    if (drawablep && assertInitialized())
    {
        if (is_light)
        {
            mLights.insert(drawablep);
            drawablep->setState(LLDrawable::LIGHT);
        }
        else
        {
            drawablep->clearState(LLDrawable::LIGHT);
            mLights.erase(drawablep);
        }
    }
}

void LLPipeline::bindLightFunc(LLGLSLShader& shader)
{
    S32 channel = shader.enableTexture(LLShaderMgr::DEFERRED_LIGHTFUNC);
    if (channel > -1)
    {
        gGL.getTexUnit(channel)->bind(mLightFunc);
    }

    channel = shader.enableTexture(LLShaderMgr::DEFERRED_BRDF_LUT, LLTexUnit::TT_TEXTURE);
    if (channel > -1)
    {
        mPbrBrdfLut.bindTexture(0, channel);
    }
}

void LLPipeline::bindDeferredShaderFast(LLGLSLShader& shader)
{
    if (shader.mCanBindFast)
    { // was previously fully bound, use fast path
        shader.bind();
        bindLightFunc(shader);
        bindShadowMaps(shader);
        bindReflectionProbes(shader);
    }
    else
    { //wasn't previously bound, use slow path
        bindDeferredShader(shader);
        shader.mCanBindFast = true;
    }
}

void LLPipeline::bindDeferredShader(LLGLSLShader& shader, LLRenderTarget* light_target, LLRenderTarget* depth_target)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LLRenderTarget* deferred_target       = &getFrameRT()->deferredScreen;
    LLRenderTarget* deferred_light_target = &getFrameRT()->deferredLight;

    shader.bind();
    S32 channel = 0;
    channel = shader.enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, deferred_target->getUsage());
    if (channel > -1)
    {
        deferred_target->bindTexture(0,channel, LLTexUnit::TFO_POINT); // frag_data[0]
        gGL.getTexUnit(channel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
    }

    channel = shader.enableTexture(LLShaderMgr::DEFERRED_SPECULAR, deferred_target->getUsage());
    if (channel > -1)
    {
        deferred_target->bindTexture(1, channel, LLTexUnit::TFO_POINT); // frag_data[1]
        gGL.getTexUnit(channel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
    }

    channel = shader.enableTexture(LLShaderMgr::NORMAL_MAP, deferred_target->getUsage());
    if (channel > -1)
    {
        deferred_target->bindTexture(2, channel, LLTexUnit::TFO_POINT); // frag_data[2]
        gGL.getTexUnit(channel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
    }

    channel = shader.enableTexture(LLShaderMgr::DEFERRED_EMISSIVE, deferred_target->getUsage());
    if (channel > -1)
    {
        deferred_target->bindTexture(3, channel, LLTexUnit::TFO_POINT); // frag_data[3]
        gGL.getTexUnit(channel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
    }

    channel = shader.enableTexture(LLShaderMgr::DEFERRED_DEPTH, deferred_target->getUsage());
    if (channel > -1)
    {
        if (depth_target)
        {
            gGL.getTexUnit(channel)->bind(depth_target, true);
        }
        else
        {
            gGL.getTexUnit(channel)->bind(deferred_target, true);
        }
    }


    channel = shader.enableTexture(LLShaderMgr::EXPOSURE_MAP);
    if (channel > -1)
    {
        gGL.getTexUnit(channel)->bind(&mExposureMap);
    }

    channel = shader.enableTexture(LLShaderMgr::DEFERRED_NOISE);
    if (channel > -1)
    {
        gGL.getTexUnit(channel)->bind(mNoiseMap);
        gGL.getTexUnit(channel)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
    }

    bindLightFunc(shader);


    light_target = light_target ? light_target : deferred_light_target;
    channel = shader.enableTexture(LLShaderMgr::DEFERRED_LIGHT, light_target->getUsage());
    if (channel > -1)
    {
        if (light_target->isComplete())
        {
            light_target->bindTexture(0, channel, LLTexUnit::TFO_POINT);
        }
        else
        {
            gGL.getTexUnit(channel)->bindFast(LLViewerFetchedTexture::sWhiteImagep);
        }
    }



    bindShadowMaps(shader);



    if (!isFrameReflectionProbesEnabled())
    {
        channel = shader.enableTexture(LLShaderMgr::ENVIRONMENT_MAP, LLTexUnit::TT_CUBE_MAP);
        if (channel > -1)
        {
            LLCubeMap* cube_map = gSky.mVOSkyp ? gSky.mVOSkyp->getCubeMap() : NULL;
            if (cube_map)
            {
                cube_map->enable(channel);
                cube_map->bind();
            }

            F32* m = gGLModelView;

            F32 mat[] = { m[0], m[1], m[2],
                          m[4], m[5], m[6],
                          m[8], m[9], m[10] };

            if (LLVKLoader::isVulkanInitialized())
            {
                LLSettingsSky::ptr_t psky_ref = LLEnvironment::instance().getCurrentSky();
                LLVKLoader::ReflectionProbe_PerProgramBind data = {};
                data.reflection_probe_ambiance = ayaDeriveReflectionProbeAmbianceVk(psky_ref, false);
                data.env_mat_col0[0] = mat[0]; data.env_mat_col0[1] = mat[3]; data.env_mat_col0[2] = mat[6];
                data.env_mat_col1[0] = mat[1]; data.env_mat_col1[1] = mat[4]; data.env_mat_col1[2] = mat[7];
                data.env_mat_col2[0] = mat[2]; data.env_mat_col2[1] = mat[5]; data.env_mat_col2[2] = mat[8];
                LLVKLoader::writeCurrentReflectionProbeUBO(data);
            }
        }
    }

    bindReflectionProbes(shader);

    if (LLVKLoader::isVulkanInitialized() && shader.mWritePerProgramUBOMinimumAlpha
        && shader.mVkPerProgramUBO != VK_NULL_HANDLE
        && shader.mVkPerProgramUBOMapped != nullptr
        && shader.mVkPerProgramUBOSize == LLVKLoader::ALPHAF_UBO_SIZE_NO_SHADOW)
    {
        char* mapped = (char*)shader.vkPerProgramBaseWritePtr();
        F32 sun_moon[8] = {
            mTransformedSunDir.mV[0],  mTransformedSunDir.mV[1],  mTransformedSunDir.mV[2],  0.f,
            mTransformedMoonDir.mV[0], mTransformedMoonDir.mV[1], mTransformedMoonDir.mV[2], 0.f,
        };
        memcpy(mapped + LLVKLoader::ALPHAF_UBO_OFFSET_SUN_MOON, sun_moon, sizeof(sun_moon));
    }

    if (LLVKLoader::isVulkanInitialized() && shader.mWritePerProgramUBOMinimumAlpha
        && shader.mVkPerProgramUBO != VK_NULL_HANDLE
        && shader.mVkPerProgramUBOMapped != nullptr
        && (shader.mVkPerProgramUBOSize == LLVKLoader::ALPHAF_UBO_SIZE_SHADOW
            || shader.mVkPerProgramUBOSize == LLVKLoader::ALPHAF_UBO_SIZE_NO_SHADOW))
    {
        char* mapped = (char*)shader.vkPerProgramBaseWritePtr();
        const F32 near_clip_v = LLViewerCamera::getInstance()->getNear() * 2.f;
        memcpy(mapped + LLVKLoader::ALPHAF_UBO_OFFSET_NEAR_CLIP, &near_clip_v, sizeof(F32));
    }

    if (LLVKLoader::isVulkanInitialized() && shader.mVkPerProgramUBO != VK_NULL_HANDLE
        && shader.mVkPerProgramUBOMapped != nullptr
        && shader.mVkPerProgramUBOSize == LLVKLoader::GLTFMR_UBO_SIZE_ALPHA_NOSHADOW)
    {
        char* mapped = (char*)shader.vkPerProgramBaseWritePtr();
        F32 sun_v[4]  = { mTransformedSunDir.mV[0],  mTransformedSunDir.mV[1],  mTransformedSunDir.mV[2],  0.f };
        F32 moon_v[4] = { mTransformedMoonDir.mV[0], mTransformedMoonDir.mV[1], mTransformedMoonDir.mV[2], 0.f };
        memcpy(mapped + LLVKLoader::GLTFMR_UBO_OFFSET_SUN_DIR,  sun_v,  16);
        memcpy(mapped + LLVKLoader::GLTFMR_UBO_OFFSET_MOON_DIR, moon_v, 16);
    }
}

// </AYAstorm:r21.1>

void LLPipeline::renderDeferredLighting()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LL_PROFILE_GPU_ZONE("renderDeferredLighting");
    LLVKLoader::gpuCheckpoint("renderDeferredLighting");
    if (!getFrameCull())
    {
        return;
    }

    llassert(!isFrameHUDPass());

    // <AYAstorm:r21.1> GPU self-rigged picker:
    // Write the self attachment LocalIDs into mObjectIDBuffer now — the
    // deferred gbuffer pass has just completed, so depth is final and
    // the rigged attachments are at their on-screen positions. Skip in
    // cube snapshot / reflection probe path (mObjectIDBuffer only exists
    // for the main RT pack, and the picker only ever reads it for the
    // main viewport).
    static LLCachedControl<bool> armed_mode(gSavedSettings, "FSSelfRiggedPickerArmedMode", true);
    if (!gCubeSnapshot && isOtherRiggedObjectIDBufferArmed())
    {
        renderOtherRiggedObjectIDBuffer();
    }
    else if (!gCubeSnapshot && (!armed_mode || isSelfRiggedObjectIDBufferArmed()))
    {
        renderSelfRiggedObjectIDBuffer();
    }
    // </AYAstorm:r21.1>

    const bool lgt_perf = LLVKLoader::perfLogEnabled() && !gCubeSnapshot;
    auto lgt_now = [&]() -> U64 { return lgt_perf ? (U64)LLTimer::getTotalTime() : 0; };
    U64 lgt_us[8] = {};
    U64 lgt_nl = 0, lgt_ns = 0;
    auto bindD = [&](LLGLSLShader& sh, LLRenderTarget* lt = nullptr, LLRenderTarget* dt = nullptr) {
        const U64 b = lgt_now();
        bindDeferredShader(sh, lt, dt);
        lgt_us[7] += lgt_now() - b;
    };

    F32 light_scale = 1.f;

    if (gCubeSnapshot)
    { //darken local lights when probe ambiance is above 1
        light_scale = mReflectionMapManager.mLightScale;
    }

    LLRenderTarget *screen_target         = &getFrameRT()->screen;
    LLRenderTarget* deferred_light_target = &getFrameRT()->deferredLight;

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("deferred");
        LLViewerCamera *camera = LLViewerCamera::getInstance();

        if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
        {
            gPipeline.toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
        }

        gGL.setColorMask(true, true);

        // draw a cube around every light
        LLVertexBuffer::unbind();

        LLGLEnable cull(GL_CULL_FACE);
        LLGLEnable blend(GL_BLEND);

        glm::mat4 mat = get_current_modelview();

        setupHWLights();  // to set mSun/MoonDir;

        glm::vec4 tc(mSunDir);
        tc = mat * tc;
        mTransformedSunDir.set(tc);

        glm::vec4 tc_moon(mMoonDir);
        tc_moon = mat * tc_moon;
        mTransformedMoonDir.set(tc_moon);

        LLDrawPoolWLSky::writeWindlightAtmosUBOs();

        if (LLVKLoader::isVulkanInitialized())
        {
            LLSettingsSky::ptr_t psky = LLEnvironment::instance().getCurrentSky();
            LLRenderTarget* deferred_target_for_du = &getFrameRT()->deferredScreen;
            LLVKLoader::DeferredUtil_PerProgramBind du = {};
            du.proj_mat[0]  = 1.f; du.proj_mat[5]  = 1.f;
            du.proj_mat[10] = 1.f; du.proj_mat[15] = 1.f;
            du.waterPlane[0] = LLDrawPoolAlpha::sWaterPlane.mV[0];
            du.waterPlane[1] = LLDrawPoolAlpha::sWaterPlane.mV[1];
            du.waterPlane[2] = LLDrawPoolAlpha::sWaterPlane.mV[2];
            du.waterPlane[3] = LLDrawPoolAlpha::sWaterPlane.mV[3];
            du.screen_res[0] = (F32)deferred_target_for_du->getWidth();
            du.screen_res[1] = (F32)deferred_target_for_du->getHeight();
            static LLCachedControl<bool> should_auto_adjust_du(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
            du.classic_mode  = (psky && psky->canAutoAdjust() && !should_auto_adjust_du()) ? 1 : 0;
            LLVKLoader::writeCurrentDeferredUtilUBO(du);
        }

        if (LLVKLoader::isVulkanInitialized())
        {
            F32 shadow_bias_error = RenderShadowBiasError * fabsf(LLViewerCamera::getInstance()->getOrigin().mV[2]) / 3000.f;
            F32 shadow_bias       = RenderShadowBias + shadow_bias_error;
            LLVKLoader::ShadowUtil_PerProgramBind su = {};
            for (U32 m = 0; m < 6; ++m)
            {
                const F32* mp = glm::value_ptr(mSunShadowMatrix[m]);
                for (U32 i = 0; i < 16; ++i)
                {
                    su.shadow_matrix[m * 16 + i] = mp[i];
                }
            }
            su.shadow_clip[0]  = mSunClipPlanes.mV[0];
            su.shadow_clip[1]  = mSunClipPlanes.mV[1];
            su.shadow_clip[2]  = mSunClipPlanes.mV[2];
            su.shadow_clip[3]  = mSunClipPlanes.mV[3];
            su.sun_dir[0]      = mTransformedSunDir.mV[0];
            su.sun_dir[1]      = mTransformedSunDir.mV[1];
            su.sun_dir[2]      = mTransformedSunDir.mV[2];
            su.shadow_bias     = shadow_bias;
            su.moon_dir[0]     = mTransformedMoonDir.mV[0];
            su.moon_dir[1]     = mTransformedMoonDir.mV[1];
            su.moon_dir[2]     = mTransformedMoonDir.mV[2];
            su.shadow_offset   = RenderShadowOffset;
            su.shadow_res[0]   = (F32)getFrameRT()->sunShadowLayered.getWidth();
            su.shadow_res[1]   = (F32)getFrameRT()->sunShadowLayered.getHeight();
            su.proj_shadow_res[0] = (F32)mSpotShadow[0].getWidth();
            su.proj_shadow_res[1] = (F32)mSpotShadow[0].getHeight();
            su.shadow_softness    = RenderShadowSoftness;
            su.spot_shadow_bias   = RenderSpotShadowBias;
            su.spot_shadow_offset = RenderSpotShadowOffset;
            LLVKLoader::writeCurrentShadowUtilUBO(su);
        }

        if (LLVKLoader::isVulkanInitialized())
        {
            LLVKLoader::WindlightSky_PerProgramBind ws = {};
            ws.sun_dir_sky[0]  = mTransformedSunDir.mV[0];
            ws.sun_dir_sky[1]  = mTransformedSunDir.mV[1];
            ws.sun_dir_sky[2]  = mTransformedSunDir.mV[2];
            ws.moon_dir_sky[0] = mTransformedMoonDir.mV[0];
            ws.moon_dir_sky[1] = mTransformedMoonDir.mV[1];
            ws.moon_dir_sky[2] = mTransformedMoonDir.mV[2];
            LLVKLoader::writeCurrentWindlightSkyUBO(ws);
        }

        if (LLVKLoader::isVulkanInitialized())
        {
            LLRenderTarget* deferred_target_for_ao = &getFrameRT()->deferredScreen;
            F32 ao_window_height = (F32)gViewerWindow->getWindowHeightRaw();
            F32 ao_target_height = (F32)deferred_target_for_ao->getHeight();
            F32 ao_screen_to_target_scale = (ao_target_height > 0.f) ?
                (ao_window_height / ao_target_height) : 1.f;
            F32 ao_ssao_factor = RenderSSAOFactor;
            LLVKLoader::AoUtil_PerProgramBind ao = {};
            ao.screen_res[0]    = (F32)deferred_target_for_ao->getWidth();
            ao.screen_res[1]    = ao_target_height;
            ao.ssao_radius      = RenderSSAOScale / ao_screen_to_target_scale;
            ao.ssao_max_radius  = RenderSSAOMaxScale / ao_screen_to_target_scale;
            ao.ssao_factor      = ao_ssao_factor;
            ao.ssao_factor_inv  = (ao_ssao_factor > 0.f) ? (1.f / ao_ssao_factor) : 0.f;
            LLVKLoader::writeCurrentAoUtilUBO(ao);
        }

        if (LLVKLoader::isVulkanInitialized())
        {
            LLVKLoader::WindlightLight_PerProgramBind wl = {};
            wl.scene_light_strength = LLPipeline::sLastSceneLightStrength;
            LLVKLoader::writeCurrentWindlightLightUBO(wl);
        }

        if (LLVKLoader::isVulkanInitialized())
        {
            LLVKLoader::GlobalF_PerProgramBind gf = {};
            gf.mirror_flag    = LLPipeline::sLastMirrorFlag;
            gf.clipPlane[0]   = LLPipeline::sLastClipPlane.mV[0];
            gf.clipPlane[1]   = LLPipeline::sLastClipPlane.mV[1];
            gf.clipPlane[2]   = LLPipeline::sLastClipPlane.mV[2];
            gf.clipPlane[3]   = LLPipeline::sLastClipPlane.mV[3];
            LLVKLoader::writeCurrentGlobalFUBO(gf);
        }

        if ((RenderDeferredSSAO && !gCubeSnapshot) || RenderShadowDetail > 0)
        {
            LL_PROFILE_GPU_ZONE("sun program");
            LLGLSLShader& sun_shader = gCubeSnapshot ? gDeferredSunProbeProgram : gDeferredSunProgram;
            if (!sun_shader.isComplete())
            {
                LL_WARNS_ONCE("Pipeline") << "Skipping SSAO/shadow light pass because the deferred sun shader failed to link." << LL_ENDL;
                notifySSAOShadowSkippedOnce("AYAstorm: SSAO/shadow smoothing was disabled because the deferred sun shader failed to load. Check AYAstorm.log for shader details.");
            }
            else
            {
                const U64 lgt_sun_t0 = lgt_now();
                {
                LLRTScope rts(*deferred_light_target, false, "sun_lightmap");
                if (rts)
                {  // paint shadow/SSAO light map (direct lighting lightmap)
                    LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("renderDeferredLighting - sun shadow");

                    bindD(sun_shader, deferred_light_target);
                    mScreenTriangleVB->setBuffer();
                    gGL.setClearColor(1, 1, 1, 1);
                    deferred_light_target->clear(GL_COLOR_BUFFER_BIT);
                    gGL.setClearColor(0, 0, 0, 0);

                    if (LLVKLoader::isVulkanInitialized() && sun_shader.mVkPerProgramUBO != VK_NULL_HANDLE
                        && sun_shader.mVkPerProgramUBOMapped != nullptr)
                    {
                        LLVKLoader::SunLightF_PerProgramBind ubo_data = {};
                        ubo_data.sun_dir[0]    = mTransformedSunDir.mV[0];
                        ubo_data.sun_dir[1]    = mTransformedSunDir.mV[1];
                        ubo_data.sun_dir[2]    = mTransformedSunDir.mV[2];
                        memcpy(sun_shader.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
                    }

                    {
                        LLGLDisable   blend(GL_BLEND);
                        LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_ALWAYS);
                        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
                    }

                    unbindDeferredShader(sun_shader);
                }
                }
                lgt_us[0] += lgt_now() - lgt_sun_t0;
            }
        }

        // <FS:AYAstorm r30 P4> RenderDeferredBlurLight gates the soften-shadow blur pass.
        // Shadow blur should remain available when SSAO is disabled; the sun
        // shader still writes directional/spot shadows into the light map.
        if ((RenderDeferredSSAO || RenderShadowDetail > 0) && RenderDeferredBlurLight && !gCubeSnapshot)
        // </FS:AYAstorm r30 P4>
        {
            const U64 lgt_blur_t0 = lgt_now();
            // soften direct lighting lightmap
            LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("renderDeferredLighting - soften shadow");
            LL_PROFILE_GPU_ZONE("soften shadow");
            if (LLVKLoader::isVulkanInitialized())
            {
                LLRenderTarget* gbuf = &getFrameRT()->deferredScreen;
                gbuf->bindForShaderRead(0);
                gbuf->bindForShaderRead(1);
                gbuf->bindForShaderRead(2);
                gbuf->bindForShaderRead(3);
                gbuf->bindForShaderRead(0, true);
            }
            // blur lightmap
            LLVector3 go = RenderShadowGaussian;
            const U32 kern_length = 4;
            F32       blur_size = RenderShadowBlurSize;
            F32       dist_factor = RenderShadowBlurDistFactor;

            // sample symmetrically with the middle sample falling exactly on 0.0
            F32 x = 0.f;

            LLVector3 gauss[32];  // xweight, yweight, offset

            for (U32 i = 0; i < kern_length; i++)
            {
                gauss[i].mV[0] = llgaussian(x, go.mV[0]);
                gauss[i].mV[1] = llgaussian(x, go.mV[1]);
                gauss[i].mV[2] = x;
                x += 1.f;
            }

            {
            LLRTScope rts(*screen_target, true, "blur_h");
            if (rts)
            {
            gGL.setClearColor(1, 1, 1, 1);
            screen_target->clear(GL_COLOR_BUFFER_BIT);
            gGL.setClearColor(0, 0, 0, 0);

            bindD(gDeferredBlurLightProgram);

            LLRenderTarget* blur_deferred_target_p1 = &getFrameRT()->deferredScreen;
            if (LLVKLoader::isVulkanInitialized()
                && gDeferredBlurLightProgram.mVkPerProgramUBO != VK_NULL_HANDLE
                && gDeferredBlurLightProgram.mVkPerProgramUBOMapped != nullptr)
            {
                LLVKLoader::BlurLightF_PerProgramBind ubo_data = {};
                for (U32 i = 0; i < kern_length; ++i)
                {
                    ubo_data.kern[i][0] = gauss[i].mV[0];
                    ubo_data.kern[i][1] = gauss[i].mV[1];
                    ubo_data.kern[i][2] = gauss[i].mV[2];
                }
                ubo_data.delta[0]      = 1.f;
                ubo_data.delta[1]      = 0.f;
                ubo_data.screen_res[0] = (F32)blur_deferred_target_p1->getWidth();
                ubo_data.screen_res[1] = (F32)blur_deferred_target_p1->getHeight();
                ubo_data.dist_factor   = dist_factor;
                ubo_data.blur_size     = RenderShadowBlurSize;
                ubo_data.kern_scale    = blur_size * (kern_length / 2.f - 0.5f);
                gDeferredBlurLightProgram.rotatePerProgramUBOSlot();
                memcpy(gDeferredBlurLightProgram.vkPerProgramActiveWritePtr(), &ubo_data, sizeof(ubo_data));
            }

            {
                LLGLDisable   blend(GL_BLEND);
                LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_ALWAYS);
                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            }

            }
            }
            unbindDeferredShader(gDeferredBlurLightProgram);

            bindD(gDeferredBlurLightProgram, screen_target);

            {
            LLRTScope rts(*deferred_light_target, false, "blur_v");
            if (rts)
            {

            LLRenderTarget* blur_deferred_target_p2 = &getFrameRT()->deferredScreen;
            if (LLVKLoader::isVulkanInitialized()
                && gDeferredBlurLightProgram.mVkPerProgramUBO != VK_NULL_HANDLE
                && gDeferredBlurLightProgram.mVkPerProgramUBOMapped != nullptr)
            {
                LLVKLoader::BlurLightF_PerProgramBind ubo_data = {};
                for (U32 i = 0; i < kern_length; ++i)
                {
                    ubo_data.kern[i][0] = gauss[i].mV[0];
                    ubo_data.kern[i][1] = gauss[i].mV[1];
                    ubo_data.kern[i][2] = gauss[i].mV[2];
                }
                ubo_data.delta[0]      = 0.f;
                ubo_data.delta[1]      = 1.f;
                ubo_data.screen_res[0] = (F32)blur_deferred_target_p2->getWidth();
                ubo_data.screen_res[1] = (F32)blur_deferred_target_p2->getHeight();
                ubo_data.dist_factor   = dist_factor;
                ubo_data.blur_size     = RenderShadowBlurSize;
                ubo_data.kern_scale    = blur_size * (kern_length / 2.f - 0.5f);
                gDeferredBlurLightProgram.rotatePerProgramUBOSlot();
                memcpy(gDeferredBlurLightProgram.vkPerProgramActiveWritePtr(), &ubo_data, sizeof(ubo_data));
            }

            {
                LLGLDisable   blend(GL_BLEND);
                LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_ALWAYS);
                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            }
            }
            }
            unbindDeferredShader(gDeferredBlurLightProgram);
            lgt_us[1] += lgt_now() - lgt_blur_t0;
        }

        if (LLVKLoader::isVulkanInitialized())
        {
            LLRenderTarget* gbuf = &getFrameRT()->deferredScreen;
            gbuf->bindForShaderRead(0);
            gbuf->bindForShaderRead(1);
            gbuf->bindForShaderRead(2);
            gbuf->bindForShaderRead(3);
            gbuf->bindForShaderRead(0, true);
        }
        {
        LLRTScope rts(getFrameRT()->screen, true, "deferred_screen");
        if (rts)
        {
        // clear color buffer here - zeroing alpha (glow) is important or it will accumulate against sky
        gGL.setClearColor(0, 0, 0, 0);
        screen_target->clear(GL_COLOR_BUFFER_BIT);

        if (RenderDeferredAtmospheric)
        {  // apply sunlight contribution
            const U64 lgt_atm_t0 = lgt_now();
            LLGLSLShader &soften_shader = gDeferredSoftenProgram;

            LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("renderDeferredLighting - atmospherics");
            LL_PROFILE_GPU_ZONE("atmospherics");
            bindD(soften_shader);


            static LLCachedControl<F32> ssao_scale(gSavedSettings, "RenderSSAOIrradianceScale", 0.5f);
            static LLCachedControl<F32> ssao_max(gSavedSettings, "RenderSSAOIrradianceMax", 0.25f);

            if (LLVKLoader::isVulkanInitialized()
                && soften_shader.mVkPerProgramUBO != VK_NULL_HANDLE
                && soften_shader.mVkPerProgramUBOMapped != nullptr)
            {
                const U32 ubo_size = soften_shader.mVkPerProgramUBOSize;
                const bool has_sun_moon = (ubo_size == 64 || ubo_size == 128);
                const bool has_ssao     = (ubo_size == 96 || ubo_size == 128);

                F32 ubo_buffer[32] = {};

                static const F32 r19_table[4][4] = {
                    { 0.00f, 1.0f, 1.0f, 0.0f },
                    { 0.15f, 2.0f, 4.0f, 0.6f },
                    { 0.25f, 1.5f, 3.0f, 1.0f },
                    { 0.35f, 1.2f, 2.5f, 1.5f },
                };
                static LLCachedControl<U32>  aya_realism_r19_vk(gSavedSettings, "AYAVisualRealismEnabled", 1);
                static LLCachedControl<bool> aya_r19_enabled_vk(gSavedSettings, "AYAR19TranslucencyEnabled", true);
                static LLCachedControl<U32>  aya_r19_tier_vk(gSavedSettings, "AYAR19TranslucencyIntensity", 1);
                static LLCachedControl<bool> aya_r19_in_cinematic_vk(gSavedSettings, "AYAR19TranslucencyInCinematicEnabled", false);
                bool r19_active_vk = (aya_realism_r19_vk() == 1 && aya_r19_enabled_vk())
                                  || (aya_realism_r19_vk() == 2 && aya_r19_in_cinematic_vk);
                U32 r19_tier_vk = r19_active_vk ? llmin<U32>(aya_r19_tier_vk(), 3u) : 0u;
                ubo_buffer[0] = r19_table[r19_tier_vk][0];
                ubo_buffer[1] = r19_table[r19_tier_vk][1];
                ubo_buffer[2] = r19_table[r19_tier_vk][2];
                ubo_buffer[3] = r19_table[r19_tier_vk][3];
                ubo_buffer[4] = 1.00f;
                ubo_buffer[5] = 0.78f;
                ubo_buffer[6] = 0.62f;
                ubo_buffer[7] = LLPipeline::sLastSkyHdrScale;

                U32 cursor = 8;

                if (has_sun_moon)
                {
                    ubo_buffer[cursor + 0] = mTransformedSunDir.mV[0];
                    ubo_buffer[cursor + 1] = mTransformedSunDir.mV[1];
                    ubo_buffer[cursor + 2] = mTransformedSunDir.mV[2];
                    ubo_buffer[cursor + 3] = 0.f;
                    ubo_buffer[cursor + 4] = mTransformedMoonDir.mV[0];
                    ubo_buffer[cursor + 5] = mTransformedMoonDir.mV[1];
                    ubo_buffer[cursor + 6] = mTransformedMoonDir.mV[2];
                    ubo_buffer[cursor + 7] = 0.f;
                    cursor += 8;
                }

                if (has_ssao)
                {
                    F32 matrix_diag    = (RenderSSAOEffect[0] + 2.0f*RenderSSAOEffect[1])/3.0f;
                    F32 matrix_nondiag = (RenderSSAOEffect[0] - RenderSSAOEffect[1])/3.0f;
                    ubo_buffer[cursor + 0]  = matrix_diag;
                    ubo_buffer[cursor + 1]  = matrix_nondiag;
                    ubo_buffer[cursor + 2]  = matrix_nondiag;
                    ubo_buffer[cursor + 3]  = 0.f;
                    ubo_buffer[cursor + 4]  = matrix_nondiag;
                    ubo_buffer[cursor + 5]  = matrix_diag;
                    ubo_buffer[cursor + 6]  = matrix_nondiag;
                    ubo_buffer[cursor + 7]  = 0.f;
                    ubo_buffer[cursor + 8]  = matrix_nondiag;
                    ubo_buffer[cursor + 9]  = matrix_nondiag;
                    ubo_buffer[cursor + 10] = matrix_diag;
                    ubo_buffer[cursor + 11] = 0.f;
                    ubo_buffer[cursor + 12] = (F32)ssao_scale;
                    ubo_buffer[cursor + 13] = (F32)ssao_max;
                    ubo_buffer[cursor + 14] = 0.f;
                    ubo_buffer[cursor + 15] = 0.f;
                    cursor += 16;
                }

                memcpy(soften_shader.vkPerProgramBaseWritePtr(), ubo_buffer, ubo_size);
            }

            {
                LLGLDepthTest depth(GL_FALSE);
                LLGLDisable   blend(GL_BLEND);

                // full screen blit
                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            }

            unbindDeferredShader(gDeferredSoftenProgram);
            lgt_us[2] += lgt_now() - lgt_atm_t0;
        }

        static LLCachedControl<S32> local_light_count(gSavedSettings, "RenderLocalLightCount", 256);
        static LLCachedControl<S32> probe_level(gSavedSettings, "RenderReflectionProbeLevel", 0);

        if (local_light_count > 0 && (!gCubeSnapshot || probe_level > 0))
        {
            gGL.setSceneBlendType(LLRender::BT_ADD);
            std::list<LLVector4>        fullscreen_lights;
            LLDrawable::drawable_list_t spot_lights;
            LLDrawable::drawable_list_t fullscreen_spot_lights;
            LLSettingsSky::ptr_t        psky        = LLEnvironment::instance().getCurrentSky();

            if (!gCubeSnapshot)
            {
                for (U32 i = 0; i < 2; i++)
                {
                    mTargetShadowSpotLight[i] = NULL;
                }
            }

            std::list<LLVector4> light_colors;

            LLVertexBuffer::unbind();

            {
                const U64 lgt_loc_t0 = lgt_now();
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("renderDeferredLighting - local lights");
                LL_PROFILE_GPU_ZONE("local lights");
                bindD(gDeferredLightProgram);

                if (mCubeVB.isNull())
                {
                    mCubeVB = ll_create_cube_vb(LLVertexBuffer::MAP_VERTEX);
                }

                mCubeVB->setBuffer();

                LLGLDepthTest depth(GL_TRUE, GL_FALSE);
                // mNearbyLights already includes distance calculation and excludes muted avatars.
                // It is calculated from mLights
                // mNearbyLights also provides fade value to gracefully fade-out out of range lights
                S32 count = 0;
                for (light_set_t::iterator iter = mNearbyLights.begin(); iter != mNearbyLights.end(); ++iter)
                {
                    count++;
                    if (count > local_light_count)
                    { //stop collecting lights once we hit the limit
                        break;
                    }

                    LLDrawable * drawablep = iter->drawable;
                    LLVOVolume * volume = drawablep->getVOVolume();
                    if (!volume)
                    {
                        continue;
                    }

                    // <FS:AYAstorm:r30-bd-port> Phase 6 step 2
                    if (isCinematicMode())
                    {
                        if (volume->isAttachment())
                        {
                            LLVOAvatar* av_bd = volume->getAvatarAncestor();
                            if ((!sRenderOtherAttachedLights && (av_bd != gAgentAvatarp))
                                || (!sRenderOwnAttachedLights && (av_bd == gAgentAvatarp)))
                            {
                                continue;
                            }
                        }
                        else if (!sRenderDeferredLights)
                        {
                            continue;
                        }
                    }
                    else
                    // </FS:AYAstorm:r30-bd-port>
                    if (volume->isAttachment())
                    {
                        if (!sRenderAttachedLights)
                        {
                            continue;
                        }
                    }

                    LLVector4a center;
                    center.load3(drawablep->getPositionAgent().mV);
                    const F32 *c = center.getF32ptr();
                    F32        s = volume->getLightRadius() * 1.5f;

                    // <FS:Beq> relocated above colour calc for early exit on small lights
                    if (s <= 0.001f)
                    {
                        continue;
                    }
                    // </FS:Beq>
                    // send light color to shader in linear space
                    LLColor3 col = volume->getLightLinearColor() * light_scale;

                    if (col.magVecSquared() < 0.001f)
                    {
                        continue;
                    }
                    // <FS:Beq> relocated above colour calc for early exit on small lights
                    // if (s <= 0.001f)
                    // {
                    //     continue;
                    // }
                    // </FS:Beq>
                    LLVector4a sa;
                    sa.splat(s);
                    if (camera->AABBInFrustumNoFarClip(center, sa) == 0)
                    {
                        continue;
                    }

                    sVisibleLightCount++;

                    if (camera->getOrigin().mV[0] > c[0] + s + 0.2f || camera->getOrigin().mV[0] < c[0] - s - 0.2f ||
                        camera->getOrigin().mV[1] > c[1] + s + 0.2f || camera->getOrigin().mV[1] < c[1] - s - 0.2f ||
                        camera->getOrigin().mV[2] > c[2] + s + 0.2f || camera->getOrigin().mV[2] < c[2] - s - 0.2f)
                    {  // draw box if camera is outside box
                        if (volume->isLightSpotlight())
                        {
                            drawablep->getVOVolume()->updateSpotLightPriority();
                            spot_lights.push_back(drawablep);
                            ++lgt_ns;
                            continue;
                        }

                        if (LLVKLoader::isVulkanInitialized()
                            && gDeferredLightProgram.mVkPerProgramUBO != VK_NULL_HANDLE)
                        {
                            gDeferredLightProgram.rotatePerProgramUBOSlot();
                            if (gDeferredLightProgram.vkPerProgramActiveWritePtr() != nullptr)
                            {
                                LLVKLoader::PointLightPerDraw ubo_data = {};
                                ubo_data.center[0]             = c[0];
                                ubo_data.center[1]             = c[1];
                                ubo_data.center[2]             = c[2];
                                ubo_data.size                  = s;
                                ubo_data.color[0]              = col.mV[0];
                                ubo_data.color[1]              = col.mV[1];
                                ubo_data.color[2]              = col.mV[2];
                                ubo_data.falloff               = volume->getLightFalloff(DEFERRED_LIGHT_FALLOFF);
                                ubo_data.global_light_strength = LLPipeline::RenderGlobalLightStrength;
                                ubo_data.classic_mode          = (psky && psky->canAutoAdjust()) ? 1 : 0;
                                memcpy(gDeferredLightProgram.vkPerProgramActiveWritePtr(), &ubo_data, sizeof(ubo_data));
                            }
                        }

                        gGL.syncMatrices();

                        mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, get_box_fan_indices(camera, center));
                        ++lgt_nl;
                    }
                    else
                    {
                        if (volume->isLightSpotlight())
                        {
                            drawablep->getVOVolume()->updateSpotLightPriority();
                            fullscreen_spot_lights.push_back(drawablep);
                            ++lgt_ns;
                            continue;
                        }

                        glm::vec3 tc(center);
                        tc = mul_mat4_vec3(mat, tc);

                        fullscreen_lights.push_back(LLVector4(tc.x, tc.y, tc.z, s));
                        light_colors.push_back(LLVector4(col.mV[0], col.mV[1], col.mV[2], volume->getLightFalloff(DEFERRED_LIGHT_FALLOFF)));
                        ++lgt_nl;
                    }
                }

                // Bookmark comment to allow searching for mSpecialRenderMode == 3 (avatar edit mode),
                // prev site of appended deferred character light, removed by SL-13522 09/20

                unbindDeferredShader(gDeferredLightProgram);
                lgt_us[3] += lgt_now() - lgt_loc_t0;
            }

            if (!spot_lights.empty())
            {
                const U64 lgt_spot_t0 = lgt_now();
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("renderDeferredLighting - projectors");
                LL_PROFILE_GPU_ZONE("projectors");
                LLGLDepthTest depth(GL_TRUE, GL_FALSE);
                bindD(gDeferredSpotLightProgram);

                mCubeVB->setBuffer();

                gDeferredSpotLightProgram.enableTexture(LLShaderMgr::DEFERRED_PROJECTION);

                for (LLDrawable::drawable_list_t::iterator iter = spot_lights.begin(); iter != spot_lights.end(); ++iter)
                {
                    LLDrawable *drawablep = *iter;

                    LLVOVolume *volume = drawablep->getVOVolume();

                    LLVector4a center;
                    center.load3(drawablep->getPositionAgent().mV);
                    const F32* c = center.getF32ptr();
                    F32        s = volume->getLightRadius() * 1.5f;

                    sVisibleLightCount++;

                    F32 spot_proj_origin[3] = { 0.f, 0.f, 0.f };
                    F32 spot_shadow_fade = 0.f;
                    S32 spot_proj_shadow_idx = -1;
                    SpotProjForVk spot_proj = {};
                    setupSpotLight(gDeferredSpotLightProgram, drawablep,
                                   spot_proj_origin, &spot_shadow_fade, &spot_proj_shadow_idx, &spot_proj);

                    // send light color to shader in linear space
                    LLColor3 col = volume->getLightLinearColor() * light_scale;

                    if (LLVKLoader::isVulkanInitialized()
                        && gDeferredSpotLightProgram.mVkPerProgramUBO != VK_NULL_HANDLE)
                    {
                        gDeferredSpotLightProgram.rotatePerProgramUBOSlot();
                        if (gDeferredSpotLightProgram.vkPerProgramActiveWritePtr() != nullptr)
                        {
                            LLVKLoader::SpotLightPerDraw sd = {};
                            sd.center[0] = c[0]; sd.center[1] = c[1]; sd.center[2] = c[2];
                            sd.size                  = s;
                            sd.proj_origin[0]        = spot_proj_origin[0];
                            sd.proj_origin[1]        = spot_proj_origin[1];
                            sd.proj_origin[2]        = spot_proj_origin[2];
                            sd.falloff               = volume->getLightFalloff(DEFERRED_LIGHT_FALLOFF);
                            sd.shadow_fade           = spot_shadow_fade;
                            sd.global_light_strength = LLPipeline::RenderGlobalLightStrength;
                            sd.proj_shadow_idx       = spot_proj_shadow_idx;
                            sd.classic_mode          = (psky && psky->canAutoAdjust()) ? 1 : 0;
                            memcpy(gDeferredSpotLightProgram.vkPerProgramActiveWritePtr(), &sd, sizeof(sd));
                        }
                        VkBuffer du_ov_buf = VK_NULL_HANDLE; void* du_ov_map = nullptr;
                        if (LLVKLoader::acquireDeferredUtilOverrideSlot(du_ov_buf, du_ov_map) && du_ov_map)
                        {
                            LLVKLoader::DeferredUtil_PerProgramBind* du =
                                (LLVKLoader::DeferredUtil_PerProgramBind*)du_ov_map;
                            memcpy(du->proj_mat, spot_proj.proj_mat, sizeof(du->proj_mat));
                            du->proj_n[0] = spot_proj.proj_n[0]; du->proj_n[1] = spot_proj.proj_n[1]; du->proj_n[2] = spot_proj.proj_n[2];
                            du->proj_focus = spot_proj.proj_focus;
                            du->proj_p[0] = spot_proj.proj_p[0]; du->proj_p[1] = spot_proj.proj_p[1]; du->proj_p[2] = spot_proj.proj_p[2];
                            du->proj_lod = spot_proj.proj_lod;
                            du->color[0] = col.mV[0]; du->color[1] = col.mV[1]; du->color[2] = col.mV[2];
                            du->size = s;
                            du->proj_range = spot_proj.proj_range;
                            du->proj_ambiance = spot_proj.proj_ambiance;
                            LLVKLoader::setDeferredUtilOverrideSlot(du_ov_buf, du_ov_map);
                        }
                    }

                    gGL.syncMatrices();

                    mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, get_box_fan_indices(camera, center));

                    LLVKLoader::clearDeferredUtilOverrideSlot();
                }
                gDeferredSpotLightProgram.disableTexture(LLShaderMgr::DEFERRED_PROJECTION);
                unbindDeferredShader(gDeferredSpotLightProgram);
                lgt_us[4] += lgt_now() - lgt_spot_t0;
            }

            {
                const U64 lgt_fsl_t0 = lgt_now();
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("renderDeferredLighting - fullscreen lights");
                LLGLDepthTest depth(GL_FALSE);
                LL_PROFILE_GPU_ZONE("fullscreen lights");

                U32 count = 0;

                const U32 max_count = LL_DEFERRED_MULTI_LIGHT_COUNT;
                LLVector4 light[max_count];
                LLVector4 col[max_count];

                F32 far_z = 0.f;

                while (!fullscreen_lights.empty())
                {
                    light[count] = fullscreen_lights.front();
                    fullscreen_lights.pop_front();
                    col[count] = light_colors.front();
                    light_colors.pop_front();

                    far_z = llmin(light[count].mV[2] - light[count].mV[3], far_z);
                    count++;
                    if (count == max_count || fullscreen_lights.empty())
                    {
                        U32 idx = count - 1;
                        bindD(gDeferredMultiLightProgram[idx]);
                        if (LLVKLoader::isVulkanInitialized()
                            && gDeferredMultiLightProgram[idx].mVkPerProgramUBO != VK_NULL_HANDLE)
                        {
                            gDeferredMultiLightProgram[idx].rotatePerProgramUBOSlot();
                            if (gDeferredMultiLightProgram[idx].vkPerProgramActiveWritePtr() != nullptr)
                            {
                                U8 buf[16 * 32 + 16] = {};
                                U32 light_arr_bytes = count * 16;
                                memcpy(buf, light, light_arr_bytes);
                                memcpy(buf + light_arr_bytes, col, light_arr_bytes);
                                F32 footer[4] = { far_z, LLPipeline::RenderGlobalLightStrength, 0.f, 0.f };
                                memcpy(buf + light_arr_bytes * 2, footer, sizeof(footer));
                                U32 ubo_size = light_arr_bytes * 2 + 16;
                                memcpy(gDeferredMultiLightProgram[idx].vkPerProgramActiveWritePtr(), buf, ubo_size);
                            }
                        }
                        far_z = 0.f;
                        count = 0;
                        mScreenTriangleVB->setBuffer();
                        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
                        unbindDeferredShader(gDeferredMultiLightProgram[idx]);
                    }
                }

                bindD(gDeferredMultiSpotLightProgram);

                gDeferredMultiSpotLightProgram.enableTexture(LLShaderMgr::DEFERRED_PROJECTION);

                mScreenTriangleVB->setBuffer();

                for (LLDrawable::drawable_list_t::iterator iter = fullscreen_spot_lights.begin(); iter != fullscreen_spot_lights.end(); ++iter)
                {
                    LLDrawable* drawablep = *iter;
                    LLVOVolume* volume = drawablep->getVOVolume();
                    LLVector3   center = drawablep->getPositionAgent();
                    F32         light_size_final = volume->getLightRadius() * 1.5f;
                    F32         light_falloff_final = volume->getLightFalloff(DEFERRED_LIGHT_FALLOFF);

                    sVisibleLightCount++;

                    glm::vec3 tc(center);
                    tc = mul_mat4_vec3(mat, tc);

                    F32 spot_proj_origin[3] = { 0.f, 0.f, 0.f };
                    F32 spot_shadow_fade = 0.f;
                    S32 spot_proj_shadow_idx = -1;
                    SpotProjForVk spot_proj = {};
                    setupSpotLight(gDeferredMultiSpotLightProgram, drawablep,
                                   spot_proj_origin, &spot_shadow_fade, &spot_proj_shadow_idx, &spot_proj);

                    // send light color to shader in linear space
                    LLColor3 col = volume->getLightLinearColor() * light_scale;

                    if (LLVKLoader::isVulkanInitialized()
                        && gDeferredMultiSpotLightProgram.mVkPerProgramUBO != VK_NULL_HANDLE)
                    {
                        gDeferredMultiSpotLightProgram.rotatePerProgramUBOSlot();
                        if (gDeferredMultiSpotLightProgram.vkPerProgramActiveWritePtr() != nullptr)
                        {
                            LLVKLoader::SpotLightPerDraw sd = {};
                            sd.center[0] = tc.x; sd.center[1] = tc.y; sd.center[2] = tc.z;
                            sd.size                  = light_size_final;
                            sd.proj_origin[0]        = spot_proj_origin[0];
                            sd.proj_origin[1]        = spot_proj_origin[1];
                            sd.proj_origin[2]        = spot_proj_origin[2];
                            sd.falloff               = light_falloff_final;
                            sd.shadow_fade           = spot_shadow_fade;
                            sd.global_light_strength = LLPipeline::RenderGlobalLightStrength;
                            sd.proj_shadow_idx       = spot_proj_shadow_idx;
                            sd.classic_mode          = (psky && psky->canAutoAdjust()) ? 1 : 0;
                            memcpy(gDeferredMultiSpotLightProgram.vkPerProgramActiveWritePtr(), &sd, sizeof(sd));
                        }
                        VkBuffer du_ov_buf = VK_NULL_HANDLE; void* du_ov_map = nullptr;
                        if (LLVKLoader::acquireDeferredUtilOverrideSlot(du_ov_buf, du_ov_map) && du_ov_map)
                        {
                            LLVKLoader::DeferredUtil_PerProgramBind* du =
                                (LLVKLoader::DeferredUtil_PerProgramBind*)du_ov_map;
                            memcpy(du->proj_mat, spot_proj.proj_mat, sizeof(du->proj_mat));
                            du->proj_n[0] = spot_proj.proj_n[0]; du->proj_n[1] = spot_proj.proj_n[1]; du->proj_n[2] = spot_proj.proj_n[2];
                            du->proj_focus = spot_proj.proj_focus;
                            du->proj_p[0] = spot_proj.proj_p[0]; du->proj_p[1] = spot_proj.proj_p[1]; du->proj_p[2] = spot_proj.proj_p[2];
                            du->proj_lod = spot_proj.proj_lod;
                            du->color[0] = col.mV[0]; du->color[1] = col.mV[1]; du->color[2] = col.mV[2];
                            du->size = light_size_final;
                            du->proj_range = spot_proj.proj_range;
                            du->proj_ambiance = spot_proj.proj_ambiance;
                            LLVKLoader::setDeferredUtilOverrideSlot(du_ov_buf, du_ov_map);
                        }
                    }

                    mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

                    LLVKLoader::clearDeferredUtilOverrideSlot();
                }

                gDeferredMultiSpotLightProgram.disableTexture(LLShaderMgr::DEFERRED_PROJECTION);
                unbindDeferredShader(gDeferredMultiSpotLightProgram);
                lgt_us[5] += lgt_now() - lgt_fsl_t0;
            }
        }

        gGL.setColorMask(true, true);
    }

    // <AYAstorm r30 P5 transparent-DoF L2-β> Snapshot the opaque-only depth
    // into mAYAAlphaDepth before forward alpha runs. Once alpha geometry
    // renders, rigged BLEND attachments (hair etc.) overwrite
    // deferredScreen.depth with their own z and pin CoF to ~0 for the
    // pixels they cover — even though the texture alpha is ≈ 0.2 and we
    // can see the background through them. mAYAAlphaDepth keeps the
    // opaque z for those pixels so cofF can compute the right bg blur.
    // A later cutoff-0.5 prepass in lldrawpoolalpha (POST_WATER, post
    // forward alpha) overwrites mAYAAlphaDepth only where alpha ≥ 0.5
    // (window grilles, foliage etc.) so subject-like alpha meshes still
    // get treated as subjects. Cheap (one fullscreen depth blit) and
    // only on main RT.
    if (!gCubeSnapshot && RenderDepthOfField)
    {
        LL_PROFILE_GPU_ZONE("aya alpha depth snapshot");
        LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_ALWAYS);

        LLRenderTarget& depth_src = getFrameRT()->deferredScreen;

        LLRTDetour det(mAYAAlphaDepth, false, "alphadepth_snap");
        if (det)
        {
        gCopyDepthProgram.bind();

        S32 diff_map  = gCopyDepthProgram.getTextureChannel(LLShaderMgr::DIFFUSE_MAP);
        S32 depth_map = gCopyDepthProgram.getTextureChannel(LLShaderMgr::DEFERRED_DEPTH);

        gGL.getTexUnit(diff_map)->bind(&getFrameRT()->screen);
        gGL.getTexUnit(depth_map)->bind(&depth_src, true);

        if (LLVKLoader::isVulkanInitialized())
        {
            getFrameRT()->screen.bindForShaderRead();
            depth_src.bindForShaderRead(0, true);
        }

        gGL.setColorMask(false, false);
        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
        gGL.setColorMask(true, true);
        }
        det.resume();
    }
    // </AYAstorm r30 P5 transparent-DoF L2-β>

    {  // render non-deferred geometry (alpha, fullbright, glow)
        const U64 lgt_fwd_t0 = lgt_now();
        LLGLDisable blend(GL_BLEND);

        pushRenderTypeMask();
        andRenderTypeMask(LLPipeline::RENDER_TYPE_ALPHA,
                          LLPipeline::RENDER_TYPE_ALPHA_PRE_WATER,
                          LLPipeline::RENDER_TYPE_ALPHA_POST_WATER,
                          LLPipeline::RENDER_TYPE_FULLBRIGHT,
                          LLPipeline::RENDER_TYPE_VOLUME,
                          LLPipeline::RENDER_TYPE_GLOW,
                          LLPipeline::RENDER_TYPE_BUMP,
                          LLPipeline::RENDER_TYPE_GLTF_PBR,
                          LLPipeline::RENDER_TYPE_PASS_SIMPLE,
                          LLPipeline::RENDER_TYPE_PASS_ALPHA,
                          LLPipeline::RENDER_TYPE_PASS_ALPHA_MASK,
                          LLPipeline::RENDER_TYPE_PASS_BUMP,
                          LLPipeline::RENDER_TYPE_PASS_POST_BUMP,
                          LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT,
                          LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_ALPHA_MASK,
                          LLPipeline::RENDER_TYPE_PASS_FULLBRIGHT_SHINY,
                          LLPipeline::RENDER_TYPE_PASS_GLOW,
                          LLPipeline::RENDER_TYPE_PASS_GLTF_GLOW,
                          LLPipeline::RENDER_TYPE_PASS_GRASS,
                          LLPipeline::RENDER_TYPE_PASS_SHINY,
                          LLPipeline::RENDER_TYPE_PASS_INVISIBLE,
                          LLPipeline::RENDER_TYPE_PASS_INVISI_SHINY,
                          LLPipeline::RENDER_TYPE_AVATAR,
                          LLPipeline::RENDER_TYPE_CONTROL_AV,
                          LLPipeline::RENDER_TYPE_ALPHA_MASK,
                          LLPipeline::RENDER_TYPE_FULLBRIGHT_ALPHA_MASK,
                          LLPipeline::RENDER_TYPE_TERRAIN,
                          LLPipeline::RENDER_TYPE_WATER,
                          LLPipeline::RENDER_TYPE_WATEREXCLUSION,
                          END_RENDER_TYPES);

        {
            renderGeomPostDeferred(*LLViewerCamera::getInstance());
        }
        popRenderTypeMask();
        lgt_us[6] += lgt_now() - lgt_fwd_t0;
    }

    // <AYAstorm r30 P2> velocity buffer pass for motion blur / SMAA T2x
    // (Cinematic mode only — gated by mVelocityMap.isComplete()).
    renderGeomMotionBlur();
    // </AYAstorm r30 P2>

    }
    }

    if (!gCubeSnapshot)
    {
        // this is the end of the 3D scene render, grab a copy of the modelview and projection
        // matrix for use in off-by-one-frame effects in the next frame
        for (U32 i = 0; i < 16; i++)
        {
            gGLLastModelView[i] = gGLModelView[i];
            gGLLastProjection[i] = gGLProjection[i];
        }
    }
    gGL.setColorMask(true, true);

    if (lgt_perf)
    {
        for (U32 i = 0; i < 8; ++i) LLVKLoader::gVkPerf.lgt_us[i] += lgt_us[i];
        LLVKLoader::gVkPerf.lgt_nl += lgt_nl;
        LLVKLoader::gVkPerf.lgt_ns += lgt_ns;
    }
}

bool LLPipeline::doAtmospherics()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    if (isFrameImpostorPass())
    { // do not attempt atmospherics on impostors
        return false;
    }

    if (RenderDeferredAtmospheric)
    {
        bool scene_depth_ok = snapshotSceneDepthToWaterDis();

        if (scene_depth_ok)
        {
        LLGLEnable blend(GL_BLEND);
        gGL.blendFunc(LLRender::BF_ONE, LLRender::BF_SOURCE_ALPHA, LLRender::BF_ZERO, LLRender::BF_SOURCE_ALPHA);
        gGL.setColorMask(true, true);

        // apply haze
        LLGLSLShader& haze_shader = gHazeProgram;

        LL_PROFILE_GPU_ZONE("haze");
        bindDeferredShader(haze_shader, nullptr, &mWaterDis);

        LLEnvironment& environment = LLEnvironment::instance();

        if (LLVKLoader::isVulkanInitialized() && haze_shader.mVkPerProgramUBO != VK_NULL_HANDLE
            && haze_shader.mVkPerProgramUBOMapped != nullptr)
        {
            LLVKLoader::HazeF_PerProgramBind ubo_data    = {};
            ubo_data.sun_dir[0]   = mTransformedSunDir.mV[0];
            ubo_data.sun_dir[1]   = mTransformedSunDir.mV[1];
            ubo_data.sun_dir[2]   = mTransformedSunDir.mV[2];
            ubo_data.moon_dir[0]  = mTransformedMoonDir.mV[0];
            ubo_data.moon_dir[1]  = mTransformedMoonDir.mV[1];
            ubo_data.moon_dir[2]  = mTransformedMoonDir.mV[2];
            ubo_data.sun_up_factor = environment.getIsSunUp() ? 1 : 0;
            haze_shader.rotatePerProgramUBOSlot();
            memcpy(haze_shader.vkPerProgramActiveWritePtr(), &ubo_data, sizeof(ubo_data));
        }

        LLGLDepthTest depth(GL_FALSE);

        // full screen blit
        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

        unbindDeferredShader(haze_shader);

        gGL.setSceneBlendType(LLRender::BT_ALPHA);
        }
        return scene_depth_ok;
    }
    return false;
}

// <FS:AYA r15 P1> godrays: screen-space shadow-driven ray-march pass.
// Mirrors the doAtmospherics() pattern (bindDeferredShader on the HDR
// scene buffer, fullscreen triangle, additive blend) so godrays land on
// getFrameRT()->screen while it is still HDR / pre-tonemap.
void LLPipeline::doGodrays(bool scene_depth_ok)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    if (isFrameImpostorPass() || gCubeSnapshot)
    { // no godrays on impostors / reflection probe snapshots
        return;
    }

    // <FS:AYAstorm r30 BD改善> mode gate は唯一の caller (line ~5184) 側に集約済。
    // BD改善 Phase で Cinematic mode==2 + AYAR15GodraysInCinematicEnabled で opt-in dispatch
    // できるようになったため、本体側の mode==1 early return は dispatch 条件を裏切る。
    // 二重 gate を撤去し、callee は呼ばれたら走るだけにする。
    // </FS:AYAstorm>

    if (!gDeferredGodraysProgram.isComplete())
    {
        return;
    }

    if (!scene_depth_ok)
    {
        scene_depth_ok = snapshotSceneDepthToWaterDis();
    }
    if (!scene_depth_ok)
    {
        return;
    }

    LLGLDepthTest depth(GL_FALSE);
    LLGLEnable    blend(GL_BLEND);
    gGL.blendFunc(LLRender::BF_ONE, LLRender::BF_ONE, LLRender::BF_ONE, LLRender::BF_ONE);
    gGL.setColorMask(true, true);

    LLGLSLShader& shader = gDeferredGodraysProgram;
    bindDeferredShader(shader, nullptr, &mWaterDis);

    LL_PROFILE_GPU_ZONE("godrays");

    if (LLVKLoader::isVulkanInitialized() && shader.mVkPerProgramUBO != VK_NULL_HANDLE
        && shader.mVkPerProgramUBOMapped != nullptr)
    {
        static LLCachedControl<U32>  aya_visual_realism(gSavedSettings, "AYAVisualRealismEnabled", 1);
        static LLCachedControl<bool> aya_r15_in_cinematic(gSavedSettings, "AYAR15GodraysInCinematicEnabled", false);
        static LLCachedControl<F32>  aya_r15_phase_exp(gSavedSettings, "AYAR15GodraysPhaseExponent", 16.0f);
        static LLCachedControl<F32>  aya_r15_strength(gSavedSettings, "AYAR15GodraysStrength", 0.15f);

        const bool r15_on = (aya_visual_realism() == 1)
                         || (aya_visual_realism() == 2 && aya_r15_in_cinematic);

        LLVKLoader::GodraysF_PerProgramBind ubo_data        = {};
        ubo_data.sun_dir[0]          = mTransformedSunDir.mV[0];
        ubo_data.sun_dir[1]          = mTransformedSunDir.mV[1];
        ubo_data.sun_dir[2]          = mTransformedSunDir.mV[2];
        ubo_data.moon_dir[0]         = mTransformedMoonDir.mV[0];
        ubo_data.moon_dir[1]         = mTransformedMoonDir.mV[1];
        ubo_data.moon_dir[2]         = mTransformedMoonDir.mV[2];
        ubo_data.aya_r15_godrays_enabled        = r15_on ? 1 : 0;
        ubo_data.aya_r15_godrays_phase_exponent = (F32)aya_r15_phase_exp;
        ubo_data.aya_r15_godrays_strength       = (F32)aya_r15_strength;
        memcpy(shader.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
    }

    mScreenTriangleVB->setBuffer();
    mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

    unbindDeferredShader(shader);

    gGL.setSceneBlendType(LLRender::BT_ALPHA);
}

// </FS:AYA>

// <FS:AYA r20 P0a> skin SSS prototype: screen-space 5-tap separable blur
// with wavelength-dependent per-channel weights. Two-pass schedule:
//   Pass 1: screen → mWaterDis  (horizontal blur, no blend, replace)
//   Pass 2: mWaterDis → screen (vertical blur, SRC_ALPHA / 1-SRC_ALPHA
//           blend so result mixes with original screen content at `strength`)
// P0a scope: NO skin whitelist — global look-evaluation prototype.
void LLPipeline::doSkinSSS()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    if (isFrameImpostorPass() || gCubeSnapshot)
    {
        return;
    }

    // <FS:AYAstorm r30 BD改善> r20 SSS: consolidation 後 mode 1 (AYAstorm View) /
    //   mode 2 (Cinematic) 共通の単一 cvar (AYAR20AvatarSkinSSSEnabled) で dispatch。
    //   旧 AYAR20AvatarSkinSSSInCinematicEnabled は AYAR20SSSMigrationVersion==0 起動時に
    //   新 cvar へ OR 合成される (llappviewer.cpp の migration コード参照)。default OFF。
    static LLCachedControl<U32>  realism_enabled(gSavedSettings, "AYAVisualRealismEnabled", 1);
    static LLCachedControl<bool> r20_enabled(gSavedSettings, "AYAR20AvatarSkinSSSEnabled", false);
    bool r20_active = (realism_enabled() > 0) && r20_enabled();
    if (!r20_active)
    {
        return;
    }
    // </FS:AYAstorm>

    if (!gDeferredSkinSSSProgram.isComplete())
    {
        return;
    }

    LL_PROFILE_GPU_ZONE("skin SSS");

    LLGLSLShader& shader = gDeferredSkinSSSProgram;

    // P0a prototype tuning. Look 判定中は live iteration の価値が大きいので
    // debug settings に逃がしている。値が確定したら hard-code に戻す予定。
    // 既定値: blur_radius=6.0 / strength=0.7 (iteration 3 で AYA OK 判定)。
    static LLCachedControl<F32> sss_blur_radius(gSavedSettings, "AYAR20AvatarSkinSSSBlurRadius", 1.0f);
    static LLCachedControl<F32> sss_strength(gSavedSettings, "AYAR20AvatarSkinSSSStrength", 0.5f);
    // <FS:AYA r20 Phase D> glow restore gain + tint (highlight boost on top of blur).
    static LLCachedControl<F32>      sss_glow_gain(gSavedSettings,  "AYAR20AvatarSkinSSSGlowGain",  0.2f);
    static LLCachedControl<LLColor4> sss_glow_color(gSavedSettings, "AYAR20AvatarSkinSSSGlowColor");
    // <FS:AYA r20 Phase D world-scale blur> blur 半径を世界座標で固定する
    // (= 距離で逆スケール) ため、shader が depth を読む。near/far の距離
    // fade cvar は不要 (遠距離では r_eff < 1px で自動 no-op)。
    const F32 strength    = llclamp((F32)sss_strength(),    0.0f, 1.0f);
    const F32 blur_radius = llmax((F32)sss_blur_radius(), 0.0f);
    const F32 glow_gain   = llmax((F32)sss_glow_gain(), 0.0f);
    const LLColor4 glow_color = sss_glow_color();
    // </FS:AYA>

    LLGLDepthTest depth(GL_FALSE, GL_FALSE);
    gGL.setColorMask(true, true);

    // <FS:AYA r20 Phase C> gbuffer3 holds the per-pixel skin bit in .a.
    // Pass 1 ignores alpha, but binding it both passes keeps state simple.
    LLRenderTarget* deferred_target = &getFrameRT()->deferredScreen;
    // </FS:AYA>

    // Pass 1: horizontal blur, screen → mWaterDis (replace; blend off)
    LLRTDetour det(mWaterDis, false, "sss_pass1");
    if (det)
    {
        LLGLDisable blend_off(GL_BLEND);

        shader.bind();
        shader.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, &getFrameRT()->screen, false, LLTexUnit::TFO_BILINEAR);
        // <FS:AYA r20 Phase C> bind gbuffer3 as the skin mask source.
        {
            S32 channel = shader.enableTexture(LLShaderMgr::DEFERRED_EMISSIVE, deferred_target->getUsage());
            if (channel > -1)
            {
                deferred_target->bindTexture(3, channel, LLTexUnit::TFO_POINT);
                gGL.getTexUnit(channel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
            }
        }
        {
            S32 nchannel = shader.enableTexture(LLShaderMgr::NORMAL_MAP, deferred_target->getUsage());
            if (nchannel > -1)
            {
                deferred_target->bindTexture(2, nchannel, LLTexUnit::TFO_POINT);
                gGL.getTexUnit(nchannel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
            }
        }
        // </FS:AYA>
        // <FS:AYA r20 Phase D world-scale blur> bind deferred depth attachment
        // so the shader can per-pixel scale blur radius by eye distance.
        {
            S32 dch = shader.enableTexture(LLShaderMgr::DEFERRED_DEPTH, deferred_target->getUsage());
            if (dch > -1)
            {
                gGL.getTexUnit(dch)->bind(deferred_target, true);
            }
        }
        // </FS:AYA>

        if (LLVKLoader::isVulkanInitialized()
            && gDeferredSkinSSSProgram.mVkPerProgramUBO != VK_NULL_HANDLE
            && gDeferredSkinSSSProgram.mVkPerProgramUBOMapped != nullptr)
        {
            LLVKLoader::SkinSSSF_PerProgramBind ubo_data = {};
            ubo_data.aya_glow_color[0]            = glow_color.mV[0];
            ubo_data.aya_glow_color[1]            = glow_color.mV[1];
            ubo_data.aya_glow_color[2]            = glow_color.mV[2];
            ubo_data.aya_glow_gain                = glow_gain;
            ubo_data.aya_blur_dir[0]              = 1.0f;
            ubo_data.aya_blur_dir[1]              = 0.0f;
            ubo_data.aya_strength                 = 1.0f;
            ubo_data.aya_blur_radius              = blur_radius;
            ubo_data.aya_visual_realism_enabled   = 1;
            ubo_data.aya_r20_skin_sss_enabled     = 1;
            gDeferredSkinSSSProgram.rotatePerProgramUBOSlot();
            memcpy(gDeferredSkinSSSProgram.vkPerProgramActiveWritePtr(), &ubo_data, sizeof(ubo_data));
        }

        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

        // <FS:AYA r20 Phase C> release the gbuffer3 channel so other passes
        // can rebind index 3 cleanly.
        shader.disableTexture(LLShaderMgr::DEFERRED_EMISSIVE, deferred_target->getUsage());
        // </FS:AYA>
        // <FS:AYA r20 Phase D world-scale blur> release depth channel.
        shader.disableTexture(LLShaderMgr::DEFERRED_DEPTH, deferred_target->getUsage());
        // </FS:AYA>
        shader.unbind();
    }
    det.endTemp();

    // Pass 2: vertical blur, mWaterDis → screen (RGB-only mix with strength,
    // alpha kept untouched so the scene-buffer sky mask is preserved —
    // memory project_aya_visual_realism_alpha_protect.md)
    if (det)
    {
        LLRTScope s2(getFrameRT()->screen, true, "sss_pass2");
        if (s2)
        {
        LLGLEnable blend_on(GL_BLEND);
        gGL.blendFunc(LLRender::BF_SOURCE_ALPHA, LLRender::BF_ONE_MINUS_SOURCE_ALPHA,
                      LLRender::BF_ZERO, LLRender::BF_ONE);

        shader.bind();
        shader.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, &mWaterDis, false, LLTexUnit::TFO_BILINEAR);
        // <FS:AYA r20 Phase C> skin mask: gbuffer3 (.a = per-pixel skin bit).
        {
            S32 channel = shader.enableTexture(LLShaderMgr::DEFERRED_EMISSIVE, deferred_target->getUsage());
            if (channel > -1)
            {
                deferred_target->bindTexture(3, channel, LLTexUnit::TFO_POINT);
                gGL.getTexUnit(channel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
            }
        }
        {
            S32 nchannel = shader.enableTexture(LLShaderMgr::NORMAL_MAP, deferred_target->getUsage());
            if (nchannel > -1)
            {
                deferred_target->bindTexture(2, nchannel, LLTexUnit::TFO_POINT);
                gGL.getTexUnit(nchannel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
            }
        }
        // </FS:AYA>
        // <FS:AYA r20 Phase D world-scale blur> bind deferred depth attachment
        // for per-pixel blur-radius scaling in skinSSSF.
        {
            S32 dch = shader.enableTexture(LLShaderMgr::DEFERRED_DEPTH, deferred_target->getUsage());
            if (dch > -1)
            {
                gGL.getTexUnit(dch)->bind(deferred_target, true);
            }
        }
        // </FS:AYA>

        if (LLVKLoader::isVulkanInitialized()
            && gDeferredSkinSSSProgram.mVkPerProgramUBO != VK_NULL_HANDLE
            && gDeferredSkinSSSProgram.mVkPerProgramUBOMapped != nullptr)
        {
            LLVKLoader::SkinSSSF_PerProgramBind ubo_data = {};
            ubo_data.aya_glow_color[0]            = glow_color.mV[0];
            ubo_data.aya_glow_color[1]            = glow_color.mV[1];
            ubo_data.aya_glow_color[2]            = glow_color.mV[2];
            ubo_data.aya_glow_gain                = glow_gain;
            ubo_data.aya_blur_dir[0]              = 0.0f;
            ubo_data.aya_blur_dir[1]              = 1.0f;
            ubo_data.aya_strength                 = strength;
            ubo_data.aya_blur_radius              = blur_radius;
            ubo_data.aya_visual_realism_enabled   = 1;
            ubo_data.aya_r20_skin_sss_enabled     = 1;
            gDeferredSkinSSSProgram.rotatePerProgramUBOSlot();
            memcpy(gDeferredSkinSSSProgram.vkPerProgramActiveWritePtr(), &ubo_data, sizeof(ubo_data));
        }

        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

        // <FS:AYA r20 Phase C>
        shader.disableTexture(LLShaderMgr::DEFERRED_EMISSIVE, deferred_target->getUsage());
        // </FS:AYA>
        shader.disableTexture(LLShaderMgr::NORMAL_MAP, deferred_target->getUsage());
        // <FS:AYA r20 Phase D world-scale blur>
        shader.disableTexture(LLShaderMgr::DEFERRED_DEPTH, deferred_target->getUsage());
        // </FS:AYA>
        shader.unbind();
        }
    }

    det.resume();
    gGL.setSceneBlendType(LLRender::BT_ALPHA);
    gGL.setColorMask(true, false);
}

// </FS:AYA>

void LLPipeline::doWaterHaze()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    if (isFrameImpostorPass())
    { // do not attempt water haze on impostors
        return;
    }

    if (RenderDeferredAtmospheric)
    {
        // copy depth buffer for use in haze shader (use water displacement map as temp storage)
        LLRTDetour det(gPipeline.mWaterDis, false, "waterhaze_depthcopy");
        if (det)
        {
            LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_ALWAYS);

            LLRenderTarget& src = getFrameRT()->screen;
            LLRenderTarget& depth_src = getFrameRT()->deferredScreen;

            gCopyDepthProgram.bind();

            S32 diff_map = gCopyDepthProgram.getTextureChannel(LLShaderMgr::DIFFUSE_MAP);
            S32 depth_map = gCopyDepthProgram.getTextureChannel(LLShaderMgr::DEFERRED_DEPTH);

            gGL.getTexUnit(diff_map)->bind(&src);
            gGL.getTexUnit(depth_map)->bind(&depth_src, true);

            if (LLVKLoader::isVulkanInitialized())
            {
                src.bindForShaderRead();
                depth_src.bindForShaderRead(0, true);
            }

            gGL.setColorMask(false, false);
            gPipeline.mScreenTriangleVB->setBuffer();
            gPipeline.mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

        }
        det.resume();

        if (det)
        {
        LLGLEnable blend(GL_BLEND);
        gGL.blendFunc(LLRender::BF_ONE, LLRender::BF_SOURCE_ALPHA, LLRender::BF_ZERO, LLRender::BF_SOURCE_ALPHA);

        gGL.setColorMask(true, true);

        // apply haze
        LLGLSLShader& haze_shader = gHazeWaterProgram;

        LL_PROFILE_GPU_ZONE("haze");
        bindDeferredShader(haze_shader, nullptr, &mWaterDis);

        const S32 above_water_val = isFrameUnderWaterRendering() ? -1 : 1;

        haze_shader.bindTexture(LLShaderMgr::WATER_EXCLUSIONTEX, &mWaterExclusionMask);

        if (LLVKLoader::isVulkanInitialized()
            && haze_shader.mVkPerProgramUBO != VK_NULL_HANDLE
            && haze_shader.mVkPerProgramUBOMapped != nullptr)
        {
            LLVKLoader::WaterHazeF_PerProgramBind ubo_data = {};
            ubo_data.above_water = above_water_val;
            memcpy(haze_shader.vkPerProgramBaseWritePtr(), &ubo_data,
                   llmin((U32)sizeof(ubo_data), haze_shader.mVkPerProgramUBOSize));
            mWaterExclusionMask.bindForShaderRead(0, false);
        }

        if (isFrameUnderWaterRendering())
        {
            LLGLDepthTest depth(GL_FALSE);

            // full screen blit
            mScreenTriangleVB->setBuffer();
            mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
        }
        else
        {
            //render water patches like LLDrawPoolWater does
            LLGLDepthTest depth(GL_TRUE, GL_FALSE);
            LLGLDisable   cull(GL_CULL_FACE);

            gGLLastMatrix = NULL;
            gGL.loadMatrix(gGLModelView);

            if (mWaterPool)
            {
                mWaterPool->pushFaceGeometry();
            }
        }

        unbindDeferredShader(haze_shader);


        gGL.setSceneBlendType(LLRender::BT_ALPHA);
        }
    }
}

void LLPipeline::setupSpotLight(LLGLSLShader& shader, LLDrawable* drawablep,
                                F32* out_proj_origin,
                                F32* out_shadow_fade,
                                S32* out_proj_shadow_idx,
                                SpotProjForVk* out_proj)
{
    //construct frustum
    LLVOVolume* volume = drawablep->getVOVolume();
    LLVector3 params = volume->getSpotLightParams();

    F32 fov = params.mV[0];
    F32 focus = params.mV[1];

    LLVector3 pos = drawablep->getPositionAgent();
    LLQuaternion quat = volume->getRenderRotation();
    LLVector3 scale = volume->getScale();

    //get near clip plane
    LLVector3 at_axis(0,0,-scale.mV[2]*0.5f);
    at_axis *= quat;

    LLVector3 np = pos+at_axis;
    at_axis.normVec();

    //get origin that has given fov for plane np, at_axis, and given scale
    F32 dist = (scale.mV[1]*0.5f)/tanf(fov*0.5f);

    LLVector3 origin = np - at_axis*dist;

    //matrix from volume space to agent space
    LLMatrix4 light_mat(quat, LLVector4(origin,1.f));

    glm::mat4 light_to_agent(glm::make_mat4((F32*) light_mat.mMatrix));
    glm::mat4 light_to_screen = get_current_modelview() * light_to_agent;

    glm::mat4 screen_to_light = glm::inverse(light_to_screen);

    F32 s = volume->getLightRadius()*1.5f;
    F32 near_clip = dist;
    F32 width = scale.mV[VX];
    F32 height = scale.mV[VY];
    F32 far_clip = s+dist-scale.mV[VZ];

    F32 fovy = fov; // radians
    F32 aspect = width/height;

    glm::vec3 p1(0, 0, -(near_clip+0.01f));
    glm::vec3 p2(0, 0, -(near_clip+1.f));

    glm::vec3 screen_origin(0, 0, 0);

    p1 = mul_mat4_vec3(light_to_screen, p1);
    p2 = mul_mat4_vec3(light_to_screen, p2);
    screen_origin = mul_mat4_vec3(light_to_screen, screen_origin);

    glm::vec3 n = p2-p1;
    n = glm::normalize(n);

    F32 proj_range = far_clip - near_clip;
    glm::mat4 light_proj = glm::perspective(fovy, aspect, near_clip, far_clip);
    screen_to_light = sGlNdcToSampleBias * light_proj * screen_to_light;
    if (out_proj_origin)
    {
        out_proj_origin[0] = screen_origin.x;
        out_proj_origin[1] = screen_origin.y;
        out_proj_origin[2] = screen_origin.z;
    }
    if (out_proj)
    {
        memcpy(out_proj->proj_mat, glm::value_ptr(screen_to_light), sizeof(out_proj->proj_mat));
        out_proj->proj_p[0] = p1.x; out_proj->proj_p[1] = p1.y; out_proj->proj_p[2] = p1.z;
        out_proj->proj_n[0] = n.x;  out_proj->proj_n[1] = n.y;  out_proj->proj_n[2] = n.z;
        out_proj->proj_range    = proj_range;
        out_proj->proj_ambiance = params.mV[2];
        out_proj->proj_focus    = 0.f;
        out_proj->proj_lod      = 0.f;
    }
    S32 s_idx = -1;

    for (U32 i = 0; i < 2; i++)
    {
        if (mShadowSpotLight[i] == drawablep)
        {
            s_idx = i;
        }
    }

    if (out_proj_shadow_idx)
    {
        *out_proj_shadow_idx = s_idx;
    }

    if (out_shadow_fade)
    {
        *out_shadow_fade = 0.f;
    }
    if (s_idx >= 0)
    {
        if (out_shadow_fade)
        {
            *out_shadow_fade = 1.f - mSpotLightFade[s_idx];
        }
    }

    // make sure we're not already targeting the same spot light with both shadow maps
    llassert(mTargetShadowSpotLight[0] != mTargetShadowSpotLight[1] || mTargetShadowSpotLight[0].isNull());

    if (!gCubeSnapshot)
    {
        LLDrawable* potential = drawablep;
        //determine if this light is higher priority than one of the existing spot shadows
        F32 m_pri = volume->getSpotLightPriority();

        for (U32 i = 0; i < 2; i++)
        {
            F32 pri = 0.f;

            if (mTargetShadowSpotLight[i].notNull())
            {
                pri = mTargetShadowSpotLight[i]->getVOVolume()->getSpotLightPriority();
            }

            if (m_pri > pri)
            {
                LLDrawable* temp = mTargetShadowSpotLight[i];
                mTargetShadowSpotLight[i] = potential;
                potential = temp;
                m_pri = pri;
            }
        }
    }

    // make sure we didn't end up targeting the same spot light with both shadow maps
    llassert(mTargetShadowSpotLight[0] != mTargetShadowSpotLight[1] || mTargetShadowSpotLight[0].isNull());

    LLViewerTexture* img = volume->getLightTexture();

    if (img == NULL)
    {
        img = LLViewerFetchedTexture::sWhiteImagep;
    }

    S32 channel = shader.enableTexture(LLShaderMgr::DEFERRED_PROJECTION);

    if (channel > -1)
    {
        if (img)
        {
            gGL.getTexUnit(channel)->bind(img);

            F32 lod_range = logf((F32)img->getWidth())/logf(2.f);

            if (out_proj)
            {
                out_proj->proj_focus = focus;
                out_proj->proj_lod   = lod_range;
            }
        }
    }

}

void LLPipeline::unbindDeferredShader(LLGLSLShader &shader)
{
    LLRenderTarget* deferred_target       = &getFrameRT()->deferredScreen;
    LLRenderTarget* deferred_light_target = &getFrameRT()->deferredLight;

    shader.disableTexture(LLShaderMgr::NORMAL_MAP, deferred_target->getUsage());
    shader.disableTexture(LLShaderMgr::DEFERRED_DIFFUSE, deferred_target->getUsage());
    shader.disableTexture(LLShaderMgr::DEFERRED_SPECULAR, deferred_target->getUsage());
    shader.disableTexture(LLShaderMgr::DEFERRED_EMISSIVE, deferred_target->getUsage());
    shader.disableTexture(LLShaderMgr::DEFERRED_BRDF_LUT);
    //shader.disableTexture(LLShaderMgr::DEFERRED_DEPTH, deferred_depth_target->getUsage());
    shader.disableTexture(LLShaderMgr::DEFERRED_DEPTH, deferred_target->getUsage());
    shader.disableTexture(LLShaderMgr::DEFERRED_LIGHT, deferred_light_target->getUsage());
    shader.disableTexture(LLShaderMgr::DIFFUSE_MAP);
    shader.disableTexture(LLShaderMgr::DEFERRED_BLOOM);

    for (U32 i = 0; i < 4; i++)
    {
        shader.disableTexture(LLShaderMgr::DEFERRED_SHADOW0+i);
    }

    for (U32 i = 4; i < 6; i++)
    {
        shader.disableTexture(LLShaderMgr::DEFERRED_SHADOW0+i);
    }

    shader.disableTexture(LLShaderMgr::DEFERRED_NOISE);
    shader.disableTexture(LLShaderMgr::DEFERRED_LIGHTFUNC);

    if (!isFrameReflectionProbesEnabled())
    {
        S32 channel = shader.disableTexture(LLShaderMgr::ENVIRONMENT_MAP, LLTexUnit::TT_CUBE_MAP);
        if (channel > -1)
        {
            LLCubeMap* cube_map = gSky.mVOSkyp ? gSky.mVOSkyp->getCubeMap() : NULL;
            if (cube_map)
            {
                cube_map->disable();
            }
        }
    }

    unbindReflectionProbes(shader);

    gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
    gGL.getTexUnit(0)->activate();
    shader.unbind();
}

void LLPipeline::setEnvMat(LLGLSLShader& shader)
{
    F32* m = gGLModelView;

    F32 mat[] = { m[0], m[1], m[2],
                    m[4], m[5], m[6],
                    m[8], m[9], m[10] };

    if (LLVKLoader::isVulkanInitialized())
    {
        LLSettingsSky::ptr_t psky_ref = LLEnvironment::instance().getCurrentSky();
        LLVKLoader::ReflectionProbe_PerProgramBind data = {};
        data.reflection_probe_ambiance = ayaDeriveReflectionProbeAmbianceVk(psky_ref, true);
        data.env_mat_col0[0] = mat[0]; data.env_mat_col0[1] = mat[3]; data.env_mat_col0[2] = mat[6];
        data.env_mat_col1[0] = mat[1]; data.env_mat_col1[1] = mat[4]; data.env_mat_col1[2] = mat[7];
        data.env_mat_col2[0] = mat[2]; data.env_mat_col2[1] = mat[5]; data.env_mat_col2[2] = mat[8];
        LLVKLoader::writeCurrentReflectionProbeUBO(data);

        LLVKLoader::ReflectionProbeF_PerProgramBind reflF = {};
        reflF.env_mat_col0[0] = mat[0]; reflF.env_mat_col0[1] = mat[3]; reflF.env_mat_col0[2] = mat[6];
        reflF.env_mat_col1[0] = mat[1]; reflF.env_mat_col1[1] = mat[4]; reflF.env_mat_col1[2] = mat[7];
        reflF.env_mat_col2[0] = mat[2]; reflF.env_mat_col2[1] = mat[5]; reflF.env_mat_col2[2] = mat[8];
        reflF.cube_snapshot   = gCubeSnapshot ? 1 : 0;
        reflF.max_probe_lod   = mReflectionMapManager.mMaxProbeLOD;
        reflF.clipPlane[0]    = LLPipeline::sLastClipPlane.mV[0];
        reflF.clipPlane[1]    = LLPipeline::sLastClipPlane.mV[1];
        reflF.clipPlane[2]    = LLPipeline::sLastClipPlane.mV[2];
        reflF.clipPlane[3]    = LLPipeline::sLastClipPlane.mV[3];
        LLVKLoader::writeCurrentReflectionProbeFUBO(reflF);
    }
}

void LLPipeline::bindReflectionProbes(LLGLSLShader& shader)
{
    if (!isFrameReflectionProbesEnabled())
    {
        return;
    }

    S32 channel = shader.enableTexture(LLShaderMgr::REFLECTION_PROBES, LLTexUnit::TT_CUBE_MAP_ARRAY);
    bool bound = false;
    if (channel > -1 && mReflectionMapManager.mTexture.notNull())
    {
        mReflectionMapManager.mTexture->bind(channel);
        bound = true;
    }

    channel = shader.enableTexture(LLShaderMgr::IRRADIANCE_PROBES, LLTexUnit::TT_CUBE_MAP_ARRAY);
    if (channel > -1 && mReflectionMapManager.mIrradianceMaps.notNull())
    {
        mReflectionMapManager.mIrradianceMaps->bind(channel);
        bound = true;
    }

    if (RenderMirrors)
    {
        channel = shader.enableTexture(LLShaderMgr::HERO_PROBE, LLTexUnit::TT_CUBE_MAP_ARRAY);
        if (channel > -1 && mHeroProbeManager.mTexture.notNull())
        {
            mHeroProbeManager.mTexture->bind(channel);
            bound = true;
        }
    }


    if (bound)
    {
        mReflectionMapManager.setUniforms();

        setEnvMat(shader);
    }

    // reflection probe shaders generally sample the scene map as well for SSR
    channel = shader.enableTexture(LLShaderMgr::SCENE_MAP);
    if (channel > -1)
    {
        gGL.getTexUnit(channel)->bind(&mSceneMap);
    }


    static LLCachedControl<F32> ssr_max_depth(gSavedSettings, "RenderScreenSpaceReflectionMaxDepth", 256.f);
    static LLCachedControl<F32> ssr_max_roughness(gSavedSettings, "RenderScreenSpaceReflectionMaxRoughness", 1.f);

    if (LLVKLoader::isVulkanInitialized())
    {
        LLVKLoader::SSRUtil_PerProgramBind ubo_data{};
        ubo_data.screen_res[0] = (F32)getFrameRT()->screen.getWidth();
        ubo_data.screen_res[1] = (F32)getFrameRT()->screen.getHeight();
        ubo_data.iterationCount = (F32)RenderScreenSpaceReflectionIterations;
        ubo_data.rayStep = RenderScreenSpaceReflectionRayStep;
        std::memcpy(ubo_data.modelview_delta,     glm::value_ptr(gGLDeltaModelView),        sizeof(ubo_data.modelview_delta));
        std::memcpy(ubo_data.inv_modelview_delta, glm::value_ptr(gGLInverseDeltaModelView), sizeof(ubo_data.inv_modelview_delta));
        ubo_data.distanceBias = RenderScreenSpaceReflectionDistanceBias;
        ubo_data.depthRejectBias = RenderScreenSpaceReflectionDepthRejectBias;
        ubo_data.adaptiveStepMultiplier = RenderScreenSpaceReflectionAdaptiveStepMultiplier;
        ubo_data.glossySampleCount = (F32)RenderScreenSpaceReflectionGlossySamples;
        ubo_data.noiseSine = 0.f;
        ubo_data.maxZDepth = llmax(1.f, (F32)ssr_max_depth);
        ubo_data.maxRoughness = llclamp((F32)ssr_max_roughness, 0.001f, 1.f);
        LLVKLoader::writeCurrentSSRUtilUBO(ubo_data);
    }

    channel = shader.enableTexture(LLShaderMgr::SCENE_DEPTH);
    if (channel > -1)
    {
        gGL.getTexUnit(channel)->bind(&mSceneMap, true);
    }


}

void LLPipeline::unbindReflectionProbes(LLGLSLShader& shader)
{
    S32 channel = shader.disableTexture(LLShaderMgr::REFLECTION_PROBES, LLTexUnit::TT_CUBE_MAP);
    if (channel > -1 && mReflectionMapManager.mTexture.notNull())
    {
        mReflectionMapManager.mTexture->unbind();
        if (channel == 0)
        {
            gGL.getTexUnit(channel)->enable(LLTexUnit::TT_TEXTURE);
        }
    }
}

