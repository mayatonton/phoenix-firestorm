/**
* @file llvkloader.h
* @brief AYAstorm r41 Vulkan loader + instance lifecycle (volk-based)
*
* $LicenseInfo:firstyear=2026&license=viewerlgpl$
* AYAstorm Viewer Source Code
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
* $/LicenseInfo$
*/

#ifndef LL_LLVKLOADER_H
#define LL_LLVKLOADER_H

#include "volk.h"

#include <vector>

class LLWindow;

namespace LLVKLoader
{
    bool initVulkan();
    void shutdownVulkan();

    bool isVulkanInitialized();

    bool beginFrame(bool acquire_swapchain = true);
    bool endFrame();
    VkCommandBuffer getCurrentCommandBuffer();

    bool     isOcclusionQueryVkAvailable();
    uint32_t acquireOcclusionQueryVk();
    void     releaseOcclusionQueryVk(uint32_t handle);
    void     cmdBeginOcclusionQueryVk(VkCommandBuffer cmd, uint32_t handle);
    void     cmdEndOcclusionQueryVk(VkCommandBuffer cmd, uint32_t handle);
    bool     getOcclusionQueryResultVk(uint32_t handle, bool& available, uint64_t& samples);

    void setScissor(S32 x, S32 y, S32 w, S32 h);

    void disableScissor();

    void setRenderViewport(S32 x, S32 y, S32 w, S32 h);

    VkDevice         getDevice();
    VkPipelineCache  getPipelineCache();

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

    U32 getCurrentFrameIndex();
    U32 getMonotonicFrameCount();

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

    struct ClipPlane_PerShaderBind
    {
        float clip_plane[4];
    };
    static_assert(sizeof(ClipPlane_PerShaderBind) == 16,
                  "ClipPlane_PerShaderBind size mismatch (std140 expects 16 B)");

    void writeCurrentClipPlaneUBO(const ClipPlane_PerShaderBind& data);

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

    struct Gaussian_PerShaderBind
    {
        float resScale;
        float _pad0;
        float direction[2];
    };
    static_assert(sizeof(Gaussian_PerShaderBind) == 16,
                  "Gaussian_PerShaderBind size mismatch (std140 expects 16 B)");

    void writeCurrentGaussianUBO(const Gaussian_PerShaderBind& data);

