// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_VolumetricLightF blueprint (= set=2 binding=18, cadence=PerProgram)
// Source: literal extract from class3/deferred/volumetricLightF.glsl:129 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=18 / cadence=PerProgram / 4 member (int + float × 3), total 64 B
// Note:   seconds60 は BD legacy dead uniform (host setter なし、shader 内未参照)。

#version 450

layout(std140, set = 2, binding = 18) uniform PerProgramUBO_VolumetricLightF
{
    int   godray_res;
    float godray_multiplier;
    float falloff_multiplier;
    float seconds60;
};

void main() {}
