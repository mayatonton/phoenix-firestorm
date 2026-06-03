// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8
// FrameLights UBO blueprint (= set=0 binding=1)
// Source: literal extract from class1/windlight/atmosphericsV.glsl:33 ifdef LL_VULKAN_GLSL block
//         (verified identical across 5 sample sites = atmosphericsV / atmosphericsFuncs /
//          simpleColorF / cinematic_bd/shadowUtil / sumLightsV)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.1
//         set=0 帯 binding=1 / cadence=Frame

#version 450

layout(std140, set = 0, binding = 1) uniform FrameLights
{
    int  sun_up_factor;
    vec3 sun_dir;
    vec3 moon_dir;
    vec4 waterPlane;
    vec4 light_position[8];
    vec3 light_direction[8];
    vec4 light_attenuation[8];
    vec3 light_diffuse[8];
    vec2 light_deferred_attenuation[8];
};

void main() {}
