// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_GodraysF blueprint (= set=2 binding=17, cadence=PerProgram)
// Source: literal extract from class1/deferred/godraysF.glsl:124 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=17 / cadence=PerProgram / 3 member (int + float × 2), total 48 B

#version 450

layout(std140, set = 2, binding = 17) uniform PerProgramUBO_GodraysF
{
    int   aya_r15_godrays_enabled;
    float aya_r15_godrays_phase_exponent;
    float aya_r15_godrays_strength;
};

void main() {}
