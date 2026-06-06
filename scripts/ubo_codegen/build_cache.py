# AYAstorm r41 UBO Codegen — incremental build cache (B4a)
# Spec: docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md
#       §11 / §11.2 / §11.5 / §11.5.1 / §11.5.2 / §11.5.3 / §11.5.7
"""Incremental build cache for the UBO codegen pipeline.

Strategy (= B4a, §11.2): GLSL mtime as a cheap pre-filter; SHA-256 of the
content (CRLF-normalised, §13 接合) confirms or rejects mtime-driven cache
misses so that a `git checkout` does not force a rebuild when content has
not actually changed.

cache state is a single JSON file (= `codegen_state.json`, schema version=1)
shaped per §11.5.1:

    {
      "version": 1,
      "codegen_tool_version": {
        "script_sha256":   "<sha256 of main.py>",
        "script_path":     "scripts/ubo_codegen/main.py",
        "modules_sha256":  {"<basename>": "<sha256>", ...}
      },
      "environment": {
        "python_version":      "3.x.y",
        "glslang_version":     "<glslangValidator --version line 1>",
        "spirv_cross_version": "<spirv-cross --version line 1>",
        "host_platform":       "linux|darwin|win32"
      },
      "input_files": {
        "<posix rel path>": {"mtime": float, "sha256_normalized": "...", "file_size": int}
      },
      "output_files": {
        "<output file name>": {"sha256": "...", "size_bytes": int}
      },
      "build_metadata": {
        "last_build_timestamp_utc": "<ISO 8601 Z>",
        "total_ubos":               int,
        "total_uniforms":           int,
        "build_duration_ms":        int
      }
    }

The public surface is intentionally pure-Python / no subprocess: the caller
(main.py) supplies `env_versions` so tests do not need glslang installed.

§11.5.7 atomic write = temp file + os.replace; safe under make -j parallel
builds even though CMake itself serialises Codegen invocations.
"""

from __future__ import annotations

import hashlib
import json
import os
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Iterable, List, Mapping, Optional, Sequence

from codegen_error import CodegenError, format_error

CACHE_SCHEMA_VERSION = 1

# Canonical output file names (§5.1 / §6 / §5.6.6). Order matters only for
# touch-on-cache-hit determinism.
OUTPUT_PERFECT_HASH = "ubo_perfect_hash.inl"
OUTPUT_METADATA = "ubo_metadata.inl"
OUTPUT_INDEX = "ubo_index.inl"
OUTPUT_DUMMY_INIT = "ubo_dummy_init.inl"
OUTPUT_HOST_LOADER = "ubo_host_loader.inl"
LAYOUT_PREFIX = "ubo_layout_"
LAYOUT_SUFFIX = ".inl"

MTIME_EPSILON_SECONDS = 1.0  # §11.5.2 step 4-2 — filesystem mtime jitter tolerance


# --- hashing ---------------------------------------------------------------

def sha256_file(path: Path) -> str:
    """Raw SHA-256 of a file (for output files / Codegen script self-hash)."""
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def sha256_file_normalized(path: Path) -> str:
    """3-OS identical SHA-256 of a GLSL source.

    CRLF → LF normalisation only — trailing whitespace is preserved because
    GLSL `#line` directives carry semantic info on their own line.
    """
    h = hashlib.sha256()
    with path.open("rb") as f:
        content = f.read()
    content = content.replace(b"\r\n", b"\n")
    h.update(content)
    return h.hexdigest()


# --- path normalisation ----------------------------------------------------

def normalize_path(p: Path, project_root: Path) -> str:
    """Return a project-root-relative POSIX path string (= 3-OS stable key)."""
    try:
        rel = p.resolve().relative_to(project_root.resolve())
    except ValueError:
        rel = p
    return rel.as_posix()


# --- environment ----------------------------------------------------------

def get_python_version() -> str:
    v = sys.version_info
    return f"{v.major}.{v.minor}.{v.micro}"


def get_host_platform() -> str:
    return sys.platform


@dataclass(frozen=True)
class EnvVersions:
    """Cache-key environment block (= §11.5.1 `environment`)."""
    glslang_version: str
    spirv_cross_version: str
    python_version: str = field(default_factory=get_python_version)
    host_platform: str = field(default_factory=get_host_platform)
    # Phase 2.α α-3 improvement 1.5.c (= 2026-06-06): C++ runtime emulation 層 dump file
    # の sha256 hash (= aya_r41_codegen_defines.toml 改訂で cache invalidation 連動)。
    # default = "no-defines" (= --defines-file 未指定時の sentinel)。
    defines_hash: str = "no-defines"

    def to_dict(self) -> Dict[str, str]:
        return {
            "python_version": self.python_version,
            "glslang_version": self.glslang_version,
            "spirv_cross_version": self.spirv_cross_version,
            "host_platform": self.host_platform,
            "defines_hash": self.defines_hash,
        }


