# Tests for main.py (PA-7.6 = set/binding forward fix + Phase 1.B entry 副次 (a) cadence prefix/suffix)
# Run: python3 -m unittest discover -s scripts/ubo_codegen/tests -t scripts/ubo_codegen

import logging
import tempfile
import unittest
from pathlib import Path

from codegen_error import CodegenError
from glsl_parser import Member, UboBlockDecl
from main import (
    CADENCE_PER_DRAW,
    CADENCE_PER_PROGRAM,
    _derive_cadence,
    _defines_hash,
    _defines_to_args,
    _load_defines_file,
    _ubo_to_block_spec,
    _verify_block_match,
)
from std140 import BlockLayout, MemberLayout


class UboToBlockSpecTests(unittest.TestCase):
    def test_set_and_binding_forwarded_when_present(self):
        # layout(std140, set=0, binding=1) → BlockSpec.descriptor_set=0, binding=1, subset=0
        ubo = UboBlockDecl(
            block_name="FrameLights",
            layout_qual={"std140": True, "set": 0, "binding": 1},
            members=[Member(name="dummy", type_str="float")],
        )
        layout = BlockLayout(name="FrameLights")
        spec = _ubo_to_block_spec(ubo, layout)
        self.assertEqual(spec.descriptor_set, 0)
        self.assertEqual(spec.binding, 1)
        self.assertEqual(spec.subset, 0)

    def test_set_and_binding_default_to_zero_when_absent(self):
        # layout(std140, push_constant) — no set/binding → defaults 0/0/subset=0
        ubo = UboBlockDecl(
            block_name="PushConstants",
            layout_qual={"std140": True, "push_constant": True},
            members=[Member(name="dummy", type_str="float")],
        )
        layout = BlockLayout(name="PushConstants")
        spec = _ubo_to_block_spec(ubo, layout)
        self.assertEqual(spec.descriptor_set, 0)
        self.assertEqual(spec.binding, 0)
        self.assertEqual(spec.subset, 0)


class SubsetSplitTests(unittest.TestCase):
    """PC-7α' — design 06c §2.3 set=1 subset split (= 1a binding<40 / 1b binding>=40)."""

    def _spec(self, set_id: int, binding: int):
        ubo = UboBlockDecl(
            block_name="ProbeBlock",
            layout_qual={"std140": True, "set": set_id, "binding": binding},
            members=[Member(name="dummy", type_str="float")],
        )
        return _ubo_to_block_spec(ubo, BlockLayout(name="ProbeBlock"))

    def test_set1_subset_split_at_binding_40_boundary(self):
        # set=1: binding<40 → subset=0 (1a), binding>=40 → subset=1 (1b).
        # Other sets always carry subset=0 regardless of binding.
        self.assertEqual(self._spec(1,  0).subset, 0)
        self.assertEqual(self._spec(1, 39).subset, 0)
        self.assertEqual(self._spec(1, 40).subset, 1)
        self.assertEqual(self._spec(1, 79).subset, 1)
        self.assertEqual(self._spec(0, 40).subset, 0)
        self.assertEqual(self._spec(2, 40).subset, 0)
        self.assertEqual(self._spec(3, 40).subset, 0)


class DeriveCadenceTests(unittest.TestCase):
    def test_perdrawubo_prefix_resolves_to_per_draw(self):
        # AYAstorm naming PerDrawUBO_* → set=2 binding=0 共存系 6 UBO 等
        self.assertEqual(_derive_cadence("PerDrawUBO_ClipPlane"), CADENCE_PER_DRAW)
        self.assertEqual(_derive_cadence("PerDrawUBO_AvatarSkin"), CADENCE_PER_DRAW)

    def test_perprogramubo_prefix_resolves_to_per_program(self):
        # AYAstorm naming PerProgramUBO_* → set=2 binding=1..25 系 24 UBO 等
        self.assertEqual(_derive_cadence("PerProgramUBO_GammaCorrect"), CADENCE_PER_PROGRAM)
        self.assertEqual(_derive_cadence("PerProgramUBO_PointLightV"), CADENCE_PER_PROGRAM)

    def test_ubo_legacy_suffix_resolves_to_per_program(self):
        # AYAstorm naming <Name>UBO_Legacy → set=1 MaterialUBO_Legacy + set=3 54 UBO 等
        self.assertEqual(_derive_cadence("MaterialUBO_Legacy"), CADENCE_PER_PROGRAM)
        self.assertEqual(_derive_cadence("WaterFogUBO_Legacy"), CADENCE_PER_PROGRAM)
        self.assertEqual(_derive_cadence("CloudsVParamUBO_Legacy"), CADENCE_PER_PROGRAM)


