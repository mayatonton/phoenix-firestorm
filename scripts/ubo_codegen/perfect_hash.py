# AYAstorm r41 UBO Codegen — perfect hash CHD generator (G/B3)
# Spec: docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md §5.3 / §5.6
#       docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md §6 / §7
"""Perfect hash (CHD) generator for UBO member-name → UniformLocation dispatch.

Implements the Belazzougui/Botelho/Dietzfelbinger 2009 "Compress, Hash, Displace"
algorithm per chapter 04 §5.6.2 / §5.6.3:

    - 2-stage hash: bucket g(key) % M → displacement d_i → final (h ⊕ d) % N
    - per-bucket seed reroll, sorted size-descending (§5.6.3 step 3)
    - invariant check (§5.6.5) — every input key resolves to its own slot
    - C++ constexpr emit (§5.6.6) with parallel key-string table for
      false-positive排除 (= unknown name hashing to an occupied slot)

Output sections (chapter 04 §5.1 + §5.2 + §5.6.6 + chapter 08 §6 / §7.1):
    (1) UniformLocation struct (§5.3.2)
    (2) per-block <BlockName>Layout struct + <BlockName>_SIZE (§5.2)
    (3) block_name → BlockMetadata constexpr table (§6 / §7.1, sorted by name)
    (4) CHD displacement / values / key-string tables (§5.6.6)
    (5) constexpr fnv1a_32 + lookup_runtime (§5.6.6)

PA-5 produces the emit string only; PA-6 wires actual file writing + cache.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, List, Optional, Sequence, Tuple

from codegen_error import CodegenError, format_error
from std140 import BlockLayout, MemberLayout

# --- constants --------------------------------------------------------------

FNV_OFFSET_BASIS = 0x811C9DC5
FNV_PRIME = 0x01000193
HASH_MASK_32 = 0xFFFFFFFF

DEFAULT_LAMBDA = 4                   # group size, chapter 04 §5.6.3
DEFAULT_CAPACITY_FACTOR = 1.1        # ε = 0.1, chapter 04 §5.6.3
DEFAULT_BASE_SEED = FNV_OFFSET_BASIS
MAX_DISPLACEMENT_SEEDS = 1 << 20     # 1M reroll cap — fails fast on pathological inputs


# --- hash helpers -----------------------------------------------------------

def fnv1a_32(s: str, seed: int = FNV_OFFSET_BASIS) -> int:
    """FNV-1a 32-bit. Mirrors the C++ constexpr emitted in §5.6.6 exactly."""
    h = seed & HASH_MASK_32
    for b in s.encode("utf-8"):
        h ^= b
        h = (h * FNV_PRIME) & HASH_MASK_32
    return h


def next_prime(n: int) -> int:
    """Smallest prime ≥ n. Co-primes the table size with FNV-1a stride."""
    if n < 2:
        return 2
    if _is_prime(n):
        return n
    if n % 2 == 0:
        n += 1
    while not _is_prime(n):
        n += 2
    return n


def _is_prime(n: int) -> bool:
    if n < 2:
        return False
    if n < 4:
        return True
    if n % 2 == 0:
        return False
    i = 3
    while i * i <= n:
        if n % i == 0:
            return False
        i += 2
    return True


# --- data types -------------------------------------------------------------

@dataclass(frozen=True)
class UniformLocation:
    """C++ side `struct UniformLocation` (§5.3.2 / §5.6.6) — 4 × uint32_t."""
    block_hash: int
    offset: int
    size: int
    cadence_tag: int


@dataclass(frozen=True)
class BlockSpec:
    """One UBO block + its computed std140 layout + binding metadata.

    `cadence_tag` / `descriptor_set` / `binding` / `subset` come from chapter 02
    §2.1 block-name parsing and chapter 07 §4.4 set assignment; PA-5 just consumes
    them (= no scope leak into routing decisions).
    """
    name: str
    layout: BlockLayout
    cadence_tag: int
    descriptor_set: int = 0
    binding: int = 0
    subset: int = 0


@dataclass(frozen=True)
class CHDResult:
    """Output of build_chd. Consumed by emit_perfect_hash_inl + verify_chd."""
    table_size: int
    bucket_count: int
    base_seed: int
    displacements: Tuple[int, ...]
    # slot i = (key, value) or None — None means "empty slot" in the CHD table.
    slots: Tuple[Optional[Tuple[str, UniformLocation]], ...]
    keys: Tuple[str, ...]                 # input order
    values: Tuple[UniformLocation, ...]   # input order, parallel to keys


# --- CHD construction --------------------------------------------------------

def build_chd(
    keys: Sequence[str],
    values: Sequence[UniformLocation],
    *,
    lambda_: int = DEFAULT_LAMBDA,
    capacity_factor: float = DEFAULT_CAPACITY_FACTOR,
    base_seed: int = DEFAULT_BASE_SEED,
    max_displacement_seeds: int = MAX_DISPLACEMENT_SEEDS,
) -> CHDResult:
    """Belazzougui/Botelho/Dietzfelbinger 2009 CHD — see chapter 04 §5.6.3.

    Raises CodegenError on:
        - len(keys) != len(values)
        - duplicate input key (= §9.4 E4 collision)
        - per-bucket seed search exhaustion (§5.6.5 invariant 2)
    """
    if len(keys) != len(values):
        raise CodegenError(format_error(
            "perfect_hash: keys/values length mismatch",
            reason=f"len(keys)={len(keys)}, len(values)={len(values)}",
            action="report this as an internal codegen bug",
        ))
    if lambda_ < 1:
        raise CodegenError(format_error(
            f"perfect_hash: lambda_ must be ≥ 1, got {lambda_}",
            action="raise lambda_ to 4 (default) or higher",
        ))
    if capacity_factor < 1.0:
        raise CodegenError(format_error(
            f"perfect_hash: capacity_factor must be ≥ 1.0, got {capacity_factor}",
            action="raise capacity_factor to 1.1 (default) or higher to give CHD breathing room",
        ))

    n = len(keys)
    if n == 0:
        return CHDResult(
            table_size=0,
            bucket_count=0,
            base_seed=base_seed,
            displacements=(),
            slots=(),
            keys=(),
            values=(),
        )

    # Duplicate detection — CHD assumes unique keys.
    seen: Dict[str, int] = {}
    for i, k in enumerate(keys):
        if k in seen:
            raise CodegenError(format_error(
                "perfect hash collision detected (duplicate input key)",
                extra_lines=[
                    f"Key 1: {k} (input index {seen[k]})",
                    f"Key 2: {k} (input index {i})",
                ],
                reason="duplicate uniform name in the CHD input set",
                action=(
                    "resolve duplicate via chapter 02 §2.4 — every (block, member) pair "
                    "must be unique within the codegen input"
                ),
            ))
        seen[k] = i

    table_size = next_prime(max(int(n * capacity_factor), n + 1))
    bucket_count = max(1, n // lambda_)

    # Step 1: route keys into buckets via outer hash g(key)
    buckets: List[List[int]] = [[] for _ in range(bucket_count)]
    for i, k in enumerate(keys):
        b = fnv1a_32(k, base_seed) % bucket_count
        buckets[b].append(i)

    # Step 2: process buckets from largest to smallest (§5.6.3 step 3) — large
    # buckets get first pick at slots; small buckets fill in around them.
    bucket_order = sorted(range(bucket_count), key=lambda i: -len(buckets[i]))

    displacements: List[int] = [0] * bucket_count
    slot_filled: List[bool] = [False] * table_size
    slot_payload: List[Optional[Tuple[str, UniformLocation]]] = [None] * table_size

    for bi in bucket_order:
        members = buckets[bi]
        if not members:
            displacements[bi] = 0
            continue

        chosen_d: Optional[int] = None
        chosen_slots: List[int] = []
        for d in range(max_displacement_seeds):
            candidate: List[int] = []
            within: set = set()
            ok = True
            for ki in members:
                h = fnv1a_32(keys[ki], base_seed ^ d) % table_size
                if slot_filled[h] or h in within:
                    ok = False
                    break
                within.add(h)
                candidate.append(h)
            if ok:
                chosen_d = d
                chosen_slots = candidate
                break

        if chosen_d is None:
            raise CodegenError(format_error(
                "CHD seed search exhausted",
                extra_lines=[
                    f"Bucket: {bi}",
                    f"Bucket size: {len(members)}",
                    f"Table size: {table_size}, bucket_count: {bucket_count}",
                    f"Tried {max_displacement_seeds} seeds",
                ],
                reason="bucket cannot be placed in the current table without collision",
                action=(
                    "raise capacity_factor (1.1 → 1.2) or lower lambda_ (4 → 3); "
                    "if the input has many keys sharing a common prefix, consider key salting"
                ),
            ))

        for ki, h in zip(members, chosen_slots):
            slot_filled[h] = True
            slot_payload[h] = (keys[ki], values[ki])
        displacements[bi] = chosen_d

    return CHDResult(
        table_size=table_size,
        bucket_count=bucket_count,
        base_seed=base_seed,
        displacements=tuple(displacements),
        slots=tuple(slot_payload),
        keys=tuple(keys),
        values=tuple(values),
    )


# --- invariant check (§5.6.5) -----------------------------------------------

def verify_chd(result: CHDResult) -> None:
    """§5.6.5 — every input key resolves to its own slot via the emitted lookup."""
    if result.table_size == 0:
        if result.keys or result.values:
            raise CodegenError(format_error(
                "perfect_hash invariant: empty table but non-empty keys/values",
                action="report this as an internal codegen bug",
            ))
        return

    for k in result.keys:
        bi = fnv1a_32(k, result.base_seed) % result.bucket_count
        d = result.displacements[bi]
        h = fnv1a_32(k, result.base_seed ^ d) % result.table_size
        slot = result.slots[h]
        if slot is None:
            raise CodegenError(format_error(
                "perfect_hash invariant: empty slot at resolved index",
                extra_lines=[f"Key: {k}", f"Bucket: {bi}, displacement: {d}, slot: {h}"],
                action="report this as an internal codegen bug",
            ))
        if slot[0] != k:
            raise CodegenError(format_error(
                "perfect hash collision detected",
                extra_lines=[f"Key 1: {slot[0]}", f"Key 2: {k}"],
                reason="lookup invariant broken — two keys resolve to the same slot",
                action=(
                    "this signals a CHD construction bug; rebuild with a higher "
                    "capacity_factor or report as an internal codegen bug"
                ),
            ))


def lookup_python(result: CHDResult, name: str) -> Optional[UniformLocation]:
    """Python-side mirror of the emitted C++ lookup_runtime — for tests + diagnostics."""
    if result.table_size == 0 or result.bucket_count == 0:
        return None
    bi = fnv1a_32(name, result.base_seed) % result.bucket_count
    d = result.displacements[bi]
    h = fnv1a_32(name, result.base_seed ^ d) % result.table_size
    slot = result.slots[h]
    if slot is None or slot[0] != name:
        return None
    return slot[1]


# --- input preparation -------------------------------------------------------

def collect_uniform_keys(
    blocks: Sequence[BlockSpec],
) -> Tuple[List[str], List[UniformLocation]]:
    """Flatten the (block, member) namespace into parallel key/value lists.

    Top-level members only — nested struct expansion is by design (§5.6): the
    per-block <BlockName>Layout struct already exposes nested layout informationally,
    and lookup_runtime returns a UniformLocation for the top-level field.
    """
    keys: List[str] = []
    values: List[UniformLocation] = []
    for spec in blocks:
        block_hash = fnv1a_32(spec.name)
        for m in spec.layout.members:
            keys.append(_uniform_key(spec.name, m.name))
            values.append(UniformLocation(
                block_hash=block_hash,
                offset=m.offset,
                size=m.size,
                cadence_tag=spec.cadence_tag,
            ))
    return keys, values


# --- C++ emit ---------------------------------------------------------------

_HEADER_COMBINED = """\
// AYAstorm r41 UBO Codegen — perfect hash + block layouts
// Generated by scripts/ubo_codegen/perfect_hash.py — DO NOT EDIT
// Spec: docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md §5.6 / §5.6.6

