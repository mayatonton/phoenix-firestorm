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
#include "llimagegl.h"

#include "hbxxh.h"
#include "llsdserialize.h"
#include "lldir.h"

#if LL_DARWIN
#include "OpenGL/OpenGL.h"
#endif

#include "llvkloader.h"
#include "llvkuboreg.h"
#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <set>
#include <sstream>
#include <tuple>
#include <unordered_set>

#include "llcontrol.h"

extern LLControlGroup gSavedSettings;

#define DEBUG_SHADER_INCLUDES 0

using std::vector;
using std::pair;
using std::make_pair;
using std::string;

LLGLSLShader* LLGLSLShader::sCurBoundShaderPtr = NULL;

VkDescriptorSet LLGLSLShader::sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
U32 LLGLSLShader::sCurPerCallVkDynamicOffsets[LLGLSLShader::MAX_VK_DYNAMIC_BINDINGS] = {};
bool LLGLSLShader::sCurPerCallVkOffsetsDirty = false;
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

LLGLSLShader    gUIProgram;
LLGLSLShader    gSolidColorProgram;

const std::string gShaderConstsKey[LLGLSLShader::NUM_SHADER_CONSTS] =
{
      "LL_SHADER_CONST_CLOUD_MOON_DEPTH"
    , "LL_SHADER_CONST_STAR_DEPTH"
};

const std::string gShaderConstsVal[LLGLSLShader::NUM_SHADER_CONSTS] =
{
      "0.99998" // SHADER_CONST_CLOUD_MOON_DEPTH // SL-14113
    , "0.99999" // SHADER_CONST_STAR_DEPTH       // SL-14113
};


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

void LLGLSLShader::placeProfileQuery(bool for_runtime)
{
    if (sProfileEnabled || for_runtime)
    {
        if (LLVKLoader::isVulkanInitialized() && LLVKLoader::isTimestampSupportedVk())
        {
            VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
            if (cmd != VK_NULL_HANDLE)
            {
                if (mVkTimestampHandle == 0)
                {
                    mVkTimestampHandle = LLVKLoader::acquireTimestampPairVk();
                }
                if (mVkTimestampHandle != 0)
                {
                    LLVKLoader::cmdWriteTimestampBeginVk(cmd, mVkTimestampHandle);
                }
            }
            return;
        }
    }
}

bool LLGLSLShader::readProfileQuery(bool for_runtime, bool force_read)
{
    if ((sProfileEnabled || for_runtime) && sCanProfile)
    {
        if (LLVKLoader::isVulkanInitialized() && LLVKLoader::isTimestampSupportedVk())
        {
            if (mVkTimestampHandle == 0)
            {
                return true;
            }

            if (!mProfilePending)
            {
                VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
                if (cmd != VK_NULL_HANDLE)
                {
                    LLVKLoader::cmdWriteTimestampEndVk(cmd, mVkTimestampHandle);
                }
                mProfilePending = for_runtime;
            }

            bool     avail      = false;
            uint64_t elapsed_ns = 0;
            LLVKLoader::getTimestampElapsedNsVk(mVkTimestampHandle, avail, elapsed_ns);

            if (mProfilePending && for_runtime && !force_read && !avail)
            {
                return false;
            }

            mTimeElapsed += elapsed_ns;
            mProfilePending = false;
            LLVKLoader::releaseTimestampPairVk(mVkTimestampHandle);
            mVkTimestampHandle = 0;
            return true;
        }
    }

    return true;
}



LLGLSLShader::LLGLSLShader()
    : mActiveTextureChannels(0),
    mShaderLevel(0),
    mShaderGroup(SG_DEFAULT),
    mFeatures()
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

    if (sCurBoundShaderPtr == this)
    {
        sCurBoundShaderPtr = NULL;
    }

    mVkEnumBoundView.clear();
    mChannelToEnum.clear();

    if (LLVKLoader::isVulkanInitialized())
    {
        for (auto& kv : mVkPipelineCache)
        {
            if (kv.second != VK_NULL_HANDLE)
            {
                LLVKLoader::destroyPipelineVk(kv.second);
            }
        }
        mVkPipelineCache.clear();
        for (auto& kv : mVkVertexShaderModulesPerProgram)
        {
            if (kv.second != VK_NULL_HANDLE)
            {
                LLVKLoader::destroyShaderModuleVk(kv.second);
            }
        }
        mVkVertexShaderModulesPerProgram.clear();
        for (auto& kv : mVkFragmentShaderModulesPerProgram)
        {
            if (kv.second != VK_NULL_HANDLE)
            {
                LLVKLoader::destroyShaderModuleVk(kv.second);
            }
        }
        mVkFragmentShaderModulesPerProgram.clear();
        for (auto& kv : mVkGeometryShaderModulesPerProgram)
        {
            if (kv.second != VK_NULL_HANDLE)
            {
                LLVKLoader::destroyShaderModuleVk(kv.second);
            }
        }
        mVkGeometryShaderModulesPerProgram.clear();
        if (mVkPipelineLayout != VK_NULL_HANDLE)
        {
            LLVKLoader::destroyPipelineLayoutVk(mVkPipelineLayout);
            mVkPipelineLayout = VK_NULL_HANDLE;
        }
        if (mVkDescriptorSetLayout != VK_NULL_HANDLE)
        {
            LLVKLoader::destroyDescriptorSetLayoutVk(mVkDescriptorSetLayout);
            mVkDescriptorSetLayout = VK_NULL_HANDLE;
        }
        if (mVkPerProgramUBO != VK_NULL_HANDLE)
        {
            LLVKLoader::destroyBufferVk(mVkPerProgramUBO, mVkPerProgramUBOAllocation);
            mVkPerProgramUBO           = VK_NULL_HANDLE;
            mVkPerProgramUBOAllocation = nullptr;
            mVkPerProgramUBOMapped     = nullptr;
            mVkPerProgramUBOSize       = 0;
        }
        for (U32 f = 0; f < 3; ++f)
        {
            for (auto& slot : mVkPerProgramUBORing[f])
            {
                if (slot.buffer != VK_NULL_HANDLE)
                {
                    LLVKLoader::destroyBufferVk(slot.buffer, slot.allocation);
                }
            }
            mVkPerProgramUBORing[f].clear();
            mVkPerProgramRingIdx[f]   = 0;
            mVkPerProgramRingFrame[f] = 0;
        }
        mVkActivePerProgramUBO       = VK_NULL_HANDLE;
        mVkActivePerProgramUBOMapped = nullptr;
        mVkPerProgramShadow.clear();
        mVkSet1DynamicCount        = 0;
        mVkDynamicBindingMask      = 0;
        mVkDynamicBindings.clear();
        mVkPerProgramUBOGeneration = 0;
        mVkPerProgramUBOBaseMapped = nullptr;
    }

    mTexture.clear();
}

bool LLGLSLShader::createShader()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    unloadInternal();

    sInstances.insert(this);

    for (U32 i = 0; i < LLRender::NUM_MATRIX_MODES; ++i)
    {
        mMatHash[i] = 0xFFFFFFFF;
    }
    mLightHash = 0xFFFFFFFF;

    llassert_always(!mShaderFiles.empty());

#if LL_DARWIN
    if(!gGLManager.mIsApple)
    {
        mDefines["OLD_SELECT"] = "1";
    }
#endif

    mShaderHash = hash();

    mComplete = false;
    mVkComplete = false;

    bool success = true;

    const bool collect_for_vulkan = LLVKLoader::isVulkanInitialized();

    if (collect_for_vulkan)
    {
#if DEBUG_SHADER_INCLUDES
        fprintf(stderr, "--- %s ---\n", mName.c_str());
#endif // DEBUG_SHADER_INCLUDES

        mStageSources.clear();

        vector< pair<string, GLenum> >::iterator fileIter = mShaderFiles.begin();
        for (; fileIter != mShaderFiles.end(); fileIter++)
        {
            std::vector<std::string> stage_sources;
            GLuint loaded = LLShaderMgr::instance()->loadShaderFile((*fileIter).first, mShaderLevel, (*fileIter).second, &mDefines, mFeatures.mIndexedTextureChannels, collect_for_vulkan ? &stage_sources : nullptr);
            LL_DEBUGS("ShaderLoading") << "SHADER FILE: " << (*fileIter).first << " mShaderLevel=" << mShaderLevel << LL_ENDL;
            if (collect_for_vulkan && !stage_sources.empty())
            {
                mStageSources.push_back({ (*fileIter).second, (*fileIter).first, std::move(stage_sources) });
            }
            if (!loaded)
            {
                success = false;
            }
        }

    }

    if (!LLShaderMgr::instance()->attachShaderFeatures(this))
    {
        unloadInternal();
        return false;
    }

    mVkBindingToChannel.fill(-1);

    mVkBindingToEnumCanonical.fill(-1);

    mVkBindingDeclaredType.fill(VKBD_NONE);
    mVkBindingStageMask.fill(0);
    mVkBindingSamplerDim.fill(VKSD_2D);
    mVkBindingSamplerUsed.fill(false);

    mVkReflBindingSamplerNames.clear();

    mVkReflUboBlocks.clear();
    mVkReflPushConstants.clear();

    mVkEnumBoundView.clear();
    mChannelToEnum.clear();

    if (success && LLVKLoader::isVulkanInitialized() && !mStageSources.empty())
    {
        mVkComplete = generatePerProgramSPIRV(mStageSources);
        LLVkUboReg::verifyProgramLayout(*this);
    }
    mStageSources.clear();
    mStageSources.shrink_to_fit();
    mVulkanAttachedVertexUtilities.clear();
    mVulkanAttachedVertexUtilities.shrink_to_fit();
    mVulkanAttachedFragmentUtilities.clear();
    mVulkanAttachedFragmentUtilities.shrink_to_fit();
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

        if (mShaderLevel > 0)
        {
            LL_SHADER_LOADING_WARNS() << "Failed to link using shader level " << mShaderLevel << " trying again using shader level " << (mShaderLevel - 1) << LL_ENDL;
            mShaderLevel--;
            return createShader();
        }
        else
        {
            unloadInternal();
        }
    }
    else if (mFeatures.mIndexedTextureChannels > 0)
    {
        llassert(mFeatures.mIndexedTextureChannels == LLGLSLShader::sIndexedTextureChannels); // these numbers must always match
        bind();
        S32 channel_count = mFeatures.mIndexedTextureChannels;

        for (U32 i = 0; i < mTexture.size(); i++)
        {
            if (mTexture[i] > -1)
            {
                S32 new_tex = mTexture[i] + channel_count;
                mTexture[i] = new_tex;
            }
        }

        mActiveTextureChannels = channel_count;
        for (auto& tex : mTexture)
        {
            mActiveTextureChannels = llmax(mActiveTextureChannels, tex + 1);
        }

        llassert(mActiveTextureChannels <= 16);
        unbind();
    }

    LL_DEBUGS("GLSLTextureChannels") << mName << " has " << mActiveTextureChannels << " active texture channels" << LL_ENDL;

    if (success && !mVkReflBindingSamplerNames.empty())
    {
        const std::vector<std::string>& reserved = LLShaderMgr::instance()->mReservedUniforms;
        for (const auto& bn : mVkReflBindingSamplerNames)
        {
            if (bn.first >= 0 && bn.first < (S32)MAX_VK_BINDING)
            {
                for (S32 ri = 0; ri < (S32)reserved.size(); ++ri)
                {
                    if (reserved[ri] == bn.second) { mVkBindingToEnumCanonical[bn.first] = ri; break; }
                }
            }
        }
    }

    if (success && !mVkReflBindingSamplerNames.empty())
    {
        for (const auto& bn : mVkReflBindingSamplerNames)
        {
            if (bn.first < 0 || bn.first >= (S32)MAX_VK_BINDING)
            {
                continue;
            }
            S32 e = mVkBindingToEnumCanonical[bn.first];
            if (e >= 0 && e < (S32)mTexture.size() && mTexture[e] >= 0)
            {
                mVkBindingToChannel[bn.first] = mTexture[e];
            }
        }
    }

    if (success && !mVkReflBindingSamplerNames.empty())
    {
        auto join_names = [](const std::set<std::string>& s) -> std::string
        {
            std::string r;
            for (const std::string& nm : s)
            {
                if (!r.empty())
                {
                    r += ",";
                }
                r += nm;
            }
            return r.empty() ? std::string("<none>") : r;
        };

        std::array<std::set<std::string>, MAX_VK_BINDING> refl_names;
        std::map<std::string, std::set<S32>> refl_name_to_bindings;
        for (const auto& bn : mVkReflBindingSamplerNames)
        {
            if (bn.first >= 0 && bn.first < (S32)MAX_VK_BINDING)
            {
                refl_names[bn.first].insert(bn.second);
                refl_name_to_bindings[bn.second].insert(bn.first);
            }
        }

        for (S32 b = 0; b < (S32)MAX_VK_BINDING; ++b)
        {
            if (refl_names[b].size() > 1)
            {
                LL_ERRS("BindReg") << "BindRegViolation shader=" << mName
                    << " binding=" << b
                    << " names=" << join_names(refl_names[b]) << LL_ENDL;
            }
        }
        for (const auto& nb : refl_name_to_bindings)
        {
            if (nb.second.size() > 1)
            {
                std::string binding_list;
                for (S32 bb : nb.second)
                {
                    if (!binding_list.empty())
                    {
                        binding_list += ",";
                    }
                    binding_list += std::to_string(bb);
                }
                LL_ERRS("BindReg") << "BindRegViolation shader=" << mName
                    << " name=" << nb.first
                    << " bindings=" << binding_list << LL_ENDL;
            }
        }
    }

    if (success && LLVKLoader::isVulkanInitialized())
    {
        mVkEnumBoundView.assign(mTexture.size(), VkEnumBoundView());
        mChannelToEnum.assign(LL_NUM_TEXTURE_LAYERS, -1);
        for (U32 i = 0; i < mTexture.size(); i++)
        {
            S32 ch = mTexture[i];
            if (ch > -1 && ch < (S32)mChannelToEnum.size())
            {
                if (mChannelToEnum[ch] == -1)
                {
                    mChannelToEnum[ch] = (S16)i;
                }
                else
                {
                    LL_WARNS_ONCE("BindReg") << "BindRegChannelShared shader=" << mName
                        << " channel=" << ch
                        << " kept=" << LLShaderMgr::instance()->mReservedUniforms[mChannelToEnum[ch]]
                        << " dropped=" << LLShaderMgr::instance()->mReservedUniforms[i] << LL_ENDL;
                }
            }
        }
    }

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

    if (LLVKLoader::isVulkanInitialized())
    {
        mComplete = mVkComplete;
        success = mVkComplete;
    }

    return success;
}

