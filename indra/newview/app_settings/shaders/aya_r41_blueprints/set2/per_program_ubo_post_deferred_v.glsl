// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_PostDeferredV blueprint (= set=2 binding=7, cadence=PerProgram)
// Source: literal extract from class1/deferred/postDeferredV.glsl:46 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=7 / cadence=PerProgram / 2 member (vec2 × 2)

#version 450

layout(std140, set = 2, binding = 7) uniform PerProgramUBO_PostDeferredV
{
    vec2 tc_scale;
    vec2 _pad_pdv0;
};

void main() {}
