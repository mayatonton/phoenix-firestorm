# PerProgramUBO_VolumetricLightF — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `per_program_ubo_volumetric_light_f.glsl` は実 shader `class3/deferred/volumetricLightF.glsl:122 ifdef LL_VULKAN_GLSL block` から literal extract 済 (= shader 側 UBO declaration は既に存在)。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_VolumetricLightF`
- **block_hash**: `0x9b219f1fu` (= FNV-1a("PerProgramUBO_VolumetricLightF"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 4
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_volumetriclightf.inl
struct PerProgramUBO_VolumetricLightFLayout {
    static constexpr std::uint32_t godray_res_OFFSET = 0u;  // size=4 align=4
    static constexpr std::uint32_t godray_multiplier_OFFSET = 4u;  // size=4 align=4
    static constexpr std::uint32_t falloff_multiplier_OFFSET = 8u;  // size=4 align=4
    static constexpr std::uint32_t seconds60_OFFSET = 12u;  // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_VolumetricLightF_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_volumetric_light_f.glsl
layout(std140, set = 2, binding = 18) uniform PerProgramUBO_VolumetricLightF
{
    int   godray_res;
    float godray_multiplier;
    float falloff_multiplier;
    float seconds60;
};
```

= 全 4 member 実 data slot 確定 (= blueprint comment `Source: literal extract from class3/deferred/volumetricLightF.glsl:129 ifdef LL_VULKAN_GLSL block (single site)`)。`seconds60` は **BD legacy dead uniform** (= blueprint comment literal: `seconds60 は BD legacy dead uniform (host setter なし、shader 内未参照)`)。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 18
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通、verify 要)
- **pipeline layout**: `sAYAStandardLayout` (= Phase 1.A 確立 5-set V3a layout、`llvkloader.cpp:881` literal)
- **set 2 配置**: PerDraw+PerProgram 帯 (= INDEX.md §B.4 set mapping)
- **source**: `ubo_metadata.inl:92` `{ "PerProgramUBO_VolumetricLightF", 0x9b219f1fu, 256u, 2u, 18u, 0u, 1u, 4u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal: `constexpr U32 kCadencePerProgram = 1u; // ubo_metadata.inl で 88 件最大`)
- **意味詳細**: program 切替時に flush、program 単位で値を保持 (= `forwardToUboUpload` switch case `kCadencePerProgram` 経路、`llglslshader.cpp:2147`)
- **source**: ubo_metadata.inl:92 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電 (= shell も未配置)
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `pipeline.cpp` 内 `doRenderGodrays` 経路 (= blueprint comment literal: `host = pipeline.cpp doRenderGodrays (uniform1i GODRAY_RES / uniform1f GODRAY_MULTIPLIER / uniform1f FALLOFF_MULTIPLIER)`、verify 要)
  - godray 関連 cvar 直接読出 (= RenderGodraysRes / RenderGodraysMultiplier / RenderGodraysFalloffMultiplier、verify 要)
- **lifetime**: program 単位 (= volumetric godray shader program bind 中のみ有効)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_volumetric_light_f.glsl` (= Phase 1.A PA-8 起案、4 member 実 data slot)
- **実 shader use site**:
  - **`class3/deferred/volumetricLightF.glsl:122 ifdef LL_VULKAN_GLSL block`** (= single site、blueprint comment `Source: literal extract from class3/deferred/volumetricLightF.glsl:129 ifdef LL_VULKAN_GLSL block (single site)` literal)
  - shader 内 UBO declaration 既存 (= blueprint と同 layout)
- **使用 uniform**: godray_res (int) / godray_multiplier (float) / falloff_multiplier (float) / seconds60 (float、dead)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし (= untouched)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL 経路 setter (= `pipeline.cpp` `doRenderGodrays` 内 `uniform1i(GODRAY_RES, ...)` / `uniform1f(GODRAY_MULTIPLIER, ...)` / `uniform1f(FALLOFF_MULTIPLIER, ...)`、blueprint comment literal、grep verify 要)
  - 31 setter 経路 (= mUseUBO 分岐) で UBO 化対応要 (= `forwardToUboUpload` redirect 経路、`llglslshader.cpp:2114-2151`)

---

## §7. 現状通電状態

- **状態**: **untouched** (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)
- **通電 commit**: なし (= 未通電)
- **通電内容**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 範囲、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電** (= Phase 2 着手時):
   - blueprint `per_program_ubo_volumetric_light_f.glsl` ベースで host 側 buffer 配置 + descriptor set 配線
   - dummy buffer write + bind 経路通電 (= `bringupTestUBO` 経路 or 直接 per-program flush 経路)
2. **実 member data 流入**:
   - 既存 OpenGL 経路 `pipeline.cpp doRenderGodrays` の `uniform1i/uniform1f` 呼出 site (= grep verify 要) を UBO 化 (= mUseUBO 分岐で UBO write、OpenGL では既経路温存)
3. **dirty 判定 logic 追加**:
   - PerProgram cadence ゆえ program 切替時に dirty (= 既経路 `forwardToUboUpload` `kCadencePerProgram` case 活用)
4. **flush logic 追加**:
   - PerProgram cadence flush (= `writePerProgramUbo` 経路、verify 要)
5. **shader 接続**:
   - 実 shader `class3/deferred/volumetricLightF.glsl:122 ifdef LL_VULKAN_GLSL block` で UBO declaration 既存 = 追加 shader 改変なし (= OS-5 充足)
   - 既存 OpenGL 経路 (`#else` block) uniform 個別宣言は温存 (= 設計原則 (1) Upstream 取り込みやすさ維持)
6. **dead member 整理**:
   - `seconds60` (BD legacy dead uniform) は member 維持 (= layout 不変契約)、host setter は no-op で良い

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合 (= memory `project_r41_phase2_4_principles` 原則 2):

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 binding=18 配置 | ✅ 維持 (= Phase 1.A 確立 set 2 帯) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16B → 256B padded (codegen 出力) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 (= 既存 ring buffer 経路 query 結果使用確認) |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader (= shader 側 UBO declaration 既存) |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 (= Phase 2 cold launch 時) |

**dead uniform 維持 risk**: `seconds60` は host setter なし dead member、削除すると layout size 変化 → 既 shader extract と不整合。**維持必須** (= layout 不変契約)。

**layout 互換性**: shell layout (= set=2 binding=18 size=256 PSO layout) は Phase 2 本実装と完全一致契約 (= roadmap §4.2)。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 = PerDraw + PerProgram 混在 (= INDEX.md §B.4 set mapping)
- 同 set=2 binding 範囲内の他 PerProgramUBO_* と同居 (= PerProgramUBO_AlphaParams binding=3 / PerProgramUBO_BlurLightF binding=22 等、ubo_metadata.inl 参照)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- ubo_metadata.inl で 73 件 (推定、INDEX.md §1) の最大 cluster、本 UBO はその 1 個
- 同 cluster 全 UBO は `writePerProgramUbo` 経路で flush (= `llglslshader.cpp:2147` case)

### §11.3 同 shader consume UBO (= class3/deferred/volumetricLightF.glsl)

- **不明 / verify 要** = volumetricLightF.glsl 内で他に consume される UBO (= FrameViewProj / FrameLights / FrameAtmosphere_Lighting 等可能性、grep verify 要)
- 既知: depth/diffuseRect sampler binding (= set=0 binding=3 / set=1 binding=4) で共存

### §11.4 同 data source UBO (= 同 host data source から派生)

- **不明 / verify 要** = godray data (cvar / pipeline state) 由来の他 UBO 確認要

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **不明 / verify 要** = godray render program 切替時の同時 dirty UBO 群

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout (= Phase 1.A 確立、全 UBO 共通)

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時に同時 bind (= `vkCmdBindDescriptorSets` set=2 帯)

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **既存 OpenGL 経路 setter call site 完全特定** = `pipeline.cpp doRenderGodrays` 内の uniform1i/uniform1f 呼出行番号 (= grep verify 要)
2. **godray data source の owner class** = LLPipeline / LLViewerCamera / godray cvar 直接読出のいずれか (= grep verify 要)
3. **`seconds60` の本来意図** = BD legacy で何の目的で導入されたか (= BD source 参照要、本 file 起案では未確認)
4. **同 shader file 内同時 consume UBO 一覧** = volumetricLightF.glsl 内全 UBO declaration grep 要
5. **shell 通電 commit** = 本 UBO 用 shell が今後配置される際の commit (= 未来作業、現時点不明)
6. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = `writePerProgramUbo` 経路実装詳細 (= verify 要)
7. **OpenGL 経路 `#else` block の dead `seconds60` host setter 不在** = 上流 OpenGL コードでも setter が無いか (= grep verify 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消、確定後に「不明」記載削除 + 確定 literal 追記。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.4.14 同期)

**Layer**: L3-14 (= B Tier β setter 推定済、PerProgram cadence、godray pipeline)
**status**: **起案済** (= 2026-06-06 C-5、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.4.14` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch + L0-4 cadence
- **(2) 不明事項**: godray cvar setter site (= `pipeline.cpp doRenderGodrays`) [要追加調査] / `seconds60` BD legacy dead 確認 (= UBO 起電時に 0 値 write OK か) [要 verify] / godray cvar 全件 (RenderGodraysRes / Multiplier / FalloffMultiplier) verify [要 verify]
- **(3) 調査手法**: D1 (`GODRAY_RES` / `GODRAY_MULTIPLIER` / `FALLOFF_MULTIPLIER` setter grep) + D2 (`doRenderGodrays` 構造)
- **(4) 設計 task**: L3 全件共通 (= PerProgram triple-buffer / `forwardToUboUpload` PER_PROGRAM / program bind 単位 flush / `volumetricLightF.glsl` LL_VULKAN_GLSL 活性化)
- **(5) 工程**: trace L3-14、工数 **S-M**、L3-15 (godrays F) 並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= volumetric godray 描画既存と同一、AYA r15 連動、visual regression ゼロ §5.4)
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `volumetricLightF.glsl #else` block uniform 個別宣言維持

**関連**: L0-1 + L0-4 / §5.4 / L3-15 GodraysF (= godray pipeline pair) / AYA r15 章
