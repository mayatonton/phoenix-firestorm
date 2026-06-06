# r41 UBO 全体設計 Chapter 08: build system 統合 (CMake / glslang / Codegen pipeline)

**起案日**: 2026-06-03
**位置付け**: chapter 04 (Codegen-UBO 機構) が確定した **生成物 / interface 仕様** と、chapter 07 (Vulkan API state) が確定した **set 帯 5 化 (set=0/1a/1b/2/3) / V1' split / S3' sampler 配置 / V3a 共通 layout / std140 alignment 256 B / dummy buffer 戦略** を、実 build pipeline (CMake + glslang + Codegen tool + autobuild) に統合する仕様を確定する。chapter 04 で chapter 08 持越とされた (A1)(P)(G) と、chapter 07 handoff §1.3 で新規提示された (B1)-(B5) を本 chapter で全件確定 (= AYA 判断仰ぎ候補として default 採用案を提示)。Phase 番号体系 / 実 program 単位 migration 順序 は chapter 09 譲り。
**pre-requisite**:
- `01-overview.md` §2 (2 大設計原則) / §4 (本 chapter は 08 行)
- `04-codegen-ubo.md` §2 (3 stage pipeline) / §3 (判断 A: GLSL read-only) / §5 (生成物確定) / §10 ((A1)(G)(P) 保留)
- `07-vulkan-api-state.md` §3 (device limit) / §4 (set=1 layout) / §5 (sampler 配置) / §7.3 (offset alignment) / §9 (共通 PSO layout)
- `06a-cache-structure-and-setter-redirect.md` §3 (cache 構造) / §5 (16 method setter 分岐)

---

## §0 本 chapter の scope

### §0.1 scope (= 本 chapter で確定するもの)

1. **build pipeline 3 stage の CMake target 配線** (= chapter 04 §2.1 概念図 → 実 CMake / autobuild への落とし込み)
2. **Codegen tool 実装言語選定** (= (B1) 解消、default Python + AYA 判断仰ぎ)
3. **glslang 統合方針** (= (B2) 解消、default vendoring + AYA 判断仰ぎ)
4. **GLSL parse 手段** (= chapter 04 (P) 解消、default 独自 mini-parser + glslang -E 前処理 + AYA 判断仰ぎ)
5. **std140 offset 計算** (= chapter 04 (A1) 解消、default Codegen 独自 calculator + SPIR-V reflection 二重保証 + AYA 判断仰ぎ)
6. **perfect hash generator** (= chapter 04 (G) + chapter 07 handoff (B3) 統合解消、default 独自 frozen-table + AYA 判断仰ぎ)
7. **`ubo_metadata.inl` 出力契約** (= chapter 07 §3.1 device limit / §4.4 set 帯 5 化 / §5.4 sampler / §7.3 256 B alignment / §9.1 共通 PSO layout の Codegen 側反映)
8. **set=1 80 → 40/40 split 自動振分け** (= chapter 07 §3.2 V1' の Codegen 側自動化、name-sort deterministic 規則)
9. **dummy buffer 連動 host 側 init コード** (= chapter 07 §4.3 全 program 未使用 binding dummy bind の host 側 init 生成)
10. **build error 検出機構** (= std140 layout 不整合 / binding 衝突 / set 帯超過 / SPIR-V reflection 不一致 を build-time fail)
11. **host 側 ubo_loader header 出力** (= chapter 04 §6.2 `mUniformUBOLoc[index]` ↔ block_name の物理 instance map)
12. **増分 build cache strategy** (= (B4) 解消、default SPIR-V binary hash + GLSL ファイル mtime 併用 + AYA 判断仰ぎ)
13. **Codegen 実行 trigger** (= (B5) 解消、default CMake add_custom_command DEPENDS 自動 + 手動 target + AYA 判断仰ぎ)
14. **3 OS 互換性保証** (= Linux/Win/Mac 全 OS で Codegen tool 動作確証、autobuild 既存 stack への影響評価)

### §0.2 非 scope (= 他 chapter / 他 phase 譲り)

- Codegen 生成物の **runtime 実利用** (= setter 内での `mUniformUBOLoc[index]` 直引き) → **chapter 06a §3 / §5**
- shader 単位 migration 順序 / Phase 番号体系 → **chapter 09 (phase-roadmap)**
- AYA 判断未確定で残った全 8 件 ((A1)(P)(G/B3)(B1)(B2)(B4)(B5)+(V1')(V3a)(S3')(W) 等) の **最終確定** → **chapter 10 (open-questions)**
- bare uniform → UBO 集約対応表 → **chapter 05 §4 (= 既起案、Codegen は出力 UBO だけ処理)**
- per-LLImageGL VkImage 配線 / per-texture sampler → **領域 7 sub-step 7.5 (= 本設計の外)**
- Phase 0 計測 spec (= LL_INFOS hook / binding 重複 grep) → **chapter 06a-prep (= 既起案)**

---

## §1 入力契約

| 入力 source | 本 chapter での用途 |
|---|---|
| `04-codegen-ubo.md` §2.1 3 stage pipeline | §2 CMake target 配線の基盤 |
| `04-codegen-ubo.md` §3.1 判断 A (GLSL read-only) | §4 glslang 統合は SPIR-V 化 + reflection のみ、GLSL rewrite 経路ゼロ |
| `04-codegen-ubo.md` §5.1 生成ファイル群 (4 種) | §2 / §6 出力 dir 配線、target dependency |
| `04-codegen-ubo.md` §5.3 perfect hash UniformLocation | §5 generator / §6 出力契約 |
| `04-codegen-ubo.md` §10 (A1)(G)(P) 持越 | §3 / §4 / §5 で本 chapter 確定 |
| `07-vulkan-api-state.md` §3.1 device limit struct 拡張 (4 field) | §6 `ubo_metadata.inl` の binding 数評価出力 |
| `07-vulkan-api-state.md` §3.2 V1' 80 → 40/40 split | §7 自動振分け規則 |
| `07-vulkan-api-state.md` §4.4 set 帯 5 化 (set=0/1a/1b/2/3) | §6 出力 set 番号 |
| `07-vulkan-api-state.md` §5.4 sampler 49 set=3 同居 | §6 出力 sampler binding |
| `07-vulkan-api-state.md` §7.3 256 B alignment | §6 padding 出力規則 |
| `07-vulkan-api-state.md` §9.1 共通 PSO layout (sAYAStandardLayout) | §10 host 側 init コード生成 |
| `06a-cache-structure-and-setter-redirect.md` §3 cache 構造 / §5.6 sampler 分岐 | §10 host header 出力 (= cache 側 include 整合) |
| handoff §1.3 (B1)-(B5) | §3 / §4 / §5 / §11 / §12 で確定 |

---

## §2 build pipeline 3 stage の CMake target 配線

### §2.1 chapter 04 §2.1 概念図の CMake target 化

```
[Stage 1: build-time pre-process]
GLSL files (read-only)
  └── add_custom_command (DEPENDS: GLSL files)
      ├── codegen_ubo (Python tool 呼出)
      │     ├── 入力: app_settings/shaders/class*/{deferred,interface,...}/**.glsl
      │     └── 出力: build/codegen/ubo/*.inl
      └── glslang -V (SPIR-V 化、既存配線、本 chapter は範囲外)
            └── 出力: build/spirv/*.spv

[Stage 2: 生成物 (build artifact)]
build/codegen/ubo/
  ubo_layout_<blockname>.inl     (1 file / UBO、§5.1)
  ubo_perfect_hash.inl            (全 uniform 名集約、§5.1)
  ubo_metadata.inl                (block_name → metadata、§6)
  ubo_dummy_init.inl              (host 側 dummy buffer init、§9)
  ubo_index.inl                   (上記の include 集約、§5.1)
  ubo_host_loader.inl             (mUniformUBOLoc[index] cache pre-fill、§10)

[Stage 3: runtime 入口]
indra/newview/llviewershadermgr 等
  └── #include "ubo_index.inl"
      └── compile-time perfect hash / runtime lookup_runtime()
```

### §2.2 CMake target 構成

- **新規 CMake target**: `codegen_ubo` (custom command、Python 実行)
- **依存関係**:
  - `codegen_ubo` DEPENDS = `app_settings/shaders/**.glsl` (= GLSL 変更検出で自動再生成、§12 (B5) 解消)
  - `llrender` / `llvkloader` の OBJECT lib DEPENDS = `codegen_ubo` (= 生成 header の include 順序保証)
- **生成 dir**: `${CMAKE_BINARY_DIR}/codegen/ubo/` (= 既存 build dir 構造に整合、git ignore)
- **include path**: `target_include_directories(llrender PUBLIC ${CMAKE_BINARY_DIR}/codegen)` (= shader 側 setter から `#include "ubo/ubo_index.inl"` 形)

### §2.3 既存 GLSL build pipeline との並列性

既存 (chapter 07 §2.8 棚卸し) で SPIR-V 化は `loadSpirvShaderModule()` (`llvkloader.h:179`) 経由で実行されているが、**build-time SPIR-V 化は AYAstorm 現状 OFF** (= runtime glslang 呼出のみ)。

→ 本 chapter は **runtime glslang 経路を温存** し、Codegen は **build-time に独立 process** で実行 (= SPIR-V 化と並走、Codegen は GLSL 入力のみ参照、`.spv` 出力には依存しない)。

= **(A1) std140 一致保証は §5.2 で別途 build-time check に独立 SPIR-V 化を 1 回追加** (= reflection 抽出専用、runtime SPIR-V 化と別の独立 process)。詳細 §5.2。

---

## §3 Codegen tool 実装言語選定 (= (B1) 解消)

### §3.1 候補と評価

| # | 候補 | 利点 | 欠点 |
|---|---|---|---|
| B1a | **Python (3.8+)** | 3 OS 揃え容易 (= 各 OS 標準 / autobuild 既存 Python 入っている)、文字列 / regex / dict 強力、CMake から `find_package(Python3)` で呼出容易 | tool 自体に Python 依存 |
| B1b | C++ standalone tool | runtime 依存ゼロ (= 実行ファイル単独) | **tool build を 3 OS で先行する必要** (= cross-compile / Linux→Win build 等の経路考慮、build dependency 大増)、文字列処理コード冗長 |
| B1c | CMake script (= pure CMake) | 追加 dependency ゼロ | 文字列操作 / dict / std140 算術が弱い、独自 perfect hash 生成困難、debug 不能 |

### §3.2 確定案 = **B1a (Python)** default

**B1a 採用根拠**:
1. **3 OS 揃え (= 設計 review 2026-06-03 §3.4 詳細化)**: 
   - **Linux**: apt/dnf 標準で Python 3.10+ 提供 (Ubuntu 24.04+/Fedora 39+ 等、AYA dev 環境含む)
   - **macOS**: system Python 3.9+ (Xcode 14+ 標準) または Homebrew/autobuild bundle、@t-noami さん検証信任の Mac 環境では autobuild 経由が default
   - **Windows**: 公式 installer (`python.org`) または autobuild bundle、AYAstorm autobuild は Python 3.11 bundle 配信実績 (= `autobuild.xml` `python` package、3 OS 一律 version)
   - AYAstorm 既存 autobuild stack には `develop.py` / `autobuild` 用に Python 必須なため、本 Codegen tool の追加 dependency 影響ゼロ (= 既存 stack 内)
   - 万一 system Python 不在の Win 環境では autobuild が Python bundle を install (= `autobuild install python`)、build 時 `find_package(Python3 3.8 REQUIRED)` で autobuild 提供 path を解決
2. **文字列 / parse / hash 処理に最適**: GLSL 中 UBO block parse / std140 offset 計算 / perfect hash 生成 (= §5) いずれも Python の str/dict/list/struct で素直に書ける、~500-1000 行で完結
3. **CMake 統合容易**: `find_package(Python3 REQUIRED)` + `add_custom_command(COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/codegen_ubo.py ...)` で 1 行
4. **debug / iterate コスト最小**: tool 改修時に rebuild 不要 (= script は直接実行)、GLSL 変更検出後の Codegen 単独実行も `python codegen_ubo.py` で完結
5. **既存 AYAstorm / FS 慣習に整合**: AYAstorm / FS は autobuild + scons 由来の Python script 多用 (= `indra/develop.py` / `indra/lib/python/` / `autobuild.xml` 周辺)

**B1b 不採用根拠**: tool build を 3 OS で先行する必要 = build pipeline 自体に build dependency が再帰、Linux native 開発機で Win build 用 Codegen を cross-compile する経路を取らねばならず構築コスト大。

**B1c 不採用根拠**: CMake script は string ops が弱く、std140 offset 計算 / perfect hash table 生成のような算術重視処理に不適。

= **(B1) → B1a Python default 確定、§17 で AYA 判断仰ぎ候補に登録**。

### §3.3 Codegen tool 配置

- script path: `scripts/codegen/codegen_ubo.py` (= リポジトリ root 配下、既存 `scripts/` 構造に整合)
- 補助 module: `scripts/codegen/std140.py` (offset 計算)、`scripts/codegen/glsl_parser.py` (parse)、`scripts/codegen/perfect_hash.py` (hash 生成)
- entrypoint: `codegen_ubo.py --input-glsl-dir <path> --output-dir <path> --spirv-reflection-dir <path>` (= §11 cache key 計算もここ集約)

### §3.4 Python version 縛り

- Python 3.8+ 必須 (= f-string / typed dict / dataclass)
- CMake check: `find_package(Python3 3.8 REQUIRED)` で起動時 fail → **build error**
- 3 OS install gate は (B2) glslang + (B1) Python の 2 dependency のみ、AYAstorm 既存 autobuild が両方含むため新規追加なし想定 (= §13 で確証 phase 明記)

---

## §4 glslang 統合方針 (= (B2) 解消)

### §4.1 候補と評価

| # | 候補 | 利点 | 欠点 |
|---|---|---|---|
| B2a | **autobuild vendoring (= AYAstorm 既存)** | autobuild stack 既設、3 OS bundle 確保済、追加 install gate 不要 | バージョン更新は autobuild package 更新経由 |
| B2b | system pkg (apt/brew/vcpkg) | autobuild stack 改修不要 | 3 OS で system install gate が増える、各 OS で別 pkg 名 / version drift |
| B2c | Codegen 内に自前 SPIR-V 出力実装 | dependency ゼロ | 巨大、現実的でない |

### §4.2 確定案 = **B2a (autobuild vendoring)** default

**B2a 採用根拠**:
1. **autobuild 既設**: AYAstorm 既存 build に `glslang` package が autobuild 経由で入っている (= chapter 07 §2.8 で確認、`loadSpirvShaderModule()` で runtime glslang 呼出)、本 chapter で **追加 dependency なし**
2. **3 OS 一律**: autobuild package は Linux/Win/Mac 全 OS で同 version 提供 (= AYAstorm autobuild の慣習)
3. **build-time / runtime 共用**: runtime 用 glslang をそのまま build-time Codegen でも使う → version drift ゼロ

**B2b 不採用根拠**: system pkg は OS 別 install gate (= apt-get install glslang-tools / brew install glslang / vcpkg install glslang) を新規追加、3 OS 揃えコスト大。

**実利用範囲**:
- Codegen は glslang を **`-E` (preprocessor 展開) のみ呼出** (= 後述 §5.1 P3 採用と整合)
- build-time check の SPIR-V reflection 抽出は glslang `-V --reflect-uniform-blocks` (= §5.2)

= **(B2) → B2a autobuild vendoring default 確定、§17 で AYA 判断仰ぎ候補に登録**。

### §4.3 glslang 呼出経路

- preprocessor 展開: `glslangValidator -E <input.glsl>` → preprocess 後 GLSL を stdout 取得 → Codegen Python が parse
- SPIR-V 化 + reflection: `glslangValidator -V -S vert <input.vert> -o tmp.spv && spirv-cross --reflect tmp.spv` (= reflection は `spirv-cross --reflect` or glslang 自身の reflection API)
- glslang binary path: `find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)` で CMake 検出、autobuild bundle path を first priority に置く

---

## §5 GLSL parse 手段 + std140 offset 計算 + perfect hash generator

### §5.1 GLSL parse 手段 (= chapter 04 (P) 解消)

| # | 候補 | 利点 | 欠点 |
|---|---|---|---|
| P1 | regex | 実装最短 | ネスト / コメント / `#ifdef` 破綻、不採用 (= chapter 04 §4.2 既論) |
| P2 | glslang library reflection (= Python から binding) | 高精度、AST 取得 | Python bindings 配信が 3 OS 揃わない、tool に glslang library link 必要 |
| P3 | **独自 mini-parser + glslang -E 前処理** | preprocessor は glslang 任せ = `#ifdef` 解決済 GLSL を受取、parse コストは UBO ブロック構文のみ (~200 行)、3 OS 共通 | LL の GLSL 慣用範囲外の構文 (= 通常出てこない) はサポート要追加 |

### §5.2 確定案 = **P3 (独自 mini-parser + glslang -E 前処理)** default

**P3 採用根拠**:
1. **chapter 04 §4.2 推奨と整合**: 同 chapter で既に P3 推奨されており、chapter 08 で確定形へ
2. **3 OS 共通**: Python pure script = 3 OS で動作差異ゼロ、glslang library binding に依存しない
3. **scope 限定**: LL の GLSL UBO 宣言は (preprocess 展開後) **純粋な block 列**、ネスト無し、`std140` layout qualifier 必須、~200 行 mini-parser で fully cover
4. **debug 容易**: parse 失敗時に **入力 GLSL ファイル名 + 行番号 + 期待構文** を error log 出力可能、glslang library reflection の opaque error より診断性高

#### §5.2.1 mini-parser 詳細化 (= Phase 2d-β-revise Deliverable B-2)

P3 mini-parser の **token grammar + state machine + glslang -E 連動 + ~200 行実装 breakdown** 確定形。Phase 1.A 実装 source of truth。

##### §5.2.1.1 input 契約 (= glslang -E 前処理後の GLSL)

mini-parser 入力 = `glslangValidator -E <input.glsl>` の stdout 出力:

```glsl
// preprocess 展開後 (= AYA 想定 input 例):
#line 1 "deferred_alphaF.glsl"

layout(std140) uniform FrameViewProj {
    mat4 view;
    mat4 proj;
    mat4 view_proj;
};

layout(std140) uniform Program_GammaCorrect {
    float gamma;
    vec3 white_point;
    float exposure;
};

layout(std140) uniform Draw_MultiLight {
    int   light_count;
    vec4  light_color[8];
    float light_radius[8];
    LightCone light_cone;     // = nested struct (§5.2.1.4 対応)
};

struct LightCone {
    float cos_inner;
    float cos_outer;
    vec3  axis;
};

uniform sampler2D diffuseMap;     // = sampler (§5.2.1.5 別 path)
uniform vec4 color;               // = bare uniform (= chapter 04 §7.1 Codegen 対象外、skip)
```

**preprocess 後の特性**:
- 全 `#ifdef` / `#define` 解決済 (= GLSL preprocess は glslang 担当)
- comment は `#line` directive 以外残らない (= glslang -E が strip)
- ネスト不要 (= UBO block 内に他の UBO 宣言は GLSL spec 上不可)
- `layout(std140)` qualifier は **mini-parser が検証必須** (= 無いと std140 layout 保証なし、build error)

##### §5.2.1.2 token grammar (= EBNF level)

```ebnf
program        = { top_level_decl } ;
top_level_decl = ubo_block | struct_def | bare_uniform | sampler_decl | other_skip ;

ubo_block      = "layout" "(" layout_qual ")" "uniform" ident "{" { member_decl } "}" [ ident ] ";" ;
struct_def     = "struct" ident "{" { member_decl } "}" ";" ;
bare_uniform   = "uniform" type_ident ident [ "[" int_lit "]" ] ";" ;
sampler_decl   = "uniform" sampler_type ident ";" ;

layout_qual    = qual_item { "," qual_item } ;
qual_item      = "std140" | "binding" "=" int_lit | "set" "=" int_lit | ... ;

member_decl    = type_ident ident [ "[" int_lit "]" ] ";" ;
type_ident     = "float" | "vec2" | "vec3" | "vec4" | "int" | ... | ident ;  -- ident = nested struct name
ident          = letter { letter | digit | "_" } ;
int_lit        = digit { digit } ;

sampler_type   = "sampler2D" | "sampler3D" | "samplerCube" | "sampler2DShadow" | ... ;

other_skip     = ? not matching above, skip to next ";" ? ;
```

##### §5.2.1.3 state machine 実装 (= ~200 行の核)

```python
# scripts/codegen/glsl_parser.py
import re
from dataclasses import dataclass, field

@dataclass
class UboBlockDecl:
    block_name: str
    layout_qual: dict     # = {"std140": True, "binding": 0, ...}
    members: list         # = [{name, type, array_count, nested_struct_or_None}]
    source_file: str
    source_line: int
    instance_name: str = ""  # block 後の `} instance;` 形式 (= LL 慣用範囲外、検出時 warn)

@dataclass
class StructDef:
    name: str
    members: list
    source_file: str
    source_line: int

@dataclass
class BareUniformDecl:
    type_str: str
    name: str
    array_count: int = 0  # 0 = non-array
    source_file: str = ""
    source_line: int = 0

# Token 種別 (= regex で抽出)
TOKEN_PATTERNS = [
    ('LINE_DIR',  r'#line\s+(\d+)\s*"([^"]*)"'),
    ('LBRACE',    r'\{'),
    ('RBRACE',    r'\}'),
    ('LPAREN',    r'\('),
    ('RPAREN',    r'\)'),
    ('LBRACK',    r'\['),
    ('RBRACK',    r'\]'),
    ('COMMA',     r','),
    ('SEMI',      r';'),
    ('EQ',        r'='),
    ('INT',       r'\d+'),
    ('IDENT',     r'[a-zA-Z_][a-zA-Z_0-9]*'),
    ('WS',        r'\s+'),
    ('SKIP',      r'.'),  # unknown char = skip 1 (= other_skip 緩衝)
]
TOKEN_RE = re.compile('|'.join(f'(?P<{name}>{pat})' for name, pat in TOKEN_PATTERNS))

def tokenize(source: str):
    """generator: 1 token / iter、WS skip、LINE_DIR で current_file/line 更新"""
    line_no = 1
    file_name = "<input>"
    for m in TOKEN_RE.finditer(source):
        kind = m.lastgroup
        text = m.group()
        if kind == 'LINE_DIR':
            line_no = int(m.group(2))   # ※ m.group() で名前付き group 番号注意、実装簡略化
            file_name = m.group(3)
            continue
        if kind == 'WS':
            line_no += text.count('\n')
            continue
        yield (kind, text, file_name, line_no)


SAMPLER_TYPES = {
    'sampler2D', 'sampler3D', 'samplerCube', 'sampler2DShadow',
    'sampler2DArray', 'samplerCubeArray', 'sampler2DArrayShadow',
    'isampler2D', 'usampler2D', 'samplerBuffer', 'isamplerBuffer', 'usamplerBuffer',
}

PRIMITIVE_TYPE_IDENTS = {  # = chapter 04 §4.3.1.1 PRIMITIVE_TYPES の key 同期
    'float', 'vec2', 'vec3', 'vec4', 'int', 'uint', 'bool',
    'mat2', 'mat3', 'mat4',
    'ivec2', 'ivec3', 'ivec4', 'uvec2', 'uvec3', 'uvec4',
    'bvec2', 'bvec3', 'bvec4',
    # 'double', 'dvec*', 'dmat*' は除外 (= §4.3.1.6 build error 対象)
}


def parse_glsl(source: str) -> tuple[list[UboBlockDecl], list[StructDef], list[BareUniformDecl], list]:
    """top-level decl の state machine、struct_defs / ubo_blocks / bare_uniforms / sampler_decls 分離出力"""
    tokens = list(tokenize(source))
    i = 0
    ubo_blocks = []
    struct_defs = {}
    bare_uniforms = []
    sampler_decls = []
    
    while i < len(tokens):
        kind, text, fn, ln = tokens[i]
        
        # Case 1: `layout ( ... ) uniform <Name> { ... }`
        if kind == 'IDENT' and text == 'layout':
            block, j = _parse_ubo_block(tokens, i, fn, ln, struct_defs)
            ubo_blocks.append(block)
            i = j
            continue
        
        # Case 2: `struct <Name> { ... }`
        if kind == 'IDENT' and text == 'struct':
            sd, j = _parse_struct(tokens, i, fn, ln)
            struct_defs[sd.name] = sd
            i = j
            continue
        
        # Case 3: `uniform <type> <name> [...] ;` (= bare or sampler)
        if kind == 'IDENT' and text == 'uniform':
            type_tok = tokens[i+1] if i+1 < len(tokens) else None
            if type_tok and type_tok[0] == 'IDENT':
                type_str = type_tok[1]
                if type_str in SAMPLER_TYPES:
                    sd, j = _parse_sampler(tokens, i, fn, ln)
                    sampler_decls.append(sd)
                else:
                    bd, j = _parse_bare_uniform(tokens, i, fn, ln)
                    bare_uniforms.append(bd)
                i = j
                continue
        
        # Case 4: skip 不明 token (= other_skip = "; まで読み飛ばし")
        i = _skip_to_semi(tokens, i + 1)
    
    return ubo_blocks, list(struct_defs.values()), bare_uniforms, sampler_decls


# helper 関数群 (~120 行、各 _parse_* / _skip_to_semi / _expect_token 等)
def _parse_ubo_block(tokens, i, fn, ln, struct_defs):
    # state: layout ( <qualifier_list> ) uniform <name> { <members> } [<instance>] ;
    # 期待 token sequence を順次 expect、不一致なら CodegenError raise
    ...

def _parse_member_decl(tokens, i, struct_defs):
    # state: <type_ident> <name> [ [ <int> ] ] ;
    # type_ident が struct_defs 内なら nested 認識、外なら primitive 照合
    ...

# (実装詳細は省略、Phase 1.A 実装時に上記 grammar 通り具体化)
```

