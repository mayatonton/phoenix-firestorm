/**
 * @file lldrawpool.cpp
 * @brief LLDrawPool class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "lldrawpool.h"
#include "llrender.h"
#include "llfasttimer.h"
#include "llviewercontrol.h"

#include "lldrawable.h"
#include "lldrawpoolalpha.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolbump.h"
#include "lldrawpoolmaterials.h"
#include "lldrawpoolpbropaque.h"
#include "lldrawpoolsimple.h"
#include "lldrawpoolsky.h"
#include "lldrawpooltree.h"
#include "lldrawpoolterrain.h"
#include "lldrawpoolwater.h"
#include "lldrawpoolwaterexclusion.h"
#include "llface.h"
#include "llagentcamera.h"
#include "llframetimer.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h" // For debug listing.
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llviewercamera.h"
#include "lldrawpoolwlsky.h"
#include "llglslshader.h"
#include "llglcommonfunc.h"
#include "llvoavatar.h"
#include "llviewershadermgr.h"

#include <algorithm>
#include <initializer_list>
#include <map>
#include <sstream>
#include <unordered_map>
#include <vector>

S32 LLDrawPool::sNumDrawPools = 0;

namespace
{
    struct FSR34MouselookWorldSourceStats
    {
        U32 mRootLocalID = 0;
        LLUUID mRootID;
        LLUUID mOwnerID;
        F32 mRootDistance = 0.f;
        F32 mRootForwardDistance = 0.f;
        F32 mRootCameraDot = 0.f;
        F32 mRootCameraAngle = -1.f;
        S32 mRootChildren = 0;
        bool mRootIsMesh = false;

        U32 mSampleLocalID = 0;
        LLUUID mSampleID;
        F32 mSampleDistance = 0.f;
        F32 mSampleForwardDistance = 0.f;
        F32 mSampleCameraDot = 0.f;
        F32 mSampleCameraAngle = -1.f;
        bool mSampleIsMesh = false;
        bool mSampleIsRiggedMesh = false;

        U64 mDrawInfos = 0;
        U64 mIndices = 0;
        U64 mPassDrawInfos[LLRenderPass::NUM_RENDER_TYPES] = {};
        U64 mPassIndices[LLRenderPass::NUM_RENDER_TYPES] = {};
    };

    struct FSR34MouselookVolumeTraceStats
    {
        bool mInitialized = false;
        LLFrameTimer mTimer;
        U64 mDrawInfos = 0;
        U64 mIndices = 0;
        U64 mRiggedDrawInfos = 0;
        U64 mWorldDrawInfos = 0;
        U64 mSelfRiggedDrawInfos = 0;
        U64 mSelfAttachmentDrawInfos = 0;
        U64 mOtherAvatarDrawInfos = 0;
        U64 mSuppressedDrawInfos = 0;
        U64 mSuppressedIndices = 0;
        U64 mAutoHeavySuppressedDrawInfos = 0;
        U64 mAutoHeavySuppressedIndices = 0;
        U64 mPassDrawInfos[LLRenderPass::NUM_RENDER_TYPES] = {};
        U64 mPassIndices[LLRenderPass::NUM_RENDER_TYPES] = {};
        std::map<U32, FSR34MouselookWorldSourceStats> mWorldSources;
        std::map<U32, FSR34MouselookWorldSourceStats> mWorldChildren;

        void reset()
        {
            mInitialized = true;
            mTimer.reset();
            mDrawInfos = 0;
            mIndices = 0;
            mRiggedDrawInfos = 0;
            mWorldDrawInfos = 0;
            mSelfRiggedDrawInfos = 0;
            mSelfAttachmentDrawInfos = 0;
            mOtherAvatarDrawInfos = 0;
            mSuppressedDrawInfos = 0;
            mSuppressedIndices = 0;
            mAutoHeavySuppressedDrawInfos = 0;
            mAutoHeavySuppressedIndices = 0;
            for (U32 i = 0; i < LLRenderPass::NUM_RENDER_TYPES; ++i)
            {
                mPassDrawInfos[i] = 0;
                mPassIndices[i] = 0;
            }
            mWorldSources.clear();
            mWorldChildren.clear();
        }
    };

    struct FSR34MouselookSourceCacheEntry
    {
        LLViewerObject* mObject = nullptr;
        LLViewerObject* mRoot = nullptr;
        U32 mRootLocalID = 0;
        F32 mRootDistance = 0.f;
        F32 mObjectDistance = 0.f;
        F32 mObjectCameraDot = 1.f;
        F32 mObjectProjectedPct = 0.f;
        S32 mRootChildren = 0;
        bool mObjectIsMesh = false;
        bool mSelected = false;
    };

    struct FSR34MouselookFrameCache
    {
        S32 mFrame = -1;
        std::unordered_map<U32, LLViewerObject*> mObjects;
        std::unordered_map<U32, FSR34MouselookSourceCacheEntry> mSources;
        std::unordered_map<U32, U64> mRootIndices;
        std::unordered_map<U32, U64> mChildIndices;

        void update()
        {
            const S32 frame = LLFrameTimer::getFrameCount();
            if (mFrame == frame)
            {
                return;
            }

            mFrame = frame;
            mObjects.clear();
            mSources.clear();
            mRootIndices.clear();
            mChildIndices.clear();

            const S32 count = gObjectList.getNumObjects();
            mObjects.reserve(count);
            for (S32 i = 0; i < count; ++i)
            {
                LLViewerObject* objectp = gObjectList.getObject(i);
                if (objectp)
                {
                    mObjects[objectp->getLocalID()] = objectp;
                }
            }
        }
    };

    FSR34MouselookVolumeTraceStats sFSR34MouselookVolumeTraceStats;
    FSR34MouselookFrameCache sFSR34MouselookFrameCache;

    void fsr34_measure_camera_relation(
        const LLVector3& object_pos,
        const LLVector3& camera_pos,
        const LLVector3& camera_at,
        F32& distance,
        F32& forward_distance,
        F32& camera_dot,
        F32& camera_angle);

    bool fsr34_mouselook_volume_trace_active()
    {
        static LLCachedControl<bool> trace_enabled(gSavedSettings, "AYAR34MouselookVolumeTraceEnabled", false);
        return trace_enabled && gAgentCamera.cameraMouselook();
    }

    bool fsr34_mouselook_top_source_trace_enabled()
    {
        static LLCachedControl<bool> trace_enabled(gSavedSettings, "AYAR34MouselookTopSourceTraceEnabled", false);
        return trace_enabled;
    }

    F32 fsr34_projected_height_pct(F32 distance, F32 radius)
    {
        LLViewerCamera* camera = LLViewerCamera::getInstance();
        const F32 view_height = static_cast<F32>(llmax(1, camera->getViewHeightInPixels()));
        const F32 safe_distance = llmax(distance, 0.001f);
        const F32 projected_height = (camera->getPixelMeterRatio() / safe_distance) * radius * 2.f;
        return llclamp((projected_height / view_height) * 100.f, 0.f, 1000.f);
    }

    bool fsr34_world_volume_draw_info(const LLDrawInfo& params)
    {
        return params.mAvatar.isNull() && params.mAttachedToAvatar.isNull();
    }

    bool fsr34_mouselook_alpha_mask_pass(U32 type)
    {
        return type == LLRenderPass::PASS_ALPHA_MASK ||
               type == LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK ||
               type == LLRenderPass::PASS_MATERIAL_ALPHA_MASK ||
               type == LLRenderPass::PASS_SPECMAP_MASK ||
               type == LLRenderPass::PASS_NORMMAP_MASK ||
               type == LLRenderPass::PASS_NORMSPEC_MASK ||
               type == LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK;
    }

    bool fsr34_mouselook_outer_cone_pass(U32 type)
    {
        return fsr34_mouselook_alpha_mask_pass(type) ||
               type == LLRenderPass::PASS_SIMPLE ||
               type == LLRenderPass::PASS_FULLBRIGHT ||
               type == LLRenderPass::PASS_GLOW ||
               type == LLRenderPass::PASS_GLTF_GLOW;
    }

    bool fsr34_mouselook_suppress_pass_matches(S32 mode, U32 type)
    {
        if (mode == 1)
        {
            return fsr34_mouselook_alpha_mask_pass(type);
        }
        if (mode == 2)
        {
            return fsr34_mouselook_outer_cone_pass(type);
        }

        return false;
    }

    U64 fsr34_pass_draw_infos(U32 type)
    {
        return type < LLRenderPass::NUM_RENDER_TYPES ? sFSR34MouselookVolumeTraceStats.mPassDrawInfos[type] : 0;
    }

    U64 fsr34_pass_triangles(U32 type)
    {
        return type < LLRenderPass::NUM_RENDER_TYPES ? sFSR34MouselookVolumeTraceStats.mPassIndices[type] / 3 : 0;
    }

    U64 fsr34_source_pass_draw_infos(const FSR34MouselookWorldSourceStats& stats, U32 type)
    {
        return type < LLRenderPass::NUM_RENDER_TYPES ? stats.mPassDrawInfos[type] : 0;
    }

    U64 fsr34_source_pass_triangles(const FSR34MouselookWorldSourceStats& stats, U32 type)
    {
        return type < LLRenderPass::NUM_RENDER_TYPES ? stats.mPassIndices[type] / 3 : 0;
    }

    U64 fsr34_pass_group_draw_infos(std::initializer_list<U32> types)
    {
        U64 count = 0;
        for (U32 type : types)
        {
            count += fsr34_pass_draw_infos(type);
        }
        return count;
    }

    U64 fsr34_pass_group_triangles(std::initializer_list<U32> types)
    {
        U64 triangles = 0;
        for (U32 type : types)
        {
            triangles += fsr34_pass_triangles(type);
        }
        return triangles;
    }

    void fsr34_append_pass_group(std::ostringstream& out, const char* name, std::initializer_list<U32> types)
    {
        const U64 draw_infos = fsr34_pass_group_draw_infos(types);
        if (draw_infos == 0)
        {
            return;
        }

        out << " " << name << "=" << draw_infos << "/" << fsr34_pass_group_triangles(types);
    }

    void fsr34_append_source_pass_group(
        std::ostringstream& out,
        const FSR34MouselookWorldSourceStats& stats,
        const char* name,
        std::initializer_list<U32> types)
    {
        U64 draw_infos = 0;
        U64 triangles = 0;
        for (U32 type : types)
        {
            draw_infos += fsr34_source_pass_draw_infos(stats, type);
            triangles += fsr34_source_pass_triangles(stats, type);
        }

        if (draw_infos == 0)
        {
            return;
        }

        out << " " << name << "=" << draw_infos << "/" << triangles;
    }

    LLViewerObject* fsr34_find_object_by_local_id(U32 local_id)
    {
        if (local_id == 0)
        {
            return nullptr;
        }

        sFSR34MouselookFrameCache.update();
        const auto it = sFSR34MouselookFrameCache.mObjects.find(local_id);
        return it != sFSR34MouselookFrameCache.mObjects.end() ? it->second : nullptr;
    }

    const FSR34MouselookSourceCacheEntry& fsr34_get_source_cache_entry(U32 local_id)
    {
        sFSR34MouselookFrameCache.update();

        auto found = sFSR34MouselookFrameCache.mSources.find(local_id);
        if (found != sFSR34MouselookFrameCache.mSources.end())
        {
            return found->second;
        }

        FSR34MouselookSourceCacheEntry entry;
        entry.mObject = fsr34_find_object_by_local_id(local_id);
        entry.mRoot = entry.mObject ? entry.mObject->getRootEdit() : nullptr;
        entry.mRootLocalID = entry.mRoot ? entry.mRoot->getLocalID() : local_id;
        entry.mRootChildren = entry.mRoot ? entry.mRoot->numChildren() : 0;
        entry.mObjectIsMesh = entry.mObject && entry.mObject->isMesh();
        entry.mSelected =
            (entry.mObject && entry.mObject->isSelected()) ||
            (entry.mRoot && entry.mRoot->isSelected());
        const LLVector3& camera_pos = gAgentCamera.getCameraPositionAgent();
        const LLVector3& camera_at = LLViewerCamera::getInstance()->getAtAxis();
        if (entry.mRoot)
        {
            F32 forward_distance = 0.f;
            F32 camera_angle = 0.f;
            fsr34_measure_camera_relation(
                entry.mRoot->getPositionAgent(),
                camera_pos,
                camera_at,
                entry.mRootDistance,
                forward_distance,
                entry.mObjectCameraDot,
                camera_angle);
        }

        if (entry.mObject)
        {
            F32 forward_distance = 0.f;
            F32 camera_angle = 0.f;
            fsr34_measure_camera_relation(
                entry.mObject->getPositionAgent(),
                camera_pos,
                camera_at,
                entry.mObjectDistance,
                forward_distance,
                entry.mObjectCameraDot,
                camera_angle);
            entry.mObjectProjectedPct = fsr34_projected_height_pct(
                entry.mObjectDistance,
                llmax(0.001f, entry.mObject->getMaxScale() * 0.5f));
        }

        const auto inserted = sFSR34MouselookFrameCache.mSources.emplace(local_id, entry);
        return inserted.first->second;
    }

    void fsr34_measure_camera_relation(
        const LLVector3& object_pos,
        const LLVector3& camera_pos,
        const LLVector3& camera_at,
        F32& distance,
        F32& forward_distance,
        F32& camera_dot,
        F32& camera_angle)
    {
        const LLVector3 camera_to_object = object_pos - camera_pos;
        distance = camera_to_object.magVec();
        forward_distance = camera_to_object * camera_at;

        if (distance > 0.001f)
        {
            camera_dot = llclamp(forward_distance / distance, -1.f, 1.f);
            camera_angle = acosf(camera_dot) * RAD_TO_DEG;
        }
        else
        {
            camera_dot = 1.f;
            camera_angle = 0.f;
        }
    }

    void fsr34_initialize_world_source_stats(
        FSR34MouselookWorldSourceStats& stats,
        LLViewerObject* rootp,
        LLViewerObject* objectp,
        U32 root_local_id)
    {
        const LLVector3& camera_pos = gAgentCamera.getCameraPositionAgent();
        const LLVector3& camera_at = LLViewerCamera::getInstance()->getAtAxis();
        stats.mRootLocalID = root_local_id;

        if (rootp)
        {
            stats.mRootID = rootp->getID();
            stats.mOwnerID = rootp->mOwnerID;
            fsr34_measure_camera_relation(
                rootp->getPositionAgent(),
                camera_pos,
                camera_at,
                stats.mRootDistance,
                stats.mRootForwardDistance,
                stats.mRootCameraDot,
                stats.mRootCameraAngle);
            stats.mRootChildren = rootp->numChildren();
            stats.mRootIsMesh = rootp->isMesh();
        }

        if (objectp)
        {
            stats.mSampleLocalID = objectp->getLocalID();
            stats.mSampleID = objectp->getID();
            fsr34_measure_camera_relation(
                objectp->getPositionAgent(),
                camera_pos,
                camera_at,
                stats.mSampleDistance,
                stats.mSampleForwardDistance,
                stats.mSampleCameraDot,
                stats.mSampleCameraAngle);
            stats.mSampleIsMesh = objectp->isMesh();
            stats.mSampleIsRiggedMesh = objectp->isRiggedMesh();
        }
    }

    void fsr34_accumulate_world_source_stats(FSR34MouselookWorldSourceStats& stats, U32 type, const LLDrawInfo& params)
    {
        ++stats.mDrawInfos;
        stats.mIndices += params.mCount;
        if (type < LLRenderPass::NUM_RENDER_TYPES)
        {
            ++stats.mPassDrawInfos[type];
            stats.mPassIndices[type] += params.mCount;
        }
    }

    void fsr34_record_world_source(U32 type, const LLDrawInfo& params)
    {
        const FSR34MouselookSourceCacheEntry& source = fsr34_get_source_cache_entry(params.mFSPickerLocalID);
        LLViewerObject* objectp = source.mObject;
        LLViewerObject* rootp = source.mRoot;
        const U32 root_local_id = source.mRootLocalID;

        FSR34MouselookWorldSourceStats& root_stats =
            sFSR34MouselookVolumeTraceStats.mWorldSources[root_local_id];
        if (root_stats.mDrawInfos == 0)
        {
            fsr34_initialize_world_source_stats(root_stats, rootp, objectp, root_local_id);
        }
        fsr34_accumulate_world_source_stats(root_stats, type, params);

        FSR34MouselookWorldSourceStats& child_stats =
            sFSR34MouselookVolumeTraceStats.mWorldChildren[params.mFSPickerLocalID];
        if (child_stats.mDrawInfos == 0)
        {
            fsr34_initialize_world_source_stats(child_stats, rootp, objectp, root_local_id);
        }
        fsr34_accumulate_world_source_stats(child_stats, type, params);
    }

    void fsr34_log_world_top_sources(
        const std::map<U32, FSR34MouselookWorldSourceStats>& source_map,
        const char* label,
        size_t limit)
    {
        if (source_map.empty())
        {
            return;
        }

        std::vector<const FSR34MouselookWorldSourceStats*> sources;
        sources.reserve(source_map.size());
        for (const auto& entry : source_map)
        {
            sources.push_back(&entry.second);
        }

        std::sort(
            sources.begin(),
            sources.end(),
            [](const FSR34MouselookWorldSourceStats* lhs, const FSR34MouselookWorldSourceStats* rhs)
            {
                if (lhs->mIndices != rhs->mIndices)
                {
                    return lhs->mIndices > rhs->mIndices;
                }
                return lhs->mDrawInfos > rhs->mDrawInfos;
            });

        std::ostringstream out;
        out << label;
        const size_t count = std::min<size_t>(sources.size(), limit);
        for (size_t i = 0; i < count; ++i)
        {
            const FSR34MouselookWorldSourceStats& source = *sources[i];

            std::ostringstream passes;
            fsr34_append_source_pass_group(passes, source, "simple", { LLRenderPass::PASS_SIMPLE, LLRenderPass::PASS_SIMPLE_RIGGED });
            fsr34_append_source_pass_group(passes, source, "fullbright", { LLRenderPass::PASS_FULLBRIGHT, LLRenderPass::PASS_FULLBRIGHT_RIGGED });
            fsr34_append_source_pass_group(passes, source, "alpha_mask", { LLRenderPass::PASS_ALPHA_MASK, LLRenderPass::PASS_ALPHA_MASK_RIGGED,
                                                                           LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK_RIGGED });
            fsr34_append_source_pass_group(passes, source, "material", { LLRenderPass::PASS_MATERIAL, LLRenderPass::PASS_MATERIAL_RIGGED,
                                                                         LLRenderPass::PASS_MATERIAL_ALPHA_MASK, LLRenderPass::PASS_MATERIAL_ALPHA_MASK_RIGGED,
                                                                         LLRenderPass::PASS_SPECMAP, LLRenderPass::PASS_SPECMAP_RIGGED,
                                                                         LLRenderPass::PASS_SPECMAP_MASK, LLRenderPass::PASS_SPECMAP_MASK_RIGGED,
                                                                         LLRenderPass::PASS_NORMMAP, LLRenderPass::PASS_NORMMAP_RIGGED,
                                                                         LLRenderPass::PASS_NORMMAP_MASK, LLRenderPass::PASS_NORMMAP_MASK_RIGGED,
                                                                         LLRenderPass::PASS_NORMSPEC, LLRenderPass::PASS_NORMSPEC_RIGGED,
                                                                         LLRenderPass::PASS_NORMSPEC_MASK, LLRenderPass::PASS_NORMSPEC_MASK_RIGGED });
            fsr34_append_source_pass_group(passes, source, "pbr", { LLRenderPass::PASS_GLTF_PBR, LLRenderPass::PASS_GLTF_PBR_RIGGED,
                                                                    LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK, LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK_RIGGED });
            fsr34_append_source_pass_group(passes, source, "glow", { LLRenderPass::PASS_GLOW, LLRenderPass::PASS_GLOW_RIGGED,
                                                                     LLRenderPass::PASS_GLTF_GLOW, LLRenderPass::PASS_GLTF_GLOW_RIGGED });
            fsr34_append_source_pass_group(passes, source, "alpha", { LLRenderPass::PASS_ALPHA, LLRenderPass::PASS_ALPHA_RIGGED,
                                                                      LLRenderPass::PASS_MATERIAL_ALPHA, LLRenderPass::PASS_MATERIAL_ALPHA_RIGGED,
                                                                      LLRenderPass::PASS_SPECMAP_BLEND, LLRenderPass::PASS_SPECMAP_BLEND_RIGGED,
                                                                      LLRenderPass::PASS_NORMMAP_BLEND, LLRenderPass::PASS_NORMMAP_BLEND_RIGGED,
                                                                      LLRenderPass::PASS_NORMSPEC_BLEND, LLRenderPass::PASS_NORMSPEC_BLEND_RIGGED });

            out << " #" << i + 1
                << " root_local=" << source.mRootLocalID
                << " root_id=" << source.mRootID
                << " owner=" << source.mOwnerID
                << " root_dist_m=" << static_cast<S32>(source.mRootDistance + 0.5f)
                << " root_forward_m=" << static_cast<S32>(source.mRootForwardDistance + (source.mRootForwardDistance >= 0.f ? 0.5f : -0.5f))
                << " root_dot=" << source.mRootCameraDot
                << " root_angle_deg=" << static_cast<S32>(source.mRootCameraAngle + 0.5f)
                << " root_children=" << source.mRootChildren
                << " root_mesh=" << source.mRootIsMesh
                << " sample_local=" << source.mSampleLocalID
                << " sample_id=" << source.mSampleID
                << " sample_dist_m=" << static_cast<S32>(source.mSampleDistance + 0.5f)
                << " sample_forward_m=" << static_cast<S32>(source.mSampleForwardDistance + (source.mSampleForwardDistance >= 0.f ? 0.5f : -0.5f))
                << " sample_dot=" << source.mSampleCameraDot
                << " sample_angle_deg=" << static_cast<S32>(source.mSampleCameraAngle + 0.5f)
                << " sample_mesh=" << source.mSampleIsMesh
                << " sample_rigged=" << source.mSampleIsRiggedMesh
                << " draw_infos=" << source.mDrawInfos
                << " triangles=" << source.mIndices / 3
                << " passes:" << passes.str();
        }

        LL_INFOS("AYAR34MouselookFPS") << out.str() << LL_ENDL;
    }

    void fsr34_log_mouselook_volume_trace(F32 seconds)
    {
        std::ostringstream passes;
        fsr34_append_pass_group(passes, "simple", { LLRenderPass::PASS_SIMPLE, LLRenderPass::PASS_SIMPLE_RIGGED });
        fsr34_append_pass_group(passes, "fullbright", { LLRenderPass::PASS_FULLBRIGHT, LLRenderPass::PASS_FULLBRIGHT_RIGGED });
        fsr34_append_pass_group(passes, "shiny", { LLRenderPass::PASS_SHINY, LLRenderPass::PASS_SHINY_RIGGED });
        fsr34_append_pass_group(passes, "bump", { LLRenderPass::PASS_BUMP, LLRenderPass::PASS_BUMP_RIGGED });
        fsr34_append_pass_group(passes, "fb_shiny", { LLRenderPass::PASS_FULLBRIGHT_SHINY, LLRenderPass::PASS_FULLBRIGHT_SHINY_RIGGED });
        fsr34_append_pass_group(passes, "alpha_mask", { LLRenderPass::PASS_ALPHA_MASK, LLRenderPass::PASS_ALPHA_MASK_RIGGED,
                                                        LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK, LLRenderPass::PASS_FULLBRIGHT_ALPHA_MASK_RIGGED });
        fsr34_append_pass_group(passes, "material", { LLRenderPass::PASS_MATERIAL, LLRenderPass::PASS_MATERIAL_RIGGED,
                                                      LLRenderPass::PASS_MATERIAL_ALPHA_MASK, LLRenderPass::PASS_MATERIAL_ALPHA_MASK_RIGGED,
                                                      LLRenderPass::PASS_SPECMAP, LLRenderPass::PASS_SPECMAP_RIGGED,
                                                      LLRenderPass::PASS_SPECMAP_MASK, LLRenderPass::PASS_SPECMAP_MASK_RIGGED,
                                                      LLRenderPass::PASS_NORMMAP, LLRenderPass::PASS_NORMMAP_RIGGED,
                                                      LLRenderPass::PASS_NORMMAP_MASK, LLRenderPass::PASS_NORMMAP_MASK_RIGGED,
                                                      LLRenderPass::PASS_NORMSPEC, LLRenderPass::PASS_NORMSPEC_RIGGED,
                                                      LLRenderPass::PASS_NORMSPEC_MASK, LLRenderPass::PASS_NORMSPEC_MASK_RIGGED });
        fsr34_append_pass_group(passes, "pbr", { LLRenderPass::PASS_GLTF_PBR, LLRenderPass::PASS_GLTF_PBR_RIGGED,
                                                 LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK, LLRenderPass::PASS_GLTF_PBR_ALPHA_MASK_RIGGED });
        fsr34_append_pass_group(passes, "glow", { LLRenderPass::PASS_GLOW, LLRenderPass::PASS_GLOW_RIGGED,
                                                  LLRenderPass::PASS_GLTF_GLOW, LLRenderPass::PASS_GLTF_GLOW_RIGGED });
        fsr34_append_pass_group(passes, "alpha", { LLRenderPass::PASS_ALPHA, LLRenderPass::PASS_ALPHA_RIGGED,
                                                   LLRenderPass::PASS_MATERIAL_ALPHA, LLRenderPass::PASS_MATERIAL_ALPHA_RIGGED,
                                                   LLRenderPass::PASS_SPECMAP_BLEND, LLRenderPass::PASS_SPECMAP_BLEND_RIGGED,
                                                   LLRenderPass::PASS_NORMMAP_BLEND, LLRenderPass::PASS_NORMMAP_BLEND_RIGGED,
                                                   LLRenderPass::PASS_NORMSPEC_BLEND, LLRenderPass::PASS_NORMSPEC_BLEND_RIGGED });

        LL_INFOS("AYAR34MouselookFPS")
            << "dt=" << seconds
            << " draw_infos=" << sFSR34MouselookVolumeTraceStats.mDrawInfos
            << " triangles=" << sFSR34MouselookVolumeTraceStats.mIndices / 3
            << " rigged_draw_infos=" << sFSR34MouselookVolumeTraceStats.mRiggedDrawInfos
            << " world_draw_infos=" << sFSR34MouselookVolumeTraceStats.mWorldDrawInfos
            << " self_rigged_draw_infos=" << sFSR34MouselookVolumeTraceStats.mSelfRiggedDrawInfos
            << " self_attachment_draw_infos=" << sFSR34MouselookVolumeTraceStats.mSelfAttachmentDrawInfos
            << " other_avatar_draw_infos=" << sFSR34MouselookVolumeTraceStats.mOtherAvatarDrawInfos
            << " suppressed_draw_infos=" << sFSR34MouselookVolumeTraceStats.mSuppressedDrawInfos
            << " suppressed_triangles=" << sFSR34MouselookVolumeTraceStats.mSuppressedIndices / 3
            << " auto_heavy_suppressed_draw_infos=" << sFSR34MouselookVolumeTraceStats.mAutoHeavySuppressedDrawInfos
            << " auto_heavy_suppressed_triangles=" << sFSR34MouselookVolumeTraceStats.mAutoHeavySuppressedIndices / 3
            << " pass_draw_infos/triangles:" << passes.str()
            << LL_ENDL;

        if (fsr34_mouselook_top_source_trace_enabled())
        {
            fsr34_log_world_top_sources(sFSR34MouselookVolumeTraceStats.mWorldSources, "world_top_sources:", 5);
            fsr34_log_world_top_sources(sFSR34MouselookVolumeTraceStats.mWorldChildren, "world_top_children:", 8);
        }
    }

    void fsr34_record_mouselook_volume_draw_info(U32 type, const LLDrawInfo& params, bool rigged)
    {
        if (!fsr34_mouselook_volume_trace_active())
        {
            if (sFSR34MouselookVolumeTraceStats.mInitialized)
            {
                sFSR34MouselookVolumeTraceStats.mInitialized = false;
            }
            return;
        }

        if (!sFSR34MouselookVolumeTraceStats.mInitialized)
        {
            sFSR34MouselookVolumeTraceStats.reset();
        }

        ++sFSR34MouselookVolumeTraceStats.mDrawInfos;
        sFSR34MouselookVolumeTraceStats.mIndices += params.mCount;
        if (rigged)
        {
            ++sFSR34MouselookVolumeTraceStats.mRiggedDrawInfos;
        }

        if (type < LLRenderPass::NUM_RENDER_TYPES)
        {
            ++sFSR34MouselookVolumeTraceStats.mPassDrawInfos[type];
            sFSR34MouselookVolumeTraceStats.mPassIndices[type] += params.mCount;
        }

        const LLVOAvatar* avatarp = params.mAvatar.get();
        const LLVOAvatar* attached_avatarp = params.mAttachedToAvatar.get();
        if (avatarp && avatarp->isSelf())
        {
            ++sFSR34MouselookVolumeTraceStats.mSelfRiggedDrawInfos;
        }
        else if (attached_avatarp && attached_avatarp->isSelf())
        {
            ++sFSR34MouselookVolumeTraceStats.mSelfAttachmentDrawInfos;
        }
        else if (avatarp || attached_avatarp)
        {
            ++sFSR34MouselookVolumeTraceStats.mOtherAvatarDrawInfos;
        }
        else
        {
            ++sFSR34MouselookVolumeTraceStats.mWorldDrawInfos;
            if (fsr34_mouselook_top_source_trace_enabled())
            {
                fsr34_record_world_source(type, params);
            }
        }

        const F32 elapsed = sFSR34MouselookVolumeTraceStats.mTimer.getElapsedTimeF32();
        if (elapsed >= 1.f)
        {
            fsr34_log_mouselook_volume_trace(elapsed);
            sFSR34MouselookVolumeTraceStats.reset();
        }
    }

    bool fsr34_suppress_mouselook_volume_draw_info(U32 type, const LLDrawInfo& params)
    {
        static LLCachedControl<U32> target_local_id(gSavedSettings, "AYAR34MouselookSuppressChildLocalID", 0);
        static LLCachedControl<U32> target_root_local_id(gSavedSettings, "AYAR34MouselookSuppressRootLocalID", 0);
        static LLCachedControl<S32> target_root_pass_mode(gSavedSettings, "AYAR34MouselookSuppressRootPassMode", 0);
        static LLCachedControl<F32> target_root_outer_dot(gSavedSettings, "AYAR34MouselookSuppressRootOuterDot", 0.8f);
        static LLCachedControl<bool> auto_root_enabled(gSavedSettings, "AYAR34MouselookSuppressAutoRootEnabled", false);
        static LLCachedControl<F32> auto_root_max_distance(gSavedSettings, "AYAR34MouselookSuppressAutoRootMaxDistance", 12.f);
        static LLCachedControl<S32> auto_root_min_children(gSavedSettings, "AYAR34MouselookSuppressAutoRootMinChildren", 50);
        static LLCachedControl<bool> auto_heavy_enabled(gSavedSettings, "AYAR34MouselookSuppressAutoHeavyEnabled", false);
        static LLCachedControl<F32> auto_heavy_max_distance(gSavedSettings, "AYAR34MouselookSuppressAutoHeavyMaxDistance", 96.f);
        static LLCachedControl<S32> auto_heavy_min_source_tris(gSavedSettings, "AYAR34MouselookSuppressAutoHeavyMinSourceTriangles", 250000);
        static LLCachedControl<F32> auto_heavy_outer_dot(gSavedSettings, "AYAR34MouselookSuppressAutoHeavyOuterDot", 0.75f);
        static LLCachedControl<F32> auto_heavy_small_screen_pct(gSavedSettings, "AYAR34MouselookSuppressAutoHeavySmallScreenPct", 1.5f);

        if ((!target_local_id && !target_root_local_id && !auto_root_enabled && !auto_heavy_enabled) || !gAgentCamera.cameraMouselook())
        {
            return false;
        }

        if (!fsr34_world_volume_draw_info(params))
        {
            return false;
        }

        bool suppress = target_local_id && params.mFSPickerLocalID == target_local_id;
        bool auto_heavy_suppress = false;

        if (!suppress &&
            (((target_root_local_id || auto_root_enabled) && target_root_pass_mode) || auto_heavy_enabled))
        {
            const FSR34MouselookSourceCacheEntry& source = fsr34_get_source_cache_entry(params.mFSPickerLocalID);
            if (source.mSelected)
            {
                return false;
            }

            LLViewerObject* rootp = source.mRoot;
            const U32 root_local_id = source.mRootLocalID;
            const S32 root_pass_mode = target_root_pass_mode;
            const bool explicit_root_match = target_root_local_id && root_local_id == target_root_local_id;
            const bool auto_root_match =
                auto_root_enabled &&
                rootp &&
                source.mRootDistance <= llmax(0.f, static_cast<F32>(auto_root_max_distance)) &&
                source.mRootChildren >= llmax(0, static_cast<S32>(auto_root_min_children));

            suppress = (explicit_root_match || auto_root_match) &&
                       fsr34_mouselook_suppress_pass_matches(root_pass_mode, type) &&
                       (root_pass_mode != 2 ||
                        source.mObjectCameraDot <= llclamp(static_cast<F32>(target_root_outer_dot), -1.f, 1.f));

            if (!suppress && auto_heavy_enabled)
            {
                sFSR34MouselookFrameCache.update();
                const U64 child_indices =
                    (sFSR34MouselookFrameCache.mChildIndices[params.mFSPickerLocalID] += params.mCount);
                const U64 root_indices =
                    (sFSR34MouselookFrameCache.mRootIndices[root_local_id] += params.mCount);
                const U64 source_tris = llmax(child_indices, root_indices) / 3;
                const F32 max_distance = llmax(0.f, static_cast<F32>(auto_heavy_max_distance));
                const U64 min_source_tris = static_cast<U64>(llmax(0, static_cast<S32>(auto_heavy_min_source_tris)));
                const F32 outer_dot = llclamp(static_cast<F32>(auto_heavy_outer_dot), -1.f, 1.f);
                const F32 small_screen_pct = llmax(0.f, static_cast<F32>(auto_heavy_small_screen_pct));
                const bool off_axis_or_small =
                    source.mObjectCameraDot <= outer_dot ||
                    source.mObjectProjectedPct <= small_screen_pct;

                auto_heavy_suppress =
                    source.mObject &&
                    source.mObjectIsMesh &&
                    source.mObjectDistance <= max_distance &&
                    source_tris >= min_source_tris &&
                    off_axis_or_small &&
                    fsr34_mouselook_outer_cone_pass(type);
                suppress = auto_heavy_suppress;
            }
        }

        if (!suppress)
        {
            return false;
        }

        if (fsr34_mouselook_volume_trace_active())
        {
            if (!sFSR34MouselookVolumeTraceStats.mInitialized)
            {
                sFSR34MouselookVolumeTraceStats.reset();
            }
            ++sFSR34MouselookVolumeTraceStats.mSuppressedDrawInfos;
            sFSR34MouselookVolumeTraceStats.mSuppressedIndices += params.mCount;
            if (auto_heavy_suppress)
            {
                ++sFSR34MouselookVolumeTraceStats.mAutoHeavySuppressedDrawInfos;
                sFSR34MouselookVolumeTraceStats.mAutoHeavySuppressedIndices += params.mCount;
            }
        }

        return true;
    }
}

//=============================
// Draw Pool Implementation
//=============================
LLDrawPool *LLDrawPool::createPool(const U32 type, LLViewerTexture *tex0)
{
    LLDrawPool *poolp = NULL;
    switch (type)
    {
    case POOL_SIMPLE:
        poolp = new LLDrawPoolSimple();
        break;
    case POOL_GRASS:
        poolp = new LLDrawPoolGrass();
        break;
    case POOL_ALPHA_MASK:
        poolp = new LLDrawPoolAlphaMask();
        break;
    case POOL_FULLBRIGHT_ALPHA_MASK:
        poolp = new LLDrawPoolFullbrightAlphaMask();
        break;
    case POOL_FULLBRIGHT:
        poolp = new LLDrawPoolFullbright();
        break;
    case POOL_GLOW:
        poolp = new LLDrawPoolGlow();
        break;
    case POOL_ALPHA_PRE_WATER:
        poolp = new LLDrawPoolAlpha(LLDrawPool::POOL_ALPHA_PRE_WATER);
        break;
    case POOL_ALPHA_POST_WATER:
        poolp = new LLDrawPoolAlpha(LLDrawPool::POOL_ALPHA_POST_WATER);
        break;
    case POOL_AVATAR:
    case POOL_CONTROL_AV:
        poolp = new LLDrawPoolAvatar(type);
        break;
    case POOL_TREE:
        poolp = new LLDrawPoolTree(tex0);
        break;
    case POOL_TERRAIN:
        poolp = new LLDrawPoolTerrain(tex0);
        break;
    case POOL_SKY:
        poolp = new LLDrawPoolSky();
        break;
    case POOL_VOIDWATER:
    case POOL_WATER:
        poolp = new LLDrawPoolWater();
        break;
    case POOL_BUMP:
        poolp = new LLDrawPoolBump();
        break;
    case POOL_MATERIALS:
        poolp = new LLDrawPoolMaterials();
        break;
    case POOL_WL_SKY:
        poolp = new LLDrawPoolWLSky();
        break;
    case POOL_GLTF_PBR:
        poolp = new LLDrawPoolGLTFPBR();
        break;
    case POOL_GLTF_PBR_ALPHA_MASK:
        poolp = new LLDrawPoolGLTFPBR(LLDrawPool::POOL_GLTF_PBR_ALPHA_MASK);
        break;
    case POOL_WATEREXCLUSION:
        poolp = new LLDrawPoolWaterExclusion();
        break;
    default:
        LL_ERRS() << "Unknown draw pool type!" << LL_ENDL;
        return NULL;
    }

    llassert(poolp->mType == type);
    return poolp;
}

LLDrawPool::LLDrawPool(const U32 type)
{
    mType = type;
    sNumDrawPools++;
    mId = sNumDrawPools;
    mShaderLevel = 0;
    mSkipRender = false;
}

LLDrawPool::~LLDrawPool()
{

}

LLViewerTexture *LLDrawPool::getDebugTexture()
{
    return NULL;
}

//virtual
void LLDrawPool::beginRenderPass( S32 pass )
{
}

//virtual
S32  LLDrawPool::getNumPasses()
{
    return 1;
}

//virtual
void LLDrawPool::beginDeferredPass(S32 pass)
{

}

//virtual
void LLDrawPool::endDeferredPass(S32 pass)
{

}

//virtual
S32 LLDrawPool::getNumDeferredPasses()
{
    return 0;
}

//virtual
void LLDrawPool::renderDeferred(S32 pass)
{

}

//virtual
void LLDrawPool::beginPostDeferredPass(S32 pass)
{

}

//virtual
void LLDrawPool::endPostDeferredPass(S32 pass)
{

}

//virtual
S32 LLDrawPool::getNumPostDeferredPasses()
{
    return 0;
}

//virtual
void LLDrawPool::renderPostDeferred(S32 pass)
{

}

//virtual
void LLDrawPool::endRenderPass( S32 pass )
{
    //make sure channel 0 is active channel
    gGL.getTexUnit(0)->activate();
}

//virtual
void LLDrawPool::beginShadowPass(S32 pass)
{

}

//virtual
void LLDrawPool::endShadowPass(S32 pass)
{

}

//virtual
S32 LLDrawPool::getNumShadowPasses()
{
    return 0;
}

//virtual
void LLDrawPool::renderShadow(S32 pass)
{

}

// <AYAstorm r30 P2> Velocity-buffer pass defaults (BD lineage).
//virtual
void LLDrawPool::beginMotionBlurPass(S32 pass)
{

}

//virtual
void LLDrawPool::endMotionBlurPass(S32 pass)
{

}

//virtual
S32 LLDrawPool::getNumMotionBlurPasses()
{
    return 0;
}

//virtual
void LLDrawPool::renderMotionBlur(S32 pass)
{

}
// </AYAstorm r30 P2>

//=============================
// Face Pool Implementation
//=============================
LLFacePool::LLFacePool(const U32 type)
: LLDrawPool(type)
{
    resetDrawOrders();
}

LLFacePool::~LLFacePool()
{
    destroy();
}

void LLFacePool::destroy()
{
    if (!mReferences.empty())
    {
        LL_INFOS() << mReferences.size() << " references left on deletion of draw pool!" << LL_ENDL;
    }
}

void LLFacePool::dirtyTextures(const std::set<LLViewerFetchedTexture*>& textures)
{
}

void LLFacePool::enqueue(LLFace* facep)
{
    mDrawFace.push_back(facep);
}

// virtual
bool LLFacePool::addFace(LLFace *facep)
{
    addFaceReference(facep);
    return true;
}

// virtual
bool LLFacePool::removeFace(LLFace *facep)
{
    removeFaceReference(facep);

    vector_replace_with_last(mDrawFace, facep);

    return true;
}

// Not absolutely sure if we should be resetting all of the chained pools as well - djs
void LLFacePool::resetDrawOrders()
{
    mDrawFace.resize(0);
}

LLViewerTexture *LLFacePool::getTexture()
{
    return NULL;
}

void LLFacePool::removeFaceReference(LLFace *facep)
{
    if (facep->getReferenceIndex() != -1)
    {
        if (facep->getReferenceIndex() != (S32)mReferences.size())
        {
            LLFace *back = mReferences.back();
            mReferences[facep->getReferenceIndex()] = back;
            back->setReferenceIndex(facep->getReferenceIndex());
        }
        mReferences.pop_back();
    }
    facep->setReferenceIndex(-1);
}

void LLFacePool::addFaceReference(LLFace *facep)
{
    if (-1 == facep->getReferenceIndex())
    {
        facep->setReferenceIndex(static_cast<S32>(mReferences.size()));
        mReferences.push_back(facep);
    }
}

void LLFacePool::pushFaceGeometry()
{
    for (LLFace* const& face : mDrawFace)
    {
        face->renderIndexed();
    }
}

bool LLFacePool::verify() const
{
    bool ok = true;

    for (std::vector<LLFace*>::const_iterator iter = mDrawFace.begin();
         iter != mDrawFace.end(); iter++)
    {
        const LLFace* facep = *iter;
        if (facep->getPool() != this)
        {
            LL_INFOS() << "Face in wrong pool!" << LL_ENDL;
            facep->printDebugInfo();
            ok = false;
        }
        else if (!facep->verify())
        {
            ok = false;
        }
    }

    return ok;
}

void LLFacePool::printDebugInfo() const
{
    LL_INFOS() << "Pool " << this << " Type: " << getType() << LL_ENDL;
}

bool LLFacePool::LLOverrideFaceColor::sOverrideFaceColor = false;

void LLFacePool::LLOverrideFaceColor::setColor(const LLColor4& color)
{
    gGL.diffuseColor4fv(color.mV);
}

void LLFacePool::LLOverrideFaceColor::setColor(const LLColor4U& color)
{
    glColor4ubv(color.mV);
}

void LLFacePool::LLOverrideFaceColor::setColor(F32 r, F32 g, F32 b, F32 a)
{
    gGL.diffuseColor4f(r,g,b,a);
}


//=============================
// Render Pass Implementation
//=============================
LLRenderPass::LLRenderPass(const U32 type)
: LLDrawPool(type)
{

}

LLRenderPass::~LLRenderPass()
{

}

void LLRenderPass::renderGroup(LLSpatialGroup* group, U32 type, bool texture)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[type];

    for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)
    {
        LLDrawInfo *pparams = *k;
        if (pparams)
        {
            pushBatch(*pparams, texture);
        }
    }
}

void LLRenderPass::renderRiggedGroup(LLSpatialGroup* group, U32 type, bool texture)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    LLSpatialGroup::drawmap_elem_t& draw_info = group->mDrawMap[type];
    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    for (LLSpatialGroup::drawmap_elem_t::iterator k = draw_info.begin(); k != draw_info.end(); ++k)
    {
        LLDrawInfo* pparams = *k;
        if (pparams)
        {
            if (uploadMatrixPalette(pparams->mAvatar, pparams->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
            {
                pushBatch(*pparams, texture);
            }
        }
    }
}

void LLRenderPass::pushBatches(U32 type, bool texture, bool batch_textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    if (texture)
    {
        auto* begin = gPipeline.beginRenderMap(type);
        auto* end = gPipeline.endRenderMap(type);
        for (LLCullResult::drawinfo_iterator i = begin; i != end; )
        {
            LLDrawInfo* pparams = *i;
            LLCullResult::increment_iterator(i, end);

            if (fsr34_suppress_mouselook_volume_draw_info(type, *pparams))
            {
                continue;
            }

            fsr34_record_mouselook_volume_draw_info(type, *pparams, false);
            pushBatch(*pparams, texture, batch_textures);
        }
    }
    else
    {
        pushUntexturedBatches(type);
    }
}

void LLRenderPass::pushUntexturedBatches(U32 type)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LLDrawInfo* pparams = *i;
        LLCullResult::increment_iterator(i, end);

        if (fsr34_suppress_mouselook_volume_draw_info(type, *pparams))
        {
            continue;
        }

        fsr34_record_mouselook_volume_draw_info(type, *pparams, false);
        pushUntexturedBatch(*pparams);
    }
}

void LLRenderPass::pushRiggedBatches(U32 type, bool texture, bool batch_textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;

    if (texture)
    {
        const LLVOAvatar* lastAvatar = nullptr;
        U64 lastMeshId = 0;
        bool skipLastSkin = false;
        auto* begin = gPipeline.beginRenderMap(type);
        auto* end = gPipeline.endRenderMap(type);
        for (LLCullResult::drawinfo_iterator i = begin; i != end; )
        {
            LLDrawInfo* pparams = *i;
            LLCullResult::increment_iterator(i, end);

            if (fsr34_suppress_mouselook_volume_draw_info(type, *pparams))
            {
                continue;
            }

            if (uploadMatrixPalette(pparams->mAvatar, pparams->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
            {
                fsr34_record_mouselook_volume_draw_info(type, *pparams, true);
                pushBatch(*pparams, texture, batch_textures);
            }
        }
    }
    else
    {
        pushUntexturedRiggedBatches(type);
    }
}

void LLRenderPass::pushUntexturedRiggedBatches(U32 type)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;
    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LLDrawInfo* pparams = *i;
        LLCullResult::increment_iterator(i, end);

        if (fsr34_suppress_mouselook_volume_draw_info(type, *pparams))
        {
            continue;
        }

        if (uploadMatrixPalette(pparams->mAvatar, pparams->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
        {
            fsr34_record_mouselook_volume_draw_info(type, *pparams, true);
            pushUntexturedBatch(*pparams);
        }
    }
}

void LLRenderPass::pushMaskBatches(U32 type, bool texture, bool batch_textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LLDrawInfo* pparams = *i;
        LLCullResult::increment_iterator(i, end);
        if (fsr34_suppress_mouselook_volume_draw_info(type, *pparams))
        {
            continue;
        }
        LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(pparams->mAlphaMaskCutoff);
        fsr34_record_mouselook_volume_draw_info(type, *pparams, false);
        pushBatch(*pparams, texture, batch_textures);
    }
}

void LLRenderPass::pushRiggedMaskBatches(U32 type, bool texture, bool batch_textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;
    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LLDrawInfo* pparams = *i;

        LLCullResult::increment_iterator(i, end);

        llassert(pparams);

        if (fsr34_suppress_mouselook_volume_draw_info(type, *pparams))
        {
            continue;
        }

        LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(pparams->mAlphaMaskCutoff);

        if (uploadMatrixPalette(pparams->mAvatar, pparams->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
        {
            fsr34_record_mouselook_volume_draw_info(type, *pparams, true);
            pushBatch(*pparams, texture, batch_textures);
        }
    }
}

void LLRenderPass::applyModelMatrix(const LLDrawInfo& params)
{
    applyModelMatrix(params.mModelMatrix);
}

void LLRenderPass::applyModelMatrix(const LLMatrix4* model_matrix)
{
    if (model_matrix != gGLLastMatrix)
    {
        gGLLastMatrix = model_matrix;
        gGL.matrixMode(LLRender::MM_MODELVIEW);
        gGL.loadMatrix(gGLModelView);
        if (model_matrix)
        {
            gGL.multMatrix((GLfloat*) model_matrix->mMatrix);
        }
        gPipeline.mMatrixOpCount++;
    }
}

void LLRenderPass::pushBatch(LLDrawInfo& params, bool texture, bool batch_textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    llassert(texture);

    if (!params.mCount)
    {
        return;
    }

    applyModelMatrix(params);

    bool tex_setup = false;

    {
        if (batch_textures && params.mTextureList.size() > 1)
        {
            for (U32 i = 0; i < params.mTextureList.size(); ++i)
            {
                if (params.mTextureList[i].notNull())
                {
                    gGL.getTexUnit(i)->bindFast(params.mTextureList[i]);
                }
            }
        }
        else
        { //not batching textures or batch has only 1 texture -- might need a texture matrix
            if (params.mTexture.notNull())
            {
                gGL.getTexUnit(0)->bindFast(params.mTexture);
                if (params.mTextureMatrix)
                {
                    tex_setup = true;
                    gGL.getTexUnit(0)->activate();
                    gGL.matrixMode(LLRender::MM_TEXTURE);
                    gGL.loadMatrix((GLfloat*) params.mTextureMatrix->mMatrix);
                    gPipeline.mTextureMatrixOps++;
                }
            }
            else
            {
                gGL.getTexUnit(0)->unbindFast(LLTexUnit::TT_TEXTURE);
            }
        }
    }
    // <FS:Beq> FIRE-34518 bugsplat access violation - place guard on unchecked mVertexBuffer access
    if (params.mVertexBuffer == nullptr)
    {
        LL_WARNS() << "LLRenderPass::pushBatch: params.mVertexBuffer is nullptr. drawRange skipped." << LL_ENDL;
        return;
    }
    // </FS:Beq>
    params.mVertexBuffer->setBuffer();
    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
    if (tex_setup)
    {
        gGL.matrixMode(LLRender::MM_TEXTURE0);
        gGL.loadIdentity();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
    }
}

void LLRenderPass::pushUntexturedBatch(LLDrawInfo& params)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;

    if (!params.mCount)
    {
        return;
    }

    applyModelMatrix(params);

    params.mVertexBuffer->setBuffer();
    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
}

// static
bool LLRenderPass::uploadMatrixPalette(LLDrawInfo& params)
{
    // upload matrix palette to shader
    return uploadMatrixPalette(params.mAvatar, params.mSkinInfo);
}

//static
bool LLRenderPass::uploadMatrixPalette(LLVOAvatar* avatar, const LLMeshSkinInfo* skinInfo) // <FS:Beq/> be defensive about UAF with skinInfo during LocalMesh
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    if (!avatar)
    {
        return false;
    }
    const LLVOAvatar::MatrixPaletteCache& mpc = avatar->updateSkinInfoMatrixPalette(skinInfo);
    U32 count = static_cast<U32>(mpc.mMatrixPalette.size());

    if (count == 0)
    {
        //skin info not loaded yet, don't render
        return false;
    }

    LLGLSLShader::sCurBoundShaderPtr->uniformMatrix3x4fv(LLViewerShaderMgr::AVATAR_MATRIX,
        count,
        false,
        (GLfloat*)&(mpc.mGLMp[0]));

    return true;
}

// Returns true if rendering should proceed
//static
bool LLRenderPass::uploadMatrixPalette(LLVOAvatar* avatar, const LLMeshSkinInfo* skinInfo, const LLVOAvatar*& lastAvatar, U64& lastMeshId, bool& skipLastSkin)// <FS:Beq/> be defensive about UAF with skinInfo during LocalMesh
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    llassert(skinInfo);
    llassert(LLGLSLShader::sCurBoundShaderPtr);

    if (!avatar)
    {
        return false;
    }

    if (avatar == lastAvatar && skinInfo->mHash == lastMeshId)
    {
        return !skipLastSkin;
    }

    const LLVOAvatar::MatrixPaletteCache& mpc = avatar->updateSkinInfoMatrixPalette(skinInfo);
    U32 count = static_cast<U32>(mpc.mMatrixPalette.size());
    // skipLastSkin -> skin info not loaded yet, don't render
    skipLastSkin = !bool(count);
    lastAvatar = avatar;
    lastMeshId = skinInfo->mHash;

    if (!skipLastSkin)
    {
        LLGLSLShader::sCurBoundShaderPtr->uniformMatrix3x4fv(LLViewerShaderMgr::AVATAR_MATRIX,
            count,
            false,
            (GLfloat*)&(mpc.mGLMp[0]));
    }

    return !skipLastSkin;
}

// Returns true if rendering should proceed
//static
bool LLRenderPass::uploadMatrixPalette(LLVOAvatar* avatar, const LLMeshSkinInfo* skinInfo, const LLVOAvatar*& lastAvatar, U64& lastMeshId, const LLGLSLShader*& lastAvatarShader, bool& skipLastSkin)// <FS:Beq/> be defensive about UAF with skinInfo during LocalMesh
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    llassert(skinInfo);
    llassert(LLGLSLShader::sCurBoundShaderPtr);

    if (!avatar)
    {
        return false;
    }

    if (avatar == lastAvatar && skinInfo->mHash == lastMeshId && lastAvatarShader == LLGLSLShader::sCurBoundShaderPtr)
    {
        return !skipLastSkin;
    }

    const LLVOAvatar::MatrixPaletteCache& mpc = avatar->updateSkinInfoMatrixPalette(skinInfo);
    U32 count = static_cast<U32>(mpc.mMatrixPalette.size());
    // skipLastSkin -> skin info not loaded yet, don't render
    skipLastSkin = !bool(count);
    lastAvatar = avatar;
    lastMeshId = skinInfo->mHash;
    lastAvatarShader = LLGLSLShader::sCurBoundShaderPtr;

    if (!skipLastSkin)
    {
        LLGLSLShader::sCurBoundShaderPtr->uniformMatrix3x4fv(LLViewerShaderMgr::AVATAR_MATRIX,
            count,
            false,
            (GLfloat*)&(mpc.mGLMp[0]));
    }

    return !skipLastSkin;
}

void setup_texture_matrix(LLDrawInfo& params)
{
    if (params.mTextureMatrix)
    { //special case implementation of texture animation here because of special handling of textures for PBR batches
        gGL.getTexUnit(0)->activate();
        gGL.matrixMode(LLRender::MM_TEXTURE);
        gGL.loadMatrix((GLfloat*)params.mTextureMatrix->mMatrix);
        gPipeline.mTextureMatrixOps++;
    }
}

void teardown_texture_matrix(LLDrawInfo& params)
{
    if (params.mTextureMatrix)
    {
        gGL.matrixMode(LLRender::MM_TEXTURE0);
        gGL.loadIdentity();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
    }
}

// <AYAstorm r30 P2> Velocity buffer push helpers (BD lineage). Iterate the
// render map for the given pool type, upload per-object last/current matrices
// via the LAST_OBJECT_MATRIX uniform (set by Step 3 enum), draw, then store
// the current matrix back into params.mLastModelMatrix for next frame.
//
// RenderMotionBlur{Self,Other}Avatars opt-out: skip the draw entirely. The
// velocity RT is cleared to (0,0) at frame start in renderGeomMotionBlur
// (pipeline.cpp), so any pixel we don't write reads back as (0,0), which
// motionBlurF.glsl's `if (speed < 2.0) return diffuseRect` branch treats as
// "no blur" — exactly the requested outcome.
void LLRenderPass::pushVelocityBatches(U32 type)
{
    static const LLMatrix4 identity;
    static LLCachedControl<bool> self_blur(gSavedSettings, "RenderMotionBlurSelfAvatar", true);
    static LLCachedControl<bool> others_blur(gSavedSettings, "RenderMotionBlurOtherAvatars", true);

    auto* begin = gPipeline.beginRenderMap(type);
    auto* end   = gPipeline.endRenderMap(type);

    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LLDrawInfo& params = **i;
        LLCullResult::increment_iterator(i, end);

        if (!params.mVertexBuffer.notNull())
        {
            continue;
        }

        if (params.mAttachedToAvatar.notNull() &&
            (params.mAttachedToAvatar->isSelf() ? !self_blur : !others_blur))
        {
            continue;
        }

        LLGLDisable cull_face(params.mGLTFMaterial && params.mGLTFMaterial->mDoubleSided ? GL_CULL_FACE : 0);

        applyModelMatrix(params);

        const LLMatrix4* last_mat = params.mLastModelMatrix ? params.mLastModelMatrix : &identity;
        LLGLSLShader::sCurBoundShaderPtr->uniformMatrix4fv(LLShaderMgr::LAST_OBJECT_MATRIX, 1, GL_FALSE, (GLfloat*)last_mat->mMatrix);

        params.mVertexBuffer->setBuffer();
        params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);

        const LLMatrix4* current_mat = params.mModelMatrix ? params.mModelMatrix : &identity;
        if (params.mLastModelMatrix)
        {
            *params.mLastModelMatrix = *current_mat;
        }
    }
}

void LLRenderPass::pushRiggedVelocityBatches(U32 type)
{
    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    static LLCachedControl<bool> self_blur(gSavedSettings, "RenderMotionBlurSelfAvatar", true);
    static LLCachedControl<bool> others_blur(gSavedSettings, "RenderMotionBlurOtherAvatars", true);

    auto* begin = gPipeline.beginRenderMap(type);
    auto* end   = gPipeline.endRenderMap(type);

    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LLDrawInfo& params = **i;
        LLCullResult::increment_iterator(i, end);

        if (!params.mVertexBuffer.notNull() || !params.mAvatar)
        {
            continue;
        }

        if (params.mAvatar->isSelf() ? !self_blur : !others_blur)
        {
            continue;
        }

        if (!uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
        {
            continue;
        }

        uploadLastMatrixPalette(params.mAvatar, params.mSkinInfo);

        applyModelMatrix(params);

        params.mVertexBuffer->setBuffer();
        params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
    }
}

void LLRenderPass::pushVelocityBatchesTextured(U32 type)
{
    static const LLMatrix4 identity;
    static LLCachedControl<bool> self_blur(gSavedSettings, "RenderMotionBlurSelfAvatar", true);
    static LLCachedControl<bool> others_blur(gSavedSettings, "RenderMotionBlurOtherAvatars", true);

    auto* begin = gPipeline.beginRenderMap(type);
    auto* end   = gPipeline.endRenderMap(type);

    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LLDrawInfo& params = **i;
        LLCullResult::increment_iterator(i, end);

        if (!params.mVertexBuffer.notNull())
        {
            continue;
        }

        if (params.mAttachedToAvatar.notNull() &&
            (params.mAttachedToAvatar->isSelf() ? !self_blur : !others_blur))
        {
            continue;
        }

        LLGLDisable cull_face(params.mGLTFMaterial && params.mGLTFMaterial->mDoubleSided ? GL_CULL_FACE : 0);

        applyModelMatrix(params);

        if (params.mTexture.notNull())
        {
            gGL.getTexUnit(0)->bindFast(params.mTexture);
        }

        const LLMatrix4* last_mat = params.mLastModelMatrix ? params.mLastModelMatrix : &identity;
        LLGLSLShader::sCurBoundShaderPtr->uniformMatrix4fv(LLShaderMgr::LAST_OBJECT_MATRIX, 1, GL_FALSE, (GLfloat*)last_mat->mMatrix);

        params.mVertexBuffer->setBuffer();
        params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);

        const LLMatrix4* current_mat = params.mModelMatrix ? params.mModelMatrix : &identity;
        if (params.mLastModelMatrix)
        {
            *params.mLastModelMatrix = *current_mat;
        }
    }
}

void LLRenderPass::pushRiggedVelocityBatchesTextured(U32 type)
{
    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    static LLCachedControl<bool> self_blur(gSavedSettings, "RenderMotionBlurSelfAvatar", true);
    static LLCachedControl<bool> others_blur(gSavedSettings, "RenderMotionBlurOtherAvatars", true);

    auto* begin = gPipeline.beginRenderMap(type);
    auto* end   = gPipeline.endRenderMap(type);

    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LLDrawInfo& params = **i;
        LLCullResult::increment_iterator(i, end);

        if (!params.mVertexBuffer.notNull() || !params.mAvatar)
        {
            continue;
        }

        if (params.mAvatar->isSelf() ? !self_blur : !others_blur)
        {
            continue;
        }

        if (!uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
        {
            continue;
        }

        uploadLastMatrixPalette(params.mAvatar, params.mSkinInfo);

        applyModelMatrix(params);

        if (params.mTexture.notNull())
        {
            gGL.getTexUnit(0)->bindFast(params.mTexture);
        }

        params.mVertexBuffer->setBuffer();
        params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
    }
}

//static
bool LLRenderPass::uploadLastMatrixPalette(LLVOAvatar* avatar, const LLMeshSkinInfo* skinInfo)
{
    if (!avatar || !skinInfo)
    {
        return false;
    }

    const LLVOAvatar::MatrixPaletteCache& mpc = avatar->updateSkinInfoMatrixPalette(skinInfo);
    U32 count = static_cast<U32>(mpc.mMatrixPalette.size());

    if (count == 0)
    {
        return false;
    }

    // First-frame fallback: when mLastGLMp hasn't been populated yet (new hash
    // or first visible frame), uploading nothing would leave lastMatrixPalette[]
    // holding bones from whichever rig drew previously — those get read as the
    // "last frame" of this rig and produce lightning-streak velocity. Upload
    // mGLMp instead so last_pose == curr_pose → velocity = 0, the correct
    // "no motion captured yet" answer.
    const std::vector<F32>& src = mpc.mLastGLMp.empty() ? mpc.mGLMp : mpc.mLastGLMp;
    if (src.empty())
    {
        return false;
    }

    LLGLSLShader::sCurBoundShaderPtr->uniformMatrix3x4fv(LLShaderMgr::AVATAR_LAST_MATRIX,
        count,
        false,
        (GLfloat*)&(src[0]));

    return true;
}
// </AYAstorm r30 P2>

void LLRenderPass::pushGLTFBatches(U32 type, bool textured)
{
    if (textured)
    {
        pushGLTFBatches(type);
    }
    else
    {
        pushUntexturedGLTFBatches(type);
    }
}

void LLRenderPass::pushGLTFBatches(U32 type)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("pushGLTFBatch");
        LLDrawInfo& params = **i;
        LLCullResult::increment_iterator(i, end);

        pushGLTFBatch(params);
    }
}

void LLRenderPass::pushUntexturedGLTFBatches(U32 type)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("pushGLTFBatch");
        LLDrawInfo& params = **i;
        LLCullResult::increment_iterator(i, end);

        pushUntexturedGLTFBatch(params);
    }
}

// static
void LLRenderPass::pushGLTFBatch(LLDrawInfo& params)
{
    auto& mat = params.mGLTFMaterial;

    if (mat.notNull())
    {
        mat->bind(params.mTexture);
    }

    LLGLDisable cull_face(mat.notNull() && mat->mDoubleSided ? GL_CULL_FACE : 0);

    setup_texture_matrix(params);

    applyModelMatrix(params);

    // <FS:AYA r20 Phase C> per-draw SSS skin marker for PBR opaque path
    // (no per-pool caching — GLTF batches each rebind material state).
    LLGLSLShader* cur = LLGLSLShader::sCurBoundShaderPtr;
    if (cur)
    {
        GLint loc = cur->getUniformLocation(LLShaderMgr::AYA_SSS_SKIN_FLAG);
        if (loc > -1)
        {
            glUniform1f(loc, params.mIsSSSTarget ? 1.f : 0.f);
        }
    }
    // </FS:AYA>

    params.mVertexBuffer->setBuffer();
    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);

    teardown_texture_matrix(params);
}

// static
void LLRenderPass::pushUntexturedGLTFBatch(LLDrawInfo& params)
{
    auto& mat = params.mGLTFMaterial;

    LLGLDisable cull_face(mat->mDoubleSided ? GL_CULL_FACE : 0);

    applyModelMatrix(params);

    params.mVertexBuffer->setBuffer();
    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
}

void LLRenderPass::pushRiggedGLTFBatches(U32 type, bool textured)
{
    if (textured)
    {
        pushRiggedGLTFBatches(type);
    }
    else
    {
        pushUntexturedRiggedGLTFBatches(type);
    }
}

void LLRenderPass::pushRiggedGLTFBatches(U32 type)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("pushRiggedGLTFBatch");
        LLDrawInfo& params = **i;
        LLCullResult::increment_iterator(i, end);

        pushRiggedGLTFBatch(params, lastAvatar, lastMeshId, skipLastSkin);
    }
}

void LLRenderPass::pushUntexturedRiggedGLTFBatches(U32 type)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    const LLVOAvatar* lastAvatar = nullptr;
    U64 lastMeshId = 0;
    bool skipLastSkin = false;

    auto* begin = gPipeline.beginRenderMap(type);
    auto* end = gPipeline.endRenderMap(type);
    for (LLCullResult::drawinfo_iterator i = begin; i != end; )
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_DRAWPOOL("pushRiggedGLTFBatch");
        LLDrawInfo& params = **i;
        LLCullResult::increment_iterator(i, end);

        pushUntexturedRiggedGLTFBatch(params, lastAvatar, lastMeshId, skipLastSkin);
    }
}


// static
void LLRenderPass::pushRiggedGLTFBatch(LLDrawInfo& params, const LLVOAvatar*& lastAvatar, U64& lastMeshId, bool& skipLastSkin)
{
    if (uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
    {
        pushGLTFBatch(params);
    }
}

// static
void LLRenderPass::pushUntexturedRiggedGLTFBatch(LLDrawInfo& params, const LLVOAvatar*& lastAvatar, U64& lastMeshId, bool& skipLastSkin)
{
    if (uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
    {
        pushUntexturedGLTFBatch(params);
    }
}
