// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// WaterVParamUBO_Legacy blueprint (= set=3 binding=60, cadence=PerProgram)
// Source: literal extract from class1/environment/waterV.glsl:61 ifdef LL_VULKAN_GLSL block
// Multi-site: verified identical at class3/environment/waterF.glsl:108
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=60 / cadence=PerProgram / 6 member

#version 450

layout(std140, set = 3, binding = 60) uniform WaterVParamUBO_Legacy
{
    vec2 waveDir1;
    vec2 waveDir2;
    float time;
    vec3 eyeVec;
    float waterHeight;
    vec3 lightDir;
};

void main() {}
