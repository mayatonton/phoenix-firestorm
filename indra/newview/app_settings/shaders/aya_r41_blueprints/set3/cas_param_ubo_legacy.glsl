// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// CASParamUBO_Legacy blueprint (= set=3 binding=12, cadence=PerProgram)
// Source: literal extract from class1/deferred/CASF.glsl:52 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=12 / cadence=PerProgram / 6 member

#version 450

layout(std140, set = 3, binding = 12) uniform CASParamUBO_Legacy
{
    vec2  out_screen_res;
    float _pad_cas_0;
    float _pad_cas_1;
    uvec4 cas_param_0;
    uvec4 cas_param_1;
};

void main() {}
