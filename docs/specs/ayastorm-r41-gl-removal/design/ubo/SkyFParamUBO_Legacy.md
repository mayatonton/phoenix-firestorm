# SkyFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.A PA-8 blueprint 起案済、Phase 1.C PC-2 通電対象に含まれていたか verify 要、bringupTestUBO 経由 zero dummy buffer write は per-program cadence 全 UBO 一括対象ゆえ shell 通電済の可能性大 / verify 要)

**本実装化に必要な作業**: shell 4 member (= hdri_split_screen / moisture_level / droplet_radius / ice_level) を実 sky environment data (= LLSettingsSky / WindLight 経由、verify 要) に置換 + dirty 判定 logic 追加 + flush logic 追加 (= per-program flush 経路既存活用) + 実 shader (= class1/deferred/skyF.glsl) consume 接続検証

---

## §1. UBO identity

- **block_name**: `SkyFParamUBO_Legacy`
- **block_hash**: `0x48b26ebfu` (= FNV-1a("SkyFParamUBO_Legacy"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B = 256 B 倍数 padding 充足)
- **member_count**: 4
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_skyfparamubo_legacy.inl
struct SkyFParamUBO_LegacyLayout {
    static constexpr std::uint32_t hdri_split_screen_OFFSET = 0u;  // size=4 align=4
    static constexpr std::uint32_t moisture_level_OFFSET = 4u;  // size=4 align=4
    static constexpr std::uint32_t droplet_radius_OFFSET = 8u;  // size=4 align=4
    static constexpr std::uint32_t ice_level_OFFSET = 12u;  // size=4 align=4
};
inline constexpr std::uint32_t SkyFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/sky_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 2) uniform SkyFParamUBO_Legacy
{
    float hdri_split_screen;
    float moisture_level;
    float droplet_radius;
    float ice_level;
};
```

= **blueprint literal extract from `class1/deferred/skyF.glsl:117` `#ifdef LL_VULKAN_GLSL` block** (= blueprint header line 3 明示)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 2
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通)
- **pipeline layout**: `sAYAStandardLayout` (= Phase 1.A 確立 5-set layout)
- **set 3 内訳**: set=3 は Asset (binding=0/1) + Skin (binding=2 = Skin_GLTFJoints) + Legacy UBO 群 (binding=2..62)
  - **本 UBO binding=2 は Skin_GLTFJoints と同 binding** = Legacy UBO は別 program ゆえ同時 bind 不要、program 単位で descriptor set 内容差替で運用 (= `kCadencePerProgram` cadence 経路、verify 要)
- **source**: `ubo_metadata.inl:106` `{ "SkyFParamUBO_Legacy", 0x48b26ebfu, 256u, 3u, 2u, 0u, 1u, 4u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal: `constexpr U32 kCadencePerProgram = 1u; // ubo_metadata.inl で 88 件最大` ※ source comment は historical literal、実数 80 件 = Phase 2.L0 sub-session 3 step 1 grep 確定、source comment 訂正は step 2 以降の `indra/` 改変 phase 持越)
- **意味詳細**: per-program cadence、`flushPerProgramUbos` 等価経路 (= llglslshader.cpp:2147 case kCadencePerProgram)、program bind 時 UBO 更新
- **source**: ubo_metadata.inl:106 + llglslshader.cpp:95

---

## §4. 物理 owner

- **shell 段階 owner**: 未確認 (= bringupTestUBO 経由 zero dummy buffer 経路で shell 通電有無 verify 要)
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `hdri_split_screen` = HDRI debug split-screen toggle (= LLSettingsSky / debug setting 由来、verify 要)
  - `moisture_level` / `droplet_radius` / `ice_level` = WindLight cloud microphysics params (= LLSettingsSky / LLEnvironment 由来、verify 要)
