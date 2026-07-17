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

#include <mutex>
#include <set>
#include <tuple>

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
#include "llviewerobjectlist.h" // For debug listing.
#include "llviewerregion.h"
#include "pipeline.h"
#include "llvkbucket.h"
#include "llspatialpartition.h"
#include "llviewercamera.h"
#include "lldrawpoolwlsky.h"
#include "llglslshader.h"
#include "llvoavatar.h"
#include "llviewershadermgr.h"
#include "llvkloader.h"
#include "llvkuboreg.h"
#include "llimagegl.h"

S32 LLDrawPool::sNumDrawPools = 0;

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

void LLDrawPool::beginRenderPass( S32 pass )
{
}

S32  LLDrawPool::getNumPasses()
{
    return 1;
}

void LLDrawPool::beginDeferredPass(S32 pass)
{

}

void LLDrawPool::endDeferredPass(S32 pass)
{

}

S32 LLDrawPool::getNumDeferredPasses()
{
    return 0;
}

void LLDrawPool::renderDeferred(S32 pass)
{

}

void LLDrawPool::beginPostDeferredPass(S32 pass)
{

}

void LLDrawPool::endPostDeferredPass(S32 pass)
{

}

S32 LLDrawPool::getNumPostDeferredPasses()
{
    return 0;
}

void LLDrawPool::renderPostDeferred(S32 pass)
{

}

void LLDrawPool::endRenderPass( S32 pass )
{
    gGL.getTexUnit(0)->activate();
}

void LLDrawPool::beginShadowPass(S32 pass)
{

}

void LLDrawPool::endShadowPass(S32 pass)
{

}

S32 LLDrawPool::getNumShadowPasses()
{
    return 0;
}

void LLDrawPool::renderShadow(S32 pass)
{

}

// <AYAstorm r30 P2> Velocity-buffer pass defaults (BD lineage).
void LLDrawPool::beginMotionBlurPass(S32 pass)
{

}

void LLDrawPool::endMotionBlurPass(S32 pass)
{

}

S32 LLDrawPool::getNumMotionBlurPasses()
{
    return 0;
}

void LLDrawPool::renderMotionBlur(S32 pass)
{

}
// </AYAstorm r30 P2>

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

bool LLFacePool::addFace(LLFace *facep)
{
    addFaceReference(facep);
    return true;
}

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
    gGL.diffuseColor4ubv(color.mV);
}

void LLFacePool::LLOverrideFaceColor::setColor(F32 r, F32 g, F32 b, F32 a)
{
    gGL.diffuseColor4f(r,g,b,a);
}


thread_local F32 LLRenderPass::sShadowBatchCullRadius = 0.f;

static inline bool vkShadowCullBatch(const LLDrawInfo& params)
{
    if (LLRenderPass::sShadowBatchCullRadius > 0.f)
    {
        if (params.mAvatar.notNull())
        {
            ++LLVKLoader::gVkPerf.shadow_rigged;
            return false;
        }
        if (params.mBoundRadius >= 0.f
            && params.mBoundRadius < LLRenderPass::sShadowBatchCullRadius)
        {
            ++LLVKLoader::gVkPerf.shadow_cull;
            return true;
        }
    }
    return false;
}

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

