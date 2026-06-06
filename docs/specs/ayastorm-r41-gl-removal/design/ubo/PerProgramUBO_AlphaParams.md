# PerProgramUBO_AlphaParams — UBO design (= 実コードベース調査資料)

**通電状態**: **untouched** (= host C++ で `PerProgramUBO_AlphaParams` への setter 呼出ゼロ、grep 確認済 2026-06-06)

**本実装化に必要な作業**: 実 `near_clip` (= viewport near clip plane distance) を host から PerProgram cadence setter 経由で書込 + class1/deferred/alphaV.glsl 内 UBO consume へ切替

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_AlphaParams`
- **block_hash**: `0x68e4c001u` (= FNV-1a("PerProgramUBO_AlphaParams"))
- **block_size**: 256 B (= std140=16 B, device-padded 256 B)
- **member_count**: 4 (= 実 1 + pad 3)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_alphaparams.inl:12-17
struct PerProgramUBO_AlphaParamsLayout {
    static constexpr std::uint32_t near_clip_OFFSET = 0u;   // size=4 align=4
    static constexpr std::uint32_t _pad_ap0_OFFSET = 4u;    // size=4 align=4
    static constexpr std::uint32_t _pad_ap1_OFFSET = 8u;    // size=4 align=4
    static constexpr std::uint32_t _pad_ap2_OFFSET = 12u;   // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_AlphaParams_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_alpha_params.glsl:9-15
layout(std140, set = 2, binding = 3) uniform PerProgramUBO_AlphaParams
{
    float near_clip;
    float _pad_ap0;
    float _pad_ap1;
    float _pad_ap2;
};
```

= **実 member 1 (near_clip) + pad 3 float = 1 vec4 slot 充填、alphaV.glsl per-program 定数**

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 3
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC` (= set=2 UBO_DYNAMIC 帯、`llvkloader.cpp:861` literal)
- **pipeline layout**: `sAYAStandardLayout`
- **set=2 binding=3** = 独立 binding (= set=2 binding=0 共有 6 UBO 群とは別 binding)
- **source**: ubo_metadata.inl:71 `{ "PerProgramUBO_AlphaParams", 0x68e4c001u, 256u, 2u, 3u, 0u, 1u, 4u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal: `constexpr U32 kCadencePerProgram = 1u; // ubo_metadata.inl で 88 件最大` ※ source comment は historical literal、実数 80 件 = Phase 2.L0 sub-session 3 step 1 grep 確定、source comment 訂正は step 2 以降の `indra/` 改変 phase 持越)
- **flush 経路**: PerProgram cadence setter (= `llglslshader.cpp:2143-2150` `case kCadencePerProgram:` dispatch)
- **意味詳細**: program 単位 (= LLGLSLShader 単位) で 1 UBO instance、program bind 時に flush
- **source**: llglslshader.cpp:95 + setter dispatch literal

---

## §4. 物理 owner

- **shell 段階**: owner なし
- **本実装化後の data source** (= **verify 要**):
  - `LLViewerCamera::getNear()` (= 推定、viewport near plane)
  - 既存 OpenGL 経路で `LLShaderMgr::NEAR_CLIP` uniform を書込む site (= grep verify 要)
