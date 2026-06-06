# -*- cmake -*-

# AYAstorm r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.A PA-7
# UBO Codegen build wiring
# Spec source of truth: docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md
#   §12.2 / §12.3 B5a (CMake DEPENDS 自動 + 手動 target 併設) default
#   §12.5.2 CONFIGURE_DEPENDS で新規 GLSL 自動検出
#   §12.5.3 add_dependencies + target_include_directories
#   §12.5.4 手動 codegen_ubo_force target (= cache 完全 invalidate)
#   §12.5.7 cache hit 時 touch on outputs (CMake DEPENDS 解消用)
#
# 動作概要:
#   build 時 scripts/ubo_codegen/main.py が走り、
#   indra/newview/app_settings/shaders/{class1,class2,class3,cinematic_bd}/ 配下
#   *.glsl を入力に ${CMAKE_BINARY_DIR}/codegen/ubo/ に 5 aggregated .inl + per-block
#   layout .inl 群を emit。cache (= codegen_state.json) hit 時は touch のみで早期 return。
#   設計時参考資料 aya_r41_blueprints/ は **入力対象外** (= Phase 2.α 案 Z 確定
#   2026-06-06、設計 doc 整合修復、blueprint dir reference 降格、AYA 指示 #5 整合
#   = design/01-overview.md:146 「85 GLSL UBO blueprint は discard しない」literal、
#   blueprint dir は物理保持しつつ Codegen 入力からは除外、詳細 =
#   aya_r41_blueprints/README.md + handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md §D)。
#
# 接続: 利用 target からは aya_attach_ubo_codegen(<target>) を呼ぶ。
#   add_dependencies(<target> codegen_ubo) + target_include_directories(<target>
#   PUBLIC ${AYA_UBO_CODEGEN_INCLUDE_DIR}) を一括設定する。

include(Python)

# ---- glslang / spirv-cross 解決 ----

# AyaShaderCompile.cmake が既に find_program 済なら再利用、未 include なら独自検出。
if (NOT DEFINED AYA_GLSLANG_VALIDATOR OR NOT AYA_GLSLANG_VALIDATOR)
    find_program(AYA_GLSLANG_VALIDATOR glslangValidator)
endif()

find_program(AYA_SPIRV_CROSS spirv-cross)

if (NOT AYA_GLSLANG_VALIDATOR)
    message(WARNING
        "AYAstorm r41 (PA-7): glslangValidator not found in PATH. "
        "UBO Codegen requires glslang for SPIR-V cross-verification. "
        "Linux: 'sudo apt install glslang-tools'. "
        "Mac/Win: install Vulkan SDK. "
        "codegen_ubo target は configure される (= path 未設定で run 時 fail)。")
endif()

if (NOT AYA_SPIRV_CROSS)
    message(STATUS
        "AYAstorm r41 (PA-7): spirv-cross not found in PATH. "
        "Codegen tool will fall back to AYA_CODEGEN_SKIP_SPIRV_CHECK=1 path "
        "(SPIR-V binding 番号 cross-check skip).")
endif()

# ---- 入出力 path 定義 ----

set(AYA_UBO_CODEGEN_SCRIPT
    "${CMAKE_SOURCE_DIR}/../scripts/ubo_codegen/main.py"
    CACHE FILEPATH "AYAstorm r41 UBO Codegen entry script")
mark_as_advanced(AYA_UBO_CODEGEN_SCRIPT)

# Phase 2.α 案 Z 確定 (= 2026-06-06): Codegen 入力 source は actual shader dir 群
# (= class*/ + cinematic_bd/) = 設計 doc 想定 (design/08-build-codegen-pipeline.md:72-74) 整合復元。
# 設計時参考資料 aya_r41_blueprints/ は **入力対象外** (= reference 降格、物理保持、
# AYA 指示 #5 = design/01-overview.md:146 「discard しない」literal 整合、
# blueprint dir 詳細 = aya_r41_blueprints/README.md)。
# 同名 UBO 複数 file 整合 verify (= class*/ ↔ cinematic_bd/ 上書き path 同 layout
# 一致 check) は main.py `_verify_block_match` (= Phase 2.α α-2 commit b66ec99f72) が担当。
set(AYA_UBO_CODEGEN_SHADER_SOURCE_DIRS
    "${CMAKE_SOURCE_DIR}/newview/app_settings/shaders/class1"
    "${CMAKE_SOURCE_DIR}/newview/app_settings/shaders/class2"
    "${CMAKE_SOURCE_DIR}/newview/app_settings/shaders/class3"
    "${CMAKE_SOURCE_DIR}/newview/app_settings/shaders/cinematic_bd"
    CACHE STRING "AYAstorm r41 UBO Codegen shader source dirs (= class*/ + cinematic_bd/ actual shader dirs、aya_r41_blueprints/ は reference 降格で対象外)")
