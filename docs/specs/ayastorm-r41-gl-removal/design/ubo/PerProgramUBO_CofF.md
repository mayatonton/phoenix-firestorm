# PerProgramUBO_CofF — UBO design (= 実コードベース調査資料)

**通電状態**: **untouched** (= host C++ で `PerProgramUBO_CofF` への setter 呼出ゼロ、grep 確認済 2026-06-06)

**本実装化に必要な作業**: 実 DoF (Circle of Confusion) 系定数 (= `depth_cutoff` / `norm_cutoff` / `focal_distance` / `blur_constant` / `tan_pixel_angle` / `magnification`) を host から PerProgram cadence setter 経由で書込 + class1/deferred/cofF.glsl 内 UBO consume へ切替

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_CofF`
- **block_hash**: `0xd36caa4fu`
- **block_size**: 256 B (= std140=32 B, device-padded 256 B)
- **member_count**: 8 (= 実 6 + pad 2)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_coff.inl:12-21
struct PerProgramUBO_CofFLayout {
    static constexpr std::uint32_t depth_cutoff_OFFSET = 0u;
    static constexpr std::uint32_t norm_cutoff_OFFSET = 4u;
    static constexpr std::uint32_t focal_distance_OFFSET = 8u;
    static constexpr std::uint32_t blur_constant_OFFSET = 12u;
    static constexpr std::uint32_t tan_pixel_angle_OFFSET = 16u;
    static constexpr std::uint32_t magnification_OFFSET = 20u;
    static constexpr std::uint32_t _pad0_OFFSET = 24u;
    static constexpr std::uint32_t _pad1_OFFSET = 28u;
};
inline constexpr std::uint32_t PerProgramUBO_CofF_SIZE = 256u; // std140=32, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_cof_f.glsl:9-19
layout(std140, set = 2, binding = 21) uniform PerProgramUBO_CofF
{
    float depth_cutoff;
    float norm_cutoff;
    float focal_distance;
    float blur_constant;
    float tan_pixel_angle;
    float magnification;
    float _pad0;
    float _pad1;
};
```

= **6 実 float member (DoF パラメータ) + 2 pad = 2 vec4 slot 充填**

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 21
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: ubo_metadata.inl:73 `{ "PerProgramUBO_CofF", 0xd36caa4fu, 256u, 2u, 21u, 0u, 1u, 8u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram**
- **flush 経路**: PerProgram cadence setter

---

## §4. 物理 owner

- **shell 段階**: owner なし
- **本実装化後の data source** (= **verify 要**):
  - `gPipeline.mDoFParams` (= 推定、CoF computation pass 用 camera + lens parameters)
  - `LLViewerCamera::getFocalDistance()` / `tan(getFovY() / 2)` 等 (= 推定)
  - 既存 OpenGL 経路で `focal_distance` / `blur_constant` / `tan_pixel_angle` / `magnification` uniform 書込 site grep verify 要
- **lifetime**: program lifetime

---

## §5. use site (shader)

- **blueprint file**: `aya_r41_blueprints/set2/per_program_ubo_cof_f.glsl`
  - source extract from `class1/deferred/cofF.glsl:56` ifdef LL_VULKAN_GLSL block (single site)
- **実 shader use site** (= grep 結果):
  - `indra/newview/app_settings/shaders/class1/deferred/cofF.glsl` (= single site、fragment shader、DoF CoF pass)

---

## §6. 既存 setter call site (host C++)

- **現状**: **PerProgramUBO_CofF 専用 setter 不在** (= grep 確認済)
- **shell 段階通電経路**: bringupTestUBO 経由 generic 通電 (= 推定)
- **本実装化後 setter** (= **不明 / verify 要**):
  - `cofF.glsl` 用 DoF parameter 書込 host site (= `LLPipeline::generateExposure` / `renderDoF` 等候補、grep verify 要)

---

## §7. 現状通電状態

- **状態**: **untouched** (= Phase 1.E 終了時点)
- **通電内容**: shader 側 UBO block 既配置、host C++ writer ゼロ

---

## §8. 本実装化に必要な作業

1. **host C++ setter 配線** = cofF program bind 時に DoF parameter 書込
2. **data source 特定** = `gPipeline.mDoFParams` 等 grep verify
3. **dirty 判定** = focus / fov 変化時 dirty
4. **shader 側 #else block 撤去** (= Phase 2+ 時)

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | set=2 binding=21 独立 | ✅ 独立配置 |
| OS-3 | std140 padding | ✅ 32B → 256B padded、float scalar 連続で罠なし |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既存 GLSL #ifdef 既配置 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**透過 DoF 構造制約注記** (= memory `project_transparent_dof_design_constraint.md` 関連):
- AYAstorm r30 章で透過 DoF の構造的制約 (= L1/L2/B/C 4 案比較) verify 済
- 本 UBO 自体は不透明 DoF (= CoF computation) ゆえ直接関係ないが、PostDeferred 系 UBO と連携 verify 要

---

## §10. 不明事項

1. **shell 通電 commit hash**
2. **既存 OpenGL setter call site** = DoF parameter 書込 host site
3. **data source 上流** = `gPipeline.mDoFParams` 等構造 verify 要
4. **focal_distance auto-focus vs manual focus** = AYAstorm Cinematic mode の DoF override (= r30 章) との関係 verify 要

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 4 binding、本 UBO は binding=21

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 本 batch PerProgram UBO 群 8 件

### §11.3 同 shader consume UBO

- `cofF.glsl` 内 = Frame_* + 他 PerProgram (= verify 要)

### §11.4 同 data source UBO

- DoF 系 UBO 関連 = `DofCombineFParamUBO_Legacy` (set=3 binding=24 cadence=1) と data source 共有可能性 (= 推定)

### §11.5 dirty 連動 UBO

- camera focus 変化時 = DofCombineFParamUBO_Legacy / FrameViewProj と連動可能性

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ cofF program bind 時 flush

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.4.12 同期)

**Layer**: L3-12 (= B Tier β setter 推定済、PerProgram cadence、DoF Circle of Confusion 6 active)
**status**: **起案済** (= 2026-06-06 C-5、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.4.12` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch + L0-4 cadence
- **(2) 不明事項**: DoF CoF parameter setter site (= `LLPipeline::generateExposure` / `renderDoF` 推定) [要追加調査] / focus / fov 変化 dirty trigger [要 verify] / 透過 DoF 構造制約 (memory `project_transparent_dof_design_constraint`) との整合 [要 AYA 判断]
- **(3) 調査手法**: D1 (`depth_cutoff` / `focal_distance` / `blur_constant` 等 setter grep) + D2 (`LLPipeline::renderDoF` 構造)
- **(4) 設計 task**: L3 全件共通 (= PerProgram triple-buffer / `forwardToUboUpload` PER_PROGRAM / program bind 単位 flush / `cofF.glsl` LL_VULKAN_GLSL 活性化)
- **(5) 工程**: trace L3-12、工数 **S-M** (= 6 setter 特定 + focus trigger verify)、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= DoF CoF computation 既存と同一、focus pull 動作 visual 同一、visual regression ゼロ §5.4)
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `cofF.glsl #else` block uniform 個別宣言維持

**関連**: L0-1 + L0-4 / §5.4 / L3-6 DofCombine (= data source 共有候補) / memory `project_transparent_dof_design_constraint`


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

