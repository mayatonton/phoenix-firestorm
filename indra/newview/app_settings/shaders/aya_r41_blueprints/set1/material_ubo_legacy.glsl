// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8
// MaterialUBO_Legacy blueprint (= set=1 binding=0, legacy 派生形)
// Source: literal extract from class3/deferred/materialF.glsl:38 ifdef LL_VULKAN_GLSL block
//         (singleton site = 1 file のみ宣言)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.2
//         set=1 帯 binding=0 / cadence=Program / size=8 member
// Note:   MaterialUBO と同 (set=1, binding=0) だが別 name = 06a §3.2 所見の通り
//         同 program 内で両者 attach は不可 (= program ごとに片方のみ宣言される運用)。
//         metadata 上は 2 entry 共存、host C++ 側 dispatch は name 経由で分離する想定。

#version 450

layout(std140, set = 1, binding = 0) uniform MaterialUBO_Legacy
{
    vec4  morphFactor;
    vec4  specular_color;
    vec3  camPosLocal;
    float emissive_brightness;
    float is_mirror;
    float env_intensity;
    float aya_sss_skin_flag;
    float _pad_material_legacy_0;
};

void main() {}
