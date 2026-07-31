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

#include <algorithm>
#include <atomic>
#include <mutex>
#include <set>
#include <tuple>
#include <utility>

#include "lldrawpool.h"
#include "llrender.h"
#include "glm/gtc/type_ptr.hpp"
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
#include "llvkcontract.h"
#include "llvkuboreg.h"
#include "llimagegl.h"
#include "llpipelineframecontext.h"

S32 LLDrawPool::sNumDrawPools = 0;

static std::string vkContractDescribeDrawInfo(const void* p)
{
    const LLDrawInfo* di = static_cast<const LLDrawInfo*>(p);
    if (di == nullptr)
    {
        return std::string();
    }
    std::ostringstream os;
    os << "obj=" << di->mFSPickerLocalID;
    if (di->mTexture.notNull())
    {
        os << " tex=" << di->mTexture->getID();
    }
    if (di->mAvatar.notNull())
    {
        os << " av='" << di->mAvatar->getFullname() << "'";
    }
    else if (di->mAttachedToAvatar.notNull())
    {
        os << " wearer='" << di->mAttachedToAvatar->getFullname() << "'";
    }
    if (di->mMaterialID.notNull())
    {
        os << " mat=" << di->mMaterialID;
    }
    os << " idx=" << di->mCount;
    return os.str();
}

static U64 vkContractDrawInfoKey(const void* p)
{
    const LLDrawInfo* di = static_cast<const LLDrawInfo*>(p);
    if (di == nullptr)
    {
        return 0;
    }
    U64 k = 1469598103934665603ull;
    k = k * 0x100000001B3ull ^ (U64)di->mFSPickerLocalID;
    k = k * 0x100000001B3ull ^ (U64)(uintptr_t)di->mTexture.get();
    k = k * 0x100000001B3ull ^ (U64)di->mCount;
    return (k != 0) ? k : 1;
}

extern bool gCubeSnapshot;
extern bool gHeroProbeMirrorRender;
extern bool gSnapshot;

static U32 vkContractObjId(const void* p)
{
    const LLDrawInfo* di = static_cast<const LLDrawInfo*>(p);
    return di != nullptr ? (U32)di->mFSPickerLocalID : 0u;
}

static U32 vkContractPassBucket()
{
    return (gCubeSnapshot || gHeroProbeMirrorRender) ? 3u : LLVKLoader::gVkPerfPassTag;
}

struct VkContractResolverInit
{
    VkContractResolverInit()
    {
        LLVKContract::setResolvers(&vkContractDescribeDrawInfo, &vkContractDrawInfoKey);
        LLVKContract::setObjIdResolver(&vkContractObjId);
        LLVKContract::setPassBucketResolver(&vkContractPassBucket);
    }
};
static VkContractResolverInit sVkContractResolverInit;

static U32 e3RigBucket()
{
    if (gCubeSnapshot || gHeroProbeMirrorRender)
    {
        return 2u;
    }
    const U32 tag = LLVKLoader::gVkPerfPassTag;
    return tag == 0u ? 0u : (tag == 1u ? 1u : 2u);
}

namespace
{
struct E3PalTimer
{
    U64  mT0;
    bool mOn;
    E3PalTimer() : mT0(0), mOn(LLVKLoader::perfLogEnabled())
    {
        if (mOn)
        {
            mT0 = (U64)LLTimer::getTotalTime();
        }
    }
    ~E3PalTimer()
    {
        if (mOn)
        {
            LLVKLoader::gVkPerf.e3_pal_us += (U64)LLTimer::getTotalTime() - mT0;
        }
    }
};
}

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
            ++LLVKLoader::gVkPerf.shadow_rigged_map[LLVKLoader::gVkPerfShadowMapIndex < 6u
                                                        ? LLVKLoader::gVkPerfShadowMapIndex : 5u];
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
    group->mVkLastFireFrame = gFrameCount;
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
    const bool e3on = LLVKLoader::perfLogEnabled();
    const U64  e3t0 = e3on ? (U64)LLTimer::getTotalTime() : 0;
    group->mVkLastFireFrame = gFrameCount;
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
    if (e3on)
    {
        LLVKLoader::gVkPerf.e3_rig_us[e3RigBucket()] += (U64)LLTimer::getTotalTime() - e3t0;
    }
}

