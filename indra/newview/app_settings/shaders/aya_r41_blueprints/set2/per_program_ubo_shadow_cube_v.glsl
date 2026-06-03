// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_ShadowCubeV blueprint (= set=2 binding=14, cadence=PerProgram)
// Source: literal extract from class1/deferred/shadowCubeV.glsl:57 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=14 / cadence=PerProgram / 4 member (vec3 + float pad + vec3 + float pad)

#version 450

layout(std140, set = 2, binding = 14) uniform PerProgramUBO_ShadowCubeV
{
    vec3  box_center;
    float _pad_shadowcube0;
    vec3  box_size;
    float _pad_shadowcube1;
};

void main() {}
