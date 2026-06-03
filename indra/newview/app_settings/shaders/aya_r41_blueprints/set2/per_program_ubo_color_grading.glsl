// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_ColorGrading blueprint (= set=2 binding=4, cadence=PerProgram)
// Source: literal extract from class1/deferred/postDeferredTonemap.glsl:74 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=4 / cadence=PerProgram / 8 member (float × 5 + int × 1 + float × 2 pad)

#version 450

layout(std140, set = 2, binding = 4) uniform PerProgramUBO_ColorGrading
{
    float color_saturation;
    float color_contrast;
    float color_temperature;
    float color_brightness;
    float color_grading_lut_intensity;
    int   color_grading_lut_enabled;
    float _pad_cg0;
    float _pad_cg1;
};

void main() {}