    void pushCurrentModelviewMatrix(const float modelview_matrix[16]);

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
                               const DynamicRenderingAttachment* depth_attachment);
    void endDynamicRendering();

    VkShaderModule loadSpirvShaderModule(const U32* spv_code, size_t code_size_bytes);

    VkShaderModule loadSpirvShaderModuleFromMemory(const std::vector<unsigned int>& spirv);

    VkFormat llGlEnumToVkFormat(U32 ll_gl_intformat);

    VkFilter             llGlEnumToVkFilter     (U32 ll_gl_filter);
    VkSamplerMipmapMode  llGlEnumToVkMipmapMode (U32 ll_gl_filter);
    VkSamplerAddressMode llGlEnumToVkWrap       (U32 ll_gl_wrap);
    VkBlendFactor        llGlEnumToVkBlendFactor(U32 ll_gl_blend);
    VkCompareOp          llGlEnumToVkCompareOp  (U32 ll_gl_func);
    VkCullModeFlags      llGlEnumToVkCullMode   (U32 ll_gl_cull);
    VkStencilOp          llGlEnumToVkStencilOp  (U32 ll_gl_op);
    U32                  vkFormatBytesPerPixel  (VkFormat format);
    U32                  glFormatSourceComponents(U32 ll_gl_format);

    bool createVertexBufferVk(U32     size_bytes,
                              VkBuffer& out_buffer,
                              void*&    out_allocation,
                              void**    out_mapped);

    bool createIndexBufferVk (U32     size_bytes,
                              VkBuffer& out_buffer,
                              void*&    out_allocation,
                              void**    out_mapped);

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

    struct LightMinimumAlpha_PerProgramBind
    {
        float minimum_alpha;
        float _pad0;
        float _pad1;
        float _pad2;
    };
    static_assert(sizeof(LightMinimumAlpha_PerProgramBind) == 16,
                  "LightMinimumAlpha_PerProgramBind size mismatch (std140 expects 16 B)");
    void writeCurrentLightMinimumAlphaUBO(const LightMinimumAlpha_PerProgramBind& data);
    bool getSharedLightMinimumAlphaUBO(VkBuffer& out_buffer, void*& out_mapped);

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

    struct ImpostorF_UBO
    {
        float minimum_alpha;
        float pad0;
        float pad1;
        float pad2;
    };
    static_assert(sizeof(ImpostorF_UBO) == 16,
                  "ImpostorF_UBO size mismatch (std140 expects 16 B)");

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

    struct OcclusionCube_PerProgramBind
    {
        float box_center[3];
        float _pad0;
        float box_size[3];
        float _pad1;
    };
    static_assert(sizeof(OcclusionCube_PerProgramBind) == 32,
                  "OcclusionCube_PerProgramBind size mismatch (std140 expects 32 B)");

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

    struct ScreenSpaceReflPostF_PerProgramBind
    {
        float zNear;
        float zFar;
        float _screenSpaceReflPostF_pad0;
        float _screenSpaceReflPostF_pad1;
    };
    static_assert(sizeof(ScreenSpaceReflPostF_PerProgramBind) == 16,
                  "ScreenSpaceReflPostF_PerProgramBind size mismatch (std140 expects 16 B)");

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
    static constexpr U32 ALPHAF_UBO_LIGHT_ARRAYS_SIZE = 512;

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
    void tickDeferredBufferFreeQueue();
    void tickDeferredImageFreeQueue();
    void destroyPipelineVk           (VkPipeline            pipeline);
    void destroyShaderModuleVk       (VkShaderModule        shader_module);
    void destroyPipelineLayoutVk     (VkPipelineLayout      pipeline_layout);
    void destroyDescriptorSetLayoutVk(VkDescriptorSetLayout descriptor_set_layout);
    void tickDeferredObjectFreeQueue();

    void bindVertexBufferVk(VkCommandBuffer cmd_buf,
                            VkBuffer        buffer,
                            VkDeviceSize    offset,
                            U32             firstBinding = 0);

    VkDescriptorSet getCurrentPerFrameDescriptorSet();

    void bindIndexBufferVk (VkCommandBuffer cmd_buf,
                            VkBuffer        buffer,
                            VkDeviceSize    offset,
                            VkIndexType     index_type);

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

    bool createReadbackImageVk(U32          width,
                               U32          height,
                               VkFormat     format,
                               VkImage&     out_image,
                               VkImageView& out_view,
                               void*&       out_allocation);

    bool uploadImageDataVk(VkImage     image,
                           U32         width,
                           U32         height,
                           VkFormat    format,
                           const void* data,
                           U32         data_size_bytes,
                           U32         mip_level = 0);

    bool generateMipChainBlitVk(VkImage image, U32 base_w, U32 base_h, U32 mip_count, VkFormat format);

    bool generateCubeMipChainBlitVk(VkImage image, U32 resolution, U32 mip_count, VkFormat format);

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
                             VkFormat    format,
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
                               VkFormat          format,
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
                                          U32           height);

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

    void transitionImageLayoutVk(VkImage              image,
                                 VkImageAspectFlags   aspect_mask,
                                 VkImageLayout        old_layout,
                                 VkImageLayout        new_layout,
                                 VkPipelineStageFlags src_stage_mask,
                                 VkPipelineStageFlags dst_stage_mask,
                                 VkAccessFlags        src_access_mask,
                                 VkAccessFlags        dst_access_mask);

    bool         initSurface(LLWindow* window);
    void         shutdownSurface();
    VkSurfaceKHR getSurface();

    bool           initSwapchain();

    bool        isVulkanPresentationEnabled();
    void        setVulkanPresentationEnabled(bool enabled);

    U32         getRenderBackendMode();
    bool        shouldUseVulkanRender();
    void        resetVulkanRenderSuspend();

    class VkRenderSuspendScope
    {
    public:
        VkRenderSuspendScope();
        ~VkRenderSuspendScope();
    };

    VkImageView getCurrentSwapchainImageView();
    VkExtent2D  getSwapchainExtent();
    U32         getCurrentSwapchainImageIndex();

    VkFormat    getSwapchainFormat();

    void        beginSwapchainRendering();
    void        endSwapchainRendering();

    bool        isInRenderPassScope();
    void        setupViewportAndScissor(VkCommandBuffer cmd, bool screen_space_copy = false);

    const float* getCurrentModelviewMatrix();

    struct ScenePerDrawBindings
    {
        static constexpr U32 MAX_SAMPLERS = 35;
        static constexpr U32 MAX_UBO_WRITES = 24;

        VkDescriptorSetLayout layout      = VK_NULL_HANDLE;

        VkBuffer              ubo         = VK_NULL_HANDLE;
        U32                   ubo_binding = 0;
        VkDeviceSize          ubo_size    = 0;

        VkSampler             sampler     = VK_NULL_HANDLE;

        U32                   sampler_count             = 0;
        U32                   sampler_bindings[MAX_SAMPLERS] = {};
        VkImageView           sampler_views[MAX_SAMPLERS]    = {};
        VkSampler             sampler_samplers[MAX_SAMPLERS] = {};

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
                                         VkDescriptorSet*            out_set);
    void tickScenePerDrawDescriptorCache();

    void notifyWindowResize(U32 width, U32 height);

    VkImageView getDefaultFallbackVkImageView();
    VkImageView getDefaultFallbackCubeArrayVkImageView();
    VkImageView getDefaultFallbackCubeVkImageView();
    VkImageView getDefaultFallback3DVkImageView();
}

#endif // LL_LLVKLOADER_H
