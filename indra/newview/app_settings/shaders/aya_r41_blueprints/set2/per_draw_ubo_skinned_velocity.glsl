// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerDrawUBO_SkinnedVelocity blueprint (= set=2 binding=0, cadence=PerDraw)
// Source: literal extract from class1/deferred/skinnedVelocityV.glsl:81 ifdef LL_VULKAN_GLSL block
//         (verified identical across 2 sample sites = skinnedVelocityV / skinnedVelocityAlphaV)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=0 / cadence=PerDraw / 1 member (mat3x4 array[110])
// Note:   MAX_JOINTS_PER_MESH_OBJECT は viewer 側 addPermutation で 110 inject
//         (= llviewershadermgr.cpp:3353/3383 で getMaxJointCount()=LL_MAX_JOINTS_PER_MESH_OBJECT=110)、
//         blueprint は literal int 110。
//         set=2 binding=0 を共有する 6 UBO の 1 つ (= per_draw_ubo_clip_plane.glsl header 参照)。

#version 450

layout(std140, set = 2, binding = 0) uniform PerDrawUBO_SkinnedVelocity
{
    mat3x4 lastMatrixPalette_skinned_velocity[110];
};

void main() {}
