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
#include "llvkloader.h"
#include "llimagegl.h"
#include "llcubemap.h"
#include "llstaticstringtable.h"
#include <boost/json.hpp>
#include <array>
#include <cstring>
#include <unordered_map>

struct VkPipelineStateKey
{
    U32 color_formats[4];
    U8  color_count;
    U8  depth_present;
    U8  is_swapchain_path;
    U8  cube_snapshot;
    U8  mode;

    U8  cull_mode;

    U8  polygon_mode;

    U8  depth_clamp_enabled;

    U32 line_width_bits;

    U8  depth_bias_enabled;
    U32 depth_bias_constant_bits;
    U32 depth_bias_slope_bits;

    U8  color_write_mask;

    U8  depth_test_enabled;
    U8  depth_write_enabled;
    U8  depth_compare_op;

    U8  blend_enabled;
    U8  blend_color_src;
    U8  blend_color_dst;
    U8  blend_alpha_src;
    U8  blend_alpha_dst;

    U8  stencil_test_enabled;
    U8  stencil_front_compare_op;
    U8  stencil_front_fail_op;
    U8  stencil_front_pass_op;
    U8  stencil_front_depth_fail_op;
    U8  stencil_back_compare_op;
    U8  stencil_back_fail_op;
    U8  stencil_back_pass_op;
    U8  stencil_back_depth_fail_op;
    U32 stencil_compare_mask;
    U32 stencil_write_mask;
    U32 stencil_reference;

    bool operator==(const VkPipelineStateKey& other) const
    {
        return std::memcmp(this, &other, sizeof(*this)) == 0;
    }
};

struct VkReflUboMember
{
    std::string name;
    U32 offset;
    U32 size;
};

struct VkReflUboBlock
{
    S32 set;
    S32 binding;
    U8 stage_mask;
    std::string block_name;
    U32 block_size;
    std::vector<VkReflUboMember> members;
};

struct VkPipelineStateKeyHash
{
    size_t operator()(const VkPipelineStateKey& k) const noexcept
    {
        const unsigned char* data = reinterpret_cast<const unsigned char*>(&k);
        U64 hash = 14695981039346656037ULL;
        for (size_t i = 0; i < sizeof(k); ++i)
        {
            hash ^= data[i];
            hash *= 1099511628211ULL;
        }
        return static_cast<size_t>(hash);
    }
};

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
    bool isPBRTerrain = false;
    bool hasTonemap = false;
    // <AYAstorm r30 P2> Gates velocityFuncV.glsl auto-attach in
    // attachShaderFeatures() so velocity-shader variants can compile.
    bool hasMotionBlur = false;
    // </AYAstorm r30 P2>
    bool usesSMAABlendWeights = false;
};

class LLGLSLShader
{
public:
    enum eShaderConsts
    {
        SHADER_CONST_CLOUD_MOON_DEPTH
        , SHADER_CONST_STAR_DEPTH
        , NUM_SHADER_CONSTS
    };

    typedef enum
    {
        SG_DEFAULT = 0,
        SG_SKY,
        SG_WATER,
        SG_ANY,
        SG_COUNT
    } eGroup;

    enum UniformBlock : GLuint
    {
        UB_REFLECTION_PROBES,   // "ReflectionProbes"
        UB_GLTF_JOINTS,
        UB_GLTF_NODES,
        UB_GLTF_MATERIALS,
        NUM_UNIFORM_BLOCKS
    };


    static std::set<LLGLSLShader*> sInstances;
    static bool sProfileEnabled;
    static bool sCanProfile;

    LLGLSLShader();
    ~LLGLSLShader();

    struct StageSource
    {
        GLenum type;
        std::string file_name;
        std::vector<std::string> sources;
    };

    static thread_local LLGLSLShader* sCurBoundShaderPtr;
    static S32 sIndexedTextureChannels;

    static U32 sMaxGLTFMaterials;
    static U32 sMaxGLTFNodes;

    static void initProfile();
    static void finishProfile(boost::json::value& stats=sDefaultStats);

    void unload();
    void clearStats();
    void dumpStats(boost::json::object& stats);

    void placeProfileQuery(bool for_runtime = false);
    bool readProfileQuery(bool for_runtime = false, bool force_read = false);

