// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_BlurLightF blueprint (= set=2 binding=22, cadence=PerProgram)
// Source: literal extract from class1/deferred/blurLightF.glsl:64 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=22 / cadence=PerProgram / 9 member (vec2 + float × 2 + vec3 array[4] + float × 4)

#version 450

layout(std140, set = 2, binding = 22) uniform PerProgramUBO_BlurLightF
{
    vec2  delta;
    float dist_factor;
    float blur_size;
    vec3  kern[4];
    float kern_scale;
    float _pad0;
    float _pad1;
    float _pad2;
};

void main() {}
