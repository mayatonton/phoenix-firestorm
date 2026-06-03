// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// SkyVParamUBO_Legacy blueprint (= set=3 binding=1, cadence=PerProgram)
// Source: literal extract from class1/deferred/skyV.glsl:83 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=1 / cadence=PerProgram / 2 member

#version 450

layout(std140, set = 3, binding = 1) uniform SkyVParamUBO_Legacy
{
    vec3 camPosLocal;
    vec3 _pad_sky_v_legacy_0;
};

void main() {}
