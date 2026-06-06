# Tests for perfect_hash.py (PA-5)
# Run: python3 -m unittest discover -s scripts/ubo_codegen/tests -t scripts/ubo_codegen

import unittest

from codegen_error import CodegenError
from std140 import BlockLayout, MemberLayout

from perfect_hash import (
    BlockSpec,
    CHDResult,
    DEFAULT_BASE_SEED,
    FNV_OFFSET_BASIS,
    FNV_PRIME,
    UniformLocation,
    build_chd,
    collect_uniform_keys,
    emit_dummy_init_inl,
    emit_host_loader_inl,
    emit_index_inl,
    emit_perfect_hash_inl,
    fnv1a_32,
    lookup_python,
    next_prime,
    verify_chd,
    _count_per_set_bindings,
    _escape_cstring,
    _is_prime,
    _wrap_uint_array,
)


def _layout(name, members, std140_size=None, block_size=None):
    if std140_size is None:
        std140_size = max((m.offset + m.size for m in members), default=0)
    if block_size is None:
        block_size = (std140_size + 255) & ~255
    return BlockLayout(
        name=name,
        members=list(members),
        std140_size=std140_size,
        block_size=block_size,
    )


def _frame_view_proj_layout():
    return _layout(
        "FrameViewProj",
        [
            MemberLayout(name="view", offset=0, size=64, align=16),
            MemberLayout(name="proj", offset=64, size=64, align=16),
            MemberLayout(name="view_proj", offset=128, size=64, align=16),
        ],
        std140_size=192,
        block_size=256,
    )


def _frame_view_proj_spec():
    return BlockSpec(
        name="FrameViewProj",
        layout=_frame_view_proj_layout(),
        cadence_tag=0,
        descriptor_set=0,
        binding=0,
        subset=0,
    )


class HashHelperTests(unittest.TestCase):
    def test_fnv1a_32_deterministic(self):
        self.assertEqual(fnv1a_32("hello"), fnv1a_32("hello"))
        self.assertNotEqual(fnv1a_32("hello"), fnv1a_32("world"))

    def test_fnv1a_32_known_empty_seed(self):
        # FNV-1a of "" must return the offset basis unchanged.
        self.assertEqual(fnv1a_32(""), FNV_OFFSET_BASIS)

    def test_fnv1a_32_known_single_byte(self):
        # FNV-1a("a") = (offset_basis XOR 'a') * FNV_PRIME, masked to 32 bit.
        expected = ((FNV_OFFSET_BASIS ^ ord("a")) * FNV_PRIME) & 0xFFFFFFFF
        self.assertEqual(fnv1a_32("a"), expected)

    def test_fnv1a_32_seed_changes_output(self):
        self.assertNotEqual(fnv1a_32("foo", 0), fnv1a_32("foo", 1))

    def test_fnv1a_32_masked_to_32_bit(self):
        # Long input must still fit in uint32_t.
        h = fnv1a_32("x" * 1024)
        self.assertGreaterEqual(h, 0)
        self.assertLessEqual(h, 0xFFFFFFFF)

    def test_next_prime_small(self):
        self.assertEqual(next_prime(0), 2)
        self.assertEqual(next_prime(1), 2)
        self.assertEqual(next_prime(2), 2)
        self.assertEqual(next_prime(3), 3)
        self.assertEqual(next_prime(4), 5)
        self.assertEqual(next_prime(10), 11)
        self.assertEqual(next_prime(11), 11)
        self.assertEqual(next_prime(96), 97)

    def test_is_prime(self):
        for n in [2, 3, 5, 7, 11, 13, 97, 113]:
            self.assertTrue(_is_prime(n), n)
        for n in [0, 1, 4, 6, 8, 9, 25, 100]:
            self.assertFalse(_is_prime(n), n)


