# AYAstorm r41 UBO Codegen — SPIR-V reflection 二重保証 (A1a)
# Spec: docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md §5.4 / §5.4.1.*
"""SPIR-V reflection extractor + per-member verifier.

Pipeline (§5.4.1.1):
    glslangValidator -V -S <stage> <glsl> -o /tmp/<block>.spv
        → SPIR-V binary
    spirv-cross --reflect --output-format json <spv>
        → JSON reflection
    extract_reflection()
        → dict[block_name][member_name] = {offset, array_stride}
    verify_layout_against_spirv()
        → raises CodegenError on offset / size / array_stride mismatch
          (= chapter 08 §9.4 format)

`extract_reflection` is the *sole* JSON-schema abstraction boundary
(§5.4.1.5). Callers receive a normalised dict and stay format-drift-free.

Environment escape (§5.4.1.6):
    AYA_CODEGEN_SKIP_SPIRV_CHECK=1 ⇒ verify is replaced by a WARN log
    (caller is expected to honour this — see should_skip_spirv_check()).
"""

from __future__ import annotations

import json
import logging
import os
import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import Any, Dict, Optional, Sequence

from codegen_error import CodegenError, LOG_PREFIX, format_error
from std140 import BlockLayout, MemberLayout


SKIP_ENV_VAR = "AYA_CODEGEN_SKIP_SPIRV_CHECK"
DEFAULT_GLSLANG = ("glslangValidator", "glslang")
DEFAULT_SPIRV_CROSS = ("spirv-cross",)
SUPPORTED_STAGES = frozenset({"vert", "frag", "geom", "tesc", "tese", "comp"})

_log = logging.getLogger("ubo_codegen.spirv_reflect")


# --- env / tool discovery ---------------------------------------------------

def should_skip_spirv_check() -> bool:
    return os.environ.get(SKIP_ENV_VAR) == "1"


def _which(candidates: Sequence[str], kind: str, explicit: Optional[str]) -> Path:
    if explicit:
        p = Path(explicit)
        if not p.is_file():
            raise CodegenError(format_error(
                f"{kind} binary not found at '{explicit}'",
                action=f"omit override or supply a valid {kind} path",
            ))
        return p
    for name in candidates:
        found = shutil.which(name)
        if found:
            return Path(found)
    raise CodegenError(format_error(
        f"{kind} not found in PATH",
        action=f"install {kind} or pass an explicit path",
    ))


# --- extraction (the format-abstraction boundary) ---------------------------

def extract_reflection(
    glsl_file: Path,
    stage: str,
    glslang_bin: Optional[Path] = None,
    spirv_cross_bin: Optional[Path] = None,
    extra_glslang_args: Sequence[str] = (),
) -> Dict[str, Any]:
    """Run glslang -V + spirv-cross --reflect, return normalised dict.

    Result schema (the *only* shape callers see):
        {
          "<BlockName>": {
            "_block_size": int | None,
            "_set": int | None,
            "_binding": int | None,
            "<member>": {"offset": int, "array_stride": int},
            ...
          },
          ...
        }
    """
    if stage not in SUPPORTED_STAGES:
        raise CodegenError(format_error(
            f"unsupported glslang stage '{stage}'",
            action=f"use one of {sorted(SUPPORTED_STAGES)}",
        ))
    glslang = glslang_bin if glslang_bin is not None else _which(DEFAULT_GLSLANG, "glslang", None)
    spirv_cross = spirv_cross_bin if spirv_cross_bin is not None else _which(DEFAULT_SPIRV_CROSS, "spirv-cross", None)
    if not glsl_file.is_file():
        raise CodegenError(format_error(
            f"GLSL input not found: {glsl_file}",
            action="check codegen --input path",
        ))

    with tempfile.NamedTemporaryFile(suffix=".spv", delete=False) as tmp:
        spv_path = Path(tmp.name)
    try:
        glslang_cmd = [str(glslang), "-V", "-S", stage, str(glsl_file), "-o", str(spv_path), *extra_glslang_args]
        r = subprocess.run(glslang_cmd, capture_output=True, text=True, check=False)
        if r.returncode != 0:
            raise CodegenError(format_error(
                f"glslang SPIR-V compile failed for {glsl_file}",
                glsl_file=str(glsl_file),
                reason=(r.stderr.strip() or f"exit code {r.returncode}"),
            ))
        cross_cmd = [str(spirv_cross), "--reflect", "--output-format", "json", str(spv_path)]
        r = subprocess.run(cross_cmd, capture_output=True, text=True, check=False)
        if r.returncode != 0:
            raise CodegenError(format_error(
                f"spirv-cross reflection failed for {spv_path}",
                reason=(r.stderr.strip() or f"exit code {r.returncode}"),
            ))
        try:
            refl = json.loads(r.stdout)
        except json.JSONDecodeError as exc:
            raise CodegenError(format_error(
                "spirv-cross JSON parse failed",
                reason=str(exc),
                action="check spirv-cross version drift (§5.4.1.5)",
            )) from exc
    finally:
        try:
            spv_path.unlink()
        except OSError:
            pass

    return _normalise_reflection(refl)


