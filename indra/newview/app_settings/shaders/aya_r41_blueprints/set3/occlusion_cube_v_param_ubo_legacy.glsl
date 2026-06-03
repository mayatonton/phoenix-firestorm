// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// OcclusionCubeVParamUBO_Legacy blueprint (= set=3 binding=50, cadence=PerProgram)
// Source: literal extract from class1/interface/occlusionCubeV.glsl:54 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=50 / cadence=PerProgram / 4 member

#version 450

layout(std140, set = 3, binding = 50) uniform OcclusionCubeVParamUBO_Legacy
{
    vec3  box_center;
    float _pad_occlusion_cube_v_legacy_0;
    vec3  box_size;
    float _pad_occlusion_cube_v_legacy_1;
};

void main() {}
