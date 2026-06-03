// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_SpotLightF blueprint (= set=2 binding=10, cadence=PerProgram)
// Source: literal extract from class3/deferred/spotLightF.glsl:74 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=10 / cadence=PerProgram / 11 member (3 chunk: 4 scalar + vec3+float + 4 scalar)
// Note:   η-27 1d/1e-A + η-28 Phase 2d-α で vec3 center 配置 (= PointLightV と分離)。

#version 450

layout(std140, set = 2, binding = 10) uniform PerProgramUBO_SpotLightF
{
    // chunk 0 (4 scalar)
    float proj_near;
    float proj_ambient_lod;
    float near_clip;
    float far_clip;
    // chunk 1 (vec3 + float)
    vec3  proj_origin;
    float sun_wash;
    // chunk 2 (4 scalar)
    int   proj_shadow_idx;
    float shadow_fade;
    float falloff;
    float global_light_strength;
};

void main() {}
