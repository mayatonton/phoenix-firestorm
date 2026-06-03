// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// GaussianFParamUBO_Legacy blueprint (= set=3 binding=58, cadence=PerProgram)
// Source: literal extract from class1/interface/gaussianF.glsl:46 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=58 / cadence=PerProgram / 2 member

#version 450

layout(std140, set = 3, binding = 58) uniform GaussianFParamUBO_Legacy
{
    float resScale;
    vec2 direction;          // std140 vec2 alignment 8 で resScale との間に 4 byte padding 自動挿入
};

void main() {}