##### §5.2.1.4 nested struct 対応 (= chapter 04 §4.3.1.3 計算側との接合)

GLSL の nested struct (= UBO block 内で `struct LightCone { ... }` を使用する member) は **2 step 処理**:

1. **Pass 1**: top-level `struct` 定義を全て収集 (= struct_defs dict)
2. **Pass 2**: UBO block 内 member の type_ident が struct_defs にあれば、その member info を `nested_struct=struct_defs[type]` で記録

mini-parser 出力 → chapter 04 §4.3.1.3 `compute_struct_type_info()` が再帰計算で std140 layout 化。

**実装上の制約**:
- struct 定義は UBO block より **前方宣言必須** (= GLSL spec 順)、後方宣言は build error
- 同名 struct 重複は build error (= struct_defs 上書き検知)

##### §5.2.1.5 sampler 抽出 path (= chapter 08 §6.1 g_sampler_metadata 出力 source)

bare `uniform sampler2D <name>;` は §6.1 `g_sampler_metadata[]` 出力 source:

```python
@dataclass
class SamplerDecl:
    sampler_type: str   # = 'sampler2D' / 'samplerCube' / ...
    name: str
    source_file: str
    source_line: int

# bare_uniform path で sampler_type 検出時に SamplerDecl emit、std140 layout 計算は対象外 (= opaque type)
```

= mini-parser の **第 4 出力** (= ubo_blocks / struct_defs / bare_uniforms / sampler_decls)、§6.1 sampler metadata generator の入力。

##### §5.2.1.6 LL GLSL 慣用範囲外の検出 + build error

LL GLSL 慣用範囲外構文の検出 + 出力 format:

| 構文 | 検出箇所 | error 出力 |
|---|---|---|
| `layout(std430)` 等 std140 以外 | _parse_ubo_block layout_qual check | "ERROR: unsupported layout qualifier 'std430' in block '<name>', UBO は std140 必須" |
| `double` / `dvec*` / `dmat*` | _parse_member_decl type check | "ERROR: unsupported type '<type>' (= chapter 04 §4.3.1.6 同型)" |
| 動的 array sub-script (= `array[N]` の N が ident) | _parse_member_decl array check | "ERROR: dynamic array size '<expr>' (= compile-time const only)" |
| UBO 内に function 宣言混入 | state machine 不一致 | "ERROR: unexpected token in UBO block '<name>'" |
| 同名 UBO 重複宣言 (= chapter 04 §4.4 既述) | post-parse 集約時に dict 重複 | "ERROR: duplicate UBO block '<name>' in [file1.glsl, file2.glsl]" |
| `uniform <name> { ... } var1, var2;` (= 複数 instance) | _parse_ubo_block 末尾 ident multi | "WARN: multiple instance name in UBO '<name>', Codegen は 1 instance のみ採用" |

出力 format は chapter 08 §9.4 build error format 完全準拠:

```
[codegen_ubo] ERROR: unsupported layout qualifier 'std430'
  GLSL file: app_settings/shaders/class3/deferred/materialF.glsl:42
  Block: MaterialUBO_Legacy
  Reason: AYAstorm r41 設計は std140 layout のみサポート (= chapter 04 §3.1 判断 A)
  Action: GLSL 宣言を `layout(std140)` に修正
```

##### §5.2.1.7 #line directive 経由のエラー位置追跡

glslang -E preprocess 後の GLSL は `#line N "file"` directive で **元の GLSL ファイル + 行番号** を保持。mini-parser の `tokenize()` 内で同 directive を track し、`(kind, text, fn, ln)` の `fn/ln` を常に **元 GLSL の位置** に維持 → §5.2.1.6 error 出力で `app_settings/shaders/class3/deferred/materialF.glsl:42` (= 元の path + line) を出力可能。

= preprocess 後 token の line 番号がずれて debug 困難という P3 既知 risk を排除。

##### §5.2.1.8 実装規模見積

