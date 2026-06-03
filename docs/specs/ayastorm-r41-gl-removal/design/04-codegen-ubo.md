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
