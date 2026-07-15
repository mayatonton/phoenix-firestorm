/**
 * @file llgl.h
 * @brief LLGL definition
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

#ifndef LL_LLGL_H
#define LL_LLGL_H

#include <functional>
#include <string>
#include <boost/unordered_map.hpp>
#include <list>

#include "llerror.h"
#include "v4color.h"
#include "llstring.h"
#include "stdtypes.h"
#include "v4math.h"
#include "llplane.h"
#include "llgltypes.h"
#include "llinstancetracker.h"

#include "llglheaders.h"
#include "glm/mat4x4.hpp"

extern bool gDebugGL;
extern bool gDebugSession;
extern bool gDebugGLSession;
extern llofstream gFailLog;

#define LL_GL_ERRS LL_ERRS("RenderState")

void ll_init_fail_log(std::string filename);

void ll_fail(std::string msg);

void ll_close_fail_log();

class LLSD;

class LLGLManager
{
public:
    LLGLManager();

    bool initGL();
    void shutdownGL();

    std::string getRawGLString();

    bool mInited;
    bool mIsDisabled;

    S32 mMaxSamples;
    S32 mNumTextureImageUnits;
    S32 mMaxSampleMaskWords;
    S32 mMaxColorTextureSamples;
    S32 mMaxDepthTextureSamples;
    S32 mMaxIntegerSamples;
    S32 mGLMaxVertexRange;
    S32 mGLMaxIndexRange;
    S32 mGLMaxTextureSize;
    F32 mMaxAnisotropy = 0.f;
    S32 mMaxUniformBlockSize = 0;
    S32 mMaxVaryingVectors = 0;

    bool mHasCubeMapArray = false;
    bool mHasAnisotropic = false;

    bool mIsAMD;
    bool mIsNVIDIA;
    bool mIsIntel;
    bool mIsApple = false;

    U32 mDownScaleMethod = 0;

#if LL_DARWIN
    bool mIsMobileGF;
#endif

    bool mHasRequirements;

    S32 mDriverVersionMajor;
    S32 mDriverVersionMinor;
    S32 mDriverVersionRelease;
    F32 mGLVersion;
    S32 mGLSLVersionMajor;
    S32 mGLSLVersionMinor;
    std::string mDriverVersionVendorString;
    std::string mGLVersionString;

    U32 mVRAM;
    S32 mVRAMDetected; // <FS:Beq/> The amount detected/reported by the OS/Drivers. If different to mVRAM there is an override in place.
    std::string getGLInfoString();
    void printGLInfoString();
    void getGLInfo(LLSD& info);

    void asLLSD(LLSD& info);

    std::string mGLVendor;
    std::string mGLVendorShort;
    std::string mGLRenderer;

private:
    void initGLStates();
};

extern LLGLManager gGLManager;

class LLQuaternion;
class LLMatrix4;

void rotate_quat(LLQuaternion& rotation);

class LLGLState
{
public:
    static void initClass();
    static void restoreGL();

    static thread_local GLenum sCullFaceMode;
    static void setCullFaceMode(GLenum mode);
    static bool  isCullFaceEnabled();

    static thread_local GLenum sPolygonMode;
    static void setPolygonMode(GLenum mode);

    static thread_local GLenum sStencilFunc;
    static thread_local GLint  sStencilRef;
    static thread_local GLuint sStencilCompareMask;
    static thread_local GLenum sStencilFailOp;
    static thread_local GLenum sStencilDepthFailOp;
    static thread_local GLenum sStencilDepthPassOp;
    static thread_local GLuint sStencilWriteMask;
    static void setStencilFunc(GLenum func, GLint ref, GLuint mask);
    static void setStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
    static void setStencilMask(GLuint mask);
    static bool  isStencilTestEnabled();

    static bool  isDepthClampEnabled();

    static bool  isPolygonOffsetEnabled();

    static bool  isBlendEnabled();

protected:
    static thread_local boost::unordered_map<LLGLenum, LLGLboolean> sStateMap;

public:
    enum { CURRENT_STATE = -2, DISABLED_STATE = 0, ENABLED_STATE = 1 };
    LLGLState(LLGLenum state, S32 enabled = CURRENT_STATE);
    ~LLGLState();
    void setEnabled(S32 enabled);
    void enable() { setEnabled(ENABLED_STATE); }
    void disable() { setEnabled(DISABLED_STATE); }
protected:
    LLGLenum mState;
    bool mWasEnabled;
    bool mIsEnabled;
};

class LLGLEnableBlending : public LLGLState
{
public:
    LLGLEnableBlending(bool enable);
};

class LLGLEnableAlphaReject : public LLGLState
{
public:
    LLGLEnableAlphaReject(bool enable);
};

class LLGLEnableFunc : LLGLState
{
public:
    LLGLEnableFunc(LLGLenum state, bool enable, std::function<void()> func)
        : LLGLState(state, enable)
    {
        if (enable)
        {
            func();
        }
    }
};

class LLGLEnable : public LLGLState
{
public:
    LLGLEnable(LLGLenum state) : LLGLState(state, ENABLED_STATE) {}
};

class LLGLDisable : public LLGLState
{
public:
    LLGLDisable(LLGLenum state) : LLGLState(state, DISABLED_STATE) {}
};

class LLGLUserClipPlane
{
public:

    LLGLUserClipPlane(const LLPlane& plane, const glm::mat4& modelview, const glm::mat4& projection, bool apply = true);
    ~LLGLUserClipPlane();

    void setPlane(F32 a, F32 b, F32 c, F32 d);
    void disable();

private:
    bool mApply;

    glm::mat4 mProjection;
    glm::mat4 mModelview;
};

/*
  Modify and load projection matrix to push depth values to far clip plane.

  Restores projection matrix on destruction.
  Saves/restores matrix mode around projection manipulation.
  Does not stack.
*/
class LLGLSquashToFarClip
{
public:
    LLGLSquashToFarClip();
    LLGLSquashToFarClip(const glm::mat4& projection, U32 layer = 0);

    void setProjectionMatrix(glm::mat4 projection, U32 layer);

    ~LLGLSquashToFarClip();
};

/*
    Interface for objects that need periodic GL updates applied to them.
    Used to synchronize GL updates with GL thread.
*/
class LLGLUpdate
{
public:

    static std::list<LLGLUpdate*> sGLQ;

    bool mInQ;
    LLGLUpdate()
        : mInQ(false)
    {
    }
    virtual ~LLGLUpdate()
    {
        if (mInQ)
        {
            std::list<LLGLUpdate*>::iterator iter = std::find(sGLQ.begin(), sGLQ.end(), this);
            if (iter != sGLQ.end())
            {
                sGLQ.erase(iter);
            }
        }
    }
    virtual void updateGL() = 0;
};

#include "llglstates.h"

extern bool gHeadlessClient;
extern bool gNonInteractive;

#endif // LL_LLGL_H