void LLRenderPass::buildAndOverrideScenePerDrawSet(LLDrawInfo* params, bool batch_textures,
                                                    U64 gltf_materials_ubo,
                                                    U32 gltf_materials_size,
                                                    U64 gltf_geometry_ubo,
                                                    U32 gltf_geometry_size)
{
    LLVKLoader::gVkPerfSetPath  = LLVKLoader::VKPERF_SETPATH_EARLY;
    LLVKLoader::gVkPerfSetCause = LLVKLoader::VKPERF_SETCZ_NONE;
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

    LLGLSLShader::sCurPerCallAuthored = false;

    const bool is_indexed = (cur->mFeatures.mIndexedTextureChannels > 0) && !cur->mVkUsesHeapSet;
    const U32 indexed_layout_count =
        llmin((U32)cur->mFeatures.mIndexedTextureChannels,
              (U32)LLVKLoader::ScenePerDrawBindings::MAX_SAMPLERS);
    const U32 set_shape = (params != nullptr && batch_textures && params->mTextureList.size() > 1 && is_indexed)
                              ? llmin((U32)params->mTextureList.size(), indexed_layout_count)
                              : (is_indexed ? 1u : 0u);

    if (cur->mVkUsesHeapSet || cur->mVkUsesSkinSet)
    {
        U32 slots[LLVKLoader::DRAWDATA_SLOT_UINTS] = {};
        if (cur->mVkUsesHeapSet)
        {
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
                if (params->mNormalMap.notNull())
                {
                    slots[1] = LLImageGL::vkHeapSlotOrDefault(params->mNormalMap->getGLTexture());
                }
                if (params->mSpecularMap.notNull())
                {
                    slots[2] = LLImageGL::vkHeapSlotOrDefault(params->mSpecularMap->getGLTexture());
                }
            }
            else
            {
                slots[0] = gGL.getTexUnit(0)->currVkHeapSlotOrDefault();
            }
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
        const void* skin_avatar = (params != nullptr && params->mAvatar.notNull() && params->mSkinInfo != nullptr)
                                      ? (const void*)params->mAvatar.get() : nullptr;
        const U64   skin_hash   = (skin_avatar != nullptr) ? params->mSkinInfo->mHash : 0;
        LLVKLoader::commitPerDrawID(id, cur->mVkUsesSkinSet, skin_avatar, skin_hash);
    }

    const bool memo_eligible = (params != nullptr && is_indexed && set_shape >= 1
                                && gltf_materials_ubo == 0 && gltf_geometry_ubo == 0);
    const U32  memo_frame    = LLVKLoader::getCurrentFrameIndex();
    const U32  lane          = LLVKLoader::getCurrentRecordLane();

    if (cur->mVkUsesHeapSet)
    {
        if (gltf_materials_ubo != 0 || gltf_geometry_ubo != 0)
        {
            LLVKLoader::gVkPerfSetCause = LLVKLoader::VKPERF_SETCZ_GLTF;
        }
        else if (!cur->mVkAccessorBindingListBuiltLanes[lane])
        {
            LLVKLoader::gVkPerfSetCause = LLVKLoader::VKPERF_SETCZ_LANE;
        }
        else
        {
            LLVKLoader::PerDrawCacheLane& bl = cur->mVkPerDrawLane[lane];
            if (bl.set[memo_frame] != VK_NULL_HANDLE
                && bl.ev[memo_frame].shape == set_shape
                && LLGLSLShader::vkValidatePerDrawSlot(cur, bl.ev[memo_frame]))
            {
                LLGLSLShader::sCurPerCallVkDescriptorSet = bl.set[memo_frame];
                LLGLSLShader::sCurPerCallVkOffsetsDirty  = true;
                LLGLSLShader::sCurPerCallVkSetShape      = set_shape;
                LLGLSLShader::sCurPerCallAuthored        = true;
                ++LLVKLoader::gVkPerf.set_memo;
                LLVKLoader::gVkPerfSetPath = LLVKLoader::VKPERF_SETPATH_BINDLESS;
                return;
            }
            LLVKLoader::gVkPerfSetCause = LLVKLoader::VKPERF_SETCZ_VAL;
        }
    }
    else if (memo_eligible && cur->mVkAccessorBindingListBuiltLanes[lane])
    {
        LLVKLoader::PerDrawCacheLane& mc = params->mVkPerDrawCache;
        if (mc.set[memo_frame] != VK_NULL_HANDLE
            && mc.ev[memo_frame].shape == set_shape
            && LLGLSLShader::vkValidatePerDrawSlot(cur, mc.ev[memo_frame]))
        {
            LLGLSLShader::sCurPerCallVkDescriptorSet = mc.set[memo_frame];
            LLGLSLShader::sCurPerCallVkOffsetsDirty  = true;
            LLGLSLShader::sCurPerCallVkSetShape      = set_shape;
            LLGLSLShader::sCurPerCallAuthored        = true;
            ++LLVKLoader::gVkPerf.set_memo;
            LLVKLoader::gVkPerfSetPath = LLVKLoader::VKPERF_SETPATH_MEMO;
            return;
        }
        LLVKLoader::gVkPerfSetCause = LLVKLoader::VKPERF_SETCZ_MVAL;
    }

    LLVKLoader::gVkPerfSetPath = LLVKLoader::VKPERF_SETPATH_BUILD;
    const bool sb_on = LLVKLoader::perfLogEnabled();
    U64 sb_t = sb_on ? (U64)LLTimer::getTotalTime() : 0;

    LLVKLoader::PerDrawEvidence ev;
    ev.shader = cur;
    ev.shape  = set_shape;
    bool can_pin = memo_eligible || (cur->mVkUsesHeapSet
                                     && gltf_materials_ubo == 0 && gltf_geometry_ubo == 0);
    auto record_ref = [&](S16 source, VkImageView view)
    {
        if (!can_pin)
        {
            return;
        }
        if (ev.refCount >= LLVKLoader::PDC_MAX_REFS)
        {
            can_pin = false;
            return;
        }
        ev.refSource[ev.refCount] = source;
        ev.refView[ev.refCount]   = (void*)view;
        ++ev.refCount;
    };
    U64  memo_ring_sig = 0;
    const bool build_accessor_list = !cur->mVkAccessorBindingListBuiltLanes[lane];

    LLVKLoader::ScenePerDrawBindings bindings;
    bindings.layout       = cur->mVkDescriptorSetLayout;
    bindings.sampler      = sampler;
    bindings.dynamic_mask = cur->mVkDynamicBindingMask;
    bindings.layout_binding_mask = cur->mVkSet1LayoutBindingMask;

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
            if (i < real_count && view_to_write != fallback_view)
            {
                record_ref((S16)(-(S32)i - 2), view_to_write);
            }
            bindings.sampler_bindings[i] = 100 + i;
            bindings.sampler_views[i]    = view_to_write;
            bindings.sampler_sources[i]  = 'I';
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
            if (i == 0 && view_to_write != fallback_view)
            {
                record_ref((S16)(-2), view_to_write);
            }
            bindings.sampler_bindings[i] = 100 + i;
            bindings.sampler_views[i]    = view_to_write;
            bindings.sampler_sources[i]  = 'I';
            bindings.sampler_samplers[i] = (i == 0)
                                               ? gGL.getTexUnit(0)->getLiveVkSampler()
                                               : VK_NULL_HANDLE;
        }
        bindings.sampler_count = indexed_layout_count;
    }
    else if (((cur->mVkSet1LayoutBindingMask >> 1) & 1) != 0)
    {
        if (cur->mVkUsesHeapSet)
        {
            bindings.sampler_bindings[0] = 1;
            bindings.sampler_views[0]    = fallback_view;
            bindings.sampler_sources[0]  = '1';
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
            if (view_to_write != fallback_view)
            {
                record_ref(l3_hit ? (S16)enum1 : (S16)(-(S32)unit1 - 2), view_to_write);
            }
            bindings.sampler_bindings[0] = 1;
            bindings.sampler_views[0]    = view_to_write;
            bindings.sampler_sources[0]  = '1';
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
        }
        else if (channel >= 0)
        {
            resolved_unit = channel;
            view = gGL.getTexUnit((S32)channel)->getLiveVkImageView();
            LLGLSLShader::vkWarnL3Fallback(cur, N, enum_value, view);
        }
        else if (enum_value >= 0 && enum_value < (S32)cur->mTexture.size())
        {
            S32 unit = cur->mTexture[enum_value];
            if (unit >= 0)
            {
                resolved_unit = unit;
                view = gGL.getTexUnit((S32)unit)->getLiveVkImageView();
                LLGLSLShader::vkWarnL3Fallback(cur, N, enum_value, view);
            }
        }
        if (l3_hit)
        {
            record_ref((S16)enum_value, view);
        }
        else if (resolved_unit >= 0)
        {
            record_ref((S16)(-(S32)resolved_unit - 2), view);
        }

        const char* vkc_fb_reason = nullptr;
        bool need_typed_fallback = (view == VK_NULL_HANDLE);
        if (need_typed_fallback)
        {
            vkc_fb_reason = "no_view";
        }
        if (!need_typed_fallback)
        {
            if (l3_hit)
            {
                if (cur->vkResolveEnumBoundDim(enum_value) != cur->mVkBindingSamplerDim[N])
                {
                    need_typed_fallback = true;
                    vkc_fb_reason = "dim_l3";
                }
            }
            else if (resolved_unit >= 0)
            {
                LLTexUnit* dim_tu = gGL.getTexUnit(resolved_unit);
                if (dim_tu != nullptr && dim_tu->getLiveVkImageViewDim() != cur->mVkBindingSamplerDim[N])
                {
                    need_typed_fallback = true;
                    vkc_fb_reason = "dim_unit";
                }
            }
        }
        if (need_typed_fallback)
        {
            LLVKContract::note(resolved_unit == 0 ? LLVKContract::C_FB_VIEW_DIFFUSE
                                                  : LLVKContract::C_FB_VIEW_AUX,
                               cur->mName);
            LLVKContract::watchFbProbe(resolved_unit == 0, vkc_fb_reason);
            LLVKContract::noteFbSlot(cur, cur->mName, N, vkc_fb_reason);
            const U8 sdim_fb = cur->mVkBindingSamplerDim[N];
            view = cur->mVkBindingSamplerShadow[N] ? LLVKLoader::getDefaultFallbackShadowVkImageView()
                 : (sdim_fb == LLGLSLShader::VKSD_CUBE_ARRAY) ? LLVKLoader::getDefaultFallbackCubeArrayVkImageView()
                 : (sdim_fb == LLGLSLShader::VKSD_CUBE)       ? LLVKLoader::getDefaultFallbackCubeVkImageView()
                 : (sdim_fb == LLGLSLShader::VKSD_3D)         ? LLVKLoader::getDefaultFallback3DVkImageView()
                 :                                              fallback_view;
        }

        bindings.sampler_bindings[bindings.sampler_count] = N;
        bindings.sampler_views[bindings.sampler_count]    = view;
        bindings.sampler_sources[bindings.sampler_count]  = need_typed_fallback
                                                              ? (l3_hit ? 'E'
                                                                 : (channel >= 0 || resolved_unit >= 0) ? 'R' : 'F')
                                                          : l3_hit              ? 'L'
                                                          : (channel >= 0)      ? 'C'
                                                          : (resolved_unit >= 0) ? 'T' : 'N';
        bindings.sampler_samplers[bindings.sampler_count] = l3_hit
                                                              ? cur->mVkEnumBoundView[enum_value].sampler
                                                              : (resolved_unit >= 0)
                                                                  ? gGL.getTexUnit(resolved_unit)->getLiveVkSampler()
                                                                  : VK_NULL_HANDLE;
        if (need_typed_fallback && cur->mVkBindingSamplerShadow[N])
        {
            bindings.sampler_samplers[bindings.sampler_count] =
                LLVKLoader::getSamplerForState((U32)LLTexUnit::TAM_CLAMP,
                                               (U32)LLTexUnit::TFO_BILINEAR,
                                               false, true);
        }

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
                const bool declared_ubo = (cur->mVkBindingDeclaredType[N] & LLGLSLShader::VKBD_UBO) != 0;
                if (!declared_ubo)
                {
                    continue;
                }
                void* mapped = nullptr;
                if (accessor(ubo_buf, mapped))
                {
                    ubo_sz = cur->sharedUBOBindingSize(N);
                }
                if (build_accessor_list && N < 256)
                {
                    cur->mVkAccessorBindingListLanes[lane].push_back((U8)N);
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
        cur->mVkAccessorBindingListBuiltLanes[lane] = true;
        if (lane == 0 && !cur->mVkSigListLogged && LLVKLoader::perfLogEnabled())
        {
            cur->mVkSigListLogged = true;
            std::string sig_list;
            for (U8 b : cur->mVkAccessorBindingListLanes[lane])
            {
                if (!sig_list.empty()) sig_list += ' ';
                sig_list += std::to_string((U32)b);
            }
            LL_INFOS() << "SIGLIST " << cur->mName << " = [" << sig_list << "]" << LL_ENDL;
        }
    }

    if (sb_on) { U64 t2 = (U64)LLTimer::getTotalTime(); LLVKLoader::gVkPerf.setb_us[0] += t2 - sb_t; sb_t = t2; }

    U32 dyn_offsets[LLGLSLShader::MAX_VK_DYNAMIC_BINDINGS] = {};
    if (!LLGLSLShader::vkCollectDynamicUBOWrites(cur, bindings, per_program_dynamic_offset, dyn_offsets))
    {
        return;
    }

    if (sb_on) { U64 t2 = (U64)LLTimer::getTotalTime(); LLVKLoader::gVkPerf.setb_us[1] += t2 - sb_t; sb_t = t2; }

    VkDescriptorSet per_draw_set = VK_NULL_HANDLE;
    void*           memo_token   = nullptr;
    const bool ens_ok = LLVKLoader::ensureScenePerDrawDescriptorSet(bindings, &per_draw_set,
                                                    (memo_eligible || cur->mVkUsesHeapSet) ? &memo_token : nullptr)
        && per_draw_set != VK_NULL_HANDLE;

    if (sb_on) { U64 t2 = (U64)LLTimer::getTotalTime(); LLVKLoader::gVkPerf.setb_us[2] += t2 - sb_t; sb_t = t2; }

    if (ens_ok)
    {
        LLGLSLShader::sCurPerCallVkDescriptorSet = per_draw_set;
        std::memcpy(LLGLSLShader::sCurPerCallVkDynamicOffsets, dyn_offsets, sizeof(dyn_offsets));
        LLGLSLShader::sCurPerCallVkOffsetsDirty = false;
        LLGLSLShader::sCurPerCallVkSetShape = (gltf_materials_ubo == 0 && gltf_geometry_ubo == 0)
                                                  ? set_shape
                                                  : 0xFFFFFFFFu;
        LLGLSLShader::sCurPerCallAuthored = true;
        ++LLVKLoader::gVkPerf.set_build;

        LLVKLoader::PerDrawCacheLane* home =
            cur->mVkUsesHeapSet ? &cur->mVkPerDrawLane[lane]
            : memo_eligible          ? &params->mVkPerDrawCache
            :                          nullptr;
        const bool ubo_cacheable = (bindings.ubo == VK_NULL_HANDLE
                                    || bindings.ubo == LLVKLoader::getPerDrawUBOArenaBuffer());
        if (home != nullptr && can_pin && memo_token != nullptr && ubo_cacheable)
        {
            ev.reloadEpoch   = LLVKLoader::gVkReloadEpoch.load(std::memory_order_relaxed);
            ev.topoGen       = LLVKLoader::gVkPerDrawTopologyGen.load(std::memory_order_relaxed);
            ev.attachmentSig = LLVKLoader::currentPassAttachmentSig();
            ev.ringSig       = memo_ring_sig;
            ev.pinnable      = true;
            LLGLSLShader::vkPinPerDrawSlot(*home, memo_frame, per_draw_set, memo_token, ev);
            ++LLVKLoader::gVkPerf.pin_store;
            ++LLVKLoader::gVkPerf.set_memo_fill;
        }
        else if (home != nullptr)
        {
            if (home->tok[memo_frame] != nullptr)
            {
                LLVKLoader::releaseScenePerDrawEntry(home->tok[memo_frame], home->pinEpoch[memo_frame]);
                home->tok[memo_frame] = nullptr;
            }
            home->set[memo_frame] = VK_NULL_HANDLE;
        }
    }

    if (sb_on) { LLVKLoader::gVkPerf.setb_us[3] += (U64)LLTimer::getTotalTime() - sb_t; }
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
            && LLGLSLShader::sCurBoundShaderPtr->mVkUsesHeapSet
            && !gSnapshot)
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
        && !gSnapshot)
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
    LLVKContract::DrawScope vkc_scope(nullptr, "mdi");
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (!LLVKLoader::beginShaderDrawOrSkip(shader, LLRender::TRIANGLES, cmd))
    {
        return false;
    }
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
                const bool gvis = id_visible(bucket.mTplGroupIds[c]);
                bool vis = gvis;
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

