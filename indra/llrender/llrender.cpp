 /**
 * @file llrender.cpp
 * @brief LLRender implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "linden_common.h"

#include "llrender.h"

#include "llvertexbuffer.h"
#include "llcubemap.h"
#include "llglslshader.h"
#include "llvkcontract.h"
#include "llimagegl.h"
#include "llrendertarget.h"
#include "lltexture.h"
#include "llshadermgr.h"
#include "llvkloader.h"
#include "hbxxh.h"
#include "glm/gtc/type_ptr.hpp"
#include <atomic>
#include <cstring>
#include <mutex>
#include <set>

thread_local LLRender gGL;

thread_local F32 gGLModelView[16];
thread_local F32 gGLLastModelView[16];
F32 gGLLastProjection[16];
F32 gGLProjection[16];

glm::mat4 gGLDeltaModelView;
glm::mat4 gGLInverseDeltaModelView;

S32 gGLViewport[4];

void llSetGLViewport(S32 x, S32 y, S32 w, S32 h)
{
    LLVKLoader::setRenderViewport(x, y, w, h);
}


U32 LLRender::sUICalls = 0;
U32 LLRender::sUIVerts = 0;
bool LLRender::sNsightDebugSupport = false;
LLVector2 LLRender::sUIGLScaleFactor = LLVector2(1.f, 1.f);

struct LLVBCache
{
    LLPointer<LLVertexBuffer> vb;
    std::chrono::steady_clock::time_point touched;
};

static std::unordered_map<U64, LLVBCache> sVBCache;
static thread_local std::list<LLVertexBufferData> *sBufferDataList = nullptr;


LLTexUnit::LLTexUnit(S32 index)
    : mCurrTexType(TT_NONE),
    mHasMipMaps(false),
    mIndex(index)
{
    llassert_always(index < (S32)LL_NUM_TEXTURE_LAYERS);
}

void LLTexUnit::refreshState(void)
{
    gGL.flush();
}

void LLTexUnit::activate(void)
{
    if (mIndex < 0) return;

    if ((S32)gGL.mCurrTextureUnitIndex != mIndex || gGL.mDirty)
    {
        gGL.flush();
        gGL.mCurrTextureUnitIndex = mIndex;
    }
}

void LLTexUnit::enable(eTextureType type)
{
    if (mIndex < 0) return;

    if ( (mCurrTexType != type || gGL.mDirty) && (type != TT_NONE) )
    {
        activate();
        if (mCurrTexType != TT_NONE && !gGL.mDirty)
        {
            disable();
        }
        mCurrTexType = type;

        gGL.flush();
    }
}

void LLTexUnit::disable(void)
{
    if (mIndex < 0) return;

    if (mCurrTexType != TT_NONE)
    {
        unbind(mCurrTexType);
        mCurrTexType = TT_NONE;
    }
}

static void vkNoteDefaultBind(LLImageGL* image, S32 unit, const char* reason)
{
    LLGLSLShader* sh = LLGLSLShader::sCurBoundShaderPtr;
    LLVKContract::noteFbSlot(sh,
                             sh != nullptr ? sh->mName : std::string("(noshader)"),
                             (U32)llmax(unit, 0), reason);
}

void LLTexUnit::vkNotifyShaderChannelBound()
{
    if (!LLVKLoader::isVulkanInitialized())
    {
        return;
    }
    LLGLSLShader* sh = LLGLSLShader::sCurBoundShaderPtr;
    if (sh == nullptr)
    {
        return;
    }
    sh->vkCaptureChannelBoundView(mIndex);
}

void LLTexUnit::bindFast(LLTexture* texture)
{
    LLImageGL* gl_tex = texture->getGLTexture();
    texture->setActive();
    gGL.mCurrTextureUnitIndex = mIndex;
    if (!gl_tex->hasVkImage())
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_PIPELINE("MISSING TEXTURE");
        texture->forceImmediateUpdate();
        gl_tex->forceUpdateBindStats();
        vkNoteDefaultBind(gl_tex, mIndex, "bindfast_default");
        texture->bindDefaultImage(mIndex);
        mCurrVkWhite = false;
        return;
    }
    const bool same_state = (mCurrImageGL == gl_tex)
                            && mCurrRenderTarget == nullptr
                            && mCurrCubeMap == nullptr
                            && !mCurrCompareMode
                            && gl_tex->hasVkImage()
                            && !gl_tex->mTexOptionsDirty
                            && mHasMipMaps == gl_tex->mHasMipMaps
                            && mCurrAddressMode == gl_tex->getAddressMode()
                            && mCurrFilterOption == gl_tex->getFilteringOption();
    mHasMipMaps = gl_tex->mHasMipMaps;
    if (gl_tex->mTexOptionsDirty)
    {
        gl_tex->mTexOptionsDirty = false;
        setTextureAddressModeFast(gl_tex->mAddressMode, gl_tex->getTarget());
        setTextureFilteringOptionFast(gl_tex->mFilterOption, gl_tex->getTarget());
    }
    mCurrImageGL = gl_tex;
    mCurrVkHeapSlot = (gl_tex->getTarget() == TT_TEXTURE)
                          ? LLImageGL::vkHeapSlotOrDefault(gl_tex)
                          : 0xFFFFFFFFu;
    mCurrRenderTarget = nullptr;
    mCurrCubeMap      = nullptr;
    mCurrCompareMode  = false;
    mCurrAddressMode  = gl_tex->getAddressMode();
    mCurrFilterOption = gl_tex->getFilteringOption();
    mCurrVkWhite      = false;
    if (same_state)
    {
        LLGLSLShader* sh = LLGLSLShader::sCurBoundShaderPtr;
        if (sh != nullptr && LLVKLoader::isVulkanInitialized())
        {
            sh->vkCaptureChannelBoundView(mIndex);
        }
    }
    else
    {
        vkNotifyShaderChannelBound();
    }
}

bool LLTexUnit::bind(LLTexture* texture, bool for_rendering, bool forceBind)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    if (mIndex >= 0)
    {
        gGL.flush();

        LLImageGL* gl_tex = NULL ;

        if (texture != NULL && (gl_tex = texture->getGLTexture()))
        {
            if (gl_tex->hasVkImage()) //if texture exists
            {
                if ((mCurrImageGL != gl_tex) || forceBind)
                {
                    activate();
                    enable(gl_tex->getTarget());
                    if(gl_tex->updateBindStats())
                    {
                        texture->setActive() ;
                        texture->updateBindStatsForTester() ;
                    }
                    mHasMipMaps = gl_tex->mHasMipMaps;
                    if (gl_tex->mTexOptionsDirty)
                    {
                        gl_tex->mTexOptionsDirty = false;
                        setTextureAddressMode(gl_tex->mAddressMode);
                        setTextureFilteringOption(gl_tex->mFilterOption);
                    }
                }
                mCurrImageGL = gl_tex;
                mCurrVkHeapSlot = (gl_tex->getTarget() == TT_TEXTURE)
                                      ? LLImageGL::vkHeapSlotOrDefault(gl_tex)
                                      : 0xFFFFFFFFu;
                mCurrRenderTarget = nullptr;
                mCurrCubeMap      = nullptr;
                mCurrCompareMode  = false;
                mCurrAddressMode  = gl_tex->getAddressMode();
                mCurrFilterOption = gl_tex->getFilteringOption();
                mCurrVkWhite      = false;
            }
            else
            {
                texture->forceImmediateUpdate() ;

                gl_tex->forceUpdateBindStats() ;
                vkNoteDefaultBind(gl_tex, mIndex, "bindtex_default");
                mCurrVkWhite = false;
                return texture->bindDefaultImage(mIndex);
            }
        }
        else
        {
            if (texture)
            {
                LL_DEBUGS() << "NULL LLTexUnit::bind GL image" << LL_ENDL;
            }
            else
            {
                LL_DEBUGS() << "NULL LLTexUnit::bind texture" << LL_ENDL;
            }
            return false;
        }
    }
    else
    {
        return false;
    }

    vkNotifyShaderChannelBound();
    return true;
}

bool LLTexUnit::bind(LLImageGL* texture, bool for_rendering, bool forceBind)
{
    if (mIndex < 0) return false;

    if(!texture)
    {
        LL_DEBUGS() << "NULL LLTexUnit::bind texture" << LL_ENDL;
        return false;
    }

    if(!texture->hasVkImage())
    {
        mCurrVkWhite = false;
        if(LLImageGL::sDefaultGLTexture && LLImageGL::sDefaultGLTexture->hasVkImage())
        {
            vkNoteDefaultBind(texture, mIndex, "bind_default");
            return bind(LLImageGL::sDefaultGLTexture) ;
        }
        return false ;
    }

    if ((mCurrImageGL != texture) || forceBind)
    {
        gGL.flush();
        activate();
        enable(texture->getTarget());
        texture->updateBindStats();
        mHasMipMaps = texture->mHasMipMaps;
        if (texture->mTexOptionsDirty)
        {
            texture->mTexOptionsDirty = false;
            setTextureAddressMode(texture->mAddressMode);
            setTextureFilteringOption(texture->mFilterOption);
        }
    }

    mCurrImageGL = texture;
    mCurrVkHeapSlot = (texture->getTarget() == TT_TEXTURE)
                          ? LLImageGL::vkHeapSlotOrDefault(texture)
                          : 0xFFFFFFFFu;
    mCurrRenderTarget = nullptr;
    mCurrCubeMap      = nullptr;
    mCurrCompareMode  = false;
    mCurrAddressMode  = texture->getAddressMode();
    mCurrFilterOption = texture->getFilteringOption();
    mCurrVkWhite      = false;

    if (LLVKContract::verboseEnabled() && mIndex < 4 && texture->getTarget() != TT_TEXTURE)
    {
        static std::atomic<U32> s_non2d_bind{0};
        const U32 n = ++s_non2d_bind;
        if ((n & (n - 1)) == 0)
        {
            LL_WARNS("VKContract") << "VKC non2d_bind n=" << n
                                   << " unit=" << mIndex
                                   << " tgt=0x" << std::hex << texture->getTarget() << std::dec
                                   << " w=" << texture->getWidth()
                                   << " gl=" << (void*)texture
                                   << " shader=" << (LLGLSLShader::sCurBoundShaderPtr != nullptr
                                                         ? LLGLSLShader::sCurBoundShaderPtr->mName
                                                         : std::string("(none)"))
                                   << " passtag=" << LLVKLoader::gVkPerfPassTag
                                   << LL_ENDL;
        }
    }

    vkNotifyShaderChannelBound();
    return true;
}

U32 LLTexUnit::currVkHeapSlotOrDefault() const
{
    return (mCurrVkHeapSlot != 0xFFFFFFFFu) ? mCurrVkHeapSlot
                                            : LLImageGL::vkHeapSlotOrDefault(nullptr);
}

bool LLTexUnit::bind(LLCubeMap* cubeMap)
{
    if (mIndex < 0) return false;

    gGL.flush();

    if (cubeMap == NULL)
    {
        LL_WARNS() << "NULL LLTexUnit::bind cubemap" << LL_ENDL;
        return false;
    }

    if (mCurrCubeMap != cubeMap)
    {
        if (LLCubeMap::sUseCubeMaps)
        {
            activate();
            enable(LLTexUnit::TT_CUBE_MAP);
            mHasMipMaps = cubeMap->mImages[0]->mHasMipMaps;
            cubeMap->mImages[0]->updateBindStats();
            if (cubeMap->mImages[0]->mTexOptionsDirty)
            {
                cubeMap->mImages[0]->mTexOptionsDirty = false;
                setTextureAddressMode(cubeMap->mImages[0]->mAddressMode);
                setTextureFilteringOption(cubeMap->mImages[0]->mFilterOption);
            }
        }
        else
        {
            LL_WARNS() << "Using cube map without extension!" << LL_ENDL;
            return false;
        }
    }

    mCurrCubeMap      = cubeMap;
    mCurrImageGL      = nullptr;
    mCurrVkHeapSlot   = 0xFFFFFFFFu;
    mCurrRenderTarget = nullptr;
    mCurrCompareMode  = false;
    mCurrAddressMode  = cubeMap->mImages[0]->getAddressMode();
    mCurrFilterOption = cubeMap->mImages[0]->getFilteringOption();
    mCurrVkWhite      = false;
    vkNotifyShaderChannelBound();
    return true;
}

bool LLTexUnit::bind(LLRenderTarget* renderTarget, bool bindDepth, U32 depthLayer)
{
    if (mIndex < 0) return false;

    gGL.flush();

    if (bindDepth)
    {
        llassert(renderTarget->hasDepth()); // target MUST have a depth buffer attachment
    }

    bindManual(renderTarget->getUsage(), 0);

    mCurrRenderTarget = renderTarget;
    mCurrRTAttachment = 0;
    mCurrRTDepth      = bindDepth;
    mCurrRTDepthLayer = depthLayer;
    mCurrCompareMode  = bindDepth && renderTarget->usesDepthCompareSampler();
    mCurrImageGL      = nullptr;
    mCurrVkHeapSlot   = 0xFFFFFFFFu;
    mCurrCubeMap      = nullptr;
    mCurrAddressMode  = TAM_WRAP;
    mCurrFilterOption = TFO_BILINEAR;
    mCurrVkWhite      = false;

    renderTarget->bindForShaderRead(mCurrRTAttachment, bindDepth);

    vkNotifyShaderChannelBound();
    return true;
}

VkImageView LLTexUnit::getLiveVkImageView() const
{
    if (mCurrRenderTarget != nullptr)
    {
        const bool same_pass = (mCurrRenderTarget == LLRenderTarget::getCurrentBoundTarget());
        if (same_pass ||
            mCurrRenderTarget->isVkActivePassAttachment(mCurrRTAttachment, mCurrRTDepth))
        {
            return VK_NULL_HANDLE;
        }
        mCurrRenderTarget->bindForShaderRead(mCurrRTAttachment, mCurrRTDepth);
        if (mCurrRTDepth)
        {
            if (mCurrRTDepthLayer != 0xFFFFFFFFu &&
                mCurrRenderTarget->getVkDepthLayerCount() > 1)
            {
                return mCurrRenderTarget->getVkDepthLayerView(mCurrRTDepthLayer);
            }
            return mCurrRenderTarget->hasVkDepth() ? mCurrRenderTarget->getVkDepthView()
                                                   : VK_NULL_HANDLE;
        }
        return mCurrRenderTarget->hasVkImage(mCurrRTAttachment)
                   ? mCurrRenderTarget->getVkImageView(mCurrRTAttachment)
                   : VK_NULL_HANDLE;
    }
    if (mCurrCubeMap != nullptr && mCurrCubeMap->hasVkCubeImage())
    {
        return mCurrCubeMap->getVkCubeImageView();
    }
    if (mCurrImageGL != nullptr && mCurrImageGL->hasVkImage())
    {
        return mCurrImageGL->getVkImageView();
    }
    if (mCurrVkWhite)
    {
        return LLVKLoader::getWhiteVkImageView();
    }
    return VK_NULL_HANDLE;
}

U8 LLTexUnit::getLiveVkImageViewDim() const
{
    if (mCurrRenderTarget != nullptr)
    {
        return LLGLSLShader::VKSD_2D;
    }
    if (mCurrCubeMap != nullptr && mCurrCubeMap->hasVkCubeImage())
    {
        return LLGLSLShader::VKSD_CUBE;
    }
    if (mCurrImageGL != nullptr && mCurrImageGL->hasVkImage())
    {
        switch (mCurrImageGL->getTarget())
        {
        case TT_CUBE_MAP:       return LLGLSLShader::VKSD_CUBE;
        case TT_CUBE_MAP_ARRAY: return LLGLSLShader::VKSD_CUBE_ARRAY;
        case TT_TEXTURE_3D:     return LLGLSLShader::VKSD_3D;
        default:                return LLGLSLShader::VKSD_2D;
        }
    }
    return LLGLSLShader::VKSD_2D;
}

VkSampler LLTexUnit::getLiveVkSampler() const
{
    eTextureFilterOptions effective_filter = mCurrFilterOption;
    if (effective_filter == TFO_ANISOTROPIC)
    {
        const bool global_aniso = gGLManager.mHasAnisotropic && LLImageGL::sGlobalUseAnisotropic;
        if (!global_aniso)
        {
            effective_filter = TFO_TRILINEAR;
        }
    }

    return LLVKLoader::getSamplerForState((U32)mCurrAddressMode,
                                          (U32)effective_filter,
                                          mHasMipMaps,
                                          mCurrCompareMode);
}

bool LLTexUnit::bindManual(eTextureType type, U32 texture, bool hasMips)
{
    if (mIndex < 0)
    {
        return false;
    }

    mCurrCompareMode = false;

    gGL.flush();

    activate();
    enable(type);
    mHasMipMaps = hasMips;

    mCurrImageGL = nullptr;
    mCurrVkHeapSlot = 0xFFFFFFFFu;
    mCurrRenderTarget = nullptr;
    mCurrCubeMap = nullptr;
    mCurrVkWhite = false;
    return true;
}

void LLTexUnit::unbind(eTextureType type)
{

    if (mIndex < 0) return;

    gGL.flush();
    activate();

    if (mCurrTexType == type)
    {
        mCurrImageGL = nullptr;
        mCurrVkHeapSlot = 0xFFFFFFFFu;
        mCurrRenderTarget = nullptr;
        mCurrCubeMap = nullptr;
        mCurrVkWhite = (type == LLTexUnit::TT_TEXTURE);

        vkNotifyShaderChannelBound();
    }
}

void LLTexUnit::unbindFast(eTextureType type)
{
    activate();

    if (mCurrTexType == type)
    {
        mCurrImageGL = nullptr;
        mCurrVkHeapSlot = 0xFFFFFFFFu;
        mCurrRenderTarget = nullptr;
        mCurrCubeMap = nullptr;
        mCurrVkWhite = (type == LLTexUnit::TT_TEXTURE);

        vkNotifyShaderChannelBound();
    }
}

void LLTexUnit::setTextureAddressMode(eTextureAddressMode mode)
{
    if (mIndex < 0) return;

    gGL.flush();

    activate();

    setTextureAddressModeFast(mode, mCurrTexType);
}

void LLTexUnit::setTextureAddressModeFast(eTextureAddressMode mode, eTextureType tex_type)
{
    mCurrAddressMode = mode;
}

void LLTexUnit::setTextureFilteringOption(LLTexUnit::eTextureFilterOptions option)
{
    if (mIndex < 0 || mCurrTexType == LLTexUnit::TT_MULTISAMPLE_TEXTURE) return;

    gGL.flush();

    setTextureFilteringOptionFast(option, mCurrTexType);
}

void LLTexUnit::setTextureFilteringOptionFast(LLTexUnit::eTextureFilterOptions option, eTextureType tex_type)
{
    mCurrFilterOption = option;
}

LLLightState::LLLightState(S32 index)
: mIndex(index),
  mEnabled(false),
  mConstantAtten(1.f),
  mLinearAtten(0.f),
  mQuadraticAtten(0.f),
  mSpotExponent(0.f),
  mSpotCutoff(180.f)
{
    if (mIndex == 0)
    {
        mDiffuse.set(1,1,1,1);
        mDiffuseB.set(0,0,0,0);
        mSpecular.set(1,1,1,1);
    }

    mSunIsPrimary = true;

    mAmbient.set(0,0,0,1);
    mPosition.set(0,0,1,0);
    mSpotDirection.set(0,0,-1);
}

void LLLightState::enable()
{
    mEnabled = true;
}

void LLLightState::disable()
{
    mEnabled = false;
}

void LLLightState::setDiffuse(const LLColor4& diffuse)
{
    if (mDiffuse != diffuse)
    {
        ++gGL.mLightHash;
        mDiffuse = diffuse;
    }
}

void LLLightState::setDiffuseB(const LLColor4& diffuse)
{
    if (mDiffuseB != diffuse)
    {
        ++gGL.mLightHash;
        mDiffuseB = diffuse;
    }
}

void LLLightState::setSunPrimary(bool v)
{
    if (mSunIsPrimary != v)
    {
        ++gGL.mLightHash;
        mSunIsPrimary = v;
    }
}

void LLLightState::setSize(F32 v)
{
    if (mSize != v)
    {
        ++gGL.mLightHash;
        mSize = v;
    }
}

void LLLightState::setFalloff(F32 v)
{
    if (mFalloff != v)
    {
        ++gGL.mLightHash;
        mFalloff = v;
    }
}

void LLLightState::setAmbient(const LLColor4& ambient)
{
    if (mAmbient != ambient)
    {
        ++gGL.mLightHash;
        mAmbient = ambient;
    }
}

void LLLightState::setSpecular(const LLColor4& specular)
{
    if (mSpecular != specular)
    {
        ++gGL.mLightHash;
        mSpecular = specular;
    }
}

void LLLightState::setPosition(const LLVector4& position)
{
    ++gGL.mLightHash;
    mPosition = position;
    glm::vec4 pos(position);
    pos = gGL.getModelviewMatrix() * pos;
    mPosition.set(glm::value_ptr(pos));
}

void LLLightState::setConstantAttenuation(const F32& atten)
{
    if (mConstantAtten != atten)
    {
        mConstantAtten = atten;
        ++gGL.mLightHash;
    }
}

void LLLightState::setLinearAttenuation(const F32& atten)
{
    if (mLinearAtten != atten)
    {
        ++gGL.mLightHash;
        mLinearAtten = atten;
    }
}

void LLLightState::setQuadraticAttenuation(const F32& atten)
{
    if (mQuadraticAtten != atten)
    {
        ++gGL.mLightHash;
        mQuadraticAtten = atten;
    }
}

void LLLightState::setSpotExponent(const F32& exponent)
{
    if (mSpotExponent != exponent)
    {
        ++gGL.mLightHash;
        mSpotExponent = exponent;
    }
}

void LLLightState::setSpotCutoff(const F32& cutoff)
{
    if (mSpotCutoff != cutoff)
    {
        ++gGL.mLightHash;
        mSpotCutoff = cutoff;
    }
}

void LLLightState::setSpotDirection(const LLVector3& direction)
{
    ++gGL.mLightHash;
    glm::vec3 dir(direction);
    const glm::mat3 mat(gGL.getModelviewMatrix());
    dir = mat * dir;

    mSpotDirection.set(glm::value_ptr(dir));
}

LLRender::LLRender()
  : mDirty(false),
    mCount(0),
    mMode(LLRender::TRIANGLES),
    mCurrTextureUnitIndex(0),
    mLineWidth(1.f), // <FS> Line width OGL core profile fix by Rye Mutt
    mPolygonOffsetFactor(0.f),
    mPolygonOffsetUnits(0.f),
    mCurrBlendColorSFactor(BF_ONE),
    mCurrBlendColorDFactor(BF_ZERO),
    mCurrBlendAlphaSFactor(BF_ONE),
    mCurrBlendAlphaDFactor(BF_ZERO),
    // <FS:Ansariel> Don't ignore OpenGL max line width
    mMaxLineWidthSmooth(1.f),
    mMaxLineWidthAliased(1.f)
    // </FS:Ansariel>
{
    for (U32 i = 0; i < LL_NUM_TEXTURE_LAYERS; i++)
    {
        mTexUnits[i].mIndex = i;
    }

    for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; ++i)
    {
        mLightState[i].mIndex = i;
    }

    for (U32 i = 0; i < 4; i++)
    {
        mCurrColorMask[i] = true;
    }

    for (U32 i = 0; i < 4; i++)
    {
        mClearColor[i] = 0.f;
    }

    mCurrBlendColorSFactor = BF_UNDEF;
    mCurrBlendAlphaSFactor = BF_UNDEF;
    mCurrBlendColorDFactor = BF_UNDEF;
    mCurrBlendAlphaDFactor = BF_UNDEF;

    mMatrixMode = LLRender::MM_MODELVIEW;

    for (U32 i = 0; i < NUM_MATRIX_MODES; ++i)
    {
        for (U32 j = 0; j < LL_MATRIX_STACK_DEPTH; ++j)
        {
            mMatrix[i][j] = glm::identity<glm::mat4>();
        }
        mMatIdx[i] = 0;
        mMatHash[i] = 0;
        mCurMatHash[i] = 0xFFFFFFFF;
        mVkSyncedMatHash[i] = 0xFFFFFFFF;
    }

    mLightHash = 0;
}

LLRender::~LLRender()
{
    resetVertexBuffer();
}

bool LLRender::init(bool needs_vertex_buffer)
{
    gGL.setSceneBlendType(LLRender::BT_ALPHA);
    gGL.setAmbientLightColor(LLColor4::black);

    LLGLState::setCullFaceMode(GL_BACK);

    if (needs_vertex_buffer)
    {
        initVertexBuffer();
    }

    mMaxLineWidthAliased = LLVKLoader::getMaxLineWidth();
    mMaxLineWidthSmooth  = mMaxLineWidthAliased;

    return true;
}

void LLRender::initVertexBuffer()
{
    llassert_always(mScratchVerts == nullptr);
    mScratchVerts     = (LLVector4a*) ll_aligned_malloc_16(4096 * sizeof(LLVector4a));
    mScratchTexcoords = (LLVector2*)  ll_aligned_malloc_16(4096 * sizeof(LLVector2));
    mScratchColors    = (LLColor4U*)  ll_aligned_malloc_16(4096 * sizeof(LLColor4U));
    mVerticesp = mScratchVerts;
    mVerticesp.setStride(0);
    mTexcoordsp = mScratchTexcoords;
    mTexcoordsp.setStride(0);
    mColorsp = mScratchColors;
    mColorsp.setStride(0);
}

void LLRender::resetVertexBuffer()
{
    ll_aligned_free_16(mScratchVerts);
    ll_aligned_free_16(mScratchTexcoords);
    ll_aligned_free_16(mScratchColors);
    mScratchVerts     = nullptr;
    mScratchTexcoords = nullptr;
    mScratchColors    = nullptr;
    mVerticesp = (LLVector4a*) nullptr;
    mTexcoordsp = (LLVector2*) nullptr;
    mColorsp = (LLColor4U*) nullptr;
}

void LLRender::shutdown()
{
    resetVertexBuffer();
    sVBCache.clear();
}

void LLRender::refreshState(void)
{
    mDirty = true;

    U32 active_unit = mCurrTextureUnitIndex;

    for (U32 i = 0; i < mTexUnits.size(); i++)
    {
        mTexUnits[i].refreshState();
    }

    mTexUnits[active_unit].activate();

    setColorMask(mCurrColorMask[0], mCurrColorMask[1], mCurrColorMask[2], mCurrColorMask[3]);

    flush();

    mDirty = false;
}

void LLRender::syncLightState()
{
    LLGLSLShader *shader = LLGLSLShader::sCurBoundShaderPtr;

    if (!shader)
    {
        return;
    }

    if (shader->mLightHash != mLightHash)
    {
        shader->mLightHash = mLightHash;

        LLVector4 position[LL_NUM_LIGHT_UNITS];
        LLVector3 direction[LL_NUM_LIGHT_UNITS];
        LLVector4 attenuation[LL_NUM_LIGHT_UNITS];
        LLVector3 diffuse[LL_NUM_LIGHT_UNITS];
        LLVector2 size[LL_NUM_LIGHT_UNITS];

        for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
        {
            LLLightState *light = &mLightState[i];

            position[i]  = light->mPosition;
            direction[i] = light->mSpotDirection;
            attenuation[i].set(light->mLinearAtten, light->mQuadraticAtten, light->mSpecular.mV[2], light->mSpecular.mV[3]);
            diffuse[i].set(light->mDiffuse.mV);
            size[i].set(light->mSize, light->mFalloff);
        }

        for (U32 dbg_i = 0; dbg_i < LL_NUM_LIGHT_UNITS; ++dbg_i)
        {
            bool dbg_bad = false;
            for (U32 k = 0; k < 4; ++k) { if (!std::isfinite(position[dbg_i].mV[k]) || !std::isfinite(attenuation[dbg_i].mV[k])) dbg_bad = true; }
            for (U32 k = 0; k < 3; ++k) { if (!std::isfinite(direction[dbg_i].mV[k]) || !std::isfinite(diffuse[dbg_i].mV[k])) dbg_bad = true; }
            for (U32 k = 0; k < 2; ++k) { if (!std::isfinite(size[dbg_i].mV[k])) dbg_bad = true; }
            if (dbg_bad)
            {
                static U32 s_dbg_light_nan = 0;
                if (s_dbg_light_nan < 5000u)
                {
                    ++s_dbg_light_nan;
                    LL_WARNS("VKNaN") << "non-finite LIGHT[" << dbg_i << "] -> GPU shader="
                                      << (shader ? shader->mName : std::string("?")) << LL_ENDL;
                }
                for (U32 k = 0; k < 4; ++k) { if (!std::isfinite(position[dbg_i].mV[k])) position[dbg_i].mV[k] = 0.f; if (!std::isfinite(attenuation[dbg_i].mV[k])) attenuation[dbg_i].mV[k] = 0.f; }
                for (U32 k = 0; k < 3; ++k) { if (!std::isfinite(direction[dbg_i].mV[k])) direction[dbg_i].mV[k] = 0.f; if (!std::isfinite(diffuse[dbg_i].mV[k])) diffuse[dbg_i].mV[k] = 0.f; }
                for (U32 k = 0; k < 2; ++k) { if (!std::isfinite(size[dbg_i].mV[k])) size[dbg_i].mV[k] = 0.f; }
            }
        }

        if (LLVKLoader::isVulkanInitialized())
        {
            LLVKLoader::Lights_PerProgramBind         lights_data = {};
            LLVKLoader::LightsSpecular_PerProgramBind lights_specular_data = {};
            for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
            {
                lights_data.light_position[i][0] = position[i].mV[0];
                lights_data.light_position[i][1] = position[i].mV[1];
                lights_data.light_position[i][2] = position[i].mV[2];
                lights_data.light_position[i][3] = position[i].mV[3];
                lights_data.light_diffuse[i][0]  = diffuse[i].mV[0];
                lights_data.light_diffuse[i][1]  = diffuse[i].mV[1];
                lights_data.light_diffuse[i][2]  = diffuse[i].mV[2];

                lights_specular_data.light_position[i][0]    = position[i].mV[0];
                lights_specular_data.light_position[i][1]    = position[i].mV[1];
                lights_specular_data.light_position[i][2]    = position[i].mV[2];
                lights_specular_data.light_position[i][3]    = position[i].mV[3];
                lights_specular_data.light_attenuation[i][0] = attenuation[i].mV[0];
                lights_specular_data.light_attenuation[i][1] = attenuation[i].mV[1];
                lights_specular_data.light_attenuation[i][2] = attenuation[i].mV[2];
                lights_specular_data.light_attenuation[i][3] = attenuation[i].mV[3];
                lights_specular_data.light_diffuse[i][0]     = diffuse[i].mV[0];
                lights_specular_data.light_diffuse[i][1]     = diffuse[i].mV[1];
                lights_specular_data.light_diffuse[i][2]     = diffuse[i].mV[2];
            }
            LLVKLoader::writeCurrentLightsUBO(lights_data);
            LLVKLoader::writeCurrentLightsSpecularUBO(lights_specular_data);
        }

        if (LLVKLoader::isVulkanInitialized() && shader->mVkPerProgramUBO != VK_NULL_HANDLE
            && shader->mVkPerProgramUBOMapped != nullptr
            && shader->mVkPerProgramUBOSize == 256)
        {
            LLVKLoader::Preview_PerProgramBind ubo_data = {};
            for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
            {
                ubo_data.light_position[i][0] = position[i].mV[0];
                ubo_data.light_position[i][1] = position[i].mV[1];
                ubo_data.light_position[i][2] = position[i].mV[2];
                ubo_data.light_position[i][3] = position[i].mV[3];
                ubo_data.light_diffuse[i][0] = diffuse[i].mV[0];
                ubo_data.light_diffuse[i][1] = diffuse[i].mV[1];
                ubo_data.light_diffuse[i][2] = diffuse[i].mV[2];
            }
            memcpy(shader->mVkPerProgramUBOMapped, &ubo_data, sizeof(ubo_data));
        }

        if (LLVKLoader::isVulkanInitialized() && shader->mWritePerProgramUBOMinimumAlpha
            && shader->mVkPerProgramUBO != VK_NULL_HANDLE
            && shader->mVkPerProgramUBOMapped != nullptr
            && (shader->mVkPerProgramUBOSize == LLVKLoader::ALPHAF_UBO_SIZE_SHADOW
                || shader->mVkPerProgramUBOSize == LLVKLoader::ALPHAF_UBO_SIZE_NO_SHADOW))
        {
            char* mapped = (char*)shader->mVkPerProgramUBOMapped;
            U32 lights_offset =
                (shader->mVkPerProgramUBOSize == LLVKLoader::ALPHAF_UBO_SIZE_NO_SHADOW)
                ? LLVKLoader::ALPHAF_UBO_OFFSET_LIGHTS_NO_SHADOW
                : LLVKLoader::ALPHAF_UBO_OFFSET_LIGHTS_SHADOW;

            for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
            {
                F32 v[4] = { position[i].mV[0], position[i].mV[1], position[i].mV[2], position[i].mV[3] };
                memcpy(mapped + lights_offset + i * 16, v, 16);
            }
            lights_offset += 128;

            for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
            {
                F32 v[4] = { direction[i].mV[0], direction[i].mV[1], direction[i].mV[2], 0.f };
                memcpy(mapped + lights_offset + i * 16, v, 16);
            }
            lights_offset += 128;

            for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
            {
                F32 v[4] = { attenuation[i].mV[0], attenuation[i].mV[1], attenuation[i].mV[2], attenuation[i].mV[3] };
                memcpy(mapped + lights_offset + i * 16, v, 16);
            }
            lights_offset += 128;

            for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
            {
                F32 v[4] = { diffuse[i].mV[0], diffuse[i].mV[1], diffuse[i].mV[2], 0.f };
                memcpy(mapped + lights_offset + i * 16, v, 16);
            }

        }

        if (LLVKLoader::isVulkanInitialized() && shader->mVkPerProgramUBO != VK_NULL_HANDLE
            && shader->mVkPerProgramUBOMapped != nullptr
            && (shader->mVkPerProgramUBOSize == LLVKLoader::GLTFMR_UBO_SIZE_ALPHA_SUNSHADOW
                || shader->mVkPerProgramUBOSize == LLVKLoader::GLTFMR_UBO_SIZE_ALPHA_NOSHADOW))
        {
            char* mapped = (char*)shader->mVkPerProgramUBOMapped;
            U32 lights_offset =
                (shader->mVkPerProgramUBOSize == LLVKLoader::GLTFMR_UBO_SIZE_ALPHA_NOSHADOW)
                ? LLVKLoader::GLTFMR_UBO_OFFSET_LIGHTS_NOSHADOW
                : LLVKLoader::GLTFMR_UBO_OFFSET_LIGHTS_SUNSHADOW;

            for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
            {
                F32 v[4] = { position[i].mV[0], position[i].mV[1], position[i].mV[2], position[i].mV[3] };
                memcpy(mapped + lights_offset + i * 16, v, 16);
            }
            lights_offset += 128;

            for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
            {
                F32 v[4] = { direction[i].mV[0], direction[i].mV[1], direction[i].mV[2], 0.f };
                memcpy(mapped + lights_offset + i * 16, v, 16);
            }
            lights_offset += 128;

            for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
            {
                F32 v[4] = { attenuation[i].mV[0], attenuation[i].mV[1], attenuation[i].mV[2], attenuation[i].mV[3] };
                memcpy(mapped + lights_offset + i * 16, v, 16);
            }
            lights_offset += 128;

            for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
            {
                F32 v[4] = { diffuse[i].mV[0], diffuse[i].mV[1], diffuse[i].mV[2], 0.f };
                memcpy(mapped + lights_offset + i * 16, v, 16);
            }
            lights_offset += 128;

            for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
            {
                F32 v[4] = { size[i].mV[0], size[i].mV[1], 0.f, 0.f };
                memcpy(mapped + lights_offset + i * 16, v, 16);
            }

        }
    }
}

void LLRender::getLightArrayData(F32* position_out, F32* direction_out, F32* attenuation_out, F32* diffuse_out) const
{
    for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
    {
        const LLLightState* light = &mLightState[i];

        position_out[i * 4 + 0] = light->mPosition.mV[0];
        position_out[i * 4 + 1] = light->mPosition.mV[1];
        position_out[i * 4 + 2] = light->mPosition.mV[2];
        position_out[i * 4 + 3] = light->mPosition.mV[3];

        direction_out[i * 3 + 0] = light->mSpotDirection.mV[0];
        direction_out[i * 3 + 1] = light->mSpotDirection.mV[1];
        direction_out[i * 3 + 2] = light->mSpotDirection.mV[2];

        attenuation_out[i * 4 + 0] = light->mLinearAtten;
        attenuation_out[i * 4 + 1] = light->mQuadraticAtten;
        attenuation_out[i * 4 + 2] = light->mSpecular.mV[2];
        attenuation_out[i * 4 + 3] = light->mSpecular.mV[3];

        diffuse_out[i * 3 + 0] = light->mDiffuse.mV[0];
        diffuse_out[i * 3 + 1] = light->mDiffuse.mV[1];
        diffuse_out[i * 3 + 2] = light->mDiffuse.mV[2];
    }
}

void LLRender::getLightDeferredAttenuationData(F32* size_out) const
{
    for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
    {
        const LLLightState* light = &mLightState[i];
        size_out[i * 2 + 0] = light->mSize;
        size_out[i * 2 + 1] = light->mFalloff;
    }
}

void LLRender::syncMatrices()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;

    LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;

    if (shader)
    {
        if (LLVKLoader::shouldUseVulkanRender())
        {
            ++LLVKLoader::gVkPerf.syncmat_call;
            const bool mats_dirty = LLVKLoader::perFrameMatrixNeedsWrite()
                                    || mVkSyncedMatHash[MM_PROJECTION] != mMatHash[MM_PROJECTION]
                                    || mVkSyncedMatHash[MM_TEXTURE0] != mMatHash[MM_TEXTURE0]
                                    || mVkSyncedMatHash[MM_TEXTURE1] != mMatHash[MM_TEXTURE1]
                                    || mVkSyncedMatHash[MM_TEXTURE2] != mMatHash[MM_TEXTURE2]
                                    || mVkSyncedMatHash[MM_TEXTURE3] != mMatHash[MM_TEXTURE3];
            if (mats_dirty)
            {
                ++LLVKLoader::gVkPerf.syncmat_build;
                LLVKLoader::PerFrameMatrixUBO perframe = {};
                LLVKLoader::TextureMatrixUBO  texmat   = {};

                const glm::mat4& proj_mat = mMatrix[MM_PROJECTION][mMatIdx[MM_PROJECTION]];
                glm::mat4 vulkan_z_correction = glm::identity<glm::mat4>();
                vulkan_z_correction[2][2] = 0.5f;
                vulkan_z_correction[3][2] = 0.5f;
                const glm::mat4 proj_mat_vulkan = vulkan_z_correction * proj_mat;

                std::memcpy(perframe.projection_matrix,
                            glm::value_ptr(proj_mat_vulkan),
                            sizeof(perframe.projection_matrix));

                glm::mat4 inv_proj = glm::inverse(proj_mat);
                {
                    const F32* ipv = glm::value_ptr(inv_proj);
                    bool ip_finite = true;
                    for (U32 ip_i = 0; ip_i < 16u; ++ip_i) { if (!std::isfinite(ipv[ip_i])) { ip_finite = false; break; } }
                    if (!ip_finite) inv_proj = glm::inverse(get_current_projection());
                }
                std::memcpy(perframe.inverse_projection_matrix,
                            glm::value_ptr(inv_proj),
                            sizeof(perframe.inverse_projection_matrix));

                const glm::mat4 identity = glm::identity<glm::mat4>();
                std::memcpy(perframe.identity_matrix,
                            glm::value_ptr(identity),
                            sizeof(perframe.identity_matrix));

                std::memcpy(perframe.last_modelview_matrix,
                            gGLLastModelView,
                            sizeof(perframe.last_modelview_matrix));

                for (U32 tex = 0; tex < 4; ++tex)
                {
                    const glm::mat4& tex_mat = mMatrix[MM_TEXTURE0 + tex][mMatIdx[MM_TEXTURE0 + tex]];
                    std::memcpy(texmat.texture_matrix[tex],
                                glm::value_ptr(tex_mat),
                                sizeof(texmat.texture_matrix[tex]));
                }

                {
                    bool dbg_bad_proj = false, dbg_bad_inv = false, dbg_bad_lmv = false;
                    for (U32 dbg_i = 0; dbg_i < 16u; ++dbg_i)
                    {
                        if (!std::isfinite(perframe.projection_matrix[dbg_i]))         dbg_bad_proj = true;
                        if (!std::isfinite(perframe.inverse_projection_matrix[dbg_i])) dbg_bad_inv  = true;
                        if (!std::isfinite(perframe.last_modelview_matrix[dbg_i]))     dbg_bad_lmv  = true;
                    }
                    if (dbg_bad_proj || dbg_bad_inv || dbg_bad_lmv)
                    {
                        static U32 s_dbg_nan_count = 0;
                        if (s_dbg_nan_count < 5000u)
                        {
                            ++s_dbg_nan_count;
                            LL_WARNS("VKNaN") << "non-finite matrix -> GPU: proj=" << dbg_bad_proj
                                              << " invproj=" << dbg_bad_inv << " lastmv=" << dbg_bad_lmv
                                              << " shader=" << (shader ? shader->mName : std::string("?"))
                                              << LL_ENDL;
                        }
                        const glm::mat4 dbg_id = glm::identity<glm::mat4>();
                        if (dbg_bad_proj) std::memcpy(perframe.projection_matrix,         glm::value_ptr(dbg_id), sizeof(perframe.projection_matrix));
                        if (dbg_bad_inv)  std::memcpy(perframe.inverse_projection_matrix, glm::value_ptr(dbg_id), sizeof(perframe.inverse_projection_matrix));
                        if (dbg_bad_lmv)  std::memcpy(perframe.last_modelview_matrix,     glm::value_ptr(dbg_id), sizeof(perframe.last_modelview_matrix));
                    }
                }
                LLVKLoader::writeCurrentPerFrameMatrixUBO(perframe, texmat);

                mVkSyncedMatHash[MM_PROJECTION] = mMatHash[MM_PROJECTION];
                mVkSyncedMatHash[MM_TEXTURE0]   = mMatHash[MM_TEXTURE0];
                mVkSyncedMatHash[MM_TEXTURE1]   = mMatHash[MM_TEXTURE1];
                mVkSyncedMatHash[MM_TEXTURE2]   = mMatHash[MM_TEXTURE2];
                mVkSyncedMatHash[MM_TEXTURE3]   = mMatHash[MM_TEXTURE3];
            }

            const glm::mat4& modelview_mat = mMatrix[MM_MODELVIEW][mMatIdx[MM_MODELVIEW]];
            {
                const F32* dbg_mv = glm::value_ptr(modelview_mat);
                bool dbg_bad_mv = false;
                for (U32 dbg_i = 0; dbg_i < 16u; ++dbg_i) if (!std::isfinite(dbg_mv[dbg_i])) dbg_bad_mv = true;
                if (dbg_bad_mv)
                {
                    static U32 s_dbg_mv_nan = 0;
                    if (s_dbg_mv_nan < 5000u)
                    {
                        ++s_dbg_mv_nan;
                        LL_WARNS("VKNaN") << "non-finite MODELVIEW -> GPU shader="
                                          << (shader ? shader->mName : std::string("?")) << LL_ENDL;
                    }
                    const glm::mat4 dbg_id = glm::identity<glm::mat4>();
                    LLVKLoader::pushCurrentModelviewMatrix(glm::value_ptr(dbg_id));
                }
                else
                {
                    LLVKLoader::pushCurrentModelviewMatrix(glm::value_ptr(modelview_mat));
                }
            }
        }

        if (shader->mFeatures.hasLighting || shader->mFeatures.calculatesLighting || shader->mFeatures.calculatesAtmospherics)
        {
            syncLightState();
        }
    }
}

void LLRender::translatef(const GLfloat& x, const GLfloat& y, const GLfloat& z)
{
    flush();

    {
        mMatrix[mMatrixMode][mMatIdx[mMatrixMode]] = glm::translate(mMatrix[mMatrixMode][mMatIdx[mMatrixMode]], glm::vec3(x, y, z));
        mMatHash[mMatrixMode]++;
    }
}

void LLRender::scalef(const GLfloat& x, const GLfloat& y, const GLfloat& z)
{
    flush();

    {
        mMatrix[mMatrixMode][mMatIdx[mMatrixMode]] = glm::scale(mMatrix[mMatrixMode][mMatIdx[mMatrixMode]], glm::vec3(x, y, z));
        mMatHash[mMatrixMode]++;
    }
}

void LLRender::ortho(F32 left, F32 right, F32 bottom, F32 top, F32 zNear, F32 zFar)
{
    flush();

    {
        mMatrix[mMatrixMode][mMatIdx[mMatrixMode]] *= glm::ortho(left, right, bottom, top, zNear, zFar);
        mMatHash[mMatrixMode]++;
    }
}

void LLRender::rotatef(const GLfloat& a, const GLfloat& x, const GLfloat& y, const GLfloat& z)
{
    flush();

    {
        mMatrix[mMatrixMode][mMatIdx[mMatrixMode]] = glm::rotate(mMatrix[mMatrixMode][mMatIdx[mMatrixMode]], glm::radians(a), glm::vec3(x,y,z));
        mMatHash[mMatrixMode]++;
    }
}

void LLRender::pushMatrix()
{
    flush();

    {
        if (mMatIdx[mMatrixMode] < LL_MATRIX_STACK_DEPTH-1)
        {
            mMatrix[mMatrixMode][mMatIdx[mMatrixMode]+1] = mMatrix[mMatrixMode][mMatIdx[mMatrixMode]];
            ++mMatIdx[mMatrixMode];
        }
        else
        {
            LL_WARNS() << "Matrix stack overflow." << LL_ENDL;
        }
    }
}

void LLRender::popMatrix()
{
    flush();
    {
        if (mMatIdx[mMatrixMode] > 0)
        {
            --mMatIdx[mMatrixMode];
            mMatHash[mMatrixMode]++;
        }
        else
        {
            LL_WARNS() << "Matrix stack underflow." << LL_ENDL;
        }
    }
}

void LLRender::loadMatrix(const GLfloat* m)
{
    flush();
    {
        mMatrix[mMatrixMode][mMatIdx[mMatrixMode]] = glm::make_mat4((GLfloat*) m);
        mMatHash[mMatrixMode]++;
    }
}

void LLRender::multMatrix(const GLfloat* m)
{
    flush();
    {
        mMatrix[mMatrixMode][mMatIdx[mMatrixMode]] *= glm::make_mat4(m);
        mMatHash[mMatrixMode]++;
    }
}

void LLRender::matrixMode(eMatrixMode mode)
{
    if (mode == MM_TEXTURE)
    {
        U32 tex_index = gGL.getCurrentTexUnitIndex();
        llassert(tex_index <= 3);
        mode = eMatrixMode(MM_TEXTURE0 + tex_index);
        if (mode > MM_TEXTURE3)
        {
            LL_WARNS_ONCE() << "Attempted to assign matrix mode out of bounds: " << mode << LL_ENDL;
            mode = MM_TEXTURE0;
        }
    }

    mMatrixMode = mode;
}

LLRender::eMatrixMode LLRender::getMatrixMode()
{
    if (mMatrixMode >= MM_TEXTURE0 && mMatrixMode <= MM_TEXTURE3)
    {
        return MM_TEXTURE;
    }

    return mMatrixMode;
}


void LLRender::loadIdentity()
{
    flush();

    {
        llassert_always(mMatrixMode < NUM_MATRIX_MODES) ;

        mMatrix[mMatrixMode][mMatIdx[mMatrixMode]] = glm::identity<glm::mat4>();
        mMatHash[mMatrixMode]++;
    }
}

const glm::mat4& LLRender::getModelviewMatrix()
{
    return mMatrix[MM_MODELVIEW][mMatIdx[MM_MODELVIEW]];
}

const glm::mat4& LLRender::getProjectionMatrix()
{
    return mMatrix[MM_PROJECTION][mMatIdx[MM_PROJECTION]];
}

void LLRender::translateUI(F32 x, F32 y, F32 z)
{
    if (mUIOffset.empty())
    {
        LL_ERRS() << "Need to push a UI translation frame before offsetting" << LL_ENDL;
    }

    mUIOffset.back().add(LLVector4a(x, y, z));
}

void LLRender::scaleUI(F32 x, F32 y, F32 z)
{
    if (mUIScale.empty())
    {
        LL_ERRS() << "Need to push a UI transformation frame before scaling." << LL_ENDL;
    }

    mUIScale.back().mul(LLVector4a(x, y, z));
}

void LLRender::pushUIMatrix()
{
    if (mUIOffset.empty())
    {
        mUIOffset.emplace_back(0.f);
    }
    else
    {
        mUIOffset.push_back(mUIOffset.back());
    }

    if (mUIScale.empty())
    {
        mUIScale.emplace_back(1.f);
    }
    else
    {
        mUIScale.push_back(mUIScale.back());
    }
}

void LLRender::popUIMatrix()
{
    if (mUIOffset.empty())
    {
        LL_ERRS() << "UI offset stack blown." << LL_ENDL;
    }
    mUIOffset.pop_back();
    mUIScale.pop_back();
}

LLVector3 LLRender::getUITranslation()
{
    if (mUIOffset.empty())
    {
        return LLVector3::zero;
    }

    return LLVector3(mUIOffset.back().getF32ptr());
}

LLVector3 LLRender::getUIScale()
{
    if (mUIScale.empty())
    {
        return LLVector3::all_one;
    }

    return LLVector3(mUIScale.back().getF32ptr());
}


void LLRender::loadUIIdentity()
{
    if (mUIOffset.empty())
    {
        LL_ERRS() << "Need to push UI translation frame before clearing offset." << LL_ENDL;
    }

    mUIOffset.back().clear();
    mUIScale.back().splat(1);
}

void LLRender::setColorMask(bool writeColor, bool writeAlpha)
{
    setColorMask(writeColor, writeColor, writeColor, writeAlpha);
}

void LLRender::setColorMask(bool writeColorR, bool writeColorG, bool writeColorB, bool writeAlpha)
{
    flush();

    if (mCurrColorMask[0] != writeColorR ||
        mCurrColorMask[1] != writeColorG ||
        mCurrColorMask[2] != writeColorB ||
        mCurrColorMask[3] != writeAlpha)
    {
        mCurrColorMask[0] = writeColorR;
        mCurrColorMask[1] = writeColorG;
        mCurrColorMask[2] = writeColorB;
        mCurrColorMask[3] = writeAlpha;
    }
}

void LLRender::setClearColor(F32 r, F32 g, F32 b, F32 a)
{
    mClearColor[0] = r;
    mClearColor[1] = g;
    mClearColor[2] = b;
    mClearColor[3] = a;
}

void LLRender::setSceneBlendType(eBlendType type)
{
    switch (type)
    {
        case BT_ALPHA:
            blendFunc(BF_SOURCE_ALPHA, BF_ONE_MINUS_SOURCE_ALPHA);
            break;
        case BT_ADD:
            blendFunc(BF_ONE, BF_ONE);
            break;
        case BT_ADD_WITH_ALPHA:
            blendFunc(BF_SOURCE_ALPHA, BF_ONE);
            break;
        case BT_MULT:
            blendFunc(BF_DEST_COLOR, BF_ZERO);
            break;
        case BT_MULT_ALPHA:
            blendFunc(BF_DEST_ALPHA, BF_ZERO);
            break;
        case BT_MULT_X2:
            blendFunc(BF_DEST_COLOR, BF_SOURCE_COLOR);
            break;
        case BT_REPLACE:
            blendFunc(BF_ONE, BF_ZERO);
            break;
        default:
            LL_ERRS() << "Unknown Scene Blend Type: " << type << LL_ENDL;
            break;
    }
}

void LLRender::blendFunc(eBlendFactor sfactor, eBlendFactor dfactor)
{
    llassert(sfactor < BF_UNDEF);
    llassert(dfactor < BF_UNDEF);
    if (mCurrBlendColorSFactor != sfactor || mCurrBlendColorDFactor != dfactor ||
        mCurrBlendAlphaSFactor != sfactor || mCurrBlendAlphaDFactor != dfactor)
    {
        mCurrBlendColorSFactor = sfactor;
        mCurrBlendAlphaSFactor = sfactor;
        mCurrBlendColorDFactor = dfactor;
        mCurrBlendAlphaDFactor = dfactor;
        flush();
    }
}

void LLRender::blendFunc(eBlendFactor color_sfactor, eBlendFactor color_dfactor,
             eBlendFactor alpha_sfactor, eBlendFactor alpha_dfactor)
{
    llassert(color_sfactor < BF_UNDEF);
    llassert(color_dfactor < BF_UNDEF);
    llassert(alpha_sfactor < BF_UNDEF);
    llassert(alpha_dfactor < BF_UNDEF);

    if (mCurrBlendColorSFactor != color_sfactor || mCurrBlendColorDFactor != color_dfactor ||
        mCurrBlendAlphaSFactor != alpha_sfactor || mCurrBlendAlphaDFactor != alpha_dfactor)
    {
        mCurrBlendColorSFactor = color_sfactor;
        mCurrBlendAlphaSFactor = alpha_sfactor;
        mCurrBlendColorDFactor = color_dfactor;
        mCurrBlendAlphaDFactor = alpha_dfactor;
        flush();
    }
}

LLTexUnit* LLRender::getTexUnit(U32 index)
{
    if (index < mTexUnits.size())
    {
        return &mTexUnits[index];
    }
    else
    {
        LL_DEBUGS() << "Non-existing texture unit layer requested: " << index << LL_ENDL;
        return &mDummyTexUnit;
    }
}

void LLRender::clearStaleImageGLRefs(LLImageGL* victim)
{
    if (victim == nullptr)
    {
        return;
    }
    for (U32 i = 0; i < LL_NUM_TEXTURE_LAYERS; ++i)
    {
        if (gGL.mTexUnits[i].mCurrImageGL == victim)
        {
            gGL.mTexUnits[i].mCurrImageGL = nullptr;
            gGL.mTexUnits[i].mCurrVkHeapSlot = 0xFFFFFFFFu;
        }
    }
    if (gGL.mDummyTexUnit.mCurrImageGL == victim)
    {
        gGL.mDummyTexUnit.mCurrImageGL = nullptr;
        gGL.mDummyTexUnit.mCurrVkHeapSlot = 0xFFFFFFFFu;
    }
    if (sBufferDataList != nullptr)
    {
        for (auto& entry : *sBufferDataList)
        {
            if (entry.mImageGL == victim)
            {
                entry.mImageGL = nullptr;
            }
        }
    }
}

void LLRender::clearStaleCubeMapRefs(LLCubeMap* victim)
{
    if (victim == nullptr)
    {
        return;
    }
    for (U32 i = 0; i < LL_NUM_TEXTURE_LAYERS; ++i)
    {
        if (gGL.mTexUnits[i].mCurrCubeMap == victim)
        {
            gGL.mTexUnits[i].mCurrCubeMap = nullptr;
        }
    }
    if (gGL.mDummyTexUnit.mCurrCubeMap == victim)
    {
        gGL.mDummyTexUnit.mCurrCubeMap = nullptr;
    }
}

LLLightState* LLRender::getLight(U32 index)
{
    if (index < mLightState.size())
    {
        return &mLightState[index];
    }

    return NULL;
}

void LLRender::setAmbientLightColor(const LLColor4& color)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    if (color != mAmbientLightColor)
    {
        ++mLightHash;
        mAmbientLightColor = color;
    }
}

// <FS> Line width OGL core profile fix by Rye Mutt
void LLRender::setLineWidth(F32 line_width)
{
    if (line_width > 1.f)
    {
        line_width = llmin(line_width, mMaxLineWidthAliased);
    }
    if (mLineWidth != line_width || mDirty)
    {
        if (mMode == LLRender::LINES || mMode == LLRender::LINE_STRIP)
        {
            flush();
        }
        mLineWidth = line_width;
    }
}
// </FS>

void LLRender::setPolygonOffset(F32 factor, F32 units)
{
    mPolygonOffsetFactor = factor;
    mPolygonOffsetUnits  = units;
}

bool LLRender::verifyTexUnitActive(U32 unitToVerify)
{
    if (mCurrTextureUnitIndex == unitToVerify)
    {
        return true;
    }
    else
    {
        LL_WARNS() << "TexUnit currently active: " << mCurrTextureUnitIndex << " (expecting " << unitToVerify << ")" << LL_ENDL;
        return false;
    }
}

void LLRender::beginList(std::list<LLVertexBufferData> *list)
{
    if (sBufferDataList)
    {
        LL_ERRS() << "beginList called while another list is open." << LL_ENDL;
    }
    llassert(LLGLSLShader::sCurBoundShaderPtr == &gUIProgram);
    flush();
    sBufferDataList = list;
}

void LLRender::endList()
{
    if (sBufferDataList)
    {
        flush();
        sBufferDataList = nullptr;
    }
    else
    {
        llassert(false); // endList called without an open list
    }
}

void LLRender::begin(const GLuint& mode)
{
    if (mode != mMode)
    {
        if (mMode == LLRender::LINES ||
            mMode == LLRender::TRIANGLES ||
            mMode == LLRender::POINTS)
        {
            flush();
        }
        else if (mCount != 0)
        {
            LL_ERRS() << "gGL.begin() called redundantly." << LL_ENDL;
        }

        mMode = mode;
    }
}

void LLRender::end()
{
    if (mCount == 0)
    {
        return;
    }

    if ((mMode != LLRender::LINES &&
        mMode != LLRender::TRIANGLES &&
        mMode != LLRender::POINTS) ||
        mCount > 2048)
    {
        flush();
    }
}

void LLRender::flush()
{
    if (mCount > 0)
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
        llassert_always(LLGLSLShader::sCurBoundShaderPtr != nullptr);

        if (!mUIOffset.empty())
        {
            sUICalls++;
            sUIVerts += mCount;
        }

        U32 count = mCount;

        if (mMode == LLRender::TRIANGLES)
        {
            if (mCount%3 != 0)
            {
            count -= (mCount % 3);
            LL_WARNS() << "Incomplete triangle requested." << LL_ENDL;
            }
        }

        if (mMode == LLRender::LINES)
        {
            if (mCount%2 != 0)
            {
                count -= (mCount % 2);
                LL_WARNS() << "Incomplete line requested." << LL_ENDL;
            }
        }

        mCount = 0;

        if (mScratchVerts)
        {

            LLVertexBuffer *vb;

            U32 attribute_mask = LLGLSLShader::sCurBoundShaderPtr->mVkAttributeMask;

            if (sBufferDataList)
            {
                vb = genBuffer(attribute_mask, count);
                sBufferDataList->emplace_back(
                    vb,
                    mMode,
                    count,
                    gGL.getTexUnit(0)->mCurrImageGL,
                    mMatrix[MM_MODELVIEW][mMatIdx[MM_MODELVIEW]],
                    mMatrix[MM_PROJECTION][mMatIdx[MM_PROJECTION]],
                    mMatrix[MM_TEXTURE0][mMatIdx[MM_TEXTURE0]]
                    );
            }
            else
            {
                vb = bufferfromCache(attribute_mask, count);
            }

            LLGLSLShader::populateAndBindUniversalDescriptorSet();

            drawBuffer(vb, mMode, count);
        }
        else
        {
            LL_ERRS() << "A flush call from outside main rendering thread" << LL_ENDL;
        }

        resetStriders(count);
    }
}

LLVertexBuffer* LLRender::bufferfromCache(U32 attribute_mask, U32 count)
{
    LLVertexBuffer *vb = nullptr;
    HBXXH64 hash;

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("vb cache hash");

        hash.update((U8*)mVerticesp.get(), count * sizeof(LLVector4a));
        if (attribute_mask & LLVertexBuffer::MAP_TEXCOORD0)
        {
            hash.update((U8*)mTexcoordsp.get(), count * sizeof(LLVector2));
        }

        if (attribute_mask & LLVertexBuffer::MAP_COLOR)
        {
            hash.update((U8*)mColorsp.get(), count * sizeof(LLColor4U));
        }

        hash.finalize();
    }

    U64 vhash = hash.digest();

    std::unordered_map<U64, LLVBCache>::iterator cache = sVBCache.find(vhash);

    if (cache != sVBCache.end())
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("vb cache hit");
        vb = cache->second.vb;
        cache->second.touched = std::chrono::steady_clock::now();
    }
    else
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("vb cache miss");
        vb = genBuffer(attribute_mask, count);

        sVBCache[vhash] = { vb , std::chrono::steady_clock::now() };

        static U32 miss_count = 0;
        miss_count++;
        if (miss_count > 1024)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("vb cache clean");
            miss_count = 0;
            auto now = std::chrono::steady_clock::now();

            using namespace std::chrono_literals;
            for (std::unordered_map<U64, LLVBCache>::iterator iter = sVBCache.begin(); iter != sVBCache.end(); )
            {
                if (now - iter->second.touched > 1s)
                {
                    iter = sVBCache.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }
    }
    return vb;
}

LLVertexBuffer* LLRender::genBuffer(U32 attribute_mask, S32 count)
{
    LLVertexBuffer * vb = new LLVertexBuffer(attribute_mask);
    vb->allocateBuffer(count, 0);

    vb->setBuffer();

    vb->setPositionData(mVerticesp.get());

    if (attribute_mask & LLVertexBuffer::MAP_TEXCOORD0)
    {
        vb->setTexCoord0Data(mTexcoordsp.get());
    }

    if (attribute_mask & LLVertexBuffer::MAP_COLOR)
    {
        vb->setColorData(mColorsp.get());
    }

#if LL_DARWIN
    vb->unmapBuffer();
#endif
    vb->unbind();

    return vb;
}

void LLRender::drawBuffer(LLVertexBuffer* vb, U32 mode, S32 count)
{
    vb->setBuffer();
    vb->drawArrays(mode, 0, count);
}

void LLRender::resetStriders(S32 count)
{
    mVerticesp[0] = mVerticesp[count];
    mTexcoordsp[0] = mTexcoordsp[count];
    mColorsp[0] = mColorsp[count];

    mCount = 0;
}

void LLRender::vertex3f(const GLfloat& x, const GLfloat& y, const GLfloat& z)
{
    if (mCount > 2048)
    {
        switch (mMode)
        {
            case LLRender::POINTS: flush(); break;
            case LLRender::TRIANGLES: if (mCount%3==0) flush(); break;
            case LLRender::LINES: if (mCount%2 == 0) flush(); break;
        }
    }

    if (mCount > 4094)
    {
        return;
    }

    if (mUIOffset.empty())
    {
        mVerticesp[mCount].set(x,y,z);
    }
    else
    {
        LLVector4a vert(x, y, z);
        vert.add(mUIOffset.back());
        vert.mul(mUIScale.back());
        mVerticesp[mCount] = vert;
    }

    mCount++;
    mVerticesp[mCount] = mVerticesp[mCount-1];
    mColorsp[mCount] = mColorsp[mCount-1];
    mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
}

void LLRender::vertexBatchPreTransformed(LLVector4a* verts, S32 vert_count)
{
    if (mCount + vert_count > 4094)
    {
        return;
    }

    for (S32 i = 0; i < vert_count; i++)
    {
        mVerticesp[mCount] = verts[i];

        mCount++;
        mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
        mColorsp[mCount] = mColorsp[mCount-1];
    }

    if( mCount > 0 )
        mVerticesp[mCount] = mVerticesp[mCount-1];
}

void LLRender::vertexBatchPreTransformed(LLVector4a* verts, LLVector2* uvs, S32 vert_count)
{
    if (mCount + vert_count > 4094)
    {
        return;
    }

    for (S32 i = 0; i < vert_count; i++)
    {
        mVerticesp[mCount] = verts[i];
        mTexcoordsp[mCount] = uvs[i];

        mCount++;
        mColorsp[mCount] = mColorsp[mCount-1];
    }

    if (mCount > 0)
    {
        mVerticesp[mCount] = mVerticesp[mCount - 1];
        mTexcoordsp[mCount] = mTexcoordsp[mCount - 1];
    }
}

void LLRender::vertexBatchPreTransformed(LLVector4a* verts, LLVector2* uvs, LLColor4U* colors, S32 vert_count)
{
    if (mCount + vert_count > 4094)
    {
        return;
    }

    for (S32 i = 0; i < vert_count; i++)
    {
        mVerticesp[mCount] = verts[i];
        mTexcoordsp[mCount] = uvs[i];
        mColorsp[mCount] = colors[i];

        mCount++;
    }

    if (mCount > 0)
    {
        mVerticesp[mCount] = mVerticesp[mCount - 1];
        mTexcoordsp[mCount] = mTexcoordsp[mCount - 1];
        mColorsp[mCount] = mColorsp[mCount - 1];
    }
}

void LLRender::vertex2i(const GLint& x, const GLint& y)
{
    vertex3f((GLfloat) x, (GLfloat) y, 0);
}

void LLRender::vertex2f(const GLfloat& x, const GLfloat& y)
{
    vertex3f(x,y,0);
}

void LLRender::vertex2fv(const GLfloat* v)
{
    vertex3f(v[0], v[1], 0);
}

void LLRender::vertex3fv(const GLfloat* v)
{
    vertex3f(v[0], v[1], v[2]);
}

void LLRender::texCoord2f(const GLfloat& x, const GLfloat& y)
{
    mTexcoordsp[mCount] = LLVector2(x,y);
}

void LLRender::texCoord2i(const GLint& x, const GLint& y)
{
    texCoord2f((GLfloat) x, (GLfloat) y);
}

void LLRender::texCoord2fv(const GLfloat* tc)
{
    texCoord2f(tc[0], tc[1]);
}

void LLRender::color4ub(const GLubyte& r, const GLubyte& g, const GLubyte& b, const GLubyte& a)
{
    if (!LLGLSLShader::sCurBoundShaderPtr || LLGLSLShader::sCurBoundShaderPtr->mVkAttributeMask & LLVertexBuffer::MAP_COLOR)
    {
        mColorsp[mCount] = LLColor4U(r,g,b,a);
    }
    else
    {
        diffuseColor4ub(r,g,b,a);
    }
}
void LLRender::color4ubv(const GLubyte* c)
{
    color4ub(c[0], c[1], c[2], c[3]);
}

void LLRender::color4f(const GLfloat& r, const GLfloat& g, const GLfloat& b, const GLfloat& a)
{
    color4ub((GLubyte) (llclamp(r, 0.f, 1.f)*255),
        (GLubyte) (llclamp(g, 0.f, 1.f)*255),
        (GLubyte) (llclamp(b, 0.f, 1.f)*255),
        (GLubyte) (llclamp(a, 0.f, 1.f)*255));
}

void LLRender::color4fv(const GLfloat* c)
{
    color4f(c[0],c[1],c[2],c[3]);
}

void LLRender::color3f(const GLfloat& r, const GLfloat& g, const GLfloat& b)
{
    color4f(r,g,b,1);
}

void LLRender::color3fv(const GLfloat* c)
{
    color4f(c[0],c[1],c[2],1);
}

void LLRender::diffuseColor3f(F32 r, F32 g, F32 b)
{
    LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
    llassert(shader != NULL);

    if (shader)
    {
        if (LLVKLoader::isVulkanInitialized())
        {
            LLVKLoader::DrawColor_PerShaderBind draw_color = { r, g, b, 1.f };
            LLVKLoader::writeCurrentDrawColorUBO(draw_color);
        }
    }
}

void LLRender::diffuseColor3fv(const F32* c)
{
    LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
    llassert(shader != NULL);

    if (shader)
    {
        if (LLVKLoader::isVulkanInitialized())
        {
            LLVKLoader::DrawColor_PerShaderBind draw_color = { c[0], c[1], c[2], 1.f };
            LLVKLoader::writeCurrentDrawColorUBO(draw_color);
        }
    }
}

void LLRender::diffuseColor4f(F32 r, F32 g, F32 b, F32 a)
{
    LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
    llassert(shader != NULL);

    if (shader)
    {
        if (LLVKLoader::isVulkanInitialized())
        {
            LLVKLoader::DrawColor_PerShaderBind draw_color = { r, g, b, a };
            LLVKLoader::writeCurrentDrawColorUBO(draw_color);
        }
    }
}

void LLRender::diffuseColor4fv(const F32* c)
{
    LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
    llassert(shader != NULL);

    if (shader)
    {
        if (LLVKLoader::isVulkanInitialized())
        {
            LLVKLoader::DrawColor_PerShaderBind draw_color = { c[0], c[1], c[2], c[3] };
            LLVKLoader::writeCurrentDrawColorUBO(draw_color);
        }
    }
}

void LLRender::diffuseColor4ubv(const U8* c)
{
    LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
    llassert(shader != NULL);

    if (shader)
    {
        if (LLVKLoader::isVulkanInitialized())
        {
            LLVKLoader::DrawColor_PerShaderBind draw_color = { c[0]/255.f, c[1]/255.f, c[2]/255.f, c[3]/255.f };
            LLVKLoader::writeCurrentDrawColorUBO(draw_color);
        }
    }
}

void LLRender::diffuseColor4ub(U8 r, U8 g, U8 b, U8 a)
{
    LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
    llassert(shader != NULL);

    if (shader)
    {
        if (LLVKLoader::isVulkanInitialized())
        {
            LLVKLoader::DrawColor_PerShaderBind draw_color = { r/255.f, g/255.f, b/255.f, a/255.f };
            LLVKLoader::writeCurrentDrawColorUBO(draw_color);
        }
    }
}


glm::mat4 get_current_modelview()
{
    return glm::make_mat4(gGLModelView);
}

glm::mat4 get_current_projection()
{
    return glm::make_mat4(gGLProjection);
}

glm::mat4 get_last_modelview()
{
    return glm::make_mat4(gGLLastModelView);
}

glm::mat4 get_last_projection()
{
    return glm::make_mat4(gGLLastProjection);
}

void copy_matrix(const glm::mat4& src, F32* dst)
{
    auto matp = glm::value_ptr(src);
    for (U32 i = 0; i < 16; i++)
    {
        dst[i] = matp[i];
    }
}

void set_current_modelview(const glm::mat4& mat)
{
    copy_matrix(mat, gGLModelView);
}

void set_current_projection(const glm::mat4& mat)
{
    copy_matrix(mat, gGLProjection);
}

void set_last_modelview(const glm::mat4& mat)
{
    copy_matrix(mat, gGLLastModelView);
}

void set_last_projection(const glm::mat4& mat)
{
    copy_matrix(mat, gGLLastProjection);
}

glm::vec3 mul_mat4_vec3(const glm::mat4& mat, const glm::vec3& vec)
{
    const float w = vec[0] * mat[0][3] + vec[1] * mat[1][3] + vec[2] * mat[2][3] + mat[3][3];
    return glm::vec3(
       (vec[0] * mat[0][0] + vec[1] * mat[1][0] + vec[2] * mat[2][0] + mat[3][0]) / w,
       (vec[0] * mat[0][1] + vec[1] * mat[1][1] + vec[2] * mat[2][1] + mat[3][1]) / w,
       (vec[0] * mat[0][2] + vec[1] * mat[1][2] + vec[2] * mat[2][2] + mat[3][2]) / w
    );
}