| 項目 | LoC |
|---|---|
| TOKEN_PATTERNS + tokenize() (§5.2.1.3) | ~30 |
| parse_glsl top-level state machine (§5.2.1.3) | ~40 |
| _parse_ubo_block + layout_qual parse | ~40 |
| _parse_struct + nested 認識 (§5.2.1.4) | ~25 |
| _parse_bare_uniform + _parse_sampler (§5.2.1.5) | ~20 |
| LL 慣用範囲外検出 + error format (§5.2.1.6) | ~30 |
| #line directive 追跡 (§5.2.1.7) | ~10 |
| unit test (= sample GLSL 10 種 round-trip) | ~80 |
| **合計** | **~275** |

= chapter 04 §4.2 「~200 行 mini-parser で fully cover」見積に整合 (= test 込み ~275、本体 ~195)。


### §5.3 std140 offset 計算 (= chapter 04 (A1) 解消)

| # | 候補 | 利点 | 欠点 |
|---|---|---|---|
| A1a | **Codegen 独自 calculator + build-time SPIR-V reflection 一致 check** | Python 実装 ~100 行、glslang 出力との不一致を build-time fail で検知 (= silent runtime corruption 排除) | reflection 抽出 process 1 回追加 (= build time 微増) |
| A1b | SPIR-V reflection 抽出のみ (= Codegen は GLSL 解釈しない) | Codegen calculator 不要 | glslang version 依存度↑、reflection format 変動に脆弱、debug 困難 |

### §5.4 確定案 = **A1a (独自 calculator + SPIR-V reflection 二重保証)** default

**A1a 採用根拠**:
1. **二重保証**: GLSL spec 7.6.2.2 std140 を Python ~100 行で実装 (= chapter 04 §3.3 既述) + glslang SPIR-V reflection の offset 値と build-time check = 不一致なら build error、両系の bug を相互検出

**set 帯 5 化注 (= 設計 review 2026-06-03 §3.4 chapter 07 §4.4 整合)**:
- A1a の二重保証 check は **set=1a / set=1b で個別実施**: Codegen 独自 calculator も SPIR-V reflection も、`subset=0` (set=1a) / `subset=1` (set=1b) を別 layout として offset 算出 → 不一致 check は subset 単位で実施
- 理由: chapter 07 §4.4 で set=1 を 1a/1b に split したため、set=1 内の binding 番号は subset 内で 0 から振り直し (= §6.3) → std140 layout は subset 単位で独立 (= UBO 単位の offset は subset 跨いで影響受けない、ただし pipeline layout 構築は両 subset 揃って 1 set として presented = §6.3)
- 実装: Codegen Python は `program_ubos` を §7.1 sort 後に 40/40 で split し、各 subset を独立 layout として offset 算出 → glslang SPIR-V reflection も `descriptor_set=1` の binding を `subset` で grouping → subset 内の binding 同士で offset 比較
2. **glslang version drift 耐性**: glslang reflection format が変わっても、Codegen 計算側が独立しているため runtime layout 自体は不変、reflection 抽出パス側だけ修復で済む
3. **debug 容易**: Codegen 出力の offset 値を Python で計算履歴付き log 可能、SPIR-V reflection 出力と diff 表示で不一致箇所即座に特定

**A1b 不採用根拠**: reflection 単独依存は glslang version 更新で format 変動した際、host 側 layout 認識まで一括破綻するリスク (= silent runtime corruption の温床)。

= **(A1) → A1a 二重保証 default 確定、§17 で AYA 判断仰ぎ候補に登録**。

#### §5.4.1 SPIR-V reflection 二重保証 mechanism 詳細化 (= Phase 2d-β-revise Deliverable B-1)

A1a 二重保証の **抽出経路 + 照合 algorithm + format drift 耐性 mechanism** 確定形。Phase 1.A (chapter 09 §4.1) 実装 source of truth。

##### §5.4.1.1 reflection 抽出経路 (= glslang → JSON intermediate)

```
[per GLSL block 単位 sequence]
1. glslangValidator -V -S <stage> <glsl_file> -o /tmp/<block>.spv
       → SPIR-V binary 出力
2. spirv-cross --reflect --output-format json /tmp/<block>.spv
       → JSON intermediate 出力 (= glslang 自身の --reflect-uniform-blocks も可、後述 §5.4.1.5)
3. Python が JSON parse → dict[block_name][member_name] = {offset, size, array_stride}
4. Codegen 独自 calculator (= chapter 04 §4.3.1) と per-member 比較
5. 不一致なら build error (= §9.4 format)
```

**JSON intermediate 中継採用根拠**:
- glslang reflection API は C++ で、Python 呼出には binding 必要 → 不採用
- `spirv-cross --reflect --output-format json` は **3 OS 共通 / autobuild 既存配信** (= chapter 07 §2.8、`indra/llrender/llvkloader.cpp` で runtime 使用済)
- JSON format は **glslang version 1.3.224+ で stable** (= AYAstorm 実装 Ubuntu 24.04 `apt install glslang-dev` 15.1.0-2 以降 (= chapter 10 §1.2 (B2) **B2b system pkg 確定**、autobuild bundle 経路は不使用)、format drift 観測対象は schema レベルのみ)

##### §5.4.1.2 JSON schema 期待形 (= spirv-cross 出力)

```json
{
  "types": {
    "_FrameViewProj": {
      "name": "FrameViewProj",
      "members": [
        {"name": "view",      "type": "mat4", "offset": 0,   "matrix_stride": 16},
        {"name": "proj",      "type": "mat4", "offset": 64,  "matrix_stride": 16},
        {"name": "view_proj", "type": "mat4", "offset": 128, "matrix_stride": 16}
      ]
    }
  },
  "ubos": [
    {
      "type": "_FrameViewProj",
      "name": "FrameViewProj",
      "block_size": 192,
      "set": 0,
      "binding": 0
    }
  ]
}
```

**Codegen が抽出する field**:
- 各 member の `offset` (= 必須 = 二重保証主軸)
- 各 member の `array_stride` (= 配列時のみ)
- block の `block_size` (= padding 後 size 確認 = §6.4 256B padding 前の std140 size)
- block の `set` / `binding` (= chapter 07 §4 set 帯確認 = §9.1 E2 / E7 補強)

##### §5.4.1.3 per-member 照合 algorithm (= chapter 04 §4.3.1.7 接合具体化)

```python
# scripts/codegen/spirv_reflect.py
import subprocess
import json
from pathlib import Path

def extract_reflection(glsl_file: Path, stage: str, glslang_path: Path, spirv_cross_path: Path) -> dict:
    """1 GLSL ファイル → reflection dict"""
    spv_tmp = glsl_file.with_suffix('.spv.tmp')
    
    # Stage 1: SPIR-V 化
    res = subprocess.run(
        [str(glslang_path), '-V', '-S', stage, str(glsl_file), '-o', str(spv_tmp)],
        capture_output=True, text=True, check=False
    )
    if res.returncode != 0:
        raise CodegenError(f"glslang SPIR-V 化失敗: {glsl_file}\n{res.stderr}")
    
    # Stage 2: reflection JSON 抽出
    res = subprocess.run(
        [str(spirv_cross_path), '--reflect', '--output-format', 'json', str(spv_tmp)],
        capture_output=True, text=True, check=False
    )
    if res.returncode != 0:
        raise CodegenError(f"spirv-cross reflection 失敗: {spv_tmp}\n{res.stderr}")
    
    refl = json.loads(res.stdout)
    spv_tmp.unlink(missing_ok=True)
    
    # Stage 3: dict[block_name][member_name] = {offset, size, array_stride} に正規化
    result = {}
    for ubo in refl.get('ubos', []):
        block_name = ubo['name']
        type_def = refl['types'][ubo['type']]
        result[block_name] = {
            '_block_size': ubo.get('block_size', None),
            '_set': ubo.get('set', None),
            '_binding': ubo.get('binding', None),
        }
        for m in type_def['members']:
            result[block_name][m['name']] = {
                'offset': m['offset'],
                'array_stride': m.get('array_stride', 0),
            }
    return result


def verify_layout_against_spirv(
    block_name: str,
    codegen_layout: list,        # = chapter 04 §4.3.1.2 compute_layout 出力
    codegen_block_size: int,
    spirv_refl: dict,
):
    """照合 algorithm 本体 = per-member offset / block_size / array_stride 比較"""
    if block_name not in spirv_refl:
        raise CodegenError(
            f"[codegen_ubo] ERROR: block '{block_name}' 未検出 in SPIR-V reflection\n"
            f"  Codegen は parse 成功、SPIR-V reflection は未認識 → glslang preprocess 差 or block name 揺れ\n"
            f"  Action: GLSL の `layout(std140) uniform <BlockName> {{ ... }}` 宣言を確認"
        )
    
    spv_block = spirv_refl[block_name]
    
    # check 1: block_size 一致 (= padding 後 std140 size)
    if spv_block.get('_block_size') != codegen_block_size:
        raise CodegenError(
            f"[codegen_ubo] ERROR: block size mismatch in '{block_name}'\n"
            f"  Codegen calculation:        {codegen_block_size}\n"
            f"  glslang SPIR-V reflection:  {spv_block['_block_size']}\n"
            f"  Likely cause: 末尾 padding miss or member size 計算違い"
        )
    
    # check 2: per-member offset 一致
    for member in codegen_layout:
        name = member['name']
        if name not in spv_block:
            raise CodegenError(
                f"[codegen_ubo] ERROR: member '{block_name}.{name}' 未検出 in SPIR-V reflection"
            )
        spv_offset = spv_block[name]['offset']
        if spv_offset != member['offset']:
            raise CodegenError(
                f"[codegen_ubo] ERROR: std140 offset mismatch in '{block_name}.{name}'\n"
                f"  Codegen calculation:        OFFSET = {member['offset']}\n"
                f"  glslang SPIR-V reflection:  OFFSET = {spv_offset}\n"
                f"  Diff = {abs(spv_offset - member['offset'])} bytes\n"
                f"  Likely cause: array stride / vec3 hole / nested struct padding\n"
                f"  Action: chapter 04 §4.3.1 algorithm review or glslang version drift 確認"
            )
        # check 3: array stride 一致 (= array member のみ)
        if member.get('array_stride', 0) > 0:
            spv_stride = spv_block[name].get('array_stride', 0)
            if spv_stride != member['array_stride']:
                raise CodegenError(
                    f"[codegen_ubo] ERROR: array stride mismatch in '{block_name}.{name}'\n"
                    f"  Codegen: stride = {member['array_stride']}\n"
                    f"  glslang: stride = {spv_stride}\n"
                    f"  Action: chapter 04 §4.3.1.4 arrayify() review"
                )
```

##### §5.4.1.4 二重保証 build error 出力 (= chapter 08 §9.4 format 完全準拠)

照合不一致時の error 出力例:

```
[codegen_ubo] ERROR: std140 offset mismatch in 'FrameViewProj.view_proj'
  Codegen calculation:        OFFSET = 128
  glslang SPIR-V reflection:  OFFSET = 144
  Diff = 16 bytes (= likely missing padding for mat3 or vec3 trailing)
  Input GLSL: app_settings/shaders/class3/deferred/materialF.glsl:42
  Action: chapter 04 §4.3.1 algorithm review or glslang version drift 確認
```

= AYA / Claude が **即座に GLSL ファイル + 行番号 + 不一致 byte 差** で原因究明可能。silent runtime corruption 排除。

##### §5.4.1.5 format drift 耐性 mechanism

glslang / spirv-cross の reflection output JSON schema が version upgrade で変動した場合の耐性:

| drift 種別 | 検出 + 対応 |
|---|---|
| field 追加 (= 新 key 追加) | dict.get(...) で安全に skip、既存 check 影響なし |
| field 削除 (= 既存 key 消滅) | `KeyError` → CodegenError で raise (= 既存 check 失敗で build error) |
| field rename (= `offset` → `byte_offset` 等) | 同上 KeyError → 早期検知 |
| nested 構造変更 (= `types`/`ubos` の階層変動) | extract_reflection 内で AttributeError → CodegenError で raise |

**実装契約**: `extract_reflection()` 関数を **唯一の format 抽象化境界** とし、JSON schema 変動はここで局所化。本関数以外は正規化済 dict のみ受取 = 上位 layer は format drift 影響ゼロ。

**format version pin** (= 2026-06-03 η-30 PA-1 entry 直前 post-completion correction = AYA「A」確定):

- **glslang**: ✅ B2b system pkg 確定 (= 実装で先行 commit 済) = `indra/cmake/Glslang.cmake` で `find_package(glslang CONFIG REQUIRED)` + `glslang-15.1.0/` vendored + Ubuntu 24.04 `apt install glslang-dev` (15.1.0-2) 経路 (= chapter 10 §1.2 (B2) verdict)。Linux first-class baseline (r41 charter §1)、Win/Mac 3 OS bundle は r42-α/β 着手時に判断 (charter §7.5)。autobuild manifest `autobuild.xml` 経路は **不使用** (= system pkg 経路で version 制御 = `apt` 側固定 + `glslang-15.1.0/` vendored で reference 担保)
- **spirv-cross**: η-30 PA-1 で取込 (= Glslang.cmake と同 pattern) = `indra/cmake/SpirvCross.cmake` 起案 + system install + `find_package(spirv_cross_c_shared CONFIG REQUIRED)` (= autobuild manifest 経路は不使用、Linux first-class baseline、Win/Mac 3 OS bundle は r42-α/β 時)
- **Python**: `indra/cmake/Python.cmake` で `find_package(Python3 COMPONENTS Interpreter)` = build tool として host 環境探索 (= autobuild manifest 経路は不使用、system Python 3.x+ 前提)
- **version drift 抑制 mechanism**: §11.5.1 cache key environment block (= `glslang_version` / `spirv_cross_version` / `python_version`) を Codegen tool が runtime 取得して cache invalidation key に使用 (= `apt` upgrade / virtualenv 切替で自動 invalidate)、autobuild manifest pin と等価な drift 検知効果を確保
- **(将来 r42-α/β + Phase K+4 OpenGL 撤廃時)**: Win/Mac 3 OS bundle 判断 + 最終 version pin policy 確定推奨 (= 必要なら autobuild_package 起こす)。それまで Linux first-class + system pkg pattern で進行

##### §5.4.1.6 二重保証 完全省略 escape hatch (= 緊急 build 用)

通常運用では二重保証必須だが、Phase 1.A 開発中 (= glslang reflection 経路自身が WIP な期間) の escape として:

```bash
# 環境変数で reflection check 一時 skip:
AYA_CODEGEN_SKIP_SPIRV_CHECK=1 cmake --build .
```

```python
# scripts/codegen/codegen_ubo.py 内
import os
if os.environ.get('AYA_CODEGEN_SKIP_SPIRV_CHECK') == '1':
    # warn を必ず出力 (= silent skip 禁止)
    print("[codegen_ubo] WARN: SPIR-V reflection check skipped (= AYA_CODEGEN_SKIP_SPIRV_CHECK=1)")
else:
    verify_layout_against_spirv(...)
```