- **lifetime**: program lifetime (= LLGLSLShader instance 単位、program 再 link 時更新)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_alpha_params.glsl`
  - source extract from `class1/deferred/alphaV.glsl:145` ifdef LL_VULKAN_GLSL block (single site)
- **実 shader use site** (= grep 結果):
  - `indra/newview/app_settings/shaders/class1/deferred/alphaV.glsl` (= single site、vertex shader)

---

## §6. 既存 setter call site (host C++)

- **現状**: **PerProgramUBO_AlphaParams 専用 setter 不在** (= grep 確認済 2026-06-06)
- **shell 段階通電経路**: 推定 = bringupTestUBO 経由 generic zero buffer 通電
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL 経路で `near_clip` uniform 書込 site (= `LLShaderMgr::NEAR_CLIP` 等候補、grep verify 要)
  - PerProgram cadence ゆえ `LLGLSLShader::bind` 時に flush 経路 (= verify 要)

---

## §7. 現状通電状態

- **状態**: **untouched** (= Phase 1.E 終了時点)
- **通電 commit**: なし
- **通電内容**: shader 側 `class1/deferred/alphaV.glsl` で UBO consume block 既配置、host C++ writer ゼロ

---

## §8. 本実装化に必要な作業

1. **host C++ setter 配線** = alphaV program bind 時に `near_clip` 値書込 (= PerProgram cadence 経路、`llglslshader.cpp:2143-2150` setter dispatch)
2. **data source 特定** = `LLViewerCamera::getNear()` 等 grep verify
3. **dirty 判定** = camera near plane 変化時 dirty、program bind 時 flush
4. **shader 側 #else block 撤去** (= Phase 2+ 時)

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | set=2 binding=3 独立 binding | ✅ 独立配置 |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded、1 vec4 slot |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既存 GLSL #ifdef LL_VULKAN_GLSL 既配置 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**pad 3 float の罠**: std140 で float 単独宣言は次 vec4 boundary までの自動 pad ありだが、codegen は明示 pad で確実化 = 罠なし

---

## §10. 不明事項

1. **shell 通電 commit hash** = bringupTestUBO 経路通電 commit
2. **既存 OpenGL setter call site** = `near_clip` uniform 書込 host site
3. **data source 上流** = `LLViewerCamera::getNear()` か別経路か verify 要

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 4 binding、本 UBO は binding=3

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram、80 件 = Phase 2.L0 sub-session 3 step 1 grep 確定、`llglslshader.cpp:95` source comment 「88 件最大」は historical literal)

- 本 batch 担当 PerProgram UBO 7 (= BlurLightF / CofF / ColorGrading / FsObjectIdF / FullbrightShinyV / FxaaF / GammaCorrect)
- 他 PerProgram UBO 多数 (= ubo_metadata.inl で **80 件** = Phase 2.L0 sub-session 3 step 1 grep 確定、`llglslshader.cpp:95` source comment 「88 件最大」は historical literal)

### §11.3 同 shader consume UBO

- `class1/deferred/alphaV.glsl` 内同時 consume = Frame_* + set=1 Material + 他 PerDraw / PerProgram (= verify 要)

### §11.4 同 data source UBO

- camera near plane 由来 = FrameViewProj (set=0 binding=0) と関連可能性 (= 推定、verify 要)

### §11.5 dirty 連動 UBO

- camera 変化時 = FrameViewProj 等と連動可能性

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時 flush + set=2 binding 構成内に常駐

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.4.1 同期)

**Layer**: L3-1 (= B Tier β setter 推定済、PerProgram cadence、1 active member near_clip)
**status**: **起案済** (= 2026-06-06 C-5、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.4.1` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch + L0-4 cadence
- **(2) 不明事項**: `NEAR_CLIP` uniform 書込 site (= grep 要) [要追加調査] / `LLViewerCamera::getNear()` data source verify [要 verify]
- **(3) 調査手法**: D1 (`LLShaderMgr::NEAR_CLIP` setter grep) + D2 (`LLViewerCamera` near plane state)
- **(4) 設計 task**: L3 全件共通 (= PerProgram cadence triple-buffer / `forwardToUboUpload` PER_PROGRAM case / program bind 単位 flush / `alphaV.glsl` `#ifdef LL_VULKAN_GLSL` block 活性化)
- **(5) 工程**: trace L3-1、工数 **S** (= 数時間)、L3-2〜L3-9 並列可
- **(6) A 確定**: setter 通電 + Vulkan validation 0 + AYA live verify (= alpha pass visual 既存と同一、**visual regression ゼロ §5.4**)
- **(7) 4 原則 gate**: 全 ✅ (= L3 全件共通)、原則 4 = `alphaV.glsl #else` block uniform 個別宣言維持

**関連**: L0-1 + L0-4 (= WORK_ORDER §2) / §5.4 visual regression policy / L1a-1 ClipF (= PerProgram cadence + manip pilot 経路 reference)


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

