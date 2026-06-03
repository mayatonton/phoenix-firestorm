// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// AvatarClothVParamUBO_Legacy blueprint (= set=3 binding=57, cadence=PerProgram)
// Source: literal extract from class1/deferred/avatarV.glsl:109 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=57 / cadence=PerProgram / 3 member

#version 450

layout(std140, set = 3, binding = 57) uniform AvatarClothVParamUBO_Legacy
{
    vec4 gWindDir;
    vec4 gSinWaveParams;
    vec4 gGravity;
};

void main() {}
