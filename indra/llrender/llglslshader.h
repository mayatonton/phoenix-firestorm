/**
 * @file llglslshader.h
 * @brief GLSL shader wrappers
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

#ifndef LL_LLGLSLSHADER_H
#define LL_LLGLSLSHADER_H

#include "llgl.h"
#include "llrender.h"
#include "llstaticstringtable.h"
#include <boost/json.hpp>
#include <unordered_map>

// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-1: codegen-emitted
// `ubo::UniformLocation` struct (= block_hash / offset / size / cadence_tag) を
// LLGLSLShader member 宣言で使う。include path は AyaUboCodegen.cmake の
// AYA_UBO_CODEGEN_INCLUDE_DIR (= ${CMAKE_BINARY_DIR}/codegen) を llrender が
// PUBLIC export しているため、llrender 依存 target に透過 (= spec 08 §99 literal
// 「shader 側 setter から `#include "ubo/ubo_index.inl"` 形」と整合)。
#include "ubo/ubo_perfect_hash.inl"

class LLShaderFeatures
{
public:
    S32 mIndexedTextureChannels = 0;
    bool calculatesLighting = false;
    bool calculatesAtmospherics = false;
    bool hasLighting = false; // implies no transport (it's possible to have neither though)
    bool isAlphaLighting = false; // indicates lighting shaders need not be linked in (lighting performed directly in alpha shader to match deferred lighting functions)
    bool isSpecular = false;
    bool hasTransport = false; // implies no lighting (it's possible to have neither though)
    bool hasSkinning = false;
    bool hasObjectSkinning = false;
    bool mGLTF = false;
    bool hasAtmospherics = false;
    bool hasGamma = false;
    bool hasShadows = false;
    bool hasAmbientOcclusion = false;
    bool hasSrgb = false;
    bool isDeferred = false;
    bool hasFullGBuffer = false;
    bool hasScreenSpaceReflections = false;
    bool hasAlphaMask = false;
    bool hasReflectionProbes = false;
    bool attachNothing = false;
    bool hasHeroProbes = false;
    bool isPBRTerrain = false;
    bool hasTonemap = false;
    // <AYAstorm r30 P2> Gates velocityFuncV.glsl auto-attach in
    // attachShaderFeatures() so velocity-shader variants can compile.
    bool hasMotionBlur = false;
    // </AYAstorm r30 P2>
};

// ============= Structure for caching shader uniforms ===============
class LLGLSLShader;

class LLShaderUniforms
{
public:

    template<typename T>
    struct UniformSetting
    {
        S32 mUniform{ 0 };
        T mValue{};
    };

    typedef UniformSetting<S32> IntSetting;
    typedef UniformSetting<F32> FloatSetting;
    typedef UniformSetting<LLVector4> VectorSetting;
    typedef UniformSetting<LLVector3> Vector3Setting;

    void clear()
    {
        mIntegers.resize(0);
        mFloats.resize(0);
        mVectors.resize(0);
        mVector3s.resize(0);
    }

    void uniform1i(S32 index, S32 value)
    {
        mIntegers.push_back({ index, value });
    }

    void uniform1f(S32 index, F32 value)
    {
        mFloats.push_back({ index, value });
    }

    void uniform4fv(S32 index, const LLVector4& value)
    {
        mVectors.push_back({ index, value });
    }

    void uniform4fv(S32 index, const F32* value)
    {
        mVectors.push_back({ index, LLVector4(value) });
    }

    void uniform3fv(S32 index, const LLVector3& value)
    {
        mVector3s.push_back({ index, value });
    }

    void uniform3fv(S32 index, const F32* value)
    {
        mVector3s.push_back({ index, LLVector3(value) });
    }

    void apply(LLGLSLShader* shader);


    std::vector<IntSetting> mIntegers;
    std::vector<FloatSetting> mFloats;
    std::vector<VectorSetting> mVectors;
    std::vector<Vector3Setting> mVector3s;
};
class LLGLSLShader
{
public:
    // NOTE: Keep gShaderConsts and LLGLSLShader::ShaderConsts_e in sync!
    enum eShaderConsts
    {
        SHADER_CONST_CLOUD_MOON_DEPTH
        , SHADER_CONST_STAR_DEPTH
        , NUM_SHADER_CONSTS
    };

    // enum primarily used to control application sky settings uniforms
    typedef enum
    {
        SG_DEFAULT = 0,  // not sky or water specific
        SG_SKY,  //
        SG_WATER,
        SG_ANY,
        SG_COUNT
    } eGroup;

    enum UniformBlock : GLuint
    {
        UB_REFLECTION_PROBES,   // "ReflectionProbes"
        UB_GLTF_JOINTS,         // "GLTFJoints"
        UB_GLTF_NODES,          // "GLTFNodes"
        UB_GLTF_MATERIALS,      // "GLTFMaterials"
        NUM_UNIFORM_BLOCKS
    };


    static std::set<LLGLSLShader*> sInstances;
    static bool sProfileEnabled;
    static bool sCanProfile;

    LLGLSLShader();
    ~LLGLSLShader();

    // r41 sub-step 4.3-γ'-port-β-2 (sub-doc 06 §3.1 sub-step 6.3 / handoff
    // -substep-4-3-gamma-prime-port-beta-2-prep.md §2 axis (a)):
    // per-program SPIR-V hook で渡す stage 単位 source 蓄積。LLGLSLShader::createShader()
    // 内の loadShaderFile() loop が Vulkan path 限定で push_back。loop 完遂後
    // generatePerProgramSPIRV() に渡し、全 stage concat → 単一 glslang::TProgram link
    // → 各 stage 個別 GlslangToSpv で program 単位 SPIR-V 生成。β-1 per-file model の
    // architectural mismatch (forward declaration mirrorClip / encodeNormal /
    // passTextureIndex / getObjectSkinnedTransform link fail) 解消が目的。
    struct StageSource
    {
        GLenum type;                        // GL_VERTEX_SHADER / GL_FRAGMENT_SHADER
        std::string file_name;              // open_file_name (gpu_class 解決後)
        std::vector<std::string> sources;   // loadShaderFile() preprocessing 後 shader_code_text[] copy
    };

    static GLuint sCurBoundShader;
    static LLGLSLShader* sCurBoundShaderPtr;
    static S32 sIndexedTextureChannels;

    static U32 sMaxGLTFMaterials;
    static U32 sMaxGLTFNodes;

    static void initProfile();
    static void finishProfile(boost::json::value& stats=sDefaultStats);

    static void startProfile();
    static void stopProfile();

    void unload();
    void clearStats();
    void dumpStats(boost::json::object& stats);

    // place query objects for profiling if profiling is enabled
    // if for_runtime is true, will place timer query only whether or not profiling is enabled
    void placeProfileQuery(bool for_runtime = false);

    // Readback query objects if profiling is enabled
    // If for_runtime is true, will readback timer query iff query is available
    // Will return false if a query is pending (try again later)
    // If force_read is true, will force an immediate readback (severe performance penalty)
    bool readProfileQuery(bool for_runtime = false, bool force_read = false);

    bool createShader();
    bool attachFragmentObject(std::string object);
    bool attachVertexObject(std::string object);
    void attachObject(GLuint object);
    void attachObjects(GLuint* objects = NULL, S32 count = 0);
    bool mapAttributes();
    bool mapUniforms();
    void mapUniform(GLint index);
    void uniform1i(U32 index, GLint i);
    void uniform1f(U32 index, GLfloat v);
    void fastUniform1f(U32 index, GLfloat v);
    void uniform2f(U32 index, GLfloat x, GLfloat y);
    void uniform3f(U32 index, GLfloat x, GLfloat y, GLfloat z);
    void uniform4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void uniform1iv(U32 index, U32 count, const GLint* i);
    void uniform4iv(U32 index, U32 count, const GLint* i);
    void uniform1fv(U32 index, U32 count, const GLfloat* v);
    void uniform2fv(U32 index, U32 count, const GLfloat* v);
    void uniform3fv(U32 index, U32 count, const GLfloat* v);
    void uniform4fv(U32 index, U32 count, const GLfloat* v);
    void uniform4uiv(U32 index, U32 count, const GLuint* v);
    void uniform2i(const LLStaticHashedString& uniform, GLint i, GLint j);
    void uniformMatrix2fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v);
    void uniformMatrix3fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v);
    void uniformMatrix3x4fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v);
    void uniformMatrix4fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v);
    void uniform1i(const LLStaticHashedString& uniform, GLint i);
    void uniform1iv(const LLStaticHashedString& uniform, U32 count, const GLint* v);
    void uniform4iv(const LLStaticHashedString& uniform, U32 count, const GLint* v);
    void uniform1f(const LLStaticHashedString& uniform, GLfloat v);
    void uniform2f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y);
    void uniform3f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y, GLfloat z);
    void uniform4f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void uniform1fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v);
    void uniform2fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v);
    void uniform3fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v);
    void uniform4fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v);
    void uniform4uiv(const LLStaticHashedString& uniform, U32 count, const GLuint* v);
    void uniformMatrix4fv(const LLStaticHashedString& uniform, U32 count, GLboolean transpose, const GLfloat* v);

    void setMinimumAlpha(F32 minimum);

    void vertexAttrib4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void vertexAttrib4fv(U32 index, GLfloat* v);

    //GLint getUniformLocation(const std::string& uniform);
    GLint getUniformLocation(const LLStaticHashedString& uniform);
    GLint getUniformLocation(U32 index);

    GLint getAttribLocation(U32 attrib);
    GLint mapUniformTextureChannel(GLint location, GLenum type, GLint size);

    void clearPermutations();
    void addPermutation(std::string name, std::string value);
    void addPermutations(const std::map<std::string, std::string>& defines)
    {
        mDefines.insert(defines.begin(), defines.end());
    }
    void removePermutation(std::string name);

    void addConstant(const LLGLSLShader::eShaderConsts shader_const);

    //enable/disable texture channel for specified uniform
    //if given texture uniform is active in the shader,
    //the corresponding channel will be active upon return
    //returns channel texture is enabled in from [0-MAX)
    S32 enableTexture(S32 uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);
    S32 disableTexture(S32 uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);

    // get the texture channel of the given uniform, or -1 if uniform is not used as a texture
    S32 getTextureChannel(S32 uniform) const;

    // bindTexture returns the texture unit we've bound the texture to.
    // You can reuse the return value to unbind a texture when required.
    S32 bindTexture(const std::string& uniform, LLTexture* texture, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);
    S32 bindTexture(S32 uniform, LLTexture* texture, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);
    S32 bindTexture(const std::string& uniform, LLRenderTarget* texture, bool depth = false, LLTexUnit::eTextureFilterOptions mode = LLTexUnit::TFO_BILINEAR);
    S32 bindTexture(S32 uniform, LLRenderTarget* texture, bool depth = false, LLTexUnit::eTextureFilterOptions mode = LLTexUnit::TFO_BILINEAR, U32 index = 0);
    S32 unbindTexture(const std::string& uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);
    S32 unbindTexture(S32 uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);

    bool link(bool suppress_errors = false);
    void bind();
    //helper to conditionally bind mRiggedVariant instead of this
    void bind(bool rigged);

    bool isComplete() const { return mProgramObject != 0; }

    LLUUID hash();

    // Unbinds any previously bound shader by explicitly binding no shader.
    static void unbind();

    U32 mMatHash[LLRender::NUM_MATRIX_MODES];
    U32 mLightHash;

    GLuint mProgramObject;
#if LL_RELEASE_WITH_DEBUG_INFO
    struct attr_name
    {
        GLint loc;
        const char* name;
        void operator = (GLint _loc) { loc = _loc; }
        operator GLint () { return loc; }
    };
    std::vector<attr_name> mAttribute; //lookup table of attribute enum to attribute channel
#else
    std::vector<GLint> mAttribute; //lookup table of attribute enum to attribute channel
#endif
    U32 mAttributeMask;  //mask of which reserved attributes are set (lines up with LLVertexBuffer::getTypeMask())
    std::vector<GLint> mUniform;   //lookup table of uniform enum to uniform location
    LLStaticStringTable<GLint> mUniformMap; //lookup map of uniform name to uniform location
    typedef std::unordered_map<GLint, LLVector4> uniform_value_map_t;
    uniform_value_map_t mValue; //lookup map of uniform location to last known value
    std::vector<GLint> mTexture;
    S32 mTotalUniformSize;
    S32 mActiveTextureChannels;
    S32 mShaderLevel;
    S32 mShaderGroup; // see LLGLSLShader::eGroup
    bool mUniformsDirty;
    LLShaderFeatures mFeatures;
    std::vector< std::pair< std::string, GLenum > > mShaderFiles;
    std::string mName;
    typedef std::map<std::string, std::string> defines_map_t; //NOTE: this must be an ordered map to maintain hash consistency
    defines_map_t mDefines;
    static defines_map_t sGlobalDefines;
    LLUUID mShaderHash;
    bool mUsingBinaryProgram = false;

    //statistics for profiling shader performance
    bool mProfilePending = false;
    U32 mTimerQuery;
    U32 mSamplesQuery;
    U32 mPrimitivesQuery;

    U64 mTimeElapsed;
    static U64 sTotalTimeElapsed;
    U32 mTrianglesDrawn;
    static U32 sTotalTrianglesDrawn;
    U64 mSamplesDrawn;
    static U64 sTotalSamplesDrawn;
    U32 mBinds;
    static U32 sTotalBinds;

    // this pointer should be set to whichever shader represents this shader's rigged variant
    LLGLSLShader* mRiggedVariant = nullptr;

    // variants for use by GLTF renderer
    // bit 0 = alpha mode blend (1) or opaque (0)
    // bit 1 = rigged (1) or static (0)
    // bit 2 = unlit (1) or lit (0)
    // bit 3 = single (0) or multi (1) uv coordinates
    struct GLTFVariant
    {
        constexpr static U8 ALPHA_BLEND = 1;
        constexpr static U8 RIGGED = 2;
        constexpr static U8 UNLIT = 4;
        constexpr static U8 MULTI_UV = 8;
    };

    constexpr static U8 NUM_GLTF_VARIANTS = 16;

    std::vector<LLGLSLShader> mGLTFVariants;

    //helper to bind GLTF variant
    void bind(U8 variant);

    // hacky flag used for optimization in LLDrawPoolAlpha
    bool mCanBindFast = false;

    // r41 sub-step 4.3-γ'-port-β-2: Vulkan path 限定で createShader() 内 loop が充填、
    // 全 stage 蓄積後 generatePerProgramSPIRV() に渡す。GL path / Vulkan 未初期化時 untouched。
    std::vector<StageSource> mStageSources;

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B2-γ: Vulkan path 限定で
    // attachVertexObject / attachFragmentObject が attach 順 (= attachShaderFeatures()
    // が決める順序) で filename を push。generatePerProgramSPIRV() は LLShaderMgr 側の
    // mVertex/FragmentShaderSourceCache から各 filename で source を引いて、stage 単位
    // concat の program-specific mShaderFiles より前に prepend する。GL path / Vulkan
    // 未初期化時 untouched。createShader() 完遂後 clear + shrink_to_fit。
    std::vector<std::string> mVulkanAttachedVertexUtilities;
    std::vector<std::string> mVulkanAttachedFragmentUtilities;

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-1:
    // UBO redirect 層 cache 構造 (spec 06a §3 / §5)。
    //
    // mUniformUBOLoc: mUniform (= GL uniform location 配列) と並列 (同 size / 同 index 順)
    //   で配置される ubo::UniformLocation 配列。integer index 経由 setter (= uniform1i /
    //   uniformMatrix4fv 等 17 method) は `mUniformUBOLoc[index]` で O(1) 引き、UBO 内
    //   offset / size / cadence_tag を得て forwardToUboUpload() に転送 (= PB-2 / PB-4)。
    //
    // mUniformUBOLocByHash: LLStaticHashedString 経由 setter (= uniform1f(const
    //   LLStaticHashedString&, ...) 等 13 method) 用の補助 cache。mReservedUniforms 未登録
    //   uniform は integer index 経路に存在しないため、hash → UniformLocation の hash map で
    //   別途引く (S1-C 採用 = build-time list `g_static_hashed_uniform_names[]` iterate で
    //   mapUniforms() 時に構築、= PB-3 / PB-5)。
    //
    // mUseUBO: false default で OpenGL path 維持 (= MUSEUBO-A 確定、09 §4.2 注 literal
    //   「Phase 1 完了時点 mUseUBO=false default で OpenGL path 経路選択」)。Vulkan path
    //   実走判定 (= 自動判定 vs cvar) は 06c で詰める = Phase 1.C 領域。
    //
    // C++ compile-time gate (= `#ifdef LL_VULKAN_GLSL`) は不使用 (= GATE-B 確定 2026-06-04、
    // LL_VULKAN_GLSL は GLSL preprocessor 専用 macro で C++ context 未定義)。runtime gate は
    // mUseUBO のみ。spec 06a §3.2 / §5.3 literal の `#ifdef LL_VULKAN_GLSL` は C++ 側のみ
    // 非適用、GLSL shader 側は引き続き有効。
    std::vector<ubo::UniformLocation> mUniformUBOLoc;
    std::unordered_map<U64 /*hash*/, ubo::UniformLocation> mUniformUBOLocByHash;
    bool mUseUBO = false;

