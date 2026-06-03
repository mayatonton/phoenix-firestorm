// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// SoftenLightParamUBO_Legacy blueprint (= set=3 binding=5, cadence=PerProgram)
// Source: literal extract from class3/deferred/softenLightF.glsl:57 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=5 / cadence=PerProgram / 9 member

#version 450

layout(std140, set = 3, binding = 5) uniform SoftenLightParamUBO_Legacy
{
    vec4  aya_translucency_params;
    vec3  aya_translucency_tint;
    float blur_size;
    float blur_fidelity;
    float ssao_irradiance_scale;
    float ssao_irradiance_max;
    float _pad_soften_light_legacy_0;
    mat3  ssao_effect_mat;
};

void main() {}
