/**
 * @file llimagegl.cpp
 * @brief Generic GL image handler
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

#include "llimagegl.h"

#include "llerror.h"
#include "llfasttimer.h"
#include "llimage.h"

#include "llmath.h"
#include "llgl.h"
#include "llglslshader.h"
#include "llrender.h"
#include "llrendertarget.h"
#include "llvkloader.h"
#include "llwindow.h"
#include "llframetimer.h"
#include <unordered_set>
#include <atomic>

extern LL_COMMON_API bool on_main_thread();

#if !LL_IMAGEGL_THREAD_CHECK
#define checkActiveThread()
#endif

const F32 MIN_TEXTURE_LIFETIME = 10.f;

U32 wpo2(U32 i);




static U16 float32ToHalf16(F32 f)
{
    U32 x;
    memcpy(&x, &f, sizeof(F32));

    const U32 sign     = (x >> 16) & 0x8000u;
    const S32 exp_in   = (S32)((x >> 23) & 0xFFu);
    const U32 mant_in  = x & 0x7FFFFFu;

    if (exp_in == 0xFF)
    {
        return (U16)(sign | 0x7C00u | (mant_in != 0 ? 0x0200u : 0u));
    }

    const S32 exp_half = exp_in - 127 + 15;

    if (exp_half >= 0x1F)
    {
        return (U16)(sign | 0x7C00u);
    }

    if (exp_half <= 0)
    {
        if (exp_half < -10)
        {
            return (U16)sign;
        }
        const U32 mant_full = mant_in | 0x800000u;
        const U32 shift     = (U32)(14 - exp_half);
        const U32 half_mant = mant_full >> shift;
        const U32 remainder = mant_full & ((1u << shift) - 1u);
        const U32 halfway   = 1u << (shift - 1);
        U32 result = half_mant;
        if (remainder > halfway || (remainder == halfway && (half_mant & 1u)))
        {
            ++result;
        }
        return (U16)(sign | result);
    }

    const U32 half_mant = mant_in >> 13;
    const U32 remainder = mant_in & 0x1FFFu;
    U32 result = (U32)(exp_half << 10) | half_mant;
    if (remainder > 0x1000u || (remainder == 0x1000u && (half_mant & 1u)))
    {
        ++result;
    }
    return (U16)(sign | result);
}

static U8* createVulkanFloat32ToHalf16Buffer(const U8* source_data,
                                             U32       source_components,
                                             U32       target_components,
                                             U32       pixel_count,
                                             U32&      out_size_bytes)
{
    out_size_bytes = 0;
    if (source_data == nullptr || pixel_count == 0 ||
        source_components == 0 || target_components == 0)
    {
        return nullptr;
    }

    const U64 target_size = (U64)target_components * 2u * (U64)pixel_count;
    U8* buf = new (std::nothrow) U8[(size_t)target_size];
    if (buf == nullptr)
    {
        LL_WARNS_ONCE("Vulkan") << "createVulkanFloat32ToHalf16Buffer: alloc failed size="
                                << (S32)target_size << LL_ENDL;
        return nullptr;
    }

    const F32* src = reinterpret_cast<const F32*>(source_data);
    U16*       dst = reinterpret_cast<U16*>(buf);
    for (U32 px = 0; px < pixel_count; ++px)
    {
        for (U32 c = 0; c < target_components; ++c)
        {
            F32 v;
            if (c < source_components)
            {
                v = src[(U64)px * source_components + c];
            }
            else
            {
                v = (c == 3) ? 1.0f : 0.0f;
            }
            dst[(U64)px * target_components + c] = float32ToHalf16(v);
        }
    }

    out_size_bytes = (U32)target_size;
    return buf;
}

static U8* createVulkanPaddingBuffer(const U8* source_data,
                                     U32       source_components,
                                     U32       source_component_bytes,
                                     U32       target_bytes_per_pixel,
                                     U32       pixel_count,
                                     U32&      out_padded_size_bytes)
{
    out_padded_size_bytes = 0;
    if (source_data == nullptr || pixel_count == 0 ||
        source_components == 0 || target_bytes_per_pixel == 0)
    {
        return nullptr;
    }

    const U32 source_bytes_per_pixel = source_components * source_component_bytes;
    if (source_bytes_per_pixel == target_bytes_per_pixel)
    {
        return nullptr;
    }

    const U64 target_size = (U64)target_bytes_per_pixel * (U64)pixel_count;
    U8* padded = new (std::nothrow) U8[target_size];
    if (padded == nullptr)
    {
        LL_WARNS_ONCE("Vulkan") << "createVulkanPaddingBuffer: alloc failed size="
                                << (S32)target_size << LL_ENDL;
        return nullptr;
    }

    const U32 target_component_count = (source_component_bytes > 0)
                                       ? (target_bytes_per_pixel / source_component_bytes)
                                       : 4;

    for (U32 px = 0; px < pixel_count; ++px)
    {
        U8* dst_pixel = padded + (U64)px * (U64)target_bytes_per_pixel;
        const U8* src_pixel = source_data + (U64)px * (U64)source_bytes_per_pixel;

        const U32 copy_bytes = source_components * source_component_bytes;
        memcpy(dst_pixel, src_pixel, copy_bytes);

        if (source_components == 1 && target_component_count >= 3)
        {
            for (U32 c = 1; c < 3 && c < target_component_count; ++c)
            {
                memcpy(dst_pixel + c * source_component_bytes, src_pixel, source_component_bytes);
            }
            if (target_component_count >= 4)
            {
                U8* alpha_dst = dst_pixel + 3 * source_component_bytes;
                if (source_component_bytes == 1)
                {
                    alpha_dst[0] = 0xFF;
                }
                else if (source_component_bytes == 2)
                {
                    alpha_dst[0] = 0x00;
                    alpha_dst[1] = 0x3C;
                }
                else if (source_component_bytes == 4)
                {
                    float one = 1.0f;
                    memcpy(alpha_dst, &one, 4);
                }
            }
        }
        else if (source_components < target_component_count)
        {
            for (U32 c = source_components; c < target_component_count; ++c)
            {
                U8* slot_dst = dst_pixel + c * source_component_bytes;
                if (c == 3)
                {
                    if (source_component_bytes == 1)
                    {
                        slot_dst[0] = 0xFF;
                    }
                    else if (source_component_bytes == 2)
                    {
                        slot_dst[0] = 0x00;
                        slot_dst[1] = 0x3C;
                    }
                    else if (source_component_bytes == 4)
                    {
                        float one = 1.0f;
                        memcpy(slot_dst, &one, 4);
                    }
                    else
                    {
                        memset(slot_dst, 0, source_component_bytes);
                    }
                }
                else
                {
                    memset(slot_dst, 0, source_component_bytes);
                }
            }
        }
    }

    out_padded_size_bytes = (U32)target_size;
    return padded;
}

static U32 pixTypeToSourceComponentBytes(U32 pixtype)
{
    switch (pixtype)
    {
        case GL_UNSIGNED_BYTE:
        case GL_BYTE:
            return 1;
        case GL_UNSIGNED_SHORT:
        case GL_SHORT:
        case GL_HALF_FLOAT:
            return 2;
        case GL_UNSIGNED_INT:
        case GL_INT:
        case GL_FLOAT:
            return 4;
        default:
            return 1;
    }
}

U64 LLImageGL::getTextureBytesAllocated()
{
    static U64     s_cached_bytes = 0;
    static LLTimer s_cache_timer;
    static bool    s_cached_once = false;
    if (!s_cached_once || s_cache_timer.getElapsedTimeF32() > 1.f)
    {
        s_cached_bytes = getVkTextureBytesAllocated();
        s_cache_timer.reset();
        s_cached_once = true;
    }
    return s_cached_bytes;
}

U64 LLImageGL::getVkTextureBytesAllocated()
{
    U64 total = 0;
    for (auto& glimage : sImageList)
    {
        if (!glimage || glimage->mVkImage == VK_NULL_HANDLE)
        {
            continue;
        }

        U32 bpp = LLVKLoader::vkFormatBytesPerPixel(glimage->mVkImageFormat);
        if (bpp == 0)
        {
            continue;
        }

        U32 w    = glimage->mVkImageWidth;
        U32 h    = glimage->mVkImageHeight;
        U32 mips = glimage->mVkImageMipLevels;
        if (mips == 0)
        {
            mips = 1;
        }

        for (U32 m = 0; m < mips; ++m)
        {
            U32 mw = llmax(1u, w >> m);
            U32 mh = llmax(1u, h >> m);
            total += (U64)mw * (U64)mh * (U64)bpp;
        }
    }
    return total;
}

U32 LLImageGL::sUniqueCount             = 0;
U32 LLImageGL::sBindCount               = 0;
S32 LLImageGL::sCount                   = 0;

bool LLImageGL::sGlobalUseAnisotropic   = false;
F32 LLImageGL::sLastFrameTime           = 0.f;
LLImageGL* LLImageGL::sDefaultGLTexture = NULL ;
LLImageGL* LLImageGL::sWhiteImageGLp   = NULL ;
bool LLImageGL::sCompressTextures = false;
std::unordered_set<LLImageGL*> LLImageGL::sImageList;


bool LLImageGLThread::sEnabledTextures = false;
bool LLImageGLThread::sEnabledMedia = false;

S32 LLImageGL::sCurTexSizeBar = -1 ;
S32 LLImageGL::sCurTexPickSize = -1 ;
S32 LLImageGL::sMaxCategories = 1 ;

bool LLImageGL::sSkipAnalyzeAlpha;

bool is_little_endian()
{
    S32 a = 0x12345678;
    U8 *c = (U8*)(&a);

    return (*c == 0x78) ;
}

void LLImageGL::initClass(LLWindow* window, S32 num_catagories, bool skip_analyze_alpha /* = false */, bool thread_texture_loads /* = false */, bool thread_media_updates /* = false */)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    sSkipAnalyzeAlpha = skip_analyze_alpha;

    if (LLVKLoader::isVulkanInitialized())
    {
        LLImageGLThread::sEnabledTextures = false;
        LLImageGLThread::sEnabledMedia = false;
    }
    else if (thread_texture_loads || thread_media_updates)
    {
        LLImageGLThread::createInstance(window);
        LLImageGLThread::sEnabledTextures = gGLManager.mGLVersion > 3.95f ? thread_texture_loads : false;
        LLImageGLThread::sEnabledMedia = gGLManager.mGLVersion > 3.95f ? thread_media_updates : false;
    }
}

void LLImageGL::cleanupClass()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LLImageGLThread::deleteSingleton();
}


