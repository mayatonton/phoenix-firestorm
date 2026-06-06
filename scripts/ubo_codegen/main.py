#!/usr/bin/env python3
# AYAstorm r41 UBO Codegen entry point
# Spec source of truth: docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md
"""AYAstorm r41 UBO Codegen entry point.

PA-2 base: arg parse + I/O contract + log + exit codes.
PA-3..PA-5 modules:
    glsl_parser  — mini-parser with `#line` directive tracking
    std140       — offset / size calculator (with 256 B device alignment)
    spirv_reflect — SPIR-V double-verification (escape via AYA_CODEGEN_SKIP_SPIRV_CHECK=1)
    perfect_hash — CHD perfect hash + per-block layout emit
PA-6 wiring (this file): parse → layout → reflection verify → 4-file emit →
incremental build cache (B4a hash + mtime, §11.5).

Invocation:
    python3 main.py --input <blueprint_path> --output <header_dir> [--cache-file <state.json>] [--force]

`<blueprint_path>` may be a single .glsl file or a directory traversed
recursively for *.glsl. With zero inputs the script logs "0 inputs" and
exits 0 (= empty-input contract, entry handoff §3 PA-2 row).
"""

from __future__ import annotations

import argparse
import logging
import os
import sys
import time
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

import build_cache
import glslang_preproc
import perfect_hash
import spirv_reflect
import std140
from build_cache import EnvVersions
from codegen_error import CodegenError, format_error
from glsl_parser import Member, UboBlockDecl, parse_glsl
from perfect_hash import BlockSpec
from std140 import BlockLayout, MemberSpec, compute_layout

LOG_PREFIX = "[codegen_ubo]"
TOOL_NAME = "ubo_codegen"
TOOL_VERSION = "0.1.0-PA6"

EXIT_OK = 0
EXIT_INTERNAL_ERROR = 1
EXIT_INVALID_ARGS = 2
EXIT_INPUT_NOT_FOUND = 3
EXIT_OUTPUT_WRITE_ERROR = 4
EXIT_CODEGEN_ERROR = 5

PYTHON_MIN = (3, 8)

# chapter 02 §2.1 — block-name prefix → CadenceTag (08 §6.1)
CADENCE_PER_FRAME = 0
CADENCE_PER_PROGRAM = 1
CADENCE_PER_DRAW = 2
CADENCE_PER_ASSET = 3
CADENCE_PER_SKIN = 4
CADENCE_SINGLETON = 5

_PREFIX_TO_CADENCE = (
    ("Frame", CADENCE_PER_FRAME),
    ("Program_", CADENCE_PER_PROGRAM),
    ("Draw_", CADENCE_PER_DRAW),
    ("Asset_", CADENCE_PER_ASSET),
    ("Skin_", CADENCE_PER_SKIN),
    ("Global_", CADENCE_SINGLETON),
    ("PerDrawUBO_", CADENCE_PER_DRAW),
    ("PerProgramUBO_", CADENCE_PER_PROGRAM),
)

# AYAstorm legacy UBO naming (= <Name>UBO_Legacy / <Name>UBO_Legacy_*)
# treated as per-program (= MaterialUBO_Legacy etc are bound once per shader use)
_SUFFIX_TO_CADENCE = (
    ("UBO_Legacy", CADENCE_PER_PROGRAM),
)

# chapter 08 §5.4.1.1 — filename stage suffix lookup for SPIR-V reflection
_STAGE_SUFFIX_MAP = {
    "V.glsl": "vert", "F.glsl": "frag", "G.glsl": "geom",
    "TC.glsl": "tesc", "TE.glsl": "tese", "C.glsl": "comp",
}
_DEFAULT_REFLECT_STAGE = "vert"


def _configure_logging(verbose: bool) -> logging.Logger:
    level = logging.DEBUG if verbose else logging.INFO
    handler = logging.StreamHandler(stream=sys.stderr)
    handler.setFormatter(logging.Formatter(f"{LOG_PREFIX} %(levelname)s: %(message)s"))
    root = logging.getLogger(TOOL_NAME)
    root.setLevel(level)
    root.handlers.clear()
    root.addHandler(handler)
    root.propagate = False
    return root


def _check_python_version(log: logging.Logger) -> None:
    if sys.version_info < PYTHON_MIN:
        log.error(
            "Python %d.%d+ required (= chapter 04 §10 (B1) / chapter 08 §3.2), got %d.%d.%d",
            PYTHON_MIN[0], PYTHON_MIN[1],
            sys.version_info.major, sys.version_info.minor, sys.version_info.micro,
        )
        sys.exit(EXIT_INVALID_ARGS)