namespace
{
    bool riggedMdiKillSwitchOn()
    {
        static const bool s_on = []()
        {
            const char* e = getenv("AYASTORM_RIGGED_MDI");
            return e != nullptr && e[0] == '1';
        }();
        return s_on;
    }

    bool riggedMdiEligible(U32 type)
    {
        (void)type;
        return riggedMdiKillSwitchOn()
            && LLPipelineFrameContext::getInstance().isShadowPass()
            && LLVKLoader::isIndirectDrawEnabled()
            && LLVKLoader::skinBindlessEnabled()
            && LLGLSLShader::sCurBoundShaderPtr != nullptr
            && LLGLSLShader::sCurBoundShaderPtr->mVkUsesHeapSet
            && !gSnapshot;
    }

    struct RiggedMdiRec
    {
        VkBuffer        vbuf  = VK_NULL_HANDLE;
        VkBuffer        ibuf  = VK_NULL_HANDLE;
        U32             itype = 0;
        LLVertexBuffer* rep   = nullptr;
        VkDrawIndexedIndirectCommand cmd{};
    };

    bool pushRiggedIndirectSpans(LLGLSLShader* shader,
                                 VkBuffer ring_buf, VkDeviceSize ring_offset,
                                 const std::vector<RiggedMdiRec>& items,
                                 const std::vector<std::pair<U32, U32> >& spans)
    {
        LLVKContract::DrawScope vkc_scope(nullptr, "mdi_rig");
        VkCommandBuffer cmd = VK_NULL_HANDLE;
        if (!LLVKLoader::beginShaderDrawOrSkip(shader, LLRender::TRIANGLES, cmd))
        {
            return false;
        }
        for (const std::pair<U32, U32>& span : spans)
        {
            items[span.first].rep->setBuffer();
            vkCmdDrawIndexedIndirect(cmd, ring_buf,
                                     ring_offset + (VkDeviceSize)span.first * sizeof(VkDrawIndexedIndirectCommand),
                                     span.second,
                                     sizeof(VkDrawIndexedIndirectCommand));
            ++LLVKLoader::gVkPerf.mdi_call;
        }
        return true;
    }

