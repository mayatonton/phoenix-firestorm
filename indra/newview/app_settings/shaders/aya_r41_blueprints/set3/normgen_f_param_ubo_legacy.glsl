// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// NormgenFParamUBO_Legacy blueprint (= set=3 binding=33, cadence=PerProgram)
// Source: literal extract from class1/deferred/normgenF.glsl:53 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=33 / cadence=PerProgram / 4 member

#version 450

layout(std140, set = 3, binding = 33) uniform NormgenFParamUBO_Legacy
{
    float stepX;
    float stepY;
    float norm_scale;
    int   bump_code;
};

void main() {}
