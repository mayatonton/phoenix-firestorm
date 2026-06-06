// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 (= 物理 dir = set2/)
// PerProgramUBO_WaterHazeV blueprint
// 旧 (Phase 1.A 起案時): set=2 binding=15
// 新 (Phase 2.L0 step 2-batch-0-a 同期書換 = 2026-06-06 phase E、commit 5d21f1af9c actual class*/ 対応): **set=1 binding=55 (slot 55)**
// Source: literal extract from class3/deferred/waterHazeV.glsl:86 ifdef LL_VULKAN_GLSL block
//         (verified identical across 2 sample sites = waterHazeV / waterHazeF [V+F shared、host 1 bind])
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         旧 set=2 帯 binding=15 → 新 set=1 帯 binding=55 / cadence=PerProgram / 4 member (int + 3 float pad)

#version 450

layout(std140, set = 1, binding = 55) uniform PerProgramUBO_WaterHazeV
{
    int   above_water;
    float _pad_waterhaze0;
    float _pad_waterhaze1;
    float _pad_waterhaze2;
};

void main() {}
