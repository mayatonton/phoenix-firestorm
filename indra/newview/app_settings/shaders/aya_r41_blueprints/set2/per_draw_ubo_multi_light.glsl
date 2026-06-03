// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerDrawUBO_MultiLight blueprint (= set=2 binding=1, cadence=PerDraw)
// Source: literal extract from class3/deferred/multiPointLightF.glsl:76 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=1 / cadence=PerDraw / 6 member (vec4[16] × 2 + 4 float)
// Note:   LIGHT_COUNT は viewer 側 addPermutation で 1..16 inject
//         (= llviewershadermgr.cpp:1756 で gDeferredMultiLightProgram[i] permutation 16 件)。
//         blueprint は最大 variant (= LL_DEFERRED_MULTI_LIGHT_COUNT=16) を canonical 採用
//         = host C++ side alloc は full 16 entry、smaller LIGHT_COUNT shader 派生は
//         trailing entry 未参照 view (= Vulkan std140 layout-compat 慣用、shader 改修ゼロ、
//         set=1 MaterialUBO option I precedent)。

#version 450

layout(std140, set = 2, binding = 1) uniform PerDrawUBO_MultiLight
{
    vec4  light[16];
    vec4  light_col[16];
    float far_z;
    float global_light_strength;
    float _pad_ml0;
    float _pad_ml1;
};

void main() {}
