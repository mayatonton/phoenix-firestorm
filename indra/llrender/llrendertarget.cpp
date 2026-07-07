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

void check_framebuffer_status()
{
    if (gDebugGL)
    {
        GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
        switch (status)
        {
        case GL_FRAMEBUFFER_COMPLETE:
            break;
        default:
            LL_WARNS() << "check_framebuffer_status failed -- " << std::hex << status << LL_ENDL;
            ll_fail("check_framebuffer_status failed");
            break;
        }
    }
}

bool LLRenderTarget::sUseFBO = false;
U32 LLRenderTarget::sCurFBO = 0;


extern S32 gGLViewport[4];

U32 LLRenderTarget::sCurResX = 0;
U32 LLRenderTarget::sCurResY = 0;

LLRenderTarget::LLRenderTarget() :
    mResX(0),
    mResY(0),
    mFBO(0),
    mDepth(0),
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
    //for accounting, get the number of pixels added/subtracted
    S32 pix_diff = (resx*resy)-(mResX*mResY);

    mResX = resx;
    mResY = resy;

    llassert(mInternalFormat.size() == mTex.size());

    for (U32 i = 0; i < mTex.size(); ++i)
    { //resize color attachments
        gGL.getTexUnit(0)->bindManual(mUsage, mTex[i]);
        LLImageGL::setManualImage(LLTexUnit::getInternalType(mUsage), 0, mInternalFormat[i], mResX, mResY, GL_RGBA, GL_UNSIGNED_BYTE, NULL, false);
        sBytesAllocated += pix_diff*4;
    }

    if (mDepth)
    {
        gGL.getTexUnit(0)->bindManual(mUsage, mDepth);
        U32 internal_type = LLTexUnit::getInternalType(mUsage);
        LLImageGL::setManualImage(internal_type, 0, GL_DEPTH_COMPONENT24, mResX, mResY, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL, false);

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
        if (mDepth)
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
        if (!allocateDepth())
        {
            LL_WARNS() << "Failed to allocate depth buffer for render target." << LL_ENDL;
            return false;
        }
    }

    glGenFramebuffers(1, (GLuint *) &mFBO);

    if (mDepth)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, LLTexUnit::getInternalType(mUsage), mDepth, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);
    }

    return addColorAttachment(color_fmt);
}

