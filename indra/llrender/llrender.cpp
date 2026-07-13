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
#include "llimagegl.h"
#include "llrendertarget.h"
#include "lltexture.h"
#include "llshadermgr.h"
#include "llvkloader.h"
#include "hbxxh.h"
#include "glm/gtc/type_ptr.hpp"
#include <cstring>

thread_local LLRender gGL;

// Handy copies of last good GL matrices
F32 gGLModelView[16];
F32 gGLLastModelView[16];
F32 gGLLastProjection[16];
F32 gGLProjection[16];

// transform from last frame's camera space to this frame's camera space (and inverse)
glm::mat4 gGLDeltaModelView;
glm::mat4 gGLInverseDeltaModelView;

S32 gGLViewport[4];

void llSetGLViewport(S32 x, S32 y, S32 w, S32 h)
{
    glViewport(x, y, w, h);
    LLVKLoader::setRenderViewport(x, y, w, h);
}


U32 LLRender::sUICalls = 0;
U32 LLRender::sUIVerts = 0;
bool LLRender::sGLCoreProfile = false;
bool LLRender::sNsightDebugSupport = false;
LLVector2 LLRender::sUIGLScaleFactor = LLVector2(1.f, 1.f);

struct LLVBCache
{
    LLPointer<LLVertexBuffer> vb;
    std::chrono::steady_clock::time_point touched;
};

static std::unordered_map<U64, LLVBCache> sVBCache;
static thread_local std::list<LLVertexBufferData> *sBufferDataList = nullptr;

static const GLenum sGLTextureType[] =
{
    GL_TEXTURE_2D,
    GL_TEXTURE_RECTANGLE,
    GL_TEXTURE_CUBE_MAP,
    GL_TEXTURE_CUBE_MAP_ARRAY,
    GL_TEXTURE_2D_MULTISAMPLE,
    GL_TEXTURE_3D
};

const U32 immediate_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR | LLVertexBuffer::MAP_TEXCOORD0;

static const GLenum sGLBlendFactor[] =
{
    GL_ONE,
    GL_ZERO,
    GL_DST_COLOR,
    GL_SRC_COLOR,
    GL_ONE_MINUS_DST_COLOR,
    GL_ONE_MINUS_SRC_COLOR,
    GL_DST_ALPHA,
    GL_SRC_ALPHA,
    GL_ONE_MINUS_DST_ALPHA,
    GL_ONE_MINUS_SRC_ALPHA,

    GL_ZERO // 'BF_UNDEF'
};

LLTexUnit::LLTexUnit(S32 index)
    : mCurrTexType(TT_NONE),
    mHasMipMaps(false),
    mIndex(index)
{
    llassert_always(index < (S32)LL_NUM_TEXTURE_LAYERS);
}

//static
U32 LLTexUnit::getInternalType(eTextureType type)
{
    return sGLTextureType[type];
}

void LLTexUnit::refreshState(void)
{
    // We set dirty to true so that the tex unit knows to ignore caching
    // and we reset the cached tex unit state

    gGL.flush();

    glActiveTexture(GL_TEXTURE0 + mIndex);
}

