/**
 * @file llcubemap.cpp
 * @brief LLCubeMap class implementation
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
#include "linden_common.h"

#include "llworkerthread.h"

#include "llcubemap.h"

#include "v4coloru.h"
#include "v3math.h"
#include "v3dmath.h"
#include "m3math.h"
#include "m4math.h"

#include "llrender.h"
#include "llglslshader.h"
#include "llvkloader.h"

#include "llglheaders.h"

namespace {
    const U16 RESOLUTION = 64;
}

bool LLCubeMap::sUseCubeMaps = true;

LLCubeMap::LLCubeMap(bool init_as_srgb)
    : mTextureStage(0),
      mMatrixStage(0),
      mIssRGB(init_as_srgb)
{
    mTargets[0] = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
    mTargets[1] = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    mTargets[2] = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
    mTargets[3] = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
    mTargets[4] = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
    mTargets[5] = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
}

LLCubeMap::~LLCubeMap()
{
    // bind 中 texunit の mCurrCubeMap raw pointer を無効化 (= 本 cube destroy 後の dangling 防止)。
    //   sky environmentMap は LLVOSky::mCubeMap (= LLPointer) で保持され、 ~LLVOSky
    //   (= region 変更/teleport で sky object 再構築、 llvosky.cpp:460) で free される =
    //   world transition 中に texunit に残った mCurrCubeMap が dangling 化する class。
    LLRender::clearStaleCubeMapRefs(this);

    if (mVkCubeImageView != VK_NULL_HANDLE || mVkCubeImage != VK_NULL_HANDLE)
    {
        LLVKLoader::destroyImageVk(mVkCubeImage, mVkCubeImageView, mVkCubeAllocation);
        mVkCubeImage      = VK_NULL_HANDLE;
        mVkCubeImageView  = VK_NULL_HANDLE;
        mVkCubeAllocation = nullptr;
    }
}

void LLCubeMap::initGL()
{
    llassert(gGLManager.mInited);

    if (LLCubeMap::sUseCubeMaps)
    {
        // Not initialized, do stuff.
        if (mImages[0].isNull())
        {
            U32 texname = 0;

            LLImageGL::generateTextures(1, &texname);

            for (int i = 0; i < 6; i++)
            {
                mImages[i] = new LLImageGL(RESOLUTION, RESOLUTION, 4, false);
            #if USE_SRGB_DECODE
                if (mIssRGB) {
                    mImages[i]->setExplicitFormat(GL_SRGB8_ALPHA8, GL_RGBA);
                }
            #endif
                mImages[i]->setTarget(mTargets[i], LLTexUnit::TT_CUBE_MAP);
                mRawImages[i] = new LLImageRaw(RESOLUTION, RESOLUTION, 4);
                if (!mImages[i]->createGLTexture(0, mRawImages[i], texname))
                {
                    LL_WARNS() << "Failed to create GL texture for environment cubemap face " << i << LL_ENDL;
                }

                gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_CUBE_MAP, texname);
                mImages[i]->setAddressMode(LLTexUnit::TAM_CLAMP);
                stop_glerror();
            }
            gGL.getTexUnit(0)->disable();
        }
        disable();
    }
    else
    {
        LL_WARNS() << "Using cube map without extension!" << LL_ENDL;
    }
}

void LLCubeMap::initRawData(const std::vector<LLPointer<LLImageRaw> >& rawimages)
{
    bool flip_x[6] =    { false, true,  false, false, true,  false };
    bool flip_y[6] =    { true,  true,  true,  false, true,  true  };
    bool transpose[6] = { false, false, false, false, true,  true  };

    // Yes, I know that this is inefficient! - djs 08/08/02
    for (int i = 0; i < 6; i++)
    {
        LLImageDataSharedLock lockIn(rawimages[i]);
        LLImageDataLock lockOut(mRawImages[i]);

        const U8 *sd = rawimages[i]->getData();
        U8 *td = mRawImages[i]->getData();

        S32 offset = 0;
        S32 sx, sy, so;
        for (int y = 0; y < 64; y++)
        {
            for (int x = 0; x < 64; x++)
            {
                sx = x;
                sy = y;
                if (flip_y[i])
                {
                    sy = 63 - y;
                }
                if (flip_x[i])
                {
                    sx = 63 - x;
                }
                if (transpose[i])
                {
                    S32 temp = sx;
                    sx = sy;
                    sy = temp;
                }

                so = 64*sy + sx;
                so *= 4;
                *(td + offset++) = *(sd + so++);
                *(td + offset++) = *(sd + so++);
                *(td + offset++) = *(sd + so++);
                *(td + offset++) = *(sd + so++);
            }
        }
    }
}

void LLCubeMap::initGLData()
{
    LL_PROFILE_ZONE_SCOPED;
    for (int i = 0; i < 6; i++)
    {
        mImages[i]->setSubImage(mRawImages[i], 0, 0, RESOLUTION, RESOLUTION);
    }

    if (LLVKLoader::isVulkanInitialized())
    {
        // 同 resolution + 同 format で既配備の場合 destroy 不要 (= re-upload 経路)
        const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        if (mVkCubeImage == VK_NULL_HANDLE ||
            mVkCubeResolution != RESOLUTION ||
            mVkCubeFormat != format)
        {
            if (mVkCubeImageView != VK_NULL_HANDLE)
            {
                LLVKLoader::destroyImageVk(mVkCubeImage, mVkCubeImageView, mVkCubeAllocation);
                mVkCubeImage = VK_NULL_HANDLE;
                mVkCubeImageView = VK_NULL_HANDLE;
                mVkCubeAllocation = nullptr;
            }
            if (LLVKLoader::createCubeImageVk(RESOLUTION, format, 1,
                                              mVkCubeImage, mVkCubeImageView,
                                              mVkCubeAllocation))
            {
                mVkCubeResolution = RESOLUTION;
                mVkCubeFormat     = format;
            }
        }
        if (mVkCubeImage != VK_NULL_HANDLE)
        {
            static const U32 gl_to_vk[6] = { 1, 0, 3, 2, 5, 4 };
            const void* face_data[6] = { nullptr };
            for (int i = 0; i < 6; ++i)
            {
                face_data[gl_to_vk[i]] = mRawImages[i]->getData();
            }
            const U32 face_size_bytes = RESOLUTION * RESOLUTION * 4;
            LLVKLoader::uploadCubeImageDataVk(mVkCubeImage, RESOLUTION, format,
                                              face_data, face_size_bytes);
        }
    }
}

void LLCubeMap::init(const std::vector<LLPointer<LLImageRaw> >& rawimages)
{
    if (!gGLManager.mIsDisabled)
    {
        initGL();
        initRawData(rawimages);
        initGLData();
    }
}

void LLCubeMap::initReflectionMap(U32 resolution, U32 components)
{
    U32 texname = 0;

    LLImageGL::generateTextures(1, &texname);

    mImages[0] = new LLImageGL(resolution, resolution, components, true);
    mImages[0]->setTexName(texname);
    mImages[0]->setTarget(mTargets[0], LLTexUnit::TT_CUBE_MAP);
    gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_CUBE_MAP, texname);
    mImages[0]->setAddressMode(LLTexUnit::TAM_CLAMP);
}

void LLCubeMap::initEnvironmentMap(const std::vector<LLPointer<LLImageRaw> >& rawimages)
{
    llassert(rawimages.size() == 6);

    U32 texname = 0;

    LLImageGL::generateTextures(1, &texname);

    U32 resolution = rawimages[0]->getWidth();
    U32 components = rawimages[0]->getComponents();

    for (int i = 0; i < 6; i++)
    {
        llassert(rawimages[i]->getWidth() == resolution);
        llassert(rawimages[i]->getHeight() == resolution);
        llassert(rawimages[i]->getComponents() == components);

        mImages[i] = new LLImageGL(resolution, resolution, components, true);
        mImages[i]->setTarget(mTargets[i], LLTexUnit::TT_CUBE_MAP);
        mRawImages[i] = rawimages[i];
        if (!mImages[i]->createGLTexture(0, mRawImages[i], texname))
        {
            LL_WARNS() << "Failed to create GL texture for environment cubemap face " << i << LL_ENDL;
        }

        gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_CUBE_MAP, texname);
        mImages[i]->setAddressMode(LLTexUnit::TAM_CLAMP);
        stop_glerror();

        mImages[i]->setSubImage(mRawImages[i], 0, 0, resolution, resolution);
    }
    enableTexture(0);
    bind();
    mImages[0]->setFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    gGL.getTexUnit(0)->disable();
    disable();

    if (LLVKLoader::isVulkanInitialized() && components == 4)
    {
        const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        if (mVkCubeImage == VK_NULL_HANDLE ||
            mVkCubeResolution != resolution ||
            mVkCubeFormat != format)
        {
            if (mVkCubeImageView != VK_NULL_HANDLE)
            {
                LLVKLoader::destroyImageVk(mVkCubeImage, mVkCubeImageView, mVkCubeAllocation);
                mVkCubeImage = VK_NULL_HANDLE;
                mVkCubeImageView = VK_NULL_HANDLE;
                mVkCubeAllocation = nullptr;
            }
            U32 vk_mip_count = 1;
            {
                U32 dim = resolution;
                while (dim > 1) { dim >>= 1; ++vk_mip_count; }
            }
            if (LLVKLoader::createCubeImageVk(resolution, format, vk_mip_count,
                                              mVkCubeImage, mVkCubeImageView,
                                              mVkCubeAllocation))
            {
                mVkCubeResolution = resolution;
                mVkCubeFormat     = format;
            }
        }
        if (mVkCubeImage != VK_NULL_HANDLE)
        {
            static const U32 gl_to_vk[6] = { 1, 0, 3, 2, 5, 4 };
            const void* face_data[6] = { nullptr };
            for (int i = 0; i < 6; ++i)
            {
                face_data[gl_to_vk[i]] = mRawImages[i]->getData();
            }
            const U32 face_size_bytes = resolution * resolution * 4;
            LLVKLoader::uploadCubeImageDataVk(mVkCubeImage, resolution, format,
                                              face_data, face_size_bytes);
            U32 vk_mip_count = 1;
            {
                U32 dim = resolution;
                while (dim > 1) { dim >>= 1; ++vk_mip_count; }
            }
            LLVKLoader::generateCubeMipChainBlitVk(mVkCubeImage, resolution, vk_mip_count, format);
        }
    }
}

void LLCubeMap::generateMipMaps()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;

    mImages[0]->setUseMipMaps(true);
    mImages[0]->setHasMipMaps(true);
    enableTexture(0);
    bind();
    mImages[0]->setFilteringOption(LLTexUnit::TFO_BILINEAR);
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_TEXTURE("cmgmm - glGenerateMipmap");
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }
    gGL.getTexUnit(0)->disable();
    disable();
}

GLuint LLCubeMap::getGLName()
{
    return mImages[0]->getTexName();
}

void LLCubeMap::bind()
{
    gGL.getTexUnit(mTextureStage)->bind(this);
}

void LLCubeMap::enable(S32 stage)
{
    enableTexture(stage);
}

void LLCubeMap::enableTexture(S32 stage)
{
    mTextureStage = stage;
    if (stage >= 0 && LLCubeMap::sUseCubeMaps)
    {
        gGL.getTexUnit(stage)->enable(LLTexUnit::TT_CUBE_MAP);
    }
}

void LLCubeMap::disable(void)
{
    disableTexture();
}

void LLCubeMap::disableTexture(void)
{
    if (mTextureStage >= 0 && LLCubeMap::sUseCubeMaps)
    {
        gGL.getTexUnit(mTextureStage)->disable();
        if (mTextureStage == 0)
        {
            gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
        }
    }
}

void LLCubeMap::setMatrix(S32 stage)
{
    mMatrixStage = stage;

    if (mMatrixStage < 0) return;

    //if (stage > 0)
    {
        gGL.getTexUnit(stage)->activate();
    }

    LLVector3 x(gGLModelView+0);
    LLVector3 y(gGLModelView+4);
    LLVector3 z(gGLModelView+8);

    LLMatrix3 mat3;
    mat3.setRows(x,y,z);
    LLMatrix4 trans(mat3);
    trans.transpose();

    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.pushMatrix();
    gGL.loadMatrix((F32 *)trans.mMatrix);
    gGL.matrixMode(LLRender::MM_MODELVIEW);

    /*if (stage > 0)
    {
        gGL.getTexUnit(0)->activate();
    }*/
}

void LLCubeMap::restoreMatrix()
{
    if (mMatrixStage < 0) return;

    //if (mMatrixStage > 0)
    {
        gGL.getTexUnit(mMatrixStage)->activate();
    }
    gGL.matrixMode(LLRender::MM_TEXTURE);
    gGL.popMatrix();
    gGL.matrixMode(LLRender::MM_MODELVIEW);

    /*if (mMatrixStage > 0)
    {
        gGL.getTexUnit(0)->activate();
    }*/
}


void LLCubeMap::destroyGL()
{
    for (S32 i = 0; i < 6; i++)
    {
        mImages[i] = NULL;
    }

    if (mVkCubeImageView != VK_NULL_HANDLE || mVkCubeImage != VK_NULL_HANDLE)
    {
        LLVKLoader::destroyImageVk(mVkCubeImage, mVkCubeImageView, mVkCubeAllocation);
        mVkCubeImage      = VK_NULL_HANDLE;
        mVkCubeImageView  = VK_NULL_HANDLE;
        mVkCubeAllocation = nullptr;
        mVkCubeResolution = 0;
        mVkCubeFormat     = VK_FORMAT_UNDEFINED;
    }
}
