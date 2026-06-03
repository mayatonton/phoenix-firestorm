// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_WaterF blueprint (= set=2 binding=23, cadence=PerProgram)
// Source: literal extract from class3/environment/waterF.glsl:119 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=23 / cadence=PerProgram / 8 member (vec3 + float + vec3 + 5 float)
// Note:   kd は declared-but-unused (host setter なし、shader 内参照なし)。

#version 450

layout(std140, set = 2, binding = 23) uniform PerProgramUBO_WaterF
{
    vec3  specular;
    float blend_factor;
    vec3  normScale;
    float blurMultiplier;
    float refScale;
    float kd;
    float fresnelScale;
    float fresnelOffset;
};

void main() {}