    bool createShader();
    bool attachFragmentObject(std::string object);
    bool attachVertexObject(std::string object);
    bool mapAttributes();
    bool mapUniforms();

    void setMinimumAlpha(F32 minimum);
    void setObjectAlpha(F32 object_alpha);
    void pushGaussianFragPC(F32 resScale, F32 dirX, F32 dirY);

    bool hasReflectedUniform(S32 reserved_enum) const;

    void clearPermutations();
    void addPermutation(std::string name, std::string value);
    void addPermutations(const std::map<std::string, std::string>& defines)
    {
        mDefines.insert(defines.begin(), defines.end());
    }
    void removePermutation(std::string name);

    void addConstant(const LLGLSLShader::eShaderConsts shader_const);

    S32 enableTexture(S32 uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);
    S32 disableTexture(S32 uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);
    S32 getTextureChannel(S32 uniform) const;
    S32 bindTexture(S32 uniform, LLTexture* texture, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);
    S32 bindTexture(S32 uniform, LLRenderTarget* texture, bool depth = false, LLTexUnit::eTextureFilterOptions mode = LLTexUnit::TFO_BILINEAR, U32 index = 0);
    S32 unbindTexture(S32 uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);

    void bind();
    void bind(bool rigged);

    bool isComplete() const { return mComplete; }

    LLUUID hash();

    // Unbinds any previously bound shader by explicitly binding no shader.
    static void unbind();

    U32 mMatHash[LLRender::NUM_MATRIX_MODES];
    U32 mLightHash;

    bool mComplete = false;
    bool mVkComplete = false;
    std::vector<GLint> mTexture;
    S32 mActiveTextureChannels;
    S32 mShaderLevel;
    S32 mShaderGroup;
    LLShaderFeatures mFeatures;
    std::vector< std::pair< std::string, GLenum > > mShaderFiles;
    std::string mName;
    typedef std::map<std::string, std::string> defines_map_t;
    defines_map_t mDefines;
    static defines_map_t sGlobalDefines;
    LLUUID mShaderHash;

    bool mProfilePending = false;
    uint32_t mVkTimestampHandle = 0;

    U64 mTimeElapsed;
    static U64 sTotalTimeElapsed;
    U32 mTrianglesDrawn;
    static U32 sTotalTrianglesDrawn;
    U64 mSamplesDrawn;
    static U64 sTotalSamplesDrawn;
    U32 mBinds;
    static U32 sTotalBinds;

    LLGLSLShader* mRiggedVariant = nullptr;

    struct GLTFVariant
    {
        constexpr static U8 ALPHA_BLEND = 1;
        constexpr static U8 RIGGED = 2;
        constexpr static U8 UNLIT = 4;
        constexpr static U8 MULTI_UV = 8;
    };

    constexpr static U8 NUM_GLTF_VARIANTS = 16;

    std::vector<LLGLSLShader> mGLTFVariants;

    void bind(U8 variant);

    bool mCanBindFast = false;

    std::vector<StageSource> mStageSources;

    std::vector<std::string> mVulkanAttachedVertexUtilities;
    std::vector<std::string> mVulkanAttachedFragmentUtilities;

    std::unordered_map<VkPipelineStateKey, VkPipeline, VkPipelineStateKeyHash>  mVkPipelineCache;

    std::map<std::string, VkShaderModule>  mVkVertexShaderModulesPerProgram;
    std::map<std::string, VkShaderModule>  mVkFragmentShaderModulesPerProgram;
    std::map<std::string, VkShaderModule>  mVkGeometryShaderModulesPerProgram;
    VkPipelineLayout           mVkPipelineLayout      = VK_NULL_HANDLE;
    VkDescriptorSetLayout      mVkDescriptorSetLayout = VK_NULL_HANDLE;

    bool                       mVkVertexPushConstantOver64 = false;
    bool                       mVkUsesBindlessHeap         = false;

    U32                        mVkAttributeMask = 0;
    bool                       mVkAttributeMaskValid = false;

    static constexpr U32 MAX_VK_BINDING = 128;
    std::array<S32, MAX_VK_BINDING> mVkBindingToEnum = {};

