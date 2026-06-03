# AYAstorm r41 UBO Codegen — glslang preprocess wrapper
# Spec: docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md §5.2 / §5.2.1.1 / §5.2.1.7
"""glslangValidator -E wrapper.

Runs `glslangValidator -E <file>` so the mini-parser (glsl_parser.py)
receives `#ifdef` / `#define`-resolved GLSL while keeping `#line N "path"`
directives intact — the parser uses them to map tokens back to the
original GLSL location (§5.2.1.7).
"""

from __future__ import annotations

import shutil
import subprocess
from pathlib import Path
from typing import Optional, Sequence

from codegen_error import CodegenError, format_error

DEFAULT_BINARIES = ("glslangValidator", "glslang")


def find_glslang(explicit: Optional[str] = None) -> Path:
    """Resolve the glslang binary path.

    Order: explicit override → PATH lookup over DEFAULT_BINARIES.
    """
    if explicit:
        p = Path(explicit)
        if not p.is_file():
            raise CodegenError(format_error(
                f"glslang binary not found at explicit path '{explicit}'",
                reason="--glslang-path override does not point to a file",
                action="omit --glslang-path or supply a valid binary path",
            ))
        return p
    for name in DEFAULT_BINARIES:
        found = shutil.which(name)
        if found:
            return Path(found)
    raise CodegenError(format_error(
        "glslangValidator not found in PATH",
        reason="chapter 10 §1.2 (B2) B2b system pkg = apt install glslang-tools",
        action="install glslang-tools (Ubuntu 24.04: 15.1.0-2) or pass --glslang-path",
    ))


def preprocess(
    glsl_path: Path,
    glslang_bin: Optional[Path] = None,
    extra_args: Sequence[str] = (),
) -> str:
    """Return preprocessed GLSL (= -E stdout) as a string.

    `extra_args` is forwarded raw (e.g. `("-DLL_VULKAN_GLSL=1",)`).
    Raises CodegenError on glslang non-zero exit.
    """
    binary = glslang_bin if glslang_bin is not None else find_glslang()
    if not glsl_path.is_file():
        raise CodegenError(format_error(
            f"GLSL input not found: {glsl_path}",
            action="check codegen --input path / GLSL file presence",
        ))
    cmd = [str(binary), "-E", str(glsl_path), *extra_args]
    result = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise CodegenError(format_error(
            f"glslang -E failed for {glsl_path}",
            glsl_file=str(glsl_path),
            reason=(result.stderr.strip() or f"exit code {result.returncode}"),
            action="fix the GLSL source or adjust extra_args (e.g. -D defines)",
        ))
    return result.stdout


def version(glslang_bin: Optional[Path] = None) -> str:
    """Return glslang `--version` first line, used as cache-key material (§11.5.1)."""
    binary = glslang_bin if glslang_bin is not None else find_glslang()
    result = subprocess.run(
        [str(binary), "--version"],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise CodegenError(format_error(
            "glslang --version failed",
            reason=result.stderr.strip() or f"exit code {result.returncode}",
        ))
    for line in result.stdout.splitlines():
        line = line.strip()
        if line:
            return line
    return ""