S32 LLImageGL::dataFormatBits(S32 dataformat)
{
    switch (dataformat)
    {
    case GL_COMPRESSED_RED:                         return 8;
    case GL_COMPRESSED_RG:                          return 16;
    case GL_COMPRESSED_RGB:                         return 24;
    case GL_COMPRESSED_SRGB:                        return 32;
    case GL_COMPRESSED_RGBA:                        return 32;
    case GL_COMPRESSED_SRGB_ALPHA:                  return 32;
    case GL_COMPRESSED_LUMINANCE:                   return 8;
    case GL_COMPRESSED_LUMINANCE_ALPHA:             return 16;
    case GL_COMPRESSED_ALPHA:                       return 8;
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:          return 4;
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:    return 4;
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:          return 8;
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:    return 8;
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:          return 8;
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:    return 8;
    case GL_LUMINANCE:                              return 8;
    case GL_LUMINANCE8:                             return 8;
    case GL_ALPHA:                                  return 8;
    case GL_ALPHA8:                                 return 8;
    case GL_RED:                                    return 8;
    case GL_R8:                                     return 8;
    case GL_COLOR_INDEX:                            return 8;
    case GL_LUMINANCE_ALPHA:                        return 16;
    case GL_LUMINANCE8_ALPHA8:                      return 16;
    case GL_RG:                                     return 16;
    case GL_RG8:                                    return 16;
    case GL_RGB:                                    return 24;
    case GL_SRGB:                                   return 24;
    case GL_RGB8:                                   return 24;
    case GL_R11F_G11F_B10F:                         return 32;
    case GL_RGBA:                                   return 32;
    case GL_RGBA8:                                  return 32;
    case GL_RGB10_A2:                               return 32;
    case GL_SRGB_ALPHA:                             return 32;
    case GL_BGRA:                                   return 32;      // Used for QuickTime media textures on the Mac
    case GL_DEPTH_COMPONENT:                        return 24;
    case GL_DEPTH_COMPONENT24:                      return 24;
    case GL_RGBA16:                                 return 64;
    case GL_R16F:                                   return 16;
    case GL_RG16F:                                  return 32;
    case GL_RGB16F:                                 return 48;
    case GL_RGBA16F:                                return 64;
    case GL_R32F:                                   return 32;
    case GL_RG32F:                                  return 64;
    case GL_RGB32F:                                 return 96;
    case GL_RGBA32F:                                return 128;
    default:
        LL_ERRS() << "LLImageGL::Unknown format: " << std::hex << dataformat << std::dec << LL_ENDL;
        return 0;
    }
}

S64 LLImageGL::dataFormatBytes(S32 dataformat, S32 width, S32 height)
{
    switch (dataformat)
    {
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        if (width < 4) width = 4;
        if (height < 4) height = 4;
        break;
    default:
        break;
    }
    S64 bytes (((S64)width * (S64)height * (S64)dataFormatBits(dataformat)+7)>>3);
    S64 aligned = (bytes+3)&~3;
    return aligned;
}

S32 LLImageGL::dataFormatComponents(S32 dataformat)
{
    switch (dataformat)
    {
      case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:    return 3;
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT: return 3;
      case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:    return 4;
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT: return 4;
      case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:    return 4;
      case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: return 4;
      case GL_LUMINANCE:                        return 1;
      case GL_ALPHA:                            return 1;
      case GL_RED:                              return 1;
      case GL_COLOR_INDEX:                      return 1;
      case GL_LUMINANCE_ALPHA:                  return 2;
      case GL_RG:                               return 2;
      case GL_RGB:                              return 3;
      case GL_SRGB:                             return 3;
      case GL_RGBA:                             return 4;
      case GL_SRGB_ALPHA:                       return 4;
      case GL_BGRA:                             return 4;       // Used for QuickTime media textures on the Mac
      default:
        LL_ERRS() << "LLImageGL::Unknown format: " << std::hex << dataformat << std::dec << LL_ENDL;
        return 0;
    }
}



void LLImageGL::updateStats(F32 current_time)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    sLastFrameTime = current_time;
}



void LLImageGL::destroyGL()
{
    for (S32 stage = 0; stage < gGLManager.mNumTextureImageUnits; stage++)
    {
        gGL.getTexUnit(stage)->unbind(LLTexUnit::TT_TEXTURE);
    }
}

void LLImageGL::dirtyTexOptions()
{
    for (auto& glimage : sImageList)
    {
        glimage->mTexOptionsDirty = true;
    }

}



bool LLImageGL::create(LLPointer<LLImageGL>& dest, bool usemipmaps)
{
    dest = new LLImageGL(usemipmaps);
    return true;
}


bool LLImageGL::create(LLPointer<LLImageGL>& dest, U32 width, U32 height, U8 components, bool usemipmaps)
{
    dest = new LLImageGL(width, height, components, usemipmaps);
    return true;
}


bool LLImageGL::create(LLPointer<LLImageGL>& dest, const LLImageRaw* imageraw, bool usemipmaps)
{
    dest = new LLImageGL(imageraw, usemipmaps);
    return true;
}



LLImageGL::LLImageGL(bool usemipmaps/* = true*/, bool allow_compression/* = true*/)
:   mSaveData(0), mExternalTexture(false)
{
    init(usemipmaps, allow_compression);
    setSize(0, 0, 0);
    sImageList.insert(this);
    sCount++;
}

LLImageGL::LLImageGL(U32 width, U32 height, U8 components, bool usemipmaps/* = true*/, bool allow_compression/* = true*/)
:   mSaveData(0), mExternalTexture(false)
{
    llassert( components <= 4 );
    init(usemipmaps, allow_compression);
    setSize(width, height, components);
    sImageList.insert(this);
    sCount++;
}

LLImageGL::LLImageGL(const LLImageRaw* imageraw, bool usemipmaps/* = true*/, bool allow_compression/* = true*/)
:   mSaveData(0), mExternalTexture(false)
{
    init(usemipmaps, allow_compression);
    setSize(0, 0, 0);
    sImageList.insert(this);
    sCount++;

    createGLTexture(0, imageraw);
}

LLImageGL::LLImageGL(
    U32 components,
    LLGLenum target,
    LLGLint  formatInternal,
    LLGLenum formatPrimary,
    LLGLenum formatType,
    LLTexUnit::eTextureAddressMode addressMode)
{
    init(false, true);
    mTarget = target;
    mComponents = components;
    mAddressMode = addressMode;
    mFormatType = formatType;
    mFormatInternal = formatInternal;
    mFormatPrimary = formatPrimary;
}


LLImageGL::~LLImageGL()
{
    if (!mExternalTexture && gGLManager.mInited)
    {
        LLRender::clearStaleImageGLRefs(this);
        LLImageGL::cleanup();
        sImageList.erase(this);
        freePickMask();
        sCount--;
    }
    if (mVkHeapSlot != 0xFFFFFFFFu)
    {
        LLVKLoader::bindlessReleaseSlotDeferred(mVkHeapSlot);
        mVkHeapSlot = 0xFFFFFFFFu;
    }
}

void LLImageGL::init(bool usemipmaps, bool allow_compression)
{
#if LL_IMAGEGL_THREAD_CHECK
    mActiveThread = LLThread::currentID();
#endif

    mTextureMemory = S64Bytes(0);
    mLastBindTime = 0.f;

    mPickMask = NULL;
    mPickMaskWidth = 0;
    mPickMaskHeight = 0;
    mUseMipMaps = usemipmaps;
    mHasExplicitFormat = false;

    mIsMask = false;
    mNeedsAlphaAndPickMask = true ;
    mAlphaStride = 0 ;
    mAlphaOffset = 0 ;

    mGLTextureCreated = false ;
    mWidth = 0;
    mHeight = 0;
    mCurrentDiscardLevel = -1;

    mAllowCompression = allow_compression;

    mTarget = GL_TEXTURE_2D;
    mBindTarget = LLTexUnit::TT_TEXTURE;
    mHasMipMaps = false;
    mMipLevels = -1;

    mIsResident = 0;

    mComponents = 0;
    mMaxDiscardLevel = MAX_DISCARD_LEVEL;

    mTexOptionsDirty = true;
    mAddressMode = LLTexUnit::TAM_WRAP;
    mFilterOption = LLTexUnit::TFO_ANISOTROPIC;

    mFormatInternal = -1;
    mFormatPrimary = (LLGLenum) 0;
    mFormatType = GL_UNSIGNED_BYTE;
    mFormatSwapBytes = false;

#ifdef DEBUG_MISS
    mMissed = false;
#endif

    mCategory = -1;

    mMainQueue = LL::WorkQueue::getInstance("mainloop");
}

void LLImageGL::cleanup()
{
    if (!gGLManager.mIsDisabled)
    {
        destroyGLTexture();
    }
    freePickMask();

    mSaveData = NULL; // deletes data
}



static bool check_power_of_two(S32 dim)
{
    if(dim < 0)
    {
        return false ;
    }
    if(!dim)//0 is a power-of-two number
    {
        return true ;
    }
    return !(dim & (dim - 1)) ;
}

bool LLImageGL::checkSize(S32 width, S32 height)
{
    return check_power_of_two(width) && check_power_of_two(height);
}

bool LLImageGL::setSize(S32 width, S32 height, S32 ncomponents, S32 discard_level)
{
    if (width != mWidth || height != mHeight || ncomponents != mComponents)
    {
        if (!checkSize(width, height))
        {
            LL_WARNS() << llformat("Texture has non power of two dimension: %dx%d",width,height) << LL_ENDL;
            return false;
        }

        mWidth = width;
        mHeight = height;
        mComponents = ncomponents;
        if (ncomponents > 0)
        {
            mMaxDiscardLevel = 0;
            while (width > 1 && height > 1 && mMaxDiscardLevel < MAX_DISCARD_LEVEL)
            {
                mMaxDiscardLevel++;
                width >>= 1;
                height >>= 1;
            }

            if(discard_level > 0)
            {
                mMaxDiscardLevel = llmax(mMaxDiscardLevel, (S8)discard_level);
                // <FS:minerjr> [FIRE-35361] RenderMaxTextureResolution caps texture resolution lower than intended
                // 2K textures could set the mMaxDiscardLevel above MAX_DISCARD_LEVEL, which would
                // cause them to not be down-scaled so they would get stuck at 0 discard all the time.
                mMaxDiscardLevel = llmin(mMaxDiscardLevel, (S8)MAX_DISCARD_LEVEL);
                // </FS:minerjr> [FIRE-35361]
            }
        }
        else
        {
            mMaxDiscardLevel = MAX_DISCARD_LEVEL;
        }
    }

    return true;
}



void LLImageGL::dump()
{
    LL_INFOS() << "mMaxDiscardLevel " << S32(mMaxDiscardLevel)
            << " mLastBindTime " << mLastBindTime
            << " mTarget " << S32(mTarget)
            << " mBindTarget " << S32(mBindTarget)
            << " mUseMipMaps " << S32(mUseMipMaps)
            << " mHasMipMaps " << S32(mHasMipMaps)
            << " mCurrentDiscardLevel " << S32(mCurrentDiscardLevel)
            << " mFormatInternal " << S32(mFormatInternal)
            << " mFormatPrimary " << S32(mFormatPrimary)
            << " mFormatType " << S32(mFormatType)
            << " mFormatSwapBytes " << S32(mFormatSwapBytes)
            << " mHasExplicitFormat " << S32(mHasExplicitFormat)
#if DEBUG_MISS
            << " mMissed " << mMissed
#endif
            << LL_ENDL;

    LL_INFOS() << " mTextureMemory " << mTextureMemory
            << " mIsResident " << S32(mIsResident)
            << LL_ENDL;
}


