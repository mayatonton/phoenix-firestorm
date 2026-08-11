 /**
 * @file lldrawpoolavatar.h
 * @brief LLDrawPoolAvatar class definition
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

#ifndef LL_LLDRAWPOOLAVATAR_H
#define LL_LLDRAWPOOLAVATAR_H

#include "lldrawpool.h"
#include "llmodel.h"

#include <unordered_map>

class LLVOAvatar;
class LLVOVolume;
class LLGLSLShader;
class LLFace;
class LLVolume;
class LLVolumeFace;
class LLDrawable;

extern U32 gFrameCount;

class LLDrawPoolAvatar : public LLFacePool
{
public:
    enum
    {
        VERTEX_DATA_MASK =  LLVertexBuffer::MAP_VERTEX |
                            LLVertexBuffer::MAP_NORMAL |
                            LLVertexBuffer::MAP_TEXCOORD0 |
                            LLVertexBuffer::MAP_WEIGHT |
                            LLVertexBuffer::MAP_CLOTHWEIGHT
    };

    ~LLDrawPoolAvatar();
    /*virtual*/ bool isDead();

typedef enum
    {
        SHADOW_PASS_AVATAR_OPAQUE,
        SHADOW_PASS_AVATAR_ALPHA_BLEND,
        SHADOW_PASS_AVATAR_ALPHA_MASK,
        NUM_SHADOW_PASSES
    } eShadowPass;

    virtual U32 getVertexDataMask() { return VERTEX_DATA_MASK; }

    virtual S32 getShaderLevel() const;

    LLDrawPoolAvatar(U32 type);

    static LLMatrix4& getModelView();

    /*virtual*/ S32  getNumPasses();
    /*virtual*/ void beginRenderPass(const LLRecordPassContext& ctx, S32 pass);
    /*virtual*/ void endRenderPass(const LLRecordPassContext& ctx, S32 pass);
    /*virtual*/ void prerender();
    /*virtual*/ void render(const LLRecordPassContext& ctx, S32 pass = 0);

    /*virtual*/ S32 getNumDeferredPasses();
    /*virtual*/ void beginDeferredPass(const LLRecordPassContext& ctx, S32 pass);
    /*virtual*/ void endDeferredPass(const LLRecordPassContext& ctx, S32 pass);
    /*virtual*/ void renderDeferred(const LLRecordPassContext& ctx, S32 pass);

    /*virtual*/ S32 getNumPostDeferredPasses();
    /*virtual*/ void beginPostDeferredPass(const LLRecordPassContext& ctx, S32 pass);
    /*virtual*/ void endPostDeferredPass(const LLRecordPassContext& ctx, S32 pass);
    /*virtual*/ void renderPostDeferred(const LLRecordPassContext& ctx, S32 pass);

    /*virtual*/ S32 getNumShadowPasses();
    /*virtual*/ void beginShadowPass(const LLRecordPassContext& ctx, S32 pass);
    /*virtual*/ void endShadowPass(const LLRecordPassContext& ctx, S32 pass);
    /*virtual*/ void renderShadow(const LLRecordPassContext& ctx, S32 pass);

    // <AYAstorm r30 P2> motion blur / velocity pass (BD lineage)
    // Known limitation: per-bone animation velocity on the classic / system
    // avatar body is degraded because lastMatrixPalette is not uploaded on
    // this path (rigged mesh attachments rendered by other pools work
    // correctly). The RenderMotionBlurSelfAvatar / OtherAvatars cvars are
    // honored by early-returning before the draw — see comment in the .cpp.
    /*virtual*/ S32 getNumMotionBlurPasses() override;
    /*virtual*/ void beginMotionBlurPass(const LLRecordPassContext& ctx, S32 pass) override;
    /*virtual*/ void endMotionBlurPass(const LLRecordPassContext& ctx, S32 pass) override;
    /*virtual*/ void renderMotionBlur(const LLRecordPassContext& ctx, S32 pass) override;
    // </AYAstorm r30 P2>

    void beginRigid(const LLRecordPassContext& ctx);
    void beginImpostor(const LLRecordPassContext& ctx);
    void beginSkinned(const LLRecordPassContext& ctx);

    void endRigid();
    void endImpostor();
    void endSkinned();

    void beginDeferredRigid(const LLRecordPassContext& ctx);
    void beginDeferredImpostor(const LLRecordPassContext& ctx);
    void beginDeferredSkinned(const LLRecordPassContext& ctx);

    void endDeferredRigid();
    void endDeferredImpostor();
    void endDeferredSkinned();

    /*virtual*/ LLViewerTexture *getDebugTexture();
    /*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display

    void renderAvatars(const LLRecordPassContext& ctx, LLVOAvatar *single_avatar, S32 pass = -1); // renders only one avatar if single_avatar is not null.

    static thread_local bool sSkipOpaque;
    static thread_local bool sSkipTransparent;
    static thread_local S32 sDiffuseChannel;

    static thread_local LLGLSLShader* sVertexProgram;
};

extern S32 AVATAR_OFFSET_POS;
extern S32 AVATAR_OFFSET_NORMAL;
extern S32 AVATAR_OFFSET_TEX0;
extern S32 AVATAR_OFFSET_TEX1;
extern S32 AVATAR_VERTEX_BYTES;
const S32 AVATAR_BUFFER_ELEMENTS = 8192; // Needs to be enough to store all avatar vertices.

#endif // LL_LLDRAWPOOLAVATAR_H
