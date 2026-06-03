// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8 set=2
// PerDrawUBO_LightParams blueprint (= set=2 binding=0, cadence=PerDraw)
// Source: literal extract from class1/deferred/deferredUtil.glsl:175 ifdef LL_VULKAN_GLSL block (single site)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.3
//         set=2 帯 binding=0 / cadence=PerDraw / 2 member (vec3 + float)
// Note:   deferredUtil.glsl では UBO 直後に #define color spot_light_color / #define size spot_light_size
//         alias が並ぶが、blueprint は UBO 本体のみ。
//         set=2 binding=0 を共有する 6 UBO の 1 つ (= per_draw_ubo_clip_plane.glsl header 参照)。

#version 450

layout(std140, set = 2, binding = 0) uniform PerDrawUBO_LightParams
{
    vec3  spot_light_color;
    float spot_light_size;
};

void main() {}
