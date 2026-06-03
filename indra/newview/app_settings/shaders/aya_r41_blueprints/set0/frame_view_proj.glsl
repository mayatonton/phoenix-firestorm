// AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-8
// FrameViewProj UBO blueprint (= set=0 binding=0)
// Source: literal extract from class1/deferred/pbropaqueF.glsl:198 ifdef LL_VULKAN_GLSL block
//         (verified identical across 5 sample sites = pbropaqueF / simpleNoColorV /
//          previewPhysicsV / bumpV / simpleNoAtmosV)
// Spec:   docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-ubo-current-state-inventory.md §3.1
//         set=0 帯 binding=0 / cadence=Frame / size=512B (推定 mat4*6 + mat3*2 + vec2)

#version 450

layout(std140, set = 0, binding = 0) uniform FrameViewProj
{
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    mat4 projection_matrix;
    mat4 inv_proj;
    mat4 proj_mat;
    mat4 last_modelview_matrix;
    mat3 env_mat;
    mat3 normal_matrix;
    vec2 screen_res;
};

void main() {}
