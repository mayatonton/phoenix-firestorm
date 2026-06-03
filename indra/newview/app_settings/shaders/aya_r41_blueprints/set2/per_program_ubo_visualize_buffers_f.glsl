// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_VisualizeBuffersF blueprint (= set=2 binding=16, cadence=PerProgram)
// Source: literal extract from class1/deferred/postDeferredVisualizeBuffers.glsl:40 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=16 / cadence=PerProgram / 4 member (float × 4)

#version 450

layout(std140, set = 2, binding = 16) uniform PerProgramUBO_VisualizeBuffersF
{
    float mipLevel;
    float _pad_visbuf0;
    float _pad_visbuf1;
    float _pad_visbuf2;
};

void main() {}
