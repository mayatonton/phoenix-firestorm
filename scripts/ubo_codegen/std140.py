# AYAstorm r41 UBO Codegen — std140 offset calculator (A1a)
# Spec: docs/specs/ayastorm-r41-gl-removal/design/04-codegen-ubo.md §4.3 / §4.3.1.*
"""std140 layout calculator.

Implements GLSL spec 7.6.2.2 std140 per chapter 04 §4.3.1:
    - base alignment / size decision table (§4.3.1.1)
    - per-block offset state machine (§4.3.1.2)
    - nested struct recursion (§4.3.1.3)
    - array stride (§4.3.1.4, the corner case)
    - trailing block padding (§4.3.1.5) then 256 B device padding (§6.4)

Pure Python; no I/O, no subprocess. Consumed by main.py and verified
against spirv_reflect.py output (§5.4.1 二重保証).
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import List, Optional, Sequence, Tuple

from codegen_error import CodegenError, format_error

DEVICE_OFFSET_ALIGN = 256  # chapter 07 §3.1 maxUniformBufferOffsetAlignment 最大値 (§6.4)
STRUCT_ALIGN_FLOOR = 16    # std140: struct / array base alignment は 16 以上


@dataclass(frozen=True)
class TypeInfo:
    """std140 base alignment / base size for a single member type."""
    base_align: int
    base_size: int
    is_array: bool = False
    array_count: int = 1
    array_stride: int = 0
    is_struct: bool = False
    struct_layout: tuple = ()  # tuple of MemberLayout dicts when is_struct


@dataclass
class MemberLayout:
    name: str
    offset: int
    size: int
    align: int
    array_stride: int = 0          # 0 = non-array
    nested_layout: tuple = ()      # nested struct expanded layout (informational)


@dataclass
class BlockLayout:
    name: str
    members: List[MemberLayout] = field(default_factory=list)
    std140_size: int = 0           # § §4.3.1.5 末尾 padding 後
    block_size: int = 0            # = pad_to_device_align(std140_size) (§6.4)


# --- decision table ---------------------------------------------------------

PRIMITIVE_TYPES = {
    # scalars
    "float": TypeInfo(4, 4),
    "int":   TypeInfo(4, 4),
    "uint":  TypeInfo(4, 4),
    "bool":  TypeInfo(4, 4),
    # vec2 family
    "vec2":  TypeInfo(8, 8),
    "ivec2": TypeInfo(8, 8),
    "uvec2": TypeInfo(8, 8),
    "bvec2": TypeInfo(8, 8),
    # vec3 family — std140 base size 12, align 16 (= trailing 4 B hole)
    "vec3":  TypeInfo(16, 12),
    "ivec3": TypeInfo(16, 12),
    "uvec3": TypeInfo(16, 12),
    "bvec3": TypeInfo(16, 12),
    # vec4 family
    "vec4":  TypeInfo(16, 16),
    "ivec4": TypeInfo(16, 16),
    "uvec4": TypeInfo(16, 16),
    "bvec4": TypeInfo(16, 16),
    # square mats — column-major, column stride 16
    "mat2":   TypeInfo(16, 32),
    "mat3":   TypeInfo(16, 48),
    "mat4":   TypeInfo(16, 64),
    "mat2x2": TypeInfo(16, 32),
    "mat3x3": TypeInfo(16, 48),
    "mat4x4": TypeInfo(16, 64),
    # rectangular mats — column count × column stride (column = vecN rounded up to vec4)
    "mat2x3": TypeInfo(16, 32),    # 2 columns × stride 16
    "mat2x4": TypeInfo(16, 32),
    "mat3x2": TypeInfo(16, 48),    # 3 columns × stride 16
    "mat3x4": TypeInfo(16, 48),
    "mat4x2": TypeInfo(16, 64),
    "mat4x3": TypeInfo(16, 64),
}


# --- utilities --------------------------------------------------------------

def round_up(x: int, align: int) -> int:
    if align <= 0:
        raise CodegenError(format_error(
            f"round_up alignment must be positive, got {align}",
            action="report this as an internal codegen bug",
        ))
    return (x + align - 1) // align * align


def pad_to_device_align(size: int) -> int:
    """chapter 08 §6.4 = 256 B multiple round up."""
    return round_up(size, DEVICE_OFFSET_ALIGN)


def arrayify(ti: TypeInfo, count: int) -> TypeInfo:
    """§4.3.1.4 — array element stride is max(base_size, vec4)."""
    if count < 1:
        raise CodegenError(format_error(
            f"array count must be ≥ 1, got {count}",
            action="check GLSL array size literal",
        ))
    stride = round_up(ti.base_size, STRUCT_ALIGN_FLOOR)
    return TypeInfo(
        base_align=max(ti.base_align, STRUCT_ALIGN_FLOOR),
        base_size=stride * count,
        is_array=True,
        array_count=count,
        array_stride=stride,
        is_struct=ti.is_struct,
        struct_layout=ti.struct_layout,
    )


# --- member descriptor for the public API -----------------------------------

@dataclass
class MemberSpec:
    """Input to compute_layout. Decouples std140 from the parser's Member type."""
    name: str
    type_str: str
    array_count: Optional[int] = None        # None or ≥ 1
    nested_members: Optional[Sequence["MemberSpec"]] = None
    # Source info — passed through to error messages.
    glsl_file: Optional[str] = None
    glsl_line: Optional[int] = None


# --- core algorithm ---------------------------------------------------------

def compute_struct_type_info(members: Sequence[MemberSpec]) -> TypeInfo:
    """§4.3.1.3 — recurse into a nested struct, returning its TypeInfo."""
    layout = compute_layout("<struct>", members)
    base_align = STRUCT_ALIGN_FLOOR
    for m in layout.members:
        if m.align > base_align:
            base_align = m.align
    return TypeInfo(
        base_align=base_align,
        base_size=layout.std140_size,
        is_struct=True,
        struct_layout=tuple(layout.members),
    )


def compute_layout(block_name: str, members: Sequence[MemberSpec]) -> BlockLayout:
    """§4.3.1.2 — walk members, emit MemberLayout list and block_size.

    `block_name` is informational (error messages only).
    """
    layout = BlockLayout(name=block_name)
    offset = 0
    max_align = STRUCT_ALIGN_FLOOR

    for spec in members:
        ti = _resolve_type_info(spec, block_name)
        if spec.array_count is not None:
            ti = arrayify(ti, spec.array_count)

        align = ti.base_align
        offset = round_up(offset, align)

        member = MemberLayout(
            name=spec.name,
            offset=offset,
            size=ti.base_size,
            align=align,
            array_stride=ti.array_stride if ti.is_array else 0,
            nested_layout=ti.struct_layout if ti.is_struct else (),
        )
        layout.members.append(member)

        offset += ti.base_size
        if align > max_align:
            max_align = align

    layout.std140_size = round_up(offset, max_align)
    layout.block_size = pad_to_device_align(layout.std140_size)
    return layout


def _resolve_type_info(spec: MemberSpec, block_name: str) -> TypeInfo:
    if spec.nested_members is not None:
        return compute_struct_type_info(spec.nested_members)
    if spec.type_str in PRIMITIVE_TYPES:
        return PRIMITIVE_TYPES[spec.type_str]
    raise CodegenError(format_error(
        f"unsupported std140 type '{spec.type_str}'",
        glsl_file=spec.glsl_file, line=spec.glsl_line,
        block=block_name, member=spec.name,
        reason="LL AYAstorm GLSL 慣用範囲外 type, std140 layout 計算未対応",
        action="chapter 05 集約表で型変換または chapter 08 §17 で type 追加判断",
    ))