    std::array<S32, MAX_VK_BINDING> mVkBindingToEnumCanonical = {};

    std::array<S32, MAX_VK_BINDING> mVkBindingToChannel = {};

    enum VkBindingDeclType : U8 { VKBD_NONE = 0, VKBD_SAMPLER = 1, VKBD_UBO = 2, VKBD_BOTH = 3 };
    std::array<U8, MAX_VK_BINDING> mVkBindingDeclaredType = {};

    static constexpr U8 VKBS_VERTEX = 0x1, VKBS_FRAGMENT = 0x2;
    std::array<U8, MAX_VK_BINDING> mVkBindingStageMask = {};

    enum VkBindingSamplerDim : U8 { VKSD_2D = 0, VKSD_CUBE = 1, VKSD_CUBE_ARRAY = 2, VKSD_3D = 3 };
    std::array<U8, MAX_VK_BINDING> mVkBindingSamplerDim = {};

    std::array<bool, MAX_VK_BINDING> mVkBindingSamplerUsed = {};

    std::vector<std::pair<S32, std::string>> mVkReflBindingSamplerNames;

    std::vector<S32> mVkReflEnumChannel;

    std::vector<VkReflUboBlock> mVkReflUboBlocks;
    std::vector<VkReflUboBlock> mVkReflPushConstants;

    struct VkEnumBoundView
    {
        LLPointer<LLImageGL> imagep;
        LLPointer<LLCubeMap> cubep;
        LLRenderTarget*      rtp = nullptr;
        U32                  rt_attachment = 0;
        bool                 rt_depth = false;
        VkSampler            sampler = VK_NULL_HANDLE;
        bool                 bound = false;
    };
    std::vector<VkEnumBoundView> mVkEnumBoundView;
    std::vector<S16> mChannelToEnum;

    void vkCaptureEnumBoundView(S32 uniform_enum, S32 channel);
    void vkCaptureChannelBoundView(S32 channel);
    VkImageView vkResolveEnumBoundView(S32 uniform_enum) const;
    U8 vkResolveEnumBoundDim(S32 uniform_enum) const;
    static void vkWarnL3Fallback(LLGLSLShader* shader, U32 binding, S32 enum_value, VkImageView old_view);

    U32 mVkPerProgramUBOBinding = 0;

    typedef bool (*SharedUBOAccessor)(VkBuffer& out_buffer, void*& out_mapped);
    std::array<SharedUBOAccessor, MAX_VK_BINDING> mVkBindingToUBOAccessor = {};

    std::vector<VkDescriptorSetLayoutBinding> mVkLayoutBindings;

    std::vector<U8> mVkAccessorBindingListLanes[LLVKLoader::MAX_RECORD_LANES];
    bool            mVkAccessorBindingListBuiltLanes[LLVKLoader::MAX_RECORD_LANES] = {};

    static void populateAndBindUniversalDescriptorSet(bool preserve_drawdata = false);

public:
    static bool vkCaptureRegimeActive();
    static bool vkUsePositiveViewport(bool render_target_bound, bool capture_regime);

    VkDeviceSize sharedUBOBindingSize(U32 binding) const;
    static thread_local VkDescriptorSet sCurPerCallVkDescriptorSet;
    static constexpr U32       MAX_VK_DYNAMIC_BINDINGS = 8;
    static thread_local U32    sCurPerCallVkDynamicOffsets[MAX_VK_DYNAMIC_BINDINGS];
    static thread_local bool   sCurPerCallVkOffsetsDirty;
    static thread_local U32    sCurPerCallVkSetShape;
    static thread_local bool   sCurPerCallAuthored;
    static void vkRefreshDynamicOffsetsForDraw();
    static VkDescriptorSet vkResolvePerCallSetForDraw();
    static void resetPerThreadRecordState();

    VkBuffer                   mVkPerProgramUBO         = VK_NULL_HANDLE;
    void*                      mVkPerProgramUBOAllocation = nullptr;
    void*                      mVkPerProgramUBOMapped   = nullptr;
    U32                        mVkPerProgramUBOSize     = 0;

