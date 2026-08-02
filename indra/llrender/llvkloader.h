/**
* @file llvkloader.h
* @brief AYAstorm r41 Vulkan loader + instance lifecycle (volk-based)
*
* $LicenseInfo:firstyear=2026&license=viewerlgpl$
* AYAstorm Viewer Source Code
* Copyright (C) 2025-2026 Ishikawa AYA (github: mayatonton, mayatonton1994@gmail.com)
*
* このファイルは Ishikawa AYA が新規に作成した独自著作物である。
* 著作権は Ishikawa AYA が保持し、パブリックドメインには置かない。
* All rights reserved by the author except as licensed below.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
* $/LicenseInfo$
*
* 光の国のひとたちと共にわたしはここにいる　彩
*/

#ifndef LL_LLVKLOADER_H
#define LL_LLVKLOADER_H

#include "volk.h"

#include <atomic>
#include <functional>
#include <string>
#include <vector>

class LLWindow;
class LLGLSLShader;

namespace LLVKLoader
{
    bool initVulkan();
    void shutdownVulkan(bool device_lost = false);
    void shutdownSwapchainAndSurface();
    void setVsyncEnabled(bool enabled);
    void vkQuiesceProducers();

    bool isVulkanInitialized();
    bool isInFrame();
    void gpuCheckpoint(const char* label);
    bool anyViewHandleDead(const void* const* views, U32 count);

    bool beginFrame(bool acquire_swapchain = true);
    bool endFrame();
    void recordToConsumer(bool on);
    void finalizeConsumerSwapchain();
    bool isUISceneSplit();
    bool isUISceneAsync();
    bool asyncProducerTryComplete();
    bool isAsyncProducerInFlight();
    U32  asyncProducerBackIndex();
    bool asyncShouldRenderScene();
    void setAsyncFrameEngaged(bool on);
    bool asyncFrameEngaged();
    bool isSwapchainImageAcquired();
    void asyncProducerBeginScene(U32 back_index);
    void setProducerPresentActive(bool on);
    bool producerSwapchainFallbackShouldSkip();
    bool beginOffscreenFrameVk();
    void endOffscreenFrameVk();
    VkCommandBuffer getCurrentCommandBuffer();

    constexpr U32 MAX_RECORD_LANES = 1;

    U32  recordWorkerCount();

    uint32_t acquireOcclusionQueryVk();
    void     releaseOcclusionQueryVk(uint32_t handle);
    void     cmdBeginOcclusionQueryVk(VkCommandBuffer cmd, uint32_t handle);
    void     cmdEndOcclusionQueryVk(VkCommandBuffer cmd, uint32_t handle);
    bool     getOcclusionQueryResultVk(uint32_t handle, bool& available, uint64_t& samples);

    bool     isTimestampSupportedVk();
    uint32_t acquireTimestampPairVk();
    void     releaseTimestampPairVk(uint32_t handle);
    void     cmdWriteTimestampBeginVk(VkCommandBuffer cmd, uint32_t handle);
    void     cmdWriteTimestampEndVk(VkCommandBuffer cmd, uint32_t handle);
    bool     getTimestampElapsedNsVk(uint32_t handle, bool& available, uint64_t& elapsed_ns);

    void setScissor(S32 x, S32 y, S32 w, S32 h);

    void disableScissor();

    void setRenderViewport(S32 x, S32 y, S32 w, S32 h);

    F32 getMaxLineWidth();

    U32 currentRenderViewMask();
    VkImageView currentRenderDepthView();
    U32 currentRenderColorCount();
    VkImageView currentRenderColorView(U32 i);

    struct DeviceCapsVk
    {
        std::string device_name;
        std::string driver_name;
        std::string driver_info;
        U32         vendor_id                       = 0;
        U32         api_version_major               = 0;
        U32         api_version_minor               = 0;
        U32         device_local_memory_mb          = 0;
        U32         max_image_dimension_2d          = 0;
        U32         max_uniform_buffer_range        = 0;
        U32         max_per_stage_sampled_images    = 0;
        U32         max_vertex_output_components    = 0;
        U32         max_sample_mask_words           = 0;
        U32         framebuffer_color_sample_counts = 0;
        U32         framebuffer_depth_sample_counts = 0;
        U32         sampled_image_integer_sample_counts = 0;
        F32         max_sampler_anisotropy          = 1.0f;
        bool        sampler_anisotropy_enabled      = false;
        bool        image_cube_array_enabled        = false;
    };
    bool getDeviceCapsVk(DeviceCapsVk& out);

    VkDevice         getDevice();

    VkPipelineLayout createStandardPipelineLayout(
        const VkDescriptorSetLayout* descriptor_set_layouts,
        U32                          descriptor_set_layout_count,
        const VkPushConstantRange*   push_constant_ranges,
        U32                          push_constant_range_count);

    bool compileGraphicsPipeline(const VkGraphicsPipelineCreateInfo& ci, VkPipeline& out_pipeline);

    struct PerFrameMatrixUBO
    {
        float projection_matrix[16];
        float inverse_projection_matrix[16];
        float identity_matrix[16];
        float last_modelview_matrix[16];
    };
    static_assert(sizeof(PerFrameMatrixUBO) == 256,
                  "PerFrameMatrixUBO size mismatch (std140 expects 256 B)");

    struct TextureMatrixUBO
    {
        float texture_matrix[4][16];
    };
    static_assert(sizeof(TextureMatrixUBO) == 256,
                  "TextureMatrixUBO size mismatch (std140 expects 256 B)");

    VkDescriptorSetLayout getPerFrameDescriptorSetLayout();

    constexpr U32 FRAMES_IN_FLIGHT = 3;

    constexpr U32 DRAWDATA_SLOT_UINTS = 12;

    U32 getCurrentFrameIndex();
    U32 getMonotonicFrameCount();
    U32 getLastCompletedMonotonic();

    void writeCurrentPerFrameMatrixUBO(const PerFrameMatrixUBO& data, const TextureMatrixUBO& texdata);

    struct ShadowParams_PerShaderBind
    {
        float shadow_target_width;
        float _pad0;
        float _pad1;
        float _pad2;
    };
    static_assert(sizeof(ShadowParams_PerShaderBind) == 16,
                  "ShadowParams_PerShaderBind size mismatch (std140 expects 16 B)");

    void writeCurrentShadowParamsUBO(const ShadowParams_PerShaderBind& data);
    bool getSharedShadowParamsUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct ShadowViewProj_PerPass { float shadow_viewproj[4][16]; };
    static_assert(sizeof(ShadowViewProj_PerPass) == 256, "ShadowViewProj std140 256B");
    void writeCurrentShadowViewProjUBO(const ShadowViewProj_PerPass& data);
    bool getSharedShadowViewProjUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct PBRMaterial_PerMaterial
    {
        float texture_base_color_transform[8];
        float texture_normal_transform[8];
        float texture_metallic_roughness_transform[8];
        float texture_emissive_transform[8];
    };
    static_assert(sizeof(PBRMaterial_PerMaterial) == 128,
                  "PBRMaterial_PerMaterial size mismatch (std140 expects 128 B)");

    void writeCurrentPBRMaterialUBO(const PBRMaterial_PerMaterial& data);
    bool getSharedPBRMaterialUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct PreviewAmbient_PerShaderBind
    {
        float ambient_color[4];
    };
    static_assert(sizeof(PreviewAmbient_PerShaderBind) == 16,
                  "PreviewAmbient_PerShaderBind size mismatch (std140 expects 16 B)");

    void writeCurrentPreviewAmbientUBO(const PreviewAmbient_PerShaderBind& data);

    struct DrawColor_PerShaderBind
    {
        float color[4];
    };
    static_assert(sizeof(DrawColor_PerShaderBind) == 16,
                  "DrawColor_PerShaderBind size mismatch (std140 expects 16 B)");

    void writeCurrentDrawColorUBO(const DrawColor_PerShaderBind& data);
    bool getSharedDrawColorUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct PbrTerrain_PerShaderBind
    {
        float region_scale;
        float _pad0;
        float _pad1;
        float _pad2;
        float terrain_texture_transforms[5][4];
    };
    static_assert(sizeof(PbrTerrain_PerShaderBind) == 96,
                  "PbrTerrain_PerShaderBind size mismatch (std140 expects 96 B)");

    void writeCurrentPbrTerrainUBO(const PbrTerrain_PerShaderBind& data);
    bool getSharedPbrTerrainUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct StarTime_PerShaderBind
    {
        float time;
        float blend_factor;
        float custom_alpha;
        float _pad0;
    };
    static_assert(sizeof(StarTime_PerShaderBind) == 16,
                  "StarTime_PerShaderBind size mismatch (std140 expects 16 B)");

    void writeCurrentStarTimeUBO(const StarTime_PerShaderBind& data);

