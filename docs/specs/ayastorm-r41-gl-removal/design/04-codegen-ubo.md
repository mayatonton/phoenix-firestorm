# r41 UBO 全体設計 Chapter 04: Codegen-UBO 機構

**起案日**: 2026-06-03
**位置付け**: 設計 doc 群の **中核機構** doc。Codegen-UBO の build-time pipeline / 生成物仕様 / runtime 入口 API shape を確定する。runtime 値書込 (= redirect 層) は chapter 06、bare uniform → UBO 集約対応表は chapter 05、build system 統合は chapter 08 に分担。
**pre-requisite**:
- `01-overview.md` §3.6 (Codegen-UBO 定義) / §2 (2 大設計原則)
- `02-naming-convention.md` §2.4 (生成識別子規則)
- `03-cadence-classification.md` (cadence 軸の独立性)

---

## §1 本 chapter の scope

### §1.1 scope (= 本 chapter で確定するもの)

1. Codegen-UBO の **pipeline 全景** (build-time pre-process → 生成物 → runtime 入口 の 3 stage)
2. **build-time pre-process step** の仕様 (入力 / parse 手段 / std140 offset 計算の責務分担)
3. **生成物の確定** (ファイル分割 / `<BlockName>Layout` 構造体 / perfect hash table / メタデータ)
4. **name-based 解決の dispatch 機構** (compile-time perfect hash + runtime 入口 API shape)
5. **bare uniform の処遇境界** (= Codegen は取り込まない、redirect 層と chapter 05 集約表の前提宣言)
6. inventory §6.2 / §6.4 / §6.5 の課題に対する Codegen-UBO の **対応宣言**

### §1.2 非 scope (= 他 chapter 譲り)

- CMake / glslang / Codegen tool の **build system 統合** → chapter 08
- runtime での値書込実装 (cadence 別 upload site / dirty 判定 / descriptor set bind) → chapter 06
- 既存 85 UBO blueprint の **cadence 別 mapping** + bare uniform → UBO 集約対応表 → chapter 05
- Vulkan API 接続点 (vkQueueSubmit / descriptor set / VMA / volk dynamic loader 状況) → chapter 07

---

## §2 Codegen-UBO の pipeline 全景

### §2.1 3 stage モデル

```
[Stage 1: build-time pre-process]
GLSL files (read-only) ──┐
                          ├──> Codegen tool ──> C++ header files (.inl)
                          │                     + perfect hash table
                          └──> glslang ───────> SPIR-V binaries

[Stage 2: 生成物 (build artifact)]
indra/newview/generated/ubo/
  ubo_layout_<blockname>.inl  (1 file / UBO)
  ubo_perfect_hash.inl        (全 uniform 名集約)
  ubo_metadata.inl            (block_name → (size, set, binding))
  ubo_index.inl               (上記の include 集約)

[Stage 3: runtime 入口 (= chapter 06 への橋渡し)]
LLGLSLShader::uniform4fv(name, ...) call
  ↓ compile-time hash(name) → perfect hash bucket lookup
  ↓ (block_hash, offset, size) を取得
  ↓ redirect 層 (chapter 06) が cadence 別 update site で memcpy
```

### §2.2 各 stage の責務

| stage | 責務 | 本 chapter での扱い |
|---|---|---|
| Stage 1 | GLSL を **read-only 入力** として parse、C++ header を生成。glslang は SPIR-V 化を **独立並列** で実行 | §3 / §4 |
| Stage 2 | 生成 C++ header の **配置 / 分割 / API 形状** を確定。3 OS 互換の細部は chapter 08 | §5 |
| Stage 3 | runtime での name 解決の **入口 API shape** までを保証。値書込本体は chapter 06 | §6 |

**含意**:
- Codegen と glslang は **同じ GLSL 入力に対して 2 系統並列の build process**
- Codegen 生成物は **build artifact のみ**、コミットしない (git ignore、chapter 08 で確定)
- runtime での setter 実装は chapter 06 に閉じる、本 chapter は **「offset table が読める状態」までを保証**

---

## §3 GLSL は read-only 入力 (= 判断 A 確定)

### §3.1 AYA 判断 (2026-06-03)

> **(A) Codegen は GLSL を改変しない**

含意:
- Codegen の出力は **C++ 側のみ** (header `*.inl` 群)
- GLSL 本体は SPIR-V 化 (glslang) で独立 build され、Codegen 経由の rewrite は発生しない
- 利点: upstream Firestorm の GLSL 取込時に **diff ゼロ** (= 原則 1 完全達成)
- 副作用: std140 layout offset は glslang 側が確定する事実上の owner、Codegen は **同じ offset 計算を独立再現** する必要

### §3.2 std140 layout 整合の保証 (= build-time check)

GLSL spec 7.6.2.2 (std140) は決定的なため、Codegen は独自 calculator で同じ offset を計算可能。glslang との一致は build-time check で保証する:

build sequence:
1. Codegen tool が GLSL 入力 → C++ header (offset 値計算)
2. glslang が同 GLSL 入力 → SPIR-V (offset 値計算)
3. **build-time check**: SPIR-V から reflection で offset を抽出 → Codegen 生成 C++ header の offset と一致確認
4. 不一致なら **build error** (= silent runtime corruption を排除)

これにより Codegen 計算ミス / glslang の std140 解釈変更 を build 段階で検知。

### §3.3 std140 規則の要約 (= Codegen が再現すべき計算)

- scalar (float / int / uint): **4-byte** align、4-byte size
- vec2: **8-byte** align、8-byte size
- vec3 / vec4: **16-byte** align、12 / 16-byte size
- mat3: 16-byte align per column、3 column × 16 = **48-byte** size
- mat4: 16-byte align、**64-byte** size
- array of vec3 (および scalar): each element は **vec4 stride** に round up
- struct: 16-byte align (max of member align)、末尾 padding で 16-byte multiple

詳細は GL 4.5 spec 7.6.2.2 を参照。本 chapter は要約のみ。

### §3.4 代替案 = SPIR-V reflection 抽出 (= 保留候補)

「**SPIR-V から reflection で抽出した offset を C++ header に書き出す**」案も技術的に成立。利点: Codegen 独自 calculator 実装不要。欠点: glslang version 依存度↑、build-time に glslang reflection 出力経由が必要。

→ **(A1) Codegen 独自 calculator vs SPIR-V reflection 抽出** として chapter 10 (open-questions) 保留、最終決定は chapter 08 (build pipeline 構成) で。本 chapter §3.2 は both 道筋を describe する状態に留める。

---

## §4 build-time pre-process step 仕様

### §4.1 入力: UBO ブロック宣言のみ (= 判断 C 確定)

