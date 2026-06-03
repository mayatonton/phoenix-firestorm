// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// PreviewVParamUBO_Legacy blueprint (= set=3 binding=40, cadence=PerProgram)
// Source: literal extract from class1/objects/previewV.glsl:47 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=40 / cadence=PerProgram / 7 member

#version 450

layout(std140, set = 3, binding = 40) uniform PreviewVParamUBO_Legacy
{
    mat4 texture_matrix0;
    vec4 ambient_color;
    vec4 color;
    vec4 light_position[8];
    vec4 light_direction[8];
    vec4 light_attenuation[8];
    vec4 light_diffuse[8];
};

void main() {}
