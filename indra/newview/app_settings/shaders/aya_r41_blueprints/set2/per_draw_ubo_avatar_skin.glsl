// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerDrawUBO_AvatarSkin blueprint (= set=2 binding=0, cadence=PerDraw)
// Source: literal extract from class1/avatar/avatarSkinV.glsl:45 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=0 / cadence=PerDraw / 1 member (vec4 array[45])
// Note:   set=2 binding=0 を共有する 6 UBO の 1 つ (= per_draw_ubo_clip_plane.glsl header 参照)。

#version 450

layout(std140, set = 2, binding = 0) uniform PerDrawUBO_AvatarSkin
{
    vec4 matrixPalette[45];
};

void main() {}