    bool pushRiggedBatchesIndirect(U32 type, bool batch_textures)
    {
        LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;

        static thread_local std::vector<RiggedMdiRec> s_items;
        s_items.clear();

        const LLVOAvatar* lastAvatar    = nullptr;
        U64               lastMeshId    = 0;
        bool              skipLastSkin  = false;
        auto*             begin         = gPipeline.beginRenderMap(type);
        auto*             end           = gPipeline.endRenderMap(type);
        for (LLCullResult::drawinfo_iterator i = begin; i != end; )
        {
            LLDrawInfo* p = *i;
            LLCullResult::increment_iterator(i, end);

            const bool refreshed = (p->mVkSkinFrame == gFrameCount);
            if (!refreshed
                && !LLRenderPass::uploadMatrixPalette(p->mAvatar, p->mSkinInfo,
                                                      lastAvatar, lastMeshId, skipLastSkin))
            {
                continue;
            }
            ++LLVKLoader::gVkPerf.rigged_rec;
            if (!p->mCount || vkShadowCullBatch(*p))
            {
                continue;
            }
            LLVertexBuffer* vb = p->mVertexBuffer.get();
            if (vb == nullptr)
            {
                continue;
            }
            const LLVKLoader::MegaSliceV& vs = vb->getVkVertexSlice();
            const LLVKLoader::MegaSliceI& is = vb->getVkIndexSlice();
            if (vs.buffer == VK_NULL_HANDLE || is.buffer == VK_NULL_HANDLE)
            {
                continue;
            }

            U32 draw_id;
            if (refreshed)
            {
                draw_id = (p->mVkDrawDataSlot == LLVKLoader::BINDLESS_INVALID_SLOT)
                              ? 0 : p->mVkDrawDataSlot;
            }
            else
            {
                U32 slots[LLVKLoader::DRAWDATA_SLOT_UINTS] = {};
                if (batch_textures && p->mTextureList.size() > 1)
                {
                    const U32 n = llmin((U32)p->mTextureList.size(), 4u);
                    for (U32 s = 0; s < n; ++s)
                    {
                        LLTexture* t = p->mTextureList[s].get();
                        slots[s] = LLImageGL::vkHeapSlotOrDefault(t ? t->getGLTexture() : nullptr);
                    }
                }
                else if (p->mTexture.notNull())
                {
                    slots[0] = LLImageGL::vkHeapSlotOrDefault(p->mTexture->getGLTexture());
                    if (p->mNormalMap.notNull())
                    {
                        slots[1] = LLImageGL::vkHeapSlotOrDefault(p->mNormalMap->getGLTexture());
                    }
                    if (p->mSpecularMap.notNull())
                    {
                        slots[2] = LLImageGL::vkHeapSlotOrDefault(p->mSpecularMap->getGLTexture());
                    }
                }
                if (!p->ensureVkDrawDataSlot(slots))
                {
                    return false;
                }
                draw_id = (p->mVkDrawDataSlot == LLVKLoader::BINDLESS_INVALID_SLOT)
                              ? 0 : p->mVkDrawDataSlot;

                if (LLVKLoader::publishDrawSkinBase(draw_id, p->mAvatar.get(),
                                                    p->mSkinInfo->mHash)
                        == LLVKLoader::BINDLESS_INVALID_SLOT)
                {
                    return false;
                }
                p->mVkSkinFrame = gFrameCount;
            }

            RiggedMdiRec rec;
            rec.vbuf              = vs.buffer;
            rec.ibuf              = is.buffer;
            rec.itype             = vb->getIndicesType();
            rec.rep               = vb;
            rec.cmd.indexCount    = p->mCount;
            rec.cmd.instanceCount = 1;
            rec.cmd.firstIndex    = is.offset / vb->getIndicesStride() + p->mOffset;
            rec.cmd.vertexOffset  = (S32)vs.first;
            rec.cmd.firstInstance = draw_id;
            s_items.push_back(rec);
        }

        if (s_items.empty())
        {
            return false;
        }

        std::stable_sort(s_items.begin(), s_items.end(),
            [](const RiggedMdiRec& a, const RiggedMdiRec& b)
            {
                if (a.vbuf != b.vbuf) return (uintptr_t)a.vbuf < (uintptr_t)b.vbuf;
                if (a.ibuf != b.ibuf) return (uintptr_t)a.ibuf < (uintptr_t)b.ibuf;
                return a.itype < b.itype;
            });

        static thread_local std::vector<VkDrawIndexedIndirectCommand> s_cmds;
        static thread_local std::vector<std::pair<U32, U32> >          s_spans;
        s_cmds.clear();
        s_spans.clear();
        for (U32 idx = 0; idx < (U32)s_items.size(); ++idx)
        {
            const RiggedMdiRec& r = s_items[idx];
            if (s_spans.empty())
            {
                s_spans.emplace_back(idx, 0u);
            }
            else
            {
                const RiggedMdiRec& rep = s_items[s_spans.back().first];
                if (rep.vbuf != r.vbuf || rep.ibuf != r.ibuf || rep.itype != r.itype)
                {
                    s_spans.emplace_back(idx, 0u);
                }
            }
            ++s_spans.back().second;
            s_cmds.push_back(r.cmd);
        }

        VkBuffer     ring_buf    = VK_NULL_HANDLE;
        VkDeviceSize ring_offset = 0;
        void*        ring_mapped = nullptr;
        if (!LLVKLoader::indirectRingAlloc((U32)s_cmds.size(), ring_buf, ring_offset, ring_mapped))
        {
            return false;
        }
        std::memcpy(ring_mapped, s_cmds.data(),
                    s_cmds.size() * sizeof(VkDrawIndexedIndirectCommand));

        LLRenderPass::applyModelMatrix((const LLMatrix4*)nullptr);
        gGL.syncMatrices();
        LLGLSLShader::sCurPerCallAuthored = false;

        if (!pushRiggedIndirectSpans(shader, ring_buf, ring_offset, s_items, s_spans))
        {
            return false;
        }
        LLVKLoader::gVkPerf.mdi_rec += (U64)s_cmds.size();
        return true;
    }
}