    struct AvatarVelocityPalette_PerShaderBind
    {
        float lastMatrixPalette[45][4];
    };
    static_assert(sizeof(AvatarVelocityPalette_PerShaderBind) == 720,
                  "AvatarVelocityPalette_PerShaderBind size mismatch (std140 expects 720 B)");

    struct GlowCombine_PerShaderBind
    {
        float greyscale_str;
        float sepia_str;
        float num_colors;
        float _pad0;
    };
    static_assert(sizeof(GlowCombine_PerShaderBind) == 16,
                  "GlowCombine_PerShaderBind size mismatch (std140 expects 16 B)");

    void writeCurrentGlowCombineUBO(const GlowCombine_PerShaderBind& data);

    void pushCurrentModelviewMatrix(const float modelview_matrix[16]);

    bool allocPerDrawUBOSlice(U32 size_bytes, VkBuffer& out_buffer, U32& out_offset, void*& out_mapped);
    VkBuffer getPerDrawUBOArenaBuffer();
    bool getSharedDynamicUBOForBinding(U32 binding, VkBuffer& out_buf, U32& out_off);
    bool peekSharedDynamicUBO(U32 binding, const void*& out_shadow, U32& out_size,
                              U32& out_off, bool& out_current, U64& out_up_hash);
    U64  sharedUBOContentHash(const void* p, U32 n);

    struct DynamicRenderingAttachment
    {
        VkImageView         image_view;
        VkImageLayout       image_layout;
        VkAttachmentLoadOp  load_op;
        VkAttachmentStoreOp store_op;
        VkClearValue        clear_value;
    };

    void beginDynamicRendering(U32                               width,
                               U32                               height,
                               const DynamicRenderingAttachment* color_attachments,
                               U32                               color_count,
                               const DynamicRenderingAttachment* depth_attachment,
                               U32                               view_mask = 0);
    void endDynamicRendering();
    void resumeSavedPass();

    VkShaderModule loadSpirvShaderModuleFromMemory(const std::vector<unsigned int>& spirv);

    VkFormat llGlEnumToVkFormat(U32 ll_gl_intformat);

    VkCompareOp          llGlEnumToVkCompareOp  (U32 ll_gl_func);
    VkStencilOp          llGlEnumToVkStencilOp  (U32 ll_gl_op);
    U32                  vkFormatBytesPerPixel  (VkFormat format);
    U32                  llGlFormatSourceComponents(U32 ll_gl_format);

    bool createPerProgramUBOVk(U32     size_bytes,
                              VkBuffer& out_buffer,
                              void*&    out_allocation,
                              void**    out_mapped);

    void ensurePerAssetUBOVk(U32       needed_size,
                             VkBuffer& inout_buffer,
                             void*&    inout_allocation,
                             void*&    inout_mapped,
                             U32&      inout_size);

    struct Water_PerProgramBind
    {
        float waveDir1[2];
        float waveDir2[2];
        float time;
        float _pad0[3];
        float eyeVec[3];
        float waterHeight;
        float lightDir[3];
        float _pad1;
    };
    static_assert(sizeof(Water_PerProgramBind) == 64,
                  "Water_PerProgramBind size mismatch (std140 expects 64 B)");

    bool     getSharedWaterVUBO(VkBuffer& out_buffer, void*& out_mapped);
    void     writeCurrentWaterVUBO(const Water_PerProgramBind& data);

    struct WindlightAtmos_PerProgramBind
    {
        float sunlight_color[3];
        int   sun_up_factor;
        float moonlight_color[3];
        int   classic_mode_wl;
        float ambient_color[3];
        int   aya_visual_realism_enabled;
        float blue_horizon[3];
        int   aya_r14_volumetric_atmosphere_enabled;
        float blue_density[3];
        float aya_r14_strength;
        float glow[3];
        float aya_r16_strength;
        float lightnorm[3];
        int   aya_r16_aerial_perspective_enabled;
        float haze_density;
        float density_multiplier;
        float distance_multiplier;
        float max_y;
        float haze_horizon;
        float cloud_shadow;
        float sun_moon_glow_factor;
        float sky_sunlight_scale;
        float sky_ambient_scale;
        float _wlAtmos_pad0;
        float _wlAtmos_pad1;
        float _wlAtmos_pad2;
    };
    static_assert(sizeof(WindlightAtmos_PerProgramBind) == 160,
                  "WindlightAtmos_PerProgramBind size mismatch (std140 expects 160 B)");
    void writeCurrentWindlightAtmosUBO(const WindlightAtmos_PerProgramBind& data);
    bool getSharedWindlightAtmosUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct WindlightSky_PerProgramBind
    {
        float sun_dir_sky[3];
        float _pad_sky0;
        float moon_dir_sky[3];
        float _pad_sky1;
    };
    static_assert(sizeof(WindlightSky_PerProgramBind) == 32,
                  "WindlightSky_PerProgramBind size mismatch (std140 expects 32 B)");
    void writeCurrentWindlightSkyUBO(const WindlightSky_PerProgramBind& data);
    bool getSharedWindlightSkyUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct WindlightHDR_PerProgramBind
    {
        float sky_hdr_scale;
        float _wlHDR_pad0;
        float _wlHDR_pad1;
        float _wlHDR_pad2;
    };
    static_assert(sizeof(WindlightHDR_PerProgramBind) == 16,
                  "WindlightHDR_PerProgramBind size mismatch (std140 expects 16 B)");
    void writeCurrentWindlightHDRUBO(const WindlightHDR_PerProgramBind& data);
    bool getSharedWindlightHDRUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct WindlightLight_PerProgramBind
    {
        float scene_light_strength;
        float _wlLight_pad0;
        float _wlLight_pad1;
        float _wlLight_pad2;
    };
    static_assert(sizeof(WindlightLight_PerProgramBind) == 16,
                  "WindlightLight_PerProgramBind size mismatch (std140 expects 16 B)");
    void writeCurrentWindlightLightUBO(const WindlightLight_PerProgramBind& data);
    bool getSharedWindlightLightUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct WaterFog_PerProgramBind
    {
        float waterPlane[4];
        float waterFogColor[4];
        float waterFogDensity;
        float waterFogKS;
        float _waterFog_pad0;
        float _waterFog_pad1;
    };
    static_assert(sizeof(WaterFog_PerProgramBind) == 48,
                  "WaterFog_PerProgramBind must match waterFogF.glsl std140 layout (48 B)");
    void writeCurrentWaterFogUBO(const WaterFog_PerProgramBind& data);
    bool getSharedWaterFogUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct GlobalF_PerProgramBind
    {
        float mirror_flag;
        float region_clip_flag;
        float _globalF_pad1;
        float _globalF_pad2;
        float clipPlane[4];
        float regionClip0[4];
        float regionClip1[4];
        float regionClip2[4];
        float regionClip3[4];
    };
    static_assert(sizeof(GlobalF_PerProgramBind) == 96,
                  "GlobalF_PerProgramBind must match globalF.glsl std140 layout (96 B)");
    void writeCurrentGlobalFUBO(const GlobalF_PerProgramBind& data);
    bool getSharedGlobalFUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct AoUtil_PerProgramBind
    {
        float screen_res[2];
        float ssao_radius;
        float ssao_max_radius;
        float ssao_factor;
        float ssao_factor_inv;
        float _aoUtil_pad0;
        float _aoUtil_pad1;
    };
    static_assert(sizeof(AoUtil_PerProgramBind) == 32,
                  "AoUtil_PerProgramBind size mismatch (std140 expects 32 B)");
    void writeCurrentAoUtilUBO(const AoUtil_PerProgramBind& data);
    bool getSharedAoUtilUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct TonemapUtilF_PerProgramBind
    {
        float exposure;
        float tonemap_mix;
        int   tonemap_type;
        float _tonemapUtilF_pad0;
    };
    static_assert(sizeof(TonemapUtilF_PerProgramBind) == 16,
                  "TonemapUtilF_PerProgramBind size mismatch (std140 expects 16 B)");
    void writeCurrentTonemapUtilFUBO(const TonemapUtilF_PerProgramBind& data);
    bool getSharedTonemapUtilFUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct DeferredUtil_PerProgramBind
    {
        float proj_mat[16];
        float waterPlane[4];
        float proj_n[3];
        float proj_focus;
        float proj_p[3];
        float proj_lod;
        float color[3];
        float size;
        float screen_res[2];
        float proj_range;
        float proj_ambiance;
        float _deferredUtil_pad_waterSign;
        int   classic_mode;
        float _deferredUtil_pad0;
        float _deferredUtil_pad1;
    };
    static_assert(sizeof(DeferredUtil_PerProgramBind) == 160,
                  "DeferredUtil_PerProgramBind size mismatch (std140 expects 160 B)");
    void writeCurrentDeferredUtilUBO(const DeferredUtil_PerProgramBind& data);
    bool getSharedDeferredUtilUBO(VkBuffer& out_buffer, void*& out_mapped);
    bool acquireDeferredUtilOverrideSlot(VkBuffer& out_buf, void*& out_mapped);
    void setDeferredUtilOverrideSlot(VkBuffer buf, void* mapped);
    void clearDeferredUtilOverrideSlot();

