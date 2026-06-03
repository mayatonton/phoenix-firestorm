// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerProgramUBO_FsObjectIdF blueprint (= set=2 binding=13, cadence=PerProgram)
// Source: literal extract from class1/deferred/fsObjectIDF.glsl:33 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=13 / cadence=PerProgram / 1 member (vec4)

#version 450

layout(std140, set = 2, binding = 13) uniform PerProgramUBO_FsObjectIdF
{
    vec4 object_id_packed;
};

void main() {}
