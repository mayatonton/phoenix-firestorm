// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// RlvFParamUBO_Legacy blueprint (= set=3 binding=56, cadence=PerProgram)
// Source: literal extract from class1/deferred/rlvF.glsl:62 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=56 / cadence=PerProgram / 9 member

#version 450

layout(std140, set = 3, binding = 56) uniform RlvFParamUBO_Legacy
{
    vec4  rlvEffectParam1;            // 0-15  Sphere origin (in local coordinates)
    vec4  rlvEffectParam2;            // 16-31 Min/max dist + min/max value
    vec4  rlvEffectParam4;            // 32-47 Sphere params (=color when using blend)
    vec2  rlvEffectParam5;            // 48-55 Blur direction (not used for blend)
    uvec2 rlvEffectParam3_uvec;       // 56-63 Min/max dist extend (bvec2 → uvec2 promote)
    int   rlvEffectMode;              // 64-67 ESphereMode
    int   _pad_rlv_legacy_0;          // 68-71
    int   _pad_rlv_legacy_1;          // 72-75
    int   _pad_rlv_legacy_2;          // 76-79
};

void main() {}