namespace {
    void ensureGlslangInitialized()
    {
        static const bool s_initialized = []() {
            glslang::InitializeProcess();
            return true;
        }();
        (void)s_initialized;
    }

    EShLanguage toGlslangStage(GLenum type)
    {
        switch (type)
        {
            case GL_VERTEX_SHADER:   return EShLangVertex;
            case GL_FRAGMENT_SHADER: return EShLangFragment;
            case GL_GEOMETRY_SHADER: return EShLangGeometry;
            default: return EShLangCount;
        }
    }

    struct LocationAllocator
    {
        std::set<int> mUsedFragOutSlots;
        int           mFragOutCursor = 0;

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
        if (stage_type != GL_FRAGMENT_SHADER && stage_type != GL_VERTEX_SHADER)
        {
            return source;
        }

        static const std::string kQuals =
            R"((?:(?:flat|smooth|noperspective|centroid|highp|mediump|lowp|invariant)\s+)*)";

        static const std::regex bare_out_pattern(
            R"(^(\s*)()" + kQuals + R"()out\s+()" + kQuals
            + R"()([a-zA-Z_][a-zA-Z0-9_]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)(\s*\[[^;]*\])?\s*;\s*$)"
        );
        static const std::regex bare_in_pattern(
            R"(^(\s*)()" + kQuals + R"()in\s+()" + kQuals
            + R"()([a-zA-Z_][a-zA-Z0-9_]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)(\s*\[[^;]*\])?\s*;\s*$)"
        );
        static const std::regex existing_layout_out_pattern(
            R"(^\s*layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*)" + kQuals + R"(out\b)"
        );
        static const std::regex existing_layout_in_pattern(
            R"(^\s*layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*)" + kQuals + R"(in\b)"
        );
        static const std::regex existing_layout_out_with_ident_pattern(
            R"(^\s*layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*)"
            + kQuals + R"(out\s+)" + kQuals
            + R"([a-zA-Z_][a-zA-Z0-9_]*\s+([a-zA-Z_][a-zA-Z0-9_]*)(?:\s*\[[^;]*\])?\s*;)"
        );
        static const std::regex existing_layout_in_with_ident_pattern(
            R"(^(\s*)layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*()"
            + kQuals + R"()in\s+()" + kQuals
            + R"()([a-zA-Z_][a-zA-Z0-9_]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)(\s*\[[^;]*\])?\s*;\s*$)"
        );

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
                            int slot = std::stoi(m[1].str());
                            alloc.mUsedVertOutSlots.insert(slot);
                            const std::string ident = m[2].str();
                            if (!ident.empty())
                            {
                                alloc.mVertOutIdentToSlot[ident] = slot;
                            }
                        }
                        catch (...) {  }
                    }
                    else if (std::regex_search(preline, m, existing_layout_out_pattern))
                    {
                        try { alloc.mUsedVertOutSlots.insert(std::stoi(m[1].str())); }
                        catch (...) {  }
                    }
                }
                else
                {
                    if (std::regex_search(preline, m, existing_layout_out_pattern))
                    {
                        try { alloc.mUsedFragOutSlots.insert(std::stoi(m[1].str())); }
                        catch (...) {  }
                    }
                    else if (std::regex_search(preline, m, existing_layout_in_pattern))
                    {
                        try { alloc.mUsedFragInSlots.insert(std::stoi(m[1].str())); }
                        catch (...) {  }
                    }
                }
            }
        }

        std::ostringstream out;
        std::istringstream in(source);
        std::string line;
        while (std::getline(in, line))
        {
            std::string trimmed = line;
            size_t slash = trimmed.find("//");
            if (slash != std::string::npos)
            {
                trimmed.resize(slash);
            }

            std::smatch m;

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
                        alloc.mUsedFragInSlots.insert(v_slot);
                        out << leading_ws << "layout(location=" << v_slot << ") "
                            << quals_before << "in " << quals_after
                            << type << " " << ident << array << ";\n";
                        continue;
                    }
                }
                out << line << '\n';
                continue;
            }

            if (trimmed.find("layout") != std::string::npos
                || trimmed.find('(') != std::string::npos
                || trimmed.find("uniform") != std::string::npos)
            {
                out << line << '\n';
                continue;
            }

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
                else
                {
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
                    int slot = it->second;
                    alloc.mUsedFragInSlots.insert(slot);

                    out << leading_ws << "layout(location=" << slot << ") "
                        << quals_before << "in " << quals_after
                        << type << " " << ident << array << ";\n";
                }
                else
                {
                    out << line << '\n';
                }
                continue;
            }

            out << line << '\n';
        }
        return out.str();
    }

}

static S32 reflectPushConstantMaxOffsetFromSpirv(const std::vector<unsigned int>& spirv)
{
    if (spirv.size() < 5)
    {
        return -1;
    }
    const unsigned int* w = spirv.data();
    const size_t n = spirv.size();

    unsigned int pc_pointer_type_id = 0;
    for (size_t i = 5; i < n; )
    {
        const unsigned int instr = w[i];
        const unsigned int wordCount = instr >> 16;
        const unsigned int opcode = instr & 0xFFFFu;
        if (wordCount == 0 || i + wordCount > n)
        {
            break;
        }
        if (opcode == 59u  && wordCount >= 4 && w[i + 3] == 9u )
        {
            pc_pointer_type_id = w[i + 1];
            break;
        }
        i += wordCount;
    }
    if (pc_pointer_type_id == 0)
    {
        return -1;
    }

    unsigned int pc_struct_id = 0;
    for (size_t i = 5; i < n; )
    {
        const unsigned int instr = w[i];
        const unsigned int wordCount = instr >> 16;
        const unsigned int opcode = instr & 0xFFFFu;
        if (wordCount == 0 || i + wordCount > n)
        {
            break;
        }
        if (opcode == 32u  && wordCount >= 4 && w[i + 1] == pc_pointer_type_id)
        {
            pc_struct_id = w[i + 3];
            break;
        }
        i += wordCount;
    }
    if (pc_struct_id == 0)
    {
        return -1;
    }

    S32 maxOffset = -1;
    for (size_t i = 5; i < n; )
    {
        const unsigned int instr = w[i];
        const unsigned int wordCount = instr >> 16;
        const unsigned int opcode = instr & 0xFFFFu;
        if (wordCount == 0 || i + wordCount > n)
        {
            break;
        }
        if (opcode == 72u  && wordCount >= 5
            && w[i + 1] == pc_struct_id && w[i + 3] == 35u )
        {
            const S32 off = (S32)w[i + 4];
            if (off > maxOffset)
            {
                maxOffset = off;
            }
        }
        i += wordCount;
    }
    return (maxOffset >= 0) ? maxOffset : 0;
}

struct VkSpirvSet1Sampler
{
    S32 binding;
    std::string name;
    unsigned int dim;
    unsigned int arrayed;
};

static U32 reflectVertexInputMaskFromSpirv(const std::vector<unsigned int>& spirv)
{
    if (spirv.size() < 5)
    {
        return 0;
    }
    const unsigned int* w = spirv.data();
    const size_t n = spirv.size();

    std::map<unsigned int, S32> locations;
    std::set<unsigned int> usedPointers;

    for (size_t i = 5; i < n; )
    {
        const unsigned int instr = w[i];
        const unsigned int wordCount = instr >> 16;
        const unsigned int opcode = instr & 0xFFFFu;
        if (wordCount == 0 || i + wordCount > n)
        {
            break;
        }
        if (opcode == 71u && wordCount >= 4 && w[i + 2] == 30u)
        {
            locations[w[i + 1]] = (S32)w[i + 3];
        }
        else if (opcode == 61u && wordCount >= 4)
        {
            usedPointers.insert(w[i + 3]);
        }
        else if ((opcode == 65u || opcode == 66u || opcode == 67u || opcode == 70u) && wordCount >= 4)
        {
            usedPointers.insert(w[i + 3]);
        }
        else if ((opcode == 63u || opcode == 64u) && wordCount >= 3)
        {
            usedPointers.insert(w[i + 2]);
        }
        i += wordCount;
    }

    U32 mask = 0;
    for (size_t i = 5; i < n; )
    {
        const unsigned int instr = w[i];
        const unsigned int wordCount = instr >> 16;
        const unsigned int opcode = instr & 0xFFFFu;
        if (wordCount == 0 || i + wordCount > n)
        {
            break;
        }
        if (opcode == 59u && wordCount >= 4 && w[i + 3] == 1u &&
            usedPointers.find(w[i + 2]) != usedPointers.end())
        {
            auto lit = locations.find(w[i + 2]);
            if (lit != locations.end() && lit->second >= 0 && lit->second < 32)
            {
                mask |= (1u << lit->second);
            }
        }
        i += wordCount;
    }
    return mask;
}

static void reflectUsedSet1SamplerBindingsFromSpirv(const std::vector<unsigned int>& spirv,
                                                    std::set<S32>& out_used_bindings)
{
    if (spirv.size() < 5)
    {
        return;
    }
    const unsigned int* w = spirv.data();
    const size_t n = spirv.size();

    std::map<unsigned int, S32> desc_sets;
    std::map<unsigned int, S32> desc_bindings;
    std::set<unsigned int> usedPointers;

    for (size_t i = 5; i < n; )
    {
        const unsigned int instr = w[i];
        const unsigned int wordCount = instr >> 16;
        const unsigned int opcode = instr & 0xFFFFu;
        if (wordCount == 0 || i + wordCount > n)
        {
            break;
        }
        if (opcode == 71u && wordCount >= 4 && w[i + 2] == 34u)
        {
            desc_sets[w[i + 1]] = (S32)w[i + 3];
        }
        else if (opcode == 71u && wordCount >= 4 && w[i + 2] == 33u)
        {
            desc_bindings[w[i + 1]] = (S32)w[i + 3];
        }
        else if (opcode == 61u && wordCount >= 4)
        {
            usedPointers.insert(w[i + 3]);
        }
        else if ((opcode == 65u || opcode == 66u || opcode == 67u || opcode == 70u) && wordCount >= 4)
        {
            usedPointers.insert(w[i + 3]);
        }
        else if ((opcode == 63u || opcode == 64u) && wordCount >= 3)
        {
            usedPointers.insert(w[i + 2]);
        }
        else if (opcode == 57u && wordCount >= 5)
        {
            for (unsigned int a = 4; a < wordCount; ++a)
            {
                usedPointers.insert(w[i + a]);
            }
        }
        i += wordCount;
    }

    for (size_t i = 5; i < n; )
    {
        const unsigned int instr = w[i];
        const unsigned int wordCount = instr >> 16;
        const unsigned int opcode = instr & 0xFFFFu;
        if (wordCount == 0 || i + wordCount > n)
        {
            break;
        }
        if (opcode == 59u && wordCount >= 4 && w[i + 3] == 0u)
        {
            const unsigned int result_id = w[i + 2];
            auto sit = desc_sets.find(result_id);
            auto bit = desc_bindings.find(result_id);
            if (sit != desc_sets.end() && sit->second == 1 && bit != desc_bindings.end() &&
                usedPointers.find(result_id) != usedPointers.end())
            {
                out_used_bindings.insert(bit->second);
            }
        }
        i += wordCount;
    }
}

static void reflectVkSet1BindingsFromSpirv(const std::vector<unsigned int>& spirv,
                                           std::vector<VkSpirvSet1Sampler>& out_samplers,
                                           std::vector<S32>& out_ubo_bindings)
{
    if (spirv.size() < 5)
    {
        return;
    }
    const unsigned int* w = spirv.data();
    const size_t n = spirv.size();

    std::map<unsigned int, std::string> names;
    std::map<unsigned int, S32> desc_sets;
    std::map<unsigned int, S32> desc_bindings;
    std::map<unsigned int, unsigned int> pointer_pointee;
    std::map<unsigned int, unsigned int> sampled_image_image;
    std::map<unsigned int, unsigned int> array_element;
    std::map<unsigned int, std::pair<unsigned int, unsigned int>> image_dim_arrayed;

    for (size_t i = 5; i < n; )
    {
        const unsigned int instr = w[i];
        const unsigned int wordCount = instr >> 16;
        const unsigned int opcode = instr & 0xFFFFu;
        if (wordCount == 0 || i + wordCount > n)
        {
            break;
        }
        switch (opcode)
        {
        case 5u:
            if (wordCount >= 3)
            {
                const char* s = reinterpret_cast<const char*>(&w[i + 2]);
                const size_t maxlen = (size_t)(wordCount - 2) * sizeof(unsigned int);
                size_t len = 0;
                while (len < maxlen && s[len] != '\0')
                {
                    ++len;
                }
                names[w[i + 1]] = std::string(s, len);
            }
            break;
        case 71u:
            if (wordCount >= 4 && w[i + 2] == 34u)
            {
                desc_sets[w[i + 1]] = (S32)w[i + 3];
            }
            else if (wordCount >= 4 && w[i + 2] == 33u)
            {
                desc_bindings[w[i + 1]] = (S32)w[i + 3];
            }
            break;
        case 25u:
            if (wordCount >= 6)
            {
                image_dim_arrayed[w[i + 1]] = { w[i + 3], w[i + 5] };
            }
            break;
        case 27u:
            if (wordCount >= 3)
            {
                sampled_image_image[w[i + 1]] = w[i + 2];
            }
            break;
        case 28u:
        case 29u:
            if (wordCount >= 3)
            {
                array_element[w[i + 1]] = w[i + 2];
            }
            break;
        case 32u:
            if (wordCount >= 4)
            {
                pointer_pointee[w[i + 1]] = w[i + 3];
            }
            break;
        default:
            break;
        }
        i += wordCount;
    }

    for (size_t i = 5; i < n; )
    {
        const unsigned int instr = w[i];
        const unsigned int wordCount = instr >> 16;
        const unsigned int opcode = instr & 0xFFFFu;
        if (wordCount == 0 || i + wordCount > n)
        {
            break;
        }
        if (opcode == 59u && wordCount >= 4)
        {
            const unsigned int type_id = w[i + 1];
            const unsigned int result_id = w[i + 2];
            const unsigned int storage = w[i + 3];
            auto sit = desc_sets.find(result_id);
            auto bit = desc_bindings.find(result_id);
            if (sit != desc_sets.end() && sit->second == 1 && bit != desc_bindings.end())
            {
                if (storage == 2u)
                {
                    out_ubo_bindings.push_back(bit->second);
                }
                else if (storage == 0u)
                {
                    auto pit = pointer_pointee.find(type_id);
                    if (pit != pointer_pointee.end())
                    {
                        unsigned int t = pit->second;
                        for (int guard = 0; guard < 4; ++guard)
                        {
                            auto ait = array_element.find(t);
                            if (ait == array_element.end())
                            {
                                break;
                            }
                            t = ait->second;
                        }
                        auto siit = sampled_image_image.find(t);
                        if (siit != sampled_image_image.end())
                        {
                            auto iit = image_dim_arrayed.find(siit->second);
                            if (iit != image_dim_arrayed.end())
                            {
                                std::string var_name;
                                auto nit = names.find(result_id);
                                if (nit != names.end())
                                {
                                    var_name = nit->second;
                                }
                                out_samplers.push_back({ bit->second, var_name, iit->second.first, iit->second.second });
                            }
                        }
                    }
                }
            }
        }
        i += wordCount;
    }
}

