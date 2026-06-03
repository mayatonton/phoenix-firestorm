// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_GammaCorrect blueprint (= set=2 binding=2, cadence=PerProgram)
// Source: literal extract from class1/deferred/postDeferredGammaCorrect.glsl:45 ifdef LL_VULKAN_GLSL block
//         (verified identical across 2 sample sites = postDeferredGammaCorrect / postDeferredTonemap)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=2 / cadence=PerProgram / 4 member (float × 4)

#version 450

layout(std140, set = 2, binding = 2) uniform PerProgramUBO_GammaCorrect
{
    float gamma;
    float _pad_gc0;
    float _pad_gc1;
    float _pad_gc2;
};

void main() {}
