/**
 * @file llrendertarget.cpp
 * @brief LLRenderTarget implementation
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

#include "llrendertarget.h"
#include "llrender.h"
#include "llgl.h"
#include "llvkloader.h"

LLRenderTarget* LLRenderTarget::sBoundTarget = NULL;
U32 LLRenderTarget::sBytesAllocated = 0;

extern S32 gGLViewport[4];

U32 LLRenderTarget::sCurResX = 0;
U32 LLRenderTarget::sCurResY = 0;

LLRenderTarget::LLRenderTarget() :
    mResX(0),
    mResY(0),
    mUseDepth(false),
    mUsage(LLTexUnit::TT_TEXTURE)
{
}

LLRenderTarget::~LLRenderTarget()
{
    release();
}

void LLRenderTarget::resize(U32 resx, U32 resy)
{
    S32 pix_diff = (resx*resy)-(mResX*mResY);

    mResX = resx;
    mResY = resy;

    for (U32 i = 0; i < mInternalFormat.size(); ++i)
    {
        sBytesAllocated += pix_diff*4;
    }

    if (mOwnDepth)
    {
        sBytesAllocated += pix_diff*4;
    }

    if (LLVKLoader::isVulkanInitialized())
    {
        for (size_t i = 0; i < mVkTex.size(); ++i)
        {
            LLVKLoader::destroyImageVk(
                mVkTex[i],
                i < mVkTexView.size()  ? mVkTexView[i]  : VK_NULL_HANDLE,
                i < mVkTexAlloc.size() ? mVkTexAlloc[i] : nullptr);
            if (i < mVkTexSampleView.size() && mVkTexSampleView[i] != VK_NULL_HANDLE)
            {
                LLVKLoader::destroyImageVk(VK_NULL_HANDLE, mVkTexSampleView[i], nullptr);
            }
        }
        mVkTex.clear();
        mVkTexView.clear();
        mVkTexSampleView.clear();
        mVkTexAlloc.clear();
        mVkTexLayout.clear();

        for (size_t i = 0; i < mInternalFormat.size(); ++i)
        {
            VkImage     vk_image       = VK_NULL_HANDLE;
            VkImageView vk_view        = VK_NULL_HANDLE;
            VkImageView vk_sample_view = VK_NULL_HANDLE;
            void*       vk_allocation  = nullptr;
            VkFormat    vk_format      = LLVKLoader::llGlEnumToVkFormat(mInternalFormat[i]);
            const U32 vk_mip = (mGenerateMipMaps != LLTexUnit::TMG_NONE && i == 0 && mMipLevels > 1)
                               ? mMipLevels : 1;
            if (!LLVKLoader::createColorAttachmentImageVk(mResX, mResY, vk_format,
                                                          vk_image, vk_view, vk_allocation,
                                                          vk_mip,
                                                          (vk_mip > 1) ? &vk_sample_view : nullptr))
            {
                LL_WARNS("Vulkan") << "createColorAttachmentImageVk failed (resize) = NULL placeholder"
                                   << " で index 整合維持 attachment=" << (S32)i
                                   << " fmt=0x" << std::hex << mInternalFormat[i] << std::dec
                                   << " vk_fmt=" << (S32)vk_format
                                   << " res=" << (S32)mResX << "x" << (S32)mResY << LL_ENDL;
            }
            mVkTex.push_back(vk_image);
            mVkTexView.push_back(vk_view);
            mVkTexSampleView.push_back(vk_sample_view);
            mVkTexAlloc.push_back(vk_allocation);
            mVkTexLayout.push_back(vk_image != VK_NULL_HANDLE
                                   ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                   : VK_IMAGE_LAYOUT_UNDEFINED);
        }

        if (mVkDepth != VK_NULL_HANDLE)
        {
            if (mVkDepthAlloc != nullptr)
            {
                LLVKLoader::destroyImageVk(mVkDepth, mVkDepthView, mVkDepthAlloc);
            }
            mVkDepth            = VK_NULL_HANDLE;
            mVkDepthView        = VK_NULL_HANDLE;
            mVkDepthAlloc       = nullptr;
            mVkDepthLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
            mVkDepthLayoutOwner = nullptr;
        }
        if (mOwnDepth)
        {
            VkImage     vk_image      = VK_NULL_HANDLE;
            VkImageView vk_view       = VK_NULL_HANDLE;
            void*       vk_allocation = nullptr;
            if (LLVKLoader::createDepthAttachmentImageVk(mResX, mResY,
                                                         VK_FORMAT_D24_UNORM_S8_UINT,
                                                         vk_image, vk_view, vk_allocation))
            {
                mVkDepth      = vk_image;
                mVkDepthView  = vk_view;
                mVkDepthAlloc = vk_allocation;
                mVkDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            }
        }
    }
}


bool LLRenderTarget::allocate(U32 resx, U32 resy, U32 color_fmt, bool depth, LLTexUnit::eTextureType usage, LLTexUnit::eTextureMipGeneration generateMipMaps)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    llassert(usage == LLTexUnit::TT_TEXTURE);
    llassert(!isBoundInStack());

    if(mResX == resx && mResY == resy && mUsage == usage && depth == mUseDepth && mGenerateMipMaps == generateMipMaps)
    {
        return true;
    }
    resx = llmin(resx, (U32) gGLManager.mGLMaxTextureSize);
    resy = llmin(resy, (U32) gGLManager.mGLMaxTextureSize);

    release();

    mResX = resx;
    mResY = resy;

    mUsage = usage;
    mUseDepth = depth;

    mGenerateMipMaps = generateMipMaps;

    if (mGenerateMipMaps != LLTexUnit::TMG_NONE) {
        // Calculate the number of mip levels based upon resolution that we should have.
        mMipLevels = 1 + (U32)floor(log10((float)llmax(mResX, mResY)) / log10(2.0));
    }

    if (depth)
    {
        allocateDepth();
    }

    mAllocated = true;

    return addColorAttachment(color_fmt);
}

void LLRenderTarget::setColorAttachment(LLImageGL* img)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    llassert(img != nullptr); // img must not be null
    llassert(!mOwnDepth); // depth buffers not supported with this mode
    llassert(mInternalFormat.empty()); // attachments must be empty with this mode
    llassert(!isBoundInStack());

    mResX = img->getWidth();
    mResY = img->getHeight();
    mUsage = img->getTarget();

    mInternalFormat.push_back(img->getPrimaryFormat());

    if (LLVKLoader::isVulkanInitialized())
    {
        VkFormat vk_format = LLVKLoader::llGlEnumToVkFormat(img->getPrimaryFormat());
        bool vk_created_here = false;
        if (!img->hasVkImage())
        {
            VkImage     vk_image = VK_NULL_HANDLE;
            VkImageView vk_view  = VK_NULL_HANDLE;
            void*       vk_alloc = nullptr;
            U32 mip_levels = 1;
            if (img->getUseMipMaps())
            {
                U32 maxdim = llmax(mResX, mResY);
                while (maxdim > 1) { maxdim >>= 1; ++mip_levels; }
            }
            if (LLVKLoader::createColorAttachmentImageVk(mResX, mResY, vk_format,
                                                         vk_image, vk_view, vk_alloc, mip_levels))
            {
                img->setExternalVkBacking(vk_image, vk_view, vk_alloc, mResX, mResY, vk_format, mip_levels);
                vk_created_here = true;
            }
        }
        mVkTex.push_back(img->getVkImage());
        mVkTexView.push_back(img->getVkImageView());
        mVkTexSampleView.push_back(VK_NULL_HANDLE);
        mVkTexAlloc.push_back(nullptr);
        mVkTexLayout.push_back(vk_created_here
                               ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                               : VK_IMAGE_LAYOUT_UNDEFINED);
    }

    mAllocated = true;
}

void LLRenderTarget::releaseColorAttachment()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    llassert(!isBoundInStack());
    llassert(mInternalFormat.size() == 1); //cannot use releaseColorAttachment with LLRenderTarget managed color targets
    llassert(mAllocated);

    mInternalFormat.clear();

    if (LLVKLoader::isVulkanInitialized() && !mVkTex.empty())
    {
        if (mVkTex[0] != VK_NULL_HANDLE && !mVkTexLayout.empty())
        {
            LLVKLoader::transitionImageLayoutVk(
                mVkTex[0],
                VK_IMAGE_ASPECT_COLOR_BIT,
                mVkTexLayout[0],
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT);
        }
        mVkTex.clear();
        mVkTexView.clear();
        mVkTexSampleView.clear();
        mVkTexAlloc.clear();
        mVkTexLayout.clear();
    }
}

bool LLRenderTarget::addColorAttachment(U32 color_fmt)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    llassert(!isBoundInStack());

    if (color_fmt == 0)
    {
        return true;
    }

    U32 offset = static_cast<U32>(mInternalFormat.size());

    if( offset >= 4 )
    {
        LL_WARNS() << "Too many color attachments" << LL_ENDL;
        llassert( offset < 4 );
        return false;
    }
    if( offset > 0 && !mAllocated )
    {
        return false;
    }

    sBytesAllocated += mResX*mResY*4;

    mInternalFormat.push_back(color_fmt);

    if (LLVKLoader::isVulkanInitialized())
    {
        VkImage     vk_image       = VK_NULL_HANDLE;
        VkImageView vk_view        = VK_NULL_HANDLE;
        VkImageView vk_sample_view = VK_NULL_HANDLE;
        void*       vk_allocation  = nullptr;
        VkFormat    vk_format      = LLVKLoader::llGlEnumToVkFormat(color_fmt);
        const U32 vk_mip = (mGenerateMipMaps != LLTexUnit::TMG_NONE && mVkTex.empty() && mMipLevels > 1)
                           ? mMipLevels : 1;
        bool ok = LLVKLoader::createColorAttachmentImageVk(mResX, mResY, vk_format,
                                                           vk_image, vk_view, vk_allocation,
                                                           vk_mip,
                                                           (vk_mip > 1) ? &vk_sample_view : nullptr);
        if (!ok)
        {
            LL_WARNS("Vulkan") << "createColorAttachmentImageVk failed (addColorAttachment) = NULL"
                               << " placeholder で index 整合維持 attachment=" << (S32)mVkTex.size()
                               << " fmt=0x" << std::hex << color_fmt << std::dec
                               << " vk_fmt=" << (S32)vk_format
                               << " res=" << (S32)mResX << "x" << (S32)mResY << LL_ENDL;
        }
        mVkTex.push_back(vk_image);
        mVkTexView.push_back(vk_view);
        mVkTexSampleView.push_back(vk_sample_view);
        mVkTexAlloc.push_back(vk_allocation);
        mVkTexLayout.push_back(vk_image != VK_NULL_HANDLE
                               ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                               : VK_IMAGE_LAYOUT_UNDEFINED);
    }


    return true;
}

void LLRenderTarget::allocateDepth()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    mOwnDepth = true;

    sBytesAllocated += mResX*mResY*4;

    if (LLVKLoader::isVulkanInitialized())
    {
        VkImage     vk_image      = VK_NULL_HANDLE;
        VkImageView vk_view       = VK_NULL_HANDLE;
        void*       vk_allocation = nullptr;
        if (LLVKLoader::createDepthAttachmentImageVk(mResX, mResY,
                                                     VK_FORMAT_D24_UNORM_S8_UINT,
                                                     vk_image, vk_view, vk_allocation))
        {
            mVkDepth      = vk_image;
            mVkDepthView  = vk_view;
            mVkDepthAlloc = vk_allocation;
            mVkDepthLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }
    }
}

void LLRenderTarget::shareDepthBuffer(LLRenderTarget& target)
{
    llassert(!isBoundInStack());

    if (!mAllocated || !target.mAllocated)
    {
        LL_ERRS() << "Cannot share depth buffer between unallocated render targets." << LL_ENDL;
    }

    if (target.mOwnDepth)
    {
        LL_ERRS() << "Attempting to override existing depth buffer.  Detach existing buffer first." << LL_ENDL;
    }

    if (target.mUseDepth)
    {
        LL_ERRS() << "Attempting to override existing shared depth buffer. Detach existing buffer first." << LL_ENDL;
    }

    if (mOwnDepth)
    {
        target.mUseDepth = true;

        if (LLVKLoader::isVulkanInitialized() && mVkDepth != VK_NULL_HANDLE)
        {
            target.mVkDepth            = mVkDepth;
            target.mVkDepthView        = mVkDepthView;
            target.mVkDepthAlloc       = nullptr;
            target.mVkDepthLayoutOwner = this;
        }
    }
}

void LLRenderTarget::release()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    llassert(!isBoundInStack());

    if (mOwnDepth)
    {
        mOwnDepth = false;

        sBytesAllocated -= mResX*mResY*4;
    }
    if (mUseDepth)
    { //detach shared depth buffer
        mUseDepth = false;
    }

    sBytesAllocated -= mResX*mResY*4*static_cast<U32>(mInternalFormat.size());

    mInternalFormat.clear();

    if (LLVKLoader::isVulkanInitialized())
    {
        for (size_t i = 0; i < mVkTex.size(); ++i)
        {
            LLVKLoader::destroyImageVk(
                mVkTex[i],
                i < mVkTexView.size()  ? mVkTexView[i]  : VK_NULL_HANDLE,
                i < mVkTexAlloc.size() ? mVkTexAlloc[i] : nullptr);
            if (i < mVkTexSampleView.size() && mVkTexSampleView[i] != VK_NULL_HANDLE)
            {
                LLVKLoader::destroyImageVk(VK_NULL_HANDLE, mVkTexSampleView[i], nullptr);
            }
        }
        mVkTex.clear();
        mVkTexView.clear();
        mVkTexSampleView.clear();
        mVkTexAlloc.clear();
        mVkTexLayout.clear();

        if (mVkDepthAlloc != nullptr)
        {
            LLVKLoader::destroyImageVk(mVkDepth, mVkDepthView, mVkDepthAlloc);
        }
        mVkDepth            = VK_NULL_HANDLE;
        mVkDepthView        = VK_NULL_HANDLE;
        mVkDepthAlloc       = nullptr;
        mVkDepthLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
        mVkDepthLayoutOwner = nullptr;
    }

    mResX = mResY = 0;
    mAllocated = false;
}

void LLRenderTarget::bindTarget()
{
    LL_PROFILE_GPU_ZONE("bindTarget");
    llassert(mAllocated);
    llassert(!isBoundInStack());

    llSetGLViewport(0, 0, mResX, mResY);
    sCurResX = mResX;
    sCurResY = mResY;

    mPreviousRT = sBoundTarget;
    sBoundTarget = this;

    if (LLVKLoader::isVulkanInitialized())
    {
        U32 color_count = static_cast<U32>(mInternalFormat.size() < 4 ? mInternalFormat.size() : 4);

        for (U32 i = 0; i < color_count && i < mVkTex.size(); ++i)
        {
            if (mVkTex[i] == VK_NULL_HANDLE || i >= mVkTexLayout.size())
            {
                continue;
            }

            const VkImageLayout cur = mVkTexLayout[i];
            VkPipelineStageFlags src_stage;
            VkAccessFlags        src_access;
            if (cur == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                src_stage  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                src_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            }
            else if (cur == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                src_stage  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                src_access = VK_ACCESS_SHADER_READ_BIT;
            }
            else
            {
                src_stage  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                src_access = 0;
            }

            LLVKLoader::transitionImageLayoutVk(
                mVkTex[i],
                VK_IMAGE_ASPECT_COLOR_BIT,
                cur,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                src_stage,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                src_access,
                VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
            mVkTexLayout[i] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        if (mUseDepth && mVkDepth != VK_NULL_HANDLE)
        {
            const VkImageLayout cur = getCurDepthLayout();
            VkPipelineStageFlags src_stage;
            VkAccessFlags        src_access;
            if (cur == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            {
                src_stage  = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                src_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            }
            else if (cur == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                src_stage  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                src_access = VK_ACCESS_SHADER_READ_BIT;
            }
            else
            {
                src_stage  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                src_access = 0;
            }

            LLVKLoader::transitionImageLayoutVk(
                mVkDepth,
                VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                cur,
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                src_stage,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                src_access,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
            setCurDepthLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        }

        LLVKLoader::DynamicRenderingAttachment color_attachments[4] = {};
        for (U32 i = 0; i < color_count; ++i)
        {
            color_attachments[i].image_view   = (i < mVkTexView.size()) ? mVkTexView[i] : VK_NULL_HANDLE;
            color_attachments[i].image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            color_attachments[i].load_op      = VK_ATTACHMENT_LOAD_OP_LOAD;
            color_attachments[i].store_op     = VK_ATTACHMENT_STORE_OP_STORE;
        }

        LLVKLoader::DynamicRenderingAttachment depth_attachment = {};
        depth_attachment.image_view   = mVkDepthView;
        depth_attachment.image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_attachment.load_op      = VK_ATTACHMENT_LOAD_OP_LOAD;
        depth_attachment.store_op     = VK_ATTACHMENT_STORE_OP_STORE;

        LLVKLoader::beginDynamicRendering(
            mResX, mResY,
            color_count > 0 ? color_attachments : nullptr,
            color_count,
            mUseDepth ? &depth_attachment : nullptr);
    }
}

void LLRenderTarget::clear(U32 mask_in)
{
    LL_PROFILE_GPU_ZONE("clear");
    llassert(mAllocated);
    U32 mask = GL_COLOR_BUFFER_BIT;
    if (mUseDepth)
    {
        mask |= GL_DEPTH_BUFFER_BIT;

    }

    U32 effective_mask = mask & mask_in;

    if (LLVKLoader::isVulkanInitialized() && LLVKLoader::isInRenderPassScope())
    {
        VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
        if (cmd != VK_NULL_HANDLE)
        {
            VkClearAttachment ca[5] = {};
            U32 ca_count = 0;

            if ((effective_mask & GL_COLOR_BUFFER_BIT) && !mVkTex.empty())
            {
                const F32* gl_cc = gGL.getClearColor();

                const U32 color_count = static_cast<U32>(mVkTex.size() < 4 ? mVkTex.size() : 4);
                for (U32 i = 0; i < color_count; ++i)
                {
                    ca[ca_count].aspectMask                  = VK_IMAGE_ASPECT_COLOR_BIT;
                    ca[ca_count].colorAttachment             = i;
                    ca[ca_count].clearValue.color.float32[0] = gl_cc[0];
                    ca[ca_count].clearValue.color.float32[1] = gl_cc[1];
                    ca[ca_count].clearValue.color.float32[2] = gl_cc[2];
                    ca[ca_count].clearValue.color.float32[3] = gl_cc[3];
                    ++ca_count;
                }
            }

            if ((effective_mask & GL_DEPTH_BUFFER_BIT) && mUseDepth &&
                mVkDepth != VK_NULL_HANDLE)
            {
                ca[ca_count].aspectMask                    = VK_IMAGE_ASPECT_DEPTH_BIT;
                ca[ca_count].clearValue.depthStencil.depth   = 1.0f;
                ca[ca_count].clearValue.depthStencil.stencil = 0;
                ++ca_count;
            }

            if (ca_count > 0)
            {
                VkClearRect rect = {};
                rect.rect.offset    = { 0, 0 };
                rect.rect.extent    = { mResX, mResY };
                rect.baseArrayLayer = 0;
                rect.layerCount     = 1;

                vkCmdClearAttachments(cmd, ca_count, ca, 1, &rect);
            }
        }
    }
}

void LLRenderTarget::clearBoundTarget(U32 mask)
{
    if (sBoundTarget)
    {
        sBoundTarget->clear(mask);
    }
}

U32 LLRenderTarget::getNumTextures() const
{
    return static_cast<U32>(mInternalFormat.size());
}

void LLRenderTarget::bindTexture(U32 index, S32 channel, LLTexUnit::eTextureFilterOptions filter_options)
{
    llassert(index < mInternalFormat.size());
    gGL.getTexUnit(channel)->bindManual(mUsage, 0, filter_options == LLTexUnit::TFO_TRILINEAR || filter_options == LLTexUnit::TFO_ANISOTROPIC);
    gGL.getTexUnit(channel)->setTextureFilteringOption(filter_options);

    LLTexUnit* tu = gGL.getTexUnit(channel);
    if (tu != nullptr)
    {
        tu->mCurrRenderTarget = this;
        tu->mCurrRTAttachment = index;
        tu->mCurrRTDepth      = false;
        tu->mCurrImageGL      = nullptr;
    }

    bindForShaderRead(index, false);

    if (tu != nullptr)
    {
        tu->vkNotifyShaderChannelBound();
    }
}

bool LLRenderTarget::isVkActivePassAttachment(U32 attachment, bool depth) const
{
    VkImageView view = depth ? mVkDepthView
                             : (attachment < mVkTexView.size() ? mVkTexView[attachment]
                                                               : VK_NULL_HANDLE);
    return LLVKLoader::isImageViewActivePassAttachment(view);
}

void LLRenderTarget::bindForShaderRead(U32 attachment, bool depth)
{
    if (!LLVKLoader::isVulkanInitialized())
    {
        return;
    }

    if (attachment < mVkTex.size() && mVkTex[attachment] != VK_NULL_HANDLE &&
        attachment < mVkTexLayout.size() &&
        mVkTexLayout[attachment] != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
        !LLVKLoader::isImageViewActivePassAttachment(
            attachment < mVkTexView.size() ? mVkTexView[attachment] : VK_NULL_HANDLE))
    {
        LLVKLoader::transitionImageLayoutVk(
            mVkTex[attachment],
            VK_IMAGE_ASPECT_COLOR_BIT,
            mVkTexLayout[attachment],
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT);
        mVkTexLayout[attachment] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    if (depth && mVkDepth != VK_NULL_HANDLE &&
        getCurDepthLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
        !LLVKLoader::isImageViewActivePassAttachment(mVkDepthView))
    {
        LLVKLoader::transitionImageLayoutVk(
            mVkDepth,
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
            getCurDepthLayout(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT);
        setCurDepthLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

bool LLRenderTarget::copyContentsInFrameVk(LLRenderTarget& source)
{
    if (!LLVKLoader::isVulkanInitialized())
    {
        return false;
    }

    if (mVkTex.empty() || mVkTex[0] == VK_NULL_HANDLE ||
        source.mVkTex.empty() || source.mVkTex[0] == VK_NULL_HANDLE)
    {
        return false;
    }

    VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
    if (cmd == VK_NULL_HANDLE)
    {
        return false;
    }

    VkImageLayout src_layout = source.mVkTexLayout.empty()
                               ? VK_IMAGE_LAYOUT_UNDEFINED
                               : source.mVkTexLayout[0];
    VkImageLayout dst_layout = mVkTexLayout.empty()
                               ? VK_IMAGE_LAYOUT_UNDEFINED
                               : mVkTexLayout[0];

    LLVKLoader::transitionImageLayoutVk(
        source.mVkTex[0], VK_IMAGE_ASPECT_COLOR_BIT,
        src_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT);

    LLVKLoader::transitionImageLayoutVk(
        mVkTex[0], VK_IMAGE_ASPECT_COLOR_BIT,
        dst_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);

    VkImageBlit blit = {};
    blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel       = 0;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount     = 1;
    blit.srcOffsets[0]                 = { 0, 0, 0 };
    blit.srcOffsets[1]                 = { (S32)source.mResX, (S32)source.mResY, 1 };
    blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel       = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount     = 1;
    blit.dstOffsets[0]                 = { 0, 0, 0 };
    blit.dstOffsets[1]                 = { (S32)mResX, (S32)mResY, 1 };
    vkCmdBlitImage(cmd,
                   source.mVkTex[0], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   mVkTex[0], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blit, VK_FILTER_LINEAR);

    LLVKLoader::transitionImageLayoutVk(
        source.mVkTex[0], VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT);
    if (!source.mVkTexLayout.empty())
    {
        source.mVkTexLayout[0] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    LLVKLoader::transitionImageLayoutVk(
        mVkTex[0], VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
    if (!mVkTexLayout.empty())
    {
        mVkTexLayout[0] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    return true;
}

void LLRenderTarget::flush()
{
    LL_PROFILE_GPU_ZONE("rt flush");
    gGL.flush();
    llassert(mAllocated);
    llassert(sBoundTarget == this);

    if (LLVKLoader::isVulkanInitialized())
    {
        LLVKLoader::endDynamicRendering();
    }

    if (mGenerateMipMaps == LLTexUnit::TMG_AUTO)
    {
        LL_PROFILE_GPU_ZONE("rt generate mipmaps");
        if (LLVKLoader::isVulkanInitialized())
        {
            if (!mVkTex.empty() && mVkTex[0] != VK_NULL_HANDLE &&
                !mVkTexSampleView.empty() && mVkTexSampleView[0] != VK_NULL_HANDLE &&
                mMipLevels > 1 && !mInternalFormat.empty())
            {
                const VkImageLayout cur = (!mVkTexLayout.empty())
                                          ? mVkTexLayout[0]
                                          : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                if (LLVKLoader::generateMipChainInFrameVk(
                        mVkTex[0], (U32)mResX, (U32)mResY, mMipLevels,
                        LLVKLoader::llGlEnumToVkFormat(mInternalFormat[0]), cur))
                {
                    if (!mVkTexLayout.empty())
                    {
                        mVkTexLayout[0] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }
                }
            }
        }
    }

    if (mPreviousRT)
    {
        sBoundTarget = mPreviousRT->mPreviousRT;
        mPreviousRT->bindTarget();
    }
    else
    {
        sBoundTarget = nullptr;
        llSetGLViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);
        sCurResX = gGLViewport[2];
        sCurResY = gGLViewport[3];
    }
}

void LLRenderTarget::resumeVkDynamicRendering()
{
    if (!LLVKLoader::isVulkanInitialized())
    {
        return;
    }

    U32 color_count = static_cast<U32>(mInternalFormat.size() < 4 ? mInternalFormat.size() : 4);

    LLVKLoader::DynamicRenderingAttachment color_attachments[4] = {};
    for (U32 i = 0; i < color_count; ++i)
    {
        color_attachments[i].image_view   = (i < mVkTexView.size()) ? mVkTexView[i] : VK_NULL_HANDLE;
        color_attachments[i].image_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[i].load_op      = VK_ATTACHMENT_LOAD_OP_LOAD;
        color_attachments[i].store_op     = VK_ATTACHMENT_STORE_OP_STORE;
    }

    LLVKLoader::DynamicRenderingAttachment depth_attachment = {};
    depth_attachment.image_view   = mVkDepthView;
    depth_attachment.image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment.load_op      = VK_ATTACHMENT_LOAD_OP_LOAD;
    depth_attachment.store_op     = VK_ATTACHMENT_STORE_OP_STORE;

    LLVKLoader::beginDynamicRendering(
        mResX, mResY,
        color_count > 0 ? color_attachments : nullptr,
        color_count,
        mUseDepth ? &depth_attachment : nullptr);
}

bool LLRenderTarget::isComplete() const
{
    return !mInternalFormat.empty() || mOwnDepth;
}

void LLRenderTarget::getViewport(S32* viewport)
{
    viewport[0] = 0;
    viewport[1] = 0;
    viewport[2] = mResX;
    viewport[3] = mResY;
}

bool LLRenderTarget::isBoundInStack() const
{
    LLRenderTarget* cur = sBoundTarget;
    while (cur && cur != this)
    {
        cur = cur->mPreviousRT;
    }

    return cur == this;
}

void LLRenderTarget::swapFBORefs(LLRenderTarget& other)
{
    llassert(mAllocated);
    llassert(other.mAllocated);
    llassert(!isBoundInStack());
    llassert(!other.isBoundInStack());
    llassert(mResX == other.mResX);
    llassert(mResY == other.mResY);
    llassert(mInternalFormat == other.mInternalFormat);
    llassert(mOwnDepth == other.mOwnDepth);
    llassert(mUseDepth == other.mUseDepth);
    llassert(mGenerateMipMaps == other.mGenerateMipMaps);
    llassert(mMipLevels == other.mMipLevels);
    llassert(mUsage == other.mUsage);

    std::swap(mVkTex, other.mVkTex);
    std::swap(mVkTexView, other.mVkTexView);
    std::swap(mVkTexSampleView, other.mVkTexSampleView);
    std::swap(mVkTexAlloc, other.mVkTexAlloc);
    std::swap(mVkTexLayout, other.mVkTexLayout);
}
