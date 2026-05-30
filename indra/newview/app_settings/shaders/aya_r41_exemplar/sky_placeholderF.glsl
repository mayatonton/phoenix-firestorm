// AYAstorm r41 sub-step 3.3-B-β-2 exemplar pre-flight (sub-doc 03 §3.1.3)
// Vulkan path: build 時 pre-compile (glslangValidator --target-env vulkan1.3)
// → runtime load (LLVKLoader::loadSpirvShaderModule) → PSO compile (3.3-B-γ で配線)
//
// 内容: 3.3-A γ embedded SPIR-V (kSkySmokeFragSpv) と等価な fixed-color sky frag。
// 出力色 = sRGB (0.4, 0.6, 0.9, 1.0) (linear 直書き、tonemap は 3.3-C-γ 以降で配線)。

#version 450

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(0.4, 0.6, 0.9, 1.0);
}