- **lifetime**: per-program (= LLGLSLShader instance lifetime)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/sky_f_param_ubo_legacy.glsl` (= Phase 1.A PA-8 起案、Source literal extract from `class1/deferred/skyF.glsl:117`)
- **実 shader use site**:
  - `class1/deferred/skyF.glsl` (= blueprint header line 3 literal reference、grep verify)
- **consume status**: blueprint literal extract source ゆえ既 consume (= shader 側 `#ifdef LL_VULKAN_GLSL` block で UBO member access、`#else` で個別 uniform)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter** (= verify 要): `LLGLSLShader::bringupTestUBO()` で per-program cadence 全 UBO 一括対象に本 UBO が含まれているか確認要 (= `llglslshader.cpp:2147` case kCadencePerProgram 経路)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL 経路 uniform setter (= `uniform1f("hdri_split_screen", ...)` 等) call site grep verify 要
  - data 書込み tip = LLSettingsSky / WindLight 経由 cloud microphysics params 集約 (= 推定、verify 要)

---

## §7. 現状通電状態

- **状態**: **shell 通電有無 verify 要** (= Phase 1.A PA-8 blueprint 起案済 + Phase 1.C PC-2/PC-7δ で per-program cadence 一括通電対象に含まれている可能性大、handoff doc 参照要)
- **通電 commit**: 不明 (= Phase 1.C handoff doc chain `handoff/phase1/c/` 参照要、本 file 起案では未引用)
- **通電内容** (= 推定、verify 要):
  - zero dummy buffer write (= 16 B std140 / 256 B device-padded 全 zero memcpy、per-program cadence 一括対象の場合)
  - per-program cadence 経路 `vkCmdBindDescriptorSets` 通電 (= Phase 1.C PC-7δ 完了、shader consume 未開始の可能性)

---

## §8. 本実装化に必要な作業

1. **shell 4 member → 実 sky data に置換**:
   - `hdri_split_screen` = HDRI debug split-screen toggle (= LLSettingsSky 由来、verify 要)
   - `moisture_level` / `droplet_radius` / `ice_level` = WindLight cloud microphysics params (= LLSettingsSky / LLEnvironment 由来、verify 要)
2. **dirty 判定 logic 追加**:
   - dirty 判定 trigger = WindLight settings 切替 (= LLEnvironment::onSettingsChanged 等)
3. **flush logic 追加**:
   - per-program cadence ゆえ `flushPerProgramUbos` 等価経路使用 (= 既経路活用、roadmap §5.1 cadence 別 update site)
4. **shader 接続検証**:
   - 既存 `class1/deferred/skyF.glsl` `#ifdef LL_VULKAN_GLSL` block (line 117 周辺) で UBO member access 動作確認
5. **codegen 再実行不要**:
   - blueprint member は実 shader literal extract と一致、Phase 2 で member 不変契約 → codegen 不要

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 (= set 3 内 binding=2、Skin_GLTFJoints と同 binding だが別 program で運用) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16 B → 256 B padded (codegen 出力で生成、4 × float = 16 B) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint = 実 shader literal extract (= byte-for-byte 不可触 charter §3 #1 担保) |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 (= Phase 2 cold launch 時) |

**binding=2 共有 risk**:
- Skin_GLTFJoints (set=3 binding=2 PerSkin) と同 binding 占有
- per-program cadence ゆえ skyF program bind 時に descriptor set 3 binding=2 を本 UBO に切替 (= verify 要)
- skyF program は GLTF skin を必要としないため衝突なし (= 推定、verify 要)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- Asset_GLTFNodes (set=3 binding=0)
- Asset_GLTFMaterials (set=3 binding=1)
- Skin_GLTFJoints (set=3 binding=2、本 UBO と同 binding、別 program で運用)
- **SkyFParamUBO_Legacy (set=3 binding=2、本 UBO)**

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram、80 件 = Phase 2.L0 sub-session 3 step 1 grep 確定、`llglslshader.cpp:95` source comment 「88 件最大」は historical literal)