static void reflectVkUboLayoutsFromSpirv(const std::vector<unsigned int>& spirv,
                                         U8 stage_mask,
                                         std::vector<VkReflUboBlock>& out_blocks,
                                         std::vector<VkReflUboBlock>& out_push_constants)
{
    if (spirv.size() < 5)
    {
        return;
    }
    const unsigned int* w = spirv.data();
    const size_t n = spirv.size();

    std::map<unsigned int, std::string> names;
    std::map<unsigned int, std::map<unsigned int, std::string>> member_names;
    std::map<unsigned int, S32> desc_sets;
    std::map<unsigned int, S32> desc_bindings;
    std::map<unsigned int, unsigned int> array_strides;
    std::map<unsigned int, std::map<unsigned int, unsigned int>> member_offsets;
    std::map<unsigned int, std::map<unsigned int, unsigned int>> member_matrix_strides;
    std::map<unsigned int, unsigned int> const_values;
    std::map<unsigned int, unsigned int> scalar_sizes;
    std::map<unsigned int, std::pair<unsigned int, unsigned int>> vector_info;
    std::map<unsigned int, std::pair<unsigned int, unsigned int>> matrix_info;
    std::map<unsigned int, std::pair<unsigned int, unsigned int>> array_info;
    std::set<unsigned int> runtime_arrays;
    std::map<unsigned int, std::vector<unsigned int>> struct_members;
    std::map<unsigned int, unsigned int> pointer_pointee;

    auto read_string = [&](size_t word_idx, unsigned int word_count_avail) -> std::string
    {
        const char* s = reinterpret_cast<const char*>(&w[word_idx]);
        const size_t maxlen = (size_t)word_count_avail * sizeof(unsigned int);
        size_t len = 0;
        while (len < maxlen && s[len] != '\0')
        {
            ++len;
        }
        return std::string(s, len);
    };

    for (size_t i = 5; i < n; )
    {
        const unsigned int instr = w[i];
        const unsigned int wordCount = instr >> 16;
        const unsigned int opcode = instr & 0xFFFFu;
        if (wordCount == 0 || i + wordCount > n)
        {
            break;
        }
        switch (opcode)
        {
        case 5u:
            if (wordCount >= 3)
            {
                names[w[i + 1]] = read_string(i + 2, wordCount - 2);
            }
            break;
        case 6u:
            if (wordCount >= 4)
            {
                member_names[w[i + 1]][w[i + 2]] = read_string(i + 3, wordCount - 3);
            }
            break;
        case 71u:
            if (wordCount >= 4 && w[i + 2] == 34u)
            {
                desc_sets[w[i + 1]] = (S32)w[i + 3];
            }
            else if (wordCount >= 4 && w[i + 2] == 33u)
            {
                desc_bindings[w[i + 1]] = (S32)w[i + 3];
            }
            else if (wordCount >= 4 && w[i + 2] == 6u)
            {
                array_strides[w[i + 1]] = w[i + 3];
            }
            break;
        case 72u:
            if (wordCount >= 5 && w[i + 3] == 35u)
            {
                member_offsets[w[i + 1]][w[i + 2]] = w[i + 4];
            }
            else if (wordCount >= 5 && w[i + 3] == 7u)
            {
                member_matrix_strides[w[i + 1]][w[i + 2]] = w[i + 4];
            }
            break;
        case 43u:
            if (wordCount >= 4)
            {
                const_values[w[i + 2]] = w[i + 3];
            }
            break;
        case 20u:
            if (wordCount >= 2)
            {
                scalar_sizes[w[i + 1]] = 4u;
            }
            break;
        case 21u:
        case 22u:
            if (wordCount >= 3)
            {
                scalar_sizes[w[i + 1]] = w[i + 2] / 8u;
            }
            break;
        case 23u:
            if (wordCount >= 4)
            {
                vector_info[w[i + 1]] = { w[i + 2], w[i + 3] };
            }
            break;
        case 24u:
            if (wordCount >= 4)
            {
                matrix_info[w[i + 1]] = { w[i + 2], w[i + 3] };
            }
            break;
        case 28u:
            if (wordCount >= 4)
            {
                array_info[w[i + 1]] = { w[i + 2], w[i + 3] };
            }
            break;
        case 29u:
            runtime_arrays.insert(w[i + 1]);
            break;
        case 30u:
            {
                std::vector<unsigned int>& members = struct_members[w[i + 1]];
                for (unsigned int m = 2; m < wordCount; ++m)
                {
                    members.push_back(w[i + m]);
                }
            }
            break;
        case 32u:
            if (wordCount >= 4)
            {
                pointer_pointee[w[i + 1]] = w[i + 3];
            }
            break;
        default:
            break;
        }
        i += wordCount;
    }

    auto compute_member_size = [&](unsigned int type_id, unsigned int matrix_stride) -> U32
    {
        auto sit = scalar_sizes.find(type_id);
        if (sit != scalar_sizes.end())
        {
            return (U32)sit->second;
        }
        auto vit = vector_info.find(type_id);
        if (vit != vector_info.end())
        {
            auto cit = scalar_sizes.find(vit->second.first);
            return (cit != scalar_sizes.end()) ? (U32)(vit->second.second * cit->second) : 0u;
        }
        auto mit = matrix_info.find(type_id);
        if (mit != matrix_info.end())
        {
            return (matrix_stride != 0) ? (U32)(mit->second.second * matrix_stride) : 0u;
        }
        auto ait = array_info.find(type_id);
        if (ait != array_info.end())
        {
            auto stride_it = array_strides.find(type_id);
            auto len_it = const_values.find(ait->second.second);
            if (stride_it != array_strides.end() && len_it != const_values.end())
            {
                return (U32)(stride_it->second * len_it->second);
            }
            return 0u;
        }
        return 0u;
    };

    auto build_block = [&](unsigned int struct_id, S32 set, S32 binding, std::vector<VkReflUboBlock>& out)
    {
        VkReflUboBlock block;
        block.set = set;
        block.binding = binding;
        block.stage_mask = stage_mask;
        auto nit = names.find(struct_id);
        block.block_name = (nit != names.end()) ? nit->second : std::string();
        block.block_size = 0;

        auto smit = struct_members.find(struct_id);
        if (smit != struct_members.end())
        {
            const auto& offs = member_offsets[struct_id];
            const auto& mstrides = member_matrix_strides[struct_id];
            const auto& mnames = member_names[struct_id];
            bool size_complete = true;
            U32 max_end = 0;
            for (unsigned int idx = 0; idx < (unsigned int)smit->second.size(); ++idx)
            {
                auto oit = offs.find(idx);
                if (oit == offs.end())
                {
                    continue;
                }
                VkReflUboMember member;
                auto mnit = mnames.find(idx);
                member.name = (mnit != mnames.end()) ? mnit->second : std::string();
                member.offset = (U32)oit->second;
                unsigned int mstride = 0;
                auto msit = mstrides.find(idx);
                if (msit != mstrides.end())
                {
                    mstride = msit->second;
                }
                member.size = compute_member_size(smit->second[idx], mstride);
                if (member.size == 0)
                {
                    size_complete = false;
                }
                else if (member.offset + member.size > max_end)
                {
                    max_end = member.offset + member.size;
                }
                block.members.push_back(member);
            }
            block.block_size = size_complete ? max_end : 0u;
        }

        for (VkReflUboBlock& existing : out)
        {
            if (existing.set == block.set && existing.binding == block.binding
                && existing.block_name == block.block_name)
            {
                bool same_layout = (existing.members.size() == block.members.size());
                if (same_layout)
                {
                    for (size_t k = 0; k < block.members.size(); ++k)
                    {
                        if (existing.members[k].name != block.members[k].name
                            || existing.members[k].offset != block.members[k].offset
                            || existing.members[k].size != block.members[k].size)
                        {
                            same_layout = false;
                            break;
                        }
                    }
                }
                if (same_layout)
                {
                    existing.stage_mask |= stage_mask;
                    return;
                }
            }
        }
        out.push_back(std::move(block));
    };

    for (size_t i = 5; i < n; )
    {
        const unsigned int instr = w[i];
        const unsigned int wordCount = instr >> 16;
        const unsigned int opcode = instr & 0xFFFFu;
        if (wordCount == 0 || i + wordCount > n)
        {
            break;
        }
        if (opcode == 59u && wordCount >= 4)
        {
            const unsigned int type_id = w[i + 1];
            const unsigned int result_id = w[i + 2];
            const unsigned int storage = w[i + 3];
            auto pit = pointer_pointee.find(type_id);
            if (pit != pointer_pointee.end())
            {
                if (storage == 2u)
                {
                    auto sit = desc_sets.find(result_id);
                    auto bit = desc_bindings.find(result_id);
                    if (sit != desc_sets.end() && bit != desc_bindings.end()
                        && (sit->second == 0 || sit->second == 1))
                    {
                        build_block(pit->second, sit->second, bit->second, out_blocks);
                    }
                }
                else if (storage == 9u)
                {
                    build_block(pit->second, -1, -1, out_push_constants);
                }
            }
        }
        i += wordCount;
    }
}