    struct ShadowUtil_PerProgramBind
    {
        float shadow_matrix[6 * 16];
        float shadow_clip[4];
        float sun_dir[3];
        float shadow_bias;
        float moon_dir[3];
        float shadow_offset;
        float shadow_res[2];
        float proj_shadow_res[2];
        float shadow_softness;
        float spot_shadow_bias;
        float spot_shadow_offset;
        float _shadowUtil_pad0;
    };
    static_assert(sizeof(ShadowUtil_PerProgramBind) == 464,
                  "ShadowUtil_PerProgramBind size mismatch (std140 expects 464 B)");
    void writeCurrentShadowUtilUBO(const ShadowUtil_PerProgramBind& data);
    bool getSharedShadowUtilUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct SSRUtil_PerProgramBind
    {
        float screen_res[2];
        float iterationCount;
        float rayStep;
        float modelview_delta[16];
        float inv_modelview_delta[16];
        float distanceBias;
        float depthRejectBias;
        float adaptiveStepMultiplier;
        float glossySampleCount;
        float splitParamsStart[3];
        float noiseSine;
        float splitParamsEnd[3];
        float maxZDepth;
        float maxRoughness;
        float _ssrUtil_pad0;
        float _ssrUtil_pad1;
        float _ssrUtil_pad2;
    };
    static_assert(sizeof(SSRUtil_PerProgramBind) == 208,
                  "SSRUtil_PerProgramBind must match cinematic_bd/screenSpaceReflUtil.glsl std140 layout (208 B)");
    void writeCurrentSSRUtilUBO(const SSRUtil_PerProgramBind& data);
    bool getSharedSSRUtilUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct SMAABlendWeightsF_PerProgramBind
    {
        float subsampleIndices[4];
    };
    static_assert(sizeof(SMAABlendWeightsF_PerProgramBind) == 16,
                  "SMAABlendWeightsF_PerProgramBind size mismatch (std140 expects 16 B)");
    void writeCurrentSMAABlendWeightsFUBO(const SMAABlendWeightsF_PerProgramBind& data);
    bool getSharedSMAABlendWeightsFUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct PbrTerrainF_PerProgramBind
    {
        float baseColorFactors[4][4];
        float metallicFactors[4];
        float roughnessFactors[4];
        float emissiveColors[4][4];
        float minimum_alphas[4];
    };
    static_assert(sizeof(PbrTerrainF_PerProgramBind) == 176,
                  "PbrTerrainF_PerProgramBind must match class1/deferred/pbrterrainF.glsl std140 layout (176 B)");
    void writeCurrentPbrTerrainFUBO(const PbrTerrainF_PerProgramBind& data);
    bool getSharedPbrTerrainFUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct ReflectionProbe_PerProgramBind
    {
        float reflection_probe_ambiance;
        float _refprobe_pad0;
        float _refprobe_pad1;
        float _refprobe_pad2;
        float env_mat_col0[4];
        float env_mat_col1[4];
        float env_mat_col2[4];
    };
    static_assert(sizeof(ReflectionProbe_PerProgramBind) == 64,
                  "ReflectionProbe_PerProgramBind must match reflectionProbeF.glsl std140 layout (64 B)");
    void writeCurrentReflectionProbeUBO(const ReflectionProbe_PerProgramBind& data);
    bool getSharedReflectionProbeUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct ReflectionProbeF_PerProgramBind
    {
        float env_mat_col0[4];
        float env_mat_col1[4];
        float env_mat_col2[4];
        int   cube_snapshot;
        float max_probe_lod;
        float _reflectionProbeF_pad0;
        float _reflectionProbeF_pad1;
        float clipPlane[4];
    };
    static_assert(sizeof(ReflectionProbeF_PerProgramBind) == 80,
                  "ReflectionProbeF_PerProgramBind: 48 (mat3 env_mat) + 4 (cube_snapshot) + 4 (max_probe_lod) + 8 (pad) + 16 (vec4 clipPlane HERO_PROBES) = 80 B per std140 spec");
    void writeCurrentReflectionProbeFUBO(const ReflectionProbeF_PerProgramBind& data);
    bool getSharedReflectionProbeFUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct AvatarSkin_PerProgramBind
    {
        float matrixPalette[45][4];
    };
    static_assert(sizeof(AvatarSkin_PerProgramBind) == 720,
                  "AvatarSkin_PerProgramBind must match class1/avatar/avatarSkinV.glsl std140 layout (720 B = vec4[45])");
    void writeCurrentAvatarSkinUBO(const AvatarSkin_PerProgramBind& data);
    bool getSharedAvatarSkinUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct ObjectSkin_PerProgramBind
    {
        float matrixPalette[110][3][4];
        float lastMatrixPalette[110][3][4];
    };
    static_assert(sizeof(ObjectSkin_PerProgramBind) == 10560,
                  "ObjectSkin_PerProgramBind must match class1/avatar/objectSkinV.glsl std140 layout (10560 B = mat3x4[110] × 2)");
    void* rotateObjectSkinSlotForWrite();
    bool getSharedObjectSkinUBO(VkBuffer& out_buffer, void*& out_mapped);
    bool objectSkinTryAdopt(const void* avatar, U64 skin_hash);
    void objectSkinStoreCache(const void* avatar, U64 skin_hash);
    U32  objectSkinLookupEntry(const void* avatar, U64 skin_hash); // B.2: bindless palette entry for (avatar,hash)
    void writeDrawSkinBase(U32 draw_id, U32 skin_entry);           // B.2: write skin base at DrawData slot
    U32  publishDrawSkinBase(U32 draw_id, const void* avatar, U64 skin_hash);

    struct Lights_PerProgramBind
    {
        float light_position[8][4];
        float light_diffuse[8][4];
    };
    static_assert(sizeof(Lights_PerProgramBind) == 256,
                  "Lights_PerProgramBind must match class1/lighting/sumLightsV.glsl std140 layout (256 B = vec4[8] + vec3[8])");
    void writeCurrentLightsUBO(const Lights_PerProgramBind& data);
    bool getSharedLightsUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct LightsSpecular_PerProgramBind
    {
        float light_position[8][4];
        float light_attenuation[8][4];
        float light_diffuse[8][4];
    };
    static_assert(sizeof(LightsSpecular_PerProgramBind) == 384,
                  "LightsSpecular_PerProgramBind must match class3/lighting/sumLightsSpecularV.glsl std140 layout (384 B = vec4[8] + vec4[8] + vec3[8])");
    void writeCurrentLightsSpecularUBO(const LightsSpecular_PerProgramBind& data);
    bool getSharedLightsSpecularUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct ReflectionProbes_PerProgramBind
    {
        float refBox[256][4][4];
        float heroBox[4][4];
        float refSphere[256][4];
        float refParams[256][4];
        float heroSphere[4];
        S32   refIndex[256][4];
        S32   refNeighbor[1024][4];
        S32   refBucket[256][4];
        S32   refmapCount;
        S32   heroShape;
        S32   heroMipCount;
        S32   heroProbeCount;
    };
    static_assert(sizeof(ReflectionProbes_PerProgramBind) == 49248,
                  "ReflectionProbes_PerProgramBind must match class3/deferred/reflectionProbeF.glsl std140 layout (49248 B)");
    void writeCurrentReflectionProbesUBO(const ReflectionProbes_PerProgramBind& data);
    bool getSharedReflectionProbesUBO(VkBuffer& out_buffer, void*& out_mapped);

    struct VolumetricLightF_PerProgramBind
    {
        int   godray_res;
        float godray_multiplier;
        float falloff_multiplier;
        float _volumetricLightF_pad0;
    };
    static_assert(sizeof(VolumetricLightF_PerProgramBind) == 16,
                  "VolumetricLightF_PerProgramBind size mismatch (std140 expects 16 B)");

    struct MotionBlurF_PerProgramBind
    {
        float _mbF_screen_res[2];
        int   _mbF_motion_blur_strength;
        int   _motionBlurF_pad0;
    };
    static_assert(sizeof(MotionBlurF_PerProgramBind) == 16,
                  "MotionBlurF_PerProgramBind size mismatch (std140 expects 16 B)");

    struct PostGammaCorrect_PerProgramBind
    {
        float gamma;
        float _postGammaCorrect_pad0;
        float _postGammaCorrect_pad1;
        float _postGammaCorrect_pad2;
    };
    static_assert(sizeof(PostGammaCorrect_PerProgramBind) == 16,
                  "PostGammaCorrect_PerProgramBind size mismatch (std140 expects 16 B)");

