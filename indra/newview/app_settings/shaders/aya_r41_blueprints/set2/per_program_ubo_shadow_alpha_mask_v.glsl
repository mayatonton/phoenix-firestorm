// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_ShadowAlphaMaskV blueprint (= set=2 binding=6, cadence=PerProgram)
// Source: literal extract from class1/deferred/shadowAlphaMaskV.glsl:85 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=6 / cadence=PerProgram / 4 member (float × 4)

#version 450

layout(std140, set = 2, binding = 6) uniform PerProgramUBO_ShadowAlphaMaskV
{
    float shadow_target_width;
    float _pad_sam0;
    float _pad_sam1;
    float _pad_sam2;
};

void main() {}
