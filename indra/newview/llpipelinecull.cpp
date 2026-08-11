/**
 * @file llpipelinecull.cpp
 * @brief Rendering pipeline: cull/occlusion/sort (pure move from pipeline.cpp).
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

static void vkcScanEmptyDrawmapDefects(LLSpatialGroup* group, U32& hasgeom, U32& zerogeomv,
                                       std::vector<LLUUID>& hole_uuids,
                                       std::vector<std::string>& zerogeomv_info)
{
    static LLCachedControl<bool> sa_protect(gSavedSettings, "RenderVolumeSAProtection");
    static LLCachedControl<F32> volume_sa_thresh(gSavedSettings, "RenderVolumeSAThreshold");
    static LLCachedControl<F32> sculpt_sa_thresh(gSavedSettings, "RenderSculptSAThreshold");

    for (LLSpatialGroup::element_iter it = group->getDataBegin(); it != group->getDataEnd(); ++it)
    {
        LLDrawable* drawablep = (LLDrawable*)(*it)->getDrawable();
        if (!drawablep || drawablep->isDead() || drawablep->isState(LLDrawable::FORCE_INVISIBLE))
        {
            continue;
        }
        if (drawablep->isState(LLDrawable::IN_REBUILD_Q))
        {
            continue;
        }
        if (LLPipeline::isParcelHideAlive(drawablep))
        {
            continue;
        }
        LLVOVolume* vobj = drawablep->getVOVolume();
        if (!vobj || vobj->isDead())
        {
            continue;
        }
        if (drawablep->isState(LLDrawable::RIGGED | LLDrawable::RIGGED_CHILD))
        {
            continue;
        }
        if (vobj->mGLTFAsset)
        {
            continue;
        }
        if (vobj->isMesh())
        {
            if ((vobj->getVolume() && !vobj->getVolume()->isMeshAssetLoaded()) || !gMeshRepo.meshRezEnabled())
            {
                continue;
            }
            if (!vobj->getSkinInfo() && !vobj->isSkinInfoUnavaliable())
            {
                continue;
            }
        }
        if (sa_protect && vobj->mVolumeSurfaceArea > (vobj->isSculpted() ? (F32)sculpt_sa_thresh : (F32)volume_sa_thresh))
        {
            continue;
        }
        bool any_geom = false;
        bool any_visible = false;
        bool any_visible_te = false;
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
            if (visible_te)
            {
                any_visible = true;
            }
        }
        if (any_visible)
        {
            ++hasgeom;
            if (hole_uuids.size() < 8)
            {
                hole_uuids.push_back(vobj->getID());
            }
        }
        else if (!any_geom && any_visible_te)
        {
            LLVolume* dbg_vol = vobj->getVolume();
            const S32 vdet = dbg_vol ? LLVolumeLODGroup::getVolumeDetailFromScale(dbg_vol->getDetail()) : -1;
            const S32 want = (vobj->isMesh() && dbg_vol) ? gMeshRepo.getActualMeshLOD(dbg_vol->getParams(), vobj->getLOD()) : vobj->getLOD();
            if (vobj->isGeometryDrawExpected() || vdet != want)
            {
                ++zerogeomv;
                if (zerogeomv_info.size() < 4)
                {
                    zerogeomv_info.push_back(llformat("%s mesh=%d vf=%d df=%d st=0x%x nv=%d vdet=%d want=%d",
                        vobj->getID().asString().c_str(), (S32)vobj->isMesh(),
                        dbg_vol ? dbg_vol->getNumVolumeFaces() : -1,
                        drawablep->getNumFaces(), (U32)drawablep->getState(),
                        (dbg_vol && dbg_vol->getNumVolumeFaces() > 0)
                            ? dbg_vol->getVolumeFace(0).mNumVertices : -1,
                        vdet, want));
                }
            }
        }
    }
}

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

bool LLPipeline::getVisibleExtents(LLCamera& camera, LLVector3& min, LLVector3& max, S32 use_occlusion)
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
                    if (!part->getVisibleExtents(camera, min, max, use_occlusion))
                    {
                        res = false;
                    }
                }
            }
        }
    }

    return res;
}

bool LLPipeline::isWaterClip()
{
    // We always pretend that we're not clipping water when rendering mirrors.
    return (gPipeline.mHeroProbeManager.isMirrorPass()) ? false : (!sRenderTransparentWater || gCubeSnapshot) && !isFrameHUDPass();
}

// </FS:AYAstorm>

void LLPipeline::updateCull(LLCamera& camera, LLCullResult& result, S32 use_occlusion, bool hud_attachments)
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
    getFrameCull()->setUseOcclusion(use_occlusion);

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
                    part->cull(camera, use_occlusion);
                }
            }
        }

        //scan the VO Cache tree
        LLVOCachePartition* vo_part = region->getVOCachePartition();
        if(vo_part)
        {
            // <FS:Beq> Fix area search again
            //vo_part->cull(camera, sUseOcclusion > 0);
            vo_part->cull(camera, gAgent.getFSAreaSearchActive() ? 0 : use_occlusion);
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
        markOccluder(group, getFrameCull()->getUseOcclusion());
    }
    mNumVisibleNodes++;
}

void LLPipeline::markOccluder(LLSpatialGroup* group, S32 use_occlusion)
{
    if (use_occlusion > 1 && group && !group->isOcclusionQueuedThisFrame((U32)gFrameCount))
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

    const S32 use_occlusion = getFrameCull()->getUseOcclusion();

    if (isFrameReflectionProbesEnabled() && use_occlusion > 1 && !isFrameShadowPass() && !gCubeSnapshot)
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

    if (use_occlusion > 1 &&
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
                group->doOcclusion(&camera, use_occlusion);
            }
        }

        //apply occlusion culling to object cache tree
        for (LLWorld::region_list_t::const_iterator iter = LLWorld::getInstance()->getRegionList().begin();
            iter != LLWorld::getInstance()->getRegionList().end(); ++iter)
        {
            LLVOCachePartition* vo_part = (*iter)->getVOCachePartition();
            if(vo_part)
            {
                vo_part->processOccluders(&camera, use_occlusion);
            }
        }

        gGL.setColorMask(true, true);
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
    const S32 use_occlusion = result.getUseOcclusion();
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("checkOcclusionAndRebuildMesh");
    for (LLCullResult::sg_iterator iter = getFrameCull()->beginDrawableGroups(); iter != getFrameCull()->endDrawableGroups(); ++iter)
    {
        LLSpatialGroup* group = *iter;
        if (group->isDead())
        {
            continue;
        }
        group->checkOcclusion(use_occlusion);
        if (use_occlusion > 1 && group->isOcclusionState(LLSpatialGroup::OCCLUDED))
        {
            markOccluder(group, use_occlusion);
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
    static U32 s_vkc_empty_frames = 0;
    static U32 s_acc_hasgeom   = 0;
    static U32 s_acc_zerogeomv = 0;
    static std::vector<LLUUID> s_vkc_hole_uuids;
    static std::vector<std::string> s_vkc_zerogeomv_info;
    for (LLCullResult::sg_iterator iter = getFrameCull()->beginVisibleGroups(); iter != getFrameCull()->endVisibleGroups(); ++iter)
    {
        LLSpatialGroup* group = *iter;
        if (group->isDead())
        {
            continue;
        }
        if (group->mDrawMap.empty() && group->getElementCount() > 0 &&
            !group->hasState(LLSpatialGroup::GEOM_DIRTY | LLSpatialGroup::ALPHA_DIRTY | LLSpatialGroup::IN_BUILD_Q1))
        {
            vkcScanEmptyDrawmapDefects(group, s_acc_hasgeom, s_acc_zerogeomv,
                                       s_vkc_hole_uuids, s_vkc_zerogeomv_info);
        }
        group->checkOcclusion(use_occlusion);
        if (use_occlusion > 1 && group->isOcclusionState(LLSpatialGroup::OCCLUDED))
        {
            markOccluder(group, use_occlusion);
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
    if ((++s_vkc_empty_frames % 60) == 0)
    {
        if (s_acc_hasgeom + s_acc_zerogeomv > 0)
        {
            std::ostringstream cls;
            if (!s_vkc_hole_uuids.empty())
            {
                cls << " hole_uuid[";
                for (size_t u = 0; u < s_vkc_hole_uuids.size(); ++u)
                {
                    cls << (u ? " " : "") << s_vkc_hole_uuids[u];
                }
                cls << "]";
            }
            if (!s_vkc_zerogeomv_info.empty())
            {
                cls << " zerogeomv[";
                for (size_t u = 0; u < s_vkc_zerogeomv_info.size(); ++u)
                {
                    cls << (u ? " | " : "") << s_vkc_zerogeomv_info[u];
                }
                cls << "]";
            }
            LL_WARNS("VKGeo") << "empty-drawmap defect (60f): hasgeom=" << s_acc_hasgeom
                              << " zerogeomv=" << s_acc_zerogeomv << cls.str() << LL_ENDL;
        }
        s_acc_hasgeom   = 0;
        s_acc_zerogeomv = 0;
        s_vkc_hole_uuids.clear();
        s_vkc_zerogeomv_info.clear();
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

void LLPipeline::postSort(LLCamera &camera)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    assertInitialized();
    const S32 use_occlusion = getFrameCull()->getUseOcclusion();
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
            if (!use_occlusion || !group->isOcclusionState(LLSpatialGroup::OCCLUDED))
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
    for (LLCullResult::sg_iterator i = getFrameCull()->beginVisibleGroups(); i != getFrameCull()->endVisibleGroups(); ++i)
    {
        LLSpatialGroup *group = *i;

        if (group->isDead())
        {
            continue;
        }

        if ((use_occlusion && group->isOcclusionState(LLSpatialGroup::OCCLUDED)) ||
            (RenderAutoHideSurfaceAreaLimit > 0.f &&
             group->mSurfaceArea > RenderAutoHideSurfaceAreaLimit * llmax(group->mObjectBoxSize, 10.f)))
        {
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

bool LLPipeline::getVisiblePointCloud(LLCamera& camera, LLVector3& min, LLVector3& max, std::vector<LLVector3>& fp, S32 use_occlusion, LLVector3 light_dir)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    //get point cloud of intersection of frust and min, max

    if (getVisibleExtents(camera, min, max, use_occlusion))
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

