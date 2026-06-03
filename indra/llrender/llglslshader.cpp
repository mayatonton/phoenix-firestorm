/**
 * @file llglslshader.cpp
 * @brief GLSL helper functions and state.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "llglslshader.h"

#include "llshadermgr.h"
#include "llfile.h"
#include "llrender.h"
#include "llvertexbuffer.h"
#include "llrendertarget.h"

#include "hbxxh.h"
#include "llsdserialize.h"
#include "lldir.h"

#if LL_DARWIN
#include "OpenGL/OpenGL.h"
#endif

// r41 sub-step 4.3-γ'-port-β-2: per-program SPIR-V hook で VkShaderModule 生成 +
// Vulkan 初期化確認 + glslang 多段 stage TProgram link 経路。
#include "llvkloader.h"
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <sstream>

// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-16 D: kill-switch / dump 用 LLCachedControl
// 経由で gSavedSettings 参照 (llfontregistry.cpp と同型 pattern)。
#include "llcontrol.h"

extern LLControlGroup gSavedSettings;

 // Print-print list of shader included source files that are linked together via glAttachShader()
 // i.e. On macOS / OSX the AMD GLSL linker will display an error if a varying is left in an undefined state.
#define DEBUG_SHADER_INCLUDES 0

// Lots of STL stuff in here, using namespace std to keep things more readable
using std::vector;
using std::pair;
using std::make_pair;
using std::string;

GLuint LLGLSLShader::sCurBoundShader = 0;
LLGLSLShader* LLGLSLShader::sCurBoundShaderPtr = NULL;
S32 LLGLSLShader::sIndexedTextureChannels = 0;
U32 LLGLSLShader::sMaxGLTFMaterials = 0;
U32 LLGLSLShader::sMaxGLTFNodes = 0;
bool LLGLSLShader::sProfileEnabled = false;
bool LLGLSLShader::sCanProfile = true;
std::set<LLGLSLShader*> LLGLSLShader::sInstances;
LLGLSLShader::defines_map_t LLGLSLShader::sGlobalDefines;
U64 LLGLSLShader::sTotalTimeElapsed = 0;
U32 LLGLSLShader::sTotalTrianglesDrawn = 0;
U64 LLGLSLShader::sTotalSamplesDrawn = 0;
U32 LLGLSLShader::sTotalBinds = 0;
boost::json::value LLGLSLShader::sDefaultStats;

//UI shader -- declared here so llui_libtest will link properly
LLGLSLShader    gUIProgram;
LLGLSLShader    gSolidColorProgram;

// NOTE: Keep gShaderConsts* and LLGLSLShader::ShaderConsts_e in sync!
const std::string gShaderConstsKey[LLGLSLShader::NUM_SHADER_CONSTS] =
{
      "LL_SHADER_CONST_CLOUD_MOON_DEPTH"
    , "LL_SHADER_CONST_STAR_DEPTH"
};

// NOTE: Keep gShaderConsts* and LLGLSLShader::ShaderConsts_e in sync!
const std::string gShaderConstsVal[LLGLSLShader::NUM_SHADER_CONSTS] =
{
      "0.99998" // SHADER_CONST_CLOUD_MOON_DEPTH // SL-14113
    , "0.99999" // SHADER_CONST_STAR_DEPTH       // SL-14113
};


bool shouldChange(const LLVector4& v1, const LLVector4& v2)
{
    return v1 != v2;
}

//===============================
// LLGLSL Shader implementation
//===============================

//static
void LLGLSLShader::initProfile()
{
    sProfileEnabled = true;
    sTotalTimeElapsed = 0;
    sTotalTrianglesDrawn = 0;
    sTotalSamplesDrawn = 0;
    sTotalBinds = 0;

    for (auto ptr : sInstances)
    {
        ptr->clearStats();
    }
}


struct LLGLSLShaderCompareTimeElapsed
{
    bool operator()(const LLGLSLShader* const& lhs, const LLGLSLShader* const& rhs)
    {
        return lhs->mTimeElapsed < rhs->mTimeElapsed;
    }
};

//static
void LLGLSLShader::finishProfile(boost::json::value& statsv)
{
    sProfileEnabled = false;

    if (! statsv.is_null())
    {
        std::vector<LLGLSLShader*> sorted(sInstances.begin(), sInstances.end());
        std::sort(sorted.begin(), sorted.end(), LLGLSLShaderCompareTimeElapsed());

        auto& stats = statsv.as_object();
        auto shadersit = stats.emplace("shaders", boost::json::array_kind).first;
        auto& shaders = shadersit->value().as_array();
        bool unbound = false;
        for (auto ptr : sorted)
        {
            if (ptr->mBinds == 0)
            {
                unbound = true;
            }
            else
            {
                auto& shaderit = shaders.emplace_back(boost::json::object_kind);
                ptr->dumpStats(shaderit.as_object());
            }
        }

        constexpr float mega = 1'000'000.f;
        float totalTimeMs = sTotalTimeElapsed / mega;
        LL_INFOS() << "-----------------------------------" << LL_ENDL;
        LL_INFOS() << "Total rendering time: " << llformat("%.4f ms", totalTimeMs) << LL_ENDL;
        LL_INFOS() << "Total samples drawn: " << llformat("%.4f million", sTotalSamplesDrawn / mega) << LL_ENDL;
        LL_INFOS() << "Total triangles drawn: " << llformat("%.3f million", sTotalTrianglesDrawn / mega) << LL_ENDL;
        LL_INFOS() << "-----------------------------------" << LL_ENDL;
        auto totalsit = stats.emplace("totals", boost::json::object_kind).first;
        auto& totals = totalsit->value().as_object();
        totals.emplace("time", totalTimeMs / 1000.0);
        totals.emplace("binds", sTotalBinds);
        totals.emplace("samples", sTotalSamplesDrawn);
        totals.emplace("triangles", sTotalTrianglesDrawn);

        auto unusedit = stats.emplace("unused", boost::json::array_kind).first;
        auto& unused = unusedit->value().as_array();
        if (unbound)
        {
            LL_INFOS() << "The following shaders were unused: " << LL_ENDL;
            for (auto ptr : sorted)
            {
                if (ptr->mBinds == 0)
                {
                    LL_INFOS() << ptr->mName << LL_ENDL;
                    unused.emplace_back(ptr->mName);
                }
            }
        }
    }
}

void LLGLSLShader::clearStats()
{
    mTrianglesDrawn = 0;
    mTimeElapsed = 0;
    mSamplesDrawn = 0;
    mBinds = 0;
}

void LLGLSLShader::dumpStats(boost::json::object& stats)
{
    stats.emplace("name", mName);
    auto filesit = stats.emplace("files", boost::json::array_kind).first;
    auto& files = filesit->value().as_array();
    LL_INFOS() << "=============================================" << LL_ENDL;
    LL_INFOS() << mName << LL_ENDL;
    for (U32 i = 0; i < mShaderFiles.size(); ++i)
    {
        LL_INFOS() << mShaderFiles[i].first << LL_ENDL;
        files.emplace_back(mShaderFiles[i].first);
    }
    LL_INFOS() << "=============================================" << LL_ENDL;

    constexpr float  mega = 1'000'000.f;
    constexpr double giga = 1'000'000'000.0;
    F32 ms = mTimeElapsed / mega;
    F32 seconds = ms / 1000.f;

    F32 pct_tris = (F32)mTrianglesDrawn / (F32)sTotalTrianglesDrawn * 100.f;
    F32 tris_sec = (F32)(mTrianglesDrawn / mega);
    tris_sec /= seconds;

    F32 pct_samples = (F32)((F64)mSamplesDrawn / (F64)sTotalSamplesDrawn) * 100.f;
    F32 samples_sec = (F32)(mSamplesDrawn / giga);
    samples_sec /= seconds;

    F32 pct_binds = (F32)mBinds / (F32)sTotalBinds * 100.f;

    LL_INFOS() << "Triangles Drawn: " << mTrianglesDrawn << " " << llformat("(%.2f pct of total, %.3f million/sec)", pct_tris, tris_sec) << LL_ENDL;
    LL_INFOS() << "Binds: " << mBinds << " " << llformat("(%.2f pct of total)", pct_binds) << LL_ENDL;
    LL_INFOS() << "SamplesDrawn: " << mSamplesDrawn << " " << llformat("(%.2f pct of total, %.3f billion/sec)", pct_samples, samples_sec) << LL_ENDL;
    LL_INFOS() << "Time Elapsed: " << mTimeElapsed << " " << llformat("(%.2f pct of total, %.5f ms)\n", (F32)((F64)mTimeElapsed / (F64)sTotalTimeElapsed) * 100.f, ms) << LL_ENDL;
    stats.emplace("time", seconds);
    stats.emplace("binds", mBinds);
    stats.emplace("samples", mSamplesDrawn);
    stats.emplace("triangles", mTrianglesDrawn);
}

//static
void LLGLSLShader::startProfile()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    if (sProfileEnabled && sCurBoundShaderPtr)
    {
        sCurBoundShaderPtr->placeProfileQuery();
    }

}

//static
void LLGLSLShader::stopProfile()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (sProfileEnabled && sCurBoundShaderPtr)
    {
        sCurBoundShaderPtr->unbind();
    }
}

void LLGLSLShader::placeProfileQuery(bool for_runtime)
{
    if (sProfileEnabled || for_runtime)
    {
        if (mTimerQuery == 0)
        {
            glGenQueries(1, &mSamplesQuery);
            glGenQueries(1, &mTimerQuery);
            glGenQueries(1, &mPrimitivesQuery);
        }

        glBeginQuery(GL_TIME_ELAPSED, mTimerQuery);

        if (!for_runtime)
        {
            glBeginQuery(GL_SAMPLES_PASSED, mSamplesQuery);
            glBeginQuery(GL_PRIMITIVES_GENERATED, mPrimitivesQuery);
        }
    }
}

bool LLGLSLShader::readProfileQuery(bool for_runtime, bool force_read)
{
    if ((sProfileEnabled || for_runtime) && sCanProfile)
    {
        if (!mProfilePending)
        {
            glEndQuery(GL_TIME_ELAPSED);
            if (!for_runtime)
            {
                glEndQuery(GL_SAMPLES_PASSED);
                glEndQuery(GL_PRIMITIVES_GENERATED);
            }
            mProfilePending = for_runtime;
        }

        if (mProfilePending && for_runtime && !force_read)
        {
            GLuint64 result = 0;
            glGetQueryObjectui64v(mTimerQuery, GL_QUERY_RESULT_AVAILABLE, &result);

            if (result != GL_TRUE)
            {
                return false;
            }
        }

        GLuint64 time_elapsed = 0;
        glGetQueryObjectui64v(mTimerQuery, GL_QUERY_RESULT, &time_elapsed);
        mTimeElapsed += time_elapsed;
        mProfilePending = false;

        if (!for_runtime)
        {
            GLuint64 samples_passed = 0;
            glGetQueryObjectui64v(mSamplesQuery, GL_QUERY_RESULT, &samples_passed);

            GLuint64 primitives_generated = 0;
            glGetQueryObjectui64v(mPrimitivesQuery, GL_QUERY_RESULT, &primitives_generated);
            sTotalTimeElapsed += time_elapsed;

            sTotalSamplesDrawn += samples_passed;
            mSamplesDrawn += samples_passed;

            U32 tri_count = (U32)primitives_generated / 3;

            mTrianglesDrawn += tri_count;
            sTotalTrianglesDrawn += tri_count;

            sTotalBinds++;
            mBinds++;
        }
    }

    return true;
}



LLGLSLShader::LLGLSLShader()
    : mProgramObject(0),
    mAttributeMask(0),
    mTotalUniformSize(0),
    mActiveTextureChannels(0),
    mShaderLevel(0),
    mShaderGroup(SG_DEFAULT),
    mFeatures(),
    mUniformsDirty(false),
    mTimerQuery(0),
    mSamplesQuery(0),
    mPrimitivesQuery(0)
{

}

LLGLSLShader::~LLGLSLShader()
{
}

void LLGLSLShader::unload()
{
    mShaderFiles.clear();
    mDefines.clear();
    mFeatures = LLShaderFeatures();

    unloadInternal();
}

void LLGLSLShader::unloadInternal()
{
    sInstances.erase(this);

    stop_glerror();
    mAttribute.clear();
    mTexture.clear();
    mUniform.clear();

    if (mProgramObject)
    {
        GLuint obj[1024];
        GLsizei count = 0;
        glGetAttachedShaders(mProgramObject, 1024, &count, obj);

        for (GLsizei i = 0; i < count; i++)
        {
            glDetachShader(mProgramObject, obj[i]);
        }

        for (GLsizei i = 0; i < count; i++)
        {
            if (glIsShader(obj[i]))
            {
                glDeleteShader(obj[i]);
            }
        }

        glDeleteProgram(mProgramObject);

        mProgramObject = 0;
    }

    if (mTimerQuery)
    {
        glDeleteQueries(1, &mTimerQuery);
        mTimerQuery = 0;
    }

    if (mSamplesQuery)
    {
        glDeleteQueries(1, &mSamplesQuery);
        mSamplesQuery = 0;
    }

    //hack to make apple not complain
    glGetError();

    stop_glerror();
}

bool LLGLSLShader::createShader()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    unloadInternal();

    sInstances.insert(this);

    //reloading, reset matrix hash values
    for (U32 i = 0; i < LLRender::NUM_MATRIX_MODES; ++i)
    {
        mMatHash[i] = 0xFFFFFFFF;
    }
    mLightHash = 0xFFFFFFFF;

    llassert_always(!mShaderFiles.empty());

#if LL_DARWIN
    if(!gGLManager.mIsApple)
    {
        // work-around missing mix(vec3,vec3,bvec3)
        mDefines["OLD_SELECT"] = "1";
    }
#endif

    mShaderHash = hash();

    // Create program
    mProgramObject = glCreateProgram();
    if (mProgramObject == 0)
    {
        // Shouldn't happen if shader related extensions, like ARB_vertex_shader, exist.
        LL_SHADER_LOADING_WARNS() << "Failed to create handle for shader: " << mName << LL_ENDL;
        unloadInternal();
        return false;
    }

    bool success = true;

    mUsingBinaryProgram =  LLShaderMgr::instance()->loadCachedProgramBinary(this);

    if (!mUsingBinaryProgram)
    {
#if DEBUG_SHADER_INCLUDES
        fprintf(stderr, "--- %s ---\n", mName.c_str());
#endif // DEBUG_SHADER_INCLUDES

        // r41 sub-step 4.3-γ'-port-β-2 (handoff-substep-4-3-gamma-prime-port-beta-2-prep.md
        // §2 axis (a)): per-program SPIR-V hook のため stage 単位 source 蓄積を準備。
        // GL path / Vulkan 未初期化時は collect_for_vulkan = false で loadShaderFile()
        // への out_sources 渡し不要 → mStageSources untouched。
        mStageSources.clear();
        const bool collect_for_vulkan = LLVKLoader::isVulkanInitialized();

        //compile new source
        vector< pair<string, GLenum> >::iterator fileIter = mShaderFiles.begin();
        for (; fileIter != mShaderFiles.end(); fileIter++)
        {
            std::vector<std::string> stage_sources;
            GLuint shaderhandle = LLShaderMgr::instance()->loadShaderFile((*fileIter).first, mShaderLevel, (*fileIter).second, &mDefines, mFeatures.mIndexedTextureChannels, collect_for_vulkan ? &stage_sources : nullptr);
            LL_DEBUGS("ShaderLoading") << "SHADER FILE: " << (*fileIter).first << " mShaderLevel=" << mShaderLevel << LL_ENDL;
            if (shaderhandle)
            {
                attachObject(shaderhandle);
                if (collect_for_vulkan && !stage_sources.empty())
                {
                    mStageSources.push_back({ (*fileIter).second, (*fileIter).first, std::move(stage_sources) });
                }
            }
            else
            {
                success = false;
            }
        }

        // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B2-γ: SPIR-V hook を attachShaderFeatures()
        // の後ろに移動。β-2-β までは loadShaderFile loop 直後で生成していたが、その時点
        // では attachShaderFeatures() が未実行のため per-program utility 集合
        // (mVulkanAttached{Vertex,Fragment}Utilities) が空で、SPIR-V concat に utility
        // shader (deferred/globalF.glsl 等) の source が乗らず "No function definition"
        // 206 件 link 失敗の構造原因となっていた。本 reorder で attach 完了後の utility
        // list を generatePerProgramSPIRV() に渡せる (実体 call は下記 attachShaderFeatures()
        // 直後)。memory 緩和の mStageSources.clear() / shrink_to_fit() も生成後に移動。
    }

    // Attach existing objects
    if (!LLShaderMgr::instance()->attachShaderFeatures(this))
    {
        unloadInternal();
        return false;
    }

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B2-γ: per-program SPIR-V 生成は
    // attachShaderFeatures() による mVulkanAttached{Vertex,Fragment}Utilities 充填後に実行。
    // 失敗時 WARN 出力のみで GL path 続行 (charter §3 #1 acceptance、GL 既 attach 済 handle
    // は破棄しない)。生成後 mStageSources + utility list を clear で memory 緩和。
    if (!mUsingBinaryProgram && success && LLVKLoader::isVulkanInitialized() && !mStageSources.empty())
    {
        generatePerProgramSPIRV(mStageSources);
    }
    mStageSources.clear();
    mStageSources.shrink_to_fit();
    mVulkanAttachedVertexUtilities.clear();
    mVulkanAttachedVertexUtilities.shrink_to_fit();
    mVulkanAttachedFragmentUtilities.clear();
    mVulkanAttachedFragmentUtilities.shrink_to_fit();
    // Map attributes and uniforms
    if (success)
    {
        success = mapAttributes();
    }
    if (success)
    {
        success = mapUniforms();
    }
    if (!success)
    {
        LL_SHADER_LOADING_WARNS() << "Failed to link shader: " << mName << LL_ENDL;

        // Try again using a lower shader level;
        if (mShaderLevel > 0)
        {
            LL_SHADER_LOADING_WARNS() << "Failed to link using shader level " << mShaderLevel << " trying again using shader level " << (mShaderLevel - 1) << LL_ENDL;
            mShaderLevel--;
            return createShader();
        }
        else
        {
            // Give up and unload shader.
            unloadInternal();
        }
    }
    else if (mFeatures.mIndexedTextureChannels > 0)
    { //override texture channels for indexed texture rendering
        llassert(mFeatures.mIndexedTextureChannels == LLGLSLShader::sIndexedTextureChannels); // these numbers must always match
        bind();
        S32 channel_count = mFeatures.mIndexedTextureChannels;

        for (S32 i = 0; i < channel_count; i++)
        {
            LLStaticHashedString uniName(llformat("tex%d", i));
            uniform1i(uniName, i);
        }

        //adjust any texture channels that might have been overwritten
        for (U32 i = 0; i < mTexture.size(); i++)
        {
            if (mTexture[i] > -1)
            {
                S32 new_tex = mTexture[i] + channel_count;
                uniform1i(i, new_tex);
                mTexture[i] = new_tex;
            }
        }

        // get the true number of active texture channels
        mActiveTextureChannels = channel_count;
        for (auto& tex : mTexture)
        {
            mActiveTextureChannels = llmax(mActiveTextureChannels, tex + 1);
        }

        // when indexed texture channels are used, enforce an upper limit of 16
        // this should act as a canary in the coal mine for adding textures
        // and breaking machines that are limited to 16 texture channels
        llassert(mActiveTextureChannels <= 16);
        unbind();
    }

    LL_DEBUGS("GLSLTextureChannels") << mName << " has " << mActiveTextureChannels << " active texture channels" << LL_ENDL;

    for (U32 i = 0; i < mTexture.size(); i++)
    {
        if (mTexture[i] > -1)
        {
            LL_DEBUGS("GLSLTextureChannels") << "Texture " << LLShaderMgr::instance()->mReservedUniforms[i] << " assigned to channel " << mTexture[i] << LL_ENDL;
        }
    }

#if LL_PROFILER_ENABLE_RENDER_DOC
    setLabel(mName.c_str());
#endif

    return success;
}

// ------------------------------------------------------------------
// r41 sub-step 4.3-γ'-port-β-2 (handoff-substep-4-3-gamma-prime-port-beta-2-prep.md
// §2 axis (a)): per-program SPIR-V 生成 hook 本体。createShader() 内 loadShaderFile()
// loop 完遂後、mapAttributes() 直前に呼出される。
//
// 入力: mStageSources (全 stage の type + file_name + preprocessing 後 source 配列)
// 出力: LLShaderMgr::mVk{Vertex,Fragment}ShaderModules に file_name キーで VkShaderModule 格納
// cache: ~/.ayastorm_x64/cache/shader_cache/<program_hash>_program.spv (custom container)
//
// container layout: [u32 stage_count][repeat: u32 type, u32 spv_word_count, spv_words...]
// hash: HBXXH128(全 stage file_name + 全 source 連結) で program 単位確定
//
// β-1 per-file model で発生していた architectural mismatch (forward decl 経由の
// passTextureIndex / mirrorClip / encodeNormal / getObjectSkinnedTransform link fail)
// を、全 stage を単一 glslang::TProgram に addShader → link で解消。SPIR-V binary は
// 各 stage 個別 GlslangToSpv で抽出 (SPIR-V は stage 単位 module = Vulkan spec)。
//
// 失敗時: WARN 出力 + false return (caller はそのまま GL path 続行、charter §3 #1
// acceptance = GL/Vulkan 並走)。
// ------------------------------------------------------------------
namespace {
    // r41 sub-step 4.3-γ'-port-β-2: glslang process-global init を function-local static で
    // 単発化。LLShaderMgr::createSPIRVFromGLSL 内にも独自 init guard が存在するが、β-2 で
    // 既存 per-file hook 削除に伴い唯一の SPIR-V 生成 path は generatePerProgramSPIRV()。
    void ensureGlslangInitialized()
    {
        static const bool s_initialized = []() {
            glslang::InitializeProcess();
            return true;
        }();
        (void)s_initialized;
    }

    EShLanguage glToGlslangStage(GLenum type)
    {
        switch (type)
        {
            case GL_VERTEX_SHADER:   return EShLangVertex;
            case GL_FRAGMENT_SHADER: return EShLangFragment;
            case GL_GEOMETRY_SHADER: return EShLangGeometry;
            default: return EShLangCount;
        }
    }

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-16 D Phase 1 / η-18 Phase 2 拡張:
    // Vulkan SPIR-V path で bare `out/in <type> <ident>;` を `layout(location=N) ...` に
    // 書換える runtime transformer。
    //   - Phase 1 (η-16):    fragment stage の output (`out`) のみ対象
    //   - Phase 2 (η-18):    vertex stage の output + fragment stage の input を pair で対象、
    //                        V out で割当てた slot を同 ident F in に再利用 (V↔F pair allocator)
    //   - Phase 3 (η-19+):   vertex attribute (V stage の bare `in`) + geometry stage 対応予定
    // 設計根拠は handoff doc §4 (η-16 prep-D-switch) + §10.7 (η-17 complete) の pair allocator 仕様。
    // 範式名: 「C++ runtime location emit 範式 + V↔F pair 範式」
    //         (η-8 §3.3 mIndexedTextureChannels Vulkan-aware 範式 + η-9 §3.1 V/F pair canonical
    //         partner 同定範式 + η-17 §3.2 V↔F pair location 同期変更範式 の自然延長)。
    struct LocationAllocator
    {
        // Phase 1 (η-16): F stage out
        std::set<int> mUsedFragOutSlots;
        int           mFragOutCursor = 0;

        // Phase 2 (η-18): V↔F pair
        //   mUsedVertOutSlots    = V stage out で既に使用済の location (手動 wrap 由来 + 本 transformer 由来)
        //   mUsedFragInSlots     = F stage in  で既に使用済の location (手動 wrap 由来、collision 回避用)
        //   mVertOutCursor       = V stage out の次空き slot 探索 cursor
        //   mVertOutIdentToSlot  = V stage out 識別子 → 割当 slot mapping (F stage 同 ident in で再利用)
        std::set<int>              mUsedVertOutSlots;
        std::set<int>              mUsedFragInSlots;
        int                        mVertOutCursor = 0;
        std::map<std::string, int> mVertOutIdentToSlot;
    };

    std::string vulkanizeStageSource(
        const std::string& source,
        GLenum             stage_type,
        LocationAllocator& alloc)
    {
        // Phase 2 (η-18): V stage / F stage の両方を対象 (geometry / compute は touch しない)。
        if (stage_type != GL_FRAGMENT_SHADER && stage_type != GL_VERTEX_SHADER)
        {
            return source;
        }

        // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-18 Phase 1-1 拡張:
        // GLSL では interpolation/precision qualifier (smooth/flat/highp 等) が storage qualifier
        // (out/in) の **前後どちらにも書ける** (spec 4.60 §4.3)。η-16 originale は `out\s+(quals?)\s+`
        // = quals が out の後にしか書けない設計で、η-18 V varying では `smooth out vec3 vary_normal;`
        // が dominant で全数 match miss していた = link failed 4 件 / overlapping 5 件 / qualifier
        // 12 件の真因。Phase 1-1 で quals_before / quals_after 両対応に re-design。
        static const std::string kQuals =
            R"((?:(?:flat|smooth|noperspective|centroid|highp|mediump|lowp|invariant)\s+)*)";

        // bare `<quals?> out <quals?> <type> <ident> (\[...\])? ;` 1 line match
        //   groups: 1=leading ws, 2=quals_before, 3=quals_after, 4=type, 5=ident, 6=array
        static const std::regex bare_out_pattern(
            R"(^(\s*)()" + kQuals + R"()out\s+()" + kQuals
            + R"()([a-zA-Z_][a-zA-Z0-9_]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)(\s*\[[^;]*\])?\s*;\s*$)"
        );
        // bare `<quals?> in <quals?> <type> <ident> (\[...\])? ;` 1 line match (F stage 用、V↔F pair)
        //   groups: 1=leading ws, 2=quals_before, 3=quals_after, 4=type, 5=ident, 6=array
        static const std::regex bare_in_pattern(
            R"(^(\s*)()" + kQuals + R"()in\s+()" + kQuals
            + R"()([a-zA-Z_][a-zA-Z0-9_]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)(\s*\[[^;]*\])?\s*;\s*$)"
        );
        // 既存 `layout(location=N) <quals?> out|in ...` を pre-pass で audit (location N のみ捕捉)
        static const std::regex existing_layout_out_pattern(
            R"(^\s*layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*)" + kQuals + R"(out\b)"
        );
        static const std::regex existing_layout_in_pattern(
            R"(^\s*layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*)" + kQuals + R"(in\b)"
        );
        // V stage 既存 `layout(location=N) <quals?> out <quals?> <type> <ident>;` を pre-pass で
        // ident → slot 記録 (手動 wrap 済の V out を F in 側の transformer が同 slot で再利用するため)
        //   groups: 1=location, 2=ident
        static const std::regex existing_layout_out_with_ident_pattern(
            R"(^\s*layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*)"
            + kQuals + R"(out\s+)" + kQuals
            + R"([a-zA-Z_][a-zA-Z0-9_]*\s+([a-zA-Z_][a-zA-Z0-9_]*)(?:\s*\[[^;]*\])?\s*;)"
        );
        // F stage 既存 `layout(location=N) <quals?> in <quals?> <type> <ident>;` を main pass で
        // 検出 → V mapping table と照合 → mismatch なら override 書換 (V↔F pair alignment 強制保証)
        //   groups: 1=ws, 2=loc, 3=quals_before, 4=quals_after, 5=type, 6=ident, 7=array
        static const std::regex existing_layout_in_with_ident_pattern(
            R"(^(\s*)layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*()"
            + kQuals + R"()in\s+()" + kQuals
            + R"()([a-zA-Z_][a-zA-Z0-9_]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)(\s*\[[^;]*\])?\s*;\s*$)"
        );

        // pre-pass: 当該 stage の既存 layout 由来 location を audit + V stage では ident → slot mapping
        //          を mVertOutIdentToSlot に記録 (手動 wrap 済 varying を F stage 同 ident in で再利用)
        {
            std::istringstream pre(source);
            std::string preline;
            while (std::getline(pre, preline))
            {
                std::smatch m;
                if (stage_type == GL_VERTEX_SHADER)
                {
                    if (std::regex_search(preline, m, existing_layout_out_with_ident_pattern))
                    {
                        try
                        {
                            // groups: 1=location, 2=ident (kQuals は non-capturing `(?:...)` で
                            // 追加 group を作らないため、ident は m[2] が正)
                            int slot = std::stoi(m[1].str());
                            alloc.mUsedVertOutSlots.insert(slot);
                            const std::string ident = m[2].str();
                            if (!ident.empty())
                            {
                                alloc.mVertOutIdentToSlot[ident] = slot;
                            }
                        }
                        catch (...) { /* malformed: skip */ }
                    }
                    else if (std::regex_search(preline, m, existing_layout_out_pattern))
                    {
                        try { alloc.mUsedVertOutSlots.insert(std::stoi(m[1].str())); }
                        catch (...) { /* malformed: skip */ }
                    }
                }
                else // GL_FRAGMENT_SHADER
                {
                    if (std::regex_search(preline, m, existing_layout_out_pattern))
                    {
                        try { alloc.mUsedFragOutSlots.insert(std::stoi(m[1].str())); }
                        catch (...) { /* malformed: skip */ }
                    }
                    else if (std::regex_search(preline, m, existing_layout_in_pattern))
                    {
                        try { alloc.mUsedFragInSlots.insert(std::stoi(m[1].str())); }
                        catch (...) { /* malformed: skip */ }
                    }
                }
            }
        }

        std::ostringstream out;
        std::istringstream in(source);
        std::string line;
        while (std::getline(in, line))
        {
            // line-comment trim (GLSL に文字列 literal 無し、block comment は state machine 無しの簡易処理: 行内 // のみ)
            std::string trimmed = line;
            size_t slash = trimmed.find("//");
            if (slash != std::string::npos)
            {
                trimmed.resize(slash);
            }

            std::smatch m;

            // F stage で 既存 `layout(location=N) <quals?> in <quals?> <type> <ident>;` を捕捉 →
            // V mapping table と照合 → mismatch なら V slot で override 書換 (V↔F pair 強制保証)。
            // skip 条件 ("layout" 含むため通常 skip) より先に判定する必要があるため、ここに置く。
            if (stage_type == GL_FRAGMENT_SHADER
                && std::regex_match(trimmed, m, existing_layout_in_with_ident_pattern))
            {
                const std::string  leading_ws    = m[1].str();
                const std::string  existing_loc  = m[2].str();
                const std::string  quals_before  = m[3].str();
                const std::string  quals_after   = m[4].str();
                const std::string  type          = m[5].str();
                const std::string  ident         = m[6].str();
                const std::string  array         = m[7].matched ? m[7].str() : "";

                auto it = alloc.mVertOutIdentToSlot.find(ident);
                if (it != alloc.mVertOutIdentToSlot.end())
                {
                    int v_slot = it->second;
                    int f_slot = -1;
                    try { f_slot = std::stoi(existing_loc); } catch (...) {}

                    if (f_slot != v_slot)
                    {
                        // mismatch 検出: V↔F pair alignment 違反 = link failed の真因
                        // V slot で override (F stage の手動 wrap location を上書き)
                        alloc.mUsedFragInSlots.insert(v_slot);
                        out << leading_ws << "layout(location=" << v_slot << ") "
                            << quals_before << "in " << quals_after
                            << type << " " << ident << array << ";\n";
                        LL_DEBUGS("Vulkan") << "vulkanizeStageSource: F stage layout in '" << ident
                                            << "' location override " << f_slot << " -> " << v_slot
                                            << " (V↔F pair alignment)" << LL_ENDL;
                        continue;
                    }
                    // match している場合は touch しない (pre-pass で既に mUsedFragInSlots 登録済)
                }
                // V mapping table に不在 = F 独自 in (touch しない)
                out << line << '\n';
                continue;
            }

            // skip: 既 layout / 関数宣言 (`(` 含む) / uniform 行
            if (trimmed.find("layout") != std::string::npos
                || trimmed.find('(') != std::string::npos
                || trimmed.find("uniform") != std::string::npos)
            {
                out << line << '\n';
                continue;
            }

            // bare `out` 検出: stage により処理を分岐
            if (std::regex_match(trimmed, m, bare_out_pattern))
            {
                const std::string  leading_ws   = m[1].str();
                const std::string  quals_before = m[2].str();
                const std::string  quals_after  = m[3].str();
                const std::string  type         = m[4].str();
                const std::string  ident        = m[5].str();
                const std::string  array        = m[6].matched ? m[6].str() : "";

                if (stage_type == GL_FRAGMENT_SHADER)
                {
                    // F stage の bare out: η-16 既存 logic (fragment color attachment slot 割当)
                    while (alloc.mUsedFragOutSlots.count(alloc.mFragOutCursor))
                    {
                        ++alloc.mFragOutCursor;
                    }
                    int slot = alloc.mFragOutCursor++;
                    alloc.mUsedFragOutSlots.insert(slot);

                    out << leading_ws << "layout(location=" << slot << ") "
                        << quals_before << "out " << quals_after
                        << type << " " << ident << array << ";\n";
                }
                else // GL_VERTEX_SHADER
                {
                    // V stage の bare out (varying): slot 割当 + ident → slot mapping 記録
                    // 同 program の F stage で同 ident in を transformer が同 slot で wrap
                    //
                    // Phase 1-2 (η-18): pre-pass で記録済 (manual layout wrap 由来) の slot は
                    // 上書きせず再利用。preprocessor 別分岐に bare 版と manual wrap 版が
                    // 共存する case (e.g. Underwater Shader: vary_AdditiveColor は #if 分岐 A で
                    // manual layout(location=20)、分岐 B で bare) で、bare 版を 0 で wrap して
                    // map を 0 に上書きしてしまうと live 分岐 (location=20) と mismatch、F 側で
                    // 0 へ override してしまい結果的に V live=20 ≠ F=0 で link failed が回帰する。
                    // manual wrap slot を canonical truth と扱い、bare 版も同 slot で wrap する。
                    int slot;
                    auto existing_it = alloc.mVertOutIdentToSlot.find(ident);
                    if (existing_it != alloc.mVertOutIdentToSlot.end())
                    {
                        slot = existing_it->second;
                    }
                    else
                    {
                        while (alloc.mUsedVertOutSlots.count(alloc.mVertOutCursor))
                        {
                            ++alloc.mVertOutCursor;
                        }
                        slot = alloc.mVertOutCursor++;
                        alloc.mUsedVertOutSlots.insert(slot);
                        alloc.mVertOutIdentToSlot[ident] = slot;
                    }

                    out << leading_ws << "layout(location=" << slot << ") "
                        << quals_before << "out " << quals_after
                        << type << " " << ident << array << ";\n";
                }
                continue;
            }

            // bare `in` 検出: F stage のみ V↔F pair として処理 (V stage の bare in = vertex attribute
            //                 は Phase 3 / η-19+ 移管、本 Phase では touch しない)
            if (stage_type == GL_FRAGMENT_SHADER && std::regex_match(trimmed, m, bare_in_pattern))
            {
                const std::string  leading_ws   = m[1].str();
                const std::string  quals_before = m[2].str();
                const std::string  quals_after  = m[3].str();
                const std::string  type         = m[4].str();
                const std::string  ident        = m[5].str();
                const std::string  array        = m[6].matched ? m[6].str() : "";

                auto it = alloc.mVertOutIdentToSlot.find(ident);
                if (it != alloc.mVertOutIdentToSlot.end())
                {
                    // V stage 同 ident out で割当てた slot を再利用 = V↔F pair location alignment 構造保証
                    int slot = it->second;
                    alloc.mUsedFragInSlots.insert(slot);

                    out << leading_ws << "layout(location=" << slot << ") "
                        << quals_before << "in " << quals_after
                        << type << " " << ident << array << ";\n";
                }
                else
                {
                    // V stage に同 ident 不在 = mapping miss (touch しない、observable に warn record)。
                    // 想定 case: F stage 独自 in (理論上 GLSL では V↔F pair 必須だが、preprocessor
                    //           分岐 / built-in 由来 derived 等で V 側未出現の case がある)。
                    LL_WARNS("Vulkan") << "vulkanizeStageSource: F stage bare in '" << ident
                                       << "' has no matching V stage out (slot allocation skipped, "
                                       << "may trigger SPIR-V missing location)" << LL_ENDL;
                    out << line << '\n';
                }
                continue;
            }

            out << line << '\n';
        }
        return out.str();
    }

    void dumpTransformedStageSource(
        const std::string& transformed,
        const LLUUID&      program_hash,
        GLenum             stage_type,
        const std::string& shader_cache_dir,
        const std::string& program_name)
    {
        if (shader_cache_dir.empty())
        {
            return;
        }
        std::string dump_dir = gDirUtilp->add(shader_cache_dir, "transformed");
        LLFile::mkdir(dump_dir);

        const char* stage_tag = "unknown";
        switch (stage_type)
        {
            case GL_VERTEX_SHADER:   stage_tag = "vert"; break;
            case GL_FRAGMENT_SHADER: stage_tag = "frag"; break;
            case GL_GEOMETRY_SHADER: stage_tag = "geom"; break;
            default: break;
        }
        std::string dump_path = gDirUtilp->add(
            dump_dir, program_hash.asString() + "_" + stage_tag + ".glsl");
        LLFILE* f = LLFile::fopen(dump_path, "wb");
        if (f)
        {
            fwrite(transformed.data(), 1, transformed.size(), f);
            fclose(f);
            LL_INFOS("Vulkan") << "generatePerProgramSPIRV: dumped transformed source for '"
                               << program_name << "' stage_tag=" << stage_tag
                               << " -> " << dump_path << LL_ENDL;
        }
        else
        {
            LL_WARNS("Vulkan") << "generatePerProgramSPIRV: failed to open dump path '"
                               << dump_path << "' for program '" << program_name << "'" << LL_ENDL;
        }
    }

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-3:
    // LLStaticHashedString 経路 uniform 名 build-time list (= S1-C 採用、spec 06a §4.3.1)。
    // mReservedUniforms 未登録 = integer index 経路非対応の uniform を、mapUniforms() Vulkan
    // path で lookup_runtime → hash 計算 → mUniformUBOLocByHash 登録 (= PB-3 block)。
    // setter 側 (= PB-5) は LLStaticHashedString::Hash() で同 hash 引きで O(1) lookup
    // (= 構築側 / setter 側で同 API 使用が hash 一致保証)。
    // 名前 list は 2026-06-04 indra/newview + indra/llrender 全 grep 抽出結果 (80 unique)。
    // 新規 uniform 追加時 = 本 list 追記必須 (silent skip risk、UBO 集約表登録時に同期更新)。
    const char* const g_static_hashed_uniform_names[] = {
        "SMAA_RT_METRICS", "NoiseTexture", "RenderTexture", "above_water", "alpha_scale",
        "ambiance", "aya_blur_dir", "aya_blur_radius", "aya_glow_color", "aya_glow_gain",
        "aya_strength", "aya_translucency_params", "aya_translucency_tint", "bloomStrength",
        "blurDirection", "blurWidth", "brightMult", "brightness", "bump_code", "camPosLocal",
        "cas_param_0", "cas_param_1", "clip_plane", "contrast", "contrastBase", "custom_alpha",
        "delta", "diffuse_luminance_scale", "direction", "dist_factor", "dither_scale",
        "dither_scale_s", "dither_scale_t", "dither_tex", "dt", "dynamic_exposure_enabled",
        "dynamic_exposure_params", "dynamic_exposure_params2", "exposure", "extractHigh",
        "extractLow", "glowMap", "hdri_split_screen", "kern", "kern_scale", "lumWeights",
        "maxRoughness", "maxZDepth", "mipLevel", "noiseStrength", "noiseVec", "norm_mat",
        "norm_scale", "object_id_packed", "offset", "out_screen_res", "probe_strength",
        "resScale", "roughness", "saturation", "screenMap", "screenRes", "sourceIdx",
        "ssao_irradiance_max", "ssao_irradiance_scale", "stepX", "stepY", "tex0", "tex1",
        "texelSize", "texture0", "texture1", "tint", "tolerance", "tonemap_mix",
        "tonemap_type", "u_width", "waterSign", "zfar", "znear",
    };
}