class CHDConstructionTests(unittest.TestCase):
    def _make_inputs(self, n, prefix="key_"):
        keys = [f"{prefix}{i:04d}" for i in range(n)]
        values = [
            UniformLocation(block_hash=i, offset=i * 4, size=4, cadence_tag=0)
            for i in range(n)
        ]
        return keys, values

    def test_empty(self):
        r = build_chd([], [])
        self.assertEqual(r.table_size, 0)
        self.assertEqual(r.bucket_count, 0)
        self.assertEqual(r.displacements, ())
        self.assertEqual(r.slots, ())

    def test_single_key(self):
        keys, values = self._make_inputs(1)
        r = build_chd(keys, values)
        self.assertGreater(r.table_size, 0)
        self.assertGreaterEqual(r.bucket_count, 1)
        verify_chd(r)
        self.assertEqual(lookup_python(r, keys[0]), values[0])

    def test_small_3_keys(self):
        keys, values = self._make_inputs(3)
        r = build_chd(keys, values)
        verify_chd(r)
        for k, v in zip(keys, values):
            self.assertEqual(lookup_python(r, k), v)

    def test_100_keys_invariant(self):
        keys, values = self._make_inputs(100)
        r = build_chd(keys, values)
        verify_chd(r)
        for k, v in zip(keys, values):
            self.assertEqual(lookup_python(r, k), v)

    def test_880_keys_stress(self):
        # 88 blocks × 10 members — chapter 04 §5.6.4 N=880 reference case.
        keys = []
        values = []
        for b in range(88):
            block_name = f"Program_Block{b:02d}"
            block_hash = fnv1a_32(block_name)
            for m in range(10):
                keys.append(f"{block_name}::member_{m}")
                values.append(UniformLocation(
                    block_hash=block_hash,
                    offset=m * 16,
                    size=16,
                    cadence_tag=1,
                ))
        r = build_chd(keys, values)
        verify_chd(r)
        for k, v in zip(keys, values):
            self.assertEqual(lookup_python(r, k), v)

    def test_lookup_unknown_returns_none(self):
        keys, values = self._make_inputs(20)
        r = build_chd(keys, values)
        self.assertIsNone(lookup_python(r, "definitely_not_a_registered_key"))

    def test_duplicate_key_rejected(self):
        with self.assertRaises(CodegenError) as ctx:
            build_chd(
                ["FrameViewProj::view", "FrameViewProj::view"],
                [UniformLocation(0, 0, 64, 0), UniformLocation(0, 0, 64, 0)],
            )
        self.assertIn("duplicate", str(ctx.exception).lower())

    def test_length_mismatch_rejected(self):
        with self.assertRaises(CodegenError):
            build_chd(["a", "b"], [UniformLocation(0, 0, 4, 0)])

    def test_invalid_lambda_rejected(self):
        with self.assertRaises(CodegenError):
            build_chd(["a"], [UniformLocation(0, 0, 4, 0)], lambda_=0)

    def test_invalid_capacity_rejected(self):
        with self.assertRaises(CodegenError):
            build_chd(["a"], [UniformLocation(0, 0, 4, 0)], capacity_factor=0.5)

    def test_seed_exhaustion_raises(self):
        # Force exhaustion: a single bucket with many keys + a tiny seed budget.
        # 50 keys in ~57 slots with 1 seed try → almost-certain collision by birthday paradox.
        keys = [f"k{i:03d}" for i in range(50)]
        values = [UniformLocation(0, i, 4, 0) for i in range(50)]
        with self.assertRaises(CodegenError) as ctx:
            build_chd(keys, values, lambda_=50, max_displacement_seeds=1)
        msg = str(ctx.exception)
        self.assertIn("CHD seed search exhausted", msg)
        self.assertIn("Action:", msg)

    def test_deterministic_output(self):
        keys, values = self._make_inputs(50)
        r1 = build_chd(keys, values)
        r2 = build_chd(keys, values)
        self.assertEqual(r1.table_size, r2.table_size)
        self.assertEqual(r1.bucket_count, r2.bucket_count)
        self.assertEqual(r1.displacements, r2.displacements)
        self.assertEqual(r1.slots, r2.slots)


class VerifyCHDTests(unittest.TestCase):
    def test_verify_passes_on_valid_build(self):
        keys = ["a", "b", "c", "d", "e", "f", "g"]
        values = [UniformLocation(0, i, 4, 0) for i in range(len(keys))]
        verify_chd(build_chd(keys, values))

    def test_verify_detects_broken_slot(self):
        keys = ["a", "b", "c", "d"]
        values = [UniformLocation(0, i, 4, 0) for i in range(len(keys))]
        r = build_chd(keys, values)
        # Replace the first non-empty slot with a wrong-key payload.
        slots = list(r.slots)
        idx = next(i for i, s in enumerate(slots) if s is not None)
        slots[idx] = ("bogus", slots[idx][1])
        broken = CHDResult(
            table_size=r.table_size,
            bucket_count=r.bucket_count,
            base_seed=r.base_seed,
            displacements=r.displacements,
            slots=tuple(slots),
            keys=r.keys,
            values=r.values,
        )
        with self.assertRaises(CodegenError) as ctx:
            verify_chd(broken)
        self.assertIn("perfect hash collision", str(ctx.exception).lower())

    def test_verify_detects_empty_table_with_keys(self):
        broken = CHDResult(
            table_size=0,
            bucket_count=0,
            base_seed=DEFAULT_BASE_SEED,
            displacements=(),
            slots=(),
            keys=("a",),
            values=(UniformLocation(0, 0, 4, 0),),
        )
        with self.assertRaises(CodegenError):
            verify_chd(broken)