void LLImageGL::forceUpdateBindStats(void) const
{
    mLastBindTime = sLastFrameTime;
}

bool LLImageGL::updateBindStats() const
{
    if (mVkImage != VK_NULL_HANDLE)
    {
#ifdef DEBUG_MISS
        mMissed = ! getIsResident(true);
#endif
        sBindCount++;
        if (mLastBindTime != sLastFrameTime)
        {
            sUniqueCount++;
            mLastBindTime = sLastFrameTime;

            return true ;
        }
    }
    return false ;
}

F32 LLImageGL::getTimePassedSinceLastBound()
{
    return sLastFrameTime - mLastBindTime ;
}

void LLImageGL::setExplicitFormat( LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format, bool swap_bytes )
{
    mHasExplicitFormat = true;
    mFormatInternal = internal_format;
    mFormatPrimary = primary_format;
    if(type_format == 0)
        mFormatType = GL_UNSIGNED_BYTE;
    else
        mFormatType = type_format;
    mFormatSwapBytes = swap_bytes;

    calcAlphaChannelOffsetAndStride() ;
}



void LLImageGL::setImage(const LLImageRaw* imageraw)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    llassert((imageraw->getWidth() == getWidth(mCurrentDiscardLevel)) &&
             (imageraw->getHeight() == getHeight(mCurrentDiscardLevel)) &&
             (imageraw->getComponents() == getComponents()));
    const U8* rawdata = imageraw->getData();
    setImage(rawdata, false);
}

bool LLImageGL::setImage(const U8* data_in, bool data_hasmips /* = false */)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    const bool is_compressed = isCompressed();

    if (mUseMipMaps)
    {
        gGL.getTexUnit(0)->unbind(mBindTarget);

        mHasMipMaps = true;
        mTexOptionsDirty = true;
        setFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
    }
    else
    {
        mHasMipMaps = false;
    }

    if (data_in == nullptr)
    {
        S32 w = getWidth();
        S32 h = getHeight();
        syncVulkanMip0Image((U32)mFormatInternal, (U32)mFormatPrimary, (U32)mFormatType, w, h, nullptr, false);
    }
    else if (mUseMipMaps)
    {
        if (data_hasmips)
        {
            for (S32 d=mCurrentDiscardLevel; d<=mMaxDiscardLevel; d++)
            {

                S32 w = getWidth(d);
                S32 h = getHeight(d);
                S32 gl_level = d-mCurrentDiscardLevel;

                mMipLevels = llmax(mMipLevels, gl_level);

                if (d > mCurrentDiscardLevel)
                {
                    data_in -= dataFormatBytes(mFormatPrimary, w, h); // see above comment
                }
                if (is_compressed)
                {
                    syncVulkanMip0Image((U32)mFormatPrimary, (U32)mFormatPrimary, (U32)GL_UNSIGNED_BYTE, w, h, data_in, true,
                                        gl_level, mMaxDiscardLevel - mCurrentDiscardLevel + 1);
                }
                else
                {
                    syncVulkanMip0Image((U32)mFormatInternal, (U32)mFormatPrimary, (U32)GL_UNSIGNED_BYTE, w, h, data_in, false,
                                        gl_level, mMaxDiscardLevel - mCurrentDiscardLevel + 1);
                    if (gl_level == 0)
                    {
                        analyzeAlpha(data_in, w, h);
                    }
                    updatePickMask(w, h, data_in);
                }
            }
        }
        else if (!is_compressed)
        {
            if (mAutoGenMips)
            {
                {
                    S32 w = getWidth(mCurrentDiscardLevel);
                    S32 h = getHeight(mCurrentDiscardLevel);

                    mMipLevels = wpo2(llmax(w, h));

                    analyzeAlpha(data_in, w, h);
                    S32 vk_mip_count = 1;
                    {
                        S32 dim = llmax(w, h);
                        while (dim > 1) { dim >>= 1; ++vk_mip_count; }
                    }
                    syncVulkanMip0Image((U32)mFormatInternal, (U32)mFormatPrimary, (U32)mFormatType, w, h, data_in, false, 0, vk_mip_count);
                    if (mVkImage != VK_NULL_HANDLE && mVkImageMipLevels > 1)
                    {
                        LLVKLoader::generateMipChainBlitVk(mVkImage, (U32)w, (U32)h, mVkImageMipLevels, mVkImageFormat);
                    }

                    updatePickMask(w, h, data_in);
                }
            }
            else
            {
                S32 width = getWidth(mCurrentDiscardLevel);
                S32 height = getHeight(mCurrentDiscardLevel);
                S32 nummips = mMaxDiscardLevel - mCurrentDiscardLevel + 1;
                S32 w = width, h = height;


                const U8* new_data = 0;
                (void)new_data;

                const U8* prev_mip_data = 0;
                const U8* cur_mip_data = 0;
#ifdef SHOW_ASSERT
                S32 cur_mip_size = 0;
#endif
                mMipLevels = nummips;

                for (int m=0; m<nummips; m++)
                {
                    if (m==0)
                    {
                        cur_mip_data = data_in;
#ifdef SHOW_ASSERT
                        cur_mip_size = width * height * mComponents;
#endif
                    }
                    else
                    {
                        S32 bytes = w * h * mComponents;
#ifdef SHOW_ASSERT
                        llassert(prev_mip_data);
                        llassert(cur_mip_size == bytes*4);
#endif
                        U8* new_data = new(std::nothrow) U8[bytes];
                        if (!new_data)
                        {

                            if (prev_mip_data)
                            {
                                if (prev_mip_data != cur_mip_data)
                                    delete[] prev_mip_data;
                                prev_mip_data = nullptr;
                            }
                            if (cur_mip_data)
                            {
                                delete[] cur_mip_data;
                                cur_mip_data = nullptr;
                            }

                            mGLTextureCreated = false;
                            return false;
                        }
                        else
                        {

#ifdef SHOW_ASSERT
                            llassert(prev_mip_data);
                            llassert(cur_mip_size == bytes * 4);
#endif

                            LLImageBase::generateMip(prev_mip_data, new_data, w, h, mComponents);
                            cur_mip_data = new_data;
#ifdef SHOW_ASSERT
                            cur_mip_size = bytes;
#endif
                        }

                    }
                    llassert(w > 0 && h > 0 && cur_mip_data);
                    (void)cur_mip_data;
                    {
                        syncVulkanMip0Image((U32)mFormatInternal, (U32)mFormatPrimary, (U32)mFormatType, w, h, cur_mip_data, false, m, nummips);
                        if (m == 0)
                        {
                            analyzeAlpha(data_in, w, h);
                            updatePickMask(w, h, cur_mip_data);
                        }
                    }
                    if (prev_mip_data && prev_mip_data != data_in)
                    {
                        delete[] prev_mip_data;
                    }
                    prev_mip_data = cur_mip_data;
                    w >>= 1;
                    h >>= 1;
                }
                if (prev_mip_data && prev_mip_data != data_in)
                {
                    delete[] prev_mip_data;
                    prev_mip_data = NULL;
                }
            }
        }
        else
        {
            LL_ERRS() << "Compressed Image has mipmaps but data does not (can not auto generate compressed mips)" << LL_ENDL;
        }
    }
    else
    {
        mMipLevels = 0;
        S32 w = getWidth();
        S32 h = getHeight();
        if (is_compressed)
        {
            syncVulkanMip0Image((U32)mFormatPrimary, (U32)mFormatPrimary, (U32)GL_UNSIGNED_BYTE, w, h, data_in, true);
        }
        else
        {
            analyzeAlpha(data_in, w, h);
            syncVulkanMip0Image((U32)mFormatInternal, (U32)mFormatPrimary, (U32)mFormatType, w, h, data_in, false);

            updatePickMask(w, h, data_in);

        }
    }
    mGLTextureCreated = true;

    return true;
}

namespace
{
    static LLMutex sVkSyncFmtUndefMutex;
    static std::unordered_map<U32, U64> sVkSyncFmtUndefCounts;

    void recordVkSyncFmtUndef(U32 fmt_src, U32 primary, bool is_compressed)
    {
        LLMutexLock lock(&sVkSyncFmtUndefMutex);
        auto it = sVkSyncFmtUndefCounts.find(fmt_src);
        if (it == sVkSyncFmtUndefCounts.end())
        {
            sVkSyncFmtUndefCounts[fmt_src] = 1;
            LL_WARNS("VulkanUpload") << "format-undefined (first seen): fmt_src=0x"
                                     << std::hex << fmt_src
                                     << " primary=0x" << primary << std::dec
                                     << " is_compressed=" << (is_compressed ? 1 : 0)
                                     << LL_ENDL;
        }
        else
        {
            ++(it->second);
        }
    }

}

