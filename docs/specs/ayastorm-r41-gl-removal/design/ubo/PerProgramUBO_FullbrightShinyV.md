# PerProgramUBO_FullbrightShinyV — UBO design (= 実コードベース調査資料)

**通電状態**: **untouched** (= host C++ で `PerProgramUBO_FullbrightShinyV` への setter 呼出ゼロ、grep 確認済 2026-06-06)

**本実装化に必要な作業**: 実 `texture_matrix1` mat4 (= reflection/cubemap UV transform matrix) を host から PerProgram cadence setter 経由で書込 + class1/deferred/fullbrightShinyV.glsl 内 UBO consume へ切替

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_FullbrightShinyV`
- **block_hash**: `0xe0051e7fu`
- **block_size**: 256 B (= std140=64 B, device-padded 256 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_fullbrightshinyv.inl:12-15
struct PerProgramUBO_FullbrightShinyVLayout {
    static constexpr std::uint32_t texture_matrix1_OFFSET = 0u;  // size=64 align=16
};
inline constexpr std::uint32_t PerProgramUBO_FullbrightShinyV_SIZE = 256u; // std140=64, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_fullbright_shiny_v.glsl:9-12
layout(std140, set = 2, binding = 8) uniform PerProgramUBO_FullbrightShinyV
{
    mat4 texture_matrix1;
};
```

= **single mat4 member (= 64 B、shiny cubemap UV transform)**

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 8
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: ubo_metadata.inl:76 `{ "PerProgramUBO_FullbrightShinyV", 0xe0051e7fu, 256u, 2u, 8u, 0u, 1u, 1u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram**
- **flush 経路**: PerProgram cadence setter

---

## §4. 物理 owner

- **shell 段階**: owner なし
- **本実装化後の data source** (= **verify 要**):
  - `LLViewerCamera::getModelview()` の cubemap rotation 部分 (= 推定)
  - 既存 OpenGL 経路で `texture_matrix1` uniformMatrix4fv 書込 site (= `LLShaderMgr::TEXTURE_MATRIX1` 等候補、grep verify 要)
- **lifetime**: program lifetime

---

## §5. use site (shader)

- **blueprint file**: `aya_r41_blueprints/set2/per_program_ubo_fullbright_shiny_v.glsl`
  - source extract from `class1/deferred/fullbrightShinyV.glsl:60` ifdef LL_VULKAN_GLSL block (single site)
- **実 shader use site** (= grep 結果):
  - `indra/newview/app_settings/shaders/class1/deferred/fullbrightShinyV.glsl` (= single site、vertex shader、fullbright shiny pass)

---

## §6. 既存 setter call site (host C++)

- **現状**: **PerProgramUBO_FullbrightShinyV 専用 setter 不在** (= grep 確認済)
- **本実装化後 setter** (= **不明 / verify 要**):
  - `texture_matrix1` uniformMatrix4fv 書込 host site (= `LLShaderMgr::TEXTURE_MATRIX1` 経由候補、grep verify 要)

---

## §7. 現状通電状態

- **状態**: **untouched** (= Phase 1.E 終了時点)

---

## §8. 本実装化に必要な作業

1. **host C++ setter 配線** = fullbrightShinyV program bind 時に texture_matrix1 書込
2. **data source 特定** = camera modelview cubemap rotation 部分 verify
3. **dirty 判定** = camera 変化時 dirty
4. **shader 側 #else block 撤去** (= Phase 2+ 時)

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | set=2 binding=8 独立 | ✅ 独立配置 |
| OS-3 | std140 padding + mat4 align=16 | ✅ 64B → 256B padded、mat4 自然 align |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既存 GLSL #ifdef 既配置 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**mat4 column-major 罠**: std140 mat4 column-major、host C++ → GLSL でも column-major、`uniformMatrix4fv(...., GL_FALSE, ...)` の `transpose=GL_FALSE` 経路と整合

---

## §10. 不明事項

1. **shell 通電 commit hash**
2. **既存 OpenGL setter call site** = `texture_matrix1` 書込 host site
3. **data source 上流** = camera modelview の cubemap rotation 部分か別経路か verify 要
4. **shiny pass の cubemap 方向別 binding** = 6 face cubemap 個別 transform か全 face 共通 transform か verify 要

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 4 binding、本 UBO は binding=8

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 本 batch PerProgram UBO 群 8 件

### §11.3 同 shader consume UBO

- `fullbrightShinyV.glsl` 内 = Frame_* + Material set=1 (= verify 要)

### §11.4 同 data source UBO

- camera modelview 由来 = FrameViewProj (set=0 binding=0) と関連可能性
- reflection probe 系 = Global_ReflectionProbes (set=0 binding=3) + ReflectionProbeUBO_Legacy (set=3 binding=17) と data source 共有可能性 (= 推定)

### §11.5 dirty 連動 UBO

- camera 変化時 = FrameViewProj と連動可能性

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ fullbrightShinyV program bind 時 flush

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.4.10 同期)

**Layer**: L3-10 (= B Tier β setter 推定済、PerProgram cadence、texture_matrix1 cubemap)
**status**: **起案済** (= 2026-06-06 C-5、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.4.10` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch + L0-4 cadence
- **(2) 不明事項**: `texture_matrix1` uniformMatrix4fv setter site (= `LLShaderMgr::TEXTURE_MATRIX1` 経由) [要追加調査] / shiny cubemap 6 face 個別 transform か全 face 共通 [要 verify]
- **(3) 調査手法**: D1 (`TEXTURE_MATRIX1` setter grep) + D2 (cubemap orientation 計算経路)
- **(4) 設計 task**: L3 全件共通 (= PerProgram triple-buffer / `forwardToUboUpload` PER_PROGRAM / program bind 単位 flush / `fullbrightShinyV.glsl` LL_VULKAN_GLSL 活性化)
- **(5) 工程**: trace L3-10、工数 **S**、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= fullbright shiny cubemap 描画既存と同一、visual regression ゼロ §5.4)
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `fullbrightShinyV.glsl #else` block uniform 個別宣言維持

**関連**: L0-1 + L0-4 / §5.4 / `LLShaderMgr::TEXTURE_MATRIX1` (= setter 経路)


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

