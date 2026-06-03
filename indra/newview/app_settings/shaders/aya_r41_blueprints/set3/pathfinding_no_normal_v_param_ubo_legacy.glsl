// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// PathfindingNoNormalVParamUBO_Legacy blueprint (= set=3 binding=48, cadence=PerProgram)
// Source: literal extract from class1/interface/pathfindingNoNormalV.glsl:65 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=48 / cadence=PerProgram / 4 member

#version 450

layout(std140, set = 3, binding = 48) uniform PathfindingNoNormalVParamUBO_Legacy
{
    float tint;
    float alpha_scale;
    float _pad_pathfinding_nonormal_v_legacy_0;
    float _pad_pathfinding_nonormal_v_legacy_1;
};

void main() {}