void LLImageGL::syncVulkanMip0Image(U32 intformat, U32 primary, U32 type,
                                    S32 w, S32 h, const void* data, bool is_compressed,
                                    S32 mip_level, S32 mip_count)
{
    if (!LLVKLoader::shouldUseVulkanRender())
    {
        return;
    }
    if (mTarget != GL_TEXTURE_2D || mExternalTexture)
    {
        return;
    }
    if (w <= 0 || h <= 0)
    {
        return;
    }

    const U32 fmt_src = is_compressed ? primary : intformat;
    VkFormat vk_format = LLVKLoader::llGlEnumToVkFormat(fmt_src);
    if (!is_compressed && primary == GL_BGRA && vk_format == VK_FORMAT_R8G8B8A8_UNORM)
    {
        vk_format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    if (vk_format == VK_FORMAT_UNDEFINED)
    {
        recordVkSyncFmtUndef(fmt_src, primary, is_compressed);
        return;
    }

    if (mip_level == 0)
    {
        const U32 want_mips = (mip_count > 0) ? (U32)mip_count : 1u;

        const bool can_reuse = (mVkImage != VK_NULL_HANDLE)
                               && (mVkImageView != VK_NULL_HANDLE)
                               && (mVkImageWidth  == (U32)w)
                               && (mVkImageHeight == (U32)h)
                               && (mVkImageFormat == vk_format)
                               && (mVkImageMipLevels == want_mips);

        if (!can_reuse && (mVkImage != VK_NULL_HANDLE || mVkImageView != VK_NULL_HANDLE ||
                           mVkAllocation != nullptr))
        {
            LLVKLoader::destroyImageVk(mVkImage, mVkImageView, mVkAllocation);
            mVkImage      = VK_NULL_HANDLE;
            mVkImageView  = VK_NULL_HANDLE;
            mVkAllocation = nullptr;
            mVkImageWidth  = 0;
            mVkImageHeight = 0;
            mVkImageMipLevels = 1;
            mVkImageFormat = VK_FORMAT_UNDEFINED;
            updateVkHeapSlot();
        }

        if (!can_reuse)
        {
            if (!LLVKLoader::createTextureImageVk((U32)w, (U32)h, vk_format,
                                                  mVkImage, mVkImageView, mVkAllocation,
                                                  want_mips))
            {
                return;
            }
            mVkImageWidth     = (U32)w;
            mVkImageHeight    = (U32)h;
            mVkImageMipLevels = want_mips;
            mVkImageFormat    = vk_format;
            updateVkHeapSlot();
        }
    }
    else
    {
        if (mVkImage == VK_NULL_HANDLE || mVkImageView == VK_NULL_HANDLE
            || mVkImageFormat != vk_format
            || (U32)mip_level >= mVkImageMipLevels)
        {
            return;
        }
    }

    if (data == nullptr)
    {
        return;
    }

    const void* upload_data     = data;
    U32         upload_size     = 0;
    U8*         padded_buffer   = nullptr;

    if (is_compressed)
    {
        upload_size = (U32)dataFormatBytes(primary, w, h);
    }
    else
    {
        const U32 source_components       = LLVKLoader::llGlFormatSourceComponents(primary);
        const U32 pixel_count             = (U32)w * (U32)h;

        U32 half16_target_components = 0;
        if (type == GL_FLOAT)
        {
            switch (vk_format)
            {
                case VK_FORMAT_R16_SFLOAT:          half16_target_components = 1; break;
                case VK_FORMAT_R16G16_SFLOAT:       half16_target_components = 2; break;
                case VK_FORMAT_R16G16B16A16_SFLOAT: half16_target_components = 4; break;
                default:                            break;
            }
        }

        if (half16_target_components != 0)
        {
            U32 conv_size_bytes = 0;
            padded_buffer = createVulkanFloat32ToHalf16Buffer(
                                (const U8*)data,
                                source_components,
                                half16_target_components,
                                pixel_count,
                                conv_size_bytes);

            if (padded_buffer == nullptr)
            {
                LLVKLoader::destroyImageVk(mVkImage, mVkImageView, mVkAllocation);
                mVkImage       = VK_NULL_HANDLE;
                mVkImageView   = VK_NULL_HANDLE;
                mVkAllocation  = nullptr;
                mVkImageWidth  = 0;
                mVkImageHeight = 0;
                mVkImageFormat = VK_FORMAT_UNDEFINED;
                updateVkHeapSlot();
                return;
            }
            upload_data = padded_buffer;
            upload_size = conv_size_bytes;
        }
        else
        {
            const U32 source_component_bytes  = pixTypeToSourceComponentBytes(type);
            const U32 target_bytes_per_pixel  = LLVKLoader::vkFormatBytesPerPixel(vk_format);

            U32 padded_size_bytes = 0;
            padded_buffer = createVulkanPaddingBuffer(
                                (const U8*)data,
                                source_components,
                                source_component_bytes,
                                target_bytes_per_pixel,
                                pixel_count,
                                padded_size_bytes);

            upload_data = (padded_buffer != nullptr) ? (const void*)padded_buffer : data;
            upload_size = (padded_buffer != nullptr)
                              ? padded_size_bytes
                              : (U32)dataFormatBytes(primary, w, h);
        }
    }

    if (upload_size > 0)
    {
        const bool upload_ok = LLVKLoader::uploadImageDataVk(
            mVkImage, (U32)w, (U32)h, upload_data, upload_size, (U32)mip_level);

        if (!upload_ok)
        {
            LLVKLoader::destroyImageVk(mVkImage, mVkImageView, mVkAllocation);
            mVkImage      = VK_NULL_HANDLE;
            mVkImageView  = VK_NULL_HANDLE;
            mVkAllocation = nullptr;
            mVkImageWidth  = 0;
            mVkImageHeight = 0;
            mVkImageMipLevels = 1;
            mVkImageFormat = VK_FORMAT_UNDEFINED;
            updateVkHeapSlot();
        }
    }

    if (padded_buffer != nullptr)
    {
        delete[] padded_buffer;
    }
}

void LLImageGL::syncVulkan3DImage(U32 intformat, U32 primary, U32 type,
                                  S32 w, S32 h, S32 depth, const void* data)
{
    if (!LLVKLoader::shouldUseVulkanRender())
    {
        return;
    }
    if (w <= 0 || h <= 0 || depth <= 0 || data == nullptr)
    {
        return;
    }

    VkFormat vk_format = LLVKLoader::llGlEnumToVkFormat(intformat);
    if (vk_format == VK_FORMAT_UNDEFINED)
    {
        LL_WARNS_ONCE("Vulkan") << "syncVulkan3DImage: unmapped GL intformat 0x"
                                << std::hex << intformat << std::dec << LL_ENDL;
        return;
    }

    if (mVkImage != VK_NULL_HANDLE || mVkImageView != VK_NULL_HANDLE || mVkAllocation != nullptr)
    {
        LLVKLoader::destroyImageVk(mVkImage, mVkImageView, mVkAllocation);
        mVkImage       = VK_NULL_HANDLE;
        mVkImageView   = VK_NULL_HANDLE;
        mVkAllocation  = nullptr;
        mVkImageWidth  = 0;
        mVkImageHeight = 0;
        mVkImageFormat = VK_FORMAT_UNDEFINED;
    }

    if (!LLVKLoader::createTexture3DImageVk((U32)w, (U32)h, (U32)depth, vk_format,
                                            mVkImage, mVkImageView, mVkAllocation))
    {
        return;
    }
    mVkImageWidth  = (U32)w;
    mVkImageHeight = (U32)h;
    mVkImageFormat = vk_format;

    const U32   source_components = LLVKLoader::llGlFormatSourceComponents(primary);
    const U32   pixel_count       = (U32)w * (U32)h * (U32)depth;
    const void* upload_data       = data;
    U32         upload_size       = 0;
    U8*         conv_buffer       = nullptr;

    U32 half16_target_components = 0;
    if (type == GL_FLOAT)
    {
        switch (vk_format)
        {
            case VK_FORMAT_R16_SFLOAT:          half16_target_components = 1; break;
            case VK_FORMAT_R16G16_SFLOAT:       half16_target_components = 2; break;
            case VK_FORMAT_R16G16B16A16_SFLOAT: half16_target_components = 4; break;
            default:                            break;
        }
    }

    if (half16_target_components != 0)
    {
        conv_buffer = createVulkanFloat32ToHalf16Buffer(
                          (const U8*)data, source_components, half16_target_components,
                          pixel_count, upload_size);
        if (conv_buffer == nullptr)
        {
            LLVKLoader::destroyImageVk(mVkImage, mVkImageView, mVkAllocation);
            mVkImage = VK_NULL_HANDLE; mVkImageView = VK_NULL_HANDLE; mVkAllocation = nullptr;
            mVkImageWidth = 0; mVkImageHeight = 0; mVkImageFormat = VK_FORMAT_UNDEFINED;
            return;
        }
        upload_data = conv_buffer;
    }
    else
    {
        const U32 source_component_bytes = pixTypeToSourceComponentBytes(type);
        const U32 target_bpp             = LLVKLoader::vkFormatBytesPerPixel(vk_format);
        U32       padded_size            = 0;
        conv_buffer = createVulkanPaddingBuffer((const U8*)data, source_components,
                                                source_component_bytes, target_bpp,
                                                pixel_count, padded_size);
        upload_data = (conv_buffer != nullptr) ? (const void*)conv_buffer : data;
        upload_size = (conv_buffer != nullptr) ? padded_size : (target_bpp * pixel_count);
    }

    if (upload_size > 0)
    {
        if (!LLVKLoader::uploadImageData3DVk(mVkImage, (U32)w, (U32)h, (U32)depth,
                                             upload_data, upload_size))
        {
            LLVKLoader::destroyImageVk(mVkImage, mVkImageView, mVkAllocation);
            mVkImage = VK_NULL_HANDLE; mVkImageView = VK_NULL_HANDLE; mVkAllocation = nullptr;
            mVkImageWidth = 0; mVkImageHeight = 0; mVkImageFormat = VK_FORMAT_UNDEFINED;
        }
    }

    if (conv_buffer != nullptr)
    {
        delete[] conv_buffer;
    }
}

void LLImageGL::setExternalVkBacking(VkImage image, VkImageView view, void* allocation,
                                     U32 w, U32 h, VkFormat format, U32 mip_levels)
{
    if (mVkImage != VK_NULL_HANDLE || mVkImageView != VK_NULL_HANDLE || mVkAllocation != nullptr)
    {
        LLVKLoader::destroyImageVk(mVkImage, mVkImageView, mVkAllocation);
    }
    mVkImage         = image;
    mVkImageView     = view;
    mVkAllocation    = allocation;
    mVkImageWidth    = w;
    mVkImageHeight   = h;
    mVkImageFormat   = format;
    mVkImageMipLevels = mip_levels;
    LLGLSLShader::sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
    updateVkHeapSlot();
}

void LLImageGL::updateVkHeapSlot()
{
    if (!LLVKLoader::isBindlessActiveVk())
    {
        return;
    }
    if (mTarget != GL_TEXTURE_2D)
    {
        return;
    }
    VkSampler smp = VK_NULL_HANDLE;
    if (mVkImageView != VK_NULL_HANDLE)
    {
        smp = LLVKLoader::getSamplerForState((U32)mAddressMode, (U32)mFilterOption, mHasMipMaps, false);
    }

    if (mVkHeapSlot == LLVKLoader::BINDLESS_INVALID_SLOT)
    {
        mVkHeapSlot = LLVKLoader::bindlessAcquireSlot(mVkImageView, smp);
        if (mVkHeapSlot == LLVKLoader::BINDLESS_INVALID_SLOT)
        {
            return;
        }
        mVkHeapSlotView    = mVkImageView;
        mVkHeapSlotSampler = smp;
        return;
    }

    if (mVkHeapSlotView != mVkImageView || mVkHeapSlotSampler != smp)
    {
        LLVKLoader::bindlessUpdateSlot(mVkHeapSlot, mVkImageView, smp);
        mVkHeapSlotView    = mVkImageView;
        mVkHeapSlotSampler = smp;
    }
}

U32 LLImageGL::vkHeapSlotOrDefault(LLImageGL* gl)
{
    if (gl != nullptr && gl->mTarget == GL_TEXTURE_2D && LLVKLoader::isBindlessActiveVk())
    {
        if (gl->mVkHeapSlot == LLVKLoader::BINDLESS_INVALID_SLOT)
        {
            gl->updateVkHeapSlot();
        }
        if (gl->mVkHeapSlot != LLVKLoader::BINDLESS_INVALID_SLOT)
        {
            return gl->mVkHeapSlot;
        }
    }
    const LLImageGL* def = sDefaultGLTexture;
    if (def != nullptr && def->getVkHeapSlot() != LLVKLoader::BINDLESS_INVALID_SLOT)
    {
        return def->getVkHeapSlot();
    }
    return 0;
}

bool LLImageGL::buildVkUploadJob(LLVkTexUploadJob& job, S32 discard_level, const LLImageRaw* imageraw) const
{
    if (!LLVKLoader::shouldUseVulkanRender())
    {
        return false;
    }
    if (mTarget != GL_TEXTURE_2D || mExternalTexture)
    {
        return false;
    }
    if (imageraw == nullptr || imageraw->isBufferInvalid())
    {
        return false;
    }

    if (discard_level < 0)
    {
        discard_level = mCurrentDiscardLevel;
    }
    if (discard_level < 0)
    {
        return false;
    }
    discard_level = llmin(discard_level, MAX_DISCARD_LEVEL);

    const S32 raw_w = imageraw->getWidth();
    const S32 raw_h = imageraw->getHeight();
    if (raw_w <= 0 || raw_h <= 0)
    {
        return false;
    }
    const S32 full_w = raw_w << discard_level;
    const S32 full_h = raw_h << discard_level;
    if (!checkSize(full_w, full_h))
    {
        return false;
    }

    const S8 components = (S8)imageraw->getComponents();

    LLGLint  fmt_int  = 0;
    LLGLenum fmt_prim = 0;
    LLGLenum fmt_type = 0;
    const bool explicit_ok = mHasExplicitFormat &&
                             !((mFormatPrimary == GL_RGBA && components < 4) ||
                               (mFormatPrimary == GL_RGB  && components < 3));
    if (explicit_ok)
    {
        fmt_int  = mFormatInternal;
        fmt_prim = mFormatPrimary;
        fmt_type = mFormatType;
    }
    else
    {
        switch (components)
        {
        case 1:
            fmt_int  = GL_LUMINANCE8;
            fmt_prim = GL_LUMINANCE;
            fmt_type = GL_UNSIGNED_BYTE;
            break;
        case 2:
            fmt_int  = GL_LUMINANCE8_ALPHA8;
            fmt_prim = GL_LUMINANCE_ALPHA;
            fmt_type = GL_UNSIGNED_BYTE;
            break;
        case 3:
            fmt_int  = GL_RGB8;
            fmt_prim = GL_RGB;
            fmt_type = GL_UNSIGNED_BYTE;
            break;
        case 4:
            fmt_int  = GL_RGBA8;
            fmt_prim = GL_RGBA;
            fmt_type = GL_UNSIGNED_BYTE;
            break;
        default:
            return false;
        }
    }

    if (fmt_type != GL_UNSIGNED_BYTE)
    {
        return false;
    }
    switch (fmt_prim)
    {
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        return false;
    default:
        break;
    }

    VkFormat vk_format = LLVKLoader::llGlEnumToVkFormat((U32)fmt_int);
    if (fmt_prim == GL_BGRA && vk_format == VK_FORMAT_R8G8B8A8_UNORM)
    {
        vk_format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    if (vk_format == VK_FORMAT_UNDEFINED)
    {
        return false;
    }

    U32 mip_count = 1;
    if (mUseMipMaps)
    {
        S32 dim = llmax(raw_w, raw_h);
        while (dim > 1)
        {
            dim >>= 1;
            ++mip_count;
        }
    }

    bool needs_mask = mNeedsAlphaAndPickMask && !sSkipAnalyzeAlpha;
    S8 alpha_stride = -1;
    S8 alpha_offset = -1;
    if (needs_mask)
    {
        switch (fmt_prim)
        {
        case GL_LUMINANCE:
        case GL_ALPHA:
            alpha_stride = 1;
            break;
        case GL_LUMINANCE_ALPHA:
            alpha_stride = 2;
            break;
        case GL_RED:
        case GL_RGB:
        case GL_SRGB:
            needs_mask = false;
            break;
        case GL_RGBA:
        case GL_SRGB_ALPHA:
        case GL_BGRA_EXT:
            alpha_stride = 4;
            break;
        default:
            break;
        }
        alpha_offset = alpha_stride - 1;
        if (alpha_stride < 1 || alpha_offset < 0)
        {
            needs_mask = false;
        }
    }

    job.mDiscard    = discard_level;
    job.mRawWidth   = raw_w;
    job.mRawHeight  = raw_h;
    job.mFullWidth  = full_w;
    job.mFullHeight = full_h;
    job.mComponents = components;
    job.mFormatInternal = fmt_int;
    job.mFormatPrimary  = fmt_prim;
    job.mFormatType     = fmt_type;
    job.mVkFormat   = vk_format;
    job.mMipCount   = mip_count;
    job.mUseMipMaps = mUseMipMaps != 0;
    job.mNeedsAlphaAndPickMask = needs_mask;
    job.mAlphaStride = alpha_stride;
    job.mAlphaOffset = alpha_offset;
    return true;
}

bool LLImageGL::runVkUploadJob(LLVkTexUploadJob& job, const U8* data)
{
    job.mOk = false;
    if (data == nullptr)
    {
        return false;
    }

    const U32 pixel_count       = (U32)job.mRawWidth * (U32)job.mRawHeight;
    const U32 source_components = LLVKLoader::llGlFormatSourceComponents(job.mFormatPrimary);
    const U32 target_bpp        = LLVKLoader::vkFormatBytesPerPixel(job.mVkFormat);

    U32 padded_size = 0;
    U8* padded = createVulkanPaddingBuffer(data, source_components, 1, target_bpp,
                                           pixel_count, padded_size);
    const void* upload_data = (padded != nullptr) ? (const void*)padded : (const void*)data;
    const U32   upload_size = (padded != nullptr)
                                  ? padded_size
                                  : (U32)dataFormatBytes(job.mFormatPrimary, job.mRawWidth, job.mRawHeight);

    bool ok = false;
    if (upload_size > 0)
    {
        ok = LLVKLoader::uploadTextureOneShotVk((U32)job.mRawWidth, (U32)job.mRawHeight,
                                                job.mVkFormat, upload_data, upload_size,
                                                job.mMipCount,
                                                job.mImage, job.mView, job.mAllocation);
    }
    if (padded != nullptr)
    {
        delete[] padded;
    }
    if (!ok)
    {
        return false;
    }

    if (job.mNeedsAlphaAndPickMask)
    {
        job.mIsMask = computeIsMask(data, (U32)job.mRawWidth, (U32)job.mRawHeight,
                                    job.mAlphaStride, job.mAlphaOffset);
        job.mHasMaskResult = true;
        if (job.mFormatType == GL_UNSIGNED_BYTE &&
            (job.mFormatPrimary == GL_RGBA || job.mFormatPrimary == GL_SRGB_ALPHA))
        {
            job.mPickMask = buildPickMask(job.mRawWidth, job.mRawHeight, data,
                                          job.mPickMaskWidth, job.mPickMaskHeight);
        }
    }

    job.mOk = true;
    return true;
}

void LLImageGL::applyVkUploadJob(LLVkTexUploadJob& job)
{
    setSize(job.mFullWidth, job.mFullHeight, job.mComponents, job.mDiscard);
    mCurrentDiscardLevel = (S8)job.mDiscard;

    if (mHasExplicitFormat &&
        ((mFormatPrimary == GL_RGBA && mComponents < 4) ||
         (mFormatPrimary == GL_RGB  && mComponents < 3)))
    {
        mHasExplicitFormat = false;
    }
    mFormatInternal = job.mFormatInternal;
    mFormatPrimary  = job.mFormatPrimary;
    mFormatType     = job.mFormatType;
    calcAlphaChannelOffsetAndStride();

    if (job.mUseMipMaps)
    {
        mAutoGenMips = true;
        mHasMipMaps = true;
        mTexOptionsDirty = true;
        setFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
        mMipLevels = wpo2(llmax(job.mRawWidth, job.mRawHeight));
    }
    else
    {
        mHasMipMaps = false;
        mMipLevels = 0;
    }

    if (mVkImage != VK_NULL_HANDLE || mVkImageView != VK_NULL_HANDLE || mVkAllocation != nullptr)
    {
        LLVKLoader::destroyImageVk(mVkImage, mVkImageView, mVkAllocation);
    }
    mVkImage          = job.mImage;
    mVkImageView      = job.mView;
    mVkAllocation     = job.mAllocation;
    mVkImageWidth     = (U32)job.mRawWidth;
    mVkImageHeight    = (U32)job.mRawHeight;
    mVkImageMipLevels = job.mMipCount;
    mVkImageFormat    = job.mVkFormat;
    job.mImage      = VK_NULL_HANDLE;
    job.mView       = VK_NULL_HANDLE;
    job.mAllocation = nullptr;
    LLGLSLShader::sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
    updateVkHeapSlot();

    if (job.mHasMaskResult)
    {
        mIsMask = job.mIsMask;
    }
    if (job.mNeedsAlphaAndPickMask)
    {
        freePickMask();
        if (job.mPickMask != nullptr)
        {
            mPickMask       = job.mPickMask;
            mPickMaskWidth  = job.mPickMaskWidth;
            mPickMaskHeight = job.mPickMaskHeight;
            job.mPickMask   = nullptr;
        }
    }

    setCategory(job.mCategory);
    mGLTextureCreated = true;

    gGL.getTexUnit(0)->setHasMipMaps(mHasMipMaps);
    gGL.getTexUnit(0)->setTextureAddressMode(mAddressMode);
    gGL.getTexUnit(0)->setTextureFilteringOption(mFilterOption);
    gGL.getTexUnit(0)->unbind(mBindTarget);

    mTextureMemory = (S64Bytes)getMipBytes(mCurrentDiscardLevel);
    mLastBindTime = sLastFrameTime;
}

bool LLImageGL::setSubImage(const U8* datap, S32 data_width, S32 data_height, S32 x_pos, S32 y_pos, S32 width, S32 height, bool force_fast_update /* = false */)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    if (!width || !height)
    {
        return true;
    }
    if (mVkImage == VK_NULL_HANDLE)
    {
        // *TODO: Re-enable warning?  Ran into thread locking issues? DK 2011-02-18
        //LL_WARNS() << "Setting subimage on image without GL texture" << LL_ENDL;
        return false;
    }
    if (datap == NULL)
    {
        // *TODO: Re-enable warning?  Ran into thread locking issues? DK 2011-02-18
        //LL_WARNS() << "Setting subimage on image with NULL datap" << LL_ENDL;
        return false;
    }

    // HACK: allow the caller to explicitly force the fast path (i.e. using glTexSubImage2D here instead of calling setImage) even when updating the full texture.
    if (!force_fast_update && x_pos == 0 && y_pos == 0 && width == getWidth() && height == getHeight() && data_width == width && data_height == height)
    {
        setImage(datap, false);
    }
    else
    {
        if (mUseMipMaps)
        {
            dump();
            LL_ERRS() << "setSubImage called with mipmapped image (not supported)" << LL_ENDL;
        }
        llassert_always(mCurrentDiscardLevel == 0);
        llassert_always(x_pos >= 0 && y_pos >= 0);

        if (((x_pos + width) > getWidth()) ||
            (y_pos + height) > getHeight())
        {
            dump();
            LL_ERRS() << "Subimage not wholly in target image!"
                   << " x_pos " << x_pos
                   << " y_pos " << y_pos
                   << " width " << width
                   << " height " << height
                   << " getWidth() " << getWidth()
                   << " getHeight() " << getHeight()
                   << LL_ENDL;
        }

        if ((x_pos + width) > data_width ||
            (y_pos + height) > data_height)
        {
            dump();
            LL_ERRS() << "Subimage not wholly in source image!"
                   << " x_pos " << x_pos
                   << " y_pos " << y_pos
                   << " width " << width
                   << " height " << height
                   << " source_width " << data_width
                   << " source_height " << data_height
                   << LL_ENDL;
        }


        if (LLVKLoader::shouldUseVulkanRender() && mVkImage != VK_NULL_HANDLE)
        {
            bool ok = LLVKLoader::uploadImageSubregionVk(mVkImage, mVkImageFormat,
                                                        (U32)x_pos, (U32)y_pos,
                                                        (U32)width, (U32)height,
                                                        datap,
                                                        (U32)data_width, (U32)data_height);
            if (!ok)
            {
                LL_WARNS("Vulkan") << "uploadImageSubregionVk failed → VK image invalidate"
                                   << " (stale 継続回避、 次 full setImage で re-sync)"
                                   << " image_gl=0x" << std::hex << (void*)this << std::dec
                                   << " mVkImage=0x" << std::hex << (void*)mVkImage << std::dec
                                   << " x=" << x_pos << " y=" << y_pos
                                   << " w=" << width << " h=" << height
                                   << " data=" << data_width << "x" << data_height
                                   << LL_ENDL;
                LLVKLoader::destroyImageVk(mVkImage, mVkImageView, mVkAllocation);
                mVkImage          = VK_NULL_HANDLE;
                mVkImageView      = VK_NULL_HANDLE;
                mVkAllocation     = nullptr;
                mVkImageWidth     = 0;
                mVkImageHeight    = 0;
                mVkImageMipLevels = 1;
                mVkImageFormat    = VK_FORMAT_UNDEFINED;
                updateVkHeapSlot();
            }
        }
        else if (LLVKLoader::shouldUseVulkanRender())
        {
            const bool full_tight = (x_pos == 0 && y_pos == 0 &&
                                     width == getWidth() && height == getHeight() &&
                                     data_width == width && data_height == height);
            if (full_tight && !isCompressed())
            {
                syncVulkanMip0Image((U32)mFormatInternal, (U32)mFormatPrimary, (U32)mFormatType,
                                    width, height, datap, false);
            }
            else
            {
                LL_WARNS("Vulkan") << "setSubImage: VK storage 未alloc で sub-update 到達"
                                   << " (先行 alloc 失敗/invalidate の二次症状) = 部分/compressed"
                                   << " ゆえ full 復元不可 = skip、 次 full setImage で re-sync。"
                                   << " image_gl=0x" << std::hex << (void*)this << std::dec
                                   << " x=" << x_pos << " y=" << y_pos
                                   << " w=" << width << " h=" << height
                                   << " tex=" << getWidth() << "x" << getHeight()
                                   << " data=" << data_width << "x" << data_height
                                   << " compressed=" << (S32)isCompressed()
                                   << LL_ENDL;
            }
        }
        mGLTextureCreated = true;
    }

    return true;
}

bool LLImageGL::setSubImage(const LLImageRaw* imageraw, S32 x_pos, S32 y_pos, S32 width, S32 height, bool force_fast_update /* = false */)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    return setSubImage(imageraw->getData(), imageraw->getWidth(), imageraw->getHeight(), x_pos, y_pos, width, height, force_fast_update);
}

