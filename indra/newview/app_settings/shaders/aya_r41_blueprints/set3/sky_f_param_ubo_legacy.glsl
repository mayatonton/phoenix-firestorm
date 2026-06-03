// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// SkyFParamUBO_Legacy blueprint (= set=3 binding=2, cadence=PerProgram)
// Source: literal extract from class1/deferred/skyF.glsl:117 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=2 / cadence=PerProgram / 4 member

#version 450

layout(std140, set = 3, binding = 2) uniform SkyFParamUBO_Legacy
{
    float hdri_split_screen;
    float moisture_level;
    float droplet_radius;
    float ice_level;
};

void main() {}
