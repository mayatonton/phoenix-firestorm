// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// AvatarFParamUBO_Legacy blueprint (= set=3 binding=54, cadence=PerProgram)
// Source: literal extract from class1/deferred/avatarF.glsl:76 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=54 / cadence=PerProgram / 4 member

#version 450

layout(std140, set = 3, binding = 54) uniform AvatarFParamUBO_Legacy
{
    float aya_sss_skin_flag;
    float _pad_avatar_f_legacy_0;
    float _pad_avatar_f_legacy_1;
    float _pad_avatar_f_legacy_2;
};

void main() {}
