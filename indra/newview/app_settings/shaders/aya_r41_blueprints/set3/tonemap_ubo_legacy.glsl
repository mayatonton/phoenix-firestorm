// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// TonemapUBO_Legacy blueprint (= set=3 binding=10, cadence=PerProgram)
// Source: literal extract from class1/deferred/tonemapUtilF.glsl:146 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=10 / cadence=PerProgram / 4 member

#version 450

layout(std140, set = 3, binding = 10) uniform TonemapUBO_Legacy
{
    float exposure;
    float tonemap_mix;
    int   tonemap_type;
    float _pad_tonemap_0;
};

void main() {}
