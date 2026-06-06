# Tests for main.py (PA-7.6 = set/binding forward fix + Phase 1.B entry 副次 (a) cadence prefix/suffix)
# Run: python3 -m unittest discover -s scripts/ubo_codegen/tests -t scripts/ubo_codegen

import unittest

from glsl_parser import Member, UboBlockDecl
from main import (
    CADENCE_PER_DRAW,
    CADENCE_PER_PROGRAM,
    _derive_cadence,
    _ubo_to_block_spec,
)
from std140 import BlockLayout


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