class CollectUniformKeysTests(unittest.TestCase):
    def test_two_blocks(self):
        b1 = _frame_view_proj_spec()
        b2 = BlockSpec(
            name="Program_GammaCorrect",
            layout=_layout(
                "Program_GammaCorrect",
                [MemberLayout(name="color", offset=0, size=16, align=16)],
                std140_size=16,
                block_size=256,
            ),
            cadence_tag=1,
            descriptor_set=1,
            binding=0,
            subset=0,
        )
        keys, values = collect_uniform_keys([b1, b2])
        self.assertEqual(
            keys,
            [
                "FrameViewProj::view",
                "FrameViewProj::proj",
                "FrameViewProj::view_proj",
                "Program_GammaCorrect::color",
            ],
        )
        # block_hash 共有: 同じ block 内 member は同じ block_hash
        self.assertEqual(values[0].block_hash, values[1].block_hash)
        self.assertEqual(values[1].block_hash, values[2].block_hash)
        self.assertNotEqual(values[0].block_hash, values[3].block_hash)
        # cadence_tag が BlockSpec から来る
        self.assertEqual(values[0].cadence_tag, 0)
        self.assertEqual(values[3].cadence_tag, 1)
        # offset / size が MemberLayout からそのまま
        self.assertEqual(values[2].offset, 128)
        self.assertEqual(values[2].size, 64)