> **(C) Codegen は bare uniform を取り込まない**

含意:
- Codegen 入力 = GLSL 中の `uniform <BlockName> { ... }` ブロック宣言**のみ**
- bare uniform (= `uniform vec4 color;`) は Codegen の対象外
- bare uniform → UBO の集約 mapping は **chapter 05 で個別表として整備**、Codegen は集約結果として登場する UBO ブロック宣言だけを処理

bare uniform 個別の path 詳細は §7。

### §4.2 parse 手段

選択肢:

| # | 手段 | 利点 | 欠点 |
|---|---|---|---|
| P1 | regex | 実装最短 | ネスト / コメント / `#ifdef` で破綻 |
| P2 | glslang library reflection | 高精度、AST 取得 | build dependency 大、tool が glslang を link |
| P3 | 独自 mini-parser | ~200 行、依存ゼロ、preprocessor は glslang -E で先処理 | 構文サポート限定 (LL の GLSL 慣用範囲) |

**推奨 = P3 (独自 mini-parser)** + 先処理として **glslang -E** で preprocessor 展開済 GLSL を入力に取る:
- block scope + member 抽出は単純構文 (preprocessor 展開後はほぼ純粋な block 列)
- glslang library を tool に embed する負担を回避
- chapter 08 でこの方針が build script 上どう実装されるかを確定

→ **(P) parse 手段選択** を chapter 10 保留、chapter 08 で最終決定。

### §4.3 std140 offset 計算

§3.3 規則を Codegen 内に実装。block 内 member を順次走査 → offset 計算 → C++ 定数として出力。

#### §4.3.1 std140 calculator algorithm 詳細化 (= Phase 2d-β-revise Deliverable A-2)

GLSL spec 7.6.2.2 std140 を Codegen Python tool 内で **decision table + state machine** として実装する確定形。Phase 1.A (chapter 09 §4.1) 実装 source of truth。

##### §4.3.1.1 base alignment 決定表 (= 全 GLSL type 網羅)

| member type | base alignment (A) | base size (S) | 注記 |
|---|---|---|---|
| `bool` / `int` / `uint` / `float` | 4 | 4 | scalar |
| `vec2` / `ivec2` / `uvec2` / `bvec2` | 8 | 8 | 2-component vector |
| `vec3` / `ivec3` / `uvec3` / `bvec3` | **16** | **12** | std140 規則: align は 16 だが size は 12 = 末尾 4 byte 隙間 |
| `vec4` / `ivec4` / `uvec4` / `bvec4` | 16 | 16 | |
| `double` | 8 | 8 | LL GLSL 慣用外、検出時は build error (= §4.3.1.6) |
| `dvec2` / `dvec3` / `dvec4` | 16 / 32 / 32 | 16 / 24 / 32 | LL GLSL 慣用外、検出時は build error |
| `mat2` | 16 | 32 | 2 column × 16-byte column stride (= column-major std140) |
| `mat3` | 16 | **48** | 3 column × 16-byte stride、各 column は vec3 だが alignment 16 ↑ |
| `mat4` | 16 | 64 | 4 column × 16-byte stride |
| `mat2x3` | 16 | 32 | 2 column × 16 (= column = vec3 → stride 16) |
| `mat3x4` | 16 | 48 | 3 column × 16 (= column = vec4 → stride 16) |
| array of T (= `T[N]`) | round_up(A_of_T, 16) | round_up(S_of_T, 16) × N | **配列 element は必ず vec4 stride に round up** |
| struct | round_up(max(A_of_members), 16) | round_up(sum_of_members_with_padding, 16) | nested struct の重要規則 |

**Codegen Python 実装 (= `scripts/codegen/std140.py` 凍結 interface)**:

```python
from dataclasses import dataclass

@dataclass(frozen=True)
class TypeInfo:
    base_align: int      # = std140 base alignment
    base_size: int       # = std140 base size (vec3 等の hole 込み)
    is_array: bool = False
    array_count: int = 1
    array_stride: int = 0
    is_struct: bool = False
    struct_members: tuple = ()    # nested member tuple

# decision table = const dict、上記表をそのまま hardcode
PRIMITIVE_TYPES = {
    'float': TypeInfo(4, 4),
    'vec2':  TypeInfo(8, 8),
    'vec3':  TypeInfo(16, 12),
    'vec4':  TypeInfo(16, 16),
    'int':   TypeInfo(4, 4),
    'uint':  TypeInfo(4, 4),
    'bool':  TypeInfo(4, 4),  # GLSL bool は 4-byte
    'mat2':  TypeInfo(16, 32),
    'mat3':  TypeInfo(16, 48),
    'mat4':  TypeInfo(16, 64),
    # ... (ivec*, uvec*, bvec*, mat*x*)
}
```

##### §4.3.1.2 offset 計算 state machine (= 1 UBO block 走査)

```python
def compute_layout(block_members: list) -> tuple[list, int]:
    """
    block_members = [(name, type_str, array_count_or_None, nested_struct_or_None), ...]
    returns (member_layout_list, block_total_size)
    """
    offset = 0
    layout = []
    max_member_align = 16   # std140 規則: block 全体の align は 16 以上

    for (name, type_str, array_count, nested) in block_members:
        # Step 1: member の type info を取得
        if nested is not None:
            ti = compute_struct_type_info(nested)   # 再帰呼出 (§4.3.1.3)
        elif type_str in PRIMITIVE_TYPES:
            ti = PRIMITIVE_TYPES[type_str]
        else:
            raise CodegenError(f"unknown type '{type_str}' (= LL GLSL 慣用外検出、§4.3.1.6)")

        # Step 2: array 化処理 (§4.3.1.4)
        if array_count is not None:
            ti = arrayify(ti, array_count)

        # Step 3: alignment 適用 = offset を base_align 倍数に round up
        align = ti.base_align
        offset = round_up(offset, align)

        # Step 4: layout record
        layout.append({
            'name': name,
            'offset': offset,
            'size': ti.base_size,
            'align': align,
            'array_stride': ti.array_stride if ti.is_array else 0,
        })

        # Step 5: offset 進める
        offset += ti.base_size
        max_member_align = max(max_member_align, align)

    # Step 6: block 末尾 padding = max(member align) 倍数に round up
    block_total = round_up(offset, max_member_align)
    return (layout, block_total)


def round_up(x: int, align: int) -> int:
    return (x + align - 1) // align * align
```

##### §4.3.1.3 nested struct 計算 (= 再帰、Codegen 実装の主要 corner case)

