// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// RadianceGenFParamUBO_Legacy blueprint (= set=3 binding=51, cadence=PerProgram)
// Source: literal extract from class1/interface/radianceGenF.glsl:42 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=51 / cadence=PerProgram / 8 member

#version 450

layout(std140, set = 3, binding = 51) uniform RadianceGenFParamUBO_Legacy
{
    int   sourceIdx;
    int   u_width;
    float mipLevel;
    float max_probe_lod;
    float probe_strength;
    float _pad_radiance_gen_f_legacy_0;
    float _pad_radiance_gen_f_legacy_1;
    float _pad_radiance_gen_f_legacy_2;
};

void main() {}
