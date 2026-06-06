# PerProgramUBO_FxaaF — UBO design (= 実コードベース調査資料)

**通電状態**: **untouched** (= host C++ で `PerProgramUBO_FxaaF` への setter 呼出ゼロ、grep 確認済 2026-06-06)

**本実装化に必要な作業**: 実 FXAA 系定数 (= `rcp_screen_res` / `rcp_frame_opt` / `rcp_frame_opt2`) を host から PerProgram cadence setter 経由で書込 + class1/deferred/fxaaF.glsl 内 UBO consume へ切替

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_FxaaF`
- **block_hash**: `0x00e77989u`
- **block_size**: 256 B (= std140=48 B, device-padded 256 B)
- **member_count**: 5 (= 実 3 + pad 2)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_fxaaf.inl:12-18
struct PerProgramUBO_FxaaFLayout {
    static constexpr std::uint32_t rcp_screen_res_OFFSET = 0u;   // size=8 align=8
    static constexpr std::uint32_t _pad_fxaa0_OFFSET = 8u;       // size=4 align=4
    static constexpr std::uint32_t _pad_fxaa1_OFFSET = 12u;      // size=4 align=4
    static constexpr std::uint32_t rcp_frame_opt_OFFSET = 16u;   // size=16 align=16
    static constexpr std::uint32_t rcp_frame_opt2_OFFSET = 32u;  // size=16 align=16
};
inline constexpr std::uint32_t PerProgramUBO_FxaaF_SIZE = 256u; // std140=48, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_fxaa_f.glsl:9-16
layout(std140, set = 2, binding = 9) uniform PerProgramUBO_FxaaF
{
    vec2  rcp_screen_res;
    float _pad_fxaa0;
    float _pad_fxaa1;
    vec4  rcp_frame_opt;
    vec4  rcp_frame_opt2;
};
```

= **3 実 member (vec2 + vec4 × 2) + 2 pad、FXAA 公式 NVIDIA constant pack**

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 9
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: ubo_metadata.inl:77 `{ "PerProgramUBO_FxaaF", 0x00e77989u, 256u, 2u, 9u, 0u, 1u, 5u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram**
- **flush 経路**: PerProgram cadence setter

---

## §4. 物理 owner

- **shell 段階**: owner なし
- **本実装化後の data source** (= **verify 要**):
  - `LLPipeline` viewport resolution (= 推定、`gPipeline.mViewport[2]` / `[3]`)
  - 既存 OpenGL 経路で `rcp_screen_res` / `rcp_frame_opt` / `rcp_frame_opt2` uniform 書込 site grep verify 要
- **lifetime**: program lifetime (= resolution 変化時更新)

---

## §5. use site (shader)

- **blueprint file**: `aya_r41_blueprints/set2/per_program_ubo_fxaa_f.glsl`
  - source extract from `class1/deferred/fxaaF.glsl:2126` ifdef LL_VULKAN_GLSL block (single site)
- **実 shader use site** (= grep 結果):
  - `indra/newview/app_settings/shaders/class1/deferred/fxaaF.glsl` (= single site、fragment shader、FXAA post-process)

---

## §6. 既存 setter call site (host C++)

- **現状**: **PerProgramUBO_FxaaF 専用 setter 不在** (= grep 確認済)
- **本実装化後 setter** (= **不明 / verify 要**):
  - FXAA pass で resolution 関連 uniform 書込 site (= `LLPipeline::renderFXAA` 等候補、grep verify 要)

---

## §7. 現状通電状態

- **状態**: **untouched** (= Phase 1.E 終了時点)

---

## §8. 本実装化に必要な作業

1. **host C++ setter 配線** = fxaaF program bind 時に FXAA constant pack 書込
2. **data source 特定** = viewport resolution + FXAA recommended constant (= NVIDIA FXAA 3.11 公式 constant、grep verify 要)
3. **dirty 判定** = resolution 変化時 dirty (= window resize / viewport 変化)
4. **shader 側 #else block 撤去** (= Phase 2+ 時)

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | set=2 binding=9 独立 | ✅ 独立配置 |
| OS-3 | std140 padding + vec2 (8B) align=8 罠 | ✅ pad 2 float 明示で 16B align 充足、次 vec4 boundary 確実 |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既存 GLSL #ifdef 既配置 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**vec2 + pad 罠**:
- vec2 (= align=8 size=8) 後に次 vec4 (= align=16) 配置時に std140 で自動 pad だが、codegen は明示 pad 2 float で確実化
- 罠回避済

---

## §10. 不明事項

1. **shell 通電 commit hash**
2. **既存 OpenGL setter call site** = FXAA constant 書込 host site
3. **NVIDIA FXAA constant 由来** = `rcp_frame_opt` / `rcp_frame_opt2` は NVIDIA FXAA 3.11 公式 recommended subpixel/edge constant か別経路か verify 要
4. **viewport resize trigger** = window resize / cinematic mode 切替時の trigger 経路 verify 要

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 4 binding、本 UBO は binding=9

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 本 batch PerProgram UBO 群 8 件

### §11.3 同 shader consume UBO

- `fxaaF.glsl` 内 = Frame_* + 他 PerProgram (= verify 要)

### §11.4 同 data source UBO

- viewport resolution 由来 = FrameViewProj (set=0 binding=0) と関連可能性
- 他 post-process UBO (= SMAAParamUBO_Legacy / GlowFParamUBO_Legacy 等) と viewport 共有可能性 (= verify 要)

### §11.5 dirty 連動 UBO

- viewport resize 時 = 多 UBO 連動可能性 (= 全 post-process pass の resolution 関連)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ fxaaF program bind 時 flush

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.4.11 同期)

**Layer**: L3-11 (= B Tier β setter 推定済、PerProgram cadence、NVIDIA FXAA constant)
**status**: **起案済** (= 2026-06-06 C-5、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.4.11` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch + L0-4 cadence
- **(2) 不明事項**: FXAA constant setter (= `LLPipeline::renderFXAA` 内) [要追加調査] / viewport resize trigger (= window resize 連動) [要 verify]
- **(3) 調査手法**: D1 (`rcp_screen_res` / `rcp_frame_opt` / `rcp_frame_opt2` setter grep) + D2 (`LLPipeline::renderFXAA` 構造)
- **(4) 設計 task**: L3 全件共通 (= PerProgram triple-buffer / `forwardToUboUpload` PER_PROGRAM / program bind 単位 flush / `fxaaF.glsl` LL_VULKAN_GLSL 活性化)
- **(5) 工程**: trace L3-11、工数 **S-M** (= FXAA pass 経路確認)、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= FXAA antialiasing 効果既存と同一、visual regression ゼロ §5.4)
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `fxaaF.glsl #else` block uniform 個別宣言維持

