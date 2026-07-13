/**
 * @file llavatarjointmesh.h
 * @brief Declaration of LLAvatarJointMesh class
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

#ifndef LL_LLAVATARJOINTMESH_H
#define LL_LLAVATARJOINTMESH_H

#include "llavatarjoint.h"
#include "llgltexture.h"
#include "llpolymesh.h"
#include "v4color.h"

class LLDrawable;
class LLFace;
class LLCharacter;
class LLTexLayerSet;

typedef enum e_avatar_render_pass
{
    AVATAR_RENDER_PASS_SINGLE,
    AVATAR_RENDER_PASS_CLOTHING_INNER,
    AVATAR_RENDER_PASS_CLOTHING_OUTER
} EAvatarRenderPass;

class LLSkinJoint
{
public:
    LLSkinJoint();
    ~LLSkinJoint();
    bool setupSkinJoint( LLAvatarJoint *joint);

    LLAvatarJoint   *mJoint;
    LLVector3       mRootToJointSkinOffset;
    LLVector3       mRootToParentJointSkinOffset;
};

class LLAvatarJointMesh : public virtual LLAvatarJoint
{
protected:
    LLColor4                    mColor;
    F32                         mShiny;
    LLPointer<LLGLTexture>      mTexture;
    LLTexLayerSet*              mLayerSet;
    LLPointer<LLGLTexture>      mTestTexture;
    LLPolyMesh*                 mMesh;
    bool                        mCullBackFaces;
    LLFace*                     mFace;

    U32                         mFaceIndexCount;

    U32                         mNumSkinJoints;
    LLSkinJoint*                mSkinJoints;
    S32                         mMeshID;

public:
    static bool                 sPipelineRender;
    static LLColor4             sClothingInnerColor;

public:
    LLAvatarJointMesh();
    virtual ~LLAvatarJointMesh();

    void getColor( F32 *red, F32 *green, F32 *blue, F32 *alpha );
    void setColor( F32 red, F32 green, F32 blue, F32 alpha );
    void setColor( const LLColor4& color );
    void setSpecular( const LLColor4& color, F32 shiny ) { mShiny = shiny; };
    void setTexture( LLGLTexture *texture );
    bool hasGLTexture() const;
    void setTestTexture( LLGLTexture* texture ) { mTestTexture = texture; }
    void setLayerSet( LLTexLayerSet* layer_set );
    bool hasComposite() const;
    LLPolyMesh *getMesh();
    void setMesh( LLPolyMesh *mesh );
    void setupJoint(LLAvatarJoint* current_joint);
    void setMeshID( S32 id ) {mMeshID = id;}
    S32 getMeshID() { return mMeshID; }
    void setIsTransparent(bool is_transparent) { mIsTransparent = is_transparent; }

private:
    bool allocateSkinData( U32 numSkinJoints );
    void freeSkinData();
};

#endif // LL_LLAVATARJOINTMESH_H
