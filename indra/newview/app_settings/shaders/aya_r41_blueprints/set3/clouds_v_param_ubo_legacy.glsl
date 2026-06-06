// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 (= 物理 dir = set3/)
// CloudsVParamUBO_Legacy blueprint
// 旧 (Phase 1.A 起案時): set=3 binding=3
// 新 (Phase 2.L0 step 2-batch-0-a 同期書換 = 2026-06-06 phase E、commit 887ddb5341 actual class*/ 対応): **set=1 binding=8 (slot 8)**
// Source: literal extract from class1/deferred/cloudsV.glsl:106 ifdef LL_VULKAN_GLSL block
// Multi-site: verified identical at class1/deferred/cloudsF.glsl:79
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         旧 set=3 帯 binding=3 → 新 set=1 帯 binding=8 / cadence=PerProgram / 4 member

#version 450

layout(std140, set = 1, binding = 8) uniform CloudsVParamUBO_Legacy
{
    vec3  camPosLocal;
    float cloud_scale;
    vec3  cloud_color;
    float _pad_clouds_v_legacy_0;
};

void main() {}