void LLRenderTarget::setColorAttachment(LLImageGL* img, LLGLuint use_name)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    llassert(img != nullptr); // img must not be null
    llassert(sUseFBO); // FBO support must be enabled
    llassert(mDepth == 0); // depth buffers not supported with this mode
    llassert(mTex.empty()); // mTex must be empty with this mode (binding target should be done via LLImageGL)
    llassert(!isBoundInStack());

    if (mFBO == 0)
    {
        glGenFramebuffers(1, (GLuint*)&mFBO);
    }

    mResX = img->getWidth();
    mResY = img->getHeight();
    mUsage = img->getTarget();

    if (use_name == 0)
    {
        use_name = img->getTexName();
    }

    mTex.push_back(use_name);
    // addColorAttachment と対称に internal format も記録する (mTex と整合維持)。
    mInternalFormat.push_back(img->getPrimaryFormat());

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            LLTexUnit::getInternalType(mUsage), use_name, 0);
        stop_glerror();

    check_framebuffer_status();

    glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);

    if (LLVKLoader::isVulkanInitialized())
    {
        VkFormat vk_format = LLVKLoader::llGlEnumToVkFormat(img->getPrimaryFormat());
        bool vk_created_here = false;
        if (!img->hasVkImage())
        {
            VkImage     vk_image = VK_NULL_HANDLE;
            VkImageView vk_view  = VK_NULL_HANDLE;
            void*       vk_alloc = nullptr;
            if (LLVKLoader::createColorAttachmentImageVk(mResX, mResY, vk_format,
                                                         vk_image, vk_view, vk_alloc))
            {
                img->setExternalVkBacking(vk_image, vk_view, vk_alloc, mResX, mResY, vk_format);
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
}

void LLRenderTarget::releaseColorAttachment()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    llassert(!isBoundInStack());
    llassert(mTex.size() == 1); //cannot use releaseColorAttachment with LLRenderTarget managed color targets
    llassert(mFBO != 0);  // mFBO must be valid

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, LLTexUnit::getInternalType(mUsage), 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);

    mTex.clear();
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

    U32 offset = static_cast<U32>(mTex.size());

    if( offset >= 4 )
    {
        LL_WARNS() << "Too many color attachments" << LL_ENDL;
        llassert( offset < 4 );
        return false;
    }
    if( offset > 0 && (mFBO == 0) )
    {
        llassert(  mFBO != 0 );
        return false;
    }

    U32 tex;
    LLImageGL::generateTextures(1, &tex);
    gGL.getTexUnit(0)->bindManual(mUsage, tex);

    stop_glerror();


    {
        clear_glerror();
        LLImageGL::setManualImage(LLTexUnit::getInternalType(mUsage), 0, color_fmt, mResX, mResY, GL_RGBA, GL_UNSIGNED_BYTE, NULL, false);
        if (glGetError() != GL_NO_ERROR)
        {
            LL_WARNS() << "Could not allocate color buffer for render target." << LL_ENDL;
            return false;
        }
    }

    sBytesAllocated += mResX*mResY*4;

    stop_glerror();


    if (offset == 0)
    { //use bilinear filtering on single texture render targets that aren't multisampled
        gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
        stop_glerror();
    }
    else
    { //don't filter data attachments
        gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);
        stop_glerror();
    }

    if (mUsage != LLTexUnit::TT_RECT_TEXTURE)
    {
        gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_MIRROR);
        stop_glerror();
    }
    else
    {
        // ATI doesn't support mirrored repeat for rectangular textures.
        gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
        stop_glerror();
    }

    if (mFBO)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+offset,
            LLTexUnit::getInternalType(mUsage), tex, 0);

        check_framebuffer_status();

        glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);
    }

    mTex.push_back(tex);
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

    if (gDebugGL)
    { //bind and unbind to validate target
        bindTarget();
        flush();
    }


    return true;
}

bool LLRenderTarget::allocateDepth()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_DISPLAY;
    LLImageGL::generateTextures(1, &mDepth);
    gGL.getTexUnit(0)->bindManual(mUsage, mDepth);

    U32 internal_type = LLTexUnit::getInternalType(mUsage);
    stop_glerror();
    clear_glerror();
    LLImageGL::setManualImage(internal_type, 0, GL_DEPTH_COMPONENT24, mResX, mResY, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL, false);
    gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_POINT);

    sBytesAllocated += mResX*mResY*4;

    if (glGetError() != GL_NO_ERROR)
    {
        LL_WARNS() << "Unable to allocate depth buffer for render target." << LL_ENDL;
        return false;
    }

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

    return true;
}

