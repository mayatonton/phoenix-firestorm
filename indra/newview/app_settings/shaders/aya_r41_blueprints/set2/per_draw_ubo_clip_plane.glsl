// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerDrawUBO_ClipPlane blueprint (= set=2 binding=0, cadence=PerDraw)
// Source: literal extract from class1/deferred/pbropaqueF.glsl:180 ifdef LL_VULKAN_GLSL block
//         (verified identical across 5 sample sites =
//          pbropaqueF / class1/gltf/pbrmetallicroughnessF / class3/deferred/softenLightF /
//          class3/deferred/reflectionProbeF / class1/deferred/globalF)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=0 / cadence=PerDraw / 1 member (vec4)
// Note:   set=2 binding=0 は 6 個の異なる UBO 名で共有 (= ClipPlane / LightParams /
//         AvatarSkin / ObjectSkin / SkinnedVelocity / AvatarVelocity)。AYA option (I)
//         採用 = 各 UBO 名を独立 blueprint として emit、Phase 1.B host wiring で
//         name-based dispatch (= set=1 MaterialUBO/MaterialUBO_Legacy 同位 finding と
//         同 precedent)。06a §3.3.1 hypothesis A (= per-program で binding=0 を再割当、
//         GL/Vulkan ともに許容) が事実として grep 確定。

#version 450

layout(std140, set = 2, binding = 0) uniform PerDrawUBO_ClipPlane
{
    vec4 clipPlane;
};

void main() {}
