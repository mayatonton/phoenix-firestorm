# Tests for std140.py (PA-4) — GLSL spec 7.6.2.2 reference cases
# Run: python3 -m unittest discover -s scripts/ubo_codegen/tests -t scripts/ubo_codegen

import unittest

from codegen_error import CodegenError
from std140 import (
    DEVICE_OFFSET_ALIGN,
    MemberSpec,
    arrayify,
    compute_layout,
    pad_to_device_align,
    PRIMITIVE_TYPES,
    round_up,
)


def offsets(layout):
    return [(m.name, m.offset, m.size, m.align, m.array_stride) for m in layout.members]


class Std140UtilTests(unittest.TestCase):
    def test_round_up(self):
        self.assertEqual(round_up(0, 16), 0)
        self.assertEqual(round_up(1, 16), 16)
        self.assertEqual(round_up(16, 16), 16)
        self.assertEqual(round_up(17, 16), 32)
        self.assertEqual(round_up(12, 16), 16)

    def test_round_up_rejects_zero_align(self):
        with self.assertRaises(CodegenError):
            round_up(8, 0)

    def test_pad_to_device_align_256(self):
        self.assertEqual(pad_to_device_align(0), 0)
        self.assertEqual(pad_to_device_align(1), DEVICE_OFFSET_ALIGN)
        self.assertEqual(pad_to_device_align(255), DEVICE_OFFSET_ALIGN)
        self.assertEqual(pad_to_device_align(256), DEVICE_OFFSET_ALIGN)
        self.assertEqual(pad_to_device_align(257), 512)

    def test_arrayify_float(self):
        ti = arrayify(PRIMITIVE_TYPES["float"], 8)
        # stride must be vec4 (= 16), total = 128
        self.assertEqual(ti.array_stride, 16)
        self.assertEqual(ti.base_size, 128)
        self.assertEqual(ti.base_align, 16)

    def test_arrayify_vec3(self):
        ti = arrayify(PRIMITIVE_TYPES["vec3"], 4)
        self.assertEqual(ti.array_stride, 16)
        self.assertEqual(ti.base_size, 64)

    def test_arrayify_rejects_zero(self):
        with self.assertRaises(CodegenError):
            arrayify(PRIMITIVE_TYPES["float"], 0)


class Std140LayoutTests(unittest.TestCase):
    def test_three_mat4(self):
        layout = compute_layout("FrameViewProj", [
            MemberSpec("view", "mat4"),
            MemberSpec("proj", "mat4"),
            MemberSpec("view_proj", "mat4"),
        ])
        self.assertEqual(offsets(layout), [
            ("view",      0,  64, 16, 0),
            ("proj",      64, 64, 16, 0),
            ("view_proj", 128, 64, 16, 0),
        ])
        self.assertEqual(layout.std140_size, 192)
        # 192 is already a 256-multiple? No: pad_to_device_align(192) = 256
        self.assertEqual(layout.block_size, 256)

    def test_vec3_trailing_hole(self):
        # vec3 has size 12, align 16 — a following float fills the hole, no padding
        layout = compute_layout("Trailing", [
            MemberSpec("v", "vec3"),
            MemberSpec("f", "float"),
        ])
        self.assertEqual(offsets(layout), [
            ("v", 0,  12, 16, 0),
            ("f", 12, 4,  4,  0),
        ])
        # block end padding: max align = 16, std140 size 16
        self.assertEqual(layout.std140_size, 16)

    def test_float_then_vec3(self):
        # float at 0 (size 4), vec3 must align to 16 → offset 16
        layout = compute_layout("Mix", [
            MemberSpec("a", "float"),
            MemberSpec("b", "vec3"),
        ])
        self.assertEqual(offsets(layout)[0], ("a", 0, 4, 4, 0))
        self.assertEqual(offsets(layout)[1], ("b", 16, 12, 16, 0))
        # trailing padding to max(member align) = 16 → 28 → 32
        self.assertEqual(layout.std140_size, 32)

    def test_float_array(self):
        # float arr[8] — element stride 16, total 128
        layout = compute_layout("Arr", [
            MemberSpec("arr", "float", array_count=8),
        ])
        self.assertEqual(layout.members[0].array_stride, 16)
        self.assertEqual(layout.members[0].size, 128)
        self.assertEqual(layout.std140_size, 128)

    def test_mat3(self):
        # mat3 has base_size 48 (3 columns × 16 stride)
        layout = compute_layout("M", [MemberSpec("m", "mat3")])
        self.assertEqual(layout.members[0].size, 48)
        self.assertEqual(layout.std140_size, 48)

    def test_mat4_array_of_3(self):
        layout = compute_layout("Bones", [
            MemberSpec("b", "mat4", array_count=3),
        ])
        self.assertEqual(layout.members[0].array_stride, 64)
        self.assertEqual(layout.members[0].size, 192)

    def test_nested_struct(self):
        # struct S { float a; vec3 v; } x;
        # Inside S: a@0 (4), v@16 (12, align 16) → S.std140_size = round_up(28, 16) = 32
        # Top: x@0 (size 32, align 16) → block size 32
        nested = [
            MemberSpec("a", "float"),
            MemberSpec("v", "vec3"),
        ]
        layout = compute_layout("WithStruct", [
            MemberSpec("x", "<struct>", nested_members=nested),
        ])
        self.assertEqual(layout.members[0].align, 16)
        self.assertEqual(layout.members[0].size, 32)
        self.assertEqual(layout.std140_size, 32)

    def test_unsupported_type_raises(self):
        with self.assertRaises(CodegenError) as cm:
            compute_layout("Bad", [MemberSpec("x", "double")])
        self.assertIn("unsupported std140 type 'double'", str(cm.exception))

    def test_max_align_trailing_padding(self):
        # int at 0 (4), then trailing padding to vec4 align = 16
        layout = compute_layout("AlignTail", [
            MemberSpec("count", "int"),
            MemberSpec("color", "vec4"),
        ])
        # color must align to 16 → offset 16
        self.assertEqual(layout.members[1].offset, 16)
        # block size = 32 (count@0 + pad + color@16..32)
        self.assertEqual(layout.std140_size, 32)


if __name__ == "__main__":
    unittest.main()