# --- tool version block ---------------------------------------------------

def collect_module_hashes(script_paths: Iterable[Path]) -> Dict[str, str]:
    """SHA-256 of every Python source in the codegen tool dir, keyed by basename."""
    return {p.name: sha256_file(p) for p in sorted(script_paths)}


def build_tool_version_block(
    script_path: Path,
    project_root: Path,
    module_paths: Iterable[Path],
) -> Dict[str, object]:
    return {
        "script_sha256": sha256_file(script_path),
        "script_path": normalize_path(script_path, project_root),
        "modules_sha256": collect_module_hashes(module_paths),
    }


# --- I/O ------------------------------------------------------------------

def load_state(state_file: Path) -> Optional[Dict[str, object]]:
    """Read cache JSON. Returns None when missing / corrupt (= fail-soft, §11.5.3)."""
    if not state_file.is_file():
        return None
    try:
        return json.loads(state_file.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError):
        return None


def write_state(state_file: Path, state: Mapping[str, object]) -> None:
    """Atomic write — temp file then os.replace (§11.5.7)."""
    try:
        state_file.parent.mkdir(parents=True, exist_ok=True)
    except OSError as exc:
        raise CodegenError(format_error(
            f"cannot create cache dir {state_file.parent}",
            reason=str(exc),
            action="check filesystem permissions / disk space",
        )) from exc
    tmp = state_file.with_suffix(state_file.suffix + ".tmp")
    tmp.write_text(json.dumps(state, indent=2, sort_keys=True), encoding="utf-8")
    os.replace(tmp, state_file)


# --- cache decision -------------------------------------------------------

@dataclass(frozen=True)
class CacheDecision:
    """Result of check_cache. `hit=True` ⇒ Codegen may be skipped."""
    hit: bool
    reason: str  # human-readable diagnosis for logs / unit tests


def _file_size(path: Path) -> int:
    try:
        return path.stat().st_size
    except OSError:
        return -1


def check_cache(
    state_file: Path,
    input_files: Sequence[Path],
    output_dir: Path,
    project_root: Path,
    script_path: Path,
    module_paths: Sequence[Path],
    env: EnvVersions,
    expected_output_file_names: Optional[Sequence[str]] = None,
) -> CacheDecision:
    """Full §11.5.2 cache-hit / miss decision.

    The decision is a single CacheDecision (= hit + diagnostic reason). The
    reason is exercised by the unit tests and surfaces in the log on miss.
    """
    state = load_state(state_file)
    if state is None:
        return CacheDecision(False, "cache state missing or corrupt")

    # Step 1 — schema version
    if state.get("version") != CACHE_SCHEMA_VERSION:
        return CacheDecision(False, f"schema version mismatch (expected {CACHE_SCHEMA_VERSION})")

    # Step 2 — tool version (= script + every codegen module)
    tool = state.get("codegen_tool_version") or {}
    if not isinstance(tool, dict):
        return CacheDecision(False, "codegen_tool_version block malformed")
    if tool.get("script_sha256") != sha256_file(script_path):
        return CacheDecision(False, "codegen main script sha256 changed")
    expected_modules = tool.get("modules_sha256") or {}
    actual_modules = collect_module_hashes(module_paths)
    if expected_modules != actual_modules:
        return CacheDecision(False, "codegen module sha256 changed")

    # Step 3 — environment (= glslang / spirv-cross / Python / host)
    env_state = state.get("environment") or {}
    if env_state != env.to_dict():
        return CacheDecision(False, "environment (toolchain) version drift")

    # Step 4 — input file set + per-file mtime/hash
    input_state = state.get("input_files") or {}
    if not isinstance(input_state, dict):
        return CacheDecision(False, "input_files block malformed")

    new_paths = {normalize_path(p, project_root) for p in input_files}
    old_paths = set(input_state.keys())
    if new_paths != old_paths:
        return CacheDecision(False, "input file set changed")

    for path in input_files:
        key = normalize_path(path, project_root)
        record = input_state.get(key) or {}
        if not isinstance(record, dict):
            return CacheDecision(False, f"input record malformed for {key}")
        try:
            cur_mtime = os.path.getmtime(path)
        except OSError:
            return CacheDecision(False, f"input file disappeared: {key}")
        record_mtime = record.get("mtime")
        if isinstance(record_mtime, (int, float)) and abs(cur_mtime - record_mtime) < MTIME_EPSILON_SECONDS:
            continue  # cheap path — mtime steady → assume unchanged
        # mtime drifted: fall back to content hash to absorb git-checkout etc.
        cur_hash = sha256_file_normalized(path)
        if cur_hash != record.get("sha256_normalized"):
            return CacheDecision(False, f"input content changed: {key}")
        # mtime drift but content stable = false positive, cache stays hot.

    # Step 5 — output file tamper / disappearance check
    output_state = state.get("output_files") or {}
    if not isinstance(output_state, dict):
        return CacheDecision(False, "output_files block malformed")
    if expected_output_file_names is not None:
        if set(output_state.keys()) != set(expected_output_file_names):
            return CacheDecision(False, "output file set drift")
    for name, rec in output_state.items():
        out_path = output_dir / name
        if not out_path.is_file():
            return CacheDecision(False, f"output missing: {name}")
        if not isinstance(rec, dict):
            return CacheDecision(False, f"output record malformed for {name}")
        if sha256_file(out_path) != rec.get("sha256"):
            return CacheDecision(False, f"output sha256 changed: {name}")

    return CacheDecision(True, "all checks passed")