void LLTexUnit::activate(void)
{
    if (mIndex < 0) return;

    if ((S32)gGL.mCurrTextureUnitIndex != mIndex || gGL.mDirty)
    {
        gGL.flush();
        glActiveTexture(GL_TEXTURE0 + mIndex);
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
            disable(); // Force a disable of a previous texture type if it's enabled.
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
        //if deleted, will re-generate it immediately
        texture->forceImmediateUpdate();
        gl_tex->forceUpdateBindStats();
        texture->bindDefaultImage(mIndex);
    }
    mHasMipMaps = gl_tex->mHasMipMaps;
    if (gl_tex->mTexOptionsDirty)
    {
        gl_tex->mTexOptionsDirty = false;
        setTextureAddressModeFast(gl_tex->mAddressMode, gl_tex->getTarget());
        setTextureFilteringOptionFast(gl_tex->mFilterOption, gl_tex->getTarget());
    }
    mCurrImageGL = gl_tex;
    mCurrRenderTarget = nullptr;
    mCurrCubeMap      = nullptr;
    mCurrCompareMode  = false;
    mCurrAddressMode  = gl_tex->getAddressMode();
    mCurrFilterOption = gl_tex->getFilteringOption();
    vkNotifyShaderChannelBound();
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
                mCurrRenderTarget = nullptr;
                mCurrCubeMap      = nullptr;
                mCurrCompareMode  = false;
                mCurrAddressMode  = gl_tex->getAddressMode();
                mCurrFilterOption = gl_tex->getFilteringOption();
            }
            else
            {
                //if deleted, will re-generate it immediately
                texture->forceImmediateUpdate() ;

                gl_tex->forceUpdateBindStats() ;
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
    { // mIndex < 0
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
        if(LLImageGL::sDefaultGLTexture && LLImageGL::sDefaultGLTexture->hasVkImage())
        {
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
    mCurrRenderTarget = nullptr;
    mCurrCubeMap      = nullptr;
    mCurrCompareMode  = false;
    mCurrAddressMode  = texture->getAddressMode();
    mCurrFilterOption = texture->getFilteringOption();


    vkNotifyShaderChannelBound();
    return true;
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
    mCurrRenderTarget = nullptr;
    mCurrCompareMode  = false;
    mCurrAddressMode  = cubeMap->mImages[0]->getAddressMode();
    mCurrFilterOption = cubeMap->mImages[0]->getFilteringOption();
    vkNotifyShaderChannelBound();
    return true;
}

bool LLTexUnit::bind(LLRenderTarget* renderTarget, bool bindDepth)
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
    mCurrCompareMode  = bindDepth && renderTarget->usesDepthCompareSampler();
    mCurrImageGL      = nullptr;
    mCurrCubeMap      = nullptr;
    mCurrAddressMode  = TAM_WRAP;
    mCurrFilterOption = TFO_BILINEAR;

    renderTarget->bindForShaderRead(mCurrRTAttachment, bindDepth);

    vkNotifyShaderChannelBound();
    return true;
}

VkImageView LLTexUnit::getLiveVkImageView() const
{
    if (mCurrRenderTarget != nullptr)
    {
        const bool same_pass = (mCurrRenderTarget == LLRenderTarget::getCurrentBoundTarget());
        if (same_pass)
        {
            return VK_NULL_HANDLE;
        }
        mCurrRenderTarget->bindForShaderRead(mCurrRTAttachment, mCurrRTDepth);
        if (mCurrRTDepth)
        {
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
    mCurrRenderTarget = nullptr;
    mCurrCubeMap = nullptr;
    return true;
}

void LLTexUnit::unbind(eTextureType type)
{

    if (mIndex < 0) return;

    //always flush and activate for consistency
    //   some code paths assume unbind always flushes and sets the active texture
    gGL.flush();
    activate();

    // Disabled caching of binding state.
    if (mCurrTexType == type)
    {
        mCurrImageGL = nullptr;
        mCurrRenderTarget = nullptr;
        mCurrCubeMap = nullptr;

        vkNotifyShaderChannelBound();
    }
}

void LLTexUnit::unbindFast(eTextureType type)
{
    activate();

    // Disabled caching of binding state.
    if (mCurrTexType == type)
    {
        mCurrImageGL = nullptr;
        mCurrRenderTarget = nullptr;
        mCurrCubeMap = nullptr;

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

GLint LLTexUnit::getTextureSource(eTextureBlendSrc src)
{
    switch(src)
    {
        // All four cases should return the same value.
        case TBS_PREV_COLOR:
        case TBS_PREV_ALPHA:
        case TBS_ONE_MINUS_PREV_COLOR:
        case TBS_ONE_MINUS_PREV_ALPHA:
            return GL_PREVIOUS;

        // All four cases should return the same value.
        case TBS_TEX_COLOR:
        case TBS_TEX_ALPHA:
        case TBS_ONE_MINUS_TEX_COLOR:
        case TBS_ONE_MINUS_TEX_ALPHA:
            return GL_TEXTURE;

        // All four cases should return the same value.
        case TBS_VERT_COLOR:
        case TBS_VERT_ALPHA:
        case TBS_ONE_MINUS_VERT_COLOR:
        case TBS_ONE_MINUS_VERT_ALPHA:
            return GL_PRIMARY_COLOR;

        // All four cases should return the same value.
        case TBS_CONST_COLOR:
        case TBS_CONST_ALPHA:
        case TBS_ONE_MINUS_CONST_COLOR:
        case TBS_ONE_MINUS_CONST_ALPHA:
            return GL_CONSTANT;

        default:
            LL_WARNS() << "Unknown eTextureBlendSrc: " << src << ".  Using Vertex Color instead." << LL_ENDL;
            return GL_PRIMARY_COLOR;
    }
}

GLint LLTexUnit::getTextureSourceType(eTextureBlendSrc src, bool isAlpha)
{
    switch(src)
    {
        // All four cases should return the same value.
        case TBS_PREV_COLOR:
        case TBS_TEX_COLOR:
        case TBS_VERT_COLOR:
        case TBS_CONST_COLOR:
            return (isAlpha) ? GL_SRC_ALPHA: GL_SRC_COLOR;

        // All four cases should return the same value.
        case TBS_PREV_ALPHA:
        case TBS_TEX_ALPHA:
        case TBS_VERT_ALPHA:
        case TBS_CONST_ALPHA:
            return GL_SRC_ALPHA;

        // All four cases should return the same value.
        case TBS_ONE_MINUS_PREV_COLOR:
        case TBS_ONE_MINUS_TEX_COLOR:
        case TBS_ONE_MINUS_VERT_COLOR:
        case TBS_ONE_MINUS_CONST_COLOR:
            return (isAlpha) ? GL_ONE_MINUS_SRC_ALPHA : GL_ONE_MINUS_SRC_COLOR;

        // All four cases should return the same value.
        case TBS_ONE_MINUS_PREV_ALPHA:
        case TBS_ONE_MINUS_TEX_ALPHA:
        case TBS_ONE_MINUS_VERT_ALPHA:
        case TBS_ONE_MINUS_CONST_ALPHA:
            return GL_ONE_MINUS_SRC_ALPHA;

        default:
            LL_WARNS() << "Unknown eTextureBlendSrc: " << src << ".  Using Source Color or Alpha instead." << LL_ENDL;
            return (isAlpha) ? GL_SRC_ALPHA: GL_SRC_COLOR;
    }
}

// Useful for debugging that you've manually assigned a texture operation to the correct
// texture unit based on the currently set active texture in opengl.
void LLTexUnit::debugTextureUnit(void)
{
    if (mIndex < 0) return;

    GLint activeTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
    if ((GL_TEXTURE0 + mIndex) != activeTexture)
    {
        U32 set_unit = (activeTexture - GL_TEXTURE0);
        LL_WARNS() << "Incorrect Texture Unit!  Expected: " << set_unit << " Actual: " << mIndex << LL_ENDL;
    }
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
    //always set position because modelview matrix may have changed
    ++gGL.mLightHash;
    mPosition = position;
    //transform position by current modelview matrix
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
    //always set direction because modelview matrix may have changed
    ++gGL.mLightHash;

    //transform direction by current modelview matrix
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
    }

    mLightHash = 0;
}

LLRender::~LLRender()
{
    shutdown();
}

bool LLRender::init(bool needs_vertex_buffer)
{
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    gGL.setSceneBlendType(LLRender::BT_ALPHA);
    gGL.setAmbientLightColor(LLColor4::black);

    LLGLState::setCullFaceMode(GL_BACK);

    // necessary for reflection maps
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

#if LL_WINDOWS
    if (glGenVertexArrays == nullptr)
    {
        return false;
    }
#endif

    { //bind a dummy vertex array object so we're core profile compliant
        U32 ret;
        glGenVertexArrays(1, &ret);
        glBindVertexArray(ret);
    }

    if (needs_vertex_buffer)
    {
        initVertexBuffer();
    }

    // <FS:Ansariel> Don't ignore OpenGL max line width
    GLfloat range[2];
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, range);
    mMaxLineWidthAliased = range[1];
    glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, range);
    mMaxLineWidthSmooth = range[1];
    // </FS:Ansariel>

    return true;
}

void LLRender::initVertexBuffer()
{
    llassert_always(mBuffer.isNull()) ;
    mBuffer = new LLVertexBuffer(immediate_mask);
    // <FS:Ansariel> Warn in case of allocation failure
    //mBuffer->allocateBuffer(4096, 0);
    if (!mBuffer->allocateBuffer(4096, 0))
    {
        // If this doesn't work, we're knee-deep in trouble!
        LL_WARNS() << "Failed to allocate Vertex Buffer for common rendering" << LL_ENDL;
    }
    mBuffer->getVertexStrider(mVerticesp);
    mBuffer->getTexCoord0Strider(mTexcoordsp);
    mBuffer->getColorStrider(mColorsp);
}

void LLRender::resetVertexBuffer()
{
    mBuffer = NULL;
}

void LLRender::shutdown()
{
    resetVertexBuffer();
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

            const glm::mat4 inv_proj = glm::inverse(proj_mat);
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

            LLVKLoader::writeCurrentPerFrameMatrixUBO(perframe, texmat);

            const glm::mat4& modelview_mat = mMatrix[MM_MODELVIEW][mMatIdx[MM_MODELVIEW]];
            LLVKLoader::pushCurrentModelviewMatrix(glm::value_ptr(modelview_mat));
        }

        if (shader->mFeatures.hasLighting || shader->mFeatures.calculatesLighting || shader->mFeatures.calculatesAtmospherics)
        { //also sync light state
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
        // the shaders don't actually reference anything beyond texture_matrix0/1 outside of terrain rendering
        llassert(tex_index <= 3);
        mode = eMatrixMode(MM_TEXTURE0 + tex_index);
        if (mode > MM_TEXTURE3)
        {
            // getCurrentTexUnitIndex() can go as high as 32 (LL_NUM_TEXTURE_LAYERS)
            // Large value will result in a crash at mMatrix
            LL_WARNS_ONCE() << "Attempted to assign matrix mode out of bounds: " << mode << LL_ENDL;
            mode = MM_TEXTURE0;
        }
    }

    mMatrixMode = mode;
}

LLRender::eMatrixMode LLRender::getMatrixMode()
{
    if (mMatrixMode >= MM_TEXTURE0 && mMatrixMode <= MM_TEXTURE3)
    { //always return MM_TEXTURE if current matrix mode points at any texture matrix
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

        glColorMask(writeColorR ? GL_TRUE : GL_FALSE,
                    writeColorG ? GL_TRUE : GL_FALSE,
                    writeColorB ? GL_TRUE : GL_FALSE,
                    writeAlpha ? GL_TRUE : GL_FALSE);
    }
}

void LLRender::setClearColor(F32 r, F32 g, F32 b, F32 a)
{
    mClearColor[0] = r;
    mClearColor[1] = g;
    mClearColor[2] = b;
    mClearColor[3] = a;

    glClearColor(r, g, b, a);
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
        glBlendFunc(sGLBlendFactor[sfactor], sGLBlendFactor[dfactor]);
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

        glBlendFuncSeparate(sGLBlendFactor[color_sfactor], sGLBlendFactor[color_dfactor],
                           sGLBlendFactor[alpha_sfactor], sGLBlendFactor[alpha_dfactor]);
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
        }
    }
    if (gGL.mDummyTexUnit.mCurrImageGL == victim)
    {
        gGL.mDummyTexUnit.mCurrImageGL = nullptr;
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
        line_width = llmin(line_width, glIsEnabled(GL_LINE_SMOOTH) ? mMaxLineWidthSmooth : mMaxLineWidthAliased);
    }
    if (mLineWidth != line_width || mDirty)
    {
        if (mMode == LLRender::LINES || mMode == LLRender::LINE_STRIP)
        {
            flush();
        }
        mLineWidth = line_width;
        glLineWidth(line_width);
    }
}
// </FS>