void LLRenderTarget::shareDepthBuffer(LLRenderTarget& target)
{
    llassert(!isBoundInStack());

    if (!mFBO || !target.mFBO)
    {
        LL_ERRS() << "Cannot share depth buffer between non FBO render targets." << LL_ENDL;
    }

    if (target.mDepth)
    {
        LL_ERRS() << "Attempting to override existing depth buffer.  Detach existing buffer first." << LL_ENDL;
    }

    if (target.mUseDepth)
    {
        LL_ERRS() << "Attempting to override existing shared depth buffer. Detach existing buffer first." << LL_ENDL;
    }

    if (mDepth)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, target.mFBO);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, LLTexUnit::getInternalType(mUsage), mDepth, 0);

        check_framebuffer_status();

        glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);

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

    if (mDepth)
    {
        LLImageGL::deleteTextures(1, &mDepth);

        mDepth = 0;

        sBytesAllocated -= mResX*mResY*4;
    }
    // else if (mFBO)
    if (mFBO)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

        if (mUseDepth)
        { //detach shared depth buffer
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, LLTexUnit::getInternalType(mUsage), 0, 0);
            mUseDepth = false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);
    }

    // Detach any extra color buffers (e.g. SRGB spec buffers)
    //
    if (mFBO && (mTex.size() > 1))
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
        size_t z;
        for (z = mTex.size() - 1; z >= 1; z--)
        {
            sBytesAllocated -= mResX*mResY*4;
            glFramebufferTexture2D(GL_FRAMEBUFFER, static_cast<GLenum>(GL_COLOR_ATTACHMENT0+z), LLTexUnit::getInternalType(mUsage), 0, 0);
            LLImageGL::deleteTextures(1, &mTex[z]);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO);
    }

    if (mFBO)
    {
        if (mFBO == sCurFBO)
        {
            sCurFBO = 0;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        glDeleteFramebuffers(1, (GLuint *) &mFBO);
        mFBO = 0;
    }

    if (mTex.size() > 0)
    {
        sBytesAllocated -= mResX*mResY*4;
        LLImageGL::deleteTextures(1, &mTex[0]);
    }

    mTex.clear();
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

        // depth 所有時のみ destroy (= GL release の `if (mDepth)` 所有判定と同じ)。
        //   共有 depth 受領側 (mVkDepthAlloc==nullptr) は destroy せず reference のみ drop。
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
}

