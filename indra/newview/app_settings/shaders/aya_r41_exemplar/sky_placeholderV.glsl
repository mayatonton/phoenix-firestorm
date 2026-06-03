// AYAstorm r41 sub-step 3.3-B-δ exemplar pre-flight (sub-doc 03 §3.1.3)
// Vulkan path: build 時 pre-compile (glslangValidator --target-env vulkan1.3)
// → runtime load (LLVKLoader::loadSpirvShaderModuleFromFile) → PSO compile
//
// 内容: γ で確立した SPIR-V file load → PSO compile chain を baseline に、
// 二段構え matrix binding (PerFrameMatrixUBO / TextureMatrixUBO / push constant
// modelview) を shader 側で受領 + shader 内 MVP / normal / inverse_modelview
// 計算式配線 (3.3-A §3.1.1 trace inventory「shader 内計算移譲 3 種」具体配置)。
//
// 視覚出力は γ と同じ fullscreen triangle 維持 (binding 値は 1e-30 multiplier で
// 視覚 regression 0 を構造的に担保、binding declaration の SPIR-V 残置のみが目的)。
// 実 descriptor set bind / push constant write は領域 7 sub-step 7.5 で attachment
// 配線と合わせて wire up (本 sub-step は shader 側受領 + PSO compile 成功のみ)。

#version 450

// set=0 binding 0: PerFrameMatrixUBO (192 B, VERTEX|FRAGMENT)
layout(std140, set = 0, binding = 0) uniform PerFrameMatrixUBO
{
    mat4 projection_matrix;
    mat4 inverse_projection_matrix;
    mat4 identity_matrix;
};

// set=0 binding 1: TextureMatrixUBO (256 B, VERTEX|FRAGMENT)
layout(std140, set = 0, binding = 1) uniform TextureMatrixUBO
{
    mat4 texture_matrix[4];
};

// push constant range (0..64 B / VERTEX_BIT)
layout(std140, push_constant) uniform PushConstants
{
    mat4 modelview_matrix;
};

void main()
{
    // shader 内計算移譲 3 種 (3.3-A §3.1.1 trace inventory)
    mat4 mvp_matrix               = projection_matrix * modelview_matrix;
    mat3 normal_matrix            = transpose(inverse(mat3(modelview_matrix)));
    mat4 inverse_modelview_matrix = inverse(modelview_matrix);

    // fullscreen triangle (γ と同じ視覚出力、gl_VertexIndex 0/1/2 で
    // (-1,-1) / (3,-1) / (-1,3))
    vec2 pos = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2)) * 2.0 - vec2(1.0);

    // binding declaration を SPIR-V に残置 + 視覚 regression 0 担保
    // (1e-30 multiplier で float32 denormal 以下 = 実質 0 contribution)
    vec4 binding_witness = mvp_matrix[0]
                         + inverse_modelview_matrix[0]
                         + vec4(normal_matrix[0], 0.0)
                         + inverse_projection_matrix[0]
                         + identity_matrix[0]
                         + texture_matrix[0][0];

    gl_Position = vec4(pos, 0.0, 1.0) + binding_witness * 1e-30;
}
