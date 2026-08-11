/**
 * @file llpipelineupdate.cpp
 * @brief Rendering pipeline: move/rebuild/visibility update phase (pure move from pipeline.cpp).
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

LLTrace::BlockTimerStatHandle FTM_GEO_UPDATE("Geo Update");


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

void LLPipeline::updateRenderTransparentWater()
{
    sRenderTransparentWater = gSavedSettings.getBOOL("RenderTransparentWater");
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

void LLPipeline::forAllVisibleDrawables(void (*func)(LLDrawable*))
{
    forAllDrawables(getFrameCull()->beginDrawableGroups(), getFrameCull()->endDrawableGroups(), func);
    forAllDrawables(getFrameCull()->beginVisibleGroups(), getFrameCull()->endVisibleGroups(), func);
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

