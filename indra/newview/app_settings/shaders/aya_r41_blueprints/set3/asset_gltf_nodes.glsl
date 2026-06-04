// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-7γ-3 set=3
// Asset_GLTFNodes blueprint (= set=3 binding=0, cadence=PerAsset)
// Source: literal extract from class1/gltf/pbrmetallicroughnessV.glsl:335-338
// Spec:   design 06b §2.4 (per-asset cadence)、PC-7γ-3 design-lock §4.1.1 step (a)/(d)
// Note:   MAX_NODES_PER_GLTF_OBJECT は viewer 側 #define で gGLManager.mMaxUniformBlockSize/48
//         由来の runtime 上限 (= Vulkan 1.3 min 16384 B → 341 nodes、OpenGL 65536 B → 1365 nodes)。
//         blueprint は std140 array 上限を Vulkan min 16384 B / 16 = 1024 vec4 で固定宣言、
//         実 buffer 確保は upload 時 runtime size を写し込む (= G5-A1: upper bound at register,
//         runtime size at write、PC-7γ-3 design-lock §4.3 ASSET_NODES_UPPER_BOUND_SIZE)。
// Set:    set=3 binding=0 (= llvkloader.cpp:657 V3A_ASSET_SET_BINDINGS=3 内 1 番目)

#version 450

layout(std140, set = 3, binding = 0) uniform Asset_GLTFNodes
{
    vec4 gltf_nodes[1024];
};

void main() {}