def _build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="ubo_codegen",
        description="AYAstorm r41 UBO Codegen (Phase 1.A PA-6)",
    )
    p.add_argument("--input", required=True, type=Path, nargs='+', metavar="<shader_path>",
                   help="UBO source: one or more directories (recursive *.glsl) or single .glsl files "
                        "(= Phase 2.α α-2 2026-06-06、複数 shader source dir 対応 = class*/ + cinematic_bd/ 入力想定)")
    p.add_argument("--output", required=True, type=Path, metavar="<header_dir>",
                   help="output directory for generated .inl headers (created if missing)")
    p.add_argument("--cache-file", type=Path, metavar="<state.json>", default=None,
                   help="incremental cache state file (default: <output>/codegen_state.json)")
    p.add_argument("--project-root", type=Path, metavar="<dir>", default=None,
                   help="project root for cache key normalisation (default: repo root auto-detect)")
    p.add_argument("--force", action="store_true",
                   help="bypass incremental cache and regenerate every output file")
    p.add_argument("--glslang-bin", type=Path, default=None,
                   help="explicit glslangValidator path (default: PATH lookup)")
    p.add_argument("--spirv-cross-bin", type=Path, default=None,
                   help="explicit spirv-cross path (default: PATH lookup)")
    p.add_argument("--verbose", "-v", action="store_true", help="enable DEBUG-level log output")
    p.add_argument("--version", action="version", version=f"{TOOL_NAME} {TOOL_VERSION}")
    return p


def _discover_inputs(input_paths: Sequence[Path], log: logging.Logger) -> List[Path]:
    """Discover .glsl files across one or more input paths (= Phase 2.α α-2 多入力対応)。"""
    all_files: List[Path] = []
    for input_path in input_paths:
        if not input_path.exists():
            log.error("input path does not exist: %s", input_path)
            sys.exit(EXIT_INPUT_NOT_FOUND)
        if input_path.is_file():
            if input_path.suffix != ".glsl":
                log.error("input file is not .glsl: %s", input_path)
                sys.exit(EXIT_INVALID_ARGS)
            all_files.append(input_path)
        elif input_path.is_dir():
            all_files.extend(input_path.rglob("*.glsl"))
        else:
            log.error("input path is neither file nor directory: %s", input_path)
            sys.exit(EXIT_INPUT_NOT_FOUND)
    return sorted(set(all_files))  # dedupe + deterministic order


def _ensure_output_dir(output_dir: Path, log: logging.Logger) -> None:
    try:
        output_dir.mkdir(parents=True, exist_ok=True)
    except OSError as exc:
        log.error("cannot create output dir %s: %s", output_dir, exc)
        sys.exit(EXIT_OUTPUT_WRITE_ERROR)


# --- routing helpers --------------------------------------------------------

def _derive_cadence(block_name: str) -> int:
    for prefix, cadence in _PREFIX_TO_CADENCE:
        if block_name.startswith(prefix):
            return cadence
    for suffix, cadence in _SUFFIX_TO_CADENCE:
        if block_name.endswith(suffix):
            return cadence
    return CADENCE_PER_PROGRAM  # safe default until PA-8 wires routing fully


def _detect_stage(glsl_path: Path) -> str:
    name = glsl_path.name
    for suffix, stage in _STAGE_SUFFIX_MAP.items():
        if name.endswith(suffix):
            return stage
    return _DEFAULT_REFLECT_STAGE


def _detect_project_root(script_path: Path) -> Path:
    # scripts/ubo_codegen/main.py → repo root = main.py.parent.parent.parent
    return script_path.resolve().parent.parent.parent


# --- conversion: parser → std140 -------------------------------------------

def _member_to_spec(m: Member) -> MemberSpec:
    nested: Optional[List[MemberSpec]] = None
    if m.nested_struct is not None:
        nested = [_member_to_spec(sub) for sub in m.nested_struct.members]
    return MemberSpec(
        name=m.name,
        type_str=m.type_str,
        array_count=m.array_count,
        nested_members=nested,
        glsl_file=m.source_file,
        glsl_line=m.source_line,
    )


_V3A_PROGRAM_SET_A_BINDINGS = 40  # design 06c §2.3 — subset=1a binding range [0, 40)


def _derive_subset(descriptor_set: int, binding: int) -> int:
    # PC-7α' — design 06c §2.3: set=1 splits into subset=1a (binding<40) / subset=1b (binding>=40).
    # Other sets always carry subset=0.
    if descriptor_set == 1 and binding >= _V3A_PROGRAM_SET_A_BINDINGS:
        return 1
    return 0