def _normalise_reflection(refl: Dict[str, Any]) -> Dict[str, Any]:
    """§5.4.1.5 — single point of contact with spirv-cross JSON schema."""
    if not isinstance(refl, dict):
        raise CodegenError(format_error(
            "spirv-cross reflection root is not an object",
            action="check spirv-cross version / output format (§5.4.1.5)",
        ))
    types = refl.get("types", {})
    if types is None:
        types = {}
    if not isinstance(types, dict):
        raise CodegenError(format_error(
            "spirv-cross reflection 'types' must be an object",
            action="check spirv-cross output schema (§5.4.1.5)",
        ))
    out: Dict[str, Any] = {}
    for ubo in refl.get("ubos", []) or []:
        if not isinstance(ubo, dict):
            continue
        block_name = ubo.get("name")
        type_key = ubo.get("type")
        if not block_name or not type_key:
            raise CodegenError(format_error(
                "spirv-cross UBO entry missing 'name' or 'type'",
                action="check spirv-cross version drift (§5.4.1.5)",
            ))
        type_def = types.get(type_key)
        if type_def is None:
            raise CodegenError(format_error(
                f"spirv-cross UBO '{block_name}' references unknown type '{type_key}'",
                action="check spirv-cross output schema (§5.4.1.5)",
            ))
        record: Dict[str, Any] = {
            "_block_size": ubo.get("block_size"),
            "_set": ubo.get("set"),
            "_binding": ubo.get("binding"),
        }
        for m in type_def.get("members", []) or []:
            mname = m.get("name")
            if not mname:
                continue
            record[mname] = {
                "offset": m.get("offset"),
                "array_stride": m.get("array_stride", 0),
            }
        out[block_name] = record
    return out


# --- verifier ---------------------------------------------------------------

def verify_layout_against_spirv(
    layout: BlockLayout,
    spirv_refl: Dict[str, Any],
    *,
    glsl_file: Optional[str] = None,
) -> None:
    """Per-member offset / block size / array stride comparison (§5.4.1.3).

    Raises CodegenError on any mismatch. Returns None on success.
    """
    if should_skip_spirv_check():
        msg = (
            f"{LOG_PREFIX} WARN: SPIR-V reflection check skipped (= {SKIP_ENV_VAR}=1) "
            f"for block '{layout.name}'"
        )
        _log.warning(msg)
        return

    block_name = layout.name
    if block_name not in spirv_refl:
        raise CodegenError(format_error(
            f"block '{block_name}' missing from SPIR-V reflection",
            glsl_file=glsl_file, block=block_name,
            reason="Codegen parsed it; reflection did not — likely preprocess / name drift",
            action="verify `layout(std140) uniform <BlockName> { ... };` decl",
        ))
    spv = spirv_refl[block_name]

    expected_size = spv.get("_block_size")
    if expected_size is not None and expected_size != layout.std140_size:
        raise CodegenError(format_error(
            f"block size mismatch in '{block_name}'",
            glsl_file=glsl_file, block=block_name,
            extra_lines=[
                f"Codegen calculation:       {layout.std140_size}",
                f"glslang SPIR-V reflection: {expected_size}",
            ],
            reason="trailing padding or member size calculation diverged",
        ))

    for member in layout.members:
        if member.name not in spv:
            raise CodegenError(format_error(
                f"member '{block_name}.{member.name}' missing from SPIR-V reflection",
                glsl_file=glsl_file, block=block_name, member=member.name,
            ))
        spv_m = spv[member.name]
        spv_offset = spv_m.get("offset")
        if spv_offset is None:
            raise CodegenError(format_error(
                f"spirv-cross member '{block_name}.{member.name}' lacks offset field",
                glsl_file=glsl_file, block=block_name, member=member.name,
                action="check spirv-cross version drift (§5.4.1.5)",
            ))
        if spv_offset != member.offset:
            raise CodegenError(format_error(
                f"std140 offset mismatch in '{block_name}.{member.name}'",
                glsl_file=glsl_file, block=block_name, member=member.name,
                extra_lines=[
                    f"Codegen calculation:       OFFSET = {member.offset}",
                    f"glslang SPIR-V reflection: OFFSET = {spv_offset}",
                    f"Diff = {abs(spv_offset - member.offset)} bytes",
                    "Likely cause: array stride / vec3 hole / nested struct padding",
                ],
                action="chapter 04 §4.3.1 algorithm review or glslang version drift 確認",
            ))
        if member.array_stride > 0:
            spv_stride = spv_m.get("array_stride", 0) or 0
            if spv_stride and spv_stride != member.array_stride:
                raise CodegenError(format_error(
                    f"array stride mismatch in '{block_name}.{member.name}'",
                    glsl_file=glsl_file, block=block_name, member=member.name,
                    extra_lines=[
                        f"Codegen: stride = {member.array_stride}",
                        f"glslang: stride = {spv_stride}",
                    ],
                    action="chapter 04 §4.3.1.4 arrayify() review",
                ))


def version(spirv_cross_bin: Optional[Path] = None) -> str:
    """spirv-cross version string for cache-key material (§11.5.1)."""
    binary = spirv_cross_bin if spirv_cross_bin is not None else _which(DEFAULT_SPIRV_CROSS, "spirv-cross", None)
    r = subprocess.run([str(binary), "--version"], capture_output=True, text=True, check=False)
    if r.returncode != 0:
        raise CodegenError(format_error(
            "spirv-cross --version failed",
            reason=r.stderr.strip() or f"exit code {r.returncode}",
        ))
    for line in r.stdout.splitlines():
        line = line.strip()
        if line:
            return line
    return ""