```python
def compute_struct_type_info(struct_members: tuple) -> TypeInfo:
    """nested struct は std140 で 16-byte align、末尾 padding 込みで total size 計算"""
    layout, total = compute_layout(list(struct_members))
    # 重要規則: nested struct の base_align は max(member align) で常に 16 以上、size は 16 倍数 round up
    align = max(m['align'] for m in layout)
    align = max(align, 16)
    return TypeInfo(
        base_align=align,
        base_size=total,  # = 既に round_up 済 (compute_layout Step 6)
        is_struct=True,
        struct_members=tuple(layout),
    )
```

##### §4.3.1.4 array stride 計算 (= 最重要 corner case)

std140: 配列 element は **常に vec4 (= 16 byte) stride に round up**。

```python
def arrayify(ti: TypeInfo, count: int) -> TypeInfo:
    """std140: 配列 element の stride は max(base_align, 16) に round up"""
    stride = round_up(ti.base_size, 16)   # = vec3 (size=12) → stride 16、float (size=4) → stride 16
    return TypeInfo(
        base_align=max(ti.base_align, 16),    # 配列 base_align も 16 以上に
        base_size=stride * count,             # 配列 total size = stride × count
        is_array=True,
        array_count=count,
        array_stride=stride,
    )
```

**実装上の落とし穴** (= chapter 09 Phase 1.A 実装時の review point):
- `float color[8]` → element stride **16** (= 4 ではない)、total size = 128 byte (= 32 ではない)
- `vec3 light_pos[4]` → element stride **16** (= 12 ではない)、total size = 64 byte
- `mat4 bones[3]` → element stride 64、total size = 192 byte (= mat4 は既に 16-multiple なので stride 据置)

##### §4.3.1.5 末尾 padding 規則 (= block total size 確定)

block 内最後の member 後に **block 全体 align (= max member align) の倍数に round up** する padding を追加:

```python
# §4.3.1.2 Step 6 既述
block_total = round_up(offset, max_member_align)
```

chapter 08 §6.4 で **更に 256 byte multiple に再 round up** (= device 別 `minUniformBufferOffsetAlignment` 最大値吸収) → Codegen 出力 `block_size` field は 256 byte 倍数:

```python
def pad_to_device_align(size: int) -> int:
    return (size + 255) & ~255  # = chapter 08 §6.4 pad_to_256()
```

= **Codegen 内 2 段 padding**: (1) std140 末尾 padding (= max member align 倍数) → (2) device 互換 padding (= 256 byte 倍数)。

##### §4.3.1.6 unsupported type 検出 + build error

LL GLSL 慣用外 type (= `double` / `dvec*` / `dmat*` / `uint64_t` 等) を input GLSL に検出した場合:

```python
class CodegenError(Exception): pass

# §4.3.1.2 Step 1 で raise:
raise CodegenError(
    f"[codegen_ubo] ERROR: unsupported std140 type '{type_str}'\n"
    f"  GLSL file: {glsl_file}:{line_number}\n"
    f"  Block: {block_name}, member: {member_name}\n"
    f"  Reason: LL AYAstorm GLSL 慣用範囲外 type, std140 layout 計算未対応\n"
    f"  Action: chapter 05 集約表で型変換または chapter 08 §17 で type 追加判断"
)
```

= silent 計算誤り排除 (= chapter 08 §9.1 E1 と独立、type level の早期 fail)。

##### §4.3.1.7 SPIR-V reflection 二重保証との接合 (= chapter 08 §5.3 / §5.4 B-1)

§3.2 build-time check で Codegen 計算結果 vs glslang SPIR-V reflection 結果 を per-member offset で照合:

```python
# scripts/codegen/codegen_ubo.py 内、§4.3.1.2 compute_layout() 完了後:
def verify_against_spirv(my_layout: list, spirv_reflection: dict, block_name: str):
    for member in my_layout:
        spv_offset = spirv_reflection[block_name][member['name']]['offset']
        if spv_offset != member['offset']:
            raise CodegenError(
                f"[codegen_ubo] ERROR: std140 offset mismatch in '{block_name}.{member['name']}'\n"
                f"  Codegen calculation:        OFFSET = {member['offset']}\n"
                f"  glslang SPIR-V reflection:  OFFSET = {spv_offset}\n"
                f"  Diff = {abs(spv_offset - member['offset'])} bytes\n"
                f"  Likely cause: array stride / vec3 hole / nested struct padding\n"
                f"  Action: §4.3.1 algorithm review or glslang version drift 確認"
            )
```

= chapter 08 §9.4 build error format 完全準拠、E1 silent corruption 排除。

##### §4.3.1.8 calculator 実装規模見積

| 項目 | LoC |
|---|---|
| PRIMITIVE_TYPES decision table (§4.3.1.1) | ~30 |
| compute_layout state machine (§4.3.1.2) | ~50 |
| compute_struct_type_info 再帰 (§4.3.1.3) | ~20 |
| arrayify (§4.3.1.4) | ~15 |
| round_up / pad_to_device_align utility | ~10 |
| CodegenError + error format (§4.3.1.6) | ~30 |
| verify_against_spirv (§4.3.1.7) | ~40 |
| unit test (= GLSL spec 7.6.2.2 から抜粋 ~20 cases) | ~150 |
| **合計** | **~345** |

= chapter 08 §3.2 Python tool 「~500-1000 行」見積に整合、計算 module 単独 ~345 行で完結。



### §4.4 同名 UBO 複数 GLSL 宣言の扱い

inventory §3.4 で `CloudsVParamUBO_Legacy` (`cloudsV.glsl` + `cloudsF.glsl`) / `WaterVParamUBO_Legacy` (`waterV.glsl` + `waterF.glsl`) / `ShadowUtilParamUBO_Legacy` (`shadowUtil.glsl` + `cinematic_bd/shadowUtil.glsl`) 等の **同名 block を複数 GLSL で再宣言** している例を確認。

Codegen の扱い:
- 全宣言が **同一 member 構成** であることを build-time check で保証
- 不一致なら build error (= 静かな layout mismatch を排除)
- 出力 C++ header は 1 block 1 file (= 重複生成しない)

---

## §5 生成物の確定

### §5.1 生成ファイル群

| ファイル | 内容 | 規模 |
|---|---|---|
| `ubo_layout_<blockname>.inl` | 1 UBO ブロックの member offset / size 定数群 | UBO 数 (現状 84 + 既存実働 4 = 88) |
| `ubo_perfect_hash.inl` | (block_name, member_name) → offset の **perfect hash table** | 1 ファイル全集約 |
| `ubo_metadata.inl` | block_name → (block_size, member 数, descriptor_set, binding) | 1 ファイル全集約 |
| `ubo_index.inl` | 全 `ubo_layout_*.inl` の include 集約 (=  setter 側は本 file 1 つを include) | 1 ファイル |