def _ubo_to_block_spec(ubo: UboBlockDecl, layout: BlockLayout) -> BlockSpec:
    descriptor_set = int(ubo.layout_qual.get("set", 0))
    binding = int(ubo.layout_qual.get("binding", 0))
    return BlockSpec(
        name=ubo.block_name,
        layout=layout,
        cadence_tag=_derive_cadence(ubo.block_name),
        descriptor_set=descriptor_set,
        binding=binding,
        subset=_derive_subset(descriptor_set, binding),
    )


# Phase 2.α α-2 (= 2026-06-06): 同名 UBO 複数 file 整合 verify
# 設計 doc 08:72-74/96 想定 = codegen 入力 = class*/ + cinematic_bd/ 配下
# 同 UBO が複数 GLSL で再宣言されている (= inventory:247-249 既認識):
#   CloudsVParamUBO_Legacy (cloudsV.glsl + cloudsF.glsl)
#   WaterVParamUBO_Legacy (waterV.glsl + waterF.glsl)
#   ShadowUtilParamUBO_Legacy (class1 + cinematic_bd 上書き path)
# 全 declaration が同 set/binding/layout/member であることを構造的に verify、
# 不一致は SPIR-V binary ↔ host C++ pipeline layout mismatch の原因ゆえ即 fail。

def _verify_block_match(
    spec: BlockSpec, spec_path: str,
    other: BlockSpec, other_path: str,
) -> None:
    """Verify two declarations of same-named UBO are structurally identical.

    cinematic_bd/ 上書き path は同名 + 同 binding + 同 layout = legitimate dual
    declaration として PASS、不一致は CodegenError abort。
    """
    if spec.descriptor_set != other.descriptor_set or spec.binding != other.binding:
        raise CodegenError(format_error(
            f"UBO '{spec.name}' has mismatched set/binding across files",
            extra_lines=[
                f"first:  set={spec.descriptor_set}, binding={spec.binding} at {spec_path}",
                f"second: set={other.descriptor_set}, binding={other.binding} at {other_path}",
            ],
            block=spec.name,
            reason="multi-file UBO declarations must share identical set/binding (= SPIR-V ↔ pipeline layout integrity)",
            action="align layout(set=N, binding=M, std140) across all declaration sites",
        ))
    if spec.subset != other.subset or spec.cadence_tag != other.cadence_tag:
        raise CodegenError(format_error(
            f"UBO '{spec.name}' has mismatched subset/cadence across files",
            extra_lines=[
                f"first:  subset={spec.subset}, cadence={spec.cadence_tag} at {spec_path}",
                f"second: subset={other.subset}, cadence={other.cadence_tag} at {other_path}",
            ],
            block=spec.name,
            reason="subset/cadence are derived deterministically — divergence implies divergent set/binding (= internal contradiction)",
        ))
    spec_members = spec.layout.members
    other_members = other.layout.members
    if len(spec_members) != len(other_members):
        raise CodegenError(format_error(
            f"UBO '{spec.name}' has mismatched member count across files",
            extra_lines=[
                f"first:  {len(spec_members)} members at {spec_path}",
                f"second: {len(other_members)} members at {other_path}",
            ],
            block=spec.name,
            reason="multi-file UBO declarations must share identical std140 layout",
            action="align block body (members + types + order) across all declaration sites",
        ))
    for i, (sm, om) in enumerate(zip(spec_members, other_members)):
        if sm.name != om.name or sm.offset != om.offset or sm.size != om.size or sm.align != om.align:
            raise CodegenError(format_error(
                f"UBO '{spec.name}' member #{i} mismatch across files",
                extra_lines=[
                    f"first:  {sm.name} offset={sm.offset} size={sm.size} align={sm.align} at {spec_path}",
                    f"second: {om.name} offset={om.offset} size={om.size} align={om.align} at {other_path}",
                ],
                block=spec.name,
                member=sm.name,
                reason="multi-file UBO member layout must be identical",
            ))


# --- pipeline: parse + layout + reflection verify --------------------------

