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
3. **algorithm**: CHD (Compress-Hash-Displace) または FCH (Fox-Chen-Heath) を採用、~500 行 Python で実装、入力 ~88 UBO × 平均 ~10 member ≈ 880 entry 規模に十分高速
4. **出力形式**: chapter 04 §5.3.2 概念形 (= `constexpr UniformLocation g_uniform_table[N]` + `constexpr uint32_t hash_name(const char*)` + `template<auto Name> constexpr UniformLocation resolve()`) を C++ header に書き出す

**G1 不採用根拠**: 3 OS 揃え (= Win で MSYS/mingw 必要) で autobuild に新規 package 追加コスト、現状 AYAstorm autobuild に gperf 不在。
**G3 不採用根拠**: header-only library は compile time 負担と template instantiate 数で build time 増、~880 entry に過剰。
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