配置 dir: `indra/newview/generated/ubo/` (build dir、git ignore、CMake 統合詳細は chapter 08)。

### §5.2 `<BlockName>Layout` 構造体 (chapter 02 §2.4 規約の展開)

例 (概念形、chapter 02 §3.1 の `FrameViewProj` を出力した場合):

```cpp
// ubo_layout_frameviewproj.inl (auto-generated, do not edit)
#pragma once
namespace ubo {
struct FrameViewProjLayout {
    static constexpr size_t view_OFFSET      = 0;    // mat4
    static constexpr size_t proj_OFFSET      = 64;   // mat4
    static constexpr size_t view_proj_OFFSET = 128;  // mat4
    // ...
};
inline constexpr size_t FrameViewProj_SIZE = /* std140-rounded */ ;
} // namespace ubo
```

規則 (chapter 02 §2.4 と整合):
- 構造体名 = `<BlockName>Layout`
- member offset 定数 = `<member>_OFFSET` (= GLSL 内 member 名そのまま)
- block size 定数 = `<BlockName>_SIZE` (= namespace スコープ)
- 全て `static constexpr` (= compile-time 値、zero runtime cost)
- ファイル名は ALL lowercase (= chapter 02 §2.4)

### §5.3 compile-time perfect hash (= 判断 B 確定)

#### §5.3.1 AYA 判断 (2026-06-03)

> **(B) name → offset dispatch = compile-time perfect hash**

全 (block_name, member_name) ペアを build-time に集約 → perfect hash function 生成 → C++ header `ubo_perfect_hash.inl` に埋め込む。

#### §5.3.2 生成形 (概念)

```cpp
// ubo_perfect_hash.inl (auto-generated)
#pragma once
namespace ubo {

struct UniformLocation {
    uint32_t block_hash;  // block 識別 hash
    uint32_t offset;      // block 内 offset
    uint32_t size;        // member 値 size (= write 量)
    uint32_t cadence_tag; // cadence (per-frame/per-program/per-draw/...) を識別する小値
};

// deterministic compile-time hash:
constexpr uint32_t hash_name(const char* s) { /* FNV-1a 等 */ }

// perfect hash bucket array (build-time 確定):
inline constexpr UniformLocation g_uniform_table[/* N */] = {
    // entry: hash("view")             → {hash_FrameViewProj, 0,  64, /*per-frame*/   0}
    // entry: hash("color")            → {hash_Program_GammaCorrect, ..., ..., /*per-program*/ 1}
    // entry: hash("light_color[0]")   → {hash_Draw_MultiLight,      ..., ..., /*per-draw*/    2}
    // ...
};

// compile-time resolve (静的 literal call site 用):
template<auto Name> constexpr UniformLocation resolve();

// runtime resolve (動的 string / mUniform[index] cache fill 用):
inline const UniformLocation* lookup_runtime(const char* name);

} // namespace ubo
```

#### §5.3.3 generator 選択 (= chapter 10 / chapter 08 持越)

| # | 手段 | 利点 | 欠点 |
|---|---|---|---|
| G1 | gperf | 老舗、信頼性高 | 外部 tool 依存 (3 OS 揃え) |
| G2 | 独自 generator | ~500 行、依存ゼロ、build 制御容易 | 実装 / 検証コスト |
| G3 | frozen 等 C++17 library | header-only | size 重め、compile-time 負担 |

→ **(G) generator 選択** を chapter 10 保留、chapter 08 で最終確定。chapter 04 は generator が **どの形を入力に取り何を出力するか** の interface 仕様までを書く。

### §5.4 衝突保証

perfect hash 生成器に **全 uniform 名集合** (= 全 UBO ブロックの全 member 名) を渡して衝突 0 を build-time 保証。衝突発生時は **build error**。

= 衝突は runtime detect 不要 (= 設計判断: build-time に閉じる)。

### §5.5 配置 path

- 生成 dir: `indra/newview/generated/ubo/`
- git ignore (= 生成物、commit しない)
- 3 OS 互換 / CMake target 配線 / generated dir の include path 露出方法 は chapter 08

### §5.6 perfect hash CHD algorithm 詳細化 (= Phase 2d-β-revise Deliverable A-3)

§5.3 で確定の compile-time perfect hash の **construction algorithm 詳細**。Phase 1.A (chapter 09 §4.1) で Codegen Python tool 実装 source of truth。default 採用 = **CHD (Compress, Hash, Displace)** algorithm (= Belazzougui, Botelho, Dietzfelbinger 2009)。

#### §5.6.1 CHD algorithm 採用根拠

CHD 採用 vs 代替案:

| algorithm | 構築時間 | table 容量 | 構築失敗率 | 実装規模 |
|---|---|---|---|---|
| **CHD** (default) | O(N) 期待 | (1+ε) N words、ε~0.1 | 極低 (= seed reroll) | ~250 行 |
| FCH (Fox-Chen-Heath) | O(N²) worst | (1+ε) N | 中 | ~200 行 |
| BDZ (Botelho-Dietzfelbinger-Ziviani) | O(N) 期待 | 2.6N bits + value table | 極低 | ~300 行 |
| naive seed search | O(N²) | N words | 高 | ~50 行、N>500 で seed 失敗多発 |

= **CHD = 880 entry 規模で構築 ~ms order、table size 最小、seed reroll で failure 回避容易**。

#### §5.6.2 CHD 概念 (= 2 段 hash + displacement table)

```
Input: N 個の string key (= 全 (block_name, member_name) ペア)
Output: hash function h: key → [0, N-1] (= 衝突 0)

構造:
  Step 1: bucket hash g: key → [0, M-1]  (M ≈ N / λ、λ = group size)
  Step 2: 各 bucket 内で displacement d_i を seed reroll で探索
  Step 3: 最終 hash = (h_inner(key) ⊕ d_{g(key)}) mod N

Lookup (= runtime):
  bucket = g(key)
  index  = (h_inner(key) XOR g_displacement[bucket]) mod N
  return g_values[index]
```

#### §5.6.3 構築 step (= Codegen Python 実装)

