// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_PostDeferredF blueprint (= set=2 binding=20, cadence=PerProgram)
// Source: literal extract from class1/deferred/postDeferredF.glsl:108 ifdef LL_VULKAN_GLSL block
//         (verified identical across 2 sample sites = postDeferredF / postDeferredHQDoFF)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=20 / cadence=PerProgram / 2 member (float × 2), total 32 B

#version 450

layout(std140, set = 2, binding = 20) uniform PerProgramUBO_PostDeferredF
{
    float res_scale;
    float chroma_str;
};

void main() {}
