// AYAstorm r41 sub-step 3.3-B-β-2 exemplar pre-flight (sub-doc 03 §3.1.3)
// Vulkan path: build 時 pre-compile (glslangValidator --target-env vulkan1.3)
// → runtime load (LLVKLoader::loadSpirvShaderModule) → PSO compile (3.3-B-γ で配線)
//
// 内容: 3.3-A γ embedded SPIR-V (kSkySmokeVertSpv) と等価な fullscreen triangle vert。
// gl_VertexIndex 0/1/2 で (-1,-1) / (3,-1) / (-1,3) を生成、vertex input binding 不要。
// 二段構え matrix binding (PerFrameMatrixUBO / TextureMatrixUBO / push constant
// modelview) の shader 側受領は B-δ 範疇、本 β-2 では build chain 実証のみ。

#version 450

void main()
{
    vec2 pos = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2)) * 2.0 - vec2(1.0);
    gl_Position = vec4(pos, 0.0, 1.0);
}
