/**
 * @file llgl.cpp
 * @brief LLGL implementation
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

#include "llsys.h"

#include "llgl.h"
#include "llglstates.h"
#include "llrender.h"

#include "llerror.h"
#include "llerrorcontrol.h"
#include "llquaternion.h"
#include "llmath.h"
#include "m4math.h"
#include "llstring.h"
#include "llstacktrace.h"

#include "llglheaders.h"
#include "llglslshader.h"
#include "llvkloader.h"

#include "glm/glm.hpp"
#include <glm/gtc/matrix_access.hpp>
#include "glm/gtc/type_ptr.hpp"

#if LL_WINDOWS
#include "lldxhardware.h"
#endif

bool gDebugSession = false;
bool gDebugGLSession = false;
bool gHeadlessClient = false;
bool gNonInteractive = false;

static const std::string HEADLESS_VENDOR_STRING("Linden Lab");
static const std::string HEADLESS_RENDERER_STRING("Headless");
static const std::string HEADLESS_VERSION_STRING("1.0");

llofstream gFailLog;

void ll_init_fail_log(std::string filename)
{
    gFailLog.open(filename.c_str());
}


void ll_fail(std::string msg)
{

    if (gDebugSession)
    {
        std::vector<std::string> lines;

        gFailLog << LLError::utcTime() << " " << msg << std::endl;

        gFailLog << "Stack Trace:" << std::endl;

        ll_get_stack_trace(lines);

        for(size_t i = 0; i < lines.size(); ++i)
        {
            gFailLog << lines[i] << std::endl;
        }

        gFailLog << "End of Stack Trace." << std::endl << std::endl;

        gFailLog.flush();
    }
};

void ll_close_fail_log()
{
    gFailLog.close();
}

std::list<LLGLUpdate*> LLGLUpdate::sGLQ;

LLGLManager gGLManager;

LLGLManager::LLGLManager() :
    mInited(false),
    mIsDisabled(false),
    mMaxSamples(0),
    mNumTextureImageUnits(1),
    mMaxSampleMaskWords(0),
    mMaxColorTextureSamples(0),
    mMaxDepthTextureSamples(0),
    mMaxIntegerSamples(0),
    mIsAMD(false),
    mIsNVIDIA(false),
    mIsIntel(false),
#if LL_DARWIN
    mIsMobileGF(false),
#endif
    mHasRequirements(true),
    mDriverVersionMajor(1),
    mDriverVersionMinor(0),
    mDriverVersionRelease(0),
    mGLVersion(1.0f),
    mGLSLVersionMajor(0),
    mGLSLVersionMinor(0),
    mVRAM(0),
    mVRAMDetected(0), // <FS:Beq/> add override support
    mGLMaxVertexRange(0),
    mGLMaxIndexRange(0)
{
}

static std::string sRawGLString;

static S32 highest_sample_count(U32 sample_count_mask)
{
    for (U32 bit = 64; bit >= 1; bit >>= 1)
    {
        if (sample_count_mask & bit)
        {
            return (S32)bit;
        }
    }
    return 0;
}

bool LLGLManager::initGL()
{
    LL_INFOS("RenderInit") << "Initializing graphics capabilities from Vulkan device" << LL_ENDL;
    if (mInited)
    {
        LL_ERRS("RenderInit") << "Calling init on LLGLManager after already initialized!" << LL_ENDL;
    }

    LLVKLoader::DeviceCapsVk caps;
    if (!LLVKLoader::getDeviceCapsVk(caps))
    {
        LL_WARNS("RenderInit") << "Vulkan device unavailable; graphics capability values remain defaults" << LL_ENDL;
        mInited = true;
        initGLStates();
        return true;
    }

    std::string vendor_raw;
    switch (caps.vendor_id)
    {
        case 0x10DE:
            vendor_raw = "NVIDIA Corporation";
            mGLVendorShort = "NVIDIA";
            mIsNVIDIA = true;
            break;
        case 0x1002:
        case 0x1022:
            vendor_raw = "AMD";
            mGLVendorShort = "AMD";
            mIsAMD = true;
            break;
        case 0x8086:
            vendor_raw = "Intel";
            mGLVendorShort = "INTEL";
            mIsIntel = true;
            break;
        case 0x106B:
            vendor_raw = "Apple";
            mGLVendorShort = "APPLE";
            mIsApple = true;
            break;
        default:
            vendor_raw = caps.driver_name;
            mGLVendorShort = "MISC";
            break;
    }

    mGLVendor = vendor_raw;
    LLStringUtil::toUpper(mGLVendor);

    mGLRenderer = caps.device_name;
    LLStringUtil::toUpper(mGLRenderer);

    sRawGLString = vendor_raw + " " + caps.device_name;

    mGLVersion = 4.6f;
    mDriverVersionMajor = 4;
    mDriverVersionMinor = 6;
    mDriverVersionRelease = 0;
    mDriverVersionVendorString = caps.driver_info;
    mGLVersionString = llformat("4.6 (Vulkan %u.%u %s %s)",
                                caps.api_version_major,
                                caps.api_version_minor,
                                caps.driver_name.c_str(),
                                caps.driver_info.c_str());

    mGLSLVersionMajor = 4;
    mGLSLVersionMinor = 60;

    mHasCubeMapArray = caps.image_cube_array_enabled;
    mHasAnisotropic = caps.sampler_anisotropy_enabled;
    if (mHasAnisotropic)
    {
        mMaxAnisotropy = caps.max_sampler_anisotropy;
    }

    if (caps.device_local_memory_mb > 0)
    {
        mVRAM = caps.device_local_memory_mb;
        LL_INFOS("RenderInit") << "VRAM Detected (Vulkan device-local heap): " << mVRAM << " MB" << LL_ENDL;
    }

    mNumTextureImageUnits = (S32)llmin(caps.max_per_stage_sampled_images, LL_NUM_TEXTURE_LAYERS);
    mMaxColorTextureSamples = highest_sample_count(caps.framebuffer_color_sample_counts);
    mMaxDepthTextureSamples = highest_sample_count(caps.framebuffer_depth_sample_counts);
    mMaxIntegerSamples = highest_sample_count(caps.sampled_image_integer_sample_counts);
    mMaxSamples = llmin(mMaxColorTextureSamples, mMaxDepthTextureSamples);
    mMaxSampleMaskWords = (S32)caps.max_sample_mask_words;
    mMaxVaryingVectors = (S32)(caps.max_vertex_output_components / 4);
    mMaxUniformBlockSize = (S32)llmin(caps.max_uniform_buffer_range, (U32)65536);

    mGLMaxVertexRange = 1048576;
    mGLMaxIndexRange = 1048576;
    mGLMaxTextureSize = (S32)caps.max_image_dimension_2d;

    mInited = true;

    initGLStates();

    return true;
}

void LLGLManager::getGLInfo(LLSD& info)
{
    if (gHeadlessClient)
    {
        info["GLInfo"]["GLVendor"] = HEADLESS_VENDOR_STRING;
        info["GLInfo"]["GLRenderer"] = HEADLESS_RENDERER_STRING;
        info["GLInfo"]["GLVersion"] = HEADLESS_VERSION_STRING;
        return;
    }
    else
    {
        info["GLInfo"]["GLVendor"] = mGLVendor;
        info["GLInfo"]["GLRenderer"] = mGLRenderer;
        info["GLInfo"]["GLVersion"] = mGLVersionString;
    }
}

std::string LLGLManager::getGLInfoString()
{
    std::string info_str;

    if (gHeadlessClient)
    {
        info_str += std::string("GL_VENDOR      ") + HEADLESS_VENDOR_STRING + std::string("\n");
        info_str += std::string("GL_RENDERER    ") + HEADLESS_RENDERER_STRING + std::string("\n");
        info_str += std::string("GL_VERSION     ") + HEADLESS_VERSION_STRING + std::string("\n");
    }
    else
    {
        info_str += std::string("GL_VENDOR      ") + mGLVendor + std::string("\n");
        info_str += std::string("GL_RENDERER    ") + mGLRenderer + std::string("\n");
        info_str += std::string("GL_VERSION     ") + mGLVersionString + std::string("\n");
    }

    return info_str;
}

void LLGLManager::printGLInfoString()
{
    if (gHeadlessClient)
    {
        LL_INFOS("RenderInit") << "GL_VENDOR:     " << HEADLESS_VENDOR_STRING << LL_ENDL;
        LL_INFOS("RenderInit") << "GL_RENDERER:   " << HEADLESS_RENDERER_STRING << LL_ENDL;
        LL_INFOS("RenderInit") << "GL_VERSION:    " << HEADLESS_VERSION_STRING << LL_ENDL;
    }
    else
    {
        LL_INFOS("RenderInit") << "GL_VENDOR:     " << mGLVendor << LL_ENDL;
        LL_INFOS("RenderInit") << "GL_RENDERER:   " << mGLRenderer << LL_ENDL;
        LL_INFOS("RenderInit") << "GL_VERSION:    " << mGLVersionString << LL_ENDL;
    }
}

std::string LLGLManager::getRawGLString()
{
    std::string gl_string;
    if (gHeadlessClient)
    {
        gl_string = HEADLESS_VENDOR_STRING + " " + HEADLESS_RENDERER_STRING;
    }
    else
    {
        gl_string = sRawGLString;
    }
    return gl_string;
}

void LLGLManager::asLLSD(LLSD& info)
{
    info["gpu_vendor"] = mGLVendorShort;
    info["gpu_version"] = mDriverVersionVendorString;
    info["opengl_version"] = mGLVersionString;

    info["vram"] = LLSD::Integer(mVRAM);

    info["max_samples"] = mMaxSamples;
    info["num_texture_image_units"] =  mNumTextureImageUnits;
    info["max_sample_mask_words"] = mMaxSampleMaskWords;
    info["max_color_texture_samples"] = mMaxColorTextureSamples;
    info["max_depth_texture_samples"] = mMaxDepthTextureSamples;
    info["max_integer_samples"] = mMaxIntegerSamples;
    info["max_vertex_range"] = mGLMaxVertexRange;
    info["max_index_range"] = mGLMaxIndexRange;
    info["max_texture_size"] = mGLMaxTextureSize;

    info["is_ati"] = mIsAMD;
    info["is_nvidia"] = mIsNVIDIA;
    info["is_intel"] = mIsIntel;

    info["gl_renderer"] = mGLRenderer;
}

void LLGLManager::shutdownGL()
{
    if (mInited)
    {
        mInited = false;
    }
}

void rotate_quat(LLQuaternion& rotation)
{
    F32 angle_radians, x, y, z;
    rotation.getAngleAxis(&angle_radians, &x, &y, &z);
    gGL.rotatef(angle_radians * RAD_TO_DEG, x, y, z);
}

boost::unordered_map<LLGLenum, LLGLboolean> LLGLState::sStateMap;

GLenum LLGLState::sCullFaceMode = GL_BACK;

void LLGLState::setCullFaceMode(GLenum mode)
{
    sCullFaceMode = mode;
}

bool LLGLState::isCullFaceEnabled()
{
    auto it = sStateMap.find(GL_CULL_FACE);
    return (it != sStateMap.end()) && (it->second != GL_FALSE);
}

bool LLGLState::isDepthClampEnabled()
{
    auto it = sStateMap.find(GL_DEPTH_CLAMP);
    return (it != sStateMap.end()) && (it->second != GL_FALSE);
}

bool LLGLState::isPolygonOffsetEnabled()
{
    auto fill_it = sStateMap.find(GL_POLYGON_OFFSET_FILL);
    auto line_it = sStateMap.find(GL_POLYGON_OFFSET_LINE);
    const bool fill_on = (fill_it != sStateMap.end()) && (fill_it->second != GL_FALSE);
    const bool line_on = (line_it != sStateMap.end()) && (line_it->second != GL_FALSE);
    return fill_on || line_on;
}

bool LLGLState::isBlendEnabled()
{
    auto it = sStateMap.find(GL_BLEND);
    return (it != sStateMap.end()) && (it->second != GL_FALSE);
}

GLenum LLGLState::sPolygonMode = GL_FILL;

void LLGLState::setPolygonMode(GLenum mode)
{
    sPolygonMode = mode;
}

GLenum LLGLState::sStencilFunc        = GL_ALWAYS;
GLint  LLGLState::sStencilRef         = 0;
GLuint LLGLState::sStencilCompareMask = 0xFFFFFFFFu;
GLenum LLGLState::sStencilFailOp      = GL_KEEP;
GLenum LLGLState::sStencilDepthFailOp = GL_KEEP;
GLenum LLGLState::sStencilDepthPassOp = GL_KEEP;
GLuint LLGLState::sStencilWriteMask   = 0xFFFFFFFFu;

void LLGLState::setStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    sStencilFunc        = func;
    sStencilRef         = ref;
    sStencilCompareMask = mask;
}

void LLGLState::setStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass)
{
    sStencilFailOp      = sfail;
    sStencilDepthFailOp = dpfail;
    sStencilDepthPassOp = dppass;
}

void LLGLState::setStencilMask(GLuint mask)
{
    sStencilWriteMask = mask;
}

bool LLGLState::isStencilTestEnabled()
{
    auto it = sStateMap.find(GL_STENCIL_TEST);
    return (it != sStateMap.end()) && (it->second != GL_FALSE);
}

GLboolean LLGLDepthTest::sDepthEnabled = GL_FALSE;
GLenum LLGLDepthTest::sDepthFunc = GL_LESS;
GLboolean LLGLDepthTest::sWriteEnabled = GL_TRUE;

void LLGLState::initClass()
{
    sStateMap[GL_DITHER] = GL_TRUE;
    sStateMap[GL_MULTISAMPLE] = GL_FALSE;
}

void LLGLState::restoreGL()
{
    sStateMap.clear();
    initClass();
}

LLGLState::LLGLState(LLGLenum state, S32 enabled) :
    mState(state), mWasEnabled(false), mIsEnabled(false)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    if (mState)
    {
        mWasEnabled = sStateMap[state];
        setEnabled(enabled);
    }
}

void LLGLState::setEnabled(S32 enabled)
{
    if (!mState)
    {
        return;
    }
    if (enabled == CURRENT_STATE)
    {
        enabled = sStateMap[mState] == GL_TRUE ? ENABLED_STATE : DISABLED_STATE;
    }
    else if (enabled == ENABLED_STATE && sStateMap[mState] != GL_TRUE)
    {
        gGL.flush();
        sStateMap[mState] = GL_TRUE;
    }
    else if (enabled == DISABLED_STATE && sStateMap[mState] != GL_FALSE)
    {
        gGL.flush();
        sStateMap[mState] = GL_FALSE;
    }
    mIsEnabled = enabled;
}

LLGLState::~LLGLState()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
    if (mState)
    {
        if (mIsEnabled != mWasEnabled)
        {
            gGL.flush();
            sStateMap[mState] = mWasEnabled ? GL_TRUE : GL_FALSE;
        }
    }
}

void LLGLManager::initGLStates()
{
    LLGLState::initClass();
}

LLGLUserClipPlane::LLGLUserClipPlane(const LLPlane& p, const glm::mat4& modelview, const glm::mat4& projection, bool apply)
{
    mApply = apply;

    if (mApply)
    {
        mModelview = modelview;
        mProjection = projection;

        setPlane(-p[0], -p[1], -p[2], -p[3]);
    }
}

void LLGLUserClipPlane::disable()
{
    if (mApply)
    {
        gGL.matrixMode(LLRender::MM_PROJECTION);
        gGL.popMatrix();
        gGL.matrixMode(LLRender::MM_MODELVIEW);
    }
    mApply = false;
}

void LLGLUserClipPlane::setPlane(F32 a, F32 b, F32 c, F32 d)
{
    const glm::mat4& P = mProjection;
    const glm::mat4& M = mModelview;

    glm::mat4 invtrans_MVP = glm::transpose(glm::inverse(P*M));
    glm::vec4 oplane(a,b,c,d);
    glm::vec4 cplane = invtrans_MVP * oplane;

    cplane /= fabs(cplane[2]);
    cplane[3] -= 1;

    if(cplane[2] < 0)
        cplane *= -1;

    glm::mat4 suffix = glm::identity<glm::mat4>();
    suffix = glm::row(suffix, 2, cplane);
    glm::mat4 newP = suffix * P;
    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.pushMatrix();
    gGL.loadMatrix(glm::value_ptr(newP));
    gGL.matrixMode(LLRender::MM_MODELVIEW);
}

LLGLUserClipPlane::~LLGLUserClipPlane()
{
    disable();
}

LLGLDepthTest::LLGLDepthTest(GLboolean depth_enabled, GLboolean write_enabled, GLenum depth_func)
: mPrevDepthEnabled(sDepthEnabled), mPrevDepthFunc(sDepthFunc), mPrevWriteEnabled(sWriteEnabled)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    if (!depth_enabled)
    {
        write_enabled = GL_FALSE;
    }

    if (depth_enabled != sDepthEnabled)
    {
        gGL.flush();
        sDepthEnabled = depth_enabled;
    }
    if (depth_func != sDepthFunc)
    {
        gGL.flush();
        sDepthFunc = depth_func;
    }
    if (write_enabled != sWriteEnabled)
    {
        gGL.flush();
        sWriteEnabled = write_enabled;
    }
}

LLGLDepthTest::~LLGLDepthTest()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;

    if (sDepthEnabled != mPrevDepthEnabled )
    {
        gGL.flush();
        sDepthEnabled = mPrevDepthEnabled;
    }
    if (sDepthFunc != mPrevDepthFunc)
    {
        gGL.flush();
        sDepthFunc = mPrevDepthFunc;
    }
    if (sWriteEnabled != mPrevWriteEnabled )
    {
        gGL.flush();
        sWriteEnabled = mPrevWriteEnabled;
    }
}

LLGLSquashToFarClip::LLGLSquashToFarClip()
{
    glm::mat4 proj = get_current_projection();
    setProjectionMatrix(proj, 0);
}

LLGLSquashToFarClip::LLGLSquashToFarClip(const glm::mat4& P, U32 layer)
{
    setProjectionMatrix(P, layer);
}

void LLGLSquashToFarClip::setProjectionMatrix(glm::mat4 projection, U32 layer)
{
    F32 depth = 0.99999f - 0.0001f * layer;

    glm::vec4 P_row_3 = glm::row(projection, 3) * depth;
    projection = glm::row(projection, 2, P_row_3);

    LLRender::eMatrixMode last_matrix_mode = gGL.getMatrixMode();

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.pushMatrix();
    gGL.loadMatrix(glm::value_ptr(projection));

    gGL.matrixMode(last_matrix_mode);
}

LLGLSquashToFarClip::~LLGLSquashToFarClip()
{
    LLRender::eMatrixMode last_matrix_mode = gGL.getMatrixMode();

    gGL.matrixMode(LLRender::MM_PROJECTION);
    gGL.popMatrix();

    gGL.matrixMode(last_matrix_mode);
}



LLGLSPipelineSkyBox::LLGLSPipelineSkyBox()
: mCullFace(GL_CULL_FACE)
, mSquashClip()
{
}

LLGLSPipelineSkyBox::~LLGLSPipelineSkyBox()
{
}

LLGLSPipelineDepthTestSkyBox::LLGLSPipelineDepthTestSkyBox(bool depth_test, bool depth_write)
: LLGLSPipelineSkyBox()
, mDepth(depth_test ? GL_TRUE : GL_FALSE, depth_write ? GL_TRUE : GL_FALSE, GL_LEQUAL)
{

}

LLGLSPipelineBlendSkyBox::LLGLSPipelineBlendSkyBox(bool depth_test, bool depth_write)
: LLGLSPipelineDepthTestSkyBox(depth_test, depth_write)
, mBlend(GL_BLEND)
{
    gGL.setSceneBlendType(LLRender::BT_ALPHA);
}

#if LL_WINDOWS
// Expose desired use of high-performance graphics processor to Optimus driver and to AMD driver
// https://docs.nvidia.com/gameworks/content/technologies/desktop/optimus.htm
extern "C"
{
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif


