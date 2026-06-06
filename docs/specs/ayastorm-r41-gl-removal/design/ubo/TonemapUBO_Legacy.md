# TonemapUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.A PA-8 blueprint 起案済、Phase 1.C PC-2/PC-7δ per-program cadence 一括通電対象に含まれている可能性大 / verify 要)

**本実装化に必要な作業**: shell 4 member (= exposure / tonemap_mix / tonemap_type / _pad) を実 tonemap data (= LLPipeline / LLEnvironment exposure system 経由、verify 要) に置換 + dirty 判定 logic 追加 + 実 shader (= class1/deferred/tonemapUtilF.glsl) consume 接続検証

---

## §1. UBO identity

- **block_name**: `TonemapUBO_Legacy`
- **block_hash**: `0x7247e6ebu`
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 4
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_tonemapubo_legacy.inl
struct TonemapUBO_LegacyLayout {
    static constexpr std::uint32_t exposure_OFFSET = 0u;  // size=4 align=4
    static constexpr std::uint32_t tonemap_mix_OFFSET = 4u;  // size=4 align=4
    static constexpr std::uint32_t tonemap_type_OFFSET = 8u;  // size=4 align=4
    static constexpr std::uint32_t _pad_tonemap_0_OFFSET = 12u;  // size=4 align=4
};
inline constexpr std::uint32_t TonemapUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/tonemap_ubo_legacy.glsl
layout(std140, set = 3, binding = 10) uniform TonemapUBO_Legacy
{
    float exposure;
    float tonemap_mix;
    int   tonemap_type;
    float _pad_tonemap_0;
};
```

= **blueprint literal extract from `class1/deferred/tonemapUtilF.glsl:146` `#ifdef LL_VULKAN_GLSL` block** (= blueprint header line 3 明示)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 10
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 内訳**: 本 UBO binding=10 = Legacy 帯独立 binding
- **source**: `ubo_metadata.inl:114` `{ "TonemapUBO_Legacy", 0x7247e6ebu, 256u, 3u, 10u, 0u, 1u, 4u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` `kCadencePerProgram = 1u`)
- **source**: ubo_metadata.inl:114 + llglslshader.cpp:95

---

## §4. 物理 owner

- **shell 段階 owner**: 未確認
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `exposure` = HDR exposure value (= LLPipeline ExposureFParamUBO_Legacy 計算結果由来 / LLEnvironment auto-exposure)
  - `tonemap_mix` = tonemap mix factor (= debug settings RenderTonemapMix / Cinematic mode 由来、verify 要)
  - `tonemap_type` = tonemap operator type int (= ACES / Reinhard / Linear 等の enum、debug settings RenderTonemapType 由来)
  - `_pad_tonemap_0` = std140 16 B alignment padding (= 4 × float = 16 B、unused)
- **lifetime**: per-program (= tonemap post-pass shader instance)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/tonemap_ubo_legacy.glsl` (= Phase 1.A PA-8 起案、Source `class1/deferred/tonemapUtilF.glsl:146`)
- **実 shader use site**:
  - `class1/deferred/tonemapUtilF.glsl` (= blueprint header line 3 literal reference)
- **consume status**: blueprint literal extract source ゆえ既 consume

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter** (= verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL setter = `uniform1f("exposure", ...)` / `uniform1f("tonemap_mix", ...)` / `uniform1i("tonemap_type", ...)` (= verify 要)
  - data source = LLPipeline post-processing pass / LLEnvironment auto-exposure / debug settings RenderTonemap*

---

## §7. 現状通電状態

- **状態**: **shell 通電有無 verify 要**
- **通電内容** (= 推定): zero dummy buffer write + per-program cadence 経路通電

---

## §8. 本実装化に必要な作業

1. **shell 4 member → 実 tonemap data 接続**:
   - exposure = auto-exposure 計算結果接続 (= ExposureFParamUBO_Legacy と data source 共有可能性)
   - tonemap_mix = debug setting / Cinematic mode tone 接続
   - tonemap_type = debug setting enum 接続
2. **dirty 判定 logic 追加**:
   - dirty 判定 trigger = exposure 変化 / tonemap settings 変更
3. **flush logic 追加**:
   - per-program cadence ゆえ既経路活用
4. **shader 接続検証**:
   - 既存 `class1/deferred/tonemapUtilF.glsl` `#ifdef LL_VULKAN_GLSL` block (line 146 周辺) で UBO member access 動作確認
   - tonemap_type int 切替で ACES / Reinhard / Linear 等動作 verify

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 (binding=10 独立) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16 B → 256 B padded (4 × float/int = 16 B std140) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint = 実 shader literal extract |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**tonemap 視覚表現章関連**:
- AYAstorm r14+ 視覚表現章 / r30 Cinematic mode で tonemap 系 cvar 多数追加 (= memory `project_ayastorm_visual_realism_chapter` + `project_ayastorm_r30_cinematic_chapter`)
- 本 UBO は tonemap util 共通 base、AYAstorm 独自拡張は別 UBO (= verify 要)

