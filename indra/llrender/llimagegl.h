/**
 * @file llimagegl.h
 * @brief Object for managing images and their textures
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


#ifndef LL_LLIMAGEGL_H
#define LL_LLIMAGEGL_H

#include "llimage.h"

#include "llgltypes.h"
#include "llpointer.h"
#include "llrefcount.h"
#include "v2math.h"
#include "llunits.h"
#include "llthreadsafequeue.h"
#include "llrender.h"
#include "llvktexresidency.h"
#include "threadpool.h"
#include "workqueue.h"
#include <unordered_set>

#define LL_IMAGEGL_THREAD_CHECK 0 //set to 1 to enable thread debugging for ImageGL

class LLWindow;

#define BYTES_TO_MEGA_BYTES(x) ((x) >> 20)
#define MEGA_BYTES_TO_BYTES(x) ((x) << 20)

class LLImageGL : public LLRefCount
{
    friend class LLTexUnit;
public:

    static U64 getTextureBytesAllocated();
    static U64 getVkTextureBytesAllocated();

    static S32 dataFormatBits(S32 dataformat);
    static S64 dataFormatBytes(S32 dataformat, S32 width, S32 height);
    static S32 dataFormatComponents(S32 dataformat);

    bool updateBindStats() const ;
    F32 getTimePassedSinceLastBound();
    void forceUpdateBindStats(void) const;

    static void updateStats(F32 current_time);
    static void destroyGL();
    static void dirtyTexOptions();

    static bool checkSize(S32 width, S32 height);

    static bool create(LLPointer<LLImageGL>& dest, bool usemipmaps = true);
    static bool create(LLPointer<LLImageGL>& dest, U32 width, U32 height, U8 components, bool usemipmaps = true);
    static bool create(LLPointer<LLImageGL>& dest, const LLImageRaw* imageraw, bool usemipmaps = true);

public:
    LLImageGL(bool usemipmaps = true, bool allow_compression = true);
    LLImageGL(U32 width, U32 height, U8 components, bool usemipmaps = true, bool allow_compression = true);
    LLImageGL(const LLImageRaw* imageraw, bool usemipmaps = true, bool allow_compression = true);

    LLImageGL(U32 components, LLGLenum target, LLGLint  formatInternal, LLGLenum formatPrimary, LLGLenum formatType, LLTexUnit::eTextureAddressMode addressMode);

protected:
    virtual ~LLImageGL();

    void analyzeAlpha(const void* data_in, U32 w, U32 h);
    void calcAlphaChannelOffsetAndStride();

public:
    virtual void dump();    // debugging info to LL_INFOS()

    void setSize(S32 width, S32 height, S32 ncomponents, S32 discard_level = -1);
    void setComponents(S32 ncomponents) { mComponents = (S8)ncomponents ;}
    void setAllowCompression(bool allow) { mAllowCompression = allow; }


    bool createGLTexture(S32 discard_level, const LLImageRaw* imageraw, bool to_create = true,
        S32 category = sMaxCategories-1, bool defer_copy = false);
    bool createGLTexture(S32 discard_level, const U8* data, bool data_hasmips = false, bool defer_copy = false);
    bool setImage(const LLImageRaw* imageraw);
    bool setImage(const U8* data_in, bool data_hasmips = false);
    bool setSubImage(const LLImageRaw* imageraw, S32 x_pos, S32 y_pos, S32 width, S32 height, bool force_fast_update = false);
    bool setSubImage(const U8* datap, S32 data_width, S32 data_height, S32 x_pos, S32 y_pos, S32 width, S32 height, bool force_fast_update = false);
    bool setSubImageFromFrameBuffer(S32 fb_x, S32 fb_y, S32 x_pos, S32 y_pos, S32 width, S32 height);

    bool readBackRaw(S32 discard_level, LLImageRaw* imageraw, bool compressed_ok) const;
    void destroyGLTexture();
    void forceToInvalidateGLTexture();

    void setExplicitFormat(LLGLint internal_format, LLGLenum primary_format, LLGLenum type_format = 0, bool swap_bytes = false);
    void setComponents(S8 ncomponents) { mComponents = ncomponents; }

    S32  getDiscardLevel() const        { return mCurrentDiscardLevel; }
    S32  getMaxDiscardLevel() const     { return mMaxDiscardLevel; }

    void setDiscardLevel(S32 level) { mCurrentDiscardLevel = level; }

    S32  getCurrentWidth() const { return mWidth ;}
    S32  getCurrentHeight() const { return mHeight ;}
    S32  getWidth(S32 discard_level = -1) const;
    S32  getHeight(S32 discard_level = -1) const;
    U8   getComponents() const { return mComponents; }
    S64  getBytes(S32 discard_level = -1) const;
    S64  getMipBytes(S32 discard_level = -1) const;
    bool getBoundRecently() const;
    bool isJustBound() const;
    bool getHasExplicitFormat() const { return mHasExplicitFormat; }
    LLGLenum getPrimaryFormat() const { return mFormatPrimary; }
    LLGLenum getFormatType() const { return mFormatType; }

    bool getHasGLTexture() const { return mVkRes.isLive(); }

    VkImageView getVkImageView() const { return mVkRes.view(); }
    bool hasVkImage() const { return mVkRes.isLive(); }
    VkImage getVkImage() const { return mVkRes.image(); }
    U32      getVkImageMipLevels() const { return mVkRes.mips(); }
    U32      getVkImageWidth() const { return mVkRes.width(); }
    U32      getVkImageHeight() const { return mVkRes.height(); }
    VkFormat getVkImageFormat() const { return mVkRes.format(); }

    U32  getVkHeapSlot() const { return mVkRes.slot(); }
    static U32 vkHeapSlotOrDefault(LLImageGL* gl);

    bool commitVkBacking(const VkBacking& b);
    void resampleVkSlot();
    U32  ensureVkSlot();

    void setExternalVkBacking(VkImage image, VkImageView view, void* allocation, U32 w, U32 h, VkFormat format, U32 mip_levels = 1);

    bool syncVulkan3DImage(U32 intformat, U32 primary, U32 type, S32 w, S32 h, S32 depth, const void* data);

    bool syncVulkanMip0Image(U32 intformat, U32 primary, U32 type, S32 w, S32 h, const void* data, bool is_compressed,
                             S32 mip_level = 0, S32 mip_count = 1);

    static bool computeIsMask(const void* data_in, U32 w, U32 h, S8 alpha_stride, S8 alpha_offset);
    static U8* buildPickMask(S32 width, S32 height, const U8* data_in, U16& out_width, U16& out_height);

    bool getIsAlphaMask() const;

    bool getIsResident(bool test_now = false); // not const

    void setTarget(const LLGLenum target, const LLTexUnit::eTextureType bind_target);

    LLTexUnit::eTextureType getTarget(void) const { return mBindTarget; }
    bool isGLTextureCreated(void) const { return mGLTextureCreated ; }
    void setGLTextureCreated (bool initialized) { mGLTextureCreated = initialized; }

    bool getUseMipMaps() const { return mUseMipMaps; }
    void setUseMipMaps(bool usemips) { mUseMipMaps = usemips; }
    void setHasMipMaps(bool hasmips) { mHasMipMaps = hasmips; }
    void updatePickMask(S32 width, S32 height, const U8* data_in);
// [RLVa:KB] - Checked: RLVa-2.2 (@setoverlay)
    bool getMask(const LLVector2 &tc) const;
// [/RLVa:KB]
    void setAddressMode(LLTexUnit::eTextureAddressMode mode);
    LLTexUnit::eTextureAddressMode getAddressMode(void) const { return mAddressMode; }

    void setFilteringOption(LLTexUnit::eTextureFilterOptions option);
    LLTexUnit::eTextureFilterOptions getFilteringOption(void) const { return mFilterOption; }

    LLGLenum getTexTarget()const { return mTarget; }

    void init(bool usemipmaps, bool allow_compression);
    virtual void cleanup();

    void setNeedsAlphaAndPickMask(bool need_mask);

#if LL_IMAGEGL_THREAD_CHECK
    // thread debugging
    std::thread::id mActiveThread;
    void checkActiveThread();
#endif

    bool scaleDown(S32 desired_discard);

public:
    S64Bytes mTextureMemory;
    mutable F32  mLastBindTime;

private:
    void freePickMask();
    bool isCompressed();

    LLPointer<LLImageRaw> mSaveData;
    LL::WorkQueue::weak_t mMainQueue;
    U8* mPickMask;
    U16 mPickMaskWidth;
    U16 mPickMaskHeight;
    S8 mUseMipMaps;
    bool mHasExplicitFormat;
    bool mAutoGenMips = false;

    bool mIsMask;
    bool mNeedsAlphaAndPickMask;
    S8   mAlphaStride ;
    S8   mAlphaOffset ;

    bool     mGLTextureCreated ;
    U16      mWidth;
    U16      mHeight;
    S8       mCurrentDiscardLevel;

    bool mAllowCompression;

protected:
    LLGLenum mTarget;
    LLTexUnit::eTextureType mBindTarget;
    bool mHasMipMaps;
    S32 mMipLevels;

    LLGLboolean mIsResident;

    S8 mComponents;
    S8 mMaxDiscardLevel;

    bool    mTexOptionsDirty;
    LLTexUnit::eTextureAddressMode      mAddressMode;
    LLTexUnit::eTextureFilterOptions    mFilterOption;

    LLGLint  mFormatInternal;
    LLGLenum mFormatPrimary;
    LLGLenum mFormatType;
    bool     mFormatSwapBytes;

    bool mExternalTexture;

    VkTexResidency mVkRes;

public:
    static std::unordered_set<LLImageGL*> sImageList;
    static S32 sCount;
    static F32 sLastFrameTime;

    static U32 sBindCount;
    static U32 sUniqueCount;
    static bool sGlobalUseAnisotropic;
    static LLImageGL* sDefaultGLTexture ;
    static LLImageGL* sWhiteImageGLp ;
    static bool sAutomatedTest;
    static bool sCompressTextures;
#if DEBUG_MISS
    bool mMissed; // Missed on last bind?
    bool getMissed() const { return mMissed; };
#else
    bool getMissed() const { return false; };
#endif

public:
    static void initClass(LLWindow* window, S32 num_catagories, bool skip_analyze_alpha = false, bool thread_texture_loads = false, bool thread_media_updates = false);
    static void cleanupClass() ;

private:
    static S32 sMaxCategories;
    static bool sSkipAnalyzeAlpha;

    static bool sAllowReadBackRaw ;
//
//The below for texture auditing use only
private:
    S32 mCategory ;
public:
    void setCategory(S32 category) {mCategory = category;}
    S32  getCategory()const {return mCategory;}



    static S32 sCurTexSizeBar ;
    static S32 sCurTexPickSize ;

    static void setCurTexSizebar(S32 index, bool set_pick_size = true) ;
    static void resetCurTexSizebar();

//End of definitions for texture auditing use only

};

class LLImageGLThread : public LLSimpleton<LLImageGLThread>, LL::ThreadPool
{
public:
    // follows gSavedSettings "RenderGLMultiThreadedTextures"
    static bool sEnabledTextures;
    // follows gSavedSettings "RenderGLMultiThreadedMedia"
    static bool sEnabledMedia;

    LLImageGLThread(LLWindow* window);

    // post a function to be executed on the LLImageGL background thread
    template <typename CALLABLE>
    bool post(CALLABLE&& func)
    {
        return getQueue().post(std::forward<CALLABLE>(func));
    }

    void run() override;

private:
    LLWindow* mWindow;
    void* mContext = nullptr;
    LLAtomicBool mFinished;
};

#endif // LL_LLIMAGEGL_H