bool LLGLSLShader::generatePerProgramSPIRV(const std::vector<StageSource>& stages)
{
    if (stages.empty())
    {
        return false;
    }

    LLShaderMgr* mgr = LLShaderMgr::instance();

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-16 D: transformer kill-switch / dump 機構
    // (handoff §4.5 / §4.6)。LLCachedControl は static 化で gSavedSettings の lookup を 1 回
    // に抑える。disable で transformer skip、η-15 末まで baseline 動作 (charter §3 #1 acceptance)。
    static LLCachedControl<bool> sVulkanShaderAutoLocation(
        gSavedSettings, "RenderVulkanShaderAutoLocation", true);
    static LLCachedControl<bool> sVulkanShaderDumpTransformed(
        gSavedSettings, "RenderVulkanShaderDumpTransformed", false);
    const bool auto_location_enabled    = sVulkanShaderAutoLocation;
    const bool dump_transformed_enabled = sVulkanShaderDumpTransformed;

    // cache key = HBXXH128(transformer version tag + kill-switch state + 全 stage file_name +
    // 全 source 連結)。transformer 投入で transformed source が SPIR-V emit の真の入力に
    // なるため、kill-switch 切替時に旧 cache hit (stale binary) を構造防止する版数 tag を
    // 先頭に混ぜる。version tag bump は transformer 仕様変更時に必須:
    //   Phase 1 (η-16) = "v1_p1_fragout"     (fragment out のみ)
    //   Phase 2 (η-18) = "v2_p2_inout_pair"  (V↔F pair = V out + F in、本 sub-bundle で bump)
    //   Phase 3 (η-19+) = "v3_*"             (vertex attribute / geometry stage 対応時に bump 予定)
    HBXXH128 program_hash_obj;
    program_hash_obj.update(std::string("vulkanize:v5_p2_inout_pair_prepass_group_fix"));
    program_hash_obj.update(std::string(auto_location_enabled ? "auto_loc=1" : "auto_loc=0"));
    for (const auto& stage : stages)
    {
        program_hash_obj.update(stage.file_name);
        for (const auto& src : stage.sources)
        {
            program_hash_obj.update(src);
        }
    }
    LLUUID program_hash = program_hash_obj.digest();

    std::string cache_path;
    if (!mgr->mShaderCacheDir.empty())
    {
        cache_path = gDirUtilp->add(mgr->mShaderCacheDir,
                                    program_hash.asString() + "_program.spv");
    }

    // Vulkan SPIR-V は stage 単位 module = 1 stage type につき 1 SPIR-V。複数 file が同じ
    // stage type を持つ場合は同 stage 内で concat (std::map で sorted unique 化、cache
    // ordering 決定性を確保)。
    std::map<GLenum, std::vector<size_t>> stages_by_type;
    for (size_t i = 0; i < stages.size(); ++i)
    {
        stages_by_type[stages[i].type].push_back(i);
    }

    struct StageSpv { GLenum type; std::vector<unsigned int> spirv; };
    std::vector<StageSpv> stage_spvs;
    bool cache_hit = false;

    // cache hit: custom container parse [u32 stage_count][repeat: u32 type, u32 word_count, words...]
    if (!cache_path.empty())
    {
        LLFILE* cache_file = LLFile::fopen(cache_path, "rb");
        if (cache_file)
        {
            uint32_t stored_stage_count = 0;
            bool ok = (fread(&stored_stage_count, sizeof(uint32_t), 1, cache_file) == 1);
            if (ok && stored_stage_count == (uint32_t)stages_by_type.size())
            {
                stage_spvs.reserve(stored_stage_count);
                for (uint32_t i = 0; ok && i < stored_stage_count; ++i)
                {
                    uint32_t stage_type = 0;
                    uint32_t word_count = 0;
                    ok = (fread(&stage_type, sizeof(uint32_t), 1, cache_file) == 1 &&
                          fread(&word_count, sizeof(uint32_t), 1, cache_file) == 1);
                    if (ok && word_count > 0)
                    {
                        StageSpv ss;
                        ss.type = (GLenum)stage_type;
                        ss.spirv.resize(word_count);
                        size_t read_words = fread(ss.spirv.data(), sizeof(unsigned int),
                                                  word_count, cache_file);
                        ok = (read_words == word_count);
                        if (ok)
                        {
                            stage_spvs.push_back(std::move(ss));
                        }
                    }
                    else
                    {
                        ok = false;
                    }
                }
                cache_hit = ok && (stage_spvs.size() == stages_by_type.size());
            }
            fclose(cache_file);
            if (!cache_hit)
            {
                stage_spvs.clear();
            }
        }
    }

    // cache miss: per stage 内 source concat → TShader 構築 + parse → 全 stage TProgram
    // link → 各 stage GlslangToSpv で SPIR-V 抽出 (prep doc §2 axis (a) literal 設計)。
    // β-1 per-file model の architectural mismatch (forward decl link fail) は本 path で
    // 単一 TProgram に全 stage が addShader されることで構造的解消、ただし最終的な
    // parse/link 成立は bundle-A/B/C (217 file structural rewrite) 完遂後に >>0% 目標。
    if (!cache_hit)
    {
        ensureGlslangInitialized();

        std::vector<std::unique_ptr<glslang::TShader>> tshaders;
        std::vector<std::string> concat_buffers;
        std::vector<EShLanguage> stage_langs_in_order;
        std::vector<GLenum> stage_types_in_order;

        // Reserve to avoid string reallocation invalidating c_str() pointers passed to setStrings.
        concat_buffers.reserve(stages_by_type.size());

        // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-18 Phase 2:
        // V↔F pair allocator は program 単位で 1 instance、stage 跨いで shared state を持つ
        // (V stage out で割当てた slot を F stage 同 ident in で再利用)。
        // stages_by_type は std::map<GLenum, ...> で key sort 順だと F (0x8B30) → V (0x8B31)
        // → G (0x8DD9) の順となり、F が先に process されると V↔F pair mapping が空のため
        // 必ず V → G → F の固定順で iterate する (data flow と一致、G stage は in/out 両方
        // 持つが η-18 では transformer 対象外 = 既存 manual wrap audit のみ実施で influence なし)。
        LocationAllocator alloc;
        static const GLenum kFixedStageOrder[] = {
            GL_VERTEX_SHADER,
            GL_GEOMETRY_SHADER,
            GL_FRAGMENT_SHADER,
        };

        for (GLenum stage_type : kFixedStageOrder)
        {
            auto stage_it = stages_by_type.find(stage_type);
            if (stage_it == stages_by_type.end())
            {
                continue;
            }
            const std::vector<size_t>& stage_indices = stage_it->second;

            EShLanguage lang = glToGlslangStage(stage_type);
            if (lang == EShLangCount)
            {
                LL_WARNS("Vulkan") << "generatePerProgramSPIRV: unsupported stage type 0x"
                                   << std::hex << (S32)stage_type << std::dec
                                   << " (program " << mName << ")" << LL_ENDL;
                return false;
            }

            // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B3: Vulkan profile #version override.
            // 同 stage に複数 file が連結される構成 (例 SMAA VERTEX = SMAAEdgeDetectV.glsl +
            // SMAA.glsl(VERTEX)) で、各 file の sources[0] (= GL profile "#version XXX\n") を
            // そのまま append すると glslang が 2 回目以降を 'must occur first' で reject。
            // 加えて GL profile (#version 420 等) は Vulkan glslang で 'bad profile name' 扱い。
            // stage 先頭 1 回のみ Vulkan profile (#version 460 + GL_KHR_vulkan_glsl extension +
            // LL_VULKAN_GLSL macro) を出力し、各 file の sources[0] は skip (GL path
            // loadShaderFile の strdup には影響なし、collect_for_vulkan=false の GL compile
            // path 不変、charter §3 #1 acceptance)。
            std::string concatenated;
            concatenated.append("#version 460\n");
            concatenated.append("#extension GL_KHR_vulkan_glsl : enable\n");
            concatenated.append("#define LL_VULKAN_GLSL 1\n");

            // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B2-γ: utility (basic library) shader
            // source の prepend。attachShaderFeatures() が GL path で glAttachShader 連発
            // するのと同じ意味論を Vulkan path で再現するため、attach 順を保ったまま
            // LLShaderMgr の filename key cache から preprocessed source を引いて
            // program-specific mShaderFiles より前に append する (各 entry の sources[0]
            // = GL profile #version は skip)。β-1/β-2-α/β-2-β/β-3 で発生した
            // "No function definition for mirrorClip / encodeNormal /
            // getObjectSkinnedTransform / srgb_to_linear ..." 206 件の link 失敗の
            // 構造原因 = utility source が SPIR-V concat に乗っていなかったこと、これを解消。
            const std::vector<std::string>* utility_files = nullptr;
            const std::map<std::string, std::vector<std::string>>* source_cache = nullptr;
            if (stage_type == GL_VERTEX_SHADER)
            {
                utility_files = &mVulkanAttachedVertexUtilities;
                source_cache  = &mgr->mVertexShaderSourceCache;
            }
            else if (stage_type == GL_FRAGMENT_SHADER)
            {
                utility_files = &mVulkanAttachedFragmentUtilities;
                source_cache  = &mgr->mFragmentShaderSourceCache;
            }
            if (utility_files && source_cache)
            {
                for (const std::string& util_file : *utility_files)
                {
                    auto it = source_cache->find(util_file);
                    if (it == source_cache->end())
                    {
                        LL_WARNS("Vulkan") << "generatePerProgramSPIRV: utility source cache miss for '"
                                           << util_file << "' (stage type 0x"
                                           << std::hex << (S32)stage_type << std::dec
                                           << ", program " << mName << ")" << LL_ENDL;
                        continue;
                    }
                    const std::vector<std::string>& util_sources = it->second;
                    for (size_t i = 1; i < util_sources.size(); ++i)
                    {
                        concatenated.append(util_sources[i]);
                        // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-ε: 各 utility source entry
                        // 末尾の '\n' 担保。glslang は preprocessor directive ('#ifdef' 等) が
                        // 前行末 token と同行扱いになると "preprocessor directive cannot be
                        // preceded by another token" で reject する。utility 境界 (前 utility
                        // 末尾 + 次 utility 先頭) でこの状態が ALL 191 件 0:127 cascade として
                        // 現れたため、append 直後に末尾改行有無を検査して欠落時のみ "\n" を
                        // 補う (cache 内 entry 自体は不変、concat path のみ補正)。
                        if (!util_sources[i].empty() && util_sources[i].back() != '\n')
                        {
                            concatenated.append("\n");
                        }
                    }
                }
            }

            for (size_t idx : stage_indices)
            {
                const auto& stage = stages[idx];
                for (size_t i = 1; i < stage.sources.size(); ++i)
                {
                    concatenated.append(stage.sources[i]);
                }
            }

            if (concatenated.empty())
            {
                LL_WARNS("Vulkan") << "generatePerProgramSPIRV: empty source for stage type 0x"
                                   << std::hex << (S32)stage_type << std::dec
                                   << " (program " << mName << ")" << LL_ENDL;
                return false;
            }

            // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-16 D Phase 1 / η-18 Phase 2: bare
            // `out` / `in` runtime wrap (kill-switch ON 時のみ)。stage 全体に shared util +
            // program-specific source が連結済の concatenated buffer に対して line 単位で書換える。
            //   Phase 1 (η-16): F stage の bare `out` のみ
            //   Phase 2 (η-18): V stage の bare `out` + F stage の bare `in` の pair allocator
            //                    (V stage の bare `in` = vertex attribute は Phase 3 / η-19+ 移管)
            // alloc は program 単位 instance で stage 跨ぎ shared state を持つ。
            if (auto_location_enabled)
            {
                concatenated = vulkanizeStageSource(concatenated, stage_type, alloc);
            }

            // r41 D diagnostic dump (RenderVulkanShaderDumpTransformed=true 時のみ)。
            // shader_cache/transformed/<program_hash>_<stage_tag>.glsl に書き出す。η-15 §3.2
            // 範式 (検証完了後 default disable) に従い、sub-bundle 完遂で settings 戻す前提。
            if (dump_transformed_enabled)
            {
                dumpTransformedStageSource(
                    concatenated, program_hash, stage_type, mgr->mShaderCacheDir, mName);
            }

            concat_buffers.push_back(std::move(concatenated));

            auto shader = std::make_unique<glslang::TShader>(lang);
            const char* src_cstr = concat_buffers.back().c_str();
            shader->setStrings(&src_cstr, 1);
            shader->setEnvInput(glslang::EShSourceGlsl, lang, glslang::EShClientVulkan, 450);
            shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
            shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

            const TBuiltInResource* resources = GetDefaultResources();
            EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);

            if (!shader->parse(resources, 450, false, messages))
            {
                LL_WARNS("Vulkan") << "generatePerProgramSPIRV: glslang parse failed for stage type 0x"
                                   << std::hex << (S32)stage_type << std::dec
                                   << " (program " << mName << ")\n"
                                   << shader->getInfoLog() << LL_ENDL;
                return false;
            }

            tshaders.push_back(std::move(shader));
            stage_langs_in_order.push_back(lang);
            stage_types_in_order.push_back(stage_type);
        }

        // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-18 Phase 2 defensive observability:
        // kFixedStageOrder enumerate が stages_by_type 全 stage を網羅したか確認。SL viewer 現
        // 実装では V/F/G のみで mismatch 発生は理論的に 0、未知 stage type 混入時のみ literal
        // 観測可能化 (process は skip、SPIR-V generation 自体は stage 数 mismatch で下流の
        // link 段階が fail = clean abort)。
        if (tshaders.size() != stages_by_type.size())
        {
            for (const auto& kv : stages_by_type)
            {
                if (kv.first != GL_VERTEX_SHADER
                    && kv.first != GL_GEOMETRY_SHADER
                    && kv.first != GL_FRAGMENT_SHADER)
                {
                    LL_WARNS("Vulkan") << "generatePerProgramSPIRV: stage type 0x"
                                       << std::hex << (S32)kv.first << std::dec
                                       << " not in kFixedStageOrder (skipped, program "
                                       << mName << ")" << LL_ENDL;
                }
            }
        }

        // 全 stage TProgram link (prep doc §2 axis (a) literal: cross-stage interface 整合
        // 検証 + forward decl resolution)
        glslang::TProgram program;
        for (auto& ts : tshaders)
        {
            program.addShader(ts.get());
        }
        EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);
        if (!program.link(messages))
        {
            LL_WARNS("Vulkan") << "generatePerProgramSPIRV: glslang link failed for program "
                               << mName << "\n" << program.getInfoLog() << LL_ENDL;
            return false;
        }

        // 各 stage 個別 GlslangToSpv で SPIR-V 抽出 (SpvOptions は createSPIRVFromGLSL 継承)
        glslang::SpvOptions spv_options;
        spv_options.generateDebugInfo = false;
        spv_options.stripDebugInfo    = true;
        spv_options.disableOptimizer  = true;  // γ' 段階 = optimizer off (PoC 重視)
        spv_options.validate          = false;

        stage_spvs.reserve(stage_langs_in_order.size());
        for (size_t i = 0; i < stage_langs_in_order.size(); ++i)
        {
            StageSpv ss;
            ss.type = stage_types_in_order[i];
            glslang::GlslangToSpv(*program.getIntermediate(stage_langs_in_order[i]),
                                  ss.spirv, &spv_options);
            if (ss.spirv.empty())
            {
                LL_WARNS("Vulkan") << "generatePerProgramSPIRV: GlslangToSpv produced empty SPIR-V "
                                   << "for stage type 0x" << std::hex << (S32)ss.type << std::dec
                                   << " (program " << mName << ")" << LL_ENDL;
                return false;
            }
            stage_spvs.push_back(std::move(ss));
        }

        // Write cache: custom container
        if (!cache_path.empty())
        {
            LLFILE* write_file = LLFile::fopen(cache_path, "wb");
            if (write_file)
            {
                uint32_t stage_count = (uint32_t)stage_spvs.size();
                fwrite(&stage_count, sizeof(uint32_t), 1, write_file);
                for (const auto& ss : stage_spvs)
                {
                    uint32_t stage_type = (uint32_t)ss.type;
                    uint32_t word_count = (uint32_t)ss.spirv.size();
                    fwrite(&stage_type, sizeof(uint32_t), 1, write_file);
                    fwrite(&word_count, sizeof(uint32_t), 1, write_file);
                    fwrite(ss.spirv.data(), sizeof(unsigned int), word_count, write_file);
                }
                fclose(write_file);
            }
        }
    }

    // VkShaderModule 生成 + LLShaderMgr maps 格納。同 stage type の複数 file は同一 SPIR-V
    // を共有するが、vkCreateShaderModule は file_name 単位で個別 module ハンドル発行
    // (1:N module sharing による destroy 時 dangling 回避)。β-1 file_name キー lookup
    // 互換性維持、後段 β-3 PSO 構築で file_name キー lookup。
    for (const auto& stage : stages)
    {
        const StageSpv* ss_ptr = nullptr;
        for (const auto& ss : stage_spvs)
        {
            if (ss.type == stage.type) { ss_ptr = &ss; break; }
        }
        if (!ss_ptr) continue;

        VkShaderModule vk_module = LLVKLoader::loadSpirvShaderModuleFromMemory(ss_ptr->spirv);
        if (vk_module == VK_NULL_HANDLE) continue;

        if (stage.type == GL_VERTEX_SHADER)
        {
            mgr->mVkVertexShaderModules[stage.file_name] = vk_module;
        }
        else if (stage.type == GL_FRAGMENT_SHADER)
        {
            mgr->mVkFragmentShaderModules[stage.file_name] = vk_module;
        }
    }

    LL_INFOS("Vulkan") << "LLGLSLShader per-program SPIR-V "
                       << (cache_hit ? "(cache hit) " : "(cache miss, generated) ")
                       << "for program " << mName
                       << " (" << stage_spvs.size() << " stages, "
                       << stages.size() << " files)" << LL_ENDL;
    return true;
}