bool LLImageGL::setSubImageFromFrameBuffer(S32 fb_x, S32 fb_y, S32 x_pos, S32 y_pos, S32 width, S32 height)
{
    mGLTextureCreated = true;

    if (LLVKLoader::shouldUseVulkanRender() && width > 0 && height > 0)
    {
        LLRenderTarget* bound_target = LLRenderTarget::getCurrentBoundTarget();
        if (bound_target != nullptr && bound_target->hasVkImage(0))
        {
            VkFormat src_format = LLVKLoader::llGlEnumToVkFormat(bound_target->getInternalFormat(0));
            bool created_now = false;
            if (src_format != VK_FORMAT_UNDEFINED
                && (mVkImage == VK_NULL_HANDLE
                    || mVkImageWidth != (U32)mWidth
                    || mVkImageHeight != (U32)mHeight
                    || mVkImageFormat != src_format))
            {
                if (mVkImage != VK_NULL_HANDLE || mVkImageView != VK_NULL_HANDLE ||
                    mVkAllocation != nullptr)
                {
                    LLVKLoader::destroyImageVk(mVkImage, mVkImageView, mVkAllocation);
                    mVkImage          = VK_NULL_HANDLE;
                    mVkImageView      = VK_NULL_HANDLE;
                    mVkAllocation     = nullptr;
                    mVkImageWidth     = 0;
                    mVkImageHeight    = 0;
                    mVkImageMipLevels = 1;
                    mVkImageFormat    = VK_FORMAT_UNDEFINED;
                }

                U32 want_mips = 1;
                if (mHasMipMaps)
                {
                    const U32 dim = llmax((U32)mWidth, (U32)mHeight);
                    while ((dim >> want_mips) > 0)
                    {
                        ++want_mips;
                    }
                }

                if (LLVKLoader::createTextureImageVk((U32)mWidth, (U32)mHeight, src_format,
                                                     mVkImage, mVkImageView, mVkAllocation,
                                                     want_mips))
                {
                    mVkImageWidth     = (U32)mWidth;
                    mVkImageHeight    = (U32)mHeight;
                    mVkImageMipLevels = want_mips;
                    mVkImageFormat    = src_format;
                    created_now = true;
                }
                else
                {
                    LL_WARNS_ONCE("Vulkan") << "setSubImageFromFrameBuffer: createTextureImageVk failed"
                                            << " w=" << (S32)mWidth << " h=" << (S32)mHeight
                                            << " fmt=" << (S32)src_format << LL_ENDL;
                }
            }

            if (mVkImage != VK_NULL_HANDLE && mVkImageFormat == src_format)
            {
                const bool in_scope = LLVKLoader::isInRenderPassScope();
                if (in_scope)
                {
                    LLVKLoader::endDynamicRendering();
                }
                LLVKLoader::copyColorImageRegionToImage2DVk(
                    bound_target->getVkImage(0), bound_target->getVkTexLayout(0),
                    fb_x, fb_y,
                    mVkImage,
                    created_now ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    x_pos, y_pos, (U32)width, (U32)height);
                if (in_scope)
                {
                    bound_target->resumeVkDynamicRendering();
                }
            }
            else if (mVkImage != VK_NULL_HANDLE)
            {
                LL_WARNS_ONCE("Vulkan") << "setSubImageFromFrameBuffer: format mismatch skip"
                                        << " tex_fmt=" << (S32)mVkImageFormat
                                        << " rt_fmt=" << (S32)src_format << LL_ENDL;
            }
        }
    }

    return true;
}