**cadence 再評価候補**:
- `exposure` は auto-exposure ゆえ per-frame で変化、per-program cadence では同 program 内 frame 間 stale data risk
- per-frame 系への移動候補、Phase 2 で再評価

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- Asset/Skin 帯 + Legacy 帯各種、本 UBO binding=10 は独立

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- PerProgram cluster 全 88 件

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- `class1/deferred/tonemapUtilF.glsl` 内同時 consume UBO (= verify 要):
  - 推定候補: post-processing pipeline 経路 UBO (= ExposureFParamUBO_Legacy / GlowCombineFParamUBO_Legacy / DofCombineFParamUBO_Legacy / PerProgramUBO_ColorGrading 等)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **ExposureFParamUBO_Legacy** (= set=3 binding=25、PerProgram、4 member、ubo_metadata.inl:39) = exposure UBO、本 UBO の exposure member と data source 共有可能性
- **LuminanceFParamUBO_Legacy** (= set=3 binding=26、PerProgram、1 member、ubo_metadata.inl:51) = luminance UBO、auto-exposure 計算経路で連動可能性
- **PerProgramUBO_ColorGrading** (= set=2 binding=4、PerProgram、8 member、ubo_metadata.inl:74) = color grading UBO、tonemap 後段 post-pass

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **ExposureFParamUBO_Legacy** (= exposure 計算更新時の連動候補)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- per-program cadence ゆえ tonemapUtilF program bind 時に descriptor set 3 更新

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電有無** = handoff doc chain 参照要
2. **exposure data source** = ExposureFParamUBO_Legacy と data source 共有経路 (= verify 要)
3. **tonemap_mix / tonemap_type data source** = debug settings RenderTonemap* / Cinematic mode 経路 (= verify 要)
4. **既存 OpenGL setter call site** = uniform1f/1i("exposure"/"tonemap_mix"/"tonemap_type", ...) grep verify 要
5. **cadence 適正性** = auto-exposure per-frame 変化の per-program cadence stale risk (= verify 要)
6. **AYAstorm 独自 tonemap 拡張** = r30 Cinematic mode tonemap 系 cvar との関係 (= verify 要)
7. **同 shader consume UBO 完全特定** = class1/deferred/tonemapUtilF.glsl 内同時 consume UBO 群 (= grep verify 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.10 同期)

**Layer**: L4-10 sub-cluster (b) (= Tonemap 1 UBO、Cinematic mode 関連)
**status**: **起案済** (= 2026-06-06 C-6-e、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.10` (= single source of truth)
**要点**: 4 member (exposure float + tonemap_mix float + tonemap_type int + _pad_tonemap_0)、setter 不明 [要追加調査]、tonemap util 共通 base、AYAstorm r30 Cinematic Control 13 cvar 関連候補 (= tonemap_mix / tonemap_type 経路、memory `project_r30_cinematic_control_tuning_deferred`)、exposure data source = ExposureFParamUBO_Legacy 計算結果由来 / LLEnvironment auto-exposure (= sub-cluster (a) と data source 共有可能性 [要 verify D4 突合])、cadence PerProgram 維持 (= frame 内 tonemap pass 1 回ゆえ frame 内 1 回 GPU upload で済む、auto-exposure per-frame 変化に追従可能)、tonemap_type ACES/Reinhard/Linear enum int 切替、tonemapUtilF.glsl:146 singleton site、工数 group 全体 L 内
**関連**: L0-1 dispatch (= 衝突なし binding=10) / §3.5.10 sub-cluster (a) Exposure (= exposure data source 共有候補) / sub-cluster (c) ColorGrading/GammaCorrect (= post-process chain + r30 Cinematic Control 連動) / sub-cluster (d) Vignette (= post-process chain 後段)