#if DEBUG_SHADER_INCLUDES
void dumpAttachObject(const char* func_name, GLuint program_object, const std::string& object_path)
{
    GLchar* info_log;
    GLint      info_len_expect = 0;
    GLint      info_len_actual = 0;

    glGetShaderiv(program_object, GL_INFO_LOG_LENGTH, , &info_len_expect);
    fprintf(stderr, " * %-20s(), log size: %d, %s\n", func_name, info_len_expect, object_path.c_str());

    if (info_len_expect > 0)
    {
        fprintf(stderr, " ========== %s() ========== \n", func_name);
        info_log = new GLchar[info_len_expect];
        glGetProgramInfoLog(program_object, info_len_expect, &info_len_actual, info_log);
        fprintf(stderr, "%s\n", info_log);
        delete[] info_log;
    }
}
#endif // DEBUG_SHADER_INCLUDES

bool LLGLSLShader::attachVertexObject(std::string object_path)
{
    if (LLShaderMgr::instance()->mVertexShaderObjects.count(object_path) > 0)
    {
        stop_glerror();
        glAttachShader(mProgramObject, LLShaderMgr::instance()->mVertexShaderObjects[object_path]);
#if DEBUG_SHADER_INCLUDES
        dumpAttachObject("attachVertexObject", mProgramObject, object_path);
#endif // DEBUG_SHADER_INCLUDES
        stop_glerror();
        // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B2-γ: Vulkan path で attach 順記録。
        // generatePerProgramSPIRV() が attach 順 (= attachShaderFeatures() 仕様順)
        // でこの list を辿り、LLShaderMgr::mVertexShaderSourceCache から source を引いて
        // stage concat の最先頭 (program-specific mShaderFiles 直前) に prepend する。
        if (LLVKLoader::isVulkanInitialized())
        {
            mVulkanAttachedVertexUtilities.push_back(object_path);
        }
        return true;
    }
    else
    {
        LL_SHADER_LOADING_WARNS() << "Attempting to attach shader object: '" << object_path << "' that hasn't been compiled." << LL_ENDL;
        return false;
    }
}

