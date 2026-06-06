# AYAstorm r41 UBO Codegen — GLSL mini-parser (P3)
# Spec: docs/specs/ayastorm-r41-gl-removal/design/08-build-codegen-pipeline.md §5.2 / §5.2.1.*
"""GLSL mini-parser for preprocessed UBO blueprints.

Input  = `glslangValidator -E` stdout (string), with `#line N "path"`
         directives preserved. 入力 source = `aya_r41_blueprints/` 配下 .glsl
         (= codegen 入力 source of truth、Phase 2.α 案 X 確定 2026-06-06、
         `aya_r41_blueprints/README.md` §0-§1 参照)、または `--verify-target-paths`
         経由で actual class*/ + cinematic_bd/ 配下 .glsl (= AYAstorm shader runtime
         compile target、二重 source 同期 verify 用)。
Output = (ubo_blocks, struct_defs, bare_uniforms, sampler_decls)

Grammar covered (§5.2.1.2 EBNF):
    program        = { top_level_decl } ;
    top_level_decl = ubo_block | struct_def | bare_uniform | sampler_decl | other_skip ;
    ubo_block      = "layout" "(" layout_qual ")" "uniform" ident "{" { member_decl } "}" [ ident ] ";" ;
    struct_def     = "struct" ident "{" { member_decl } "}" ";" ;
    bare_uniform   = "uniform" type_ident ident [ "[" int_lit "]" ] ";" ;
    sampler_decl   = "uniform" sampler_type ident ";" ;

Unsupported constructs surface as CodegenError per §5.2.1.6.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from typing import List, Optional, Tuple

from codegen_error import CodegenError, format_error


# --- token / decl types -----------------------------------------------------

@dataclass
class Member:
    name: str
    type_str: str                # e.g. 'float', 'mat4', 'vec3', or a struct name
    array_count: Optional[int] = None   # None = non-array; int = explicit size (>=1)
    nested_struct: Optional["StructDef"] = None  # set when type_str matches a struct_def
    source_file: str = "<input>"
    source_line: int = 0


@dataclass
class UboBlockDecl:
    block_name: str
    layout_qual: dict
    members: List[Member]
    source_file: str = "<input>"
    source_line: int = 0
    instance_name: str = ""


@dataclass
class StructDef:
    name: str
    members: List[Member]
    source_file: str = "<input>"
    source_line: int = 0


@dataclass
class BareUniformDecl:
    type_str: str
    name: str
    array_count: Optional[int] = None
    source_file: str = "<input>"
    source_line: int = 0


@dataclass
class SamplerDecl:
    sampler_type: str
    name: str
    source_file: str = "<input>"
    source_line: int = 0


@dataclass
class ParseResult:
    ubo_blocks: List[UboBlockDecl] = field(default_factory=list)
    struct_defs: List[StructDef] = field(default_factory=list)
    bare_uniforms: List[BareUniformDecl] = field(default_factory=list)
    sampler_decls: List[SamplerDecl] = field(default_factory=list)


# --- token table ------------------------------------------------------------

_TOKEN_SPEC = [
    ("LINE_DIR", r'\#line\s+(?P<lineno>\d+)(?:\s+"(?P<file>[^"]*)")?[^\n]*'),
    ("HASH_DIR", r'\#[^\n]*'),                       # any other directive — skip
    ("WS",       r'[ \t\r]+'),
    ("NL",       r'\n'),
    ("LBRACE",   r'\{'),
    ("RBRACE",   r'\}'),
    ("LPAREN",   r'\('),
    ("RPAREN",   r'\)'),
    ("LBRACK",   r'\['),
    ("RBRACK",   r'\]'),
    ("COMMA",    r','),
    ("SEMI",     r';'),
    ("EQ",       r'='),
    ("INT",      r'\d+'),
    ("IDENT",    r'[a-zA-Z_][a-zA-Z_0-9]*'),
    ("SKIP",     r'.'),
]
_TOKEN_RE = re.compile("|".join(f"(?P<{n}>{p})" for n, p in _TOKEN_SPEC))


SAMPLER_TYPES = frozenset({
    "sampler2D", "sampler3D", "samplerCube", "sampler2DShadow",
    "sampler2DArray", "samplerCubeArray", "sampler2DArrayShadow",
    "isampler2D", "usampler2D",
    "samplerBuffer", "isamplerBuffer", "usamplerBuffer",
    "sampler2DMS", "sampler2DMSArray",
    "samplerCubeShadow",
})


# Whitelist GLSL primitive types accepted in UBO members. Mirrors
# chapter 04 §4.3.1.1 PRIMITIVE_TYPES; double / dvec* / dmat* are NOT here
# because LL GLSL慣用範囲外 (§5.2.1.6 / §4.3.1.6 build error).
PRIMITIVE_TYPE_IDENTS = frozenset({
    "float", "int", "uint", "bool",
    "vec2", "vec3", "vec4",
    "ivec2", "ivec3", "ivec4",
    "uvec2", "uvec3", "uvec4",
    "bvec2", "bvec3", "bvec4",
    "mat2", "mat3", "mat4",
    "mat2x2", "mat2x3", "mat2x4",
    "mat3x2", "mat3x3", "mat3x4",
    "mat4x2", "mat4x3", "mat4x4",
})


UNSUPPORTED_TYPE_IDENTS = frozenset({
    "double",
    "dvec2", "dvec3", "dvec4",
    "dmat2", "dmat3", "dmat4",
    "dmat2x2", "dmat2x3", "dmat2x4",
    "dmat3x2", "dmat3x3", "dmat3x4",
    "dmat4x2", "dmat4x3", "dmat4x4",
    "uint64_t", "int64_t",
})


# --- tokenization -----------------------------------------------------------

class _Token:
    __slots__ = ("kind", "text", "file", "line")

    def __init__(self, kind: str, text: str, file: str, line: int) -> None:
        self.kind = kind
        self.text = text
        self.file = file
        self.line = line

    def __repr__(self) -> str:  # debug only
        return f"Token({self.kind}, {self.text!r}, {self.file}:{self.line})"


def tokenize(source: str, default_file: str = "<input>") -> List[_Token]:
    """Tokenize preprocessed GLSL.

    `#line N "path"` directives update the current location so subsequent
    tokens carry the original source file + line (§5.2.1.7).
    """
    tokens: List[_Token] = []
    cur_file = default_file
    cur_line = 1
    for m in _TOKEN_RE.finditer(source):
        kind = m.lastgroup
        text = m.group()
        if kind == "LINE_DIR":
            # #line N applies to the NEXT source line — subtract 1 so the
            # newline that follows the directive lifts the counter to N.
            cur_line = int(m.group("lineno")) - 1
            if m.group("file") is not None:
                cur_file = m.group("file")
            continue
        if kind == "HASH_DIR":
            continue
        if kind == "WS":
            continue
        if kind == "NL":
            cur_line += 1
            continue
        if kind == "SKIP":
            # unknown 1-char fallback — keep going (other_skip緩衝)
            continue
        tokens.append(_Token(kind, text, cur_file, cur_line))
    return tokens


# --- helpers ----------------------------------------------------------------

def _peek(tokens: List[_Token], i: int) -> Optional[_Token]:
    return tokens[i] if i < len(tokens) else None


def _expect(tokens: List[_Token], i: int, kind: str, text: Optional[str] = None) -> _Token:
    tok = _peek(tokens, i)
    if tok is None or tok.kind != kind or (text is not None and tok.text != text):
        loc_file = tok.file if tok else "<eof>"
        loc_line = tok.line if tok else 0
        got = f"{tok.kind}:{tok.text!r}" if tok else "<eof>"
        raise CodegenError(format_error(
            f"unexpected token (expected {kind}{f' {text!r}' if text else ''}, got {got})",
            glsl_file=loc_file, line=loc_line,
            action="check GLSL syntax around this location",
        ))
    return tok


def _skip_to_semi(tokens: List[_Token], i: int) -> int:
    while i < len(tokens) and tokens[i].kind != "SEMI":
        i += 1
    return i + 1  # past the semicolon


# --- top-level parser -------------------------------------------------------

def parse_glsl(source: str, source_file_hint: str = "<input>") -> ParseResult:
    """Parse preprocessed GLSL → ParseResult."""
    tokens = tokenize(source, default_file=source_file_hint)
    result = ParseResult()
    struct_lookup: dict = {}

    i = 0
    while i < len(tokens):
        tok = tokens[i]
        if tok.kind == "IDENT" and tok.text == "layout":
            # `layout(...)` は UBO 専用ではない (= location/binding 等 in/out/sampler
            # にも付く)。直後を peek し、UBO block pattern (= `uniform <ident> {`)
            # のみ _parse_ubo_block で消費、それ以外は semi まで skip。
            j = i + 1
            if j < len(tokens) and tokens[j].kind == "LPAREN":
                depth = 1
                j += 1
                while j < len(tokens) and depth > 0:
                    if tokens[j].kind == "LPAREN":
                        depth += 1
                    elif tokens[j].kind == "RPAREN":
                        depth -= 1
                    j += 1
            is_ubo_block = (
                j + 2 < len(tokens)
                and tokens[j].kind == "IDENT" and tokens[j].text == "uniform"
                and tokens[j + 1].kind == "IDENT"
                and tokens[j + 2].kind == "LBRACE"
            )
            if is_ubo_block:
                block, i = _parse_ubo_block(tokens, i, struct_lookup)
                result.ubo_blocks.append(block)
                continue
            i = _skip_to_semi(tokens, i + 1)
            continue
        if tok.kind == "IDENT" and tok.text == "struct":
            sd, i = _parse_struct(tokens, i)
            if sd.name in struct_lookup:
                raise CodegenError(format_error(
                    f"duplicate struct '{sd.name}'",
                    glsl_file=sd.source_file, line=sd.source_line,
                    action="rename or remove the duplicate definition",
                ))
            struct_lookup[sd.name] = sd
            result.struct_defs.append(sd)
            continue
        if tok.kind == "IDENT" and tok.text == "uniform":
            i = _parse_uniform_or_sampler(tokens, i, result)
            continue
        # unrecognised — advance to next semicolon (other_skip)
        i = _skip_to_semi(tokens, i + 1)

    # Pass 2: resolve nested struct refs inside UBO blocks
    for block in result.ubo_blocks:
        for m in block.members:
            if m.type_str in struct_lookup:
                m.nested_struct = struct_lookup[m.type_str]
    return result


# --- ubo_block --------------------------------------------------------------

def _parse_layout_qual(tokens: List[_Token], i: int) -> Tuple[dict, int]:
    _expect(tokens, i, "IDENT", "layout"); i += 1
    _expect(tokens, i, "LPAREN");          i += 1
    quals: dict = {}
    while True:
        tok = _peek(tokens, i)
        if tok is None:
            raise CodegenError(format_error(
                "unterminated layout(...) qualifier",
                action="add closing ')' to the layout qualifier",
            ))
        if tok.kind == "RPAREN":
            i += 1
            break
        if tok.kind == "COMMA":
            i += 1
            continue
        if tok.kind == "IDENT":
            key = tok.text
            i += 1
            nxt = _peek(tokens, i)
            if nxt is not None and nxt.kind == "EQ":
                i += 1
                val_tok = _expect(tokens, i, "INT"); i += 1
                quals[key] = int(val_tok.text)
            else:
                quals[key] = True
            continue
        # anything else — fail loudly
        raise CodegenError(format_error(
            f"unexpected token in layout(...): {tok.kind}:{tok.text!r}",
            glsl_file=tok.file, line=tok.line,
        ))
    return quals, i


def _parse_ubo_block(tokens, i, struct_lookup):
    start_tok = tokens[i]
    quals, i = _parse_layout_qual(tokens, i)
    _expect(tokens, i, "IDENT", "uniform"); i += 1
    name_tok = _expect(tokens, i, "IDENT"); i += 1
    _expect(tokens, i, "LBRACE"); i += 1

    if "std140" not in quals:
        raise CodegenError(format_error(
            f"unsupported layout qualifier for UBO '{name_tok.text}' (std140 required)",
            glsl_file=start_tok.file, line=start_tok.line,
            block=name_tok.text,
            reason="chapter 04 §3.1 判断 A = AYAstorm r41 設計は std140 layout のみサポート",
            action="add `layout(std140)` to the block declaration",
        ))

    members: List[Member] = []
    while True:
        tok = _peek(tokens, i)
        if tok is None:
            raise CodegenError(format_error(
                f"unterminated UBO block '{name_tok.text}'",
                glsl_file=start_tok.file, line=start_tok.line,
            ))
        if tok.kind == "RBRACE":
            i += 1
            break
        member, i = _parse_member_decl(tokens, i, block_name=name_tok.text)
        members.append(member)

    # optional instance name
    instance_name = ""
    inst_tok = _peek(tokens, i)
    if inst_tok and inst_tok.kind == "IDENT":
        instance_name = inst_tok.text
        i += 1
        # support `} a, b;` multi-instance — keep first, warn? spec says LL 慣用外
        nxt = _peek(tokens, i)
        if nxt and nxt.kind == "COMMA":
            raise CodegenError(format_error(
                f"UBO '{name_tok.text}' has multiple instance names — Codegen は 1 instance のみ採用",
                glsl_file=start_tok.file, line=start_tok.line,
                block=name_tok.text,
            ))
    _expect(tokens, i, "SEMI"); i += 1

    block = UboBlockDecl(
        block_name=name_tok.text,
        layout_qual=quals,
        members=members,
        source_file=start_tok.file,
        source_line=start_tok.line,
        instance_name=instance_name,
    )
    return block, i


# --- struct -----------------------------------------------------------------

def _parse_struct(tokens, i):
    start_tok = tokens[i]
    _expect(tokens, i, "IDENT", "struct"); i += 1
    name_tok = _expect(tokens, i, "IDENT"); i += 1
    _expect(tokens, i, "LBRACE"); i += 1

    members: List[Member] = []
    while True:
        tok = _peek(tokens, i)
        if tok is None:
            raise CodegenError(format_error(
                f"unterminated struct '{name_tok.text}'",
                glsl_file=start_tok.file, line=start_tok.line,
            ))
        if tok.kind == "RBRACE":
            i += 1
            break
        member, i = _parse_member_decl(tokens, i, block_name=name_tok.text)
        members.append(member)
    _expect(tokens, i, "SEMI"); i += 1

    return StructDef(
        name=name_tok.text,
        members=members,
        source_file=start_tok.file,
        source_line=start_tok.line,
    ), i


# --- member -----------------------------------------------------------------

def _parse_member_decl(tokens, i, *, block_name: str) -> Tuple[Member, int]:
    type_tok = _peek(tokens, i)
    if type_tok is None or type_tok.kind != "IDENT":
        raise CodegenError(format_error(
            f"expected type name in '{block_name}'",
            glsl_file=type_tok.file if type_tok else "<eof>",
            line=type_tok.line if type_tok else 0,
            block=block_name,
        ))
    type_str = type_tok.text
    i += 1

    if type_str in UNSUPPORTED_TYPE_IDENTS:
        raise CodegenError(format_error(
            f"unsupported type '{type_str}' in '{block_name}'",
            glsl_file=type_tok.file, line=type_tok.line,
            block=block_name,
            reason="LL AYAstorm GLSL 慣用範囲外 (= chapter 04 §4.3.1.6 build error 対象)",
            action="rewrite using a supported std140 type",
        ))

    name_tok = _expect(tokens, i, "IDENT"); i += 1
    member_name = name_tok.text

    array_count: Optional[int] = None
    bracket_tok = _peek(tokens, i)
    if bracket_tok and bracket_tok.kind == "LBRACK":
        i += 1
        size_tok = _peek(tokens, i)
        if size_tok is None or size_tok.kind != "INT":
            raise CodegenError(format_error(
                f"dynamic / non-literal array size in '{block_name}.{member_name}'",
                glsl_file=name_tok.file, line=name_tok.line,
                block=block_name, member=member_name,
                reason="compile-time constant array size only",
            ))
        array_count = int(size_tok.text)
        i += 1
        _expect(tokens, i, "RBRACK"); i += 1
    _expect(tokens, i, "SEMI"); i += 1

    member = Member(
        name=member_name,
        type_str=type_str,
        array_count=array_count,
        source_file=name_tok.file,
        source_line=name_tok.line,
    )
    return member, i


# --- bare uniform / sampler -------------------------------------------------

def _parse_uniform_or_sampler(tokens: List[_Token], i: int, result: ParseResult) -> int:
    start_tok = tokens[i]
    _expect(tokens, i, "IDENT", "uniform"); i += 1
    type_tok = _peek(tokens, i)
    if type_tok is None or type_tok.kind != "IDENT":
        # malformed — skip to ; and move on
        return _skip_to_semi(tokens, i)
    type_str = type_tok.text
    i += 1
    name_tok = _peek(tokens, i)
    if name_tok is None or name_tok.kind != "IDENT":
        return _skip_to_semi(tokens, i)
    name_str = name_tok.text
    i += 1

    array_count: Optional[int] = None
    nxt = _peek(tokens, i)
    if nxt and nxt.kind == "LBRACK":
        i += 1
        size_tok = _expect(tokens, i, "INT"); i += 1
        array_count = int(size_tok.text)
        _expect(tokens, i, "RBRACK"); i += 1
        nxt = _peek(tokens, i)

    # Must terminate with ;
    if nxt is None or nxt.kind != "SEMI":
        return _skip_to_semi(tokens, i)
    i += 1

    if type_str in SAMPLER_TYPES:
        result.sampler_decls.append(SamplerDecl(
            sampler_type=type_str,
            name=name_str,
            source_file=start_tok.file,
            source_line=start_tok.line,
        ))
    else:
        result.bare_uniforms.append(BareUniformDecl(
            type_str=type_str,
            name=name_str,
            array_count=array_count,
            source_file=start_tok.file,
            source_line=start_tok.line,
        ))
    return i
