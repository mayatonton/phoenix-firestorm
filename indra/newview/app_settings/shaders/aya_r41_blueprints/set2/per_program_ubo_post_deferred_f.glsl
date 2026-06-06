// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 (= 物理 dir = set2/)
// PerProgramUBO_PostDeferredF blueprint
// 旧 (Phase 1.A 起案時): set=2 binding=20
// 新 (Phase 2.L0 step 2-batch-0-a 同期書換 = 2026-06-06 phase E、commit d4cabb8cfc actual class*/ 対応): **set=1 binding=45 (slot 45)**
// Source: literal extract from class1/deferred/postDeferredF.glsl:108 ifdef LL_VULKAN_GLSL block
//         (verified identical across 2 sample sites = postDeferredF / postDeferredHQDoFF)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         旧 set=2 帯 binding=20 → 新 set=1 帯 binding=45 / cadence=PerProgram / 2 member (float × 2), total 32 B

#version 450

layout(std140, set = 1, binding = 45) uniform PerProgramUBO_PostDeferredF
{
    float res_scale;
    float chroma_str;
};

void main() {}
