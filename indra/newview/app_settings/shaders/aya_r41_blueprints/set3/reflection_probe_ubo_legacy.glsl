// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// ReflectionProbeUBO_Legacy blueprint (= set=3 binding=17, cadence=PerProgram)
// Source: literal extract from class3/deferred/reflectionProbeF.glsl:80 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=17 / cadence=PerProgram / 2 member

#version 450

layout(std140, set = 3, binding = 17) uniform ReflectionProbeUBO_Legacy
{
    float max_probe_lod;
    bool transparent_surface;
};

void main() {}
