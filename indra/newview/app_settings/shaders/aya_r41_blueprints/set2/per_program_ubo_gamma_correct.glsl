// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 (= 物理 dir = set2/)
// PerProgramUBO_GammaCorrect blueprint
// 旧 (Phase 1.A 起案時): set=2 binding=2
// 新 (Phase 2.L0 step 2-batch-0-a 同期書換 = 2026-06-06 phase E、commit e539b384ed actual class*/ 対応): **set=1 binding=39 (slot 39)**
// Source: literal extract from class1/deferred/postDeferredGammaCorrect.glsl:45 ifdef LL_VULKAN_GLSL block
//         (verified identical across 2 sample sites = postDeferredGammaCorrect / postDeferredTonemap)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         旧 set=2 帯 binding=2 → 新 set=1 帯 binding=39 / cadence=PerProgram / 4 member (float × 4)

#version 450

layout(std140, set = 1, binding = 39) uniform PerProgramUBO_GammaCorrect
{
    float gamma;
    float _pad_gc0;
    float _pad_gc1;
    float _pad_gc2;
};

void main() {}