bool LLGLSLShader::attachFragmentObject(std::string object_path)
{
    if(mUsingBinaryProgram)
        return true;

    if (LLShaderMgr::instance()->mFragmentShaderObjects.count(object_path) > 0)
    {
        stop_glerror();
        glAttachShader(mProgramObject, LLShaderMgr::instance()->mFragmentShaderObjects[object_path]);
#if DEBUG_SHADER_INCLUDES
        dumpAttachObject("attachFragmentObject", mProgramObject, object_path);
#endif // DEBUG_SHADER_INCLUDES
        stop_glerror();
        // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B2-γ: Vulkan path で attach 順記録。
        // attachVertexObject と対称、Fragment 側 utility cache を参照 source として記録。
        if (LLVKLoader::isVulkanInitialized())
        {
            mVulkanAttachedFragmentUtilities.push_back(object_path);
        }
        return true;
    }
    else
    {
        LL_SHADER_LOADING_WARNS() << "Attempting to attach shader object: '" << object_path << "' that hasn't been compiled." << LL_ENDL;
        return false;
    }
}

void LLGLSLShader::attachObject(GLuint object)
{
    if(mUsingBinaryProgram)
        return;

    if (object != 0)
    {
        stop_glerror();
        glAttachShader(mProgramObject, object);
#if DEBUG_SHADER_INCLUDES
        std::string object_path("???");
        dumpAttachObject("attachObject", mProgramObject, object_path);
#endif // DEBUG_SHADER_INCLUDES
        stop_glerror();
    }
    else
    {
        LL_SHADER_LOADING_WARNS() << "Attempting to attach non existing shader object. " << LL_ENDL;
    }
}