- PerProgram cluster = ubo_metadata.inl 全 94 UBO 中 **80 件** (= Phase 2.L0 sub-session 3 step 1 grep 確定、`llglslshader.cpp:95` source comment 「88 件最大」は historical literal)
- 本 UBO 含む PerProgram cluster 全 **80 件**は `flushPerProgramUbos` 等価経路で一括 flush

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- `class1/deferred/skyF.glsl` 内同時 consume UBO (= verify 要):
  - 推定候補: FrameViewProj (set=0 binding=0) + FrameAtmosphere_Lighting (set=0 binding=2) + FrameLights (set=0 binding=1) + AtmoExtraUBO_Legacy (set=3 binding=0) + SkyVParamUBO_Legacy (set=3 binding=1)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **SkyVParamUBO_Legacy** (= set=3 binding=1、cadence_tag=1 PerProgram、`ubo_metadata.inl:107`) = sky V param UBO、本 UBO の F 対応 V UBO
- **CloudsFParamUBO_Legacy** / **CloudsVParamUBO_Legacy** = clouds 系 UBO、moisture/droplet/ice 共通 data source 可能性 (= verify 要)
- **AtmoExtraUBO_Legacy** = 大気拡張 UBO、sky/cloud 系と data source 連動可能性 (= verify 要)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **不明 / verify 要** = WindLight settings 切替時に同時 dirty になる UBO 確認要
- 推定候補: SkyVParamUBO_Legacy / CloudsFParamUBO_Legacy / CloudsVParamUBO_Legacy / AtmoExtraUBO_Legacy

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout (= Phase 1.A 確立、全 UBO 共通)

### §11.7 bind 順序関係

- per-program cadence ゆえ skyF program bind 時に descriptor set 3 更新 (= verify 要)

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

本 file 起案時点で実コード調査で確定できなかった項目:

1. **shell 通電有無** = bringupTestUBO 経路で per-program cadence 一括対象に含まれているか (= handoff doc chain `handoff/phase1/c/` 参照要)
2. **実 sky/cloud data 構造** = LLSettingsSky / WindLight 内 hdri_split_screen / moisture_level / droplet_radius / ice_level state 構造 (= grep verify 要、`indra/newview/llsettingssky.h` 等)
3. **既存 OpenGL setter call site** = 既存経路で 4 member uniform を書込む call site (= grep verify 要)
4. **binding=2 program 別運用詳細** = Skin_GLTFJoints と同 binding 占有、program bind 時 descriptor set 内容差替経路 verify 要
5. **dirty 判定 trigger** = WindLight settings 切替 event hook 具体実装 (= verify 要)
6. **同 shader consume UBO 完全特定** = class1/deferred/skyF.glsl 内同時 consume UBO 群 (= grep verify 要)
7. **同 data source UBO 連動範囲** = sky/cloud/atmo 系 UBO の data source 共有関係 (= verify 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消、確定後に「不明」記載削除 + 確定 literal 追記。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.7 同期)

**Layer**: L4-7 sub-cluster (c) (= Sky V/F pair 2 UBO、binding=2 衝突)
**status**: **起案済** (= 2026-06-06 C-6-b、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.7` (= single source of truth)
**要点**: 4 member (hdri_split_screen / moisture_level / droplet_radius / ice_level、全 float)、sky preset 切替 trigger で 4 member 同時 dirty、WindLight cloud microphysics + HDRI debug split-screen toggle、4 member setter 全件不明 [要追加調査]、binding=2 衝突 = Skin_GLTFJoints (PerSkin) (= skyF program は GLTF skin 不要、衝突なし推定) [要 verify L0-1 dispatch]、cadence PerProgram 維持 (= sky preset 切替 trigger ゆえ frame 内 stable)、工数 group 全体 L 内
**関連**: L0-1 dispatch (= binding=2 衝突 SkyF ↔ Skin_GLTFJoints) / §3.5.7 sub-cluster (c) SkyV (= V/F pair 同 sky preset 連動) / §3.5.7 sub-cluster (a) FrameAtmosphere_Lighting (= 同 LLSettingsSky owner) / sub-cluster (b) AtmoExtra (= 同 sky preset 連動 9 member)


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