bool LLGLSLShader::generatePerProgramSPIRV(const std::vector<StageSource>& stages)
{
    if (stages.empty())
    {
        return false;
    }

    LLShaderMgr* mgr = LLShaderMgr::instance();

    HBXXH128 program_hash_obj;
    program_hash_obj.update(std::string("vulkanize:v5_p2_inout_pair_prepass_group_fix"));
    program_hash_obj.update(std::string("auto_loc=1"));
    program_hash_obj.update(std::string("spv_debug_names=1"));
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

    std::map<GLenum, std::vector<size_t>> stages_by_type;
    for (size_t i = 0; i < stages.size(); ++i)
    {
        stages_by_type[stages[i].type].push_back(i);
    }

    struct StageSpv { GLenum type; std::vector<unsigned int> spirv; };
    std::vector<StageSpv> stage_spvs;
    bool cache_hit = false;

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

    if (!cache_hit)
    {
        ensureGlslangInitialized();

        std::vector<std::unique_ptr<glslang::TShader>> tshaders;
        std::vector<std::string> concat_buffers;
        std::vector<EShLanguage> stage_langs_in_order;
        std::vector<GLenum> stage_types_in_order;

        concat_buffers.reserve(stages_by_type.size());

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

            EShLanguage lang = toGlslangStage(stage_type);
            if (lang == EShLangCount)
            {
                return false;
            }

            std::string concatenated;
            concatenated.append("#version 460\n");
            concatenated.append("#extension GL_KHR_vulkan_glsl : enable\n");
            concatenated.append("#define LL_VULKAN_GLSL 1\n");

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
                        continue;
                    }
                    const std::vector<std::string>& util_sources = it->second;
                    for (size_t i = 1; i < util_sources.size(); ++i)
                    {
                        concatenated.append(util_sources[i]);
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
                return false;
            }

            concatenated = vulkanizeStageSource(concatenated, stage_type, alloc);

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
                LL_WARNS("Vulkan") << "generatePerProgramSPIRV: glslang parse failed for '"
                                   << mName << "' stage "
                                   << (stage_type == GL_VERTEX_SHADER ? "V" :
                                       stage_type == GL_FRAGMENT_SHADER ? "F" : "G")
                                   << "\n" << shader->getInfoLog()
                                   << "\n" << shader->getInfoDebugLog() << LL_ENDL;
                return false;
            }

            tshaders.push_back(std::move(shader));
            stage_langs_in_order.push_back(lang);
            stage_types_in_order.push_back(stage_type);
        }


        glslang::TProgram program;
        for (auto& ts : tshaders)
        {
            program.addShader(ts.get());
        }
        EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);
        if (!program.link(messages))
        {
            LL_WARNS("Vulkan") << "generatePerProgramSPIRV: glslang link failed for '"
                               << mName << "'\n" << program.getInfoLog()
                               << "\n" << program.getInfoDebugLog() << LL_ENDL;
            return false;
        }

        glslang::SpvOptions spv_options;
        spv_options.generateDebugInfo = false;
        spv_options.stripDebugInfo    = false;
        spv_options.disableOptimizer  = false;
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
                LL_WARNS("Vulkan") << "generatePerProgramSPIRV: GlslangToSpv produced empty SPIR-V for '"
                                   << mName << "' stage "
                                   << (ss.type == GL_VERTEX_SHADER ? "V" :
                                       ss.type == GL_FRAGMENT_SHADER ? "F" : "G")
                                   << LL_ENDL;
                return false;
            }
            stage_spvs.push_back(std::move(ss));
        }

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
            mVkVertexShaderModulesPerProgram[stage.file_name] = vk_module;
        }
        else if (stage.type == GL_FRAGMENT_SHADER)
        {
            mVkFragmentShaderModulesPerProgram[stage.file_name] = vk_module;
        }
        else if (stage.type == GL_GEOMETRY_SHADER)
        {
            mVkGeometryShaderModulesPerProgram[stage.file_name] = vk_module;
        }
    }

    mVkVertexPushConstantOver64 = false;
    for (const auto& ss : stage_spvs)
    {
        if (ss.type == GL_VERTEX_SHADER)
        {
            const S32 vpc_max = reflectPushConstantMaxOffsetFromSpirv(ss.spirv);
            mVkVertexPushConstantOver64 = (vpc_max >= 64);
        }
    }

    mVkAttributeMask = 0;
    mVkAttributeMaskValid = false;
    for (const auto& ss : stage_spvs)
    {
        if (ss.type == GL_VERTEX_SHADER)
        {
            mVkAttributeMask = reflectVertexInputMaskFromSpirv(ss.spirv);
            mVkAttributeMaskValid = true;
        }
    }

    for (const auto& ss : stage_spvs)
    {
        const U8 stage_mask = (ss.type == GL_FRAGMENT_SHADER) ? VKBS_FRAGMENT
                            : (ss.type == GL_VERTEX_SHADER)   ? VKBS_VERTEX : (U8)0;
        std::vector<VkSpirvSet1Sampler> samplers;
        std::vector<S32> ubo_bindings;
        reflectVkSet1BindingsFromSpirv(ss.spirv, samplers, ubo_bindings);
        reflectVkUboLayoutsFromSpirv(ss.spirv, stage_mask, mVkReflUboBlocks, mVkReflPushConstants);
        std::set<S32> used_sampler_bindings;
        reflectUsedSet1SamplerBindingsFromSpirv(ss.spirv, used_sampler_bindings);
        for (S32 ub : used_sampler_bindings)
        {
            if (ub >= 0 && ub < (S32)MAX_VK_BINDING)
            {
                mVkBindingSamplerUsed[ub] = true;
            }
        }
        for (const auto& smp : samplers)
        {
            if (smp.binding < 0 || smp.binding >= (S32)MAX_VK_BINDING)
            {
                continue;
            }
            U8& t = mVkBindingDeclaredType[smp.binding];
            t = (t == VKBD_NONE || t == VKBD_SAMPLER) ? (U8)VKBD_SAMPLER : (U8)VKBD_BOTH;
            mVkBindingStageMask[smp.binding] |= stage_mask;
            U8 dim = VKSD_2D;
            if (smp.dim == 3u)
            {
                dim = smp.arrayed ? (U8)VKSD_CUBE_ARRAY : (U8)VKSD_CUBE;
            }
            else if (smp.dim == 2u)
            {
                dim = VKSD_3D;
            }
            mVkBindingSamplerDim[smp.binding] = dim;
            mVkReflBindingSamplerNames.emplace_back(smp.binding, smp.name);
        }
        for (S32 ub : ubo_bindings)
        {
            if (ub < 0 || ub >= (S32)MAX_VK_BINDING)
            {
                continue;
            }
            U8& t = mVkBindingDeclaredType[ub];
            t = (t == VKBD_NONE || t == VKBD_UBO) ? (U8)VKBD_UBO : (U8)VKBD_BOTH;
            mVkBindingStageMask[ub] |= stage_mask;
        }
    }

    return true;
}

bool LLGLSLShader::attachVertexObject(std::string object_path)
{
    if (LLVKLoader::isVulkanInitialized() && LLShaderMgr::instance()->mVertexShaderSourceCache.count(object_path) > 0)
    {
        mVulkanAttachedVertexUtilities.push_back(object_path);
    }
    if (LLShaderMgr::instance()->mVertexShaderSourceCache.count(object_path) > 0)
    {
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
    if (LLVKLoader::isVulkanInitialized() && LLShaderMgr::instance()->mFragmentShaderSourceCache.count(object_path) > 0)
    {
        mVulkanAttachedFragmentUtilities.push_back(object_path);
    }

    if (LLShaderMgr::instance()->mFragmentShaderSourceCache.count(object_path) > 0)
    {
        return true;
    }
    else
    {
        LL_SHADER_LOADING_WARNS() << "Attempting to attach shader object: '" << object_path << "' that hasn't been compiled." << LL_ENDL;
        return false;
    }
}

bool LLGLSLShader::mapAttributes()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    return true;
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

bool LLGLSLShader::mapUniforms()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    bool res = true;

    mActiveTextureChannels = 0;
    mTexture.clear();
    mTexture.resize(LLShaderMgr::instance()->mReservedUniforms.size(), -1);

    mVkReflEnumChannel.clear();
    if (LLVKLoader::isVulkanInitialized() && !mVkReflBindingSamplerNames.empty())
    {
        const std::vector<std::string>& reserved = LLShaderMgr::instance()->mReservedUniforms;
        std::set<S32> refl_enums;
        for (const auto& bn : mVkReflBindingSamplerNames)
        {
            if (bn.first < 0 || bn.first >= (S32)MAX_VK_BINDING)
            {
                continue;
            }
            if (!mVkBindingSamplerUsed[bn.first])
            {
                continue;
            }
            for (S32 ri = 0; ri < (S32)reserved.size(); ++ri)
            {
                if (reserved[ri] == bn.second)
                {
                    refl_enums.insert(ri);
                    break;
                }
            }
        }

        mVkReflEnumChannel.assign(reserved.size(), -1);
        S32 next_ch = 0;
        const S32 diffuse_enum = LLShaderMgr::DIFFUSE_MAP;
        if (refl_enums.count(diffuse_enum))
        {
            mVkReflEnumChannel[diffuse_enum] = 0;
            next_ch = 1;
        }
        for (S32 e : refl_enums)
        {
            if (e == diffuse_enum)
            {
                continue;
            }
            mVkReflEnumChannel[e] = next_ch++;
        }

        for (U32 i = 0; i < mTexture.size() && i < mVkReflEnumChannel.size(); ++i)
        {
            mTexture[i] = mVkReflEnumChannel[i];
            if (mTexture[i] > -1)
            {
                mActiveTextureChannels = llmax(mActiveTextureChannels, mTexture[i] + 1);
            }
        }
    }

    return res;
}

void LLGLSLShader::bind()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    gGL.flush();

    if (sCurBoundShaderPtr != this)  // Don't re-bind current shader
    {
        if (sCurBoundShaderPtr)
        {
            sCurBoundShaderPtr->readProfileQuery();
        }
        LLVertexBuffer::unbind();
        sCurBoundShaderPtr = this;
        placeProfileQuery();

        sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;

        if (LLVKLoader::isVulkanInitialized() && mVkPipelineLayout != VK_NULL_HANDLE)
        {
            VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
            if (cmd != VK_NULL_HANDLE)
            {
                VkPipeline pipeline = getOrCreateVkPipelineForBoundRT();
                if (pipeline != VK_NULL_HANDLE)
                {
                    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                }
            }
        }

        if (LLVKLoader::isVulkanInitialized() && !mChannelToEnum.empty())
        {
            const S32 snap_count = llmin(mActiveTextureChannels, (S32)mChannelToEnum.size());
            for (S32 ch = 0; ch < snap_count; ++ch)
            {
                vkCaptureChannelBoundView(ch);
            }
        }
    }

    llassert_always(sCurBoundShaderPtr != nullptr);
    llassert_always(sCurBoundShaderPtr == this);
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

    sCurBoundShaderPtr = NULL;
}

void LLGLSLShader::vkCaptureEnumBoundView(S32 uniform_enum, S32 channel)
{
    if (uniform_enum < 0 || uniform_enum >= (S32)mVkEnumBoundView.size())
    {
        return;
    }
    LLTexUnit* tu = gGL.getTexUnit(channel);
    if (tu == nullptr)
    {
        return;
    }
    VkEnumBoundView& e = mVkEnumBoundView[uniform_enum];
    e.imagep        = tu->mCurrImageGL;
    e.cubep         = tu->mCurrCubeMap;
    e.rtp           = tu->mCurrRenderTarget;
    e.rt_attachment = tu->mCurrRTAttachment;
    e.rt_depth      = tu->mCurrRTDepth;
    e.sampler       = tu->getLiveVkSampler();
    e.bound         = (e.imagep.notNull() || e.cubep.notNull() || e.rtp != nullptr);
}

void LLGLSLShader::vkCaptureChannelBoundView(S32 channel)
{
    if (channel < 0 || channel >= (S32)mChannelToEnum.size())
    {
        return;
    }
    S32 e = mChannelToEnum[channel];
    if (e < 0)
    {
        return;
    }
    vkCaptureEnumBoundView(e, channel);
}

VkImageView LLGLSLShader::vkResolveEnumBoundView(S32 uniform_enum) const
{
    if (uniform_enum < 0 || uniform_enum >= (S32)mVkEnumBoundView.size())
    {
        return VK_NULL_HANDLE;
    }
    const VkEnumBoundView& e = mVkEnumBoundView[uniform_enum];
    if (e.rtp != nullptr)
    {
        if (e.rtp == LLRenderTarget::getCurrentBoundTarget())
        {
            return VK_NULL_HANDLE;
        }
        e.rtp->bindForShaderRead(e.rt_attachment, e.rt_depth);
        if (e.rt_depth)
        {
            return e.rtp->hasVkDepth() ? e.rtp->getVkDepthView() : VK_NULL_HANDLE;
        }
        return e.rtp->hasVkImage(e.rt_attachment) ? e.rtp->getVkImageView(e.rt_attachment)
                                                  : VK_NULL_HANDLE;
    }
    if (e.cubep.notNull() && e.cubep->hasVkCubeImage())
    {
        return e.cubep->getVkCubeImageView();
    }
    if (e.imagep.notNull() && e.imagep->hasVkImage())
    {
        return e.imagep->getVkImageView();
    }
    return VK_NULL_HANDLE;
}

U8 LLGLSLShader::vkResolveEnumBoundDim(S32 uniform_enum) const
{
    if (uniform_enum < 0 || uniform_enum >= (S32)mVkEnumBoundView.size())
    {
        return VKSD_2D;
    }
    const VkEnumBoundView& e = mVkEnumBoundView[uniform_enum];
    if (e.rtp != nullptr)
    {
        return VKSD_2D;
    }
    if (e.cubep.notNull() && e.cubep->hasVkCubeImage())
    {
        return VKSD_CUBE;
    }
    if (e.imagep.notNull() && e.imagep->hasVkImage())
    {
        switch (e.imagep->getTarget())
        {
        case LLTexUnit::TT_CUBE_MAP:       return VKSD_CUBE;
        case LLTexUnit::TT_CUBE_MAP_ARRAY: return VKSD_CUBE_ARRAY;
        case LLTexUnit::TT_TEXTURE_3D:     return VKSD_3D;
        default:                           return VKSD_2D;
        }
    }
    return VKSD_2D;
}

void LLGLSLShader::vkWarnL3Fallback(LLGLSLShader* shader, U32 binding, S32 enum_value, VkImageView old_view)
{
    if (shader == nullptr || enum_value < 0 || old_view == VK_NULL_HANDLE)
    {
        return;
    }
    static std::set<std::pair<const void*, U32>> logged_sites;
    if (!logged_sites.insert(std::make_pair((const void*)shader, binding)).second)
    {
        return;
    }
    const std::vector<std::string>& reserved = LLShaderMgr::instance()->mReservedUniforms;
    std::string ename = (enum_value < (S32)reserved.size()) ? reserved[enum_value] : std::string();
    LL_WARNS("BindReg") << "BindRegFallback shader=" << shader->mName
        << " binding=" << binding
        << " enum=" << enum_value << "(" << ename << ")" << LL_ENDL;
}

S32 LLGLSLShader::bindTexture(S32 uniform, LLTexture* texture, LLTexUnit::eTextureType mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mTexture.size() << " index: " << uniform << LL_ENDL;
        llassert(false);
        return -1;
    }

    S32 uniform_enum = uniform;
    uniform = mTexture[uniform];

    if (uniform > -1)
    {
        gGL.getTexUnit(uniform)->bindFast(texture);
        if (LLVKLoader::isVulkanInitialized())
        {
            vkCaptureEnumBoundView(uniform_enum, uniform);
        }
    }

    return uniform;
}

S32 LLGLSLShader::bindTexture(S32 uniform, LLRenderTarget* texture, bool depth, LLTexUnit::eTextureFilterOptions mode, U32 index)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mTexture.size() << " index: " << uniform << LL_ENDL;
        llassert(false);
        return -1;
    }

    S32 uniform_enum = uniform;
    uniform = getTextureChannel(uniform);

    if (uniform > -1)
    {
        if (depth) {
            gGL.getTexUnit(uniform)->bind(texture, true);
        }
        else {
            bool has_mips = mode == LLTexUnit::TFO_TRILINEAR || mode == LLTexUnit::TFO_ANISOTROPIC;
            gGL.getTexUnit(uniform)->bindManual(texture->getUsage(), 0, has_mips);
        }

        gGL.getTexUnit(uniform)->setTextureFilteringOption(mode);
        gGL.getTexUnit(uniform)->setTextureAddressMode(LLTexUnit::TAM_WRAP);

        LLTexUnit* rt_tu = gGL.getTexUnit(uniform);
        rt_tu->mCurrRenderTarget = texture;
        rt_tu->mCurrRTAttachment = index;
        rt_tu->mCurrRTDepth      = depth;
        rt_tu->mCurrImageGL      = nullptr;

        if (LLVKLoader::isVulkanInitialized())
        {
            vkCaptureEnumBoundView(uniform_enum, uniform);
        }
    }

    return uniform;
}

