# AYAstorm r41 UBO Codegen — unit tests for build_cache.py (PA-6).
# tests/ is repo-wide .gitignore (feedback_tests_dir_never_commit) — local only.
"""Unit tests covering chapter 08 §11.5 cache strategy.

Scenarios mirror §11.5.3 (cache invalidation trigger 完全 enumerate):

    1. fresh build → cache miss "state missing"
    2. immediate re-run with identical inputs/env → hit
    3. schema version mismatch → miss
    4. main-script sha256 changed → miss
    5. codegen module sha256 changed → miss
    6. glslang version drift → miss
    7. spirv-cross version drift → miss
    8. python version drift → miss
    9. host platform drift → miss
   10. input file added → miss
   11. input file removed → miss
   12. input file content changed → miss
   13. input mtime drift + content unchanged → hit (= git checkout absorber)
   14. input file deleted between writes → miss
   15. output file deleted → miss
   16. output file tampered → miss
   17. output file set drift (with expected_output_file_names) → miss
   18. atomic write under write_state
   19. canonical_output_files ordering
"""

from __future__ import annotations

import json
import os
import sys
import tempfile
import time
import unittest
from pathlib import Path

THIS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(THIS_DIR.parent))

import build_cache  # noqa: E402
from build_cache import (  # noqa: E402  # pylint: disable=wrong-import-position
    OUTPUT_DUMMY_INIT, OUTPUT_HOST_LOADER,
    CACHE_SCHEMA_VERSION, CacheDecision, EnvVersions,
    OUTPUT_INDEX, OUTPUT_METADATA, OUTPUT_PERFECT_HASH,
    build_state, canonical_output_files, check_cache, collect_module_hashes,
    layout_filename, load_state, normalize_path, sha256_file,
    sha256_file_normalized, touch_outputs, write_state,
)
from codegen_error import CodegenError  # noqa: E402


# --- shared helpers --------------------------------------------------------

DEFAULT_ENV = EnvVersions(glslang_version="11:15.1.0", spirv_cross_version="1.3.239.0")


def _make_glsl(tmp: Path, name: str, body: str = "void main(){}") -> Path:
    p = tmp / name
    p.write_text(body, encoding="utf-8")
    return p


def _make_output(tmp: Path, name: str, body: str = "// generated\n") -> Path:
    p = tmp / name
    p.write_text(body, encoding="utf-8")
    return p


def _make_script(tmp: Path, name: str, body: str = "# fake codegen module\n") -> Path:
    p = tmp / name
    p.write_text(body, encoding="utf-8")
    return p


def _write_state_then_check(
    state: dict, state_file: Path, inputs, output_dir, project_root,
    script, modules, env=DEFAULT_ENV, expected=None,
) -> CacheDecision:
    write_state(state_file, state)
    return check_cache(
        state_file, inputs, output_dir, project_root,
        script, modules, env, expected_output_file_names=expected,
    )


# --- hashing tests ---------------------------------------------------------

class HashingTests(unittest.TestCase):
    def test_sha256_file_deterministic(self):
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "a.bin"
            p.write_bytes(b"hello world")
            self.assertEqual(sha256_file(p), sha256_file(p))

    def test_sha256_file_normalized_strips_crlf(self):
        with tempfile.TemporaryDirectory() as td:
            crlf = Path(td) / "crlf.glsl"
            crlf.write_bytes(b"line1\r\nline2\r\n")
            lf = Path(td) / "lf.glsl"
            lf.write_bytes(b"line1\nline2\n")
            self.assertEqual(sha256_file_normalized(crlf), sha256_file_normalized(lf))

    def test_sha256_file_normalized_preserves_trailing_whitespace(self):
        with tempfile.TemporaryDirectory() as td:
            with_trail = Path(td) / "trail.glsl"
            with_trail.write_bytes(b"void main(){}   \n")
            no_trail = Path(td) / "no_trail.glsl"
            no_trail.write_bytes(b"void main(){}\n")
            self.assertNotEqual(
                sha256_file_normalized(with_trail),
                sha256_file_normalized(no_trail),
            )