#if LL_PROFILER_ENABLE_RENDER_DOC
    void setLabel(const char* label);
#endif

private:
    // r41 sub-step 4.3-γ'-port-β-2: 全 stage concat → glslang::TProgram link → 各 stage
    // 個別 GlslangToSpv で program 単位 SPIR-V 生成 + cache layer + VkShaderModule 生成。
    // cache key = HBXXH128(全 stage file_name + 全 stage source concat)、cache file 命名
    // <program_hash>_program.spv (custom container: [u32 stage_count][repeat: u32 type,
    // u32 spv_word_count, spv_words...])。生成後 mStageSources を clear で memory 緩和。
    bool generatePerProgramSPIRV(const std::vector<StageSource>& stages);

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-6: UBO redirect 層
    // shell 実装 (= FWD-1 採用、空 stub)。spec 06a §5.2 literal「forwardToUboUpload(loc,
    // &x, sizeof(GLfloat));  // 06b で実装」の interface 宣言部のみ本 sub-step で確定し、
    // 本体 (= 実 memcpy / ring buffer / dynamic offset / thread 配線) は 06b / chapter 07
    // で実装する。本 sub-step は declaration + 空 stub の link 通し限定。
    //
    // PB-4 (= 17 method integer index 経路) / PB-5 (= 13 method LLStaticHashedString 経路)
    // の各 setter から `if (mUseUBO) { ... forwardToUboUpload(loc, data, size); return; }`
    // pattern で call する。mUseUBO=false default ゆえ本 stub は実走しない (= MUSEUBO-A
    // 整合)。順序組替えで PB-4 直前に前倒し (= PB-3 complete handoff §3.6) = call site
    // (PB-4) より先に declaration + stub が link error 回避の technical compile dependency。
    void forwardToUboUpload(const ubo::UniformLocation& loc, const void* data, size_t size);

    void unloadInternal();
    // This must be static because finishProfile() is called at least once
    // within a __try block. If we default its stats parameter to a temporary
    // json::value, that temporary must be destroyed when the stack is
    // unwound, which __try forbids.
    static boost::json::value sDefaultStats;
};

//UI shader (declared here so llui_libtest will link properly)
extern LLGLSLShader         gUIProgram;
//output vec4(color.rgb,color.a*tex0[tc0].a)
extern LLGLSLShader         gSolidColorProgram;
//Alpha mask shader (declared here so llappearance can access properly)
extern LLGLSLShader         gAlphaMaskProgram;

#if LL_PROFILER_ENABLE_RENDER_DOC
#define LL_SET_SHADER_LABEL(shader) shader.setLabel(#shader)
#else
#define LL_SET_SHADER_LABEL(shader, label)
#endif

#endif
