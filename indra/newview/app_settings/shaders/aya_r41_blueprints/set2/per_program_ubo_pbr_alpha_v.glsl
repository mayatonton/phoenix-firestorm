// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_PbrAlphaV blueprint (= set=2 binding=11, cadence=PerProgram)
// Source: literal extract from class1/deferred/pbralphaV.glsl:98 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=11 / cadence=PerProgram / 2 member (vec4 array[2] × 2)

#version 450

layout(std140, set = 2, binding = 11) uniform PerProgramUBO_PbrAlphaV
{
    vec4 texture_normal_transform[2];
    vec4 texture_metallic_roughness_transform[2];
};

void main() {}
