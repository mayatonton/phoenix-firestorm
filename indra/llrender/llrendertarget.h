/**
 * @file llrendertarget.h
 * @brief Off screen render target abstraction.
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

#ifndef LL_LLRENDERTARGET_H
#define LL_LLRENDERTARGET_H

#include "llgl.h"
#include "llrender.h"

/*
 Render-to-texture target backed by Vulkan images.

 SAMPLE USAGE:

    LLRenderTarget target;

    ...

    //allocate a 256x256 RGBA render target with depth buffer
    target.allocate(256,256,GL_RGBA,TRUE);

    //render to contents of offscreen buffer
    target.bindTarget();
    target.clear();
    ... <issue drawing commands> ...
    target.flush();

    ...

    //use target as a texture
    gGL.getTexUnit(INDEX)->bind(&target);
    ... <issue drawing commands> ...

*/

class LLRenderTarget
{
public:
    static U32 sBytesAllocated;
    static thread_local U32 sCurResX;
    static thread_local U32 sCurResY;


    LLRenderTarget();
    ~LLRenderTarget();

    //allocate resources for rendering
    //must be called before use
    //multiple calls will release previously allocated resources
    // resX - width
    // resY - height
    // color_fmt - GL color format (e.g. GL_RGB)
    // depth - if true, allocate a depth buffer
    // usage - deprecated, should always be TT_TEXTURE
    bool allocate(U32 resx, U32 resy, U32 color_fmt, bool depth = false, LLTexUnit::eTextureType usage = LLTexUnit::TT_TEXTURE, LLTexUnit::eTextureMipGeneration generateMipMaps = LLTexUnit::TMG_NONE);

    //resize existing attachments to use new resolution and color format
    // CAUTION: if the GL runs out of memory attempting to resize, this render target will be undefined
    // DO NOT use for screen space buffers or for scratch space for an image that might be uploaded
    // DO use for render targets that resize often and aren't likely to ruin someone's day if they break
    void resize(U32 resx, U32 resy);

    //point this render target at a particular LLImageGL
    //   Intended usage:
    //      LLRenderTarget target;
    //      target.setColorAttachment(image);
    //      target.bindTarget();
    //      < issue draw calls>
    //      target.flush();
    //      target.releaseColorAttachment();
    //
    // attachment -- LLImageGL to render into
    // NOTE: setColorAttachment and releaseColorAttachment cannot be used in conjuction with
    // addColorAttachment, allocateDepth, resize, etc.
    void setColorAttachment(LLImageGL* attachment);

    // detach from current color attachment
    void releaseColorAttachment();

    //add color buffer attachment
    //limit of 4 color attachments per render target
    bool addColorAttachment(U32 color_fmt);

    //allocate a depth attachment
    void allocateDepth();

    //share depth buffer with provided render target
    void shareDepthBuffer(LLRenderTarget& target);

    //free any allocated resources
    //safe to call redundantly
    // asserts that this target is not currently bound or present in the RT stack
    void release();

    //bind target for rendering
    //applies appropriate viewport
    //  If an LLRenderTarget is currently bound, stores a reference to that LLRenderTarget
    //  and restores previous binding on flush() (maintains a stack of Render Targets)
    //  Asserts that this target is not currently bound in the stack
    void bindTarget();

    //clear render targer, clears depth buffer if present,
    //uses scissor rect if in copy-to-texture mode
    // asserts that this target is currently bound
    void clear(U32 mask = 0xFFFFFFFF);

    //get applied viewport
    void getViewport(S32* viewport);

    //get X resolution
    U32 getWidth() const { return mResX; }

    //get Y resolution
    U32 getHeight() const { return mResY; }

    LLTexUnit::eTextureType getUsage(void) const { return mUsage; }

    U32 getNumTextures() const;

    void setUseDepthCompareSampler(bool b) { mUseDepthCompareSampler = b; }
    bool usesDepthCompareSampler() const   { return mUseDepthCompareSampler; }

    U32  getInternalFormat(U32 attachment = 0) const { return mInternalFormat[attachment]; }
    bool hasDepth() const { return mUseDepth; }

    VkImageView getVkImageView(U32 attachment = 0) const
    {
        if (attachment < mVkTexSampleView.size() && mVkTexSampleView[attachment] != VK_NULL_HANDLE)
        {
            return mVkTexSampleView[attachment];
        }
        return (attachment < mVkTexView.size()) ? mVkTexView[attachment] : VK_NULL_HANDLE;
    }
    VkImageView getVkDepthView() const { return mVkDepthView; }

