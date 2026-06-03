// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// WaterFogUBO_Legacy blueprint (= set=3 binding=9, cadence=PerProgram)
// Source: literal extract from class1/environment/waterFogF.glsl:48 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=9 / cadence=PerProgram / 5 member

#version 450

layout(std140, set = 3, binding = 9) uniform WaterFogUBO_Legacy
{
    vec4  waterFogColor;
    float waterFogDensity;
    float waterFogKS;
    float _pad_waterfog_0;
    float _pad_waterfog_1;
};

void main() {}
