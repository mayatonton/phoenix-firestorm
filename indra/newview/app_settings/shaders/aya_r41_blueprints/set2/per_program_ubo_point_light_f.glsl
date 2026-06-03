// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_PointLightF blueprint (= set=2 binding=25, cadence=PerProgram)
// Source: literal extract from class3/deferred/pointLightF.glsl:53 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=25 / cadence=PerProgram / 5 member (vec4 + float × 4)
// Note:   sun_wash は dead uniform (host setter あり、shader 本体未参照)。

#version 450

layout(std140, set = 2, binding = 25) uniform PerProgramUBO_PointLightF
{
    vec4  viewport;
    float sun_wash;
    float falloff;
    float global_light_strength;
    float _pad0;
};

void main() {}
