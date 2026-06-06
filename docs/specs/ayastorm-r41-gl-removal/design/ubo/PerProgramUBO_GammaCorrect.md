# PerProgramUBO_GammaCorrect — UBO design (= 実コードベース調査資料)

**通電状態**: **untouched** (= host C++ で `PerProgramUBO_GammaCorrect` への setter 呼出ゼロ、grep 確認済 2026-06-06)

**本実装化に必要な作業**: 実 `gamma` float (= display gamma correction value、SRGB 出力路) を host から PerProgram cadence setter 経由で書込 + class1/deferred/postDeferredGammaCorrect.glsl + postDeferredTonemap.glsl 内 UBO consume へ切替

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_GammaCorrect`
- **block_hash**: `0xf34eebc8u`
- **block_size**: 256 B (= std140=16 B, device-padded 256 B)
- **member_count**: 4 (= 実 1 + pad 3)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_gammacorrect.inl:12-17
struct PerProgramUBO_GammaCorrectLayout {
    static constexpr std::uint32_t gamma_OFFSET = 0u;        // size=4 align=4
    static constexpr std::uint32_t _pad_gc0_OFFSET = 4u;     // size=4 align=4
    static constexpr std::uint32_t _pad_gc1_OFFSET = 8u;     // size=4 align=4
    static constexpr std::uint32_t _pad_gc2_OFFSET = 12u;    // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_GammaCorrect_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_gamma_correct.glsl:10-15
layout(std140, set = 2, binding = 2) uniform PerProgramUBO_GammaCorrect
{
    float gamma;
    float _pad_gc0;
    float _pad_gc1;
    float _pad_gc2;
};
```

= **実 member 1 (gamma) + pad 3、display gamma correction**

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 2
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: ubo_metadata.inl:78 `{ "PerProgramUBO_GammaCorrect", 0xf34eebc8u, 256u, 2u, 2u, 0u, 1u, 4u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram**
- **flush 経路**: PerProgram cadence setter

---

## §4. 物理 owner

- **shell 段階**: owner なし
- **本実装化後の data source** (= **verify 要**):
  - `gSavedSettings.getF32("RenderDeferredDisplayGamma")` (= 推定、display gamma cvar)
  - 既存 OpenGL 経路で `gamma` uniform 書込 site (= `LLShaderMgr::DISPLAY_GAMMA` 等候補、grep verify 要)
- **lifetime**: program lifetime (= cvar 変化時更新)

---

## §5. use site (shader)

- **blueprint file**: `aya_r41_blueprints/set2/per_program_ubo_gamma_correct.glsl`
  - source extract from `class1/deferred/postDeferredGammaCorrect.glsl:45` ifdef LL_VULKAN_GLSL block (verified identical across 2 sample sites = postDeferredGammaCorrect / postDeferredTonemap、blueprint header:1-7 literal)
- **実 shader use site** (= grep 結果):
  - `indra/newview/app_settings/shaders/class1/deferred/postDeferredGammaCorrect.glsl` (= primary site)
  - `indra/newview/app_settings/shaders/class1/deferred/postDeferredTonemap.glsl` (= 2nd site、同 UBO consume)
- = **2 shader file consume**

---

## §6. 既存 setter call site (host C++)

- **現状**: **PerProgramUBO_GammaCorrect 専用 setter 不在** (= grep 確認済)
- **本実装化後 setter** (= **不明 / verify 要**):
  - `LLPipeline::renderGammaCorrect` / `renderTonemap` 経路で gamma 書込 (= grep verify 要)

---

## §7. 現状通電状態

- **状態**: **untouched** (= Phase 1.E 終了時点)

---

## §8. 本実装化に必要な作業

1. **host C++ setter 配線** = postDeferredGammaCorrect + postDeferredTonemap program bind 時に gamma 書込 (= 2 program 共通 UBO ゆえ setter 経路 verify 要)
2. **data source 特定** = `RenderDeferredDisplayGamma` cvar verify
3. **dirty 判定** = cvar 変化時 dirty
4. **shader 側 #else block 撤去** (= Phase 2+ 時)

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | set=2 binding=2 独立 | ✅ 独立配置 |
| OS-3 | std140 padding | ✅ 16B → 256B padded、float scalar + pad で罠なし |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既存 GLSL #ifdef 既配置 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**2 shader file 共有 risk**:
- postDeferredGammaCorrect + postDeferredTonemap 両者で同 UBO consume = host 側 1 UBO instance を 2 program bind 共有可能 (= PerProgram cadence でも data 共通 ゆえ 1 alloc 2 bind 可能性)
- 別案 = 各 program 別 UBO instance (= cadence=1 PerProgram 厳密解釈) = verify 要

---

## §10. 不明事項

1. **shell 通電 commit hash**
2. **既存 OpenGL setter call site** = `gamma` uniform 書込 host site (= `LLShaderMgr::DISPLAY_GAMMA` 等候補、grep verify 要)
3. **2 shader UBO 共有 dispatch** = PerProgram cadence で 2 program 共通 instance か別 instance か verify 要
4. **AYAstorm gamma cvar との関係** = `RenderDeferredDisplayGamma` 以外の AYAstorm cvar (= Cinematic Control 等) の影響可能性 verify 要

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 4 binding、本 UBO は binding=2

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 本 batch PerProgram UBO 群 8 件

### §11.3 同 shader consume UBO

- postDeferredTonemap.glsl 内同時 consume = **PerProgramUBO_ColorGrading (set=2 binding=4)** (= 本 batch、同 shader 内 confirmed)
- postDeferredGammaCorrect.glsl 内 = Frame_* + 他 PerProgram (= verify 要)

### §11.4 同 data source UBO

- gamma correction 系 = TonemapUBO_Legacy (set=3 binding=10 cadence=1) と data source 共有可能性 (= 推定)
- PerProgramUBO_ColorGrading (本 batch) と postDeferredTonemap 内共起 = display 出力路 group

### §11.5 dirty 連動 UBO

- `RenderDeferredDisplayGamma` cvar 変化時 = TonemapUBO_Legacy + PerProgramUBO_ColorGrading と連動可能性

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ postDeferredGammaCorrect / postDeferredTonemap program bind 時 flush

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.10 同期)

**Layer**: L4-10 sub-cluster (c) (= display correction 2 UBO 共有)
**status**: **起案済** (= 2026-06-06 C-6-e、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.10` (= single source of truth)
**要点**: 1 active member (gamma float) + pad ×3、**2 shader file 共有** (= postDeferredGammaCorrect.glsl + postDeferredTonemap.glsl、blueprint `verified identical across 2 sample sites`)、setter 不明 (= `RenderDeferredDisplayGamma` cvar / `LLShaderMgr::DISPLAY_GAMMA` 経路想定) [要追加調査]、**2 shader UBO 共有 dispatch 確定要** (= 1 instance 2 program bind 共有 vs 別 instance、PerProgram cadence 厳密解釈) [要 AYA 判断 + L0-1 dispatch verify]、AYAstorm gamma cvar 連動可能性 (= r30 Cinematic Control 関連) [要 verify]、cadence PerProgram 維持、工数 group 全体 L 内
**関連**: L0-1 dispatch (= 衝突なし binding=2、ただし set=2 PerDraw+PerProgram 混在帯) / §3.5.10 sub-cluster (c) ColorGrading (= postDeferredTonemap 同 shader 内同時 consume) / sub-cluster (b) Tonemap (= display correction + Cinematic Control 連動) / AYAstorm r30 Cinematic Control 13 cvar 配信先候補


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

