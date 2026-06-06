# Tests for spirv_reflect.py (PA-4) — JSON schema drift + verify mismatch
# Run: python3 -m unittest discover -s scripts/ubo_codegen/tests -t scripts/ubo_codegen
#
# These tests do NOT spawn glslang / spirv-cross. They hit the normalisation
# boundary (_normalise_reflection) and the verifier (verify_layout_against_spirv)
# directly — that is exactly the §5.4.1.5 design intent: the JSON schema lives
# at one wall, everything else consumes the normalised dict.

import os
import unittest

from codegen_error import CodegenError
from spirv_reflect import (
    SKIP_ENV_VAR,
    _normalise_reflection,
    verify_layout_against_spirv,
)
from std140 import BlockLayout, MemberLayout


def _layout(name, members):
    bl = BlockLayout(name=name)
    bl.members = members
    bl.std140_size = sum(m.size for m in members)
    bl.block_size = 256
    return bl


class NormaliseReflectionTests(unittest.TestCase):
    def test_basic_block(self):
        refl = {
            "types": {
                "_T1": {
                    "name": "FrameViewProj",
                    "members": [
                        {"name": "view",      "type": "mat4", "offset": 0},
                        {"name": "proj",      "type": "mat4", "offset": 64},
                        {"name": "view_proj", "type": "mat4", "offset": 128},
                    ],
                }
            },
            "ubos": [
                {"type": "_T1", "name": "FrameViewProj",
                 "block_size": 192, "set": 0, "binding": 0},
            ],
        }
        norm = _normalise_reflection(refl)
        self.assertIn("FrameViewProj", norm)
        b = norm["FrameViewProj"]
        self.assertEqual(b["_block_size"], 192)
        self.assertEqual(b["_set"], 0)
        self.assertEqual(b["_binding"], 0)
        self.assertEqual(b["view"]["offset"], 0)
        self.assertEqual(b["proj"]["offset"], 64)
        self.assertEqual(b["view_proj"]["offset"], 128)

    def test_array_stride_passthrough(self):
        refl = {
            "types": {"_T": {"name": "Arr",
                              "members": [
                                  {"name": "arr", "type": "float",
                                   "offset": 0, "array_stride": 16},
                              ]}},
            "ubos": [{"type": "_T", "name": "Arr", "block_size": 128}],
        }
        b = _normalise_reflection(refl)["Arr"]
        self.assertEqual(b["arr"]["array_stride"], 16)

    def test_missing_array_stride_defaults_zero(self):
        refl = {
            "types": {"_T": {"name": "B", "members": [
                {"name": "x", "type": "float", "offset": 0},
            ]}},
            "ubos": [{"type": "_T", "name": "B"}],
        }
        b = _normalise_reflection(refl)["B"]
        self.assertEqual(b["x"]["array_stride"], 0)

    def test_root_not_object_raises(self):
        with self.assertRaises(CodegenError) as cm:
            _normalise_reflection([])  # type: ignore[arg-type]
        self.assertIn("root is not an object", str(cm.exception))

    def test_types_not_object_raises(self):
        with self.assertRaises(CodegenError) as cm:
            _normalise_reflection({"types": []})
        self.assertIn("'types' must be an object", str(cm.exception))

    def test_ubo_missing_name_raises(self):
        refl = {"types": {"_T": {"members": []}}, "ubos": [{"type": "_T"}]}
        with self.assertRaises(CodegenError) as cm:
            _normalise_reflection(refl)
        self.assertIn("missing 'name' or 'type'", str(cm.exception))

    def test_ubo_unknown_type_raises(self):
        refl = {"types": {}, "ubos": [{"type": "_T", "name": "B"}]}
        with self.assertRaises(CodegenError) as cm:
            _normalise_reflection(refl)
        self.assertIn("unknown type", str(cm.exception))

    def test_empty_ubos_returns_empty(self):
        self.assertEqual(_normalise_reflection({"types": {}, "ubos": []}), {})

    def test_ubos_field_missing_returns_empty(self):
        # spirv-cross may emit a reflection with no "ubos" key at all.
        self.assertEqual(_normalise_reflection({"types": {}}), {})


