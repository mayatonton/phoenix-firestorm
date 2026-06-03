// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8
// MaterialUBO blueprint (= set=1 binding=0, PBR-extended canonical form)
// Source: literal extract from class1/deferred/pbropaqueF.glsl:44 ifdef LL_VULKAN_GLSL block
//         (verified identical across 4 PBR-extended sample sites =
//          pbropaqueF / pbropaqueV / pbralphaV / class2/pbralphaF)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.2
//         set=1 帯 binding=0 / cadence=Program / size=10 member (= base 6 + PBR 4)
// Note:   46 file 中、4 PBR site が 10-member、残り 45 site が 6-member base のみを宣言。
//         AYA option (I) 採用 = 10-member full を canonical blueprint とする = host C++
//         側 allocate は full size、6-member shader は trailing 4 member 未参照 view
//         (= Vulkan std140 layout-compat 慣用: larger buffer に smaller block view 合法、
//          既存 OpenGL 動作で proven、shader 改修ゼロ堅持)。

#version 450

layout(std140, set = 1, binding = 0) uniform MaterialUBO
{
    mat4  texture_matrix0;
    vec4  texture_base_color_transform[2];
    vec4  texture_emissive_transform[2];
    vec4  color;
    vec3  emissiveColor;
    float _pad_emissive;
    float metallicFactor;
    float roughnessFactor;
    float _pad_material0;
    float _pad_material1;
};

void main() {}