bool LLImageGL::createGLTexture(S32 discard_level, const LLImageRaw* imageraw, bool to_create, S32 category, bool defer_copy)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    checkActiveThread();


    if (gGLManager.mIsDisabled)
    {
        LL_WARNS() << "Trying to create a texture while GL is disabled!" << LL_ENDL;
        return false;
    }

    llassert(gGLManager.mInited);

    if (!imageraw || imageraw->isBufferInvalid())
    {
        LL_WARNS() << "Trying to create a texture from invalid image data" << LL_ENDL;
        mGLTextureCreated = false;
        return false;
    }

    if (discard_level < 0)
    {
        llassert(mCurrentDiscardLevel >= 0);
        discard_level = mCurrentDiscardLevel;
    }
    discard_level = llmin(discard_level, MAX_DISCARD_LEVEL);

    S32 raw_w = imageraw->getWidth() ;
    S32 raw_h = imageraw->getHeight() ;

    S32 w = raw_w << discard_level;
    S32 h = raw_h << discard_level;

    if (!setSize(w, h, imageraw->getComponents(), discard_level))
    {
        LL_WARNS() << "Trying to create a texture with incorrect dimensions!" << LL_ENDL;
        mGLTextureCreated = false;
        return false;
    }

    if (mHasExplicitFormat &&
        ((mFormatPrimary == GL_RGBA && mComponents < 4) ||
         (mFormatPrimary == GL_RGB  && mComponents < 3)))

    {
        LL_WARNS()  << "Incorrect format: " << std::hex << mFormatPrimary << " components: " << (U32)mComponents <<  LL_ENDL;
        mHasExplicitFormat = false;
    }

    if( !mHasExplicitFormat )
    {
        switch (mComponents)
        {
        case 1:
            mFormatInternal = GL_LUMINANCE8;
            mFormatPrimary = GL_LUMINANCE;
            mFormatType = GL_UNSIGNED_BYTE;
            break;
        case 2:
            mFormatInternal = GL_LUMINANCE8_ALPHA8;
            mFormatPrimary = GL_LUMINANCE_ALPHA;
            mFormatType = GL_UNSIGNED_BYTE;
            break;
        case 3:
            mFormatInternal = GL_RGB8;
            mFormatPrimary = GL_RGB;
            mFormatType = GL_UNSIGNED_BYTE;
            break;
        case 4:
            mFormatInternal = GL_RGBA8;
            mFormatPrimary = GL_RGBA;
            mFormatType = GL_UNSIGNED_BYTE;
            break;
        default:
            LL_ERRS() << "Bad number of components for texture: " << (U32)getComponents() << LL_ENDL;
        }

        calcAlphaChannelOffsetAndStride() ;
    }

    if(!to_create)
    {
        destroyGLTexture();
        mCurrentDiscardLevel = discard_level;
        mLastBindTime = sLastFrameTime;
        mGLTextureCreated = false;
        return true ;
    }

    setCategory(category);
    const U8* rawdata = imageraw->getData();
    return createGLTexture(discard_level, rawdata, false, defer_copy);
}