void LLGLSLShader::attachObjects(GLuint* objects, S32 count)
{
    if(mUsingBinaryProgram)
        return;

    for (S32 i = 0; i < count; i++)
    {
        attachObject(objects[i]);
    }
}

bool LLGLSLShader::mapAttributes()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    bool res = true;
    if (!mUsingBinaryProgram)
    {
        //before linking, make sure reserved attributes always have consistent locations
        for (U32 i = 0; i < LLShaderMgr::instance()->mReservedAttribs.size(); i++)
        {
            const char* name = LLShaderMgr::instance()->mReservedAttribs[i].c_str();
            glBindAttribLocation(mProgramObject, i, (const GLchar*)name);
        }

        //link the program
        res = link();
    }

    mAttribute.clear();
#if LL_RELEASE_WITH_DEBUG_INFO
    mAttribute.resize(LLShaderMgr::instance()->mReservedAttribs.size(), { -1, NULL });
#else
    mAttribute.resize(LLShaderMgr::instance()->mReservedAttribs.size(), -1);
#endif

    if (res)
    { //read back channel locations

        mAttributeMask = 0;

        //read back reserved channels first
        for (U32 i = 0; i < LLShaderMgr::instance()->mReservedAttribs.size(); i++)
        {
            const char* name = LLShaderMgr::instance()->mReservedAttribs[i].c_str();
            S32 index = glGetAttribLocation(mProgramObject, (const GLchar*)name);
            if (index != -1)
            {
#if LL_RELEASE_WITH_DEBUG_INFO
                mAttribute[i] = { index, name };
#else
                mAttribute[i] = index;
#endif
                mAttributeMask |= 1 << i;
                LL_DEBUGS("ShaderUniform") << "Attribute " << name << " assigned to channel " << index << LL_ENDL;
            }
        }

        return true;
    }

    return false;
}

