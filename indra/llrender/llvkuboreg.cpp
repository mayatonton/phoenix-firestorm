/**
* @file llvkuboreg.cpp
* @brief AYAstorm r41 UBO offset/size single-source-of-truth verification (UBOReg)
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

#include "linden_common.h"

#include "llvkuboreg.h"

#include "llglslshader.h"
#include "llvkloader.h"

#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

using LLVkUboReg::MemberEntry;
using LLVkUboReg::BlockEntry;
using LLVkUboReg::PcEntry;
using LLVkUboReg::LedgerEntry;

namespace
{
    constexpr MemberEntry kM_PerFrameMatrixUBO[] = {
        UBOREG_M(PerFrameMatrixUBO, projection_matrix),
        UBOREG_M(PerFrameMatrixUBO, inverse_projection_matrix),
        UBOREG_M(PerFrameMatrixUBO, identity_matrix),
        UBOREG_M(PerFrameMatrixUBO, last_modelview_matrix),
    };

    constexpr MemberEntry kM_TextureMatrixUBO[] = {
        UBOREG_M(TextureMatrixUBO, texture_matrix),
    };

    constexpr MemberEntry kM_ShadowParams[] = {
        UBOREG_M(ShadowParams_PerShaderBind, shadow_target_width),
        UBOREG_M(ShadowParams_PerShaderBind, _pad0),
        UBOREG_M(ShadowParams_PerShaderBind, _pad1),
        UBOREG_M(ShadowParams_PerShaderBind, _pad2),
    };

    constexpr MemberEntry kM_ShadowViewProj[] = {
        UBOREG_M(ShadowViewProj_PerPass, shadow_viewproj),
    };

    constexpr MemberEntry kM_PreviewAmbient[] = {
        UBOREG_M(PreviewAmbient_PerShaderBind, ambient_color),
    };

    constexpr MemberEntry kM_PbrTerrain[] = {
        UBOREG_M(PbrTerrain_PerShaderBind, region_scale),
        UBOREG_M(PbrTerrain_PerShaderBind, terrain_texture_transforms),
    };

    constexpr MemberEntry kM_StarTime[] = {
        UBOREG_M(StarTime_PerShaderBind, time),
        UBOREG_M(StarTime_PerShaderBind, blend_factor),
        UBOREG_M(StarTime_PerShaderBind, custom_alpha),
    };

    constexpr MemberEntry kM_AvatarVelocityPalette[] = {
        UBOREG_M(AvatarVelocityPalette_PerShaderBind, lastMatrixPalette),
    };

    constexpr MemberEntry kM_GlowCombine[] = {
        UBOREG_M(GlowCombine_PerShaderBind, greyscale_str),
        UBOREG_M(GlowCombine_PerShaderBind, sepia_str),
        UBOREG_M(GlowCombine_PerShaderBind, num_colors),
        UBOREG_M(GlowCombine_PerShaderBind, _pad0),
    };

    constexpr MemberEntry kM_SMAABlendWeightsF[] = {
        UBOREG_M(SMAABlendWeightsF_PerProgramBind, subsampleIndices),
    };

    constexpr MemberEntry kM_WindlightAtmos[] = {
        UBOREG_M(WindlightAtmos_PerProgramBind, sunlight_color),
        UBOREG_M(WindlightAtmos_PerProgramBind, sun_up_factor),
        UBOREG_M(WindlightAtmos_PerProgramBind, moonlight_color),
        UBOREG_M(WindlightAtmos_PerProgramBind, classic_mode_wl),
        UBOREG_M(WindlightAtmos_PerProgramBind, ambient_color),
        UBOREG_M(WindlightAtmos_PerProgramBind, aya_visual_realism_enabled),
        UBOREG_M(WindlightAtmos_PerProgramBind, blue_horizon),
        UBOREG_M(WindlightAtmos_PerProgramBind, aya_r14_volumetric_atmosphere_enabled),
        UBOREG_M(WindlightAtmos_PerProgramBind, blue_density),
        UBOREG_M(WindlightAtmos_PerProgramBind, aya_r14_strength),
        UBOREG_M(WindlightAtmos_PerProgramBind, glow),
        UBOREG_M(WindlightAtmos_PerProgramBind, aya_r16_strength),
        UBOREG_M(WindlightAtmos_PerProgramBind, lightnorm),
        UBOREG_M(WindlightAtmos_PerProgramBind, aya_r16_aerial_perspective_enabled),
        UBOREG_M(WindlightAtmos_PerProgramBind, haze_density),
        UBOREG_M(WindlightAtmos_PerProgramBind, density_multiplier),
        UBOREG_M(WindlightAtmos_PerProgramBind, distance_multiplier),
        UBOREG_M(WindlightAtmos_PerProgramBind, max_y),
        UBOREG_M(WindlightAtmos_PerProgramBind, haze_horizon),
        UBOREG_M(WindlightAtmos_PerProgramBind, cloud_shadow),
        UBOREG_M(WindlightAtmos_PerProgramBind, sun_moon_glow_factor),
        UBOREG_M(WindlightAtmos_PerProgramBind, sky_sunlight_scale),
        UBOREG_M(WindlightAtmos_PerProgramBind, sky_ambient_scale),
        UBOREG_M(WindlightAtmos_PerProgramBind, _wlAtmos_pad0),
        UBOREG_M(WindlightAtmos_PerProgramBind, _wlAtmos_pad1),
        UBOREG_M(WindlightAtmos_PerProgramBind, _wlAtmos_pad2),
    };

    constexpr MemberEntry kM_WindlightSky[] = {
        UBOREG_M(WindlightSky_PerProgramBind, sun_dir_sky),
        UBOREG_M(WindlightSky_PerProgramBind, _pad_sky0),
        UBOREG_M(WindlightSky_PerProgramBind, moon_dir_sky),
        UBOREG_M(WindlightSky_PerProgramBind, _pad_sky1),
    };

    constexpr MemberEntry kM_WindlightHDR[] = {
        UBOREG_M(WindlightHDR_PerProgramBind, sky_hdr_scale),
        UBOREG_MN(WindlightHDR_PerProgramBind, _wlHDR_pad0, "_pad_hdr0"),
        UBOREG_MN(WindlightHDR_PerProgramBind, _wlHDR_pad1, "_pad_hdr1"),
        UBOREG_MN(WindlightHDR_PerProgramBind, _wlHDR_pad2, "_pad_hdr2"),
    };

    constexpr MemberEntry kM_WindlightLight[] = {
        UBOREG_M(WindlightLight_PerProgramBind, scene_light_strength),
        UBOREG_MN(WindlightLight_PerProgramBind, _wlLight_pad0, "_pad_light0"),
        UBOREG_MN(WindlightLight_PerProgramBind, _wlLight_pad1, "_pad_light1"),
        UBOREG_MN(WindlightLight_PerProgramBind, _wlLight_pad2, "_pad_light2"),
    };

    constexpr MemberEntry kM_WaterFog[] = {
        UBOREG_MN(WaterFog_PerProgramBind, waterPlane, "_wfog_waterPlane"),
        UBOREG_MN(WaterFog_PerProgramBind, waterFogColor, "_wfog_waterFogColor"),
        UBOREG_MN(WaterFog_PerProgramBind, waterFogDensity, "_wfog_waterFogDensity"),
        UBOREG_MN(WaterFog_PerProgramBind, waterFogKS, "_wfog_waterFogKS"),
        UBOREG_MN(WaterFog_PerProgramBind, _waterFog_pad0, "_pad_waterfog0"),
        UBOREG_MN(WaterFog_PerProgramBind, _waterFog_pad1, "_pad_waterfog1"),
    };

    constexpr MemberEntry kM_Water[] = {
        UBOREG_M(Water_PerProgramBind, waveDir1),
        UBOREG_M(Water_PerProgramBind, waveDir2),
        UBOREG_M(Water_PerProgramBind, time),
        UBOREG_M(Water_PerProgramBind, eyeVec),
        UBOREG_M(Water_PerProgramBind, waterHeight),
        UBOREG_M(Water_PerProgramBind, lightDir),
    };

    constexpr MemberEntry kM_ReflectionProbe[] = {
        UBOREG_M(ReflectionProbe_PerProgramBind, reflection_probe_ambiance),
        UBOREG_MN(ReflectionProbe_PerProgramBind, _refprobe_pad0, "_reflectionProbeF_pad0"),
        UBOREG_MN(ReflectionProbe_PerProgramBind, _refprobe_pad1, "_reflectionProbeF_pad1"),
        UBOREG_MN(ReflectionProbe_PerProgramBind, _refprobe_pad2, "_reflectionProbeF_pad2"),
        UBOREG_M3(ReflectionProbe_PerProgramBind, env_mat, "env_mat"),
    };

    constexpr MemberEntry kM_GlobalF[] = {
        UBOREG_M(GlobalF_PerProgramBind, mirror_flag),
        UBOREG_M(GlobalF_PerProgramBind, region_clip_flag),
        UBOREG_M(GlobalF_PerProgramBind, _globalF_pad1),
        UBOREG_M(GlobalF_PerProgramBind, _globalF_pad2),
        UBOREG_M(GlobalF_PerProgramBind, clipPlane),
        UBOREG_M(GlobalF_PerProgramBind, regionClip0),
        UBOREG_M(GlobalF_PerProgramBind, regionClip1),
        UBOREG_M(GlobalF_PerProgramBind, regionClip2),
        UBOREG_M(GlobalF_PerProgramBind, regionClip3),
    };

    constexpr MemberEntry kM_AoUtil[] = {
        UBOREG_M(AoUtil_PerProgramBind, screen_res),
        UBOREG_M(AoUtil_PerProgramBind, ssao_radius),
        UBOREG_M(AoUtil_PerProgramBind, ssao_max_radius),
        UBOREG_M(AoUtil_PerProgramBind, ssao_factor),
        UBOREG_M(AoUtil_PerProgramBind, ssao_factor_inv),
        UBOREG_M(AoUtil_PerProgramBind, _aoUtil_pad0),
        UBOREG_M(AoUtil_PerProgramBind, _aoUtil_pad1),
    };

    constexpr MemberEntry kM_TonemapUtilF[] = {
        UBOREG_M(TonemapUtilF_PerProgramBind, exposure),
        UBOREG_M(TonemapUtilF_PerProgramBind, tonemap_mix),
        UBOREG_M(TonemapUtilF_PerProgramBind, tonemap_type),
        UBOREG_M(TonemapUtilF_PerProgramBind, _tonemapUtilF_pad0),
    };

    constexpr MemberEntry kM_PbrTerrainF[] = {
        UBOREG_M(PbrTerrainF_PerProgramBind, baseColorFactors),
        UBOREG_M(PbrTerrainF_PerProgramBind, metallicFactors),
        UBOREG_M(PbrTerrainF_PerProgramBind, roughnessFactors),
        UBOREG_M(PbrTerrainF_PerProgramBind, emissiveColors),
        UBOREG_M(PbrTerrainF_PerProgramBind, minimum_alphas),
    };

    constexpr MemberEntry kM_DeferredUtil[] = {
        UBOREG_M(DeferredUtil_PerProgramBind, proj_mat),
        UBOREG_M(DeferredUtil_PerProgramBind, waterPlane),
        UBOREG_M(DeferredUtil_PerProgramBind, proj_n),
        UBOREG_M(DeferredUtil_PerProgramBind, proj_focus),
        UBOREG_M(DeferredUtil_PerProgramBind, proj_p),
        UBOREG_M(DeferredUtil_PerProgramBind, proj_lod),
        UBOREG_M(DeferredUtil_PerProgramBind, color),
        UBOREG_M(DeferredUtil_PerProgramBind, size),
        UBOREG_M(DeferredUtil_PerProgramBind, screen_res),
        UBOREG_M(DeferredUtil_PerProgramBind, proj_range),
        UBOREG_M(DeferredUtil_PerProgramBind, proj_ambiance),
        UBOREG_M(DeferredUtil_PerProgramBind, _deferredUtil_pad_waterSign),
        UBOREG_M(DeferredUtil_PerProgramBind, classic_mode),
        UBOREG_M(DeferredUtil_PerProgramBind, _deferredUtil_pad0),
        UBOREG_M(DeferredUtil_PerProgramBind, _deferredUtil_pad1),
    };

    constexpr MemberEntry kM_ShadowUtil[] = {
        UBOREG_M(ShadowUtil_PerProgramBind, shadow_matrix),
        UBOREG_M(ShadowUtil_PerProgramBind, shadow_clip),
        UBOREG_M(ShadowUtil_PerProgramBind, sun_dir),
        UBOREG_M(ShadowUtil_PerProgramBind, shadow_bias),
        UBOREG_M(ShadowUtil_PerProgramBind, moon_dir),
        UBOREG_M(ShadowUtil_PerProgramBind, shadow_offset),
        UBOREG_M(ShadowUtil_PerProgramBind, shadow_res),
        UBOREG_M(ShadowUtil_PerProgramBind, proj_shadow_res),
        UBOREG_M(ShadowUtil_PerProgramBind, shadow_softness),
        UBOREG_M(ShadowUtil_PerProgramBind, spot_shadow_bias),
        UBOREG_M(ShadowUtil_PerProgramBind, spot_shadow_offset),
        UBOREG_M(ShadowUtil_PerProgramBind, _shadowUtil_pad0),
    };

    constexpr MemberEntry kM_ReflectionProbes[] = {
        UBOREG_M(ReflectionProbes_PerProgramBind, refBox),
        UBOREG_M(ReflectionProbes_PerProgramBind, heroBox),
        UBOREG_M(ReflectionProbes_PerProgramBind, refSphere),
        UBOREG_M(ReflectionProbes_PerProgramBind, refParams),
        UBOREG_M(ReflectionProbes_PerProgramBind, heroSphere),
        UBOREG_M(ReflectionProbes_PerProgramBind, refIndex),
        UBOREG_M(ReflectionProbes_PerProgramBind, refNeighbor),
        UBOREG_M(ReflectionProbes_PerProgramBind, refBucket),
        UBOREG_M(ReflectionProbes_PerProgramBind, refmapCount),
        UBOREG_M(ReflectionProbes_PerProgramBind, heroShape),
        UBOREG_M(ReflectionProbes_PerProgramBind, heroMipCount),
        UBOREG_M(ReflectionProbes_PerProgramBind, heroProbeCount),
    };

    constexpr MemberEntry kM_AvatarSkin[] = {
        UBOREG_M(AvatarSkin_PerProgramBind, matrixPalette),
    };

    constexpr MemberEntry kM_ObjectSkin[] = {
        UBOREG_M(ObjectSkin_PerProgramBind, matrixPalette),
        UBOREG_M(ObjectSkin_PerProgramBind, lastMatrixPalette),
    };

    constexpr MemberEntry kM_Lights[] = {
        UBOREG_M(Lights_PerProgramBind, light_position),
        UBOREG_M(Lights_PerProgramBind, light_diffuse),
    };

    constexpr MemberEntry kM_LightsSpecular[] = {
        UBOREG_M(LightsSpecular_PerProgramBind, light_position),
        UBOREG_M(LightsSpecular_PerProgramBind, light_attenuation),
        UBOREG_M(LightsSpecular_PerProgramBind, light_diffuse),
    };

    constexpr MemberEntry kM_PBRMaterial[] = {
        UBOREG_M(PBRMaterial_PerMaterial, texture_base_color_transform),
        UBOREG_M(PBRMaterial_PerMaterial, texture_normal_transform),
        UBOREG_M(PBRMaterial_PerMaterial, texture_metallic_roughness_transform),
        UBOREG_M(PBRMaterial_PerMaterial, texture_emissive_transform),
    };

    constexpr MemberEntry kM_DrawColor[] = {
        UBOREG_M(DrawColor_PerShaderBind, color),
    };

    constexpr MemberEntry kM_VolumetricLightF[] = {
        UBOREG_M(VolumetricLightF_PerProgramBind, godray_res),
        UBOREG_M(VolumetricLightF_PerProgramBind, godray_multiplier),
        UBOREG_M(VolumetricLightF_PerProgramBind, falloff_multiplier),
        UBOREG_M(VolumetricLightF_PerProgramBind, _volumetricLightF_pad0),
    };

    constexpr MemberEntry kM_MotionBlurF[] = {
        UBOREG_M(MotionBlurF_PerProgramBind, _mbF_screen_res),
        UBOREG_M(MotionBlurF_PerProgramBind, _mbF_motion_blur_strength),
        UBOREG_M(MotionBlurF_PerProgramBind, _motionBlurF_pad0),
    };

    constexpr MemberEntry kM_PostGammaCorrect[] = {
        UBOREG_M(PostGammaCorrect_PerProgramBind, gamma),
    };

    constexpr MemberEntry kM_CofF[] = {
        UBOREG_M(CofF_PerProgramBind, focal_distance),
        UBOREG_M(CofF_PerProgramBind, blur_constant),
        UBOREG_M(CofF_PerProgramBind, tan_pixel_angle),
        UBOREG_M(CofF_PerProgramBind, magnification),
        UBOREG_M(CofF_PerProgramBind, max_cof),
        UBOREG_MN(CofF_PerProgramBind, _cofF_pad0, "_pad0"),
        UBOREG_MN(CofF_PerProgramBind, _cofF_pad1, "_pad1"),
        UBOREG_MN(CofF_PerProgramBind, _cofF_pad2, "_pad2"),
    };

    constexpr MemberEntry kM_DofCombineF[] = {
        UBOREG_M(DofCombineF_PerProgramBind, _dofC_screen_res),
        UBOREG_M(DofCombineF_PerProgramBind, _dofC_pad0),
        UBOREG_M(DofCombineF_PerProgramBind, _dofC_max_cof),
        UBOREG_M(DofCombineF_PerProgramBind, _dofC_res_scale),
        UBOREG_M(DofCombineF_PerProgramBind, _dofC_dof_width),
        UBOREG_M(DofCombineF_PerProgramBind, _dofC_dof_height),
    };

    constexpr MemberEntry kM_FsObjectIDF[] = {
        UBOREG_M(FsObjectIDF_PerProgramBind, object_id_packed),
    };

    constexpr MemberEntry kM_FxaaShared[] = {
        UBOREG_M(FxaaShared_PerProgramBind, tc_scale),
        UBOREG_M(FxaaShared_PerProgramBind, rcp_screen_res),
        UBOREG_M(FxaaShared_PerProgramBind, rcp_frame_opt),
        UBOREG_M(FxaaShared_PerProgramBind, rcp_frame_opt2),
    };

    constexpr MemberEntry kM_AvatarVCloth[] = {
        UBOREG_M(AvatarVCloth_PerProgramBind, gWindDir),
        UBOREG_M(AvatarVCloth_PerProgramBind, gSinWaveParams),
        UBOREG_M(AvatarVCloth_PerProgramBind, gGravity),
    };

    constexpr MemberEntry kM_BlurLightF[] = {
        UBOREG_MN(BlurLightF_PerProgramBind, kern, "_blF_kern"),
        UBOREG_MN(BlurLightF_PerProgramBind, delta, "_blF_delta"),
        UBOREG_MN(BlurLightF_PerProgramBind, screen_res, "_blF_screen_res"),
        UBOREG_MN(BlurLightF_PerProgramBind, dist_factor, "_blF_dist_factor"),
        UBOREG_MN(BlurLightF_PerProgramBind, blur_size, "_blF_blur_size"),
        UBOREG_MN(BlurLightF_PerProgramBind, kern_scale, "_blF_kern_scale"),
        UBOREG_MN(BlurLightF_PerProgramBind, pad0, "_blurLightF_pad0"),
    };

    constexpr MemberEntry kM_Cloud[] = {
        UBOREG_M(Cloud_PerProgramBind, camPosLocal),
        UBOREG_MN(Cloud_PerProgramBind, _pad0, "_cloud_pad0"),
        UBOREG_M(Cloud_PerProgramBind, cloud_color),
        UBOREG_M(Cloud_PerProgramBind, cloud_scale_v),
        UBOREG_M(Cloud_PerProgramBind, cloud_pos_density1),
        UBOREG_MN(Cloud_PerProgramBind, _pad1, "_cloud_pad1"),
        UBOREG_M(Cloud_PerProgramBind, cloud_pos_density2),
        UBOREG_MN(Cloud_PerProgramBind, _pad2, "_cloud_pad2"),
        UBOREG_M(Cloud_PerProgramBind, blend_factor),
        UBOREG_M(Cloud_PerProgramBind, cloud_scale),
        UBOREG_M(Cloud_PerProgramBind, cloud_variance),
        UBOREG_M(Cloud_PerProgramBind, aya_r18_cloud_volumetric_enabled),
        UBOREG_M(Cloud_PerProgramBind, aya_r18_strength),
        UBOREG_MN(Cloud_PerProgramBind, _pad3, "_cloud_pad3"),
        UBOREG_MN(Cloud_PerProgramBind, _pad4, "_cloud_pad4"),
        UBOREG_MN(Cloud_PerProgramBind, _pad5, "_cloud_pad5"),
    };

    constexpr MemberEntry kM_ExposureF[] = {
        UBOREG_M(ExposureF_PerProgramBind, dynamic_exposure_params),
        UBOREG_M(ExposureF_PerProgramBind, dynamic_exposure_params2),
        UBOREG_M(ExposureF_PerProgramBind, dt),
        UBOREG_MN(ExposureF_PerProgramBind, _exposureF_pad0, "_pad0"),
        UBOREG_MN(ExposureF_PerProgramBind, _exposureF_pad1, "_pad1"),
        UBOREG_MN(ExposureF_PerProgramBind, _exposureF_pad2, "_pad2"),
    };

    constexpr MemberEntry kM_Glow[] = {
        UBOREG_M(Glow_PerProgramBind, glowDelta),
        UBOREG_M(Glow_PerProgramBind, glowStrength),
        UBOREG_M(Glow_PerProgramBind, _pad0),
    };

    constexpr MemberEntry kM_GodraysF[] = {
        UBOREG_M(GodraysF_PerProgramBind, sun_dir),
        UBOREG_MN(GodraysF_PerProgramBind, pad0, "_godraysF_pad0"),
        UBOREG_M(GodraysF_PerProgramBind, moon_dir),
        UBOREG_M(GodraysF_PerProgramBind, aya_r15_godrays_enabled),
        UBOREG_M(GodraysF_PerProgramBind, aya_r15_godrays_phase_exponent),
        UBOREG_M(GodraysF_PerProgramBind, aya_r15_godrays_strength),
        UBOREG_MN(GodraysF_PerProgramBind, pad1, "_godraysF_pad1"),
        UBOREG_MN(GodraysF_PerProgramBind, pad2, "_godraysF_pad2"),
    };

    constexpr MemberEntry kM_HazeF[] = {
        UBOREG_M(HazeF_PerProgramBind, sun_dir),
        UBOREG_MN(HazeF_PerProgramBind, pad0, "_hazeF_pad0"),
        UBOREG_M(HazeF_PerProgramBind, moon_dir),
        UBOREG_MN(HazeF_PerProgramBind, sun_up_factor, "sun_up_factor_haze"),
    };

    constexpr MemberEntry kM_IrradianceGen[] = {
        UBOREG_M(IrradianceGen_PerProgramBind, sourceIdx),
        UBOREG_M(IrradianceGen_PerProgramBind, max_probe_lod),
        UBOREG_MN(IrradianceGen_PerProgramBind, pad0, "_pad0"),
        UBOREG_MN(IrradianceGen_PerProgramBind, pad1, "_pad1"),
    };

    constexpr MemberEntry kM_LuminanceF[] = {
        UBOREG_M(LuminanceF_PerProgramBind, diffuse_luminance_scale),
        UBOREG_MN(LuminanceF_PerProgramBind, _luminanceF_pad0, "_pad0"),
        UBOREG_MN(LuminanceF_PerProgramBind, _luminanceF_pad1, "_pad1"),
        UBOREG_MN(LuminanceF_PerProgramBind, _luminanceF_pad2, "_pad2"),
    };

    constexpr MemberEntry kM_MoonF[] = {
        UBOREG_M(MoonF_PerProgramBind, color),
        UBOREG_M(MoonF_PerProgramBind, moon_dir),
        UBOREG_M(MoonF_PerProgramBind, moon_brightness),
    };

    constexpr MemberEntry kM_NormalDebug[] = {
        UBOREG_M(NormalDebug_PerProgramBind, debug_normal_draw_length),
        UBOREG_M(NormalDebug_PerProgramBind, _pad0),
        UBOREG_M(NormalDebug_PerProgramBind, _pad1),
        UBOREG_M(NormalDebug_PerProgramBind, _pad2),
    };

    constexpr MemberEntry kM_NormgenF[] = {
        UBOREG_M(NormgenF_PerProgramBind, stepX),
        UBOREG_M(NormgenF_PerProgramBind, stepY),
        UBOREG_M(NormgenF_PerProgramBind, norm_scale),
        UBOREG_M(NormgenF_PerProgramBind, bump_code),
    };

    constexpr MemberEntry kM_OneTextureFilter[] = {
        UBOREG_M(OneTextureFilter_PerProgramBind, tolerance),
        UBOREG_M(OneTextureFilter_PerProgramBind, _pad0),
        UBOREG_M(OneTextureFilter_PerProgramBind, _pad1),
        UBOREG_M(OneTextureFilter_PerProgramBind, _pad2),
    };

    constexpr MemberEntry kM_Pathfinding[] = {
        UBOREG_M(Pathfinding_PerProgramBind, tint),
        UBOREG_M(Pathfinding_PerProgramBind, ambiance),
        UBOREG_M(Pathfinding_PerProgramBind, alpha_scale),
        UBOREG_MN(Pathfinding_PerProgramBind, pad0, "_pad0"),
    };

    constexpr MemberEntry kM_PointLightPerDraw[] = {
        UBOREG_M(PointLightPerDraw, center),
        UBOREG_M(PointLightPerDraw, size),
        UBOREG_M(PointLightPerDraw, color),
        UBOREG_M(PointLightPerDraw, falloff),
        UBOREG_M(PointLightPerDraw, global_light_strength),
        UBOREG_M(PointLightPerDraw, classic_mode),
        UBOREG_MN(PointLightPerDraw, pad0, "_ppd_pad0"),
        UBOREG_MN(PointLightPerDraw, pad1, "_ppd_pad1"),
    };

    constexpr MemberEntry kM_PostF[] = {
        UBOREG_M(PostF_PerProgramBind, screen_res),
        UBOREG_M(PostF_PerProgramBind, max_cof),
        UBOREG_M(PostF_PerProgramBind, chroma_str),
    };

    constexpr MemberEntry kM_PostHQDoFF[] = {
        UBOREG_MN(PostF_PerProgramBind, screen_res, "_phqd_screen_res"),
        UBOREG_MN(PostF_PerProgramBind, max_cof, "_phqd_max_cof"),
        UBOREG_MN(PostF_PerProgramBind, chroma_str, "_phqd_chroma_str"),
    };

    constexpr MemberEntry kM_PostNoDoFF[] = {
        UBOREG_MN(PostNoDoFF_PerProgramBind, screen_res, "_pndff_screen_res"),
        UBOREG_MN(PostNoDoFF_PerProgramBind, chroma_str, "_pndff_chroma_str"),
    };

    constexpr MemberEntry kM_PostSnapshotFrame[] = {
        UBOREG_M(PostSnapshotFrame_PerProgramBind, screen_res),
        UBOREG_MN(PostSnapshotFrame_PerProgramBind, pad0, "_pad0"),
        UBOREG_M(PostSnapshotFrame_PerProgramBind, frame_rect),
        UBOREG_M(PostSnapshotFrame_PerProgramBind, border_color),
        UBOREG_M(PostSnapshotFrame_PerProgramBind, border_thickness),
    };

    constexpr MemberEntry kM_PostVignette[] = {
        UBOREG_M(PostVignette_PerProgramBind, screen_res),
        UBOREG_MN(PostVignette_PerProgramBind, pad0, "_pad0"),
        UBOREG_M(PostVignette_PerProgramBind, vignette),
        UBOREG_MN(PostVignette_PerProgramBind, pad1, "_pad1"),
    };

    constexpr MemberEntry kM_PostVisualizeBuffers[] = {
        UBOREG_M(PostVisualizeBuffers_PerProgramBind, mipLevel),
    };

    constexpr MemberEntry kM_Preview[] = {
        UBOREG_M(Preview_PerProgramBind, light_position),
        UBOREG_M(Preview_PerProgramBind, light_diffuse),
    };

    constexpr MemberEntry kM_RadianceGen[] = {
        UBOREG_M(RadianceGen_PerProgramBind, sourceIdx),
        UBOREG_M(RadianceGen_PerProgramBind, mipLevel),
        UBOREG_M(RadianceGen_PerProgramBind, u_width),
        UBOREG_M(RadianceGen_PerProgramBind, max_probe_lod),
        UBOREG_M(RadianceGen_PerProgramBind, probe_strength),
        UBOREG_MN(RadianceGen_PerProgramBind, pad0, "_pad0"),
        UBOREG_MN(RadianceGen_PerProgramBind, pad1, "_pad1"),
        UBOREG_MN(RadianceGen_PerProgramBind, pad2, "_pad2"),
    };

    constexpr MemberEntry kM_RlvF[] = {
        UBOREG_M(RlvF_PerProgramBind, rlvEffectParam1),
        UBOREG_M(RlvF_PerProgramBind, rlvEffectParam2),
        UBOREG_M(RlvF_PerProgramBind, rlvEffectParam4),
        UBOREG_M(RlvF_PerProgramBind, rlvEffectParam5),
        UBOREG_M(RlvF_PerProgramBind, _rlvF_screen_res),
        UBOREG_M(RlvF_PerProgramBind, rlvEffectParam3),
        UBOREG_M(RlvF_PerProgramBind, rlvEffectMode),
        UBOREG_M(RlvF_PerProgramBind, _rlvF_pad0),
    };

    constexpr MemberEntry kM_SkinSSSF[] = {
        UBOREG_M(SkinSSSF_PerProgramBind, aya_glow_color),
        UBOREG_M(SkinSSSF_PerProgramBind, aya_glow_gain),
        UBOREG_M(SkinSSSF_PerProgramBind, aya_blur_dir),
        UBOREG_M(SkinSSSF_PerProgramBind, aya_strength),
        UBOREG_M(SkinSSSF_PerProgramBind, aya_blur_radius),
        UBOREG_MN(SkinSSSF_PerProgramBind, aya_visual_realism_enabled, "_skinSSSF_aya_visual_realism_enabled"),
        UBOREG_M(SkinSSSF_PerProgramBind, aya_r20_skin_sss_enabled),
        UBOREG_MN(SkinSSSF_PerProgramBind, pad0, "_skinSSSF_pad0"),
        UBOREG_MN(SkinSSSF_PerProgramBind, pad1, "_skinSSSF_pad1"),
    };

    constexpr MemberEntry kM_SMAA[] = {
        UBOREG_M(SMAA_PerProgramBind, SMAA_RT_METRICS),
    };

    constexpr MemberEntry kM_SpotLightPerDraw[] = {
        UBOREG_M(SpotLightPerDraw, center),
        UBOREG_M(SpotLightPerDraw, size),
        UBOREG_M(SpotLightPerDraw, proj_origin),
        UBOREG_M(SpotLightPerDraw, falloff),
        UBOREG_M(SpotLightPerDraw, shadow_fade),
        UBOREG_M(SpotLightPerDraw, global_light_strength),
        UBOREG_M(SpotLightPerDraw, proj_shadow_idx),
        UBOREG_M(SpotLightPerDraw, classic_mode),
    };

    constexpr MemberEntry kM_SunDiscF[] = {
        UBOREG_M(SunDiscF_PerProgramBind, blend_factor),
        UBOREG_MN(SunDiscF_PerProgramBind, pad0, "_sunDiscF_pad0"),
        UBOREG_MN(SunDiscF_PerProgramBind, pad1, "_sunDiscF_pad1"),
        UBOREG_MN(SunDiscF_PerProgramBind, pad2, "_sunDiscF_pad2"),
    };

    constexpr MemberEntry kM_SunLightF[] = {
        UBOREG_MN(SunLightF_PerProgramBind, sun_dir, "sun_dir_sunlightf"),
        UBOREG_MN(SunLightF_PerProgramBind, pad0, "_sunLightF_pad0"),
    };

    constexpr MemberEntry kM_TerrainV[] = {
        UBOREG_M(TerrainV_PerProgramBind, object_plane_s),
        UBOREG_M(TerrainV_PerProgramBind, object_plane_t),
    };

    constexpr MemberEntry kM_TwoTextureCompare[] = {
        UBOREG_M(TwoTextureCompare_PerProgramBind, dither_scale),
        UBOREG_M(TwoTextureCompare_PerProgramBind, dither_scale_s),
        UBOREG_M(TwoTextureCompare_PerProgramBind, dither_scale_t),
        UBOREG_M(TwoTextureCompare_PerProgramBind, _pad0),
    };

    constexpr MemberEntry kM_UnderWaterF[] = {
        UBOREG_M(UnderWaterF_PerProgramBind, waterFogColorLinear),
        UBOREG_M(UnderWaterF_PerProgramBind, refScale),
    };

    constexpr MemberEntry kM_WaterF[] = {
        UBOREG_MN(WaterF_PerProgramBind, lightDir, "lightDir_waterf"),
        UBOREG_M(WaterF_PerProgramBind, blurMultiplier),
        UBOREG_M(WaterF_PerProgramBind, specular),
        UBOREG_M(WaterF_PerProgramBind, refScale),
        UBOREG_M(WaterF_PerProgramBind, normScale),
        UBOREG_M(WaterF_PerProgramBind, fresnelScale),
        UBOREG_M(WaterF_PerProgramBind, fresnelOffset),
        UBOREG_M(WaterF_PerProgramBind, blend_factor),
        UBOREG_MN(WaterF_PerProgramBind, classic_mode, "classic_mode_water"),
        UBOREG_M(WaterF_PerProgramBind, _pad_waterf0),
    };

    constexpr MemberEntry kM_WaterHazeF[] = {
        UBOREG_M(WaterHazeF_PerProgramBind, above_water),
        UBOREG_MN(WaterHazeF_PerProgramBind, _pad0, "_waterHazeF_pad0"),
        UBOREG_MN(WaterHazeF_PerProgramBind, _pad1, "_waterHazeF_pad1"),
        UBOREG_MN(WaterHazeF_PerProgramBind, _pad2, "_waterHazeF_pad2"),
    };

#define UBOREG_BLOCK(set, glsl_name, S, arr) \
    { set, glsl_name, (U32)sizeof(LLVKLoader::S), arr, (U32)(sizeof(arr) / sizeof(arr[0])) }

    constexpr BlockEntry kBlocks[] = {
        UBOREG_BLOCK(0, "PerFrameMatrixUBO",                    PerFrameMatrixUBO,                    kM_PerFrameMatrixUBO),
        UBOREG_BLOCK(0, "TextureMatrixUBO",                     TextureMatrixUBO,                     kM_TextureMatrixUBO),
        UBOREG_BLOCK(1, "ShadowParams_PerShaderBind",           ShadowParams_PerShaderBind,           kM_ShadowParams),
        UBOREG_BLOCK(1, "ShadowViewProjUBO",                    ShadowViewProj_PerPass,               kM_ShadowViewProj),
        UBOREG_BLOCK(0, "PreviewAmbient_PerShaderBind",         PreviewAmbient_PerShaderBind,         kM_PreviewAmbient),
        UBOREG_BLOCK(1, "PbrTerrain_PerShaderBind",             PbrTerrain_PerShaderBind,             kM_PbrTerrain),
        UBOREG_BLOCK(0, "StarTime_PerShaderBind",               StarTime_PerShaderBind,               kM_StarTime),
        UBOREG_BLOCK(0, "AvatarVelocityPalette_PerShaderBind",  AvatarVelocityPalette_PerShaderBind,  kM_AvatarVelocityPalette),
        UBOREG_BLOCK(0, "GlowCombine_PerShaderBind",            GlowCombine_PerShaderBind,            kM_GlowCombine),
        UBOREG_BLOCK(1, "SMAABlendWeightsF_PerProgramBind",     SMAABlendWeightsF_PerProgramBind,     kM_SMAABlendWeightsF),
        UBOREG_BLOCK(1, "WindlightAtmos_PerProgramBind",        WindlightAtmos_PerProgramBind,        kM_WindlightAtmos),
        UBOREG_BLOCK(1, "WindlightSky_PerProgramBind",          WindlightSky_PerProgramBind,          kM_WindlightSky),
        UBOREG_BLOCK(1, "WindlightHDR_PerProgramBind",          WindlightHDR_PerProgramBind,          kM_WindlightHDR),
        UBOREG_BLOCK(1, "WindlightLight_PerProgramBind",        WindlightLight_PerProgramBind,        kM_WindlightLight),
        UBOREG_BLOCK(1, "WaterFog_PerProgramBind",              WaterFog_PerProgramBind,              kM_WaterFog),
        UBOREG_BLOCK(1, "Water_PerProgramBind",                 Water_PerProgramBind,                 kM_Water),
        UBOREG_BLOCK(1, "ReflectionProbe_PerProgramBind",       ReflectionProbe_PerProgramBind,       kM_ReflectionProbe),
        UBOREG_BLOCK(1, "GlobalF_PerProgramBind",               GlobalF_PerProgramBind,               kM_GlobalF),
        UBOREG_BLOCK(1, "AoUtil_PerProgramBind",                AoUtil_PerProgramBind,                kM_AoUtil),
        UBOREG_BLOCK(1, "TonemapUtilF_PerProgramBind",          TonemapUtilF_PerProgramBind,          kM_TonemapUtilF),
        UBOREG_BLOCK(1, "PbrTerrainF_PerProgramBind",           PbrTerrainF_PerProgramBind,           kM_PbrTerrainF),
        UBOREG_BLOCK(1, "DeferredUtil_PerProgramBind",          DeferredUtil_PerProgramBind,          kM_DeferredUtil),
        UBOREG_BLOCK(1, "ShadowUtil_PerProgramBind",            ShadowUtil_PerProgramBind,            kM_ShadowUtil),
        UBOREG_BLOCK(1, "ReflectionProbes",                     ReflectionProbes_PerProgramBind,      kM_ReflectionProbes),
        UBOREG_BLOCK(1, "AvatarSkin_PerProgramBind",            AvatarSkin_PerProgramBind,            kM_AvatarSkin),
        UBOREG_BLOCK(1, "ObjectSkin_PerProgramBind",            ObjectSkin_PerProgramBind,            kM_ObjectSkin),
        UBOREG_BLOCK(1, "Lights_PerProgramBind",                Lights_PerProgramBind,                kM_Lights),
        UBOREG_BLOCK(1, "LightsSpecular_PerProgramBind",        LightsSpecular_PerProgramBind,        kM_LightsSpecular),
        UBOREG_BLOCK(1, "PBRMaterial_PerMaterial",              PBRMaterial_PerMaterial,              kM_PBRMaterial),
        UBOREG_BLOCK(1, "DrawColor_PerShaderBind",              DrawColor_PerShaderBind,              kM_DrawColor),
        UBOREG_BLOCK(1, "VolumetricLightF_PerProgramBind",      VolumetricLightF_PerProgramBind,      kM_VolumetricLightF),
        UBOREG_BLOCK(1, "MotionBlurF_PerProgramBind",           MotionBlurF_PerProgramBind,           kM_MotionBlurF),
        UBOREG_BLOCK(1, "PostGammaCorrect_PerProgramBind",      PostGammaCorrect_PerProgramBind,      kM_PostGammaCorrect),
        UBOREG_BLOCK(1, "CofF_PerProgramBind",                  CofF_PerProgramBind,                  kM_CofF),
        UBOREG_BLOCK(1, "DofCombineF_PerProgramBind",           DofCombineF_PerProgramBind,           kM_DofCombineF),
        UBOREG_BLOCK(1, "FsObjectIDF_PerProgramBind",           FsObjectIDF_PerProgramBind,           kM_FsObjectIDF),
        UBOREG_BLOCK(1, "FxaaShared_PerProgramBind",            FxaaShared_PerProgramBind,            kM_FxaaShared),
        UBOREG_BLOCK(1, "AvatarVCloth_PerProgramBind",          AvatarVCloth_PerProgramBind,          kM_AvatarVCloth),
        UBOREG_BLOCK(1, "BlurLightF_PerProgramBind",            BlurLightF_PerProgramBind,            kM_BlurLightF),
        UBOREG_BLOCK(1, "Cloud_PerProgramBind",                 Cloud_PerProgramBind,                 kM_Cloud),
        UBOREG_BLOCK(1, "ExposureF_PerProgramBind",             ExposureF_PerProgramBind,             kM_ExposureF),
        UBOREG_BLOCK(1, "Glow_PerProgramBind",                  Glow_PerProgramBind,                  kM_Glow),
        UBOREG_BLOCK(1, "GodraysF_PerProgramBind",              GodraysF_PerProgramBind,              kM_GodraysF),
        UBOREG_BLOCK(1, "HazeF_PerProgramBind",                 HazeF_PerProgramBind,                 kM_HazeF),
        UBOREG_BLOCK(1, "IrradianceGen_PerProgramBind",         IrradianceGen_PerProgramBind,         kM_IrradianceGen),
        UBOREG_BLOCK(1, "LuminanceF_PerProgramBind",            LuminanceF_PerProgramBind,            kM_LuminanceF),
        UBOREG_BLOCK(1, "MoonF_PerProgramBind",                 MoonF_PerProgramBind,                 kM_MoonF),
        UBOREG_BLOCK(1, "NormalDebug_PerProgramBind",           NormalDebug_PerProgramBind,           kM_NormalDebug),
        UBOREG_BLOCK(1, "NormgenF_PerProgramBind",              NormgenF_PerProgramBind,              kM_NormgenF),
        UBOREG_BLOCK(1, "OneTextureFilter_PerProgramBind",      OneTextureFilter_PerProgramBind,      kM_OneTextureFilter),
        UBOREG_BLOCK(1, "Pathfinding_PerProgramBind",           Pathfinding_PerProgramBind,           kM_Pathfinding),
        UBOREG_BLOCK(1, "PointLightPerDraw",                    PointLightPerDraw,                    kM_PointLightPerDraw),
        UBOREG_BLOCK(1, "PostF_PerProgramBind",                 PostF_PerProgramBind,                 kM_PostF),
        UBOREG_BLOCK(1, "PostHQDoFF_PerProgramBind",            PostF_PerProgramBind,                 kM_PostHQDoFF),
        UBOREG_BLOCK(1, "PostNoDoFF_PerProgramBind",            PostNoDoFF_PerProgramBind,            kM_PostNoDoFF),
        UBOREG_BLOCK(1, "PostSnapshotFrame_PerProgramBind",     PostSnapshotFrame_PerProgramBind,     kM_PostSnapshotFrame),
        UBOREG_BLOCK(1, "PostVignette_PerProgramBind",          PostVignette_PerProgramBind,          kM_PostVignette),
        UBOREG_BLOCK(1, "PostVisualizeBuffers_PerProgramBind",  PostVisualizeBuffers_PerProgramBind,  kM_PostVisualizeBuffers),
        UBOREG_BLOCK(1, "Preview_PerProgramBind",               Preview_PerProgramBind,               kM_Preview),
        UBOREG_BLOCK(1, "RadianceGen_PerProgramBind",           RadianceGen_PerProgramBind,           kM_RadianceGen),
        UBOREG_BLOCK(1, "RlvF_PerProgramBind",                  RlvF_PerProgramBind,                  kM_RlvF),
        UBOREG_BLOCK(1, "SkinSSSF_PerProgramBind",              SkinSSSF_PerProgramBind,              kM_SkinSSSF),
        UBOREG_BLOCK(1, "SMAA_PerProgramBind",                  SMAA_PerProgramBind,                  kM_SMAA),
        UBOREG_BLOCK(1, "SpotLightPerDraw",                     SpotLightPerDraw,                     kM_SpotLightPerDraw),
        UBOREG_BLOCK(1, "SunDiscF_PerProgramBind",              SunDiscF_PerProgramBind,              kM_SunDiscF),
        UBOREG_BLOCK(1, "SunLightF_PerProgramBind",             SunLightF_PerProgramBind,             kM_SunLightF),
        UBOREG_BLOCK(1, "TerrainV_PerProgramBind",              TerrainV_PerProgramBind,              kM_TerrainV),
        UBOREG_BLOCK(1, "TwoTextureCompare_PerProgramBind",     TwoTextureCompare_PerProgramBind,     kM_TwoTextureCompare),
        UBOREG_BLOCK(1, "UnderWaterF_PerProgramBind",           UnderWaterF_PerProgramBind,           kM_UnderWaterF),
        UBOREG_BLOCK(1, "WaterF_PerProgramBind",                WaterF_PerProgramBind,                kM_WaterF),
        UBOREG_BLOCK(1, "WaterHazeF_PerProgramBind",            WaterHazeF_PerProgramBind,            kM_WaterHazeF),
    };

#undef UBOREG_BLOCK

    const PcEntry kPcTable[] = {
        { "modelview_matrix",          LLVkUboReg::PC_OFF_MODELVIEW,            (U32)sizeof(F32) * 16 },
        { "point_size",                LLVkUboReg::PC_OFF_POINT_SIZE,           (U32)sizeof(F32) },
        { "last_object_matrix",        LLVkUboReg::PC_OFF_LAST_OBJECT_MATRIX,   (U32)sizeof(F32) * 16 },
        { "minimum_alpha",             LLVkUboReg::PC_OFF_MINIMUM_ALPHA,        (U32)sizeof(F32) },
        { "aya_sss_skin_flag",         LLVkUboReg::PC_OFF_SSS_SKIN_FLAG,        (U32)sizeof(F32) },
        { "object_alpha",              LLVkUboReg::PC_OFF_OBJECT_ALPHA,         (U32)sizeof(F32) },
        { "waterSign",                 LLVkUboReg::PC_OFF_WATER_SIGN,           (U32)sizeof(F32) },
        { "aya_preview_neutral_atmos", LLVkUboReg::PC_OFF_PREVIEW_NEUTRAL_ATMOS, (U32)sizeof(F32) },
        { "emissiveColor",             LLVkUboReg::PC_OFF_EMISSIVE_COLOR,       (U32)sizeof(F32) * 3 },
        { "metallicFactor",            LLVkUboReg::PC_OFF_METALLIC,             (U32)sizeof(F32) },
        { "roughnessFactor",           LLVkUboReg::PC_OFF_ROUGHNESS,            (U32)sizeof(F32) },
        { "resScale",                  LLVkUboReg::PC_OFF_GAUSSIAN_RES_SCALE,   (U32)sizeof(F32) },
        { "direction",                 LLVkUboReg::PC_OFF_GAUSSIAN_DIRECTION,   (U32)sizeof(F32) * 2 },
        { "box_center",                LLVkUboReg::PC_OFF_BOX_CENTER,           (U32)sizeof(F32) * 3 },
        { "box_size",                  LLVkUboReg::PC_OFF_BOX_SIZE,             (U32)sizeof(F32) * 3 },
    };

    const LedgerEntry kLedger[] = {
        { "SMAA T2x Resolve (Low)",                   "SMAA_PerProgramBind", "V5", 0, 16 },
        { "SMAA T2x Resolve (Medium)",                "SMAA_PerProgramBind", "V5", 0, 16 },
        { "SMAA T2x Resolve (High)",                  "SMAA_PerProgramBind", "V5", 0, 16 },
        { "SMAA T2x Resolve (Ultra)",                 "SMAA_PerProgramBind", "V5", 0, 16 },
        { nullptr, "Asset_GLTFMaterials",                "V4", 0, 0 },
        { nullptr, "Asset_GLTFNodes",                    "V4", 0, 0 },
        { nullptr, "Skin_GLTFJoints",                    "V4", 0, 0 },
    };

    const LedgerEntry* ledgerMatch(const std::string& shader_name, const std::string& block_name, const char* kind,
                                   U32 pipeline_size, U32 block_size)
    {
        for (const LedgerEntry& le : kLedger)
        {
            if (strcmp(le.kind, kind) != 0)
            {
                continue;
            }
            if (le.shader_name != nullptr && shader_name != le.shader_name)
            {
                continue;
            }
            if (le.block_name != nullptr && block_name != le.block_name)
            {
                continue;
            }
            if (le.expect_pipeline_size != 0 || le.expect_block_size != 0)
            {
                if (le.expect_pipeline_size != pipeline_size || le.expect_block_size != block_size)
                {
                    continue;
                }
            }
            return &le;
        }
        return nullptr;
    }

    bool logOnce(const std::string& key)
    {
        static std::set<std::string> seen;
        return seen.insert(key).second;
    }

    const BlockEntry* findBlockEntry(S32 set, const std::string& block_name)
    {
        for (const BlockEntry& be : kBlocks)
        {
            if (be.set == set && block_name == be.block_name)
            {
                return &be;
            }
        }
        return nullptr;
    }

    const PcEntry* findPcEntry(const std::string& name)
    {
        for (const PcEntry& pe : kPcTable)
        {
            if (name == pe.name)
            {
                return &pe;
            }
        }
        return nullptr;
    }

    std::map<std::string, LLVkUboReg::VariantVerifier>& variantVerifierMap()
    {
        static std::map<std::string, LLVkUboReg::VariantVerifier> s_map;
        return s_map;
    }

    void reportLedger(const LLGLSLShader& shader, const VkReflUboBlock& block, const char* kind, U32 pipeline_size)
    {
        std::string key = shader.mName + "|ledger|" + block.block_name + "|" + kind;
        if (logOnce(key))
        {
            LL_INFOS("UBOReg") << "UBORegLedger shader=" << shader.mName
                               << " set=" << block.set << " binding=" << block.binding
                               << " block=" << block.block_name << " kind=" << kind
                               << " pipeline_size=" << pipeline_size
                               << " block_size=" << block.block_size << LL_ENDL;
        }
    }

    void reportParseLimited(const LLGLSLShader& shader, const VkReflUboBlock& block, const std::string& member)
    {
        std::string key = shader.mName + "|parse|" + block.block_name;
        if (logOnce(key))
        {
            LL_WARNS("UBOReg") << "UBORegParse shader=" << shader.mName
                               << " set=" << block.set << " binding=" << block.binding
                               << " block=" << block.block_name << " member=" << member
                               << " size not derivable, offset-only verification" << LL_ENDL;
        }
    }

    std::map<std::string, const VkReflUboMember*> buildNormalizedMemberMap(const VkReflUboBlock& block)
    {
        std::map<std::string, const VkReflUboMember*> out;
        for (const VkReflUboMember& m : block.members)
        {
            out[LLVkUboReg::normalizeMemberName(block.block_name, m.name)] = &m;
        }
        return out;
    }

    void expectVariantMember(const LLGLSLShader& shader, const VkReflUboBlock& block,
                             const std::map<std::string, const VkReflUboMember*>& refl,
                             const char* name, U32 off, U32 sz)
    {
        auto it = refl.find(name);
        if (it == refl.end())
        {
            LLVkUboReg::reportDiff(shader, block, name, "V2", off, sz, 0, 0);
            return;
        }
        const VkReflUboMember* rm = it->second;
        if (rm->offset != off || (rm->size != 0 && sz != 0 && rm->size != sz))
        {
            LLVkUboReg::reportDiff(shader, block, name, "V1", off, sz, rm->offset, rm->size);
        }
    }

    bool hasPrefix(const std::string& s, const char* prefix)
    {
        const size_t n = strlen(prefix);
        return s.size() >= n && s.compare(0, n, prefix) == 0;
    }

    void verifyAlphaFVariant(const LLGLSLShader& shader, const VkReflUboBlock& block)
    {
        auto refl = buildNormalizedMemberMap(block);
        std::set<std::string> expected;
        auto expect = [&](const char* n, U32 off, U32 sz)
        {
            expected.insert(n);
            expectVariantMember(shader, block, refl, n, off, sz);
        };

        const U32 bs = block.block_size;
        if (bs == LLVKLoader::ALPHAF_UBO_SIZE_IMPOSTOR)
        {
            expect("minimum_alpha", 0, (U32)sizeof(F32));
            expect("near_clip", LLVKLoader::ALPHAF_UBO_OFFSET_NEAR_CLIP, (U32)sizeof(F32));
        }
        else if (bs == LLVKLoader::ALPHAF_UBO_SIZE_SHADOW || bs == LLVKLoader::ALPHAF_UBO_SIZE_NO_SHADOW)
        {
            expect("minimum_alpha", 0, (U32)sizeof(F32));
            expect("near_clip", LLVKLoader::ALPHAF_UBO_OFFSET_NEAR_CLIP, (U32)sizeof(F32));
            U32 lights_offset = LLVKLoader::ALPHAF_UBO_OFFSET_LIGHTS_SHADOW;
            if (bs == LLVKLoader::ALPHAF_UBO_SIZE_NO_SHADOW)
            {
                lights_offset = LLVKLoader::ALPHAF_UBO_OFFSET_LIGHTS_NO_SHADOW;
                expect("sun_dir_alphaf", LLVKLoader::ALPHAF_UBO_OFFSET_SUN_MOON, (U32)sizeof(F32) * 3);
                expect("moon_dir_alphaf", LLVKLoader::ALPHAF_UBO_OFFSET_SUN_MOON + 16, (U32)sizeof(F32) * 3);
            }
            expect("light_position", lights_offset, 128);
            lights_offset += 128;
            expect("light_direction", lights_offset, 128);
            lights_offset += 128;
            expect("light_attenuation", lights_offset, 128);
            lights_offset += 128;
            expect("light_diffuse", lights_offset, 128);
        }
        else
        {
            LLVkUboReg::reportDiff(shader, block, "(block)", "V1",
                                   LLVKLoader::ALPHAF_UBO_SIZE_IMPOSTOR, LLVKLoader::ALPHAF_UBO_SIZE_NO_SHADOW, 0, bs);
            return;
        }

        for (const auto& kv : refl)
        {
            if (expected.count(kv.first) == 0 && !hasPrefix(kv.first, "_alphaF_pad"))
            {
                LLVkUboReg::reportUnknown(shader, block, kv.first, "V3");
            }
        }
    }

    void verifyGltfMrVariant(const LLGLSLShader& shader, const VkReflUboBlock& block)
    {
        auto refl = buildNormalizedMemberMap(block);
        std::set<std::string> expected;
        auto expect = [&](const char* n, U32 off, U32 sz)
        {
            expected.insert(n);
            expectVariantMember(shader, block, refl, n, off, sz);
        };

        expected.insert("gltf_node_id");
        if (refl.count("gltf_node_id"))
        {
            expectVariantMember(shader, block, refl, "gltf_node_id", (U32)sizeof(S32), (U32)sizeof(S32));
        }

        const U32 bs = block.block_size;
        if (bs == LLVKLoader::GLTFMR_UBO_SIZE_HEADER)
        {
            expect("gltf_material_id", 0, (U32)sizeof(S32));
        }
        else if (bs == LLVKLoader::GLTFMR_UBO_SIZE_ALPHA_SUNSHADOW || bs == LLVKLoader::GLTFMR_UBO_SIZE_ALPHA_NOSHADOW)
        {
            expect("gltf_material_id", 0, (U32)sizeof(S32));
            U32 lights_offset = LLVKLoader::GLTFMR_UBO_OFFSET_LIGHTS_SUNSHADOW;
            if (bs == LLVKLoader::GLTFMR_UBO_SIZE_ALPHA_NOSHADOW)
            {
                lights_offset = LLVKLoader::GLTFMR_UBO_OFFSET_LIGHTS_NOSHADOW;
                expect("sun_dir", LLVKLoader::GLTFMR_UBO_OFFSET_SUN_DIR, (U32)sizeof(F32) * 3);
                expect("moon_dir", LLVKLoader::GLTFMR_UBO_OFFSET_MOON_DIR, (U32)sizeof(F32) * 3);
            }
            expect("light_position", lights_offset, 128);
            lights_offset += 128;
            expect("light_direction", lights_offset, 128);
            lights_offset += 128;
            expect("light_attenuation", lights_offset, 128);
            lights_offset += 128;
            expect("light_diffuse", lights_offset, 128);
            lights_offset += 128;
            expect("light_deferred_attenuation", lights_offset, 128);
        }
        else
        {
            LLVkUboReg::reportDiff(shader, block, "(block)", "V1",
                                   LLVKLoader::GLTFMR_UBO_SIZE_HEADER, LLVKLoader::GLTFMR_UBO_SIZE_ALPHA_NOSHADOW, 0, bs);
            return;
        }

        for (const auto& kv : refl)
        {
            if (expected.count(kv.first) == 0 && !hasPrefix(kv.first, "_pbrmr_"))
            {
                LLVkUboReg::reportUnknown(shader, block, kv.first, "V3");
            }
        }
    }

    constexpr MemberEntry kM_ReflectionProbeF_fixed[] = {
        UBOREG_M3(ReflectionProbeF_PerProgramBind, env_mat, "env_mat"),
        UBOREG_M(ReflectionProbeF_PerProgramBind, cube_snapshot),
        UBOREG_M(ReflectionProbeF_PerProgramBind, max_probe_lod),
        UBOREG_M(ReflectionProbeF_PerProgramBind, _reflectionProbeF_pad0),
        UBOREG_M(ReflectionProbeF_PerProgramBind, _reflectionProbeF_pad1),
    };

    void verifyReflectionProbeFVariant(const LLGLSLShader& shader, const VkReflUboBlock& block)
    {
        auto refl = buildNormalizedMemberMap(block);
        std::set<std::string> expected;
        for (const MemberEntry& te : kM_ReflectionProbeF_fixed)
        {
            expected.insert(te.name);
            expectVariantMember(shader, block, refl, te.name, te.offset, te.size);
        }

        const U32 clip_offset = (U32)offsetof(LLVKLoader::ReflectionProbeF_PerProgramBind, clipPlane);
        const U32 full_size   = (U32)sizeof(LLVKLoader::ReflectionProbeF_PerProgramBind);
        expected.insert("clipPlane");
        const bool has_clip = refl.count("clipPlane") != 0;
        if (has_clip)
        {
            expectVariantMember(shader, block, refl, "clipPlane", clip_offset,
                                (U32)sizeof(((LLVKLoader::ReflectionProbeF_PerProgramBind*)0)->clipPlane));
        }

        const U32 expect_size = has_clip ? full_size : clip_offset;
        if (block.block_size != 0 && block.block_size != expect_size)
        {
            LLVkUboReg::reportDiff(shader, block, "(block)", "V1", 0, expect_size, 0, block.block_size);
        }

        for (const auto& kv : refl)
        {
            if (expected.count(kv.first) == 0)
            {
                LLVkUboReg::reportUnknown(shader, block, kv.first, "V3");
            }
        }
    }

    void verifySSRUtilVariant(const LLGLSLShader& shader, const VkReflUboBlock& block)
    {
        auto refl = buildNormalizedMemberMap(block);
        std::set<std::string> expected;
        auto expect = [&](const char* n, U32 off, U32 sz)
        {
            expected.insert(n);
            expectVariantMember(shader, block, refl, n, off, sz);
        };
        typedef LLVKLoader::SSRUtil_PerProgramBind S;
        expect("_ssr_screen_res", (U32)offsetof(S, screen_res), (U32)sizeof(((S*)0)->screen_res));
        expect("iterationCount", (U32)offsetof(S, iterationCount), (U32)sizeof(F32));
        expect("rayStep", (U32)offsetof(S, rayStep), (U32)sizeof(F32));
        expect("modelview_delta", (U32)offsetof(S, modelview_delta), (U32)sizeof(((S*)0)->modelview_delta));
        expect("inv_modelview_delta", (U32)offsetof(S, inv_modelview_delta), (U32)sizeof(((S*)0)->inv_modelview_delta));
        expect("distanceBias", (U32)offsetof(S, distanceBias), (U32)sizeof(F32));
        expect("depthRejectBias", (U32)offsetof(S, depthRejectBias), (U32)sizeof(F32));
        expect("adaptiveStepMultiplier", (U32)offsetof(S, adaptiveStepMultiplier), (U32)sizeof(F32));
        expect("glossySampleCount", (U32)offsetof(S, glossySampleCount), (U32)sizeof(F32));
        const bool cinematic = refl.count("splitParamsStart") != 0;
        U32 expect_size;
        if (cinematic)
        {
            expect("splitParamsStart", (U32)offsetof(S, splitParamsStart), (U32)sizeof(((S*)0)->splitParamsStart));
            expect("noiseSine", (U32)offsetof(S, noiseSine), (U32)sizeof(F32));
            expect("splitParamsEnd", (U32)offsetof(S, splitParamsEnd), (U32)sizeof(((S*)0)->splitParamsEnd));
            expect("maxZDepth", (U32)offsetof(S, maxZDepth), (U32)sizeof(F32));
            expect("maxRoughness", (U32)offsetof(S, maxRoughness), (U32)sizeof(F32));
            expect("_ssrUtil_pad0", (U32)offsetof(S, _ssrUtil_pad0), (U32)sizeof(F32));
            expect("_ssrUtil_pad1", (U32)offsetof(S, _ssrUtil_pad1), (U32)sizeof(F32));
            expect("_ssrUtil_pad2", (U32)offsetof(S, _ssrUtil_pad2), (U32)sizeof(F32));
            expect_size = (U32)sizeof(S);
        }
        else
        {
            expect("_ssrUtil_pad_split", (U32)offsetof(S, splitParamsStart), (U32)sizeof(((S*)0)->splitParamsStart));
            expect("noiseSine", (U32)offsetof(S, noiseSine), (U32)sizeof(F32));
            expect_size = (U32)offsetof(S, splitParamsEnd);
        }
        if (block.block_size != 0 && block.block_size != expect_size)
        {
            LLVkUboReg::reportDiff(shader, block, "(block)", "V1", 0, expect_size, 0, block.block_size);
        }
        for (const auto& kv : refl)
        {
            if (expected.count(kv.first) == 0)
            {
                LLVkUboReg::reportUnknown(shader, block, kv.first, "V3");
            }
        }
    }

    void verifyGlowExtractVariant(const LLGLSLShader& shader, const VkReflUboBlock& block)
    {
        auto refl = buildNormalizedMemberMap(block);
        std::set<std::string> expected;
        auto expect = [&](const char* n, U32 off, U32 sz)
        {
            expected.insert(n);
            expectVariantMember(shader, block, refl, n, off, sz);
        };
        typedef LLVKLoader::GlowExtract_PerProgramBind S;
        expect("lumWeights", (U32)offsetof(S, lumWeights), (U32)sizeof(((S*)0)->lumWeights));
        expect("minLuminance", (U32)offsetof(S, minLuminance), (U32)sizeof(F32));
        expect("warmthWeights", (U32)offsetof(S, warmthWeights), (U32)sizeof(((S*)0)->warmthWeights));
        expect("maxExtractAlpha", (U32)offsetof(S, maxExtractAlpha), (U32)sizeof(F32));
        expect("warmthAmount", (U32)offsetof(S, warmthAmount), (U32)sizeof(F32));
        expect("_pad0", (U32)offsetof(S, _pad0), (U32)sizeof(F32));
        expected.insert("screen_res");
        const bool has_noise = refl.count("screen_res") != 0;
        if (has_noise)
        {
            expectVariantMember(shader, block, refl, "screen_res",
                                (U32)offsetof(S, screen_res), (U32)sizeof(((S*)0)->screen_res));
        }
        const U32 expect_size = has_noise ? (U32)sizeof(S) : (U32)offsetof(S, screen_res);
        if (block.block_size != 0 && block.block_size != expect_size)
        {
            LLVkUboReg::reportDiff(shader, block, "(block)", "V1", 0, expect_size, 0, block.block_size);
        }
        for (const auto& kv : refl)
        {
            if (expected.count(kv.first) == 0)
            {
                LLVkUboReg::reportUnknown(shader, block, kv.first, "V3");
            }
        }
    }

    void verifyPostTonemapVariant(const LLGLSLShader& shader, const VkReflUboBlock& block)
    {
        auto refl = buildNormalizedMemberMap(block);
        std::set<std::string> expected;
        auto expect = [&](const char* n, U32 off, U32 sz)
        {
            expected.insert(n);
            expectVariantMember(shader, block, refl, n, off, sz);
        };
        typedef LLVKLoader::PostTonemap_PerProgramBind S;
        expect("color_saturation", (U32)offsetof(S, color_saturation), (U32)sizeof(F32));
        expect("color_contrast", (U32)offsetof(S, color_contrast), (U32)sizeof(F32));
        expect("color_temperature", (U32)offsetof(S, color_temperature), (U32)sizeof(F32));
        expect("color_brightness", (U32)offsetof(S, color_brightness), (U32)sizeof(F32));
        expect("color_grading_lut_intensity", (U32)offsetof(S, color_grading_lut_intensity), (U32)sizeof(F32));
        expect("color_grading_lut_enabled", (U32)offsetof(S, color_grading_lut_enabled), (U32)sizeof(S32));
        expected.insert("gamma");
        const bool has_gamma = refl.count("gamma") != 0;
        if (has_gamma)
        {
            expectVariantMember(shader, block, refl, "gamma", (U32)offsetof(S, gamma), (U32)sizeof(F32));
        }
        const U32 expect_size = has_gamma ? (U32)offsetof(S, _postTonemap_pad1) : (U32)offsetof(S, gamma);
        if (block.block_size != 0 && block.block_size != expect_size)
        {
            LLVkUboReg::reportDiff(shader, block, "(block)", "V1", 0, expect_size, 0, block.block_size);
        }
        for (const auto& kv : refl)
        {
            if (expected.count(kv.first) == 0)
            {
                LLVkUboReg::reportUnknown(shader, block, kv.first, "V3");
            }
        }
    }

    void verifyCasFVariant(const LLGLSLShader& shader, const VkReflUboBlock& block)
    {
        auto refl = buildNormalizedMemberMap(block);
        std::set<std::string> expected;
        auto expect = [&](const char* n, U32 off, U32 sz)
        {
            expected.insert(n);
            expectVariantMember(shader, block, refl, n, off, sz);
        };
        typedef LLVKLoader::CasF_PerProgramBind S;
        expect("out_screen_res", (U32)offsetof(S, out_screen_res_uniform), (U32)sizeof(((S*)0)->out_screen_res_uniform));
        expect("_pad0", (U32)offsetof(S, _pad0), (U32)sizeof(((S*)0)->_pad0));
        expect("cas_param_0", (U32)offsetof(S, cas_param_0_uniform), (U32)sizeof(((S*)0)->cas_param_0_uniform));
        expect("cas_param_1", (U32)offsetof(S, cas_param_1_uniform), (U32)sizeof(((S*)0)->cas_param_1_uniform));
        expected.insert("gamma");
        const bool has_gamma = refl.count("gamma") != 0;
        if (has_gamma)
        {
            expectVariantMember(shader, block, refl, "gamma", (U32)offsetof(S, gamma), (U32)sizeof(F32));
        }
        const U32 expect_size = has_gamma ? (U32)sizeof(S) : (U32)offsetof(S, gamma);
        if (block.block_size != 0 && block.block_size != expect_size)
        {
            LLVkUboReg::reportDiff(shader, block, "(block)", "V1", 0, expect_size, 0, block.block_size);
        }
        for (const auto& kv : refl)
        {
            if (expected.count(kv.first) == 0 && !hasPrefix(kv.first, "_pad"))
            {
                LLVkUboReg::reportUnknown(shader, block, kv.first, "V3");
            }
        }
    }

    void verifySkyVariant(const LLGLSLShader& shader, const VkReflUboBlock& block)
    {
        auto refl = buildNormalizedMemberMap(block);
        std::set<std::string> expected;
        auto expect = [&](const char* n, U32 off, U32 sz)
        {
            expected.insert(n);
            expectVariantMember(shader, block, refl, n, off, sz);
        };
        expect("camPosLocal", 0, (U32)sizeof(F32) * 3);
        expect("cube_snapshot", 12, (U32)sizeof(S32));
        expect("moisture_level", 16, (U32)sizeof(F32));
        expect("droplet_radius", 20, (U32)sizeof(F32));
        expect("ice_level", 24, (U32)sizeof(F32));
        expected.insert("_sky_sky_hdr_scale");
        expected.insert("hdri_split_screen");
        expected.insert("_sky_env_mat");
        const bool has_hdri = refl.count("_sky_sky_hdr_scale") != 0;
        if (has_hdri)
        {
            expectVariantMember(shader, block, refl, "_sky_sky_hdr_scale", 32, (U32)sizeof(F32));
            expectVariantMember(shader, block, refl, "hdri_split_screen", 36, (U32)sizeof(F32));
            expectVariantMember(shader, block, refl, "_sky_env_mat", 48, 48);
        }
        const U32 expect_size = has_hdri ? 96u : 32u;
        if (block.block_size != 0 && block.block_size != expect_size)
        {
            LLVkUboReg::reportDiff(shader, block, "(block)", "V1", 0, expect_size, 0, block.block_size);
        }
        for (const auto& kv : refl)
        {
            if (expected.count(kv.first) == 0 && !hasPrefix(kv.first, "_sky_pad"))
            {
                LLVkUboReg::reportUnknown(shader, block, kv.first, "V3");
            }
        }
    }

    void verifySoftenLightFVariant(const LLGLSLShader& shader, const VkReflUboBlock& block)
    {
        auto refl = buildNormalizedMemberMap(block);
        std::set<std::string> expected;
        auto expect = [&](const char* n, U32 off, U32 sz)
        {
            expected.insert(n);
            expectVariantMember(shader, block, refl, n, off, sz);
        };
        expect("aya_translucency_params", 0, (U32)sizeof(F32) * 4);
        expect("aya_translucency_tint", 16, (U32)sizeof(F32) * 3);
        expect("sky_hdr_scale_soften", 28, (U32)sizeof(F32));
        U32 cursor = 32;
        expected.insert("sun_dir");
        expected.insert("moon_dir");
        if (refl.count("sun_dir"))
        {
            expectVariantMember(shader, block, refl, "sun_dir", cursor, (U32)sizeof(F32) * 3);
            expectVariantMember(shader, block, refl, "moon_dir", cursor + 16, (U32)sizeof(F32) * 3);
            cursor += 32;
        }
        expected.insert("ssao_effect_mat");
        expected.insert("ssao_irradiance_scale");
        expected.insert("ssao_irradiance_max");
        if (refl.count("ssao_effect_mat"))
        {
            expectVariantMember(shader, block, refl, "ssao_effect_mat", cursor, 48);
            expectVariantMember(shader, block, refl, "ssao_irradiance_scale", cursor + 48, (U32)sizeof(F32));
            expectVariantMember(shader, block, refl, "ssao_irradiance_max", cursor + 52, (U32)sizeof(F32));
            cursor += 64;
        }
        if (block.block_size != 0 && block.block_size != cursor)
        {
            LLVkUboReg::reportDiff(shader, block, "(block)", "V1", 0, cursor, 0, block.block_size);
        }
        for (const auto& kv : refl)
        {
            if (expected.count(kv.first) == 0 && !hasPrefix(kv.first, "_softenLightF_pad"))
            {
                LLVkUboReg::reportUnknown(shader, block, kv.first, "V3");
            }
        }
    }

    void verifyMultiPointLightFVariant(const LLGLSLShader& shader, const VkReflUboBlock& block)
    {
        auto refl = buildNormalizedMemberMap(block);
        const U32 bs = block.block_size;
        if (bs == 0)
        {
            return;
        }
        if (bs < 48 || (bs - 16) % 32 != 0 || (bs - 16) / 32 > 16)
        {
            LLVkUboReg::reportDiff(shader, block, "(block)", "V1", 0, 0, 0, bs);
            return;
        }
        const U32 n = (bs - 16) / 32;
        std::set<std::string> expected;
        auto expect = [&](const char* nm, U32 off, U32 sz)
        {
            expected.insert(nm);
            expectVariantMember(shader, block, refl, nm, off, sz);
        };
        expect("light", 0, n * 16);
        expect("light_col", n * 16, n * 16);
        expect("far_z", n * 32, (U32)sizeof(F32));
        expect("global_light_strength", n * 32 + 4, (U32)sizeof(F32));
        for (const auto& kv : refl)
        {
            if (expected.count(kv.first) == 0 && !hasPrefix(kv.first, "_multiPointLightF_pad"))
            {
                LLVkUboReg::reportUnknown(shader, block, kv.first, "V3");
            }
        }
    }

    void verifyPBRAlphaFVariant(const LLGLSLShader& shader, const VkReflUboBlock& block)
    {
        auto refl = buildNormalizedMemberMap(block);
        std::set<std::string> expected;
        auto expect = [&](const char* n, U32 off, U32 sz)
        {
            expected.insert(n);
            expectVariantMember(shader, block, refl, n, off, sz);
        };
        const bool has_mask = refl.count("minimum_alpha") != 0;
        expected.insert("minimum_alpha");
        U32 expect_size = 0;
        if (refl.count("metallicFactor") == 0)
        {
            expect("emissiveColor", 0, (U32)sizeof(F32) * 3);
            if (has_mask)
            {
                expectVariantMember(shader, block, refl, "minimum_alpha", 16, (U32)sizeof(F32));
            }
            expect_size = has_mask ? 32u : 16u;
        }
        else
        {
            expect("metallicFactor", 0, (U32)sizeof(F32));
            expect("roughnessFactor", 4, (U32)sizeof(F32));
            expect("emissiveColor", 16, (U32)sizeof(F32) * 3);
            U32 lights_offset = 32;
            if (has_mask)
            {
                expectVariantMember(shader, block, refl, "minimum_alpha", lights_offset, (U32)sizeof(F32));
                lights_offset += 16;
            }
            expected.insert("sun_dir");
            expected.insert("moon_dir");
            if (refl.count("sun_dir"))
            {
                expectVariantMember(shader, block, refl, "sun_dir", lights_offset, (U32)sizeof(F32) * 3);
                expectVariantMember(shader, block, refl, "moon_dir", lights_offset + 16, (U32)sizeof(F32) * 3);
                lights_offset += 32;
            }
            expect("light_position", lights_offset, 128);
            lights_offset += 128;
            expect("light_direction", lights_offset, 128);
            lights_offset += 128;
            expect("light_attenuation", lights_offset, 128);
            lights_offset += 128;
            expect("light_diffuse", lights_offset, 128);
            lights_offset += 128;
            expect("light_deferred_attenuation", lights_offset, 128);
            lights_offset += 128;
            expect_size = lights_offset;
        }
        if (block.block_size != 0 && block.block_size != expect_size)
        {
            LLVkUboReg::reportDiff(shader, block, "(block)", "V1", 0, expect_size, 0, block.block_size);
        }
        for (const auto& kv : refl)
        {
            if (expected.count(kv.first) == 0 && !hasPrefix(kv.first, "_pbralphaF_"))
            {
                LLVkUboReg::reportUnknown(shader, block, kv.first, "V3");
            }
        }
    }

    void verifyBlockAgainstEntry(const LLGLSLShader& shader, const VkReflUboBlock& block, const BlockEntry& entry)
    {
        auto refl = buildNormalizedMemberMap(block);

        std::set<std::string> known;
        for (U32 i = 0; i < entry.member_count; ++i)
        {
            const MemberEntry& te = entry.members[i];
            known.insert(te.name);
            auto it = refl.find(te.name);
            if (it == refl.end())
            {
                LLVkUboReg::reportDiff(shader, block, te.name, "V2", te.offset, te.size, 0, 0);
                continue;
            }
            const VkReflUboMember* rm = it->second;
            if (rm->offset != te.offset || (rm->size != 0 && rm->size != te.size))
            {
                LLVkUboReg::reportDiff(shader, block, te.name, "V1", te.offset, te.size, rm->offset, rm->size);
            }
        }

        for (const auto& kv : refl)
        {
            if (known.count(kv.first) == 0)
            {
                LLVkUboReg::reportUnknown(shader, block, kv.first, "V3");
            }
        }

        if (block.block_size > 0 && block.block_size > entry.cpp_size)
        {
            LLVkUboReg::reportDiff(shader, block, "(block)", "V1", 0, entry.cpp_size, 0, block.block_size);
        }
    }

    bool ensureBuiltinVerifiers()
    {
        LLVkUboReg::registerVariantVerifier("AlphaF_PerProgramBind", verifyAlphaFVariant);
        LLVkUboReg::registerVariantVerifier("PBRMetallicRoughnessF_PerProgramBind", verifyGltfMrVariant);
        LLVkUboReg::registerVariantVerifier("ReflectionProbeF_PerProgramBind", verifyReflectionProbeFVariant);
        LLVkUboReg::registerVariantVerifier("GlowExtract_PerProgramBind", verifyGlowExtractVariant);
        LLVkUboReg::registerVariantVerifier("PostTonemap_PerProgramBind", verifyPostTonemapVariant);
        LLVkUboReg::registerVariantVerifier("CasF_PerProgramBind", verifyCasFVariant);
        LLVkUboReg::registerVariantVerifier("Sky_PerProgramBind", verifySkyVariant);
        LLVkUboReg::registerVariantVerifier("SoftenLightF_PerProgramBind", verifySoftenLightFVariant);
        LLVkUboReg::registerVariantVerifier("MultiPointLightF_PerProgramBind", verifyMultiPointLightFVariant);
        LLVkUboReg::registerVariantVerifier("PBRAlphaF_PerProgramBind", verifyPBRAlphaFVariant);
        LLVkUboReg::registerVariantVerifier("SSRUtil_PerProgramBind", verifySSRUtilVariant);
        return true;
    }
}

namespace LLVkUboReg
{

void registerVariantVerifier(const char* block_name, VariantVerifier fn)
{
    variantVerifierMap()[block_name] = fn;
}

std::string normalizeMemberName(const std::string& block_name, const std::string& member_name)
{
    if (!hasPrefix(member_name, "_dup_"))
    {
        return member_name;
    }
    std::string tags[2];
    tags[0] = block_name;
    const size_t per = block_name.find("_Per");
    tags[1] = (per != std::string::npos) ? block_name.substr(0, per) : block_name;
    for (const std::string& tag : tags)
    {
        const std::string full = "_dup_" + tag + "_";
        if (member_name.size() > full.size() && member_name.compare(0, full.size(), full) == 0)
        {
            return member_name.substr(full.size());
        }
    }
    return member_name;
}

void reportDiff(const LLGLSLShader& shader, const VkReflUboBlock& block, const std::string& member,
                const char* kind, U32 expect_off, U32 expect_size, U32 refl_off, U32 refl_size)
{
    std::string key = shader.mName + "|" + std::to_string(block.set) + "|" + std::to_string(block.binding)
                    + "|" + block.block_name + "|" + member + "|" + kind;
    if (logOnce(key))
    {
        LL_ERRS("UBOReg") << "UBORegDiff shader=" << shader.mName
                           << " set=" << block.set << " binding=" << block.binding
                           << " block=" << block.block_name << " member=" << member
                           << " kind=" << kind
                           << " expect=" << expect_off << "/" << expect_size
                           << " refl=" << refl_off << "/" << refl_size << LL_ENDL;
    }
}

void reportUnknown(const LLGLSLShader& shader, const VkReflUboBlock& block, const std::string& member,
                   const char* kind)
{
    std::string key = shader.mName + "|" + std::to_string(block.set) + "|" + std::to_string(block.binding)
                    + "|" + block.block_name + "|" + member + "|" + kind;
    if (logOnce(key))
    {
        LL_ERRS("UBOReg") << "UBORegUnknown shader=" << shader.mName
                           << " set=" << block.set << " binding=" << block.binding
                           << " block=" << block.block_name << " member=" << member
                           << " kind=" << kind << LL_ENDL;
    }
}

void verifyProgramLayout(LLGLSLShader& shader)
{
    static const bool s_builtin_registered = ensureBuiltinVerifiers();
    (void)s_builtin_registered;

    for (const VkReflUboBlock& block : shader.mVkReflUboBlocks)
    {
        for (const VkReflUboMember& m : block.members)
        {
            if (m.size == 0)
            {
                reportParseLimited(shader, block, m.name);
                break;
            }
        }

        auto vit = variantVerifierMap().find(block.block_name);
        if (vit != variantVerifierMap().end())
        {
            vit->second(shader, block);
            continue;
        }

        const BlockEntry* entry = findBlockEntry(block.set, block.block_name);
        if (entry != nullptr)
        {
            verifyBlockAgainstEntry(shader, block, *entry);
            continue;
        }

        if (ledgerMatch(shader.mName, block.block_name, "V4", 0, 0) != nullptr)
        {
            reportLedger(shader, block, "V4", 0);
            continue;
        }

        std::string key = shader.mName + "|" + block.block_name + "|V4";
        if (logOnce(key))
        {
            LL_ERRS("UBOReg") << "UBORegUnknown shader=" << shader.mName
                               << " set=" << block.set << " binding=" << block.binding
                               << " block=" << block.block_name << " kind=V4" << LL_ENDL;
        }
    }

    const size_t n = shader.mVkReflUboBlocks.size();
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = i + 1; j < n; ++j)
        {
            const VkReflUboBlock& a = shader.mVkReflUboBlocks[i];
            const VkReflUboBlock& b = shader.mVkReflUboBlocks[j];
            if (a.set == b.set && a.binding == b.binding)
            {
                reportDiff(shader, a, b.block_name, "V6", 0, a.block_size, 0, b.block_size);
            }
        }
    }

    for (const VkReflUboBlock& block : shader.mVkReflUboBlocks)
    {
        if (block.set != 1 || block.binding < 0 || block.block_size == 0)
        {
            continue;
        }
        const VkDeviceSize shared_size = shader.sharedUBOBindingSize((U32)block.binding);
        if (shared_size > 0 && (U32)shared_size < block.block_size)
        {
            reportDiff(shader, block, "(block)", "V7", 0, (U32)shared_size, 0, block.block_size);
        }
        else if (shared_size > 0 && (U32)shared_size > block.block_size)
        {
            reportLedger(shader, block, "V7", (U32)shared_size);
        }
    }

    for (const VkReflUboBlock& pc : shader.mVkReflPushConstants)
    {
        for (const VkReflUboMember& m : pc.members)
        {
            if (m.size == 0)
            {
                reportParseLimited(shader, pc, m.name);
            }
            const std::string name = normalizeMemberName(pc.block_name, m.name);
            const PcEntry* pe = findPcEntry(name);
            if (pe == nullptr)
            {
                reportUnknown(shader, pc, name, "V3");
                continue;
            }
            if (m.offset != pe->offset || (m.size != 0 && m.size != pe->size))
            {
                reportDiff(shader, pc, name, "V1", pe->offset, pe->size, m.offset, m.size);
            }
        }
    }
}

void verifyPerProgramSize(LLGLSLShader& shader, U32 per_program_ubo_size)
{
    const S32 binding = (S32)shader.mVkPerProgramUBOBinding;
    const VkReflUboBlock* largest = nullptr;
    for (const VkReflUboBlock& b : shader.mVkReflUboBlocks)
    {
        if (b.set != 1 || b.binding != binding)
        {
            continue;
        }
        if (b.block_size == 0)
        {
            return;
        }
        if (largest == nullptr || b.block_size > largest->block_size)
        {
            largest = &b;
        }
    }
    if (largest == nullptr)
    {
        if (per_program_ubo_size > 0)
        {
            VkReflUboBlock none{ 1, binding, 0, "(none)", 0, {} };
            reportLedger(shader, none, "V5", per_program_ubo_size);
        }
        return;
    }

    if (per_program_ubo_size == largest->block_size)
    {
        return;
    }

    if (per_program_ubo_size > largest->block_size)
    {
        reportLedger(shader, *largest, "V5", per_program_ubo_size);
        return;
    }

    if (ledgerMatch(shader.mName, largest->block_name, "V5", per_program_ubo_size, largest->block_size) != nullptr)
    {
        reportLedger(shader, *largest, "V5", per_program_ubo_size);
        return;
    }

    reportDiff(shader, *largest, "(block)", "V5", 0, per_program_ubo_size, 0, largest->block_size);
}

}