class EmitTests(unittest.TestCase):
    def test_escape_cstring_basic(self):
        self.assertEqual(_escape_cstring("abc"), '"abc"')

    def test_escape_cstring_quote_and_backslash(self):
        self.assertEqual(_escape_cstring('a"b\\c'), '"a\\"b\\\\c"')

    def test_escape_cstring_non_ascii(self):
        self.assertEqual(_escape_cstring("\n"), '"\\x0a"')
        self.assertEqual(_escape_cstring("\x7f"), '"\\x7f"')

    def test_wrap_uint_array_groups_per_line(self):
        out = _wrap_uint_array(list(range(10)), per_line=4)
        self.assertEqual(len(out), 3)  # 4 + 4 + 2
        self.assertTrue(all(line.startswith("    ") for line in out))
        self.assertTrue(all(line.endswith(",") for line in out))

    def test_emit_perfect_hash_inl_contains_sections(self):
        b = _frame_view_proj_spec()
        out = emit_perfect_hash_inl([b])
        # (1) UniformLocation struct
        self.assertIn("struct UniformLocation", out)
        # (2) per-block <BlockName>Layout + <BlockName>_SIZE
        self.assertIn("struct FrameViewProjLayout", out)
        self.assertIn("static constexpr std::uint32_t view_OFFSET = 0u;", out)
        self.assertIn("static constexpr std::uint32_t view_proj_OFFSET = 128u;", out)
        self.assertIn("FrameViewProj_SIZE", out)
        # (3) BlockMetadata table
        self.assertIn("struct BlockMetadata", out)
        self.assertIn("g_block_metadata", out)
        self.assertIn("g_block_count", out)
        self.assertIn("lookup_block", out)
        # (4) CHD tables
        self.assertIn("g_chd_displacement", out)
        self.assertIn("g_chd_values", out)
        self.assertIn("g_chd_key_strings", out)
        self.assertIn("g_chd_table_size", out)
        self.assertIn("g_chd_bucket_count", out)
        # (5) lookup
        self.assertIn("constexpr std::uint32_t fnv1a_32", out)
        self.assertIn("const UniformLocation* lookup_runtime", out)
        # header
        self.assertIn("#pragma once", out)
        self.assertIn("#include <cstdint>", out)
        self.assertIn("#include <cstring>", out)
        self.assertIn("namespace ubo {", out)
        self.assertIn("} // namespace ubo", out)

    def test_emit_perfect_hash_inl_empty(self):
        out = emit_perfect_hash_inl([])
        # Framework still emitted; tables are empty sentinels.
        self.assertIn("struct UniformLocation", out)
        self.assertIn("g_block_count = 0u", out)
        self.assertIn("g_chd_table_size = 0u", out)
        self.assertIn("g_chd_bucket_count = 0u", out)
        self.assertIn("namespace ubo {", out)
        self.assertIn("} // namespace ubo", out)

    def test_emit_perfect_hash_inl_blocks_sorted_by_name(self):
        # Input intentionally in reverse-sorted order; emit must sort ascending (§7.1).
        b1 = _frame_view_proj_spec()
        b2 = BlockSpec(
            name="Program_AAA",
            layout=_layout(
                "Program_AAA",
                [MemberLayout(name="alpha", offset=0, size=16, align=16)],
            ),
            cadence_tag=1,
        )
        out = emit_perfect_hash_inl([b2, b1])  # reversed input order
        # 'F' < 'P' lexicographically → FrameViewProj struct comes first.
        i_fvp = out.find("struct FrameViewProjLayout")
        i_aaa = out.find("struct Program_AAALayout")
        self.assertGreaterEqual(i_fvp, 0)
        self.assertGreaterEqual(i_aaa, 0)
        self.assertLess(i_fvp, i_aaa)

    def test_emit_perfect_hash_inl_metadata_fields(self):
        b = BlockSpec(
            name="Program_Material",
            layout=_layout(
                "Program_Material",
                [
                    MemberLayout(name="kd", offset=0, size=16, align=16),
                    MemberLayout(name="ks", offset=16, size=16, align=16),
                ],
                std140_size=32,
                block_size=256,
            ),
            cadence_tag=1,
            descriptor_set=1,
            binding=7,
            subset=0,
        )
        out = emit_perfect_hash_inl([b])
        # Metadata literal includes binding 7u, block_size 256u, member_count 2u.
        self.assertIn('"Program_Material"', out)
        self.assertIn(" 256u,", out)  # block_size
        self.assertIn(" 7u,", out)    # binding
        self.assertIn(" 2u ",  out)   # member_count (trailing space before })

    def test_emit_perfect_hash_inl_keys_resolve(self):
        # End-to-end: emit, then Python-side simulate lookup for every member.
        b1 = _frame_view_proj_spec()
        b2 = BlockSpec(
            name="Program_GammaCorrect",
            layout=_layout(
                "Program_GammaCorrect",
                [MemberLayout(name="color", offset=0, size=16, align=16)],
            ),
            cadence_tag=1,
        )
        keys, values = collect_uniform_keys(sorted([b1, b2], key=lambda b: b.name))
        chd = build_chd(keys, values)
        verify_chd(chd)
        for k, v in zip(keys, values):
            self.assertEqual(lookup_python(chd, k), v)
        # Also emit smoke check (sorted order doesn't crash):
        self.assertIn("FrameViewProjLayout", emit_perfect_hash_inl([b1, b2]))


class EmitDummyInitTests(unittest.TestCase):
    """08 §8.2 stub: g_program_count=0, sentinel 1-element array."""

    def test_empty_blocks_stub(self):
        out = emit_dummy_init_inl([])
        self.assertIn("struct ProgramDummyMask", out)
        self.assertIn("g_program_count = 0u", out)
        self.assertIn("g_program_dummy_masks[1] = {", out)
        self.assertIn("nullptr, 0ull, 0ull", out)

    def test_non_empty_blocks_still_stub_at_phase_1a(self):
        # Phase 1.A intentionally ignores blocks (real enumeration is Phase 1.B+).
        b = _frame_view_proj_spec()
        out = emit_dummy_init_inl([b])
        self.assertIn("g_program_count = 0u", out)

    def test_struct_field_order_and_widths(self):
        out = emit_dummy_init_inl([])
        # program_name first, then subset_a, then subset_b
        prog_idx = out.find("program_name")
        a_idx = out.find("unused_subset_a_mask")
        b_idx = out.find("unused_subset_b_mask")
        self.assertGreater(a_idx, prog_idx)
        self.assertGreater(b_idx, a_idx)
        self.assertIn("std::uint64_t", out)

    def test_namespace_and_pragma_once(self):
        out = emit_dummy_init_inl([])
        self.assertIn("#pragma once", out)
        self.assertIn("namespace ubo {", out)
        self.assertIn("} // namespace ubo", out)
        self.assertIn('#include "ubo_metadata.inl"', out)


