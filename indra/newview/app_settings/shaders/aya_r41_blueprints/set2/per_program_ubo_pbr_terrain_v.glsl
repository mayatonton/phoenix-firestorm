// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_PbrTerrainV blueprint (= set=2 binding=24, cadence=PerProgram)
// Source: literal extract from class1/deferred/pbrterrainV.glsl:79 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=24 / cadence=PerProgram / 5 member (vec4 array[5] + 4 float)

#version 450

layout(std140, set = 2, binding = 24) uniform PerProgramUBO_PbrTerrainV
{
    vec4  terrain_texture_transforms[5];
    float region_scale;
    float _pad0;
    float _pad1;
    float _pad2;
};

void main() {}