    struct CofF_PerProgramBind
    {
        float focal_distance;
        float blur_constant;
        float tan_pixel_angle;
        float magnification;
        float max_cof;
        float _cofF_pad0;
        float _cofF_pad1;
        float _cofF_pad2;
    };
    static_assert(sizeof(CofF_PerProgramBind) == 32,
                  "CofF_PerProgramBind size mismatch (std140 expects 32 B)");

    struct DofCombineF_PerProgramBind
    {
        float _dofC_screen_res[2];
        float _dofC_pad0[2];
        float _dofC_max_cof;
        float _dofC_res_scale;
        float _dofC_dof_width;
        float _dofC_dof_height;
    };
    static_assert(sizeof(DofCombineF_PerProgramBind) == 32,
                  "DofCombineF_PerProgramBind size mismatch (std140 expects 32 B)");

    struct FsObjectIDF_PerProgramBind
    {
        float object_id_packed[4];
    };
    static_assert(sizeof(FsObjectIDF_PerProgramBind) == 16,
                  "FsObjectIDF_PerProgramBind size mismatch (std140 expects 16 B)");

    struct FxaaShared_PerProgramBind
    {
        float tc_scale[2];
        float rcp_screen_res[2];
        float rcp_frame_opt[4];
        float rcp_frame_opt2[4];
    };
    static_assert(sizeof(FxaaShared_PerProgramBind) == 48,
                  "FxaaShared_PerProgramBind size mismatch (std140 expects 48 B)");

    struct AvatarVCloth_PerProgramBind
    {
        float gWindDir[4];
        float gSinWaveParams[4];
        float gGravity[4];
    };
    static_assert(sizeof(AvatarVCloth_PerProgramBind) == 48,
                  "AvatarVCloth_PerProgramBind size mismatch (std140 expects 48 B)");

    struct BlurLightF_PerProgramBind
    {
        float kern[4][4];
        float delta[2];
        float screen_res[2];
        float dist_factor;
        float blur_size;
        float kern_scale;
        float pad0;
    };
    static_assert(sizeof(BlurLightF_PerProgramBind) == 96,
                  "BlurLightF_PerProgramBind size mismatch (std140 expects 96 B)");

    struct Cloud_PerProgramBind
    {
        float camPosLocal[3];
        float _pad0;
        float cloud_color[3];
        float cloud_scale_v;
        float cloud_pos_density1[3];
        float _pad1;
        float cloud_pos_density2[3];
        float _pad2;
        float blend_factor;
        float cloud_scale;
        float cloud_variance;
        S32   aya_r18_cloud_volumetric_enabled;
        float aya_r18_strength;
        float _pad3;
        float _pad4;
        float _pad5;
    };
    static_assert(sizeof(Cloud_PerProgramBind) == 96,
                  "Cloud_PerProgramBind size mismatch (std140 expects 96 B)");

    struct ExposureF_PerProgramBind
    {
        float dynamic_exposure_params[4];
        float dynamic_exposure_params2[4];
        float dt;
        float _exposureF_pad0;
        float _exposureF_pad1;
        float _exposureF_pad2;
    };
    static_assert(sizeof(ExposureF_PerProgramBind) == 48,
                  "ExposureF_PerProgramBind size mismatch (std140 expects 48 B)");

    struct GlowExtract_PerProgramBind
    {
        float lumWeights[3];
        float minLuminance;
        float warmthWeights[3];
        float maxExtractAlpha;
        float warmthAmount;
        float _pad0;
        float screen_res[2];
    };
    static_assert(sizeof(GlowExtract_PerProgramBind) == 48,
                  "GlowExtract_PerProgramBind size mismatch (std140 expects 48 B)");

    struct Glow_PerProgramBind
    {
        float glowDelta[2];
        float glowStrength;
        float _pad0;
    };
    static_assert(sizeof(Glow_PerProgramBind) == 16,
                  "Glow_PerProgramBind size mismatch (std140 expects 16 B)");

    struct GodraysF_PerProgramBind
    {
        float sun_dir[3];
        float pad0;
        float moon_dir[3];
        S32   aya_r15_godrays_enabled;
        float aya_r15_godrays_phase_exponent;
        float aya_r15_godrays_strength;
        float pad1;
        float pad2;
    };
    static_assert(sizeof(GodraysF_PerProgramBind) == 48,
                  "GodraysF_PerProgramBind size mismatch (std140 expects 48 B)");

    struct HazeF_PerProgramBind
    {
        float sun_dir[3];
        float pad0;
        float moon_dir[3];
        S32   sun_up_factor;
    };
    static_assert(sizeof(HazeF_PerProgramBind) == 32,
                  "HazeF_PerProgramBind size mismatch (std140 expects 32 B)");

    struct IrradianceGen_PerProgramBind
    {
        S32   sourceIdx;
        float max_probe_lod;
        float pad0;
        float pad1;
    };
    static_assert(sizeof(IrradianceGen_PerProgramBind) == 16,
                  "IrradianceGen_PerProgramBind size mismatch (std140 expects 16 B)");

    struct LuminanceF_PerProgramBind
    {
        float diffuse_luminance_scale;
        float _luminanceF_pad0;
        float _luminanceF_pad1;
        float _luminanceF_pad2;
    };
    static_assert(sizeof(LuminanceF_PerProgramBind) == 16,
                  "LuminanceF_PerProgramBind size mismatch (std140 expects 16 B)");

    struct MoonF_PerProgramBind
    {
        float color[4];
        float moon_dir[3];
        float moon_brightness;
    };
    static_assert(sizeof(MoonF_PerProgramBind) == 32,
                  "MoonF_PerProgramBind size mismatch (std140 expects 32 B)");

    struct NormalDebug_PerProgramBind
    {
        float debug_normal_draw_length;
        float _pad0;
        float _pad1;
        float _pad2;
    };
    static_assert(sizeof(NormalDebug_PerProgramBind) == 16,
                  "NormalDebug_PerProgramBind size mismatch (std140 expects 16 B)");

    struct NormgenF_PerProgramBind
    {
        float stepX;
        float stepY;
        float norm_scale;
        S32   bump_code;
    };
    static_assert(sizeof(NormgenF_PerProgramBind) == 16,
                  "NormgenF_PerProgramBind size mismatch (std140 expects 16 B)");

    struct OcclusionCube_PushConstant
    {
        float box_center[3];
        float _pad0;
        float box_size[3];
        float _pad1;
    };
    static_assert(sizeof(OcclusionCube_PushConstant) == 32,
                  "OcclusionCube_PushConstant size mismatch (std430 push constant expects 32 B)");

    struct OneTextureFilter_PerProgramBind
    {
        float tolerance;
        float _pad0;
        float _pad1;
        float _pad2;
    };
    static_assert(sizeof(OneTextureFilter_PerProgramBind) == 16,
                  "OneTextureFilter_PerProgramBind size mismatch (std140 expects 16 B)");

    struct Pathfinding_PerProgramBind
    {
        float tint;
        float ambiance;
        float alpha_scale;
        float pad0;
    };
    static_assert(sizeof(Pathfinding_PerProgramBind) == 16,
                  "Pathfinding_PerProgramBind size mismatch (std140 expects 16 B)");

    struct PointLightPerDraw
    {
        float center[3];
        float size;
        float color[3];
        float falloff;
        float global_light_strength;
        S32   classic_mode;
        float pad0;
        float pad1;
    };
    static_assert(sizeof(PointLightPerDraw) == 48,
                  "PointLightPerDraw size mismatch (std140 expects 48 B)");

    struct PostF_PerProgramBind
    {
        float screen_res[2];
        float max_cof;
        float chroma_str;
    };
    static_assert(sizeof(PostF_PerProgramBind) == 16,
                  "PostF_PerProgramBind size mismatch (std140 expects 16 B)");

    struct PostNoDoFF_PerProgramBind
    {
        float screen_res[2];
        float chroma_str;
        float pad0;
    };
    static_assert(sizeof(PostNoDoFF_PerProgramBind) == 16,
                  "PostNoDoFF_PerProgramBind size mismatch (std140 expects 16 B)");

    struct PostSnapshotFrame_PerProgramBind
    {
        float screen_res[2];
        float pad0[2];
        float frame_rect[4];
        float border_color[3];
        float border_thickness;
    };
    static_assert(sizeof(PostSnapshotFrame_PerProgramBind) == 48,
                  "PostSnapshotFrame_PerProgramBind size mismatch (std140 expects 48 B)");

