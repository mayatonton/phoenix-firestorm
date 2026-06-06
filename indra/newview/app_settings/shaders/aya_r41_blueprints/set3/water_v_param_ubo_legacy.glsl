// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 (= 物理 dir = set3/)
// WaterVParamUBO_Legacy blueprint
// 旧 (Phase 1.A 起案時): set=3 binding=60
// 新 (Phase 2.L0 step 2-batch-0-a 同期書換 = 2026-06-06 phase E、commit e5f57d57ff actual class*/ + class3/ 対応): **set=1 binding=79 (slot 79)**
// Source: literal extract from class1/environment/waterV.glsl:61 ifdef LL_VULKAN_GLSL block
// Multi-site: verified identical at class3/environment/waterF.glsl:108
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         旧 set=3 帯 binding=60 → 新 set=1 帯 binding=79 / cadence=PerProgram / 6 member

#version 450

layout(std140, set = 1, binding = 79) uniform WaterVParamUBO_Legacy
{
    vec2 waveDir1;
    vec2 waveDir2;
    float time;
    vec3 eyeVec;
    float waterHeight;
    vec3 lightDir;
};

void main() {}
