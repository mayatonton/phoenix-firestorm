/**
 * @file llpipelinedebug.cpp
 * @brief Rendering pipeline: debug/toggles/beacons/picking (pure move from pipeline.cpp).
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

extern std::set<LLSpatialGroup*> visible_selected_groups;

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
                        memcpy(gPathfindingProgram.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
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
                            memcpy(gPathfindingProgram.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
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
                        memcpy(gPathfindingProgram.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
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
                            memcpy(gPathfindingNoNormalsProgram.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
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
                            memcpy(gPathfindingProgram.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
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
                            memcpy(gPathfindingProgram.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
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
                            memcpy(gPathfindingProgram.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
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
                            memcpy(gPathfindingNoNormalsProgram.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
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
                            memcpy(gPathfindingProgram.vkPerProgramBaseWritePtr(), &ubo_data, sizeof(ubo_data));
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

    {
    LLRTScope rts(getFrameRT()->deferredScreen, false, "profile_avatar");
    if (rts)
    {
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

    }
    }

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

void LLPipeline::addDebugBlip(const LLVector3& position, const LLColor4& color)
{
    DebugBlip blip(position, color);
    mDebugBlips.push_back(blip);
}

