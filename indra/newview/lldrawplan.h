#ifndef LL_LLDRAWPLAN_H
#define LL_LLDRAWPLAN_H

#include "llpointer.h"
#include "llmatrix4.h"
#include "stdtypes.h"
#include <vector>

class LLVertexBuffer;
class LLViewerTexture;
class LLVOAvatar;
class LLMeshSkinInfo;
class LLFetchedGLTFMaterial;

struct LLDrawPlanItem
{
    LLPointer<LLVertexBuffer>                mVertexBuffer;
    U16                                      mStart  = 0;
    U16                                      mEnd    = 0;
    U32                                      mCount  = 0;
    U32                                      mOffset = 0;

    LLPointer<LLViewerTexture>               mTexture;
    LLPointer<LLViewerTexture>               mNormalMap;
    LLPointer<LLViewerTexture>               mSpecularMap;
    std::vector<LLPointer<LLViewerTexture> > mTextureList;

    const LLMatrix4*                         mTextureMatrix = nullptr;
    const LLMatrix4*                         mModelMatrix   = nullptr;

    LLPointer<LLVOAvatar>                    mAvatar;
    LLConstPointer<LLMeshSkinInfo>           mSkinInfo;
    LLPointer<LLFetchedGLTFMaterial>         mGLTFMaterial;

    F32                                      mBoundRadius = -1.f;
    F32                                      mObjectAlpha = 1.f;
    bool                                     mIsSSSTarget = false;
};

struct LLDrawPlan
{
    U32                         mProgramId = 0;
    std::vector<LLDrawPlanItem> mItems;
};

#endif // LL_LLDRAWPLAN_H