**運用規律**:
- escape hatch 使用は Phase 1.A 入口 ~ Phase 1.A 中盤までの WIP 期間限定
- 通常 build / CI / release では **必ず check ON** (= cmake script で warn → build error に昇格させる option 提供、chapter 09 §13 起案規律で固定)
- escape hatch 使用中の commit は `[WIP]` prefix 必須 (= AYA release flow `feedback_release_flow` 補強)

##### §5.4.1.7 set=1 split 整合 (= §5.4 既述の subset 単位 check)

§5.4 既述: set=1a / set=1b で個別 check を実施。具体実装:

```python
# §7.1 split 後の subset 単位 verify:
for ubo in all_ubos:
    if ubo.descriptor_set == 1:
        # subset で grouping 済 (= §7.1)、各 subset 内で binding が 0..39 / 0..38
        verify_layout_against_spirv(
            ubo.block_name, ubo.codegen_layout, ubo.codegen_block_size,
            extract_reflection(ubo.glsl_file, ubo.shader_stage, ...)
        )
    else:
        verify_layout_against_spirv(...)  # set=0/2/3 は subset 区別不要
```

subset の binding 番号は **layout 計算結果に影響しない** (= std140 offset は member 内の type/alignment のみで決定、binding 番号は descriptor set 配線情報) → subset 区別は §6.3 host 側 `sProgramSetLayoutA/B` 構築のみで反映、reflection 照合は通常 path。

##### §5.4.1.8 実装規模見積

| 項目 | LoC |
|---|---|
| extract_reflection (§5.4.1.3) | ~50 |
| verify_layout_against_spirv (§5.4.1.3) | ~70 |
| JSON schema 抽象化 + drift 検知 (§5.4.1.5) | ~30 |
| escape hatch + warn (§5.4.1.6) | ~15 |
| error format (§5.4.1.4) | ~30 |
| unit test (= 既知 std140 sample 5 種で reflection vs calculator 一致) | ~80 |
| **合計** | **~275** |

= chapter 04 §4.3.1.8 calculator (~345 行) + 本 §5.4.1.8 reflection (~275 行) ≈ 620 行 = chapter 08 §3.2 Python tool 「~500-1000 行」の主要 module 2 件で大半占有。

### §5.5 perfect hash generator (= chapter 04 (G) + handoff (B3) 統合解消)

| # | 候補 | 利点 | 欠点 |
|---|---|---|---|
| G1/B3a | gperf | 老舗、信頼性高 | 外部 tool dep、3 OS 揃え (Win で MSYS / mingw 必要)、autobuild bundle に gperf 入ってない |
| G2/B3b | **独自 Python frozen-table generator** | dependency ゼロ、~500 行、build 制御容易、3 OS 共通 | 実装 + 検証コスト (= 衝突無し保証 algorithm に CHD or FCH or 二段 hash 等を採用) |
| G3/B3c | frozen 等 C++17 header-only library | 外部 binary 不要、header だけ | compile-time 負担、template instantiate 多数で build time 増、ややサイズ重め |
| G4/B3d | 既存 LL hash 利用 (= LLViewerShaderMgr 等で使う簡易 hash) | LL 既存資産 | 衝突保証なし、本設計の build-time 衝突 0 保証 (= chapter 04 §5.4) 達成不可、**不適合** |

### §5.6 確定案 = **G2/B3b (独自 Python frozen-table generator)** default

**G2/B3b 採用根拠**:
1. **dependency ゼロ**: gperf の OS 別 install gate (= G1) を回避、autobuild stack に gperf 追加不要、3 OS 共通動作
2. **build 制御容易**: 衝突 0 を Python が build-time に保証 + 不一致時に **どの 2 つの uniform 名が衝突したか** を error log で人読可能 (= gperf の opaque error より良い)
3. **algorithm**: CHD (Compress-Hash-Displace) または FCH (Fox-Chen-Heath) を採用、~500 行 Python で実装、入力 ~80 UBO × 平均 ~10 member ≈ 800 entry 規模に十分高速 (Phase 2 step 1 grep 確定)
4. **出力形式**: chapter 04 §5.3.2 概念形 (= `constexpr UniformLocation g_uniform_table[N]` + `constexpr uint32_t hash_name(const char*)` + `template<auto Name> constexpr UniformLocation resolve()`) を C++ header に書き出す

**G1 不採用根拠**: 3 OS 揃え (= Win で MSYS/mingw 必要) で autobuild に新規 package 追加コスト、現状 AYAstorm autobuild に gperf 不在。
**G3 不採用根拠**: header-only library は compile time 負担と template instantiate 数で build time 増、~800 entry に過剰。
**G4 不採用根拠**: 衝突保証なし、本設計の build-time 衝突 0 要件 (chapter 04 §5.4) を達成不可。

= **(G) + (B3) → G2/B3b Python frozen-table default 確定、§17 で AYA 判断仰ぎ候補に登録**。

### §5.7 G2/B3b 出力例

```cpp
// build/codegen/ubo/ubo_perfect_hash.inl (auto-generated)
#pragma once
#include <cstdint>

namespace ubo {

struct UniformLocation {
    uint32_t block_hash;    // 0 = invalid
    uint32_t offset;        // block 内 byte offset
    uint32_t size;          // write 量 (bytes)
    uint32_t cadence_tag;   // 0=per-frame / 1=per-program / 2=per-draw / 3=per-asset
};

// FNV-1a 32-bit (= deterministic、衝突は frozen-table 側で吸収)
constexpr uint32_t hash_name(const char* s) {
    uint32_t h = 0x811c9dc5u;
    while (*s) { h ^= static_cast<uint8_t>(*s++); h *= 0x01000193u; }
    return h;
}

// frozen-table (= CHD displacement 表 + value 表)
inline constexpr uint16_t g_displacement[/* M */] = { /* ... */ };
inline constexpr UniformLocation g_values[/* N */] = { /* ... */ };

inline constexpr const UniformLocation* lookup_runtime(const char* name) {
    const uint32_t h = hash_name(name);
    const uint32_t bucket = (h ^ g_displacement[h % /* M */]) % /* N */;
    return &g_values[bucket];
}

} // namespace ubo
```

---

## §6 `ubo_metadata.inl` 出力契約 (= chapter 07 反映)

### §6.1 出力 schema (= 設計 review 2026-06-03 §3.4 schema 整合)

**schema の論理 5 set 帯 表現規約**:
- `descriptor_set` field は **論理 set ID をそのまま 0/1/2/3 で格納**、`subset` field で set=1 の subset (1a/1b) を区別する 2 段構造
- 論理 set 帯 = 5 (= set=0/1a/1b/2/3、chapter 07 §4.4) ↔ schema field = `(descriptor_set, subset)` 2 タプル
- 例: set=1a → `(descriptor_set=1, subset=0)` / set=1b → `(descriptor_set=1, subset=1)` / set=2 → `(descriptor_set=2, subset=0)` (= subset 未使用は常に 0)
- host 側 `sProgramSetLayoutA` / `sProgramSetLayoutB` 構築時 (= chapter 07 §9.1) は `descriptor_set==1` を `subset` で 2 グループに分割して別 `VkDescriptorSetLayout` 生成 (= §6.3)
- chapter 07 §4.4.1 「bind 時 4 set 制約」は **schema 側では関与しない** (= host 側の `vkCmdBindDescriptorSets` 呼出 sequence で表現)

```cpp
// build/codegen/ubo/ubo_metadata.inl (auto-generated)
#pragma once
#include <cstdint>

namespace ubo {

enum class CadenceTag : uint8_t {
    PerFrame   = 0,  // → descriptor_set=0
    PerProgram = 1,  // → descriptor_set=1, subset=0 or 1 (§7.1 sort で振分)
    PerDraw    = 2,  // → descriptor_set=2 (dynamic offset)
    PerAsset   = 3,  // → descriptor_set=3
    PerSkin    = 4,  // → descriptor_set=3 (UBO binding 番号別、sampler 49 も同 set=3 §5.4)
    Singleton  = 5,  // → descriptor_set=0 binding 末尾 (= Global_*)
};

struct UboMetadata {
    const char* block_name;
    uint32_t    block_hash;
    uint32_t    block_size;        // std140 final size (= alignment padding 込み、§6.4 で 256 B 切上)
    uint16_t    descriptor_set;    // 論理 set ID = 0 / 1 / 2 / 3 (set=1 内 subset は subset field、§6.3)
    uint16_t    binding;           // set 内 binding 番号 (= subset 内 0-39 / 0-38 for set=1a/1b)
    uint16_t    subset;            // descriptor_set==1 時のみ意味 (0=1a / 1=1b)、他は 0 固定
    CadenceTag  cadence;
    uint16_t    member_count;
};

inline constexpr UboMetadata g_ubo_metadata[/* 88 */] = {
    // chapter 04 §5.1 で生成、chapter 07 §3.2 V1' split 結果反映
    { "FrameViewProj",            0xXXXXXXXX,   192, 0, 0, 0, CadenceTag::PerFrame,   3 },
    { "FrameLights",              0xYYYYYYYY,   512, 0, 1, 0, CadenceTag::PerFrame,   8 },
    // ...
    { "Program_GammaCorrect",     0xZZZZZZZZ,    16, 1, 0, 0, CadenceTag::PerProgram, 1 },
    // set=1 split で名前 sort 前半 40 個は subset=0 (= set=1a)、後半 39 個は subset=1 (= set=1b)
    { "Program_MaterialBasic",    0xWWWWWWWW,   128, 1, 0, 1, CadenceTag::PerProgram, 4 },
    // ...
};

inline constexpr uint32_t g_ubo_count = /* 88 */;

// sampler 49 個も同居出力 (= chapter 07 §5.4 set=3 同居)
struct SamplerBinding {
    const char* sampler_name;
    uint32_t    name_hash;
    uint16_t    descriptor_set;    // 3 固定
    uint16_t    binding;           // set=3 内 binding (= 3 + 0..48 = 3..51)
    uint16_t    stage;             // VK_SHADER_STAGE_FRAGMENT_BIT 固定
};

inline constexpr SamplerBinding g_sampler_metadata[/* 49 */] = {
    { "diffuseMap",  0xAAAAAAAA, 3,  3, /* FRAGMENT */ 0x10 },
    { "normalMap",   0xBBBBBBBB, 3,  4, /* FRAGMENT */ 0x10 },
    // ...
};

} // namespace ubo
```

### §6.2 chapter 07 反映項目 + (B1)-(B5) handoff item 接続 (= 設計 review 2026-06-03 §3.4 接続明示)

| chapter 07 確定事項 | 本 chapter §6 反映 | 関連 (B1)-(B5) handoff item |
|---|---|---|
| §3.1 device limit 拡張 (4 field) | 直接出力なし (= runtime query で host が取得)、Codegen 出力には影響なし | — |
| §3.2 V1' set=1 80 → 40/40 split | `subset` field 出力 (= 0/1)、§7 で sort 規則確定 | (B3) perfect hash も subset 単位で衝突 check (= §5.6 G2/B3b、subset 内で hash 衝突保証) |
| §4.4 set 帯 5 化 (set=0/1a/1b/2/3) | `descriptor_set` 値 0/1/2/3、set=1a/1b は subset で区別 (= §6.1 schema 規約) | (B5) 自動 trigger で GLSL 変更 → set=1 sort + subset 再振分が自動波及 (§7.2) |
| §5.4 sampler 49 set=3 同居 | `g_sampler_metadata[]` 配列出力、binding 3..51 | (B1) Python tool が GLSL parse で sampler 抽出 (= §3 + §5.1 P3) |
| §7.3 256 B alignment | `block_size` は std140 計算後 256 B multiple 切上 (= padding 込み)、§6.4 | (A1) A1a 二重保証は padding 後 size でも実施、SPIR-V reflection の `Block.size` と比較 |
| §9.1 共通 PSO layout | 直接出力なし (= host 側 init コード §10 で参照)、layout 構築は host 側 | (B2) glslang autobuild vendoring で 3 OS layout 出力一律性確保 |
| §4.4.1 bind 時 4 set 制約 | 直接出力なし (= bind sequence は host 側 §9.2) | (B4) cache 機構は bind sequence の変更検知不要 (= GLSL 不変なら cache hit) |

### §6.3 set=1a / set=1b の subset 表現

`descriptor_set=1` + `subset=0/1` で表現 (= 1 layout 内に subset = 別 VkDescriptorSetLayout 2 種に展開)。host 側 `sProgramSetLayoutA` / `sProgramSetLayoutB` は `subset=0/1` を filter して構築。

### §6.4 256 B padding 規則

std140 計算後の `block_size` を **必ず 256 B multiple に切上** (= chapter 07 §7.3 `minUniformBufferOffsetAlignment` 最大値 = device 別差異吸収):

```python
def pad_to_256(size):
    return (size + 255) & ~255
```

= per-draw ring buffer の dynamic offset は **256 B 単位** で確保され、Vulkan 1.3 spec 最大 256 を死守。

---

## §7 set=1 80 → 40/40 split 自動振分け (= chapter 07 §3.2 V1' 実装)

### §7.1 sort 規則

Program_* UBO 80 個を **block_name 文字列の lexicographic sort** で deterministic 順序化:
- 前半 40 個 → `subset=0` (= set=1a)
- 後半 40 個 → `subset=1` (= set=1b)

```python
program_ubos = [u for u in all_ubos if u.cadence == CadenceTag.PerProgram]
program_ubos.sort(key=lambda u: u.block_name)
for i, u in enumerate(program_ubos):
    u.subset = 0 if i < 40 else 1
    u.binding = i % 40 if i < 40 else i - 40
```

### §7.2 不変性保証

- block_name は chapter 02 §3 rename 後、`Program_<Domain><Purpose>` 規約で固定 → sort 結果が GLSL 変更されない限り deterministic
- 新規 Program_* UBO 追加時は **40 個目以降にずれ込み発生**、build 時に `g_ubo_metadata[]` 出力が変わる = 再 build 必要 (= CMake DEPENDS で自動検出、§12)

### §7.3 80 → 40/40 split の妥当性 (= chapter 07 §3.2 再掲)

- Vulkan 1.3 spec 最小 `maxDescriptorSetUniformBuffers=72` ≥ 40 = **全 device で set=1a / set=1b ともに 1 set fit**
- 不均衡 split (= 例: 60/20) 不要、name-sort で 40/40 deterministic 化 = AYA 実機計測 (chapter 07 §3.3 = 06a-prep §3 と並走) で OK 検知時に **runtime split 不要、layout 固定で起動**

### §7.4 set=1 split 妥当性 build-time check

Codegen 出力後、build-time check で:
1. `subset=0` 帯 binding 数 ≤ 40
2. `subset=1` 帯 binding 数 ≤ 39
3. 各 subset 内 binding 番号衝突なし (= 0..39 / 0..38)

不適合なら **build error** (= silent layout corruption 排除)。

---

## §8 dummy buffer 連動 host 側 init コード (= chapter 07 §4.3 反映)

