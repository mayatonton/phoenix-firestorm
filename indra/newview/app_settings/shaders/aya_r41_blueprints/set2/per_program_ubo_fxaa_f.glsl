// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_FxaaF blueprint (= set=2 binding=9, cadence=PerProgram)
// Source: literal extract from class1/deferred/fxaaF.glsl:2126 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=9 / cadence=PerProgram / 5 member (vec2 + 2 float pad + vec4 × 2)

#version 450

layout(std140, set = 2, binding = 9) uniform PerProgramUBO_FxaaF
{
    vec2  rcp_screen_res;
    float _pad_fxaa0;
    float _pad_fxaa1;
    vec4  rcp_frame_opt;
    vec4  rcp_frame_opt2;
};

void main() {}