```python
import hashlib

# scripts/codegen/perfect_hash.py 凍結 interface
def build_chd(keys: list[str], values: list, lambda_: int = 4) -> dict:
    """
    keys = 全 uniform 名 (= block_name + "::" + member_name)
    values = 対応 UniformLocation
    lambda_ = group size (= bucket 当たり期待 entry 数、default 4)
    """
    N = len(keys)
    # Step 1: 容量決定
    table_size = next_prime(int(N * 1.1))   # ε = 0.1
    bucket_count = max(1, N // lambda_)
    
    # Step 2: 全 key を bucket に振分け (= bucket hash g)
    buckets = [[] for _ in range(bucket_count)]
    for k, v in zip(keys, values):
        b = fnv1a_32(k, seed=0x811c9dc5) % bucket_count
        buckets[b].append((k, v))
    
    # Step 3: bucket を size 降順 sort (= 大きい bucket から displacement 決定で seed 探索容易化)
    bucket_indices = sorted(range(bucket_count), key=lambda i: -len(buckets[i]))
    
    # Step 4: 各 bucket で displacement seed 探索
    displacements = [0] * bucket_count
    value_table = [None] * table_size
    
    for bi in bucket_indices:
        bucket = buckets[bi]
        if not bucket:
            continue
        
        # seed reroll: bucket 内全 key が衝突無く配置可能な displacement d を探す
        for d in range(0, 0xFFFFFFFF):
            slots_for_this_bucket = []
            ok = True
            for (k, v) in bucket:
                h = fnv1a_32(k, seed=0x811c9dc5 ^ d) % table_size
                if value_table[h] is not None or h in slots_for_this_bucket:
                    ok = False
                    break
                slots_for_this_bucket.append(h)
            if ok:
                # 確定: 全 slot に書込
                for ((k, v), h) in zip(bucket, slots_for_this_bucket):
                    value_table[h] = (k, v)
                displacements[bi] = d
                break
        else:
            raise CodegenError(
                f"[codegen_ubo] ERROR: CHD seed search exhausted for bucket {bi}\n"
                f"  Bucket size: {len(bucket)}, table_size: {table_size}\n"
                f"  Action: increase table_size (= 1.1 → 1.2) or lambda_ (= 4 → 3)"
            )
    
    return {
        'table_size': table_size,
        'bucket_count': bucket_count,
        'displacements': displacements,   # = g_displacement[M] in C++
        'value_table': value_table,       # = g_values[N] in C++
    }


def fnv1a_32(s: str, seed: int = 0x811c9dc5) -> int:
    """FNV-1a 32-bit、deterministic"""
    h = seed
    for b in s.encode('utf-8'):
        h ^= b
        h = (h * 0x01000193) & 0xFFFFFFFF
    return h


def next_prime(n: int) -> int:
    """table size を prime に揃える = hash 分散の deterministic 性確保 (任意)"""
    # ... 標準的 prime search、~10 行
```

#### §5.6.4 algorithm 性能特性

| 規模 | bucket 数 (λ=4) | 期待 seed reroll | 構築時間 |
|---|---|---|---|
| N=100 | 25 | <100 reroll / bucket | ~0.1 ms |
| **N=880** (= 88 UBO × ~10 member) | **220** | **<200 reroll / bucket** | **~5 ms** |
| N=5000 | 1250 | <500 reroll / bucket | ~50 ms |

= Codegen build cycle (= 全体 ~500-1000 ms) 内で **誤差 level** (= chapter 08 §3.2 / §12.4 timing 表参照)。

#### §5.6.5 衝突 0 build-time 保証 invariant

CHD 構築完了時、以下の invariant が自動成立:

1. **value_table の全 N slot が一意 entry で埋まる** (= seed reroll 内 ok 判定で保証)
2. **bucket 内 key の最終 hash が table 内衝突しない** (= reroll 探索の停止条件)
3. **lookup 関数が deterministic** = 同じ key → 同じ slot index (= bucket hash + displacement で唯一決定)

build-time check (= chapter 08 §9.1 E4):

```python
# Codegen 構築直後の self-check (= §5.4 build-time 衝突 0):
for k in keys:
    bi = fnv1a_32(k) % bucket_count
    d  = displacements[bi]
    h  = fnv1a_32(k, seed=0x811c9dc5 ^ d) % table_size
    assert value_table[h][0] == k, f"CHD lookup mismatch for key '{k}'"
```

不適合検出時は **build error + 衝突 2 key 出力** (= chapter 08 §9.4 E4 format):

```
[codegen_ubo] ERROR: perfect hash collision detected
  Key 1: FrameViewProj::view_proj
  Key 2: GlobalSky::view_proj
  Action: 名前衝突を chapter 02 §2.4 規約で resolve (= 重複 member 名は block prefix で区別)
```

#### §5.6.6 出力 C++ data 形式 (= chapter 08 §5.7 出力契約)

```cpp
// ubo_perfect_hash.inl (auto-generated)
namespace ubo {

// CHD 構築結果:
inline constexpr uint16_t g_chd_displacement[/* M */] = {
    /* 220 entry × 4 byte = 880 byte */
    0x0000, 0x0042, 0x10A5, /* ... */
};

inline constexpr UniformLocation g_chd_values[/* N */] = {
    /* 880 entry × 16 byte = ~14 KB */
    { 0xBLOCK_HASH_FrameViewProj, 0,  64, /*per-frame*/  0 },   // = "FrameViewProj::view"
    { 0xBLOCK_HASH_FrameViewProj, 64, 64, /*per-frame*/  0 },   // = "FrameViewProj::proj"
    /* ... */
};

inline constexpr uint32_t g_chd_table_size = /* 968 */;
inline constexpr uint32_t g_chd_bucket_count = /* 220 */;

constexpr uint32_t fnv1a_32(const char* s, uint32_t seed = 0x811c9dc5u) {
    uint32_t h = seed;
    while (*s) { h ^= static_cast<uint8_t>(*s++); h *= 0x01000193u; }
    return h;
}

constexpr const UniformLocation* lookup_runtime(const char* name) {
    const uint32_t bi = fnv1a_32(name) % g_chd_bucket_count;
    const uint32_t d  = g_chd_displacement[bi];
    const uint32_t h  = fnv1a_32(name, 0x811c9dc5u ^ d) % g_chd_table_size;
    // bound check + name 比較で false positive 検出 (= unknown name は別 key と衝突して非 nullptr に解決される可能性、§6.4.4 fallback と接合)
    if (h >= g_chd_table_size) return nullptr;
    // (= name 文字列も g_chd_values に同居 emit、strcmp で false positive 排除)
    if (strcmp(g_chd_key_strings[h], name) != 0) return nullptr;
    return &g_chd_values[h];
}

} // namespace ubo
```

**false positive 排除規則**:
- CHD は **登録 key には衝突 0** だが、**unknown key (= 登録外 string) は別 key の slot に collide する可能性**
- 対策: 各 slot に **元 key string も保持** (= `g_chd_key_strings[N]`) して `strcmp` で final 検証
- string table size: 880 entry × 平均 ~30 char = ~30 KB (= `ubo_perfect_hash.inl` 末尾)
- = §6.4.4 unresolved name fallback policy への確実な接合 path