class VerifyLayoutTests(unittest.TestCase):
    def _frame_layout(self):
        return _layout("FrameViewProj", [
            MemberLayout(name="view",      offset=0,   size=64, align=16),
            MemberLayout(name="proj",      offset=64,  size=64, align=16),
            MemberLayout(name="view_proj", offset=128, size=64, align=16),
        ])

    def _frame_spirv(self):
        return {
            "FrameViewProj": {
                "_block_size": 192, "_set": 0, "_binding": 0,
                "view":      {"offset": 0,   "array_stride": 0},
                "proj":      {"offset": 64,  "array_stride": 0},
                "view_proj": {"offset": 128, "array_stride": 0},
            }
        }

    def test_match_passes(self):
        layout = self._frame_layout()
        layout.std140_size = 192
        verify_layout_against_spirv(layout, self._frame_spirv())  # no raise

    def test_block_missing(self):
        with self.assertRaises(CodegenError) as cm:
            verify_layout_against_spirv(self._frame_layout(), {})
        self.assertIn("missing from SPIR-V reflection", str(cm.exception))

    def test_block_size_mismatch(self):
        layout = self._frame_layout()
        layout.std140_size = 256  # diverged from 192
        with self.assertRaises(CodegenError) as cm:
            verify_layout_against_spirv(layout, self._frame_spirv())
        self.assertIn("block size mismatch", str(cm.exception))

    def test_member_missing(self):
        layout = self._frame_layout()
        layout.std140_size = 192
        spv = self._frame_spirv()
        del spv["FrameViewProj"]["proj"]
        with self.assertRaises(CodegenError) as cm:
            verify_layout_against_spirv(layout, spv)
        self.assertIn("member 'FrameViewProj.proj' missing", str(cm.exception))

    def test_member_offset_mismatch(self):
        layout = self._frame_layout()
        layout.std140_size = 192
        spv = self._frame_spirv()
        spv["FrameViewProj"]["proj"]["offset"] = 80  # wrong
        with self.assertRaises(CodegenError) as cm:
            verify_layout_against_spirv(layout, spv)
        self.assertIn("std140 offset mismatch", str(cm.exception))
        self.assertIn("FrameViewProj.proj", str(cm.exception))

    def test_array_stride_mismatch(self):
        layout = _layout("Arr", [
            MemberLayout(name="arr", offset=0, size=128, align=16, array_stride=16),
        ])
        layout.std140_size = 128
        spv = {"Arr": {
            "_block_size": 128,
            "arr": {"offset": 0, "array_stride": 32},  # wrong
        }}
        with self.assertRaises(CodegenError) as cm:
            verify_layout_against_spirv(layout, spv)
        self.assertIn("array stride mismatch", str(cm.exception))

    def test_array_stride_match_passes(self):
        layout = _layout("Arr", [
            MemberLayout(name="arr", offset=0, size=128, align=16, array_stride=16),
        ])
        layout.std140_size = 128
        spv = {"Arr": {
            "_block_size": 128,
            "arr": {"offset": 0, "array_stride": 16},
        }}
        verify_layout_against_spirv(layout, spv)  # no raise

    def test_offset_field_absent_raises(self):
        layout = self._frame_layout()
        layout.std140_size = 192
        spv = self._frame_spirv()
        spv["FrameViewProj"]["proj"]["offset"] = None
        with self.assertRaises(CodegenError) as cm:
            verify_layout_against_spirv(layout, spv)
        self.assertIn("lacks offset field", str(cm.exception))

    def test_escape_hatch_skips(self):
        layout = self._frame_layout()
        layout.std140_size = 999  # would otherwise fail
        os.environ[SKIP_ENV_VAR] = "1"
        try:
            verify_layout_against_spirv(layout, {})  # missing block, but skipped
        finally:
            del os.environ[SKIP_ENV_VAR]


if __name__ == "__main__":
    unittest.main()
