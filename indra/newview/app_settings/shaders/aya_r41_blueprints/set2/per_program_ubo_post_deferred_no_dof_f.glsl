// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_PostDeferredNoDoFF blueprint (= set=2 binding=12, cadence=PerProgram)
// Source: literal extract from class1/deferred/postDeferredNoDoFF.glsl:81 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=12 / cadence=PerProgram / 4 member (float × 4)

#version 450

layout(std140, set = 2, binding = 12) uniform PerProgramUBO_PostDeferredNoDoFF
{
    float chroma_str;
    float _pad_nodof0;
    float _pad_nodof1;
    float _pad_nodof2;
};

void main() {}