void LLGLSLShader::mapUniform(GLint index)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (index == -1)
    {
        return;
    }

    GLenum type;
    GLsizei length;
    GLint size = -1;
    char name[1024];        /* Flawfinder: ignore */
    name[0] = 0;


    glGetActiveUniform(mProgramObject, index, 1024, &length, &size, &type, (GLchar*)name);
    if (size > 0)
    {
        switch (type)
        {
        case GL_FLOAT_VEC2: size *= 2; break;
        case GL_FLOAT_VEC3: size *= 3; break;
        case GL_FLOAT_VEC4: size *= 4; break;
        case GL_DOUBLE: size *= 2; break;
        case GL_DOUBLE_VEC2: size *= 2; break;
        case GL_DOUBLE_VEC3: size *= 6; break;
        case GL_DOUBLE_VEC4: size *= 8; break;
        case GL_INT_VEC2: size *= 2; break;
        case GL_INT_VEC3: size *= 3; break;
        case GL_INT_VEC4: size *= 4; break;
        case GL_UNSIGNED_INT_VEC2: size *= 2; break;
        case GL_UNSIGNED_INT_VEC3: size *= 3; break;
        case GL_UNSIGNED_INT_VEC4: size *= 4; break;
        case GL_BOOL_VEC2: size *= 2; break;
        case GL_BOOL_VEC3: size *= 3; break;
        case GL_BOOL_VEC4: size *= 4; break;
        case GL_FLOAT_MAT2: size *= 4; break;
        case GL_FLOAT_MAT3: size *= 9; break;
        case GL_FLOAT_MAT4: size *= 16; break;
        case GL_FLOAT_MAT2x3: size *= 6; break;
        case GL_FLOAT_MAT2x4: size *= 8; break;
        case GL_FLOAT_MAT3x2: size *= 6; break;
        case GL_FLOAT_MAT3x4: size *= 12; break;
        case GL_FLOAT_MAT4x2: size *= 8; break;
        case GL_FLOAT_MAT4x3: size *= 12; break;
        case GL_DOUBLE_MAT2: size *= 8; break;
        case GL_DOUBLE_MAT3: size *= 18; break;
        case GL_DOUBLE_MAT4: size *= 32; break;
        case GL_DOUBLE_MAT2x3: size *= 12; break;
        case GL_DOUBLE_MAT2x4: size *= 16; break;
        case GL_DOUBLE_MAT3x2: size *= 12; break;
        case GL_DOUBLE_MAT3x4: size *= 24; break;
        case GL_DOUBLE_MAT4x2: size *= 16; break;
        case GL_DOUBLE_MAT4x3: size *= 24; break;
        }
        mTotalUniformSize += size;
    }

    S32 location = glGetUniformLocation(mProgramObject, name);
    if (location != -1)
    {
        //chop off "[0]" so we can always access the first element
        //of an array by the array name
        char* is_array = strstr(name, "[0]");
        if (is_array)
        {
            is_array[0] = 0;
        }

        LLStaticHashedString hashedName(name);
        mUniformMap[hashedName] = location;

        LL_DEBUGS("ShaderUniform") << "Uniform " << name << " is at location " << location << LL_ENDL;

        //find the index of this uniform
        for (S32 i = 0; i < (S32)LLShaderMgr::instance()->mReservedUniforms.size(); i++)
        {
            if ((mUniform[i] == -1)
                && (LLShaderMgr::instance()->mReservedUniforms[i] == name))
            {
                //found it
                mUniform[i] = location;
                mTexture[i] = mapUniformTextureChannel(location, type, size);
                if (mTexture[i] != -1)
                {
                    LL_DEBUGS("GLSLTextureChannels") << name << " assigned to texture channel " << mTexture[i] << LL_ENDL;
                }
                return;
            }
        }
    }
}

void LLGLSLShader::clearPermutations()
{
    mDefines.clear();
}

void LLGLSLShader::addPermutation(std::string name, std::string value)
{
    mDefines[name] = value;
}

void LLGLSLShader::addConstant(const LLGLSLShader::eShaderConsts shader_const)
{
    addPermutation(gShaderConstsKey[shader_const], gShaderConstsVal[shader_const]);
}

void LLGLSLShader::removePermutation(std::string name)
{
    mDefines.erase(name);
}

GLint LLGLSLShader::mapUniformTextureChannel(GLint location, GLenum type, GLint size)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if ((type >= GL_SAMPLER_1D && type <= GL_SAMPLER_2D_RECT_SHADOW) ||
        type == GL_SAMPLER_2D_MULTISAMPLE ||
        type == GL_SAMPLER_CUBE_MAP_ARRAY)
    {   //this here is a texture
        GLint ret = mActiveTextureChannels;
        if (size == 1)
        {
            glUniform1i(location, mActiveTextureChannels);
            mActiveTextureChannels++;
        }
        else
        {
            //is array of textures, make sequential after this texture
            GLint channel[16]; // <=== only support up to 16 texture channels
            llassert(size <= 16);
            size = llmin(size, 16);
            for (int i = 0; i < size; ++i)
            {
                channel[i] = mActiveTextureChannels++;
            }
            glUniform1iv(location, size, channel);
        }

        return ret;
    }
    return -1;
}