void LLRender::setPolygonOffset(F32 factor, F32 units)
{
    mPolygonOffsetFactor = factor;
    mPolygonOffsetUnits  = units;
    glPolygonOffset(factor, units);
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

void LLRender::clearErrors()
{
    while (glGetError())
    {
        //loop until no more error flags left
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
        //IMM_ERRS << "GL begin and end called with no vertices specified." << LL_ENDL;
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

        //store mCount in a local variable to avoid re-entrance (drawArrays may call flush)
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

        if (mBuffer)
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

            LLGLSLShader::sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
        }
        else
        {
            // mBuffer is present in main thread and not present in an image thread
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

    // check the VB cache before making a new vertex buffer
    // This is a giant hack to deal with (mostly) our terrible UI rendering code
    // that was built on top of OpenGL immediate mode.  Huge performance wins
    // can be had by not uploading geometry to VRAM unless absolutely necessary.
    // Most of our usage of the "immediate mode" style draw calls is actually
    // sending the same geometry over and over again.
    // To leverage this, we maintain a running hash of the vertex stream being
    // built up before a flush, and then check that hash against a VB
    // cache just before creating a vertex buffer in VRAM
    std::unordered_map<U64, LLVBCache>::iterator cache = sVBCache.find(vhash);

    if (cache != sVBCache.end())
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("vb cache hit");
        // cache hit, just use the cached buffer
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
            // every 1024 misses, clean the cache of any VBs that haven't been touched in the last second
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
    //the range of mVerticesp, mColorsp and mTexcoordsp is [0, 4095]
    if (mCount > 2048)
    { //break when buffer gets reasonably full to keep GL command buffers happy and avoid overflow below
        switch (mMode)
        {
            case LLRender::POINTS: flush(); break;
            case LLRender::TRIANGLES: if (mCount%3==0) flush(); break;
            case LLRender::LINES: if (mCount%2 == 0) flush(); break;
        }
    }

    if (mCount > 4094)
    {
    //  LL_WARNS() << "GL immediate mode overflow.  Some geometry not drawn." << LL_ENDL;
        return;
    }

    LLVector4a vert(x, y, z);
    transform(vert);
    mVerticesp[mCount] = vert;

    mCount++;
    mVerticesp[mCount] = mVerticesp[mCount-1];
    mColorsp[mCount] = mColorsp[mCount-1];
    mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
}

void LLRender::transform(LLVector3& vert)
{
    if (!mUIOffset.empty())
    {
        vert += LLVector3(mUIOffset.back().getF32ptr());
        vert *= LLVector3(mUIScale.back().getF32ptr());
    }
}

void LLRender::transform(LLVector4a& vert)
{
    if (!mUIOffset.empty())
    {
        vert.add(mUIOffset.back());
        vert.mul(mUIScale.back());
    }
}

void LLRender::untransform(LLVector3& vert)
{
    if (!mUIOffset.empty())
    {
        vert /= LLVector3(mUIScale.back().getF32ptr());
        vert -= LLVector3(mUIOffset.back().getF32ptr());
    }
}

void LLRender::batchTransform(LLVector4a* verts, U32 vert_count)
{
    if (!mUIOffset.empty())
    {
        const LLVector4a& offset = mUIOffset.back();
        const LLVector4a& scale = mUIScale.back();

        for (U32 i = 0; i < vert_count; ++i)
        {
            verts[i].add(offset);
            verts[i].mul(scale);
        }
    }
}

void LLRender::vertexBatchPreTransformed(const std::vector<LLVector4a>& verts)
{
    vertexBatchPreTransformed(verts.data(), narrow(verts.size()));
}

void LLRender::vertexBatchPreTransformed(const LLVector4a* verts, S32 vert_count)
{
    if (mCount + vert_count > 4094)
    {
        //  LL_WARNS() << "GL immediate mode overflow.  Some geometry not drawn." << LL_ENDL;
        return;
    }

    for (S32 i = 0; i < vert_count; i++)
    {
        mVerticesp[mCount] = verts[i];

        mCount++;
        mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
        mColorsp[mCount] = mColorsp[mCount-1];
    }

    if( mCount > 0 ) // ND: Guard against crashes if mCount is zero, yes it can happen
        mVerticesp[mCount] = mVerticesp[mCount-1];
}

void LLRender::vertexBatchPreTransformed(const LLVector4a* verts, const LLVector2* uvs, S32 vert_count)
{
    if (mCount + vert_count > 4094)
    {
        //  LL_WARNS() << "GL immediate mode overflow.  Some geometry not drawn." << LL_ENDL;
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

void LLRender::vertexBatchPreTransformed(const LLVector4a* verts, const LLVector2* uvs, const LLColor4U* colors, S32 vert_count)
{
    if (mCount + vert_count > 4094)
    {
        //  LL_WARNS() << "GL immediate mode overflow.  Some geometry not drawn." << LL_ENDL;
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
    { //not using shaders or shader reads color from a uniform
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


void LLRender::debugTexUnits(void)
{
    LL_INFOS("TextureUnit") << "Active TexUnit: " << mCurrTextureUnitIndex << LL_ENDL;
    std::string active_enabled = "false";
    for (U32 i = 0; i < mTexUnits.size(); i++)
    {
        if (getTexUnit(i)->mCurrTexType != LLTexUnit::TT_NONE)
        {
            if (i == mCurrTextureUnitIndex) active_enabled = "true";
            LL_INFOS("TextureUnit") << "TexUnit: " << i << " Enabled" << LL_ENDL;
            LL_INFOS("TextureUnit") << "Enabled As: " ;
            switch (getTexUnit(i)->mCurrTexType)
            {
                case LLTexUnit::TT_TEXTURE:
                    LL_CONT << "Texture 2D";
                    break;
                case LLTexUnit::TT_RECT_TEXTURE:
                    LL_CONT << "Texture Rectangle";
                    break;
                case LLTexUnit::TT_CUBE_MAP:
                    LL_CONT << "Cube Map";
                    break;
                default:
                    LL_CONT << "ARGH!!! NONE!";
                    break;
            }
            LL_CONT << ", Texture Bound: " << getTexUnit(i)->mCurrImageGL << LL_ENDL;
        }
    }
    LL_INFOS("TextureUnit") << "Active TexUnit Enabled : " << active_enabled << LL_ENDL;
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
#if 1 // SIMD path results in strange crashes. Fall back to scalar for now.
    const float w = vec[0] * mat[0][3] + vec[1] * mat[1][3] + vec[2] * mat[2][3] + mat[3][3];
    return glm::vec3(
       (vec[0] * mat[0][0] + vec[1] * mat[1][0] + vec[2] * mat[2][0] + mat[3][0]) / w,
       (vec[0] * mat[0][1] + vec[1] * mat[1][1] + vec[2] * mat[2][1] + mat[3][1]) / w,
       (vec[0] * mat[0][2] + vec[1] * mat[1][2] + vec[2] * mat[2][2] + mat[3][2]) / w
    );
#else
    LLVector4a x, y, z, s, t, p, q;

    x.splat(vec.x);
    y.splat(vec.y);
    z.splat(vec.z);

    s.splat<3>(mat[0].data);
    t.splat<3>(mat[1].data);
    p.splat<3>(mat[2].data);
    q.splat<3>(mat[3].data);

    s.mul(x);
    t.mul(y);
    p.mul(z);
    q.add(s);
    t.add(p);
    q.add(t);

    x.mul(mat[0].data);
    y.mul(mat[1].data);
    z.mul(mat[2].data);

    x.add(y);
    z.add(mat[3].data);
    LLVector4a res;
    res.load3(glm::value_ptr(vec));
    res.setAdd(x, z);
    res.div(q);
    return glm::make_vec3(res.getF32ptr());
#endif
}