### §8.1 dummy buffer 仕様

chapter 07 §4.3 = 全 program で未使用 binding に dummy `VkBuffer` 参照を `vkUpdateDescriptorSets` で投入。

- dummy buffer = 起動時 1 個 `vmaCreateBuffer(size=1 KB, HOST_VISIBLE+MAPPED)` (= chapter 07 §4.3)
- 全 program で **どの binding が未使用か** は Codegen が build-time に判定可能 (= GLSL parse で `uniform Program_XXX { ... } u_xxx;` 宣言の有無)

### §8.2 ubo_dummy_init.inl 出力

```cpp
// build/codegen/ubo/ubo_dummy_init.inl (auto-generated)
#pragma once
#include "ubo_metadata.inl"

namespace ubo {

// program_name → 未使用 binding bitset
struct ProgramDummyMask {
    const char* program_name;
    uint64_t    unused_subset_a_mask;  // bit i = subset=0 binding i が未使用
    uint64_t    unused_subset_b_mask;  // bit i = subset=1 binding i が未使用
};

inline constexpr ProgramDummyMask g_program_dummy_masks[/* shader 数、暫定 200 */] = {
    { "deferred_alpha",          0xFFFFFFFFFFFFFFFEull /* binding 0 = used, rest = dummy */,
                                  0xFFFFFFFFFFFFFFFFull /* subset=1 全 dummy */ },
    // ...
};

inline constexpr uint32_t g_program_count = /* 200 */;

} // namespace ubo
```

### §8.3 host 側使用パターン (= chapter 06a / chapter 07 §4.3 ref)

host 側 (= `llvkloader.cpp` or `llglslshader.cpp`) の shader bind 時に:
```cpp
const auto& mask = lookup_program_mask(shader_name);
for (uint32_t i = 0; i < 40; ++i) {
    if (mask.unused_subset_a_mask & (1ull << i)) {
        // dummy buffer を binding i に vkUpdateDescriptorSets 投入
    }
}
// 同様 subset_b
```

= **host 側 init は build-time に固定**、runtime 走査ゼロ。

### §8.4 GLSL parse での「未使用」判定基準

- GLSL 内に `uniform Program_XXX { ... } u_xxx;` 宣言が **無い** shader = unused mask = 1
- 宣言があっても `u_xxx.member` を **shader code 中で 1 度も参照していない** 場合は `u_xxx` 全体 unused (= driver が compile 時 optimize out、host は dummy 投入不要だが安全のため dummy 投入)
- 単純化のため **「宣言の有無のみ」** で unused 判定 default、参照解析は §17 (P-future) で保留

---

## §9 build error 検出機構

### §9.1 検出すべき不整合 (= silent corruption 排除)

