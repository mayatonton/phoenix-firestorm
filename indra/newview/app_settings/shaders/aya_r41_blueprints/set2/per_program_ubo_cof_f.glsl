// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_CofF blueprint (= set=2 binding=21, cadence=PerProgram)
// Source: literal extract from class1/deferred/cofF.glsl:56 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=21 / cadence=PerProgram / 8 member (float × 8), total 32 B

#version 450

layout(std140, set = 2, binding = 21) uniform PerProgramUBO_CofF
{
    float depth_cutoff;
    float norm_cutoff;
    float focal_distance;
    float blur_constant;
    float tan_pixel_angle;
    float magnification;
    float _pad0;
    float _pad1;
};

void main() {}
