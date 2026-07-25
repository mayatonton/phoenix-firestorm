# -*- cmake -*-
#
# Copyright (C) 2025-2026 Ishikawa AYA (github: mayatonton, mayatonton1994@gmail.com)
#
# このファイルは Ishikawa AYA が新規に作成した独自著作物である。
# 著作権は Ishikawa AYA が保持し、パブリックドメインには置かない。
# All rights reserved by the author except as licensed below.
#
# Licensed under the GNU Lesser General Public License, version 2.1
# (same license as the Firestorm Viewer). See the LICENSE file for details.
#
# 光の国のひとたちと共にわたしはここにいる　彩
#

# AYAstorm r41 sub-step 4.3-γ'-port-α (sub-doc 06 §3.1 case ② = runtime SPIR-V 生成):
# glslang library integration for LLShaderMgr Vulkan path
# (llshadermgr.cpp:908 直前 gVK.isEnabled() conditional branch → glslang runtime API →
#  SPIR-V binary → vkCreateShaderModule、SPIR-V cache layer 案 C は shader_cache/ 内併存).
#
# System glslang-dev (Ubuntu 24.04 apt 15.1.0-2) provides:
#   - /usr/include/glslang/ (Public/ShaderLang.h + SPIRV/GlslangToSpv.h + SpvTools.h + ResourceLimits.h)
#   - /usr/lib/x86_64-linux-gnu/libglslang.a + libSPIRV.a + libglslang-default-resource-limits.a (静的)
#   - /usr/lib/x86_64-linux-gnu/cmake/glslang/glslang-config.cmake (find_package CONFIG mode 対応)
#
# Linux first-class baseline (r41 charter §1).
# macOS resolves the same imported targets from the pinned vulkan_sdk_macos prebuilt.
# 3.3-B exemplar (試作レール、aya_r41_exemplar/sky_placeholder + AyaShaderCompile.cmake +
# loadSpirvShaderModuleFromFile()) は sub-doc 03 §3.1.3 役割再定義注記で並存維持.

if (DARWIN)
    include(Prebuilt)
    use_prebuilt_binary(vulkan_sdk_macos)
    set(glslang_DIR "${LIBS_PREBUILT_DIR}/lib/cmake/glslang")
    set("SPIRV-Tools_DIR" "${LIBS_PREBUILT_DIR}/lib/cmake/SPIRV-Tools")
    set("SPIRV-Tools-opt_DIR" "${LIBS_PREBUILT_DIR}/lib/cmake/SPIRV-Tools-opt")
    find_package(glslang CONFIG REQUIRED NO_DEFAULT_PATH)
else()
    find_package(glslang CONFIG REQUIRED)
endif()

add_library( ll::glslang INTERFACE IMPORTED )

target_link_libraries( ll::glslang INTERFACE
    glslang::glslang
    glslang::SPIRV
    glslang::glslang-default-resource-limits
    )
