// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=3
// DeferredUtilParamUBO_Legacy blueprint (= set=3 binding=6, cadence=PerProgram)
// Source: literal extract from class1/deferred/deferredUtil.glsl:99 ifdef LL_VULKAN_GLSL block
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.4
//         set=3 帯 binding=6 / cadence=PerProgram / 8 member

#version 450

layout(std140, set = 3, binding = 6) uniform DeferredUtilParamUBO_Legacy
{
    vec3  proj_n;          // projector normal
    float proj_focus;      // distance from plane to begin blurring
    vec3  proj_p;          // plane projection is emitting from (in screen space)
    float proj_lod;        // (number of mips in proj map)
    float proj_range;      // range between near clip and far clip plane of projection
    float proj_ambiance;
    float waterSign;
    float _pad_deferred_util_legacy_0;
};

void main() {}