mark_as_advanced(AYA_UBO_CODEGEN_SHADER_SOURCE_DIRS)

set(AYA_UBO_CODEGEN_OUTPUT_DIR
    "${CMAKE_BINARY_DIR}/codegen/ubo"
    CACHE PATH "AYAstorm r41 UBO Codegen output directory")
mark_as_advanced(AYA_UBO_CODEGEN_OUTPUT_DIR)

set(AYA_UBO_CODEGEN_INCLUDE_DIR
    "${CMAKE_BINARY_DIR}/codegen"
    CACHE PATH "AYAstorm r41 UBO Codegen include root (= parent of ubo/)")
mark_as_advanced(AYA_UBO_CODEGEN_INCLUDE_DIR)

set(AYA_UBO_CODEGEN_CACHE_FILE
    "${CMAKE_BINARY_DIR}/codegen/cache/codegen_state.json"
    CACHE FILEPATH "AYAstorm r41 UBO Codegen incremental cache state file")
mark_as_advanced(AYA_UBO_CODEGEN_CACHE_FILE)

# 出力 dir / cache dir を configure 時に作成 (= run 時の mkdir race 排除)
file(MAKE_DIRECTORY "${AYA_UBO_CODEGEN_OUTPUT_DIR}")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/codegen/cache")

# ---- DEPENDS 構築 ----

# §12.5.2: CONFIGURE_DEPENDS で build 時 GLSL dir mtime 検査 → 新規 .glsl 自動検出
# (= Phase 2.α 案 Z 確定: SHADER_SOURCE_DIRS 全 dir に GLOB_RECURSE 連合 pattern)
set(_aya_codegen_glsl_globs)
foreach(_aya_codegen_dir IN LISTS AYA_UBO_CODEGEN_SHADER_SOURCE_DIRS)
    list(APPEND _aya_codegen_glsl_globs "${_aya_codegen_dir}/*.glsl")
endforeach()
file(GLOB_RECURSE AYA_UBO_CODEGEN_GLSL_FILES
    CONFIGURE_DEPENDS
    ${_aya_codegen_glsl_globs}
)
unset(_aya_codegen_glsl_globs)

# Python module 群 (= main.py が import する全 module) の変更も走行 trigger に。
# CONFIGURE_DEPENDS なし = module 追加時は手動 reconfigure (= shader 追加と同流)。
file(GLOB AYA_UBO_CODEGEN_MODULES
    "${CMAKE_SOURCE_DIR}/../scripts/ubo_codegen/*.py"
)

# ---- 期待出力 (= add_custom_command OUTPUT 列挙) ----
#
# 5 aggregated 固定 file。per-block layout (= ubo_layout_<name>.inl) は
# blueprint 数で動的決定 = OUTPUT に列挙不可。ubo_index.inl が層集約 include 役で、
# 利用 target は ubo_index.inl 経由で per-block layout を取り込む。
# cache hit path も canonical_output_files() 全件 touch するため CMake DEPENDS は解消される。
set(AYA_UBO_CODEGEN_OUTPUTS
    "${AYA_UBO_CODEGEN_OUTPUT_DIR}/ubo_index.inl"
    "${AYA_UBO_CODEGEN_OUTPUT_DIR}/ubo_perfect_hash.inl"
    "${AYA_UBO_CODEGEN_OUTPUT_DIR}/ubo_metadata.inl"
    "${AYA_UBO_CODEGEN_OUTPUT_DIR}/ubo_dummy_init.inl"
    "${AYA_UBO_CODEGEN_OUTPUT_DIR}/ubo_host_loader.inl"
)

# ---- 走行 command 共通引数 ----

