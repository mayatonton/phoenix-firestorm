// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// AtmoExtraUBO_Legacy blueprint (= set=3 binding=0, cadence=PerProgram)
// Source: literal extract from class1/windlight/atmosphericsFuncs.glsl:89 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=0 / cadence=PerProgram / 10 member

#version 450

layout(std140, set = 3, binding = 0) uniform AtmoExtraUBO_Legacy
{
    vec3  lightnorm;
    float haze_horizon;
    float cloud_shadow;
    float sun_moon_glow_factor;
    int   aya_visual_realism_enabled;
    int   aya_r14_volumetric_atmosphere_enabled;
    float aya_r14_strength;
    int   aya_r16_aerial_perspective_enabled;
    float aya_r16_strength;
    float _pad_atmo_extra_legacy_0;
};

void main() {}
