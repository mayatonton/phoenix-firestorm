/**
 * @file llpipelinealloc.cpp
 * @brief Rendering pipeline: init/cleanup and buffer allocation (pure move from pipeline.cpp).
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

#include "SMAAAreaTex.h"
#include "SMAASearchTex.h"
#include "llerror.h"
#include "llpipelineinternal.h"

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
    connectRefreshCachedSettingsSafe("RenderShadowDistance");
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

void LLPipeline::cleanup()
{
    assertInitialized();

    mGroupQ1.clear() ;

    mBuildQ1.clear();
    mPartitionQ.clear();
    mCreateQ.clear();
    mMeshDirtyGroup.clear();
    mLights.clear();
    mNearbyLights.clear();
    mRetexturedList.clear();
    mGroupSaveQ1.clear();
    for (U32 i = 0; i < kSpotShadowCount; ++i)
    {
        mShadowSpotLight[i] = NULL;
        mTargetShadowSpotLight[i] = NULL;
    }

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

bool LLPipeline::resizeScreenTexture()
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
            eFBOStatus ret = doAllocateScreenBuffer(resX, resY);
            if (ret == FBO_SUCCESS_LOWRES)
            {
                LL_WARNS("Pipeline") << "resizeScreenTexture: allocated at reduced resolution" << LL_ENDL;
            }
            return ret != FBO_FAILURE;
        }
    }
    return true;
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

bool LLPipeline::mainChainComplete() const
{
    return getFrameRT()->screen.isComplete();
}

bool LLPipeline::probeChainComplete() const
{
    return mAuxillaryRT.screen.isComplete();
}

bool LLPipeline::heroChainComplete() const
{
    return mHeroProbeRT.screen.isComplete();
}

bool LLPipeline::allocateProbeChains()
{
    if (getFrameRT() != &mMainRT)
    {
        return true;
    }

    LLPipeline::RenderTargetPack* saved = LLPipelineFrameContext::getInstance().getActiveRT();
    bool ok = true;

    gCubeSnapshot = true;

    if (isFrameReflectionProbesEnabled())
    {
        mReflectionMapManager.initReflectionMaps();
    }

    LLPipelineFrameContext::getInstance().setActiveRT(&mAuxillaryRT);
    U32 res = mReflectionMapManager.mProbeResolution * 4;
    if (!allocateScreenBufferInternal(res, res))
    {
        mAuxillaryRT.screen.release();
        mAuxillaryRT.deferredScreen.release();
        mAuxillaryRT.deferredLight.release();
        mAuxillaryRT.sunShadowLayered.release();
        ok = false;
    }

    if (RenderMirrors)
    {
        mHeroProbeManager.initReflectionMaps();
        res = mHeroProbeManager.mProbeResolution;
        LLPipelineFrameContext::getInstance().setActiveRT(&mHeroProbeRT);
        if (!allocateScreenBufferInternal(res, res))
        {
            mHeroProbeRT.screen.release();
            mHeroProbeRT.deferredScreen.release();
            mHeroProbeRT.deferredLight.release();
            mHeroProbeRT.sunShadowLayered.release();
            ok = false;
        }
    }

    LLPipelineFrameContext::getInstance().setActiveRT(saved);
    gCubeSnapshot = false;

    return ok;
}

bool LLPipeline::allocateScreenBufferInternal(U32 resX, U32 resY)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;

    static LLCachedControl<bool> has_hdr(gSavedSettings, "RenderHDREnabled", true);
    bool hdr = gGLManager.mGLVersion > 4.05f && has_hdr();

    allocateProbeChains();

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

    if (!allocateShadowBuffer(resX, resY)) return false;

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
        if (!mWaterDis.allocate(resX, resY, screenFormat, true)) return false;

        if(RenderScreenSpaceReflections)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("SSRBuffer"); // <FS:Beq/> improve Tracy scoping
            if (!mSceneMap.allocate(resX, resY, screenFormat, true)) return false;
        }
        else
        {
            mSceneMap.release();
        }

        {LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("mPostMapBuffer"); // <FS:Beq/> improve Tracy scoping
        if (!mPostPingMap.allocate(resX, resY, GL_RGBA)) return false;
        if (!mPostPongMap.allocate(resX, resY, GL_RGBA)) return false;
        } // <FS:Beq/> improve Tracy scoping
        // The water exclusion mask needs its own depth buffer so we can take care of the problem of multiple water planes.
        // Should we ever make water not just a plane, it also aids with that as well as the water planes will be rendered into the mask.
        // Why do we do this? Because it saves us some janky logic in the exclusion shader when we generate the mask.
        // Regardless, this should always only be an R8 texture unless we choose to start having multiple kinds of exclusion that 8 bits can't handle.
        // - Geenz 2025-02-06
        if (!mWaterExclusionMask.allocate(resX, resY, GL_R8, true)) return false;

        // used to scale down textures
        // See LLViwerTextureList::updateImagesCreateTextures and LLImageGL::scaleDown
        {LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("DownResBuffer");// <FS:Beq/> create an independent preview screen target
        if (!mDownResMap.allocate(1024, 1024, GL_RGBA)) return false;
        }// <FS:Beq/> create an independent preview screen target

        // <FS:Beq> create an independent preview screen target
        {LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("PreviewScreenBuffer");
        if (!mPreviewScreen.allocate(MAX_PREVIEW_WIDTH, MAX_PREVIEW_HEIGHT, GL_RGBA, true)) return false;
        } // </FS:Beq>
        {LL_PROFILE_ZONE_NAMED_CATEGORY_DISPLAY("BakeMapBuffer");// <FS:Beq/> create an independent preview screen target
        if (!mBakeMap.allocate(LLAvatarAppearanceDefines::SCRATCH_TEX_WIDTH, LLAvatarAppearanceDefines::SCRATCH_TEX_HEIGHT, GL_RGBA)) return false;
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
    RenderShadowFarClipVec = gSavedSettings.getVector4("RenderShadowDistance");
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
    mColorGradingLUTValid = false;

    mPbrBrdfLut.release();

    mExposureMap.release();
    mLuminanceMap.release();
    mLastExposure.release();

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

    // <AYAstorm r30 P5 transparent-DoF C-(a)> alpha BLEND color RT
    mAYAAlphaColor.release();
    // </AYAstorm r30 P5 transparent-DoF C-(a)>

    mForwardColor.release();
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

bool LLPipeline::loadColorGradingLUT(const std::string& filename)
{
    mColorGradingLUTValid = false;
    mColorGradingLUTName = filename;

    int lut_size = 0;
    std::vector<float> lut_data;
    bool loaded = false;

    if (!filename.empty())
    {
        std::string path = filename;
        if (!gDirUtilp->fileExists(path))
            path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "luts", filename);

        llifstream file(path.c_str());
        if (!file.is_open())
        {
            LL_WARNS("LUT") << "Failed to open LUT file: " << path << LL_ENDL;
        }
        else
        {
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
                lut_size = 0;
                lut_data.clear();
            }
            else
            {
                loaded = true;
                LL_INFOS("LUT") << "Loaded color grading LUT: " << path << LL_ENDL;
            }
        }
    }

    if (!loaded)
    {
        lut_size = 2;
        lut_data.clear();
        lut_data.reserve((size_t)lut_size * lut_size * lut_size * 3);
        for (int z = 0; z < lut_size; ++z)
            for (int y = 0; y < lut_size; ++y)
                for (int x = 0; x < lut_size; ++x)
                {
                    lut_data.push_back((float)x / (float)(lut_size - 1));
                    lut_data.push_back((float)y / (float)(lut_size - 1));
                    lut_data.push_back((float)z / (float)(lut_size - 1));
                }
    }

    mColorGradingLUT = new LLImageGL(lut_size, lut_size, 3, false);
    mColorGradingLUT->setTarget(GL_TEXTURE_3D, LLTexUnit::TT_TEXTURE_3D);
    mColorGradingLUT->syncVulkan3DImage(GL_RGB16F, GL_RGB, GL_FLOAT, lut_size, lut_size, lut_size, lut_data.data());

    mColorGradingLUTValid = loaded;
    return filename.empty() || loaded;
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
    {
        LLRTScope rts(mExposureMap, false, "lut_exposure");
        if (rts)
        {
            gGL.setClearColor(1, 1, 1, 0);
            mExposureMap.clear();
            gGL.setClearColor(0, 0, 0, 0);
        }
    }

    mLuminanceMap.allocate(256, 256, GL_R16F, false, LLTexUnit::TT_TEXTURE, LLTexUnit::TMG_AUTO);

    mLastExposure.allocate(1, 1, GL_R16F);
    {
        LLRTScope rts(mLastExposure, false, "lut_lastexposure");
        if (rts)
        {
            gGL.setClearColor(1, 1, 1, 0);
            mLastExposure.clear();
            gGL.setClearColor(0, 0, 0, 0);
        }
    }
}

void LLPipeline::generateBrdfLut()
{
    LLRTScope rts(mPbrBrdfLut, false, "brdf_lut");
    if (rts)
    {
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
    }
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