class EmitHostLoaderTests(unittest.TestCase):
    """08 §10.1 prefill_uniform_ubo_loc + §10.3 per-set binding counts (stub)."""

    def test_prefill_uniform_ubo_loc_signature(self):
        out = emit_host_loader_inl([])
        self.assertIn("inline void prefill_uniform_ubo_loc(", out)
        self.assertIn("const std::vector<std::string>& uniform_names", out)
        self.assertIn("std::vector<UniformLocation>& out_loc", out)
        self.assertIn("lookup_runtime(uniform_names[i].c_str())", out)

    def test_includes_required(self):
        out = emit_host_loader_inl([])
        for inc in ("<cstddef>", "<cstdint>", "<string>", "<vector>",
                    '"ubo_perfect_hash.inl"', '"ubo_metadata.inl"'):
            self.assertIn(inc, out, f"missing include {inc!r}")

    def test_per_set_binding_counts_empty(self):
        out = emit_host_loader_inl([])
        for sym in ("g_set0_binding_count  = 0u",
                    "g_set1a_binding_count = 0u",
                    "g_set1b_binding_count = 0u",
                    "g_set2_binding_count  = 0u",
                    "g_set3_binding_count  = 0u"):
            self.assertIn(sym, out)

    def test_per_set_binding_counts_distribution(self):
        # one block in each of set=0, set=1a, set=1b, set=2, set=3
        blocks = [
            BlockSpec(name="Frame0",   layout=_layout("Frame0",   []), cadence_tag=0,
                      descriptor_set=0, binding=0, subset=0),
            BlockSpec(name="Program1A",layout=_layout("Program1A",[]), cadence_tag=1,
                      descriptor_set=1, binding=0, subset=0),
            BlockSpec(name="Program1B",layout=_layout("Program1B",[]), cadence_tag=1,
                      descriptor_set=1, binding=0, subset=1),
            BlockSpec(name="Draw2",    layout=_layout("Draw2",    []), cadence_tag=2,
                      descriptor_set=2, binding=0, subset=0),
            BlockSpec(name="Asset3",   layout=_layout("Asset3",   []), cadence_tag=3,
                      descriptor_set=3, binding=0, subset=0),
        ]
        out = emit_host_loader_inl(blocks)
        for sym in ("g_set0_binding_count  = 1u",
                    "g_set1a_binding_count = 1u",
                    "g_set1b_binding_count = 1u",
                    "g_set2_binding_count  = 1u",
                    "g_set3_binding_count  = 1u"):
            self.assertIn(sym, out)

    def test_namespace_and_pragma_once(self):
        out = emit_host_loader_inl([])
        self.assertIn("#pragma once", out)
        self.assertIn("namespace ubo {", out)
        self.assertIn("} // namespace ubo", out)


class CountPerSetBindingsTests(unittest.TestCase):
    def test_subset_split_for_set1(self):
        b1a = BlockSpec(name="A", layout=_layout("A", []), cadence_tag=1,
                        descriptor_set=1, binding=0, subset=0)
        b1b = BlockSpec(name="B", layout=_layout("B", []), cadence_tag=1,
                        descriptor_set=1, binding=0, subset=1)
        counts = _count_per_set_bindings([b1a, b1b])
        self.assertEqual(counts["set1a"], 1)
        self.assertEqual(counts["set1b"], 1)
        self.assertEqual(counts["set0"], 0)

    def test_unknown_set_ignored(self):
        # Defensive: a stray descriptor_set value should not crash, just drop.
        odd = BlockSpec(name="X", layout=_layout("X", []), cadence_tag=5,
                       descriptor_set=99, binding=0, subset=0)
        counts = _count_per_set_bindings([odd])
        self.assertEqual(sum(counts.values()), 0)


class EmitIndexAggregateTests(unittest.TestCase):
    def test_index_includes_all_phase1a_outputs(self):
        out = emit_index_inl([_frame_view_proj_spec()])
        for inc in ('#include "ubo_layout_frameviewproj.inl"',
                    '#include "ubo_metadata.inl"',
                    '#include "ubo_perfect_hash.inl"',
                    '#include "ubo_dummy_init.inl"',
                    '#include "ubo_host_loader.inl"'):
            self.assertIn(inc, out, f"missing include line: {inc!r}")


if __name__ == "__main__":
    unittest.main()