void LLRenderTarget::bindTarget()
{
    LL_PROFILE_GPU_ZONE("bindTarget");
    llassert(mFBO);
    llassert(!isBoundInStack());

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    sCurFBO = mFBO;

    //setup multiple render targets
    GLenum drawbuffers[] = {GL_COLOR_ATTACHMENT0,
                            GL_COLOR_ATTACHMENT1,
                            GL_COLOR_ATTACHMENT2,
                            GL_COLOR_ATTACHMENT3};

    if (mTex.empty())
    { //no color buffer to draw to
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }
    else
    {
        glDrawBuffers(static_cast<GLsizei>(mTex.size()), drawbuffers);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
    }
    check_framebuffer_status();

    llSetGLViewport(0, 0, mResX, mResY);
    sCurResX = mResX;
    sCurResY = mResY;

    mPreviousRT = sBoundTarget;
    sBoundTarget = this;

    if (LLVKLoader::isVulkanInitialized())
    {
        U32 color_count = static_cast<U32>(mTex.size() < 4 ? mTex.size() : 4);

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

        if (mIsSwapchainTarget && color_count > 0 && LLVKLoader::isVulkanPresentationEnabled())
        {
            VkImageView swapchain_view = LLVKLoader::getCurrentSwapchainImageView();
            if (swapchain_view != VK_NULL_HANDLE)
            {
                color_attachments[0].image_view = swapchain_view;
            }
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
    llassert(mFBO);
    U32 mask = GL_COLOR_BUFFER_BIT;
    if (mUseDepth)
    {
        mask |= GL_DEPTH_BUFFER_BIT;

    }

    U32 effective_mask = mask & mask_in;

    if (mFBO)
    {
        check_framebuffer_status();
        stop_glerror();
        glClear(effective_mask);
        stop_glerror();
    }
    else
    {
        LLGLEnable scissor(GL_SCISSOR_TEST);
        glScissor(0, 0, mResX, mResY);
        stop_glerror();
        glClear(effective_mask);
    }

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

// raw glClear() は現 bound FBO を clear ゆえ、現 bound LLRenderTarget (= sBoundTarget) の clear() に委譲。
//   bound target 不在 (= default framebuffer) は GL のみ。
void LLRenderTarget::clearBoundTarget(U32 mask)
{
    if (sBoundTarget)
    {
        sBoundTarget->clear(mask);
    }
    else
    {
        stop_glerror();
        glClear(mask);
        stop_glerror();
    }
}

U32 LLRenderTarget::getTexture(U32 attachment) const
{
    if (attachment >= mTex.size())
    {
        LL_WARNS() << "Invalid attachment index " << attachment << " for size " << mTex.size() << LL_ENDL;
        llassert(false);
        return 0;
    }
    return mTex[attachment];
}

U32 LLRenderTarget::getNumTextures() const
{
    return static_cast<U32>(mTex.size());
}

void LLRenderTarget::bindTexture(U32 index, S32 channel, LLTexUnit::eTextureFilterOptions filter_options)
{
    gGL.getTexUnit(channel)->bindManual(mUsage, getTexture(index), filter_options == LLTexUnit::TFO_TRILINEAR || filter_options == LLTexUnit::TFO_ANISOTROPIC);
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

void LLRenderTarget::bindForShaderRead(U32 attachment, bool depth)
{
    if (!LLVKLoader::isVulkanInitialized())
    {
        return;
    }

    if (attachment < mVkTex.size() && mVkTex[attachment] != VK_NULL_HANDLE &&
        attachment < mVkTexLayout.size() &&
        mVkTexLayout[attachment] != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
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

    if (depth && mVkDepth != VK_NULL_HANDLE)
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

void LLRenderTarget::flush()
{
    LL_PROFILE_GPU_ZONE("rt flush");
    gGL.flush();
    llassert(mFBO);
    llassert(sCurFBO == mFBO);
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
        else
        {
            bindTexture(0, 0, LLTexUnit::TFO_TRILINEAR);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }

    if (mPreviousRT)
    {
        // a bit hacky -- pop the RT stack back two frames and push
        // the previous frame back on to play nice with the GL state machine
        sBoundTarget = mPreviousRT->mPreviousRT;
        mPreviousRT->bindTarget();
    }
    else
    {
        sBoundTarget = nullptr;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        sCurFBO = 0;
        llSetGLViewport(gGLViewport[0], gGLViewport[1], gGLViewport[2], gGLViewport[3]);
        sCurResX = gGLViewport[2];
        sCurResY = gGLViewport[3];
        glReadBuffer(GL_BACK);
        glDrawBuffer(GL_BACK);
    }
}

void LLRenderTarget::resumeVkDynamicRendering()
{
    if (!LLVKLoader::isVulkanInitialized())
    {
        return;
    }

    U32 color_count = static_cast<U32>(mTex.size() < 4 ? mTex.size() : 4);

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
    return !mTex.empty() || mDepth;
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
    // Must be initialized
    llassert(mFBO);
    llassert(other.mFBO);

    // Must be unbound
    // *NOTE: mPreviousRT can be non-null even if this target is unbound - presumably for debugging purposes?
    llassert(sCurFBO != mFBO);
    llassert(sCurFBO != other.mFBO);
    llassert(!isBoundInStack());
    llassert(!other.isBoundInStack());

    // Must be same type
    llassert(sUseFBO == other.sUseFBO);
    llassert(mResX == other.mResX);
    llassert(mResY == other.mResY);
    llassert(mInternalFormat == other.mInternalFormat);
    llassert(mTex.size() == other.mTex.size());
    llassert(mDepth == other.mDepth);
    llassert(mUseDepth == other.mUseDepth);
    llassert(mGenerateMipMaps == other.mGenerateMipMaps);
    llassert(mMipLevels == other.mMipLevels);
    llassert(mUsage == other.mUsage);

    std::swap(mFBO, other.mFBO);
    std::swap(mTex, other.mTex);

    std::swap(mVkTex, other.mVkTex);
    std::swap(mVkTexView, other.mVkTexView);
    std::swap(mVkTexSampleView, other.mVkTexSampleView);
    std::swap(mVkTexAlloc, other.mVkTexAlloc);
    std::swap(mVkTexLayout, other.mVkTexLayout);
}