S32 LLGLSLShader::unbindTexture(S32 uniform, LLTexUnit::eTextureType mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mTexture.size() << " index: " << uniform << LL_ENDL;
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
        LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mTexture.size() << " index: " << uniform << LL_ENDL;
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
        LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mTexture.size() << " index: " << uniform << LL_ENDL;
        llassert(false);
        return -1;
    }

    S32 index = mTexture[uniform];
    if (index < 0)
    {
        return index;
    }

    LLTexUnit* tex_unit = gGL.getTexUnit(index);
    if (!tex_unit)
    {
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

bool LLGLSLShader::hasReflectedUniform(S32 reserved_enum) const
{
    const std::vector<std::string>& reserved = LLShaderMgr::instance()->mReservedUniforms;
    if (reserved_enum < 0 || reserved_enum >= (S32)reserved.size())
    {
        return false;
    }
    const std::string& name = reserved[reserved_enum];

    for (const VkReflUboBlock& pc : mVkReflPushConstants)
    {
        for (const VkReflUboMember& m : pc.members)
        {
            if (m.name == name)
            {
                return true;
            }
        }
    }
    for (const VkReflUboBlock& ubo : mVkReflUboBlocks)
    {
        for (const VkReflUboMember& m : ubo.members)
        {
            if (m.name == name)
            {
                return true;
            }
        }
    }
    for (const std::pair<S32, std::string>& bn : mVkReflBindingSamplerNames)
    {
        if (bn.second == name)
        {
            return true;
        }
    }
    if (reserved_enum < (S32)mTexture.size() && mTexture[reserved_enum] >= 0)
    {
        return true;
    }
    return false;
}

void LLGLSLShader::pushGaussianFragPC(F32 resScale, F32 dirX, F32 dirY)
{
    if (LLVKLoader::isVulkanInitialized() && mVkPipelineLayout != VK_NULL_HANDLE)
    {
        VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
        if (cmd != VK_NULL_HANDLE)
        {
            const F32 gaussian_pc[4] = { resScale, 0.f, dirX, dirY };
            vkCmdPushConstants(cmd, mVkPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                               LLVkUboReg::PC_OFF_GAUSSIAN_RES_SCALE, sizeof(gaussian_pc), gaussian_pc);
        }
    }
}

void LLGLSLShader::setMinimumAlpha(F32 minimum)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    gGL.flush();

    if (LLVKLoader::isVulkanInitialized() && mVkPipelineLayout != VK_NULL_HANDLE)
    {
        VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
        if (cmd != VK_NULL_HANDLE)
        {
            const F32 minimum_alpha_pc = minimum;
            vkCmdPushConstants(cmd, mVkPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                               LLVkUboReg::PC_OFF_MINIMUM_ALPHA, sizeof(F32), &minimum_alpha_pc);
        }
    }

    if (LLVKLoader::isVulkanInitialized() && mWritePerProgramUBOMinimumAlpha
        && mVkPerProgramUBO != VK_NULL_HANDLE && mVkActivePerProgramUBOMapped != nullptr)
    {
        std::memcpy(mVkActivePerProgramUBOMapped, &minimum, sizeof(F32));
        ++mVkPerProgramUBOGeneration;
        if (sCurBoundShaderPtr == this)
        {
            sCurPerCallVkOffsetsDirty = true;
        }
    }
}

