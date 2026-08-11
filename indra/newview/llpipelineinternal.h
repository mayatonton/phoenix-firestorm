/**
 * @file llpipelineinternal.h
 * @brief Shared file-scope pieces for the split pipeline translation units (pure move from pipeline.cpp).
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

#ifndef LL_LLPIPELINEINTERNAL_H
#define LL_LLPIPELINEINTERNAL_H

#include "pipeline.h"
#include "llpipelineframecontext.h"
#include "llviewercontrol.h"
#include "llsettingssky.h"
#include "llrendertarget.h"
#include "llviewertexture.h"

extern bool gSnapshot;
extern bool gShiftFrame;
extern bool gAvatarBacklight;

void drawBox(const LLVector4a& c, const LLVector4a& r);
void drawBoxOutline(const LLVector3& pos, const LLVector3& size);

const F32 BACKLIGHT_DAY_MAGNITUDE_OBJECT = 0.1f;
const F32 BACKLIGHT_NIGHT_MAGNITUDE_OBJECT = 0.08f;
const F32 ALPHA_BLEND_CUTOFF = 0.598f;
const F32 DEFERRED_LIGHT_FALLOFF = 0.5f;
const U32 DEFERRED_VB_MASK = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_TEXCOORD0 | LLVertexBuffer::MAP_TEXCOORD1;

static const glm::mat4 sGlNdcToSampleBias(0.5f, 0.0f, 0.0f, 0.0f,
                                          0.0f, 0.5f, 0.0f, 0.0f,
                                          0.0f, 0.0f, 0.5f, 0.0f,
                                          0.5f, 0.5f, 0.5f, 1.0f);

extern bool gCubeSnapshot;
extern bool gSnapshotNoPost;
extern bool gHeroProbeMirrorRender;

U32 nhpo2(U32 v);
LLVertexBuffer* ll_create_cube_vb(U32 type_mask);

void display_update_camera();

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

    inline F32 ayaDeriveReflectionProbeAmbianceVk(const LLSettingsSky::ptr_t& psky, bool probes_enabled)
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

inline bool addDeferredAttachments(LLRenderTarget& target, bool for_impostor = false)
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

inline U32 BlurHappySize(U32 x, F32 scale) { return U32( x * scale + 16.0f) & ~0xF; }

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

#endif