bool LLGLSLShader::mapUniforms()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    bool res = true;

    mTotalUniformSize = 0;
    mActiveTextureChannels = 0;
    mUniform.clear();
    mUniformMap.clear();
    mTexture.clear();
    mValue.clear();
    //initialize arrays
    mUniform.resize(LLShaderMgr::instance()->mReservedUniforms.size(), -1);
    mTexture.resize(LLShaderMgr::instance()->mReservedUniforms.size(), -1);

    bind();

    //get the number of active uniforms
    GLint activeCount;
    glGetProgramiv(mProgramObject, GL_ACTIVE_UNIFORMS, &activeCount);

    //........................................................................................................................................
    //........................................................................................

    /*
    EXPLANATION:
    This is part of code is temporary because as the final result the mapUniform() should be rewrited.
    But it's a huge a volume of work which is need to be a more carefully performed for avoid possible
    regression's (i.e. it should be formalized a separate ticket in JIRA).

    RESON:
    The reason of this code is that SL engine is very sensitive to fact that "diffuseMap" should be appear
    first as uniform parameter which is should get 0-"texture channel" index (see mapUniformTextureChannel() and mActiveTextureChannels)
    it influence to which is texture matrix will be updated during rendering.

    But, order of indexe's of uniform variables is not defined and GLSL compiler can change it as want
    , even if the "diffuseMap" will be appear and use first in shader code.

    As example where this situation appear see: "Deferred Material Shader 28/29/30/31"
    And tickets: MAINT-4165, MAINT-4839, MAINT-3568, MAINT-6437

    --- davep TODO -- pretty sure the entire block here is superstitious and that the uniform index has nothing to do with the texture channel
                texture channel should follow the uniform VALUE
    */


    S32 diffuseMap = glGetUniformLocation(mProgramObject, "diffuseMap");
    S32 specularMap = glGetUniformLocation(mProgramObject, "specularMap");
    S32 bumpMap = glGetUniformLocation(mProgramObject, "bumpMap");
    S32 altDiffuseMap = glGetUniformLocation(mProgramObject, "altDiffuseMap");
    S32 environmentMap = glGetUniformLocation(mProgramObject, "environmentMap");
    S32 reflectionMap = glGetUniformLocation(mProgramObject, "reflectionMap");

    std::set<S32> skip_index;

    if (-1 != diffuseMap && (-1 != specularMap || -1 != bumpMap || -1 != environmentMap || -1 != altDiffuseMap))
    {
        GLenum type;
        GLsizei length;
        GLint size = -1;
        char name[1024];

        diffuseMap = altDiffuseMap = specularMap = bumpMap = environmentMap = -1;

        for (S32 i = 0; i < activeCount; i++)
        {
            name[0] = '\0';

            glGetActiveUniform(mProgramObject, i, 1024, &length, &size, &type, (GLchar*)name);

            if (-1 == diffuseMap && std::string(name) == "diffuseMap")
            {
                diffuseMap = i;
                continue;
            }

            if (-1 == specularMap && std::string(name) == "specularMap")
            {
                specularMap = i;
                continue;
            }

            if (-1 == bumpMap && std::string(name) == "bumpMap")
            {
                bumpMap = i;
                continue;
            }

            if (-1 == environmentMap && std::string(name) == "environmentMap")
            {
                environmentMap = i;
                continue;
            }

            if (-1 == reflectionMap && std::string(name) == "reflectionMap")
            {
                reflectionMap = i;
                continue;
            }

            if (-1 == altDiffuseMap && std::string(name) == "altDiffuseMap")
            {
                altDiffuseMap = i;
                continue;
            }
        }

        bool specularDiff = specularMap < diffuseMap && -1 != specularMap;
        bool bumpLessDiff = bumpMap < diffuseMap && -1 != bumpMap;
        bool envLessDiff = environmentMap < diffuseMap && -1 != environmentMap;
        bool refLessDiff = reflectionMap < diffuseMap && -1 != reflectionMap;

        if (specularDiff || bumpLessDiff || envLessDiff || refLessDiff)
        {
            mapUniform(diffuseMap);
            skip_index.insert(diffuseMap);

            if (-1 != specularMap) {
                mapUniform(specularMap);
                skip_index.insert(specularMap);
            }

            if (-1 != bumpMap) {
                mapUniform(bumpMap);
                skip_index.insert(bumpMap);
            }

            if (-1 != environmentMap) {
                mapUniform(environmentMap);
                skip_index.insert(environmentMap);
            }

            if (-1 != reflectionMap) {
                mapUniform(reflectionMap);
                skip_index.insert(reflectionMap);
            }
        }
    }

    //........................................................................................

    for (S32 i = 0; i < activeCount; i++)
    {
        //........................................................................................
        if (skip_index.end() != skip_index.find(i)) continue;
        //........................................................................................

        mapUniform(i);
    }
    //........................................................................................................................................

    // Set up block binding, in a way supported by Apple (rather than binding = 1 in .glsl).
    // See slide 35 and more of https://docs.huihoo.com/apple/wwdc/2011/session_420__advances_in_opengl_for_mac_os_x_lion.pdf
    const char* ubo_names[] =
    {
        "ReflectionProbes", // UB_REFLECTION_PROBES
        "GLTFJoints",       // UB_GLTF_JOINTS
        "GLTFNodes",        // UB_GLTF_NODES
        "GLTFMaterials",    // UB_GLTF_MATERIALS
    };

    llassert(LL_ARRAY_SIZE(ubo_names) == NUM_UNIFORM_BLOCKS);

    for (U32 i = 0; i < NUM_UNIFORM_BLOCKS; ++i)
    {
        GLuint UBOBlockIndex = glGetUniformBlockIndex(mProgramObject, ubo_names[i]);
        if (UBOBlockIndex != GL_INVALID_INDEX)
        {
            glUniformBlockBinding(mProgramObject, UBOBlockIndex, i);
        }
    }

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-2:
    // Vulkan path integer index 経路 cache 構築 (spec 06a §4.1 literal)。
    // mReservedUniforms と並列に mUniformUBOLoc を構築、shader link 時 1 回限りで
    // runtime hash 計算を済ませ、frame 内 setter は mUniformUBOLoc[index] 直引きで O(1)。
    // GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 不使用、mUseUBO runtime flag 単独 gate。
    // mUseUBO=false default (= MUSEUBO-A) ゆえ既存 OpenGL build / 既存挙動 100% 維持。
    if (mUseUBO)
    {
        const auto& reserved = LLShaderMgr::instance()->mReservedUniforms;
        mUniformUBOLoc.resize(reserved.size());
        for (size_t i = 0; i < reserved.size(); ++i)
        {
            const ubo::UniformLocation* loc = ubo::lookup_runtime(reserved[i].c_str());
            if (loc)
            {
                mUniformUBOLoc[i] = *loc;
            }
            else
            {
                // UBO 集約表に entry 無し = bare uniform 残存中の silent skip path
                // (spec 06a §3.3 CADENCE_INVALID = 0xFFFFFFFF sentinel、setter 側で skip)。
                mUniformUBOLoc[i] = ubo::UniformLocation{ 0u, 0u, 0u, 0xFFFFFFFFu };
            }
        }
    }

    // r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-3:
    // Vulkan path LLStaticHashedString 経路補助 cache 構築 (spec 06a §4.3 / §4.3.1 S1-C)。
    // build-time list g_static_hashed_uniform_names[] iterate で lookup_runtime → hash 計算 →
    // mUniformUBOLocByHash 登録。setter 側 (= PB-5) は uniform.Hash() で同 hash 引きで O(1)
    // lookup (= hash 一致保証は LLStaticHashedString::Hash() 共通使用)。
    // GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 不使用、mUseUBO runtime flag 単独 gate。
    // miss 時 = UBO 集約表に entry 無し = silent skip (setter 側 find() で end() 返却 →
    // OpenGL path fallback、bare uniform 残存中の正常 path)。
    if (mUseUBO)
    {
        for (const char* name : g_static_hashed_uniform_names)
        {
            const ubo::UniformLocation* loc = ubo::lookup_runtime(name);
            if (loc)
            {
                const U64 hash = static_cast<U64>(LLStaticHashedString(name).Hash());
                mUniformUBOLocByHash[hash] = *loc;
            }
        }
    }

    unbind();

    LL_DEBUGS("ShaderUniform") << "Total Uniform Size: " << mTotalUniformSize << LL_ENDL;
    return res;
}

// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.B PB-6: UBO redirect 層
// shell 実装 (= FWD-1 採用、空 stub)。spec 06a §5.2 literal「forwardToUboUpload(loc,
// &x, sizeof(GLfloat));  // 06b で実装」の本体 = 実 memcpy / ring buffer / dynamic
// offset / thread 配線は 06b / chapter 07 で実装する。本 sub-step は declaration +
// 空 stub の link 通し限定 = PB-4 (= 17 method integer index 経路) / PB-5 (= 13 method
// LLStaticHashedString 経路) で各 setter から call する link error 回避の technical
// compile dependency 目的 (= 順序組替え PB-3 complete handoff §3.6 で PB-4 直前に前倒し)。
// mUseUBO=false default ゆえ本 stub は実走しない (= MUSEUBO-A 整合、既存 OpenGL 挙動
// 100% 維持)。Phase 1.C (= 06b 実装着手) で本体実装される。
void LLGLSLShader::forwardToUboUpload(const ubo::UniformLocation& loc, const void* data, size_t size)
{
}


bool LLGLSLShader::link(bool suppress_errors)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    bool success = LLShaderMgr::instance()->linkProgramObject(mProgramObject, suppress_errors);

    if (!success && !suppress_errors)
    {
        LLShaderMgr::instance()->dumpObjectLog(mProgramObject, !success, mName);
    }

    if (success)
    {
        LLShaderMgr::instance()->saveCachedProgramBinary(this);
    }

    return success;
}

void LLGLSLShader::bind()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    llassert_always(mProgramObject != 0);

    gGL.flush();

    if (sCurBoundShader != mProgramObject)  // Don't re-bind current shader
    {
        if (sCurBoundShaderPtr)
        {
            sCurBoundShaderPtr->readProfileQuery();
        }
        LLVertexBuffer::unbind();
        glUseProgram(mProgramObject);
        sCurBoundShader = mProgramObject;
        sCurBoundShaderPtr = this;
        placeProfileQuery();
        LLVertexBuffer::setupClientArrays(mAttributeMask);
    }

    if (mUniformsDirty)
    {
        LLShaderMgr::instance()->updateShaderUniforms(this);
        mUniformsDirty = false;
    }

    llassert_always(sCurBoundShaderPtr != nullptr);
    llassert_always(sCurBoundShader == mProgramObject);
}

void LLGLSLShader::bind(U8 variant)
{
    llassert_always(mGLTFVariants.size() == LLGLSLShader::NUM_GLTF_VARIANTS);
    llassert_always(variant < LLGLSLShader::NUM_GLTF_VARIANTS);
    mGLTFVariants[variant].bind();
}

void LLGLSLShader::bind(bool rigged)
{
    if (rigged)
    {
        llassert_always(mRiggedVariant);
        mRiggedVariant->bind();
    }
    else
    {
        bind();
    }
}

void LLGLSLShader::unbind(void)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    gGL.flush();
    LLVertexBuffer::unbind();

    if (sCurBoundShaderPtr)
    {
        sCurBoundShaderPtr->readProfileQuery();
    }

    glUseProgram(0);
    sCurBoundShader = 0;
    sCurBoundShaderPtr = NULL;
}

S32 LLGLSLShader::bindTexture(const std::string& uniform, LLTexture* texture, LLTexUnit::eTextureType mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    S32 channel = 0;
    channel = getUniformLocation(uniform);

    return bindTexture(channel, texture, mode);
}

S32 LLGLSLShader::bindTexture(S32 uniform, LLTexture* texture, LLTexUnit::eTextureType mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << uniform << LL_ENDL;
        llassert(false);
        return -1;
    }

    uniform = mTexture[uniform];

    if (uniform > -1)
    {
        gGL.getTexUnit(uniform)->bindFast(texture);
    }

    return uniform;
}

S32 LLGLSLShader::bindTexture(S32 uniform, LLRenderTarget* texture, bool depth, LLTexUnit::eTextureFilterOptions mode, U32 index)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << uniform << LL_ENDL;
        llassert(false);
        return -1;
    }

    uniform = getTextureChannel(uniform);

    if (uniform > -1)
    {
        if (depth) {
            gGL.getTexUnit(uniform)->bind(texture, true);
        }
        else {
            bool has_mips = mode == LLTexUnit::TFO_TRILINEAR || mode == LLTexUnit::TFO_ANISOTROPIC;
            gGL.getTexUnit(uniform)->bindManual(texture->getUsage(), texture->getTexture(index), has_mips);
        }

        gGL.getTexUnit(uniform)->setTextureFilteringOption(mode);
    }

    return uniform;
}

S32 LLGLSLShader::bindTexture(const std::string& uniform, LLRenderTarget* texture, bool depth, LLTexUnit::eTextureFilterOptions mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    S32 channel = 0;
    channel = getUniformLocation(uniform);

    return bindTexture(channel, texture, depth, mode);
}