    struct VkBindlessSet1LaneState
    {
        VkDescriptorSet set[3]     = {};
        void*           tok[3]     = {};
        U64             ringSig[3] = {};
        U64             pinEpoch   = 0;
        U64             topoGen    = 0;
        void*           l3Views[6] = {};
        S16             l3Enums[6] = {};
        U8              l3Count    = 0;
    };
    VkBindlessSet1LaneState mVkBindlessSet1Lanes[LLVKLoader::MAX_RECORD_LANES];
    void clearVkBindlessSet1Pins();

    struct VkImmediateSet1LaneState
    {
        VkDescriptorSet set[3]     = {};
        void*           tok[3]     = {};
        U64             sig[3]     = {};
        U64             ringSig[3] = {};
        U64             topoGen    = 0;
        void*           l3Views[6] = {};
        S16             l3Enums[6] = {};
        U8              l3Count    = 0;
        U64             pinEpoch   = 0;
    };
    VkImmediateSet1LaneState mVkImmediateSet1Lanes[LLVKLoader::MAX_RECORD_LANES];
    U32  mVkImmediateSigUnits = 0xFFFFFFFFu;
    bool mVkImmediateUncacheable = false;
    U32  mVkImmediateHits  = 0;
    U32  mVkImmediateFills = 0;
    bool mVkImmediateNoFill = false;
    void clearVkImmediateSet1Pins();

    VkBuffer                   mVkActivePerProgramUBO       = VK_NULL_HANDLE;
    void*                      mVkActivePerProgramUBOMapped = nullptr;
    struct PerProgramUBORingSlot
    {
        VkBuffer buffer     = VK_NULL_HANDLE;
        void*    allocation = nullptr;
        void*    mapped     = nullptr;
    };
    std::vector<PerProgramUBORingSlot> mVkPerProgramUBORing[3];
    U32                        mVkPerProgramRingIdx[3]   = { 0, 0, 0 };
    U64                        mVkPerProgramRingFrame[3] = { 0, 0, 0 };
    void rotatePerProgramUBOSlot();

    U32                        mVkSet1DynamicCount        = 0;
    U64                        mVkDynamicBindingMask      = 0;
    std::vector<U32>           mVkDynamicBindings;
    U64                        mVkPerProgramUBOGeneration = 0;
    void*                      mVkPerProgramUBOBaseMapped = nullptr;
    std::vector<U8>            mVkPerProgramShadow;
    bool vkResolvePerProgramForDraw(VkBuffer& out_buf, U32& out_offset);
    static bool vkCollectDynamicUBOWrites(LLGLSLShader*                     cur,
                                          LLVKLoader::ScenePerDrawBindings& bindings,
                                          U32                               per_program_dynamic_offset,
                                          U32*                              out_offsets);
    static bool vkValidatePerCallCache(LLGLSLShader* cur, U64 stored_ring_sig,
                                       const void* const* stored_l3_views,
                                       const S16* stored_l3_enums, U8 stored_l3_count);
    static U64  vkComputeImmediateSig(LLGLSLShader* cur);
    static bool vkImmediateCacheEnabled();

    bool                       mWritePerProgramUBOMinimumAlpha = false;

    bool createVkPipeline(U32 perProgramUBOSize = 0, bool needsSharedWaterVUBO = false);

    VkPipeline getOrCreateVkPipelineForBoundRT(U32 mode = 0 );

#if LL_PROFILER_ENABLE_RENDER_DOC
    void setLabel(const char* label);
#endif

private:
    bool generatePerProgramSPIRV(const std::vector<StageSource>& stages);

    void unloadInternal();
    // This must be static because finishProfile() is called at least once
    // within a __try block. If we default its stats parameter to a temporary
    // json::value, that temporary must be destroyed when the stack is
    // unwound, which __try forbids.
    static boost::json::value sDefaultStats;
};

extern LLGLSLShader         gUIProgram;
extern LLGLSLShader         gSolidColorProgram;
extern LLGLSLShader         gAlphaMaskProgram;

#if LL_PROFILER_ENABLE_RENDER_DOC
#define LL_SET_SHADER_LABEL(shader) shader.setLabel(#shader)
#else
#define LL_SET_SHADER_LABEL(shader, label)
#endif

#endif
