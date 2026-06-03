// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerDrawUBO_ObjectSkin blueprint (= set=2 binding=0, cadence=PerDraw)
// Source: literal extract from class1/avatar/objectSkinV.glsl:43 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=0 / cadence=PerDraw / 2 member (mat3x4 array[110] × 2)
// Note:   MAX_JOINTS_PER_MESH_OBJECT は viewer 側 #define で 110 固定
//         (= indra/llcharacter/lljoint.h:48 LL_MAX_JOINTS_PER_MESH_OBJECT、
//          llviewershadermgr.cpp:870 で全 shader に inject)、blueprint は literal int 110。
//         set=2 binding=0 を共有する 6 UBO の 1 つ (= per_draw_ubo_clip_plane.glsl header 参照)。

#version 450

layout(std140, set = 2, binding = 0) uniform PerDrawUBO_ObjectSkin
{
    mat3x4 matrixPalette[110];
    mat3x4 lastMatrixPalette[110];
};

void main() {}