#pragma once

#include <cstdint>
#include <cstring>
"""

# PA-6 split emit headers — one self-contained header per generated file.
_HEADER_LAYOUT = """\
// AYAstorm r41 UBO Codegen — block layout (one block)
// Generated by scripts/ubo_codegen/perfect_hash.py — DO NOT EDIT
// Spec: docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md §5.1 / §5.2

#pragma once

#include <cstdint>
"""

_HEADER_METADATA = """\
// AYAstorm r41 UBO Codegen — block metadata dispatch table
// Generated by scripts/ubo_codegen/perfect_hash.py — DO NOT EDIT
// Spec: docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md §5.1
//       docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md §6 / §7.1

#pragma once

#include <cstdint>
#include <cstring>
"""

_HEADER_PERFECT_HASH = """\
// AYAstorm r41 UBO Codegen — CHD perfect hash table
// Generated by scripts/ubo_codegen/perfect_hash.py — DO NOT EDIT
// Spec: docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md §5.1 / §5.6

#pragma once

#include <cstdint>
#include <cstring>
"""

_HEADER_INDEX = """\
// AYAstorm r41 UBO Codegen — aggregate include (setter-side entry point)
// Generated by scripts/ubo_codegen/perfect_hash.py — DO NOT EDIT
// Spec: docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md §5.1