#### §5.6.7 algorithm 実装規模見積

| 項目 | LoC |
|---|---|
| build_chd construction (§5.6.3) | ~80 |
| fnv1a_32 / next_prime utility | ~15 |
| invariant self-check (§5.6.5) | ~20 |
| C++ emit (§5.6.6 出力) | ~60 |
| string table emit + escape処理 | ~30 |
| error format + collision diagnosis (§5.6.5) | ~25 |
| unit test (= 100 / 880 / 5000 entry での再現) | ~80 |
| **合計** | **~310** |

= chapter 08 §3.2 Python tool 「~500-1000 行」見積に整合、perfect hash module 単独 ~310 行で完結。

---

## §6 name-based 解決の dispatch 詳細

### §6.1 解決の 3 局面

| 局面 | 解決時期 | 解決手段 |
|---|---|---|
| R1 | compile-time | 静的 literal `uniform4fv("color", ...)` → template + constexpr で perfect hash を直引き |
| R2 | runtime | 動的 string `uniform4fv(name_var, ...)` → perfect hash function を runtime 実行、bucket lookup (= O(1)) |
| R3 | hybrid | `mUniform[index]` 経由 = shader link 時に **index → UniformLocation を pre-cache**、setter 内では integer index 直引き |

### §6.2 LL 既存 `mUniform[index]` cache との関係 (= 原則 1 完全達成の鍵)

LL の uniform setter は **既に integer index 経由** (= `uniform1f(LLShaderMgr::SHINY_COLOR, x)` 等) が主流。string 経由は補助 path。

→ **R3 を最速 path として設計** することで:
- 既存 call site (= integer index) は **改修ゼロ** で UBO offset に到達
- string 経由 (R1 / R2) は補助 path として残置
- = 原則 1 (call site API 温存) を完全達成

shader link 時の pre-cache 流れ:
1. shader link 時、LL は `mUniform[index]` を index → uniform 名 で初期化済
2. **link 直後の post-process** として、各 index に対し `lookup_runtime(uniform_name)` を呼んで UniformLocation を得る
3. `mUniform[index]` と並列の `mUniformUBOLoc[index]` (= chapter 06 で新設) に UniformLocation を cache
4. setter (= `uniform*fv()`) は integer index で **mUniformUBOLoc[index] を直引き** → OpenGL path なら `mUniform[index]` を使い従来通り、Vulkan path なら `mUniformUBOLoc[index]` の offset へ memcpy

= **runtime hash 計算は link 時 1 度のみ**、frame 内では cache 直引き。

具体 cache 構造 / dirty 判定 / cadence 別 update site は chapter 06 で実装詳細を詰める。本 chapter は **interface 仕様までを確定**。

### §6.3 動的 uniform 名の扱い

shader 内で runtime 動的に名前生成される uniform (= 例: `light[i].color` の i 動的) は perfect hash の事前 enumerate を破綻させる。

LL の GLSL 現状調査必要事項:
- array uniform は **N 固定展開** (= `light[0].color`, `light[1].color`, ... が GLSL preprocess 後に flatten される) か
- 動的 index 経由 setter (= `uniform4fv("light[" + i + "].color", ...)` 等) が C++ 側に存在するか

→ **(D) 動的 uniform 名の存在確認** を chapter 10 保留、**chapter 06a-prep §7 (P1)-(P4) Phase 0 grep task に接続** (= chapter 06 系列分割後の新接続先、本査読 2026-06-03 §3.2)。

現時点の前提: LL の uniform は GLSL preprocess 後に flatten され、N 固定で perfect hash 可能と想定。例外発生時は chapter 06 で local fallback 設計。

**(D) Phase 0 接続 task 仕様** (= chapter 06a-prep §7 起案要事項):
- 対象 dir: `indra/newview/app_settings/shaders/` 配下全 GLSL ファイル
- grep pattern: array uniform 宣言 (`uniform <type> <name>[<N>]`) + setter call site (`uniform*fv("<name>[" + ... + "]"`)
- pass-fail criteria: 動的 index 経由 setter 0 件 (= 全 array が N 固定展開) なら perfect hash 成立、1 件以上検出なら chapter 06 redirect 層に local fallback 仕様追加

### §6.4 name-based dispatch algorithm 詳細化 (= Phase 2d-β-revise Deliverable A-1)

§6.1 R1/R2/R3 3 mode を **algorithm level の擬似コード** で確定する。Phase 1.A (= chapter 09 §4.1) で Codegen Python tool 実装 + redirect 層 (chapter 06a §3 / §5) 配線時の **interface 仕様 source of truth** とする。

#### §6.4.1 R3 (hybrid via `mUniform[index]` pre-cache) = primary path

LL 既存 setter (= `LLGLSLShader::uniform1f(LLShaderMgr::SHINY_COLOR, x)` 等) は **integer index 経由が主流** (§6.2 既述)。本 path を最速化すれば call site 改修ゼロで原則 1 完全達成。

**Stage A: shader link 直後 1 度 (= pre-cache fill)**:

```cpp
// chapter 06a §3 mUniformUBOLoc[index] pre-fill (chapter 08 §10 ubo_host_loader.inl 出力)
void LLGLSLShader::postLink_prefillUboLoc() {
    mUniformUBOLoc.resize(mUniform.size());
    for (size_t i = 0; i < mUniform.size(); ++i) {
        const char* name = mUniformName[i].c_str();   // = LL 既存 mUniform name 配列
        const ubo::UniformLocation* loc = ubo::lookup_runtime(name);  // §6.4.4 fallback で nullptr 可
        mUniformUBOLoc[i] = (loc != nullptr) ? *loc : ubo::UniformLocation::sentinel();
    }
}
```

**Stage B: setter call (= frame 内、最高頻度 path)**:

```cpp
// LLGLSLShader::uniform4fv(S32 index, const F32* v)  (chapter 06a §5 redirect 層)
void LLGLSLShader::uniform4fv(S32 index, const F32* v) {
    const ubo::UniformLocation& loc = mUniformUBOLoc[index];   // O(1) cache 直引き
    if (loc.block_hash == 0) {
        // §6.4.4 fallback = OpenGL path 強制 (= bare uniform 等の path)
        glUniform4fv(mUniform[index], 1, v);
        return;
    }
    // Vulkan path = UBO offset 直 memcpy (chapter 06b cadence 別 update site)
    redirect_to_ubo(loc.block_hash, loc.offset, loc.size, loc.cadence_tag, v);
}
```