    VkImageView getVkAttachmentView(U32 attachment = 0) const
    {
        return (attachment < mVkTexView.size()) ? mVkTexView[attachment] : VK_NULL_HANDLE;
    }

    VkImage getVkImage(U32 attachment = 0) const
    {
        return (attachment < mVkTex.size()) ? mVkTex[attachment] : VK_NULL_HANDLE;
    }
    bool hasVkImage(U32 attachment = 0) const
    {
        return (attachment < mVkTex.size()) && mVkTex[attachment] != VK_NULL_HANDLE;
    }
    bool hasVkDepth() const { return mVkDepth != VK_NULL_HANDLE; }
    VkImage getVkDepthImage() const { return mVkDepth; }

    VkImageLayout getVkTexLayout(U32 attachment = 0) const
    {
        return (attachment < mVkTexLayout.size()) ? mVkTexLayout[attachment] : VK_IMAGE_LAYOUT_UNDEFINED;
    }
    VkImageLayout getVkDepthLayout() const { return getCurDepthLayout(); }

    void bindForShaderRead(U32 attachment = 0, bool depth = false);

    bool isVkActivePassAttachment(U32 attachment = 0, bool depth = false) const;

    bool copyContentsInFrameVk(LLRenderTarget& source);

    void bindTexture(U32 index, S32 channel, LLTexUnit::eTextureFilterOptions filter_options = LLTexUnit::TFO_BILINEAR);

    //flush rendering operations
    //must be called when rendering is complete
    //should be used 1:1 with bindTarget
    // call bindTarget once, do all your rendering, call flush once
    // If an LLRenderTarget was bound when bindTarget was called, binds that RenderTarget for rendering (maintains RT stack)
    // asserts  that this target is currently bound
    void flush();

    void resumeVkDynamicRendering();

    //Returns TRUE if target is ready to be rendered into.
    //That is, if the target has been allocated with at least
    //one renderable attachment (i.e. color buffer, depth buffer).
    bool isComplete() const;

    // Returns true if this RenderTarget is bound somewhere in the stack
    bool isBoundInStack() const;

    bool wasWrittenThisFrame() const;

    static LLRenderTarget* getCurrentBoundTarget() { return sBoundTarget; }

    static void clearBoundTarget(U32 mask = 0xFFFFFFFF);

    // *HACK
    void swapFBORefs(LLRenderTarget& other);

    void setVkDepthLayout(VkImageLayout layout) { setCurDepthLayout(layout); }

    void setVkTexLayout(U32 attachment, VkImageLayout layout)
    {
        if (attachment < mVkTexLayout.size())
        {
            mVkTexLayout[attachment] = layout;
        }
    }

    static thread_local LLRenderTarget* sBoundTarget;

protected:
    U32 mResX;
    U32 mResY;
    U32 mLastBoundMonotonicFrame = 0;
    std::vector<U32> mInternalFormat;
    bool mAllocated = false;
    LLRenderTarget* mPreviousRT = nullptr;

    bool mOwnDepth = false;
    bool mUseDepth;
    bool mUseDepthCompareSampler = false;
    LLTexUnit::eTextureMipGeneration mGenerateMipMaps;
    U32 mMipLevels;

    LLTexUnit::eTextureType mUsage;

    std::vector<VkImage>     mVkTex;
    std::vector<VkImageView> mVkTexView;
    std::vector<VkImageView> mVkTexSampleView;
    std::vector<void*>       mVkTexAlloc;

    VkImage     mVkDepth      = VK_NULL_HANDLE;
    VkImageView mVkDepthView  = VK_NULL_HANDLE;
    void*       mVkDepthAlloc = nullptr;

    std::vector<VkImageLayout> mVkTexLayout;
    VkImageLayout              mVkDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    LLRenderTarget*            mVkDepthLayoutOwner = nullptr;

    VkImageLayout getCurDepthLayout() const
    {
        return mVkDepthLayoutOwner ? mVkDepthLayoutOwner->mVkDepthLayout : mVkDepthLayout;
    }
    void setCurDepthLayout(VkImageLayout layout)
    {
        if (mVkDepthLayoutOwner) { mVkDepthLayoutOwner->mVkDepthLayout = layout; }
        else                     { mVkDepthLayout = layout; }
    }
};

#endif

