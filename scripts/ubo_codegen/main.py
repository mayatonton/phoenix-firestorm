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
from codegen_error import CodegenError
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
    p.add_argument("--input", required=True, type=Path, metavar="<blueprint_path>",
                   help="UBO blueprint source: directory (recursive *.glsl) or single .glsl file")
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


def _discover_inputs(input_path: Path, log: logging.Logger) -> List[Path]:
    if not input_path.exists():
        log.error("input path does not exist: %s", input_path)
        sys.exit(EXIT_INPUT_NOT_FOUND)
    if input_path.is_file():
        if input_path.suffix != ".glsl":
            log.error("input file is not .glsl: %s", input_path)
            sys.exit(EXIT_INVALID_ARGS)
        return [input_path]
    if input_path.is_dir():
        return sorted(input_path.rglob("*.glsl"))
    log.error("input path is neither file nor directory: %s", input_path)
    sys.exit(EXIT_INPUT_NOT_FOUND)


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


def _ubo_to_block_spec(ubo: UboBlockDecl, layout: BlockLayout) -> BlockSpec:
    return BlockSpec(
        name=ubo.block_name,
        layout=layout,
        cadence_tag=_derive_cadence(ubo.block_name),
    )


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
    blocks: List[BlockSpec] = []
    try:
        for glsl_path in inputs:
            blocks.extend(_process_glsl_file(
                glsl_path, log, args.glslang_bin, args.spirv_cross_bin, skip_spirv,
            ))
    except CodegenError as exc:
        log.error("codegen failed: %s", exc)
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
