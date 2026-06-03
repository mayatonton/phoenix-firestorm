// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// GlowCombineFParamUBO_Legacy blueprint (= set=3 binding=49, cadence=PerProgram)
// Source: literal extract from class1/interface/glowcombineF.glsl:44 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=49 / cadence=PerProgram / 4 member

#version 450

layout(std140, set = 3, binding = 49) uniform GlowCombineFParamUBO_Legacy
{
    float greyscale_str;
    float sepia_str;
    float num_colors;
    float _pad_glowcombine_f_legacy_0;
};

void main() {}
