 /**
 * @file llviewerjointmesh.cpp
 * @brief Implementation of LLViewerJointMesh class
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

#include "llviewerprecompiledheaders.h"

#include "llfasttimer.h"
#include "llrender.h"

#include "llapr.h"
#include "llbox.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolbump.h"
#include "lldynamictexture.h"
#include "llface.h"
#include "llglheaders.h"
#include "llviewertexlayer.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewertexturelist.h"
#include "llviewerjointmesh.h"
#include "llvoavatar.h"
#include "llsky.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llmath.h"
#include "v4math.h"
#include "m3math.h"
#include "m4math.h"
#include "llmatrix4a.h"
#include "llperfstats.h"
#include "llvkloader.h"
#include "llimagegl.h"


LLViewerJointMesh::LLViewerJointMesh()
    :
    LLAvatarJointMesh()
{
}

LLViewerJointMesh::~LLViewerJointMesh()
{
}

const S32 NUM_AXES = 3;

static LLMatrix4    gJointMatUnaligned[32];
static LLMatrix4a   gJointMatAligned[32];
static LLMatrix3    gJointRotUnaligned[32];
static LLVector4    gJointPivot[32];

void LLViewerJointMesh::uploadJointMatrices()
{
    S32 joint_num;
    LLPolyMesh *reference_mesh = mMesh->getReferenceMesh();
    LLDrawPool *poolp = mFace ? mFace->getPool() : NULL;
    bool hardware_skinning = (poolp && poolp->getShaderLevel() > 0);

    for (joint_num = 0; joint_num < reference_mesh->mJointRenderData.size(); joint_num++)
    {
        LLMatrix4 joint_mat = *reference_mesh->mJointRenderData[joint_num]->mWorldMatrix;

        if (hardware_skinning)
        {
            joint_mat *= LLDrawPoolAvatar::getModelView();
        }
        gJointMatUnaligned[joint_num] = joint_mat;
        gJointRotUnaligned[joint_num] = joint_mat.getMat3();
    }

    bool last_pivot_uploaded{ false };
    S32 j = 0;

    for (joint_num = 0; joint_num < reference_mesh->mJointRenderData.size(); joint_num++)
    {
        LLSkinJoint *sj = reference_mesh->mJointRenderData[joint_num]->mSkinJoint;
        if (sj)
        {
            if (!last_pivot_uploaded)
            {
                LLVector4 parent_pivot(sj->mRootToParentJointSkinOffset);
                parent_pivot.mV[VW] = 0.f;
                gJointPivot[j++] = parent_pivot;
            }

            LLVector4 child_pivot(sj->mRootToJointSkinOffset);
            child_pivot.mV[VW] = 0.f;

            gJointPivot[j++] = child_pivot;

            last_pivot_uploaded = true;
        }
        else
        {
            last_pivot_uploaded = false;
        }
    }

    for (S32 i = 0; i < j; i++)
    {
        LLVector3 pivot;
        pivot = LLVector3(gJointPivot[i]);
        pivot = pivot * gJointRotUnaligned[i];
        gJointMatUnaligned[i].translate(pivot);
    }

    if (hardware_skinning)
    {
        GLfloat mat[45*4];
        memset(mat, 0, sizeof(GLfloat)*45*4);

        for (joint_num = 0; joint_num < reference_mesh->mJointRenderData.size(); joint_num++)
        {
            gJointMatUnaligned[joint_num].transpose();

            for (S32 axis = 0; axis < NUM_AXES; axis++)
            {
                F32* vector = gJointMatUnaligned[joint_num].mMatrix[axis];
                U32 offset = LL_CHARACTER_MAX_JOINTS_PER_MESH*axis+joint_num;
                memcpy(mat+offset*4, vector, sizeof(GLfloat)*4);
            }
        }
        if (LLGLSLShader::sCurBoundShaderPtr)
        {
            if (LLVKLoader::isVulkanInitialized())
            {
                LLVKLoader::AvatarSkin_PerProgramBind data;
                std::memcpy(data.matrixPalette, mat, sizeof(data.matrixPalette));
                LLVKLoader::writeCurrentAvatarSkinUBO(data);
            }
        }
    }
    else
    {
        //load gJointMatUnaligned into gJointMatAligned
        for (joint_num = 0; joint_num < reference_mesh->mJointRenderData.size(); ++joint_num)
        {
            gJointMatAligned[joint_num].loadu(gJointMatUnaligned[joint_num]);
        }
    }
}

int compare_int(const void *a, const void *b)
{
    if (*(U32*)a < *(U32*)b)
    {
        return -1;
    }
    else if (*(U32*)a > *(U32*)b)
    {
        return 1;
    }
    else return 0;
}

U32 LLViewerJointMesh::drawShape( F32 pixelArea, bool first_pass, bool is_dummy)
{
    LL_PROFILE_ZONE_SCOPED;
    if (!mValid || !mMesh || !mFace || !mVisible ||
        !mFace->getVertexBuffer() ||
        mMesh->getNumFaces() == 0 ||
        LLGLSLShader::sCurBoundShaderPtr == NULL)
    {
        return 0;
    }

    U32 triangle_count = 0;

    S32 diffuse_channel = LLDrawPoolAvatar::sDiffuseChannel;


    if (is_dummy)
        gGL.diffuseColor4fv(LLVOAvatar::getDummyColor().mV);
    else
        gGL.diffuseColor4fv(mColor.mV);
    llassert( !(mTexture.notNull() && mLayerSet) );  // mutually exclusive

    LLViewerTexLayerSet *layerset = dynamic_cast<LLViewerTexLayerSet*>(mLayerSet);
    if (mTestTexture.notNull())
    {
        gGL.getTexUnit(diffuse_channel)->bind(mTestTexture);

        if (mIsTransparent)
        {
            gGL.diffuseColor4f(1.f, 1.f, 1.f, 1.f);
        }
        else
        {
            gGL.diffuseColor4f(0.7f, 0.6f, 0.3f, 1.f);
        }
    }
    else if( !is_dummy && layerset )
    {
        if( layerset->hasComposite() )
        {
            gGL.getTexUnit(diffuse_channel)->bind(layerset->getViewerComposite());
        }
        else
        {
            gGL.getTexUnit(diffuse_channel)->bind(LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT));
        }
    }
    else if ( !is_dummy && mTexture.notNull() )
    {
        gGL.getTexUnit(diffuse_channel)->bind(mTexture);
    }
    else
    {
        gGL.getTexUnit(diffuse_channel)->bind(LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT));
    }

    U32 start = mMesh->mFaceVertexOffset;
    U32 end = start + mMesh->mFaceVertexCount - 1;
    U32 count = mMesh->mFaceIndexCount;
    U32 offset = mMesh->mFaceIndexOffset;

    LLVertexBuffer* buff = mFace->getVertexBuffer();

    if (mMesh->hasWeights())
    {
        if ((mFace->getPool()->getShaderLevel() > 0))
        {
            if (first_pass)
            {
                uploadJointMatrices();
            }
        }

        buff->setBuffer();
        const U32 id = LLRenderPass::buildAndOverrideScenePerDrawSet(nullptr, false);
        buff->drawRange(LLRender::TRIANGLES, start, end, count, offset, id);
    }
    else
    {
        gGL.pushMatrix();
        LLMatrix4 jointToWorld = getWorldMatrix();
        gGL.multMatrix((GLfloat*)jointToWorld.mMatrix);
        buff->setBuffer();
        const U32 id = LLRenderPass::buildAndOverrideScenePerDrawSet(nullptr, false);
        buff->drawRange(LLRender::TRIANGLES, start, end, count, offset, id);
        gGL.popMatrix();
    }
    gPipeline.addTrianglesDrawn(count);

    triangle_count += count;

    return triangle_count;
}

void LLViewerJointMesh::updateFaceSizes(U32 &num_vertices, U32& num_indices, F32 pixel_area)
{
    num_vertices = (num_vertices + 0x3) & ~0x3;
    if (mMesh && mValid)
    {
        mMesh->mFaceVertexOffset = num_vertices;
        mMesh->mFaceVertexCount = mMesh->getNumVertices();
        mMesh->mFaceIndexOffset = num_indices;
        mMesh->mFaceIndexCount = mMesh->getSharedData()->mNumTriangleIndices;

        mMesh->getReferenceMesh()->mCurVertexCount = mMesh->mFaceVertexCount;

        num_vertices += mMesh->getNumVertices();
        num_indices += mMesh->mFaceIndexCount;
    }
}

void LLViewerJointMesh::updateFaceData(LLFace *face, F32 pixel_area, bool damp_wind, bool terse_update)
{

    mFace = face;

    if (!mFace->getVertexBuffer())
    {
        return;
    }

    LLDrawPool *poolp = mFace->getPool();
    bool hardware_skinning = (poolp && poolp->getShaderLevel() > 0);

    if (!hardware_skinning && terse_update)
    { //no need to do terse updates if we're doing software vertex skinning
     // since mMesh is being copied into mVertexBuffer every frame
        return;
    }

    LL_PROFILE_ZONE_SCOPED;

    LLStrider<LLVector3> verticesp;
    LLStrider<LLVector3> normalsp;
    LLStrider<LLVector2> tex_coordsp;
    LLStrider<F32>       vertex_weightsp;
    // <FS:Ansariel> Vectorized Weight4Strider and ClothWeightStrider by Drake Arconis
    //LLStrider<LLVector4> clothing_weightsp;
    LLStrider<LLVector4a> clothing_weightsp;
    LLStrider<U16> indicesp;

    if (mMesh && mValid)
    {
        const U32 num_verts = mMesh->getNumVertices();

        if (num_verts)
        {
            LLVertexBuffer* buffer = face->getVertexBuffer();
            const U32 vert_base = face->getGeomIndex() + mMesh->mFaceVertexOffset;

            buffer->getVertexStrider(verticesp, vert_base, num_verts);
            buffer->getNormalStrider(normalsp, vert_base, num_verts);

            F32* v = (F32*) verticesp.get();
            F32* n = (F32*) normalsp.get();

            U32 words = num_verts*4;

            LLVector4a::memcpyNonAliased16(v, (F32*) mMesh->getCoords(), words*sizeof(F32));
            LLVector4a::memcpyNonAliased16(n, (F32*) mMesh->getNormals(), words*sizeof(F32));


            if (!terse_update)
            {
                buffer->getTexCoord0Strider(tex_coordsp, vert_base, num_verts);
                buffer->getWeightStrider(vertex_weightsp, vert_base, num_verts);
                buffer->getClothWeightStrider(clothing_weightsp, vert_base, num_verts);

                F32* tc = (F32*) tex_coordsp.get();
                F32* vw = (F32*) vertex_weightsp.get();
                F32* cw = (F32*) clothing_weightsp.get();

                // Both allocated in LLPolyMeshSharedData::allocateVertexData(unsigned int)

                memcpy(tc, mMesh->getTexCoords(), num_verts*2*sizeof(F32) );
                memcpy(vw, mMesh->getWeights(), num_verts*sizeof(F32) );

                LLVector4a::memcpyNonAliased16(cw, (F32*) mMesh->getClothingWeights(), num_verts*4*sizeof(F32));
            }

            const U32 idx_count = mMesh->getNumFaces()*3;

            buffer->getIndexStrider(indicesp, face->getIndicesStart() + mMesh->mFaceIndexOffset, idx_count);

            U16* __restrict idx = indicesp.get();
            S32* __restrict src_idx = (S32*) mMesh->getFaces();

            const S32 offset = (S32) mMesh->mFaceVertexOffset;

            for (U32 i = 0; i < idx_count; ++i)
            {
                *(idx++) = *(src_idx++)+offset;
            }
        }
    }
}



// updateLOD()
bool LLViewerJointMesh::updateLOD(F32 pixel_area, bool activate)
{
    bool valid = mValid;
    setValid(activate, true);
    return (valid != activate);
}

void LLViewerJointMesh::updateGeometry(LLFace *mFace, LLPolyMesh *mMesh)
{
    LLStrider<LLVector3> o_vertices;
    LLStrider<LLVector3> o_normals;

    LLVertexBuffer* buffer = mFace->getVertexBuffer();
    buffer->getVertexStrider(o_vertices, mMesh->mFaceVertexOffset, mMesh->getNumVertices());
    buffer->getNormalStrider(o_normals, mMesh->mFaceVertexOffset, mMesh->getNumVertices());

    F32* __restrict vert = o_vertices[0].mV;
    F32* __restrict norm = o_normals[0].mV;

    const F32* __restrict weights = mMesh->getWeights();
    const LLVector4a* __restrict coords = (LLVector4a*) mMesh->getCoords();
    const LLVector4a* __restrict normals = (LLVector4a*) mMesh->getNormals();

    for (U32 index = 0; index < mMesh->getNumVertices(); index++)
    {
        // equivalent to joint = floorf(weights[index]);
        S32 joint = _mm_cvtt_ss2si(_mm_load_ss(weights+index));
        F32 w = weights[index] - joint;

        LLMatrix4a gBlendMat;

        if (w != 0.f)
        {
            gBlendMat.setLerp(gJointMatAligned[joint+0],
                              gJointMatAligned[joint+1], w);

            LLVector4a res;
            gBlendMat.affineTransform(coords[index], res);
            res.store4a(vert+index*4);
            gBlendMat.rotate(normals[index], res);
            res.store4a(norm+index*4);
        }
        else
        {  // No lerp required in this case.
            LLVector4a res;
            gJointMatAligned[joint].affineTransform(coords[index], res);
            res.store4a(vert+index*4);
            gJointMatAligned[joint].rotate(normals[index], res);
            res.store4a(norm+index*4);
        }
    }

    buffer->unmapBuffer();
}

void LLViewerJointMesh::updateJointGeometry()
{
    if (!(mValid
          && mMesh
          && mFace
          && mMesh->hasWeights()
          && mFace->getVertexBuffer()
          && LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_AVATAR) == 0))
    {
        return;
    }

    uploadJointMatrices();
    updateGeometry(mFace, mMesh);
}

void LLViewerJointMesh::dump()
{
    if (mValid)
    {
        LL_INFOS() << "Usable LOD " << mName << LL_ENDL;
    }
}