def _process_glsl_file(
    glsl_path: Path,
    log: logging.Logger,
    glslang_bin: Optional[Path],
    spirv_cross_bin: Optional[Path],
    skip_spirv: bool,
) -> List[BlockSpec]:
    stage = _detect_stage(glsl_path)
    log.debug("preprocess: %s (stage=%s)", glsl_path, stage)
    source = glslang_preproc.preprocess(
        glsl_path, glslang_bin=glslang_bin, extra_args=("-S", stage),
    )
    parse_result = parse_glsl(source, source_file_hint=str(glsl_path))
    if not parse_result.ubo_blocks:
        return []

    reflection: Optional[Dict[str, dict]] = None
    if not skip_spirv:
        log.debug("spirv-cross reflect: %s (stage=%s)", glsl_path, stage)
        reflection = spirv_reflect.extract_reflection(
            glsl_path, stage,
            glslang_bin=glslang_bin,
            spirv_cross_bin=spirv_cross_bin,
        )

    blocks: List[BlockSpec] = []
    for ubo in parse_result.ubo_blocks:
        members = [_member_to_spec(m) for m in ubo.members]
        layout = compute_layout(ubo.block_name, members)
        if reflection is not None:
            spirv_reflect.verify_layout_against_spirv(
                layout, reflection, glsl_file=str(glsl_path),
            )
        blocks.append(_ubo_to_block_spec(ubo, layout))
    return blocks


# --- file write ------------------------------------------------------------

def _write_file(path: Path, content: str, log: logging.Logger) -> None:
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
    except OSError as exc:
        log.error("cannot write %s: %s", path, exc)
        raise


def _emit_all(
    blocks: Sequence[BlockSpec],
    output_dir: Path,
    log: logging.Logger,
) -> List[str]:
    """Emit the Phase 1.A set (= 04 §5.1 + 08 §8.2 + 08 §10.1).

    Returns the list of file names written. Order = per-block layouts,
    metadata, perfect_hash, index, dummy_init, host_loader.
    """
    written: List[str] = []
    for block in blocks:
        name = build_cache.layout_filename(block.name)
        _write_file(output_dir / name, perfect_hash.emit_layout_inl(block), log)
        written.append(name)
    _write_file(output_dir / build_cache.OUTPUT_METADATA,
                perfect_hash.emit_metadata_inl(blocks), log)
    written.append(build_cache.OUTPUT_METADATA)
    _write_file(output_dir / build_cache.OUTPUT_PERFECT_HASH,
                perfect_hash.emit_perfect_hash_inl_split(blocks), log)
    written.append(build_cache.OUTPUT_PERFECT_HASH)
    _write_file(output_dir / build_cache.OUTPUT_DUMMY_INIT,
                perfect_hash.emit_dummy_init_inl(blocks), log)
    written.append(build_cache.OUTPUT_DUMMY_INIT)
    _write_file(output_dir / build_cache.OUTPUT_HOST_LOADER,
                perfect_hash.emit_host_loader_inl(blocks), log)
    written.append(build_cache.OUTPUT_HOST_LOADER)
    _write_file(output_dir / build_cache.OUTPUT_INDEX,
                perfect_hash.emit_index_inl(blocks), log)
    written.append(build_cache.OUTPUT_INDEX)
    return written


# --- env block construction ------------------------------------------------

def _collect_env(
    glslang_bin: Optional[Path],
    spirv_cross_bin: Optional[Path],
    skip_spirv: bool,
    log: logging.Logger,
) -> EnvVersions:
    glslang_ver = glslang_preproc.version(glslang_bin=glslang_bin)
    if skip_spirv:
        spirv_cross_ver = "skipped"
    else:
        spirv_cross_ver = spirv_reflect.version(spirv_cross_bin=spirv_cross_bin)
    log.debug("env: glslang=%s spirv-cross=%s python=%s platform=%s",
              glslang_ver, spirv_cross_ver,
              build_cache.get_python_version(), build_cache.get_host_platform())
    return EnvVersions(glslang_version=glslang_ver, spirv_cross_version=spirv_cross_ver)


# --- main pipeline ---------------------------------------------------------