void LLRenderPass::buildAndOverrideScenePerDrawSet(LLDrawInfo* params, bool batch_textures,
                                                    U64 gltf_materials_ubo,
                                                    U32 gltf_materials_size,
                                                    U64 gltf_geometry_ubo,
                                                    U32 gltf_geometry_size)
{
    if (!LLVKLoader::isVulkanInitialized())
    {
        return;
    }
    LLGLSLShader* cur = LLGLSLShader::sCurBoundShaderPtr;
    if (cur == nullptr)
    {
        return;
    }
    VkSampler sampler = LLVKLoader::getStandardLinearSampler();
    if (sampler == VK_NULL_HANDLE || cur->mVkDescriptorSetLayout == VK_NULL_HANDLE)
    {
        return;
    }

    const bool is_indexed = (cur->mFeatures.mIndexedTextureChannels > 0) && !cur->mVkUsesBindlessHeap;
    const U32 indexed_layout_count =
        llmin((U32)cur->mFeatures.mIndexedTextureChannels,
              (U32)LLVKLoader::ScenePerDrawBindings::MAX_SAMPLERS);
    const U32 set_shape = (params != nullptr && batch_textures && params->mTextureList.size() > 1 && is_indexed)
                              ? llmin((U32)params->mTextureList.size(), indexed_layout_count)
                              : (is_indexed ? 1u : 0u);

    if (cur->mVkUsesBindlessHeap)
    {
        U32 slots[4] = { 0, 0, 0, 0 };
        if (params != nullptr && batch_textures && params->mTextureList.size() > 1)
        {
            const U32 n = llmin((U32)params->mTextureList.size(), 4u);
            for (U32 i = 0; i < n; ++i)
            {
                LLTexture* t = params->mTextureList[i].get();
                slots[i] = LLImageGL::vkHeapSlotOrDefault(t ? t->getGLTexture() : nullptr);
            }
        }
        else if (params != nullptr && params->mTexture.notNull())
        {
            slots[0] = LLImageGL::vkHeapSlotOrDefault(params->mTexture->getGLTexture());
        }
        else
        {
            slots[0] = LLImageGL::vkHeapSlotOrDefault(gGL.getTexUnit(0)->mCurrImageGL);
        }
        U32 id = 0;
        if (params != nullptr)
        {
            const bool ok = params->ensureVkDrawDataSlot(slots);
            id = ok ? params->mVkDrawDataSlot : LLVKLoader::drawDataWriteScratch(slots);
        }
        else
        {
            id = LLVKLoader::drawDataWriteScratch(slots);
        }
        LLVKLoader::setCurrentDrawDataID(id);
    }

    if (gltf_materials_ubo == 0 && gltf_geometry_ubo == 0
        && LLGLSLShader::sCurPerCallVkDescriptorSet != VK_NULL_HANDLE
        && LLGLSLShader::sCurPerCallVkSetShape == set_shape)
    {
        ++LLVKLoader::gVkPerf.set_reuse;
        return;
    }

    const bool memo_eligible = (params != nullptr && is_indexed && set_shape >= 1
                                && gltf_materials_ubo == 0 && gltf_geometry_ubo == 0);
    const U32  memo_frame    = LLVKLoader::getCurrentFrameIndex();
    U64        memo_sig      = 0;

    if (memo_eligible)
    {
        for (U32 i = 0; i < set_shape; ++i)
        {
            LLTexUnit* tu = gGL.getTexUnit((S32)i);
            memo_sig = memo_sig * 0x100000001B3ull ^ (U64)(uintptr_t)tu->getLiveVkImageView();
            memo_sig = memo_sig * 0x100000001B3ull ^ (U64)(uintptr_t)tu->getLiveVkSampler();
            memo_sig = memo_sig * 0x100000001B3ull ^ (U64)(uintptr_t)tu->mCurrImageGL;
        }

        if (params->mVkSetMemoShader == cur
            && params->mVkSetMemoShape == set_shape
            && params->mVkSetMemoSet[memo_frame] != nullptr
            && params->mVkSetMemoTexSig == memo_sig
            && cur->mVkAccessorBindingListBuilt
            && params->mVkSetMemoTopoGen == LLVKLoader::gVkPerDrawTopologyGen.load(std::memory_order_relaxed))
        {
            if (LLGLSLShader::vkValidatePerCallCache(cur, params->mVkSetMemoRingSig[memo_frame],
                    params->mVkSetMemoL3Views, params->mVkSetMemoL3Enums, params->mVkSetMemoL3Count))
            {
                LLGLSLShader::sCurPerCallVkDescriptorSet =
                    (VkDescriptorSet)params->mVkSetMemoSet[memo_frame];
                LLGLSLShader::sCurPerCallVkOffsetsDirty = true;
                LLGLSLShader::sCurPerCallVkSetShape     = set_shape;
                ++LLVKLoader::gVkPerf.set_memo;
                return;
            }
        }
    }

    if (cur->mVkUsesBindlessHeap && gltf_materials_ubo == 0 && gltf_geometry_ubo == 0
        && cur->mVkAccessorBindingListBuilt
        && cur->mVkBindlessSet1[memo_frame] != nullptr
        && cur->mVkBindlessSet1TopoGen == LLVKLoader::gVkPerDrawTopologyGen.load(std::memory_order_relaxed)
        && LLGLSLShader::vkValidatePerCallCache(cur, cur->mVkBindlessSet1RingSig[memo_frame],
               cur->mVkBindlessSet1L3Views, cur->mVkBindlessSet1L3Enums, cur->mVkBindlessSet1L3Count))
    {
        LLGLSLShader::sCurPerCallVkDescriptorSet = cur->mVkBindlessSet1[memo_frame];
        LLGLSLShader::sCurPerCallVkOffsetsDirty  = true;
        LLGLSLShader::sCurPerCallVkSetShape      = set_shape;
        ++LLVKLoader::gVkPerf.set_memo;
        return;
    }

    bool memo_fill     = memo_eligible;
    bool bindless_fill = cur->mVkUsesBindlessHeap && gltf_materials_ubo == 0 && gltf_geometry_ubo == 0;
    U64  memo_ring_sig = 0;
    const bool build_accessor_list = !cur->mVkAccessorBindingListBuilt;
    U8   memo_l3_cnt   = 0;
    S16  memo_l3_enums[6] = {};
    void* memo_l3_views[6] = {};

    LLVKLoader::ScenePerDrawBindings bindings;
    bindings.layout       = cur->mVkDescriptorSetLayout;
    bindings.sampler      = sampler;
    bindings.dynamic_mask = cur->mVkDynamicBindingMask;

    U32 per_program_dynamic_offset = 0;
    if (cur->mVkPerProgramUBO != VK_NULL_HANDLE && cur->mVkPerProgramUBOSize > 0)
    {
        VkBuffer pp_buf = VK_NULL_HANDLE;
        if (cur->vkResolvePerProgramForDraw(pp_buf, per_program_dynamic_offset))
        {
            bindings.ubo         = pp_buf;
            bindings.ubo_binding = cur->mVkPerProgramUBOBinding;
            bindings.ubo_size    = cur->mVkPerProgramUBOSize;
        }
    }

    LLImageGL*  fb_img = (LLImageGL::sDefaultGLTexture != nullptr && LLImageGL::sDefaultGLTexture->hasVkImage())
                             ? LLImageGL::sDefaultGLTexture
                             : LLImageGL::sWhiteImageGLp;
    VkImageView fallback_view = VK_NULL_HANDLE;
    if (fb_img != nullptr && fb_img->hasVkImage())
    {
        fallback_view = fb_img->getVkImageView();
    }

    if (params != nullptr && batch_textures && params->mTextureList.size() > 1 && is_indexed)
    {
        const U32 real_count = llmin((U32)params->mTextureList.size(),
                                     indexed_layout_count);
        for (U32 i = 0; i < indexed_layout_count; ++i)
        {
            VkImageView view_to_write = VK_NULL_HANDLE;
            if (i < real_count)
            {
                view_to_write = gGL.getTexUnit((S32)i)->getLiveVkImageView();
                if (view_to_write != VK_NULL_HANDLE &&
                    (100 + i) < LLGLSLShader::MAX_VK_BINDING &&
                    gGL.getTexUnit((S32)i)->getLiveVkImageViewDim() != cur->mVkBindingSamplerDim[100 + i])
                {
                    view_to_write = VK_NULL_HANDLE;
                }
                if (view_to_write == VK_NULL_HANDLE)
                {
                    view_to_write = fallback_view;
                }
            }
            else
            {
                view_to_write = fallback_view;
            }
            bindings.sampler_bindings[i] = 100 + i;
            bindings.sampler_views[i]    = view_to_write;
            bindings.sampler_samplers[i] = (i < real_count)
                                               ? gGL.getTexUnit((S32)i)->getLiveVkSampler()
                                               : VK_NULL_HANDLE;
        }
        bindings.sampler_count = indexed_layout_count;
    }
    else if (is_indexed)
    {
        for (U32 i = 0; i < indexed_layout_count; ++i)
        {
            VkImageView view_to_write = VK_NULL_HANDLE;
            if (i == 0)
            {
                view_to_write = gGL.getTexUnit(0)->getLiveVkImageView();
                if (view_to_write != VK_NULL_HANDLE &&
                    gGL.getTexUnit(0)->getLiveVkImageViewDim() != cur->mVkBindingSamplerDim[100])
                {
                    view_to_write = VK_NULL_HANDLE;
                }
                if (view_to_write == VK_NULL_HANDLE)
                {
                    view_to_write = fallback_view;
                }
            }
            else
            {
                view_to_write = fallback_view;
            }
            bindings.sampler_bindings[i] = 100 + i;
            bindings.sampler_views[i]    = view_to_write;
            bindings.sampler_samplers[i] = (i == 0)
                                               ? gGL.getTexUnit(0)->getLiveVkSampler()
                                               : VK_NULL_HANDLE;
        }
        bindings.sampler_count = indexed_layout_count;
    }
    else
    {
        if (cur->mVkUsesBindlessHeap)
        {
            bindings.sampler_bindings[0] = 1;
            bindings.sampler_views[0]    = fallback_view;
            bindings.sampler_samplers[0] = sampler;
            bindings.sampler_count       = 1;
        }
        else
        {
            const S32 enum1 = cur->mVkBindingToEnum[1];
            const S32 unit1 = (cur->mVkBindingToChannel[1] >= 0) ? cur->mVkBindingToChannel[1] : 0;
            const bool l3_hit = (enum1 >= 0 && enum1 < (S32)cur->mVkEnumBoundView.size()
                                 && cur->mVkEnumBoundView[enum1].bound);
            VkImageView view_to_write = VK_NULL_HANDLE;
            VkSampler   sampler1      = VK_NULL_HANDLE;
            if (l3_hit)
            {
                view_to_write = cur->vkResolveEnumBoundView(enum1);
                sampler1      = cur->mVkEnumBoundView[enum1].sampler;
                if (view_to_write != VK_NULL_HANDLE &&
                    cur->vkResolveEnumBoundDim(enum1) != cur->mVkBindingSamplerDim[1])
                {
                    view_to_write = VK_NULL_HANDLE;
                }
            }
            else
            {
                view_to_write = gGL.getTexUnit(unit1)->getLiveVkImageView();
                sampler1      = gGL.getTexUnit(unit1)->getLiveVkSampler();
                LLGLSLShader::vkWarnL3Fallback(cur, 1, enum1, view_to_write);
                if (view_to_write != VK_NULL_HANDLE &&
                    gGL.getTexUnit(unit1)->getLiveVkImageViewDim() != cur->mVkBindingSamplerDim[1])
                {
                    view_to_write = VK_NULL_HANDLE;
                }
            }
            if (view_to_write == VK_NULL_HANDLE)
            {
                view_to_write = fallback_view;
            }
            bindings.sampler_bindings[0] = 1;
            bindings.sampler_views[0]    = view_to_write;
            bindings.sampler_samplers[0] = sampler1;
            bindings.sampler_count       = 1;
        }
    }

    for (const auto& layout_binding : cur->mVkLayoutBindings)
    {
        if (layout_binding.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        {
            continue;
        }
        U32 N = layout_binding.binding;
        if (N >= LLGLSLShader::MAX_VK_BINDING)
        {
            continue;
        }
        S32 channel    = cur->mVkBindingToChannel[N];
        S32 enum_value = cur->mVkBindingToEnum[N];
        if (enum_value == -2)
        {
            continue;
        }
        if (channel < 0 && enum_value < 0)
        {
            if ((cur->mVkBindingDeclaredType[N] & LLGLSLShader::VKBD_SAMPLER) == 0)
            {
                continue;
            }
        }
        bool already_set = false;
        for (U32 j = 0; j < bindings.sampler_count; ++j)
        {
            if (bindings.sampler_bindings[j] == N)
            {
                already_set = true;
                break;
            }
        }
        if (already_set)
        {
            continue;
        }
        if (bindings.sampler_count >= LLVKLoader::ScenePerDrawBindings::MAX_SAMPLERS)
        {
            break;
        }

        VkImageView view = VK_NULL_HANDLE;
        S32 resolved_unit = -1;
        const bool l3_hit = (enum_value >= 0 && enum_value < (S32)cur->mVkEnumBoundView.size()
                             && cur->mVkEnumBoundView[enum_value].bound);
        if (l3_hit)
        {
            view = cur->vkResolveEnumBoundView(enum_value);
            if (memo_fill || bindless_fill)
            {
                if (memo_l3_cnt < 6)
                {
                    memo_l3_enums[memo_l3_cnt] = (S16)enum_value;
                    memo_l3_views[memo_l3_cnt] = (void*)view;
                    ++memo_l3_cnt;
                }
                else
                {
                    memo_fill = false;
                    bindless_fill = false;
                }
            }
        }
        else if (channel >= 0)
        {
            resolved_unit = channel;
            view = gGL.getTexUnit((S32)channel)->getLiveVkImageView();
            LLGLSLShader::vkWarnL3Fallback(cur, N, enum_value, view);
            if (view != VK_NULL_HANDLE)
            {
                memo_fill = false;
            }
        }
        else if (enum_value >= 0 && enum_value < (S32)cur->mTexture.size())
        {
            S32 unit = cur->mTexture[enum_value];
            if (unit >= 0)
            {
                resolved_unit = unit;
                view = gGL.getTexUnit((S32)unit)->getLiveVkImageView();
                LLGLSLShader::vkWarnL3Fallback(cur, N, enum_value, view);
                if (view != VK_NULL_HANDLE)
                {
                    memo_fill = false;
                }
            }
        }

        bool need_typed_fallback = (view == VK_NULL_HANDLE);
        if (!need_typed_fallback)
        {
            if (l3_hit)
            {
                if (cur->vkResolveEnumBoundDim(enum_value) != cur->mVkBindingSamplerDim[N])
                {
                    need_typed_fallback = true;
                }
            }
            else if (resolved_unit >= 0)
            {
                LLTexUnit* dim_tu = gGL.getTexUnit(resolved_unit);
                if (dim_tu != nullptr && dim_tu->getLiveVkImageViewDim() != cur->mVkBindingSamplerDim[N])
                {
                    need_typed_fallback = true;
                }
            }
        }
        if (need_typed_fallback)
        {
            const U8 sdim_fb = cur->mVkBindingSamplerDim[N];
            view = (sdim_fb == LLGLSLShader::VKSD_CUBE_ARRAY) ? LLVKLoader::getDefaultFallbackCubeArrayVkImageView()
                 : (sdim_fb == LLGLSLShader::VKSD_CUBE)       ? LLVKLoader::getDefaultFallbackCubeVkImageView()
                 : (sdim_fb == LLGLSLShader::VKSD_3D)         ? LLVKLoader::getDefaultFallback3DVkImageView()
                 :                                              fallback_view;
        }

        bindings.sampler_bindings[bindings.sampler_count] = N;
        bindings.sampler_views[bindings.sampler_count]    = view;
        bindings.sampler_samplers[bindings.sampler_count] = l3_hit
                                                              ? cur->mVkEnumBoundView[enum_value].sampler
                                                              : (resolved_unit >= 0)
                                                                  ? gGL.getTexUnit(resolved_unit)->getLiveVkSampler()
                                                                  : VK_NULL_HANDLE;

        ++bindings.sampler_count;
    }

    if (gltf_materials_ubo != 0 && gltf_materials_size > 0 &&
        bindings.ubo_count < LLVKLoader::ScenePerDrawBindings::MAX_UBO_WRITES)
    {
        auto& entry = bindings.ubo_writes[bindings.ubo_count];
        entry.binding = 7;
        entry.buf     = reinterpret_cast<VkBuffer>(gltf_materials_ubo);
        entry.offset  = 0;
        entry.size    = gltf_materials_size;
        ++bindings.ubo_count;
    }
    if (gltf_geometry_ubo != 0 && gltf_geometry_size > 0 &&
        bindings.ubo_count < LLVKLoader::ScenePerDrawBindings::MAX_UBO_WRITES)
    {
        auto& entry = bindings.ubo_writes[bindings.ubo_count];
        entry.binding = 44;
        entry.buf     = reinterpret_cast<VkBuffer>(gltf_geometry_ubo);
        entry.offset  = 0;
        entry.size    = gltf_geometry_size;
        ++bindings.ubo_count;
    }

    for (const auto& layout_binding : cur->mVkLayoutBindings)
    {
        if (layout_binding.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
            && layout_binding.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
        {
            continue;
        }
        U32 N = layout_binding.binding;
        if (N >= LLGLSLShader::MAX_VK_BINDING)
        {
            continue;
        }
        if (bindings.ubo_count >= LLVKLoader::ScenePerDrawBindings::MAX_UBO_WRITES)
        {
            break;
        }

        VkBuffer ubo_buf = VK_NULL_HANDLE;
        VkDeviceSize ubo_sz = 0;

        if (N == 0)
        {
            if (bindings.ubo != VK_NULL_HANDLE && bindings.ubo_binding == 0)
            {
                ubo_buf = bindings.ubo;
                ubo_sz  = bindings.ubo_size;
            }
        }
        else if (N < 64 && ((cur->mVkDynamicBindingMask >> N) & 1))
        {
            continue;
        }
        else
        {
            LLGLSLShader::SharedUBOAccessor accessor = cur->mVkBindingToUBOAccessor[N];
            if (accessor)
            {
                void* mapped = nullptr;
                if (accessor(ubo_buf, mapped))
                {
                    ubo_sz = cur->sharedUBOBindingSize(N);
                }
                if (build_accessor_list && N < 256)
                {
                    cur->mVkAccessorBindingList.push_back((U8)N);
                }
                memo_ring_sig = memo_ring_sig * 0x100000001B3ull ^ (U64)(uintptr_t)ubo_buf;
            }
        }

        if (ubo_buf != VK_NULL_HANDLE && ubo_sz > 0)
        {
            auto& entry = bindings.ubo_writes[bindings.ubo_count];
            entry.binding = N;
            entry.buf     = ubo_buf;
            entry.offset  = 0;
            entry.size    = ubo_sz;
            ++bindings.ubo_count;
        }
    }

    if (build_accessor_list)
    {
        cur->mVkAccessorBindingListBuilt = true;
    }

    U32 dyn_offsets[LLGLSLShader::MAX_VK_DYNAMIC_BINDINGS] = {};
    if (!LLGLSLShader::vkCollectDynamicUBOWrites(cur, bindings, per_program_dynamic_offset, dyn_offsets))
    {
        return;
    }

    VkDescriptorSet per_draw_set = VK_NULL_HANDLE;
    void*           memo_token   = nullptr;
    if (LLVKLoader::ensureScenePerDrawDescriptorSet(bindings, &per_draw_set,
                                                    (memo_eligible || cur->mVkUsesBindlessHeap) ? &memo_token : nullptr)
        && per_draw_set != VK_NULL_HANDLE)
    {
        LLGLSLShader::sCurPerCallVkDescriptorSet = per_draw_set;
        std::memcpy(LLGLSLShader::sCurPerCallVkDynamicOffsets, dyn_offsets, sizeof(dyn_offsets));
        LLGLSLShader::sCurPerCallVkOffsetsDirty = false;
        LLGLSLShader::sCurPerCallVkSetShape = (gltf_materials_ubo == 0 && gltf_geometry_ubo == 0)
                                                  ? set_shape
                                                  : 0xFFFFFFFFu;
        ++LLVKLoader::gVkPerf.set_build;

        if (memo_fill
            && memo_token != nullptr
            && (bindings.ubo == VK_NULL_HANDLE
                || bindings.ubo == LLVKLoader::getPerDrawUBOArenaBuffer()))
        {
            const U64 topo_gen  = LLVKLoader::gVkPerDrawTopologyGen.load(std::memory_order_relaxed);
            const bool header_same =
                params->mVkSetMemoShader == cur
                && params->mVkSetMemoShape == set_shape
                && params->mVkSetMemoTexSig == memo_sig
                && params->mVkSetMemoTopoGen == topo_gen
                && params->mVkSetMemoL3Count == memo_l3_cnt
                && std::memcmp(params->mVkSetMemoL3Enums, memo_l3_enums, sizeof(memo_l3_enums)) == 0
                && std::memcmp(params->mVkSetMemoL3Views, memo_l3_views, sizeof(memo_l3_views)) == 0;
            if (!header_same)
            {
                params->clearVkSetMemoPins();
                params->mVkSetMemoShader   = cur;
                params->mVkSetMemoShape    = set_shape;
                params->mVkSetMemoTexSig   = memo_sig;
                params->mVkSetMemoTopoGen  = topo_gen;
                params->mVkSetMemoL3Count   = memo_l3_cnt;
                std::memcpy(params->mVkSetMemoL3Enums, memo_l3_enums, sizeof(memo_l3_enums));
                std::memcpy(params->mVkSetMemoL3Views, memo_l3_views, sizeof(memo_l3_views));
            }
            if (params->mVkSetMemoEntryTok[memo_frame] != memo_token)
            {
                LLVKLoader::releaseScenePerDrawEntry(params->mVkSetMemoEntryTok[memo_frame],
                                                     params->mVkSetMemoPinEpoch);
                LLVKLoader::pinScenePerDrawEntry(memo_token);
                params->mVkSetMemoEntryTok[memo_frame] = memo_token;
                params->mVkSetMemoPinEpoch = LLVKLoader::getScenePerDrawCacheEpoch();
            }
            params->mVkSetMemoRingSig[memo_frame] = memo_ring_sig;
            params->mVkSetMemoSet[memo_frame] = (void*)per_draw_set;
            ++LLVKLoader::gVkPerf.set_memo_fill;
        }
        else if (bindless_fill
            && memo_token != nullptr
            && (bindings.ubo == VK_NULL_HANDLE
                || bindings.ubo == LLVKLoader::getPerDrawUBOArenaBuffer()))
        {
            const U64 topo_gen = LLVKLoader::gVkPerDrawTopologyGen.load(std::memory_order_relaxed);
            cur->mVkBindlessSet1TopoGen = topo_gen;
            cur->mVkBindlessSet1L3Count = memo_l3_cnt;
            std::memcpy(cur->mVkBindlessSet1L3Enums, memo_l3_enums, sizeof(memo_l3_enums));
            std::memcpy(cur->mVkBindlessSet1L3Views, memo_l3_views, sizeof(memo_l3_views));
            cur->mVkBindlessSet1RingSig[memo_frame] = memo_ring_sig;
            if (cur->mVkBindlessSet1Tok[memo_frame] != memo_token)
            {
                LLVKLoader::releaseScenePerDrawEntry(cur->mVkBindlessSet1Tok[memo_frame],
                                                     cur->mVkBindlessSet1PinEpoch);
                LLVKLoader::pinScenePerDrawEntry(memo_token);
                cur->mVkBindlessSet1Tok[memo_frame] = memo_token;
                cur->mVkBindlessSet1PinEpoch = LLVKLoader::getScenePerDrawCacheEpoch();
            }
            cur->mVkBindlessSet1[memo_frame] = per_draw_set;
            ++LLVKLoader::gVkPerf.set_memo_fill;
        }
        else if (params != nullptr)
        {
            params->clearVkSetMemoPins();
            params->mVkSetMemoShader = nullptr;
        }
    }
}

void LLRenderPass::pushBatches(U32 type, bool texture, bool batch_textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    if (texture)
    {
        if (LLVKBucket::isCameraMdiPass(type)
            && LLVKBucket::emitActive(type)
            && LLVKLoader::isIndirectDrawEnabled()
            && batch_textures
            && LLGLSLShader::sCurBoundShaderPtr != nullptr
            && LLGLSLShader::sCurBoundShaderPtr->mVkUsesBindlessHeap
            && !LLVKLoader::isRecordJobActive())
        {
            const std::vector<U64>* bits = LLVKBucket::currentVisBits();
            if (bits != nullptr)
            {
                for (LLVKBucket::Bucket* bucket : LLVKBucket::bucketsForPass(type))
                {
                    pushIndirectBucket(*bucket, *bits, true);
                }
                return;
            }
        }
        LLVKBucket::forEachSource(type, [&](LLDrawInfo& params)
        {
            pushBatch(params, texture, batch_textures);
        });
    }
    else
    {
        pushUntexturedBatches(type);
    }
}

void LLRenderPass::pushUntexturedBatches(U32 type)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    if (LLVKBucket::emitActive(type)
        && LLVKLoader::isIndirectDrawEnabled()
        && LLGLSLShader::sCurBoundShaderPtr != nullptr
        && !LLVKLoader::isRecordJobActive())
    {
        const std::vector<U64>* bits = LLVKBucket::currentVisBits();
        if (bits != nullptr)
        {
            for (LLVKBucket::Bucket* bucket : LLVKBucket::bucketsForPass(type))
            {
                pushIndirectBucket(*bucket, *bits, false);
            }
            return;
        }
    }
    LLVKBucket::forEachSource(type, [&](LLDrawInfo& params)
    {
        pushUntexturedBatch(params);
    });
}

static bool pushIndirectSpans(LLVKBucket::Bucket& bucket, VkBuffer ring_buf, VkDeviceSize ring_offset)
{
    LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
    VkDescriptorSet set_to_bind = LLGLSLShader::sCurPerCallVkDescriptorSet;
    if (set_to_bind != VK_NULL_HANDLE && LLGLSLShader::sCurPerCallVkOffsetsDirty)
    {
        LLGLSLShader::vkRefreshDynamicOffsetsForDraw();
        set_to_bind = LLGLSLShader::sCurPerCallVkDescriptorSet;
    }
    if (set_to_bind == VK_NULL_HANDLE)
    {
        LLGLSLShader::populateAndBindUniversalDescriptorSet();
        set_to_bind = LLGLSLShader::sCurPerCallVkDescriptorSet;
    }
    if (set_to_bind == VK_NULL_HANDLE)
    {
        return false;
    }
    VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
    if (cmd == VK_NULL_HANDLE)
    {
        return false;
    }
    VkPipeline pipeline = shader->getOrCreateVkPipelineForBoundRT(LLRender::TRIANGLES);
    if (pipeline == VK_NULL_HANDLE)
    {
        static std::set<std::string> s_mdi_pipe_fail;
        if (s_mdi_pipe_fail.insert(shader->mName).second)
        {
            LL_WARNS("Vulkan") << "pushIndirectSpans pipeline NULL shader='" << shader->mName
                               << "' = 個別要 fix" << LL_ENDL;
        }
        return false;
    }
    if (!LLVKLoader::isInRenderPassScope())
    {
        LLRenderTarget* bound_rt = LLRenderTarget::getCurrentBoundTarget();
        if (bound_rt == nullptr)
        {
            LLVKLoader::beginSwapchainRendering();
        }
        else
        {
            bound_rt->resumeVkDynamicRendering();
        }
    }
    LLVKLoader::bindGraphicsPipelineOnce(cmd, pipeline);
    {
        const bool vk_screen_space_copy = LLGLSLShader::vkUsePositiveViewport(
            LLRenderTarget::getCurrentBoundTarget() != nullptr,
            LLGLSLShader::vkCaptureRegimeActive());
        LLVKLoader::setupViewportAndScissor(cmd, vk_screen_space_copy);
    }
    LLVKLoader::bindDrawDescriptorSetsOnce(cmd,
                                           shader->mVkPipelineLayout,
                                           LLVKLoader::getCurrentPerFrameDescriptorSet(),
                                           set_to_bind,
                                           shader->mVkSet1DynamicCount,
                                           LLGLSLShader::sCurPerCallVkDynamicOffsets);
    LLVKLoader::pushModelviewOnce(cmd,
                                  shader->mVkPipelineLayout,
                                  LLVKLoader::getCurrentModelviewMatrix());
    for (const LLVKBucket::TplChunkSpan& span : bucket.mTplChunkSpans)
    {
        span.mRep->mVertexBuffer->setBuffer();
        vkCmdDrawIndexedIndirect(cmd, ring_buf,
                                 ring_offset + (VkDeviceSize)span.mFirst * sizeof(VkDrawIndexedIndirectCommand),
                                 span.mCount,
                                 sizeof(VkDrawIndexedIndirectCommand));
        ++LLVKLoader::gVkPerf.mdi_call;
    }
    return true;
}

void LLRenderPass::pushIndirectBucket(LLVKBucket::Bucket& bucket, const std::vector<U64>& vis_bits, bool textured)
{
    LLVKBucket::rebuildTemplateIfDirty(bucket);
    if (bucket.mTplCommands.empty() && bucket.mTplDyn.empty())
    {
        return;
    }

    auto id_visible = [&](U32 id) -> bool
    {
        return id != LLVKBucket::INVALID_GROUP_ID
            && (id >> 6) < vis_bits.size()
            && (vis_bits[id >> 6] & (1ULL << (id & 63))) != 0;
    };

    bool any_visible = false;
    for (const LLVKBucket::Range& range : bucket.mRanges)
    {
        if (!range.mRecords.empty() && id_visible(range.mGroupId))
        {
            any_visible = true;
            break;
        }
    }
    if (!any_visible)
    {
        return;
    }

    const size_t n = bucket.mTplCommands.size();
    if (n > 0 && bucket.mRegion != nullptr)
    {
        const F32 cull_radius = sShadowBatchCullRadius;
        VkBuffer     ring_buf    = VK_NULL_HANDLE;
        VkDeviceSize ring_offset = 0;
        void*        ring_mapped = nullptr;
        if (LLVKLoader::indirectRingAlloc((U32)n, ring_buf, ring_offset, ring_mapped))
        {
            applyModelMatrix(&bucket.mRegion->mRenderMatrix);
            gGL.syncMatrices();
            VkDrawIndexedIndirectCommand* cmds = (VkDrawIndexedIndirectCommand*)ring_mapped;
            std::memcpy(cmds, bucket.mTplCommands.data(),
                        n * sizeof(VkDrawIndexedIndirectCommand));
            U64 zeroed = 0;
            for (size_t c = 0; c < n; ++c)
            {
                bool vis = id_visible(bucket.mTplGroupIds[c]);
                if (vis && cull_radius > 0.f)
                {
                    const F32 r = bucket.mTplRadius[c];
                    vis = !(r >= 0.f && r < cull_radius);
                }
                if (!vis)
                {
                    cmds[c].instanceCount = 0;
                    ++zeroed;
                }
            }
            LLVKLoader::gVkPerf.mdi_zero += zeroed;
            if (pushIndirectSpans(bucket, ring_buf, ring_offset))
            {
                LLVKLoader::gVkPerf.mdi_rec += (U64)n - zeroed;
            }
        }
        else
        {
            for (size_t c = 0; c < n; ++c)
            {
                if (id_visible(bucket.mTplGroupIds[c]))
                {
                    if (textured)
                    {
                        pushBatch(*bucket.mTplRecords[c], true, true);
                    }
                    else
                    {
                        pushUntexturedBatch(*bucket.mTplRecords[c]);
                    }
                }
            }
        }
    }

    for (size_t d = 0; d < bucket.mTplDyn.size(); ++d)
    {
        if (id_visible(bucket.mTplDynGroupIds[d]))
        {
            if (textured)
            {
                pushBatch(*bucket.mTplDyn[d], true, true);
            }
            else
            {
                pushUntexturedBatch(*bucket.mTplDyn[d]);
            }
            ++LLVKLoader::gVkPerf.mdi_dyn;
        }
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

            if (uploadMatrixPalette(pparams->mAvatar, pparams->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
            {
                ++LLVKLoader::gVkPerf.rigged_rec;
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

        if (uploadMatrixPalette(pparams->mAvatar, pparams->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
        {
            ++LLVKLoader::gVkPerf.rigged_rec;
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
        LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(pparams->mAlphaMaskCutoff);
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

        LLGLSLShader::sCurBoundShaderPtr->setMinimumAlpha(pparams->mAlphaMaskCutoff);

        if (uploadMatrixPalette(pparams->mAvatar, pparams->mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
        {
            ++LLVKLoader::gVkPerf.rigged_rec;
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

    if (!params.mCount || vkShadowCullBatch(params))
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

        LLRenderPass::buildAndOverrideScenePerDrawSet(&params, batch_textures);
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

    if (!params.mCount || vkShadowCullBatch(params))
    {
        return;
    }

    if (LLVKLoader::isRecordJobActive()
        && (params.mVertexBuffer == nullptr || params.mVertexBuffer->isMapped()))
    {
        static std::atomic<U32> s_worker_vb_skips{0};
        const U32 n = ++s_worker_vb_skips;
        if ((n & (n - 1)) == 0)
        {
            LL_WARNS("Vulkan") << "record job skipped batch (vb "
                               << (params.mVertexBuffer == nullptr ? "null" : "mapped")
                               << ") count=" << n << LL_ENDL;
        }
        return;
    }

    applyModelMatrix(params);

    params.mVertexBuffer->setBuffer();
    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
}

bool LLRenderPass::uploadMatrixPalette(LLDrawInfo& params)
{
    // upload matrix palette to shader
    return uploadMatrixPalette(params.mAvatar, params.mSkinInfo);
}

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

    writeObjectSkinUBO(*LLGLSLShader::sCurBoundShaderPtr, (F32*)&(mpc.mGLMp[0]), count);

    return true;
}

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
        writeObjectSkinUBO(*LLGLSLShader::sCurBoundShaderPtr, (F32*)&(mpc.mGLMp[0]), count);
    }

    return !skipLastSkin;
}

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
        writeObjectSkinUBO(*LLGLSLShader::sCurBoundShaderPtr, (F32*)&(mpc.mGLMp[0]), count);
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

    LLVKBucket::forEachSource(type, [&](LLDrawInfo& params)
    {
        if (!params.mVertexBuffer.notNull())
        {
            return;
        }

        if (params.mAttachedToAvatar.notNull() &&
            (params.mAttachedToAvatar->isSelf() ? !self_blur : !others_blur))
        {
            return;
        }

        LLGLDisable cull_face(params.mGLTFMaterial && params.mGLTFMaterial->mDoubleSided ? GL_CULL_FACE : 0);

        applyModelMatrix(params);

        const LLMatrix4* last_mat = params.mLastModelMatrix ? params.mLastModelMatrix : &identity;
        if (LLVKLoader::isVulkanInitialized()
            && LLGLSLShader::sCurBoundShaderPtr
            && LLGLSLShader::sCurBoundShaderPtr->mVkPipelineLayout != VK_NULL_HANDLE
            && LLGLSLShader::sCurBoundShaderPtr->mVkVertexPushConstantOver64)
        {
            VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
            if (cmd != VK_NULL_HANDLE)
            {
                vkCmdPushConstants(cmd, LLGLSLShader::sCurBoundShaderPtr->mVkPipelineLayout,
                                   VK_SHADER_STAGE_VERTEX_BIT, LLVkUboReg::PC_OFF_LAST_OBJECT_MATRIX, sizeof(F32) * 16,
                                   (const F32*)last_mat->mMatrix);
            }
        }

        params.mVertexBuffer->setBuffer();
        params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);

        const LLMatrix4* current_mat = params.mModelMatrix ? params.mModelMatrix : &identity;
        if (params.mLastModelMatrix)
        {
            *params.mLastModelMatrix = *current_mat;
        }
    });
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
        if (LLVKLoader::isVulkanInitialized()
            && LLGLSLShader::sCurBoundShaderPtr
            && LLGLSLShader::sCurBoundShaderPtr->mVkPipelineLayout != VK_NULL_HANDLE
            && LLGLSLShader::sCurBoundShaderPtr->mVkVertexPushConstantOver64)
        {
            VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
            if (cmd != VK_NULL_HANDLE)
            {
                vkCmdPushConstants(cmd, LLGLSLShader::sCurBoundShaderPtr->mVkPipelineLayout,
                                   VK_SHADER_STAGE_VERTEX_BIT, LLVkUboReg::PC_OFF_LAST_OBJECT_MATRIX, sizeof(F32) * 16,
                                   (const F32*)last_mat->mMatrix);
            }
        }

        LLRenderPass::buildAndOverrideScenePerDrawSet(&params, false);

        params.mVertexBuffer->setBuffer();
        params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
        LLGLSLShader::sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;

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

        LLRenderPass::buildAndOverrideScenePerDrawSet(&params, false);

        params.mVertexBuffer->setBuffer();
        params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
        LLGLSLShader::sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
    }
}

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

    writeObjectSkinLastUBO((const F32*)&(src[0]), count);

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

void LLRenderPass::pushGLTFBatch(LLDrawInfo& params)
{
    if (vkShadowCullBatch(params))
    {
        return;
    }
    auto& mat = params.mGLTFMaterial;

    if (mat.notNull())
    {
        mat->bind(params.mTexture);
    }

    LLGLDisable cull_face(mat.notNull() && mat->mDoubleSided ? GL_CULL_FACE : 0);

    setup_texture_matrix(params);

    applyModelMatrix(params);

    LLGLSLShader* cur = LLGLSLShader::sCurBoundShaderPtr;
    if (cur)
    {
        if (cur->hasReflectedUniform(LLShaderMgr::AYA_SSS_SKIN_FLAG))
        {
            const F32 sssFlag = params.mIsSSSTarget ? 1.f : 0.f;
            if (LLVKLoader::isVulkanInitialized() && cur->mVkPipelineLayout != VK_NULL_HANDLE)
            {
                VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
                if (cmd != VK_NULL_HANDLE)
                {
                    vkCmdPushConstants(cmd, cur->mVkPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                                       LLVkUboReg::PC_OFF_SSS_SKIN_FLAG, sizeof(F32), &sssFlag);
                }
            }
        }
    }
    // </FS:AYA>

    params.mVertexBuffer->setBuffer();

    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);

    teardown_texture_matrix(params);
}

void LLRenderPass::pushUntexturedGLTFBatch(LLDrawInfo& params)
{
    if (vkShadowCullBatch(params))
    {
        return;
    }

    if (LLVKLoader::isRecordJobActive()
        && (params.mVertexBuffer == nullptr || params.mVertexBuffer->isMapped()))
    {
        return;
    }

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


void LLRenderPass::pushRiggedGLTFBatch(LLDrawInfo& params, const LLVOAvatar*& lastAvatar, U64& lastMeshId, bool& skipLastSkin)
{
    if (uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
    {
        pushGLTFBatch(params);
    }
}

void LLRenderPass::pushUntexturedRiggedGLTFBatch(LLDrawInfo& params, const LLVOAvatar*& lastAvatar, U64& lastMeshId, bool& skipLastSkin)
{
    if (uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, skipLastSkin))
    {
        pushUntexturedGLTFBatch(params);
    }
}