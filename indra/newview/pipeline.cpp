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


// PCH 経由で Xlib (X11/Xlib.h:84) の `#define None 0L` が流入し、enum class member
// 等の `None` トークンを数値リテラル `0L` に置換してしまう。pipeline.cpp は X11
// API を直接呼ばないのでファイル冒頭で undef して局所的に無効化する。
// memory: project_linux_xlib_status_define_trap.md
#undef None

#include "llpipelineinternal.h"

extern bool gSnapshot;
bool gShiftFrame = false;




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
LLVector4 LLPipeline::RenderShadowFarClipVec;
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



extern S32 gBoxFrame;
extern bool gDisplaySwapBuffers;
extern bool gDebugGL;

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


// Add color attachments for deferred rendering
// target -- RenderTarget to add attachments to

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



LLPipeline::~LLPipeline()
{
}
















// must be even to avoid a stripe in the horizontal shadow blur











F32 lerpf(F32 a, F32 b, F32 w)
{
    return a + w * (b - a);
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







static LLTrace::BlockTimerStatHandle FTM_CULL("Object Culling");


































//function for creating scripted beacons



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





U32 LLPipeline::sCurRenderPoolType = 0 ;













extern std::set<LLSpatialGroup*> visible_selected_groups;




















class LLMenuItemGL;
class LLInvFVBridge;
struct cat_folder_pair;
class LLVOBranch;
class LLVOLeaf;



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





extern LLPointer<LLImageGL> gEXRImage;




















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





















inline float sgn(float a)
{
    if (a > 0.0F) return (1.0F);
    if (a < 0.0F) return (-1.0F);
    return (0.0F);
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














static LLTrace::BlockTimerStatHandle FTM_GEN_SUN_SHADOW("Gen Sun Shadow");
static LLTrace::BlockTimerStatHandle FTM_GEN_SUN_SHADOW_SPOT_RENDER("Spot Shadow Render");

// helper class for disabling occlusion culling for the current stack frame







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












// </FS:Ansariel>