S32 LLGLSLShader::unbindTexture(const std::string& uniform, LLTexUnit::eTextureType mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    S32 channel = 0;
    channel = getUniformLocation(uniform);

    return unbindTexture(channel);
}

S32 LLGLSLShader::unbindTexture(S32 uniform, LLTexUnit::eTextureType mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << uniform << LL_ENDL;
        llassert(false);
        return -1;
    }

    uniform = mTexture[uniform];

    if (uniform > -1)
    {
        gGL.getTexUnit(uniform)->unbindFast(mode);
    }

    return uniform;
}

S32 LLGLSLShader::getTextureChannel(S32 uniform) const
{
    return mTexture[uniform];
}

S32 LLGLSLShader::enableTexture(S32 uniform, LLTexUnit::eTextureType mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << uniform << LL_ENDL;
        llassert(false);
        return -1;
    }


    S32 index = mTexture[uniform];
    if (index != -1)
    {
        gGL.getTexUnit(index)->activate();
        gGL.getTexUnit(index)->enable(mode);
    }
    return index;
}

S32 LLGLSLShader::disableTexture(S32 uniform, LLTexUnit::eTextureType mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << uniform << LL_ENDL;
        llassert(false);
        return -1;
    }

    S32 index = mTexture[uniform];
    if (index < 0)
    {
        // Invalid texture index - nothing to disable
        return index;
    }

    LLTexUnit* tex_unit = gGL.getTexUnit(index);
    if (!tex_unit)
    {
        // Invalid texture unit
        LL_WARNS_ONCE("Shader") << "Invalid texture unit at index: " << index << LL_ENDL;
        return index;
    }

    LLTexUnit::eTextureType curr_type = tex_unit->getCurrType();
    if (curr_type != LLTexUnit::TT_NONE)
    {
        if (gDebugGL && curr_type != mode)
        {
            if (gDebugSession)
            {
                gFailLog << "Texture channel " << index << " texture type corrupted. Expected: " << mode << ", Found: " << curr_type << std::endl;
                ll_fail("LLGLSLShader::disableTexture failed");
            }
            else
            {
                LL_ERRS() << "Texture channel " << index << " texture type corrupted. Expected: " << mode << ", Found: " << curr_type << LL_ENDL;
            }
        }
        tex_unit->disable();
    }

    return index;
}

void LLGLSLShader::uniform1i(U32 index, GLint x)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);
    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            if (iter == mValue.end() || iter->second.mV[0] != x)
            {
                glUniform1i(mUniform[index], x);
                mValue[mUniform[index]] = LLVector4((F32)x, 0.f, 0.f, 0.f);
            }
        }
    }
}

void LLGLSLShader::uniform1f(U32 index, GLfloat x)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            if (iter == mValue.end() || iter->second.mV[0] != x)
            {
                glUniform1f(mUniform[index], x);
                mValue[mUniform[index]] = LLVector4(x, 0.f, 0.f, 0.f);
            }
        }
    }
}

void LLGLSLShader::fastUniform1f(U32 index, GLfloat x)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);
    llassert(mProgramObject);
    llassert(mUniform.size() <= index);
    llassert(mUniform[index] >= 0);
    glUniform1f(mUniform[index], x);
}

void LLGLSLShader::uniform2f(U32 index, GLfloat x, GLfloat y)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec(x, y, 0.f, 0.f);
            if (iter == mValue.end() || shouldChange(iter->second, vec))
            {
                glUniform2f(mUniform[index], x, y);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform3f(U32 index, GLfloat x, GLfloat y, GLfloat z)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec(x, y, z, 0.f);
            if (iter == mValue.end() || shouldChange(iter->second, vec))
            {
                glUniform3f(mUniform[index], x, y, z);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec(x, y, z, w);
            if (iter == mValue.end() || shouldChange(iter->second, vec))
            {
                glUniform4f(mUniform[index], x, y, z, w);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform1iv(U32 index, U32 count, const GLint* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec((F32)v[0], 0.f, 0.f, 0.f);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                glUniform1iv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform4iv(U32 index, U32 count, const GLint* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec((F32)v[0], (F32)v[1], (F32)v[2], (F32)v[3]);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                glUniform1iv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}


void LLGLSLShader::uniform1fv(U32 index, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec(v[0], 0.f, 0.f, 0.f);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                glUniform1fv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform2fv(U32 index, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec(v[0], v[1], 0.f, 0.f);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                glUniform2fv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform3fv(U32 index, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec(v[0], v[1], v[2], 0.f);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                glUniform3fv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform4fv(U32 index, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec(v[0], v[1], v[2], v[3]);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
                glUniform4fv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform4uiv(U32 index, U32 count, const GLuint* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec((F32)v[0], (F32)v[1], (F32)v[2], (F32)v[3]);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
                glUniform4uiv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniformMatrix2fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            glUniformMatrix2fv(mUniform[index], count, transpose, v);
        }
    }
}

void LLGLSLShader::uniformMatrix3fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            glUniformMatrix3fv(mUniform[index], count, transpose, v);
        }
    }
}

void LLGLSLShader::uniformMatrix3x4fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            glUniformMatrix3x4fv(mUniform[index], count, transpose, v);
        }
    }
}

void LLGLSLShader::uniformMatrix4fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            glUniformMatrix4fv(mUniform[index], count, transpose, v);
        }
    }
}

GLint LLGLSLShader::getUniformLocation(const LLStaticHashedString& uniform)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    GLint ret = -1;
    if (mProgramObject)
    {
        LLStaticStringTable<GLint>::iterator iter = mUniformMap.find(uniform);
        if (iter != mUniformMap.end())
        {
            if (gDebugGL)
            {
                stop_glerror();
                if (iter->second != glGetUniformLocation(mProgramObject, uniform.String().c_str()))
                {
                    LL_ERRS() << "Uniform does not match." << LL_ENDL;
                }
                stop_glerror();
            }
            ret = iter->second;
        }
    }

    return ret;
}

GLint LLGLSLShader::getUniformLocation(U32 index)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    GLint ret = -1;
    if (mProgramObject)
    {
        if (index >= mUniform.size())
        {
            LL_WARNS_ONCE("Shader") << "Uniform index " << index << " out of bounds " << (S32)mUniform.size() << LL_ENDL;
            return ret;
        }
        return mUniform[index];
    }

    return ret;
}

GLint LLGLSLShader::getAttribLocation(U32 attrib)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (attrib < mAttribute.size())
    {
        return mAttribute[attrib];
    }
    else
    {
        return -1;
    }
}

void LLGLSLShader::uniform1i(const LLStaticHashedString& uniform, GLint v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec((F32)v, 0.f, 0.f, 0.f);
        if (iter == mValue.end() || shouldChange(iter->second, vec))
        {
            glUniform1i(location, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform1iv(const LLStaticHashedString& uniform, U32 count, const GLint* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        LLVector4 vec((F32)v[0], 0.f, 0.f, 0.f);
        const auto& iter = mValue.find(location);
        if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
            glUniform1iv(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform4iv(const LLStaticHashedString& uniform, U32 count, const GLint* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        LLVector4 vec((F32)v[0], (F32)v[1], (F32)v[2], (F32)v[3]);
        const auto& iter = mValue.find(location);
        if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
            glUniform4iv(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform2i(const LLStaticHashedString& uniform, GLint i, GLint j)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec((F32)i, (F32)j, 0.f, 0.f);
        if (iter == mValue.end() || shouldChange(iter->second, vec))
        {
            glUniform2i(location, i, j);
            mValue[location] = vec;
        }
    }
}


void LLGLSLShader::uniform1f(const LLStaticHashedString& uniform, GLfloat v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec(v, 0.f, 0.f, 0.f);
        if (iter == mValue.end() || shouldChange(iter->second, vec))
        {
            glUniform1f(location, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform2f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec(x, y, 0.f, 0.f);
        if (iter == mValue.end() || shouldChange(iter->second, vec))
        {
            glUniform2f(location, x, y);
            mValue[location] = vec;
        }
    }

}

void LLGLSLShader::uniform3f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y, GLfloat z)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec(x, y, z, 0.f);
        if (iter == mValue.end() || shouldChange(iter->second, vec))
        {
            glUniform3f(location, x, y, z);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform4f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec(x, y, z, w);
        if (iter == mValue.end() || shouldChange(iter->second, vec))
        {
            glUniform4f(location, x, y, z, w);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform1fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec(v[0], 0.f, 0.f, 0.f);
        if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
        {
            glUniform1fv(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform2fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec(v[0], v[1], 0.f, 0.f);
        if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
        {
            glUniform2fv(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform3fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec(v[0], v[1], v[2], 0.f);
        if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
        {
            glUniform3fv(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform4fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        LLVector4 vec(v);
        const auto& iter = mValue.find(location);
        if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
            glUniform4fv(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform4uiv(const LLStaticHashedString& uniform, U32 count, const GLuint* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        LLVector4 vec((F32)v[0], (F32)v[1], (F32)v[2], (F32)v[3]);
        const auto& iter = mValue.find(location);
        if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
            glUniform4uiv(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniformMatrix4fv(const LLStaticHashedString& uniform, U32 count, GLboolean transpose, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        stop_glerror();
        glUniformMatrix4fv(location, count, transpose, v);
        stop_glerror();
    }
}


void LLGLSLShader::vertexAttrib4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    if (mAttribute[index] > 0)
    {
        glVertexAttrib4f(mAttribute[index], x, y, z, w);
    }
}

void LLGLSLShader::vertexAttrib4fv(U32 index, GLfloat* v)
{
    if (mAttribute[index] > 0)
    {
        glVertexAttrib4fv(mAttribute[index], v);
    }
}

void LLGLSLShader::setMinimumAlpha(F32 minimum)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    gGL.flush();
    uniform1f(LLShaderMgr::MINIMUM_ALPHA, minimum);
}

void LLShaderUniforms::apply(LLGLSLShader* shader)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    for (auto& uniform : mIntegers)
    {
        shader->uniform1i(uniform.mUniform, uniform.mValue);
    }

    for (auto& uniform : mFloats)
    {
        shader->uniform1f(uniform.mUniform, uniform.mValue);
    }

    for (auto& uniform : mVectors)
    {
        shader->uniform4fv(uniform.mUniform, 1, uniform.mValue.mV);
    }

    for (auto& uniform : mVector3s)
    {
        shader->uniform3fv(uniform.mUniform, 1, uniform.mValue.mV);
    }
}

LLUUID LLGLSLShader::hash()
{
    HBXXH128 hash_obj;
    hash_obj.update(mName);
    hash_obj.update(&mShaderGroup, sizeof(mShaderGroup));
    hash_obj.update(&mShaderLevel, sizeof(mShaderLevel));
    for (const auto& shdr_pair : mShaderFiles)
    {
        hash_obj.update(shdr_pair.first);
        hash_obj.update(&shdr_pair.second, sizeof(GLenum));
    }
    for (const auto& define_pair : mDefines)
    {
        hash_obj.update(define_pair.first);
        hash_obj.update(define_pair.second);

    }
    for (const auto& define_pair : LLGLSLShader::sGlobalDefines)
    {
        hash_obj.update(define_pair.first);
        hash_obj.update(define_pair.second);

    }
    hash_obj.update(&mFeatures, sizeof(LLShaderFeatures));
    hash_obj.update(gGLManager.mGLVendor);
    hash_obj.update(gGLManager.mGLRenderer);
    hash_obj.update(gGLManager.mGLVersionString);
    return hash_obj.digest();
}

#if LL_PROFILER_ENABLE_RENDER_DOC
void LLGLSLShader::setLabel(const char* label) {
    LL_LABEL_OBJECT_GL(GL_PROGRAM, mProgramObject, strlen(label), label);
}
#endif