bool LLImageGL::createGLTexture(S32 discard_level, const U8* data_in, bool data_hasmips, bool defer_copy)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    LL_PROFILE_GPU_ZONE("createGLTexture");
    checkActiveThread();

    bool main_thread = on_main_thread();

    if (defer_copy)
    {
        data_in = nullptr;
    }
    else
    {
        llassert(data_in);
    }


    if (discard_level < 0)
    {
        llassert(mCurrentDiscardLevel >= 0);
        discard_level = mCurrentDiscardLevel;
    }
    discard_level = llclamp(discard_level, 0, (S32)mMaxDiscardLevel);
    discard_level = llmin(discard_level, MAX_DISCARD_LEVEL);

    if (main_thread
        && !defer_copy
        && mVkImage != VK_NULL_HANDLE && discard_level == mCurrentDiscardLevel)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("cglt - early setImage");
        return setImage(data_in, data_hasmips);
    }

    if (mUseMipMaps)
    {
        mAutoGenMips = true;
    }

    mCurrentDiscardLevel = discard_level;

    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("cglt - late setImage");
        if (!setImage(data_in, data_hasmips))
        {
            return false;
        }
    }

    gGL.getTexUnit(0)->setHasMipMaps(mHasMipMaps);
    gGL.getTexUnit(0)->setTextureAddressMode(mAddressMode);
    gGL.getTexUnit(0)->setTextureFilteringOption(mFilterOption);

    gGL.getTexUnit(0)->unbind(mBindTarget);

    mTextureMemory = (S64Bytes)getMipBytes(mCurrentDiscardLevel);

    mLastBindTime = sLastFrameTime;

    checkActiveThread();
    return true;
}

bool LLImageGL::readBackRaw(S32 discard_level, LLImageRaw* imageraw, bool compressed_ok) const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    if (discard_level < 0)
    {
        discard_level = mCurrentDiscardLevel;
    }

    if (mVkImage == VK_NULL_HANDLE || discard_level < mCurrentDiscardLevel || discard_level > mMaxDiscardLevel )
    {
        return false;
    }

    S32 gl_discard = discard_level - mCurrentDiscardLevel;

    if (gl_discard != 0 || mVkImage == VK_NULL_HANDLE)
    {
        return false;
    }

    S32 ncomponents = getComponents();
    if (ncomponents < 1 || ncomponents > 4)
    {
        return false;
    }

    const U32 vk_width  = mVkImageWidth;
    const U32 vk_height = mVkImageHeight;
    if (vk_width == 0 || vk_height == 0)
    {
        return false;
    }

    S32 src_nc = 0;
    bool src_bgra = false;
    switch (mVkImageFormat)
    {
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_SNORM:
            src_nc = 4; src_bgra = false; break;
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
            src_nc = 4; src_bgra = true; break;
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SNORM:
            src_nc = 2; src_bgra = false; break;
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SNORM:
            src_nc = 1; src_bgra = false; break;
        default:
            src_nc = 0; break;
    }

    const U32 vk_bpp = LLVKLoader::vkFormatBytesPerPixel(mVkImageFormat);
    if (src_nc == 0 || vk_bpp != (U32)src_nc)
    {
        LL_WARNS_ONCE("Vulkan") << "readBackRaw: unsupported Vk format for readback fmt="
                                << (S32)mVkImageFormat << LL_ENDL;
        return false;
    }

    LLImageDataLock lock(imageraw);

    if (!imageraw->allocateDataSize((S32)vk_width, (S32)vk_height, ncomponents))
    {
        LL_WARNS() << "Memory allocation failed for reading back texture." << LL_ENDL;
        LL_WARNS() << "width: " << (S32)vk_width << "height: " << (S32)vk_height << "components: " << ncomponents << LL_ENDL;
        return false;
    }

    const size_t px_count = (size_t)vk_width * (size_t)vk_height;
    std::vector<U8> vk_pixels((size_t)vk_bpp * px_count);

    if (!LLVKLoader::readbackColorImageRegionVk(mVkImage,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            0, 0, vk_width, vk_height, vk_bpp, vk_pixels.data()))
    {
        imageraw->deleteData();
        return false;
    }

    U8* dst = imageraw->getData();
    if (dst == nullptr)
    {
        return false;
    }

    for (size_t p = 0; p < px_count; ++p)
    {
        const U8* s = &vk_pixels[p * vk_bpp];
        U8 r, g, b, a;
        if (src_bgra)
        {
            b = s[0]; g = s[1]; r = s[2]; a = s[3];
        }
        else
        {
            r = s[0];
            g = (src_nc > 1) ? s[1] : (U8)0;
            b = (src_nc > 2) ? s[2] : (U8)0;
            a = (src_nc > 3) ? s[3] : (U8)255;
        }

        U8* d = &dst[p * (size_t)ncomponents];
        switch (ncomponents)
        {
            case 1: d[0] = r; break;
            case 2: d[0] = r; d[1] = g; break;
            case 3: d[0] = r; d[1] = g; d[2] = b; break;
            case 4: d[0] = r; d[1] = g; d[2] = b; d[3] = a; break;
            default: break;
        }
    }

    return true;
}

void LLImageGL::destroyGLTexture()
{
    checkActiveThread();

    bool had_texture = (mVkImage != VK_NULL_HANDLE)
                       || (mVkImageView != VK_NULL_HANDLE) || (mVkAllocation != nullptr);

    if (mVkImage != VK_NULL_HANDLE || mVkImageView != VK_NULL_HANDLE || mVkAllocation != nullptr)
    {
        LLVKLoader::destroyImageVk(mVkImage, mVkImageView, mVkAllocation);
        mVkImage      = VK_NULL_HANDLE;
        mVkImageView  = VK_NULL_HANDLE;
        mVkAllocation = nullptr;
        mVkImageWidth  = 0;
        mVkImageHeight = 0;
        mVkImageFormat = VK_FORMAT_UNDEFINED;
        updateVkHeapSlot();
    }

    if (had_texture)
    {
        if (mTextureMemory != S64Bytes(0))
        {
            mTextureMemory = (S64Bytes)0;
        }
        mCurrentDiscardLevel = -1 ;
        mGLTextureCreated = false ;
    }
}

void LLImageGL::forceToInvalidateGLTexture()
{
    checkActiveThread();
    if (mVkImage != VK_NULL_HANDLE)
    {
        destroyGLTexture();
    }
    else
    {
        mCurrentDiscardLevel = -1 ;
    }
}



void LLImageGL::setAddressMode(LLTexUnit::eTextureAddressMode mode)
{
    if (mAddressMode != mode)
    {
        mTexOptionsDirty = true;
        mAddressMode = mode;
        updateVkHeapSlot();
    }

    if (gGL.getTexUnit(gGL.getCurrentTexUnitIndex())->mCurrImageGL == this)
    {
        gGL.getTexUnit(gGL.getCurrentTexUnitIndex())->setTextureAddressMode(mode);
        mTexOptionsDirty = false;
    }
}

void LLImageGL::setFilteringOption(LLTexUnit::eTextureFilterOptions option)
{
    if (mFilterOption != option)
    {
        mTexOptionsDirty = true;
        mFilterOption = option;
        updateVkHeapSlot();
    }

    if (mVkImage != VK_NULL_HANDLE && gGL.getTexUnit(gGL.getCurrentTexUnitIndex())->mCurrImageGL == this)
    {
        gGL.getTexUnit(gGL.getCurrentTexUnitIndex())->setTextureFilteringOption(option);
        mTexOptionsDirty = false;
    }
}

bool LLImageGL::getIsResident(bool test_now)
{
    if (test_now)
    {
        mIsResident = (mVkImage != VK_NULL_HANDLE);
    }

    return mIsResident;
}

S32 LLImageGL::getHeight(S32 discard_level) const
{
    if (discard_level < 0)
    {
        discard_level = mCurrentDiscardLevel;
    }
    S32 height = mHeight >> discard_level;
    if (height < 1) height = 1;
    return height;
}

S32 LLImageGL::getWidth(S32 discard_level) const
{
    if (discard_level < 0)
    {
        discard_level = mCurrentDiscardLevel;
    }
    S32 width = mWidth >> discard_level;
    if (width < 1) width = 1;
    return width;
}

S64 LLImageGL::getBytes(S32 discard_level) const
{
    if (discard_level < 0)
    {
        discard_level = mCurrentDiscardLevel;
    }
    S32 w = mWidth>>discard_level;
    S32 h = mHeight>>discard_level;
    if (w == 0) w = 1;
    if (h == 0) h = 1;
    return dataFormatBytes(mFormatPrimary, w, h);
}

S64 LLImageGL::getMipBytes(S32 discard_level) const
{
    if (discard_level < 0)
    {
        discard_level = mCurrentDiscardLevel;
    }
    S32 w = mWidth>>discard_level;
    S32 h = mHeight>>discard_level;
    S64 res = dataFormatBytes(mFormatPrimary, w, h);
    if (mUseMipMaps)
    {
        while (w > 1 && h > 1)
        {
            w >>= 1; if (w == 0) w = 1;
            h >>= 1; if (h == 0) h = 1;
            res += dataFormatBytes(mFormatPrimary, w, h);
        }
    }
    return res;
}

bool LLImageGL::isJustBound() const
{
    return sLastFrameTime - mLastBindTime < 0.5f;
}

bool LLImageGL::getBoundRecently() const
{
    return (bool)(sLastFrameTime - mLastBindTime < MIN_TEXTURE_LIFETIME);
}

bool LLImageGL::getIsAlphaMask() const
{
    llassert_always(!sSkipAnalyzeAlpha);
    return mIsMask;
}

void LLImageGL::setTarget(const LLGLenum target, const LLTexUnit::eTextureType bind_target)
{
    mTarget = target;
    mBindTarget = bind_target;
}

const S8 INVALID_OFFSET = -99 ;
void LLImageGL::setNeedsAlphaAndPickMask(bool need_mask)
{
    if(mNeedsAlphaAndPickMask != need_mask)
    {
        mNeedsAlphaAndPickMask = need_mask;

        if(mNeedsAlphaAndPickMask)
        {
            mAlphaOffset = 0 ;
        }
        else //do not need alpha mask
        {
            mAlphaOffset = INVALID_OFFSET ;
            mIsMask = false;
        }
    }
}

void LLImageGL::calcAlphaChannelOffsetAndStride()
{
    if(mAlphaOffset == INVALID_OFFSET)//do not need alpha mask
    {
        return ;
    }

    mAlphaStride = -1 ;
    switch (mFormatPrimary)
    {
    case GL_LUMINANCE:
    case GL_ALPHA:
        mAlphaStride = 1;
        break;
    case GL_LUMINANCE_ALPHA:
        mAlphaStride = 2;
        break;
    case GL_RED:
    case GL_RGB:
    case GL_SRGB:
        mNeedsAlphaAndPickMask = false;
        mIsMask = false;
        return; //no alpha channel.
    case GL_RGBA:
    case GL_SRGB_ALPHA:
        mAlphaStride = 4;
        break;
    case GL_BGRA_EXT:
        mAlphaStride = 4;
        break;
    default:
        break;
    }

    mAlphaOffset = -1 ;
    if (mFormatType == GL_UNSIGNED_BYTE)
    {
        mAlphaOffset = mAlphaStride - 1 ;
    }
    else if(is_little_endian())
    {
        if (mFormatType == GL_UNSIGNED_INT_8_8_8_8)
        {
            mAlphaOffset = 0 ;
        }
        else if (mFormatType == GL_UNSIGNED_INT_8_8_8_8_REV)
        {
            mAlphaOffset = 3 ;
        }
    }
    else //big endian
    {
        if (mFormatType == GL_UNSIGNED_INT_8_8_8_8)
        {
            mAlphaOffset = 3 ;
        }
        else if (mFormatType == GL_UNSIGNED_INT_8_8_8_8_REV)
        {
            mAlphaOffset = 0 ;
        }
    }

    if( mAlphaStride < 1 || //unsupported format
        mAlphaOffset < 0 || //unsupported type
        (mFormatPrimary == GL_BGRA_EXT && mFormatType != GL_UNSIGNED_BYTE)) //unknown situation
    {
        LL_WARNS() << "Cannot analyze alpha for image with format type " << std::hex << mFormatType << std::dec << LL_ENDL;

        mNeedsAlphaAndPickMask = false ;
        mIsMask = false;
    }
}