| # | 不整合 | 検出手段 | 失敗時動作 |
|---|---|---|---|
| E1 | std140 layout 不整合 (Codegen 独自 calculator vs SPIR-V reflection) | §5.4 build-time check | build error + 該当 UBO 名 + Codegen offset vs reflection offset 出力 |
| E2 | UBO ブロック binding 衝突 (= 同 set / 同 binding 番号で 2 UBO) | Codegen build-time check | build error + 衝突 2 UBO 名 + binding 番号出力 |
| E3 | set 帯超過 (set=0 が `maxDescriptorSetUniformBuffers` 超え) | Codegen build-time check | build warn (= 起動 runtime 計測で AYA 実機限界判定、build error にしない、§9.4) |
| E4 | perfect hash 衝突 | §5.6 generator 内 build-time check | build error + 衝突 2 uniform 名出力 |
| E5 | 同名 UBO 複数 GLSL 宣言の member 構成不一致 | §5.4 + chapter 04 §4.4 既述 | build error + 該当 UBO 名 + 不一致 GLSL ファイル名出力 |
| E6 | sampler 49 + Asset/Skin UBO 合計 > `maxDescriptorSetSamplers` (= 96) | Codegen build-time check | build warn (= runtime device 計測で確認、build error にしない) |
| E7 | set=1a / set=1b binding 数 > 40 / 40 (= V1' split 不適合) | §7.4 build-time check | build error + Program_* 数 出力、Codegen の sort 規則 review 要 |

### §9.2 全部 stop-the-line 方針

E1 / E2 / E4 / E5 / E7 は **build error で停止** (= silent corruption は本設計の thesis violation、build pipeline で死守)。

### §9.3 device limit 関連 (E3 / E6) は warn のみ

E3 / E6 は **device 依存** (= AYA 実機 spec 最小 72/96 を超える driver 多数)、build error にせず warn log + runtime 起動時 device limit 計測 (chapter 07 §3.3 = 06a-prep §3) で実機検証任せ。

build error にしないが、build log で **明示 warn 出力** + Phase 0 計測項目に追加。

### §9.4 build error 出力 format

```
[codegen_ubo] ERROR: std140 offset mismatch in 'FrameViewProj'
  Codegen calculation:        view_proj OFFSET = 128
  glslang SPIR-V reflection:  view_proj OFFSET = 144
  Diff = 16 bytes (= likely missing padding for mat3 or vec3 trailing)
  Input GLSL: app_settings/shaders/class3/deferred/materialF.glsl:42
```

= AYA / Claude が即時診断可能な log 設計。

---

## §10 host 側 ubo_loader header 出力 (= chapter 04 §6.2 / 06a §3 反映)

### §10.1 出力責務

chapter 04 §6.2 で確定の **`mUniformUBOLoc[index]` cache** pre-fill を host 側で実行するための header を Codegen が生成:

```cpp
// build/codegen/ubo/ubo_host_loader.inl (auto-generated)
#pragma once
#include "ubo_perfect_hash.inl"
#include "ubo_metadata.inl"

namespace ubo {

// LLGLSLShader::link() 直後に呼ぶ post-process:
inline void prefill_uniform_ubo_loc(
    const char* shader_name,
    const std::vector<std::string>& uniform_names,  // = mUniform[index] の name 配列
    std::vector<UniformLocation>& out_loc           // = mUniformUBOLoc[index] への書込先
) {
    out_loc.resize(uniform_names.size());
    for (size_t i = 0; i < uniform_names.size(); ++i) {
        const UniformLocation* loc = lookup_runtime(uniform_names[i].c_str());
        out_loc[i] = (loc != nullptr) ? *loc : UniformLocation{0, 0, 0, 0};
    }
}

// shader_name → 共通 PSO layout 構築 (= chapter 07 §9.1 sAYAStandardLayout 参照):
inline void build_pso_layout_for_shader(
    const char* shader_name,
    /* VkDescriptorSetLayout[] params, etc. */
);

} // namespace ubo
```

### §10.2 chapter 06a §3 cache 構造との接合

- `LLGLSLShader::mUniformUBOLoc` (= chapter 06a §3 で新設) の **type** = `std::vector<ubo::UniformLocation>` (= 本 chapter §5.3 schema)
- shader link 後の post-process で `prefill_uniform_ubo_loc()` 呼出 = 1 度のみ実行、frame 内 setter call は **index 直引き** (= chapter 04 §6.2 R3 達成)

### §10.3 共通 PSO layout 構築コード

chapter 07 §9.1 `sAYAStandardLayout` の構築 = 全 program 共通 = host 側で 1 回構築 + 全 shader 使い回し。Codegen は **layout description (= `VkDescriptorSetLayoutBinding[]` 配列定数)** を出力、`vkCreateDescriptorSetLayout` 呼出は host 側 (= `llvkloader.cpp::createStandardPipelineLayout()` 拡張)。

```cpp
// ubo_host_loader.inl 末尾
inline constexpr VkDescriptorSetLayoutBinding g_set0_bindings[] = { /* per-frame 4 binding */ };
inline constexpr VkDescriptorSetLayoutBinding g_set1a_bindings[/* 40 */] = { /* set=1a */ };
inline constexpr VkDescriptorSetLayoutBinding g_set1b_bindings[/* 39 */] = { /* set=1b */ };
inline constexpr VkDescriptorSetLayoutBinding g_set2_bindings[] = { /* per-draw 4 dynamic */ };
inline constexpr VkDescriptorSetLayoutBinding g_set3_bindings[/* 52 */] = { /* per-asset 3 UBO + 49 sampler */ };
```

= host 側 `createStandardPipelineLayout()` (`llvkloader.cpp:2557`) を **本 header 参照に書換え**、追加 layout 構築コード ~80 行で完結。

---

## §11 増分 build cache strategy (= (B4) 解消)

### §11.1 候補と評価

| # | 候補 | 利点 | 欠点 |
|---|---|---|---|
| B4a | **SPIR-V binary hash + GLSL ファイル mtime 併用** | mtime で粗判定 → 一致なら skip、不一致なら SPIR-V 化 → hash 比較で実 diff 確認、無駄な Codegen 実行回避 | hash 計算 cost (= SHA256 等、~ms order) |
| B4b | mtime のみ | cost 最小 | mtime 変動 (= git checkout 等) で不要 rebuild、cache miss 多発 |
| B4c | hash only (= GLSL ファイル hash) | mtime 影響なし | 毎 build で全 GLSL hash 計算 (= 全体 ~MB の hash、~100 ms) |
| B4d | ccache 風 system | 既存資産 | 3 OS 揃え + tool config 複雑 |

### §11.2 確定案 = **B4a (hash + mtime 併用)** default

**B4a 採用根拠**:
1. **粗判定 mtime + 精判定 hash**: mtime 一致なら skip (= cost ゼロ)、mtime 不一致なら GLSL ファイル content hash (= SHA256) を計算、前回 build cache と比較 → 一致なら Codegen skip (= 内容無変更で touch のみ)
2. **cache disk footprint**: `build/codegen/cache/codegen_state.json` に `{file: {mtime, sha256, codegen_output_hash}}` の dict を JSON 保存、~10-100 KB
3. **3 OS 共通**: Python hashlib.sha256 + os.path.getmtime で実装、外部 tool 依存ゼロ

**B4b 不採用根拠**: git checkout で mtime 全 reset → 不要 rebuild 多発、開発体験悪化。
**B4c 不採用根拠**: 全 build で SHA256 全計算は ~100 ms 増、開発 iterate cost 大。
**B4d 不採用根拠**: ccache の 3 OS 揃え + build script 統合複雑、Codegen 単独機構には過剰。

### §11.3 cache 構造

```json
{
  "version": 1,
  "files": {
    "app_settings/shaders/class3/deferred/materialF.glsl": {
      "mtime": 1717372800,
      "sha256": "abc123...",
      "codegen_output_files": ["ubo_layout_program_materialbasic.inl", "..."]
    }
  },
  "global_output_hash": "def456..."
}
```

= cache miss 時のみ Codegen tool 実行、cache hit 時は touch のみで CMake DEPENDS 解消。

### §11.4 cache invalidation

- Codegen script 自体の version change (= `scripts/codegen/codegen_ubo.py` の sha256 が `version` field に反映) → cache 全 invalidate
- chapter 04 §5.1 ファイル分割規則 変更時は手動 `rm -rf build/codegen/` 推奨 (= release note に明記)

= **(B4) → B4a hash + mtime default 確定、§17 で AYA 判断仰ぎ候補に登録**。

### §11.5 増分 build cache 詳細化 (= Phase 2d-β-revise Deliverable B-3)

B4a hash + mtime 併用の **cache key 構成 + invalidation trigger 完全 enumerate + 3 OS path 正規化** 確定形。Phase 1.A 実装 source of truth。

#### §11.5.1 cache key 構成 (= 何を hash 化するか)

`codegen_state.json` の **完全 schema** (= §11.3 を拡張):

```json
{
  "version": 1,
  "codegen_tool_version": {
    "script_sha256": "abc123...",
    "script_path": "scripts/codegen/codegen_ubo.py",
    "modules_sha256": {
      "std140.py": "...",
      "glsl_parser.py": "...",
      "perfect_hash.py": "...",
      "spirv_reflect.py": "..."
    }
  },
  "environment": {
    "python_version": "3.12.3",
    "glslang_version": "15.1.0",
    "spirv_cross_version": "1.3.239.0",
    "host_platform": "linux"
  },
  "input_files": {
    "app_settings/shaders/class3/deferred/materialF.glsl": {
      "mtime": 1717372800,
      "sha256_normalized": "def456...",
      "file_size": 4096
    }
  },
  "output_files": {
    "ubo_layout_program_materialbasic.inl": {
      "sha256": "789xyz...",
      "size_bytes": 2048
    },
    "ubo_perfect_hash.inl": { "sha256": "...", "size_bytes": 70000 },
    "ubo_metadata.inl":     { "sha256": "...", "size_bytes": 12000 },
    "ubo_dummy_init.inl":   { "sha256": "...", "size_bytes": 30000 },
    "ubo_host_loader.inl":  { "sha256": "...", "size_bytes": 8000 }
  },
  "build_metadata": {
    "last_build_timestamp_utc": "2026-06-03T14:30:00Z",
    "total_ubos": 88,
    "total_uniforms": 880,
    "build_duration_ms": 850
  }
}
```

#### §11.5.2 cache hit / miss 判定 algorithm (= 段階判定)

```python
# scripts/codegen/cache.py
import hashlib
import json
import os
from pathlib import Path

def check_cache(state_file: Path, input_files: list[Path], script_paths: list[Path]) -> bool:
    """
    returns True if cache hit (= Codegen skip 可)
    """
    if not state_file.exists():
        return False  # 初回 build
    
    state = json.loads(state_file.read_text(encoding='utf-8'))
    
    # Step 1: schema version check
    if state.get('version') != 1:
        return False
    
    # Step 2: tool version check (= script self-modify 検出)
    for script in script_paths:
        rel = script.name
        old_hash = state.get('codegen_tool_version', {}).get('modules_sha256', {}).get(rel)
        new_hash = sha256_file(script)
        if old_hash != new_hash:
            return False
    
    # Step 3: environment check (= glslang / spirv-cross / Python version drift 検出)
    env = state.get('environment', {})
    if env.get('glslang_version')     != get_glslang_version()     \
    or env.get('spirv_cross_version') != get_spirv_cross_version() \
    or env.get('python_version')      != get_python_version():
        return False
    
    # Step 4: input file 走査 = mtime 粗判定 → 不一致なら sha256 精判定
    old_inputs = state.get('input_files', {})
    
    # 4-1: 入力 file 集合一致 (= 新規 GLSL 追加 / 既存 GLSL 削除 検出)
    new_paths = {normalize_path(f, state_file.parent) for f in input_files}
    old_paths = set(old_inputs.keys())
    if new_paths != old_paths:
        return False
    
    # 4-2: 各 file の mtime + content hash
    for path in input_files:
        norm = normalize_path(path, state_file.parent)
        record = old_inputs[norm]
        try:
            cur_mtime = os.path.getmtime(path)
        except FileNotFoundError:
            return False  # 削除済
        
        if abs(cur_mtime - record['mtime']) < 1.0:
            continue  # mtime 一致 = 高確率 unchanged、skip content hash
        
        # mtime drift 検出 → content hash で再確認
        cur_hash = sha256_file_normalized(path)
        if cur_hash != record['sha256_normalized']:
            return False  # 真の change
        # mtime 不一致 + content 一致 = git checkout 等の偽 drift、cache hit 維持
    
    # Step 5: output file 存在 + sha256 check (= 生成物 tamper 検出)
    output_dir = state_file.parent / 'ubo'
    for out_name, rec in state.get('output_files', {}).items():
        out_path = output_dir / out_name
        if not out_path.exists():
            return False
        if sha256_file(out_path) != rec['sha256']:
            return False  # 手動編集 / 破損 検出
    
    return True  # 全 check pass = cache hit


def sha256_file_normalized(path: Path) -> str:
    """3 OS で binary identical な hash を返す (= §13 と接合):
       - line ending CRLF → LF normalize
       - trailing whitespace は維持 (= GLSL semantic 保持)
    """
    h = hashlib.sha256()
    with path.open('rb') as f:
        content = f.read()
    content = content.replace(b'\r\n', b'\n')   # CRLF → LF
    h.update(content)
    return h.hexdigest()


def normalize_path(p: Path, base: Path) -> str:
    """3 OS 共通 cache key 用 path normalize"""
    try:
        rel = p.relative_to(base.parent.parent)  # = project root からの relative
    except ValueError:
        rel = p
    # POSIX separator 強制 (= Win backslash → forward slash)
    return rel.as_posix()
```

#### §11.5.3 cache invalidation trigger 完全 enumerate

| trigger 種別 | 検出 | 対応 |
|---|---|---|
| GLSL ファイル content 変更 | Step 4-2 sha256 不一致 | partial 再 build (= 該当 GLSL に紐づく UBO 単位、ただし perfect hash は全 entry 再構築必須なので **実質全 invalidate**) |
| GLSL ファイル追加 | Step 4-1 新規 path 検出 | 全 invalidate (= perfect hash table 再構築必須) |
| GLSL ファイル削除 | Step 4-1 古 path 検出 | 全 invalidate |
| GLSL ファイル mtime のみ変動 (= git checkout) | Step 4-2 mtime 不一致 + sha256 一致 | cache hit 維持 (= 偽 drift 吸収) |
| Codegen script 自体変更 | Step 2 modules_sha256 不一致 | 全 invalidate |
| glslang version upgrade | Step 3 environment 不一致 | 全 invalidate (= reflection format drift risk) |
| spirv-cross version upgrade | 同上 | 全 invalidate |
| Python version upgrade | 同上 | 全 invalidate |
| 出力 file 手動編集 / 削除 | Step 5 sha256 不一致 or missing | 該当 file 再生成 (= ただし perfect hash table 整合性のため事実上全 invalidate) |
| `codegen_state.json` 自体破損 | json.loads 失敗 | 全 invalidate (= 安全側 fail-soft) |
| user の手動 `rm -rf build/codegen/` | state_file 不在 | 全 invalidate (= §11.4 既述、release note 明記) |
| chapter 02 §2.4 ファイル分割規則変更 | (= tool 内 hardcoded、検出不能) | 手動 invalidate 必須 (= §11.4 既述) |

#### §11.5.4 build 開始時 cache 適用 flow

```python
# scripts/codegen/codegen_ubo.py main entry
def main(args):
    state_file = Path(args.cache_file)
    output_dir = Path(args.output_dir)
    input_files = list(Path(args.input_glsl_dir).rglob('*.glsl'))
    script_paths = list(Path(__file__).parent.glob('*.py'))
    
    if args.force or not check_cache(state_file, input_files, script_paths):
        # cache miss = Codegen 走行
        layouts, perfect_hash, metadata, dummy_masks, host_loader = run_codegen(input_files)
        write_outputs(output_dir, layouts, perfect_hash, metadata, dummy_masks, host_loader)
        write_cache(state_file, input_files, script_paths, output_dir)
        print(f"[codegen_ubo] cache miss, regenerated ({len(input_files)} GLSL, {len(layouts)} UBO)")
    else:
        # cache hit = touch のみ (= CMake DEPENDS の OUTPUT を「更新済」と認識させる)
        now = time.time()
        for out_name in OUTPUT_FILE_LIST:
            os.utime(output_dir / out_name, (now, now))
        print(f"[codegen_ubo] cache hit, skipped")
```

#### §11.5.5 cache disk footprint + 上限

| 項目 | 規模 |
|---|---|
| `codegen_state.json` 単独 | ~30-100 KB (= 200 GLSL × ~200 byte / entry + metadata) |
| 出力 `.inl` 群 (= `build/codegen/ubo/`) | ~150 KB 合計 (= §5.6.6 + §6.1 + §8.2 + §10.3 合計) |
| `build/codegen/spirv_reflect/` (= reflection JSON temp) | ~5 MB ピーク (= build 中のみ、§5.4.1.3 spv_tmp 都度削除) |
| **build/codegen/ 全体最大** | **~6 MB** (= 上限 8 MB を release note `feedback_release_notes_link_only` で明記) |

#### §11.5.6 cache GC (= (cache-grow) §17 持越接合)

長期 build cycle で `build/codegen/cache/` が肥大化した場合の GC policy:

```python
# scripts/codegen/cache_gc.py (Phase 2..K 期間の sub-task として発動、§17 (cache-grow))
def gc_old_outputs(cache_dir: Path, max_age_days: int = 30):
    """state_file から外れた出力 file を削除 (= 古 UBO 名残り)"""
    state = json.loads((cache_dir / 'codegen_state.json').read_text())
    known_outputs = set(state['output_files'].keys())
    
    output_dir = cache_dir / 'ubo'
    now = time.time()
    for f in output_dir.iterdir():
        if f.name in known_outputs:
            continue
        if now - f.stat().st_mtime > max_age_days * 86400:
            f.unlink()
            print(f"[codegen_ubo] GC: removed stale output {f.name}")
```

= chapter 09 §10.2 / §17 (cache-grow) Phase 紐付け = Phase 2..K 期間に発動可能 sub-task、本 chapter §11.5 で algorithm 確定。

#### §11.5.7 race condition + 並列 build 耐性

並列 build (= make -j4) で同 cache file を複数 process が同時 update する race を回避:

```python
# atomic write pattern = temp file → rename
def write_cache(state_file: Path, ...):
    tmp = state_file.with_suffix('.json.tmp')
    tmp.write_text(json.dumps(state, indent=2, sort_keys=True), encoding='utf-8')
    tmp.replace(state_file)  # POSIX rename = atomic
```

**Codegen tool 自身は single thread / single invocation** (= CMake add_custom_command が 1 回呼ぶ、§12 で並列性は CMake 側で sequence 保証) → cache file 競合は **常識的に発生しない**、ただし安全側で atomic write を採用。

#### §11.5.8 実装規模見積

| 項目 | LoC |
|---|---|
| check_cache + sha256 normalize (§11.5.2) | ~80 |
| write_cache + atomic write (§11.5.7) | ~25 |
| invalidation trigger 検知 (§11.5.3) | ~30 |
| environment check 関数群 (= get_glslang_version 等) | ~25 |
| cache GC (§11.5.6) | ~20 |
| unit test (= cache hit/miss 10 種シナリオ) | ~100 |
| **合計** | **~280** |

---

## §12 Codegen 実行 trigger (= (B5) 解消)

### §12.1 候補と評価

| # | 候補 | 利点 | 欠点 |
|---|---|---|---|
| B5a | **CMake `add_custom_command(DEPENDS=GLSL files)` 自動** | GLSL 変更検出で自動再生成、CMake 標準 mechanism | 全 GLSL を DEPENDS に列挙、CMakeLists.txt 肥大 |
| B5b | 全 build 時に毎回実行 | 確実、miss なし | cache 機構あっても hash 計算オーバーヘッド毎 build 発生 |
| B5c | 手動 cmake target (= `make codegen_ubo`) | コスト最小、明示制御 | 開発者が忘れる → silent old layout 使用、致命的 |

### §12.2 確定案 = **B5a (CMake DEPENDS 自動 + 手動 target 併設)** default

**B5a 採用根拠**:
1. **自動再生成**: GLSL 変更で CMake が DEPENDS 解析 → Codegen tool 自動実行、開発者が忘れる risk ゼロ
2. **CMake 標準 mechanism**: `file(GLOB_RECURSE GLSL_FILES "app_settings/shaders/*.glsl")` + `add_custom_command(OUTPUT ${CODEGEN_OUTPUTS} DEPENDS ${GLSL_FILES} COMMAND ${Python3_EXECUTABLE} ...)` の標準形
3. **手動 target 併設**: `add_custom_target(codegen_ubo_force COMMAND ${Python3_EXECUTABLE} ... --force)` で cache 無効強制再生成 = debug / cache 不整合時の救済路

**B5b 不採用根拠**: 毎 build で Codegen tool 起動 (Python interpreter 起動 + script parse) = ~500 ms 増、cache 機構の効果半減。
**B5c 不採用根拠**: 開発者の手動 trigger 忘れで old layout の silent 使用、CI 環境でも常に手動指示必要、運用 risk 高。

### §12.3 CMake snippet 例

```cmake
find_package(Python3 3.8 REQUIRED)

file(GLOB_RECURSE AYA_GLSL_FILES
    "${CMAKE_SOURCE_DIR}/indra/newview/app_settings/shaders/*.glsl"
)

set(AYA_CODEGEN_OUTPUTS
    "${CMAKE_BINARY_DIR}/codegen/ubo/ubo_index.inl"
    "${CMAKE_BINARY_DIR}/codegen/ubo/ubo_perfect_hash.inl"
    "${CMAKE_BINARY_DIR}/codegen/ubo/ubo_metadata.inl"
    "${CMAKE_BINARY_DIR}/codegen/ubo/ubo_dummy_init.inl"
    "${CMAKE_BINARY_DIR}/codegen/ubo/ubo_host_loader.inl"
)

add_custom_command(
    OUTPUT ${AYA_CODEGEN_OUTPUTS}
    DEPENDS
        ${AYA_GLSL_FILES}
        "${CMAKE_SOURCE_DIR}/scripts/codegen/codegen_ubo.py"
    COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_SOURCE_DIR}/scripts/codegen/codegen_ubo.py"
        --input-glsl-dir "${CMAKE_SOURCE_DIR}/indra/newview/app_settings/shaders"
        --output-dir "${CMAKE_BINARY_DIR}/codegen/ubo"
        --spirv-reflection-dir "${CMAKE_BINARY_DIR}/codegen/spirv_reflect"
        --glslang "${GLSLANG_VALIDATOR_PATH}"
        --cache-file "${CMAKE_BINARY_DIR}/codegen/cache/codegen_state.json"
    COMMENT "Generating UBO codegen artifacts"
)

add_custom_target(codegen_ubo DEPENDS ${AYA_CODEGEN_OUTPUTS})
add_custom_target(codegen_ubo_force
    COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_SOURCE_DIR}/scripts/codegen/codegen_ubo.py"
        --force
        # ...
    COMMENT "Force-regenerating UBO codegen artifacts"
)

add_dependencies(llrender codegen_ubo)
add_dependencies(llvkloader codegen_ubo)
target_include_directories(llrender PUBLIC "${CMAKE_BINARY_DIR}/codegen")
target_include_directories(llvkloader PUBLIC "${CMAKE_BINARY_DIR}/codegen")
```

= **(B5) → B5a 自動 + 手動 target 併設 default 確定、§17 で AYA 判断仰ぎ候補に登録**。

### §12.4 generator timing 全体接続 (= 設計 review 2026-06-03 §3.4 §5/§8/§12 timing 明示)

3 generator (= §5 perfect hash / §8 dummy buffer init / §12 全 Codegen trigger) の build cycle 内 timing 関係:

| timing 段階 | 走る generator | 入力 | 出力 |
|---|---|---|---|
| build 開始 | (B5) `add_custom_command` 起動判定 | GLSL files + Python script の mtime/hash (= §11) | cache hit なら skip / miss なら §5/§8 走る |
| Codegen tool 起動 (1) | Codegen Python tool 起動 + GLSL parse (= §5.1 P3) | GLSL files + glslang -E (= §5.1) | UBO block 構造 (in-memory) |
| Codegen tool 起動 (2) | §5.3 std140 offset calculator + §5.4 SPIR-V reflection 二重保証 | UBO block + glslang SPIR-V reflection | offset 確定済 UBO + binding 番号 |
| Codegen tool 起動 (3) | §7 set=1 split (sort → 40/40 振分) | per-program UBO 80 個 | subset 振分済 metadata |
| Codegen tool 起動 (4) | §5.5 perfect hash generator (G2/B3b) | name 列 (= UBO block name + member name) | `ubo_perfect_hash.inl` (= §5.7) |
| Codegen tool 起動 (5) | §6 `ubo_metadata.inl` 出力 (= schema 序列化) | (2)+(3) 結果 | `ubo_metadata.inl` |
| Codegen tool 起動 (6) | §8 dummy buffer init 出力 | unused UBO 判定 (= §8.4) | `ubo_dummy_init.inl` |
| Codegen tool 起動 (7) | §10 host loader header 出力 | (5)+(6) ↔ host C++ I/F | `ubo_host_loader.inl` |
| Codegen tool 終了 | (B5) cache 更新 (= §11.3 sha256 + mtime 記録) | 全出力 .inl files | `codegen_state.json` |
| 後続 build | host C++ compile | (5)+(6)+(7) include | `llrender` / `llvkloader` の object files |

**timing 制約**:
- (1) ↔ (2) sequential (= parse 完了後に std140 計算)
- (3) は (2) 完了後 (= subset 振分は offset 確定後の単純振分)
- (4)/(5)/(6)/(7) は (3) 完了後に **並列可能** (= 出力 .inl files が独立)、現 phase は Python single thread で sequential、将来 phase で `multiprocessing` 並列化検討余地
- (B5) cache 機構は **全出力 .inl files の sha256 を一括 record**、cache hit 時は (1)-(7) を skip して cmake DEPENDS 解消のための touch のみ実施 (= §11.3)

**異常系**: いずれかの段階で error (= §9 build error 7 種) → Codegen tool が non-zero exit → CMake が後続 host compile を中止 → AYA に build error 表示 (= §9.4 format)

### §12.5 CMake DEPENDS + 手動 target 詳細化 (= Phase 2d-β-revise Deliverable B-4)

B5a (CMake DEPENDS 自動 + 手動 target 併設) の **再生成 timing 完全 enumerate + 新規 GLSL 検出 + add_dependencies 連鎖 + build order 保証** 確定形。Phase 1.A 実装 source of truth。

#### §12.5.1 再生成 timing 完全 enumerate (= どの編集で Codegen が走るか)

| 編集種別 | DEPENDS 検出 | Codegen 走行 | cache 判定 |
|---|---|---|---|
| 既存 GLSL ファイルの content 変更 | ✅ mtime 変動 → CMake が DEPENDS 不整合判定 | ✅ 走行 | §11.5.3 sha256 不一致 → 全 invalidate |
| 既存 GLSL ファイルの mtime のみ変動 (= git checkout) | ✅ mtime 変動 → CMake が走行命令 | ✅ Codegen 起動 | §11.5.3 sha256 一致 → cache hit、touch のみ |
| 新規 GLSL ファイル追加 (= shader 増設) | ❌ **`file(GLOB_RECURSE)` の再評価が必要** (§12.5.2) | (CMake 再 configure 後に) ✅ | sha256 全 invalidate |
| 既存 GLSL ファイル削除 | 同上 | 同上 | 同上 |
| Codegen script (= `scripts/codegen/*.py`) 変更 | ✅ DEPENDS に列挙 | ✅ 走行 | §11.5.3 script_sha256 不一致 → 全 invalidate |
| 出力 `.inl` の手動編集 | (DEPENDS 上は detect されない) | 次回 build 時に §11.5.2 Step 5 で sha256 不一致 → 走行 | 全 invalidate |
| `codegen_state.json` 削除 | (DEPENDS 上は detect されない) | 次回 build 時に §11.5.2 state_file 不在 → 走行 | 初回 build 同型 |
| CMakeLists.txt の DEPENDS 引数変更 | ✅ CMake 再 configure 必須 | (configure 後に) ✅ | configure 直後は全 invalidate |
| glslang / spirv-cross / Python upgrade | ❌ DEPENDS 上は detect されない | (CMake configure を手動再実行で検出) | §11.5.3 environment 不一致 → 全 invalidate |

#### §12.5.2 新規 GLSL ファイル追加検出 (= GLOB_RECURSE の取扱)

CMake `file(GLOB_RECURSE)` は **configure 時点で展開**、build 時点では再評価されない → 新規 GLSL ファイル追加時 CMake configure 再実行が必要:

**対応 1: 開発者運用ルール明記** (= AYAstorm 既存運用と整合)
- GLSL 新規追加時は **必ず `cmake --build . --target reconfigure`** を release note + dev doc に明記
- AYAstorm 既存 build flow (memory `project_build_procedure`) の configure step で自動取り込み

**対応 2: CONFIGURE_DEPENDS option** (= CMake 3.12+)
```cmake
file(GLOB_RECURSE AYA_GLSL_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/indra/newview/app_settings/shaders/*.glsl"
)
```
- `CONFIGURE_DEPENDS` 指定で **build 時に GLSL dir mtime check** → 変動検出時 自動 reconfigure
- AYAstorm 既存 CMake version: 3.16+ (= chapter 07 §2 build environment) → **使用可**

**default 採用 = 対応 2 (CONFIGURE_DEPENDS)**:
- 開発者運用ルールに頼らない安全側 (= memory `feedback_self_bug_no_defer_option` 準拠 = 自分の機構で対応)
- CMake 公式機能、3 OS 共通動作 (= cmake 3.12+ doc 確認)
- 副作用: build 開始時に GLSL dir スキャン cost 微増 (= ~ms order、許容)

#### §12.5.3 add_dependencies 連鎖 + build order 保証

```cmake
# 上位 target の依存配線 (= 順序保証):
add_dependencies(llrender codegen_ubo)
add_dependencies(llvkloader codegen_ubo)

# include path 露出 (= 生成 header の取込):
target_include_directories(llrender   PUBLIC "${CMAKE_BINARY_DIR}/codegen")
target_include_directories(llvkloader PUBLIC "${CMAKE_BINARY_DIR}/codegen")

# 依存連鎖図 (= cmake target graph):
codegen_ubo (= ${AYA_CODEGEN_OUTPUTS})
    DEPENDS = ${AYA_GLSL_FILES} + scripts/codegen/codegen_ubo.py + 他 *.py
    ↓
llrender / llvkloader (= OBJECT lib)
    ↓
ayastorm-binary (= 最終 link)
```

**build order 保証 mechanism**:
1. CMake が `codegen_ubo` を **leaf level** (= depend されるだけで何にも depend しない) と認識
2. `llrender` / `llvkloader` の compile 開始前に **必ず `codegen_ubo` を完走** (= add_dependencies 効果)
3. 並列 build (= make -j4) でも `codegen_ubo` だけは sequence 先頭で 1 回走行 (= 並列 compile による race 排除)
4. `codegen_ubo` 内部は Python tool 単 process / single thread (= §11.5.7 race 自体発生せず)

#### §12.5.4 手動 codegen_ubo_force target の cache 無効化動作

```cmake
add_custom_target(codegen_ubo_force
    COMMAND ${Python3_EXECUTABLE}
        "${CMAKE_SOURCE_DIR}/scripts/codegen/codegen_ubo.py"
        --force
        --input-glsl-dir "${CMAKE_SOURCE_DIR}/indra/newview/app_settings/shaders"
        --output-dir "${CMAKE_BINARY_DIR}/codegen/ubo"
        --cache-file "${CMAKE_BINARY_DIR}/codegen/cache/codegen_state.json"
    COMMENT "Force-regenerating UBO codegen artifacts (= cache 完全 invalidate)"
)
```

```python
# scripts/codegen/codegen_ubo.py
if args.force:
    # cache を読まず全 invalidate、出力 .inl + state_file 全再生成
    state_file.unlink(missing_ok=True)
    for f in (output_dir / 'ubo').glob('*.inl'):
        f.unlink()
    print("[codegen_ubo] --force: cache invalidated, full regeneration")
    # 通常 flow へ続行
```

**手動 target 使用 case**:
- cache 破損疑い時の救済 (= §11.5.3 余 trigger 検出後の再 build)
- Codegen tool の debug 中 (= cache hit 誤判定の切り分け)
- chapter 02 §2.4 ファイル分割規則変更時 (= §11.4 既述、自動 invalidate 不可)

呼出: `cmake --build build/ --target codegen_ubo_force`

#### §12.5.5 build 失敗時のクリーンアップ + retry 規律

Codegen tool が non-zero exit した時の build 状態:

| 失敗段階 | 出力 file 状態 | state_file 状態 | retry 時挙動 |
|---|---|---|---|
| Stage 1 (parse fail) | 未生成 (= 部分書込なし) | 未更新 | 既存 cache 維持 (= 前回成功状態) |
| Stage 2 (offset calc fail) | 未生成 | 未更新 | 同上 |
| Stage 3 (SPIR-V reflection mismatch) | 未生成 | 未更新 | 同上 |
| Stage 4-7 中の partial write | **部分書込済 .inl が残る** | 未更新 | 次回 §11.5.2 Step 5 で sha256 不一致 → 走行 |

**実装契約** (= atomic write 拡張):

```python
# scripts/codegen/codegen_ubo.py write_outputs() 内
def write_outputs_atomic(output_dir, ...):
    """全 .inl を temp dir に書出 → 全成功時に一括 rename (= partial write 防止)"""
    tmp_dir = output_dir / f'.tmp_{os.getpid()}'
    tmp_dir.mkdir(exist_ok=True)
    try:
        write_layout_inl(tmp_dir, ...)
        write_perfect_hash_inl(tmp_dir, ...)
        write_metadata_inl(tmp_dir, ...)
        write_dummy_init_inl(tmp_dir, ...)
        write_host_loader_inl(tmp_dir, ...)
        write_index_inl(tmp_dir, ...)
        # 全成功 → 一括 move
        for f in tmp_dir.iterdir():
            (output_dir / f.name).unlink(missing_ok=True)
            f.rename(output_dir / f.name)
    finally:
        if tmp_dir.exists():
            shutil.rmtree(tmp_dir)
```

= silent partial write による次回 build 誤判定排除。

#### §12.5.6 verbose log + build log integration

Codegen tool 出力は **CMake build log に統合**:

```python
# scripts/codegen/codegen_ubo.py main()
print(f"[codegen_ubo] start: {len(input_files)} GLSL files")
print(f"[codegen_ubo] glslang: {get_glslang_version()}")
print(f"[codegen_ubo] spirv-cross: {get_spirv_cross_version()}")
print(f"[codegen_ubo] cache: {'hit' if cached else 'miss'}")
if not cached:
    print(f"[codegen_ubo] parsed: {n_ubos} UBO blocks, {n_members} members")
    print(f"[codegen_ubo] perfect hash: {n_entries} entries, table_size={table_size}")
    print(f"[codegen_ubo] outputs: {len(outputs)} files, total {total_bytes} bytes")
print(f"[codegen_ubo] done: {elapsed_ms} ms")
```

`make` 経由 build の場合 stdout は build log に通常出力、`ninja` 経由でも同様。**CMake `COMMENT`** で build console に短文表示 (= "Generating UBO codegen artifacts")。

#### §12.5.7 incremental build = "no change" path の verify

cache hit (= GLSL 無変更 / Codegen skip) を CMake が認識する仕組み:

1. `add_custom_command` の OUTPUT が **既存 file** + mtime が **DEPENDS より新しい** なら CMake は走行不要と判定
2. Codegen tool が cache hit 時に **`touch` 相当の `os.utime(output_file, (now, now))`** で OUTPUT mtime を最新化
3. 次回 build では OUTPUT mtime > DEPENDS mtime → CMake 走行 skip

```python
# scripts/codegen/codegen_ubo.py main() cache hit path
if cached:
    now = time.time()
    for out_name in EXPECTED_OUTPUT_LIST:
        out_path = output_dir / out_name
        os.utime(out_path, (now, now))
    print("[codegen_ubo] cache hit, touched outputs")
    return 0
```

#### §12.5.8 build script 実装規模見積

| 項目 | LoC |
|---|---|
| CMakeLists.txt 追加分 (= §12.3 snippet + CONFIGURE_DEPENDS + add_dependencies) | ~40 |
| codegen_ubo.py main argparse + dispatch | ~50 |
| write_outputs_atomic (§12.5.5) | ~30 |
| --force handling (§12.5.4) | ~15 |
| verbose log + version 取得 (§12.5.6) | ~20 |
| touch on cache hit (§12.5.7) | ~10 |
| **合計** | **~165** |

= chapter 08 §3.2 Python tool 「~500-1000 行」見積に整合、tool main entry + CMake 追加分は ~165 行。

---

## §13 3 OS 互換性保証

### §13.1 3 OS 動作確証項目

AYAstorm は Linux/Win/Mac 3 OS 対応 (= memory `project_ayastorm_three_platforms`) のため、Codegen pipeline は全 OS で動作必須:

| 項目 | Linux | Win | Mac |
|---|---|---|---|
| Python 3.8+ availability | ✅ apt/dnf 標準 (3.10+) | ✅ autobuild bundle / 公式 installer | ✅ system Python 3.9+ / brew |
| glslang vendoring (autobuild) | ✅ 既存配信 | ✅ 既存配信 | ✅ 既存配信 |
| `add_custom_command` 動作 | ✅ make / ninja | ✅ MSBuild / ninja | ✅ Xcode / ninja |
| path 区切り `/` vs `\` | ✅ POSIX | ⚠️ CMake が `/` を自動変換 | ✅ POSIX |
| line ending (CRLF vs LF) | ✅ LF | ⚠️ Python `open(mode='r')` で auto-translate、`mode='rb'` で binary diff 取得 | ✅ LF |
| 生成 dir permission | ✅ | ✅ | ✅ |

### §13.2 Win 固有注意点

- path separator: CMake は `/` を normalize、Python script 内も `pathlib.Path` 使用で抽象化
- line ending: GLSL 入力ファイルが CRLF 含む場合、std140 offset 計算には影響しない (= byte level) が、hash 計算 (= §11) では mtime + hash 共に CRLF/LF 差で異なる → **Python 側で content normalize 後に hash 取る**
- glslang binary 拡張子: `glslangValidator.exe` (Win) vs `glslangValidator` (Linux/Mac) を CMake `find_program` で吸収

### §13.3 Mac 固有注意点

- system Python 3.9+ で動作確認、autobuild 経由の Python (= AYAstorm 既存) も併用可
- glslang autobuild package は Mac でも同 version 配信 (= AYAstorm 既存運用、@t-noami さん検証信任、memory `feedback_mac_only_fixes_accept_as_is` 範囲内)
- Codegen tool は Mac 動作確証を **AYA 実機 / @t-noami さん検証** で後追い (= chapter 09 Phase 入口で実 build 確認)

### §13.4 3 OS 確証 Phase

chapter 09 Phase Roadmap で **Codegen pipeline 起動確認 phase** を per-OS で配置:
- Phase X-α: Linux native 起動 + Codegen tool 動作 + 生成物確認
- Phase X-β: Win autobuild 経由 build + 生成物確認
- Phase X-γ: Mac autobuild 経由 build + 生成物確認 (= @t-noami さん検証信任)

= 3 OS 一律 OK 確認後、本 chapter 確定 default を runtime 実利用 phase (= chapter 09 Phase 後続) で投入。

### §13.5 3 OS binary identical 保証 mechanism 詳細化 (= Phase 2d-β-revise Deliverable B-5)

`ubo_metadata.inl` / `ubo_perfect_hash.inl` 等 Codegen 出力が **3 OS で byte-for-byte 同一** を保証する mechanism 確定形。Phase 1.A 実装 source of truth。

#### §13.5.1 なぜ binary identical が必要か

- shader code 内 std140 offset は **GLSL spec 7.6.2.2 で OS 非依存** = 既に決定的
- ただし **Codegen 出力 C++ header の文字列表現** が 3 OS で差分発生すると:
  - host 側 compile 結果 (= `llrender.so` / `llrender.dll` / `llrender.dylib`) が OS 別に微妙に異なる
  - 3 OS 共通 build artifact (= `ubo_metadata.inl` 等を deploy 物に含める運用) で確証困難
  - chapter 09 Phase K+1/+2/+3 (= 3 OS 確証) で render parity 確認時、Codegen 出力差で false positive 検出
- → **生成物 byte-identical** が確証 phase の **必須前提**

#### §13.5.2 非決定要因 + 対策 完全 enumerate

| # | 非決定要因 | 発生箇所 | 対策 |
|---|---|---|---|
| ND1 | dict / set iteration 順序 (= Python 3.7+ insertion order だが、set は順不定) | perfect hash table の entry 列挙 / ubo metadata の entry 列挙 | **常に explicit sort** (= §13.5.3) |
| ND2 | GLSL ファイル列挙順 (= `glob.glob` / `Path.rglob` の OS 別差) | input GLSL file 順序 | **`sorted()` で path 文字列 lexicographic sort** |
| ND3 | line ending (= CRLF vs LF) | Win 側で GLSL に CRLF 含む可能性 | content normalize (= §11.5.2 sha256_file_normalized) + Codegen 内も LF 統一出力 |
| ND4 | path separator (`/` vs `\`) | Win 側で `Path.as_posix()` 必要 | normalize_path (= §11.5.2) |
| ND5 | float の str() 表現 (= Python の浮動小数表現は IEEE 754 で deterministic だが、出力 format は実装依存) | std140 offset 表は全部 int のため発生せず、ただし将来 padding 計算で float 経由する可能性 | Python では `repr(float)` 使用、必要時 `f"{val:.17g}"` で IEEE 754 round-trip 保証 |
| ND6 | hash 関数の seed (= FNV-1a の seed は固定 `0x811c9dc5`、対応済) | perfect hash 構築 | seed を hardcode (= §5.6.6 既述) |
| ND7 | timestamp 埋め込み (= 自動 generated comment の build_date 等) | header 先頭の `// auto-generated YYYY-MM-DD HH:MM:SS` | **timestamp 埋め込み禁止** (= §13.5.4) |
| ND8 | absolute path 埋め込み (= 開発者の workspace path leak) | header コメント内の source file path | **`Path.relative_to(project_root)` で relative path のみ出力** |
| ND9 | random seed (= 衝突回避 reroll で乱数使用すると非決定) | perfect hash 構築 | CHD seed search は 0 から increment で deterministic (= §5.6.3 既述) |
| ND10 | Python 内部 hash randomization (= PYTHONHASHSEED) | dict key 順序が起動毎に変動 | **Codegen 起動時に `PYTHONHASHSEED=0` 強制 or 全 dict/set を sorted iterate** |
| ND11 | locale 依存 string sort (= Turkish I 等) | sort key 比較 | Python `sorted()` は default で codepoint 順 = locale 非依存、ただし `locale.strcoll()` 不使用を明示 |
| ND12 | OS 別 newline output (= `print()` の line ending) | header 出力 | **常に `'\n'` literal で改行、`open(mode='w', newline='')` で auto-translate 抑制** |

#### §13.5.3 explicit sort 規律 (= ND1 / ND2 対応)

```python
# perfect hash 構築前の key sort:
all_keys = []
for ubo in sorted(parsed_ubos, key=lambda u: u.block_name):    # ND1
    for member in ubo.members:                                  # = parser が宣言順を保持済
        all_keys.append(f"{ubo.block_name}::{member.name}")

# input GLSL ファイル列挙:
input_files = sorted(                                            # ND2
    Path(args.input_glsl_dir).rglob('*.glsl'),
    key=lambda p: p.as_posix()                                   # ND4 path separator 統一
)

# ubo_metadata.inl 出力時の entry 列挙:
for ubo in sorted(all_ubos, key=lambda u: (u.descriptor_set, u.subset, u.binding)):  # ND1
    emit_ubo_metadata_entry(ubo)

# CHD displacement / value table 出力:
for i in range(table_size):                                       # = index 順、deterministic
    emit_chd_value(i, value_table[i])
```

#### §13.5.4 timestamp / absolute path 禁止 (= ND7 / ND8 対応)

```python
# ❌ 禁止 (= non-deterministic):
header.append(f"// Generated on {datetime.now()}")
header.append(f"// Source: {abs_glsl_path}")

# ✅ 採用 (= deterministic):
header.append("// auto-generated by codegen_ubo.py, do not edit")
header.append(f"// Source: {rel_glsl_path.as_posix()}")  # = relative path + POSIX 統一
```

build_date / version 等の動的情報が必要な場合は **`codegen_state.json` 側に保存**、生成 `.inl` には埋め込まない (= §11.5.1 既述)。

#### §13.5.5 出力 file 書込 規律 (= ND12 対応)

```python
def write_inl(path: Path, content: str):
    """3 OS 共通 byte-identical 出力"""
    # newline='' で OS 別 line ending auto-translate 抑制 (= LF 統一)
    with path.open('w', encoding='utf-8', newline='') as f:
        f.write(content)
    # = 結果: Linux/Mac/Win 全 OS で同一 byte sequence
```

content 内の改行は全て **`'\n'` literal**、Python の `print()` (= OS 別改行) は使わない。

#### §13.5.6 PYTHONHASHSEED 強制 (= ND10 対応)

```cmake
# CMake 側で環境変数を Codegen invocation に注入:
add_custom_command(
    OUTPUT ${AYA_CODEGEN_OUTPUTS}
    DEPENDS ${AYA_GLSL_FILES} ...
    COMMAND ${CMAKE_COMMAND} -E env PYTHONHASHSEED=0
        ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/codegen/codegen_ubo.py" ...
    COMMENT "Generating UBO codegen artifacts"
)
```

加えて Codegen tool 内部で **全 dict / set を必ず sorted iterate** (= §13.5.3) として **二重保証**:

```python
# Python 起動時 self-check:
import os, sys
if os.environ.get('PYTHONHASHSEED') != '0':
    print("[codegen_ubo] WARN: PYTHONHASHSEED not '0', output may be non-deterministic", file=sys.stderr)
    # warn のみで継続 (= sorted iterate で実質保証されているため)
```

#### §13.5.7 binary identical 検証 mechanism (= chapter 09 Phase K+1/+2/+3 接合)

3 OS 確証 phase で binary identical を検証する手順:

```bash
# Phase X-α (Linux 起動): 出力を保存
cd build/codegen/ubo
sha256sum *.inl > /tmp/codegen_linux.sha256

# Phase X-β (Win): 出力 hash 比較
sha256sum *.inl > /tmp/codegen_win.sha256
diff /tmp/codegen_linux.sha256 /tmp/codegen_win.sha256
# = empty diff なら byte identical 確証

# Phase X-γ (Mac): 同上
sha256sum *.inl > /tmp/codegen_mac.sha256
diff /tmp/codegen_linux.sha256 /tmp/codegen_mac.sha256
```

**chapter 09 §6 Phase K+1/+2/+3 Exit Criteria 追加項目** (= 本 §13.5.7 由来):
- "Codegen 出力 .inl 群が **3 OS で sha256 一致**" を Phase K+2/K+3 Exit Criteria に追加 (= 本 chapter §13.4 X-β/γ verify task)

#### §13.5.8 unit test by ci 風 (= ローカル検証 task)

CI 不在の AYAstorm でも、開発者が手動で 3 OS binary identical を検証可能な test runner:

```bash
# scripts/codegen/test_deterministic.sh (Phase 1.A で追加予定)
set -e
echo "[test] determinism check: run codegen 2 回連続で出力 sha256 一致"
python3 scripts/codegen/codegen_ubo.py \
    --input-glsl-dir indra/newview/app_settings/shaders \
    --output-dir /tmp/codegen_run1 \
    --force
python3 scripts/codegen/codegen_ubo.py \
    --input-glsl-dir indra/newview/app_settings/shaders \
    --output-dir /tmp/codegen_run2 \
    --force
diff -r /tmp/codegen_run1 /tmp/codegen_run2 && echo "[test] PASS" || echo "[test] FAIL"
```

= 同 OS で 2 回実行して binary identical を確認 (= ND10 等の起動毎変動を検出)。3 OS 比較は実機 build 経由必須 (= chapter 09 Phase K+1/+2/+3)。

#### §13.5.9 binary identical 失敗時の trace + debug

失敗検出時の原因切り分け手順:

1. **diff -r で異なる file 特定** (= どの `.inl` が違うか)
2. **`diff` で line 単位 diff 表示** (= 順序差 / 内容差 / 改行差)
3. 順序差なら ND1 / ND2 起源 → 該当 emit ループに sort 漏れ追加
4. 内容差なら ND5 (float repr) / ND7 (timestamp leak) / ND8 (path leak) → 該当出力箇所修正
5. 改行差なら ND12 → write_inl の newline 引数確認

**diff trace example**:
```
$ diff /tmp/codegen_linux/ubo_perfect_hash.inl /tmp/codegen_win/ubo_perfect_hash.inl
42c42
< { 0xBLOCK_HASH_FrameViewProj, 64, 64, 0 },   // = "FrameViewProj::proj"
---
> { 0xBLOCK_HASH_FrameViewProj, 64, 64, 0 },   // = "FrameViewProj::proj"\r
```
= 末尾 CR 差 → ND12 違反、`open(newline='')` 適用漏れの specific 箇所特定。

#### §13.5.10 実装規模見積

| 項目 | LoC |
|---|---|
| sorted iterate 規律 (§13.5.3) | (本体 logic 内に内嵌、追加 LoC ~0) |
| write_inl + newline 抑制 (§13.5.5) | ~15 |
| PYTHONHASHSEED 検知 + warn (§13.5.6) | ~10 |
| absolute path → relative 変換 (§13.5.4) | ~10 |
| test_deterministic.sh (§13.5.8) | ~20 |
| **合計** | **~55** |

= 既存 Codegen tool に **追加 ~55 行** で 3 OS binary identical 保証完成。

---

## §14 chapter 04 / 06a / 07 / 09 との分担境界

| chapter | 本 chapter からの入力 | 本 chapter への出力 |
|---|---|---|
| **04** (codegen-ubo) | 3 stage pipeline / 生成物 schema / 判断 (A)(B)(C) | (A1)(P)(G) 確定、本 chapter §3/§4/§5 で default 採用案 + AYA 判断仰ぎ候補登録 |
| **06a** (cache-structure-and-setter-redirect) | `mUniformUBOLoc[index]` cache 構造 / 16 method setter 分岐 | `ubo_host_loader.inl` で cache 構造の C++ schema 提供 (= §10) |
| **07** (vulkan-api-state) | device limit / set 帯 5 化 / V1' split / S3' sampler / 256 B alignment / 共通 PSO layout | (本 chapter は受け側) — set 番号 / subset 出力 / sampler binding / padding / layout description を Codegen 出力に反映 (= §6 / §7 / §10) |
| **09** (phase-roadmap) | Codegen 起動 Phase / 3 OS 確証 Phase / cache 機構実装 Phase | (本 chapter は提供側) — §13.4 Phase 候補登録、chapter 09 で Phase 番号体系に組込 |

---

## §15 chapter 06a §5 sampler 分岐との接合

chapter 06a §5.6 で「sampler は OpenGL path 強制 + Vulkan path descriptor set 経由」と確定済。本 chapter §6 で sampler 49 個 を `g_sampler_metadata[]` として Codegen 出力 = 06a §5.6 Vulkan path 側の **binding 番号 source of truth** に。

接合フロー:
1. Codegen が GLSL parse 時に `uniform sampler2D diffuseMap;` 等を抽出 → `g_sampler_metadata[]` 出力
2. 06a §5 setter (= `LLGLSLShader::uniformTexture(name, ...)`) は Vulkan path で `lookup_sampler(name)` を呼出 (= 本 chapter §5 perfect hash 同居 or §6 metadata 配列線形探索 = 49 個なので O(N) 許容)
3. 取得した `SamplerBinding.binding` で `vkUpdateDescriptorSets` 投入

= 06a §5.6 注記の **chapter 07 確定後 update 項目** に「sampler 配置 set=3 同居」+ 本 chapter Codegen 出力契約を併記 (= 06a §5.6 を chapter 07 / 08 確定後の波及 phase で update)。

---

## §16 update 規律

- §2 CMake target 配線は実 CMakeLists.txt 変更時に live reflect (= 実装 phase 入口で chapter 09 Phase と連動)
- §3 (B1) / §4 (B2) / §5.1 (P) / §5.3 (A1) / §5.5 (G) / §11 (B4) / §12 (B5) の AYA 判断結果は本 chapter §17 から「保留候補」を剥がして reflect、chapter 10 (open-questions) でも追跡
- §6 `ubo_metadata.inl` schema 変更は本 chapter に集約、chapter 04 §5 + 06a §3 へ波及
- §7 sort 規則は不変、変更時は本 chapter で版数管理 (= cache invalidation §11.4 と連動)
- §8 dummy mask 出力 + §10 host header 出力は実装 phase 入口で **Codegen tool 初期実装と並走**、interface 仕様は本 chapter で凍結
- §9 build error format は実装で詳細 polish、本 chapter §9.4 は方針のみ
- §13 3 OS 確証 Phase は chapter 09 Phase 番号体系決定後に Phase X-α/β/γ を具体番号に置換し本 chapter で reflect

---

## §17 未確定事項 (= AYA 判断仰ぎ候補 + chapter 10 持越)

| # | 項目 | default 採用案 | 解消先 |
|---|---|---|---|
| (A1) | std140 offset 計算: Codegen 独自 calculator + SPIR-V reflection 二重保証 vs reflection only | **A1a 二重保証** | **chapter 10 / AYA 判断** |
| (P) | GLSL parse 手段: 独自 mini-parser + glslang -E vs glslang library reflection | **P3 mini-parser + glslang -E** | **chapter 10 / AYA 判断** |
| (G/B3) | perfect hash generator: 独自 Python frozen-table vs gperf vs frozen library | **G2/B3b Python frozen-table** | **chapter 10 / AYA 判断** |
| **(G/B3) ID 衝突注 (= 設計 review 2026-06-03 §3.1 ID rename 整合)**: 本 chapter §17 / §5.5 / chapter 04 §10 の (G) = **perfect hash generator** を指す (= 本 chapter 固有 ID)。chapter 05 §6 の旧 (G) = **per-material cadence** は設計 review §3.1 で **(MC)** に rename 済 (= material cadence prefix)、別概念で衝突しない。本 chapter / chapter 04 の (G) ID はそのまま維持、handoff §4.4 にも (G) ID 衝突解消経緯を反映予定 (= chapter 05 (G)→(MC) のみ rename、chapter 04/08 (G) は不変) | — | — |
| (B1) | Codegen 実装言語: Python vs C++ standalone vs CMake script | **B1a Python 3.8+** | **chapter 10 / AYA 判断** |
| (B2) | glslang 統合: autobuild vendoring vs system pkg vs 自前実装 | **B2a autobuild vendoring (既存温存)** | **chapter 10 / AYA 判断** |
| (B4) | 増分 build cache strategy: hash+mtime vs mtime only vs hash only vs ccache | **B4a hash + mtime 併用** | **chapter 10 / AYA 判断** |
| (B5) | Codegen 実行 trigger: CMake DEPENDS 自動 vs 全 build 時 vs 手動 target only | **B5a 自動 + 手動 target 併設** | **chapter 10 / AYA 判断** |
| (P-future) | 「unused mask」判定基準: GLSL 宣言の有無 only vs shader code 中の member 参照解析 | **宣言の有無 only (= 安全側 dummy 投入)** | 実装 phase 入口で再評価 |
| (cache-grow) | cache file (= codegen_state.json) サイズ上限 / 古 entry GC | **未確定** | chapter 09 Phase Roadmap |

= 8 件 default 提案 + 2 件実装 phase 評価。AYA 判断後の reflect 順序は chapter 10 で集約管理。

---

**= 本 chapter で build system 統合 (CMake / glslang / Codegen tool / autobuild) + (A1)(P)(G/B3)(B1)(B2)(B4)(B5) 7 件確定形 (= default 採用案) + `ubo_metadata.inl` / `ubo_perfect_hash.inl` / `ubo_dummy_init.inl` / `ubo_host_loader.inl` 出力契約 + set=1 80 → 40/40 deterministic split + dummy buffer host 側 init + 7 種 build error 検出 + 3 OS 互換性方針 が確定したため、chapter 09 (phase-roadmap) で Phase 番号体系再編 + 1 UBO ずつ migration scope の起案に進める**。
