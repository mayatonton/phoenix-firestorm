// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_AlphaParams blueprint (= set=2 binding=3, cadence=PerProgram)
// Source: literal extract from class1/deferred/alphaV.glsl:145 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=3 / cadence=PerProgram / 4 member (float × 4)

#version 450

layout(std140, set = 2, binding = 3) uniform PerProgramUBO_AlphaParams
{
    float near_clip;
    float _pad_ap0;
    float _pad_ap1;
    float _pad_ap2;
};

void main() {}
