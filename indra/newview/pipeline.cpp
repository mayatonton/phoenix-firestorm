/**
 * @file pipeline.cpp
 * @brief Rendering pipeline.
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

#include "pipeline.h"
#include "llvkbucket.h"

#include <unordered_map>

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
// #include "llpanelface.h"  // <FS:Zi> switchable edit texture/materials panel - include not needed
#include "llpathfindingpathtool.h"
#include "llscenemonitor.h"
#include "llprogressview.h"
#include "llcleanup.h"
#include "gltfscenemanager.h"
// [RLVa:KB] - Checked: RLVa-2.0.0
#include "llvisualeffect.h"
#include "rlvactions.h"
#include "rlvlocks.h"
// [/RLVa:KB]

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

extern bool gSnapshot;
bool gShiftFrame = false;

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

bool LLPipeline::WindLightUseAtmosShaders;
bool LLPipeline::RenderDeferred;
F32 LLPipeline::RenderDeferredSunWash;
U32 LLPipeline::RenderFSAAType;
U32 LLPipeline::RenderResolutionDivisor;
// [SL:KB] - Patch: Settings-RenderResolutionMultiplier | Checked: Catznip-5.4
F32 LLPipeline::RenderResolutionMultiplier;
// [/SL:KB]
bool LLPipeline::RenderUIBuffer;
S32 LLPipeline::RenderShadowDetail;
S32 LLPipeline::RenderShadowSplits;
bool LLPipeline::RenderDeferredSSAO;
F32 LLPipeline::RenderShadowResolutionScale;
// <FS:AYAstorm:r30-bd-port> Phase 3.9: BD sidebar gating flag for manual shadow distance entry.
bool LLPipeline::RenderShadowAutomaticDistance;
// </FS:AYAstorm:r30-bd-port>
// <FS:AYAstorm:r30-bd-port> Phase 6 step 1: BD per-channel shadow allocation (Cinematic only)
LLVector4 LLPipeline::RenderShadowResolution;
LLVector4 LLPipeline::RenderShadowFarClipVec;
LLVector2 LLPipeline::RenderProjectorShadowResolution;
// </FS:AYAstorm:r30-bd-port>
// <FS:AYAstorm:r30-bd-port> Phase 6 step 2: BD live scalar cvar (Cinematic only)
F32 LLPipeline::RenderShadowFarClip;
F32 LLPipeline::RenderGlobalLightStrength;
// </FS:AYAstorm:r30-bd-port>
// <FS:AYAstorm:r30-bd-port> Phase 6 step 3: BD live Post FX scalar cvar (Cinematic only)
F32 LLPipeline::RenderSepiaStrength;
F32 LLPipeline::RenderGreyscaleStrength;
U32 LLPipeline::RenderNumColors;
// </FS:AYAstorm:r30-bd-port>
bool LLPipeline::RenderDelayCreation;
//bool LLPipeline::RenderAnimateRes; <FS:Beq> FIRE-23122 BUG-225920 Remove broken RenderAnimateRes functionality.
bool LLPipeline::FreezeTime;
S32 LLPipeline::DebugBeaconLineWidth;
F32 LLPipeline::RenderHighlightBrightness;
LLColor4 LLPipeline::RenderHighlightColor;
F32 LLPipeline::RenderHighlightThickness;
bool LLPipeline::RenderSpotLightsInNondeferred;
LLColor4 LLPipeline::PreviewAmbientColor;
LLColor4 LLPipeline::PreviewDiffuse0;
LLColor4 LLPipeline::PreviewSpecular0;
LLColor4 LLPipeline::PreviewDiffuse1;
LLColor4 LLPipeline::PreviewSpecular1;
LLColor4 LLPipeline::PreviewDiffuse2;
LLColor4 LLPipeline::PreviewSpecular2;
LLVector3 LLPipeline::PreviewDirection0;
LLVector3 LLPipeline::PreviewDirection1;
LLVector3 LLPipeline::PreviewDirection2;
F32 LLPipeline::RenderGlowMinLuminance;
// <FS:AYAstorm r30 P4>
bool LLPipeline::RenderDeferredBlurLight;
bool LLPipeline::RenderMotionBlur;
// </FS:AYAstorm r30 P4>
F32 LLPipeline::RenderGlowMaxExtractAlpha;
F32 LLPipeline::RenderGlowWarmthAmount;
LLVector3 LLPipeline::RenderGlowLumWeights;
LLVector3 LLPipeline::RenderGlowWarmthWeights;
S32 LLPipeline::RenderGlowResolutionPow;
S32 LLPipeline::RenderGlowIterations;
F32 LLPipeline::RenderGlowWidth;
F32 LLPipeline::RenderGlowStrength;
bool LLPipeline::RenderGlowNoise;
bool LLPipeline::RenderDepthOfField;
bool LLPipeline::RenderDepthOfFieldInEditMode;
// <FS:Beq> FIRE-16728 Add free aim mouse and focus lock
bool LLPipeline::FSFocusPointLocked;
bool LLPipeline::FSFocusPointFollowsPointer;
// </FS:Beq>
F32 LLPipeline::CameraFocusTransitionTime;
F32 LLPipeline::CameraFNumber;
F32 LLPipeline::CameraFocalLength;
F32 LLPipeline::CameraFieldOfView;
F32 LLPipeline::RenderShadowNoise;
F32 LLPipeline::RenderShadowSoftness;
F32 LLPipeline::RenderShadowBlurSize;
F32 LLPipeline::RenderSSAOScale;
U32 LLPipeline::RenderSSAOMaxScale;
F32 LLPipeline::RenderSSAOFactor;
LLVector3 LLPipeline::RenderSSAOEffect;
F32 LLPipeline::RenderShadowOffsetError;
F32 LLPipeline::RenderShadowBiasError;
F32 LLPipeline::RenderShadowOffset;
F32 LLPipeline::RenderShadowBias;
F32 LLPipeline::RenderSpotShadowOffset;
F32 LLPipeline::RenderSpotShadowBias;
LLDrawable* LLPipeline::RenderSpotLight = nullptr;
F32 LLPipeline::RenderEdgeDepthCutoff;
F32 LLPipeline::RenderEdgeNormCutoff;
LLVector3 LLPipeline::RenderShadowGaussian;
F32 LLPipeline::RenderShadowBlurDistFactor;
bool LLPipeline::RenderDeferredAtmospheric;
F32 LLPipeline::RenderHighlightFadeTime;
F32 LLPipeline::RenderFarClip;
LLVector3 LLPipeline::RenderShadowSplitExponent;
F32 LLPipeline::RenderShadowErrorCutoff;
F32 LLPipeline::RenderShadowFOVCutoff;
bool LLPipeline::CameraOffset;
F32 LLPipeline::CameraMaxCoF;
F32 LLPipeline::CameraDoFResScale;
LLVector3 LLPipeline::RenderVignette;
F32 LLPipeline::RenderAutoHideSurfaceAreaLimit;
bool LLPipeline::RenderScreenSpaceReflections;
S32 LLPipeline::RenderScreenSpaceReflectionIterations;
F32 LLPipeline::RenderScreenSpaceReflectionRayStep;
F32 LLPipeline::RenderScreenSpaceReflectionDistanceBias;
F32 LLPipeline::RenderScreenSpaceReflectionDepthRejectBias;
F32 LLPipeline::RenderScreenSpaceReflectionAdaptiveStepMultiplier;
S32 LLPipeline::RenderScreenSpaceReflectionGlossySamples;
S32 LLPipeline::RenderBufferVisualization;
bool LLPipeline::RenderMirrors;
S32 LLPipeline::RenderHeroProbeUpdateRate;
S32 LLPipeline::RenderHeroProbeConservativeUpdateMultiplier;
bool LLPipeline::RenderAvatarCloth;
LLTrace::EventStatHandle<S64> LLPipeline::sStatBatchSize("renderbatchsize");

constexpr U32 LLPipeline::MAX_PREVIEW_WIDTH = 2048;
constexpr U32 LLPipeline::MAX_PREVIEW_HEIGHT = 2048;

const F32 BACKLIGHT_DAY_MAGNITUDE_OBJECT = 0.1f;
const F32 BACKLIGHT_NIGHT_MAGNITUDE_OBJECT = 0.08f;
const F32 ALPHA_BLEND_CUTOFF = 0.598f;
const F32 DEFERRED_LIGHT_FALLOFF = 0.5f;
const U32 DEFERRED_VB_MASK = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_TEXCOORD1;

static const glm::mat4 sGlNdcToSampleBias(0.5f, 0.0f, 0.0f, 0.0f,
                                          0.0f, 0.5f, 0.0f, 0.0f,
                                          0.0f, 0.0f, 0.5f, 0.0f,
                                          0.5f, 0.5f, 0.5f, 1.0f);

extern S32 gBoxFrame;
extern bool gDisplaySwapBuffers;
extern bool gDebugGL;
extern bool gCubeSnapshot;
extern bool gSnapshotNoPost;
extern bool gHeroProbeMirrorRender;

static bool sSceneDepthCopyActive = false;

bool    gAvatarBacklight = false;

LLPipeline gPipeline;
thread_local const LLMatrix4* gGLLastMatrix = NULL;

LLTrace::BlockTimerStatHandle FTM_RENDER_GEOMETRY("Render Geometry");
LLTrace::BlockTimerStatHandle FTM_RENDER_GRASS("Grass");
LLTrace::BlockTimerStatHandle FTM_RENDER_INVISIBLE("Invisible");
LLTrace::BlockTimerStatHandle FTM_RENDER_SHINY("Shiny");
LLTrace::BlockTimerStatHandle FTM_RENDER_SIMPLE("Simple");
LLTrace::BlockTimerStatHandle FTM_RENDER_TERRAIN("Terrain");
LLTrace::BlockTimerStatHandle FTM_RENDER_TREES("Trees");
LLTrace::BlockTimerStatHandle FTM_RENDER_UI("UI");
LLTrace::BlockTimerStatHandle FTM_RENDER_WATER("Water");
LLTrace::BlockTimerStatHandle FTM_RENDER_WL_SKY("Windlight Sky");
LLTrace::BlockTimerStatHandle FTM_RENDER_ALPHA("Alpha Objects");
LLTrace::BlockTimerStatHandle FTM_RENDER_CHARACTERS("Avatars");
LLTrace::BlockTimerStatHandle FTM_RENDER_BUMP("Bump");
LLTrace::BlockTimerStatHandle FTM_RENDER_MATERIALS("Render Materials");
LLTrace::BlockTimerStatHandle FTM_RENDER_FULLBRIGHT("Fullbright");
LLTrace::BlockTimerStatHandle FTM_RENDER_GLOW("Glow");
LLTrace::BlockTimerStatHandle FTM_GEO_UPDATE("Geo Update");
LLTrace::BlockTimerStatHandle FTM_POOLRENDER("RenderPool");
LLTrace::BlockTimerStatHandle FTM_POOLS("Pools");
LLTrace::BlockTimerStatHandle FTM_DEFERRED_POOLRENDER("RenderPool (Deferred)");
LLTrace::BlockTimerStatHandle FTM_DEFERRED_POOLS("Pools (Deferred)");
LLTrace::BlockTimerStatHandle FTM_POST_DEFERRED_POOLRENDER("RenderPool (Post)");
LLTrace::BlockTimerStatHandle FTM_POST_DEFERRED_POOLS("Pools (Post)");
LLTrace::BlockTimerStatHandle FTM_STATESORT("Sort Draw State");
LLTrace::BlockTimerStatHandle FTM_PIPELINE("Pipeline");
LLTrace::BlockTimerStatHandle FTM_CLIENT_COPY("Client Copy");
LLTrace::BlockTimerStatHandle FTM_RENDER_DEFERRED("Deferred Shading");

LLTrace::BlockTimerStatHandle FTM_RENDER_UI_HUD("HUD");
LLTrace::BlockTimerStatHandle FTM_RENDER_UI_3D("3D");
LLTrace::BlockTimerStatHandle FTM_RENDER_UI_2D("2D");

static LLTrace::BlockTimerStatHandle FTM_STATESORT_DRAWABLE("Sort Drawables");


void drawBox(const LLVector4a& c, const LLVector4a& r);
void drawBoxOutline(const LLVector3& pos, const LLVector3& size);
U32 nhpo2(U32 v);
LLVertexBuffer* ll_create_cube_vb(U32 type_mask);

void display_update_camera();

S32     LLPipeline::sCompiles = 0;

bool    LLPipeline::sPickAvatar = true;
bool    LLPipeline::sDynamicLOD = true;
bool    LLPipeline::sShowHUDAttachments = true;
bool    LLPipeline::sRenderMOAPBeacons = false;
bool    LLPipeline::sRenderPhysicalBeacons = true;
bool    LLPipeline::sRenderScriptedBeacons = false;
bool    LLPipeline::sRenderScriptedTouchBeacons = true;
bool    LLPipeline::sRenderParticleBeacons = false;
bool    LLPipeline::sRenderSoundBeacons = false;
bool    LLPipeline::sRenderRegionCornerBeacons = false; // <FS:PP> FIRE-33085 Region corner markers
bool    LLPipeline::sRenderBeacons = false;
bool    LLPipeline::sRenderHighlight = true;
LLRender::eTexIndex LLPipeline::sRenderHighlightTextureChannel = LLRender::DIFFUSE_MAP;
bool    LLPipeline::sForceOldBakedUpload = false;
S32     LLPipeline::sUseOcclusion = 0;
bool    LLPipeline::sAutoMaskAlphaDeferred = true;
bool    LLPipeline::sAutoMaskAlphaNonDeferred = false;
bool    LLPipeline::sRenderTransparentWater = true;
// <FS:AYA> [ParcelHide]
bool    LLPipeline::sParcelHideEnabled = false;
bool    LLPipeline::sParcelHideKeepAvatars = true;
bool    LLPipeline::sParcelHideKeepOwn = true;
S32     LLPipeline::sParcelCheckSeq = 0;
// [ParcelHide-Tag] parsed override values for the agent parcel
bool    LLPipeline::sParcelOwnerTagActive = false;
bool    LLPipeline::sParcelOwnerTagKeepAvatars = false;
bool    LLPipeline::sParcelOwnerTagKeepOwn = false;
std::vector<std::pair<F32, F32>> LLPipeline::sParcelOwnerTagAltRanges;
// </FS:AYA>
bool    LLPipeline::sBakeSunlight = false;
bool    LLPipeline::sNoAlpha = false;
bool    LLPipeline::sUseFarClip = true;
bool    LLPipeline::sDistortionRender = false;
bool    LLPipeline::sImpostorRenderAlphaDepthPass = false;
// <AYAstorm r30 P2>
bool    LLPipeline::sT2xJitterEnabled = false;
bool    LLPipeline::sVelocityRender = false;
// </AYAstorm r30 P2>
bool    LLPipeline::sShowJellyDollAsImpostor = true;
bool    LLPipeline::sTextureBindTest = false;
bool    LLPipeline::sRenderAttachedLights = true;
// <FS:AYAstorm:r30-bd-port> Phase 6 step 2: BD-verbatim attached-light split (Cinematic only)
bool    LLPipeline::sRenderOtherAttachedLights = true;
bool    LLPipeline::sRenderOwnAttachedLights = true;
bool    LLPipeline::sRenderDeferredLights = true;
// </FS:AYAstorm:r30-bd-port>
bool    LLPipeline::sRenderAttachedParticles = true;
S32     LLPipeline::sReflectionProbeLevel = (S32)LLReflectionMap::ProbeLevel::NONE; // <FS:Beq/> [FIRE-35070] Address progressive FPS loss.
S32     LLPipeline::sVisibleLightCount = 0;
F32     LLPipeline::sDistortionWaterClipPlaneMargin = 1.0125f;
LLVector3 LLPipeline::sLastFocusPoint={};// <FS:Beq/> FIRE-16728 focus point lock & free focus DoF 
bool    LLPipeline::sDoFEnabled = false;

F32 LLPipeline::sVolumeSAFrame = 0.f; // ZK LBG

F32 LLPipeline::sLastSkyHdrScale = 1.0f;

F32 LLPipeline::sLastSceneLightStrength = 3.0f;

F32         LLPipeline::sLastMirrorFlag = 0.f;
LLVector4   LLPipeline::sLastClipPlane  = LLVector4(0.f, 0.f, 0.f, 0.f);

bool        LLPipeline::sRenderingDefaultProbeClip = false;
LLVector4   LLPipeline::sRegionClipPlane[4] = { LLVector4(0.f,0.f,0.f,0.f), LLVector4(0.f,0.f,0.f,0.f), LLVector4(0.f,0.f,0.f,0.f), LLVector4(0.f,0.f,0.f,0.f) };

bool    LLPipeline::sRenderParticles; // <FS:LO> flag to hold correct, user selected, status of particles
// [SL:KB] - Patch: Render-TextureToggle (Catznip-4.0)
bool    LLPipeline::sRenderTextures = true;
// [/SL:KB]

// EventHost API LLPipeline listener.
static LLPipelineListener sPipelineListener;

namespace
{
    inline LLCullResult* getFrameCull()
    {
        return LLPipelineFrameContext::getInstance().getCullResult();
    }

    inline LLPipeline::RenderTargetPack* getFrameRT()
    {
        return LLPipelineFrameContext::getInstance().getActiveRT();
    }

    inline bool isFrameShadowPass()     { return LLPipelineFrameContext::getInstance().isShadowPass(); }
    inline bool isFrameReflectionPass() { return LLPipelineFrameContext::getInstance().isReflectionPass(); }
    inline bool isFrameImpostorPass()   { return LLPipelineFrameContext::getInstance().isImpostorPass(); }
    inline bool isFrameHUDPass()        { return LLPipelineFrameContext::getInstance().isHUDPass(); }
    inline bool isFrameDoFPass()        { return LLPipelineFrameContext::getInstance().isDoFPass(); }

    inline bool isFrameRenderingGlow()           { return LLPipelineFrameContext::getInstance().isRenderingGlow(); }
    inline bool isFrameRenderingDeferred()       { return LLPipelineFrameContext::getInstance().isRenderingDeferred(); }
    inline bool isFrameUnderWaterRendering()     { return LLPipelineFrameContext::getInstance().isUnderWaterRendering(); }
    inline bool isFrameReflectionProbesEnabled() { return LLPipelineFrameContext::getInstance().isReflectionProbesEnabled(); }

    F32 ayaDeriveReflectionProbeAmbianceVk(const LLSettingsSky::ptr_t& psky, bool probes_enabled)
    {
        if (!psky) return 0.f;
        F32 pa = (F32)psky->getReflectionProbeAmbiance();
        if (pa != 0.f)
        {
            if (!probes_enabled)
                pa = LLSettingsSky::DEFAULT_AUTO_ADJUST_PROBE_AMBIANCE;
        }
        else if (psky->canAutoAdjust())
        {
            static LLCachedControl<bool> should_auto_adjust(gSavedSettings, "RenderSkyAutoAdjustLegacy", false);
            if (should_auto_adjust)
                pa = LLSettingsSky::sAutoAdjustProbeAmbiance;
        }
        return pa;
    }
}

// Add color attachments for deferred rendering
// target -- RenderTarget to add attachments to
bool addDeferredAttachments(LLRenderTarget& target, bool for_impostor = false)
{
    U32 orm = GL_RGBA;
    U32 norm = GL_RGBA16;
    // <FS:AYA r20 Phase C> gbuffer3 (DEFERRED_EMISSIVE) needs an alpha
    // channel to carry the per-pixel skin bit for the SSS pass. Existing
    // readers only consume .rgb (softenLightF / pointLightF /
    // multiPointLightF), so widening to RGBA is non-breaking.
    U32 emissive = GL_RGBA16F;
    // </FS:AYA>
    // <FS:Beq> FIRE-34483 additional fix
    if (target.getNumTextures() > 1)
    {
        LL_DEBUGS() << "LLPipeline::addDeferredAttachments() - target already has textures - skipping" << LL_ENDL;
        return true;
    }
    // </FS:Beq>
    static LLCachedControl<bool> has_emissive(gSavedSettings, "RenderEnableEmissiveBuffer", false);
    static LLCachedControl<bool> has_hdr(gSavedSettings, "RenderHDREnabled", true);
    bool hdr = has_hdr() && gGLManager.mGLVersion > 4.05f;

    if (!hdr)
    {
        norm = GL_RGB10_A2;
        // <FS:AYA r20 Phase C> see comment above — gbuffer3 needs alpha
        emissive = GL_RGBA;
        // </FS:AYA>
    }

    bool valid = true;
    valid      = valid && target.addColorAttachment(orm);    // frag-data[1] specular OR PBR ORM
    valid      = valid && target.addColorAttachment(norm);
    if (has_emissive)
    {
        valid = valid && target.addColorAttachment(emissive); // frag_data[3] PBR emissive OR material env intensity
    }

    return valid;
}

LLPipeline::LLPipeline() :
    mBackfaceCull(false),
    mMatrixOpCount(0),
    mTextureMatrixOps(0),
    mNumVisibleNodes(0),
    mNumVisibleFaces(0),
    mPoissonOffset(0),

    mInitialized(false),
    mShadersLoaded(false),
    mRenderDebugFeatureMask(0),
    mRenderDebugMask(0),
    mOldRenderDebugMask(0),
    mGroupQ1Locked(false),
    mResetVertexBuffers(false),
    mLastRebuildPool(NULL),
    mLightMask(0),
    mLightMovingMask(0)
{
    for(U32 i = 0; i < 8; i++)
    {
        mHWLightColors[i] = LLColor4::black;
    }
}

void LLPipeline::connectRefreshCachedSettingsSafe(const std::string name)
{
    LLPointer<LLControlVariable> cntrl_ptr = gSavedSettings.getControl(name);
    if ( cntrl_ptr.isNull() )
    {
        LL_WARNS() << "Global setting name not found:" << name << LL_ENDL;
    }
    else
    {
        cntrl_ptr->getCommitSignal()->connect(boost::bind(&LLPipeline::refreshCachedSettings));
    }
}

void LLPipeline::init()
{
    refreshCachedSettings();

    LLPipelineFrameContext::getInstance().setActiveRT(&mMainRT);

    gOctreeMaxCapacity = gSavedSettings.getU32("OctreeMaxNodeCapacity");
    gOctreeMinSize = gSavedSettings.getF32("OctreeMinimumNodeSize");
    sDynamicLOD = gSavedSettings.getBOOL("RenderDynamicLOD");
    sRenderAttachedLights = gSavedSettings.getBOOL("RenderAttachedLights");
    // <FS:AYAstorm:r30-bd-port> Phase 6 step 2
    sRenderOtherAttachedLights = gSavedSettings.getBOOL("RenderOtherAttachedLights");
    sRenderOwnAttachedLights = gSavedSettings.getBOOL("RenderOwnAttachedLights");
    sRenderDeferredLights = gSavedSettings.getBOOL("RenderDeferredLights");
    // </FS:AYAstorm:r30-bd-port>
    sRenderAttachedParticles = gSavedSettings.getBOOL("RenderAttachedParticles");

    sRenderMOAPBeacons = gSavedSettings.getBOOL("moapbeacon");
    sRenderPhysicalBeacons = gSavedSettings.getBOOL("physicalbeacon");
    sRenderScriptedBeacons = gSavedSettings.getBOOL("scriptsbeacon");
    sRenderScriptedTouchBeacons = gSavedSettings.getBOOL("scripttouchbeacon");
    sRenderParticleBeacons = gSavedSettings.getBOOL("particlesbeacon");
    sRenderSoundBeacons = gSavedSettings.getBOOL("soundsbeacon");
    sRenderRegionCornerBeacons = gSavedSettings.getBOOL("fsregioncornerbeacons"); // <FS:PP> FIRE-33085 Region corner markers
    sRenderBeacons = gSavedSettings.getBOOL("renderbeacons");
    sRenderHighlight = gSavedSettings.getBOOL("renderhighlights");

    mReflectionMapManager.refreshSettings();

    mInitialized = true;


    //create render pass pools
    getPool(LLDrawPool::POOL_WATEREXCLUSION);
    getPool(LLDrawPool::POOL_ALPHA_PRE_WATER);
    getPool(LLDrawPool::POOL_ALPHA_POST_WATER);
    getPool(LLDrawPool::POOL_SIMPLE);
    getPool(LLDrawPool::POOL_ALPHA_MASK);
    getPool(LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK);
    getPool(LLDrawPool::POOL_GRASS);
    getPool(LLDrawPool::POOL_FULLBRIGHT);
    getPool(LLDrawPool::POOL_BUMP);
    getPool(LLDrawPool::POOL_MATERIALS);
    getPool(LLDrawPool::POOL_GLOW);
    getPool(LLDrawPool::POOL_GLTF_PBR);
    getPool(LLDrawPool::POOL_GLTF_PBR_ALPHA_MASK);

    resetFrameStats();

    if (gSavedSettings.getBOOL("DisableAllRenderFeatures"))
    {
        clearAllRenderDebugFeatures();
    }
    else
    {
        setAllRenderDebugFeatures(); // By default, all debugging features on
    }
    clearAllRenderDebugDisplays(); // All debug displays off

    sRenderParticles = true; // <FS:LO> flag to hold correct, user selected, status of particles

    if (gSavedSettings.getBOOL("DisableAllRenderTypes"))
    {
        clearAllRenderTypes();
    }
    else if (gNonInteractive)
    {
        clearAllRenderTypes();
    }
    else
    {
        setAllRenderTypes(); // By default, all rendering types start enabled
    }

    // make sure RenderPerformanceTest persists (hackity hack hack)
    // disables non-object rendering (UI, sky, water, etc)
    if (gSavedSettings.getBOOL("RenderPerformanceTest"))
    {
        gSavedSettings.setBOOL("RenderPerformanceTest", false);
        gSavedSettings.setBOOL("RenderPerformanceTest", true);
    }

    mOldRenderDebugMask = mRenderDebugMask;

    mBackfaceCull = true;

    // Enable features
    LLViewerShaderMgr::instance()->setShaders();

    for (U32 i = 0; i < 2; ++i)
    {
        mSpotLightFade[i] = 1.f;
    }

    if (mCubeVB.isNull())
    {
        mCubeVB = ll_create_cube_vb(LLVertexBuffer::MAP_VERTEX);
    }

    // <FS:Ansariel> Reset VB during TP
    //mDeferredVB = new LLVertexBuffer(DEFERRED_VB_MASK);
    //mDeferredVB->allocateBuffer(8, 0);
    initDeferredVB();
    // </FS:Ansariel>

    {
        mScreenTriangleVB = new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX);
        mScreenTriangleVB->allocateBuffer(3, 0);
        LLStrider<LLVector3> vert;
        mScreenTriangleVB->getVertexStrider(vert);

        vert[0].set(-1, 1, 0);
        vert[1].set(-1, -3, 0);
        vert[2].set(3, 1, 0);

        mScreenTriangleVB->unmapBuffer();
    }

    //
    // Update all settings to trigger a cached settings refresh
    //
    connectRefreshCachedSettingsSafe("RenderAutoMaskAlphaDeferred");
    connectRefreshCachedSettingsSafe("RenderAutoMaskAlphaNonDeferred");
    connectRefreshCachedSettingsSafe("RenderUseFarClip");
    connectRefreshCachedSettingsSafe("RenderAvatarMaxNonImpostors");
    connectRefreshCachedSettingsSafe("UseOcclusion");
    // DEPRECATED -- connectRefreshCachedSettingsSafe("WindLightUseAtmosShaders");
    // DEPRECATED -- connectRefreshCachedSettingsSafe("RenderDeferred");
    connectRefreshCachedSettingsSafe("RenderDeferredSunWash");
    connectRefreshCachedSettingsSafe("RenderFSAAType");
    connectRefreshCachedSettingsSafe("RenderResolutionDivisor");
// [SL:KB] - Patch: Settings-RenderResolutionMultiplier | Checked: Catznip-5.4
    connectRefreshCachedSettingsSafe("RenderResolutionMultiplier");
// [/SL:KB]
    connectRefreshCachedSettingsSafe("RenderUIBuffer");
    connectRefreshCachedSettingsSafe("RenderShadowDetail");
    connectRefreshCachedSettingsSafe("RenderShadowSplits");
    connectRefreshCachedSettingsSafe("RenderDeferredSSAO");
    connectRefreshCachedSettingsSafe("RenderShadowResolutionScale");
    // <FS:AYAstorm:r30-bd-port> Phase 3.9
    connectRefreshCachedSettingsSafe("RenderShadowAutomaticDistance");
    // </FS:AYAstorm:r30-bd-port>
    // <FS:AYAstorm:r30-bd-port> Phase 6 step 1
    connectRefreshCachedSettingsSafe("RenderShadowResolution");
    connectRefreshCachedSettingsSafe("RenderShadowDistance");
    connectRefreshCachedSettingsSafe("RenderProjectorShadowResolution");
    // </FS:AYAstorm:r30-bd-port>
    // <FS:AYAstorm:r30-bd-port> Phase 6 step 2
    connectRefreshCachedSettingsSafe("RenderShadowFarClip");
    connectRefreshCachedSettingsSafe("RenderGlobalLightStrength");
    // </FS:AYAstorm:r30-bd-port>
    // <FS:AYAstorm:r30-bd-port> Phase 6 step 3
    connectRefreshCachedSettingsSafe("RenderPostSepiaStrength");
    connectRefreshCachedSettingsSafe("RenderPostGreyscaleStrength");
    connectRefreshCachedSettingsSafe("RenderPostPosterizationSamples");
    // </FS:AYAstorm:r30-bd-port>
    connectRefreshCachedSettingsSafe("RenderDelayCreation");
//  connectRefreshCachedSettingsSafe("RenderAnimateRes"); <FS:Beq> FIRE-23122 BUG-225920 Remove broken RenderAnimateRes functionality.
    connectRefreshCachedSettingsSafe("FreezeTime");
    connectRefreshCachedSettingsSafe("DebugBeaconLineWidth");
    connectRefreshCachedSettingsSafe("RenderHighlightBrightness");
    connectRefreshCachedSettingsSafe("RenderHighlightColor");
    connectRefreshCachedSettingsSafe("RenderHighlightThickness");
    connectRefreshCachedSettingsSafe("RenderSpotLightsInNondeferred");
    connectRefreshCachedSettingsSafe("PreviewAmbientColor");
    connectRefreshCachedSettingsSafe("PreviewDiffuse0");
    connectRefreshCachedSettingsSafe("PreviewSpecular0");
    connectRefreshCachedSettingsSafe("PreviewDiffuse1");
    connectRefreshCachedSettingsSafe("PreviewSpecular1");
    connectRefreshCachedSettingsSafe("PreviewDiffuse2");
    connectRefreshCachedSettingsSafe("PreviewSpecular2");
    connectRefreshCachedSettingsSafe("PreviewDirection0");
    connectRefreshCachedSettingsSafe("PreviewDirection1");
    connectRefreshCachedSettingsSafe("PreviewDirection2");
    connectRefreshCachedSettingsSafe("RenderGlowMinLuminance");
    connectRefreshCachedSettingsSafe("RenderGlowMaxExtractAlpha");
    connectRefreshCachedSettingsSafe("RenderGlowWarmthAmount");
    // <FS:AYAstorm r30 P4> Cinematic Controls switches
    connectRefreshCachedSettingsSafe("RenderDeferredBlurLight");
    connectRefreshCachedSettingsSafe("RenderMotionBlur");
    // </FS:AYAstorm r30 P4>
    connectRefreshCachedSettingsSafe("RenderGlowLumWeights");
    connectRefreshCachedSettingsSafe("RenderGlowWarmthWeights");
    connectRefreshCachedSettingsSafe("RenderGlowResolutionPow");
    connectRefreshCachedSettingsSafe("RenderGlowIterations");
    connectRefreshCachedSettingsSafe("RenderGlowWidth");
    connectRefreshCachedSettingsSafe("RenderGlowStrength");
    connectRefreshCachedSettingsSafe("RenderGlowNoise");
    connectRefreshCachedSettingsSafe("RenderDepthOfField");
    connectRefreshCachedSettingsSafe("RenderDepthOfFieldInEditMode");
    connectRefreshCachedSettingsSafe("CameraFocusTransitionTime");
    connectRefreshCachedSettingsSafe("CameraFNumber");
    connectRefreshCachedSettingsSafe("CameraFocalLength");
    connectRefreshCachedSettingsSafe("CameraFieldOfView");
    connectRefreshCachedSettingsSafe("RenderShadowNoise");
    connectRefreshCachedSettingsSafe("RenderShadowSoftness");
    connectRefreshCachedSettingsSafe("RenderShadowBlurSize");
    connectRefreshCachedSettingsSafe("RenderSSAOScale");
    connectRefreshCachedSettingsSafe("RenderSSAOMaxScale");
    connectRefreshCachedSettingsSafe("RenderSSAOFactor");
    connectRefreshCachedSettingsSafe("RenderSSAOEffect");
    connectRefreshCachedSettingsSafe("RenderShadowOffsetError");
    connectRefreshCachedSettingsSafe("RenderShadowBiasError");
    connectRefreshCachedSettingsSafe("RenderShadowOffset");
    connectRefreshCachedSettingsSafe("RenderShadowBias");
    connectRefreshCachedSettingsSafe("RenderSpotShadowOffset");
    connectRefreshCachedSettingsSafe("RenderSpotShadowBias");
    connectRefreshCachedSettingsSafe("RenderEdgeDepthCutoff");
    connectRefreshCachedSettingsSafe("RenderEdgeNormCutoff");
    connectRefreshCachedSettingsSafe("RenderShadowGaussian");
    connectRefreshCachedSettingsSafe("RenderShadowBlurDistFactor");
    connectRefreshCachedSettingsSafe("RenderDeferredAtmospheric");
    connectRefreshCachedSettingsSafe("RenderHighlightFadeTime");
    connectRefreshCachedSettingsSafe("RenderFarClip");
    connectRefreshCachedSettingsSafe("RenderShadowSplitExponent");
    connectRefreshCachedSettingsSafe("RenderShadowErrorCutoff");
    connectRefreshCachedSettingsSafe("RenderShadowFOVCutoff");
    connectRefreshCachedSettingsSafe("CameraOffset");
    connectRefreshCachedSettingsSafe("CameraMaxCoF");
    connectRefreshCachedSettingsSafe("CameraDoFResScale");
    connectRefreshCachedSettingsSafe("RenderAutoHideSurfaceAreaLimit");
    connectRefreshCachedSettingsSafe("RenderScreenSpaceReflections");
    connectRefreshCachedSettingsSafe("RenderScreenSpaceReflectionIterations");
    connectRefreshCachedSettingsSafe("RenderScreenSpaceReflectionRayStep");
    connectRefreshCachedSettingsSafe("RenderScreenSpaceReflectionDistanceBias");
    connectRefreshCachedSettingsSafe("RenderScreenSpaceReflectionDepthRejectBias");
    connectRefreshCachedSettingsSafe("RenderScreenSpaceReflectionAdaptiveStepMultiplier");
    connectRefreshCachedSettingsSafe("RenderScreenSpaceReflectionGlossySamples");
    connectRefreshCachedSettingsSafe("RenderBufferVisualization");
    connectRefreshCachedSettingsSafe("RenderMirrors");
    connectRefreshCachedSettingsSafe("RenderHeroProbeUpdateRate");
    connectRefreshCachedSettingsSafe("RenderHeroProbeConservativeUpdateMultiplier");
    connectRefreshCachedSettingsSafe("RenderAvatarCloth");
    connectRefreshCachedSettingsSafe("FSRenderVignette");   // <FS:CR> Import Vignette from Exodus
    // <FS:Ansariel> Make change to RenderAttachedLights & RenderAttachedParticles instant
    connectRefreshCachedSettingsSafe("RenderAttachedLights");
    connectRefreshCachedSettingsSafe("RenderAttachedParticles");
    // </FS:Ansariel>
    // <FS:AYAstorm:r30-bd-port> Phase 6 step 2
    connectRefreshCachedSettingsSafe("RenderOtherAttachedLights");
    connectRefreshCachedSettingsSafe("RenderOwnAttachedLights");
    connectRefreshCachedSettingsSafe("RenderDeferredLights");
    // </FS:AYAstorm:r30-bd-port>
    // <FS:Beq> FIRE-16728 Add free aim mouse and focus lock
    connectRefreshCachedSettingsSafe("FSFocusPointFollowsPointer");
    connectRefreshCachedSettingsSafe("FSFocusPointLocked");
    // </FS:Beq>
    // <FS:PP> FIRE-33085 Region corner markers
    connectRefreshCachedSettingsSafe("fsregioncornerbeacons");
    // </FS:PP>
    // <FS:PP> FIRE-36767 Sync beacon settings when changed via debug settings
    connectRefreshCachedSettingsSafe("physicalbeacon");
    connectRefreshCachedSettingsSafe("scriptsbeacon");
    connectRefreshCachedSettingsSafe("scripttouchbeacon");
    connectRefreshCachedSettingsSafe("soundsbeacon");
    connectRefreshCachedSettingsSafe("particlesbeacon");
    connectRefreshCachedSettingsSafe("moapbeacon");
    connectRefreshCachedSettingsSafe("renderbeacons");
    connectRefreshCachedSettingsSafe("renderhighlights");
    // </FS:PP>

    LLPointer<LLControlVariable> cntrl_ptr = gSavedSettings.getControl("CollectFontVertexBuffers");
    if (cntrl_ptr.notNull())
    {
        cntrl_ptr->getCommitSignal()->connect([](LLControlVariable* control, const LLSD& value, const LLSD& previous)
        {
            LLFontVertexBuffer::enableBufferCollection(control->getValue().asBoolean());
        });
    }

    if (LLVKLoader::shouldUseVulkanRender())
    {
        LLVKLoader::setVulkanPresentationEnabled(true);
    }
}

LLPipeline::~LLPipeline()
{
}

void LLPipeline::cleanup()
{
    assertInitialized();

    mGroupQ1.clear() ;

    for(pool_set_t::iterator iter = mPools.begin();
        iter != mPools.end(); )
    {
        pool_set_t::iterator curiter = iter++;
        LLDrawPool* poolp = *curiter;
        if (poolp->isFacePool())
        {
            LLFacePool* face_pool = (LLFacePool*) poolp;
            if (face_pool->mReferences.empty())
            {
                mPools.erase(curiter);
                removeFromQuickLookup( poolp );
                delete poolp;
            }
        }
        else
        {
            mPools.erase(curiter);
            removeFromQuickLookup( poolp );
            delete poolp;
        }
    }

    if (!mTerrainPools.empty())
    {
        LL_WARNS() << "Terrain Pools not cleaned up" << LL_ENDL;
    }
    if (!mTreePools.empty())
    {
        LL_WARNS() << "Tree Pools not cleaned up" << LL_ENDL;
    }

    delete mAlphaPoolPreWater;
    mAlphaPoolPreWater = nullptr;
    delete mAlphaPoolPostWater;
    mAlphaPoolPostWater = nullptr;
    delete mSkyPool;
    mSkyPool = NULL;
    delete mTerrainPool;
    mTerrainPool = NULL;
    delete mWaterPool;
    mWaterPool = NULL;
    delete mSimplePool;
    mSimplePool = NULL;
    delete mFullbrightPool;
    mFullbrightPool = NULL;
    delete mGlowPool;
    mGlowPool = NULL;
    delete mBumpPool;
    mBumpPool = NULL;
    // don't delete wl sky pool it was handled above in the for loop
    //delete mWLSkyPool;
    mWLSkyPool = NULL;
    delete mWaterExclusionPool;
    mWaterExclusionPool = nullptr;

    releaseGLBuffers();

    mFaceSelectImagep = NULL;

    mMovedList.clear();
    mMovedBridge.clear();
    mShiftList.clear();

    mInitialized = false;

    mDeferredVB = NULL;
    mScreenTriangleVB = nullptr;

    mCubeVB = NULL;

    mReflectionMapManager.cleanup();
    mHeroProbeManager.cleanup();
}


void LLPipeline::destroyGL()
{
    unloadShaders();
    mHighlightFaces.clear();

    resetDrawOrders();

    releaseGLBuffers();

}

void LLPipeline::requestResizeScreenTexture()
{
    gResizeScreenTexture = true;
}

void LLPipeline::requestResizeShadowTexture()
{
    gResizeShadowTexture = true;
}

void LLPipeline::resizeShadowTexture()
{
    releaseSunShadowTargets();
    releaseSpotShadowTargets();
    allocateShadowBuffer(getFrameRT()->screen.getWidth(), getFrameRT()->screen.getHeight()); // <FS:Beq> revert and correct previous shadowres fix that leads to FPS drop (FIRE-3200)
    gResizeShadowTexture = false;
}

void LLPipeline::resizeScreenTexture()
{
    if (gPipeline.shadersLoaded())
    {
        GLuint resX = gViewerWindow->getWorldViewWidthRaw();
        GLuint resY = gViewerWindow->getWorldViewHeightRaw();

// [SL:KB] - Patch: Settings-RenderResolutionMultiplier | Checked: Catznip-5.4
        GLuint scaledResX = resX;
        GLuint scaledResY = resY;
        if ( (RenderResolutionDivisor > 1) && (RenderResolutionDivisor < resX) && (RenderResolutionDivisor < resY) )
        {
            scaledResX /= RenderResolutionDivisor;
            scaledResY /= RenderResolutionDivisor;
        }
        else if (RenderResolutionMultiplier > 0.f && RenderResolutionMultiplier < 1.f)
        {
            scaledResX = (GLuint)(scaledResX * RenderResolutionMultiplier);
            scaledResY = (GLuint)(scaledResY * RenderResolutionMultiplier);
        }
// [/SL:KB]

//      if (gResizeScreenTexture || (resX != getFrameRT()->screen.getWidth()) || (resY != getFrameRT()->screen.getHeight()))
// [SL:KB] - Patch: Settings-RenderResolutionMultiplier | Checked: Catznip-5.4
        if (gResizeScreenTexture || (scaledResX != getFrameRT()->screen.getWidth()) || (scaledResY != getFrameRT()->screen.getHeight()))
// [/SL:KB]
        {
            releaseScreenBuffers();
            releaseSunShadowTargets();
            releaseSpotShadowTargets();
            allocateScreenBuffer(resX,resY);
            gResizeScreenTexture = false;
        }
    }
}

bool LLPipeline::allocateScreenBuffer(U32 resX, U32 resY)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    eFBOStatus ret = doAllocateScreenBuffer(resX, resY);

    return ret == FBO_SUCCESS_FULLRES;
}


LLPipeline::eFBOStatus LLPipeline::doAllocateScreenBuffer(U32 resX, U32 resY)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    // try to allocate screen buffers at requested resolution and samples
    // - on failure, shrink number of samples and try again
    // - if not multisampled, shrink resolution and try again (favor X resolution over Y)
    // Make sure to call "releaseScreenBuffers" after each failure to cleanup the partially loaded state

    // refresh cached settings here to protect against inconsistent event handling order
    refreshCachedSettings();

    eFBOStatus ret = FBO_SUCCESS_FULLRES;
    if (!allocateScreenBufferInternal(resX, resY))
    {
        //failed to allocate at requested specification, return false
        ret = FBO_FAILURE;

        releaseScreenBuffers();

        //reduce resolution
        while (resY > 0 && resX > 0)
        {
            resY /= 2;
            if (allocateScreenBufferInternal(resX, resY))
            {
                return FBO_SUCCESS_LOWRES;
            }
            releaseScreenBuffers();

            resX /= 2;
            if (allocateScreenBufferInternal(resX, resY))
            {
                return FBO_SUCCESS_LOWRES;
            }
            releaseScreenBuffers();
        }

        LL_WARNS() << "Unable to allocate screen buffer at any resolution!" << LL_ENDL;
    }

    return ret;
}

bool LLPipeline::allocateScreenBufferInternal(U32 resX, U32 resY)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;

    static LLCachedControl<bool> has_hdr(gSavedSettings, "RenderHDREnabled", true);
    bool hdr = gGLManager.mGLVersion > 4.05f && has_hdr();

    if (getFrameRT() == &mMainRT)
    { // hacky -- allocate auxillary buffer
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("check reflection map setup"); // <FS:Beq/> improve Tracy scoping 

        gCubeSnapshot = true;

        if (isFrameReflectionProbesEnabled())
        {
            mReflectionMapManager.initReflectionMaps();
        }

        LLPipelineFrameContext::getInstance().setActiveRT(&mAuxillaryRT);
        U32 res = mReflectionMapManager.mProbeResolution * 4;  //multiply by 4 because probes will be 16x super sampled
        allocateScreenBufferInternal(res, res);

        if (RenderMirrors)
        {
            mHeroProbeManager.initReflectionMaps();
            res = mHeroProbeManager.mProbeResolution;  // We also scale the hero probe RT to the probe res since we don't super sample it.
            LLPipelineFrameContext::getInstance().setActiveRT(&mHeroProbeRT);
            allocateScreenBufferInternal(res, res);
        }

        LLPipelineFrameContext::getInstance().setActiveRT(&mMainRT);
        gCubeSnapshot = false;
    }

    // remember these dimensions
    getFrameRT()->width = resX;
    getFrameRT()->height = resY;

    U32 res_mod = RenderResolutionDivisor;

    //<FS:TS> FIRE-7066: RenderResolutionDivisor broken if higher than
    //      smallest screen dimension
    if (res_mod >= resX)
    {
        res_mod = resX - 1;
    }
    if (res_mod >= resY)
    {
        res_mod = resY - 1;
    }
    //</FS:TS> FIRE-7066

    if (res_mod > 1 && res_mod < resX && res_mod < resY)
    {
        resX /= res_mod;
        resY /= res_mod;
    }
// [SL:KB] - Patch: Settings-RenderResolutionMultiplier | Checked: Catznip-5.4
    else if (RenderResolutionMultiplier > 0.f && RenderResolutionMultiplier < 1.f)
    {
        resX = (GLuint)(resX * RenderResolutionMultiplier);
        resY = (GLuint)(resY * RenderResolutionMultiplier);
    }
// [/SL:KB]

    S32 shadow_detail = RenderShadowDetail;
    bool ssao = RenderDeferredSSAO;

    //allocate deferred rendering color buffers
    if (!getFrameRT()->deferredScreen.allocate(resX, resY, GL_RGBA, true)) return false;
    if (!addDeferredAttachments(getFrameRT()->deferredScreen)) return false;

    GLuint screenFormat = hdr ? GL_RGBA16F : GL_RGBA;

    if (!getFrameRT()->screen.allocate(resX, resY, GL_RGBA16F)) return false;

    getFrameRT()->deferredScreen.shareDepthBuffer(getFrameRT()->screen);

    // <FS:Beq> restore setSphere
    // if (hdr || shadow_detail > 0 || ssao || RenderDepthOfField))
    if (hdr || shadow_detail > 0 || ssao || RenderDepthOfField || RlvActions::hasPostProcess())
    // </FS:Beq>
    { //only need getFrameRT()->deferredLight for shadows OR ssao OR dof OR fxaa
        if (!getFrameRT()->deferredLight.allocate(resX, resY, screenFormat)) return false;
    }
    else
    {
        getFrameRT()->deferredLight.release();
    }

    allocateShadowBuffer(resX, resY);

    if (!gCubeSnapshot) // hack to not re-allocate various targets for cube snapshots
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("non-cube allocations"); // <FS:Beq/> improve Tracy scoping
        if (RenderUIBuffer)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("UIBuffer"); // <FS:Beq/> improve Tracy scoping
            if (!mUIScreen.allocate(resX, resY, GL_RGBA))
            {
                return false;
            }
        }

        if (!mScenePresentRT[0].allocate(resX, resY, GL_RGBA, true)) return false;
        if (!mScenePresentRT[1].allocate(resX, resY, GL_RGBA)) return false;
        mScenePresentRT[0].shareDepthBuffer(mScenePresentRT[1]);

        // <AYAstorm:r21.1> GPU self-rigged picker ID buffer.
        // Allocated only on the main RT (not aux / hero probe). Borrows the
        // deferred depth buffer so the ID pass agrees pixel-for-pixel with
        // the real scene without re-writing depth. Note the call order:
        // `A.shareDepthBuffer(B)` lends A's depth to B, so the lender (the
        // one that already owns depth) goes on the left.
        if (getFrameRT() == &mMainRT)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("ObjectIDBuffer");
            if (!mObjectIDBuffer.allocate(resX, resY, GL_RGBA, false)) return false;
            getFrameRT()->deferredScreen.shareDepthBuffer(mObjectIDBuffer);
        }
        // </AYAstorm:r21.1>

        // <AYAstorm r30 P2> Velocity buffer + SMAA T2x history buffer.
        // Cinematic-only (AYAVisualRealismEnabled == 2). Same allocate
        // pattern as r21.1 mObjectIDBuffer just above: main RT only, share
        // depth with deferredScreen. Reads the cvar via LLCachedControl
        // because the View Mode is restart-required (r30 P1) — switching
        // away mid-session does not actually re-enter this code path until
        // the next allocateScreenBufferInternal call, and the display() side
        // gate checks mVelocityMap.isComplete() before using it.
        if (getFrameRT() == &mMainRT)
        {
            static LLCachedControl<U32> aya_view_mode(gSavedSettings, "AYAVisualRealismEnabled", 1);
            if (aya_view_mode == 2)
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("CinematicBuffers");
                if (!mVelocityMap.allocate(resX, resY, GL_RG16F, false)) return false;
                getFrameRT()->deferredScreen.shareDepthBuffer(mVelocityMap);
                if (!mSMAAHistory.allocate(resX, resY, GL_RGBA, false)) return false;
                mSMAAHistory.bindTarget();
                mSMAAHistory.clear();
                mSMAAHistory.flush();
                LL_INFOS("Pipeline") << "AYAstorm r30 P2: allocated mVelocityMap (RG16F) + mSMAAHistory (RGBA) at " << resX << "x" << resY << LL_ENDL;
            }
            else
            {
                mVelocityMap.release();
                mSMAAHistory.release();
            }
        }
        // </AYAstorm r30 P2>

        // <AYAstorm r30 P5 transparent-DoF L2-β> Allocate the alpha-aware
        // depth RT. Color attachment is unused (we only read/write depth)
        // but LLRenderTarget needs depth+color for gCopyDepthProgram to
        // emit gl_FragDepth. Main RT only — DoF doesn't run on aux/probe
        // paths.
        if (getFrameRT() == &mMainRT)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("AYAAlphaDepth");
            if (!mAYAAlphaDepth.allocate(resX, resY, GL_RGBA, true)) return false;
            if (LLVKLoader::isVulkanInitialized())
            {
                if (!mSceneDepthCopy.allocate(resX, resY, GL_RGBA, true)) return false;
            }
        }
        // </AYAstorm r30 P5 transparent-DoF L2-β>

        // <AYAstorm r30 P5 transparent-DoF C-(a)> Dedicated color RT for
        // forward alpha BLEND. RGBA16F to preserve HDR scene buffer
        // precision (matches getFrameRT()->screen). depth=false here — we share
        // getFrameRT()->screen's depth attachment via shareDepthBuffer below so
        // alpha BLEND draws still depth-test against opaque geometry
        // without re-allocating depth. Main RT only — DoF doesn't run on
        // aux / probe / impostor / HUD paths.
        if (getFrameRT() == &mMainRT)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("AYAAlphaColor");
            if (!mAYAAlphaColor.allocate(resX, resY, GL_RGBA16F, false)) return false;
            // deferredScreen owns depth (allocate(..., true) above) and has
            // already lent it to getFrameRT()->screen. Borrow the same attachment so
            // alpha BLEND depth-tests/writes match the rest of the scene.
            getFrameRT()->deferredScreen.shareDepthBuffer(mAYAAlphaColor);
        }
        // </AYAstorm r30 P5 transparent-DoF C-(a)>

        if (getFrameRT() == &mMainRT)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("ForwardColor");
            if (!mForwardColor.allocate(resX, resY, GL_RGBA16F, false)) return false;
            getFrameRT()->deferredScreen.shareDepthBuffer(mForwardColor);
        }

        if (RenderFSAAType > 0)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("FSAABuffer"); // <FS:Beq/> improve Tracy scoping 
            if (!mFXAAMap.allocate(resX, resY, GL_RGBA)) return false;
            if (RenderFSAAType == 2)
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("SMAABuffer"); // <FS:Beq/> improve Tracy scoping 
                if (!mSMAABlendBuffer.allocate(resX, resY, GL_RGBA, false)) return false;
            }
        }
        else
        {
            mFXAAMap.release();
            mSMAABlendBuffer.release();
        }

        //water reflection texture (always needed as scratch space whether or not transparent water is enabled)
        mWaterDis.allocate(resX, resY, screenFormat, true);

        if(RenderScreenSpaceReflections)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("SSRBuffer"); // <FS:Beq/> improve Tracy scoping 
            mSceneMap.allocate(resX, resY, screenFormat, true);
        }
        else
        {
            mSceneMap.release();
        }

        {LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("mPostMapBuffer"); // <FS:Beq/> improve Tracy scoping
        mPostPingMap.allocate(resX, resY, GL_RGBA);
        mPostPongMap.allocate(resX, resY, GL_RGBA);
        } // <FS:Beq/> improve Tracy scoping
        // The water exclusion mask needs its own depth buffer so we can take care of the problem of multiple water planes.
        // Should we ever make water not just a plane, it also aids with that as well as the water planes will be rendered into the mask.
        // Why do we do this? Because it saves us some janky logic in the exclusion shader when we generate the mask.
        // Regardless, this should always only be an R8 texture unless we choose to start having multiple kinds of exclusion that 8 bits can't handle.
        // - Geenz 2025-02-06
        bool success = mWaterExclusionMask.allocate(resX, resY, GL_R8, true);

        assert(success);

        // used to scale down textures
        // See LLViwerTextureList::updateImagesCreateTextures and LLImageGL::scaleDown
        {LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("DownResBuffer");// <FS:Beq/> create an independent preview screen target
        mDownResMap.allocate(1024, 1024, GL_RGBA);
        }// <FS:Beq/> create an independent preview screen target

        // <FS:Beq> create an independent preview screen target
        {LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("PreviewScreenBuffer");
        mPreviewScreen.allocate(MAX_PREVIEW_WIDTH, MAX_PREVIEW_HEIGHT, GL_RGBA, true); 
        } // </FS:Beq>
        {LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("BakeMapBuffer");// <FS:Beq/> create an independent preview screen target
        mBakeMap.allocate(LLAvatarAppearanceDefines::SCRATCH_TEX_WIDTH, LLAvatarAppearanceDefines::SCRATCH_TEX_HEIGHT, GL_RGBA);
        }// <FS:Beq/> create an independent preview screen target
    }
    //HACK make screenbuffer allocations start failing after 30 seconds
    if (gSavedSettings.getBOOL("SimulateFBOFailure"))
    {
        return false;
    }

    gGL.getTexUnit(0)->disable();


    return true;
}

// must be even to avoid a stripe in the horizontal shadow blur
inline U32 BlurHappySize(U32 x, F32 scale) { return U32( x * scale + 16.0f) & ~0xF; }

bool LLPipeline::allocateShadowBuffer(U32 resX, U32 resY)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    S32 shadow_detail = RenderShadowDetail;

    F32 scale = gCubeSnapshot ? 1.0f : llmax(0.f, RenderShadowResolutionScale); // Don't scale probe shadow maps
    U32 sun_shadow_map_width = BlurHappySize(resX, scale);
    U32 sun_shadow_map_height = BlurHappySize(resY, scale);

    // <FS:AYAstorm:r30-bd-port> Phase 6 step 1: BD per-cascade shadow allocation (Cinematic only)
    const bool cinematic_per_channel_shadow = isCinematicMode() && !gCubeSnapshot;
    // </FS:AYAstorm:r30-bd-port>

    if (shadow_detail > 0)
    { //allocate 4 sun shadow maps
        for (U32 i = 0; i < LLPipeline::kSunShadowCount; i++)
        {
            // <FS:AYAstorm:r30-bd-port> Phase 6 step 1
            // <FS:AYAstorm r30 cleanup A.3> Apply RenderShadowResolutionScale uniformly to
            // the per-cascade Vector4. Without this, the scale slider was inert in Cinematic.
            // llmax(64.f, ...) prevents 0-size allocation when scale is set to 0.
            if (cinematic_per_channel_shadow)
            {
                U32 res = (U32)llmax(64.f, RenderShadowResolution.mV[i] * scale);
                if (getFrameRT()->shadow[i].getWidth() != res)
                {
                    if (!getFrameRT()->shadow[i].allocate(res, res, 0, true))
                    {
                        return false;
                    }
                }
                continue;
            }
            // </FS:AYAstorm:r30-bd-port>
            if (!getFrameRT()->shadow[i].allocate(sun_shadow_map_width, sun_shadow_map_height, 0, true))
            {
                return false;
            }
        }
    }
    else
    {
        for (U32 i = 0; i < 4; i++)
        {
            releaseSunShadowTarget(i);
        }
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
                // <FS:AYAstorm:r30-bd-port> Phase 6 step 1
                // <FS:AYAstorm r30 cleanup A.3> Scale applied for symmetry with sun cascades.
                if (cinematic_per_channel_shadow)
                {
                    U32 res = (U32)llmax(64.f, RenderProjectorShadowResolution.mV[i] * scale);
                    if (!mSpotShadow[i].allocate(res, res, 0, true))
                    {
                        return false;
                    }
                    continue;
                }
                // </FS:AYAstorm:r30-bd-port>
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


    // set up shadow map filtering and compare modes
    if (shadow_detail > 0)
    {
        for (U32 i = 0; i < LLPipeline::kSunShadowCount; i++)
        {
            LLRenderTarget* shadow_target = getSunShadowTarget(i);
            if (shadow_target)
            {
                gGL.getTexUnit(0)->bind(getSunShadowTarget(i), true);
                gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
                gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);

                shadow_target->setUseDepthCompareSampler(true);
            }
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

void LLPipeline::updateRenderTransparentWater()
{
    sRenderTransparentWater = gSavedSettings.getBOOL("RenderTransparentWater");
}

void LLPipeline::refreshCachedSettings()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    // <FS:AYAstorm r30 BD full port Phase 3.4>
    // Cinematic (mode 2) では BD default に固定、それ以外は gSavedSettings の値。
    // 対象 cvar 一覧と BD default は
    // docs/specs/ayastorm-r30-bd-full-port-phase2-spec.md §3.3 / Phase 0 inventory cvar bucket 3 を参照。
    LLPipeline::sAutoMaskAlphaDeferred = gSavedSettings.getBOOL("RenderAutoMaskAlphaDeferred");
    LLPipeline::sAutoMaskAlphaNonDeferred = gSavedSettings.getBOOL("RenderAutoMaskAlphaNonDeferred");
    // </FS:AYAstorm>
    LLPipeline::sUseFarClip = gSavedSettings.getBOOL("RenderUseFarClip");
    // <FS:AYAstorm r30 BD full port Phase 3.7 cat 01> Cinematic は BD impostor
    // 機構と等価 = JellyDoll を impostor 化 (true)。AY default も true で同値、
    // mode 0/1 は gSavedSettings 値維持。spec §3.2 phase3.5-ay-only Category B。
    LLPipeline::sShowJellyDollAsImpostor = gSavedSettings.getBOOL("RenderJellyDollsAsImpostors");
    // </FS:AYAstorm>
    LLVOAvatar::sMaxNonImpostors = gSavedSettings.getU32("RenderAvatarMaxNonImpostors");
    LLVOAvatar::updateImpostorRendering(LLVOAvatar::sMaxNonImpostors);
    // <FS:Ansariel> Make change to RenderAttachedLights & RenderAttachedParticles instant
    LLPipeline::sRenderAttachedLights = gSavedSettings.getBOOL("RenderAttachedLights");
    LLPipeline::sRenderAttachedParticles = gSavedSettings.getBOOL("RenderAttachedParticles");
    // </FS:Ansariel>
    // <FS:AYAstorm:r30-bd-port> Phase 6 step 2
    LLPipeline::sRenderOtherAttachedLights = gSavedSettings.getBOOL("RenderOtherAttachedLights");
    LLPipeline::sRenderOwnAttachedLights = gSavedSettings.getBOOL("RenderOwnAttachedLights");
    LLPipeline::sRenderDeferredLights = gSavedSettings.getBOOL("RenderDeferredLights");
    // </FS:AYAstorm:r30-bd-port>
    // <FS:PP> FIRE-33085 Region corner markers
    LLPipeline::sRenderRegionCornerBeacons = gSavedSettings.getBOOL("fsregioncornerbeacons");
    // </FS:PP>
    // <FS:PP> FIRE-36767 Sync beacon settings when changed via debug settings
    LLPipeline::sRenderPhysicalBeacons = gSavedSettings.getBOOL("physicalbeacon");
    LLPipeline::sRenderScriptedBeacons = gSavedSettings.getBOOL("scriptsbeacon");
    LLPipeline::sRenderScriptedTouchBeacons = gSavedSettings.getBOOL("scripttouchbeacon");
    LLPipeline::sRenderSoundBeacons = gSavedSettings.getBOOL("soundsbeacon");
    LLPipeline::sRenderParticleBeacons = gSavedSettings.getBOOL("particlesbeacon");
    LLPipeline::sRenderMOAPBeacons = gSavedSettings.getBOOL("moapbeacon");
    LLPipeline::sRenderBeacons = gSavedSettings.getBOOL("renderbeacons");
    LLPipeline::sRenderHighlight = gSavedSettings.getBOOL("renderhighlights");
    // </FS:PP>

    LLPipeline::sUseOcclusion =
            (!gUseWireframe
            && LLFeatureManager::getInstance()->isFeatureAvailable("UseOcclusion")
            && gSavedSettings.getBOOL("UseOcclusion")) ? 2 : 0;

    WindLightUseAtmosShaders = true; // DEPRECATED -- gSavedSettings.getBOOL("WindLightUseAtmosShaders");
    RenderDeferred = true; // DEPRECATED -- gSavedSettings.getBOOL("RenderDeferred");
    RenderDeferredSunWash = gSavedSettings.getF32("RenderDeferredSunWash");
    RenderFSAAType = gSavedSettings.getU32("RenderFSAAType");
    RenderResolutionDivisor = gSavedSettings.getU32("RenderResolutionDivisor");
// [SL:KB] - Patch: Settings-RenderResolutionMultiplier | Checked: Catznip-5.4
    // <FS:AYAstorm r30 BD full port Phase 3.7 cat 01> Cinematic は BD parity
    // (1.0f = multiplier 無効、native 解像度) に固定。BD には resolution
    // multiplier 機構が無い。spec §3.2 phase3.5-ay-only Category B。
    RenderResolutionMultiplier = gSavedSettings.getF32("RenderResolutionMultiplier");
    // </FS:AYAstorm>
// [/SL:KB]
    RenderUIBuffer = gSavedSettings.getBOOL("RenderUIBuffer");
    RenderShadowDetail = gSavedSettings.getS32("RenderShadowDetail");
    RenderShadowSplits = gSavedSettings.getS32("RenderShadowSplits");
    RenderDeferredSSAO = gSavedSettings.getBOOL("RenderDeferredSSAO");
    RenderShadowResolutionScale = gSavedSettings.getF32("RenderShadowResolutionScale");
    // <FS:AYAstorm:r30-bd-port> Phase 3.9
    RenderShadowAutomaticDistance = gSavedSettings.getBOOL("RenderShadowAutomaticDistance");
    // </FS:AYAstorm:r30-bd-port>
    // <FS:AYAstorm:r30-bd-port> Phase 6 step 1: BD per-channel shadow allocation
    RenderShadowResolution = gSavedSettings.getVector4("RenderShadowResolution");
    RenderShadowFarClipVec = gSavedSettings.getVector4("RenderShadowDistance");
    RenderProjectorShadowResolution = gSavedSettings.getVector2("RenderProjectorShadowResolution");
    // </FS:AYAstorm:r30-bd-port>
    // <FS:AYAstorm:r30-bd-port> Phase 6 step 2: BD live scalar cvar
    RenderShadowFarClip = gSavedSettings.getF32("RenderShadowFarClip");
    RenderGlobalLightStrength = gSavedSettings.getF32("RenderGlobalLightStrength");
    // </FS:AYAstorm:r30-bd-port>
    // <FS:AYAstorm:r30-bd-port> Phase 6 step 3: BD live Post FX scalar cvar
    RenderSepiaStrength = gSavedSettings.getF32("RenderPostSepiaStrength");
    RenderGreyscaleStrength = gSavedSettings.getF32("RenderPostGreyscaleStrength");
    RenderNumColors = gSavedSettings.getU32("RenderPostPosterizationSamples");
    // </FS:AYAstorm:r30-bd-port>
    RenderDelayCreation = gSavedSettings.getBOOL("RenderDelayCreation");
//  RenderAnimateRes = gSavedSettings.getBOOL("RenderAnimateRes"); <FS:Beq> FIRE-23122 BUG-225920 Remove broken RenderAnimateRes functionality.
    FreezeTime = gSavedSettings.getBOOL("FreezeTime");
    DebugBeaconLineWidth = gSavedSettings.getS32("DebugBeaconLineWidth");
    RenderHighlightBrightness = gSavedSettings.getF32("RenderHighlightBrightness");
    RenderHighlightColor = gSavedSettings.getColor4("RenderHighlightColor");
    RenderHighlightThickness = gSavedSettings.getF32("RenderHighlightThickness");
    RenderSpotLightsInNondeferred = gSavedSettings.getBOOL("RenderSpotLightsInNondeferred");
    PreviewAmbientColor = gSavedSettings.getColor4("PreviewAmbientColor");
    PreviewDiffuse0 = gSavedSettings.getColor4("PreviewDiffuse0");
    PreviewSpecular0 = gSavedSettings.getColor4("PreviewSpecular0");
    PreviewDiffuse1 = gSavedSettings.getColor4("PreviewDiffuse1");
    PreviewSpecular1 = gSavedSettings.getColor4("PreviewSpecular1");
    PreviewDiffuse2 = gSavedSettings.getColor4("PreviewDiffuse2");
    PreviewSpecular2 = gSavedSettings.getColor4("PreviewSpecular2");
    PreviewDirection0 = gSavedSettings.getVector3("PreviewDirection0");
    PreviewDirection1 = gSavedSettings.getVector3("PreviewDirection1");
    PreviewDirection2 = gSavedSettings.getVector3("PreviewDirection2");
    RenderGlowMaxExtractAlpha = gSavedSettings.getF32("RenderGlowMaxExtractAlpha");
    RenderGlowMinLuminance = gSavedSettings.getF32("RenderGlowMinLuminance");
    // <FS:AYAstorm r30 P4> Cinematic Controls switches must take effect at runtime.
    RenderDeferredBlurLight = gSavedSettings.getBOOL("RenderDeferredBlurLight");
    RenderMotionBlur = gSavedSettings.getBOOL("RenderMotionBlur");
    // </FS:AYAstorm r30 P4>
    RenderGlowWarmthAmount = gSavedSettings.getF32("RenderGlowWarmthAmount");
    RenderGlowLumWeights = gSavedSettings.getVector3("RenderGlowLumWeights");
    RenderGlowWarmthWeights = gSavedSettings.getVector3("RenderGlowWarmthWeights");
    RenderGlowResolutionPow = gSavedSettings.getS32("RenderGlowResolutionPow");
    RenderGlowIterations = gSavedSettings.getS32("RenderGlowIterations");
    RenderGlowWidth = gSavedSettings.getF32("RenderGlowWidth");
    RenderGlowStrength = gSavedSettings.getF32("RenderGlowStrength");
    RenderGlowNoise = gSavedSettings.getBOOL("RenderGlowNoise");
    RenderDepthOfField = gSavedSettings.getBOOL("RenderDepthOfField");
    RenderDepthOfFieldInEditMode = gSavedSettings.getBOOL("RenderDepthOfFieldInEditMode");
    // <FS:Beq> FIRE-16728 Add free aim mouse and focus lock
    FSFocusPointLocked = gSavedSettings.getBOOL("FSFocusPointLocked");
    FSFocusPointFollowsPointer = gSavedSettings.getBOOL("FSFocusPointFollowsPointer");
    // </FS:Beq>
    CameraFocusTransitionTime = gSavedSettings.getF32("CameraFocusTransitionTime");
    CameraFNumber = gSavedSettings.getF32("CameraFNumber");
    CameraFocalLength = gSavedSettings.getF32("CameraFocalLength");
    CameraFieldOfView = gSavedSettings.getF32("CameraFieldOfView");
    RenderShadowNoise = gSavedSettings.getF32("RenderShadowNoise");
    // <FS:AYAstorm r30 BD full port Phase 3.7 cat 01> AY-only shadow softness
    // 拡張、Cinematic は BD parity (1.0f = 無補正) に固定。spec §3.2 phase3.5-ay-only。
    RenderShadowSoftness = gSavedSettings.getF32("RenderShadowSoftness");
    // </FS:AYAstorm>
    RenderShadowBlurSize = gSavedSettings.getF32("RenderShadowBlurSize");
    RenderSSAOScale = gSavedSettings.getF32("RenderSSAOScale");
    RenderSSAOMaxScale = gSavedSettings.getU32("RenderSSAOMaxScale");
    RenderSSAOFactor = gSavedSettings.getF32("RenderSSAOFactor");
    RenderSSAOEffect = gSavedSettings.getVector3("RenderSSAOEffect");
    RenderShadowOffsetError = gSavedSettings.getF32("RenderShadowOffsetError");
    RenderShadowBiasError = gSavedSettings.getF32("RenderShadowBiasError");
    RenderShadowOffset = gSavedSettings.getF32("RenderShadowOffset");
    RenderShadowBias = gSavedSettings.getF32("RenderShadowBias");
    RenderSpotShadowOffset = gSavedSettings.getF32("RenderSpotShadowOffset");
    RenderSpotShadowBias = gSavedSettings.getF32("RenderSpotShadowBias");
    RenderEdgeDepthCutoff = gSavedSettings.getF32("RenderEdgeDepthCutoff");
    RenderEdgeNormCutoff = gSavedSettings.getF32("RenderEdgeNormCutoff");
    RenderShadowGaussian = gSavedSettings.getVector3("RenderShadowGaussian");
    RenderShadowBlurDistFactor = gSavedSettings.getF32("RenderShadowBlurDistFactor");
    RenderDeferredAtmospheric = gSavedSettings.getBOOL("RenderDeferredAtmospheric");
    RenderHighlightFadeTime = gSavedSettings.getF32("RenderHighlightFadeTime");
    RenderFarClip = gSavedSettings.getF32("RenderFarClip");
    RenderShadowSplitExponent = gSavedSettings.getVector3("RenderShadowSplitExponent");
    RenderShadowErrorCutoff = gSavedSettings.getF32("RenderShadowErrorCutoff");
    RenderShadowFOVCutoff = gSavedSettings.getF32("RenderShadowFOVCutoff");
    CameraOffset = gSavedSettings.getBOOL("CameraOffset");
    CameraMaxCoF = gSavedSettings.getF32("CameraMaxCoF");
    CameraDoFResScale = gSavedSettings.getF32("CameraDoFResScale");
    RenderVignette = gSavedSettings.getVector3("FSRenderVignette"); // <FS:Beq/> redo the vignette

    RenderAutoHideSurfaceAreaLimit = gSavedSettings.getF32("RenderAutoHideSurfaceAreaLimit");
    RenderScreenSpaceReflections = gSavedSettings.getBOOL("RenderScreenSpaceReflections");
    RenderScreenSpaceReflectionIterations = gSavedSettings.getS32("RenderScreenSpaceReflectionIterations");
    RenderScreenSpaceReflectionRayStep = gSavedSettings.getF32("RenderScreenSpaceReflectionRayStep");
    RenderScreenSpaceReflectionDistanceBias = gSavedSettings.getF32("RenderScreenSpaceReflectionDistanceBias");
    RenderScreenSpaceReflectionDepthRejectBias = gSavedSettings.getF32("RenderScreenSpaceReflectionDepthRejectBias");
    RenderScreenSpaceReflectionAdaptiveStepMultiplier = gSavedSettings.getF32("RenderScreenSpaceReflectionAdaptiveStepMultiplier");
    RenderScreenSpaceReflectionGlossySamples = gSavedSettings.getS32("RenderScreenSpaceReflectionGlossySamples");
    RenderBufferVisualization = gSavedSettings.getS32("RenderBufferVisualization");
    RenderMirrors = gSavedSettings.getBOOL("RenderMirrors");
    RenderHeroProbeUpdateRate = gSavedSettings.getS32("RenderHeroProbeUpdateRate");
    RenderHeroProbeConservativeUpdateMultiplier = gSavedSettings.getS32("RenderHeroProbeConservativeUpdateMultiplier");
    RenderAvatarCloth = gSavedSettings.getBOOL("RenderAvatarCloth");

    LLPipelineFrameContext::getInstance().setReflectionProbesEnabled(LLFeatureManager::getInstance()->isFeatureAvailable("RenderReflectionsEnabled") && gSavedSettings.getBOOL("RenderReflectionsEnabled"));
    // <FS:Beq> [FIRE-35070] Instead of using the above we'll add a new static level variable to save some lookups. Making the above "work" with ProbeLevel will break everything.
    sReflectionProbeLevel = gSavedSettings.getS32("RenderReflectionProbeLevel");
    // <FS:Beq/>
    RenderSpotLight = nullptr;

    if (gNonInteractive)
    {
        LLVOAvatar::sMaxNonImpostors = 1;
        LLVOAvatar::updateImpostorRendering(LLVOAvatar::sMaxNonImpostors);
    }

    LLFontVertexBuffer::enableBufferCollection(gSavedSettings.getBOOL("CollectFontVertexBuffers"));
}

void LLPipeline::releaseGLBuffers()
{
    assertInitialized();

    mNoiseMap = nullptr;
    mTrueNoiseMap = nullptr;

    mSMAAAreaMap = nullptr;
    mSMAASearchMap = nullptr;
    mSMAASampleMap = nullptr;

    releaseLUTBuffers();

    mWaterDis.release();

    mSceneMap.release();

    mWaterExclusionMask.release();

    mPostPingMap.release();
    mPostPongMap.release();

    mFXAAMap.release();

    mUIScreen.release();

    mScenePresentRT[0].release();
    mScenePresentRT[1].release();

    mDownResMap.release();

    mBakeMap.release();

    for (U32 i = 0; i < 3; i++)
    {
        mGlow[i].release();
    }

    mHeroProbeManager.cleanup(); // release hero probes

    releaseScreenBuffers();

    gBumpImageList.destroyGL();
    LLVOAvatar::resetImpostors();
}

void LLPipeline::releaseLUTBuffers()
{
    mLightFunc = nullptr;

    mColorGradingLUT = nullptr;
    mColorGradingLUTName.clear();

    mPbrBrdfLut.release();

    mExposureMap.release();
    mLuminanceMap.release();
    mLastExposure.release();

}

void LLPipeline::releaseShadowBuffers()
{
    releaseSunShadowTargets();
    releaseSpotShadowTargets();
}

void LLPipeline::releaseScreenBuffers()
{
    getFrameRT()->screen.release();
    getFrameRT()->deferredScreen.release();
    getFrameRT()->deferredLight.release();

    mAuxillaryRT.screen.release();
    mAuxillaryRT.deferredScreen.release();
    mAuxillaryRT.deferredLight.release();

    mHeroProbeRT.screen.release();
    mHeroProbeRT.deferredScreen.release();
    mHeroProbeRT.deferredLight.release();

    mPreviewScreen.release(); // <FS:Beq/> dedicated preview target

    // <AYAstorm:r21.1> GPU self-rigged picker ID buffer
    mObjectIDBuffer.release();
    // </AYAstorm:r21.1>

    // <AYAstorm r30 P2> Velocity + T2x history buffers (Cinematic mode)
    mVelocityMap.release();
    mSMAAHistory.release();
    // </AYAstorm r30 P2>

    // <AYAstorm r30 P5 transparent-DoF L2-β> alpha-aware depth for cofF.glsl
    mAYAAlphaDepth.release();
    // </AYAstorm r30 P5 transparent-DoF L2-β>
    mSceneDepthCopy.release();

    // <AYAstorm r30 P5 transparent-DoF C-(a)> alpha BLEND color RT
    mAYAAlphaColor.release();
    // </AYAstorm r30 P5 transparent-DoF C-(a)>

    mForwardColor.release();
}

void LLPipeline::releaseSunShadowTarget(U32 index)
{
    llassert(index < 4);
    getFrameRT()->shadow[index].release();
}

void LLPipeline::releaseSunShadowTargets()
{
    for (U32 i = 0; i < 4; i++)
    {
        releaseSunShadowTarget(i);
    }
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

void LLPipeline::createGLBuffers()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    assertInitialized();


    GLuint resX = gViewerWindow->getWorldViewWidthRaw();
    GLuint resY = gViewerWindow->getWorldViewHeightRaw();

    // allocate screen space glow buffers
    // <FS:AYAstorm r30 BD full port Phase 3.4> Cinematic では BD default (10) に固定
    const U32 glow_res = llmax(1, llmin(512, 1 << gSavedSettings.getS32("RenderGlowResolutionPow")));
    // </FS:AYAstorm>
    const bool glow_hdr = gSavedSettings.getBOOL("RenderGlowHDR");
    const U32 glow_color_fmt = glow_hdr ? GL_RGBA16F : GL_RGBA;
    for (U32 i = 0; i < 3; i++)
    {
        mGlow[i].allocate(512, glow_res, glow_color_fmt);
    }

    allocateScreenBuffer(resX, resY);
    // Do not zero out getFrameRT() dimensions here. allocateScreenBuffer() above
    // already sets the correct dimensions. Zeroing them caused resizeShadowTexture()
    // to fail if called immediately after createGLBuffers (e.g., post graphics change).
    // getFrameRT()->width = 0;
    // getFrameRT()->height = 0;


    if (!mNoiseMap)
    {
        const U32 noiseRes = 128;
        LLVector3 noise[noiseRes*noiseRes];

        F32 scaler = gSavedSettings.getF32("RenderDeferredNoise")/100.f;
        for (U32 i = 0; i < noiseRes*noiseRes; ++i)
        {
            noise[i] = LLVector3(ll_frand()-0.5f, ll_frand()-0.5f, 0.f);
            noise[i].normVec();
            noise[i].mV[2] = ll_frand()*scaler+1.f-scaler/2.f;
        }

        mNoiseMap = new LLImageGL(false, false);
        mNoiseMap->setExplicitFormat(GL_RGB16F, GL_RGB, GL_FLOAT);
        mNoiseMap->setSize(noiseRes, noiseRes, 3);
        mNoiseMap->createGLTexture(0, (const U8*)noise, false);
        gGL.getTexUnit(0)->bind(mNoiseMap);
        mNoiseMap->setFilteringOption(LLTexUnit::TFO_POINT);
    }

    if (!mTrueNoiseMap)
    {
        const U32 noiseRes = 128;
        F32 noise[noiseRes*noiseRes*3];
        for (U32 i = 0; i < noiseRes*noiseRes*3; i++)
        {
            noise[i] = ll_frand()*2.0f-1.0f;
        }

        mTrueNoiseMap = new LLImageGL(false, false);
        mTrueNoiseMap->setExplicitFormat(GL_RGB16F, GL_RGB, GL_FLOAT);
        mTrueNoiseMap->setSize(noiseRes, noiseRes, 3);
        mTrueNoiseMap->createGLTexture(0, (const U8*)noise, false);
        gGL.getTexUnit(0)->bind(mTrueNoiseMap);
        mTrueNoiseMap->setFilteringOption(LLTexUnit::TFO_POINT);
    }

    if (!mSMAAAreaMap)
    {
        std::vector<U8> tempBuffer(AREATEX_SIZE);
        for (U32 y = 0; y < AREATEX_HEIGHT; y++)
        {
            U32 srcY = AREATEX_HEIGHT - 1 - y;
            // unsigned int srcY = y;
            memcpy(&tempBuffer[y * AREATEX_PITCH], areaTexBytes + srcY * AREATEX_PITCH, AREATEX_PITCH);
        }

        mSMAAAreaMap = new LLImageGL(false, false);
        mSMAAAreaMap->setExplicitFormat(GL_RG8, GL_RG, GL_UNSIGNED_BYTE);
        mSMAAAreaMap->setSize(AREATEX_WIDTH, AREATEX_HEIGHT, 2);
        mSMAAAreaMap->createGLTexture(0, (const U8*)tempBuffer.data(), false);
        gGL.getTexUnit(0)->bind(mSMAAAreaMap);
        mSMAAAreaMap->setFilteringOption(LLTexUnit::TFO_BILINEAR);
        mSMAAAreaMap->setAddressMode(LLTexUnit::TAM_CLAMP);
    }

    if (!mSMAASearchMap)
    {
        std::vector<U8> tempBuffer(SEARCHTEX_SIZE);
        for (U32 y = 0; y < SEARCHTEX_HEIGHT; y++)
        {
            U32 srcY = SEARCHTEX_HEIGHT - 1 - y;
            // unsigned int srcY = y;
            memcpy(&tempBuffer[y * SEARCHTEX_PITCH], searchTexBytes + srcY * SEARCHTEX_PITCH, SEARCHTEX_PITCH);
        }

        mSMAASearchMap = new LLImageGL(false, false);
        mSMAASearchMap->setExplicitFormat(GL_R8, GL_RED, GL_UNSIGNED_BYTE);
        mSMAASearchMap->setSize(SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 1);
        mSMAASearchMap->createGLTexture(0, (const U8*)tempBuffer.data(), false);
        gGL.getTexUnit(0)->bind(mSMAASearchMap);
        mSMAASearchMap->setFilteringOption(LLTexUnit::TFO_BILINEAR);
        mSMAASearchMap->setAddressMode(LLTexUnit::TAM_CLAMP);
    }

    if (!mSMAASampleMap)
    {
        LLPointer<LLImageRaw>               raw_image = new LLImageRaw;
        LLPointer<LLImagePNG>               png_image = new LLImagePNG;
        static LLCachedControl<std::string> sample_path(gSavedSettings, "SamplePath", "");
        if (gDirUtilp->fileExists(sample_path()) && png_image->load(sample_path()) && png_image->decode(raw_image, 0.0f))
        {
            U32 format = 0;
            switch (raw_image->getComponents())
            {
            case 1:
                format = GL_RED;
                break;
            case 2:
                format = GL_RG;
                break;
            case 3:
                format = GL_RGB;
                break;
            case 4:
                format = GL_RGBA;
                break;
            default:
                return;
            };
            mSMAASampleMap = new LLImageGL(false, false);
            mSMAASampleMap->setExplicitFormat(GL_RGB, format, GL_UNSIGNED_BYTE);
            mSMAASampleMap->setSize(raw_image->getWidth(), raw_image->getHeight(), raw_image->getComponents());
            mSMAASampleMap->createGLTexture(0, (const U8*)raw_image->getData(), false);
            gGL.getTexUnit(0)->bind(mSMAASampleMap);
            mSMAASampleMap->setFilteringOption(LLTexUnit::TFO_BILINEAR);
            mSMAASampleMap->setAddressMode(LLTexUnit::TAM_CLAMP);
        }
    }

    createLUTBuffers();

    gBumpImageList.restoreGL();
}

F32 lerpf(F32 a, F32 b, F32 w)
{
    return a + w * (b - a);
}

bool LLPipeline::loadColorGradingLUT(const std::string& filename)
{
    mColorGradingLUT = nullptr;
    mColorGradingLUTName.clear();

    if (filename.empty())
        return true;

    std::string path = filename;
    if (!gDirUtilp->fileExists(path))
        path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "luts", filename);

    llifstream file(path.c_str());
    if (!file.is_open())
    {
        LL_WARNS("LUT") << "Failed to open LUT file: " << path << LL_ENDL;
        return false;
    }

    int lut_size = 0;
    std::vector<float> lut_data;
    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#') continue;
        if (line.substr(0, 12) == "LUT_3D_SIZE ")
        {
            lut_size = std::stoi(line.substr(12));
            lut_data.reserve((size_t)lut_size * lut_size * lut_size * 3);
            continue;
        }
        if (lut_size > 0)
        {
            float r, g, b;
            if (sscanf(line.c_str(), "%f %f %f", &r, &g, &b) == 3)
            {
                lut_data.push_back(r);
                lut_data.push_back(g);
                lut_data.push_back(b);
            }
        }
    }

    if (lut_size <= 0 || (int)lut_data.size() != lut_size * lut_size * lut_size * 3)
    {
        LL_WARNS("LUT") << "Invalid LUT file (size=" << lut_size
            << " entries=" << lut_data.size() << "): " << path << LL_ENDL;
        return false;
    }

    mColorGradingLUT = new LLImageGL(lut_size, lut_size, 3, false);
    mColorGradingLUT->setTarget(GL_TEXTURE_3D, LLTexUnit::TT_TEXTURE_3D);
    mColorGradingLUT->syncVulkan3DImage(GL_RGB16F, GL_RGB, GL_FLOAT, lut_size, lut_size, lut_size, lut_data.data());

    mColorGradingLUTName = filename;
    LL_INFOS("LUT") << "Loaded color grading LUT: " << path << LL_ENDL;
    return true;
}

void LLPipeline::createLUTBuffers()
{
    if (!mLightFunc)
    {
        U32 lightResX = gSavedSettings.getU32("RenderSpecularResX");
        U32 lightResY = gSavedSettings.getU32("RenderSpecularResY");
        F32* ls = nullptr;
        try
        {
            ls = new F32[lightResX*lightResY];
        }
        catch (std::bad_alloc&)
        {
            LLError::LLUserWarningMsg::showOutOfMemory();
            // might be better to set the error into mFatalMessage and rethrow
            LL_ERRS() << "Bad memory allocation in createLUTBuffers! lightResX: "
                << lightResX << " lightResY: " << lightResY << LL_ENDL;
        }
        F32 specExp = gSavedSettings.getF32("RenderSpecularExponent");
        // Calculate the (normalized) blinn-phong specular lookup texture. (with a few tweaks)
        for (U32 y = 0; y < lightResY; ++y)
        {
            for (U32 x = 0; x < lightResX; ++x)
            {
                ls[y*lightResX+x] = 0;
                F32 sa = (F32) x/(lightResX-1);
                F32 spec = (F32) y/(lightResY-1);
                F32 n = spec * spec * specExp;

                // Nothing special here.  Just your typical blinn-phong term.
                spec = powf(sa, n);

                // Apply our normalization function.
                // Note: This is the full equation that applies the full normalization curve, not an approximation.
                // This is fine, given we only need to create our LUT once per buffer initialization.
                spec *= (((n + 2) * (n + 4)) / (8 * F_PI * (powf(2, -n/2) + n)));

                // Since we use R16F, we no longer have a dynamic range issue we need to work around here.
                // Though some older drivers may not like this, newer drivers shouldn't have this problem.
                ls[y*lightResX+x] = spec;
            }
        }

        U32 pix_format = GL_R16F;
#if LL_DARWIN
        if(!gGLManager.mIsApple)
        {
            // Need to work around limited precision with 10.6.8 and older drivers
            //
            pix_format = GL_R32F;
        }
#endif
        mLightFunc = new LLImageGL(false, false);
        mLightFunc->setExplicitFormat(pix_format, GL_RED, GL_FLOAT);
        mLightFunc->setSize(lightResX, lightResY, 1);
        mLightFunc->createGLTexture(0, (const U8*)ls, false);
        gGL.getTexUnit(0)->bind(mLightFunc);
        mLightFunc->setAddressMode(LLTexUnit::TAM_CLAMP);
        mLightFunc->setFilteringOption(LLTexUnit::TFO_TRILINEAR);

        delete [] ls;
    }

    mPbrBrdfLut.allocate(512, 512, GL_RG16F);

    if (LLVKLoader::isVulkanInitialized())
    {
        mBrdfLutDirty = true;
    }
    else
    {
        generateBrdfLut();
    }

    mExposureMap.allocate(1, 1, GL_R16F);
    mExposureMap.bindTarget();
    gGL.setClearColor(1, 1, 1, 0);
    mExposureMap.clear();
    gGL.setClearColor(0, 0, 0, 0);
    mExposureMap.flush();

    mLuminanceMap.allocate(256, 256, GL_R16F, false, LLTexUnit::TT_TEXTURE, LLTexUnit::TMG_AUTO);

    mLastExposure.allocate(1, 1, GL_R16F);
    mLastExposure.bindTarget();
    gGL.setClearColor(1, 1, 1, 0);
    mLastExposure.clear();
    gGL.setClearColor(0, 0, 0, 0);
    mLastExposure.flush();
}

void LLPipeline::generateBrdfLut()
{
    mPbrBrdfLut.bindTarget();

    if (gDeferredGenBrdfLutProgram.isComplete())
    {
        gDeferredGenBrdfLutProgram.bind();
        llassert_always(LLGLSLShader::sCurBoundShaderPtr != nullptr);


        gGL.begin(LLRender::TRIANGLE_STRIP);
        gGL.vertex2f(-1, -1);
        gGL.vertex2f(-1, 1);
        gGL.vertex2f(1, -1);
        gGL.vertex2f(1, 1);
        gGL.end();
        gGL.flush();
    }
    else
    {
        LL_WARNS("Brad") << gDeferredGenBrdfLutProgram.mName << " failed to load, cannot be used!" << LL_ENDL;
    }

    gDeferredGenBrdfLutProgram.unbind();
    mPbrBrdfLut.flush();
}

void LLPipeline::updateBrdfLut()
{
    if (!mBrdfLutDirty)
    {
        return;
    }

    if (LLVKLoader::isVulkanInitialized() && LLVKLoader::getCurrentCommandBuffer() == VK_NULL_HANDLE)
    {
        return;
    }

    if (!gDeferredGenBrdfLutProgram.isComplete())
    {
        return;
    }

    generateBrdfLut();
    mBrdfLutDirty = false;
}


void LLPipeline::restoreGL()
{
    assertInitialized();

    LLViewerShaderMgr::instance()->setShaders();

    for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
            iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
    {
        LLViewerRegion* region = *iter;
        for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
        {
            LLSpatialPartition* part = region->getSpatialPartition(i);
            if (part)
            {
                part->restoreGL();
        }
        }
    }
}

bool LLPipeline::shadersLoaded()
{
    return (assertInitialized() && mShadersLoaded);
}

bool LLPipeline::canUseWindLightShaders() const
{
    return true;
}

bool LLPipeline::canUseAntiAliasing() const
{
    return true;
}

void LLPipeline::unloadShaders()
{
    LLViewerShaderMgr::instance()->unloadShaders();
    mShadersLoaded = false;
}

void LLPipeline::assertInitializedDoError()
{
    LL_ERRS() << "LLPipeline used when uninitialized." << LL_ENDL;
}


void LLPipeline::enableShadows(const bool enable_shadows)
{
    //should probably do something here to wrangle shadows....
}

class LLOctreeDirtyTexture : public OctreeTraveler
{
public:
    const std::set<LLViewerFetchedTexture*>& mTextures;

    LLOctreeDirtyTexture(const std::set<LLViewerFetchedTexture*>& textures) : mTextures(textures) { }

    virtual void visit(const OctreeNode* node)
    {
        LLSpatialGroup* group = (LLSpatialGroup*) node->getListener(0);

        if (!group->hasState(LLSpatialGroup::GEOM_DIRTY) && !group->isEmpty())
        {
            for (LLSpatialGroup::draw_map_t::iterator i = group->mDrawMap.begin(); i != group->mDrawMap.end(); ++i)
            {
                for (LLSpatialGroup::drawmap_elem_t::iterator j = i->second.begin(); j != i->second.end(); ++j)
                {
                    LLDrawInfo* params = *j;
                    LLViewerFetchedTexture* tex = LLViewerTextureManager::staticCastToFetchedTexture(params->mTexture);
                    if (tex && mTextures.find(tex) != mTextures.end())
                    {
                        group->setState(LLSpatialGroup::GEOM_DIRTY);
                        ++LLVKLoader::gVkPerf.geo_dirty_site[9];
                    }
                }
            }
        }

        for (LLSpatialGroup::bridge_list_t::iterator i = group->mBridgeList.begin(); i != group->mBridgeList.end(); ++i)
        {
            LLSpatialBridge* bridge = *i;
            traverse(bridge->mOctree);
        }
    }
};

// Called when a texture changes # of channels (causes faces to move to alpha pool)
void LLPipeline::dirtyPoolObjectTextures(const std::set<LLViewerFetchedTexture*>& textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    assertInitialized();

    // *TODO: This is inefficient and causes frame spikes; need a better way to do this
    //        Most of the time is spent in dirty.traverse.

    for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
    {
        LLDrawPool *poolp = *iter;
        if (poolp->isFacePool())
        {
            ((LLFacePool*) poolp)->dirtyTextures(textures);
        }
    }

    LLOctreeDirtyTexture dirty(textures);
    for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
            iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
    {
        LLViewerRegion* region = *iter;
        for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
        {
            LLSpatialPartition* part = region->getSpatialPartition(i);
            if (part)
            {
                dirty.traverse(part->mOctree);
            }
        }
    }
}

LLDrawPool *LLPipeline::findPool(const U32 type, LLViewerTexture *tex0)
{
    assertInitialized();

    LLDrawPool *poolp = NULL;
    switch( type )
    {
    case LLDrawPool::POOL_SIMPLE:
        poolp = mSimplePool;
        break;

    case LLDrawPool::POOL_GRASS:
        poolp = mGrassPool;
        break;

    case LLDrawPool::POOL_ALPHA_MASK:
        poolp = mAlphaMaskPool;
        break;

    case LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK:
        poolp = mFullbrightAlphaMaskPool;
        break;

    case LLDrawPool::POOL_FULLBRIGHT:
        poolp = mFullbrightPool;
        break;

    case LLDrawPool::POOL_GLOW:
        poolp = mGlowPool;
        break;

    case LLDrawPool::POOL_TREE:
        poolp = get_if_there(mTreePools, (uintptr_t)tex0, (LLDrawPool*)0 );
        break;

    case LLDrawPool::POOL_TERRAIN:
        poolp = get_if_there(mTerrainPools, (uintptr_t)tex0, (LLDrawPool*)0 );
        break;

    case LLDrawPool::POOL_BUMP:
        poolp = mBumpPool;
        break;
    case LLDrawPool::POOL_MATERIALS:
        poolp = mMaterialsPool;
        break;
    case LLDrawPool::POOL_ALPHA_PRE_WATER:
        poolp = mAlphaPoolPreWater;
        break;
    case LLDrawPool::POOL_ALPHA_POST_WATER:
        poolp = mAlphaPoolPostWater;
        break;

    case LLDrawPool::POOL_AVATAR:
    case LLDrawPool::POOL_CONTROL_AV:
        break; // Do nothing

    case LLDrawPool::POOL_SKY:
        poolp = mSkyPool;
        break;

    case LLDrawPool::POOL_WATER:
        poolp = mWaterPool;
        break;

    case LLDrawPool::POOL_WL_SKY:
        poolp = mWLSkyPool;
        break;

    case LLDrawPool::POOL_GLTF_PBR:
        poolp = mPBROpaquePool;
        break;
    case LLDrawPool::POOL_GLTF_PBR_ALPHA_MASK:
        poolp = mPBRAlphaMaskPool;
        break;

    case LLDrawPool::POOL_WATEREXCLUSION:
        poolp = mWaterExclusionPool;
        break;

    default:
        llassert(0);
        LL_ERRS() << "Invalid Pool Type in  LLPipeline::findPool() type=" << type << LL_ENDL;
        break;
    }

    return poolp;
}


LLDrawPool *LLPipeline::getPool(const U32 type, LLViewerTexture *tex0)
{
    LLDrawPool *poolp = findPool(type, tex0);
    if (poolp)
    {
        return poolp;
    }

    LLDrawPool *new_poolp = LLDrawPool::createPool(type, tex0);
    addPool( new_poolp );

    return new_poolp;
}


LLDrawPool* LLPipeline::getPoolFromTE(const LLTextureEntry* te, LLViewerTexture* imagep)
{
    U32 type = getPoolTypeFromTE(te, imagep);
    return gPipeline.getPool(type, imagep);
}

U32 LLPipeline::getPoolTypeFromTE(const LLTextureEntry* te, LLViewerTexture* imagep)
{
    if (!te || !imagep)
    {
        return 0;
    }

    LLMaterial* mat = te->getMaterialParams().get();
    LLGLTFMaterial* gltf_mat = te->getGLTFRenderMaterial();

    bool color_alpha = te->getColor().mV[3] < 0.999f;
    bool alpha = color_alpha;
    if (imagep)
    {
        alpha = alpha || (imagep->getComponents() == 4 && imagep->getType() != LLViewerTexture::MEDIA_TEXTURE) || (imagep->getComponents() == 2);
    }

    if (alpha && mat)
    {
        switch (mat->getDiffuseAlphaMode())
        {
            case 1:
                alpha = true; // Material's alpha mode is set to blend.  Toss it into the alpha draw pool.
                break;
            case 0: //alpha mode set to none, never go to alpha pool
            case 3: //alpha mode set to emissive, never go to alpha pool
                alpha = color_alpha;
                break;
            default: //alpha mode set to "mask", go to alpha pool if fullbright
                alpha = color_alpha; // Material's alpha mode is set to none, mask, or emissive.  Toss it into the opaque material draw pool.
                break;
        }
    }

    if (alpha || (gltf_mat && gltf_mat->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_BLEND))
    {
        return LLDrawPool::POOL_ALPHA;
    }
    else if ((te->getBumpmap() || te->getShiny()) && (!mat || mat->getNormalID().isNull()))
    {
        return LLDrawPool::POOL_BUMP;
    }
    else if (gltf_mat)
    {
        return LLDrawPool::POOL_GLTF_PBR;
    }
    else if (mat && !alpha)
    {
        return LLDrawPool::POOL_MATERIALS;
    }
    else
    {
        return LLDrawPool::POOL_SIMPLE;
    }
}


void LLPipeline::addPool(LLDrawPool *new_poolp)
{
    assertInitialized();
    mPools.insert(new_poolp);
    addToQuickLookup( new_poolp );
}

void LLPipeline::allocDrawable(LLViewerObject *vobj)
{
    LLDrawable *drawable = new LLDrawable(vobj);
    vobj->mDrawable = drawable;

    //encompass completely sheared objects by taking
    //the most extreme point possible (<1,1,0.5>)
    drawable->setRadius(LLVector3(1,1,0.5f).scaleVec(vobj->getScale()).length());
    if (vobj->isOrphaned())
    {
        drawable->setState(LLDrawable::FORCE_INVISIBLE);
    }
    drawable->updateXform(true);
}


void LLPipeline::unlinkDrawable(LLDrawable *drawable)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    assertInitialized();

    LLPointer<LLDrawable> drawablep = drawable; // make sure this doesn't get deleted before we are done

    // Based on flags, remove the drawable from the queues that it's on.
    if (drawablep->isState(LLDrawable::ON_MOVE_LIST))
    {
        LLDrawable::drawable_vector_t::iterator iter = std::find(mMovedList.begin(), mMovedList.end(), drawablep);
        if (iter != mMovedList.end())
        {
            mMovedList.erase(iter);
        }
    }

    if (drawablep->getSpatialGroup())
    {
        if (!drawablep->getSpatialGroup()->getSpatialPartition()->remove(drawablep, drawablep->getSpatialGroup()))
        {
#ifdef LL_RELEASE_FOR_DOWNLOAD
            LL_WARNS() << "Couldn't remove object from spatial group!" << LL_ENDL;
#else
            LL_ERRS() << "Couldn't remove object from spatial group!" << LL_ENDL;
#endif
        }
    }

    mLights.erase(drawablep);

    for (light_set_t::iterator iter = mNearbyLights.begin();
                iter != mNearbyLights.end(); iter++)
    {
        if (iter->drawable == drawablep)
        {
            mNearbyLights.erase(iter);
            break;
        }
    }

    for (U32 i = 0; i < 2; ++i)
    {
        if (mShadowSpotLight[i] == drawablep)
        {
            mShadowSpotLight[i] = NULL;
        }

        if (mTargetShadowSpotLight[i] == drawablep)
        {
            mTargetShadowSpotLight[i] = NULL;
        }
    }
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

U32 LLPipeline::addObject(LLViewerObject *vobj)
{
    if (RenderDelayCreation)
    {
        mCreateQ.push_back(vobj);
    }
    else
    {
        createObject(vobj);
    }

    return 1;
}

void LLPipeline::createObjects(F32 max_dtime)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    LLTimer update_timer;

    while (!mCreateQ.empty() && update_timer.getElapsedTimeF32() < max_dtime)
    {
        LLViewerObject* vobj = mCreateQ.front();
        if (!vobj->isDead())
        {
            createObject(vobj);
        }
        mCreateQ.pop_front();
    }

    //for (LLViewerObject::vobj_list_t::iterator iter = mCreateQ.begin(); iter != mCreateQ.end(); ++iter)
    //{
    //  createObject(*iter);
    //}

    //mCreateQ.clear();
}

void LLPipeline::createObject(LLViewerObject* vobj)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LLDrawable* drawablep = vobj->mDrawable;

    if (!drawablep)
    {
        drawablep = vobj->createDrawable(this);
    }
    else
    {
        LL_ERRS() << "Redundant drawable creation!" << LL_ENDL;
    }

    llassert(drawablep);

    if (vobj->getParent())
    {
        vobj->setDrawableParent(((LLViewerObject*)vobj->getParent())->mDrawable); // LLPipeline::addObject 1
    }
    else
    {
        vobj->setDrawableParent(NULL); // LLPipeline::addObject 2
    }

    markRebuild(drawablep, LLDrawable::REBUILD_ALL);

    // <FS:Beq> FIRE-23122 BUG-225920 Remove broken RenderAnimateRes functionality.
    //if (drawablep->getVOVolume() && RenderAnimateRes)
    //{
    //  // fun animated res
    //  drawablep->updateXform(true);
    //  drawablep->clearState(LLDrawable::MOVE_UNDAMPED);
    //  drawablep->setScale(LLVector3(0,0,0));
    //  drawablep->makeActive();
    //}
}


void LLPipeline::resetFrameStats()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    assertInitialized();

    sCompiles        = 0;
    mNumVisibleFaces = 0;

    if (mOldRenderDebugMask != mRenderDebugMask)
    {
        gObjectList.clearDebugText();
        mOldRenderDebugMask = mRenderDebugMask;
    }
}

//external functions for asynchronous updating
void LLPipeline::updateMoveDampedAsync(LLDrawable* drawablep)
{
    LL_PROFILE_ZONE_SCOPED;
    if (FreezeTime)
    {
        return;
    }
    if (!drawablep)
    {
        LL_ERRS() << "updateMove called with NULL drawablep" << LL_ENDL;
        return;
    }
    if (drawablep->isState(LLDrawable::EARLY_MOVE))
    {
        return;
    }

    assertInitialized();

    // update drawable now
    drawablep->clearState(LLDrawable::MOVE_UNDAMPED); // force to DAMPED
    drawablep->updateMove(); // returns done
    drawablep->setState(LLDrawable::EARLY_MOVE); // flag says we already did an undamped move this frame
    // Put on move list so that EARLY_MOVE gets cleared
    if (!drawablep->isState(LLDrawable::ON_MOVE_LIST))
    {
        mMovedList.push_back(drawablep);
        drawablep->setState(LLDrawable::ON_MOVE_LIST);
    }
}

void LLPipeline::updateMoveNormalAsync(LLDrawable* drawablep)
{
    LL_PROFILE_ZONE_SCOPED;
    if (FreezeTime)
    {
        return;
    }
    if (!drawablep)
    {
        LL_ERRS() << "updateMove called with NULL drawablep" << LL_ENDL;
        return;
    }
    if (drawablep->isState(LLDrawable::EARLY_MOVE))
    {
        return;
    }

    assertInitialized();

    // update drawable now
    drawablep->setState(LLDrawable::MOVE_UNDAMPED); // force to UNDAMPED
    drawablep->updateMove();
    drawablep->setState(LLDrawable::EARLY_MOVE); // flag says we already did an undamped move this frame
    // Put on move list so that EARLY_MOVE gets cleared
    if (!drawablep->isState(LLDrawable::ON_MOVE_LIST))
    {
        mMovedList.push_back(drawablep);
        drawablep->setState(LLDrawable::ON_MOVE_LIST);
    }
}

void LLPipeline::updateMovedList(LLDrawable::drawable_vector_t& moved_list)
{
    LL_PROFILE_ZONE_SCOPED;
    for (LLDrawable::drawable_vector_t::iterator iter = moved_list.begin();
         iter != moved_list.end(); )
    {
        LLDrawable::drawable_vector_t::iterator curiter = iter++;
        LLDrawable *drawablep = *curiter;
        if (!drawablep)
        {
            iter = moved_list.erase(curiter);
            continue;
        }
        bool done = true;
        if (!drawablep->isDead() && (!drawablep->isState(LLDrawable::EARLY_MOVE)))
        {
            done = drawablep->updateMove();
        }
        drawablep->clearState(LLDrawable::EARLY_MOVE | LLDrawable::MOVE_UNDAMPED);
        if (done)
        {
            if (drawablep->isRoot() && !drawablep->isState(LLDrawable::ACTIVE))
            {
                drawablep->makeStatic();
            }
            drawablep->clearState(LLDrawable::ON_MOVE_LIST);
            if (drawablep->isState(LLDrawable::ANIMATED_CHILD))
            { //will likely not receive any future world matrix updates
                // -- this keeps attachments from getting stuck in space and falling off your avatar
                drawablep->clearState(LLDrawable::ANIMATED_CHILD);
                markRebuild(drawablep, LLDrawable::REBUILD_VOLUME);
                if (drawablep->getVObj())
                {
                    ++LLVKLoader::gVkPerf.geo_dirty_site[1];
                    drawablep->getVObj()->dirtySpatialGroup();
                }
            }
            iter = moved_list.erase(curiter);
        }
    }
}

void LLPipeline::updateMove()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    if (FreezeTime)
    {
        return;
    }

    assertInitialized();

    for (LLDrawable::drawable_set_t::iterator iter = mRetexturedList.begin();
            iter != mRetexturedList.end(); ++iter)
    {
        LLDrawable* drawablep = *iter;
        if (drawablep && !drawablep->isDead())
        {
            drawablep->updateTexture();
        }
    }
    mRetexturedList.clear();

    updateMovedList(mMovedList);

    //balance octrees
    for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
        iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
    {
        LLViewerRegion* region = *iter;
        for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
        {
            LLSpatialPartition* part = region->getSpatialPartition(i);
            if (part)
            {
                part->mOctree->balance();
            }
        }

        //balance the VO Cache tree
        LLVOCachePartition* vo_part = region->getVOCachePartition();
        if(vo_part)
        {
            vo_part->mOctree->balance();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// Culling and occlusion testing
/////////////////////////////////////////////////////////////////////////////

F32 LLPipeline::calcPixelArea(LLVector3 center, LLVector3 size, LLCamera &camera)
{
    llassert(!gCubeSnapshot); // shouldn't be doing ANY of this during cube snap shots
    LLVector3 lookAt = center - camera.getOrigin();
    F32 dist = lookAt.length();

    //ramp down distance for nearby objects
    //shrink dist by dist/16.
    if (dist < 16.f)
    {
        dist /= 16.f;
        dist *= dist;
        dist *= 16.f;
    }

    //get area of circle around node
    F32 app_angle = atanf(size.length()/dist);
    F32 radius = app_angle*LLDrawable::sCurPixelAngle;
    return radius*radius * F_PI;
}

F32 LLPipeline::calcPixelArea(const LLVector4a& center, const LLVector4a& size, LLCamera &camera)
{
    LLVector4a origin;
    origin.load3(camera.getOrigin().mV);

    LLVector4a lookAt;
    lookAt.setSub(center, origin);
    F32 dist = lookAt.getLength3().getF32();

    //ramp down distance for nearby objects
    //shrink dist by dist/16.
    if (dist < 16.f)
    {
        dist /= 16.f;
        dist *= dist;
        dist *= 16.f;
    }

    //get area of circle around node
    F32 app_angle = atanf(size.getLength3().getF32() / dist);
    F32 radius = app_angle * LLDrawable::sCurPixelAngle;
    return radius * radius * F_PI;
}

void LLPipeline::grabReferences(LLCullResult& result)
{
    LLPipelineFrameContext::getInstance().setCullResult(&result);
}

void LLPipeline::clearReferences()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LLPipelineFrameContext::getInstance().setCullResult(nullptr);
    mGroupSaveQ1.clear();
}

void check_references(LLSpatialGroup* group, LLDrawable* drawable)
{
    for (LLSpatialGroup::element_iter i = group->getDataBegin(); i != group->getDataEnd(); ++i)
    {
        LLDrawable* drawablep = (LLDrawable*)(*i)->getDrawable();
        if (drawable == drawablep)
        {
            LL_ERRS() << "LLDrawable deleted while actively reference by LLPipeline." << LL_ENDL;
        }
    }
}

void check_references(LLDrawable* drawable, LLFace* face)
{
    for (S32 i = 0; i < drawable->getNumFaces(); ++i)
    {
        if (drawable->getFace(i) == face)
        {
            LL_ERRS() << "LLFace deleted while actively referenced by LLPipeline." << LL_ENDL;
        }
    }
}

void check_references(LLSpatialGroup* group, LLFace* face)
{
    for (LLSpatialGroup::element_iter i = group->getDataBegin(); i != group->getDataEnd(); ++i)
    {
        LLDrawable* drawable = (LLDrawable*)(*i)->getDrawable();
        if(drawable)
        {
        check_references(drawable, face);
    }
}
}

void LLPipeline::checkReferences(LLFace* face)
{
#if 0
    if (getFrameCull())
    {
        for (LLCullResult::sg_iterator iter = getFrameCull()->beginVisibleGroups(); iter != getFrameCull()->endVisibleGroups(); ++iter)
        {
            LLSpatialGroup* group = *iter;
            check_references(group, face);
        }

        for (LLCullResult::sg_iterator iter = getFrameCull()->beginAlphaGroups(); iter != getFrameCull()->endAlphaGroups(); ++iter)
        {
            LLSpatialGroup* group = *iter;
            check_references(group, face);
        }

        for (LLCullResult::sg_iterator iter = getFrameCull()->beginDrawableGroups(); iter != getFrameCull()->endDrawableGroups(); ++iter)
        {
            LLSpatialGroup* group = *iter;
            check_references(group, face);
        }

        for (LLCullResult::drawable_iterator iter = getFrameCull()->beginVisibleList(); iter != getFrameCull()->endVisibleList(); ++iter)
        {
            LLDrawable* drawable = *iter;
            check_references(drawable, face);
        }
    }
#endif
}

void LLPipeline::checkReferences(LLDrawable* drawable)
{
#if 0
    if (getFrameCull())
    {
        for (LLCullResult::sg_iterator iter = getFrameCull()->beginVisibleGroups(); iter != getFrameCull()->endVisibleGroups(); ++iter)
        {
            LLSpatialGroup* group = *iter;
            check_references(group, drawable);
        }

        for (LLCullResult::sg_iterator iter = getFrameCull()->beginAlphaGroups(); iter != getFrameCull()->endAlphaGroups(); ++iter)
        {
            LLSpatialGroup* group = *iter;
            check_references(group, drawable);
        }

        for (LLCullResult::sg_iterator iter = getFrameCull()->beginDrawableGroups(); iter != getFrameCull()->endDrawableGroups(); ++iter)
        {
            LLSpatialGroup* group = *iter;
            check_references(group, drawable);
        }

        for (LLCullResult::drawable_iterator iter = getFrameCull()->beginVisibleList(); iter != getFrameCull()->endVisibleList(); ++iter)
        {
            if (drawable == *iter)
            {
                LL_ERRS() << "LLDrawable deleted while actively referenced by LLPipeline." << LL_ENDL;
            }
        }
    }
#endif
}

void check_references(LLSpatialGroup* group, LLDrawInfo* draw_info)
{
    for (LLSpatialGroup::draw_map_t::iterator i = group->mDrawMap.begin(); i != group->mDrawMap.end(); ++i)
    {
        LLSpatialGroup::drawmap_elem_t& draw_vec = i->second;
        for (LLSpatialGroup::drawmap_elem_t::iterator j = draw_vec.begin(); j != draw_vec.end(); ++j)
        {
            LLDrawInfo* params = *j;
            if (params == draw_info)
            {
                LL_ERRS() << "LLDrawInfo deleted while actively referenced by LLPipeline." << LL_ENDL;
            }
        }
    }
}


void LLPipeline::checkReferences(LLDrawInfo* draw_info)
{
#if 0
    if (getFrameCull())
    {
        for (LLCullResult::sg_iterator iter = getFrameCull()->beginVisibleGroups(); iter != getFrameCull()->endVisibleGroups(); ++iter)
        {
            LLSpatialGroup* group = *iter;
            check_references(group, draw_info);
        }

        for (LLCullResult::sg_iterator iter = getFrameCull()->beginAlphaGroups(); iter != getFrameCull()->endAlphaGroups(); ++iter)
        {
            LLSpatialGroup* group = *iter;
            check_references(group, draw_info);
        }

        for (LLCullResult::sg_iterator iter = getFrameCull()->beginDrawableGroups(); iter != getFrameCull()->endDrawableGroups(); ++iter)
        {
            LLSpatialGroup* group = *iter;
            check_references(group, draw_info);
        }
    }
#endif
}

void LLPipeline::checkReferences(LLSpatialGroup* group)
{
#if CHECK_PIPELINE_REFERENCES
    if (getFrameCull())
    {
        for (LLCullResult::sg_iterator iter = getFrameCull()->beginVisibleGroups(); iter != getFrameCull()->endVisibleGroups(); ++iter)
        {
            if (group == *iter)
            {
                LL_ERRS() << "LLSpatialGroup deleted while actively referenced by LLPipeline." << LL_ENDL;
            }
        }

        for (LLCullResult::sg_iterator iter = getFrameCull()->beginAlphaGroups(); iter != getFrameCull()->endAlphaGroups(); ++iter)
        {
            if (group == *iter)
            {
                LL_ERRS() << "LLSpatialGroup deleted while actively referenced by LLPipeline." << LL_ENDL;
            }
        }

        for (LLCullResult::sg_iterator iter = getFrameCull()->beginDrawableGroups(); iter != getFrameCull()->endDrawableGroups(); ++iter)
        {
            if (group == *iter)
            {
                LL_ERRS() << "LLSpatialGroup deleted while actively referenced by LLPipeline." << LL_ENDL;
            }
        }
    }
#endif
}


bool LLPipeline::visibleObjectsInFrustum(LLCamera& camera)
{
    for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
            iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
    {
        LLViewerRegion* region = *iter;

        for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
        {
            LLSpatialPartition* part = region->getSpatialPartition(i);
            if (part)
            {
                if (hasRenderType(part->mDrawableType))
                {
                    if (part->visibleObjectsInFrustum(camera))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool LLPipeline::getVisibleExtents(LLCamera& camera, LLVector3& min, LLVector3& max)
{
    const F32 X = 65536.f;

    min = LLVector3(X,X,X);
    max = LLVector3(-X,-X,-X);

    LLPipelineFrameContext::ScopedCameraID camera_scope(LLViewerCamera::CAMERA_WORLD);

    bool res = true;

    for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
            iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
    {
        LLViewerRegion* region = *iter;

        for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
        {
            LLSpatialPartition* part = region->getSpatialPartition(i);
            if (part)
            {
                if (hasRenderType(part->mDrawableType))
                {
                    if (!part->getVisibleExtents(camera, min, max))
                    {
                        res = false;
                    }
                }
            }
        }
    }

    return res;
}

static LLTrace::BlockTimerStatHandle FTM_CULL("Object Culling");

bool LLPipeline::isWaterClip()
{
    // We always pretend that we're not clipping water when rendering mirrors.
    return (gPipeline.mHeroProbeManager.isMirrorPass()) ? false : (!sRenderTransparentWater || gCubeSnapshot) && !isFrameHUDPass();
}

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
// </FS:AYAstorm>

void LLPipeline::updateCull(LLCamera& camera, LLCullResult& result, bool hud_attachments)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE; //LL_RECORD_BLOCK_TIME(FTM_CULL);
    LL_PROFILE_GPU_ZONE("updateCull"); // should always be zero GPU time, but drop a timer to flush stuff out

    bool water_clip = isWaterClip();

    if (water_clip)
    {

        LLVector3 pnorm;

        F32 water_height = LLEnvironment::instance().getWaterHeight();

        if (isFrameUnderWaterRendering())
        {
            //camera is below water, cull above water
            pnorm.setVec(0, 0, 1);
        }
        else
        {
            //camera is above water, cull below water
            pnorm = LLVector3(0, 0, -1);
        }

        LLPlane plane;
        plane.setVec(LLVector3(0, 0, water_height), pnorm);

        camera.setUserClipPlane(plane);
    }
    else
    {
        camera.disableUserClipPlane();
    }

    grabReferences(result);

    getFrameCull()->clear();

    for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
            iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
    {
        LLViewerRegion* region = *iter;

        for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
        {
            LLSpatialPartition* part = region->getSpatialPartition(i);
            if (part)
            {
                if (!hud_attachments ? LLViewerRegion::PARTITION_BRIDGE == i || hasRenderType(part->mDrawableType) : hasRenderType(part->mDrawableType))
                {
                    part->cull(camera);
                }
            }
        }

        //scan the VO Cache tree
        LLVOCachePartition* vo_part = region->getVOCachePartition();
        if(vo_part)
        {
            // <FS:Beq> Fix area search again
            //vo_part->cull(camera, sUseOcclusion > 0);
            vo_part->cull(camera, sUseOcclusion > 0 && !gAgent.getFSAreaSearchActive());
        }
    }

    if (hasRenderType(LLPipeline::RENDER_TYPE_SKY) &&
        gSky.mVOSkyp.notNull() &&
        gSky.mVOSkyp->mDrawable.notNull())
    {
        gSky.mVOSkyp->mDrawable->setVisible(camera);
        getFrameCull()->pushDrawable(gSky.mVOSkyp->mDrawable);
        gSky.updateCull();
    }

    if (hasRenderType(LLPipeline::RENDER_TYPE_WL_SKY) &&
        gPipeline.canUseWindLightShaders() &&
        gSky.mVOWLSkyp.notNull() &&
        gSky.mVOWLSkyp->mDrawable.notNull())
    {
        gSky.mVOWLSkyp->mDrawable->setVisible(camera);
        getFrameCull()->pushDrawable(gSky.mVOWLSkyp->mDrawable);
    }
}

void LLPipeline::markNotCulled(LLSpatialGroup* group, LLCamera& camera)
{
    if (group->isEmpty())
    {
        return;
    }

    group->setVisible();

    if (LLViewerCamera::getCurCameraID() == LLViewerCamera::CAMERA_WORLD && !gCubeSnapshot)
    {
        group->updateDistance(camera);
    }

    assertInitialized();

    if (!group->getSpatialPartition()->mRenderByGroup)
    { //render by drawable
        getFrameCull()->pushDrawableGroup(group);
    }
    else
    {   //render by group
        getFrameCull()->pushVisibleGroup(group);
    }

    if (group->needsUpdate() ||
        group->getVisible(LLViewerCamera::getCurCameraID()) < LLDrawable::getCurrentFrame() - 1)
    {
        // include this group in occlusion groups, not because it is an occluder, but because we want to run
        // an occlusion query to find out if it's an occluder
        markOccluder(group);
    }
    mNumVisibleNodes++;
}

void LLPipeline::markOccluder(LLSpatialGroup* group)
{
    if (sUseOcclusion > 1 && group && !group->isOcclusionQueuedThisFrame((U32)gFrameCount))
    {
        LLSpatialGroup* parent = group->getParent();

        if (!parent || !parent->isOcclusionState(LLSpatialGroup::OCCLUDED))
        { //only mark top most occluders as active occlusion
            getFrameCull()->pushOcclusionGroup(group);
            group->setOcclusionQueuedFrame((U32)gFrameCount);

            if (parent &&
                !parent->isOcclusionQueuedThisFrame((U32)gFrameCount) &&
                parent->getElementCount() == 0 &&
                parent->needsUpdate())
            {
                getFrameCull()->pushOcclusionGroup(group);
                parent->setOcclusionQueuedFrame((U32)gFrameCount);
            }
        }
    }
}

void LLPipeline::doOcclusion(LLCamera& camera)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LL_PROFILE_GPU_ZONE("doOcclusion");
    LLVKLoader::gpuCheckpoint("doOcclusion");
    llassert(!gCubeSnapshot);
    LLVKLoader::VkPerfPassScope perf_pass_scope(2);

    if (isFrameReflectionProbesEnabled() && sUseOcclusion > 1 && !isFrameShadowPass() && !gCubeSnapshot)
    {
        gGL.setColorMask(false, false);
        LLGLDepthTest depth(GL_TRUE, GL_FALSE);
        LLGLDisable cull(GL_CULL_FACE);

        gOcclusionCubeProgram.bind();

        if (mCubeVB.isNull())
        { //cube VB will be used for issuing occlusion queries
            mCubeVB = ll_create_cube_vb(LLVertexBuffer::MAP_VERTEX);
        }
        mCubeVB->setBuffer();

        mReflectionMapManager.doOcclusion();
        mHeroProbeManager.doOcclusion();
        gOcclusionCubeProgram.unbind();

        gGL.setColorMask(true, true);
    }

    if (LLPipeline::sUseOcclusion > 1 &&
        (getFrameCull()->hasOcclusionGroups() || LLVOCachePartition::sNeedsOcclusionCheck))
    {
        LLVertexBuffer::unbind();

        gGL.setColorMask(false, false);

        LLGLDisable blend(GL_BLEND);
        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
        LLGLDepthTest depth(GL_TRUE, GL_FALSE);

        LLGLDisable cull(GL_CULL_FACE);

        gOcclusionCubeProgram.bind();

        if (mCubeVB.isNull())
        { //cube VB will be used for issuing occlusion queries
            mCubeVB = ll_create_cube_vb(LLVertexBuffer::MAP_VERTEX);
        }
        mCubeVB->setBuffer();

        {
            static U32  s_sentinel_q       = 0;
            static bool s_sentinel_pending = false;

            uint64_t sent_samples = 0;
            bool     sent_avail   = false;
            if (s_sentinel_pending && s_sentinel_q)
            {
                LLVKLoader::getOcclusionQueryResultVk(s_sentinel_q, sent_avail, sent_samples);
            }

            if (!s_sentinel_pending || sent_avail)
            {
                if (sent_avail)
                {
                    if (sent_samples == 0)
                    {
                        static U32 s_sentinel_zero_count = 0;
                        ++s_sentinel_zero_count;
                        if ((s_sentinel_zero_count & (s_sentinel_zero_count - 1)) == 0)
                        {
                            LL_WARNS("RenderDrop") << "occlusion sentinel returned 0 samples"
                                                   << " (query/pass machinery suspect, count="
                                                   << s_sentinel_zero_count << ")" << LL_ENDL;
                        }
                    }
                }

                LLVKLoader::releaseOcclusionQueryVk(s_sentinel_q);
                s_sentinel_q       = LLVKLoader::acquireOcclusionQueryVk();
                s_sentinel_pending = false;
                if (s_sentinel_q != 0)
                {
                    LLVKLoader::cmdBeginOcclusionQueryVk(LLVKLoader::getCurrentCommandBuffer(), s_sentinel_q);

                    LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
                    if (LLVKLoader::isVulkanInitialized() && shader != nullptr
                        && shader->mVkPipelineLayout != VK_NULL_HANDLE)
                    {
                        VkCommandBuffer pc_cmd = LLVKLoader::getCurrentCommandBuffer();
                        if (pc_cmd != VK_NULL_HANDLE)
                        {
                            LLVKLoader::OcclusionCube_PushConstant pc_data = {};
                            const LLVector3& o = camera.getOrigin();
                            pc_data.box_center[0] = o.mV[0];
                            pc_data.box_center[1] = o.mV[1];
                            pc_data.box_center[2] = o.mV[2];
                            pc_data.box_size[0]   = 64.f;
                            pc_data.box_size[1]   = 64.f;
                            pc_data.box_size[2]   = 64.f;
                            vkCmdPushConstants(pc_cmd, shader->mVkPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                                               LLVkUboReg::PC_OFF_BOX_CENTER, sizeof(pc_data), &pc_data);
                        }
                    }

                    {
                        LLGLDepthTest sentinel_depth(GL_FALSE, GL_FALSE);
                        mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, 0);
                        mCubeVB->drawRange(LLRender::TRIANGLE_FAN, 0, 7, 8, 56);
                    }

                    LLVKLoader::cmdEndOcclusionQueryVk(LLVKLoader::getCurrentCommandBuffer(), s_sentinel_q);
                    s_sentinel_pending = true;
                }
            }
        }

        for (LLCullResult::sg_iterator iter = getFrameCull()->beginOcclusionGroups(); iter != getFrameCull()->endOcclusionGroups(); ++iter)
        {
            LLSpatialGroup* group = *iter;
            if (!group->isDead())
            {
                group->doOcclusion(&camera);
            }
        }

        //apply occlusion culling to object cache tree
        for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
            iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
        {
            LLVOCachePartition* vo_part = (*iter)->getVOCachePartition();
            if(vo_part)
            {
                vo_part->processOccluders(&camera);
            }
        }

        gGL.setColorMask(true, true);
    }
}

bool LLPipeline::updateDrawableGeom(LLDrawable* drawablep)
{
    bool update_complete = drawablep->updateGeometry();
    if (update_complete && assertInitialized())
    {
        drawablep->setState(LLDrawable::BUILT);
    }
    return update_complete;
}

void LLPipeline::updateGL()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    {
        while (!LLGLUpdate::sGLQ.empty())
        {
            LLGLUpdate* glu = LLGLUpdate::sGLQ.front();
            glu->updateGL();
            glu->mInQ = false;
            LLGLUpdate::sGLQ.pop_front();
        }
    }
}

void LLPipeline::clearRebuildGroups()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LLSpatialGroup::sg_vector_t hudGroups;

    mGroupQ1Locked = true;
    // Iterate through all drawables on the priority build queue,
    for (LLSpatialGroup::sg_vector_t::iterator iter = mGroupQ1.begin();
         iter != mGroupQ1.end(); ++iter)
    {
        LLSpatialGroup* group = *iter;

        if (!group || group->isDead())
        {
            continue;
        }
        // If the group contains HUD objects, save the group
        if (group->isHUDGroup())
        {
            hudGroups.push_back(group);
        }
        // Else, no HUD objects so clear the build state
        else
        {
            group->clearState(LLSpatialGroup::IN_BUILD_Q1);
        }
    }

    // Clear the group
    mGroupQ1.clear();

    // Copy the saved HUD groups back in
    mGroupQ1.assign(hudGroups.begin(), hudGroups.end());
    mGroupQ1Locked = false;
}

void LLPipeline::clearRebuildDrawables()
{
    // Clear all drawables on the priority build queue,
    for (LLDrawable::drawable_list_t::iterator iter = mBuildQ1.begin();
         iter != mBuildQ1.end(); ++iter)
    {
        LLDrawable* drawablep = *iter;
        if (drawablep && !drawablep->isDead())
        {
            drawablep->clearState(LLDrawable::IN_REBUILD_Q);
        }
    }
    mBuildQ1.clear();

    //clear all moving bridges
    for (LLDrawable::drawable_vector_t::iterator iter = mMovedBridge.begin();
         iter != mMovedBridge.end(); ++iter)
    {
        LLDrawable *drawablep = *iter;
        drawablep->clearState(LLDrawable::EARLY_MOVE | LLDrawable::MOVE_UNDAMPED | LLDrawable::ON_MOVE_LIST | LLDrawable::ANIMATED_CHILD);
    }
    mMovedBridge.clear();

    //clear all moving drawables
    for (LLDrawable::drawable_vector_t::iterator iter = mMovedList.begin();
         iter != mMovedList.end(); ++iter)
    {
        LLDrawable *drawablep = *iter;
        drawablep->clearState(LLDrawable::EARLY_MOVE | LLDrawable::MOVE_UNDAMPED | LLDrawable::ON_MOVE_LIST | LLDrawable::ANIMATED_CHILD);
    }
    mMovedList.clear();

    for (LLDrawable::drawable_vector_t::iterator iter = mShiftList.begin();
        iter != mShiftList.end(); ++iter)
    {
        LLDrawable *drawablep = *iter;
        drawablep->clearState(LLDrawable::EARLY_MOVE | LLDrawable::MOVE_UNDAMPED | LLDrawable::ON_MOVE_LIST | LLDrawable::ANIMATED_CHILD | LLDrawable::ON_SHIFT_LIST);
    }
    mShiftList.clear();
}

void LLPipeline::rebuildPriorityGroups()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LL_PROFILE_GPU_ZONE("rebuildPriorityGroups");

    LLTimer update_timer;
    assertInitialized();

    gMeshRepo.notifyLoadedMeshes();

    mGroupQ1Locked = true;
    LLSpatialGroup::sg_vector_t deferred;
    // Iterate through all drawables on the priority build queue,
    for (LLSpatialGroup::sg_vector_t::iterator iter = mGroupQ1.begin();
         iter != mGroupQ1.end(); ++iter)
    {
        LLSpatialGroup* group = *iter;
        group->rebuildGeom();
        if (!group->isDead()
            && !group->mVkGeoInflight
            && group->hasState(LLSpatialGroup::GEOM_DIRTY | LLSpatialGroup::ALPHA_DIRTY)
            && (group->mVkRebuildRet == 2 || group->mVkRebuildRet == 4))
        {
            deferred.push_back(group);
            continue;
        }
        group->clearState(LLSpatialGroup::IN_BUILD_Q1);
    }

    mGroupSaveQ1 = mGroupQ1;
    mGroupQ1.swap(deferred);
    mGroupQ1Locked = false;

}

void LLPipeline::updateGeom(F32 max_dtime)
{
    LLTimer update_timer;
    LLPointer<LLDrawable> drawablep;

    LL_RECORD_BLOCK_TIME(FTM_GEO_UPDATE);
    if (gCubeSnapshot)
    {
        return;
    }

    assertInitialized();

    // notify various object types to reset internal cost metrics, etc.
    // for now, only LLVOVolume does this to throttle LOD changes
    LLVOVolume::preUpdateGeom();

    // Iterate through all drawables on the priority build queue,
    for (LLDrawable::drawable_list_t::iterator iter = mBuildQ1.begin();
         iter != mBuildQ1.end();)
    {
        LLDrawable::drawable_list_t::iterator curiter = iter++;
        LLDrawable* drawablep = *curiter;
        if (drawablep && !drawablep->isDead())
        {
            if (drawablep->isUnload())
            {
                drawablep->unload();
                drawablep->clearState(LLDrawable::FOR_UNLOAD);
            }

            if (updateDrawableGeom(drawablep))
            {
                drawablep->clearState(LLDrawable::IN_REBUILD_Q);
                mBuildQ1.erase(curiter);
            }
        }
        else
        {
            mBuildQ1.erase(curiter);
        }
    }

    updateMovedList(mMovedBridge);
}

// <FS:AYA> [ParcelHide]
// Parse a parcel description for the [parcelhide:{key:value}{key:value}...]
// tag. The legacy [AYAstorm:...] prefix is accepted as a permanent alias so
// already-published parcel descriptions keep working without re-edit.
// See docs/ayastorm-render-hide-outside-parcel.md section 7.1 for the spec.
//
// Behavior summary:
//   - Tag absent: ParcelTagOverride{active=false}.
//   - Tag present, hideoutside missing or true (default): active=true with
//     keepavatars / keepownobject parsed (each defaulting to false).
//   - Tag present with hideoutside:false: tag is treated as a temporary
//     no-op, returning active=false (Phase A behavior applies).
//   - Prefix is matched case-sensitively. Keys are lowercase. Values
//     "true"/"false" are case-insensitive.
LLPipeline::ParcelTagOverride LLPipeline::parseParcelHideTag(const std::string& desc)
{
    ParcelTagOverride out;

    static const std::string PREFIX     = "[parcelhide:";
    static const std::string PREFIX_OLD = "[AYAstorm:"; // r4 and earlier
    std::string::size_type start = desc.find(PREFIX);
    std::string::size_type prefix_len = PREFIX.size();
    if (start == std::string::npos)
    {
        start = desc.find(PREFIX_OLD);
        prefix_len = PREFIX_OLD.size();
    }
    if (start == std::string::npos)
    {
        return out;
    }
    const std::string::size_type body_begin = start + prefix_len;
    const std::string::size_type end = desc.find(']', body_begin);
    if (end == std::string::npos)
    {
        return out;
    }
    const std::string body = desc.substr(body_begin, end - body_begin);

    bool hideoutside = true; // default when tag is present
    bool keepavatars = false;
    bool keepown = false;

    // Walk the body looking for {key:value} pairs. Anything outside braces
    // (whitespace, stray separators) is ignored.
    std::string::size_type i = 0;
    while (i < body.size())
    {
        const std::string::size_type ob = body.find('{', i);
        if (ob == std::string::npos) break;
        const std::string::size_type cb = body.find('}', ob + 1);
        if (cb == std::string::npos) break;
        const std::string pair = body.substr(ob + 1, cb - ob - 1);
        i = cb + 1;

        const std::string::size_type colon = pair.find(':');
        if (colon == std::string::npos) continue;

        std::string key = pair.substr(0, colon);
        std::string val = pair.substr(colon + 1);

        // Trim ASCII whitespace from key and val
        auto trim = [](std::string& s) {
            const std::string ws = " \t\r\n";
            std::string::size_type a = s.find_first_not_of(ws);
            std::string::size_type b = s.find_last_not_of(ws);
            if (a == std::string::npos) { s.clear(); return; }
            s = s.substr(a, b - a + 1);
        };
        trim(key);
        trim(val);

        // Lowercase the value so "True" / "FALSE" both work.
        for (char& c : val) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }

        // Non-boolean keys handled before the bool parse.
        if (key == "altitude")
        {
            // val: "min-max" or "min-max,min-max,..." (both ends required, integers or floats).
            // Hyphen is the range separator; comma separates multiple ranges.
            std::vector<std::pair<F32, F32>> ranges;
            std::string::size_type p = 0;
            while (p <= val.size())
            {
                std::string::size_type comma = val.find(',', p);
                std::string token = (comma == std::string::npos)
                    ? val.substr(p) : val.substr(p, comma - p);
                std::string::size_type dash = token.find('-');
                if (dash != std::string::npos && dash > 0 && dash + 1 < token.size())
                {
                    try
                    {
                        F32 lo = std::stof(token.substr(0, dash));
                        F32 hi = std::stof(token.substr(dash + 1));
                        if (lo <= hi)
                        {
                            ranges.emplace_back(lo, hi);
                        }
                    }
                    catch (...) { /* malformed token silently dropped */ }
                }
                if (comma == std::string::npos) break;
                p = comma + 1;
            }
            out.altRanges = std::move(ranges);
            continue;
        }

        bool bval;
        if (val == "true") bval = true;
        else if (val == "false") bval = false;
        else continue;

        if (key == "hideoutside") hideoutside = bval;
        else if (key == "keepavatars") keepavatars = bval;
        else if (key == "keepownobject") keepown = bval;
        // unknown keys are silently ignored
    }

    if (!hideoutside)
    {
        // Owner has temporarily disabled the tag — Phase A behavior applies.
        return out; // active=false
    }

    out.active = true;
    out.keepAvatars = keepavatars;
    out.keepOwn = keepown;
    return out;
}

// Returns true if the given drawable should be hidden because it is outside
// the agent's current parcel. Callers should additionally gate on
// (sParcelHideEnabled || sParcelOwnerTagActive) before invoking.
// Spatial bridges (e.g. avatar attachment hierarchies) are never hidden here;
// they are handled at the wrapped object level.
//
// When the agent parcel description carries a [parcelhide:...] tag
// (sParcelOwnerTagActive==true), the keepavatars / keepownobject decisions use
// the tag-supplied values instead of the visitor's settings. This lets the
// parcel owner enforce their immersion intent.
bool LLPipeline::shouldHideForOutsideParcel(LLDrawable* drawablep)
{
    if (!drawablep || drawablep->isSpatialBridge())
    {
        return false;
    }

    LLViewerObject* vobj = drawablep->getVObj();
    if (!vobj)
    {
        return false;
    }

    // HUDs are user-interface elements that live in the agent's HUD slot,
    // not world geometry. They must remain visible regardless of any
    // keepavatars setting (visitor preference or parcel-owner tag).
    if (vobj->isHUDAttachment())
    {
        return false;
    }

    // Self-worn attachments are always at gAgent's position, which is by
    // definition inside the agent parcel that drives this filter. Skip the
    // position-check entirely: relying on the attachment drawable's cached
    // agent-frame position is unsafe right after a TP — mPositionAgent can
    // briefly hold the old SIM's value while gAgent has already rebound to
    // the new region, producing a bogus global coordinate that gets locked
    // into the per-seq cache as "hidden". See
    // docs/ayastorm-fix-parcel-hide-stale-position.md §3.1 / §4.1.
    if (vobj->isAttachment())
    {
        LLVOAvatar* wearer = vobj->getAvatar();
        if (wearer && wearer->isSelf())
        {
            return false;
        }
    }

    const bool keep_avatars = sParcelOwnerTagActive
        ? sParcelOwnerTagKeepAvatars
        : sParcelHideKeepAvatars;
    const bool keep_own = sParcelOwnerTagActive
        ? sParcelOwnerTagKeepOwn
        : sParcelHideKeepOwn;

    if (keep_avatars && (vobj->isAvatar() || vobj->isAttachment()))
    {
        return false;
    }

    if (keep_own && vobj->permYouOwner())
    {
        return false;
    }

    // Defer the verdict (and skip caching) while the object's region is
    // unresolved. During TP / region transitions LLViewerObject::getPositionGlobal()
    // falls back to returning getPosition() — a local-frame value cast as
    // a global coordinate — which lies outside any parcel. Caching that
    // false-negative for the current sParcelCheckSeq would leave the object
    // hidden until the next agent-parcel change. See
    // docs/ayastorm-fix-parcel-hide-stale-position.md §3.2 / §4.2.
    LLViewerRegion* objRegion = vobj->getRegion();
    if (!objRegion || !LLWorld::instance().isRegionListed(objRegion))
    {
        return false;
    }

    if (drawablep->mLastParcelCheckSeq == sParcelCheckSeq)
    {
        return drawablep->mLastParcelCheckHidden;
    }

    bool hidden = !LLViewerParcelMgr::getInstance()->inAgentParcel(vobj->getPositionGlobal());
    drawablep->mLastParcelCheckSeq = sParcelCheckSeq;
    drawablep->mLastParcelCheckHidden = hidden;
    return hidden;
}

// Combined gate used by render pipelines. Encapsulates:
//   1) visitor preference (sParcelHideEnabled) — unconditional, no altitude window
//   2) parcel-owner [parcelhide:...] tag — gated on agent altitude when the
//      tag carries an altitude:min-max[,min-max...] specification; an empty
//      altRanges list means the tag applies at all heights (back-compat).
// shouldHideForOutsideParcel() is invoked only when one of the two paths is
// alive, so its per-drawable cache stays coherent across altitude crossings.
bool LLPipeline::isParcelHideAlive(LLDrawable* drawablep)
{
    if (sParcelHideEnabled)
    {
        return shouldHideForOutsideParcel(drawablep);
    }
    if (sParcelOwnerTagActive)
    {
        if (!sParcelOwnerTagAltRanges.empty())
        {
            const F32 z = gAgent.getPositionAgent().mV[VZ];
            bool in_range = false;
            for (const auto& r : sParcelOwnerTagAltRanges)
            {
                if (z >= r.first && z <= r.second) { in_range = true; break; }
            }
            if (!in_range) return false;
        }
        return shouldHideForOutsideParcel(drawablep);
    }
    return false;
}

void LLPipeline::refreshOutsideParcelHiding()
{
    sParcelCheckSeq++;

    // Re-evaluate the parcel-owner [parcelhide:...] tag (legacy
    // [AYAstorm:...] also accepted) for the parcel the agent is currently
    // standing in. This drives sParcelOwnerTagActive and the tag-supplied
    // keepavatars / keepownobject overrides used inside
    // shouldHideForOutsideParcel().
    LLParcel* agent_parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    if (agent_parcel)
    {
        ParcelTagOverride tag = parseParcelHideTag(agent_parcel->getDesc());
        sParcelOwnerTagActive = tag.active;
        sParcelOwnerTagKeepAvatars = tag.keepAvatars;
        sParcelOwnerTagKeepOwn = tag.keepOwn;
        sParcelOwnerTagAltRanges = std::move(tag.altRanges);
    }
    else
    {
        sParcelOwnerTagActive = false;
        sParcelOwnerTagKeepAvatars = false;
        sParcelOwnerTagKeepOwn = false;
        sParcelOwnerTagAltRanges.clear();
    }

    if (!gPipeline.assertInitialized())
    {
        return;
    }

    // Mark every volume spatial group GEOM_DIRTY (same mechanism as the
    // "Highlight Transparent" toggle) so that rebuildGeom re-runs and the
    // parcel filter is applied.
    gPipeline.rebuildDrawInfo();

    // The bulk path above sometimes fails to immediately re-batch every
    // visible group (depends on cull state / IN_BUILD_Q1 timing). Also walk
    // the live object list and mark each volume drawable's spatial group
    // dirty — this mirrors the per-object rebuild path that we know works
    // (the same one triggered by edit-selecting an object).
    const S32 num_objects = gObjectList.getNumObjects();
    for (S32 i = 0; i < num_objects; ++i)
    {
        LLViewerObject* objp = gObjectList.getObject(i);
        if (!objp || objp->isDead())
        {
            continue;
        }
        LLDrawable* drawablep = objp->mDrawable;
        if (!drawablep || drawablep->isDead())
        {
            continue;
        }
        LLSpatialGroup* groupp = drawablep->getSpatialGroup();
        if (groupp)
        {
            groupp->dirtyGeom();
            gPipeline.markRebuild(groupp);
        }
    }
}
// </FS:AYA>

void LLPipeline::markVisible(LLDrawable *drawablep, LLCamera& camera)
{
    if(drawablep && !drawablep->isDead())
    {
        if (drawablep->isSpatialBridge())
        {
            const LLDrawable* root = ((LLSpatialBridge*) drawablep)->mDrawable;
            llassert(root); // trying to catch a bad assumption

            if (root && //  // this test may not be needed, see above
                    root->getVObj()->isAttachment())
            {
                LLDrawable* rootparent = root->getParent();
                if (rootparent) // this IS sometimes NULL
                {
                    LLViewerObject *vobj = rootparent->getVObj();
                    llassert(vobj); // trying to catch a bad assumption
                    if (vobj) // this test may not be needed, see above
                    {
                        LLVOAvatar* av = vobj->asAvatar();
                        if (av &&
                            ((!isFrameImpostorPass() && av->isImpostor()) //ignore impostor flag during impostor pass
                             //|| av->isInMuteList() // <FS:Ansariel> Partially undo MAINT-5700: Draw imposter for muted avatars
                             || (LLVOAvatar::AOA_JELLYDOLL == av->getOverallAppearance() && !av->needsImpostorUpdate()) ))
                        {
                            return;
                        }
                    }
                }
            }
            getFrameCull()->pushBridge((LLSpatialBridge*) drawablep);
        }
        else
        {

            getFrameCull()->pushDrawable(drawablep);
        }

        drawablep->setVisible(camera);
    }
}

void LLPipeline::markMoved(LLDrawable *drawablep, bool damped_motion)
{
    if (!drawablep)
    {
        //LL_ERRS() << "Sending null drawable to moved list!" << LL_ENDL;
        return;
    }

    if (drawablep->isDead())
    {
        LL_WARNS() << "Marking NULL or dead drawable moved!" << LL_ENDL;
        return;
    }

    if (drawablep->getParent())
    {
        //ensure that parent drawables are moved first
        markMoved(drawablep->getParent(), damped_motion);
    }

    assertInitialized();

    if (!drawablep->isState(LLDrawable::ON_MOVE_LIST))
    {
        if (drawablep->isSpatialBridge())
        {
            mMovedBridge.push_back(drawablep);
        }
        else
        {
            mMovedList.push_back(drawablep);
        }
        drawablep->setState(LLDrawable::ON_MOVE_LIST);
    }
    if (! damped_motion)
    {
        drawablep->setState(LLDrawable::MOVE_UNDAMPED); // UNDAMPED trumps DAMPED
    }
    else if (drawablep->isState(LLDrawable::MOVE_UNDAMPED))
    {
        drawablep->clearState(LLDrawable::MOVE_UNDAMPED);
    }
}

void LLPipeline::markShift(LLDrawable *drawablep)
{
    if (!drawablep || drawablep->isDead() || !drawablep->getVObj())
    {
        return;
    }

    assertInitialized();

    if (!drawablep->isState(LLDrawable::ON_SHIFT_LIST))
    {
        drawablep->getVObj()->setChanged(LLXform::SHIFTED | LLXform::SILHOUETTE);
        if (drawablep->getParent())
        {
            markShift(drawablep->getParent());
        }
        mShiftList.push_back(drawablep);
        drawablep->setState(LLDrawable::ON_SHIFT_LIST);
    }
}

void LLPipeline::shiftObjects(const LLVector3 &offset)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    assertInitialized();

    LLRenderTarget::clearBoundTarget(GL_DEPTH_BUFFER_BIT);
    gDepthDirty = true;

    LLVector4a offseta;
    offseta.load3(offset.mV);

    for (LLDrawable::drawable_vector_t::iterator iter = mShiftList.begin();
            iter != mShiftList.end(); iter++)
    {
        LLDrawable *drawablep = *iter;
        if (drawablep->isDead() || !drawablep->getVObj())
        {
            continue;
        }
        drawablep->shiftPos(offseta);
        drawablep->clearState(LLDrawable::ON_SHIFT_LIST);
    }
    mShiftList.resize(0);

    for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
            iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
    {
        LLViewerRegion* region = *iter;
        for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
        {
            LLSpatialPartition* part = region->getSpatialPartition(i);
            if (part)
            {
                part->shift(offseta);
            }
        }
    }

    mReflectionMapManager.shift(offseta);

    LLHUDText::shiftAll(offset);
    LLHUDNameTag::shiftAll(offset);

    display_update_camera();
}

void LLPipeline::markTextured(LLDrawable *drawablep)
{
    if (drawablep && !drawablep->isDead() && assertInitialized())
    {
        mRetexturedList.insert(drawablep);
    }
}

void LLPipeline::markGLRebuild(LLGLUpdate* glu)
{
    if (glu && !glu->mInQ)
    {
        LLGLUpdate::sGLQ.push_back(glu);
        glu->mInQ = true;
    }
}

void LLPipeline::markPartitionMove(LLDrawable* drawable)
{
    if (!drawable->isState(LLDrawable::PARTITION_MOVE) &&
        !drawable->getPositionGroup().equals3(LLVector4a::getZero()))
    {
        drawable->setState(LLDrawable::PARTITION_MOVE);
        mPartitionQ.push_back(drawable);
    }
}

void LLPipeline::processPartitionQ()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    // <FS:ND> A vector is much better suited for the use case of mPartitionQ
    // for (LLDrawable::drawable_list_t::iterator iter = mPartitionQ.begin(); iter != mPartitionQ.end(); ++iter)
    for (LLDrawable::drawable_vector_t::iterator iter = mPartitionQ.begin(); iter != mPartitionQ.end(); ++iter)
    // </FS:ND>
    {
        LLDrawable* drawable = *iter;
        if (!drawable->isDead())
        {
            drawable->updateBinRadius();
            drawable->movePartition();
        }
        drawable->clearState(LLDrawable::PARTITION_MOVE);
    }

    mPartitionQ.clear();
}

void LLPipeline::markMeshDirty(LLSpatialGroup* group)
{
    mMeshDirtyGroup.push_back(group);
}

void LLPipeline::markRebuild(LLSpatialGroup* group)
{
    if (group && !group->isDead() && group->getSpatialPartition())
    {
        if (!group->hasState(LLSpatialGroup::IN_BUILD_Q1))
        {
            llassert_always(!mGroupQ1Locked);

            mGroupQ1.push_back(group);
            group->setState(LLSpatialGroup::IN_BUILD_Q1);
        }
    }
}

void LLPipeline::markRebuild(LLDrawable *drawablep, LLDrawable::EDrawableFlags flag)
{
    if (drawablep && !drawablep->isDead() && assertInitialized())
    {
        if (!drawablep->isState(LLDrawable::IN_REBUILD_Q))
        {
            mBuildQ1.push_back(drawablep);
            drawablep->setState(LLDrawable::IN_REBUILD_Q); // mark drawable as being in priority queue
        }

        // <FS:Ansariel> FIRE-16485: Crash when calling texture refresh on an object that has a blacklisted copy
        //if (flag & (LLDrawable::REBUILD_VOLUME | LLDrawable::REBUILD_POSITION))
        if ((flag & (LLDrawable::REBUILD_VOLUME | LLDrawable::REBUILD_POSITION)) && drawablep->getVObj().notNull())
        // </FS:Ansariel>
        {
            drawablep->getVObj()->setChanged(LLXform::SILHOUETTE);
        }
        drawablep->setState(flag);
    }
}

static void vkcClassifyEmptyOtherGroup(LLSpatialGroup* group, U32* cls, std::vector<LLUUID>& hole_uuids,
                                       std::vector<std::string>& zerogeom_info, std::vector<std::string>& transp_info)
{
    static LLCachedControl<bool> sa_protect(gSavedSettings, "RenderVolumeSAProtection");
    static LLCachedControl<F32> volume_sa_thresh(gSavedSettings, "RenderVolumeSAThreshold");
    static LLCachedControl<F32> sculpt_sa_thresh(gSavedSettings, "RenderSculptSAThreshold");

    for (LLSpatialGroup::element_iter it = group->getDataBegin(); it != group->getDataEnd(); ++it)
    {
        LLDrawable* drawablep = (LLDrawable*)(*it)->getDrawable();
        if (!drawablep || drawablep->isDead() || drawablep->isState(LLDrawable::FORCE_INVISIBLE))
        {
            ++cls[0];
            continue;
        }
        if (LLPipeline::isParcelHideAlive(drawablep))
        {
            ++cls[1];
            continue;
        }
        LLVOVolume* vobj = drawablep->getVOVolume();
        if (!vobj || vobj->isDead())
        {
            ++cls[0];
            continue;
        }
        if (drawablep->isState(LLDrawable::RIGGED | LLDrawable::RIGGED_CHILD))
        {
            continue;
        }
        if (vobj->mGLTFAsset)
        {
            ++cls[2];
            continue;
        }
        if (vobj->isMesh())
        {
            if ((vobj->getVolume() && !vobj->getVolume()->isMeshAssetLoaded()) || !gMeshRepo.meshRezEnabled())
            {
                ++cls[3];
                continue;
            }
            if (!vobj->getSkinInfo() && !vobj->isSkinInfoUnavaliable())
            {
                ++cls[4];
                continue;
            }
        }
        if (sa_protect && vobj->mVolumeSurfaceArea > (vobj->isSculpted() ? (F32)sculpt_sa_thresh : (F32)volume_sa_thresh))
        {
            ++cls[5];
            continue;
        }
        bool any_geom = false;
        bool any_visible = false;
        bool any_visible_te = false;
        F32 dbg_alpha = -1.f;
        for (S32 i = 0; i < drawablep->getNumFaces(); ++i)
        {
            LLFace* facep = drawablep->getFace(i);
            if (!facep)
            {
                continue;
            }
            const LLTextureEntry* te = facep->getTextureEntry();
            LLGLTFMaterial* gltf_mat = te ? te->getGLTFRenderMaterial() : nullptr;
            F32 alpha = gltf_mat ? gltf_mat->mBaseColor.mV[3] : (te ? te->getColor().mV[3] : 1.f);
            const bool visible_te = (alpha > 0.f || (te && te->getGlow() > 0.f));
            any_visible_te |= visible_te;
            if (facep->getIndicesCount() <= 0 || facep->getGeomCount() <= 0)
            {
                continue;
            }
            any_geom = true;
            if (dbg_alpha < 0.f)
            {
                dbg_alpha = alpha;
            }
            if (visible_te)
            {
                any_visible = true;
            }
        }
        if (any_visible)
        {
            ++cls[8];
            if (hole_uuids.size() < 8)
            {
                hole_uuids.push_back(vobj->getID());
            }
        }
        else if (any_geom)
        {
            ++cls[7];
            if (transp_info.size() < 4)
            {
                transp_info.push_back(llformat("%s a=%.2f nf=%d mesh=%d",
                    vobj->getID().asString().c_str(), dbg_alpha,
                    drawablep->getNumFaces(), (S32)vobj->isMesh()));
            }
        }
        else if (any_visible_te)
        {
            ++cls[9];
            if (zerogeom_info.size() < 4)
            {
                LLVolume* dbg_vol = vobj->getVolume();
                zerogeom_info.push_back(llformat("%s mesh=%d vf=%d df=%d st=0x%x nv=%d",
                    vobj->getID().asString().c_str(), (S32)vobj->isMesh(),
                    dbg_vol ? dbg_vol->getNumVolumeFaces() : -1,
                    drawablep->getNumFaces(), (U32)drawablep->getState(),
                    (dbg_vol && dbg_vol->getNumVolumeFaces() > 0)
                        ? dbg_vol->getVolumeFace(0).mNumVertices : -1));
            }
        }
        else
        {
            ++cls[6];
        }
    }
}

void LLPipeline::stateSort(LLCamera& camera, LLCullResult &result)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LL_PROFILE_GPU_ZONE("stateSort");

    if (hasAnyRenderType(LLPipeline::RENDER_TYPE_AVATAR,
                      LLPipeline::RENDER_TYPE_CONTROL_AV,
                      LLPipeline::RENDER_TYPE_TERRAIN,
                      LLPipeline::RENDER_TYPE_TREE,
                      LLPipeline::RENDER_TYPE_SKY,
                      LLPipeline::RENDER_TYPE_VOIDWATER,
                      LLPipeline::RENDER_TYPE_WATER,
                      LLPipeline::END_RENDER_TYPES))
    {
        //clear faces from face pools
        gPipeline.resetDrawOrders();
    }

    //LLVertexBuffer::unbind();

    grabReferences(result);
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("checkOcclusionAndRebuildMesh");
    for (LLCullResult::sg_iterator iter = getFrameCull()->beginDrawableGroups(); iter != getFrameCull()->endDrawableGroups(); ++iter)
    {
        LLSpatialGroup* group = *iter;
        if (group->isDead())
        {
            continue;
        }
        group->checkOcclusion();
        if (sUseOcclusion > 1 && group->isOcclusionState(LLSpatialGroup::OCCLUDED))
        {
            markOccluder(group);
        }
        else
        {
            group->setVisible();
            for (LLSpatialGroup::element_iter i = group->getDataBegin(); i != group->getDataEnd(); ++i)
            {
                LLDrawable* drawablep = (LLDrawable*)(*i)->getDrawable();
                markVisible(drawablep, camera);
            }

            { //rebuild mesh as soon as we know it's visible
                group->rebuildMesh();
            }
        }
    }
    }

    if (LLViewerCamera::getCurCameraID() == LLViewerCamera::CAMERA_WORLD && !gCubeSnapshot)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("WorldCamera");
        LLSpatialGroup* last_group = NULL;
        bool fov_changed = LLViewerCamera::getInstance()->isDefaultFOVChanged();
        for (LLCullResult::bridge_iterator i = getFrameCull()->beginVisibleBridge(); i != getFrameCull()->endVisibleBridge(); ++i)
        {
            LLCullResult::bridge_iterator cur_iter = i;
            LLSpatialBridge* bridge = *cur_iter;
            LLSpatialGroup* group = bridge->getSpatialGroup();

            if (last_group == NULL)
            {
                last_group = group;
            }

            if (!bridge->isDead() && group && !group->isOcclusionState(LLSpatialGroup::OCCLUDED))
            {
                stateSort(bridge, camera, fov_changed);
            }

            if (LLViewerCamera::getCurCameraID() == LLViewerCamera::CAMERA_WORLD &&
                last_group != group && last_group->changeLOD())
            {
                last_group->mLastUpdateDistance = last_group->mDistance;
            }

            last_group = group;
        }

        if (LLViewerCamera::getCurCameraID() == LLViewerCamera::CAMERA_WORLD &&
            last_group && last_group->changeLOD())
        {
            last_group->mLastUpdateDistance = last_group->mDistance;
        }
    }
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("StateSort: visible groups");
    U32 vkc_empty_inflight = 0;
    U32 vkc_empty_dirty    = 0;
    U32 vkc_empty_other    = 0;
    static U32 s_vkc_other_class[10] = {};
    static std::vector<LLUUID> s_vkc_hole_uuids;
    static std::vector<std::string> s_vkc_zerogeom_info;
    static std::vector<std::string> s_vkc_transp_info;
    for (LLCullResult::sg_iterator iter = getFrameCull()->beginVisibleGroups(); iter != getFrameCull()->endVisibleGroups(); ++iter)
    {
        LLSpatialGroup* group = *iter;
        if (group->isDead())
        {
            continue;
        }
        if (group->mDrawMap.empty() && group->getElementCount() > 0)
        {
            if (group->mVkGeoInflight)
            {
                ++vkc_empty_inflight;
            }
            else if (group->hasState(LLSpatialGroup::GEOM_DIRTY | LLSpatialGroup::ALPHA_DIRTY))
            {
                ++vkc_empty_dirty;
            }
            else
            {
                ++vkc_empty_other;
                if (LLVKContract::verboseEnabled())
                {
                    vkcClassifyEmptyOtherGroup(group, s_vkc_other_class, s_vkc_hole_uuids,
                                               s_vkc_zerogeom_info, s_vkc_transp_info);
                }
            }
        }
        group->checkOcclusion();
        if (sUseOcclusion > 1 && group->isOcclusionState(LLSpatialGroup::OCCLUDED))
        {
            markOccluder(group);
        }
        else
        {
            group->setVisible();
            stateSort(group, camera);

            { //rebuild mesh as soon as we know it's visible
                group->rebuildMesh();
            }
        }
    }
    if (vkc_empty_inflight + vkc_empty_dirty + vkc_empty_other > 0)
    {
        static U32 s_vkc_empty_frames = 0;
        static U32 s_acc_inflight = 0;
        static U32 s_acc_dirty = 0;
        static U32 s_acc_other = 0;
        s_acc_inflight += vkc_empty_inflight;
        s_acc_dirty    += vkc_empty_dirty;
        s_acc_other    += vkc_empty_other;
        if ((++s_vkc_empty_frames % 60) == 1)
        {
            std::ostringstream cls;
            if (LLVKContract::verboseEnabled())
            {
                cls << " other_class{dead=" << s_vkc_other_class[0]
                    << " parcel=" << s_vkc_other_class[1]
                    << " gltf=" << s_vkc_other_class[2]
                    << " meshwait=" << s_vkc_other_class[3]
                    << " skinwait=" << s_vkc_other_class[4]
                    << " sa=" << s_vkc_other_class[5]
                    << " zerogeom=" << s_vkc_other_class[6]
                    << " transp=" << s_vkc_other_class[7]
                    << " hasgeom=" << s_vkc_other_class[8]
                    << " zerogeomv=" << s_vkc_other_class[9] << "}";
                if (!s_vkc_hole_uuids.empty())
                {
                    cls << " hole_uuid[";
                    for (size_t u = 0; u < s_vkc_hole_uuids.size(); ++u)
                    {
                        cls << (u ? " " : "") << s_vkc_hole_uuids[u];
                    }
                    cls << "]";
                }
                if (!s_vkc_zerogeom_info.empty())
                {
                    cls << " zerogeomv[";
                    for (size_t u = 0; u < s_vkc_zerogeom_info.size(); ++u)
                    {
                        cls << (u ? " | " : "") << s_vkc_zerogeom_info[u];
                    }
                    cls << "]";
                }
                if (!s_vkc_transp_info.empty())
                {
                    cls << " transp[";
                    for (size_t u = 0; u < s_vkc_transp_info.size(); ++u)
                    {
                        cls << (u ? " | " : "") << s_vkc_transp_info[u];
                    }
                    cls << "]";
                }
            }
            LL_WARNS("VKGeo") << "visible empty-drawmap groups (60f acc): inflight=" << s_acc_inflight
                              << " dirty=" << s_acc_dirty
                              << " other=" << s_acc_other << cls.str() << LL_ENDL;
            s_acc_inflight = s_acc_dirty = s_acc_other = 0;
            for (U32 ci = 0; ci < 10; ++ci)
            {
                s_vkc_other_class[ci] = 0;
            }
            s_vkc_hole_uuids.clear();
            s_vkc_zerogeom_info.clear();
            s_vkc_transp_info.clear();
        }
    }}

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWABLE("stateSort"); // LL_RECORD_BLOCK_TIME(FTM_STATESORT_DRAWABLE);
        for (LLCullResult::drawable_iterator iter = getFrameCull()->beginVisibleList();
             iter != getFrameCull()->endVisibleList(); ++iter)
        {
            LLDrawable *drawablep = *iter;
            if (!drawablep->isDead())
            {
                stateSort(drawablep, camera);
            }
        }
    }

    postSort(camera);
}

void LLPipeline::stateSort(LLSpatialGroup* group, LLCamera& camera)
{
    if (group->changeLOD())
    {
        for (LLSpatialGroup::element_iter i = group->getDataBegin(); i != group->getDataEnd(); ++i)
        {
            LLDrawable* drawablep = (LLDrawable*)(*i)->getDrawable();
            stateSort(drawablep, camera);
        }

        if (LLViewerCamera::getCurCameraID() == LLViewerCamera::CAMERA_WORLD && !gCubeSnapshot)
        { //avoid redundant stateSort calls
            group->mLastUpdateDistance = group->mDistance;
        }
    }
}

void LLPipeline::stateSort(LLSpatialBridge* bridge, LLCamera& camera, bool fov_changed)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    if (bridge->getSpatialGroup()->changeLOD() || fov_changed)
    {
        bool force_update = false;
        bridge->updateDistance(camera, force_update);
    }
}

void LLPipeline::stateSort(LLDrawable* drawablep, LLCamera& camera)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    if (!drawablep
        || drawablep->isDead()
        || !hasRenderType(drawablep->getRenderType()))
    {
        return;
    }

    // SL-11353
    // ignore our own geo when rendering spotlight shadowmaps...
    //
    if (RenderSpotLight && drawablep == RenderSpotLight)
    {
        return;
    }

    if (LLSelectMgr::getInstance()->mHideSelectedObjects)
    {
//      if (drawablep->getVObj().notNull() &&
//          drawablep->getVObj()->isSelected())
// [RLVa:KB] - Checked: 2010-09-28 (RLVa-1.2.1f) | Modified: RLVa-1.2.1f
        const LLViewerObject* pObj = drawablep->getVObj();
        if ( (pObj) && (pObj->isSelected()) &&
             ( (!RlvActions::isRlvEnabled()) ||
               ( ((!pObj->isHUDAttachment()) || (!gRlvAttachmentLocks.isLockedAttachment(pObj->getRootEdit()))) &&
                 (RlvActions::canEdit(pObj)) ) ) )
// [/RVLa:KB]
        {
            return;
        }
    }

    if (drawablep->isAvatar())
    { //don't draw avatars beyond render distance or if we don't have a spatial group.
        if ((drawablep->getSpatialGroup() == NULL) ||
            (drawablep->getSpatialGroup()->mDistance > LLVOAvatar::sRenderDistance))
        {
            return;
        }

        LLVOAvatar* avatarp = (LLVOAvatar*) drawablep->getVObj().get();
        if (!avatarp->isVisible())
        {
            return;
        }
    }

    assertInitialized();

    if (hasRenderType(drawablep->mRenderType))
    {
        if (!drawablep->isState(LLDrawable::INVISIBLE|LLDrawable::FORCE_INVISIBLE))
        {
            drawablep->setVisible(camera, NULL, false);
        }
    }

    if (LLViewerCamera::getCurCameraID() == LLViewerCamera::CAMERA_WORLD && !gCubeSnapshot)
    {
        //if (drawablep->isVisible()) isVisible() check here is redundant, if it wasn't visible, it wouldn't be here
        {
            if (!drawablep->isActive())
            {
                bool force_update = false;
                drawablep->updateDistance(camera, force_update);
            }
            else if (drawablep->isAvatar())
            {
                bool force_update = false;
                drawablep->updateDistance(camera, force_update); // calls vobj->updateLOD() which calls LLVOAvatar::updateVisibility()
            }
        }
    }

    if (!drawablep->getVOVolume())
    {
        for (LLDrawable::face_list_t::iterator iter = drawablep->mFaces.begin();
                iter != drawablep->mFaces.end(); iter++)
        {
            LLFace* facep = *iter;

            if (facep->hasGeometry())
            {
                if (facep->getPool())
                {
                    facep->getPool()->enqueue(facep);
                }
                else
                {
                    break;
                }
            }
        }
    }

    mNumVisibleFaces += drawablep->getNumFaces();
}


void forAllDrawables(LLCullResult::sg_iterator begin,
                     LLCullResult::sg_iterator end,
                     void (*func)(LLDrawable*))
{
    for (LLCullResult::sg_iterator i = begin; i != end; ++i)
    {
        LLSpatialGroup* group = *i;
        if (group->isDead())
        {
            continue;
        }
        for (LLSpatialGroup::element_iter j = group->getDataBegin(); j != group->getDataEnd(); ++j)
        {
            if((*j)->hasDrawable())
            {
                func((LLDrawable*)(*j)->getDrawable());
            }
        }
    }
}

void LLPipeline::forAllVisibleDrawables(void (*func)(LLDrawable*))
{
    forAllDrawables(getFrameCull()->beginDrawableGroups(), getFrameCull()->endDrawableGroups(), func);
    forAllDrawables(getFrameCull()->beginVisibleGroups(), getFrameCull()->endVisibleGroups(), func);
}

//function for creating scripted beacons
void renderScriptedBeacons(LLDrawable* drawablep)
{
    LLViewerObject *vobj = drawablep->getVObj();
    if (vobj
        && !vobj->isAvatar()
        && !vobj->getParent()
        && vobj->flagScripted())
    {
        if (gPipeline.sRenderBeacons)
        {
            gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", LLColor4(1.f, 0.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f), LLPipeline::DebugBeaconLineWidth);
        }

        if (gPipeline.sRenderHighlight)
        {
            S32 face_id;
            S32 count = drawablep->getNumFaces();
            for (face_id = 0; face_id < count; face_id++)
            {
                LLFace * facep = drawablep->getFace(face_id);
                if (facep)
                {
                    gPipeline.mHighlightFaces.push_back(facep);
                }
            }
        }
    }
}

void renderScriptedTouchBeacons(LLDrawable *drawablep)
{
    LLViewerObject *vobj = drawablep->getVObj();
    if (vobj && !vobj->isAvatar() && !vobj->getParent() && vobj->flagScripted() && vobj->flagHandleTouch())
    {
        if (gPipeline.sRenderBeacons)
        {
            gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", LLColor4(1.f, 0.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f),
                                       LLPipeline::DebugBeaconLineWidth);
        }

        if (gPipeline.sRenderHighlight)
        {
            S32 face_id;
            S32 count = drawablep->getNumFaces();
            for (face_id = 0; face_id < count; face_id++)
            {
                LLFace *facep = drawablep->getFace(face_id);
                if (facep)
                {
                    gPipeline.mHighlightFaces.push_back(facep);
                }
            }
        }
    }
}

void renderPhysicalBeacons(LLDrawable *drawablep)
{
    LLViewerObject *vobj = drawablep->getVObj();
    if (vobj &&
        !vobj->isAvatar()
        //&& !vobj->getParent()
        && vobj->flagUsePhysics())
    {
        if (gPipeline.sRenderBeacons)
        {
            gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", LLColor4(0.f, 1.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f),
                                       LLPipeline::DebugBeaconLineWidth);
        }

        if (gPipeline.sRenderHighlight)
        {
            S32 face_id;
            S32 count = drawablep->getNumFaces();
            for (face_id = 0; face_id < count; face_id++)
            {
                LLFace *facep = drawablep->getFace(face_id);
                if (facep)
                {
                    gPipeline.mHighlightFaces.push_back(facep);
                }
            }
        }
    }
}

void renderMOAPBeacons(LLDrawable *drawablep)
{
    LLViewerObject *vobj = drawablep->getVObj();

    if (!vobj || vobj->isAvatar())
        return;

    bool beacon  = false;
    U8   tecount = vobj->getNumTEs();
    for (int x = 0; x < tecount; x++)
    {
        if (vobj->getTEref(x).hasMedia())
        {
            beacon = true;
            break;
        }
    }
    if (beacon)
    {
        if (gPipeline.sRenderBeacons)
        {
            gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", LLColor4(1.f, 1.f, 1.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f),
                                       LLPipeline::DebugBeaconLineWidth);
        }

        if (gPipeline.sRenderHighlight)
        {
            S32 face_id;
            S32 count = drawablep->getNumFaces();
            for (face_id = 0; face_id < count; face_id++)
            {
                LLFace *facep = drawablep->getFace(face_id);
                if (facep)
                {
                    gPipeline.mHighlightFaces.push_back(facep);
                }
            }
        }
    }
}

void renderParticleBeacons(LLDrawable *drawablep)
{
    // Look for attachments, objects, etc.
    LLViewerObject *vobj = drawablep->getVObj();
    if (vobj && vobj->isParticleSource())
    {
        if (gPipeline.sRenderBeacons)
        {
            LLColor4 light_blue(0.5f, 0.5f, 1.f, 0.5f);
            gObjectList.addDebugBeacon(vobj->getPositionAgent(), "", light_blue, LLColor4(1.f, 1.f, 1.f, 0.5f),
                                       LLPipeline::DebugBeaconLineWidth);
        }

        if (gPipeline.sRenderHighlight)
        {
            S32 face_id;
            S32 count = drawablep->getNumFaces();
            for (face_id = 0; face_id < count; face_id++)
            {
                LLFace *facep = drawablep->getFace(face_id);
                if (facep)
                {
                    gPipeline.mHighlightFaces.push_back(facep);
                }
            }
        }
    }
}

void renderSoundHighlights(LLDrawable *drawablep)
{
    // Look for attachments, objects, etc.
    LLViewerObject *vobj = drawablep->getVObj();
    if (vobj && vobj->isAudioSource())
    {
        if (gPipeline.sRenderHighlight)
        {
            S32 face_id;
            S32 count = drawablep->getNumFaces();
            for (face_id = 0; face_id < count; face_id++)
            {
                LLFace *facep = drawablep->getFace(face_id);
                if (facep)
                {
                    gPipeline.mHighlightFaces.push_back(facep);
                }
            }
        }
    }
}

namespace LLVKListOracle
{
    struct Entry
    {
        LLVector4a center;
        LLVector4a size;
    };
    static std::unordered_map<U32, Entry> sPrev;
    static std::unordered_map<U32, Entry> sCur;

    static void record(U32 objid, const LLVector4a* bounds)
    {
        if (objid == 0)
        {
            return;
        }
        Entry e;
        e.center = bounds[0];
        e.size   = bounds[1];
        sCur.emplace(objid, e);
    }
}

namespace LLVKUuidWatch
{
    struct Member
    {
        LLUUID uuid;
        U32 localid = 0;
        U32 absent = 0;
        bool haveBounds = false;
        S32 prevVis = -2;
        S32 prevOccl = -2;
        S32 prevDirty = -2;
        S32 prevInflight = -2;
        S32 prevRecs = -2;
        S32 prevOwn = -2;
        LL_ALIGN_16(LLVector4a center);
        LL_ALIGN_16(LLVector4a size);
    };
    static std::vector<LLUUID> sPending;
    static std::vector<Member> sMembers;
    static bool sLoaded = false;

    static void loadOnce()
    {
        if (sLoaded)
        {
            return;
        }
        sLoaded = true;
        const char* path = getenv("AYASTORM_VKC_UUIDS");
        if (path == nullptr)
        {
            return;
        }
        std::ifstream in(path);
        std::string line;
        U32 n = 0;
        while (std::getline(in, line))
        {
            LLStringUtil::trim(line);
            LLUUID id;
            if (line.size() >= 36 && LLUUID::parseUUID(line, &id) && id.notNull())
            {
                sPending.push_back(id);
                ++n;
            }
        }
        LL_WARNS("VKContract") << "VKC-UUID loaded " << n << " watch uuids from " << path << LL_ENDL;
    }

    static void tick(LLCamera& camera, U32 frame)
    {
        loadOnce();
        if (!sPending.empty())
        {
            size_t w = 0;
            for (size_t r = 0; r < sPending.size(); ++r)
            {
                LLViewerObject* o = gObjectList.findObject(sPending[r]);
                if (o != nullptr && o->getLocalID() != 0)
                {
                    Member m;
                    m.uuid    = sPending[r];
                    m.localid = o->getLocalID();
                    sMembers.push_back(m);
                    LLVKContract::watchAddLocal(m.localid);
                    LL_WARNS("VKContract") << "VKC-UUID resolved uuid=" << m.uuid
                                           << " local=" << m.localid << LL_ENDL;
                }
                else
                {
                    if (w != r)
                    {
                        sPending[w] = sPending[r];
                    }
                    ++w;
                }
            }
            sPending.resize(w);
        }
        static F64 s_state_last = 0.0;
        const F64 state_now = LLFrameTimer::getTotalSeconds();
        const bool emit_state = (state_now - s_state_last) >= 10.0;
        if (emit_state)
        {
            s_state_last = state_now;
        }
        for (Member& m : sMembers)
        {
            S32 vis = -1, occl = -1, dirty = -1, inflight = -1, recs = -1, own = -1;
            S32 rbage = -1, rbret = -1;
            LLViewerObject* o = gObjectList.findObject(m.uuid);
            if (o != nullptr && o->mDrawable.notNull())
            {
                LLSpatialGroup* g = o->mDrawable->getSpatialGroup();
                if (g != nullptr)
                {
                    vis      = g->isVisible() ? 1 : 0;
                    occl     = g->isOcclusionState(LLSpatialGroup::OCCLUDED) ? 1 : 0;
                    dirty    = g->hasState(LLSpatialGroup::GEOM_DIRTY | LLSpatialGroup::ALPHA_DIRTY) ? 1 : 0;
                    inflight = g->mVkGeoInflight ? 1 : 0;
                    rbage    = (S32)((U32)gFrameCount - g->mVkRebuildVisitFrame);
                    rbret    = (S32)g->mVkRebuildRet;
                    recs     = 0;
                    own      = 0;
                    LLDrawable* dr = o->mDrawable.get();
                    for (LLSpatialGroup::draw_map_t::iterator j3 = g->mDrawMap.begin(); j3 != g->mDrawMap.end(); ++j3)
                    {
                        recs += (S32)j3->second.size();
                        for (LLSpatialGroup::drawmap_elem_t::iterator k3 = j3->second.begin(); k3 != j3->second.end(); ++k3)
                        {
                            if (k3->notNull() && (*k3)->mSrcDrawable.get() == dr)
                            {
                                ++own;
                            }
                        }
                    }
                }
            }
            if (emit_state)
            {
                LL_WARNS("VKContract") << "VKC-UUID state uuid=" << m.uuid
                                       << " local=" << m.localid
                                       << " vis=" << vis << " occl=" << occl
                                       << " dirty=" << dirty << " inflight=" << inflight
                                       << " recs=" << recs << " own=" << own
                                       << " rbage=" << rbage << " rbret=" << rbret
                                       << " fires=" << LLVKContract::watchTakeFires(m.localid)
                                       << " absent=" << m.absent << LL_ENDL;
            }
            auto it = LLVKListOracle::sCur.find(m.localid);
            if (it != LLVKListOracle::sCur.end())
            {
                if (m.absent >= 2)
                {
                    LL_WARNS("VKContract") << "VKC-UUID resumed local=" << m.localid
                                           << " gap=" << m.absent << " frame=" << frame << LL_ENDL;
                }
                m.absent     = 0;
                m.haveBounds = true;
                m.center     = it->second.center;
                m.size       = it->second.size;
            }
            else if (m.haveBounds && camera.AABBInFrustum(m.center, m.size) > 0)
            {
                ++m.absent;
                LLVKContract::cause(LLVKContract::C_UUID_ABSENT);
                if (m.absent == 1 || m.absent == 8 || m.absent == 32 || m.absent == 128)
                {
                    U32 esite = 0, erecs = 0;
                    U64 eage = 0, sage = 0;
                    const char* swhat = nullptr;
                    U32 sn = 0;
                    const bool he = LLVKContract::watchLastEvict(m.localid, esite, erecs, eage);
                    const bool hs = LLVKContract::watchLastStage(m.localid, swhat, sn, sage);
                    LL_WARNS("VKContract") << "VKC-UUID absent local=" << m.localid
                                           << " streak=" << m.absent << " frame=" << frame
                                           << " vis=" << m.prevVis << " occl=" << m.prevOccl
                                           << " dirty=" << m.prevDirty << " inflight=" << m.prevInflight
                                           << " recs=" << m.prevRecs
                                           << " own=" << m.prevOwn
                                           << " evict=" << (he ? LLVKContract::sentinelSiteName(esite) : "-")
                                           << "/" << erecs << "@-" << eage
                                           << " stage=" << (hs && swhat != nullptr ? swhat : "-")
                                           << "(" << sn << ")@-" << sage << LL_ENDL;
                }
            }
            m.prevVis      = vis;
            m.prevOccl     = occl;
            m.prevDirty    = dirty;
            m.prevInflight = inflight;
            m.prevRecs     = recs;
            m.prevOwn      = own;
        }
    }
}

namespace LLVKListOracle
{
    static void rotateAndReport(LLCamera& camera, U32 frame)
    {
        LLVKUuidWatch::tick(camera, frame);
        for (auto& p : sPrev)
        {
            if (sCur.find(p.first) == sCur.end())
            {
                if (camera.AABBInFrustum(p.second.center, p.second.size) > 0)
                {
                    LLVKContract::cause(LLVKContract::C_LIST_DROP_INFRUSTUM);
                }
            }
        }
        sPrev.swap(sCur);
        sCur.clear();
    }
}

void LLPipeline::postSort(LLCamera &camera)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    assertInitialized();
    sVolumeSAFrame = 0.f; //ZK LBG

    LL_PUSH_CALLSTACKS();

    if (!gCubeSnapshot)
    {
        // rebuild drawable geometry
        for (LLCullResult::sg_iterator i = getFrameCull()->beginDrawableGroups(); i != getFrameCull()->endDrawableGroups(); ++i)
        {
            LLSpatialGroup *group = *i;
            if (group->isDead())
            {
                continue;
            }
            if (!sUseOcclusion || !group->isOcclusionState(LLSpatialGroup::OCCLUDED))
            {
                group->rebuildGeom();
            }
        }
        LL_PUSH_CALLSTACKS();
        // rebuild groups
        getFrameCull()->assertDrawMapsEmpty();

        rebuildPriorityGroups();
    }

    LL_PUSH_CALLSTACKS();

    // build render map
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("build render map");
    const bool vkc_list_watch = LLVKContract::verboseEnabled()
        && LLViewerCamera::getCurCameraID() == LLViewerCamera::CAMERA_WORLD
        && !gCubeSnapshot && !isFrameShadowPass() && !isFrameReflectionPass()
        && !hasRenderType(LLPipeline::RENDER_TYPE_HUD);
    if (vkc_list_watch)
    {
        LLVKListOracle::rotateAndReport(camera, (U32)gFrameCount);
    }
    for (LLCullResult::sg_iterator i = getFrameCull()->beginVisibleGroups(); i != getFrameCull()->endVisibleGroups(); ++i)
    {
        LLSpatialGroup *group = *i;

        if (group->isDead())
        {
            continue;
        }

        if ((sUseOcclusion && group->isOcclusionState(LLSpatialGroup::OCCLUDED)) ||
            (RenderAutoHideSurfaceAreaLimit > 0.f &&
             group->mSurfaceArea > RenderAutoHideSurfaceAreaLimit * llmax(group->mObjectBoxSize, 10.f)))
        {
            if (vkc_list_watch && group->mVkLastFireFrame == gFrameCount - 1
                && camera.AABBInFrustum(group->getBounds()[0], group->getBounds()[1]) > 0)
            {
                U32 sample_obj = 0;
                U32 recs = 0;
                for (LLSpatialGroup::draw_map_t::iterator j2 = group->mDrawMap.begin(); j2 != group->mDrawMap.end(); ++j2)
                {
                    for (LLSpatialGroup::drawmap_elem_t::iterator k2 = j2->second.begin(); k2 != j2->second.end(); ++k2)
                    {
                        if (k2->notNull())
                        {
                            ++recs;
                            if (sample_obj == 0)
                            {
                                sample_obj = (*k2)->mFSPickerLocalID;
                            }
                        }
                    }
                }
                std::ostringstream os;
                os << "obj=" << sample_obj << " recs=" << recs
                   << " occl=" << ((sUseOcclusion && group->isOcclusionState(LLSpatialGroup::OCCLUDED)) ? 1 : 0)
                   << " frame=" << gFrameCount;
                LLVKContract::noteDetail(LLVKContract::C_LIST_OCCL_DROP, "occl", os.str());
            }
            continue;
        }

        getFrameCull()->setBucketVisible(group);
        group->mVkLastFireFrame = gFrameCount;
        if (group->mVkBucketIndexCount > 0 &&
            !isFrameShadowPass() && !isFrameReflectionPass() && !gCubeSnapshot)
        {
            addTrianglesDrawn(group->mVkBucketIndexCount);
        }

        for (LLSpatialGroup::draw_map_t::iterator j = group->mDrawMap.begin(); j != group->mDrawMap.end(); ++j)
        {
            LLSpatialGroup::drawmap_elem_t &src_vec = j->second;
            if (LLVKBucket::isBucketizedPass(j->first))
            {
                continue;
            }
            if (!hasRenderType(j->first))
            {
                continue;
            }

            for (LLSpatialGroup::drawmap_elem_t::iterator k = src_vec.begin(); k != src_vec.end(); ++k)
            {
                LLDrawInfo *info = *k;

                getFrameCull()->pushDrawInfo(j->first, info);
                if (vkc_list_watch)
                {
                    LLVKListOracle::record(info->mFSPickerLocalID, group->getBounds());
                }
                if (!isFrameShadowPass() && !isFrameReflectionPass() && !gCubeSnapshot)
                {
                    addTrianglesDrawn(info->mCount);
                }
            }
        }

        if (hasRenderType(LLPipeline::RENDER_TYPE_PASS_ALPHA))
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("Collect Alpha groups");
            LLSpatialGroup::draw_map_t::iterator alpha = group->mDrawMap.find(LLRenderPass::PASS_ALPHA);

            if (alpha != group->mDrawMap.end())
            {  // store alpha groups for sorting
                LLSpatialBridge *bridge = group->getSpatialPartition()->asBridge();
                if (LLViewerCamera::getCurCameraID() == LLViewerCamera::CAMERA_WORLD && !gCubeSnapshot)
                {
                    if (bridge)
                    {
                        LLCamera trans_camera = bridge->transformCamera(camera);
                        group->updateDistance(trans_camera);
                    }
                    else
                    {
                        group->updateDistance(camera);
                    }
                }

                if (hasRenderType(LLDrawPool::POOL_ALPHA))
                {
                    getFrameCull()->pushAlphaGroup(group);
                }
            }

            LLSpatialGroup::draw_map_t::iterator rigged_alpha = group->mDrawMap.find(LLRenderPass::PASS_ALPHA_RIGGED);

            if (rigged_alpha != group->mDrawMap.end())
            {  // store rigged alpha groups for LLDrawPoolAlpha prepass (skip distance update, rigged attachments use depth buffer)
                if (hasRenderType(LLDrawPool::POOL_ALPHA))
                {
                    getFrameCull()->pushRiggedAlphaGroup(group);
                }
            }
        }
    }
    }

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("rebuild delayed upd groups");
    // pack vertex buffers for groups that chose to delay their updates
    {
        LL_PROFILE_GPU_ZONE("rebuildMesh");
        for (LLSpatialGroup::sg_vector_t::iterator iter = mMeshDirtyGroup.begin(); iter != mMeshDirtyGroup.end(); ++iter)
        {
            (*iter)->rebuildMesh();
        }
    }
    }


    mMeshDirtyGroup.clear();

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("sort alpha groups");
    if (!isFrameShadowPass())
    {
        if (LLViewerCamera::getCurCameraID() == LLViewerCamera::CAMERA_WORLD && !gCubeSnapshot)
        {
            const LLVector3& cam_origin = camera.getOrigin();
            const LLVector3& cam_at = camera.getAtAxis();
            for (LLCullResult::sg_iterator iter = getFrameCull()->beginRiggedAlphaGroups(); iter != getFrameCull()->endRiggedAlphaGroups(); ++iter)
            {
                LLSpatialGroup* rgroup = *iter;
                LLVOAvatar* av = rgroup->mAvatarp;
                if (av && av->mDrawable)
                {
                    const LLVector3* av_box = av->getLastAnimExtents();
                    LLVector3 center = (av_box[0] + av_box[1]) * 0.5f - cam_origin;
                    LLVector3 half = (av_box[1] - av_box[0]) * 0.5f;
                    rgroup->mDepth = center * cam_at
                        - (fabsf(cam_at.mV[0]) * half.mV[0]
                         + fabsf(cam_at.mV[1]) * half.mV[1]
                         + fabsf(cam_at.mV[2]) * half.mV[2]);
                }
            }
        }

        // order alpha groups by distance
        std::sort(getFrameCull()->beginAlphaGroups(), getFrameCull()->endAlphaGroups(), LLSpatialGroup::CompareDepthGreater());

        // order rigged alpha groups by avatar attachment order
        std::sort(getFrameCull()->beginRiggedAlphaGroups(), getFrameCull()->endRiggedAlphaGroups(), LLSpatialGroup::CompareDepthGreaterRiggedRun());
    }
    }

    LL_PUSH_CALLSTACKS();
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("beacon rendering flags");
    // only render if the flag is set. The flag is only set if we are in edit mode or the toggle is set in the menus
    // <FS:Ansariel> Make beacons also show when beacons floater is closed.
    if (/*LLFloaterReg::instanceVisible("beacons") &&*/ !isFrameShadowPass() && !gCubeSnapshot)
    {
        if (sRenderScriptedTouchBeacons)
        {
            // Only show the beacon on the root object.
            forAllVisibleDrawables(renderScriptedTouchBeacons);
        }
        else if (sRenderScriptedBeacons)
        {
            // Only show the beacon on the root object.
            forAllVisibleDrawables(renderScriptedBeacons);
        }

        if (sRenderPhysicalBeacons)
        {
            // Only show the beacon on the root object.
            forAllVisibleDrawables(renderPhysicalBeacons);
        }

        if (sRenderMOAPBeacons)
        {
            forAllVisibleDrawables(renderMOAPBeacons);
        }

        if (sRenderParticleBeacons)
        {
            forAllVisibleDrawables(renderParticleBeacons);
        }

        // If god mode, also show audio cues
        if (sRenderSoundBeacons && gAudiop)
        {
            // Walk all sound sources and render out beacons for them. Note, this isn't done in the ForAllVisibleDrawables function, because
            // some are not visible.
            LLAudioEngine::source_map::iterator iter;
            for (iter = gAudiop->mAllSources.begin(); iter != gAudiop->mAllSources.end(); ++iter)
            {
                LLAudioSource *sourcep = iter->second;

                LLVector3d pos_global = sourcep->getPositionGlobal();
                LLVector3  pos        = gAgent.getPosAgentFromGlobal(pos_global);
                if (gPipeline.sRenderBeacons)
                {
                    // pos += LLVector3(0.f, 0.f, 0.2f);
                    gObjectList.addDebugBeacon(pos, "", LLColor4(1.f, 1.f, 0.f, 0.5f), LLColor4(1.f, 1.f, 1.f, 0.5f), DebugBeaconLineWidth);
                }
            }
            // now deal with highlights for all those seeable sound sources
            forAllVisibleDrawables(renderSoundHighlights);
        }

        // <FS:PP> FIRE-33085 Region corner markers
        if (sRenderRegionCornerBeacons)
        {
            LLViewerRegion* region = gAgent.getRegion();
            if (region)
            {
                LLVector3 origin = region->getOriginAgent();
                F32 width = region->getWidth();

                LLVector3 corner1 = origin; // Southwest
                LLVector3 corner2 = origin + LLVector3(width, 0, 0); // Southeast
                LLVector3 corner3 = origin + LLVector3(0, width, 0); // Northwest
                LLVector3 corner4 = origin + LLVector3(width, width, 0); // Northeast

                corner1.mV[VZ] = region->getLandHeightRegion(LLVector3(0, 0, 0));
                corner2.mV[VZ] = region->getLandHeightRegion(LLVector3(width, 0, 0));
                corner3.mV[VZ] = region->getLandHeightRegion(LLVector3(0, width, 0));
                corner4.mV[VZ] = region->getLandHeightRegion(LLVector3(width, width, 0));

                LLColor4 corner_color(1.0f, 1.0f, 0.0f, 0.8f);
                LLColor4 text_color(1.0f, 1.0f, 1.0f, 1.0f);

                gObjectList.addDebugBeacon(corner1, "SW", corner_color, text_color, DebugBeaconLineWidth);
                gObjectList.addDebugBeacon(corner2, "SE", corner_color, text_color, DebugBeaconLineWidth);
                gObjectList.addDebugBeacon(corner3, "NW", corner_color, text_color, DebugBeaconLineWidth);
                gObjectList.addDebugBeacon(corner4, "NE", corner_color, text_color, DebugBeaconLineWidth);
            }
        }
        // </FS:PP>

    }
    }
    LL_PUSH_CALLSTACKS();
    // If managing your telehub, draw beacons at telehub and currently selected spawnpoint.
    if (LLFloaterTelehub::renderBeacons() && !isFrameShadowPass() && !gCubeSnapshot)
    {
        LLFloaterTelehub::addBeacons();
    }

    if (!isFrameShadowPass() && !gCubeSnapshot)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("Render face highlights");
        mSelectedFaces.clear();

        bool tex_index_changed = false;
        if (!gNonInteractive)
        {
            LLRender::eTexIndex tex_index = sRenderHighlightTextureChannel;
            // <FS:Zi> switchable edit texture/materials panel
            //setRenderHighlightTextureChannel(gFloaterTools->getPanelFace()->getTextureChannelToEdit());
            setRenderHighlightTextureChannel(gFloaterTools->getTextureChannelToEdit());
            // </FS:Zi>
            tex_index_changed = sRenderHighlightTextureChannel != tex_index;
        }

        // Draw face highlights for selected faces.
        if (LLSelectMgr::getInstance()->getTEMode())
        {
            struct f : public LLSelectedTEFunctor
            {
                virtual bool apply(LLViewerObject *object, S32 te)
                {
                    if (object->mDrawable)
                    {
                        LLFace *facep = object->mDrawable->getFace(te);
                        if (facep)
                        {
                            gPipeline.mSelectedFaces.push_back(facep);
                        }
                    }
                    return true;
                }
            } func;
            LLSelectMgr::getInstance()->getSelection()->applyToTEs(&func);

            if (tex_index_changed)
            {
                // Rebuild geometry for all selected faces with PBR textures
                for (const LLFace* face : gPipeline.mSelectedFaces)
                {
                    if (const LLViewerObject* vobj = face->getViewerObject())
                    {
                        if (const LLTextureEntry* tep = vobj->getTE(face->getTEOffset()))
                        {
                            if (tep->getGLTFRenderMaterial())
                            {
                                gPipeline.markRebuild(face->getDrawable(), LLDrawable::REBUILD_VOLUME);
                            }
                        }
                    }
                }
            }
        }
    }

    LLVertexBuffer::flushBuffers();
    // LLSpatialGroup::sNoDelete = false;
    LL_PUSH_CALLSTACKS();
}


void render_hud_elements()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI; //LL_RECORD_BLOCK_TIME(FTM_RENDER_UI);
    gPipeline.disableLights();

    LLGLSUIDefault gls_ui;

    gUIProgram.bind();
    gGL.color4f(1, 1, 1, 1);
    LLGLDepthTest depth(GL_TRUE, GL_FALSE);

    if (!isFrameReflectionPass() && gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
    {
        gViewerWindow->renderSelections(false, false, false); // For HUD version in render_ui_3d()

        // Draw the tracking overlays
        LLTracker::render3D();

        if (LLWorld::instanceExists())
        {
            // Show the property lines
            LLWorld::getInstance()->renderPropertyLines();
        }
        LLViewerParcelMgr::getInstance()->render();
        LLViewerParcelMgr::getInstance()->renderParcelCollision();
    }
    else if (gForceRenderLandFence)
    {
        // This is only set when not rendering the UI, for parcel snapshots
        LLViewerParcelMgr::getInstance()->render();
    }
    else if (gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_HUD))
    {
        LLHUDText::renderAllHUD();
    }

    gUIProgram.unbind();
}

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

void LLPipeline::renderSelectedFaces(const LLColor4& color)
{
    if (!mFaceSelectImagep)
    {
        mFaceSelectImagep = LLViewerTextureManager::getFetchedTexture(IMG_FACE_SELECT);
    }

    if (mFaceSelectImagep)
    {
        // Make sure the selection image gets downloaded and decoded
        mFaceSelectImagep->addTextureStats((F32)MAX_IMAGE_AREA);

        for (auto facep : mSelectedFaces)
        {
            if (!facep || !facep->getViewerObject())
            {
                LLSelectMgr::getInstance()->clearSelections();
                return;
            }
            if (!facep->getDrawable() || facep->getDrawable()->isDead())
            {
                LL_ERRS() << "Bad face on selection" << LL_ENDL;
                return;
            }

            facep->renderSelected(mFaceSelectImagep, color);
        }
    }
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

U32 LLPipeline::sCurRenderPoolType = 0 ;

// <AYAstorm r30 P2> Velocity pass (BD lineage). Bind mVelocityMap, clear, run
// each pool's renderMotionBlur(). Step 5 wires the display() callsite; until
// then this stays unreferenced. Pools that don't override the new virtuals
// (Step 4c per-pool override list) contribute zero passes — safe to call
// before any override exists, just produces a cleared RG16F target.
void LLPipeline::renderGeomMotionBlur()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LL_PROFILE_GPU_ZONE("renderGeomMotionBlur");

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

    mVelocityMap.bindTarget();
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
            poolp->beginMotionBlurPass(i);
            poolp->renderMotionBlur(i);
            poolp->endMotionBlurPass(i);
        }
    }

    sVelocityRender = false;

    mVelocityMap.flush();
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

    dst->bindTarget();

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
        std::memcpy(gDeferredMotionBlurProgram.mVkPerProgramUBOMapped,
                    &ubo_data,
                    sizeof(ubo_data));
    }

    mScreenTriangleVB->setBuffer();
    mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

    gDeferredMotionBlurProgram.unbind();
    dst->flush();
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

    src->bindTarget();
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
        std::memcpy(gVolumetricLightProgram.mVkPerProgramUBOMapped,
                    &ubo_data,
                    sizeof(ubo_data));
    }

    mScreenTriangleVB->setBuffer();
    mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

    unbindDeferredShader(gVolumetricLightProgram);
    src->flush();

    gGL.setColorMask(true, true);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);
}
// </AYAstorm r30 P3>

namespace
{
    struct CameraWorkerSeeds
    {
        bool attempted = false;
        bool valid     = false;
        LLGLSLShader::record_seed_map_t map;
    };
    CameraWorkerSeeds sCameraWorkerSeeds;

    std::vector<LLPointer<LLDrawInfo>> sCameraRecordPins;

    struct CameraClearCtx
    {
        VkImage       colors[4] = {};
        VkImageView   color_views[4] = {};
        VkImageLayout color_layouts[4] = {};
        U32           color_count = 0;
        VkImage       depth = VK_NULL_HANDLE;
        VkImageView   depth_view = VK_NULL_HANDLE;
        VkImageLayout depth_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        U32           width = 0;
        U32           height = 0;
        F32           clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
    };

    struct CameraPassCtx
    {
        F32             view[16];
        F32             proj[16];
        F32             last_modelview[16];
        LLCullResult*   cull = nullptr;
        LLPipeline::RenderTargetPack* rt_pack = nullptr;
        LLRenderTarget* rt = nullptr;
        VkImage         colors[4] = {};
        VkImageView     color_views[4] = {};
        U32             color_count = 0;
        VkImage         depth = VK_NULL_HANDLE;
        VkImageView     depth_view = VK_NULL_HANDLE;
        U32             width = 0;
        U32             height = 0;
        LLDrawPool*     pool = nullptr;
        S32             pass = 0;
        const LLGLSLShader::record_seed_map_t* seeds = nullptr;
    };

    void cameraAttachmentTransition(VkImage image, bool is_depth, VkImageLayout old_layout)
    {
        VkPipelineStageFlags src_stage;
        VkAccessFlags        src_access;
        VkImageLayout        new_layout;
        VkImageAspectFlags   aspect;
        if (is_depth)
        {
            new_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            aspect     = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else
        {
            new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            aspect     = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        if (old_layout == new_layout)
        {
            src_stage  = is_depth ? (VkPipelineStageFlags)VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
                                  : (VkPipelineStageFlags)VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            src_access = is_depth ? (VkAccessFlags)VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
                                  : (VkAccessFlags)VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            src_stage  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            src_access = VK_ACCESS_SHADER_READ_BIT;
        }
        else
        {
            src_stage  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            src_access = 0;
        }
        const VkPipelineStageFlags dst_stage = is_depth
            ? (VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT)
            : (VkPipelineStageFlags)VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        const VkAccessFlags dst_access = is_depth
            ? (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
            : (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        LLVKLoader::transitionImageLayoutVk(image, aspect, old_layout, new_layout,
                                            src_stage, dst_stage, src_access, dst_access);
    }

    void recordCameraClearJob(const CameraClearCtx& ctx, VkCommandBuffer cmd)
    {
        LLVKLoader::gpuCheckpoint("rw:camera_clear");
        for (U32 i = 0; i < ctx.color_count; ++i)
        {
            if (ctx.colors[i] != VK_NULL_HANDLE)
            {
                cameraAttachmentTransition(ctx.colors[i], false, ctx.color_layouts[i]);
            }
        }
        if (ctx.depth != VK_NULL_HANDLE)
        {
            cameraAttachmentTransition(ctx.depth, true, ctx.depth_layout);
        }

        LLVKLoader::DynamicRenderingAttachment colors[4] = {};
        for (U32 i = 0; i < ctx.color_count; ++i)
        {
            colors[i].image_view   = ctx.color_views[i];
            colors[i].image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colors[i].load_op      = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colors[i].store_op     = VK_ATTACHMENT_STORE_OP_STORE;
            colors[i].clear_value.color.float32[0] = ctx.clear_color[0];
            colors[i].clear_value.color.float32[1] = ctx.clear_color[1];
            colors[i].clear_value.color.float32[2] = ctx.clear_color[2];
            colors[i].clear_value.color.float32[3] = ctx.clear_color[3];
        }
        LLVKLoader::DynamicRenderingAttachment depth_att = {};
        depth_att.image_view   = ctx.depth_view;
        depth_att.image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_att.load_op      = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_att.store_op     = VK_ATTACHMENT_STORE_OP_STORE;
        depth_att.clear_value.depthStencil.depth   = 1.0f;
        depth_att.clear_value.depthStencil.stencil = 0;

        LLVKLoader::beginDynamicRendering(ctx.width, ctx.height,
                                          ctx.color_count > 0 ? colors : nullptr,
                                          ctx.color_count,
                                          ctx.depth != VK_NULL_HANDLE ? &depth_att : nullptr);
        LLVKLoader::endDynamicRendering();
        LLVKLoader::cmdCameraGbufferBarrierVk(cmd, ctx.colors, ctx.color_count, ctx.depth);
    }

    void recordCameraPoolPass(const CameraPassCtx& ctx, VkCommandBuffer cmd)
    {
        LLVKLoader::gpuCheckpoint("rw:camera_mat");
        LLPipelineFrameContext& fctx = LLPipelineFrameContext::getInstance();
        LLCullResult*   prev_cull     = fctx.getCullResult();
        LLPipeline::RenderTargetPack* prev_rt_pack = fctx.getActiveRT();
        const bool      prev_shadow   = fctx.isShadowPass();
        const bool      prev_deferred = fctx.isRenderingDeferred();
        LLRenderTarget* prev_bound    = LLRenderTarget::sBoundTarget;
        const U32       prev_res_x    = LLRenderTarget::sCurResX;
        const U32       prev_res_y    = LLRenderTarget::sCurResY;
        const U32       prev_pass_tag = LLVKLoader::gVkPerfPassTag;
        const LLGLSLShader::record_seed_map_t* prev_seeds = LLGLSLShader::sRecordSeedMap;

        fctx.setCullResult(ctx.cull);
        fctx.setActiveRT(ctx.rt_pack);
        fctx.setShadowPass(false);
        fctx.setRenderingDeferred(true);
        LLRenderTarget::sBoundTarget = ctx.rt;
        LLRenderTarget::sCurResX = ctx.width;
        LLRenderTarget::sCurResY = ctx.height;
        LLGLSLShader::sRecordSeedMap = ctx.seeds;
        std::memcpy(gGLModelView, ctx.view, sizeof(F32) * 16);
        std::memcpy(gGLLastModelView, ctx.last_modelview, sizeof(F32) * 16);
        gGLLastMatrix = NULL;
        LLVKLoader::setRenderViewport(0, 0, (S32)ctx.width, (S32)ctx.height);
        LLVKLoader::gVkPerfPassTag = 0;

        LLVKLoader::DynamicRenderingAttachment colors[4] = {};
        for (U32 i = 0; i < ctx.color_count; ++i)
        {
            colors[i].image_view   = ctx.color_views[i];
            colors[i].image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colors[i].load_op      = VK_ATTACHMENT_LOAD_OP_LOAD;
            colors[i].store_op     = VK_ATTACHMENT_STORE_OP_STORE;
        }
        LLVKLoader::DynamicRenderingAttachment depth_att = {};
        depth_att.image_view   = ctx.depth_view;
        depth_att.image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_att.load_op      = VK_ATTACHMENT_LOAD_OP_LOAD;
        depth_att.store_op     = VK_ATTACHMENT_STORE_OP_STORE;
        LLVKLoader::beginDynamicRendering(ctx.width, ctx.height,
                                          ctx.color_count > 0 ? colors : nullptr,
                                          ctx.color_count,
                                          ctx.depth_view != VK_NULL_HANDLE ? &depth_att : nullptr);

        {
            LLGLEnable cull_face(GL_CULL_FACE);
            LLGLDisable blend(GL_BLEND);
            LLGLDepthTest depth_test(GL_TRUE, GL_TRUE, GL_LESS);

            const LLRender::eBlendFactor prev_bf_cs = gGL.getCurrBlendColorSFactor();
            const LLRender::eBlendFactor prev_bf_cd = gGL.getCurrBlendColorDFactor();
            const LLRender::eBlendFactor prev_bf_as = gGL.getCurrBlendAlphaSFactor();
            const LLRender::eBlendFactor prev_bf_ad = gGL.getCurrBlendAlphaDFactor();
            gGL.blendFunc(LLRender::BF_ONE, LLRender::BF_ZERO);
            gGL.setColorMask(true, true);

            gGL.matrixMode(LLRender::MM_PROJECTION);
            gGL.pushMatrix();
            gGL.loadMatrix(ctx.proj);
            gGL.matrixMode(LLRender::MM_MODELVIEW);
            gGL.pushMatrix();
            gGL.loadMatrix(ctx.view);
            gGLLastMatrix = NULL;

            LLVertexBuffer::unbind();
            ctx.pool->beginDeferredPass(ctx.pass);
            ctx.pool->renderDeferred(ctx.pass);
            ctx.pool->endDeferredPass(ctx.pass);
            LLVertexBuffer::unbind();

            gGL.matrixMode(LLRender::MM_PROJECTION);
            gGL.popMatrix();
            gGL.matrixMode(LLRender::MM_MODELVIEW);
            gGL.popMatrix();

            if (prev_bf_cs != LLRender::BF_UNDEF && prev_bf_cd != LLRender::BF_UNDEF
                && prev_bf_as != LLRender::BF_UNDEF && prev_bf_ad != LLRender::BF_UNDEF)
            {
                gGL.blendFunc(prev_bf_cs, prev_bf_cd, prev_bf_as, prev_bf_ad);
            }
        }

        LLVKLoader::endDynamicRendering();
        LLVKLoader::cmdCameraGbufferBarrierVk(cmd, ctx.colors, ctx.color_count, ctx.depth);

        LLGLSLShader::sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
        LLGLSLShader::sRecordSeedMap = prev_seeds;
        LLRenderTarget::sBoundTarget = prev_bound;
        LLRenderTarget::sCurResX = prev_res_x;
        LLRenderTarget::sCurResY = prev_res_y;
        fctx.setCullResult(prev_cull);
        fctx.setActiveRT(prev_rt_pack);
        fctx.setShadowPass(prev_shadow);
        fctx.setRenderingDeferred(prev_deferred);
        LLVKLoader::gVkPerfPassTag = prev_pass_tag;
        gGLLastMatrix = NULL;
    }

    void ensureCameraWorkerSeeds(CameraWorkerSeeds& seeds, LLDrawPool* poolp)
    {
        if (seeds.attempted)
        {
            return;
        }
        seeds.attempted = true;
        if (!LLVKLoader::isVulkanInitialized() || !LLVKLoader::isBindlessActiveVk())
        {
            return;
        }
        bool all_ok = true;
        const S32 n = poolp->getNumDeferredPasses();
        for (S32 i = 0; i < n && all_ok; ++i)
        {
            LLVertexBuffer::unbind();
            poolp->beginDeferredPass(i);
            LLGLSLShader* s = LLGLSLShader::sCurBoundShaderPtr;
            if (s == nullptr)
            {
                all_ok = false;
            }
            else
            {
                LLGLSLShader::sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
                LLGLSLShader::sCurPerCallAuthored        = false;
                VkDescriptorSet set = LLGLSLShader::vkResolvePerCallSetForDraw();
                if (set == VK_NULL_HANDLE)
                {
                    all_ok = false;
                }
                else
                {
                    LLGLSLShader::RecordSeed seed;
                    seed.set   = set;
                    seed.shape = LLGLSLShader::sCurPerCallVkSetShape;
                    if (!LLGLSLShader::vkCaptureSeedDynamicBuffers(seed))
                    {
                        all_ok = false;
                    }
                    else
                    {
                        seeds.map[s] = seed;
                    }
                }
            }
            poolp->endDeferredPass(i);
        }
        LLVertexBuffer::unbind();
        seeds.valid = all_ok;
        if (!all_ok)
        {
            seeds.map.clear();
        }
    }

    void pinCameraWorkerDrawInfos()
    {
        static const U32 mat_types[] = {
            LLRenderPass::PASS_MATERIAL,
            LLRenderPass::PASS_MATERIAL_ALPHA_MASK,
            LLRenderPass::PASS_MATERIAL_ALPHA_EMISSIVE,
            LLRenderPass::PASS_SPECMAP,
            LLRenderPass::PASS_SPECMAP_MASK,
            LLRenderPass::PASS_SPECMAP_EMISSIVE,
            LLRenderPass::PASS_NORMMAP,
            LLRenderPass::PASS_NORMMAP_MASK,
            LLRenderPass::PASS_NORMMAP_EMISSIVE,
            LLRenderPass::PASS_NORMSPEC,
            LLRenderPass::PASS_NORMSPEC_MASK,
            LLRenderPass::PASS_NORMSPEC_EMISSIVE,
        };
        for (U32 base : mat_types)
        {
            for (U32 type = base; type <= base + 1; ++type)
            {
                LLVKBucket::forEachSource(type, [](LLDrawInfo& params)
                {
                    sCameraRecordPins.emplace_back(&params);
                    if (params.mAvatar.notNull() && params.mSkinInfo != nullptr)
                    {
                        params.mAvatar->updateSkinInfoMatrixPalette(params.mSkinInfo);
                    }
                });
            }
        }
    }
}

bool LLPipeline::beginCameraRecordSplit()
{
    mCameraRecordSplitActive = false;
    sCameraWorkerSeeds = CameraWorkerSeeds();
    sCameraRecordPins.clear();

    static LLCachedControl<bool> camera_record_mt(gSavedSettings, "AYACameraRecordMT", true);
    {
        static bool s_prev_camera_record_mt = true;
        if (s_prev_camera_record_mt != (bool)camera_record_mt)
        {
            s_prev_camera_record_mt = camera_record_mt;
            LL_INFOS("VkPerf") << "AYACameraRecordMT -> " << (s_prev_camera_record_mt ? 1 : 0) << LL_ENDL;
        }
    }
    static LLCachedControl<bool> render_depth_pre_pass(gSavedSettings, "RenderDepthPrePass", false);

    if (!camera_record_mt
        || render_depth_pre_pass
        || gCubeSnapshot
        || !LLVKLoader::isVulkanInitialized()
        || !LLVKLoader::shouldUseVulkanRender()
        || !LLVKLoader::isBindlessActiveVk())
    {
        return false;
    }

    LLRenderTarget& rt = getFrameRT()->deferredScreen;
    const U32 color_count = llmin(rt.getNumTextures(), 4u);
    if (color_count == 0 || !rt.hasVkDepth() || rt.getVkDepthView() == VK_NULL_HANDLE)
    {
        return false;
    }

    CameraClearCtx ctx;
    ctx.color_count = color_count;
    for (U32 i = 0; i < color_count; ++i)
    {
        if (!rt.hasVkImage(i) || rt.getVkAttachmentView(i) == VK_NULL_HANDLE)
        {
            return false;
        }
        ctx.colors[i]        = rt.getVkImage(i);
        ctx.color_views[i]   = rt.getVkAttachmentView(i);
        ctx.color_layouts[i] = rt.getVkTexLayout(i);
    }
    ctx.depth        = rt.getVkDepthImage();
    ctx.depth_view   = rt.getVkDepthView();
    ctx.depth_layout = rt.getVkDepthLayout();
    ctx.width        = rt.getWidth();
    ctx.height       = rt.getHeight();
    const F32* cc = gGL.getClearColor();
    ctx.clear_color[0] = cc[0];
    ctx.clear_color[1] = cc[1];
    ctx.clear_color[2] = cc[2];
    ctx.clear_color[3] = cc[3];

    if (!LLVKLoader::dispatchRecordJob([ctx](VkCommandBuffer cmd) { recordCameraClearJob(ctx, cmd); }))
    {
        return false;
    }

    for (U32 i = 0; i < color_count; ++i)
    {
        rt.setVkTexLayout(i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }
    rt.setVkDepthLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    mCameraRecordSplitActive = true;
    return true;
}

void LLPipeline::finishCameraRecordSplit()
{
    if (!mCameraRecordSplitActive)
    {
        return;
    }
    LLVKLoader::joinRecordJobs();
    sCameraRecordPins.clear();
    mCameraRecordSplitActive = false;
}

void LLPipeline::renderGeomDeferred(LLCamera& camera, bool do_occlusion)
{
    LLAppViewer::instance()->pingMainloopTimeout("Pipeline:RenderGeomDeferred");
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

    bool occlude = LLPipeline::sUseOcclusion > 1 && do_occlusion && !LLGLSLShader::sProfileEnabled;

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
            bool mt_materials = mCameraRecordSplitActive
                                && cur_type == LLDrawPool::POOL_MATERIALS
                                && &camera == LLViewerCamera::getInstance()
                                && hasRenderType(poolp->getType())
                                && poolp->getNumDeferredPasses() > 0;
            if (mt_materials)
            {
                ensureCameraWorkerSeeds(sCameraWorkerSeeds, poolp);
                mt_materials = sCameraWorkerSeeds.valid;
            }
            if (mt_materials)
            {
                const bool fam_track = !gCubeSnapshot && !gHeroProbeMirrorRender
                                       && cur_type < LLDrawPool::NUM_POOL_TYPES;
                const U64 fam_t0 = fam_track ? (U64)LLTimer::getTotalTime() : 0;
                const U32 fam_d0 = LLVertexBuffer::sVkDrawCallCount.load();

                pinCameraWorkerDrawInfos();

                LLRenderTarget& mt_rt = getFrameRT()->deferredScreen;
                CameraPassCtx base;
                std::memcpy(base.view, gGLModelView, sizeof(base.view));
                std::memcpy(base.proj, gGLProjection, sizeof(base.proj));
                std::memcpy(base.last_modelview, gGLLastModelView, sizeof(base.last_modelview));
                base.cull        = LLPipelineFrameContext::getInstance().getCullResult();
                base.rt_pack     = LLPipelineFrameContext::getInstance().getActiveRT();
                base.rt          = &mt_rt;
                base.color_count = llmin(mt_rt.getNumTextures(), 4u);
                for (U32 ci = 0; ci < base.color_count; ++ci)
                {
                    base.colors[ci]      = mt_rt.getVkImage(ci);
                    base.color_views[ci] = mt_rt.getVkAttachmentView(ci);
                }
                base.depth      = mt_rt.getVkDepthImage();
                base.depth_view = mt_rt.getVkDepthView();
                base.width      = mt_rt.getWidth();
                base.height     = mt_rt.getHeight();
                base.pool       = poolp;
                base.seeds      = &sCameraWorkerSeeds.map;

                const S32 n_passes = poolp->getNumDeferredPasses();
                S32 inline_from = n_passes;
                for (S32 i = 0; i < n_passes; ++i)
                {
                    CameraPassCtx ctx = base;
                    ctx.pass = i;
                    if (!LLVKLoader::dispatchRecordJob([ctx](VkCommandBuffer cmd) { recordCameraPoolPass(ctx, cmd); }))
                    {
                        inline_from = i;
                        break;
                    }
                }
                for (S32 i = inline_from; i < n_passes; ++i)
                {
                    LLVertexBuffer::unbind();
                    poolp->beginDeferredPass(i);
                    if (!poolp->getSkipRenderFlag()) { poolp->renderDeferred(i); }
                    poolp->endDeferredPass(i);
                    LLVertexBuffer::unbind();
                }

                gGLLastMatrix = NULL;
                gGL.loadMatrix(gGLModelView);

                if (fam_track)
                {
                    LLVKLoader::gVkPerf.fam_us[cur_type] += (U64)LLTimer::getTotalTime() - fam_t0;
                    LLVKLoader::gVkPerf.fam_draws[cur_type] += LLVertexBuffer::sVkDrawCallCount.load() - fam_d0;
                }

                for (iter2 = iter1; iter2 != mPools.end(); iter2++)
                {
                    LLDrawPool *p = *iter2;
                    if (p->getType() != cur_type)
                    {
                        break;
                    }
                }
            }
            else if (hasRenderType(poolp->getType()) && poolp->getNumDeferredPasses() > 0)
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
                    poolp->beginDeferredPass(i);
                    for (iter2 = iter1; iter2 != mPools.end(); iter2++)
                    {
                        LLDrawPool *p = *iter2;
                        if (p->getType() != cur_type)
                        {
                            break;
                        }

                        if ( !p->getSkipRenderFlag() ) { p->renderDeferred(i); }
                    }
                    poolp->endDeferredPass(i);
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

    getFrameRT()->screen.bindTarget();

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

    getFrameRT()->screen.flush();
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

// Render all of our geometry that's required after our deferred pass.
// This is gonna be stuff like alpha, water, etc.
void LLPipeline::renderGeomPostDeferred(LLCamera& camera)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LL_PROFILE_GPU_ZONE("renderGeomPostDeferred");
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
            doAtmospherics();
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
                doGodrays();
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
                poolp->beginPostDeferredPass(i);
                for (iter2 = iter1; iter2 != mPools.end(); iter2++)
                {
                    LLDrawPool *p = *iter2;
                    if (p->getType() != cur_type)
                    {
                        break;
                    }

                    p->renderPostDeferred(i);
                }
                poolp->endPostDeferredPass(i);
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

void LLPipeline::renderGeomShadow(LLCamera& camera)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LL_PROFILE_GPU_ZONE("renderGeomShadow");
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
            poolp->prerender() ;

            gGLLastMatrix = NULL;
            gGL.loadMatrix(gGLModelView);

            for( S32 i = 0; i < poolp->getNumShadowPasses(); i++ )
            {
                LLVertexBuffer::unbind();
                poolp->beginShadowPass(i);
                for (iter2 = iter1; iter2 != mPools.end(); iter2++)
                {
                    LLDrawPool *p = *iter2;
                    if (p->getType() != cur_type)
                    {
                        break;
                    }

                    p->renderShadow(i);
                }
                poolp->endShadowPass(i);
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


static U32 sIndicesDrawnCount = 0;

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

// <FS:Beq> Rework Snapshot Guide Rendering
void LLPipeline::renderSnapshotGuidesOverlay()
{
    if (!mSnapshotGuideState.active || !mSnapshotGuideState.show_guides)
    {
        mSnapshotGuideState.active = false;
        return;
    }

    if (!gViewerWindow || !gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
    {
        mSnapshotGuideState.active = false;
        return;
    }

    LLRect view_rect = gViewerWindow->getWorldViewRectRaw();
    const F32 width = (F32)view_rect.getWidth();
    const F32 height = (F32)view_rect.getHeight();
    if (width <= 0.f || height <= 0.f)
    {
        mSnapshotGuideState.active = false;
        return;
    }

    const F32 left_norm = llmin(mSnapshotGuideState.left, mSnapshotGuideState.right);
    const F32 right_norm = llmax(mSnapshotGuideState.left, mSnapshotGuideState.right);
    const F32 bottom_norm = llmin(mSnapshotGuideState.bottom, mSnapshotGuideState.top);
    const F32 top_norm = llmax(mSnapshotGuideState.bottom, mSnapshotGuideState.top);

    const F32 left_px = left_norm * width;
    const F32 right_px = right_norm * width;
    const F32 bottom_px = bottom_norm * height;
    const F32 top_px = top_norm * height;

    const F32 frame_width = right_px - left_px;
    const F32 frame_height = top_px - bottom_px;
    if (frame_width <= 0.f || frame_height <= 0.f)
    {
        mSnapshotGuideState.active = false;
        return;
    }

    const F32 alpha = llclamp(mSnapshotGuideState.visibility, 0.f, 1.f);
    if (alpha <= 0.f)
    {
        mSnapshotGuideState.active = false;
        return;
    }

    LLGLDisable depth(GL_DEPTH_TEST);
    LLGLDisable cull(GL_CULL_FACE);
    LLGLDisable stencil(GL_STENCIL_TEST);
    LLGLEnable blend(GL_BLEND);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    LLGLSLShader* ui_shader = &gUIProgram;
    ui_shader->bind();

    if (!LLViewerFetchedTexture::sWhiteImagep.isNull())
    {
        gGL.getTexUnit(0)->bind(LLViewerFetchedTexture::sWhiteImagep);
    }

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.pushMatrix();
    gGL.loadIdentity();
    gGL.ortho(0.f, width, 0.f, height, -1.f, 1.f);

    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.pushMatrix();
    gGL.loadIdentity();
    gGLLastMatrix = nullptr;

    const LLColor4 line_color(mSnapshotGuideState.color, alpha);
    gGL.color4fv(line_color.mV);

    const F32 thickness = llmax(mSnapshotGuideState.thickness, 0.f);
    const F32 half_thickness = thickness * 0.5f;
    auto draw_filled_rect = [&](F32 l, F32 b, F32 r, F32 t)
    {
        const S32 left_i = ll_round(l);
        const S32 right_i = ll_round(r);
        const S32 top_i = ll_round(t);
        const S32 bottom_i = ll_round(b);
        gl_rect_2d(left_i, top_i, right_i, bottom_i, line_color, true);
    };

    auto draw_vertical_norm = [&](F32 norm)
    {
        const F32 x = left_px + frame_width * norm;
        draw_filled_rect(x - half_thickness, bottom_px, x + half_thickness, top_px);
    };

    auto draw_horizontal_norm = [&](F32 norm)
    {
        const F32 y = bottom_px + frame_height * norm;
        draw_filled_rect(left_px, y - half_thickness, right_px, y + half_thickness);
    };

    switch (mSnapshotGuideState.style)
    {
        case SnapshotGuideState::Style::RuleOfThirds:
        {
            constexpr std::array<F32, 2> offsets = { 1.f / 3.f, 2.f / 3.f };
            for (F32 offset : offsets)
            {
                draw_vertical_norm(offset);
                draw_horizontal_norm(offset);
            }
            break;
        }
        case SnapshotGuideState::Style::GoldenRatio:
        {
            constexpr F32                               phi         = 1.61803398875f;
            const SnapshotGuideState::GoldenOrientation orientation = mSnapshotGuideState.golden_orientation;

            const F32 scale = llmin(frame_width / phi, frame_height);
            if (scale <= 0.f)
            {
                break;
            }

            const F32 golden_width  = phi * scale;
            const F32 golden_height = scale;
            const F32 pad_x         = frame_width - golden_width;
            const F32 pad_y         = frame_height - golden_height;

            F32 anchor_x = left_px;
            F32 anchor_y = bottom_px;
            switch (orientation)
            {
                case SnapshotGuideState::GoldenOrientation::TopLeft:
                    anchor_y += pad_y;
                    break;
                case SnapshotGuideState::GoldenOrientation::TopRight:
                    anchor_x += pad_x;
                    anchor_y += pad_y;
                    break;
                case SnapshotGuideState::GoldenOrientation::BottomRight:
                    anchor_x += pad_x;
                    break;
                case SnapshotGuideState::GoldenOrientation::BottomLeft:
                default:
                    break;
            }

            auto map_point = [&](F32 local_x, F32 local_y) -> LLVector2
            {
                F32 x = local_x;
                F32 y = local_y;

                if (orientation == SnapshotGuideState::GoldenOrientation::TopLeft ||
                    orientation == SnapshotGuideState::GoldenOrientation::BottomLeft)
                {
                    x = golden_width - local_x;
                }

                if (orientation == SnapshotGuideState::GoldenOrientation::BottomLeft ||
                    orientation == SnapshotGuideState::GoldenOrientation::BottomRight)
                {
                    y = golden_height - local_y;
                }

                return LLVector2(anchor_x + x, anchor_y + y);
            };

            std::vector<std::pair<LLVector2, LLVector2>> line_segments;
            line_segments.reserve(24);

            auto add_line = [&](F32 x0, F32 y0, F32 x1, F32 y1)
            {
                line_segments.emplace_back(map_point(x0, y0), map_point(x1, y1));
            };

            // Outline of the fitted golden rectangle.
            add_line(0.f, 0.f, golden_width, 0.f);
            add_line(0.f, golden_height, golden_width, golden_height);
            add_line(0.f, 0.f, 0.f, golden_height);
            add_line(golden_width, 0.f, golden_width, golden_height);

            // Generate subdivision lines while we walk the squares.
            F32 x0 = 0.f;
            F32 y0 = 0.f;
            F32 x1 = golden_width;
            F32 y1 = golden_height;

            for (U32 step = 0; step < 12; ++step)
            {
                const F32 width  = x1 - x0;
                const F32 height = y1 - y0;
                if (width <= 1.f || height <= 1.f)
                {
                    break;
                }

                switch (step % 4)
                {
                    case 0:
                        x0 += height;
                        add_line(x0, y0, x0, y1);
                        break;
                    case 1:
                        y0 += width;
                        add_line(x0, y0, x1, y0);
                        break;
                    case 2:
                        x1 -= height;
                        add_line(x1, y0, x1, y1);
                        break;
                    default:
                        y1 -= width;
                        add_line(x0, y1, x1, y1);
                        break;
                }
            }

            auto draw_golden_spiral = [&](U32 max_depth)
            {
                gGL.begin(LLRender::LINE_STRIP);

                F32 spiral_x0 = 0.f;
                F32 spiral_y0 = 0.f;
                F32 spiral_x1 = golden_width;
                F32 spiral_y1 = golden_height;

                for (U32 step = 0; step < max_depth; ++step)
                {
                    const F32 width  = spiral_x1 - spiral_x0;
                    const F32 height = spiral_y1 - spiral_y0;
                    if (width <= 1.f || height <= 1.f)
                    {
                        break;
                    }

                    F32 size        = 0.f;
                    F32 cx          = 0.f;
                    F32 cy          = 0.f;
                    F32 start_angle = 0.f;
                    F32 end_angle   = 0.f;

                    switch (step % 4)
                    {
                        case 0: // left square
                            size        = height;
                            cx          = spiral_x0 + size;
                            cy          = spiral_y0 + size;
                            start_angle = F_PI;
                            end_angle   = 1.5f * F_PI;
                            spiral_x0 += size;
                            break;
                        case 1: // bottom square
                            size        = width;
                            cx          = spiral_x0;
                            cy          = spiral_y0 + size;
                            start_angle = 1.5f * F_PI;
                            end_angle   = 2.f * F_PI;
                            spiral_y0 += size;
                            break;
                        case 2: // right square
                            size        = height;
                            cx          = spiral_x1 - size;
                            cy          = spiral_y0;
                            start_angle = 0.f;
                            end_angle   = F_PI_BY_TWO;
                            spiral_x1 -= size;
                            break;
                        case 3: // top square
                        default:
                            size        = width;
                            cx          = spiral_x0 + size;
                            cy          = spiral_y1 - size;
                            start_angle = F_PI_BY_TWO;
                            end_angle   = F_PI;
                            spiral_y1 -= size;
                            break;
                    }

                    if (size <= 0.f)
                    {
                        break;
                    }

                    const S32 segments = llclamp((S32)(size / 4.f), 12, 64);
                    for (S32 i = 0; i <= segments; ++i)
                    {
                        const F32 t       = start_angle + (end_angle - start_angle) * (F32)i / (F32)segments;
                        const F32 local_x = cx + cosf(t) * size;
                        const F32 local_y = cy + sinf(t) * size;
                        LLVector2 mapped  = map_point(local_x, local_y);
                        gGL.vertex2f(mapped.mV[0], mapped.mV[1]);
                    }
                }

                gGL.end();
            };

            gGL.flush();
            const F32 line_width = llmax(thickness, 1.f);
            gGL.setLineWidth(line_width);
            draw_golden_spiral(12);
            gGL.setLineWidth(1.f);

            if (!line_segments.empty())
            {
                gGL.flush();
                gGL.setLineWidth(line_width);
                gGL.begin(LLRender::LINES);
                for (const auto& segment : line_segments)
                {
                    gGL.vertex2f(segment.first.mV[VX], segment.first.mV[VY]);
                    gGL.vertex2f(segment.second.mV[VX], segment.second.mV[VY]);
                }
                gGL.end();
                gGL.setLineWidth(1.f);
            }
            break;
        }
        case SnapshotGuideState::Style::Diagonal:
        {
            const F32 line_width = llmax(thickness, 1.f);
            gGL.flush();
            gGL.setLineWidth(line_width);
            gGL.begin(LLRender::LINES);
            gGL.vertex2f(left_px, bottom_px);
            gGL.vertex2f(right_px, top_px);
            gGL.vertex2f(left_px, top_px);
            gGL.vertex2f(right_px, bottom_px);
            gGL.end();
            gGL.setLineWidth(1.f);
            break;
        }
    }

    gGL.matrixMode(LLRender::MM_MODELVIEW);
    gGL.popMatrix();
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.popMatrix();
    gGLLastMatrix = nullptr;

    ui_shader->unbind();

    mSnapshotGuideState.active = false;
}
// </FS:Beq>

// <FS:Beq> FIRE-32023 Focus Point Rendering
void LLPipeline::renderFocusPoint()
{
    static LLCachedControl<bool> render_focus_point_crosshair(gSavedSettings, "FSFocusPointRender", false);
    if (isFrameDoFPass() && render_focus_point_crosshair && gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
    {
        gDebugProgram.bind();
        LLVector3 focus_point = sLastFocusPoint;
        F32 size = 0.02f;
        LLGLDepthTest gls_depth(GL_FALSE);
        gGL.pushMatrix();
        gGL.translatef(focus_point.mV[VX], focus_point.mV[VY], focus_point.mV[VZ]);
           
        gGL.begin(LLRender::LINES);
        if (LLPipeline::FSFocusPointLocked)
        {
            gGL.color4f(1.0f, 0.0f, 0.0f, 0.5f);
        }
        else
        {
            gGL.color4f(1.0f, 1.0f, 0.0f, 0.5f);
        }
        gGL.vertex3f(-size, 0.0f, 0.0f);
        gGL.vertex3f(size, 0.0f, 0.0f);

        // Y-axis (Green)
        gGL.vertex3f(0.0f, -size, 0.0f);
        gGL.vertex3f(0.0f, size, 0.0f);

        // Z-axis (Blue)
        gGL.vertex3f(0.0f, 0.0f, -size);
        gGL.vertex3f(0.0f, 0.0f, size);
    
        gGL.end();

        gGL.popMatrix();
        gGL.flush();
        gDebugProgram.unbind();
    }
}
// </FS:Beq>

void LLPipeline::renderPhysicsDisplay()
{
    if (!hasRenderDebugMask(LLPipeline::RENDER_DEBUG_PHYSICS_SHAPES))
    {
        return;
    }

    gGL.flush();
    gDebugProgram.bind();

    LLGLEnable polygon_offset_line(GL_POLYGON_OFFSET_LINE);
    gGL.setPolygonOffset(3.f, 3.f);
    gGL.setLineWidth(3.f);
    LLGLEnable blend(GL_BLEND);
    gGL.setSceneBlendType(LLRender::BT_ALPHA);

    for (int pass = 0; pass < 3; ++pass)
    {
        // pass 0 - depth write enabled, color write disabled, fill
        // pass 1 - depth write disabled, color write enabled, fill
        // pass 2 - depth write disabled, color write enabled, wireframe
        gGL.setColorMask(pass >= 1, false);
        LLGLDepthTest depth(GL_TRUE, pass == 0);

        bool wireframe = (pass == 2);

        if (wireframe)
        {
            LLGLState::setPolygonMode(GL_LINE);
        }

        for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
            iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
        {
            LLViewerRegion* region = *iter;
            for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
            {
                LLSpatialPartition* part = region->getSpatialPartition(i);
                if (part)
                {
                    if (hasRenderType(part->mDrawableType))
                    {
                        part->renderPhysicsShapes(wireframe);
                    }
                }
            }
        }
        gGL.flush();

        if (wireframe)
        {
            LLGLState::setPolygonMode(GL_FILL);
        }
    }
    gGL.setLineWidth(1.f);
    gDebugProgram.unbind();

}

extern std::set<LLSpatialGroup*> visible_selected_groups;

void LLPipeline::renderDebug()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    assertInitialized();

    bool hud_only = hasRenderType(LLPipeline::RENDER_TYPE_HUD);

    if (!hud_only )
    {
        //Render any navmesh geometry
        LLPathingLib *llPathingLibInstance = LLPathingLib::getInstance();
        if ( llPathingLibInstance != NULL )
        {
            //character floater renderables

            LLHandle<LLFloaterPathfindingCharacters> pathfindingCharacterHandle = LLFloaterPathfindingCharacters::getInstanceHandle();
            if ( !pathfindingCharacterHandle.isDead() )
            {
                LLFloaterPathfindingCharacters *pathfindingCharacter = pathfindingCharacterHandle.get();

                if ( pathfindingCharacter->getVisible() || gAgentCamera.cameraMouselook() )
                {
                    gPathfindingProgram.bind();

                    if (LLVKLoader::isVulkanInitialized()
                        && gPathfindingProgram.mVkPerProgramUBO != VK_NULL_HANDLE
                        && gPathfindingProgram.mVkPerProgramUBOMapped != nullptr)
                    {
                        LLVKLoader::Pathfinding_PerProgramBind ubo_data = {};
                        ubo_data.tint        = 1.f;
                        ubo_data.ambiance    = 1.f;
                        ubo_data.alpha_scale = 1.f;
                        memcpy(gPathfindingProgram.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
                    }

                    //Requried character physics capsule render parameters
                    LLUUID id;
                    LLVector3 pos;
                    LLQuaternion rot;

                    if ( pathfindingCharacter->isPhysicsCapsuleEnabled( id, pos, rot ) )
                    {
                        //remove blending artifacts
                        gGL.setColorMask(false, false);
                        llPathingLibInstance->renderSimpleShapeCapsuleID( gGL, id, pos, rot );
                        gGL.setColorMask(true, false);
                        LLGLEnable blend(GL_BLEND);
                        llPathingLibInstance->renderSimpleShapeCapsuleID( gGL, id, pos, rot );
                        gPathfindingProgram.bind();
                        if (LLVKLoader::isVulkanInitialized()
                            && gPathfindingProgram.mVkPerProgramUBO != VK_NULL_HANDLE
                            && gPathfindingProgram.mVkPerProgramUBOMapped != nullptr)
                        {
                            LLVKLoader::Pathfinding_PerProgramBind ubo_data = {};
                            ubo_data.tint        = 1.f;
                            ubo_data.ambiance    = 1.f;
                            ubo_data.alpha_scale = 0.90f;
                            memcpy(gPathfindingProgram.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
                        }
                    }
                }
            }


            //pathing console renderables
            LLHandle<LLFloaterPathfindingConsole> pathfindingConsoleHandle = LLFloaterPathfindingConsole::getInstanceHandle();
            if (!pathfindingConsoleHandle.isDead())
            {
                LLFloaterPathfindingConsole *pathfindingConsole = pathfindingConsoleHandle.get();

                if ( pathfindingConsole->getVisible() || gAgentCamera.cameraMouselook() )
                {
                    F32 ambiance = gSavedSettings.getF32("PathfindingAmbiance");

                    gPathfindingProgram.bind();

                    if (LLVKLoader::isVulkanInitialized()
                        && gPathfindingProgram.mVkPerProgramUBO != VK_NULL_HANDLE
                        && gPathfindingProgram.mVkPerProgramUBOMapped != nullptr)
                    {
                        LLVKLoader::Pathfinding_PerProgramBind ubo_data = {};
                        ubo_data.tint        = 1.f;
                        ubo_data.ambiance    = ambiance;
                        ubo_data.alpha_scale = 1.f;
                        memcpy(gPathfindingProgram.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
                    }

                    if ( !pathfindingConsole->isRenderWorld() )
                    {
                        const LLColor4 clearColor = gSavedSettings.getColor4("PathfindingNavMeshClear");
                        gGL.setColorMask(true, true);
                        gGL.setClearColor(clearColor.mV[0],clearColor.mV[1],clearColor.mV[2],0);
                        LLRenderTarget::clearBoundTarget(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
                        gGL.setColorMask(true, false);
                        LLGLState::setPolygonMode(GL_FILL);
                    }

                    //NavMesh
                    if ( pathfindingConsole->isRenderNavMesh() )
                    {
                        gGL.flush();
                        gGL.setLineWidth(2.0f); // <FS> Line width OGL core profile fix by Rye Mutt
                        LLGLEnable cull(GL_CULL_FACE);
                        LLGLDisable blend(GL_BLEND);

                        if ( pathfindingConsole->isRenderWorld() )
                        {
                            LLGLEnable blend(GL_BLEND);
                            llPathingLibInstance->renderNavMesh();
                        }
                        else
                        {
                            llPathingLibInstance->renderNavMesh();
                        }

                        //render edges
                        gPathfindingNoNormalsProgram.bind();
                        if (LLVKLoader::isVulkanInitialized()
                            && gPathfindingNoNormalsProgram.mVkPerProgramUBO != VK_NULL_HANDLE
                            && gPathfindingNoNormalsProgram.mVkPerProgramUBOMapped != nullptr)
                        {
                            LLVKLoader::Pathfinding_PerProgramBind ubo_data = {};
                            ubo_data.tint        = 1.f;
                            ubo_data.ambiance    = 1.f;
                            ubo_data.alpha_scale = 1.f;
                            memcpy(gPathfindingNoNormalsProgram.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
                        }
                        llPathingLibInstance->renderNavMeshEdges();
                        gPathfindingProgram.bind();
                        if (LLVKLoader::isVulkanInitialized()
                            && gPathfindingProgram.mVkPerProgramUBO != VK_NULL_HANDLE
                            && gPathfindingProgram.mVkPerProgramUBOMapped != nullptr)
                        {
                            LLVKLoader::Pathfinding_PerProgramBind ubo_data = {};
                            ubo_data.tint        = 1.f;
                            ubo_data.ambiance    = ambiance;
                            ubo_data.alpha_scale = 1.f;
                            memcpy(gPathfindingProgram.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
                        }

                        gGL.flush();
                        LLGLState::setPolygonMode(GL_FILL);
                        gGL.setLineWidth(1.0f); // <FS> Line width OGL core profile fix by Rye Mutt
                        gGL.flush();
                    }
                    //User designated path
                    if ( LLPathfindingPathTool::getInstance()->isRenderPath() )
                    {
                        //The path
                        gUIProgram.bind();
                        gGL.getTexUnit(0)->bind(LLViewerFetchedTexture::sWhiteImagep);
                        llPathingLibInstance->renderPath();
                        gPathfindingProgram.bind();
                        if (LLVKLoader::isVulkanInitialized()
                            && gPathfindingProgram.mVkPerProgramUBO != VK_NULL_HANDLE
                            && gPathfindingProgram.mVkPerProgramUBOMapped != nullptr)
                        {
                            LLVKLoader::Pathfinding_PerProgramBind ubo_data = {};
                            ubo_data.tint        = 1.f;
                            ubo_data.ambiance    = ambiance;
                            ubo_data.alpha_scale = 1.f;
                            memcpy(gPathfindingProgram.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
                        }

                        //The bookends
                        //remove blending artifacts
                        gGL.setColorMask(false, false);
                        llPathingLibInstance->renderPathBookend( gGL, LLPathingLib::LLPL_START );
                        llPathingLibInstance->renderPathBookend( gGL, LLPathingLib::LLPL_END );

                        gGL.setColorMask(true, false);
                        //render the bookends
                        LLGLEnable blend(GL_BLEND);
                        llPathingLibInstance->renderPathBookend( gGL, LLPathingLib::LLPL_START );
                        llPathingLibInstance->renderPathBookend( gGL, LLPathingLib::LLPL_END );
                        gPathfindingProgram.bind();
                        if (LLVKLoader::isVulkanInitialized()
                            && gPathfindingProgram.mVkPerProgramUBO != VK_NULL_HANDLE
                            && gPathfindingProgram.mVkPerProgramUBOMapped != nullptr)
                        {
                            LLVKLoader::Pathfinding_PerProgramBind ubo_data = {};
                            ubo_data.tint        = 1.f;
                            ubo_data.ambiance    = ambiance;
                            ubo_data.alpha_scale = 0.90f;
                            memcpy(gPathfindingProgram.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
                        }
                    }

                    if ( pathfindingConsole->isRenderWaterPlane() )
                    {
                        LLGLEnable blend(GL_BLEND);
                        llPathingLibInstance->renderSimpleShapes( gGL, gAgent.getRegion()->getWaterHeight() );
                    }
                //physics/exclusion shapes
                if ( pathfindingConsole->isRenderAnyShapes() )
                {
                        U32 render_order[] = {
                            1 << LLPathingLib::LLST_ObstacleObjects,
                            1 << LLPathingLib::LLST_WalkableObjects,
                            1 << LLPathingLib::LLST_ExclusionPhantoms,
                            1 << LLPathingLib::LLST_MaterialPhantoms,
                        };

                        U32 flags = pathfindingConsole->getRenderShapeFlags();

                        for (U32 i = 0; i < 4; i++)
                        {
                            if (!(flags & render_order[i]))
                            {
                                continue;
                            }

                            //turn off backface culling for volumes so they are visible when camera is inside volume
                            LLGLDisable cull(i >= 2 ? GL_CULL_FACE : 0);

                            gGL.flush();
                            LLGLState::setPolygonMode(GL_FILL);

                            //get rid of some z-fighting
                            LLGLEnable polyOffset(GL_POLYGON_OFFSET_FILL);
                            gGL.setPolygonOffset(1.0f, 1.0f);

                            //render to depth first to avoid blending artifacts
                            gGL.setColorMask(false, false);
                            llPathingLibInstance->renderNavMeshShapesVBO( render_order[i] );
                            gGL.setColorMask(true, false);

                            //get rid of some z-fighting
                            gGL.setPolygonOffset(0.f, 0.f);

                            LLGLEnable blend(GL_BLEND);

                            {
                                { //draw solid overlay
                                    LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_LEQUAL);
                                    llPathingLibInstance->renderNavMeshShapesVBO( render_order[i] );
                                    gGL.flush();
                                }

                                LLGLEnable lineOffset(GL_POLYGON_OFFSET_LINE);
                                LLGLState::setPolygonMode(GL_LINE);

                                F32 offset = gSavedSettings.getF32("PathfindingLineOffset");

                                if (pathfindingConsole->isRenderXRay())
                                {
                                    LLGLEnable blend(GL_BLEND);
                                    LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_GREATER);

                                    gGL.setPolygonOffset(offset, -offset);

                                    if (gSavedSettings.getBOOL("PathfindingXRayWireframe"))
                                    { //draw hidden wireframe as darker and less opaque
                                        llPathingLibInstance->renderNavMeshShapesVBO( render_order[i] );
                                    }
                                    else
                                    {
                                        LLGLState::setPolygonMode(GL_FILL);
                                        llPathingLibInstance->renderNavMeshShapesVBO( render_order[i] );
                                        LLGLState::setPolygonMode(GL_LINE);
                                    }
                                }

                                { //draw visible wireframe as brighter, thicker and more opaque
                                    gGL.setPolygonOffset(offset, offset);

                                    gGL.setLineWidth(gSavedSettings.getF32("PathfindingLineWidth")); // <FS> Line width OGL core profile fix by Rye Mutt
                                    LLGLDisable blendOut(GL_BLEND);
                                    llPathingLibInstance->renderNavMeshShapesVBO( render_order[i] );
                                    gGL.flush();
                                    gGL.setLineWidth(1.f); // <FS> Line width OGL core profile fix by Rye Mutt
                                }

                                LLGLState::setPolygonMode(GL_FILL);
                            }
                        }
                    }

                    gGL.setPolygonOffset(0.f, 0.f);

                    if ( pathfindingConsole->isRenderNavMesh() && pathfindingConsole->isRenderXRay() )
                    {   //render navmesh xray
                        F32 ambiance = gSavedSettings.getF32("PathfindingAmbiance");

                        LLGLEnable lineOffset(GL_POLYGON_OFFSET_LINE);
                        LLGLEnable polyOffset(GL_POLYGON_OFFSET_FILL);

                        F32 offset = gSavedSettings.getF32("PathfindingLineOffset");
                        gGL.setPolygonOffset(offset, -offset);

                        LLGLEnable blend(GL_BLEND);
                        LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_GREATER);
                        gGL.flush();
                        gGL.setLineWidth(2.0f); // <FS> Line width OGL core profile fix by Rye Mutt
                        LLGLEnable cull(GL_CULL_FACE);

                        if (gSavedSettings.getBOOL("PathfindingXRayWireframe"))
                        { //draw hidden wireframe as darker and less opaque
                            LLGLState::setPolygonMode(GL_LINE);
                            llPathingLibInstance->renderNavMesh();
                            LLGLState::setPolygonMode(GL_FILL);
                        }
                        else
                        {
                            llPathingLibInstance->renderNavMesh();
                        }

                        //render edges
                        gPathfindingNoNormalsProgram.bind();
                        if (LLVKLoader::isVulkanInitialized()
                            && gPathfindingNoNormalsProgram.mVkPerProgramUBO != VK_NULL_HANDLE
                            && gPathfindingNoNormalsProgram.mVkPerProgramUBOMapped != nullptr)
                        {
                            LLVKLoader::Pathfinding_PerProgramBind ubo_data = {};
                            ubo_data.tint        = gSavedSettings.getF32("PathfindingXRayTint");
                            ubo_data.ambiance    = 1.f;
                            ubo_data.alpha_scale = gSavedSettings.getF32("PathfindingXRayOpacity");
                            memcpy(gPathfindingNoNormalsProgram.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
                        }
                        llPathingLibInstance->renderNavMeshEdges();
                        gPathfindingProgram.bind();
                        if (LLVKLoader::isVulkanInitialized()
                            && gPathfindingProgram.mVkPerProgramUBO != VK_NULL_HANDLE
                            && gPathfindingProgram.mVkPerProgramUBOMapped != nullptr)
                        {
                            LLVKLoader::Pathfinding_PerProgramBind ubo_data = {};
                            ubo_data.tint        = gSavedSettings.getF32("PathfindingXRayTint");
                            ubo_data.ambiance    = gSavedSettings.getBOOL("PathfindingXRayWireframe") ? 1.f : ambiance;
                            ubo_data.alpha_scale = gSavedSettings.getF32("PathfindingXRayOpacity");
                            memcpy(gPathfindingProgram.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
                        }

                        gGL.flush();
                        gGL.setLineWidth(1.0f); // <FS> Line width OGL core profile fix by Rye Mutt
                    }

                    gGL.setPolygonOffset(0.f, 0.f);

                    gGL.flush();
                    gPathfindingProgram.unbind();
                }
            }
        }
    }

    gGLLastMatrix = NULL;
    gGL.loadMatrix(gGLModelView);
    gGL.setColorMask(true, false);


    if (!hud_only && !mDebugBlips.empty())
    { //render debug blips
        gUIProgram.bind();
        gGL.color4f(1, 1, 1, 1);

        gGL.getTexUnit(0)->bind(LLViewerFetchedTexture::sWhiteImagep, true);

        if (LLVKLoader::isVulkanInitialized() && gUIProgram.mVkPipelineLayout != VK_NULL_HANDLE)
        {
            VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
            if (cmd != VK_NULL_HANDLE)
            {
                F32 ps = 8.f;
                vkCmdPushConstants(cmd, gUIProgram.mVkPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                                   LLVkUboReg::PC_OFF_POINT_SIZE, sizeof(F32), &ps);
            }
        }
        LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_ALWAYS);

        gGL.begin(LLRender::POINTS);
        for (std::list<DebugBlip>::iterator iter = mDebugBlips.begin(); iter != mDebugBlips.end(); )
        {
            DebugBlip& blip = *iter;

            blip.mAge += gFrameIntervalSeconds.value();
            if (blip.mAge > 2.f)
            {
                mDebugBlips.erase(iter++);
            }
            else
            {
                iter++;
            }

            blip.mPosition.mV[2] += gFrameIntervalSeconds.value()*2.f;

            gGL.color4fv(blip.mColor.mV);
            gGL.vertex3fv(blip.mPosition.mV);
        }
        gGL.end();
        gGL.flush();
        if (LLVKLoader::isVulkanInitialized() && gUIProgram.mVkPipelineLayout != VK_NULL_HANDLE)
        {
            VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
            if (cmd != VK_NULL_HANDLE)
            {
                F32 ps = 1.f;
                vkCmdPushConstants(cmd, gUIProgram.mVkPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                                   LLVkUboReg::PC_OFF_POINT_SIZE, sizeof(F32), &ps);
            }
        }
    }

    // Debug stuff.
    if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCTREE |
        LLPipeline::RENDER_DEBUG_OCCLUSION |
        LLPipeline::RENDER_DEBUG_LIGHTS |
        LLPipeline::RENDER_DEBUG_BATCH_SIZE |
        LLPipeline::RENDER_DEBUG_UPDATE_TYPE |
        LLPipeline::RENDER_DEBUG_BBOXES |
        LLPipeline::RENDER_DEBUG_NORMALS |
        LLPipeline::RENDER_DEBUG_POINTS |
        LLPipeline::RENDER_DEBUG_TEXTURE_AREA |
        LLPipeline::RENDER_DEBUG_TEXTURE_ANIM |
        LLPipeline::RENDER_DEBUG_RAYCAST |
        LLPipeline::RENDER_DEBUG_AVATAR_VOLUME |
        LLPipeline::RENDER_DEBUG_AVATAR_JOINTS |
        LLPipeline::RENDER_DEBUG_AGENT_TARGET |
        LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA |
        LLPipeline::RENDER_DEBUG_TEXEL_DENSITY))
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("render debug bridges");

        for (LLViewerRegion* region : LLWorld::getInstance()->getRegionList())
        {
            for (U32 i = 0; i < LLViewerRegion::NUM_PARTITIONS; i++)
            {
                LLSpatialPartition* part = region->getSpatialPartition(i);
                if (part)
                {
                    if ((hud_only && (part->mDrawableType == RENDER_TYPE_HUD || part->mDrawableType == RENDER_TYPE_HUD_PARTICLES)) ||
                        (!hud_only && hasRenderType(part->mDrawableType)))
                    {
                        part->renderDebug();
                    }
                }
            }
        }

        for (LLCullResult::bridge_iterator i = getFrameCull()->beginVisibleBridge(); i != getFrameCull()->endVisibleBridge(); ++i)
        {
            LLSpatialBridge* bridge = *i;
            if (!bridge->isDead() && hasRenderType(bridge->mDrawableType))
            {
                gGL.pushMatrix();
                gGL.multMatrix((F32*)bridge->mDrawable->getRenderMatrix().mMatrix);
                bridge->renderDebug();
                gGL.popMatrix();
            }
        }
    }

    LL::GLTFSceneManager::instance().renderDebug();

    if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_OCCLUSION))
    { //render visible selected group occlusion geometry
        gDebugProgram.bind();
        LLGLDepthTest depth(GL_TRUE, GL_FALSE);
        gGL.diffuseColor3f(1,0,1);
        for (std::set<LLSpatialGroup*>::iterator iter = visible_selected_groups.begin(); iter != visible_selected_groups.end(); ++iter)
        {
            LLSpatialGroup* group = *iter;

            LLVector4a fudge;
            fudge.splat(0.25f); //SG_OCCLUSION_FUDGE

            LLVector4a size;
            const LLVector4a* bounds = group->getBounds();
            size.setAdd(fudge, bounds[1]);

            drawBox(bounds[0], size);
        }
    }

    visible_selected_groups.clear();

    // r13: AYAstorm OBB occlusion debug overlay. Gated on a CachedControl so
    // the per-frame check is a single int load when the toggle is off.
    static LLCachedControl<bool> show_occluders(gSavedSettings, "Stream3DShowOccluders");
    if (show_occluders && !hud_only)
    {
        gDebugProgram.bind();
        LLGLDepthTest depth(GL_TRUE, GL_FALSE);
        LLOcclusionGeometryMgr::instance().renderDebug();
        gDebugProgram.unbind();
    }

    //draw reflection probes and links between them
    if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_REFLECTION_PROBES) && !hud_only)
    {
        mReflectionMapManager.renderDebug();
    }

    static LLCachedControl<bool> render_ref_probe_volumes(gSavedSettings, "RenderReflectionProbeVolumes");
    if (render_ref_probe_volumes && !hud_only)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("probe debug display");

        bindDeferredShader(gReflectionProbeDisplayProgram, NULL);
        mScreenTriangleVB->setBuffer();

        LLGLEnable blend(GL_BLEND);
        LLGLDepthTest depth(GL_FALSE);

        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

        unbindDeferredShader(gReflectionProbeDisplayProgram);
    }

    gUIProgram.bind();

    if (hasRenderDebugMask(LLPipeline::RENDER_DEBUG_RAYCAST) && !hud_only)
    { //draw crosshairs on particle intersection
        if (gDebugRaycastParticle)
        {
            gDebugProgram.bind();

            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

            LLVector3 center(gDebugRaycastParticleIntersection.getF32ptr());
            LLVector3 size(0.1f, 0.1f, 0.1f);

            LLVector3 p[6];

            p[0] = center + size.scaledVec(LLVector3(1,0,0));
            p[1] = center + size.scaledVec(LLVector3(-1,0,0));
            p[2] = center + size.scaledVec(LLVector3(0,1,0));
            p[3] = center + size.scaledVec(LLVector3(0,-1,0));
            p[4] = center + size.scaledVec(LLVector3(0,0,1));
            p[5] = center + size.scaledVec(LLVector3(0,0,-1));

            gGL.begin(LLRender::LINES);
            gGL.diffuseColor3f(1.f, 1.f, 0.f);
            for (U32 i = 0; i < 6; i++)
            {
                gGL.vertex3fv(p[i].mV);
            }
            gGL.end();
            gGL.flush();

            gDebugProgram.unbind();
        }
    }

    if (hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA) && !hud_only)
    {
        LLVertexBuffer::unbind();

        LLGLEnable blend(GL_BLEND);
        LLGLDepthTest depth(true, false);
        LLGLDisable cull(GL_CULL_FACE);

        gGL.color4f(1,1,1,1);
        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

        F32 a = 0.1f;

        F32 col[] =
        {
            1,0,0,a,
            0,1,0,a,
            0,0,1,a,
            1,0,1,a,

            1,1,0,a,
            0,1,1,a,
            1,1,1,a,
            1,0,1,a,
        };

        for (U32 i = 0; i < LLPipeline::kShadowCameraCount; i++)
        {
            LLVector3* frust = mShadowCamera[i].mAgentFrustum;

            if (i >= LLPipeline::kSunShadowCount)
            { //render shadow frusta as volumes
                if (mShadowFrustPoints[i - LLPipeline::kSunShadowCount].empty())
                {
                    continue;
                }

                gGL.color4fv(col+(i - LLPipeline::kSunShadowCount)*4);

                gGL.begin(LLRender::TRIANGLE_STRIP);
                gGL.vertex3fv(frust[0].mV); gGL.vertex3fv(frust[4].mV);
                gGL.vertex3fv(frust[1].mV); gGL.vertex3fv(frust[5].mV);
                gGL.vertex3fv(frust[2].mV); gGL.vertex3fv(frust[6].mV);
                gGL.vertex3fv(frust[3].mV); gGL.vertex3fv(frust[7].mV);
                gGL.vertex3fv(frust[0].mV); gGL.vertex3fv(frust[4].mV);
                gGL.end();


                gGL.begin(LLRender::TRIANGLE_STRIP);
                gGL.vertex3fv(frust[0].mV);
                gGL.vertex3fv(frust[1].mV);
                gGL.vertex3fv(frust[3].mV);
                gGL.vertex3fv(frust[2].mV);
                gGL.end();

                gGL.begin(LLRender::TRIANGLE_STRIP);
                gGL.vertex3fv(frust[4].mV);
                gGL.vertex3fv(frust[5].mV);
                gGL.vertex3fv(frust[7].mV);
                gGL.vertex3fv(frust[6].mV);
                gGL.end();
            }


            if (i < LLPipeline::kSunShadowCount)
            {

                //if (i == 0 || !mShadowFrustPoints[i].empty())
                {
                    //render visible point cloud
                    gGL.flush();
                    if (LLVKLoader::isVulkanInitialized() && gUIProgram.mVkPipelineLayout != VK_NULL_HANDLE)
                    {
                        VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
                        if (cmd != VK_NULL_HANDLE)
                        {
                            F32 ps = 8.f;
                            vkCmdPushConstants(cmd, gUIProgram.mVkPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                                               LLVkUboReg::PC_OFF_POINT_SIZE, sizeof(F32), &ps);
                        }
                    }
                    gGL.begin(LLRender::POINTS);

                    F32* c = col+i*4;
                    gGL.color3fv(c);

                    for (U32 j = 0; j < mShadowFrustPoints[i].size(); ++j)
                        {
                            gGL.vertex3fv(mShadowFrustPoints[i][j].mV);

                        }
                    gGL.end();

                    gGL.flush();
                    if (LLVKLoader::isVulkanInitialized() && gUIProgram.mVkPipelineLayout != VK_NULL_HANDLE)
                    {
                        VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
                        if (cmd != VK_NULL_HANDLE)
                        {
                            F32 ps = 1.f;
                            vkCmdPushConstants(cmd, gUIProgram.mVkPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                                               LLVkUboReg::PC_OFF_POINT_SIZE, sizeof(F32), &ps);
                        }
                    }

                    LLVector3* ext = mShadowExtents[i];
                    LLVector3 pos = (ext[0]+ext[1])*0.5f;
                    LLVector3 size = (ext[1]-ext[0])*0.5f;
                    drawBoxOutline(pos, size);

                    //render camera frustum splits as outlines
                    gGL.begin(LLRender::LINES);
                    gGL.vertex3fv(frust[0].mV); gGL.vertex3fv(frust[1].mV);
                    gGL.vertex3fv(frust[1].mV); gGL.vertex3fv(frust[2].mV);
                    gGL.vertex3fv(frust[2].mV); gGL.vertex3fv(frust[3].mV);
                    gGL.vertex3fv(frust[3].mV); gGL.vertex3fv(frust[0].mV);
                    gGL.vertex3fv(frust[4].mV); gGL.vertex3fv(frust[5].mV);
                    gGL.vertex3fv(frust[5].mV); gGL.vertex3fv(frust[6].mV);
                    gGL.vertex3fv(frust[6].mV); gGL.vertex3fv(frust[7].mV);
                    gGL.vertex3fv(frust[7].mV); gGL.vertex3fv(frust[4].mV);
                    gGL.vertex3fv(frust[0].mV); gGL.vertex3fv(frust[4].mV);
                    gGL.vertex3fv(frust[1].mV); gGL.vertex3fv(frust[5].mV);
                    gGL.vertex3fv(frust[2].mV); gGL.vertex3fv(frust[6].mV);
                    gGL.vertex3fv(frust[3].mV); gGL.vertex3fv(frust[7].mV);
                    gGL.end();
                }
            }

            /*gGL.flush();
            gGL.setLineWidth(16-i*2); // <FS> Line width OGL core profile fix by Rye Mutt
            for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
                    iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
            {
                LLViewerRegion* region = *iter;
                for (U32 j = 0; j < LLViewerRegion::NUM_PARTITIONS; j++)
                {
                    LLSpatialPartition* part = region->getSpatialPartition(j);
                    if (part)
                    {
                        if (hasRenderType(part->mDrawableType))
                        {
                            part->renderIntersectingBBoxes(&mShadowCamera[i]);
                        }
                    }
                }
            }
            gGL.flush();
            gGL.setLineWidth(1.f);*/ // <FS> Line width OGL core profile fix by Rye Mutt
        }
    }

    if (mRenderDebugMask & RENDER_DEBUG_WIND_VECTORS)
    {
        gAgent.getRegion()->mWind.renderVectors();
    }

    if (mRenderDebugMask & RENDER_DEBUG_COMPOSITION)
    {
        // Debug composition layers
        F32 x, y;

        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

        if (gAgent.getRegion())
        {
            gGL.begin(LLRender::POINTS);
            // Draw the composition layer for the region that I'm in.
            for (x = 0; x <= 260; x++)
            {
                for (y = 0; y <= 260; y++)
                {
                    if ((x > 255) || (y > 255))
                    {
                        gGL.color4f(1.f, 0.f, 0.f, 1.f);
                    }
                    else
                    {
                        gGL.color4f(0.f, 0.f, 1.f, 1.f);
                    }
                    F32 z = gAgent.getRegion()->getCompositionXY((S32)x, (S32)y);
                    z *= 5.f;
                    z += 50.f;
                    gGL.vertex3f(x, y, z);
                }
            }
            gGL.end();
        }
    }

    gGL.flush();
    gUIProgram.unbind();
}

void LLPipeline::rebuildPools()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    assertInitialized();

    auto max_count = mPools.size();
    pool_set_t::iterator iter1 = mPools.upper_bound(mLastRebuildPool);
    while(max_count > 0 && mPools.size() > 0) // && num_rebuilds < MAX_REBUILDS)
    {
        if (iter1 == mPools.end())
        {
            iter1 = mPools.begin();
        }
        LLDrawPool* poolp = *iter1;

        if (poolp->isDead())
        {
            mPools.erase(iter1++);
            removeFromQuickLookup( poolp );
            if (poolp == mLastRebuildPool)
            {
                mLastRebuildPool = NULL;
            }
            delete poolp;
        }
        else
        {
            mLastRebuildPool = poolp;
            iter1++;
        }
        max_count--;
    }
}

void LLPipeline::addToQuickLookup( LLDrawPool* new_poolp )
{
    assertInitialized();

    switch( new_poolp->getType() )
    {
    case LLDrawPool::POOL_SIMPLE:
        if (mSimplePool)
        {
            llassert(0);
            LL_WARNS() << "Ignoring duplicate simple pool." << LL_ENDL;
        }
        else
        {
            mSimplePool = (LLRenderPass*) new_poolp;
        }
        break;

    case LLDrawPool::POOL_ALPHA_MASK:
        if (mAlphaMaskPool)
        {
            llassert(0);
            LL_WARNS() << "Ignoring duplicate alpha mask pool." << LL_ENDL;
            break;
        }
        else
        {
            mAlphaMaskPool = (LLRenderPass*) new_poolp;
        }
        break;

    case LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK:
        if (mFullbrightAlphaMaskPool)
        {
            llassert(0);
            LL_WARNS() << "Ignoring duplicate alpha mask pool." << LL_ENDL;
            break;
        }
        else
        {
            mFullbrightAlphaMaskPool = (LLRenderPass*) new_poolp;
        }
        break;

    case LLDrawPool::POOL_GRASS:
        if (mGrassPool)
        {
            llassert(0);
            LL_WARNS() << "Ignoring duplicate grass pool." << LL_ENDL;
        }
        else
        {
            mGrassPool = (LLRenderPass*) new_poolp;
        }
        break;

    case LLDrawPool::POOL_FULLBRIGHT:
        if (mFullbrightPool)
        {
            llassert(0);
            LL_WARNS() << "Ignoring duplicate simple pool." << LL_ENDL;
        }
        else
        {
            mFullbrightPool = (LLRenderPass*) new_poolp;
        }
        break;

    case LLDrawPool::POOL_GLOW:
        if (mGlowPool)
        {
            llassert(0);
            LL_WARNS() << "Ignoring duplicate glow pool." << LL_ENDL;
        }
        else
        {
            mGlowPool = (LLRenderPass*) new_poolp;
        }
        break;

    case LLDrawPool::POOL_TREE:
        mTreePools[ uintptr_t(new_poolp->getTexture()) ] = new_poolp ;
        break;

    case LLDrawPool::POOL_TERRAIN:
        mTerrainPools[ uintptr_t(new_poolp->getTexture()) ] = new_poolp ;
        break;

    case LLDrawPool::POOL_BUMP:
        if (mBumpPool)
        {
            llassert(0);
            LL_WARNS() << "Ignoring duplicate bump pool." << LL_ENDL;
        }
        else
        {
            mBumpPool = new_poolp;
        }
        break;
    case LLDrawPool::POOL_MATERIALS:
        if (mMaterialsPool)
        {
            llassert(0);
            LL_WARNS() << "Ignorning duplicate materials pool." << LL_ENDL;
        }
        else
        {
            mMaterialsPool = new_poolp;
        }
        break;
    case LLDrawPool::POOL_ALPHA_PRE_WATER:
        if( mAlphaPoolPreWater )
        {
            llassert(0);
            LL_WARNS() << "LLPipeline::addPool(): Ignoring duplicate Alpha pre-water pool" << LL_ENDL;
        }
        else
        {
            mAlphaPoolPreWater = (LLDrawPoolAlpha*) new_poolp;
        }
        break;
    case LLDrawPool::POOL_ALPHA_POST_WATER:
        if (mAlphaPoolPostWater)
        {
            llassert(0);
            LL_WARNS() << "LLPipeline::addPool(): Ignoring duplicate Alpha post-water pool" << LL_ENDL;
        }
        else
        {
            mAlphaPoolPostWater = (LLDrawPoolAlpha*)new_poolp;
        }
        break;

    case LLDrawPool::POOL_AVATAR:
    case LLDrawPool::POOL_CONTROL_AV:
        break; // Do nothing

    case LLDrawPool::POOL_SKY:
        if( mSkyPool )
        {
            llassert(0);
            LL_WARNS() << "LLPipeline::addPool(): Ignoring duplicate Sky pool" << LL_ENDL;
        }
        else
        {
            mSkyPool = new_poolp;
        }
        break;

    case LLDrawPool::POOL_WATER:
        if( mWaterPool )
        {
            llassert(0);
            LL_WARNS() << "LLPipeline::addPool(): Ignoring duplicate Water pool" << LL_ENDL;
        }
        else
        {
            mWaterPool = new_poolp;
        }
        break;

    case LLDrawPool::POOL_WL_SKY:
        if( mWLSkyPool )
        {
            llassert(0);
            LL_WARNS() << "LLPipeline::addPool(): Ignoring duplicate WLSky Pool" << LL_ENDL;
        }
        else
        {
            mWLSkyPool = new_poolp;
        }
        break;

    case LLDrawPool::POOL_GLTF_PBR:
        if( mPBROpaquePool )
        {
            llassert(0);
            LL_WARNS() << "LLPipeline::addPool(): Ignoring duplicate PBR Opaque Pool" << LL_ENDL;
        }
        else
        {
            mPBROpaquePool = new_poolp;
        }
        break;

    case LLDrawPool::POOL_GLTF_PBR_ALPHA_MASK:
        if (mPBRAlphaMaskPool)
        {
            llassert(0);
            LL_WARNS() << "LLPipeline::addPool(): Ignoring duplicate PBR Alpha Mask Pool" << LL_ENDL;
        }
        else
        {
            mPBRAlphaMaskPool = new_poolp;
        }
        break;

    case LLDrawPool::POOL_WATEREXCLUSION:
        if (mWaterExclusionPool)
        {
            llassert(0);
            LL_WARNS() << "LLPipeline::addPool(): Ignoring duplicate Water Exclusion Pool" << LL_ENDL;
        }
        else
        {
            mWaterExclusionPool = new_poolp;
        }
        break;

    default:
        llassert(0);
        LL_WARNS() << "Invalid Pool Type in  LLPipeline::addPool()" << LL_ENDL;
        break;
    }
}

void LLPipeline::removePool( LLDrawPool* poolp )
{
    assertInitialized();
    removeFromQuickLookup(poolp);
    mPools.erase(poolp);
    delete poolp;
}

void LLPipeline::removeFromQuickLookup( LLDrawPool* poolp )
{
    assertInitialized();
    switch( poolp->getType() )
    {
    case LLDrawPool::POOL_SIMPLE:
        llassert(mSimplePool == poolp);
        mSimplePool = NULL;
        break;

    case LLDrawPool::POOL_ALPHA_MASK:
        llassert(mAlphaMaskPool == poolp);
        mAlphaMaskPool = NULL;
        break;

    case LLDrawPool::POOL_FULLBRIGHT_ALPHA_MASK:
        llassert(mFullbrightAlphaMaskPool == poolp);
        mFullbrightAlphaMaskPool = NULL;
        break;

    case LLDrawPool::POOL_GRASS:
        llassert(mGrassPool == poolp);
        mGrassPool = NULL;
        break;

    case LLDrawPool::POOL_FULLBRIGHT:
        llassert(mFullbrightPool == poolp);
        mFullbrightPool = NULL;
        break;

    case LLDrawPool::POOL_WL_SKY:
        llassert(mWLSkyPool == poolp);
        mWLSkyPool = NULL;
        break;

    case LLDrawPool::POOL_GLOW:
        llassert(mGlowPool == poolp);
        mGlowPool = NULL;
        break;

    case LLDrawPool::POOL_TREE:
        #ifdef _DEBUG
            {
                bool found = mTreePools.erase( (uintptr_t)poolp->getTexture() );
                llassert( found );
            }
        #else
            mTreePools.erase( (uintptr_t)poolp->getTexture() );
        #endif
        break;

    case LLDrawPool::POOL_TERRAIN:
        #ifdef _DEBUG
            {
                bool found = mTerrainPools.erase( (uintptr_t)poolp->getTexture() );
                llassert( found );
            }
        #else
            mTerrainPools.erase( (uintptr_t)poolp->getTexture() );
        #endif
        break;

    case LLDrawPool::POOL_BUMP:
        llassert( poolp == mBumpPool );
        mBumpPool = NULL;
        break;

    case LLDrawPool::POOL_MATERIALS:
        llassert(poolp == mMaterialsPool);
        mMaterialsPool = NULL;
        break;

    case LLDrawPool::POOL_ALPHA_PRE_WATER:
        llassert( poolp == mAlphaPoolPreWater );
        mAlphaPoolPreWater = nullptr;
        break;

    case LLDrawPool::POOL_ALPHA_POST_WATER:
        llassert(poolp == mAlphaPoolPostWater);
        mAlphaPoolPostWater = nullptr;
        break;

    case LLDrawPool::POOL_AVATAR:
    case LLDrawPool::POOL_CONTROL_AV:
        break; // Do nothing

    case LLDrawPool::POOL_SKY:
        llassert( poolp == mSkyPool );
        mSkyPool = NULL;
        break;

    case LLDrawPool::POOL_WATER:
        llassert( poolp == mWaterPool );
        mWaterPool = NULL;
        break;

    case LLDrawPool::POOL_GLTF_PBR:
        llassert( poolp == mPBROpaquePool );
        mPBROpaquePool = NULL;
        break;

    case LLDrawPool::POOL_GLTF_PBR_ALPHA_MASK:
        llassert(poolp == mPBRAlphaMaskPool);
        mPBRAlphaMaskPool = NULL;
        break;

    case LLDrawPool::POOL_WATEREXCLUSION:
        llassert(poolp == mWaterExclusionPool);
        mWaterExclusionPool = nullptr;
        break;

    default:
        llassert(0);
        LL_WARNS() << "Invalid Pool Type in  LLPipeline::removeFromQuickLookup() type=" << poolp->getType() << LL_ENDL;
        break;
    }
}

void LLPipeline::resetDrawOrders()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    assertInitialized();
    // Iterate through all of the draw pools and rebuild them.
    for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
    {
        LLDrawPool *poolp = *iter;
        poolp->resetDrawOrders();
    }
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


class LLMenuItemGL;
class LLInvFVBridge;
struct cat_folder_pair;
class LLVOBranch;
class LLVOLeaf;

void LLPipeline::findReferences(LLDrawable *drawablep)
{
    assertInitialized();
    if (mLights.find(drawablep) != mLights.end())
    {
        LL_INFOS() << "In mLights" << LL_ENDL;
    }
    if (std::find(mMovedList.begin(), mMovedList.end(), drawablep) != mMovedList.end())
    {
        LL_INFOS() << "In mMovedList" << LL_ENDL;
    }
    if (std::find(mShiftList.begin(), mShiftList.end(), drawablep) != mShiftList.end())
    {
        LL_INFOS() << "In mShiftList" << LL_ENDL;
    }
    if (mRetexturedList.find(drawablep) != mRetexturedList.end())
    {
        LL_INFOS() << "In mRetexturedList" << LL_ENDL;
    }

    if (std::find(mBuildQ1.begin(), mBuildQ1.end(), drawablep) != mBuildQ1.end())
    {
        LL_INFOS() << "In mBuildQ1" << LL_ENDL;
    }

    S32 count;

    count = gObjectList.findReferences(drawablep);
    if (count)
    {
        LL_INFOS() << "In other drawables: " << count << " references" << LL_ENDL;
    }
}

bool LLPipeline::verify()
{
    bool ok = assertInitialized();
    if (ok)
    {
        for (pool_set_t::iterator iter = mPools.begin(); iter != mPools.end(); ++iter)
        {
            LLDrawPool *poolp = *iter;
            if (!poolp->verify())
            {
                ok = false;
            }
        }
    }

    if (!ok)
    {
        LL_WARNS() << "Pipeline verify failed!" << LL_ENDL;
    }
    return ok;
}

//////////////////////////////
//
// Collision detection
//
//

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *  A method to compute a ray-AABB intersection.
 *  Original code by Andrew Woo, from "Graphics Gems", Academic Press, 1990
 *  Optimized code by Pierre Terdiman, 2000 (~20-30% faster on my Celeron 500)
 *  Epsilon value added by Klaus Hartmann. (discarding it saves a few cycles only)
 *
 *  Hence this version is faster as well as more robust than the original one.
 *
 *  Should work provided:
 *  1) the integer representation of 0.0f is 0x00000000
 *  2) the sign bit of the float is the most significant one
 *
 *  Report bugs: p.terdiman@codercorner.com
 *
 *  \param      aabb        [in] the axis-aligned bounding box
 *  \param      origin      [in] ray origin
 *  \param      dir         [in] ray direction
 *  \param      coord       [out] impact coordinates
 *  \return     true if ray intersects AABB
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define RAYAABB_EPSILON 0.00001f
#define IR(x)   ((U32&)x)

bool LLRayAABB(const LLVector3 &center, const LLVector3 &size, const LLVector3& origin, const LLVector3& dir, LLVector3 &coord, F32 epsilon)
{
    bool Inside = true;
    LLVector3 MinB = center - size;
    LLVector3 MaxB = center + size;
    LLVector3 MaxT;
    MaxT.mV[VX]=MaxT.mV[VY]=MaxT.mV[VZ]=-1.0f;

    // Find candidate planes.
    for(U32 i=0;i<3;i++)
    {
        if(origin.mV[i] < MinB.mV[i])
        {
            coord.mV[i] = MinB.mV[i];
            Inside      = false;

            // Calculate T distances to candidate planes
            if(IR(dir.mV[i]))   MaxT.mV[i] = (MinB.mV[i] - origin.mV[i]) / dir.mV[i];
        }
        else if(origin.mV[i] > MaxB.mV[i])
        {
            coord.mV[i] = MaxB.mV[i];
            Inside      = false;

            // Calculate T distances to candidate planes
            if(IR(dir.mV[i]))   MaxT.mV[i] = (MaxB.mV[i] - origin.mV[i]) / dir.mV[i];
        }
    }

    // Ray origin inside bounding box
    if(Inside)
    {
        coord = origin;
        return true;
    }

    // Get largest of the maxT's for final choice of intersection
    U32 WhichPlane = 0;
    if(MaxT.mV[1] > MaxT.mV[WhichPlane])    WhichPlane = 1;
    if(MaxT.mV[2] > MaxT.mV[WhichPlane])    WhichPlane = 2;

    // Check final candidate actually inside box
    if(IR(MaxT.mV[WhichPlane])&0x80000000) return false;

    for(U32 i=0;i<3;i++)
    {
        if(i!=WhichPlane)
        {
            coord.mV[i] = origin.mV[i] + MaxT.mV[WhichPlane] * dir.mV[i];
            if (epsilon > 0)
            {
                if(coord.mV[i] < MinB.mV[i] - epsilon || coord.mV[i] > MaxB.mV[i] + epsilon)    return false;
            }
            else
            {
                if(coord.mV[i] < MinB.mV[i] || coord.mV[i] > MaxB.mV[i])    return false;
            }
        }
    }
    return true;    // ray hits box
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

void LLPipeline::toggleRenderType(U32 type)
{
    gPipeline.mRenderTypeEnabled[type] = !gPipeline.mRenderTypeEnabled[type];
    if (type == LLPipeline::RENDER_TYPE_WATER)
    {
        gPipeline.mRenderTypeEnabled[LLPipeline::RENDER_TYPE_VOIDWATER] = !gPipeline.mRenderTypeEnabled[LLPipeline::RENDER_TYPE_VOIDWATER];
    }
}

void LLPipeline::toggleRenderTypeControl(U32 type)
{
    gPipeline.toggleRenderType(type);
}

bool LLPipeline::hasRenderTypeControl(U32 type)
{
    return gPipeline.hasRenderType(type);
}

// Allows UI items labeled "Hide foo" instead of "Show foo"
bool LLPipeline::toggleRenderTypeControlNegated(S32 type)
{
    return !gPipeline.hasRenderType(type);
}

void LLPipeline::toggleRenderDebug(U64 bit)
{
    if (gPipeline.hasRenderDebugMask(bit))
    {
        LL_INFOS() << "Toggling render debug mask " << std::hex << bit << " off" << std::dec << LL_ENDL;
    }
    else
    {
        LL_INFOS() << "Toggling render debug mask " << std::hex << bit << " on" << std::dec << LL_ENDL;
    }
    gPipeline.mRenderDebugMask ^= bit;
}


bool LLPipeline::toggleRenderDebugControl(U64 bit)
{
    return gPipeline.hasRenderDebugMask(bit);
}

void LLPipeline::toggleRenderDebugFeature(U32 bit)
{
    gPipeline.mRenderDebugFeatureMask ^= bit;
}


bool LLPipeline::toggleRenderDebugFeatureControl(U32 bit)
{
    return gPipeline.hasRenderDebugFeatureMask(bit);
}

void LLPipeline::setRenderDebugFeatureControl(U32 bit, bool value)
{
    if (value)
    {
        gPipeline.mRenderDebugFeatureMask |= bit;
    }
    else
    {
        gPipeline.mRenderDebugFeatureMask &= !bit;
    }
}

void LLPipeline::pushRenderDebugFeatureMask()
{
    mRenderDebugFeatureStack.push(mRenderDebugFeatureMask);
}

void LLPipeline::popRenderDebugFeatureMask()
{
    if (mRenderDebugFeatureStack.empty())
    {
        LL_ERRS() << "Depleted render feature stack." << LL_ENDL;
    }

    mRenderDebugFeatureMask = mRenderDebugFeatureStack.top();
    mRenderDebugFeatureStack.pop();
}

void LLPipeline::setRenderScriptedBeacons(bool val)
{
    sRenderScriptedBeacons = val;
}

void LLPipeline::toggleRenderScriptedBeacons()
{
    sRenderScriptedBeacons = !sRenderScriptedBeacons;
}

bool LLPipeline::getRenderScriptedBeacons()
{
    return sRenderScriptedBeacons;
}

void LLPipeline::setRenderScriptedTouchBeacons(bool val)
{
    sRenderScriptedTouchBeacons = val;
}

void LLPipeline::toggleRenderScriptedTouchBeacons()
{
    sRenderScriptedTouchBeacons = !sRenderScriptedTouchBeacons;
}

bool LLPipeline::getRenderScriptedTouchBeacons()
{
    return sRenderScriptedTouchBeacons;
}

void LLPipeline::setRenderMOAPBeacons(bool val)
{
    sRenderMOAPBeacons = val;
}

void LLPipeline::toggleRenderMOAPBeacons()
{
    sRenderMOAPBeacons = !sRenderMOAPBeacons;
}

bool LLPipeline::getRenderMOAPBeacons()
{
    return sRenderMOAPBeacons;
}

void LLPipeline::setRenderPhysicalBeacons(bool val)
{
    sRenderPhysicalBeacons = val;
}

void LLPipeline::toggleRenderPhysicalBeacons()
{
    sRenderPhysicalBeacons = !sRenderPhysicalBeacons;
}

bool LLPipeline::getRenderPhysicalBeacons()
{
    return sRenderPhysicalBeacons;
}

void LLPipeline::setRenderParticleBeacons(bool val)
{
    sRenderParticleBeacons = val;
}

void LLPipeline::toggleRenderParticleBeacons()
{
    sRenderParticleBeacons = !sRenderParticleBeacons;
}

bool LLPipeline::getRenderParticleBeacons()
{
    return sRenderParticleBeacons;
}

void LLPipeline::setRenderSoundBeacons(bool val)
{
    sRenderSoundBeacons = val;
}

void LLPipeline::toggleRenderSoundBeacons()
{
    sRenderSoundBeacons = !sRenderSoundBeacons;
}

bool LLPipeline::getRenderSoundBeacons()
{
    return sRenderSoundBeacons;
}

void LLPipeline::setRenderBeacons(bool val)
{
    sRenderBeacons = val;
}

void LLPipeline::toggleRenderBeacons()
{
    sRenderBeacons = !sRenderBeacons;
}

bool LLPipeline::getRenderBeacons()
{
    return sRenderBeacons;
}

void LLPipeline::setRenderHighlights(bool val)
{
    sRenderHighlight = val;
}

void LLPipeline::toggleRenderHighlights()
{
    sRenderHighlight = !sRenderHighlight;
}

bool LLPipeline::getRenderHighlights()
{
    return sRenderHighlight;
}

// <FS:PP> FIRE-33085 Region corner markers
void LLPipeline::setRenderRegionCornerBeacons(bool val)
{
    sRenderRegionCornerBeacons = val;
}

void LLPipeline::toggleRenderRegionCornerBeacons()
{
    sRenderRegionCornerBeacons = !sRenderRegionCornerBeacons;
}

bool LLPipeline::getRenderRegionCornerBeacons()
{
    return sRenderRegionCornerBeacons;
}
// </FS:PP>

void LLPipeline::setRenderHighlightTextureChannel(LLRender::eTexIndex channel)
{
    if (channel != sRenderHighlightTextureChannel)
    {
        sRenderHighlightTextureChannel = channel;
    }
}

LLVOPartGroup* LLPipeline::lineSegmentIntersectParticle(const LLVector4a& start, const LLVector4a& end, LLVector4a* intersection,
                                                        S32* face_hit)
{
    LLVector4a local_end = end;

    LLVector4a position;

    LLDrawable* drawable = NULL;

    for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
            iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
    {
        LLViewerRegion* region = *iter;

        LLSpatialPartition* part = region->getSpatialPartition(LLViewerRegion::PARTITION_PARTICLE);
        if (part && hasRenderType(part->mDrawableType))
        {
            LLDrawable* hit = part->lineSegmentIntersect(start, local_end, true, false, true, false, face_hit, &position, NULL, NULL, NULL);
            if (hit)
            {
                drawable = hit;
                local_end = position;
            }
        }
    }

    LLVOPartGroup* ret = NULL;
    if (drawable)
    {
        //make sure we're returning an LLVOPartGroup
        llassert(drawable->getVObj()->getPCode() == LLViewerObject::LL_VO_PART_GROUP);
        ret = (LLVOPartGroup*) drawable->getVObj().get();
    }

    if (intersection)
    {
        *intersection = position;
    }

    return ret;
}

LLViewerObject* LLPipeline::lineSegmentIntersectInWorld(const LLVector4a& start, const LLVector4a& end,
                                                        bool pick_transparent,
                                                        bool pick_rigged,
                                                        bool pick_unselectable,
                                                        bool pick_reflection_probe,
                                                        S32* face_hit,
                                                        S32* gltf_node_hit,
                                                        S32* gltf_primitive_hit,
                                                        LLVector4a* intersection,         // return the intersection point
                                                        LLVector2* tex_coord,            // return the texture coordinates of the intersection point
                                                        LLVector4a* normal,               // return the surface normal at the intersection point
                                                        LLVector4a* tangent             // return the surface tangent at the intersection point
    )
{
    LLDrawable* drawable = NULL;

    LLVector4a local_end = end;

    LLVector4a position;

    sPickAvatar = false; //! LLToolMgr::getInstance()->inBuildMode();

    for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
            iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
    {
        LLViewerRegion* region = *iter;

        for (U32 j = 0; j < LLViewerRegion::NUM_PARTITIONS; j++)
        {
            if ((j == LLViewerRegion::PARTITION_VOLUME) ||
                (j == LLViewerRegion::PARTITION_BRIDGE) ||
                (j == LLViewerRegion::PARTITION_AVATAR) || // for attachments
                (j == LLViewerRegion::PARTITION_CONTROL_AV) ||
                (j == LLViewerRegion::PARTITION_TERRAIN) ||
                (j == LLViewerRegion::PARTITION_TREE) ||
                (j == LLViewerRegion::PARTITION_GRASS))  // only check these partitions for now
            {
                LLSpatialPartition* part = region->getSpatialPartition(j);
                if (part && hasRenderType(part->mDrawableType))
                {
                    LLDrawable* hit = part->lineSegmentIntersect(start, local_end, pick_transparent, pick_rigged, pick_unselectable, pick_reflection_probe, face_hit, &position, tex_coord, normal, tangent);
                    if (hit)
                    {
                        drawable = hit;
                        local_end = position;
                    }
                }
            }
        }
    }

    if (!sPickAvatar)
    {
        //save hit info in case we need to restore
        //due to attachment override
        LLVector4a local_normal;
        LLVector4a local_tangent;
        LLVector2 local_texcoord;
        S32 local_face_hit = -1;

        if (face_hit)
        {
            local_face_hit = *face_hit;
        }
        if (tex_coord)
        {
            local_texcoord = *tex_coord;
        }
        if (tangent)
        {
            local_tangent = *tangent;
        }
        else
        {
            local_tangent.clear();
        }
        if (normal)
        {
            local_normal = *normal;
        }
        else
        {
            local_normal.clear();
        }

        const F32 ATTACHMENT_OVERRIDE_DIST = 0.1f;

        //check against avatars
        sPickAvatar = true;
        for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
                iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
        {
            LLViewerRegion* region = *iter;

            LLSpatialPartition* part = region->getSpatialPartition(LLViewerRegion::PARTITION_AVATAR);
            if (part && hasRenderType(part->mDrawableType))
            {
                LLDrawable* hit = part->lineSegmentIntersect(start, local_end, pick_transparent, pick_rigged, pick_unselectable, pick_reflection_probe, face_hit, &position, tex_coord, normal, tangent);
                if (hit)
                {
                    LLVector4a delta;
                    delta.setSub(position, local_end);

                    if (!drawable ||
                        !drawable->getVObj()->isAttachment() ||
                        delta.getLength3().getF32() > ATTACHMENT_OVERRIDE_DIST)
                    { //avatar overrides if previously hit drawable is not an attachment or
                      //attachment is far enough away from detected intersection
                        drawable = hit;
                        local_end = position;
                    }
                    else
                    { //prioritize attachments over avatars
                        position = local_end;

                        if (face_hit)
                        {
                            *face_hit = local_face_hit;
                        }
                        if (tex_coord)
                        {
                            *tex_coord = local_texcoord;
                        }
                        if (tangent)
                        {
                            *tangent = local_tangent;
                        }
                        if (normal)
                        {
                            *normal = local_normal;
                        }
                    }
                }
            }
        }
    }

    // check all avatar nametags (silly, isn't it?)
    for (LLCharacter* character : LLCharacter::sInstances)
    {
        LLVOAvatar* avatar = (LLVOAvatar*)character;
        if (avatar->mNameText.notNull() &&
            avatar->mNameText->lineSegmentIntersect(start, local_end, position))
        {
            drawable = avatar->mDrawable;
            local_end = position;
        }
    }

    S32 node_hit = -1;
    S32 primitive_hit = -1;
    LLDrawable* hit = LL::GLTFSceneManager::instance().lineSegmentIntersect(start, local_end, pick_transparent, pick_rigged, pick_unselectable, pick_reflection_probe, &node_hit, &primitive_hit, &position, tex_coord, normal, tangent);
    if (hit)
    {
        drawable = hit;
        local_end = position;
    }

    if (gltf_node_hit)
    {
        *gltf_node_hit = node_hit;
    }

    if (gltf_primitive_hit)
    {
        *gltf_primitive_hit = primitive_hit;
    }

    if (intersection)
    {
        *intersection = position;
    }

    return drawable ? drawable->getVObj().get() : NULL;
}

LLViewerObject* LLPipeline::lineSegmentIntersectInHUD(const LLVector4a& start, const LLVector4a& end,
                                                      bool pick_transparent,
                                                      S32* face_hit,
                                                      LLVector4a* intersection,         // return the intersection point
                                                      LLVector2* tex_coord,            // return the texture coordinates of the intersection point
                                                      LLVector4a* normal,               // return the surface normal at the intersection point
                                                      LLVector4a* tangent               // return the surface tangent at the intersection point
    )
{
    LLDrawable* drawable = NULL;

    for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
            iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
    {
        LLViewerRegion* region = *iter;

        bool toggle = false;
        if (!hasRenderType(LLPipeline::RENDER_TYPE_HUD))
        {
            toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
            toggle = true;
        }

        LLSpatialPartition* part = region->getSpatialPartition(LLViewerRegion::PARTITION_HUD);
        if (part)
        {
            LLDrawable* hit = part->lineSegmentIntersect(start, end, pick_transparent, false, true, false, face_hit, intersection, tex_coord, normal, tangent);
            if (hit)
            {
                drawable = hit;
            }
        }

        if (toggle)
        {
            toggleRenderType(LLPipeline::RENDER_TYPE_HUD);
        }
    }
    return drawable ? drawable->getVObj().get() : NULL;
}

LLSpatialPartition* LLPipeline::getSpatialPartition(LLViewerObject* vobj)
{
    if (vobj)
    {
        LLViewerRegion* region = vobj->getRegion();
        if (region)
        {
            return region->getSpatialPartition(vobj->getPartitionType());
        }
    }
    return NULL;
}

void LLPipeline::resetVertexBuffers(LLDrawable* drawable)
{
    if (!drawable)
    {
        return;
    }

    for (S32 i = 0; i < drawable->getNumFaces(); i++)
    {
        LLFace* facep = drawable->getFace(i);
        if (facep)
        {
            facep->clearVertexBuffer();
        }
    }
}

void LLPipeline::renderObjects(U32 type, bool texture, bool batch_texture, bool rigged)
{
    assertInitialized();
    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;

    if (rigged)
    {
        mSimplePool->pushRiggedBatches(type + 1, texture, batch_texture);
    }
    else
    {
        mSimplePool->pushBatches(type, texture, batch_texture);
    }

    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;
}

void LLPipeline::renderGLTFObjects(U32 type, bool texture, bool rigged, bool scene_manager)
{
    assertInitialized();
    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;

    if (rigged)
    {
        mSimplePool->pushRiggedGLTFBatches(type + 1, texture);
    }
    else
    {
        mSimplePool->pushGLTFBatches(type, texture);
    }

    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;

    if (!scene_manager)
    {
        return;
    }

    if (!rigged)
    {
        LL::GLTFSceneManager::instance().renderOpaque();
    }
    else
    {
        LL::GLTFSceneManager::instance().render(true, true);
    }
}

// Currently only used for shadows -Cosmic,2023-04-19
void LLPipeline::renderAlphaObjects(bool rigged, S32 gltf_mode)
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
    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);

    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LLDrawInfo* pparams = *i;
        LLCullResult::increment_iterator(i, end);

        if (rigged != (pparams->mAvatar != nullptr))
        {
            // Pool contains both rigged and non-rigged DrawInfos. Only draw
            // the objects we're interested in in this pass.
            continue;
        }

        const bool is_gltf = (pparams->mGLTFMaterial != nullptr);
        if ((gltf_mode == 1 && is_gltf) || (gltf_mode == 2 && !is_gltf))
        {
            continue;
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
                LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(pparams->mObjectAlpha);
                LLRenderPass::pushRiggedGLTFBatch(*pparams, lastAvatarGLTF, lastMeshIdGLTF, skipLastSkinGLTF);
            }
            else
            {
                gDeferredShadowAlphaMaskProgram.bind(rigged);
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
                LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(pparams->mObjectAlpha);
                LLRenderPass::pushGLTFBatch(*pparams);
            }
            else
            {
                gDeferredShadowAlphaMaskProgram.bind(rigged);
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
    }

    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;
}

// Currently only used for shadows -Cosmic,2023-04-19
void LLPipeline::renderMaskedObjects(U32 type, bool texture, bool batch_texture, bool rigged)
{
    assertInitialized();
    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;
    if (rigged)
    {
        mAlphaMaskPool->pushRiggedMaskBatches(type+1, texture, batch_texture);
    }
    else
    {
        mAlphaMaskPool->pushMaskBatches(type, texture, batch_texture);
    }
    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;
}

// Currently only used for shadows -Cosmic,2023-04-19
void LLPipeline::renderFullbrightMaskedObjects(U32 type, bool texture, bool batch_texture, bool rigged)
{
    assertInitialized();
    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;
    if (rigged)
    {
        mFullbrightAlphaMaskPool->pushRiggedMaskBatches(type+1, texture, batch_texture);
    }
    else
    {
        mFullbrightAlphaMaskPool->pushMaskBatches(type, texture, batch_texture);
    }
    gGL.loadMatrix(gGLModelView);
    gGLLastMatrix = NULL;
}

void apply_cube_face_rotation(U32 face)
{
    switch (face)
    {
        case 0:
            gGL.rotatef(90.f, 0, 1, 0);
            gGL.rotatef(180.f, 1, 0, 0);
        break;
        case 2:
            gGL.rotatef(-90.f, 1, 0, 0);
        break;
        case 4:
            gGL.rotatef(180.f, 0, 1, 0);
            gGL.rotatef(180.f, 0, 0, 1);
        break;
        case 1:
            gGL.rotatef(-90.f, 0, 1, 0);
            gGL.rotatef(180.f, 1, 0, 0);
        break;
        case 3:
            gGL.rotatef(90, 1, 0, 0);
        break;
        case 5:
            gGL.rotatef(180, 0, 0, 1);
        break;
    }
}

static LLTrace::BlockTimerStatHandle FTM_RENDER_BLOOM("Bloom");

void LLPipeline::visualizeBuffers(LLRenderTarget* src, LLRenderTarget* dst, U32 bufferIndex)
{
    dst->bindTarget();
    gDeferredBufferVisualProgram.bind();
    gDeferredBufferVisualProgram.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, src, false, LLTexUnit::TFO_BILINEAR, bufferIndex);

    if (LLVKLoader::isVulkanInitialized()
        && gDeferredBufferVisualProgram.mVkPerProgramUBO != VK_NULL_HANDLE
        && gDeferredBufferVisualProgram.mVkPerProgramUBOMapped != nullptr)
    {
        LLVKLoader::PostVisualizeBuffers_PerProgramBind ubo_data = {};
        ubo_data.mipLevel = (RenderBufferVisualization != 4) ? 0.f : 8.f;
        std::memcpy(gDeferredBufferVisualProgram.mVkPerProgramUBOMapped, &ubo_data,
                    llmin((U32)sizeof(ubo_data), gDeferredBufferVisualProgram.mVkPerProgramUBOSize));
    }

    mScreenTriangleVB->setBuffer();
    mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
    gDeferredBufferVisualProgram.unbind();
    dst->flush();
}

void LLPipeline::generateLuminance(LLRenderTarget* src, LLRenderTarget* dst)
{
    // luminance sample and mipmap generation
    {
        LL_PROFILE_GPU_ZONE("luminance sample");

        dst->bindTarget();

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
            std::memcpy(gLuminanceProgram.mVkPerProgramUBOMapped, &ubo_data,
                        llmin((U32)sizeof(ubo_data), gLuminanceProgram.mVkPerProgramUBOSize));
        }

        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
        dst->flush();

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
            mLastExposure.bindTarget();
            gCopyProgram.bind();
            gGL.getTexUnit(0)->bind(dst);

            if (LLVKLoader::isVulkanInitialized())
            {
                dst->bindForShaderRead();
            }

            mScreenTriangleVB->setBuffer();
            mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

            mLastExposure.flush();
        }

        dst->bindTarget();

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
            std::memcpy(shader->mVkPerProgramUBOMapped, &ubo_data,
                        llmin((U32)sizeof(ubo_data), shader->mVkPerProgramUBOSize));
        }

        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

        if (use_history)
        {
            gGL.getTexUnit(channel)->unbind(mLastExposure.getUsage());
        }
        shader->unbind();
        dst->flush();
    }
}

extern LLPointer<LLImageGL> gEXRImage;

void LLPipeline::tonemap(LLRenderTarget* src, LLRenderTarget* dst, bool gamma_correct)
{
    LL_PROFILE_GPU_ZONE("tonemap");

    dst->bindTarget();
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
            if (lut_name != mColorGradingLUTName)
                loadColorGradingLUT(lut_name);
            // </FS:AYAstorm>
        }

        S32 lut_channel = shader->enableTexture(LLShaderMgr::COLOR_GRADING_LUT, LLTexUnit::TT_TEXTURE_3D);
        if (lut_channel > -1 && mColorGradingLUT.notNull())
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
            ubo_data.color_grading_lut_enabled   = (mColorGradingLUT.notNull()) ? 1 : 0;
            ubo_data.gamma                       = gamma_correct ? (F32)psky->getGamma() : 0.f;
            std::memcpy(shader->mVkPerProgramUBOMapped, &ubo_data,
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
    dst->flush();
    LLVKLoader::gpuCheckpoint("tm:flush");
}

void LLPipeline::gammaCorrect(LLRenderTarget* src, LLRenderTarget* dst)
{
    LL_PROFILE_GPU_ZONE("gamma correct");

    dst->bindTarget();
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
            std::memcpy(shader.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
        }

        shader.bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, src, false, LLTexUnit::TFO_POINT);

        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

        shader.unbind();
    }
    dst->flush();
}

void LLPipeline::copyScreenSpaceReflections(LLRenderTarget* src, LLRenderTarget* dst)
{

    if (RenderScreenSpaceReflections && !gCubeSnapshot)
    {
        LL_PROFILE_GPU_ZONE("ssr copy");
        LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_ALWAYS);

        LLRenderTarget& depth_src = getFrameRT()->deferredScreen;

        dst->bindTarget();
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

        dst->flush();
    }
}

void LLPipeline::generateGlow(LLRenderTarget* src)
{
    LL_PROFILE_GPU_ZONE("glow generate");
    if (isFrameRenderingGlow())
    {
        mGlow[2].bindTarget();
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

                memcpy(gGlowExtractProgram.mVkPerProgramUBOMapped, &ubo_data,
                       llmin((U32)sizeof(ubo_data), gGlowExtractProgram.mVkPerProgramUBOSize));
            }

            mScreenTriangleVB->setBuffer();
            mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

            mGlow[2].flush();
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
            mGlow[i % 2].bindTarget();
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
                memcpy(gGlowProgram.mVkActivePerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
            }

            mScreenTriangleVB->setBuffer();
            mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

            mGlow[i % 2].flush();
        }

        gGlowProgram.unbind();

    }
    else // !isFrameRenderingGlow(), skip the glow ping-pong and just clear the result target
    {
        mGlow[1].bindTarget();
        gGL.setClearColor(0.f, 0.f, 0.f, 0.f);
        mGlow[1].clear(GL_COLOR_BUFFER_BIT);
        mGlow[1].flush();
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
    dst->bindTarget();
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
            memcpy(sharpen_shader->mVkPerProgramUBOMapped, &ubo_data,
                   llmin((U32)sizeof(ubo_data), sharpen_shader->mVkPerProgramUBOSize));
        }
    }

    sharpen_shader->bindTexture(LLShaderMgr::DEFERRED_DIFFUSE, src, false, LLTexUnit::TFO_POINT);

    // Draw
    gPipeline.mScreenTriangleVB->setBuffer();
    gPipeline.mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
    LLVKLoader::gpuCheckpoint("cas:draw");

    sharpen_shader->unbind();

    dst->flush();
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
            mFXAAMap.bindTarget();
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

            mFXAAMap.flush();

            dst->bindTarget();

            static LLCachedControl<U32> aa_quality(gSavedSettings, "RenderFSAASamples", 0U);
            U32 fsaa_quality = std::clamp(aa_quality(), 0U, 3U);

            shader = &gFXAAProgram[fsaa_quality];
            shader->bind();

            channel = shader->enableTexture(LLShaderMgr::DIFFUSE_MAP, mFXAAMap.getUsage());
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
                std::memcpy(shader->mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
            }

            {
                LLGLDepthTest depth_test(GL_TRUE, GL_TRUE, GL_ALWAYS);
                S32 depth_channel = shader->getTextureChannel(LLShaderMgr::DEFERRED_DEPTH);
                gGL.getTexUnit(depth_channel)->bind(&getFrameRT()->deferredScreen, true);

                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            }

            shader->unbind();
            dst->flush();
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

            dest.bindTarget();
            dest.clear(GL_COLOR_BUFFER_BIT);

            edge_shader.bind();
            if (LLVKLoader::isVulkanInitialized()
                && edge_shader.mVkPerProgramUBO != VK_NULL_HANDLE
                && edge_shader.mVkPerProgramUBOMapped != nullptr)
            {
                LLVKLoader::SMAA_PerProgramBind smaa_ubo = {};
                memcpy(smaa_ubo.SMAA_RT_METRICS, rt_metrics, sizeof(smaa_ubo.SMAA_RT_METRICS));
                memcpy(edge_shader.mVkPerProgramUBOMapped, &smaa_ubo, sizeof(smaa_ubo));
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
            dest.flush();

            gGL.getTexUnit(channel)->unbindFast(LLTexUnit::TT_TEXTURE);
        }

        {
            // Bind setup:
            LLRenderTarget& dest = mSMAABlendBuffer;
            LLGLSLShader& blend_weights_shader = gSMAABlendWeightsProgram[fsaa_quality];

            dest.bindTarget();
            dest.clear(GL_COLOR_BUFFER_BIT);

            blend_weights_shader.bind();
            if (LLVKLoader::isVulkanInitialized()
                && blend_weights_shader.mVkPerProgramUBO != VK_NULL_HANDLE
                && blend_weights_shader.mVkPerProgramUBOMapped != nullptr)
            {
                LLVKLoader::SMAA_PerProgramBind smaa_ubo = {};
                memcpy(smaa_ubo.SMAA_RT_METRICS, rt_metrics, sizeof(smaa_ubo.SMAA_RT_METRICS));
                memcpy(blend_weights_shader.mVkPerProgramUBOMapped, &smaa_ubo, sizeof(smaa_ubo));
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
            dest.flush();
            gGL.getTexUnit(edge_tex_channel)->unbindFast(LLTexUnit::TT_TEXTURE);
            gGL.getTexUnit(area_tex_channel)->unbindFast(LLTexUnit::TT_TEXTURE);
            gGL.getTexUnit(search_tex_channel)->unbindFast(LLTexUnit::TT_TEXTURE);
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

            bound_target->bindTarget();
            bound_target->clear(GL_COLOR_BUFFER_BIT);

            blend_shader.bind();
            if (LLVKLoader::isVulkanInitialized()
                && blend_shader.mVkPerProgramUBO != VK_NULL_HANDLE
                && blend_shader.mVkPerProgramUBOMapped != nullptr)
            {
                LLVKLoader::SMAA_PerProgramBind smaa_ubo = {};
                memcpy(smaa_ubo.SMAA_RT_METRICS, rt_metrics, sizeof(smaa_ubo.SMAA_RT_METRICS));
                memcpy(blend_shader.mVkPerProgramUBOMapped, &smaa_ubo, sizeof(smaa_ubo));
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

            bound_target->flush();
            blend_shader.unbind();
            gGL.getTexUnit(diffuse_channel)->unbindFast(LLTexUnit::TT_TEXTURE);
            gGL.getTexUnit(blend_channel)->unbindFast(LLTexUnit::TT_TEXTURE);
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

    dst->bindTarget();

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
    dst->flush();

    // Save the current SMAA'd frame (not the resolved output) to history so
    // the next frame's resolve sees a true 50/50 blend between the two
    // jitter samples instead of exponential history decay.
    copyRenderTarget(src, &mSMAAHistory);
}
// </AYAstorm r30 P2 step 5c>

void LLPipeline::copyRenderTarget(LLRenderTarget* src, LLRenderTarget* dst)
{

    LL_PROFILE_GPU_ZONE("copyRenderTarget");
    dst->bindTarget();

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
        memcpy(gDeferredPostNoDoFProgram.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
    }

    {
        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
    }

    gDeferredPostNoDoFProgram.unbind();

    dst->flush();
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
    dst->bindTarget();

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

    dst->flush();
}

// <FS:Beq> updated Vignette code (based on original Exo Vignette)
bool LLPipeline::renderVignette(LLRenderTarget* src, LLRenderTarget* dst)
{
    if (RenderVignette.mV[0] > 0.f)
    {
        LL_PROFILE_GPU_ZONE("Vignette");
        dst->bindTarget();
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
            memcpy(shader->mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
        }

        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

        shader->disableTexture(LLShaderMgr::DEFERRED_DIFFUSE, src->getUsage());
        shader->unbind();
        dst->flush();
        return true;
    }
    else
    {
        return false;
    }
}
// </FS:Beq>

// <FS:Beq> Render Snapshot frame oerlay
bool LLPipeline::renderSnapshotFrame(LLRenderTarget* src, LLRenderTarget* dst)
{
    static LLCachedControl<bool> show_frame(gSavedSettings, "FSSnapshotShowCaptureFrame", false);
    static LLCachedControl<bool> show_guides(gSavedSettings, "FSSnapshotShowGuides", false);

    mSnapshotGuideState.active = false;
    mSnapshotGuideState.show_guides = false;

    float left   = 0.f;
    float top    = 0.f;
    float right  = 1.f;
    float bottom = 1.f;        

    // TODO - add debug settings to control the appearance of the snapshot frameand guides
    static LLCachedControl<LLColor3> border_color(gSavedSettings, "FSSnapshotFrameBorderColor", LLColor3(1.f, 0.f, 0.f));    
    static LLCachedControl<LLColor3> guide_color(gSavedSettings, "FSSnapshotFrameGuideColor", LLColor3(1.f, 1.f, 0.f));    
    static LLCachedControl<F32> border_thickness(gSavedSettings, "FSSnapshotFrameBorderWidth", 2.0f);    
    static LLCachedControl<F32> guide_thickness(gSavedSettings, "FSSnapshotFrameGuideWidth", 2.0f);    
    static LLCachedControl<F32> guide_visibility(gSavedSettings, "FSSnapshotGuideVisibility", 0.5f);
    static LLCachedControl<std::string> guide_style_setting(gSavedSettings, "FSSnapshotGuideStyle", std::string("rule_of_thirds"));

    SnapshotGuideState::Style guide_style = SnapshotGuideState::Style::RuleOfThirds;
    SnapshotGuideState::GoldenOrientation golden_orientation = SnapshotGuideState::GoldenOrientation::TopLeft;
    const std::string style_value = guide_style_setting();
    if (style_value == "golden_ratio" || style_value == "golden_ratio_top_left")
    {
        guide_style = SnapshotGuideState::Style::GoldenRatio;
        golden_orientation = SnapshotGuideState::GoldenOrientation::TopLeft;
    }
    else if (style_value == "golden_ratio_top_right")
    {
        guide_style = SnapshotGuideState::Style::GoldenRatio;
        golden_orientation = SnapshotGuideState::GoldenOrientation::TopRight;
    }
    else if (style_value == "golden_ratio_bottom_left")
    {
        guide_style = SnapshotGuideState::Style::GoldenRatio;
        golden_orientation = SnapshotGuideState::GoldenOrientation::BottomLeft;
    }
    else if (style_value == "golden_ratio_bottom_right")
    {
        guide_style = SnapshotGuideState::Style::GoldenRatio;
        golden_orientation = SnapshotGuideState::GoldenOrientation::BottomRight;
    }
    else if (style_value == "diagonal")
    {
        guide_style = SnapshotGuideState::Style::Diagonal;
    }
    else
    {
        guide_style = SnapshotGuideState::Style::RuleOfThirds;
    }
    const F32 guide_visibility_value = show_guides ? (F32)guide_visibility : 0.f;
    const bool simple_snapshot_visible = LLFloaterReg::instanceVisible("simple_snapshot");
    const bool flickr_snapshot_visible = LLFloaterReg::instanceVisible("flickr");
    const bool primfeed_snapshot_visible = LLFloaterReg::instanceVisible("primfeed"); // <FS:Beq/> Primfeed integration
    const bool snapshot_visible = LLFloaterReg::instanceVisible("snapshot");
    const bool any_snapshot_visible = simple_snapshot_visible || flickr_snapshot_visible || primfeed_snapshot_visible || snapshot_visible; // <FS:Beq/> Primfeed integration
    if (!show_frame || !any_snapshot_visible || !gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
    {
        return false;
    }
    LLSnapshotLivePreview * previewView = nullptr;
    if (snapshot_visible)
    {
        auto * floater =dynamic_cast<LLFloaterSnapshotBase*>(LLFloaterReg::findInstance("snapshot"));
        previewView = floater->impl->getPreviewView();
    }
    // Note: simple_snapshot not supported as there can be more than one active and more complex selection is required
    if (flickr_snapshot_visible && !previewView)
    {
        auto * floater = dynamic_cast<LLFloaterFlickr*>(LLFloaterReg::findInstance("flickr"));
        previewView = floater->getPreviewView();
    }
     // <FS:Beq> Primfeed integration
    if (primfeed_snapshot_visible && !previewView)
    {
        auto * floater = dynamic_cast<FSFloaterPrimfeed*>(LLFloaterReg::findInstance("primfeed"));
        previewView = floater->getPreviewView();
    }
    // </FS:Beq>
    if(!previewView)
    {
        return false;
    }
    
    static LLCachedControl<bool> keep_aspect(gSavedSettings, "KeepAspectForSnapshot", false);
    
    S32 snapshot_width;
    S32 snapshot_height;
    previewView->getSize(snapshot_width, snapshot_height);
    F32 screen_aspect = float(gViewerWindow->getWindowWidthRaw()) / float(gViewerWindow->getWindowHeightRaw());
    F32 snapshot_aspect = float(snapshot_width) / float(snapshot_height);

    if (keep_aspect || (std::fabs(screen_aspect - snapshot_aspect) < 1e-6f) )
    {
        top    = 0.0f;
        left   = 0.0f;
        bottom = 1.0f;
        right  = 1.0f;
    }

    float w = screen_aspect;
    float h = 1.0;
    if (snapshot_aspect > screen_aspect)
    {
        float frame_width = w;
        float frame_height = frame_width / snapshot_aspect;
        // Centre this box in [0..1]x[0..1]
        float y_offset = 0.5f * (h - frame_height);
        left   = 0.f;
        top    = y_offset / h;
        right  = 1.f;
        bottom = (y_offset + frame_height) / h;        
    }
    else
    {
        float frame_height = h;
        float frame_width = h * snapshot_aspect;
        // Centre this box in [0..1]x[0..1]
        float x_offset = 0.5f * (w - frame_width);
        left   = x_offset / w;
        top    = 0.f;
        right  = (x_offset + frame_width) / w;
        bottom = 1.f;        

    }
    LL_PROFILE_GPU_ZONE("Snapshot Frame");
    dst->bindTarget();
    LLGLSLShader *shader = &gPostSnapshotFrameProgram;

    // bind the program and output to screentriangle VBO
    shader->bind();

    S32 channel = shader->enableTexture(LLShaderMgr::DEFERRED_DIFFUSE, src->getUsage());
    if (channel > -1)
    {
        src->bindTexture(0, channel, LLTexUnit::TFO_POINT);
    }
    else
    {
        LL_ERRS("snapshot_frame") << "Failed to bind diffuse texture" << LL_ENDL;
    }

    // Assuming frame_rect is a static or accessible variable containing the frame dimensions

    if (LLVKLoader::isVulkanInitialized() && shader->mVkPerProgramUBO != VK_NULL_HANDLE
        && shader->mVkPerProgramUBOMapped != nullptr)
    {
        LLVKLoader::PostSnapshotFrame_PerProgramBind ubo_data = {};
        ubo_data.screen_res[0]          = (F32)dst->getWidth();
        ubo_data.screen_res[1]          = (F32)dst->getHeight();
        ubo_data.frame_rect[0]          = (F32)left;
        ubo_data.frame_rect[1]          = (F32)top;
        ubo_data.frame_rect[2]          = (F32)right;
        ubo_data.frame_rect[3]          = (F32)bottom;
        ubo_data.border_color[0]        = border_color().mV[0];
        ubo_data.border_color[1]        = border_color().mV[1];
        ubo_data.border_color[2]        = border_color().mV[2];
        ubo_data.border_thickness       = (F32)border_thickness;
        memcpy(shader->mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
    }

    // Guides are rendered in a later UI pass; no additional uniforms required here.

    mScreenTriangleVB->setBuffer();
    mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

    shader->disableTexture(LLShaderMgr::DEFERRED_DIFFUSE, src->getUsage());
    shader->unbind();
    dst->flush();

    if (show_frame && show_guides && gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_UI))
    {
        mSnapshotGuideState.active = true;
        mSnapshotGuideState.show_guides = true;
        mSnapshotGuideState.left = left;
        mSnapshotGuideState.right = right;
        mSnapshotGuideState.bottom = bottom;
        mSnapshotGuideState.top = top;
        mSnapshotGuideState.color = guide_color();
        mSnapshotGuideState.thickness = guide_thickness();
        mSnapshotGuideState.visibility = llclamp(guide_visibility_value, 0.f, 1.f);
        mSnapshotGuideState.style = guide_style;
        mSnapshotGuideState.golden_orientation = golden_orientation;
    }

    return true;
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
                getFrameRT()->deferredLight.bindTarget();

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
                    std::memcpy(gDeferredCoFProgram.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
                }

                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
                gDeferredCoFProgram.unbind();
                getFrameRT()->deferredLight.flush();
            }

            U32 dof_width = (U32)(getFrameRT()->screen.getWidth() * CameraDoFResScale);
            U32 dof_height = (U32)(getFrameRT()->screen.getHeight() * CameraDoFResScale);

            { // perform DoF sampling at half-res (preserve alpha channel)
                src->bindTarget();
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
                    memcpy(gDeferredPostProgram.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
                }

                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

                gDeferredPostProgram.unbind();

                src->flush();
                gGL.setColorMask(true, true);
            }

            { // combine result based on alpha

                dst->bindTarget();
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
                    std::memcpy(gDeferredDoFCombineProgram.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
                }

                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

                gDeferredDoFCombineProgram.unbind();

                dst->flush();
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
        getFrameRT()->screen.bindTarget();

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

        getFrameRT()->screen.flush();
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
        memcpy(gDeferredPostNoDoFNoiseProgram.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
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
                gGL.getTexUnit(channel)->bind(getSunShadowTarget(i), true);
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

    if (sSceneDepthCopyActive && LLVKLoader::isVulkanInitialized() && mSceneDepthCopy.isComplete())
    {
        S32 dch = shader.getTextureChannel(LLShaderMgr::DEFERRED_DEPTH);
        if (dch > -1)
        {
            gGL.getTexUnit(dch)->bind(&mSceneDepthCopy, true);
        }
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
        else if (sSceneDepthCopyActive && LLVKLoader::isVulkanInitialized() && mSceneDepthCopy.isComplete())
        {
            gGL.getTexUnit(channel)->bind(&mSceneDepthCopy, true);
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
        char* mapped = (char*)shader.mVkPerProgramUBOMapped;
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
        char* mapped = (char*)shader.mVkPerProgramUBOMapped;
        const F32 near_clip_v = LLViewerCamera::getInstance()->getNear() * 2.f;
        memcpy(mapped + LLVKLoader::ALPHAF_UBO_OFFSET_NEAR_CLIP, &near_clip_v, sizeof(F32));
    }

    if (LLVKLoader::isVulkanInitialized() && shader.mVkPerProgramUBO != VK_NULL_HANDLE
        && shader.mVkPerProgramUBOMapped != nullptr
        && shader.mVkPerProgramUBOSize == LLVKLoader::GLTFMR_UBO_SIZE_ALPHA_NOSHADOW)
    {
        char* mapped = (char*)shader.mVkPerProgramUBOMapped;
        F32 sun_v[4]  = { mTransformedSunDir.mV[0],  mTransformedSunDir.mV[1],  mTransformedSunDir.mV[2],  0.f };
        F32 moon_v[4] = { mTransformedMoonDir.mV[0], mTransformedMoonDir.mV[1], mTransformedMoonDir.mV[2], 0.f };
        memcpy(mapped + LLVKLoader::GLTFMR_UBO_OFFSET_SUN_DIR,  sun_v,  16);
        memcpy(mapped + LLVKLoader::GLTFMR_UBO_OFFSET_MOON_DIR, moon_v, 16);
    }
}


LLColor3 pow3f(LLColor3 v, F32 f)
{
    v.mV[0] = powf(v.mV[0], f);
    v.mV[1] = powf(v.mV[1], f);
    v.mV[2] = powf(v.mV[2], f);
    return v;
}

LLVector4 pow4fsrgb(LLVector4 v, F32 f)
{
    v.mV[0] = powf(v.mV[0], f);
    v.mV[1] = powf(v.mV[1], f);
    v.mV[2] = powf(v.mV[2], f);
    return v;
}

// <AYAstorm:r21.1> GPU self-rigged picker helpers ----------------------------
namespace
{
    LLFrameTimer sFSSelfRiggedPickerArmTimer;
    F32 sFSSelfRiggedPickerArmSeconds = 0.f;
    U32 sFSSelfRiggedPickerArmGeneration = 0;
    U32 sFSSelfRiggedPickerRenderGeneration = 0;

    enum class FSRiggedPickerObjectIDBufferOwner
    {
        None,
        Self,
        Other
    };

    FSRiggedPickerObjectIDBufferOwner sFSRiggedPickerObjectIDBufferOwner =
        FSRiggedPickerObjectIDBufferOwner::None;

    LLFrameTimer sFSOtherRiggedPickerArmTimer;
    F32 sFSOtherRiggedPickerArmSeconds = 0.f;
    U32 sFSOtherRiggedPickerArmGeneration = 0;
    U32 sFSOtherRiggedPickerRenderGeneration = 0;
    LLPointer<LLVOAvatar> sFSOtherRiggedPickerAvatar;
    LLUUID sFSOtherRiggedPickerAvatarID;

    // All PASS_*_RIGGED types in the LL render map. The visible deferred opaque
    // pass dispatches rigged geometry through these via renderRiggedGroup /
    // pushRiggedBatches (see lldrawpool.cpp:410, 466). Iterating the same set
    // gives pixel-perfect agreement with what the user actually sees.
    const U32 kFSRiggedPasses[] = {
        LLRenderPass::PASS_SIMPLE_RIGGED,
        LLRenderPass::PASS_FULLBRIGHT_RIGGED,
        LLRenderPass::PASS_INVISIBLE_RIGGED,
        LLRenderPass::PASS_INVISI_SHINY_RIGGED,
        LLRenderPass::PASS_FULLBRIGHT_SHINY_RIGGED,
        LLRenderPass::PASS_SHINY_RIGGED,
        LLRenderPass::PASS_BUMP_RIGGED,
        LLRenderPass::PASS_POST_BUMP_RIGGED,
        LLRenderPass::PASS_MATERIAL_RIGGED,
        LLRenderPass::PASS_MATERIAL_ALPHA_RIGGED,
        LLRenderPass::PASS_MATERIAL_ALPHA_MASK_RIGGED,
        LLRenderPass::PASS_MATERIAL_ALPHA_EMISSIVE_RIGGED,
        LLRenderPass::PASS_SPECMAP_RIGGED,
        LLRenderPass::PASS_SPECMAP_BLEND_RIGGED,
        LLRenderPass::PASS_SPECMAP_MASK_RIGGED,
        LLRenderPass::PASS_SPECMAP_EMISSIVE_RIGGED,
        LLRenderPass::PASS_NORMMAP_RIGGED,
        LLRenderPass::PASS_NORMMAP_BLEND_RIGGED,
        LLRenderPass::PASS_NORMMAP_MASK_RIGGED,
        LLRenderPass::PASS_NORMMAP_EMISSIVE_RIGGED,
        LLRenderPass::PASS_NORMSPEC_RIGGED,
        LLRenderPass::PASS_NORMSPEC_BLEND_RIGGED,
        LLRenderPass::PASS_NORMSPEC_MASK_RIGGED,
        LLRenderPass::PASS_NORMSPEC_EMISSIVE_RIGGED,
        LLRenderPass::PASS_GLOW_RIGGED,
        LLRenderPass::PASS_GLTF_GLOW_RIGGED,
        LLRenderPass::PASS_ALPHA_RIGGED,
        LLRenderPass::PASS_ALPHA_MASK_RIGGED,
        LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK_RIGGED,
        LLRenderPass::PASS_ALPHA_INVISIBLE_RIGGED,
        LLRenderPass::PASS_GLTF_PBR_RIGGED,
        LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK_RIGGED,
    };
}

bool LLPipeline::renderRiggedObjectIDBufferForAvatar(LLVOAvatar* target_avatar,
                                                     U32 max_draw_calls,
                                                     U32 max_triangles)
{
    if (!target_avatar || target_avatar->isDead()) return false;
    if (!mObjectIDBuffer.isComplete()) return false;

    gGL.flush();

    GLboolean previous_color_mask[4] = {
        (GLboolean)(gGL.getColorMaskR() ? GL_TRUE : GL_FALSE),
        (GLboolean)(gGL.getColorMaskG() ? GL_TRUE : GL_FALSE),
        (GLboolean)(gGL.getColorMaskB() ? GL_TRUE : GL_FALSE),
        (GLboolean)(gGL.getColorMaskA() ? GL_TRUE : GL_FALSE) };
    GLfloat previous_clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
    GLint previous_cull_face_mode = LLGLState::sCullFaceMode;
    const F32* cur_cc = gGL.getClearColor();
    previous_clear_color[0] = cur_cc[0];
    previous_clear_color[1] = cur_cc[1];
    previous_clear_color[2] = cur_cc[2];
    previous_clear_color[3] = cur_cc[3];

    mObjectIDBuffer.bindTarget();
    // gbuffer3 has no alpha in default LL config (project memory
    // reference_gbuffer3_storage); make sure all four channels are writable so
    // the top 8 bits of each packed ID survive the write. Go through gGL so
    // LLRender's cached mask stays in sync with the actual GL state.
    gGL.setColorMask(false, false, false, false);
    gGL.setColorMask(true, true, true, true);
    gGL.setClearColor(0.f, 0.f, 0.f, 0.f);
    LLRenderTarget::clearBoundTarget(GL_COLOR_BUFFER_BIT);

    // Depth shared with deferredScreen — test only, no write.
    LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_LEQUAL);
    LLGLDisable   blend(GL_BLEND);
    // Cull pinned to BACK to match deferred opaque (back-facing collar
    // interiors must not write IDs at chin pixels).
    LLGLEnable    cull (GL_CULL_FACE);
    LLGLState::setCullFaceMode(GL_BACK);

    gFSObjectIDShader.bind();

    // uploadMatrixPalette caches the last (avatar, mesh) pair to skip redundant
    // GPU uploads for back-to-back DrawInfos with the same skin.
    const LLVOAvatar* lastAvatar = nullptr;
    U64  lastMeshId   = 0;
    bool skipLastSkin = false;
    U32 draw_calls = 0;
    U32 triangles = 0;
    bool over_budget = false;

    for (U32 pass_type : kFSRiggedPasses)
    {
        LLCullResult::drawinfo_iterator begin = beginRenderMap(pass_type);
        LLCullResult::drawinfo_iterator end   = endRenderMap(pass_type);
        for (LLCullResult::drawinfo_iterator i = begin; i != end; )
        {
            LLDrawInfo* info = *i;
            LLCullResult::increment_iterator(i, end);
            if (!info || !info->mVertexBuffer || info->mCount == 0) continue;
            if (info->mAvatar.get() != target_avatar) continue;
            const LLMeshSkinInfo* skin = info->mSkinInfo.get();
            if (!skin || skin->mHash == 0) continue;
            U32 id = info->mFSPickerLocalID;
            if (id == 0)
            {
                // DrawInfo wasn't stamped with a LocalID at construction
                // time (non-prim source, or stale batch from a removed
                // attachment). Skip — its pixels stay 0 in the buffer and
                // resolve as "no rigged attachment here".
                continue;
            }

            const U32 next_draw_calls = draw_calls + 1;
            const U32 next_triangles = triangles + (info->mCount / 3);
            if ((max_draw_calls > 0 && next_draw_calls > max_draw_calls) ||
                (max_triangles > 0 && next_triangles > max_triangles))
            {
                over_budget = true;
                break;
            }

            F32 r = ((id >>  0) & 0xff) / 255.f;
            F32 g = ((id >>  8) & 0xff) / 255.f;
            F32 b = ((id >> 16) & 0xff) / 255.f;
            F32 a = ((id >> 24) & 0xff) / 255.f;
            if (LLVKLoader::isVulkanInitialized()
                && gFSObjectIDShader.mVkPerProgramUBOMapped != nullptr
                && gFSObjectIDShader.mVkPerProgramUBOSize >= sizeof(LLVKLoader::FsObjectIDF_PerProgramBind))
            {
                LLVKLoader::FsObjectIDF_PerProgramBind ubo_data{};
                ubo_data.object_id_packed[0] = r;
                ubo_data.object_id_packed[1] = g;
                ubo_data.object_id_packed[2] = b;
                ubo_data.object_id_packed[3] = a;
                gFSObjectIDShader.rotatePerProgramUBOSlot();
                std::memcpy(gFSObjectIDShader.mVkActivePerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
            }

            if (!LLRenderPass::uploadMatrixPalette(target_avatar, skin,
                                                   lastAvatar, lastMeshId, skipLastSkin))
            {
                continue;
            }

            info->mVertexBuffer->setBuffer();
            info->mVertexBuffer->drawRange(LLRender::TRIANGLES,
                                           info->mStart, info->mEnd,
                                           info->mCount, info->mOffset);
            draw_calls = next_draw_calls;
            triangles = next_triangles;
        }

        if (over_budget)
        {
            break;
        }
    }

    gFSObjectIDShader.unbind();

    if (over_budget)
    {
        gGL.setClearColor(0.f, 0.f, 0.f, 0.f);
        LLRenderTarget::clearBoundTarget(GL_COLOR_BUFFER_BIT);
    }

    mObjectIDBuffer.flush();

    gGL.setColorMask(previous_color_mask[0] == GL_TRUE,
                     previous_color_mask[1] == GL_TRUE,
                     previous_color_mask[2] == GL_TRUE,
                     previous_color_mask[3] == GL_TRUE);
    gGL.setClearColor(previous_clear_color[0], previous_clear_color[1], previous_clear_color[2], previous_clear_color[3]);
    LLGLState::setCullFaceMode(previous_cull_face_mode);

    return !over_budget;
}

void LLPipeline::armSelfRiggedObjectIDBuffer(F32 seconds)
{
    if (seconds <= 0.f)
    {
        return;
    }

    const bool was_armed = isSelfRiggedObjectIDBufferArmed();
    sFSSelfRiggedPickerArmSeconds = seconds;
    sFSSelfRiggedPickerArmTimer.reset();

    if (!was_armed)
    {
        ++sFSSelfRiggedPickerArmGeneration;
    }
}

bool LLPipeline::isSelfRiggedObjectIDBufferArmed() const
{
    return sFSSelfRiggedPickerArmSeconds > 0.f &&
           sFSSelfRiggedPickerArmTimer.getElapsedTimeF32() <= sFSSelfRiggedPickerArmSeconds;
}

bool LLPipeline::isSelfRiggedObjectIDBufferReady() const
{
    return isSelfRiggedObjectIDBufferArmed() &&
           sFSRiggedPickerObjectIDBufferOwner == FSRiggedPickerObjectIDBufferOwner::Self &&
           sFSSelfRiggedPickerRenderGeneration == sFSSelfRiggedPickerArmGeneration;
}

void LLPipeline::armOtherRiggedObjectIDBuffer(LLVOAvatar* avatar, F32 seconds)
{
    if (seconds <= 0.f || !avatar || avatar->isDead())
    {
        return;
    }
    if (isAgentAvatarValid() && avatar == gAgentAvatarp.get())
    {
        return;
    }
    if (avatar->isImpostor())
    {
        return;
    }

    const bool was_armed = isOtherRiggedObjectIDBufferArmed();
    const bool target_changed = (sFSOtherRiggedPickerAvatarID != avatar->getID());
    sFSOtherRiggedPickerAvatar = avatar;
    sFSOtherRiggedPickerAvatarID = avatar->getID();
    sFSOtherRiggedPickerArmSeconds = seconds;
    sFSOtherRiggedPickerArmTimer.reset();

    if (!was_armed || target_changed)
    {
        ++sFSOtherRiggedPickerArmGeneration;
    }
}

bool LLPipeline::isOtherRiggedObjectIDBufferArmed() const
{
    if (sFSOtherRiggedPickerArmSeconds <= 0.f ||
        sFSOtherRiggedPickerArmTimer.getElapsedTimeF32() > sFSOtherRiggedPickerArmSeconds)
    {
        return false;
    }
    return sFSOtherRiggedPickerAvatar.notNull() &&
           !sFSOtherRiggedPickerAvatar->isDead() &&
           sFSOtherRiggedPickerAvatarID.notNull();
}

bool LLPipeline::isOtherRiggedObjectIDBufferReady(const LLUUID& avatar_id) const
{
    return avatar_id.notNull() &&
           isOtherRiggedObjectIDBufferArmed() &&
           sFSOtherRiggedPickerAvatarID == avatar_id &&
           sFSRiggedPickerObjectIDBufferOwner == FSRiggedPickerObjectIDBufferOwner::Other &&
           sFSOtherRiggedPickerRenderGeneration == sFSOtherRiggedPickerArmGeneration;
}

void LLPipeline::clearOtherRiggedObjectIDBuffer()
{
    if (!isOtherRiggedObjectIDBufferArmed() &&
        sFSOtherRiggedPickerArmSeconds <= 0.f &&
        sFSOtherRiggedPickerAvatar.isNull() &&
        sFSOtherRiggedPickerAvatarID.isNull())
    {
        return;
    }

    sFSOtherRiggedPickerArmSeconds = 0.f;
    sFSOtherRiggedPickerAvatar = nullptr;
    sFSOtherRiggedPickerAvatarID.setNull();
    ++sFSOtherRiggedPickerArmGeneration;
    if (sFSRiggedPickerObjectIDBufferOwner == FSRiggedPickerObjectIDBufferOwner::Other)
    {
        sFSRiggedPickerObjectIDBufferOwner = FSRiggedPickerObjectIDBufferOwner::None;
    }
}

void LLPipeline::renderSelfRiggedObjectIDBuffer()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LL_PROFILE_GPU_ZONE("renderSelfRiggedObjectIDBuffer");

    static LLCachedControl<bool> gpu_enable(gSavedSettings, "FSSelfRiggedPickerGPU", false);
    if (!gpu_enable) return;
    if (!isAgentAvatarValid()) return;

    if (renderRiggedObjectIDBufferForAvatar(gAgentAvatarp.get(), 0, 0))
    {
        sFSRiggedPickerObjectIDBufferOwner = FSRiggedPickerObjectIDBufferOwner::Self;
        sFSSelfRiggedPickerRenderGeneration = sFSSelfRiggedPickerArmGeneration;
    }
    else
    {
        sFSRiggedPickerObjectIDBufferOwner = FSRiggedPickerObjectIDBufferOwner::None;
        sFSSelfRiggedPickerRenderGeneration = 0;
    }
}

void LLPipeline::renderOtherRiggedObjectIDBuffer()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LL_PROFILE_GPU_ZONE("renderOtherRiggedObjectIDBuffer");

    static LLCachedControl<bool> enable(gSavedSettings, "FSOtherRiggedPickerEnable", false);
    static LLCachedControl<bool> gpu_enable(gSavedSettings, "FSOtherRiggedPickerGPU", true);
    // AYA P0 fixup: MaxDrawCalls / MaxTriangles were cvars; hardcoded now.
    static constexpr U32 kMaxDrawCalls = 512;
    static constexpr U32 kMaxTriangles = 1200000;
    if (!enable || !gpu_enable) return;
    if (gAgentCamera.getCameraMode() == CAMERA_MODE_MOUSELOOK ||
        gAgentCamera.cameraCustomizeAvatar())
    {
        clearOtherRiggedObjectIDBuffer();
        return;
    }
    if (!isOtherRiggedObjectIDBufferArmed()) return;
    LLVOAvatar* target_avatar = sFSOtherRiggedPickerAvatar.get();
    if (!target_avatar || target_avatar->isDead() || target_avatar->isImpostor())
    {
        return;
    }

    if (renderRiggedObjectIDBufferForAvatar(target_avatar, kMaxDrawCalls, kMaxTriangles))
    {
        sFSRiggedPickerObjectIDBufferOwner = FSRiggedPickerObjectIDBufferOwner::Other;
        sFSOtherRiggedPickerRenderGeneration = sFSOtherRiggedPickerArmGeneration;
    }
    else
    {
        sFSRiggedPickerObjectIDBufferOwner = FSRiggedPickerObjectIDBufferOwner::None;
        sFSOtherRiggedPickerRenderGeneration = 0;
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

    sSceneDepthCopyActive = false;
    if (LLVKLoader::isVulkanInitialized() && !gCubeSnapshot && mSceneDepthCopy.isComplete())
    {
        LL_PROFILE_GPU_ZONE("scene depth copy for lighting");
        LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_ALWAYS);
        LLRenderTarget& depth_src = getFrameRT()->deferredScreen;
        mSceneDepthCopy.bindTarget();
        gCopyDepthProgram.bind();
        S32 diff_map  = gCopyDepthProgram.getTextureChannel(LLShaderMgr::DIFFUSE_MAP);
        S32 depth_map = gCopyDepthProgram.getTextureChannel(LLShaderMgr::DEFERRED_DEPTH);
        gGL.getTexUnit(diff_map)->bind(&depth_src);
        gGL.getTexUnit(depth_map)->bind(&depth_src, true);
        depth_src.bindForShaderRead(0, true);
        gGL.setColorMask(false, false);
        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
        gGL.setColorMask(true, true);
        mSceneDepthCopy.flush();
        sSceneDepthCopyActive = true;
    }

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
            su.shadow_res[0]   = (F32)getFrameRT()->shadow[0].getWidth();
            su.shadow_res[1]   = (F32)getFrameRT()->shadow[0].getHeight();
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
                deferred_light_target->bindTarget();
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
                        memcpy(sun_shader.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
                    }

                    {
                        LLGLDisable   blend(GL_BLEND);
                        LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_ALWAYS);
                        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
                    }

                    unbindDeferredShader(sun_shader);
                }
                deferred_light_target->flush();
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
            screen_target->bindTarget();
            gGL.setClearColor(1, 1, 1, 1);
            screen_target->clear(GL_COLOR_BUFFER_BIT);
            gGL.setClearColor(0, 0, 0, 0);

            bindD(gDeferredBlurLightProgram);

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
                memcpy(gDeferredBlurLightProgram.mVkActivePerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
            }

            {
                LLGLDisable   blend(GL_BLEND);
                LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_ALWAYS);
                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            }

            screen_target->flush();
            unbindDeferredShader(gDeferredBlurLightProgram);

            bindD(gDeferredBlurLightProgram, screen_target);

            deferred_light_target->bindTarget();

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
                memcpy(gDeferredBlurLightProgram.mVkActivePerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
            }

            {
                LLGLDisable   blend(GL_BLEND);
                LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_ALWAYS);
                mScreenTriangleVB->setBuffer();
                mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);
            }
            deferred_light_target->flush();
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
        screen_target->bindTarget();
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

                memcpy(soften_shader.mVkPerProgramUBOMapped, ubo_buffer, ubo_size);
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
                            if (gDeferredLightProgram.mVkActivePerProgramUBOMapped != nullptr)
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
                                memcpy(gDeferredLightProgram.mVkActivePerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
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
                        if (gDeferredSpotLightProgram.mVkActivePerProgramUBOMapped != nullptr)
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
                            memcpy(gDeferredSpotLightProgram.mVkActivePerProgramUBOMapped, &sd, sizeof(sd));
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
                            if (gDeferredMultiLightProgram[idx].mVkActivePerProgramUBOMapped != nullptr)
                            {
                                U8 buf[16 * 32 + 16] = {};
                                U32 light_arr_bytes = count * 16;
                                memcpy(buf, light, light_arr_bytes);
                                memcpy(buf + light_arr_bytes, col, light_arr_bytes);
                                F32 footer[4] = { far_z, LLPipeline::RenderGlobalLightStrength, 0.f, 0.f };
                                memcpy(buf + light_arr_bytes * 2, footer, sizeof(footer));
                                U32 ubo_size = light_arr_bytes * 2 + 16;
                                memcpy(gDeferredMultiLightProgram[idx].mVkActivePerProgramUBOMapped, buf, ubo_size);
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
                        if (gDeferredMultiSpotLightProgram.mVkActivePerProgramUBOMapped != nullptr)
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
                            memcpy(gDeferredMultiSpotLightProgram.mVkActivePerProgramUBOMapped, &sd, sizeof(sd));
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
    if (mAYAAlphaDepth.isComplete() && !gCubeSnapshot && RenderDepthOfField)
    {
        LL_PROFILE_GPU_ZONE("aya alpha depth snapshot");
        LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_ALWAYS);

        LLRenderTarget& depth_src = getFrameRT()->deferredScreen;

        getFrameRT()->screen.flush();
        mAYAAlphaDepth.bindTarget();
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

        mAYAAlphaDepth.flush();
        getFrameRT()->screen.bindTarget();
    }
    // </AYAstorm r30 P5 transparent-DoF L2-β>

    sSceneDepthCopyActive = false;

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

    screen_target->flush();

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

void LLPipeline::doAtmospherics()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    if (isFrameImpostorPass())
    { // do not attempt atmospherics on impostors
        return;
    }

    if (RenderDeferredAtmospheric)
    {
        {
            // copy depth buffer for use in haze shader (use water displacement map as temp storage)
            LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_ALWAYS);

            LLRenderTarget& src = getFrameRT()->screen;
            LLRenderTarget& depth_src = getFrameRT()->deferredScreen;
            LLRenderTarget& dst = gPipeline.mWaterDis;

            getFrameRT()->screen.flush();
            dst.bindTarget();
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

            dst.flush();
            getFrameRT()->screen.bindTarget();
        }

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
            memcpy(haze_shader.mVkActivePerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
        }

        LLGLDepthTest depth(GL_FALSE);

        // full screen blit
        mScreenTriangleVB->setBuffer();
        mScreenTriangleVB->drawArrays(LLRender::TRIANGLES, 0, 3);

        unbindDeferredShader(haze_shader);

        gGL.setSceneBlendType(LLRender::BT_ALPHA);
    }
}

// <FS:AYA r15 P1> godrays: screen-space shadow-driven ray-march pass.
// Mirrors the doAtmospherics() pattern (bindDeferredShader on the HDR
// scene buffer, fullscreen triangle, additive blend) so godrays land on
// getFrameRT()->screen while it is still HDR / pre-tonemap.
void LLPipeline::doGodrays()
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

    LLGLDepthTest depth(GL_FALSE);
    LLGLEnable    blend(GL_BLEND);
    gGL.blendFunc(LLRender::BF_ONE, LLRender::BF_ONE, LLRender::BF_ONE, LLRender::BF_ONE);
    gGL.setColorMask(true, true);

    LLGLSLShader& shader = gDeferredGodraysProgram;
    bindDeferredShader(shader);

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
        memcpy(shader.mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
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
    {
        LLGLDisable blend_off(GL_BLEND);

        getFrameRT()->screen.flush();
        mWaterDis.bindTarget();

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
                if (LLVKLoader::isVulkanInitialized() && mSceneDepthCopy.isComplete())
                    gGL.getTexUnit(dch)->bind(&mSceneDepthCopy, true);
                else
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
            memcpy(gDeferredSkinSSSProgram.mVkActivePerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
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
        mWaterDis.flush();
    }

    // Pass 2: vertical blur, mWaterDis → screen (RGB-only mix with strength,
    // alpha kept untouched so the scene-buffer sky mask is preserved —
    // memory project_aya_visual_realism_alpha_protect.md)
    {
        getFrameRT()->screen.bindTarget();

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
                if (LLVKLoader::isVulkanInitialized() && mSceneDepthCopy.isComplete())
                    gGL.getTexUnit(dch)->bind(&mSceneDepthCopy, true);
                else
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
            memcpy(gDeferredSkinSSSProgram.mVkActivePerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
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
        getFrameRT()->screen.flush();
    }

    getFrameRT()->screen.bindTarget();
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
        {
            LLGLDepthTest depth(GL_TRUE, GL_TRUE, GL_ALWAYS);

            LLRenderTarget& src = getFrameRT()->screen;
            LLRenderTarget& depth_src = getFrameRT()->deferredScreen;
            LLRenderTarget& dst = gPipeline.mWaterDis;

            getFrameRT()->screen.flush();
            dst.bindTarget();
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

            dst.flush();
            getFrameRT()->screen.bindTarget();
        }

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
            memcpy(haze_shader.mVkPerProgramUBOMapped, &ubo_data,
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

void LLPipeline::doWaterExclusionMask()
{
    mWaterExclusionMask.bindTarget();
    gGL.setClearColor(1, 1, 1, 1);
    mWaterExclusionMask.clear();
    mWaterExclusionPool->render();

    mWaterExclusionMask.flush();
    gGL.setClearColor(0, 0, 0, 0);
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


inline float sgn(float a)
{
    if (a > 0.0F) return (1.0F);
    if (a < 0.0F) return (-1.0F);
    return (0.0F);
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

static LLTrace::BlockTimerStatHandle FTM_SHADOW_RENDER("Render Shadows");
static LLTrace::BlockTimerStatHandle FTM_SHADOW_ALPHA("Alpha Shadow");
static LLTrace::BlockTimerStatHandle FTM_SHADOW_SIMPLE("Simple Shadow");
static LLTrace::BlockTimerStatHandle FTM_SHADOW_GEOM("Shadow Geom");

static LLTrace::BlockTimerStatHandle FTM_SHADOW_ALPHA_MASKED("Alpha Masked");
static LLTrace::BlockTimerStatHandle FTM_SHADOW_ALPHA_BLEND("Alpha Blend");
static LLTrace::BlockTimerStatHandle FTM_SHADOW_ALPHA_TREE("Alpha Tree");
static LLTrace::BlockTimerStatHandle FTM_SHADOW_ALPHA_GRASS("Alpha Grass");
static LLTrace::BlockTimerStatHandle FTM_SHADOW_FULLBRIGHT_ALPHA_MASKED("Fullbright Alpha Masked");

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
    struct ShadowWorkerSeeds
    {
        bool attempted = false;
        bool valid     = false;
        LLGLSLShader::record_seed_map_t map;
    };

    struct ShadowRecordCtx
    {
        F32             view[16];
        F32             proj[16];
        F32             last_modelview[16];
        F32             cull_radius = 0.f;
        LLCullResult*   result = nullptr;
        LLRenderTarget* rt = nullptr;
        VkImage         depth_image = VK_NULL_HANDLE;
        VkImageView     depth_view = VK_NULL_HANDLE;
        U32             width = 0;
        U32             height = 0;
        VkImageLayout   initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        U32             map_index = 0;
        bool            color_mask_off = false;
        bool            depth_clamp = true;
        const LLGLSLShader::record_seed_map_t* seeds = nullptr;
    };

    // III-1a: worker 化する cascade/spot の per-slot 記録計画。dispatch 前に prep 相 (Pass 1) で
    // 埋め、dispatch (Pass 2) / render (Pass 3) は全 mutation 完了後にまとめて回す (S8-b 根治)。
    struct ShadowRenderPlan
    {
        bool            mt = false;              // worker 化する slot か (Pass 2/3 対象)
        bool            dispatch_failed = false; // dispatch 失敗時 = Pass 3 で clear + static のみ描画
        ShadowRecordCtx ctx;
        LLCamera        shadow_cam;
        F32             cull_radius = 0.f;
        LLRenderTarget* rt = nullptr;
        LLCullResult*   result = nullptr;
        bool            depth_clamp = true;      // renderShadow 5th arg + ctx.depth_clamp (sun=true/spot=false)
        S32             sun_j = -1;              // sun cascade index (s_cascade_valid 用)・-1=spot
        LLDrawable*     spot_light = nullptr;    // Pass 3 の RenderSpotLight (spot のみ)
    };

    std::vector<LLPointer<LLDrawInfo>> sShadowRecordPins;

    void pinShadowWorkerDrawInfo(LLDrawInfo* info)
    {
        sShadowRecordPins.emplace_back(info);
        if (info->mAvatar.notNull() && info->mSkinInfo != nullptr)
        {
            info->mAvatar->updateSkinInfoMatrixPalette(info->mSkinInfo);
        }
    }

    void pinShadowWorkerDrawInfos(LLCullResult& result)
    {
        for (U32 ti = 0; ti < LLVKBucket::kBucketizedPassCount; ++ti)
        {
            const U32 type = LLVKBucket::kBucketizedPasses[ti] + 1;
            auto* pb = result.beginRenderMap(type);
            auto* pe = result.endRenderMap(type);
            for (auto* pi = pb; pi != pe; ++pi)
            {
                pinShadowWorkerDrawInfo(*pi);
            }
        }
        const U32 extra_types[] = {
            LLRenderPass::PASS_ALPHA_MASK,
            LLRenderPass::PASS_ALPHA_MASK + 1,
            LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK,
            LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK + 1,
            LLRenderPass::PASS_ALPHA,
            LLRenderPass::PASS_GLTF_PBR + 1,
        };
        for (U32 type : extra_types)
        {
            auto* pb = result.beginRenderMap(type);
            auto* pe = result.endRenderMap(type);
            for (auto* pi = pb; pi != pe; ++pi)
            {
                pinShadowWorkerDrawInfo(*pi);
            }
        }
    }

    void ensureShadowWorkerSeeds(ShadowWorkerSeeds& seeds, LLRenderTarget& shadow_rt)
    {
        if (seeds.attempted)
        {
            return;
        }
        seeds.attempted = true;
        if (!LLVKLoader::isVulkanInitialized() || !LLVKLoader::isBindlessActiveVk())
        {
            return;
        }
        LLGLSLShader* worker_shaders[] = {
            gDeferredShadowProgram.mRiggedVariant,
            &gDeferredShadowAlphaMaskProgram,
            gDeferredShadowAlphaMaskProgram.mRiggedVariant,
            &gDeferredShadowFullbrightAlphaMaskProgram,
            gDeferredShadowFullbrightAlphaMaskProgram.mRiggedVariant,
        };
        for (LLGLSLShader* s : worker_shaders)
        {
            if (s == nullptr)
            {
                return;
            }
        }
        LLRenderTarget* prev_bound = LLRenderTarget::sBoundTarget;
        LLRenderTarget::sBoundTarget = &shadow_rt;
        bool all_ok = true;
        {
            LLGLEnable cull(GL_CULL_FACE);
            LLGLEnable clamp_depth(GL_DEPTH_CLAMP);
            LLGLDepthTest depth_test(GL_TRUE, GL_TRUE, GL_LESS);
            const bool mask_off = LLPipeline::RenderShadowDetail <= 2;
            if (mask_off)
            {
                gGL.setColorMask(false, false);
            }
            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
            for (LLGLSLShader* s : worker_shaders)
            {
                s->bind();
                gGL.diffuseColor4f(1, 1, 1, 1);
                LLGLSLShader::sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
                LLGLSLShader::sCurPerCallAuthored        = false;
                VkDescriptorSet set = LLGLSLShader::vkResolvePerCallSetForDraw();
                if (set == VK_NULL_HANDLE)
                {
                    all_ok = false;
                    break;
                }
                LLGLSLShader::RecordSeed seed;
                seed.set   = set;
                seed.shape = LLGLSLShader::sCurPerCallVkSetShape;
                if (!LLGLSLShader::vkCaptureSeedDynamicBuffers(seed))
                {
                    all_ok = false;
                    break;
                }
                seeds.map[s] = seed;
            }
            gGL.setColorMask(true, true);
        }
        LLRenderTarget::sBoundTarget = prev_bound;
        seeds.valid = all_ok;
        if (!all_ok)
        {
            seeds.map.clear();
        }
    }

    void recordShadowWorkerFamilies(const ShadowRecordCtx& ctx, VkCommandBuffer cmd)
    {
        LLVKLoader::gpuCheckpoint("rw:shadow");
        LLPipelineFrameContext& fctx = LLPipelineFrameContext::getInstance();
        LLCullResult*   prev_cull      = fctx.getCullResult();
        const bool      prev_shadow    = fctx.isShadowPass();
        LLRenderTarget* prev_bound     = LLRenderTarget::sBoundTarget;
        const F32       prev_radius    = LLRenderPass::sShadowBatchCullRadius;
        const U32       prev_pass_tag  = LLVKLoader::gVkPerfPassTag;
        const U32       prev_map_index = LLVKLoader::gVkPerfShadowMapIndex;
        const U32       prev_res_x     = LLRenderTarget::sCurResX;
        const U32       prev_res_y     = LLRenderTarget::sCurResY;
        const LLGLSLShader::record_seed_map_t* prev_seeds = LLGLSLShader::sRecordSeedMap;

        fctx.setCullResult(ctx.result);
        fctx.setShadowPass(true);
        LLRenderTarget::sBoundTarget = ctx.rt;
        LLRenderTarget::sCurResX = ctx.width;
        LLRenderTarget::sCurResY = ctx.height;
        LLRenderPass::sShadowBatchCullRadius = ctx.cull_radius;
        LLGLSLShader::sRecordSeedMap = ctx.seeds;
        std::memcpy(gGLModelView, ctx.view, sizeof(F32) * 16);
        std::memcpy(gGLLastModelView, ctx.last_modelview, sizeof(F32) * 16);
        gGLLastMatrix = NULL;
        LLVKLoader::setRenderViewport(0, 0, (S32)ctx.width, (S32)ctx.height);
        LLVKLoader::gVkPerfPassTag = 1;
        LLVKLoader::gVkPerfShadowMapIndex = ctx.map_index;

        VkPipelineStageFlags src_stage;
        VkAccessFlags        src_access;
        if (ctx.initial_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            src_stage  = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            src_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }
        else if (ctx.initial_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            src_stage  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            src_access = VK_ACCESS_SHADER_READ_BIT;
        }
        else
        {
            src_stage  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            src_access = 0;
        }
        LLVKLoader::transitionImageLayoutVk(
            ctx.depth_image,
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
            ctx.initial_layout,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            src_stage,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            src_access,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

        LLVKLoader::DynamicRenderingAttachment depth_att = {};
        depth_att.image_view   = ctx.depth_view;
        depth_att.image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_att.load_op      = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_att.store_op     = VK_ATTACHMENT_STORE_OP_STORE;
        depth_att.clear_value.depthStencil.depth   = 1.0f;
        depth_att.clear_value.depthStencil.stencil = 0;
        LLVKLoader::beginDynamicRendering(ctx.width, ctx.height, nullptr, 0, &depth_att);

        {
            LLGLEnable cull(GL_CULL_FACE);
            LLGLEnable clamp_depth(ctx.depth_clamp ? GL_DEPTH_CLAMP : 0);
            LLGLDepthTest depth_test(GL_TRUE, GL_TRUE, GL_LESS);

            const LLRender::eBlendFactor prev_bf_cs = gGL.getCurrBlendColorSFactor();
            const LLRender::eBlendFactor prev_bf_cd = gGL.getCurrBlendColorDFactor();
            const LLRender::eBlendFactor prev_bf_as = gGL.getCurrBlendAlphaSFactor();
            const LLRender::eBlendFactor prev_bf_ad = gGL.getCurrBlendAlphaDFactor();
            gGL.blendFunc(LLRender::BF_ONE, LLRender::BF_ZERO);

            gGL.matrixMode(LLRender::MM_PROJECTION);
            gGL.pushMatrix();
            gGL.loadMatrix(ctx.proj);
            gGL.matrixMode(LLRender::MM_MODELVIEW);
            gGL.pushMatrix();
            gGL.loadMatrix(ctx.view);

            gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
            if (ctx.color_mask_off)
            {
                gGL.setColorMask(false, false);
            }

            gDeferredShadowProgram.bind(true);
            gGL.diffuseColor4f(1, 1, 1, 1);
            gGL.getTexUnit(0)->disable();
            for (U32 ti = 0; ti < LLVKBucket::kBucketizedPassCount; ++ti)
            {
                gPipeline.renderObjects(LLVKBucket::kBucketizedPasses[ti], false, false, true);
            }
            gPipeline.renderGLTFObjects(LLRenderPass::PASS_GLTF_PBR, false, true, false);
            gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);

            const U32 target_width = ctx.width;
            for (int i = 0; i < 2; ++i)
            {
                bool rigged = i == 1;

                gDeferredShadowAlphaMaskProgram.bind(rigged);
                LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);
                LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(1.f);
                if (LLVKLoader::isVulkanInitialized())
                {
                    LLVKLoader::ShadowParams_PerShaderBind shadow_params = {};
                    shadow_params.shadow_target_width = (float)target_width;
                    LLVKLoader::writeCurrentShadowParamsUBO(shadow_params);
                }
                gPipeline.renderMaskedObjects(LLRenderPass::PASS_ALPHA_MASK, true, true, rigged);

                gPipeline.renderAlphaObjects(rigged, 1);

                gDeferredShadowFullbrightAlphaMaskProgram.bind(rigged);
                LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);
                LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(1.f);
                if (LLVKLoader::isVulkanInitialized())
                {
                    LLVKLoader::ShadowParams_PerShaderBind shadow_params = {};
                    shadow_params.shadow_target_width = (float)target_width;
                    LLVKLoader::writeCurrentShadowParamsUBO(shadow_params);
                }
                gPipeline.renderFullbrightMaskedObjects(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, true, true, rigged);
            }

            gGL.setColorMask(true, true);

            gGL.matrixMode(LLRender::MM_PROJECTION);
            gGL.popMatrix();
            gGL.matrixMode(LLRender::MM_MODELVIEW);
            gGL.popMatrix();

            if (prev_bf_cs != LLRender::BF_UNDEF && prev_bf_cd != LLRender::BF_UNDEF
                && prev_bf_as != LLRender::BF_UNDEF && prev_bf_ad != LLRender::BF_UNDEF)
            {
                gGL.blendFunc(prev_bf_cs, prev_bf_cd, prev_bf_as, prev_bf_ad);
            }
        }

        LLVKLoader::endDynamicRendering();
        LLVKLoader::cmdShadowDepthWawBarrierVk(cmd, ctx.depth_image);

        LLGLSLShader::sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
        LLGLSLShader::sRecordSeedMap = prev_seeds;
        LLRenderPass::sShadowBatchCullRadius = prev_radius;
        LLRenderTarget::sBoundTarget = prev_bound;
        LLRenderTarget::sCurResX = prev_res_x;
        LLRenderTarget::sCurResY = prev_res_y;
        fctx.setCullResult(prev_cull);
        fctx.setShadowPass(prev_shadow);
        LLVKLoader::gVkPerfPassTag = prev_pass_tag;
        LLVKLoader::gVkPerfShadowMapIndex = prev_map_index;
        gGLLastMatrix = NULL;
    }
}

void LLPipeline::renderShadow(const glm::mat4& view, const glm::mat4& proj, LLCamera& shadow_cam, LLCullResult& result, bool depth_clamp, bool mt_split)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE; //LL_RECORD_BLOCK_TIME(FTM_SHADOW_RENDER);
    LL_PROFILE_GPU_ZONE("renderShadow");
    LLVKLoader::gpuCheckpoint("renderShadow");

    if (!shadowMatricesFinite(view, proj))
    {
        return;
    }

    LLPipelineFrameContext::getInstance().setShadowPass(true);

    // disable occlusion culling during shadow render
    U32 saved_occlusion = sUseOcclusion;
    sUseOcclusion = 0;

    // List of render pass types that use the prim volume as the shadow,
    // ignoring textures.
    const U32* types = LLVKBucket::kBucketizedPasses;
    const U32 types_count = LLVKBucket::kBucketizedPassCount;

    LLGLEnable cull(GL_CULL_FACE);

    //enable depth clamping if available
    LLGLEnable clamp_depth(depth_clamp ? GL_DEPTH_CLAMP : 0);

    LLGLDepthTest depth_test(GL_TRUE, GL_TRUE, GL_LESS);

    if (!mt_split)
    {
        updateCull(shadow_cam, result);

        stateSort(shadow_cam, result);
    }

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
    for (int j = 0; j < 2; ++j) // 0 -- static, 1 -- rigged
    {
        bool rigged = j == 1;
        gDeferredShadowProgram.bind(rigged);

        gGL.diffuseColor4f(1, 1, 1, 1);

        S32 shadow_detail = RenderShadowDetail;

        // if not using VSM, disable color writes
        if (shadow_detail <= 2)
        {
            gGL.setColorMask(false, false);
        }

        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow simple"); //LL_RECORD_BLOCK_TIME(FTM_SHADOW_SIMPLE);
        LL_PROFILE_GPU_ZONE("shadow simple");
        gGL.getTexUnit(0)->disable();

        if (!mt_split || !rigged)
        {
            for (U32 ti = 0; ti < types_count; ++ti)
            {
                renderObjects(types[ti], false, false, rigged);
            }

            renderGLTFObjects(LLRenderPass::PASS_GLTF_PBR, false, rigged);
        }
        else
        {
            gGL.loadMatrix(gGLModelView);
            gGLLastMatrix = NULL;
            LL::GLTFSceneManager::instance().render(true, true);
            gGL.loadMatrix(gGLModelView);
            gGLLastMatrix = NULL;
        }

        gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
    }

    if (LLPipeline::sUseOcclusion > 1)
    { // do occlusion culling against non-masked only to take advantage of hierarchical Z
        doOcclusion(shadow_cam);
    }


    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow geom");
        renderGeomShadow(shadow_cam);
    }

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow alpha");
        LL_PROFILE_GPU_ZONE("shadow alpha");
        U32 target_width = LLRenderTarget::sCurResX;

        for (int i = 0; i < 2; ++i)
        {
            bool rigged = i == 1;

            if (!mt_split)
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow alpha masked");
                LL_PROFILE_GPU_ZONE("shadow alpha masked");
                gDeferredShadowAlphaMaskProgram.bind(rigged);
                LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);
                LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(1.f);
                if (LLVKLoader::isVulkanInitialized())
                {
                    LLVKLoader::ShadowParams_PerShaderBind shadow_params = {};
                    shadow_params.shadow_target_width = (float)target_width;
                    LLVKLoader::writeCurrentShadowParamsUBO(shadow_params);
                }
                renderMaskedObjects(LLRenderPass::PASS_ALPHA_MASK, true, true, rigged);
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow alpha blend");
                LL_PROFILE_GPU_ZONE("shadow alpha blend");
                renderAlphaObjects(rigged, mt_split ? 2 : 0);
            }

            if (!mt_split)
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow fullbright alpha masked");
                LL_PROFILE_GPU_ZONE("shadow alpha masked");
                gDeferredShadowFullbrightAlphaMaskProgram.bind(rigged);
                LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);
                LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(1.f);
                if (LLVKLoader::isVulkanInitialized())
                {
                    LLVKLoader::ShadowParams_PerShaderBind shadow_params = {};
                    shadow_params.shadow_target_width = (float)target_width;
                    LLVKLoader::writeCurrentShadowParamsUBO(shadow_params);
                }
                renderFullbrightMaskedObjects(LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, true, true, rigged);
            }

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow alpha grass");
                LL_PROFILE_GPU_ZONE("shadow alpha grass");
                gDeferredTreeShadowProgram.bind(rigged);
                LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);

                if (i == 0)
                {
                    renderObjects(LLRenderPass::PASS_GRASS, true);
                }

                {
                    LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("shadow alpha material");
                    LL_PROFILE_GPU_ZONE("shadow alpha material");
                    renderMaskedObjects(LLRenderPass::PASS_NORMSPEC_MASK, true, false, rigged);
                    renderMaskedObjects(LLRenderPass::PASS_MATERIAL_ALPHA_MASK, true, false, rigged);
                    renderMaskedObjects(LLRenderPass::PASS_SPECMAP_MASK, true, false, rigged);
                    renderMaskedObjects(LLRenderPass::PASS_NORMMAP_MASK, true, false, rigged);
                }
            }
        }

        for (int i = 0; i < 2; ++i)
        {
            bool rigged = i == 1;
            gDeferredShadowGLTFAlphaMaskProgram.bind(rigged);
            LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(ALPHA_BLEND_CUTOFF);
            LLGLSLShader::sCurBoundShaderPtr->setObjectAlpha(1.f);
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
                mAlphaMaskPool->pushRiggedGLTFBatches(type + 1);
            }
            else
            {
                mAlphaMaskPool->pushGLTFBatches(type);
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

bool LLPipeline::getVisiblePointCloud(LLCamera& camera, LLVector3& min, LLVector3& max, std::vector<LLVector3>& fp, LLVector3 light_dir)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    //get point cloud of intersection of frust and min, max

    if (getVisibleExtents(camera, min, max))
    {
        return false;
    }

    //get set of planes on bounding box
    LLPlane bp[] = {
        LLPlane(min, LLVector3(-1,0,0)),
        LLPlane(min, LLVector3(0,-1,0)),
        LLPlane(min, LLVector3(0,0,-1)),
        LLPlane(max, LLVector3(1,0,0)),
        LLPlane(max, LLVector3(0,1,0)),
        LLPlane(max, LLVector3(0,0,1))};

    //potential points
    std::vector<LLVector3> pp;

    //add corners of AABB
    pp.push_back(LLVector3(min.mV[0], min.mV[1], min.mV[2]));
    pp.push_back(LLVector3(max.mV[0], min.mV[1], min.mV[2]));
    pp.push_back(LLVector3(min.mV[0], max.mV[1], min.mV[2]));
    pp.push_back(LLVector3(max.mV[0], max.mV[1], min.mV[2]));
    pp.push_back(LLVector3(min.mV[0], min.mV[1], max.mV[2]));
    pp.push_back(LLVector3(max.mV[0], min.mV[1], max.mV[2]));
    pp.push_back(LLVector3(min.mV[0], max.mV[1], max.mV[2]));
    pp.push_back(LLVector3(max.mV[0], max.mV[1], max.mV[2]));

    //add corners of camera frustum
    for (U32 i = 0; i < LLCamera::AGENT_FRUSTRUM_NUM; i++)
    {
        pp.push_back(camera.mAgentFrustum[i]);
    }


    //bounding box line segments
    U32 bs[] =
            {
        0,1,
        1,3,
        3,2,
        2,0,

        4,5,
        5,7,
        7,6,
        6,4,

        0,4,
        1,5,
        3,7,
        2,6
    };

    for (U32 i = 0; i < 12; i++)
    { //for each line segment in bounding box
        for (U32 j = 0; j < LLCamera::AGENT_PLANE_NO_USER_CLIP_NUM; j++)
        { //for each plane in camera frustum
            const LLPlane& cp = camera.getAgentPlane(j);
            const LLVector3& v1 = pp[bs[i*2+0]];
            const LLVector3& v2 = pp[bs[i*2+1]];
            LLVector3 n;
            cp.getVector3(n);

            LLVector3 line = v1-v2;

            F32 d1 = line*n;
            F32 d2 = -cp.dist(v2);

            F32 t = d2/d1;

            if (t > 0.f && t < 1.f)
            {
                LLVector3 intersect = v2+line*t;
                pp.push_back(intersect);
            }
        }
    }

    //camera frustum line segments
    const U32 fs[] =
    {
        0,1,
        1,2,
        2,3,
        3,0,

        4,5,
        5,6,
        6,7,
        7,4,

        0,4,
        1,5,
        2,6,
        3,7
    };

    for (U32 i = 0; i < 12; i++)
    {
        for (U32 j = 0; j < 6; ++j)
        {
            const LLVector3& v1 = pp[fs[i*2+0]+8];
            const LLVector3& v2 = pp[fs[i*2+1]+8];
            const LLPlane& cp = bp[j];
            LLVector3 n;
            cp.getVector3(n);

            LLVector3 line = v1-v2;

            F32 d1 = line*n;
            F32 d2 = -cp.dist(v2);

            F32 t = d2/d1;

            if (t > 0.f && t < 1.f)
            {
                LLVector3 intersect = v2+line*t;
                pp.push_back(intersect);
            }
        }
    }

    LLVector3 ext[] = { min-LLVector3(0.05f,0.05f,0.05f),
        max+LLVector3(0.05f,0.05f,0.05f) };

    for (U32 i = 0; i < pp.size(); ++i)
    {
        bool found = true;

        const F32* p = pp[i].mV;

        for (U32 j = 0; j < 3; ++j)
        {
            if (p[j] < ext[0].mV[j] ||
                p[j] > ext[1].mV[j])
            {
                found = false;
                break;
            }
        }

        for (U32 j = 0; j < LLCamera::AGENT_PLANE_NO_USER_CLIP_NUM; ++j)
        {
            const LLPlane& cp = camera.getAgentPlane(j);
            F32 dist = cp.dist(pp[i]);
            if (dist > 0.05f) //point is above some plane, not contained
            {
                found = false;
                break;
            }
        }

        if (found)
        {
            fp.push_back(pp[i]);
        }
    }

    if (fp.empty())
    {
        return false;
    }

    return true;
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


LLRenderTarget* LLPipeline::getSunShadowTarget(U32 i)
{
    llassert(i < LLPipeline::kSunShadowCount);
    return &getFrameRT()->shadow[i];
}

LLRenderTarget* LLPipeline::getSpotShadowTarget(U32 i)
{
    llassert(i < LLPipeline::kSpotShadowCount);
    return &mSpotShadow[i];
}

static LLTrace::BlockTimerStatHandle FTM_GEN_SUN_SHADOW("Gen Sun Shadow");
static LLTrace::BlockTimerStatHandle FTM_GEN_SUN_SHADOW_SPOT_RENDER("Spot Shadow Render");

// helper class for disabling occlusion culling for the current stack frame
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

    ShadowWorkerSeeds shadow_seeds;

    // III-1a 3-phase 再構成: prep(Pass 1)=全 cascade/spot の matrix+cull+stateSort(mutation)+seed+pin+ctx を
    // dispatch 前に完了 → dispatch(Pass 2) → render(Pass 3)。sun[0..3]/spot[4..5] を 1 本の slot 空間で扱う。
    ShadowRenderPlan shadow_plan[6];
    static LLCullResult sun_result[4];
    static LLCullResult spot_result[2];
    static bool s_cascade_valid[4] = { false, false, false, false };
    static U32 s_cascade_res[4] = { 0, 0, 0, 0 };

    static LLCachedControl<bool> shadow_record_mt(gSavedSettings, "AYAShadowRecordMT", true);
    {
        static bool s_prev_shadow_record_mt = true;
        if (s_prev_shadow_record_mt != (bool)shadow_record_mt)
        {
            s_prev_shadow_record_mt = shadow_record_mt;
            LL_INFOS("VkPerf") << "AYAShadowRecordMT -> " << (s_prev_shadow_record_mt ? 1 : 0) << LL_ENDL;
        }
    }
    const bool mt_shadow_capable = shadow_record_mt
                                   && !gCubeSnapshot
                                   && LLVKLoader::isVulkanInitialized()
                                   && LLVKLoader::shouldUseVulkanRender()
                                   && LLVKLoader::isBindlessActiveVk();

    if (mSunDiffuse == LLColor4::black)
    { //sun diffuse is totally black shadows don't matter
        skipRenderingShadows();
    }
    else
    {
        static const bool s_shadow_rr = []() -> bool {
            const char* e = getenv("AYASTORM_SHADOW_RR");
            return (e == nullptr) || (atof(e) != 0.0);
        }();
        static U32 s_shadow_frame = 0;
        if (!gCubeSnapshot)
        {
            ++s_shadow_frame;
        }

        for (S32 j = 0; j < (gCubeSnapshot ? 2 : 4); j++)
        {
            LLVKLoader::gVkPerfShadowMapIndex = (U32)j;

            if (s_shadow_rr && !gCubeSnapshot && j >= 2)
            {
                const U32 cur_res = getFrameRT()->shadow[j].getWidth();
                const bool cadence_skip = (j == 2) ? ((s_shadow_frame % 2u) != 0u)
                                                   : ((s_shadow_frame % 4u) != 1u);
                if (cadence_skip && s_cascade_valid[j] && s_cascade_res[j] == cur_res)
                {
                    view[j] = mShadowModelview[j];
                    proj[j] = mShadowProjection[j];
                    mSunShadowMatrix[j] = sGlNdcToSampleBias * proj[j] * view[j] * inv_view;
                    continue;
                }
            }

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

                getFrameRT()->shadow[j].bindTarget();
                {
                    LLGLDepthTest depth(GL_TRUE);
                    getFrameRT()->shadow[j].clear();
                }
                getFrameRT()->shadow[j].flush();
                getFrameRT()->shadow[j].bindForShaderRead(0, true);

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


            {
                static const F32 s_cull_texels = []() -> F32 {
                    const char* e = getenv("AYASTORM_SHADOW_CULL_TEXELS");
                    return (e != nullptr) ? (F32)atof(e) : 1.0f;
                }();
                const F32 split_span = llmax(max.mV[0] - min.mV[0], max.mV[2] - min.mV[2]);
                const F32 map_res = (F32)getFrameRT()->shadow[j].getWidth();
                const F32 batch_cull_radius =
                    (s_cull_texels > 0.f && split_span > 0.f && map_res > 0.f)
                        ? s_cull_texels * split_span / map_res
                        : 0.f;

                LLRenderTarget& shadow_rt = getFrameRT()->shadow[j];

                const bool mt_eligible = mt_shadow_capable
                                && shadow_rt.getVkDepthImage() != VK_NULL_HANDLE
                                && shadow_rt.getVkDepthView() != VK_NULL_HANDLE;

                // III-1a Pass 1 (prep): mutation (updateCull+stateSort=rebuildMesh) は worker 窓が
                // 開く前に完了させる。mt 対象は plan へ退避 (dispatch/render は Pass 2/3)。
                // 非 mt はここでインライン描画 = 従来挙動 (Pass 1 は窓を開かないので安全)。
                bool deferred = false;
                if (mt_eligible)
                {
                    {
                        LLPipelineFrameContext::getInstance().setShadowPass(true);
                        U32 saved_occlusion = sUseOcclusion;
                        sUseOcclusion = 0;
                        LLGLEnable cull(GL_CULL_FACE);
                        LLGLEnable clamp_depth(GL_DEPTH_CLAMP);
                        LLGLDepthTest depth_test(GL_TRUE, GL_TRUE, GL_LESS);
                        updateCull(shadow_cam, sun_result[j]);
                        stateSort(shadow_cam, sun_result[j]);
                        sUseOcclusion = saved_occlusion;
                        LLPipelineFrameContext::getInstance().setShadowPass(false);
                    }

                    ensureShadowWorkerSeeds(shadow_seeds, shadow_rt);

                    if (shadow_seeds.valid)
                    {
                        pinShadowWorkerDrawInfos(sun_result[j]);

                        ShadowRenderPlan& p = shadow_plan[j];
                        p.mt          = true;
                        p.shadow_cam  = shadow_cam;
                        p.cull_radius = batch_cull_radius;
                        p.rt          = &shadow_rt;
                        p.result      = &sun_result[j];
                        p.depth_clamp = true;
                        p.sun_j       = j;
                        std::memcpy(p.ctx.view, glm::value_ptr(view[j]), sizeof(p.ctx.view));
                        std::memcpy(p.ctx.proj, glm::value_ptr(proj[j]), sizeof(p.ctx.proj));
                        std::memcpy(p.ctx.last_modelview, gGLLastModelView, sizeof(p.ctx.last_modelview));
                        p.ctx.cull_radius    = batch_cull_radius;
                        p.ctx.result         = &sun_result[j];
                        p.ctx.rt             = &shadow_rt;
                        p.ctx.depth_image    = shadow_rt.getVkDepthImage();
                        p.ctx.depth_view     = shadow_rt.getVkDepthView();
                        p.ctx.width          = shadow_rt.getWidth();
                        p.ctx.height         = shadow_rt.getHeight();
                        p.ctx.initial_layout = shadow_rt.getVkDepthLayout();
                        p.ctx.map_index      = (U32)j;
                        p.ctx.color_mask_off = RenderShadowDetail <= 2;
                        p.ctx.depth_clamp    = true;
                        p.ctx.seeds          = &shadow_seeds.map;
                        deferred = true;
                    }
                }

                if (!deferred)
                {
                    shadow_rt.bindTarget();
                    shadow_rt.getViewport(gGLViewport);
                    shadow_rt.clear();

                    LLRenderPass::sShadowBatchCullRadius = batch_cull_radius;
                    renderShadow(view[j], proj[j], shadow_cam, sun_result[j], true, false);
                    LLRenderPass::sShadowBatchCullRadius = 0.f;

                    if (!gCubeSnapshot)
                    {
                        s_cascade_valid[j] = true;
                        s_cascade_res[j]   = getFrameRT()->shadow[j].getWidth();
                    }

                    shadow_rt.flush();
                    shadow_rt.bindForShaderRead(0, true);
                }
            }

            if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHADOW_FRUSTA) && !gCubeSnapshot)
            {
                mShadowCamera[j + LLPipeline::kSunShadowCount] = shadow_cam;
            }
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

                LLRenderTarget& spot_rt = mSpotShadow[i];

                const bool mt_eligible = mt_shadow_capable
                                && spot_rt.getVkDepthImage() != VK_NULL_HANDLE
                                && spot_rt.getVkDepthView() != VK_NULL_HANDLE;

                // III-1a Pass 1 (prep): sun cascade と同一方針。mt は plan 退避 (dispatch/render は Pass 2/3)。
                bool deferred = false;
                if (mt_eligible)
                {
                    RenderSpotLight = drawable;
                    {
                        LLPipelineFrameContext::getInstance().setShadowPass(true);
                        U32 saved_occlusion = sUseOcclusion;
                        sUseOcclusion = 0;
                        LLGLEnable cull(GL_CULL_FACE);
                        LLGLDepthTest depth_test(GL_TRUE, GL_TRUE, GL_LESS);
                        updateCull(shadow_cam, spot_result[i]);
                        stateSort(shadow_cam, spot_result[i]);
                        sUseOcclusion = saved_occlusion;
                        LLPipelineFrameContext::getInstance().setShadowPass(false);
                    }
                    RenderSpotLight = nullptr;

                    ensureShadowWorkerSeeds(shadow_seeds, spot_rt);

                    if (shadow_seeds.valid)
                    {
                        pinShadowWorkerDrawInfos(spot_result[i]);

                        ShadowRenderPlan& p = shadow_plan[4 + i];
                        p.mt          = true;
                        p.shadow_cam  = shadow_cam;
                        p.cull_radius = 0.f;
                        p.rt          = &spot_rt;
                        p.result      = &spot_result[i];
                        p.depth_clamp = false;
                        p.sun_j       = -1;
                        p.spot_light  = drawable;
                        std::memcpy(p.ctx.view, glm::value_ptr(view[i + 4]), sizeof(p.ctx.view));
                        std::memcpy(p.ctx.proj, glm::value_ptr(proj[i + 4]), sizeof(p.ctx.proj));
                        std::memcpy(p.ctx.last_modelview, gGLLastModelView, sizeof(p.ctx.last_modelview));
                        p.ctx.cull_radius    = 0.f;
                        p.ctx.result         = &spot_result[i];
                        p.ctx.rt             = &spot_rt;
                        p.ctx.depth_image    = spot_rt.getVkDepthImage();
                        p.ctx.depth_view     = spot_rt.getVkDepthView();
                        p.ctx.width          = spot_rt.getWidth();
                        p.ctx.height         = spot_rt.getHeight();
                        p.ctx.initial_layout = spot_rt.getVkDepthLayout();
                        p.ctx.map_index      = 4u + (U32)i;
                        p.ctx.color_mask_off = RenderShadowDetail <= 2;
                        p.ctx.depth_clamp    = false;
                        p.ctx.seeds          = &shadow_seeds.map;
                        deferred = true;
                    }
                }

                if (!deferred)
                {
                    RenderSpotLight = drawable;
                    spot_rt.bindTarget();
                    spot_rt.getViewport(gGLViewport);
                    spot_rt.clear();

                    renderShadow(view[i + 4], proj[i + 4], shadow_cam, spot_result[i], false, false);

                    RenderSpotLight = nullptr;

                    spot_rt.flush();
                    spot_rt.bindForShaderRead(0, true);
                }
            }
        }
    }
    else
    { //no spotlight shadows
        mShadowSpotLight[0] = mShadowSpotLight[1] = NULL;
    }

    // III-1a Pass 2 (dispatch): 全 cascade/spot の mutation 完了後にまとめて worker へ dispatch。
    // ここで初めて record 窓が開く (以降 Pass 3 も含め mutation ゼロ = S8-b 根治)。
    for (S32 k = 0; k < 6; ++k)
    {
        ShadowRenderPlan& p = shadow_plan[k];
        if (!p.mt)
        {
            continue;
        }
        LLVKLoader::endDynamicRendering();
        ShadowRecordCtx ctx = p.ctx;
        const bool dispatched = LLVKLoader::dispatchRecordJob(
            [ctx](VkCommandBuffer cmd) { recordShadowWorkerFamilies(ctx, cmd); });
        if (dispatched)
        {
            p.rt->setVkDepthLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        }
        else
        {
            p.dispatch_failed = true;
        }
    }

    // III-1a Pass 3 (render): worker 窓が開いた状態で main が static+gltf を LOAD 描画。
    // renderShadow は mt_split=true で updateCull/stateSort/rebuildMesh を呼ばない = 窓中 mutation ゼロ。
    // cull は grabReferences で slot 別 result を明示 set (Pass 1 の last stateSort が別 result を残すため)。
    for (S32 k = 0; k < 6; ++k)
    {
        ShadowRenderPlan& p = shadow_plan[k];
        if (!p.mt)
        {
            continue;
        }
        grabReferences(*p.result);
        p.rt->bindTarget();
        p.rt->getViewport(gGLViewport);
        if (p.dispatch_failed)
        {
            p.rt->clear();
        }
        if (p.sun_j < 0)
        {
            RenderSpotLight = p.spot_light;
        }
        LLRenderPass::sShadowBatchCullRadius = p.cull_radius;
        renderShadow(view[k], proj[k], p.shadow_cam, *p.result, p.depth_clamp, true);
        LLRenderPass::sShadowBatchCullRadius = 0.f;
        if (p.sun_j < 0)
        {
            RenderSpotLight = nullptr;
        }
        if (p.sun_j >= 0 && !gCubeSnapshot)
        {
            s_cascade_valid[p.sun_j] = true;
            s_cascade_res[p.sun_j]   = getFrameRT()->shadow[p.sun_j].getWidth();
        }
        p.rt->flush();
        p.rt->bindForShaderRead(0, true);
    }

    LLVKLoader::joinRecordJobs();
    sShadowRecordPins.clear();

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

void LLPipeline::renderGroups(LLRenderPass* pass, U32 type, bool texture)
{
    for (LLCullResult::sg_iterator i = getFrameCull()->beginVisibleGroups(); i != getFrameCull()->endVisibleGroups(); ++i)
    {
        LLSpatialGroup* group = *i;
        if (!group->isDead() &&
            (!sUseOcclusion || !group->isOcclusionState(LLSpatialGroup::OCCLUDED)) &&
            gPipeline.hasRenderType(group->getSpatialPartition()->mDrawableType) &&
            group->mDrawMap.find(type) != group->mDrawMap.end())
        {
            pass->renderGroup(group,type,texture);
        }
    }
}

void LLPipeline::renderRiggedGroups(LLRenderPass* pass, U32 type, bool texture)
{
    for (LLCullResult::sg_iterator i = getFrameCull()->beginVisibleGroups(); i != getFrameCull()->endVisibleGroups(); ++i)
    {
        LLSpatialGroup* group = *i;
        if (!group->isDead() &&
            (!sUseOcclusion || !group->isOcclusionState(LLSpatialGroup::OCCLUDED)) &&
            gPipeline.hasRenderType(group->getSpatialPartition()->mDrawableType) &&
            group->mDrawMap.find(type) != group->mDrawMap.end())
        {
            pass->renderRiggedGroup(group, type, texture);
        }
    }
}

void LLPipeline::profileAvatar(LLVOAvatar* avatar, bool profile_attachments)
{
    if (gGLManager.mGLVersion < 3.25f)
    { // profiling requires GL 3.3 or later
        return;
    }

    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    // don't continue to profile an avatar that is known to be too slow
    llassert(!avatar->isTooSlow());

    LLGLSLShader* cur_shader = LLGLSLShader::sCurBoundShaderPtr;

    getFrameRT()->deferredScreen.bindTarget();
    getFrameRT()->deferredScreen.clear();

    if (!profile_attachments)
    {
        // profile entire avatar all at once and readback asynchronously
        avatar->placeProfileQuery();

        LLTimer cpu_timer;

        generateImpostor(avatar, false, true);

        avatar->mCPURenderTime = (F32)cpu_timer.getElapsedTimeF32() * 1000.f;

        avatar->readProfileQuery(5); // allow up to 5 frames of latency
    }
    else
    {
        // profile attachments one at a time
        LLVOAvatar::attachment_map_t::iterator iter;
        LLVOAvatar::attachment_map_t::iterator begin = avatar->mAttachmentPoints.begin();
        LLVOAvatar::attachment_map_t::iterator end = avatar->mAttachmentPoints.end();

        for (iter = begin;
            iter != end;
            ++iter)
        {
            LLViewerJointAttachment* attachment = iter->second;
            for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
                attachment_iter != attachment->mAttachedObjects.end();
                ++attachment_iter)
            {
                LLViewerObject* attached_object = attachment_iter->get();
                if (attached_object)
                {
                    if (LLVKLoader::isVulkanInitialized() && LLVKLoader::isTimestampSupportedVk())
                    {
                        const LLUUID aid = attached_object->getID();
                        if (mPendingAttachmentProfiles.find(aid) != mPendingAttachmentProfiles.end())
                        {
                            continue;
                        }
                        VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
                        uint32_t h = LLVKLoader::acquireTimestampPairVk();
                        if (cmd != VK_NULL_HANDLE && h != 0)
                        {
                            LLVKLoader::cmdWriteTimestampBeginVk(cmd, h);
                            generateImpostor(avatar, false, true, attached_object);
                            LLVKLoader::cmdWriteTimestampEndVk(cmd, h);
                            mPendingAttachmentProfiles[aid] = h;
                        }
                        else
                        {
                            if (h != 0)
                            {
                                LLVKLoader::releaseTimestampPairVk(h);
                            }
                            generateImpostor(avatar, false, true, attached_object);
                        }
                    }
                    else
                    {
                        generateImpostor(avatar, false, true, attached_object);
                    }
                }
            }
        }
    }

    getFrameRT()->deferredScreen.flush();

    if (cur_shader)
    {
        cur_shader->bind();
    }
}

void LLPipeline::enqueueProfileAvatar(const LLUUID& id)
{
    if (mPendingProfileSet.insert(id).second)
    {
        mPendingProfileAvatars.push_back(id);
    }
}

void LLPipeline::drainPendingProfileAvatars(S32 max_count)
{
    for (S32 i = 0; i < max_count && !mPendingProfileAvatars.empty(); ++i)
    {
        LLUUID id = mPendingProfileAvatars.front();
        mPendingProfileAvatars.pop_front();
        mPendingProfileSet.erase(id);

        LLViewerObject* obj = gObjectList.findObject(id);
        if (obj && !obj->isDead() && obj->isAvatar() && obj->mDrawable)
        {
            LLVOAvatar* av = (LLVOAvatar*)obj;
            if (!av->isControlAvatar() && !av->isTooSlow())
            {
                profileAvatar(av);
            }
        }
    }
}

void LLPipeline::drainPendingAttachmentProfiles()
{
    for (std::unordered_map<LLUUID, uint32_t>::iterator it = mPendingAttachmentProfiles.begin();
         it != mPendingAttachmentProfiles.end();)
    {
        bool available = false;
        uint64_t ns = 0;
        LLVKLoader::getTimestampElapsedNsVk(it->second, available, ns);
        if (available)
        {
            LLViewerObject* obj = gObjectList.findObject(it->first);
            if (obj)
            {
                obj->mGPURenderTime = (F32)ns / 1000000.f;
            }
            LLVKLoader::releaseTimestampPairVk(it->second);
            it = mPendingAttachmentProfiles.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void LLPipeline::generateImpostor(LLVOAvatar* avatar, bool preview_avatar, bool for_profile, LLViewerObject* specific_attachment)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    LL_PROFILE_GPU_ZONE("generateImpostor");

    static LLCullResult result;
    result.clear();
    grabReferences(result);

    if (!avatar || avatar->isDead() || !avatar->mDrawable)
    {
        LL_WARNS_ONCE("AvatarRenderPipeline") << "Avatar is " << (avatar ? "not drawable" : "null") << LL_ENDL;
        return;
    }
    LL_DEBUGS_ONCE("AvatarRenderPipeline") << "Avatar " << avatar->getID() << " is drawable" << LL_ENDL;

    assertInitialized();

    // previews can't be muted or impostered
    bool visually_muted = !for_profile && !preview_avatar && avatar->isVisuallyMuted();
    LL_DEBUGS_ONCE("AvatarRenderPipeline") << "Avatar " << avatar->getID()
                              << " is " << ( visually_muted ? "" : "not ") << "visually muted"
                              << LL_ENDL;
    bool too_complex = !for_profile && !preview_avatar && avatar->isTooComplex();
    LL_DEBUGS_ONCE("AvatarRenderPipeline") << "Avatar " << avatar->getID()
                              << " is " << ( too_complex ? "" : "not ") << "too complex"
                              << LL_ENDL;

    pushRenderTypeMask();

    if (visually_muted || too_complex)
    {
        // only show jelly doll geometry
        andRenderTypeMask(LLPipeline::RENDER_TYPE_AVATAR,
                            LLPipeline::RENDER_TYPE_CONTROL_AV,
                            END_RENDER_TYPES);
    }
    else
    {
        //hide world geometry
        clearRenderTypeMask(
            RENDER_TYPE_SKY,
            RENDER_TYPE_WL_SKY,
            RENDER_TYPE_TERRAIN,
            RENDER_TYPE_GRASS,
            RENDER_TYPE_CONTROL_AV, // Animesh
            RENDER_TYPE_TREE,
            RENDER_TYPE_VOIDWATER,
            RENDER_TYPE_WATER,
            RENDER_TYPE_ALPHA_PRE_WATER,
            RENDER_TYPE_PASS_GRASS,
            RENDER_TYPE_HUD,
            RENDER_TYPE_PARTICLES,
            RENDER_TYPE_CLOUDS,
            RENDER_TYPE_HUD_PARTICLES,
            END_RENDER_TYPES
         );
    }

    if (specific_attachment && specific_attachment->isHUDAttachment())
    { //enable HUD rendering
        setRenderTypeMask(RENDER_TYPE_HUD, END_RENDER_TYPES);
    }

    S32 occlusion = sUseOcclusion;
    sUseOcclusion = 0;

    LLPipelineFrameContext::getInstance().setReflectionPass(!isFrameRenderingDeferred());

    LLPipelineFrameContext::getInstance().setShadowPass(true);
    LLPipelineFrameContext::getInstance().setImpostorPass(true);

    LLViewerCamera* viewer_camera = LLViewerCamera::getInstance();

    {
        markVisible(avatar->mDrawable, *viewer_camera);

        if (preview_avatar)
        {
            // Only show rigged attachments for preview
            // For the sake of performance and so that static
            // objects won't obstruct previewing changes
            LLVOAvatar::attachment_map_t::iterator iter;
            for (iter = avatar->mAttachmentPoints.begin();
                iter != avatar->mAttachmentPoints.end();
                ++iter)
            {
                LLViewerJointAttachment *attachment = iter->second;
                for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
                    attachment_iter != attachment->mAttachedObjects.end();
                    ++attachment_iter)
                {
                    LLViewerObject* attached_object = attachment_iter->get();
                    // <FS:Ansariel> FIRE-31966: Some mesh bodies/objects don't show in shape editor previews -> show everything but animesh
                    //if (attached_object)
                    //{
                    //    if (attached_object->isRiggedMesh())
                    //    {
                    //        markVisible(attached_object->mDrawable->getSpatialBridge(), *viewer_camera);
                    //    }
                    //    else
                    //    {
                    //        // sometimes object is a linkset and rigged mesh is a child
                    //        LLViewerObject::const_child_list_t& child_list = attached_object->getChildren();
                    //        for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
                    //            iter != child_list.end(); iter++)
                    //        {
                    //            LLViewerObject* child = *iter;
                    //            if (child->isRiggedMesh())
                    //            {
                    //                markVisible(attached_object->mDrawable->getSpatialBridge(), *viewer_camera);
                    //                break;
                    //            }
                    //        }
                    //    }
                    //}
                    if (attached_object && !attached_object->getControlAvatar())
                    {
                        markVisible(attached_object->mDrawable->getSpatialBridge(), *viewer_camera);
                    }
                    // </FS:Ansariel>
                }
            }
        }
        else
        {
            if (specific_attachment)
            {
                markVisible(specific_attachment->mDrawable->getSpatialBridge(), *viewer_camera);
            }
            else
            {
                LLVOAvatar::attachment_map_t::iterator iter;
                LLVOAvatar::attachment_map_t::iterator begin = avatar->mAttachmentPoints.begin();
                LLVOAvatar::attachment_map_t::iterator end = avatar->mAttachmentPoints.end();

                for (iter = begin;
                    iter != end;
                    ++iter)
                {
                    LLViewerJointAttachment* attachment = iter->second;
                    for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
                        attachment_iter != attachment->mAttachedObjects.end();
                        ++attachment_iter)
                    {
                        LLViewerObject* attached_object = attachment_iter->get();
                        if (attached_object)
                        {
                            markVisible(attached_object->mDrawable->getSpatialBridge(), *viewer_camera);
                        }
                    }
                }
            }
        }
    }

    stateSort(*LLViewerCamera::getInstance(), result);

    LLCamera camera = *viewer_camera;
    LLVector2 tdim;
    U32 resY = 0;
    U32 resX = 0;

    if (!preview_avatar)
    {
        const LLVector4a* ext = avatar->mDrawable->getSpatialExtents();
        LLVector3 pos(avatar->getRenderPosition()+avatar->getImpostorOffset());

        camera.lookAt(viewer_camera->getOrigin(), pos, viewer_camera->getUpAxis());

        LLVector4a half_height;
        half_height.setSub(ext[1], ext[0]);
        half_height.mul(0.5f);

        LLVector4a left;
        left.load3(camera.getLeftAxis().mV);
        left.mul(left);
        llassert(left.dot3(left).getF32() > F_APPROXIMATELY_ZERO);
        left.normalize3fast();

        LLVector4a up;
        up.load3(camera.getUpAxis().mV);
        up.mul(up);
        llassert(up.dot3(up).getF32() > F_APPROXIMATELY_ZERO);
        up.normalize3fast();

        tdim.mV[0] = fabsf(half_height.dot3(left).getF32());
        tdim.mV[1] = fabsf(half_height.dot3(up).getF32());

        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.pushMatrix();

        F32 distance = (pos-camera.getOrigin()).length();
        F32 fov = atanf(tdim.mV[1]/distance)*2.f*RAD_TO_DEG;
        F32 aspect = tdim.mV[0]/tdim.mV[1];
        glm::mat4 persp = glm::perspective(glm::radians(fov), aspect, 1.f, 256.f);
        set_current_projection(persp);
        gGL.loadMatrix(glm::value_ptr(persp));

        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.pushMatrix();

        F32 ogl_mat[16];
        camera.getOpenGLTransform(ogl_mat);
        glm::mat4 mat = glm::make_mat4((GLfloat*) OGL_TO_CFR_ROTATION) * glm::make_mat4(ogl_mat);

        gGL.loadMatrix(glm::value_ptr(mat));
        set_current_modelview(mat);

        gGL.setClearColor(0.0f,0.0f,0.0f,0.0f);
        gGL.setColorMask(true, true);

        // get the number of pixels per angle
        F32 pa = gViewerWindow->getWindowHeightRaw() / (RAD_TO_DEG * viewer_camera->getView());

        //get resolution based on angle width and height of impostor (double desired resolution to prevent aliasing)
        resY = llmin(nhpo2((U32) (fov*pa)), (U32) 512);
        resX = llmin(nhpo2((U32) (atanf(tdim.mV[0]/distance)*2.f*RAD_TO_DEG*pa)), (U32) 512);

        if (!for_profile)
        {
            if (!avatar->mImpostor.isComplete())
            {
                avatar->mImpostor.allocate(resX, resY, GL_RGBA, true);

                if (isFrameRenderingDeferred())
                {
                    addDeferredAttachments(avatar->mImpostor, true);
                }

                gGL.getTexUnit(0)->bind(&avatar->mImpostor);
                gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
                gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
            }
            else if (resX != avatar->mImpostor.getWidth() || resY != avatar->mImpostor.getHeight())
            {
                avatar->mImpostor.resize(resX, resY);
            }

            avatar->mImpostor.bindTarget();
        }
    }

    F32 old_alpha = LLDrawPoolAvatar::sMinimumAlpha;

    if (visually_muted || too_complex)
    { //disable alpha masking for muted avatars (get whole skin silhouette)
        LLDrawPoolAvatar::sMinimumAlpha = 0.f;
    }

    if (preview_avatar || for_profile)
    {
        // previews and profiles don't care about imposters
        renderGeomDeferred(camera);
        renderGeomPostDeferred(camera);
    }
    else
    {
        avatar->mImpostor.clear();
        renderGeomDeferred(camera);

        renderGeomPostDeferred(camera);

        // Shameless hack time: render it all again,
        // this time writing the depth
        // values we need to generate the alpha mask below
        // while preserving the alpha-sorted color rendering
        // from the previous pass
        //
        sImpostorRenderAlphaDepthPass = true;
        // depth-only here...
        //
        gGL.setColorMask(false,false);
        renderGeomPostDeferred(camera);

        sImpostorRenderAlphaDepthPass = false;

    }

    LLDrawPoolAvatar::sMinimumAlpha = old_alpha;

    if (!for_profile)
    { //create alpha mask based on depth buffer (grey out if muted)
        LLGLDisable blend(GL_BLEND);

        if (visually_muted || too_complex)
        {
            gGL.setColorMask(true, true);
        }
        else
        {
            gGL.setColorMask(false, true);
        }

        gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

        LLGLDepthTest depth(GL_TRUE, GL_FALSE, GL_GREATER);

        gGL.flush();

        gGL.pushMatrix();
        gGL.loadIdentity();
        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.pushMatrix();
        gGL.loadIdentity();

        static const F32 clip_plane = 0.99999f;

        gDebugProgram.bind();

        if (visually_muted)
        {   // Visually muted avatar
            LLColor4 muted_color(avatar->getMutedAVColor());
            LL_DEBUGS_ONCE("AvatarRenderPipeline") << "Avatar " << avatar->getID() << " MUTED set solid color " << muted_color << LL_ENDL;
            gGL.diffuseColor4fv( muted_color.mV );
        }
        else if (!preview_avatar)
        { //grey muted avatar
            LL_DEBUGS_ONCE("AvatarRenderPipeline") << "Avatar " << avatar->getID() << " MUTED set grey" << LL_ENDL;
            gGL.diffuseColor4fv(LLColor4::pink.mV );
        }

        gGL.begin(LLRender::TRIANGLES);
        {
            gGL.vertex3f(-1.f, -1.f, clip_plane);
            gGL.vertex3f(1.f, -1.f, clip_plane);
            gGL.vertex3f(1.f, 1.f, clip_plane);

            gGL.vertex3f(-1.f, -1.f, clip_plane);
            gGL.vertex3f(1.f, 1.f, clip_plane);
            gGL.vertex3f(-1.f, 1.f, clip_plane);
        }
        gGL.end();
        gGL.flush();

        gDebugProgram.unbind();

        gGL.popMatrix();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.popMatrix();
    }

    if (!preview_avatar && !for_profile)
    {
        avatar->mImpostor.flush();
        avatar->setImpostorDim(tdim);
    }

    sUseOcclusion = occlusion;
    LLPipelineFrameContext::getInstance().setReflectionPass(false);
    LLPipelineFrameContext::getInstance().setImpostorPass(false);
    LLPipelineFrameContext::getInstance().setShadowPass(false);
    popRenderTypeMask();

    if (!preview_avatar)
    {
        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.popMatrix();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.popMatrix();
    }

    if (!preview_avatar && !for_profile)
    {
        avatar->mNeedsImpostorUpdate = false;
        avatar->cacheImpostorValues();
        avatar->mLastImpostorUpdateFrameTime = gFrameTimeSeconds;
    }

    LLVertexBuffer::unbind();
}

bool LLPipeline::hasRenderBatches(const U32 type) const
{
    // <FS:ND>  FIRE-31942, getFrameCull() can be invalid if triggering 360 snapshosts fast enough  (due to snapshots running in their own co routine)
    if( !getFrameCull() )
        return {};
    // </FS:ND>

    return getFrameCull()->getRenderMapSize(type) > 0;
}

LLCullResult::drawinfo_iterator LLPipeline::beginRenderMap(U32 type)
{
    // <FS:ND>  FIRE-31942, getFrameCull() can be invalid if triggering 360 snapshosts fast enough  (due to snapshots running in their own co routine)
    if( !getFrameCull() )
        return {};
    // </FS:ND>

    return getFrameCull()->beginRenderMap(type);
}

LLCullResult::drawinfo_iterator LLPipeline::endRenderMap(U32 type)
{
    // <FS:ND>  FIRE-31942, getFrameCull() can be invalid if triggering 360 snapshosts fast enough  (due to snapshots running in their own co routine)
    if( !getFrameCull() )
        return {};
    // </FS:ND>

    return getFrameCull()->endRenderMap(type);
}

LLCullResult::sg_iterator LLPipeline::beginAlphaGroups()
{
    // <FS:ND>  FIRE-31942, getFrameCull() can be invalid if triggering 360 snapshosts fast enough  (due to snapshots running in their own co routine)
    if( !getFrameCull() )
        return {};
    // </FS:ND>

    return getFrameCull()->beginAlphaGroups();
}

LLCullResult::sg_iterator LLPipeline::endAlphaGroups()
{
    // <FS:ND>  FIRE-31942, getFrameCull() can be invalid if triggering 360 snapshosts fast enough  (due to snapshots running in their own co routine)
    if( !getFrameCull() )
        return {};
    // </FS:ND>

    return getFrameCull()->endAlphaGroups();
}

LLCullResult::sg_iterator LLPipeline::beginRiggedAlphaGroups()
{
    // <FS:ND>  FIRE-31942, getFrameCull() can be invalid if triggering 360 snapshosts fast enough  (due to snapshots running in their own co routine)
    if( !getFrameCull() )
        return {};
    // </FS:ND>

    return getFrameCull()->beginRiggedAlphaGroups();
}

LLCullResult::sg_iterator LLPipeline::endRiggedAlphaGroups()
{
    // <FS:ND>  FIRE-31942, getFrameCull() can be invalid if triggering 360 snapshosts fast enough  (due to snapshots running in their own co routine)
    if( !getFrameCull() )
        return {};
    // </FS:ND>

    return getFrameCull()->endRiggedAlphaGroups();
}

bool LLPipeline::hasRenderType(const U32 type) const
{
    // STORM-365 : LLViewerJointAttachment::setAttachmentVisibility() is setting type to 0 to actually mean "do not render"
    // We then need to test that value here and return false to prevent attachment to render (in mouselook for instance)
    // TODO: reintroduce RENDER_TYPE_NONE in LLRenderTypeMask and initialize its mRenderTypeEnabled[RENDER_TYPE_NONE] to false explicitely
    return (type == 0 ? false : mRenderTypeEnabled[type]);
}

void LLPipeline::setRenderTypeMask(U32 type, ...)
{
    va_list args;

    va_start(args, type);
    while (type < END_RENDER_TYPES)
    {
        mRenderTypeEnabled[type] = true;
        type = va_arg(args, U32);
    }
    va_end(args);

    if (type > END_RENDER_TYPES)
    {
        LL_ERRS() << "Invalid render type." << LL_ENDL;
    }
}

bool LLPipeline::hasAnyRenderType(U32 type, ...) const
{
    va_list args;

    va_start(args, type);
    while (type < END_RENDER_TYPES)
    {
        if (mRenderTypeEnabled[type])
        {
            va_end(args);
            return true;
        }
        type = va_arg(args, U32);
    }
    va_end(args);

    if (type > END_RENDER_TYPES)
    {
        LL_ERRS() << "Invalid render type." << LL_ENDL;
    }

    return false;
}

void LLPipeline::pushRenderTypeMask()
{
    std::string cur_mask;
    cur_mask.assign((const char*) mRenderTypeEnabled, sizeof(mRenderTypeEnabled));
    mRenderTypeEnableStack.push(cur_mask);
}

void LLPipeline::popRenderTypeMask()
{
    if (mRenderTypeEnableStack.empty())
    {
        LL_ERRS() << "Depleted render type stack." << LL_ENDL;
    }

    memcpy(mRenderTypeEnabled, mRenderTypeEnableStack.top().data(), sizeof(mRenderTypeEnabled));
    mRenderTypeEnableStack.pop();
}

void LLPipeline::andRenderTypeMask(U32 type, ...)
{
    va_list args;

    bool tmp[NUM_RENDER_TYPES];
    for (U32 i = 0; i < NUM_RENDER_TYPES; ++i)
    {
        tmp[i] = false;
    }

    va_start(args, type);
    while (type < END_RENDER_TYPES)
    {
        if (mRenderTypeEnabled[type])
        {
            tmp[type] = true;
        }

        type = va_arg(args, U32);
    }
    va_end(args);

    if (type > END_RENDER_TYPES)
    {
        LL_ERRS() << "Invalid render type." << LL_ENDL;
    }

    for (U32 i = 0; i < LLPipeline::NUM_RENDER_TYPES; ++i)
    {
        mRenderTypeEnabled[i] = tmp[i];
    }

}

void LLPipeline::clearRenderTypeMask(U32 type, ...)
{
    va_list args;

    va_start(args, type);
    while (type < END_RENDER_TYPES)
    {
        mRenderTypeEnabled[type] = false;

        type = va_arg(args, U32);
    }
    va_end(args);

    if (type > END_RENDER_TYPES)
    {
        LL_ERRS() << "Invalid render type." << LL_ENDL;
    }
}

void LLPipeline::setAllRenderTypes()
{
    for (U32 i = 0; i < NUM_RENDER_TYPES; ++i)
    {
        mRenderTypeEnabled[i] = true;
    }
}

void LLPipeline::clearAllRenderTypes()
{
    for (U32 i = 0; i < NUM_RENDER_TYPES; ++i)
    {
        mRenderTypeEnabled[i] = false;
    }
}

void LLPipeline::addDebugBlip(const LLVector3& position, const LLColor4& color)
{
    DebugBlip blip(position, color);
    mDebugBlips.push_back(blip);
}

void LLPipeline::hidePermanentObjects( std::vector<U32>& restoreList )
{
    //This method is used to hide any vo's from the object list that may have
    //the permanent flag set.

    U32 objCnt = gObjectList.getNumObjects();
    for (U32 i = 0; i < objCnt; ++i)
    {
        LLViewerObject* pObject = gObjectList.getObject(i);
        if ( pObject && pObject->flagObjectPermanent() )
        {
            LLDrawable *pDrawable = pObject->mDrawable;

            if ( pDrawable )
            {
                restoreList.push_back( i );
                hideDrawable( pDrawable );
            }
        }
    }

    skipRenderingOfTerrain( true );
}

void LLPipeline::restorePermanentObjects( const std::vector<U32>& restoreList )
{
    //This method is used to restore(unhide) any vo's from the object list that may have
    //been hidden because their permanency flag was set.

    std::vector<U32>::const_iterator itCurrent  = restoreList.begin();
    std::vector<U32>::const_iterator itEnd      = restoreList.end();

    U32 objCnt = gObjectList.getNumObjects();

    while ( itCurrent != itEnd )
    {
        U32 index = *itCurrent;
        LLViewerObject* pObject = NULL;
        if ( index < objCnt )
        {
            pObject = gObjectList.getObject( index );
        }
        if ( pObject )
        {
            LLDrawable *pDrawable = pObject->mDrawable;
            if ( pDrawable )
            {
                pDrawable->clearState( LLDrawable::FORCE_INVISIBLE );
                unhideDrawable( pDrawable );
            }
        }
        ++itCurrent;
    }

    skipRenderingOfTerrain( false );
}

void LLPipeline::skipRenderingOfTerrain( bool flag )
{
    pool_set_t::iterator iter = mPools.begin();
    while ( iter != mPools.end() )
    {
        LLDrawPool* pPool = *iter;
        U32 poolType = pPool->getType();
        if ( hasRenderType( pPool->getType() ) && poolType == LLDrawPool::POOL_TERRAIN )
        {
            pPool->setSkipRenderFlag( flag );
        }
        ++iter;
    }
}

void LLPipeline::hideObject( const LLUUID& id )
{
    LLViewerObject *pVO = gObjectList.findObject( id );

    if ( pVO )
    {
        LLDrawable *pDrawable = pVO->mDrawable;

        if ( pDrawable )
        {
            hideDrawable( pDrawable );
        }
    }
}

void LLPipeline::hideDrawable( LLDrawable *pDrawable )
{
    pDrawable->setState( LLDrawable::FORCE_INVISIBLE );
    markRebuild( pDrawable, LLDrawable::REBUILD_ALL);
    //hide the children
    LLViewerObject::const_child_list_t& child_list = pDrawable->getVObj()->getChildren();
    for ( LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
          iter != child_list.end(); iter++ )
    {
        LLViewerObject* child = *iter;
        LLDrawable* drawable = child->mDrawable;
        if ( drawable )
        {
            drawable->setState( LLDrawable::FORCE_INVISIBLE );
            markRebuild( drawable, LLDrawable::REBUILD_ALL);
        }
    }
}
void LLPipeline::unhideDrawable( LLDrawable *pDrawable )
{
    pDrawable->clearState( LLDrawable::FORCE_INVISIBLE );
    markRebuild( pDrawable, LLDrawable::REBUILD_ALL);
    //restore children
    LLViewerObject::const_child_list_t& child_list = pDrawable->getVObj()->getChildren();
    for ( LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
          iter != child_list.end(); iter++)
    {
        LLViewerObject* child = *iter;
        LLDrawable* drawable = child->mDrawable;
        if ( drawable )
        {
            drawable->clearState( LLDrawable::FORCE_INVISIBLE );
            markRebuild( drawable, LLDrawable::REBUILD_ALL);
        }
    }
}
void LLPipeline::restoreHiddenObject( const LLUUID& id )
{
    LLViewerObject *pVO = gObjectList.findObject( id );

    if ( pVO )
    {
        LLDrawable *pDrawable = pVO->mDrawable;
        if ( pDrawable )
        {
            unhideDrawable( pDrawable );
        }
    }
}

void LLPipeline::skipRenderingShadows()
{
    LLGLDepthTest depth(GL_TRUE);

    for (S32 j = 0; j < 4; j++)
    {
        getFrameRT()->shadow[j].bindTarget();
        getFrameRT()->shadow[j].clear();
        getFrameRT()->shadow[j].flush();
        getFrameRT()->shadow[j].bindForShaderRead(0, true);
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

class LLOctreeDirty : public OctreeTraveler
{
public:
    virtual void visit(const OctreeNode* state)
    {
        LLSpatialGroup* group = (LLSpatialGroup*)state->getListener(0);

        if (group->getSpatialPartition()->mRenderByGroup)
        {
            group->setState(LLSpatialGroup::GEOM_DIRTY);
            ++LLVKLoader::gVkPerf.geo_dirty_site[10];
            gPipeline.markRebuild(group);
        }

        for (LLSpatialGroup::bridge_list_t::iterator i = group->mBridgeList.begin(); i != group->mBridgeList.end(); ++i)
        {
            LLSpatialBridge* bridge = *i;
            traverse(bridge->mOctree);
        }
    }
};

// Called from LLViewHighlightTransparent when "Highlight Transparent" is toggled
void LLPipeline::rebuildDrawInfo()
{
    const U32 types_to_traverse[] =
    {
        LLViewerRegion::PARTITION_VOLUME,
        // <FS:Beq> Fix for alpha blend issues with highlight transparent resulting from fix for LL#2577
        // LLViewerRegion::PARTITION_BRIDGE,
        // LLViewerRegion::PARTITION_AVATAR,
        LLViewerRegion::PARTITION_BRIDGE
        // </FS:Beq>
    };

    LLOctreeDirty dirty;
    for (LLViewerRegion* region : LLWorld::getInstance()->getRegionList())
    {
        for (U32 type : types_to_traverse)
        {
            LLSpatialPartition* part = region->getSpatialPartition(type);
            dirty.traverse(part->mOctree);
        }
    }
}

void LLPipeline::rebuildTerrain()
{
    for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
        iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
    {
        LLViewerRegion* region = *iter;
        region->dirtyAllPatches();
    }
}

// <FS:Ansariel> Reset VB during TP
void LLPipeline::initDeferredVB()
{
    mDeferredVB = new LLVertexBuffer(DEFERRED_VB_MASK);
    if (!mDeferredVB->allocateBuffer(8, 0))
    {
        // Most likely going to crash...
        LL_WARNS() << "Failed to allocate Vertex Buffer for deferred rendering" << LL_ENDL;
    }
}
// </FS:Ansariel>
