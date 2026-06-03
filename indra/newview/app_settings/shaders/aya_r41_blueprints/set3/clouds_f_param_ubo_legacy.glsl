// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// CloudsFParamUBO_Legacy blueprint (= set=3 binding=4, cadence=PerProgram)
// Source: literal extract from class1/deferred/cloudsF.glsl:61 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=4 / cadence=PerProgram / 8 member

#version 450

layout(std140, set = 3, binding = 4) uniform CloudsFParamUBO_Legacy
{
    vec3  cloud_pos_density1;
    float blend_factor;
    vec3  cloud_pos_density2;
    float cloud_variance;
    int   aya_r18_cloud_volumetric_enabled;
    float aya_r18_strength;
    float _pad_clouds_f_legacy_0;
    float _pad_clouds_f_legacy_1;
};

void main() {}
