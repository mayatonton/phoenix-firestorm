// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// GlobalFParamUBO_Legacy blueprint (= set=3 binding=11, cadence=PerProgram)
// Source: literal extract from class1/deferred/globalF.glsl:32 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=11 / cadence=PerProgram / 4 member

#version 450

layout(std140, set = 3, binding = 11) uniform GlobalFParamUBO_Legacy
{
    float mirror_flag;
    float clipSign;
    float _pad_globalf_0;
    float _pad_globalf_1;
};

void main() {}
