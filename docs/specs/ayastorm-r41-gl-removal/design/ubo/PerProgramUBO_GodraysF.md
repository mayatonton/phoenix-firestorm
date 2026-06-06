# PerProgramUBO_GodraysF — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `per_program_ubo_godrays_f.glsl` は実 shader `class1/deferred/godraysF.glsl:124 ifdef LL_VULKAN_GLSL block` から literal extract 済 (= shader 側 UBO declaration 既存)。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_GodraysF`
- **block_hash**: `0x422cfe9eu` (= FNV-1a("PerProgramUBO_GodraysF"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 3
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_godraysf.inl
struct PerProgramUBO_GodraysFLayout {
    static constexpr std::uint32_t aya_r15_godrays_enabled_OFFSET = 0u;  // size=4 align=4
    static constexpr std::uint32_t aya_r15_godrays_phase_exponent_OFFSET = 4u;  // size=4 align=4
    static constexpr std::uint32_t aya_r15_godrays_strength_OFFSET = 8u;  // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_GodraysF_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_godrays_f.glsl
layout(std140, set = 2, binding = 17) uniform PerProgramUBO_GodraysF
{
    int   aya_r15_godrays_enabled;
    float aya_r15_godrays_phase_exponent;
    float aya_r15_godrays_strength;
};
```

= 全 3 member 実 data slot 確定 (= blueprint comment literal: `Source: literal extract from class1/deferred/godraysF.glsl:124 ifdef LL_VULKAN_GLSL block (single site)`)。全 3 member は AYAstorm r15 (godrays) cvar 由来 (= blueprint comment: `AYAstorm r15 cvar 群 (aya_r15_godrays_*) を UBO 化。godraysV は uniform 不使用 = F 単独 attach`)。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 17
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通、verify 要)
- **pipeline layout**: `sAYAStandardLayout` (= Phase 1.A 確立 5-set V3a layout)
- **set 2 配置**: PerDraw + PerProgram 帯 (= INDEX.md §B.4 set mapping)
- **source**: `ubo_metadata.inl:79` `{ "PerProgramUBO_GodraysF", 0x422cfe9eu, 256u, 2u, 17u, 0u, 1u, 3u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal: `constexpr U32 kCadencePerProgram = 1u; // ubo_metadata.inl で 88 件最大`)
- **意味詳細**: program 切替時に flush、program 単位で値を保持 (= `forwardToUboUpload` switch case `kCadencePerProgram` 経路、`llglslshader.cpp:2147`)
- **source**: ubo_metadata.inl:79 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電 (= shell も未配置)
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `pipeline.cpp:5357` `LLCachedControl<bool> aya_r15_in_cinematic(gSavedSettings, "AYAR15GodraysInCinematicEnabled", false)` 経由 cvar (= AYAstorm r15 godray cvar 群、AYAR15Godrays* cvar 直接読出経路、verify 要)
  - `aya_r15_godrays_enabled` / `aya_r15_godrays_phase_exponent` / `aya_r15_godrays_strength` の各 cvar key (= grep verify 要)
- **lifetime**: program 単位 (= godrays shader program bind 中のみ有効)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_godrays_f.glsl` (= Phase 1.A PA-8 起案、3 member 実 data slot)
- **実 shader use site**:
  - **`class1/deferred/godraysF.glsl:124 ifdef LL_VULKAN_GLSL block`** (= single site、blueprint comment literal)
  - 既存 UBO block (godraysF.glsl:125-127):
    ```glsl
    int   aya_r15_godrays_enabled;          // offset 0,  size 4 + 12 pad
    float aya_r15_godrays_phase_exponent;   // offset 16, size 4 + 12 pad
    float aya_r15_godrays_strength;         // offset 32, size 4 + 12 pad
    ```
  - 既存 OpenGL `#else` block (godraysF.glsl:131-133):
    ```glsl
    uniform int aya_r15_godrays_enabled;
    uniform float aya_r15_godrays_phase_exponent;
    uniform float aya_r15_godrays_strength;
    ```
  - 使用箇所: `godraysF.glsl:143` (`if (aya_r15_godrays_enabled <= 0)`) + `:231` (`pow(cos_theta, max(aya_r15_godrays_phase_exponent, 1.0))`) + `:234` (`light_color * accum * phase * aya_r15_godrays_strength`)
- **GLSL ↔ blueprint padding 差分注意**: shader `#ifdef LL_VULKAN_GLSL` block は **scalar に 12 B pad 込み** (offset 0/16/32) で記述、blueprint と codegen layout は **packed** (offset 0/4/8) で std140 解釈。**要 verify**: shader 側 `// offset N` コメントは documentation のみ、std140 が確定するのは codegen layout。

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし (= untouched)
- **既存 OpenGL 経路 setter**:
  - `llshadermgr.cpp:1603-1605` で `mReservedUniforms.push_back("aya_r15_godrays_enabled" / "_phase_exponent" / "_strength")` (= reserved uniform 登録、`<FS:AYAstorm r30 BD改善>` tag)
  - 実 `uniform1i` / `uniform1f` 呼出 site (= **不明 / verify 要**、pipeline.cpp 内 godrays render 経路、`5357` 周辺 cinematic gate と関連)
- **本実装化後 setter** (= **不明 / verify 要**):
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
   - blueprint `per_program_ubo_godrays_f.glsl` ベースで host 側 buffer 配置 + descriptor set 配線
   - dummy buffer write + bind 経路通電 (= `bringupTestUBO` 経路 or 直接 per-program flush 経路)
2. **実 member data 流入**:
   - 既存 OpenGL 経路 `pipeline.cpp` godrays uniform setter (= grep verify 要、`5357` cinematic gate 周辺) を UBO 化 (= mUseUBO 分岐で UBO write、OpenGL では既経路温存)
3. **dirty 判定 logic 追加**:
   - PerProgram cadence ゆえ program 切替時に dirty (= 既経路 `forwardToUboUpload` `kCadencePerProgram` case 活用)
4. **flush logic 追加**:
   - PerProgram cadence flush (= `writeProgramUbo` 経路、`llvkloader.cpp:5503` literal、verify 要)
5. **shader 接続**:
   - 実 shader `class1/deferred/godraysF.glsl:124 ifdef LL_VULKAN_GLSL block` で UBO declaration 既存 = 追加 shader 改変なし (= OS-5 充足)
   - 既存 OpenGL `#else` block (`:131-133`) uniform 個別宣言は温存 (= 設計原則 (1) Upstream 取り込みやすさ維持)
6. **AYAstorm r15 cvar 連動**:
   - `AYAR15GodraysInCinematicEnabled` (= `pipeline.cpp:5357` LLCachedControl) で dispatch 制御、本 UBO 自体は cvar 値の flush wrapping を担う

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合 (= memory `project_r41_phase2_4_principles` 原則 2):

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 binding=17 配置 | ✅ 維持 (= Phase 1.A 確立 set 2 帯) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16B → 256B padded (codegen 出力) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 (= 既存 ring buffer 経路 query 結果使用確認) |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader (= shader 側 UBO declaration 既存) |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 (= Phase 2 cold launch 時) |

**std140 解釈差分 risk**: shader 側 UBO block の `// offset 0/16/32 + 12 pad` コメントと codegen layout `OFFSET = 0u/4u/8u` (packed) が**異なる**。codegen が std140 上正しいが、SPIR-V compile 時の実際の offset を validation 要 (= Phase 2 cold launch 時 vk validation 確認)。

**AYAstorm 独自 cvar**: 全 3 member は AYAstorm r15 独自 cvar 由来 (= upstream Firestorm 系列に存在しない)。upstream 取り込み時に conflict なし (= 設計原則 (1) 充足)。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 = PerDraw + PerProgram 混在 (= INDEX.md §B.4 set mapping)
- 同 set=2 binding 範囲内の他 PerProgramUBO_* と同居 (= ubo_metadata.inl 参照)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- ubo_metadata.inl で 73 件 (推定、INDEX.md §1) の最大 cluster、本 UBO はその 1 個
- 同 cluster 全 UBO は `writeProgramUbo` 経路で flush (= `llvkloader.cpp:5503` + `llglslshader.cpp:2147` case)

### §11.3 同 shader consume UBO (= class1/deferred/godraysF.glsl)

- **不明 / verify 要** = godraysF.glsl 内で他に consume される UBO (= FrameViewProj / FrameLights / FrameAtmosphere_Lighting 等可能性、grep verify 要)
- 既知: 深度 sampler / depthMap sampler 経由の rendering、godrays shader は post-deferred stage

### §11.4 同 data source UBO (= 同 host data source から派生)

- **不明 / verify 要** = AYAstorm r15 godray cvar 群由来の他 UBO (= cinematic mode 関連 UBO、verify 要)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **不明 / verify 要** = godrays render program 切替時の同時 dirty UBO 群

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout (= Phase 1.A 確立、全 UBO 共通)

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時に同時 bind (= `vkCmdBindDescriptorSets` set=2 帯)

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **既存 OpenGL 経路 setter call site 完全特定** = `pipeline.cpp` 内 `aya_r15_godrays_*` uniform setter 呼出行番号 (= grep verify 要、`5357` cinematic gate 周辺)
2. **godray cvar 直接 owner** = `AYAR15GodraysEnabled` / `AYAR15GodraysPhaseExponent` / `AYAR15GodraysStrength` cvar key 実在確認 (= grep verify 要)
3. **shader `// offset 0/16/32 + 12 pad` コメント vs codegen `OFFSET = 0u/4u/8u` の正解** = std140 仕様上どちらが SPIR-V compile 後の実 offset か (= Phase 2 cold launch validation 必須)
4. **同 shader file 内同時 consume UBO 一覧** = godraysF.glsl 内全 UBO declaration grep 要
5. **shell 通電 commit** = 本 UBO 用 shell が今後配置される際の commit (= 未来作業、現時点不明)
6. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = `writeProgramUbo` 経路実装詳細 (= verify 要)
7. **AYAR15GodraysInCinematicEnabled と本 UBO の relation** = cinematic gate が本 UBO の write/flush 経路を gate するか、それとも shader dispatch のみ gate するか (= `pipeline.cpp:5357 / 11819` verify 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消、確定後に「不明」記載削除 + 確定 literal 追記。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.4.15 同期)

**Layer**: L3-15 (= B Tier β setter 推定済、PerProgram cadence、AYAstorm r15 godray cvar 3 件)
**status**: **起案済** (= 2026-06-06 C-5、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.4.15` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch + L0-4 cadence
- **(2) 不明事項**: AYAstorm r15 cvar setter (= `pipeline.cpp:5357` `aya_r15_in_cinematic` 周辺 + direct cvar 読出) [要追加調査] / shader `// offset 0/16/32 + 12 pad` vs codegen packed 整合 [要 verify]
- **(3) 調査手法**: D1 (`aya_r15_godrays_enabled` / `_phase_exponent` / `_strength` setter grep) + D4 (shader vs codegen offset 整合)
- **(4) 設計 task**: L3 全件共通 (= PerProgram triple-buffer / `forwardToUboUpload` PER_PROGRAM / program bind 単位 flush / `godraysF.glsl` LL_VULKAN_GLSL 活性化)
- **(5) 工程**: trace L3-15、工数 **S-M**、L3-14 並列可、AYA r15 既存機能維持必須 (= memory `project_ayastorm_r30_bd_improvement_phase`)
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= godray 描画 + 強度・位相既存と同一、AYA r15 章機能維持、visual regression ゼロ §5.4)
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `godraysF.glsl #else` block uniform 個別宣言維持

**関連**: L0-1 + L0-4 / §5.4 / L3-14 VolumetricLightF (= godray pipeline pair) / AYA r15 章 (= memory)