void LLGLSLShader::setObjectAlpha(F32 object_alpha)
{
    if (LLVKLoader::isVulkanInitialized() && mVkPipelineLayout != VK_NULL_HANDLE)
    {
        VkCommandBuffer cmd = LLVKLoader::getCurrentCommandBuffer();
        if (cmd != VK_NULL_HANDLE)
        {
            const F32 object_alpha_pc = object_alpha;
            vkCmdPushConstants(cmd, mVkPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                               LLVkUboReg::PC_OFF_OBJECT_ALPHA, sizeof(F32), &object_alpha_pc);
        }
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
    hash_obj.update(&LLShaderMgr::sCinematicMode, sizeof(LLShaderMgr::sCinematicMode));
    hash_obj.update(gGLManager.mGLVendor);
    hash_obj.update(gGLManager.mGLRenderer);
    hash_obj.update(gGLManager.mGLVersionString);
    return hash_obj.digest();
}

#if LL_PROFILER_ENABLE_RENDER_DOC
void LLGLSLShader::setLabel(const char* label) {
}
#endif

bool LLGLSLShader::createVkPipeline(U32 perProgramUBOSize, bool needsSharedWaterVUBO)
{
    if (!LLVKLoader::isVulkanInitialized())
    {
        return false;
    }

    VkDevice dev = LLVKLoader::getDevice();
    if (dev == VK_NULL_HANDLE)
    {
        return false;
    }

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(64);

    mVkBindingToEnum.fill(-1);
    mVkBindingToUBOAccessor.fill(nullptr);

    auto add_ubo = [&](U32 binding, VkShaderStageFlags stage, LLGLSLShader::SharedUBOAccessor accessor,
                       VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
    {
        VkDescriptorSetLayoutBinding b = {};
        b.binding         = binding;
        b.descriptorType  = type;
        b.descriptorCount = 1;
        b.stageFlags      = stage;
        bindings.push_back(b);

        if (binding < MAX_VK_BINDING)
        {
            mVkBindingToUBOAccessor[binding] = accessor;
        }
    };
    auto add_sampler = [&](U32 binding, VkShaderStageFlags stage, S32 enum_value, U8 sampler_dim = VKSD_2D)
    {
        VkDescriptorSetLayoutBinding b = {};
        b.binding         = binding;
        b.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        b.descriptorCount = 1;
        b.stageFlags      = stage;
        bindings.push_back(b);

        if (binding < MAX_VK_BINDING)
        {
            mVkBindingToEnum[binding] = enum_value;
            if (sampler_dim != VKSD_2D)
            {
                mVkBindingSamplerDim[binding] = sampler_dim;
            }
        }
    };

    add_ubo    (0,  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr,
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    add_sampler(1,  VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::DIFFUSE_MAP);
    add_sampler(2,  VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::BUMP_MAP);
    add_sampler(3,  VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::SPECULAR_MAP);
    if (mFeatures.usesSMAABlendWeights)
    {
        add_ubo    (4, VK_SHADER_STAGE_FRAGMENT_BIT, LLVKLoader::getSharedSMAABlendWeightsFUBO);
    }
    else
    {
        add_sampler(4, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::DEFERRED_DEPTH);
    }
    add_sampler(5,  VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::WATER_EXCLUSIONTEX);
    add_ubo    (7,  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);
    add_ubo    (8,  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, LLVKLoader::getSharedWindlightAtmosUBO);
    add_ubo    (9,  VK_SHADER_STAGE_VERTEX_BIT, LLVKLoader::getSharedWindlightSkyUBO);
    add_ubo    (10, VK_SHADER_STAGE_FRAGMENT_BIT, LLVKLoader::getSharedWindlightHDRUBO);
    add_ubo    (11, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, LLVKLoader::getSharedWindlightLightUBO);
    if (mFeatures.isSpecular && LLShaderMgr::sSumLightsClass >= 3)
    {
        add_ubo(12, VK_SHADER_STAGE_VERTEX_BIT, LLVKLoader::getSharedLightsSpecularUBO);
    }
    else
    {
        add_ubo(12, VK_SHADER_STAGE_VERTEX_BIT, LLVKLoader::getSharedLightsUBO);
    }
    add_ubo    (14, VK_SHADER_STAGE_FRAGMENT_BIT, LLVKLoader::getSharedWaterFogUBO);
    if (needsSharedWaterVUBO)
    {
        add_ubo(15, VK_SHADER_STAGE_VERTEX_BIT, LLVKLoader::getSharedWaterVUBO);
    }
    add_ubo    (16, VK_SHADER_STAGE_FRAGMENT_BIT, LLVKLoader::getSharedReflectionProbeUBO);
    add_sampler(17, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::ENVIRONMENT_MAP);
    add_ubo    (18, VK_SHADER_STAGE_FRAGMENT_BIT, LLVKLoader::getSharedGlobalFUBO);
    add_sampler(19, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::DEFERRED_DIFFUSE);
    add_sampler(20, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::DEFERRED_SPECULAR);
    add_sampler(21, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::DEFERRED_EMISSIVE);
    add_ubo    (22, VK_SHADER_STAGE_FRAGMENT_BIT, LLVKLoader::getSharedAoUtilUBO);
    add_sampler(23, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::DEFERRED_NOISE);
    add_sampler(24, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::DEFERRED_DEPTH);
    add_sampler(25, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::EXPOSURE_MAP);
    add_ubo    (26, VK_SHADER_STAGE_FRAGMENT_BIT, LLVKLoader::getSharedTonemapUtilFUBO);
    add_sampler(27, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::NORMAL_MAP);
    if (mFeatures.isPBRTerrain)
    {
        add_ubo    (28, VK_SHADER_STAGE_FRAGMENT_BIT, LLVKLoader::getSharedPbrTerrainFUBO);
        add_ubo    (52, VK_SHADER_STAGE_VERTEX_BIT,   LLVKLoader::getSharedPbrTerrainUBO);
    }
    else
    {
        add_sampler(28, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::DEFERRED_PROJECTION);
    }
    add_sampler(29, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::DEFERRED_BRDF_LUT);
    add_ubo    (30, VK_SHADER_STAGE_FRAGMENT_BIT, LLVKLoader::getSharedDeferredUtilUBO);
    add_ubo    (31, VK_SHADER_STAGE_FRAGMENT_BIT, LLVKLoader::getSharedShadowUtilUBO);
    add_sampler(32, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::DEFERRED_SHADOW0);
    add_sampler(33, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::DEFERRED_SHADOW1);
    add_sampler(34, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::DEFERRED_SHADOW2);
    add_sampler(35, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::DEFERRED_SHADOW3);
    add_sampler(36, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::DEFERRED_SHADOW4);
    add_sampler(37, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::DEFERRED_SHADOW5);
    add_ubo    (38, VK_SHADER_STAGE_FRAGMENT_BIT, LLVKLoader::getSharedReflectionProbesUBO);
    add_sampler(43, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::SCENE_MAP);
    add_ubo    (44, VK_SHADER_STAGE_VERTEX_BIT, nullptr);
    add_ubo    (45, VK_SHADER_STAGE_VERTEX_BIT, LLVKLoader::getSharedAvatarSkinUBO,
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    add_ubo    (46, VK_SHADER_STAGE_VERTEX_BIT, LLVKLoader::getSharedObjectSkinUBO,
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    add_sampler(47, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::SCENE_DEPTH);
    if (mFeatures.hasReflectionProbes)
    {
        add_sampler(40, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::REFLECTION_PROBES, VKSD_CUBE_ARRAY);
        add_sampler(41, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::IRRADIANCE_PROBES, VKSD_CUBE_ARRAY);
        add_ubo    (39, VK_SHADER_STAGE_FRAGMENT_BIT, LLVKLoader::getSharedReflectionProbeFUBO);
    }
    {
        auto hero_it = sGlobalDefines.find("HERO_PROBES");
        bool hero_probes_enabled = (hero_it != sGlobalDefines.end() && hero_it->second == "1");
        if (hero_probes_enabled && mFeatures.hasReflectionProbes)
        {
            add_sampler(42, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::HERO_PROBE, VKSD_CUBE_ARRAY);
        }
    }
    add_ubo    (48, VK_SHADER_STAGE_VERTEX_BIT, LLVKLoader::getSharedPBRMaterialUBO,
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    add_ubo    (49, VK_SHADER_STAGE_FRAGMENT_BIT, LLVKLoader::getSharedSSRUtilUBO);
    add_ubo    (51, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, LLVKLoader::getSharedDrawColorUBO,
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    add_ubo    (53, VK_SHADER_STAGE_VERTEX_BIT, LLVKLoader::getSharedShadowParamsUBO,
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
    add_sampler(50, VK_SHADER_STAGE_FRAGMENT_BIT, LLShaderMgr::DEFERRED_LIGHTFUNC);

    if (mFeatures.mIndexedTextureChannels > 0)
    {
        for (S32 i = 0; i < mFeatures.mIndexedTextureChannels && i < 8; ++i)
        {
            add_sampler(100 + i, VK_SHADER_STAGE_FRAGMENT_BIT, -2);
        }
    }

    for (auto& b : bindings)
    {
        if (b.binding >= MAX_VK_BINDING) continue;
        U8 declared = mVkBindingDeclaredType[b.binding];
        if (declared == VKBD_SAMPLER && (b.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                         || b.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC))
        {
            b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            mVkBindingToUBOAccessor[b.binding] = nullptr;
        }
        else if (declared == VKBD_UBO && b.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        {
            b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            mVkBindingToEnum[b.binding] = -1;
        }
    }
    {
        std::array<bool, MAX_VK_BINDING> present = {};
        for (const auto& b : bindings) { if (b.binding < MAX_VK_BINDING) present[b.binding] = true; }
        for (U32 bnd = 0; bnd < MAX_VK_BINDING; ++bnd)
        {
            if (mVkBindingDeclaredType[bnd] != VKBD_SAMPLER && mVkBindingDeclaredType[bnd] != VKBD_UBO) continue;
            if (present[bnd]) continue;
            VkDescriptorSetLayoutBinding b = {};
            b.binding         = bnd;
            b.descriptorType  = (mVkBindingDeclaredType[bnd] == VKBD_UBO)
                                  ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                  : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            b.descriptorCount = 1;
            b.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings.push_back(b);
        }
    }
    for (U32 bnd = 0; bnd < MAX_VK_BINDING; ++bnd)
    {
        if (mVkBindingToEnumCanonical[bnd] >= 0)
        {
            if (mVkBindingToEnum[bnd] >= 0)
            {
                mVkBindingToEnum[bnd] = mVkBindingToEnumCanonical[bnd];
            }
        }
        else if ((mVkBindingDeclaredType[bnd] & VKBD_SAMPLER) != 0 && mVkBindingToEnum[bnd] >= 0)
        {
            mVkBindingToEnum[bnd] = -1;
        }
    }
    mVkPerProgramUBOBinding = 0;
    bool ppbinding_found = false;
    for (U32 pass = 0; pass < 2 && !ppbinding_found; ++pass)
    {
        const U8 stage_want = (pass == 0) ? VKBS_FRAGMENT : (U8)0;
        for (U32 bnd = 0; bnd < MAX_VK_BINDING; ++bnd)
        {
            if (mVkBindingDeclaredType[bnd] == VKBD_UBO
                && mVkBindingToUBOAccessor[bnd] == nullptr
                && bnd != 7 && bnd != 44
                && (stage_want == 0 || (mVkBindingStageMask[bnd] & stage_want)))
            {
                mVkPerProgramUBOBinding = bnd;
                ppbinding_found = true;
                break;
            }
        }
    }

    mVkLayoutBindings = bindings;

    mVkSet1DynamicCount   = 0;
    mVkDynamicBindingMask = 0;
    mVkDynamicBindings.clear();
    for (const auto& b : bindings)
    {
        if (b.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
        {
            ++mVkSet1DynamicCount;
            if (b.binding < 64)
            {
                mVkDynamicBindingMask |= (1ull << b.binding);
            }
            mVkDynamicBindings.push_back(b.binding);
        }
    }
    std::sort(mVkDynamicBindings.begin(), mVkDynamicBindings.end());

    VkDescriptorSetLayoutCreateInfo dsl_info = {};
    dsl_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsl_info.bindingCount = static_cast<uint32_t>(bindings.size());
    dsl_info.pBindings    = bindings.data();

    if (vkCreateDescriptorSetLayout(dev, &dsl_info, nullptr, &mVkDescriptorSetLayout) != VK_SUCCESS)
    {
        mVkDescriptorSetLayout = VK_NULL_HANDLE;
        return false;
    }

    VkDescriptorSetLayout set_layouts[2] = {
        LLVKLoader::getPerFrameDescriptorSetLayout(),
        mVkDescriptorSetLayout
    };

    VkPushConstantRange pc_ranges[2] = {};
    U32 pc_range_count;
    if (mVkVertexPushConstantOver64)
    {
        pc_ranges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pc_ranges[0].offset     = 0;
        pc_ranges[0].size       = 128;
        pc_range_count = 1;
    }
    else
    {
        pc_ranges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pc_ranges[0].offset     = 0;
        pc_ranges[0].size       = 64;
        pc_ranges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pc_ranges[1].offset     = 64;
        pc_ranges[1].size       = 56;
        pc_range_count = 2;
    }

    mVkPipelineLayout = LLVKLoader::createStandardPipelineLayout(set_layouts, 2, pc_ranges, pc_range_count);
    if (mVkPipelineLayout == VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(dev, mVkDescriptorSetLayout, nullptr);
        mVkDescriptorSetLayout = VK_NULL_HANDLE;
        return false;
    }

    if (perProgramUBOSize > 0)
    {
        if (LLVKLoader::createPerProgramUBOVk(perProgramUBOSize,
                                              mVkPerProgramUBO,
                                              mVkPerProgramUBOAllocation,
                                              &mVkPerProgramUBOMapped))
        {
            mVkPerProgramUBOSize = perProgramUBOSize;
            mVkPerProgramUBOBaseMapped   = mVkPerProgramUBOMapped;
            mVkActivePerProgramUBO       = mVkPerProgramUBO;
            mVkActivePerProgramUBOMapped = mVkPerProgramUBOMapped;
            if (mVkPerProgramUBOBinding == 0 && mVkSet1DynamicCount > 0)
            {
                mVkPerProgramShadow.assign(perProgramUBOSize, 0);
                mVkPerProgramUBOMapped       = mVkPerProgramShadow.data();
                mVkActivePerProgramUBOMapped = mVkPerProgramShadow.data();
            }
        }
    }

    LLVkUboReg::verifyPerProgramSize(*this, perProgramUBOSize);

    return true;
}

void LLGLSLShader::rotatePerProgramUBOSlot()
{
    if (mVkPerProgramUBOSize == 0 || !LLVKLoader::isVulkanInitialized())
    {
        return;
    }
    if (mVkPerProgramUBOBinding == 0 && mVkSet1DynamicCount > 0)
    {
        ++mVkPerProgramUBOGeneration;
        if (sCurBoundShaderPtr == this)
        {
            sCurPerCallVkOffsetsDirty = true;
        }
        return;
    }
    const U32 f = LLVKLoader::getCurrentFrameIndex();
    if (f >= 3)
    {
        return;
    }
    const U64 mono = LLVKLoader::getMonotonicFrameCount();
    if (mVkPerProgramRingFrame[f] != mono)
    {
        mVkPerProgramRingFrame[f] = mono;
        mVkPerProgramRingIdx[f]   = 0;
    }
    const U32 idx = mVkPerProgramRingIdx[f];
    while (mVkPerProgramUBORing[f].size() <= (size_t)idx)
    {
        PerProgramUBORingSlot slot;
        if (!LLVKLoader::createPerProgramUBOVk(mVkPerProgramUBOSize,
                                               slot.buffer,
                                               slot.allocation,
                                               &slot.mapped))
        {
            return;
        }
        mVkPerProgramUBORing[f].push_back(slot);
    }
    mVkPerProgramRingIdx[f]       = idx + 1;
    mVkActivePerProgramUBO        = mVkPerProgramUBORing[f][idx].buffer;
    mVkActivePerProgramUBOMapped  = mVkPerProgramUBORing[f][idx].mapped;
    if (sCurBoundShaderPtr == this)
    {
        sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
    }
}

bool LLGLSLShader::vkCollectDynamicUBOWrites(LLGLSLShader*                     cur,
                                             LLVKLoader::ScenePerDrawBindings& bindings,
                                             U32                               per_program_dynamic_offset,
                                             U32*                              out_offsets)
{
    U32 idx = 0;
    for (U32 db : cur->mVkDynamicBindings)
    {
        if (idx >= MAX_VK_DYNAMIC_BINDINGS)
        {
            break;
        }
        U32          off  = 0;
        VkBuffer     dbuf = VK_NULL_HANDLE;
        VkDeviceSize dsz  = 0;
        if (db == 0 && cur->mVkPerProgramUBOBinding == 0)
        {
            off = per_program_dynamic_offset;
            bool have = (bindings.ubo != VK_NULL_HANDLE && bindings.ubo_binding == 0);
            for (U32 i = 0; !have && i < bindings.ubo_count; ++i)
            {
                if (bindings.ubo_writes[i].binding == 0 && bindings.ubo_writes[i].buf != VK_NULL_HANDLE)
                {
                    have = true;
                }
            }
            if (!have)
            {
                dbuf = LLVKLoader::getPerDrawUBOArenaBuffer();
                dsz  = 64;
                off  = 0;
            }
        }
        else
        {
            U32 doff = 0;
            if (LLVKLoader::getSharedDynamicUBOForBinding(db, dbuf, doff) && dbuf != VK_NULL_HANDLE)
            {
                dsz = cur->sharedUBOBindingSize(db);
                off = doff;
            }
            else
            {
                dbuf = LLVKLoader::getPerDrawUBOArenaBuffer();
                dsz  = 64;
                off  = 0;
            }
        }
        if (dbuf != VK_NULL_HANDLE && dsz > 0)
        {
            if (bindings.ubo_count >= LLVKLoader::ScenePerDrawBindings::MAX_UBO_WRITES)
            {
                return false;
            }
            auto& entry = bindings.ubo_writes[bindings.ubo_count];
            entry.binding = db;
            entry.buf     = dbuf;
            entry.offset  = 0;
            entry.size    = dsz;
            ++bindings.ubo_count;
        }
        out_offsets[idx++] = off;
    }
    return true;
}

void LLGLSLShader::vkRefreshDynamicOffsetsForDraw()
{
    LLGLSLShader* cur = sCurBoundShaderPtr;
    if (cur == nullptr)
    {
        sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
        return;
    }
    U32 idx = 0;
    for (U32 db : cur->mVkDynamicBindings)
    {
        if (idx >= MAX_VK_DYNAMIC_BINDINGS)
        {
            break;
        }
        U32 off = 0;
        if (db == 0 && cur->mVkPerProgramUBOBinding == 0)
        {
            if (cur->mVkPerProgramUBO != VK_NULL_HANDLE && cur->mVkPerProgramUBOSize > 0)
            {
                VkBuffer pp_buf = VK_NULL_HANDLE;
                if (!cur->vkResolvePerProgramForDraw(pp_buf, off))
                {
                    sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
                    return;
                }
            }
        }
        else
        {
            VkBuffer dbuf = VK_NULL_HANDLE;
            U32      doff = 0;
            if (LLVKLoader::getSharedDynamicUBOForBinding(db, dbuf, doff) && dbuf != VK_NULL_HANDLE)
            {
                off = doff;
            }
            else
            {
                sCurPerCallVkDescriptorSet = VK_NULL_HANDLE;
                return;
            }
        }
        sCurPerCallVkDynamicOffsets[idx++] = off;
    }
    sCurPerCallVkOffsetsDirty = false;
}

bool LLGLSLShader::vkResolvePerProgramForDraw(VkBuffer& out_buf, U32& out_offset)
{
    out_buf    = VK_NULL_HANDLE;
    out_offset = 0;
    if (mVkPerProgramUBOSize == 0 || mVkPerProgramUBO == VK_NULL_HANDLE)
    {
        return false;
    }
    if (mVkPerProgramUBOBinding != 0 || mVkSet1DynamicCount == 0
        || mVkPerProgramShadow.size() < mVkPerProgramUBOSize)
    {
        out_buf = mVkActivePerProgramUBO;
        return out_buf != VK_NULL_HANDLE;
    }
    VkBuffer arena = VK_NULL_HANDLE;
    U32      aoff  = 0;
    void*    aptr  = nullptr;
    if (LLVKLoader::allocPerDrawUBOSlice(mVkPerProgramUBOSize, arena, aoff, aptr))
    {
        std::memcpy(aptr, mVkPerProgramShadow.data(), mVkPerProgramUBOSize);
        out_buf    = arena;
        out_offset = aoff;
        return true;
    }
    if (mVkPerProgramUBOBaseMapped != nullptr)
    {
        std::memcpy(mVkPerProgramUBOBaseMapped, mVkPerProgramShadow.data(), mVkPerProgramUBOSize);
    }
    out_buf = mVkPerProgramUBO;
    return true;
}

VkDeviceSize LLGLSLShader::sharedUBOBindingSize(U32 binding) const
{
    switch (binding)
    {
        case 4:  return sizeof(LLVKLoader::SMAABlendWeightsF_PerProgramBind);
        case 8:  return sizeof(LLVKLoader::WindlightAtmos_PerProgramBind);
        case 9:  return sizeof(LLVKLoader::WindlightSky_PerProgramBind);
        case 10: return sizeof(LLVKLoader::WindlightHDR_PerProgramBind);
        case 11: return sizeof(LLVKLoader::WindlightLight_PerProgramBind);
        case 12: return mFeatures.isSpecular
                            ? sizeof(LLVKLoader::LightsSpecular_PerProgramBind)
                            : sizeof(LLVKLoader::Lights_PerProgramBind);
        case 14: return sizeof(LLVKLoader::WaterFog_PerProgramBind);
        case 15: return sizeof(LLVKLoader::Water_PerProgramBind);
        case 16: return sizeof(LLVKLoader::ReflectionProbe_PerProgramBind);
        case 18: return sizeof(LLVKLoader::GlobalF_PerProgramBind);
        case 22: return sizeof(LLVKLoader::AoUtil_PerProgramBind);
        case 26: return sizeof(LLVKLoader::TonemapUtilF_PerProgramBind);
        case 28: return sizeof(LLVKLoader::PbrTerrainF_PerProgramBind);
        case 30: return sizeof(LLVKLoader::DeferredUtil_PerProgramBind);
        case 31: return sizeof(LLVKLoader::ShadowUtil_PerProgramBind);
        case 38: return sizeof(LLVKLoader::ReflectionProbes_PerProgramBind);
        case 39: return sizeof(LLVKLoader::ReflectionProbeF_PerProgramBind);
        case 45: return sizeof(LLVKLoader::AvatarSkin_PerProgramBind);
        case 46: return sizeof(LLVKLoader::ObjectSkin_PerProgramBind);
        case 48: return sizeof(LLVKLoader::PBRMaterial_PerMaterial);
        case 49: return sizeof(LLVKLoader::SSRUtil_PerProgramBind);
        case 51: return sizeof(LLVKLoader::DrawColor_PerShaderBind);
        case 52: return sizeof(LLVKLoader::PbrTerrain_PerShaderBind);
        case 53: return sizeof(LLVKLoader::ShadowParams_PerShaderBind);
        default: return 0;
    }
}

void LLGLSLShader::populateAndBindUniversalDescriptorSet()
{
    if (!LLVKLoader::isVulkanInitialized())
    {
        return;
    }
    LLGLSLShader* cur = LLGLSLShader::sCurBoundShaderPtr;
    if (cur == nullptr || cur->mVkDescriptorSetLayout == VK_NULL_HANDLE)
    {
        return;
    }
    VkSampler sampler = LLVKLoader::getStandardLinearSampler();
    if (sampler == VK_NULL_HANDLE)
    {
        return;
    }

    LLVKLoader::ScenePerDrawBindings bindings;
    bindings.layout       = cur->mVkDescriptorSetLayout;
    bindings.sampler      = sampler;
    bindings.dynamic_mask = cur->mVkDynamicBindingMask;

    U32 per_program_dynamic_offset = 0;

    VkImageView fallback_view = LLVKLoader::getDefaultFallbackVkImageView();

    auto live_view = [](U32 unit) -> VkImageView
    {
        LLTexUnit* tu = gGL.getTexUnit((S32)unit);
        if (tu == nullptr) return VK_NULL_HANDLE;
        return tu->getLiveVkImageView();
    };

    bindings.sampler_count = 0;
    for (const auto& layout_binding : cur->mVkLayoutBindings)
    {
        if (layout_binding.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        {
            continue;
        }
        U32 N = layout_binding.binding;
        if (N >= LLGLSLShader::MAX_VK_BINDING)
        {
            continue;
        }
        if (bindings.sampler_count >= LLVKLoader::ScenePerDrawBindings::MAX_SAMPLERS)
        {
            break;
        }

        VkImageView view = VK_NULL_HANDLE;
        S32 channel = cur->mVkBindingToChannel[N];
        S32 enum_value = cur->mVkBindingToEnum[N];
        S32 resolved_unit = -1;
        const bool l3_hit = (enum_value >= 0 && enum_value < (S32)cur->mVkEnumBoundView.size()
                             && cur->mVkEnumBoundView[enum_value].bound);

        if (enum_value == -2)
        {
            S32 idx_unit = (S32)N - 100;
            if (idx_unit >= 0)
            {
                resolved_unit = idx_unit;
                view = live_view((U32)idx_unit);
            }
        }
        else if (l3_hit)
        {
            view = cur->vkResolveEnumBoundView(enum_value);
        }
        else if (channel >= 0)
        {
            resolved_unit = channel;
            view = live_view((U32)channel);
            vkWarnL3Fallback(cur, N, enum_value, view);
        }
        else
        {
            if (enum_value >= 0 && enum_value < (S32)cur->mTexture.size())
            {
                S32 unit = cur->mTexture[enum_value];
                if (unit >= 0)
                {
                    resolved_unit = unit;
                    view = live_view((U32)unit);
                    vkWarnL3Fallback(cur, N, enum_value, view);
                }
            }
        }

        if (view != VK_NULL_HANDLE && LLVKLoader::isImageViewActivePassAttachment(view))
        {
            view = VK_NULL_HANDLE;
        }

        bool used_fallback = (view == VK_NULL_HANDLE);
        if (!used_fallback)
        {
            if (l3_hit)
            {
                if (cur->vkResolveEnumBoundDim(enum_value) != cur->mVkBindingSamplerDim[N])
                {
                    used_fallback = true;
                }
            }
            else if (resolved_unit >= 0)
            {
                LLTexUnit* dim_tu = gGL.getTexUnit(resolved_unit);
                if (dim_tu != nullptr && dim_tu->getLiveVkImageViewDim() != cur->mVkBindingSamplerDim[N])
                {
                    used_fallback = true;
                }
            }
        }
        if (used_fallback)
        {
            const U8 sdim = cur->mVkBindingSamplerDim[N];
            view = (sdim == VKSD_CUBE_ARRAY) ? LLVKLoader::getDefaultFallbackCubeArrayVkImageView()
                 : (sdim == VKSD_CUBE)       ? LLVKLoader::getDefaultFallbackCubeVkImageView()
                 : (sdim == VKSD_3D)         ? LLVKLoader::getDefaultFallback3DVkImageView()
                 :                             fallback_view;
        }

        VkSampler binding_sampler = VK_NULL_HANDLE;
        if (l3_hit)
        {
            binding_sampler = cur->mVkEnumBoundView[enum_value].sampler;
        }
        else if (resolved_unit >= 0)
        {
            LLTexUnit* stu = gGL.getTexUnit(resolved_unit);
            if (stu != nullptr)
            {
                binding_sampler = stu->getLiveVkSampler();
            }
        }

        bindings.sampler_bindings[bindings.sampler_count] = N;
        bindings.sampler_views[bindings.sampler_count]    = view;
        bindings.sampler_samplers[bindings.sampler_count] = binding_sampler;
        ++bindings.sampler_count;
    }

    for (const auto& layout_binding : cur->mVkLayoutBindings)
    {
        if (layout_binding.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
            && layout_binding.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
        {
            continue;
        }
        U32 N = layout_binding.binding;
        if (N >= LLGLSLShader::MAX_VK_BINDING)
        {
            continue;
        }
        if (bindings.ubo_count >= LLVKLoader::ScenePerDrawBindings::MAX_UBO_WRITES)
        {
            break;
        }

        VkBuffer ubo_buf = VK_NULL_HANDLE;
        VkDeviceSize ubo_sz = 0;

        if (N == cur->mVkPerProgramUBOBinding)
        {
            U32 pp_off = 0;
            if (cur->vkResolvePerProgramForDraw(ubo_buf, pp_off))
            {
                ubo_sz = cur->mVkPerProgramUBOSize;
                if (N == 0)
                {
                    per_program_dynamic_offset = pp_off;
                }
            }
        }
        else if (N < 64 && ((cur->mVkDynamicBindingMask >> N) & 1))
        {
            continue;
        }
        else
        {
            LLGLSLShader::SharedUBOAccessor accessor = cur->mVkBindingToUBOAccessor[N];
            if (accessor)
            {
                void* mapped = nullptr;
                if (accessor(ubo_buf, mapped))
                {
                    ubo_sz = cur->sharedUBOBindingSize(N);
                }
            }
        }

        bool ubo_provided = (ubo_buf != VK_NULL_HANDLE && ubo_sz > 0);

        if (ubo_provided)
        {
            auto& entry = bindings.ubo_writes[bindings.ubo_count];
            entry.binding = N;
            entry.buf     = ubo_buf;
            entry.offset  = 0;
            entry.size    = ubo_sz;
            ++bindings.ubo_count;
        }
    }

    U32 dyn_offsets[LLGLSLShader::MAX_VK_DYNAMIC_BINDINGS] = {};
    if (!vkCollectDynamicUBOWrites(cur, bindings, per_program_dynamic_offset, dyn_offsets))
    {
        return;
    }

    VkDescriptorSet per_draw_set = VK_NULL_HANDLE;
    if (LLVKLoader::ensureScenePerDrawDescriptorSet(bindings, &per_draw_set)
        && per_draw_set != VK_NULL_HANDLE)
    {
        LLGLSLShader::sCurPerCallVkDescriptorSet = per_draw_set;
        std::memcpy(LLGLSLShader::sCurPerCallVkDynamicOffsets, dyn_offsets, sizeof(dyn_offsets));
        LLGLSLShader::sCurPerCallVkOffsetsDirty = false;
    }
}

extern bool gCubeSnapshot;
extern bool gHeroProbeMirrorRender;

bool LLGLSLShader::vkCaptureRegimeActive()
{
    return gCubeSnapshot || gHeroProbeMirrorRender;
}

bool LLGLSLShader::vkUsePositiveViewport(bool render_target_bound, bool capture_regime)
{
    return render_target_bound && !capture_regime;
}

VkPipeline LLGLSLShader::getOrCreateVkPipelineForBoundRT(U32 mode)
{
    if (mVkPipelineLayout == VK_NULL_HANDLE)
    {
        return VK_NULL_HANDLE;
    }

    LLRenderTarget* rt = LLRenderTarget::getCurrentBoundTarget();
    U32 color_count;
    bool has_depth;
    VkFormat color_formats[4] = { VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED, VK_FORMAT_UNDEFINED };
    VkFormat depth_format;

    VkPipelineStateKey key;
    std::memset(&key, 0, sizeof(key));
    key.mode = static_cast<U8>(mode & 0x3Fu);
    key.cube_snapshot = vkCaptureRegimeActive() ? 1u : 0u;
    if (rt)
    {
        color_count = rt->getNumTextures();
        has_depth = rt->hasDepth();
        if (color_count > 4 || (color_count == 0 && !has_depth))
        {
            return VK_NULL_HANDLE;
        }
        key.color_count       = static_cast<U8>(color_count);
        key.depth_present     = has_depth ? 1u : 0u;
        key.is_swapchain_path = 0u;
        for (U32 i = 0; i < color_count; ++i)
        {
            U32 gl_fmt = rt->getInternalFormat(i);
            color_formats[i]      = LLVKLoader::llGlEnumToVkFormat(gl_fmt);
            key.color_formats[i]  = gl_fmt;
        }
        depth_format = has_depth ? VK_FORMAT_D24_UNORM_S8_UINT : VK_FORMAT_UNDEFINED;
    }
    else
    {
        VkFormat swap_format = LLVKLoader::getSwapchainFormat();
        if (swap_format == VK_FORMAT_UNDEFINED)
        {
            return VK_NULL_HANDLE;
        }
        const bool swap_depth  = LLVKLoader::hasSwapchainDepth();
        color_count            = 1;
        color_formats[0]       = swap_format;
        depth_format           = swap_depth ? VK_FORMAT_D24_UNORM_S8_UINT : VK_FORMAT_UNDEFINED;
        key.color_count        = 1u;
        key.depth_present      = swap_depth ? 1u : 0u;
        key.is_swapchain_path  = 1u;
        key.color_formats[0]   = static_cast<U32>(swap_format);
    }

    VkCullModeFlags vk_cull_mode;
    if (!LLGLState::isCullFaceEnabled())
    {
        key.cull_mode = 0u;
        vk_cull_mode  = VK_CULL_MODE_NONE;
    }
    else
    {
        switch (LLGLState::sCullFaceMode)
        {
            case GL_FRONT:
                key.cull_mode = 1u;
                vk_cull_mode  = VK_CULL_MODE_FRONT_BIT;
                break;
            case GL_FRONT_AND_BACK:
                key.cull_mode = 3u;
                vk_cull_mode  = VK_CULL_MODE_FRONT_AND_BACK;
                break;
            case GL_BACK:
            default:
                key.cull_mode = 2u;
                vk_cull_mode  = VK_CULL_MODE_BACK_BIT;
                break;
        }
    }

    VkPolygonMode vk_polygon_mode;
    if (LLGLState::sPolygonMode == GL_LINE)
    {
        key.polygon_mode = 1u;
        vk_polygon_mode  = VK_POLYGON_MODE_LINE;
    }
    else
    {
        key.polygon_mode = 0u;
        vk_polygon_mode  = VK_POLYGON_MODE_FILL;
    }

    key.depth_clamp_enabled = LLGLState::isDepthClampEnabled() ? 1u : 0u;

    {
        const F32 lw_f = gGL.getLineWidth();
        std::memcpy(&key.line_width_bits, &lw_f, sizeof(U32));
    }

    key.depth_bias_enabled = LLGLState::isPolygonOffsetEnabled() ? 1u : 0u;
    {
        const F32 factor_f = gGL.getPolygonOffsetFactor();
        const F32 units_f  = gGL.getPolygonOffsetUnits();
        std::memcpy(&key.depth_bias_slope_bits,    &factor_f, sizeof(U32));
        std::memcpy(&key.depth_bias_constant_bits, &units_f,  sizeof(U32));
    }

    key.blend_enabled    = LLGLState::isBlendEnabled() ? 1u : 0u;
    key.blend_color_src  = static_cast<U8>(gGL.getCurrBlendColorSFactor());
    key.blend_color_dst  = static_cast<U8>(gGL.getCurrBlendColorDFactor());
    key.blend_alpha_src  = static_cast<U8>(gGL.getCurrBlendAlphaSFactor());
    key.blend_alpha_dst  = static_cast<U8>(gGL.getCurrBlendAlphaDFactor());
    key.color_write_mask =
        (gGL.getColorMaskR() ? 0x1u : 0u) |
        (gGL.getColorMaskG() ? 0x2u : 0u) |
        (gGL.getColorMaskB() ? 0x4u : 0u) |
        (gGL.getColorMaskA() ? 0x8u : 0u);

    const bool stencil_enabled            = LLGLState::isStencilTestEnabled();
    const VkCompareOp stencil_vk_compare_op    = LLVKLoader::llGlEnumToVkCompareOp(LLGLState::sStencilFunc);
    const VkStencilOp stencil_vk_fail_op       = LLVKLoader::llGlEnumToVkStencilOp(LLGLState::sStencilFailOp);
    const VkStencilOp stencil_vk_pass_op       = LLVKLoader::llGlEnumToVkStencilOp(LLGLState::sStencilDepthPassOp);
    const VkStencilOp stencil_vk_depth_fail_op = LLVKLoader::llGlEnumToVkStencilOp(LLGLState::sStencilDepthFailOp);
    key.stencil_test_enabled        = stencil_enabled ? 1u : 0u;
    key.stencil_front_compare_op    = static_cast<U8>(stencil_vk_compare_op);
    key.stencil_front_fail_op       = static_cast<U8>(stencil_vk_fail_op);
    key.stencil_front_pass_op       = static_cast<U8>(stencil_vk_pass_op);
    key.stencil_front_depth_fail_op = static_cast<U8>(stencil_vk_depth_fail_op);
    key.stencil_back_compare_op     = static_cast<U8>(stencil_vk_compare_op);
    key.stencil_back_fail_op        = static_cast<U8>(stencil_vk_fail_op);
    key.stencil_back_pass_op        = static_cast<U8>(stencil_vk_pass_op);
    key.stencil_back_depth_fail_op  = static_cast<U8>(stencil_vk_depth_fail_op);
    key.stencil_compare_mask        = static_cast<U32>(LLGLState::sStencilCompareMask);
    key.stencil_write_mask          = static_cast<U32>(LLGLState::sStencilWriteMask);
    key.stencil_reference           = static_cast<U32>(LLGLState::sStencilRef);

    key.depth_test_enabled  = LLGLDepthTest::isCurrentDepthEnabled() ? 1u : 0u;
    key.depth_write_enabled = LLGLDepthTest::isCurrentWriteEnabled() ? 1u : 0u;
    key.depth_compare_op    = static_cast<U8>(LLVKLoader::llGlEnumToVkCompareOp(LLGLDepthTest::getCurrentDepthFunc()));

    auto it = mVkPipelineCache.find(key);
    if (it != mVkPipelineCache.end())
    {
        return it->second;
    }

    VkShaderModule vert_module = VK_NULL_HANDLE;
    VkShaderModule frag_module = VK_NULL_HANDLE;
    VkShaderModule geom_module = VK_NULL_HANDLE;
    for (const auto& sf : mShaderFiles)
    {
        if (sf.second == GL_VERTEX_SHADER)
        {
            auto vit = mVkVertexShaderModulesPerProgram.find(sf.first);
            if (vit != mVkVertexShaderModulesPerProgram.end()) vert_module = vit->second;
        }
        else if (sf.second == GL_FRAGMENT_SHADER)
        {
            auto fit = mVkFragmentShaderModulesPerProgram.find(sf.first);
            if (fit != mVkFragmentShaderModulesPerProgram.end()) frag_module = fit->second;
        }
        else if (sf.second == GL_GEOMETRY_SHADER)
        {
            auto git = mVkGeometryShaderModulesPerProgram.find(sf.first);
            if (git != mVkGeometryShaderModulesPerProgram.end()) geom_module = git->second;
        }
    }
    if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE)
    {
        mVkPipelineCache[key] = VK_NULL_HANDLE;
        return VK_NULL_HANDLE;
    }

    VkPipelineShaderStageCreateInfo stages[3] = {};
    uint32_t stage_count = 0;
    stages[stage_count].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[stage_count].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[stage_count].module = vert_module;
    stages[stage_count].pName  = "main";
    ++stage_count;
    if (geom_module != VK_NULL_HANDLE && LLVKLoader::isGeometryShaderEnabledVk())
    {
        stages[stage_count].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[stage_count].stage  = VK_SHADER_STAGE_GEOMETRY_BIT;
        stages[stage_count].module = geom_module;
        stages[stage_count].pName  = "main";
        ++stage_count;
    }
    stages[stage_count].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[stage_count].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[stage_count].module = frag_module;
    stages[stage_count].pName  = "main";
    ++stage_count;

    auto get_vk_format_stride = [](U32 type) -> std::pair<VkFormat, U32> {
        switch (type)
        {
            case LLVertexBuffer::TYPE_VERTEX:        return { VK_FORMAT_R32G32B32_SFLOAT,    16 };
            case LLVertexBuffer::TYPE_NORMAL:        return { VK_FORMAT_R32G32B32_SFLOAT,    16 };
            case LLVertexBuffer::TYPE_TEXCOORD0:     return { VK_FORMAT_R32G32_SFLOAT,        8 };
            case LLVertexBuffer::TYPE_TEXCOORD1:     return { VK_FORMAT_R32G32_SFLOAT,        8 };
            case LLVertexBuffer::TYPE_TEXCOORD2:     return { VK_FORMAT_R32G32_SFLOAT,        8 };
            case LLVertexBuffer::TYPE_TEXCOORD3:     return { VK_FORMAT_R32G32_SFLOAT,        8 };
            case LLVertexBuffer::TYPE_COLOR:         return { VK_FORMAT_R8G8B8A8_UNORM,       4 };
            case LLVertexBuffer::TYPE_EMISSIVE:      return { VK_FORMAT_R8G8B8A8_UNORM,       4 };
            case LLVertexBuffer::TYPE_TANGENT:       return { VK_FORMAT_R32G32B32A32_SFLOAT, 16 };
            case LLVertexBuffer::TYPE_WEIGHT:        return { VK_FORMAT_R32_SFLOAT,           4 };
            case LLVertexBuffer::TYPE_WEIGHT4:       return { VK_FORMAT_R32G32B32A32_SFLOAT, 16 };
            case LLVertexBuffer::TYPE_CLOTHWEIGHT:   return { VK_FORMAT_R32G32B32A32_SFLOAT, 16 };
            case LLVertexBuffer::TYPE_JOINT:         return { VK_FORMAT_R16G16B16A16_UINT,    8 };
            case LLVertexBuffer::TYPE_TEXTURE_INDEX: return { VK_FORMAT_R32_SINT,            16 };
            default:                                  return { VK_FORMAT_UNDEFINED,           0 };
        }
    };

    VkVertexInputBindingDescription   vi_bindings[LLVertexBuffer::TYPE_MAX] = {};
    VkVertexInputAttributeDescription vi_attrs[LLVertexBuffer::TYPE_MAX] = {};
    U32 vi_count = 0;

    const U32 vk_attr_mask = mVkAttributeMask;

    for (U32 type = 0; type < LLVertexBuffer::TYPE_MAX; ++type)
    {
        if (!(vk_attr_mask & (1u << type)))
            continue;
        auto fmt_stride = get_vk_format_stride(type);
        if (fmt_stride.first == VK_FORMAT_UNDEFINED)
            continue;

        vi_bindings[vi_count].binding   = type;
        vi_bindings[vi_count].stride    = fmt_stride.second;
        vi_bindings[vi_count].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        vi_attrs[vi_count].location = type;
        vi_attrs[vi_count].binding  = type;
        vi_attrs[vi_count].format   = fmt_stride.first;
        vi_attrs[vi_count].offset   = 0;

        ++vi_count;
    }

    U32 attr_count = vi_count;
    if (vk_attr_mask & (1u << LLVertexBuffer::TYPE_VERTEX))
    {
        for (U32 type = 0; type < LLVertexBuffer::TYPE_MAX; ++type)
        {
            if (vk_attr_mask & (1u << type))
                continue;
            auto fmt_stride = get_vk_format_stride(type);
            if (fmt_stride.first == VK_FORMAT_UNDEFINED)
                continue;

            vi_attrs[attr_count].location = type;
            vi_attrs[attr_count].binding  = LLVertexBuffer::TYPE_VERTEX;
            vi_attrs[attr_count].format   = fmt_stride.first;
            vi_attrs[attr_count].offset   = 0;

            ++attr_count;
        }
    }

    VkPipelineVertexInputStateCreateInfo vi = {};
    vi.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount   = vi_count;
    vi.pVertexBindingDescriptions      = vi_bindings;
    vi.vertexAttributeDescriptionCount = attr_count;
    vi.pVertexAttributeDescriptions    = vi_attrs;

    VkPrimitiveTopology topology;
    switch (mode)
    {
        case 1:  topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break;
        case 2:  topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;   break;
        case 3:  topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;     break;
        case 4:  topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;      break;
        case 5:  topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;     break;
        case 6:  topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;     break;
        case 0:
        default: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;  break;
    }

    VkPipelineInputAssemblyStateCreateInfo ia = {};
    ia.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology               = topology;
    ia.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo vp = {};
    vp.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rs = {};
    rs.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    VkPipelineRasterizationProvokingVertexStateCreateInfoEXT pv_state = {};
    if (LLVKLoader::isProvokingVertexLastEnabled())
    {
        pv_state.sType              = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_PROVOKING_VERTEX_STATE_CREATE_INFO_EXT;
        pv_state.provokingVertexMode = VK_PROVOKING_VERTEX_MODE_LAST_VERTEX_EXT;
        rs.pNext                    = &pv_state;
    }
    rs.polygonMode             = vk_polygon_mode;
    rs.cullMode                = vk_cull_mode;
    rs.frontFace               = vkUsePositiveViewport(key.is_swapchain_path == 0u, key.cube_snapshot != 0u)
                                 ? VK_FRONT_FACE_CLOCKWISE
                                 : VK_FRONT_FACE_COUNTER_CLOCKWISE;
    {
        F32 lw_f;
        std::memcpy(&lw_f, &key.line_width_bits, sizeof(F32));
        rs.lineWidth = lw_f;
    }
    rs.depthClampEnable        = key.depth_clamp_enabled ? VK_TRUE : VK_FALSE;
    rs.depthBiasEnable = key.depth_bias_enabled ? VK_TRUE : VK_FALSE;
    {
        F32 slope_f, constant_f;
        std::memcpy(&slope_f,    &key.depth_bias_slope_bits,    sizeof(F32));
        std::memcpy(&constant_f, &key.depth_bias_constant_bits, sizeof(F32));
        rs.depthBiasSlopeFactor    = slope_f;
        rs.depthBiasConstantFactor = constant_f;
        rs.depthBiasClamp          = 0.0f;
    }

    VkPipelineMultisampleStateCreateInfo ms = {};
    ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds = {};
    ds.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable  = key.depth_test_enabled  ? VK_TRUE : VK_FALSE;
    ds.depthWriteEnable = key.depth_write_enabled ? VK_TRUE : VK_FALSE;
    ds.depthCompareOp   = static_cast<VkCompareOp>(key.depth_compare_op);

    ds.stencilTestEnable = stencil_enabled ? VK_TRUE : VK_FALSE;
    {
        VkStencilOpState st = {};
        st.failOp      = stencil_vk_fail_op;
        st.passOp      = stencil_vk_pass_op;
        st.depthFailOp = stencil_vk_depth_fail_op;
        st.compareOp   = stencil_vk_compare_op;
        st.compareMask = static_cast<U32>(LLGLState::sStencilCompareMask);
        st.writeMask   = static_cast<U32>(LLGLState::sStencilWriteMask);
        st.reference   = static_cast<U32>(LLGLState::sStencilRef);
        ds.front = st;
        ds.back  = st;
    }

    auto gl_blend_factor_to_vk = [](LLRender::eBlendFactor bf) -> VkBlendFactor {
        switch (bf)
        {
            case LLRender::BF_ONE:                       return VK_BLEND_FACTOR_ONE;
            case LLRender::BF_ZERO:                      return VK_BLEND_FACTOR_ZERO;
            case LLRender::BF_DEST_COLOR:                return VK_BLEND_FACTOR_DST_COLOR;
            case LLRender::BF_SOURCE_COLOR:              return VK_BLEND_FACTOR_SRC_COLOR;
            case LLRender::BF_ONE_MINUS_DEST_COLOR:      return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
            case LLRender::BF_ONE_MINUS_SOURCE_COLOR:    return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            case LLRender::BF_DEST_ALPHA:                return VK_BLEND_FACTOR_DST_ALPHA;
            case LLRender::BF_SOURCE_ALPHA:              return VK_BLEND_FACTOR_SRC_ALPHA;
            case LLRender::BF_ONE_MINUS_DEST_ALPHA:      return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            case LLRender::BF_ONE_MINUS_SOURCE_ALPHA:    return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            case LLRender::BF_UNDEF:
                LL_ERRS("Vulkan") << "gl_blend_factor_to_vk: BF_UNDEF (= blendFunc 未設定) detected = RULE 4 stub fallback 禁止ゆえ fatal abort" << LL_ENDL;
                return VK_BLEND_FACTOR_ZERO;
        }
        LL_ERRS("Vulkan") << "gl_blend_factor_to_vk: unknown eBlendFactor value = " << (S32)bf << " = enum 拡張未対応 + silent fallback 禁止ゆえ fatal abort" << LL_ENDL;
        return VK_BLEND_FACTOR_ZERO;
    };
    const bool blend_enabled_final  = (key.blend_enabled != 0u);
    const VkBlendFactor vk_color_src = gl_blend_factor_to_vk(static_cast<LLRender::eBlendFactor>(key.blend_color_src));
    const VkBlendFactor vk_color_dst = gl_blend_factor_to_vk(static_cast<LLRender::eBlendFactor>(key.blend_color_dst));
    const VkBlendFactor vk_alpha_src = gl_blend_factor_to_vk(static_cast<LLRender::eBlendFactor>(key.blend_alpha_src));
    const VkBlendFactor vk_alpha_dst = gl_blend_factor_to_vk(static_cast<LLRender::eBlendFactor>(key.blend_alpha_dst));
    VkColorComponentFlags vk_color_write_mask = 0;
    if (key.color_write_mask & 0x1u) vk_color_write_mask |= VK_COLOR_COMPONENT_R_BIT;
    if (key.color_write_mask & 0x2u) vk_color_write_mask |= VK_COLOR_COMPONENT_G_BIT;
    if (key.color_write_mask & 0x4u) vk_color_write_mask |= VK_COLOR_COMPONENT_B_BIT;
    if (key.color_write_mask & 0x8u) vk_color_write_mask |= VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendAttachmentState cb_attach[4] = {};
    for (U32 i = 0; i < color_count; ++i)
    {
        cb_attach[i].blendEnable         = blend_enabled_final ? VK_TRUE : VK_FALSE;
        cb_attach[i].srcColorBlendFactor = vk_color_src;
        cb_attach[i].dstColorBlendFactor = vk_color_dst;
        cb_attach[i].colorBlendOp        = VK_BLEND_OP_ADD;
        cb_attach[i].srcAlphaBlendFactor = vk_alpha_src;
        cb_attach[i].dstAlphaBlendFactor = vk_alpha_dst;
        cb_attach[i].alphaBlendOp        = VK_BLEND_OP_ADD;
        cb_attach[i].colorWriteMask      = vk_color_write_mask;
    }

    VkPipelineColorBlendStateCreateInfo cb = {};
    cb.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = color_count;
    cb.pAttachments    = cb_attach;

    VkDynamicState dyn_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dyn = {};
    dyn.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn.dynamicStateCount = 2;
    dyn.pDynamicStates    = dyn_states;

    VkPipelineRenderingCreateInfo rendering_info = {};
    rendering_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    rendering_info.colorAttachmentCount    = color_count;
    rendering_info.pColorAttachmentFormats = color_formats;
    rendering_info.depthAttachmentFormat   = depth_format;
    rendering_info.stencilAttachmentFormat = depth_format;

    VkGraphicsPipelineCreateInfo ci = {};
    ci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    ci.pNext               = &rendering_info;
    ci.stageCount          = stage_count;
    ci.pStages             = stages;
    ci.pVertexInputState   = &vi;
    ci.pInputAssemblyState = &ia;
    ci.pViewportState      = &vp;
    ci.pRasterizationState = &rs;
    ci.pMultisampleState   = &ms;
    ci.pDepthStencilState  = &ds;
    ci.pColorBlendState    = &cb;
    ci.pDynamicState       = &dyn;
    ci.layout              = mVkPipelineLayout;
    ci.renderPass          = VK_NULL_HANDLE;

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (!LLVKLoader::compileGraphicsPipeline(ci, pipeline))
    {
        mVkPipelineCache[key] = VK_NULL_HANDLE;
        return VK_NULL_HANDLE;
    }

    mVkPipelineCache[key] = pipeline;
    return pipeline;
}