def run(args: argparse.Namespace, log: logging.Logger) -> int:
    log.info("tool=%s version=%s python=%d.%d.%d",
             TOOL_NAME, TOOL_VERSION,
             sys.version_info.major, sys.version_info.minor, sys.version_info.micro)
    log.info("input=%s output=%s", args.input, args.output)

    inputs = _discover_inputs(args.input, log)
    _ensure_output_dir(args.output, log)

    if not inputs:
        log.info("0 .glsl inputs discovered, nothing to generate (empty-input contract)")
        return EXIT_OK

    log.info("%d .glsl input(s) discovered", len(inputs))
    for f in inputs:
        log.debug("  input: %s", f)

    script_path = Path(__file__).resolve()
    module_paths = sorted(p for p in script_path.parent.glob("*.py")
                          if p.name != "__init__.py")
    project_root = args.project_root.resolve() if args.project_root else _detect_project_root(script_path)
    cache_file = args.cache_file if args.cache_file else (args.output / "codegen_state.json")
    skip_spirv = spirv_reflect.should_skip_spirv_check()
    if skip_spirv:
        log.warning("AYA_CODEGEN_SKIP_SPIRV_CHECK=1 — SPIR-V reflection verify disabled")

    try:
        env = _collect_env(args.glslang_bin, args.spirv_cross_bin, skip_spirv, log)
    except CodegenError as exc:
        log.error("toolchain probe failed: %s", exc)
        return EXIT_CODEGEN_ERROR

    if not args.force:
        decision = build_cache.check_cache(
            cache_file, inputs, args.output, project_root,
            script_path, module_paths, env,
        )
        if decision.hit:
            log.info("cache hit (%s) — touching outputs only", decision.reason)
            state = build_cache.load_state(cache_file) or {}
            output_names = list((state.get("output_files") or {}).keys())
            build_cache.touch_outputs(args.output, output_names)
            return EXIT_OK
        log.info("cache miss: %s", decision.reason)
    else:
        log.info("--force given, bypassing incremental cache")

    t_start = time.monotonic()
    # Phase 2.α α-2 (= 2026-06-06): 同名 UBO 複数 file 整合 verify + dedupe
    # design 08:72-74/96 想定 = codegen 入力 class*/ + cinematic_bd/ 配下、
    # 同名 UBO 複数 declaration (= inventory:247-249 既認識 = CloudsV/WaterV/ShadowUtil)
    # を構造的に verify、不一致時 CodegenError abort、verify PASS 時 1 件のみ集約。
    blocks_by_name: Dict[str, List[Tuple[BlockSpec, str]]] = {}
    try:
        for glsl_path in inputs:
            file_blocks = _process_glsl_file(
                glsl_path, log, args.glslang_bin, args.spirv_cross_bin, skip_spirv,
            )
            for block in file_blocks:
                blocks_by_name.setdefault(block.name, []).append((block, str(glsl_path)))
    except CodegenError as exc:
        log.error("codegen failed: %s", exc)
        return EXIT_CODEGEN_ERROR

    blocks: List[BlockSpec] = []
    try:
        for name, entries in blocks_by_name.items():
            spec_block, spec_path = entries[0]
            for other_block, other_path in entries[1:]:
                _verify_block_match(spec_block, spec_path, other_block, other_path)
            if len(entries) > 1:
                log.debug("multi-file UBO '%s' verified identical across %d files", name, len(entries))
            blocks.append(spec_block)
    except CodegenError as exc:
        log.error("multi-file UBO integrity check failed: %s", exc)
        return EXIT_CODEGEN_ERROR

    if not blocks:
        log.info("inputs yielded 0 UBO blocks (parser found no `layout(std140) uniform` decls)")
        # Still write an empty cache so a future input change re-runs codegen.

    try:
        written = _emit_all(blocks, args.output, log)
    except OSError:
        return EXIT_OUTPUT_WRITE_ERROR
    except CodegenError as exc:
        log.error("emit failed: %s", exc)
        return EXIT_CODEGEN_ERROR

    duration_ms = int((time.monotonic() - t_start) * 1000)
    total_members = sum(len(b.layout.members) for b in blocks)
    log.info("emitted %d file(s) for %d block(s) / %d member(s) in %d ms",
             len(written), len(blocks), total_members, duration_ms)

    try:
        state = build_cache.build_state(
            inputs, args.output, written, project_root,
            script_path, module_paths, env,
            total_ubos=len(blocks),
            total_uniforms=total_members,
            build_duration_ms=duration_ms,
        )
        build_cache.write_state(cache_file, state)
    except CodegenError as exc:
        log.error("cache write failed: %s", exc)
        return EXIT_CODEGEN_ERROR
    log.info("cache written: %s", cache_file)
    return EXIT_OK


def main(argv: List[str]) -> int:
    parser = _build_arg_parser()
    args = parser.parse_args(argv)
    log = _configure_logging(args.verbose)
    _check_python_version(log)

    try:
        return run(args, log)
    except KeyboardInterrupt:
        log.error("interrupted")
        return EXIT_INTERNAL_ERROR
    except SystemExit:
        raise
    except Exception as exc:  # noqa: BLE001 - top-level guard
        log.exception("internal error: %s", exc)
        return EXIT_INTERNAL_ERROR


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