    struct PostTonemap_PerProgramBind
    {
        float color_saturation;
        float color_contrast;
        float color_temperature;
        float color_brightness;
        float color_grading_lut_intensity;
        S32   color_grading_lut_enabled;
        float gamma;
        float _postTonemap_pad1;
    };
    static_assert(sizeof(PostTonemap_PerProgramBind) == 32,
                  "PostTonemap_PerProgramBind size mismatch (std140 expects 32 B)");

    struct PostVignette_PerProgramBind
    {
        float screen_res[2];
        float pad0[2];
        float vignette[3];
        float pad1;
    };
    static_assert(sizeof(PostVignette_PerProgramBind) == 32,
                  "PostVignette_PerProgramBind size mismatch (std140 expects 32 B)");

    struct PostVisualizeBuffers_PerProgramBind
    {
        float mipLevel;
    };
    static_assert(sizeof(PostVisualizeBuffers_PerProgramBind) == 4,
                  "PostVisualizeBuffers_PerProgramBind size mismatch (std140 expects 4 B)");

    struct Preview_PerProgramBind
    {
        float light_position[8][4];
        float light_diffuse[8][4];
    };
    static_assert(sizeof(Preview_PerProgramBind) == 256,
                  "Preview_PerProgramBind size mismatch (std140 expects 256 B)");

    struct RadianceGen_PerProgramBind
    {
        S32   sourceIdx;
        float mipLevel;
        S32   u_width;
        float max_probe_lod;
        float probe_strength;
        float pad0;
        float pad1;
        float pad2;
    };
    static_assert(sizeof(RadianceGen_PerProgramBind) == 32,
                  "RadianceGen_PerProgramBind size mismatch (std140 expects 32 B)");

    struct RlvF_PerProgramBind
    {
        float rlvEffectParam1[4];
        float rlvEffectParam2[4];
        float rlvEffectParam4[4];
        float rlvEffectParam5[2];
        float _rlvF_screen_res[2];
        U32   rlvEffectParam3[2];
        S32   rlvEffectMode;
        S32   _rlvF_pad0;
    };
    static_assert(sizeof(RlvF_PerProgramBind) == 80,
                  "RlvF_PerProgramBind size mismatch (std140 expects 80 B)");

    struct SkinSSSF_PerProgramBind
    {
        float aya_glow_color[3];
        float aya_glow_gain;
        float aya_blur_dir[2];
        float aya_strength;
        float aya_blur_radius;
        S32   aya_visual_realism_enabled;
        S32   aya_r20_skin_sss_enabled;
        float pad0;
        float pad1;
    };
    static_assert(sizeof(SkinSSSF_PerProgramBind) == 48,
                  "SkinSSSF_PerProgramBind size mismatch (std140 expects 48 B)");

    struct SMAA_PerProgramBind
    {
        float SMAA_RT_METRICS[4];
    };
    static_assert(sizeof(SMAA_PerProgramBind) == 16,
                  "SMAA_PerProgramBind size mismatch (std140 expects 16 B)");

    struct SpotLightPerDraw
    {
        float center[3];
        float size;
        float proj_origin[3];
        float falloff;
        float shadow_fade;
        float global_light_strength;
        S32   proj_shadow_idx;
        S32   classic_mode;
    };
    static_assert(sizeof(SpotLightPerDraw) == 48,
                  "SpotLightPerDraw size mismatch (std140 expects 48 B)");

    struct SunDiscF_PerProgramBind
    {
        float blend_factor;
        float pad0;
        float pad1;
        float pad2;
    };
    static_assert(sizeof(SunDiscF_PerProgramBind) == 16,
                  "SunDiscF_PerProgramBind size mismatch (std140 expects 16 B)");

    struct SunLightF_PerProgramBind
    {
        float sun_dir[3];
        float pad0;
    };
    static_assert(sizeof(SunLightF_PerProgramBind) == 16,
                  "SunLightF_PerProgramBind size mismatch (std140 expects 16 B)");

    struct TerrainV_PerProgramBind
    {
        float object_plane_s[4];
        float object_plane_t[4];
    };
    static_assert(sizeof(TerrainV_PerProgramBind) == 32,
                  "TerrainV_PerProgramBind size mismatch (std140 expects 32 B)");

    struct TwoTextureCompare_PerProgramBind
    {
        float dither_scale;
        float dither_scale_s;
        float dither_scale_t;
        float _pad0;
    };
    static_assert(sizeof(TwoTextureCompare_PerProgramBind) == 16,
                  "TwoTextureCompare_PerProgramBind size mismatch (std140 expects 16 B)");

    struct UnderWaterF_PerProgramBind
    {
        float waterFogColorLinear[3];
        float refScale;
    };
    static_assert(sizeof(UnderWaterF_PerProgramBind) == 16,
                  "UnderWaterF_PerProgramBind size mismatch (std140 expects 16 B)");

    struct WaterF_PerProgramBind
    {
        float lightDir[3];
        float blurMultiplier;
        float specular[3];
        float refScale;
        float normScale[3];
        float fresnelScale;
        float fresnelOffset;
        float blend_factor;
        S32   classic_mode;
        float _pad_waterf0;
    };
    static_assert(sizeof(WaterF_PerProgramBind) == 64,
                  "WaterF_PerProgramBind size mismatch (std140 expects 64 B)");

    struct WaterHazeF_PerProgramBind
    {
        S32 above_water;
        S32 _pad0;
        S32 _pad1;
        S32 _pad2;
    };
    static_assert(sizeof(WaterHazeF_PerProgramBind) == 16,
                  "WaterHazeF_PerProgramBind size mismatch (std140 expects 16 B)");

    struct CasF_PerProgramBind
    {
        float out_screen_res_uniform[2];
        float _pad0[2];
        U32   cas_param_0_uniform[4];
        U32   cas_param_1_uniform[4];
        float gamma;
        float _pad1;
        float _pad2;
        float _pad3;
    };
    static_assert(sizeof(CasF_PerProgramBind) == 64,
                  "CasF_PerProgramBind size mismatch (std140 expects 64 B)");

    static constexpr U32 ALPHAF_UBO_SIZE_IMPOSTOR  = 16;
    static constexpr U32 ALPHAF_UBO_SIZE_SHADOW    = 528;
    static constexpr U32 ALPHAF_UBO_SIZE_NO_SHADOW = 560;
    static constexpr U32 ALPHAF_UBO_OFFSET_NEAR_CLIP = 4;
    static constexpr U32 ALPHAF_UBO_OFFSET_SUN_MOON = 16;
    static constexpr U32 ALPHAF_UBO_OFFSET_LIGHTS_SHADOW    = 16;
    static constexpr U32 ALPHAF_UBO_OFFSET_LIGHTS_NO_SHADOW = 48;

    static constexpr U32 GLTFMR_UBO_SIZE_HEADER          =  16;
    static constexpr U32 GLTFMR_UBO_SIZE_ALPHA_SUNSHADOW = 656;
    static constexpr U32 GLTFMR_UBO_SIZE_ALPHA_NOSHADOW  = 688;
    static constexpr U32 GLTFMR_UBO_OFFSET_SUN_DIR       =  16;
    static constexpr U32 GLTFMR_UBO_OFFSET_MOON_DIR      =  32;
    static constexpr U32 GLTFMR_UBO_OFFSET_LIGHTS_SUNSHADOW =  16;
    static constexpr U32 GLTFMR_UBO_OFFSET_LIGHTS_NOSHADOW  =  48;

    static constexpr U32 PBRALPHAF_UBO_SIZE_SHADOW    = 672;
    static constexpr U32 PBRALPHAF_UBO_SIZE_NO_SHADOW  = 704;

    void destroyBufferVk     (VkBuffer  buffer,
                              void*     allocation);
    void destroyPipelineVk           (VkPipeline            pipeline);
    void destroyShaderModuleVk       (VkShaderModule        shader_module);
    void destroyPipelineLayoutVk     (VkPipelineLayout      pipeline_layout);
    void destroyDescriptorSetLayoutVk(VkDescriptorSetLayout descriptor_set_layout);

    struct MegaSliceV
    {
        VkBuffer   buffer = VK_NULL_HANDLE;
        U8*        mapped = nullptr;
        U32        first  = 0;
        U32        count  = 0;
        const U32* region_offsets = nullptr;
        U64        chunk  = 0;
        U64        owner_token = 0;
    };