void LLRenderPass::pushRiggedBatches(U32 type, bool texture, bool batch_textures)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DRAWPOOL;
    const bool e3on = LLVKLoader::perfLogEnabled();
    const U64  e3t0 = e3on ? (U64)LLTimer::getTotalTime() : 0;

    if (riggedMdiEligible(type) && pushRiggedBatchesIndirect(type, batch_textures))
    {
        if (e3on)
        {
            LLVKLoader::gVkPerf.e3_rig_us[e3RigBucket()] += (U64)LLTimer::getTotalTime() - e3t0;
        }
        return;
    }

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
    if (e3on)
    {
        LLVKLoader::gVkPerf.e3_rig_us[e3RigBucket()] += (U64)LLTimer::getTotalTime() - e3t0;
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
    const bool e3on = LLVKLoader::perfLogEnabled();
    const U64  e3t0 = e3on ? (U64)LLTimer::getTotalTime() : 0;
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
    if (e3on)
    {
        LLVKLoader::gVkPerf.e3_rig_us[e3RigBucket()] += (U64)LLTimer::getTotalTime() - e3t0;
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

void LLRenderPass::vkcVerifyDrawModelview(const LLDrawInfo& params)
{
    if (!LLVKContract::verboseEnabled())
    {
        return;
    }
    LLVKContract::vfyTick(LLVKContract::VFY_MV);
    glm::mat4 expected = glm::make_mat4(gGLModelView);
    if (params.mModelMatrix)
    {
        expected *= glm::make_mat4((const GLfloat*)params.mModelMatrix->mMatrix);
    }
    const glm::mat4& actual = gGL.getModelviewMatrix();
    const F32* e = glm::value_ptr(expected);
    const F32* a = glm::value_ptr(actual);
    F32 maxd = 0.f;
    for (U32 i = 0; i < 16; ++i)
    {
        const F32 d = fabsf(e[i] - a[i]);
        if (d > maxd)
        {
            maxd = d;
        }
    }
    if (maxd > 0.001f)
    {
        const char* tag = LLVKContract::currentDrawTag();
        LLVKContract::causeNamed(LLVKContract::C_MV_STALE_VALUE,
                                 std::string(tag != nullptr ? tag : "?") + '|'
                                     + (maxd > 1.f ? "diff_1m+"
                                                   : (maxd > 0.1f ? "diff_0.1+" : "diff_small")));
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
    LLVKContract::DrawScope vkc_scope(&params, "scene");
    params.mVertexBuffer->setBuffer();
    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
    vkcVerifyDrawModelview(params);
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

    applyModelMatrix(params);

    LLVKContract::DrawScope vkc_scope(&params, "scene");
    params.mVertexBuffer->setBuffer();
    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
    vkcVerifyDrawModelview(params);
}

bool LLRenderPass::uploadMatrixPalette(LLDrawInfo& params)
{
    // upload matrix palette to shader
    return uploadMatrixPalette(params.mAvatar, params.mSkinInfo);
}

bool LLRenderPass::uploadMatrixPalette(LLVOAvatar* avatar, const LLMeshSkinInfo* skinInfo) // <FS:Beq/> be defensive about UAF with skinInfo during LocalMesh
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
    E3PalTimer _e3pal;

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

    if (!LLVKLoader::objectSkinTryAdopt(avatar, skinInfo->mHash))
    {
        writeObjectSkinUBO(*LLGLSLShader::sCurBoundShaderPtr, (F32*)&(mpc.mGLMp[0]), count, avatar->getID());
        LLVKLoader::objectSkinStoreCache(avatar, skinInfo->mHash);
    }

    return true;
}

bool LLRenderPass::uploadMatrixPalette(LLVOAvatar* avatar, const LLMeshSkinInfo* skinInfo, const LLVOAvatar*& lastAvatar, U64& lastMeshId, bool& skipLastSkin, bool allow_dedup)// <FS:Beq/> be defensive about UAF with skinInfo during LocalMesh
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
    E3PalTimer _e3pal;

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
        if (!allow_dedup)
        {
            writeObjectSkinUBO(*LLGLSLShader::sCurBoundShaderPtr, (F32*)&(mpc.mGLMp[0]), count, avatar->getID());
        }
        else if (!LLVKLoader::objectSkinTryAdopt(avatar, skinInfo->mHash))
        {
            writeObjectSkinUBO(*LLGLSLShader::sCurBoundShaderPtr, (F32*)&(mpc.mGLMp[0]), count, avatar->getID());
            LLVKLoader::objectSkinStoreCache(avatar, skinInfo->mHash);
        }
    }

    return !skipLastSkin;
}