**特性**:
- frame 内 hash 計算 **ゼロ** (= Stage A で 1 度のみ)
- index → UniformLocation の **1 段 indirection** のみ (= cache line friendly)
- block_hash == 0 を **無効値 sentinel** に予約 (= ubo::UniformLocation::sentinel() で固定値返却)

#### §6.4.2 R1 (compile-time static literal) = 補助 path

`uniform4fv("color", ...)` の文字列 literal を call site で **template + constexpr** に展開して compile-time 解決:

```cpp
// chapter 08 §5.7 g_uniform_table 生成済前提
namespace ubo {

template<auto NameLiteral>  // C++20 NTTP (Non-Type Template Parameter) で string literal を受取
struct CompileTimeResolve {
    static constexpr uint32_t h = hash_name(NameLiteral.data);
    static constexpr UniformLocation loc = lookup_compile_time<h>();  // §6.4.5 で具体化
    static_assert(loc.block_hash != 0, "uniform name unresolved (= chapter 05 集約表未登録)");
};

// call site (= 任意の C++ 編集箇所):
constexpr auto kLocColor = CompileTimeResolve<"color">::loc;   // compile-time 計算、runtime cost ゼロ
glsl_shader->writeUboMember(kLocColor, color_value);
```

**特性**:
- runtime hash 計算 **ゼロ** (= compile-time に block_hash / offset / size 確定)
- static_assert で **未登録 name を build error 化** (= silent runtime nullptr 排除)
- C++20 NTTP 必要 (= AYAstorm 既存 build は C++17 default、Phase 1.A で C++20 切替判定が必要 = §6.4.7 chapter 09 持越)

**用途**: 描画 pipeline core の hot path で integer index path より明示的に意図表現したい箇所 (= 任意採用、必須でない)。

#### §6.4.3 R2 (runtime dynamic string) = 動的 string path

LL 慣用範囲外 (= 配列 index に runtime 値を embed する path 等、§6.3 (D) で grep 検証予定) で必要なら使用:

```cpp
// LLGLSLShader::uniform4fv(const char* name, const F32* v)  (= LL 既存補助 overload)
void LLGLSLShader::uniform4fv(const char* name, const F32* v) {
    const ubo::UniformLocation* loc = ubo::lookup_runtime(name);  // CHD lookup = O(1)
    if (loc == nullptr) {
        // unknown name = log warning + drop (= chapter 06a §5 fallback policy)
        LL_DEBUGS("UBO") << "lookup_runtime miss: name=" << name << LL_ENDL;
        return;
    }
    redirect_to_ubo(loc->block_hash, loc->offset, loc->size, loc->cadence_tag, v);
}
```

**特性**:
- runtime hash 計算 **1 回 / call** (= FNV-1a `~32 cycles`、CHD bucket lookup `~10 cycles`、合計 ~50 ns order)
- frame 内連続呼出時は §6.4.1 R3 path 採用推奨 (= R2 は補助のみ)

#### §6.4.4 unresolved name fallback policy

`lookup_runtime()` が nullptr 返す cases:
- **bare uniform path** (= chapter 05 集約表で UBO 未集約の uniform): OpenGL path 強制継続、Vulkan path では no-op (= chapter 07 §7.2 (a) 案 default)
- **typo / unknown name**: LL_DEBUGS("UBO") log + write skip
- **GLSL preprocess 差で disable された uniform** (= `#ifdef HAS_NORMAL_MAP` で消えた member): R3 では sentinel 経由で OpenGL path に流れる、R1 では static_assert で build 段階 fail

**実装契約** (= chapter 06a §5 redirect 層 source of truth):

```cpp
// ubo::UniformLocation::sentinel() = block_hash==0 の invalid value
inline constexpr UniformLocation UniformLocation::sentinel() {
    return UniformLocation{0, 0, 0, 0};
}

// redirect 層 = sentinel 検知時の policy:
inline void redirect_to_ubo(uint32_t bh, uint32_t off, uint32_t sz, uint32_t cad, const void* v) {
    if (bh == 0) {
        // OpenGL path 既存挙動温存 (= R3 setter から既に glUniform 呼出済)
        return;
    }
    // (chapter 06b cadence 別 update site への分岐)
    forwardToUboUpload(bh, off, sz, cad, v);
}
```

#### §6.4.5 compile-time lookup の具体化 (= R1 path table)

```cpp
// ubo_perfect_hash.inl (Codegen 出力、chapter 08 §5.7) に追加 emit:
namespace ubo::detail {

// 全 (block_name, member_name) entry の compile-time index map:
template<uint32_t Hash> struct CompileTimeEntry;

// Codegen が build-time に 1 entry / 1 specialization を emit:
template<> struct CompileTimeEntry<0xXXXXXXXX> {
    static constexpr UniformLocation value = { 0xBLOCK_HASH, 0, 64, 0 };
};
template<> struct CompileTimeEntry<0xYYYYYYYY> {
    static constexpr UniformLocation value = { 0xBLOCK_HASH, 64, 16, 1 };
};
// ... 880 entry / 880 specialization (= 全 uniform 名集合)

} // namespace ubo::detail

namespace ubo {
template<uint32_t H> constexpr UniformLocation lookup_compile_time() {
    return detail::CompileTimeEntry<H>::value;   // 未 specialization なら compile error
}
}
```

**生成規模**: ~880 entry × ~80 byte / entry ≈ 70 KB の `ubo_perfect_hash.inl` 末尾追加 (= chapter 08 §11 cache invalidation 単位は同一)。

#### §6.4.6 3 path 性能比較表 (= AYA 判断材料 = Phase 1.A 実装方針)

| path | 解決時期 | runtime cost | 用途 | 採用範囲 |
|---|---|---|---|---|
| R1 (compile-time literal) | compile-time | 0 | hot path で意図明示 | 任意 (= 必須でない) |
| R2 (runtime dynamic string) | runtime / call | ~50 ns | 動的 name path | 補助 |
| **R3 (mUniform[index] pre-cache)** | shader link 1 度 + frame 直引き | ~5 ns / call | **既存 setter 全て** | **primary** |

R3 を **既存 30 setter method (chapter 06a §5) の default path** とし、R1/R2 は補助。

#### §6.4.7 C++20 NTTP 採否 (= chapter 09 持越判定材料)

R1 の `template<auto NameLiteral>` は C++20 NTTP に依存:
- AYAstorm 既存 build standard: C++17 default (= chapter 07 §2 既述、build flag `-std=c++17`)
- C++20 切替判定: Phase 1.A 入口で **R1 採用必要性 vs C++20 切替 cost** で AYA 判断、現時点 default = **R1 不採用** (= R3 で十分、R1 は task 完了後の polish 候補)