# --input は main.py argparse `nargs='+'` (= α-2 commit b66ec99f72) で複数 dir 受領
# (= SHADER_SOURCE_DIRS list を空白展開で複数 arg として渡す)
set(_aya_codegen_common_args
    "${AYA_UBO_CODEGEN_SCRIPT}"
    --input  ${AYA_UBO_CODEGEN_SHADER_SOURCE_DIRS}
    --output "${AYA_UBO_CODEGEN_OUTPUT_DIR}"
    --cache-file "${AYA_UBO_CODEGEN_CACHE_FILE}"
    --project-root "${CMAKE_SOURCE_DIR}/.."
)

if (AYA_GLSLANG_VALIDATOR)
    list(APPEND _aya_codegen_common_args --glslang-bin "${AYA_GLSLANG_VALIDATOR}")
endif()

if (AYA_SPIRV_CROSS)
    list(APPEND _aya_codegen_common_args --spirv-cross-bin "${AYA_SPIRV_CROSS}")
endif()

# spirv-cross 未検出時は AYA_CODEGEN_SKIP_SPIRV_CHECK=1 を env で渡して
# Codegen tool の toolchain probe を skip (= SPIR-V binding 番号 cross-check 省略)。
set(_aya_codegen_command_prefix)
if (NOT AYA_SPIRV_CROSS)
    set(_aya_codegen_command_prefix
        "${CMAKE_COMMAND}" -E env "AYA_CODEGEN_SKIP_SPIRV_CHECK=1")
endif()

# ---- 自動 trigger custom_command + codegen_ubo target (§12.3 / §12.5.3) ----

add_custom_command(
    OUTPUT  ${AYA_UBO_CODEGEN_OUTPUTS}
    COMMAND ${_aya_codegen_command_prefix} "${PYTHON_EXECUTABLE}" ${_aya_codegen_common_args}
    DEPENDS
        ${AYA_UBO_CODEGEN_GLSL_FILES}
        ${AYA_UBO_CODEGEN_MODULES}
        "${AYA_UBO_CODEGEN_SCRIPT}"
    COMMENT "AYAstorm r41 (PA-7): generating UBO codegen artifacts"
    VERBATIM
)

add_custom_target(codegen_ubo
    DEPENDS ${AYA_UBO_CODEGEN_OUTPUTS}
)

# ---- 手動 force target (§12.5.4) ----

add_custom_target(codegen_ubo_force
    COMMAND ${_aya_codegen_command_prefix} "${PYTHON_EXECUTABLE}" ${_aya_codegen_common_args} --force
    COMMENT "AYAstorm r41 (PA-7): force-regenerating UBO codegen artifacts (= cache 完全 invalidate)"
    VERBATIM
)

# ---- 利用 target への接続 helper ----
#
# aya_attach_ubo_codegen(<target>)
#   <target>: codegen 出力 .inl を include する CMake target (例: llrender / llvkloader)
#
# add_dependencies(<target> codegen_ubo) + target_include_directories(<target>
# PUBLIC ${AYA_UBO_CODEGEN_INCLUDE_DIR}) を一括設定。
# build order = codegen_ubo 完了 → <target> compile (= §12.5.3)。

function(aya_attach_ubo_codegen TARGET_NAME)
    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR
            "aya_attach_ubo_codegen: target '${TARGET_NAME}' does not exist. "
            "Call this function after the target is created.")
    endif()
    add_dependencies(${TARGET_NAME} codegen_ubo)
    target_include_directories(${TARGET_NAME}
        PUBLIC "${AYA_UBO_CODEGEN_INCLUDE_DIR}")
endfunction()

message(STATUS "AYAstorm r41 (PA-7): UBO Codegen wired = ${AYA_UBO_CODEGEN_SCRIPT}")
foreach(_aya_codegen_dir IN LISTS AYA_UBO_CODEGEN_SHADER_SOURCE_DIRS)
    message(STATUS "AYAstorm r41 (PA-7):   shader source dir = ${_aya_codegen_dir}")
endforeach()
unset(_aya_codegen_dir)
message(STATUS "AYAstorm r41 (PA-7):   output dir      = ${AYA_UBO_CODEGEN_OUTPUT_DIR}")
message(STATUS "AYAstorm r41 (PA-7):   cache file      = ${AYA_UBO_CODEGEN_CACHE_FILE}")
list(LENGTH AYA_UBO_CODEGEN_GLSL_FILES _aya_ubo_glsl_count)
message(STATUS "AYAstorm r41 (PA-7):   GLSL source count = ${_aya_ubo_glsl_count} .glsl files (class*/ + cinematic_bd/)")
