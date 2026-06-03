// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// ShadowUtilParamUBO_Legacy blueprint (= set=3 binding=7, cadence=PerProgram)
// Source: literal extract from class1/deferred/shadowUtil.glsl:80 ifdef LL_VULKAN_GLSL block
// Multi-site: verified identical at cinematic_bd/class1/deferred/shadowUtil.glsl:105
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=7 / cadence=PerProgram / 13 member

#version 450

layout(std140, set = 3, binding = 7) uniform ShadowUtilParamUBO_Legacy
{
    mat4  shadow_matrix[6];
    vec4  shadow_clip;
    vec2  shadow_res;
    vec2  proj_shadow_res;
    float shadow_bias;
    float shadow_offset;
    float shadow_softness;
    float spot_shadow_bias;
    float spot_shadow_offset;
    float _pad_shadow_util_legacy_0;
    float _pad_shadow_util_legacy_1;
    float _pad_shadow_util_legacy_2;
};

void main() {}
