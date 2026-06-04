// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-7γ-3 set=3
// Skin_GLTFJoints blueprint (= set=3 binding=2, cadence=PerSkin)
// Source: literal extract from class1/gltf/pbrmetallicroughnessV.glsl:284-287
// Spec:   design 06b §2.5 (per-skin cadence)、PC-7γ-3 design-lock §4.1.1 step (a)/(d)
// Note:   MAX_NODES_PER_GLTF_OBJECT は viewer 側 #define で gGLManager.mMaxUniformBlockSize/48
//         由来の runtime 上限 (= Vulkan 1.3 min 16384 B → 341 nodes、OpenGL 65536 B → 1365 nodes)。
//         GLSL アクセスは gltf_joints[i*3 + 0..2] で 3 vec4 = mat3x4 1 joint (= pbrmetallicroughnessV.glsl:313-321)。
//         blueprint は std140 array 上限を Vulkan min 16384 B / 16 = 1024 vec4 で固定宣言、
//         実 buffer 確保は upload 時 runtime size を写し込む (= G5-A1: upper bound at register,
//         runtime size at write、PC-7γ-3 design-lock §4.3 SKIN_JOINTS_UPPER_BOUND_SIZE)。
// Set:    set=3 binding=2 (= llvkloader.cpp:657 V3A_ASSET_SET_BINDINGS=3 内 3 番目、PerAsset 同居)

#version 450

layout(std140, set = 3, binding = 2) uniform Skin_GLTFJoints
{
    vec4 gltf_joints[1024];
};

void main() {}
