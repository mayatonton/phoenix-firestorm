# -*- cmake -*-

# AYAstorm r41 sub-step 3.3-B-β-2 SPIR-V shader pre-compile chain
# sub-doc 03 §3.1.3 (AYA 確定 2026-05-30)
#
# 役割:
#   build 時に GLSL → SPIR-V (vulkan1.3 target) を pre-compile し、出力 .spv を
#   source tree (.glsl と同位置) に配置。viewer_manifest.py line 104
#   `self.path("shaders")` の recursive copy で packaging 時に同梱される。
#
# 設計選択 (AYA 確定 2026-05-30、β-2 推奨案 / sub-doc 03 §3.1.3):
#   - glslangValidator は system path から find_program、未発見時は WARNING + skip
#     (Mac/Win build host で未 install を許容、3.3-B exemplar は Linux 環境で
#      reference SPIR-V 生成済み → embedded byte array path で fallback 可)。
#   - 出力 .spv は source dir 内 (build dir 外)、*.spv は .gitignore で commit 除外。
#   - 領域 6 sub-step 6.1 一括化で 248 file 全 GLSL に同 chain を拡張予定。

find_program(AYA_GLSLANG_VALIDATOR glslangValidator)

if (NOT AYA_GLSLANG_VALIDATOR)
    message(WARNING
        "AYAstorm r41: glslangValidator not found in PATH. "
        "SPIR-V pre-compile chain (sub-step 3.3-B-β-2) will be skipped. "
        "Linux: 'sudo apt install glslang-tools'. "
        "Mac/Win: install Vulkan SDK or skip (embedded byte array fallback in llvkloader.cpp).")
else()
    message(STATUS "AYAstorm r41: glslangValidator = ${AYA_GLSLANG_VALIDATOR}")
endif()

# aya_compile_shader_spirv(<STAGE> <INPUT_GLSL> <OUTPUT_SPV>)
#   STAGE       : vert / frag / comp / geom / tesc / tese (glslangValidator -S 引数)
#   INPUT_GLSL  : absolute path to .glsl source
#   OUTPUT_SPV  : absolute path to .spv output (typically alongside .glsl)
#
# 出力 .spv は OUTPUT_SPV に生成され、custom_target 経由で build 全体に依存付け可能。
# (caller 側で list(APPEND AYA_SHADER_SPV_OUTPUTS ${OUTPUT_SPV}) → custom_target で集約)
function(aya_compile_shader_spirv STAGE INPUT_GLSL OUTPUT_SPV)
    if (NOT AYA_GLSLANG_VALIDATOR)
        return()
    endif()

    add_custom_command(
        OUTPUT  "${OUTPUT_SPV}"
        COMMAND "${AYA_GLSLANG_VALIDATOR}"
                -V
                --target-env vulkan1.3
                -S "${STAGE}"
                -o "${OUTPUT_SPV}"
                "${INPUT_GLSL}"
        DEPENDS "${INPUT_GLSL}"
        COMMENT "AYAstorm r41: glslang ${STAGE} ${INPUT_GLSL} -> ${OUTPUT_SPV}"
        VERBATIM)
endfunction()
