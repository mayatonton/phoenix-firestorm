#!/usr/bin/env python3
# AYAstorm r41 UBO Codegen entry point (Phase 1.A PA-2 base)
# Spec source of truth: docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md
"""AYAstorm r41 UBO Codegen entry point.

PA-2 = base structure only: arg parse + I/O contract + log + exit codes.
Parser / std140 calculator / perfect hash / cache / CMake glue come in
PA-3 ... PA-7. PA-2 must run end-to-end on empty input and exit 0.

Invocation:
    python3 main.py --input <blueprint_path> --output <header_dir>

`<blueprint_path>` is a directory (or a single file) of GLSL UBO blueprint
sources. With no `.glsl` files discovered the script logs "0 inputs" and
exits 0 (= empty-input contract, entry handoff §3 PA-2 row).
"""

from __future__ import annotations

import argparse
import logging
import sys
from pathlib import Path
from typing import List

LOG_PREFIX = "[codegen_ubo]"
TOOL_NAME = "ubo_codegen"
TOOL_VERSION = "0.1.0-PA2"

EXIT_OK = 0
EXIT_INTERNAL_ERROR = 1
EXIT_INVALID_ARGS = 2
EXIT_INPUT_NOT_FOUND = 3
EXIT_OUTPUT_WRITE_ERROR = 4

PYTHON_MIN = (3, 8)


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
        description="AYAstorm r41 UBO Codegen (Phase 1.A base = PA-2)",
    )
    p.add_argument(
        "--input",
        required=True,
        type=Path,
        metavar="<blueprint_path>",
        help="UBO blueprint source: directory (recursive *.glsl) or single .glsl file",
    )
    p.add_argument(
        "--output",
        required=True,
        type=Path,
        metavar="<header_dir>",
        help="output directory for generated .inl headers (created if missing)",
    )
    p.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="enable DEBUG-level log output",
    )
    p.add_argument(
        "--version",
        action="version",
        version=f"{TOOL_NAME} {TOOL_VERSION}",
    )
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
        files = sorted(input_path.rglob("*.glsl"))
        return files

    log.error("input path is neither file nor directory: %s", input_path)
    sys.exit(EXIT_INPUT_NOT_FOUND)


def _ensure_output_dir(output_dir: Path, log: logging.Logger) -> None:
    try:
        output_dir.mkdir(parents=True, exist_ok=True)
    except OSError as exc:
        log.error("cannot create output dir %s: %s", output_dir, exc)
        sys.exit(EXIT_OUTPUT_WRITE_ERROR)


def run(input_path: Path, output_dir: Path, log: logging.Logger) -> int:
    log.info("tool=%s version=%s python=%d.%d.%d",
             TOOL_NAME, TOOL_VERSION,
             sys.version_info.major, sys.version_info.minor, sys.version_info.micro)
    log.info("input=%s output=%s", input_path, output_dir)

    inputs = _discover_inputs(input_path, log)
    _ensure_output_dir(output_dir, log)

    if not inputs:
        log.info("0 .glsl inputs discovered, nothing to generate (empty-input contract)")
        return EXIT_OK

    log.info("%d .glsl input(s) discovered", len(inputs))
    for f in inputs:
        log.debug("  input: %s", f)

    # PA-3 ... PA-7 will fill in: glslang -E + mini-parser, std140 calculator,
    # SPIR-V reflection, perfect hash CHD, incremental cache, CMake glue.
    log.info("parser / calculator / perfect-hash / cache not yet wired (PA-3..PA-7)")
    log.info("PA-2 base run complete (= no .inl emitted in PA-2 scope)")
    return EXIT_OK


def main(argv: List[str]) -> int:
    parser = _build_arg_parser()
    args = parser.parse_args(argv)
    log = _configure_logging(args.verbose)
    _check_python_version(log)

    try:
        return run(args.input, args.output, log)
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