bool LLImageGL::computeIsMask(const void* data_in, U32 w, U32 h, S8 alpha_stride, S8 alpha_offset)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    U32 length = w * h;
    U32 alphatotal = 0;

    U32 sample[16];
    memset(sample, 0, sizeof(U32)*16);

    if (w >= 2 && h >= 2)
    {
        llassert(w % 2 == 0);
        llassert(h % 2 == 0);
        const GLubyte* rowstart = ((const GLubyte*) data_in) + alpha_offset;
        for (U32 y = 0; y < h; y += 2)
        {
            const GLubyte* current = rowstart;
            for (U32 x = 0; x < w; x += 2)
            {
                const U32 s1 = current[0];
                alphatotal += s1;
                const U32 s2 = current[w * alpha_stride];
                alphatotal += s2;
                current += alpha_stride;
                const U32 s3 = current[0];
                alphatotal += s3;
                const U32 s4 = current[w * alpha_stride];
                alphatotal += s4;
                current += alpha_stride;

                ++sample[s1/16];
                ++sample[s2/16];
                ++sample[s3/16];
                ++sample[s4/16];

                const U32 asum = (s1+s2+s3+s4);
                alphatotal += asum;
                sample[asum/(16*4)] += 4;
            }

            rowstart += 2 * w * alpha_stride;
        }
        length *= 2; // we sampled everything twice, essentially
    }
    else
    {
        const GLubyte* current = ((const GLubyte*) data_in) + alpha_offset;
        for (U32 i = 0; i < length; i++)
        {
            const U32 s1 = *current;
            alphatotal += s1;
            ++sample[s1/16];
            current += alpha_stride;
        }
    }

    U32 midrangetotal = 0;
    for (U32 i = 2; i < 13; i++)
    {
        midrangetotal += sample[i];
    }
    U32 lowerhalftotal = 0;
    for (U32 i = 0; i < 8; i++)
    {
        lowerhalftotal += sample[i];
    }
    U32 upperhalftotal = 0;
    for (U32 i = 8; i < 16; i++)
    {
        upperhalftotal += sample[i];
    }

    if (midrangetotal > length/48 || // lots of midrange, or
        (lowerhalftotal == length && alphatotal != 0) || // all close to transparent but not all totally transparent, or
        (upperhalftotal == length && alphatotal != 255*length)) // all close to opaque but not all totally opaque
    {
        return false; // not suitable for masking
    }
    return true;
}

void LLImageGL::analyzeAlpha(const void* data_in, U32 w, U32 h)
{
    if(sSkipAnalyzeAlpha || !mNeedsAlphaAndPickMask)
    {
        return ;
    }

    mIsMask = computeIsMask(data_in, w, h, mAlphaStride, mAlphaOffset);
}


void LLImageGL::freePickMask()
{
    if (mPickMask != NULL)
    {
        delete [] mPickMask;
    }
    mPickMask = NULL;
    mPickMaskWidth = mPickMaskHeight = 0;
}

bool LLImageGL::isCompressed()
{
    llassert(mFormatPrimary != 0);
    bool is_compressed = false;
    switch (mFormatPrimary)
    {
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        is_compressed = true;
        break;
    default:
        break;
    }
    return is_compressed;
}


U8* LLImageGL::buildPickMask(S32 width, S32 height, const U8* data_in, U16& out_width, U16& out_height)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    const U32 pick_width = width/2 + 1;
    const U32 pick_height = height/2 + 1;

    U32 size = pick_width * pick_height;
    size = (size + 7) / 8; // pixelcount-to-bits
    U8* mask = new (std::nothrow) U8[size];
    if (mask == nullptr)
    {
        out_width = 0;
        out_height = 0;
        return nullptr;
    }
    memset(mask, 0, sizeof(U8) * size);
    out_width = (U16)(pick_width - 1);
    out_height = (U16)(pick_height - 1);

    U32 pick_bit = 0;

    for (S32 y = 0; y < height; y += 2)
    {
        for (S32 x = 0; x < width; x += 2)
        {
            U8 alpha = data_in[(y*width+x)*4+3];

            if (alpha > 32)
            {
                U32 pick_idx = pick_bit/8;
                U32 pick_offset = pick_bit%8;
                llassert(pick_idx < size);

                mask[pick_idx] |= 1 << pick_offset;
            }

            ++pick_bit;
        }
    }

    return mask;
}

void LLImageGL::updatePickMask(S32 width, S32 height, const U8* data_in)
{
    if(!mNeedsAlphaAndPickMask)
    {
        return ;
    }

    if (mFormatType != GL_UNSIGNED_BYTE ||
        ((mFormatPrimary != GL_RGBA)
      && (mFormatPrimary != GL_SRGB_ALPHA)))
    {
        freePickMask();
        return;
    }

    freePickMask();
    mPickMask = buildPickMask(width, height, data_in, mPickMaskWidth, mPickMaskHeight);
    if (mPickMask == nullptr)
    {
        mPickMaskWidth = 0;
        mPickMaskHeight = 0;
    }
}

// [RLVa:KB] - Checked: RLVa-2.2 (@setoverlay)
bool LLImageGL::getMask(const LLVector2 &tc) const
// [/RLVa:KB]
{
    bool res = true;

    if (mPickMask)
    {
        F32 u,v;
        if (LL_LIKELY(tc.isFinite()))
        {
            u = tc.mV[0] - floorf(tc.mV[0]);
            v = tc.mV[1] - floorf(tc.mV[1]);
        }
        else
        {
            LL_WARNS_ONCE("render") << "Ugh, non-finite u/v in mask pick" << LL_ENDL;
            u = v = 0.f;
        }

        if (LL_UNLIKELY(u < 0.f || u > 1.f ||
                v < 0.f || v > 1.f))
        {
            LL_WARNS_ONCE("render") << "Ugh, u/v out of range in image mask pick" << LL_ENDL;
            u = v = 0.f;
        }

        S32 x = llfloor(u * mPickMaskWidth);
        S32 y = llfloor(v * mPickMaskHeight);

        if (LL_UNLIKELY(x > mPickMaskWidth))
        {
            LL_WARNS_ONCE("render") << "Ooh, width overrun on pick mask read, that coulda been bad." << LL_ENDL;
            x = llmax((U16)0, mPickMaskWidth);
        }
        if (LL_UNLIKELY(y > mPickMaskHeight))
        {
            LL_WARNS_ONCE("render") << "Ooh, height overrun on pick mask read, that woulda been bad." << LL_ENDL;
            y = llmax((U16)0, mPickMaskHeight);
        }

        S32 idx = y*mPickMaskWidth+x;
        S32 offset = idx%8;

        res = (mPickMask[idx/8] & (1 << offset)) != 0;
    }

    return res;
}

void LLImageGL::setCurTexSizebar(S32 index, bool set_pick_size)
{
    sCurTexSizeBar = index ;

    if(set_pick_size)
    {
        sCurTexPickSize = (1 << index) ;
    }
    else
    {
        sCurTexPickSize = -1 ;
    }
}
void LLImageGL::resetCurTexSizebar()
{
    sCurTexSizeBar = -1 ;
    sCurTexPickSize = -1 ;
}

bool LLImageGL::scaleDown(S32 desired_discard)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    if (mTarget != GL_TEXTURE_2D
        || mFormatInternal == -1 // not initialized
        )
    {
        return false;
    }

    desired_discard = llmin(desired_discard, mMaxDiscardLevel);

    if (desired_discard <= mCurrentDiscardLevel)
    {
        return false;
    }

    S32 mip = desired_discard - mCurrentDiscardLevel;

    S32 desired_width = getWidth(desired_discard);
    S32 desired_height = getHeight(desired_discard);

    if (LLVKLoader::shouldUseVulkanRender())
    {
        if (mVkImage == VK_NULL_HANDLE || mVkImageFormat == VK_FORMAT_UNDEFINED)
        {
            return false;
        }

        const U32 src_mip    = (U32)llmin(mip, (S32)mVkImageMipLevels - 1);
        const S32 src_width  = llmax(1, (S32)(mVkImageWidth  >> src_mip));
        const S32 src_height = llmax(1, (S32)(mVkImageHeight >> src_mip));

        U32 dst_mips = 1;
        if (mHasMipMaps)
        {
            S32 dim = llmax(desired_width, desired_height);
            while (dim > 1) { dim >>= 1; ++dst_mips; }
        }

        VkImage     new_image = VK_NULL_HANDLE;
        VkImageView new_view  = VK_NULL_HANDLE;
        void*       new_alloc = nullptr;
        if (!LLVKLoader::downscaleImageVk(mVkImage, src_mip, (U32)src_width, (U32)src_height,
                                          (U32)desired_width, (U32)desired_height, mVkImageFormat,
                                          dst_mips, new_image, new_view, new_alloc))
        {
            return false;
        }

        setExternalVkBacking(new_image, new_view, new_alloc,
                             (U32)desired_width, (U32)desired_height, mVkImageFormat, dst_mips);

        mCurrentDiscardLevel = desired_discard;
        return true;
    }

    return false;
}



#if LL_IMAGEGL_THREAD_CHECK
void LLImageGL::checkActiveThread()
{
    llassert(mActiveThread == LLThread::currentID());
}
#endif




LLImageGLThread::LLImageGLThread(LLWindow* window)
    // We want exactly one thread.
    : LL::ThreadPool("LLImageGL", 1)
    , mWindow(window)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    mFinished = false;

    mContext = mWindow->createSharedContext();

    // <FS:ND> If context creating is not supported (SDL1), mark texture thread disabled and exit
    if( !mContext )
    {
        sEnabledTextures = false;
        sEnabledMedia = false;
        mFinished = true;
        return;
    }
    // </FS:ND>

    LL::ThreadPool::start();
}

void LLImageGLThread::run()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
    // We must perform setup on this thread before actually servicing our
    // WorkQueue, likewise cleanup afterwards.
    mWindow->makeContextCurrent(mContext);
    gGL.init(false);
    LL_PROFILER_GPU_CONTEXT_NS("LLImageGL Context", 17);
    LL::ThreadPool::run();
    gGL.shutdown();
    mWindow->destroySharedContext(mContext);
}