# --- cache write ----------------------------------------------------------

def _utcnow_iso() -> str:
    return time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())


def build_state(
    input_files: Sequence[Path],
    output_dir: Path,
    output_file_names: Sequence[str],
    project_root: Path,
    script_path: Path,
    module_paths: Sequence[Path],
    env: EnvVersions,
    total_ubos: int,
    total_uniforms: int,
    build_duration_ms: int,
) -> Dict[str, object]:
    """Build the canonical state dict for write_state (= §11.5.1 schema)."""
    input_block: Dict[str, Dict[str, object]] = {}
    for p in input_files:
        key = normalize_path(p, project_root)
        input_block[key] = {
            "mtime": os.path.getmtime(p),
            "sha256_normalized": sha256_file_normalized(p),
            "file_size": _file_size(p),
        }

    output_block: Dict[str, Dict[str, object]] = {}
    for name in output_file_names:
        path = output_dir / name
        if not path.is_file():
            raise CodegenError(format_error(
                f"output file missing during cache write: {name}",
                reason="emit phase did not produce the expected file",
                action="check emit functions in perfect_hash.py and main.py wiring",
            ))
        output_block[name] = {
            "sha256": sha256_file(path),
            "size_bytes": _file_size(path),
        }

    return {
        "version": CACHE_SCHEMA_VERSION,
        "codegen_tool_version": build_tool_version_block(
            script_path, project_root, module_paths,
        ),
        "environment": env.to_dict(),
        "input_files": input_block,
        "output_files": output_block,
        "build_metadata": {
            "last_build_timestamp_utc": _utcnow_iso(),
            "total_ubos": total_ubos,
            "total_uniforms": total_uniforms,
            "build_duration_ms": build_duration_ms,
        },
    }


# --- cache-hit side effects ----------------------------------------------

def touch_outputs(output_dir: Path, output_file_names: Sequence[str], now: Optional[float] = None) -> None:
    """On cache hit, bump output mtime so CMake DEPENDS reports them up-to-date."""
    when = time.time() if now is None else now
    for name in output_file_names:
        path = output_dir / name
        if path.is_file():
            os.utime(path, (when, when))


# --- canonical output file enumeration -----------------------------------

def canonical_output_files(block_names: Sequence[str]) -> List[str]:
    """Phase 1.A emit set: per-block layouts + 5 aggregated files.

    Aggregated set covers 04 §5.1 (= metadata, perfect_hash, index) plus
    08 §8.2 dummy_init and 08 §10.1 host_loader, which together satisfy the
    Phase 1.A Exit Criteria in 09 §4.2 (= 4 file generation + compile-time
    name resolution conflict = 0).
    """
    layouts = [f"{LAYOUT_PREFIX}{name.lower()}{LAYOUT_SUFFIX}" for name in block_names]
    return layouts + [
        OUTPUT_PERFECT_HASH,
        OUTPUT_METADATA,
        OUTPUT_INDEX,
        OUTPUT_DUMMY_INIT,
        OUTPUT_HOST_LOADER,
    ]


def layout_filename(block_name: str) -> str:
    return f"{LAYOUT_PREFIX}{block_name.lower()}{LAYOUT_SUFFIX}"
