// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_PointLightV blueprint (= set=2 binding=5, cadence=PerProgram)
// Source: literal extract from class3/deferred/pointLightV.glsl:63 ifdef LL_VULKAN_GLSL block
//         (verified identical across 2 sample sites = pointLightV / spotLightF [cross-stage V+F shared])
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=5 / cadence=PerProgram / 2 member (vec3 + float)
// Note:   spotLightF.glsl:151 で declared-but-unused (= host bind は同 binding 共有、
//         frag は別 UBO 経由で size 参照、η-28-C type 3 範式)。

#version 450

layout(std140, set = 2, binding = 5) uniform PerProgramUBO_PointLightV
{
    vec3  center;
    float size;
};

void main() {}