**関連**: L0-1 + L0-4 / §5.4 / L3-2 PostDeferredV (= FXAA_TC_SCALE 共有候補)


---

## §13. Phase 2.α 案 X 確定 record (= blueprint dir 位置付け + 二重 source 同期 protocol)

### §13.1 blueprint dir の位置付け = codegen 入力 source of truth

- **blueprint file** (= `aya_r41_blueprints/<set>/<ubo_lower>.glsl`) は本 UBO の **codegen 入力 source of truth** (= 案 X 確定 2026-06-06)。`indra/cmake/AyaUboCodegen.cmake` の `AYA_UBO_CODEGEN_BLUEPRINT_DIR` 経由で `scripts/ubo_codegen/main.py` の入力に渡され、`ubo_metadata.inl` + `ubo_layout_<ubo>.inl` を生成する。
- **AYAstorm shader runtime compile target は別 GLSL 系統** (= `class*/` + `cinematic_bd/` 配下の実 shader use site) で並列 build process (= design/04-codegen-ubo.md §2.2 literal「別 GLSL 並列 build process」)。
- 二系統は二重 source として共存し、**`scripts/ubo_codegen/main.py` の二重 source 同期 protocol で整合 verify** される (= §13.2)。
- 案 X 確定 source of truth = `docs/specs/ayastorm-r41-gl-removal/handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D.9
- blueprint dir 内 README = `indra/newview/app_settings/shaders/aya_r41_blueprints/README.md` (= phase B commit `f95182ded5`、位置付け literal source)

### §13.2 二重 source 同期 protocol (= main.py で formal化)

- **`_verify_block_match`** (= α-2 commit `b66ec99f72`) = 同名 UBO 複数 file (= blueprint + actual の cross-source pair、または cinematic_bd 上書き path) の set/binding + subset/cadence + member 全件 layout 一致を構造的 verify。不一致時 `CodegenError` で abort。
- **`_verify_blueprint_actual_consistency`** + **`--verify-target-paths`** option (= phase F commit `868bc38cc9`) = blueprint と actual の二重 source 整合 verify を formal化、`--verify-target-paths` で blueprint と actual を区別して対称的 cross-verify。
- 本 UBO の場合 = blueprint file (= §5 / §1 で記載) と実 shader use site (= §5 で記載) が **両 path で同一 layout (set/binding/member)** を保持する protocol。改修時は両方を同期書換するか、blueprint 側のみ書換後 codegen 再生成 + actual の `#ifdef LL_VULKAN_GLSL` block を手動同期する。
- sub-session 5 step 2-batch-0-a 7 UBO の同期書換 record = phase E commit `09ee5e8a8e` (= actual class*/ + cinematic_bd/ 14 file の新 set/binding を blueprint dir 内 7 UBO 7 file に同期反映、案 X 確定後の整合修復)

### §13.3 cross-ref

- 設計 doc = `design/04-codegen-ubo.md` §2.2 (= 別 GLSL 並列 build process) / §4.4 (= 同名 UBO 複数 GLSL 宣言の整合 verify)
- handoff doc = `handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D.9 (= 案 X 確定 source of truth、6 commit revert record + 改修方針 9 件)
- blueprint dir README = `indra/newview/app_settings/shaders/aya_r41_blueprints/README.md` (= phase B commit `f95182ded5`)
- 二重 source 同期 protocol formal化 = `scripts/ubo_codegen/main.py` `_verify_block_match` + `_verify_blueprint_actual_consistency`