→ **(NTTP) R1 compile-time literal path 採否** = chapter 09 §11 (Q-NTTP) として AYA 判断仰ぎ候補に登録 (Phase 1.A 入口判定)。

---

## §7 bare uniform の処遇 (= 判断 C 確定の含意)

### §7.1 Codegen の責務外

bare uniform (= `uniform vec4 color;` のような UBO ブロック外宣言) は Codegen の入力対象外。

理由:
- bare → UBO 集約の判断は **意味論的判断** (= どの UBO に集約するか) で、build-time の機械処理に不適
- chapter 05 で集約対応表を整備し、Codegen は **集約結果として登場する UBO ブロックだけ**を処理

### §7.2 bare uniform の path

- **OpenGL path**: bare uniform は従来通り `glUniform*` 直呼び (= 原則 1 維持)
- **Vulkan path**: bare uniform は GLSL spec 上 opaque type 以外宣言禁止 → chapter 05 集約表で **UBO の member に集約された後** Codegen 経由で処理
- **両 path 共存期**: bare uniform の setter call site は **chapter 06 redirect 層で path 分岐** (OpenGL → 従来 `glUniform*`、Vulkan → UBO offset 書込)

#### §7.2.1 過渡期動作 (= chapter 09 Phase Exit Criteria 接続、2026-06-03 査読 §3.3)

chapter 09 phase roadmap で「N UBO ずつ移行」する間、まだ chapter 05 集約表で UBO に取り込まれていない bare uniform は **Vulkan path 上で未 redirect** (= chapter 01 §1.2 の「85 blueprint が dead」と同型の過渡期状態)。

**chapter 09 Phase Exit Criteria 要求事項** (= 本 chapter から chapter 09 への入力契約):
- 各 Phase で「未集約 bare uniform の Vulkan path 動作」を Exit Criteria に明示
- 取り得る policy 案 (chapter 09 で確定):
  - (a) 未集約 bare の Vulkan path 投入は no-op (= 値ゼロ / 初期値で描画)
  - (b) 未集約 bare の Vulkan path 投入は build-time check で error (= 集約済 bare のみ Vulkan path 通過許可)
  - (c) Phase 単位 staging (= 該当 Phase で集約済の bare のみ Vulkan path 有効、他は OpenGL path 強制)
- chapter 09 Phase 0 計測完了後、上記 (a)/(b)/(c) のいずれを採用するか確定

### §7.3 chapter 05 集約対応表との関係

chapter 05 で整備すべき表 (概念):

```
[bare uniform name] → [UBO block name].[member name]
"color"             → Program_GammaCorrect.color
"alpha_threshold"   → Program_AlphaParams.threshold
...
```

統合フロー:
1. chapter 05 で bare uniform `"color"` → `Program_GammaCorrect.color` の集約を決定
2. 該当 GLSL に `Program_GammaCorrect` UBO ブロックを追加 (= chapter 05 が GLSL を改変、Codegen ではない)
3. Codegen が `Program_GammaCorrect` ブロックを parse → `Program_GammaCorrectLayout` + perfect hash entry `("color")` を生成
4. chapter 06 redirect 層が setter `uniform4fv("color", ...)` を受けて perfect hash で offset 取得 → UBO memcpy

= **Codegen 自体は集約表を持たない**、表は **chapter 05 が定義 / chapter 06 が利用**、Codegen は出力 UBO の **層** に閉じる。

---

## §8 Codegen-UBO が解決する inventory 上の課題

| inventory § | 課題 | Codegen-UBO の対応 |
|---|---|---|
| §6.2 | 85 GLSL UBO blueprint が host C++ で値来ず dead | Codegen 生成 layout に redirect 層 (chapter 06) が値を流せば実体化、blueprint は **discard せず再利用** (chapter 01 §5 #5) |
| §6.4 | `LLGLSLShader::uniform*fv()` 16 method に Vulkan path redirect 痕跡ゼロ | Codegen 生成 perfect hash を redirect 層が利用、setter 内で **path 分岐 1 箇所** で吸収可能に |
| §6.5 | bare uniform → UBO 集約粒度未定義 | Codegen は粒度判断しない (= 判断 C)、chapter 05 集約表が決定権者、Codegen はその出力 UBO だけ処理 |

---

## §9 chapter 05-08 との分担境界 (= 入出力契約)

| chapter | 本 chapter からの入力 | 本 chapter への出力 |
|---|---|---|
| 05 (existing-inventory-link) | (なし) | 集約後の UBO ブロック宣言 (= Codegen の入力 GLSL に reflect される) |
| 06 (redirect-layer-design) | `lookup_runtime()` / `g_uniform_table[]` / `<BlockName>Layout` (= §6.2 mUniformUBOLoc cache) | (なし、本 chapter は受け側を describe) |
| 07 (vulkan-api-state) | (なし、独立棚卸し) | (なし) |
| 08 (build-codegen-pipeline) | (A1)(G)(P) 3 判断の最終確定先 / CMake target 配線 / 3 OS 互換 | Codegen tool 実行 / glslang 並列実行 / 生成 dir 配置の build script |

---

## §10 未確定事項 (→ chapter 10 持ち越し)

| # | 項目 | 解消先 |
|---|---|---|
| A1 | std140 offset 計算: **Codegen 独自 calculator vs SPIR-V reflection 抽出** | chapter 08 (build pipeline 構成と紐づき) |
| G | perfect hash generator: **gperf / 独自 / frozen** 等の選択 | chapter 08 |
| D | shader 内 **動的 uniform 名** (array flatten 等) の存在確認 | chapter 06 起案時 grep |
| P | parse 手段: **独自 mini-parser vs glslang reflection 経由** | chapter 08 |

本 chapter §3 / §4 / §5 / §6 で both 道筋を describe し、最終確定は紐付け chapter (06 / 08) で行う。

---

## §11 本 chapter update 規律

- pipeline 3 stage の追加 / 変更は本 chapter に集約、他 chapter は後追い反映
- 生成物の構造変更 (= §5.1 ファイル分割) は chapter 02 §2.4 と整合維持
- 判断 (A)(B)(C) の見直しは本 chapter §3 / §5 / §7 + AYA 確認を経て update (= Claude 単独変更禁止、原則の根幹)
- §10 持越項目が chapter 06 / 08 で確定したら本 chapter から「保留候補」の語を剥がして reflect

---

**= 本 chapter で Codegen-UBO の機構が確定したため、chapter 05 で既存 85 UBO blueprint の cadence 別 mapping + bare uniform → UBO 集約対応表の整備に進める**。
