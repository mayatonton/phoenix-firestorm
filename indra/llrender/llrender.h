/**
 * @file llrender.h
 * @brief LLRender definition
 *
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

#ifndef LL_LLGLRENDER_H
#define LL_LLGLRENDER_H

#include "v2math.h"
#include "v3math.h"
#include "v4coloru.h"
#include "v4math.h"
#include "llstrider.h"
#include "llpointer.h"
#include "llglheaders.h"
#include "llmatrix4a.h"
#include "glm/mat4x4.hpp"
#include <boost/align/aligned_allocator.hpp>

#include <array>
#include <list>

class LLVertexBuffer;
class LLCubeMap;
class LLImageGL;
class LLRenderTarget;
class LLTexture;
class LLVertexBufferData;

#define LL_MATRIX_STACK_DEPTH 32

constexpr U32 LL_NUM_TEXTURE_LAYERS = 32;
constexpr U32 LL_NUM_LIGHT_UNITS = 8;

class LLTexUnit
{
    friend class LLRender;
public:

    typedef enum
    {
        TT_TEXTURE = 0,
        TT_RECT_TEXTURE,
        TT_CUBE_MAP,
        TT_CUBE_MAP_ARRAY,
        TT_MULTISAMPLE_TEXTURE,
        TT_TEXTURE_3D,
        TT_NONE,
    } eTextureType;

    typedef enum
    {
        TAM_WRAP = 0,
        TAM_MIRROR,
        TAM_CLAMP
    } eTextureAddressMode;

    typedef enum
    {
        TFO_POINT = 0,
        TFO_BILINEAR,
        TFO_TRILINEAR,
        TFO_ANISOTROPIC
    } eTextureFilterOptions;

    typedef enum
    {
        TMG_NONE = 0,
        TMG_AUTO,
        TMG_MANUAL
    } eTextureMipGeneration;

    LLTexUnit(S32 index = -1);

    void refreshState(void);

    S32 getIndex(void) const { return mIndex; }

    void activate(void);

    void enable(eTextureType type);

    void disable(void);

    bool bind(LLImageGL* texture, bool for_rendering = false, bool forceBind = false);
    bool bind(LLTexture* texture, bool for_rendering = false, bool forceBind = false);

    void bindFast(LLTexture* texture);

    bool bind(LLCubeMap* cubeMap);

    bool bind(LLRenderTarget * renderTarget, bool bindDepth = false);

    bool bindManual(eTextureType type, U32 texture, bool hasMips = false);

    void unbind(eTextureType type);

    void unbindFast(eTextureType type);

    void setTextureAddressMode(eTextureAddressMode mode);
    void setTextureAddressModeFast(eTextureAddressMode mode, eTextureType tex_type);

    void setTextureFilteringOption(LLTexUnit::eTextureFilterOptions option);
    void setTextureFilteringOptionFast(LLTexUnit::eTextureFilterOptions option, eTextureType tex_type);

    eTextureType getCurrType(void) { return mCurrTexType; }

    void setHasMipMaps(bool hasMips) { mHasMipMaps = hasMips; }

    LLImageGL*          mCurrImageGL = nullptr;

    LLCubeMap*          mCurrCubeMap = nullptr;

    LLRenderTarget*     mCurrRenderTarget = nullptr;
    U32                 mCurrRTAttachment = 0;
    bool                mCurrRTDepth = false;
    bool                mCurrCompareMode = false;

    VkImageView         getLiveVkImageView() const;

    U8                  getLiveVkImageViewDim() const;

    eTextureAddressMode   mCurrAddressMode  = TAM_WRAP;
    eTextureFilterOptions mCurrFilterOption = TFO_ANISOTROPIC;

    VkSampler           getLiveVkSampler() const;

    void vkNotifyShaderChannelBound();

protected:
    friend class LLRender;

    S32                 mIndex;
    eTextureType        mCurrTexType;
    bool                mHasMipMaps;
};

class LLLightState
{
public:
    LLLightState(S32 index = -1);

    void enable();
    void disable();
    void setDiffuse(const LLColor4& diffuse);
    void setDiffuseB(const LLColor4& diffuse);
    void setAmbient(const LLColor4& ambient);
    void setSpecular(const LLColor4& specular);
    void setPosition(const LLVector4& position);
    void setConstantAttenuation(const F32& atten);
    void setLinearAttenuation(const F32& atten);
    void setQuadraticAttenuation(const F32& atten);
    void setSpotExponent(const F32& exponent);
    void setSpotCutoff(const F32& cutoff);
    void setSpotDirection(const LLVector3& direction);
    void setSunPrimary(bool v);
    void setSize(F32 size);
    void setFalloff(F32 falloff);

protected:
    friend class LLRender;

    S32 mIndex;
    bool mEnabled;
    LLColor4 mDiffuse;
    LLColor4 mDiffuseB;
    bool     mSunIsPrimary;
    LLColor4 mAmbient;
    LLColor4 mSpecular;
    LLVector4 mPosition;
    LLVector3 mSpotDirection;

    F32 mConstantAtten;
    F32 mLinearAtten;
    F32 mQuadraticAtten;

    F32 mSpotExponent;
    F32 mSpotCutoff;
    F32 mSize = 0.f;
    F32 mFalloff = 0.f;
};

class LLRender
{
    friend class LLTexUnit;
public:

    enum eTexIndex : U8
    {
        DIFFUSE_MAP            = 0,
        ALTERNATE_DIFFUSE_MAP  = 1,
        NORMAL_MAP             = 1,
        SPECULAR_MAP           = 2,
        BASECOLOR_MAP          = 3,
        METALLIC_ROUGHNESS_MAP = 4,
        GLTF_NORMAL_MAP        = 5,
        EMISSIVE_MAP           = 6,
        NUM_TEXTURE_CHANNELS   = 7,
    };

    enum eVolumeTexIndex : U8
    {
        LIGHT_TEX = 0,
        SCULPT_TEX,
        NUM_VOLUME_TEXTURE_CHANNELS,
    };

    enum eGeomModes : U8
    {
        TRIANGLES = 0,
        TRIANGLE_STRIP,
        TRIANGLE_FAN,
        POINTS,
        LINES,
        LINE_STRIP,
        LINE_LOOP,
        NUM_MODES
    };

    enum eCompareFunc : U8
    {
        CF_NEVER = 0,
        CF_ALWAYS,
        CF_LESS,
        CF_LESS_EQUAL,
        CF_EQUAL,
        CF_NOT_EQUAL,
        CF_GREATER_EQUAL,
        CF_GREATER,
        CF_DEFAULT
    };

    enum eBlendType : U8
    {
        BT_ALPHA = 0,
        BT_ADD,
        BT_ADD_WITH_ALPHA,
        BT_MULT,
        BT_MULT_ALPHA,
        BT_MULT_X2,
        BT_REPLACE
    };

    enum eBlendFactor : U8
    {
        BF_ONE = 0,
        BF_ZERO = 1,
        BF_DEST_COLOR = 2,
        BF_SOURCE_COLOR = 3,
        BF_ONE_MINUS_DEST_COLOR = 4,
        BF_ONE_MINUS_SOURCE_COLOR = 5,
        BF_DEST_ALPHA = 6,
        BF_SOURCE_ALPHA = 7,
        BF_ONE_MINUS_DEST_ALPHA = 8,
        BF_ONE_MINUS_SOURCE_ALPHA = 9,
        BF_UNDEF
    };

    enum eMatrixMode : U8
    {
        MM_MODELVIEW = 0,
        MM_PROJECTION,
        MM_TEXTURE0,
        MM_TEXTURE1,
        MM_TEXTURE2,
        MM_TEXTURE3,
        NUM_MATRIX_MODES,
        MM_TEXTURE
    };

    LLRender();
    ~LLRender();
    bool init(bool needs_vertex_buffer);
    void initVertexBuffer();
    void resetVertexBuffer();
    void shutdown();

    void refreshState(void);

    void translatef(const GLfloat& x, const GLfloat& y, const GLfloat& z);
    void scalef(const GLfloat& x, const GLfloat& y, const GLfloat& z);
    void rotatef(const GLfloat& a, const GLfloat& x, const GLfloat& y, const GLfloat& z);
    void ortho(F32 left, F32 right, F32 bottom, F32 top, F32 zNear, F32 zFar);

    void pushMatrix();
    void popMatrix();
    void loadMatrix(const GLfloat* m);
    void loadIdentity();
    void multMatrix(const GLfloat* m);
    void matrixMode(eMatrixMode mode);
    eMatrixMode getMatrixMode();

    const glm::mat4& getModelviewMatrix();
    const glm::mat4& getProjectionMatrix();

    void syncMatrices();
    void syncLightState();

    void translateUI(F32 x, F32 y, F32 z);
    void scaleUI(F32 x, F32 y, F32 z);
    void pushUIMatrix();
    void popUIMatrix();
    void loadUIIdentity();
    LLVector3 getUITranslation();
    LLVector3 getUIScale();

    void flush();

    void beginList(std::list<LLVertexBufferData> *list);
    void endList();

    void begin(const GLuint& mode);
    void end();

    U8 getMode() const { return mMode; }

    void vertex2i(const GLint& x, const GLint& y);
    void vertex2f(const GLfloat& x, const GLfloat& y);
    void vertex3f(const GLfloat& x, const GLfloat& y, const GLfloat& z);
    void vertex2fv(const GLfloat* v);
    void vertex3fv(const GLfloat* v);

    void texCoord2i(const GLint& x, const GLint& y);
    void texCoord2f(const GLfloat& x, const GLfloat& y);
    void texCoord2fv(const GLfloat* tc);

    void color4ub(const GLubyte& r, const GLubyte& g, const GLubyte& b, const GLubyte& a);
    void color4f(const GLfloat& r, const GLfloat& g, const GLfloat& b, const GLfloat& a);
    void color4fv(const GLfloat* c);
    void color3f(const GLfloat& r, const GLfloat& g, const GLfloat& b);
    void color3fv(const GLfloat* c);
    void color4ubv(const GLubyte* c);

    void diffuseColor3f(F32 r, F32 g, F32 b);
    void diffuseColor3fv(const F32* c);
    void diffuseColor4f(F32 r, F32 g, F32 b, F32 a);
    void diffuseColor4fv(const F32* c);
    void diffuseColor4ubv(const U8* c);
    void diffuseColor4ub(U8 r, U8 g, U8 b, U8 a);

    void transform(LLVector3& vert);
    void transform(LLVector4a& vert);
    void untransform(LLVector3& vert);

    void batchTransform(LLVector4a* verts, U32 vert_count);

    void vertexBatchPreTransformed(const std::vector<LLVector4a>& verts);
    void vertexBatchPreTransformed(const LLVector4a* verts, S32 vert_count);
    void vertexBatchPreTransformed(const LLVector4a* verts, const LLVector2* uvs, S32 vert_count);
    void vertexBatchPreTransformed(const LLVector4a* verts, const LLVector2* uvs, const LLColor4U*, S32 vert_count);

    void setColorMask(bool writeColor, bool writeAlpha);
    void setColorMask(bool writeColorR, bool writeColorG, bool writeColorB, bool writeAlpha);
    void setSceneBlendType(eBlendType type);

    void blendFunc(eBlendFactor sfactor, eBlendFactor dfactor);
    void blendFunc(eBlendFactor color_sfactor, eBlendFactor color_dfactor,
               eBlendFactor alpha_sfactor, eBlendFactor alpha_dfactor);

    LLLightState* getLight(U32 index);

    void getLightArrayData(F32* position_out, F32* direction_out, F32* attenuation_out, F32* diffuse_out) const;

    void getLightDeferredAttenuationData(F32* size_out) const;

    void setAmbientLightColor(const LLColor4& color);

    void setLineWidth(F32 line_width); // <FS> Line width OGL core profile fix by Rye Mutt
    F32  getLineWidth() const { return mLineWidth; }

    void setPolygonOffset(F32 factor, F32 units);
    F32  getPolygonOffsetFactor() const { return mPolygonOffsetFactor; }
    F32  getPolygonOffsetUnits()  const { return mPolygonOffsetUnits; }

    eBlendFactor getCurrBlendColorSFactor() const { return mCurrBlendColorSFactor; }
    eBlendFactor getCurrBlendColorDFactor() const { return mCurrBlendColorDFactor; }
    eBlendFactor getCurrBlendAlphaSFactor() const { return mCurrBlendAlphaSFactor; }
    eBlendFactor getCurrBlendAlphaDFactor() const { return mCurrBlendAlphaDFactor; }

    bool getColorMaskR() const { return mCurrColorMask[0]; }
    bool getColorMaskG() const { return mCurrColorMask[1]; }
    bool getColorMaskB() const { return mCurrColorMask[2]; }
    bool getColorMaskA() const { return mCurrColorMask[3]; }

    void setClearColor(F32 r, F32 g, F32 b, F32 a);
    const F32* getClearColor() const { return mClearColor; }

    LLTexUnit* getTexUnit(U32 index);

    static void clearStaleImageGLRefs(LLImageGL* victim);

    static void clearStaleCubeMapRefs(LLCubeMap* victim);

    U32 getCurrentTexUnitIndex(void) const { return mCurrTextureUnitIndex; }

    bool verifyTexUnitActive(U32 unitToVerify);

    struct Vertex
    {
        GLfloat v[3];
        GLubyte c[4];
        GLfloat uv[2];
    };

public:
    static U32 sUICalls;
    static U32 sUIVerts;
    static bool sNsightDebugSupport;
    static LLVector2 sUIGLScaleFactor;

private:
    friend class LLLightState;

    LLVertexBuffer* bufferfromCache(U32 attribute_mask, U32 count);
    LLVertexBuffer* genBuffer(U32 attribute_mask, S32 count);
    void drawBuffer(LLVertexBuffer* vb, U32 mode, S32 count);
    void resetStriders(S32 count);

    eMatrixMode mMatrixMode;
    U32 mMatIdx[NUM_MATRIX_MODES];
    U32 mMatHash[NUM_MATRIX_MODES];
    glm::mat4 mMatrix[NUM_MATRIX_MODES][LL_MATRIX_STACK_DEPTH];
    U32 mCurMatHash[NUM_MATRIX_MODES];
    U32 mVkSyncedMatHash[NUM_MATRIX_MODES];
    U32 mLightHash;
    LLColor4 mAmbientLightColor;

    bool            mDirty;
    U32             mCount;
    U32             mMode;
    U32             mCurrTextureUnitIndex;
    bool                mCurrColorMask[4];
    F32                 mClearColor[4];
    F32             mLineWidth; // <FS> Line width OGL core profile fix by Rye Mutt
    F32             mPolygonOffsetFactor;
    F32             mPolygonOffsetUnits;
    // <FS:Ansariel> Don't ignore OpenGL max line width
    F32             mMaxLineWidthSmooth;
    F32             mMaxLineWidthAliased;
    // </FS:Ansariel>

    LLVector4a*                 mScratchVerts = nullptr;
    LLVector2*                  mScratchTexcoords = nullptr;
    LLColor4U*                  mScratchColors = nullptr;
    LLStrider<LLVector4a>       mVerticesp;
    LLStrider<LLVector2>        mTexcoordsp;
    LLStrider<LLColor4U>        mColorsp;
    std::array<LLTexUnit, LL_NUM_TEXTURE_LAYERS> mTexUnits;
    LLTexUnit           mDummyTexUnit;
    std::array<LLLightState, LL_NUM_LIGHT_UNITS> mLightState;

    eBlendFactor mCurrBlendColorSFactor;
    eBlendFactor mCurrBlendColorDFactor;
    eBlendFactor mCurrBlendAlphaSFactor;
    eBlendFactor mCurrBlendAlphaDFactor;

    std::vector<LLVector4a, boost::alignment::aligned_allocator<LLVector4a, 16> > mUIOffset;
    std::vector<LLVector4a, boost::alignment::aligned_allocator<LLVector4a, 16> > mUIScale;
};

extern thread_local F32 gGLModelView[16];
extern thread_local F32 gGLLastModelView[16];
extern F32 gGLLastProjection[16];
extern F32 gGLProjection[16];
extern S32 gGLViewport[4];
extern glm::mat4 gGLDeltaModelView;
extern glm::mat4 gGLInverseDeltaModelView;

void llSetGLViewport(S32 x, S32 y, S32 w, S32 h);

extern thread_local LLRender gGL;

const F32 OGL_TO_CFR_ROTATION[16] = {  0.f,  0.f, -1.f,  0.f,
                                      -1.f,  0.f,  0.f,  0.f,
                                       0.f,  1.f,  0.f,  0.f,
                                       0.f,  0.f,  0.f,  1.f };

glm::mat4 get_current_modelview();
glm::mat4 get_current_projection();
glm::mat4 get_last_modelview();
glm::mat4 get_last_projection();

void copy_matrix(const glm::mat4& src, F32* dst);
void set_current_modelview(const glm::mat4& mat);
void set_current_projection(const glm::mat4& mat);
void set_last_modelview(const glm::mat4& mat);
void set_last_projection(const glm::mat4& mat);

glm::vec3 mul_mat4_vec3(const glm::mat4& mat, const glm::vec3& vec);

#define LL_SHADER_LOADING_WARNS(...) LL_WARNS()

#endif
