// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// SkinSSSPrototypeFParamUBO_Legacy blueprint (= set=3 binding=30, cadence=PerProgram)
// Source: literal extract from class1/deferred/skinSSSF.glsl:80 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=30 / cadence=PerProgram / 8 member

#version 450

layout(std140, set = 3, binding = 30) uniform SkinSSSPrototypeFParamUBO_Legacy
{
    vec2  aya_blur_dir;
    float aya_strength;
    float aya_blur_radius;
    float aya_glow_gain;
    vec3  aya_glow_color;
    int   aya_visual_realism_enabled_skinsss_legacy;
    int   aya_r20_skin_sss_enabled;
};

void main() {}