class MultiFileIntegrityTests(unittest.TestCase):
    """Phase 2.α α-2 (= 2026-06-06): 同名 UBO 複数 file 整合 verify。

    design 08:72-74/96 想定 = codegen 入力 = class*/ + cinematic_bd/ 配下、
    inventory:247-249 既認識の同名 UBO 複数 file declaration を構造的に verify、
    cinematic_bd 上書き path は同 layout なら legitimate dual declaration として PASS。
    """

    def _spec(self, name, set_id, binding, members):
        layout = BlockLayout(name=name, members=list(members))
        ubo = UboBlockDecl(
            block_name=name,
            layout_qual={"std140": True, "set": set_id, "binding": binding},
            members=[Member(name=m.name, type_str="float") for m in members],
        )
        return _ubo_to_block_spec(ubo, layout)

    def test_matching_blocks_pass(self):
        # 同名 UBO 2 file 同 binding/layout → verify PASS, no raise
        m1 = MemberLayout(name="x", offset=0, size=4, align=4)
        spec_a = self._spec("WaterVParamUBO_Legacy", 1, 79, [m1])
        spec_b = self._spec("WaterVParamUBO_Legacy", 1, 79, [m1])
        _verify_block_match(spec_a, "waterV.glsl", spec_b, "waterF.glsl")

    def test_binding_mismatch_raises(self):
        m1 = MemberLayout(name="x", offset=0, size=4, align=4)
        spec_a = self._spec("WaterVParamUBO_Legacy", 1, 79, [m1])
        spec_b = self._spec("WaterVParamUBO_Legacy", 1, 78, [m1])
        with self.assertRaises(CodegenError) as ctx:
            _verify_block_match(spec_a, "waterV.glsl", spec_b, "waterF.glsl")
        self.assertIn("set/binding", str(ctx.exception))

    def test_set_mismatch_raises(self):
        m1 = MemberLayout(name="x", offset=0, size=4, align=4)
        spec_a = self._spec("WaterVParamUBO_Legacy", 1, 79, [m1])
        spec_b = self._spec("WaterVParamUBO_Legacy", 2, 79, [m1])
        with self.assertRaises(CodegenError) as ctx:
            _verify_block_match(spec_a, "waterV.glsl", spec_b, "waterF.glsl")
        self.assertIn("set/binding", str(ctx.exception))

    def test_member_count_mismatch_raises(self):
        m1 = MemberLayout(name="x", offset=0, size=4, align=4)
        m2 = MemberLayout(name="y", offset=4, size=4, align=4)
        spec_a = self._spec("WaterVParamUBO_Legacy", 1, 79, [m1])
        spec_b = self._spec("WaterVParamUBO_Legacy", 1, 79, [m1, m2])
        with self.assertRaises(CodegenError) as ctx:
            _verify_block_match(spec_a, "waterV.glsl", spec_b, "waterF.glsl")
        self.assertIn("member count", str(ctx.exception))

    def test_member_name_mismatch_raises(self):
        m1 = MemberLayout(name="x", offset=0, size=4, align=4)
        m2 = MemberLayout(name="y", offset=0, size=4, align=4)
        spec_a = self._spec("WaterVParamUBO_Legacy", 1, 79, [m1])
        spec_b = self._spec("WaterVParamUBO_Legacy", 1, 79, [m2])
        with self.assertRaises(CodegenError) as ctx:
            _verify_block_match(spec_a, "waterV.glsl", spec_b, "waterF.glsl")
        self.assertIn("mismatch", str(ctx.exception))

    def test_member_offset_mismatch_raises(self):
        m1 = MemberLayout(name="x", offset=0, size=4, align=4)
        m2 = MemberLayout(name="x", offset=4, size=4, align=4)
        spec_a = self._spec("WaterVParamUBO_Legacy", 1, 79, [m1])
        spec_b = self._spec("WaterVParamUBO_Legacy", 1, 79, [m2])
        with self.assertRaises(CodegenError) as ctx:
            _verify_block_match(spec_a, "waterV.glsl", spec_b, "waterF.glsl")
        self.assertIn("mismatch", str(ctx.exception))

    def test_cinematic_bd_overlay_path_pass(self):
        # cinematic_bd 上書き path + class1 path で同名 UBO 同 layout → PASS
        # (= ShadowUtilParamUBO_Legacy 想定、inventory:247-249 確認済)
        m1 = MemberLayout(name="x", offset=0, size=4, align=4)
        spec_a = self._spec("ShadowUtilParamUBO_Legacy", 1, 63, [m1])
        spec_b = self._spec("ShadowUtilParamUBO_Legacy", 1, 63, [m1])
        _verify_block_match(
            spec_a, "cinematic_bd/class1/deferred/shadowUtil.glsl",
            spec_b, "class1/deferred/shadowUtil.glsl",
        )


