// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// UnderWaterFParamUBO_Legacy blueprint (= set=3 binding=39, cadence=PerProgram)
// Source: literal extract from class3/environment/underWaterF.glsl:63 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=39 / cadence=PerProgram / 14 member

#version 450

layout(std140, set = 3, binding = 39) uniform UnderWaterFParamUBO_Legacy
{
    vec4  fogCol;
    vec3  lightDir_underwater_legacy;
    float lightExp;
    vec3  specular;
    float refScale;
    vec2  fbScale;
    float znear;
    float zfar;
    float kd;
    vec3  eyeVec_underwater_legacy;
    vec4  waterFogColor_underwater_legacy;
    vec3  waterFogColorLinear;
    float waterFogKS_underwater_legacy;
    vec2  screenRes;
};

void main() {}
