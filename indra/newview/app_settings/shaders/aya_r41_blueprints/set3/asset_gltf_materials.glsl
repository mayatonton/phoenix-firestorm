// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-7γ-3 set=3
// Asset_GLTFMaterials blueprint (= set=3 binding=1, cadence=PerAsset)
// Source: literal extract from class1/gltf/pbrmetallicroughnessV.glsl:66-82
//         + class1/gltf/pbrmetallicroughnessF.glsl:38-42 (= 同 block 定義 V/F 共有)
// Spec:   design 06b §2.4 (per-asset cadence)、PC-7γ-3 design-lock §4.1.1 step (a)/(d)
// Note:   MAX_UBO_VEC4S は viewer 側 #define で gGLManager.mMaxUniformBlockSize/16
//         由来の runtime 上限 (= Vulkan 1.3 min 16384 B → 1024 vec4、OpenGL 65536 B → 4096 vec4)。
//         blueprint は std140 array 上限を Vulkan min 16384 B / 16 = 1024 vec4 で固定宣言、
//         実 buffer 確保は upload 時 runtime size を写し込む (= G5-A1: upper bound at register,
//         runtime size at write、PC-7γ-3 design-lock §4.3 ASSET_MATERIALS_UPPER_BOUND_SIZE)。
// Set:    set=3 binding=1 (= llvkloader.cpp:657 V3A_ASSET_SET_BINDINGS=3 内 2 番目)

#version 450

layout(std140, set = 3, binding = 1) uniform Asset_GLTFMaterials
{
    // see class1/gltf/pbrmetallicroughnessV.glsl:66-82 for packing layout
    vec4 gltf_material_data[1024];
};

void main() {}