# --- path normalisation ---------------------------------------------------

class NormalizePathTests(unittest.TestCase):
    def test_returns_posix_relative(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            sub = root / "scripts" / "ubo_codegen" / "main.py"
            sub.parent.mkdir(parents=True)
            sub.write_text("")
            self.assertEqual(normalize_path(sub, root), "scripts/ubo_codegen/main.py")

    def test_outside_root_falls_back_to_input(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td) / "rootA"
            root.mkdir()
            outside = Path(td) / "elsewhere.txt"
            outside.write_text("")
            # Should not raise; falls back to the input path's posix form.
            result = normalize_path(outside, root)
            self.assertIn("elsewhere.txt", result)


# --- env / tool-version blocks --------------------------------------------

class EnvBlockTests(unittest.TestCase):
    def test_env_to_dict_round_trip(self):
        env = EnvVersions(glslang_version="x", spirv_cross_version="y",
                          python_version="3.12.3", host_platform="linux")
        d = env.to_dict()
        # Phase 2.α α-3 improvement 1.5.c (= 2026-06-06): defines_hash field 追加、
        # default sentinel "no-defines" (= --defines-file 未指定時) を round-trip 含む。
        self.assertEqual(d, {
            "python_version": "3.12.3",
            "glslang_version": "x",
            "spirv_cross_version": "y",
            "host_platform": "linux",
            "defines_hash": "no-defines",
        })

    def test_env_to_dict_with_explicit_defines_hash(self):
        # --defines-file 指定時 = aya_r41_codegen_defines.toml の sha256 が hash として渡る、
        # to_dict() で round-trip。cache invalidation 連動の前提。
        env = EnvVersions(glslang_version="x", spirv_cross_version="y",
                          python_version="3.12.3", host_platform="linux",
                          defines_hash="abc123def456")
        d = env.to_dict()
        self.assertEqual(d["defines_hash"], "abc123def456")

    def test_collect_module_hashes_keyed_by_basename(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            a = _make_script(tmp, "alpha.py", body="A")
            b = _make_script(tmp, "beta.py", body="B")
            out = collect_module_hashes([a, b])
            self.assertEqual(set(out.keys()), {"alpha.py", "beta.py"})
            self.assertNotEqual(out["alpha.py"], out["beta.py"])


# --- I/O ------------------------------------------------------------------

class StateIOTests(unittest.TestCase):
    def test_load_state_missing_returns_none(self):
        with tempfile.TemporaryDirectory() as td:
            self.assertIsNone(load_state(Path(td) / "no_such.json"))

    def test_load_state_corrupt_returns_none(self):
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "corrupt.json"
            p.write_text("not json {{{", encoding="utf-8")
            self.assertIsNone(load_state(p))

    def test_write_state_atomic_and_sorted(self):
        with tempfile.TemporaryDirectory() as td:
            sf = Path(td) / "state.json"
            payload = {"z": 1, "a": 2, "version": CACHE_SCHEMA_VERSION}
            write_state(sf, payload)
            text = sf.read_text(encoding="utf-8")
            # sort_keys=True ⇒ "a" appears before "z" in serialised form.
            self.assertLess(text.index('"a"'), text.index('"z"'))
            self.assertEqual(load_state(sf), payload)

    def test_write_state_no_tmp_left_behind(self):
        with tempfile.TemporaryDirectory() as td:
            sf = Path(td) / "state.json"
            write_state(sf, {"version": CACHE_SCHEMA_VERSION})
            tmp = sf.with_suffix(sf.suffix + ".tmp")
            self.assertFalse(tmp.exists())


# --- check_cache scenarios -----------------------------------------------

class CheckCacheTests(unittest.TestCase):
    """Each test sets up a tmp dir, populates inputs/outputs/scripts, builds
    one cache state, then mutates one thing and asserts the decision."""

    def setUp(self):
        self._td = tempfile.TemporaryDirectory()
        tmp = Path(self._td.name)
        self.root = tmp
        self.inputs_dir = tmp / "inputs"; self.inputs_dir.mkdir()
        self.outputs_dir = tmp / "outputs"; self.outputs_dir.mkdir()
        self.scripts_dir = tmp / "scripts"; self.scripts_dir.mkdir()
        self.cache_file = tmp / "cache" / "codegen_state.json"
        self.glsl = _make_glsl(self.inputs_dir, "block.glsl", body="layout(std140) uniform F {} ;\n")
        self.script = _make_script(self.scripts_dir, "main.py", body="# main entry\n")
        self.module = _make_script(self.scripts_dir, "perfect_hash.py", body="# mod\n")
        self.modules = [self.script, self.module]
        self.layout_name = layout_filename("F")
        self.out_layout = _make_output(self.outputs_dir, self.layout_name)
        self.out_meta = _make_output(self.outputs_dir, OUTPUT_METADATA)
        self.out_hash = _make_output(self.outputs_dir, OUTPUT_PERFECT_HASH)
        self.out_index = _make_output(self.outputs_dir, OUTPUT_INDEX)
        self.output_names = [self.layout_name, OUTPUT_METADATA, OUTPUT_PERFECT_HASH, OUTPUT_INDEX]
        self.state = build_state(
            [self.glsl], self.outputs_dir, self.output_names, self.root,
            self.script, self.modules, DEFAULT_ENV,
            total_ubos=1, total_uniforms=3, build_duration_ms=42,
        )

    def tearDown(self):
        self._td.cleanup()

    def _check(self, expected_names=None) -> CacheDecision:
        return check_cache(
            self.cache_file, [self.glsl], self.outputs_dir, self.root,
            self.script, self.modules, DEFAULT_ENV,
            expected_output_file_names=expected_names,
        )

    # --- baseline & schema --------------------------------------------------

    def test_01_missing_state_is_miss(self):
        self.assertFalse(self._check().hit)

    def test_02_immediate_rerun_is_hit(self):
        write_state(self.cache_file, self.state)
        d = self._check()
        self.assertTrue(d.hit, msg=d.reason)

    def test_03_schema_version_mismatch(self):
        self.state["version"] = 99
        write_state(self.cache_file, self.state)
        self.assertFalse(self._check().hit)

    # --- tool version block -------------------------------------------------

    def test_04_main_script_sha256_changed(self):
        write_state(self.cache_file, self.state)
        self.script.write_text("# modified\n")
        d = self._check()
        self.assertFalse(d.hit)
        self.assertIn("script", d.reason)

    def test_05_module_sha256_changed(self):
        write_state(self.cache_file, self.state)
        self.module.write_text("# modified module\n")
        d = self._check()
        self.assertFalse(d.hit)
        self.assertIn("module", d.reason)

    # --- environment drift --------------------------------------------------

    def test_06_glslang_version_drift(self):
        write_state(self.cache_file, self.state)
        drifted = EnvVersions(glslang_version="other", spirv_cross_version=DEFAULT_ENV.spirv_cross_version,
                              python_version=DEFAULT_ENV.python_version, host_platform=DEFAULT_ENV.host_platform)
        d = check_cache(self.cache_file, [self.glsl], self.outputs_dir, self.root,
                        self.script, self.modules, drifted)
        self.assertFalse(d.hit)
        self.assertIn("environment", d.reason)

    def test_07_spirv_cross_version_drift(self):
        write_state(self.cache_file, self.state)
        drifted = EnvVersions(glslang_version=DEFAULT_ENV.glslang_version, spirv_cross_version="bumped",
                              python_version=DEFAULT_ENV.python_version, host_platform=DEFAULT_ENV.host_platform)
        d = check_cache(self.cache_file, [self.glsl], self.outputs_dir, self.root,
                        self.script, self.modules, drifted)
        self.assertFalse(d.hit)

    def test_08_python_version_drift(self):
        write_state(self.cache_file, self.state)
        drifted = EnvVersions(glslang_version=DEFAULT_ENV.glslang_version,
                              spirv_cross_version=DEFAULT_ENV.spirv_cross_version,
                              python_version="9.9.9",
                              host_platform=DEFAULT_ENV.host_platform)
        self.assertFalse(check_cache(self.cache_file, [self.glsl], self.outputs_dir, self.root,
                                     self.script, self.modules, drifted).hit)

    def test_09_host_platform_drift(self):
        write_state(self.cache_file, self.state)
        drifted = EnvVersions(glslang_version=DEFAULT_ENV.glslang_version,
                              spirv_cross_version=DEFAULT_ENV.spirv_cross_version,
                              python_version=DEFAULT_ENV.python_version,
                              host_platform="hypothetical_os")
        self.assertFalse(check_cache(self.cache_file, [self.glsl], self.outputs_dir, self.root,
                                     self.script, self.modules, drifted).hit)

    # --- input file set & content ------------------------------------------

    def test_10_input_file_added(self):
        write_state(self.cache_file, self.state)
        extra = _make_glsl(self.inputs_dir, "extra.glsl", body="layout(std140) uniform G {} ;\n")
        d = check_cache(self.cache_file, [self.glsl, extra], self.outputs_dir, self.root,
                        self.script, self.modules, DEFAULT_ENV)
        self.assertFalse(d.hit)
        self.assertIn("input file set", d.reason)

    def test_11_input_file_removed(self):
        extra = _make_glsl(self.inputs_dir, "extra.glsl", body="x\n")
        state2 = build_state(
            [self.glsl, extra], self.outputs_dir, self.output_names, self.root,
            self.script, self.modules, DEFAULT_ENV, 1, 3, 0,
        )
        write_state(self.cache_file, state2)
        d = check_cache(self.cache_file, [self.glsl], self.outputs_dir, self.root,
                        self.script, self.modules, DEFAULT_ENV)
        self.assertFalse(d.hit)

    def test_12_input_content_changed(self):
        write_state(self.cache_file, self.state)
        # Bump mtime + change content.
        time.sleep(0.01)
        self.glsl.write_text("layout(std140) uniform F { int x; };\n")
        # Force mtime jump beyond MTIME_EPSILON_SECONDS.
        future = time.time() + 5.0
        os.utime(self.glsl, (future, future))
        d = self._check()
        self.assertFalse(d.hit)
        self.assertIn("content", d.reason)

    def test_13_mtime_drift_content_unchanged_is_hit(self):
        write_state(self.cache_file, self.state)
        # Touch the file to bump mtime, content unchanged.
        future = time.time() + 5.0
        os.utime(self.glsl, (future, future))
        d = self._check()
        self.assertTrue(d.hit, msg=d.reason)

    def test_14_input_disappeared(self):
        write_state(self.cache_file, self.state)
        self.glsl.unlink()
        # Pretend the caller still includes the path in input_files
        # (e.g. CMake DEPENDS lingering); check_cache should miss.
        d = check_cache(self.cache_file, [self.glsl], self.outputs_dir, self.root,
                        self.script, self.modules, DEFAULT_ENV)
        self.assertFalse(d.hit)

    # --- output integrity ---------------------------------------------------

    def test_15_output_missing(self):
        write_state(self.cache_file, self.state)
        self.out_layout.unlink()
        d = self._check()
        self.assertFalse(d.hit)
        self.assertIn("output missing", d.reason)

    def test_16_output_tampered(self):
        write_state(self.cache_file, self.state)
        self.out_meta.write_text("// tampered\n", encoding="utf-8")
        d = self._check()
        self.assertFalse(d.hit)
        self.assertIn("sha256 changed", d.reason)

    def test_17_expected_output_set_drift(self):
        write_state(self.cache_file, self.state)
        # Caller passes an expected list that doesn't match recorded outputs.
        d = self._check(expected_names=["ubo_layout_other.inl"] + self.output_names)
        self.assertFalse(d.hit)
        self.assertIn("output file set drift", d.reason)


# --- build_state schema --------------------------------------------------

class BuildStateSchemaTests(unittest.TestCase):
    def test_schema_fields_present(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            inp = _make_glsl(tmp, "x.glsl", body="void main(){}\n")
            out = tmp / "out"; out.mkdir()
            o = _make_output(out, "ubo_index.inl")
            script = _make_script(tmp, "main.py")
            mod = _make_script(tmp, "perfect_hash.py")
            state = build_state(
                [inp], out, ["ubo_index.inl"], tmp,
                script, [script, mod], DEFAULT_ENV,
                total_ubos=7, total_uniforms=42, build_duration_ms=123,
            )
            self.assertEqual(state["version"], CACHE_SCHEMA_VERSION)
            self.assertIn("codegen_tool_version", state)
            self.assertEqual(state["codegen_tool_version"]["script_path"], "main.py")
            self.assertEqual(set(state["codegen_tool_version"]["modules_sha256"].keys()),
                             {"main.py", "perfect_hash.py"})
            self.assertEqual(state["environment"]["glslang_version"], DEFAULT_ENV.glslang_version)
            self.assertEqual(set(state["input_files"].keys()), {"x.glsl"})
            self.assertEqual(set(state["output_files"].keys()), {"ubo_index.inl"})
            md = state["build_metadata"]
            self.assertEqual(md["total_ubos"], 7)
            self.assertEqual(md["total_uniforms"], 42)
            self.assertEqual(md["build_duration_ms"], 123)
            self.assertRegex(md["last_build_timestamp_utc"],
                             r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$")

    def test_build_state_raises_when_output_missing(self):
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            inp = _make_glsl(tmp, "x.glsl")
            out = tmp / "out"; out.mkdir()
            script = _make_script(tmp, "main.py")
            with self.assertRaises(CodegenError):
                build_state(
                    [inp], out, ["never_written.inl"], tmp,
                    script, [script], DEFAULT_ENV,
                    total_ubos=0, total_uniforms=0, build_duration_ms=0,
                )


# --- touch_outputs / canonical names -------------------------------------

class TouchOutputsTests(unittest.TestCase):
    def test_touch_bumps_mtime(self):
        with tempfile.TemporaryDirectory() as td:
            out = Path(td)
            p = _make_output(out, "ubo_index.inl")
            old = p.stat().st_mtime
            time.sleep(0.01)
            future = time.time() + 60.0
            touch_outputs(out, ["ubo_index.inl"], now=future)
            self.assertAlmostEqual(p.stat().st_mtime, future, places=0)
            self.assertGreater(p.stat().st_mtime, old)

    def test_touch_missing_file_is_silent(self):
        with tempfile.TemporaryDirectory() as td:
            # Should not raise on a name with no file behind it.
            touch_outputs(Path(td), ["ghost.inl"])


class CanonicalNamesTests(unittest.TestCase):
    def test_layout_filename_lowercased(self):
        self.assertEqual(layout_filename("FrameViewProj"), "ubo_layout_frameviewproj.inl")
        self.assertEqual(layout_filename("Program_GammaCorrect"),
                         "ubo_layout_program_gammacorrect.inl")

    def test_canonical_output_files_order(self):
        names = canonical_output_files(["FrameA", "FrameB"])
        self.assertEqual(names[:2], ["ubo_layout_framea.inl", "ubo_layout_frameb.inl"])
        self.assertEqual(names[2:], [
            OUTPUT_PERFECT_HASH,
            OUTPUT_METADATA,
            OUTPUT_INDEX,
            OUTPUT_DUMMY_INIT,
            OUTPUT_HOST_LOADER,
        ])

    def test_canonical_output_files_empty_blocks(self):
        # Phase 1.A allows 0-block emit (= empty blueprint dir); the aggregated
        # 5-file set must still appear so cache hit path can touch them.
        names = canonical_output_files([])
        self.assertEqual(names, [
            OUTPUT_PERFECT_HASH,
            OUTPUT_METADATA,
            OUTPUT_INDEX,
            OUTPUT_DUMMY_INIT,
            OUTPUT_HOST_LOADER,
        ])


if __name__ == "__main__":
    unittest.main()