bool LLRenderPass::uploadMatrixPalette(LLVOAvatar* avatar, const LLMeshSkinInfo* skinInfo, const LLVOAvatar*& lastAvatar, U64& lastMeshId, const LLGLSLShader*& lastAvatarShader, bool& skipLastSkin)// <FS:Beq/> be defensive about UAF with skinInfo during LocalMesh
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
    E3PalTimer _e3pal;

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
        if (!LLVKLoader::objectSkinTryAdopt(avatar, skinInfo->mHash))
        {
            writeObjectSkinUBO(*LLGLSLShader::sCurBoundShaderPtr, (F32*)&(mpc.mGLMp[0]), count, avatar->getID());
            LLVKLoader::objectSkinStoreCache(avatar, skinInfo->mHash);
        }
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

        LLVKContract::DrawScope vkc_scope(&params, "scene");
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

        if (!uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, skipLastSkin, false))
        {
            continue;
        }

        uploadLastMatrixPalette(params.mAvatar, params.mSkinInfo);

        applyModelMatrix(params);

        LLVKContract::DrawScope vkc_scope(&params, "scene");
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

        LLVKContract::DrawScope vkc_scope(&params, "scene");
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

        if (!uploadMatrixPalette(params.mAvatar, params.mSkinInfo, lastAvatar, lastMeshId, skipLastSkin, false))
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

        LLVKContract::DrawScope vkc_scope(&params, "scene");
        params.mVertexBuffer->setBuffer();
        params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
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
            cur->vkPushFragPC(LLVkUboReg::PC_OFF_SSS_SKIN_FLAG, sizeof(F32), &sssFlag);
        }
    }
    // </FS:AYA>

    LLVKContract::DrawScope vkc_scope(&params, "scene");
    params.mVertexBuffer->setBuffer();

    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
    vkcVerifyDrawModelview(params);

    teardown_texture_matrix(params);
}

void LLRenderPass::pushUntexturedGLTFBatch(LLDrawInfo& params)
{
    if (vkShadowCullBatch(params))
    {
        return;
    }

    auto& mat = params.mGLTFMaterial;

    LLGLDisable cull_face(mat->mDoubleSided ? GL_CULL_FACE : 0);

    applyModelMatrix(params);

    LLVKContract::DrawScope vkc_scope(&params, "scene");
    params.mVertexBuffer->setBuffer();
    params.mVertexBuffer->drawRange(LLRender::TRIANGLES, params.mStart, params.mEnd, params.mCount, params.mOffset);
    vkcVerifyDrawModelview(params);
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