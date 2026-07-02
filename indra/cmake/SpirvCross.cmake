# -*- cmake -*-

# AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-1
# (= UBO codegen pipeline = host C++ side std140 layout reflection / cross-compile):
# spirv-cross C shared library integration for `scripts/ubo_codegen/` Python tool chain.
#
# Role: SPIR-V binary (glslang -V 出力) を入力に std140 offset/stride/alignment を
#   reflection し、Python codegen 側 (= chapter 08 §5.4.1 / §11.5) で
#   85 UBO blueprint → 4 file 生成 (LLUboLayouts.h / .cpp / vk_descriptors.h /
#   binding map) の二重保証層 (= mini-parser + SPIR-V reflection cross-check) を実現.
#
# System libspirv-cross-c-shared-dev (Ubuntu 24.04 apt) provides:
#   - /usr/include/spirv_cross/spirv_cross_c.h (C API header)
#   - /usr/lib/x86_64-linux-gnu/libspirv-cross-c-shared.so (動的)
#   - /usr/lib/x86_64-linux-gnu/cmake/spirv_cross_c_shared/spirv_cross_c_shared-config.cmake
#     (find_package CONFIG mode 対応)
#
# Linux first-class baseline (r41 charter §1)、Mac/Win 3 OS bundle 判断は r42-α/β 着手時
# (charter §7.5). Glslang.cmake と完全 symmetric pattern (system pkg + find_package CONFIG
# REQUIRED + ll::* INTERFACE IMPORTED + target_link_libraries).

find_package(spirv_cross_c_shared CONFIG REQUIRED)

add_library( ll::spirv_cross INTERFACE IMPORTED )

target_link_libraries( ll::spirv_cross INTERFACE
    spirv-cross-c-shared
    )
