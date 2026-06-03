// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// IrradianceGenFParamUBO_Legacy blueprint (= set=3 binding=52, cadence=PerProgram)
// Source: literal extract from class2/interface/irradianceGenF.glsl:42 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=52 / cadence=PerProgram / 4 member

#version 450

layout(std140, set = 3, binding = 52) uniform IrradianceGenFParamUBO_Legacy
{
    int   sourceIdx;
    float max_probe_lod;
    float _pad_irradiance_gen_f_legacy_0;
    float _pad_irradiance_gen_f_legacy_1;
};

void main() {}
