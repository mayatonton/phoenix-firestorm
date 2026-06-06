# Tests for glsl_parser.py (PA-3)
# Run: python3 -m unittest discover -s scripts/ubo_codegen/tests -t scripts/ubo_codegen

import unittest

from codegen_error import CodegenError
from glsl_parser import parse_glsl


SIMPLE = """\
#line 1 "frame.glsl"
layout(std140) uniform FrameViewProj {
    mat4 view;
    mat4 proj;
    mat4 view_proj;
};
"""

NESTED = """\
#line 1 "lights.glsl"
struct LightCone {
    float cos_inner;
    float cos_outer;
    vec3  axis;
};

layout(std140) uniform Draw_MultiLight {
    int   light_count;
    vec4  light_color[8];
    float light_radius[8];
    LightCone light_cone;
};
"""

MIXED = """\
#line 1 "materialF.glsl"
uniform sampler2D diffuseMap;
uniform vec4 color;

layout(std140) uniform Program_Gamma {
    float gamma;
};
"""

MISSING_STD140 = """\
layout(set=0, binding=0) uniform BadBlock {
    float x;
};
"""

UNSUPPORTED_TYPE = """\
layout(std140) uniform DoubleBlock {
    double x;
};
"""

DYNAMIC_ARRAY = """\
layout(std140) uniform DynArr {
    float arr[N];
};
"""

DUPLICATE_STRUCT = """\
struct S { float a; };
struct S { float b; };
"""

INSTANCE_NAME = """\
layout(std140) uniform FrameViewProj {
    mat4 view;
} u_view;
"""

MULTI_INSTANCE = """\
layout(std140) uniform FrameViewProj {
    mat4 view;
} u_a, u_b;
"""


class ParserTests(unittest.TestCase):
    def test_simple_ubo(self):
        r = parse_glsl(SIMPLE)
        self.assertEqual(len(r.ubo_blocks), 1)
        b = r.ubo_blocks[0]
        self.assertEqual(b.block_name, "FrameViewProj")
        self.assertTrue(b.layout_qual.get("std140"))
        self.assertEqual([m.name for m in b.members], ["view", "proj", "view_proj"])
        self.assertEqual([m.type_str for m in b.members], ["mat4", "mat4", "mat4"])
        self.assertEqual(b.source_file, "frame.glsl")

    def test_nested_struct(self):
        r = parse_glsl(NESTED)
        self.assertEqual(len(r.struct_defs), 1)
        self.assertEqual(r.struct_defs[0].name, "LightCone")
        self.assertEqual(len(r.ubo_blocks), 1)
        b = r.ubo_blocks[0]
        self.assertEqual(len(b.members), 4)
        self.assertEqual(b.members[1].array_count, 8)
        # nested struct member should be linked
        nested_member = b.members[3]
        self.assertEqual(nested_member.type_str, "LightCone")
        self.assertIsNotNone(nested_member.nested_struct)
        self.assertEqual(nested_member.nested_struct.name, "LightCone")

    def test_sampler_and_bare_uniform(self):
        r = parse_glsl(MIXED)
        self.assertEqual(len(r.sampler_decls), 1)
        self.assertEqual(r.sampler_decls[0].sampler_type, "sampler2D")
        self.assertEqual(r.sampler_decls[0].name, "diffuseMap")
        self.assertEqual(len(r.bare_uniforms), 1)
        self.assertEqual(r.bare_uniforms[0].type_str, "vec4")
        self.assertEqual(r.bare_uniforms[0].name, "color")
        self.assertEqual(len(r.ubo_blocks), 1)

    def test_missing_std140_raises(self):
        with self.assertRaises(CodegenError) as cm:
            parse_glsl(MISSING_STD140)
        self.assertIn("std140 required", str(cm.exception))

    def test_unsupported_type_raises(self):
        with self.assertRaises(CodegenError) as cm:
            parse_glsl(UNSUPPORTED_TYPE)
        self.assertIn("unsupported type 'double'", str(cm.exception))

    def test_dynamic_array_size_raises(self):
        with self.assertRaises(CodegenError) as cm:
            parse_glsl(DYNAMIC_ARRAY)
        self.assertIn("dynamic / non-literal array size", str(cm.exception))

    def test_duplicate_struct_raises(self):
        with self.assertRaises(CodegenError):
            parse_glsl(DUPLICATE_STRUCT)

    def test_instance_name_kept(self):
        r = parse_glsl(INSTANCE_NAME)
        self.assertEqual(r.ubo_blocks[0].instance_name, "u_view")

    def test_multi_instance_rejected(self):
        with self.assertRaises(CodegenError):
            parse_glsl(MULTI_INSTANCE)

    def test_line_directive_tracked(self):
        src = '#line 42 "deep/path.glsl"\nlayout(std140) uniform A { float x; };\n'
        r = parse_glsl(src)
        b = r.ubo_blocks[0]
        self.assertEqual(b.source_file, "deep/path.glsl")
        self.assertEqual(b.source_line, 42)

    def test_non_ubo_layout_skipped_out(self):
        src = (
            "#version 450\n"
            "layout(location = 0) out vec4 outColor;\n"
            "layout(std140, set = 0, binding = 0) uniform U { float x; };\n"
        )
        r = parse_glsl(src)
        self.assertEqual(len(r.ubo_blocks), 1)
        self.assertEqual(r.ubo_blocks[0].block_name, "U")

    def test_non_ubo_layout_skipped_in(self):
        src = (
            "layout(location = 1) in vec3 inNormal;\n"
            "layout(std140) uniform V { float y; };\n"
        )
        r = parse_glsl(src)
        self.assertEqual(len(r.ubo_blocks), 1)
        self.assertEqual(r.ubo_blocks[0].block_name, "V")

    def test_non_ubo_layout_skipped_sampler(self):
        src = (
            "layout(binding = 5) uniform sampler2D tex;\n"
            "layout(std140) uniform W { float z; };\n"
        )
        r = parse_glsl(src)
        self.assertEqual(len(r.ubo_blocks), 1)
        self.assertEqual(r.ubo_blocks[0].block_name, "W")


if __name__ == "__main__":
    unittest.main()