#pragma once
"""


def emit_perfect_hash_inl(blocks: Sequence[BlockSpec]) -> str:
    """Backwards-compatible single-string emit (PA-5 API).

    Sections (in order):
        (1) UniformLocation struct
        (2) per-block <BlockName>Layout struct + <BlockName>_SIZE constant
        (3) block_name → BlockMetadata constexpr table (sorted by name = §7.1)
        (4) CHD displacement + values + key-string tables
        (5) constexpr fnv1a_32 + lookup_runtime functions

    PA-6 wires individual emit_{layout,metadata,perfect_hash,index}_inl
    for the 4-file split contract (04 §5.1); this combined emitter is kept
    for unit tests and as the canonical reference of section order.
    """
    sorted_blocks = sorted(blocks, key=lambda b: b.name)
    keys, values = collect_uniform_keys(sorted_blocks)
    chd = build_chd(keys, values)
    verify_chd(chd)

    parts: List[str] = [_HEADER_COMBINED, "namespace ubo {", ""]
    parts.append(_emit_uniform_location_struct())
    parts.append(_emit_block_layouts(sorted_blocks))
    parts.append(_emit_block_metadata_table(sorted_blocks))
    parts.append(_emit_chd_tables(chd))
    parts.append(_emit_lookup_functions(chd))
    parts.append("} // namespace ubo")
    return "\n".join(parts) + "\n"


# --- PA-6 4-file split emit -------------------------------------------------

def emit_layout_inl(block: BlockSpec) -> str:
    """One `ubo_layout_<blockname>.inl` (= 04 §5.1 row 1, §5.2)."""
    parts: List[str] = [_HEADER_LAYOUT, "namespace ubo {", ""]
    parts.append(_emit_block_layouts([block]))
    parts.append("} // namespace ubo")
    return "\n".join(parts) + "\n"


def emit_metadata_inl(blocks: Sequence[BlockSpec]) -> str:
    """`ubo_metadata.inl` (= 04 §5.1 row 3, 08 §6 / §7.1)."""
    sorted_blocks = sorted(blocks, key=lambda b: b.name)
    parts: List[str] = [_HEADER_METADATA, "namespace ubo {", ""]
    parts.append(_emit_block_metadata_table(sorted_blocks))
    parts.append("} // namespace ubo")
    return "\n".join(parts) + "\n"


def emit_perfect_hash_inl_split(blocks: Sequence[BlockSpec]) -> str:
    """`ubo_perfect_hash.inl` (= 04 §5.1 row 2, §5.6.6).

    Self-contained: UniformLocation struct + CHD tables + lookup_runtime,
    so callers may include only this file when they need name dispatch.
    """
    sorted_blocks = sorted(blocks, key=lambda b: b.name)
    keys, values = collect_uniform_keys(sorted_blocks)
    chd = build_chd(keys, values)
    verify_chd(chd)

    parts: List[str] = [_HEADER_PERFECT_HASH, "namespace ubo {", ""]
    parts.append(_emit_uniform_location_struct())
    parts.append(_emit_chd_tables(chd))
    parts.append(_emit_lookup_functions(chd))
    parts.append("} // namespace ubo")
    return "\n".join(parts) + "\n"


def emit_index_inl(blocks: Sequence[BlockSpec]) -> str:
    """`ubo_index.inl` aggregating every generated header (= 04 §5.1 row 4).

    Setter-side TUs include this single file and get layouts + metadata +
    perfect hash dispatch all at once.
    """
    sorted_blocks = sorted(blocks, key=lambda b: b.name)
    lines: List[str] = [_HEADER_INDEX]
    for spec in sorted_blocks:
        lines.append(f'#include "ubo_layout_{spec.name.lower()}.inl"')
    lines.append('#include "ubo_metadata.inl"')
    lines.append('#include "ubo_perfect_hash.inl"')
    lines.append("")
    return "\n".join(lines) + "\n"


def _emit_uniform_location_struct() -> str:
    return (
        "// (1) §5.3.2 — UniformLocation = compile-time perfect-hash value payload\n"
        "struct UniformLocation {\n"
        "    std::uint32_t block_hash;   // FNV-1a of block_name\n"
        "    std::uint32_t offset;       // byte offset within the block\n"
        "    std::uint32_t size;         // value size in bytes (= write量)\n"
        "    std::uint32_t cadence_tag;  // 0:PerFrame 1:PerProgram 2:PerDraw 3:PerAsset 4:PerSkin 5:Singleton\n"
        "};\n"
    )


def _emit_block_layouts(blocks: Sequence[BlockSpec]) -> str:
    lines: List[str] = ["// (2) §5.2 — <BlockName>Layout + <BlockName>_SIZE constants"]
    for spec in blocks:
        lines.append(f"struct {spec.name}Layout {{")
        for m in spec.layout.members:
            lines.append(
                f"    static constexpr std::uint32_t {m.name}_OFFSET = {m.offset}u;"
                f"{_layout_member_comment(m)}"
            )
        lines.append("};")
        lines.append(
            f"inline constexpr std::uint32_t {spec.name}_SIZE = {spec.layout.block_size}u; "
            f"// std140={spec.layout.std140_size}, device-padded={spec.layout.block_size}"
        )
        lines.append("")
    return "\n".join(lines)


def _layout_member_comment(m: MemberLayout) -> str:
    bits = [f"size={m.size}", f"align={m.align}"]
    if m.array_stride:
        bits.append(f"stride={m.array_stride}")
    return "  // " + " ".join(bits)


def _emit_block_metadata_table(blocks: Sequence[BlockSpec]) -> str:
    lines: List[str] = [
        "// (3) §6.1 / §7.1 — block_name → BlockMetadata, sorted by name (deterministic dispatch)",
        "struct BlockMetadata {",
        "    const char*   block_name;",
        "    std::uint32_t block_hash;",
        "    std::uint32_t block_size;",
        "    std::uint16_t descriptor_set;",
        "    std::uint16_t binding;",
        "    std::uint16_t subset;",
        "    std::uint16_t cadence_tag;",
        "    std::uint16_t member_count;",
        "};",
        f"inline constexpr std::uint32_t g_block_count = {len(blocks)}u;",
    ]
    if not blocks:
        # C++ forbids zero-sized arrays; emit a 1-element sentinel + count=0.
        lines.append(
            "inline constexpr BlockMetadata g_block_metadata[1] = "
            "{ { nullptr, 0u, 0u, 0u, 0u, 0u, 0u, 0u } };"
        )
    else:
        lines.append(f"inline constexpr BlockMetadata g_block_metadata[{len(blocks)}] = {{")
        for spec in blocks:
            block_hash = fnv1a_32(spec.name)
            lines.append(
                "    { "
                f"{_escape_cstring(spec.name)}, 0x{block_hash:08x}u, {spec.layout.block_size}u, "
                f"{spec.descriptor_set}u, {spec.binding}u, {spec.subset}u, "
                f"{spec.cadence_tag}u, {len(spec.layout.members)}u "
                "},"
            )
        lines.append("};")
    lines.append("")
    lines.append("inline const BlockMetadata* lookup_block(const char* name) {")
    lines.append("    for (std::uint32_t i = 0; i < g_block_count; ++i) {")
    lines.append("        if (g_block_metadata[i].block_name == nullptr) continue;")
    lines.append("        if (std::strcmp(g_block_metadata[i].block_name, name) == 0) {")
    lines.append("            return &g_block_metadata[i];")
    lines.append("        }")
    lines.append("    }")
    lines.append("    return nullptr;")
    lines.append("}")
    lines.append("")
    return "\n".join(lines)


def _emit_chd_tables(chd: CHDResult) -> str:
    lines: List[str] = [
        "// (4) §5.6.6 — CHD displacement + value + key-string tables",
        f"inline constexpr std::uint32_t g_chd_table_size = {chd.table_size}u;",
        f"inline constexpr std::uint32_t g_chd_bucket_count = {chd.bucket_count}u;",
        f"inline constexpr std::uint32_t g_chd_base_seed = 0x{chd.base_seed:08x}u;",
        f"inline constexpr std::uint32_t g_chd_entry_count = {len(chd.keys)}u;",
    ]

    if chd.table_size == 0:
        lines.append("// (table is empty — no CHD entries)")
        lines.append(
            "inline constexpr std::uint32_t g_chd_displacement[1] = { 0u };"
        )
        lines.append(
            "inline constexpr UniformLocation g_chd_values[1] = { { 0u, 0u, 0u, 0u } };"
        )
        lines.append(
            "inline constexpr const char* g_chd_key_strings[1] = { nullptr };"
        )
        lines.append("")
        return "\n".join(lines)

    lines.append(f"inline constexpr std::uint32_t g_chd_displacement[{chd.bucket_count}] = {{")
    lines.extend(_wrap_uint_array(chd.displacements))
    lines.append("};")

    lines.append(f"inline constexpr UniformLocation g_chd_values[{chd.table_size}] = {{")
    for slot in chd.slots:
        if slot is None:
            lines.append("    { 0u, 0u, 0u, 0u },")
        else:
            _, v = slot
            lines.append(
                f"    {{ 0x{v.block_hash:08x}u, {v.offset}u, {v.size}u, {v.cadence_tag}u }},"
            )
    lines.append("};")

    lines.append(f"inline constexpr const char* g_chd_key_strings[{chd.table_size}] = {{")
    for slot in chd.slots:
        if slot is None:
            lines.append("    nullptr,")
        else:
            k, _ = slot
            lines.append(f"    {_escape_cstring(k)},")
    lines.append("};")
    lines.append("")
    return "\n".join(lines)


def _wrap_uint_array(values: Sequence[int], per_line: int = 8) -> List[str]:
    out: List[str] = []
    buf: List[str] = []
    for v in values:
        buf.append(f"0x{v:08x}u")
        if len(buf) == per_line:
            out.append("    " + ", ".join(buf) + ",")
            buf = []
    if buf:
        out.append("    " + ", ".join(buf) + ",")
    return out


def _emit_lookup_functions(chd: CHDResult) -> str:
    lines = [
        "// (5) §5.6.6 — constexpr fnv1a_32 + lookup_runtime with false-positive排除",
        f"constexpr std::uint32_t fnv1a_32(const char* s, std::uint32_t seed = 0x{FNV_OFFSET_BASIS:08x}u) {{",
        "    std::uint32_t h = seed;",
        "    while (*s) {",
        "        h ^= static_cast<std::uint8_t>(*s);",
        f"        h *= 0x{FNV_PRIME:08x}u;",
        "        ++s;",
        "    }",
        "    return h;",
        "}",
        "",
        "inline const UniformLocation* lookup_runtime(const char* name) {",
        "    if (g_chd_bucket_count == 0u || g_chd_table_size == 0u) {",
        "        return nullptr;",
        "    }",
        "    const std::uint32_t bi = fnv1a_32(name, g_chd_base_seed) % g_chd_bucket_count;",
        "    const std::uint32_t d  = g_chd_displacement[bi];",
        "    const std::uint32_t h  = fnv1a_32(name, g_chd_base_seed ^ d) % g_chd_table_size;",
        "    const char* slot_key = g_chd_key_strings[h];",
        "    if (slot_key == nullptr) return nullptr;",
        "    if (std::strcmp(slot_key, name) != 0) return nullptr;",
        "    return &g_chd_values[h];",
        "}",
    ]
    return "\n".join(lines)


# --- private helpers --------------------------------------------------------

def _uniform_key(block: str, member: str) -> str:
    return f"{block}::{member}"


def _escape_cstring(s: str) -> str:
    out: List[str] = []
    for ch in s:
        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif 0x20 <= ord(ch) < 0x7F:
            out.append(ch)
        else:
            out.append(f"\\x{ord(ch):02x}")
    return '"' + "".join(out) + '"'