    struct MegaSliceI
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        U8*      mapped = nullptr;
        U32      offset = 0;
        U32      size   = 0;
        U64      chunk  = 0;
    };

    void megabufInit(const U32* type_sizes, U32 type_count);
    void megabufShutdown();
    bool megabufAcquireVertex(U32 typemask, U32 nverts, MegaSliceV& out);
    void megabufReleaseVertex(const MegaSliceV& slice);
    bool megabufAcquireIndex(U32 size_bytes, MegaSliceI& out);
    void megabufReleaseIndex(const MegaSliceI& slice);
    void megabufStats(U64& chunks, U64& capacity_bytes, U64& used_bytes);
    U64  megaCurrentRangeOwner(U64 chunk_id, U32 first);

    void bindVertexBufferVk(VkCommandBuffer cmd_buf,
                            VkBuffer        buffer,
                            VkDeviceSize    offset,
                            U32             firstBinding = 0);

    VkDescriptorSet getCurrentPerFrameDescriptorSet();

    void bindIndexBufferVk (VkCommandBuffer cmd_buf,
                            VkBuffer        buffer,
                            VkDeviceSize    offset,
                            VkIndexType     index_type);

    bool isIndirectDrawEnabled();
    bool indirectRingAlloc(U32           count,
                           VkBuffer&     out_buffer,
                           VkDeviceSize& out_offset,
                           void*&        out_mapped);

    bool createColorAttachmentImageVk(U32          width,
                                      U32          height,
                                      VkFormat     format,
                                      VkImage&     out_image,
                                      VkImageView& out_view,
                                      void*&       out_allocation,
                                      U32          mip_levels      = 1,
                                      VkImageView* out_sample_view = nullptr);

    bool generateMipChainInFrameVk(VkImage        image,
                                   U32            base_w,
                                   U32            base_h,
                                   U32            mip_count,
                                   VkFormat       format,
                                   VkImageLayout  mip0_src_layout);

    bool createDepthAttachmentImageVk(U32          width,
                                      U32          height,
                                      VkFormat     format,
                                      VkImage&     out_image,
                                      VkImageView& out_view,
                                      void*&       out_allocation);

    bool createLayeredDepthAttachmentImageVk(U32                       width,
                                             U32                       height,
                                             VkFormat                  format,
                                             U32                       layerCount,
                                             VkImage&                  out_image,
                                             VkImageView&              out_array_view,
                                             std::vector<VkImageView>& out_layer_views,
                                             void*&                    out_allocation);

    void destroyImageVk(VkImage     image,
                        VkImageView view,
                        void*       allocation);

    bool createTextureImageVk(U32          width,
                              U32          height,
                              VkFormat     format,
                              VkImage&     out_image,
                              VkImageView& out_view,
                              void*&       out_allocation,
                              U32          mip_levels = 1);

    bool uploadImageDataVk(VkImage     image,
                           U32         width,
                           U32         height,
                           const void* data,
                           U32         data_size_bytes,
                           U32         mip_level = 0);

    bool generateMipChainBlitVk(VkImage image, U32 base_w, U32 base_h, U32 mip_count, VkFormat format);

    void setVkGeoWorkerStopHook(void (*fn)());
    void setVkBakeWorkerStopHook(void (*fn)());
    void setVkDeviceLostHook(void (*fn)());
    void parWorkerForbiddenCheck();

    bool downscaleImageVk(VkImage      src_image,
                          U32          src_mip,
                          U32          src_w,
                          U32          src_h,
                          U32          dst_w,
                          U32          dst_h,
                          VkFormat     format,
                          U32          dst_mip_levels,
                          VkImage&     out_image,
                          VkImageView& out_view,
                          void*&       out_allocation);

    bool createTexture3DImageVk(U32          width,
                                U32          height,
                                U32          depth,
                                VkFormat     format,
                                VkImage&     out_image,
                                VkImageView& out_view,
                                void*&       out_allocation);

    bool uploadImageData3DVk(VkImage     image,
                             U32         width,
                             U32         height,
                             U32         depth,
                             const void* data,
                             U32         data_size_bytes);

    bool uploadImageSubregionVk(VkImage     image,
                                VkFormat    format,
                                U32         x_pos,
                                U32         y_pos,
                                U32         sub_width,
                                U32         sub_height,
                                const void* data,
                                U32         data_width,
                                U32         data_height);

    bool createCubeImageVk(U32          resolution,
                           VkFormat     format,
                           U32          mip_count,
                           VkImage&     out_image,
                           VkImageView& out_view,
                           void*&       out_allocation);

    bool uploadCubeImageDataVk(VkImage           image,
                               U32               resolution,
                               const void* const face_data[6],
                               U32               face_size_bytes);

    bool createCubeArrayImageVk(U32          resolution,
                                U32          count,
                                U32          mips,
                                VkFormat     format,
                                VkImage&     out_image,
                                VkImageView& out_view,
                                void*&       out_allocation);

    bool copyColorImageToCubeArrayLayerVk(VkImage       src_image,
                                          VkImageLayout src_layout,
                                          VkImage       dst_cube_array,
                                          U32           dst_layer,
                                          U32           dst_mip,
                                          U32           width,
                                          U32           height,
                                          U32           src_y = 0);

    bool blitCubeArrayVk(VkImage       src,
                         VkImageLayout src_layout,
                         U32           src_res,
                         VkImage       dst,
                         VkImageLayout dst_layout,
                         U32           dst_res,
                         U32           layer_count);

    bool copyColorImageRegionToImage2DVk(VkImage       src_image,
                                         VkImageLayout src_layout,
                                         S32           src_x,
                                         S32           src_y,
                                         VkImage       dst_image,
                                         VkImageLayout dst_current_layout,
                                         S32           dst_x,
                                         S32           dst_y,
                                         U32           width,
                                         U32           height);

    bool readbackColorImageRegionVk(VkImage       image,
                                    VkImageLayout current_layout,
                                    S32           x,
                                    S32           y,
                                    U32           width,
                                    U32           height,
                                    U32           bytes_per_pixel,
                                    void*         out_pixels);

    bool createReadbackBufferVk(U32       bytes,
                                VkBuffer& out_buffer,
                                void*&    out_allocation,
                                void*&    out_mapped);

    bool copyColorImageRegionToBufferVk(VkImage       src_image,
                                        VkImageLayout src_layout,
                                        S32           src_x,
                                        S32           src_y,
                                        U32           width,
                                        U32           height,
                                        VkBuffer      dst_buffer);

    bool readbackDepthImageRegionVk(VkImage       image,
                                    VkImageLayout current_layout,
                                    S32           x,
                                    S32           y,
                                    U32           width,
                                    U32           height,
                                    VkFormat      format,
                                    F32*          out_depth);

    VkSampler getStandardLinearSampler();

    VkSampler getSamplerForState(U32 address_mode, U32 filter_option, bool has_mipmaps, bool compare = false);

    bool isProvokingVertexLastEnabled();

    bool isGeometryShaderEnabledVk();

    bool isMultiviewEnabled();

    constexpr U32 BINDLESS_INVALID_SLOT = 0xFFFFFFFFu;
    constexpr U32 PERDRAW_SLOT_INHERIT = 0xFFFFFFFEu;

    bool isBindlessActiveVk();
    bool skinBindlessEnabled();

    U32  bindlessAcquireSlot(VkImageView view, VkSampler sampler);
    void bindlessUpdateSlot(U32 slot, VkImageView view, VkSampler sampler);

    void bindlessReleaseSlotDeferred(U32 slot);

    VkDescriptorSetLayout getBindlessHeapLayout();
    VkDescriptorSetLayout getSkinBaseLayout();
    VkDescriptorSetLayout getEmptySetLayout();

    U32  drawDataAcquireSlot(const U32* slots4);

    void drawDataReleaseSlotDeferred(U32 slot);

    U32  createRenderDomain();

    void renderDomainReclaim(U32 id);

    void setThreadAllocDomain(U32 id);

    U32  drawDataWriteScratch(const U32* slots4);

    void commitPerDrawID(U32 id, bool publish_skin, const void* avatar, U64 skin_hash);

    void transitionImageLayoutVk(VkImage              image,
                                 VkImageAspectFlags   aspect_mask,
                                 VkImageLayout        old_layout,
                                 VkImageLayout        new_layout,
                                 VkPipelineStageFlags src_stage_mask,
                                 VkPipelineStageFlags dst_stage_mask,
                                 VkAccessFlags        src_access_mask,
                                 VkAccessFlags        dst_access_mask,
                                 U32                  layer_count = 1);

    bool         initSurface(LLWindow* window);
    bool         auxWindowInitVk(void* native_display, void* native_window);
    void         auxWindowShutdownVk();
    bool         auxWindowActiveVk();
    void         auxWindowNotifyResizeVk();
    bool         auxWindowExtentVk(U32& out_w, U32& out_h);
    bool         auxWindowBeginUIFrameVk();
    bool         auxWindowEndUIFrameVk();
    VkSurfaceKHR getSurface();

    bool           initSwapchain();

    bool        isVulkanPresentationEnabled();
    void        setVulkanPresentationEnabled(bool enabled);

    U32         getRenderBackendMode();
    bool        shouldUseVulkanRender();
    void        resetVulkanRenderSuspend();

    VkFormat    getSwapchainFormat();

    bool        hasSwapchainDepth();

    void        beginSwapchainRendering();

    bool        isInRenderPassScope();
    bool        beginShaderDrawOrSkip(LLGLSLShader* shader, U32 render_mode, VkCommandBuffer& out_cmd);
    bool        isImageViewActivePassAttachment(VkImageView view);
    U64         currentPassAttachmentSig();
    void        setupViewportAndScissor(VkCommandBuffer cmd, bool screen_space_copy = false);
    void        bindGraphicsPipelineOnce(VkCommandBuffer cmd, VkPipeline pipeline);
    void        bindDrawDescriptorSetsOnce(VkCommandBuffer cmd, VkPipelineLayout layout,
                                           VkDescriptorSet set0, VkDescriptorSet set1,
                                           U32 dyn_count, const U32* offsets);
    void        pushModelviewOnce(VkCommandBuffer cmd, VkPipelineLayout layout, const float* mv16);
    bool        perFrameMatrixNeedsWrite();

    enum : U32
    {
        VKPERF_SHCTX_NONE     = 0,
        VKPERF_SHCTX_MV       = 1,
        VKPERF_SHCTX_REST     = 2,
        VKPERF_SHCTX_FALLBACK = 3,
        VKPERF_SHCTX_SPOT     = 4,
        VKPERF_SHCTX_COUNT    = 5,

        VKPERF_SHSEC_OTHER        = 0,
        VKPERF_SHSEC_OPAQUE       = 1,
        VKPERF_SHSEC_GEOM_TERRAIN = 3,
        VKPERF_SHSEC_GEOM_TREE    = 4,
        VKPERF_SHSEC_GEOM_AVATAR  = 5,
        VKPERF_SHSEC_AMASK        = 6,
        VKPERF_SHSEC_ABLEND       = 8,
        VKPERF_SHSEC_FBMASK       = 10,
        VKPERF_SHSEC_GRASSMAT     = 12,
        VKPERF_SHSEC_GLTF_AMASK   = 14,
        VKPERF_SHSEC_GLTF_PBR     = 16,
        VKPERF_SHSEC_COUNT        = 18,
    };

    struct VkPerfCounters
    {
        std::atomic<U64> pipe_bind{0};
        std::atomic<U64> pipe_skip{0};
        std::atomic<U64> desc_bind{0};
        std::atomic<U64> desc_skip{0};
        std::atomic<U64> mv_push{0};
        std::atomic<U64> mv_skip{0};
        std::atomic<U64> vp_set{0};
        std::atomic<U64> vp_skip{0};
        std::atomic<U64> set_build{0};
        std::atomic<U64> set_memo{0};
        std::atomic<U64> set_memo_fill{0};
        std::atomic<U64> populate{0};
        std::atomic<U64> populate_us{0};
        std::atomic<U64> populate_bl{0};
        std::atomic<U64> populate_pl{0};
        std::atomic<U64> populate_blhit{0};
        std::atomic<U64> populate_hit{0};
        std::atomic<U64> populate_miss{0};
        std::atomic<U64> syncmat_call{0};
        std::atomic<U64> syncmat_build{0};
        std::atomic<U64> vb_bind{0};
        std::atomic<U64> vb_skip{0};
        std::atomic<U64> ib_bind{0};
        std::atomic<U64> ib_skip{0};
        std::atomic<U64> draws_pass[5] = {};
        std::atomic<U64> draws_shadow_map[7] = {};
        std::atomic<U64> draws_shadow_site[VKPERF_SHCTX_COUNT][VKPERF_SHSEC_COUNT] = {};
        std::atomic<U64> shadow_cull{0};
        std::atomic<U64> shadow_rigged{0};
        std::atomic<U64> shadow_rigged_map[7] = {};
        std::atomic<U64> bkt_patch{0};
        std::atomic<U64> bkt_range{0};
        std::atomic<U64> bkt_rec{0};
        std::atomic<U64> bkt_skip{0};
        std::atomic<U64> mat_draws{0};
        std::atomic<U64> mat_bindless_draws{0};
        std::atomic<U64> mat_cen[12][2] = {};
        std::atomic<U64> mdi_call{0};
        std::atomic<U64> mdi_rec{0};
        std::atomic<U64> mdi_zero{0};
        std::atomic<U64> mdi_dyn{0};
        std::atomic<U64> mdi_full{0};
        std::atomic<U64> alp_run{0};
        std::atomic<U64> alp_col{0};
        std::atomic<U64> alp_inl{0};
        std::atomic<U64> alpha_us[12] = {};
        std::atomic<U64> emi_us[8] = {};
        std::atomic<U64> emi_grp{0};
        std::atomic<U64> emi_rtn{0};
        std::atomic<U64> emi_n[4] = {};
        std::atomic<U64> lgt_us[8] = {};
        std::atomic<U64> lgt_nl{0};
        std::atomic<U64> lgt_ns{0};
        std::atomic<U64> fam_us[24] = {};
        std::atomic<U64> fam_draws[24] = {};
        std::atomic<U64> rigged_rec{0};
        std::atomic<U64> skin_up{0};
        std::atomic<U64> skin_bl_fill{0};
        std::atomic<U64> skin_bl_of{0};
        std::atomic<U64> skin_base_wr{0};
        std::atomic<U64> e3_rig_us[3] = {};
        std::atomic<U64> e3_pal_us{0};
        std::atomic<U64> als_n[4] = {};
        std::atomic<U64> als_us[4] = {};
        std::atomic<U64> als_cause[8] = {};
        std::atomic<U64> als_val_pass[5] = {};
        std::atomic<U64> als_val_ring{0};
        std::atomic<U64> als_val_l3{0};
        std::atomic<U64> pin_store{0};
        std::atomic<U64> pin_refuse_live{0};
        std::atomic<U64> pin_refuse_cover{0};
        std::atomic<U64> ring_hw[16] = {};
        std::atomic<U64> setb_us[4] = {};
        std::atomic<U64> ens_hit{0};
        std::atomic<U64> ens_alloc{0};
        std::atomic<U64> set1_dead_purge{0};
        std::atomic<U64> phase_us[16] = {};
        std::atomic<U64> idle_us[32] = {};
        std::atomic<U64> img_us[12] = {};
        std::atomic<U64> mlp_us[16] = {};
        std::atomic<U64> tex_enq{0};
        std::atomic<U64> tex_pub{0};
        std::atomic<U64> tex_fail{0};
        std::atomic<U64> tex_dec{0};
        std::atomic<U64> img_pri_skip{0};
        std::atomic<U64> img_pri_full{0};
        std::atomic<U64> geo_enq{0};
        std::atomic<U64> geo_pub{0};
        std::atomic<U64> geo_pub_us{0};
        std::atomic<U64> geo_orphan{0};
        std::atomic<U64> geo_dis{0};
        std::atomic<U64> geo_inl{0};
        std::atomic<U64> geo_defer{0};
        std::atomic<U64> geo_snap_bytes{0};
        std::atomic<U64> geo_rsn_alpha{0};
        std::atomic<U64> geo_rsn_afill{0};
        std::atomic<U64> geo_rsn_geom{0};
        std::atomic<U64> geo_rsn_gfill{0};
        std::atomic<U64> geo_dirty_site[24] = {};
        std::atomic<U64> geo_rsn_geomb{0};
        std::atomic<U64> geo_rsn_gbfill{0};
        std::atomic<U64> gupd_us[5] = {};
        std::atomic<U64> bake_enq{0};
        std::atomic<U64> bake_pub{0};
        std::atomic<U64> bake_defer{0};
        std::atomic<U64> bake_drain_us{0};

        void reset()
        {
            pipe_bind = 0; pipe_skip = 0; desc_bind = 0; desc_skip = 0;
            mv_push = 0; mv_skip = 0; vp_set = 0; vp_skip = 0;
            set_build = 0; set_memo = 0; set_memo_fill = 0; populate = 0;
            populate_us = 0;
            populate_bl = 0; populate_pl = 0; populate_blhit = 0;
            populate_hit = 0; populate_miss = 0;
            syncmat_call = 0; syncmat_build = 0;
            vb_bind = 0; vb_skip = 0; ib_bind = 0; ib_skip = 0;
            for (auto& v : draws_pass) v = 0;
            for (auto& v : draws_shadow_map) v = 0;
            for (auto& row : draws_shadow_site) for (auto& v : row) v = 0;
            shadow_cull = 0; shadow_rigged = 0;
            for (auto& v : shadow_rigged_map) v = 0;
            bkt_patch = 0; bkt_range = 0; bkt_rec = 0; bkt_skip = 0; mat_draws = 0; mat_bindless_draws = 0;
            for (auto& row : mat_cen) for (auto& v : row) v = 0;
            mdi_call = 0; mdi_rec = 0; mdi_zero = 0; mdi_dyn = 0; mdi_full = 0;
            alp_run = 0; alp_col = 0; alp_inl = 0;
            for (auto& v : alpha_us) v = 0;
            for (auto& v : emi_us) v = 0;
            emi_grp = 0; emi_rtn = 0;
            for (auto& v : emi_n) v = 0;
            for (auto& v : lgt_us) v = 0;
            lgt_nl = 0; lgt_ns = 0;
            for (auto& v : fam_us) v = 0;
            for (auto& v : fam_draws) v = 0;
            rigged_rec = 0;
            skin_up = 0;
            for (auto& v : e3_rig_us) v = 0;
            e3_pal_us = 0;
            for (auto& v : als_n) v = 0;
            for (auto& v : als_us) v = 0;
            for (auto& v : als_cause) v = 0;
            for (auto& v : als_val_pass) v = 0;
            als_val_ring = 0; als_val_l3 = 0;
            pin_store = 0; pin_refuse_live = 0; pin_refuse_cover = 0;
            for (auto& v : ring_hw) v = 0;
            for (auto& v : setb_us) v = 0;
            ens_hit = 0; ens_alloc = 0;
            skin_bl_fill = 0; skin_bl_of = 0; skin_base_wr = 0;
            for (auto& v : phase_us) v = 0;
            for (auto& v : idle_us) v = 0;
            for (auto& v : img_us) v = 0;
            for (auto& v : mlp_us) v = 0;
            for (auto& v : gupd_us) v = 0;
            tex_enq = 0; tex_pub = 0; tex_fail = 0; tex_dec = 0;
            img_pri_skip = 0; img_pri_full = 0;
            geo_enq = 0; geo_pub = 0; geo_pub_us = 0; geo_dis = 0; geo_inl = 0; geo_defer = 0;
            geo_snap_bytes = 0;
            geo_rsn_alpha = 0; geo_rsn_afill = 0; geo_rsn_geom = 0; geo_rsn_gfill = 0;
            for (auto& v : geo_dirty_site) v = 0;
            geo_rsn_geomb = 0; geo_rsn_gbfill = 0;
            bake_enq = 0; bake_pub = 0; bake_defer = 0; bake_drain_us = 0;
        }
    };
    extern VkPerfCounters gVkPerf;
    extern std::atomic<U64> gVkGeoInflightBytes;
    extern thread_local U32 gVkPerfPassTag;
    extern thread_local U32 gVkPerfShadowMapIndex;
    extern thread_local U32 gVkPerfShadowCtx;
    extern thread_local U32 gVkPerfShadowSection;
    extern thread_local U32 gVkPerfSetPath;
    extern thread_local U32 gVkPerfSetCause;
    extern thread_local U32 gVkPerfValFailKind;
    U32 perfPassBucket();

    enum : U32
    {
        VKPERF_SETPATH_EARLY    = 0,
        VKPERF_SETPATH_MEMO     = 1,
        VKPERF_SETPATH_BINDLESS = 2,
        VKPERF_SETPATH_BUILD    = 3,
    };
    enum : U32
    {
        VKPERF_SETCZ_NONE        = 0,
        VKPERF_SETCZ_NONBINDLESS = 1,
        VKPERF_SETCZ_GLTF        = 2,
        VKPERF_SETCZ_LANE        = 3,
        VKPERF_SETCZ_NOSET       = 4,
        VKPERF_SETCZ_TOPO        = 5,
        VKPERF_SETCZ_VAL         = 6,
        VKPERF_SETCZ_MHDR        = 7,
        VKPERF_SETCZ_MVAL        = 8,
    };
    enum : U32
    {
        RINGHW_WLATMOS      = 0,
        RINGHW_WLSKY        = 1,
        RINGHW_AOUTIL       = 2,
        RINGHW_GLOBALF      = 3,
        RINGHW_WATERFOG     = 4,
        RINGHW_WATERV       = 5,
        RINGHW_RP           = 6,
        RINGHW_RPS          = 7,
        RINGHW_RPF          = 8,
        RINGHW_SSRUTIL      = 9,
        RINGHW_LIGHTS       = 10,
        RINGHW_LIGHTSSPEC   = 11,
        RINGHW_PBRTERRAINF  = 12,
        RINGHW_PBRTERRAIN   = 13,
        RINGHW_SHADOWUTIL   = 14,
        RINGHW_DEFERREDUTIL = 15,
    };

    extern std::atomic<U64> gVkPerDrawTopologyGen;
    extern std::atomic<U64> gVkViewDestroyGen;
    extern std::atomic<U64> gVkReloadEpoch;

    static constexpr U32 PDC_MAX_REFS = 16;
    struct PerDrawEvidence
    {
        const void* shader        = nullptr;
        U64         reloadEpoch   = 0;
        U64         topoGen       = 0;
        U64         attachmentSig = 0;
        U64         ringSig       = 0;
        U32         shape         = 0;
        U8          refCount      = 0;
        bool        pinnable      = false;
        S16         refSource[PDC_MAX_REFS] = {};
        void*       refView[PDC_MAX_REFS]   = {};
    };
    struct PerDrawCacheLane
    {
        VkDescriptorSet set[3]      = {};
        void*           tok[3]      = {};
        U64             pinEpoch[3] = {};
        PerDrawEvidence ev[3];
    };

    struct VkPerfPassScope
    {
        U32 mPrev;
        VkPerfPassScope(U32 tag) : mPrev(gVkPerfPassTag) { gVkPerfPassTag = tag; }
        ~VkPerfPassScope() { gVkPerfPassTag = mPrev; }
    };

    struct VkPerfShadowCtxScope
    {
        U32 mPrev;
        VkPerfShadowCtxScope(U32 ctx) : mPrev(gVkPerfShadowCtx) { gVkPerfShadowCtx = ctx; }
        ~VkPerfShadowCtxScope() { gVkPerfShadowCtx = mPrev; }
    };

    struct VkPerfShadowSectionScope
    {
        U32 mPrev;
        VkPerfShadowSectionScope(U32 sec) : mPrev(gVkPerfShadowSection) { gVkPerfShadowSection = sec; }
        ~VkPerfShadowSectionScope() { gVkPerfShadowSection = mPrev; }
    };

    struct VkPerfPhaseScope
    {
        U64 mT0;
        U32 mIdx;
        VkPerfPhaseScope(U32 idx);
        ~VkPerfPhaseScope();
    };

    struct VkPerfIdleScope
    {
        U64 mT0;
        U32 mIdx;
        VkPerfIdleScope(U32 idx);
        ~VkPerfIdleScope();
    };

    struct VkPerfImgScope
    {
        U64 mT0;
        U32 mIdx;
        VkPerfImgScope(U32 idx);
        ~VkPerfImgScope();
    };

    struct VkPerfMainScope
    {
        U64 mT0;
        U32 mIdx;
        VkPerfMainScope(U32 idx);
        ~VkPerfMainScope();
    };

    bool perfLogEnabled();

    const float* getCurrentModelviewMatrix();

    struct ScenePerDrawBindings
    {
        static constexpr U32 MAX_SAMPLERS = 35;
        static constexpr U32 MAX_UBO_WRITES = 30;

        VkDescriptorSetLayout layout      = VK_NULL_HANDLE;

        U64                   dynamic_mask = 0;
        U64                   layout_binding_mask = 0;

        VkBuffer              ubo         = VK_NULL_HANDLE;
        U32                   ubo_binding = 0;
        VkDeviceSize          ubo_size    = 0;

        VkSampler             sampler     = VK_NULL_HANDLE;

        U32                   sampler_count             = 0;
        U32                   sampler_bindings[MAX_SAMPLERS] = {};
        VkImageView           sampler_views[MAX_SAMPLERS]    = {};
        VkSampler             sampler_samplers[MAX_SAMPLERS] = {};
        char                  sampler_sources[MAX_SAMPLERS]  = {};

        struct UBOEntry
        {
            U32          binding = 0;
            VkBuffer     buf     = VK_NULL_HANDLE;
            VkDeviceSize offset  = 0;
            VkDeviceSize size    = 0;
        };
        U32      ubo_count                 = 0;
        UBOEntry ubo_writes[MAX_UBO_WRITES] = {};
    };

    bool ensureScenePerDrawDescriptorSet(const ScenePerDrawBindings& bindings,
                                         VkDescriptorSet*            out_set,
                                         void**                      out_token = nullptr);

    U64  getScenePerDrawCacheEpoch();
    void pinScenePerDrawEntry(void* token);
    void releaseScenePerDrawEntry(void* token, U64 epoch);

    void notifyWindowResize(U32 width, U32 height);

    VkImageView getDefaultFallbackVkImageView();
    VkImageView getDefaultFallbackCubeArrayVkImageView();
    VkImageView getDefaultFallbackCubeVkImageView();
    VkImageView getDefaultFallback3DVkImageView();
    VkImageView getDefaultFallbackShadowVkImageView();
}

#endif // LL_LLVKLOADER_H