class RuntimeEmulationDefinesTests(unittest.TestCase):
    """Phase 2.α α-3 improvement 1.5.c (= 2026-06-06): C++ runtime emulation 層
    AYAstorm C++ const dump file (= aya_r41_codegen_defines.toml) 読込 + glslang -E への
    -D<key>=<value> prepend 連携。handoff §D.9.5 #4 + design 04 §4.5 + 08 §5.0。
    """

    def setUp(self):
        self.log = logging.getLogger("test_runtime_emulation_defines")
        self.log.addHandler(logging.NullHandler())

    def _write_toml(self, content: str) -> Path:
        tmp = tempfile.NamedTemporaryFile(suffix=".toml", delete=False, mode="w", encoding="utf-8")
        tmp.write(content)
        tmp.close()
        return Path(tmp.name)

    def test_load_defines_returns_empty_when_path_is_none(self):
        # --defines-file 未指定時は空 dict 返し (= bare preprocess fallback、後方互換維持)
        result = _load_defines_file(None, self.log)
        self.assertEqual(result, {})

    def test_load_defines_parses_valid_toml(self):
        # 正常系: [defines] table を str → str dict として返す
        path = self._write_toml(
            '[defines]\n'
            'MAX_JOINTS_PER_MESH_OBJECT = "110"\n'
            'LIGHT_COUNT = "16"\n'
        )
        result = _load_defines_file(path, self.log)
        self.assertEqual(result, {"MAX_JOINTS_PER_MESH_OBJECT": "110", "LIGHT_COUNT": "16"})
        path.unlink()

    def test_load_defines_raises_on_missing_file(self):
        # path が存在しない → CodegenError
        with self.assertRaises(CodegenError):
            _load_defines_file(Path("/tmp/non_existent_aya_defines.toml"), self.log)

    def test_load_defines_raises_on_invalid_toml(self):
        # TOML parse error → CodegenError
        path = self._write_toml('[defines\nbroken = "no closing bracket"')
        try:
            with self.assertRaises(CodegenError):
                _load_defines_file(path, self.log)
        finally:
            path.unlink()

    def test_load_defines_raises_on_non_string_value(self):
        # value が int (= "110" でなく 110) → CodegenError (= glslang -D は string 必須)
        path = self._write_toml(
            '[defines]\n'
            'MAX_JOINTS_PER_MESH_OBJECT = 110\n'
        )
        try:
            with self.assertRaises(CodegenError):
                _load_defines_file(path, self.log)
        finally:
            path.unlink()

    def test_load_defines_returns_empty_when_defines_table_missing(self):
        # [defines] table 不在 = 空 dict (= dump file 雛形のみ等、エラーにしない)
        path = self._write_toml('[metadata]\nversion = 1\n')
        try:
            result = _load_defines_file(path, self.log)
            self.assertEqual(result, {})
        finally:
            path.unlink()

    def test_defines_to_args_converts_to_glslang_d_flags(self):
        # {name: value} → ["-D<name>=<value>", ...] (= glslang -E に渡す形式)
        defines = {"MAX_JOINTS_PER_MESH_OBJECT": "110", "LIGHT_COUNT": "16"}
        args = _defines_to_args(defines)
        self.assertIn("-DMAX_JOINTS_PER_MESH_OBJECT=110", args)
        self.assertIn("-DLIGHT_COUNT=16", args)
        self.assertEqual(len(args), 2)

    def test_defines_to_args_empty_dict_returns_empty_list(self):
        self.assertEqual(_defines_to_args({}), [])

    def test_defines_hash_returns_sentinel_when_empty(self):
        # 空 defines = "no-defines" sentinel (= cache key で識別可能)
        self.assertEqual(_defines_hash({}), "no-defines")

    def test_defines_hash_stable_across_key_order(self):
        # 同 content の defines は dict 順序によらず同 hash (= 順序非依存 cache key)
        h1 = _defines_hash({"A": "1", "B": "2"})
        h2 = _defines_hash({"B": "2", "A": "1"})
        self.assertEqual(h1, h2)
        # 異なる value は異 hash (= cache invalidation 連動)
        h3 = _defines_hash({"A": "1", "B": "3"})
        self.assertNotEqual(h1, h3)

    def test_load_actual_aya_r41_codegen_defines_toml(self):
        # 実 dump file (= scripts/ubo_codegen/aya_r41_codegen_defines.toml、improvement 1.5.b)
        # を読込、想定 macro 5 件すべて str → str で取得できる。
        repo_dump = Path(__file__).resolve().parent.parent / "aya_r41_codegen_defines.toml"
        self.assertTrue(repo_dump.is_file(), f"actual dump file missing: {repo_dump}")
        result = _load_defines_file(repo_dump, self.log)
        # dump 必須 5 macro 全件存在 confirm
        self.assertEqual(result.get("MAX_JOINTS_PER_MESH_OBJECT"), "110")
        self.assertEqual(result.get("MAX_NODES_PER_GLTF_OBJECT"), "1365")
        self.assertEqual(result.get("MAX_UBO_VEC4S"), "4096")
        self.assertEqual(result.get("LIGHT_COUNT"), "16")
        self.assertEqual(result.get("REF_SAMPLE_COUNT"), "32")
        # dump 不要 macro (= GLSL self-define) は dump file に含まれない
        self.assertNotIn("MAX_REFMAP_COUNT", result)
        self.assertNotIn("NUM_DIRECTIONS", result)
