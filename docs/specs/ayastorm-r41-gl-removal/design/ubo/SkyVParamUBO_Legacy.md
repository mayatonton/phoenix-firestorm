# SkyVParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.A PA-8 blueprint 起案済、Phase 1.C PC-2/PC-7δ per-program cadence 一括通電対象に含まれている可能性大 / verify 要)

**本実装化に必要な作業**: shell 2 member (= camPosLocal + _pad_sky_v_legacy_0) を実 sky V data (= camera position local space、LLViewerCamera 経由、verify 要) に置換 + dirty 判定 logic 追加 + 実 shader (= class1/deferred/skyV.glsl) consume 接続検証

---

## §1. UBO identity

- **block_name**: `SkyVParamUBO_Legacy`
- **block_hash**: `0xf4ef024fu` (= FNV-1a("SkyVParamUBO_Legacy"))
- **block_size**: 256 B (= std140 32 B、device-padded 256 B)
- **member_count**: 2
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_skyvparamubo_legacy.inl
struct SkyVParamUBO_LegacyLayout {
    static constexpr std::uint32_t camPosLocal_OFFSET = 0u;  // size=12 align=16
    static constexpr std::uint32_t _pad_sky_v_legacy_0_OFFSET = 16u;  // size=12 align=16
};
inline constexpr std::uint32_t SkyVParamUBO_Legacy_SIZE = 256u; // std140=32, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/sky_v_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 1) uniform SkyVParamUBO_Legacy
{
    vec3 camPosLocal;
    vec3 _pad_sky_v_legacy_0;
};
```

= **blueprint literal extract from `class1/deferred/skyV.glsl:83` `#ifdef LL_VULKAN_GLSL` block** (= blueprint header line 3 明示)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 1
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: `sAYAStandardLayout` (= Phase 1.A 確立 5-set layout)
- **set 3 内訳**: set=3 は Asset (binding=0/1) + Skin (binding=2) + Legacy UBO 群 (binding=2..62)
  - **本 UBO binding=1 は Asset_GLTFMaterials と同 binding** = 別 program ゆえ descriptor set 内容差替で運用 (= verify 要)
- **source**: `ubo_metadata.inl:107` `{ "SkyVParamUBO_Legacy", 0xf4ef024fu, 256u, 3u, 1u, 0u, 1u, 2u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` `constexpr U32 kCadencePerProgram = 1u`)
- **意味詳細**: per-program cadence、program bind 時 UBO 更新
- **source**: ubo_metadata.inl:107 + llglslshader.cpp:95

---

## §4. 物理 owner

- **shell 段階 owner**: 未確認 (= verify 要)
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `camPosLocal` = camera position local space (= LLViewerCamera::getOrigin() local 等、verify 要)
- **lifetime**: per-program

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/sky_v_param_ubo_legacy.glsl` (= Phase 1.A PA-8 起案、Source literal extract from `class1/deferred/skyV.glsl:83`)
- **実 shader use site**:
  - `class1/deferred/skyV.glsl` (= blueprint header line 3 literal reference)
- **同名 member 共有 shader** (= grep `camPosLocal` 結果 7 件):
  - `class3/deferred/materialF.glsl` / `class1/deferred/cloudsF.glsl` / `class1/deferred/cloudsV.glsl` / `aya_r41_blueprints/set1/material_ubo_legacy.glsl` / `aya_r41_blueprints/set3/clouds_v_param_ubo_legacy.glsl` (= camPosLocal 同名 member 持つ別 UBO 候補、verify 要)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter** (= verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL 経路 uniform setter (= `uniform3fv("camPosLocal", ...)`) call site grep verify 要
  - data source = LLViewerCamera 経由 camera position local 計算

---

## §7. 現状通電状態

- **状態**: **shell 通電有無 verify 要** (= per-program cadence 一括通電対象の可能性大)
- **通電 commit**: 不明
- **通電内容** (= 推定): zero dummy buffer write + per-program cadence `vkCmdBindDescriptorSets` 通電

---

## §8. 本実装化に必要な作業

1. **shell `camPosLocal` → 実 camera position local 接続**:
   - 既存 OpenGL setter (= `LLViewerCamera::getOrigin()` 経由 local space 変換、verify 要) と同経路で接続
2. **dirty 判定 logic 追加**:
   - dirty 判定 trigger = camera move event (= per-frame dirty の可能性、cadence 再評価要)
3. **flush logic 追加**:
   - per-program cadence ゆえ `flushPerProgramUbos` 等価経路使用
4. **shader 接続検証**:
   - 既存 `class1/deferred/skyV.glsl` `#ifdef LL_VULKAN_GLSL` block (line 83 周辺) で UBO member access 動作確認

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 32 B → 256 B padded (vec3+pad vec3 = 32 B std140) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint = 実 shader literal extract |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**cadence 再評価候補**:
- `camPosLocal` は per-frame で変化 (= camera move 毎)、per-program cadence では同 program 内 frame 間 stale data 表示 risk
- 本格 cadence は per-frame 系 (= FrameViewProj 帯) への移動候補、現 per-program は Phase 1 初期段階 shell 設計、Phase 2 で再評価

**`_pad_sky_v_legacy_0` 命名**:
- pad member だが命名上 `_pad_sky_v_legacy_0` = std140 vec3 直後 alignment 要件のため挿入された padding
- 実 shader 側は unused、UBO 書込み時は zero 充填 OK

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- Asset_GLTFNodes (set=3 binding=0)
- Asset_GLTFMaterials (set=3 binding=1、本 UBO と同 binding、別 program で運用)
- **SkyVParamUBO_Legacy (set=3 binding=1、本 UBO)**
- Skin_GLTFJoints (set=3 binding=2)
- + Legacy UBO 群

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- PerProgram cluster 全 **80 件** (= Phase 2.L0 sub-session 3 step 1 grep 確定、`llglslshader.cpp:95` source comment 「88 件最大」は historical literal)、本 UBO 含む

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- `class1/deferred/skyV.glsl` 内同時 consume UBO (= verify 要):
  - 推定候補: FrameViewProj (set=0 binding=0、view+proj matrix) + SkyFParamUBO_Legacy (sky V/F pair)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **SkyFParamUBO_Legacy** (= set=3 binding=2、PerProgram) = sky V/F pair
- **CloudsVParamUBO_Legacy** (= set=3 binding=3、PerProgram、camPosLocal 同名 member grep 結果) = clouds V UBO、camera position local 共有可能性 (= verify 要)
- **MaterialUBO_Legacy** (= set=1 binding=0、PerProgram、camPosLocal 同名 member grep 結果) = material UBO、camera position 共有可能性 (= verify 要)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **不明 / verify 要** = camera move 時に同時 dirty になる UBO 確認要 (= FrameViewProj 等 per-frame UBO と同期可能性)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- per-program cadence ゆえ skyV program bind 時に descriptor set 3 更新

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電有無** = handoff doc chain 参照要
2. **camPosLocal data source** = LLViewerCamera 経由 local space 変換経路 (= verify 要)
3. **既存 OpenGL setter call site** = uniform3fv("camPosLocal", ...) grep verify 要
4. **cadence 適正性** = per-frame で変化する camera position が per-program cadence で正常動作するか (= verify 要、stale risk)
5. **binding=1 program 別運用詳細** = Asset_GLTFMaterials と同 binding 占有、program bind 時 descriptor set 内容差替経路 verify 要
6. **同名 member 別 UBO 連動** = camPosLocal を持つ他 5 shader/blueprint との data source 関係 (= verify 要)
7. **`_pad_sky_v_legacy_0` 元 GLSL** = blueprint extract source の `class1/deferred/skyV.glsl:83` `#ifdef LL_VULKAN_GLSL` block 内 padding 由来 (= 元 shader Read verify 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.7 同期)

**Layer**: L4-7 sub-cluster (c) (= Sky V/F pair 2 UBO、binding=1 衝突)
**status**: **起案済** (= 2026-06-06 C-6-b、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.7` (= single source of truth)
**要点**: 2 member (camPosLocal vec3 + pad vec3)、camera move trigger で camPosLocal 5 use site (= 本 UBO + CloudsV + CloudsF (複製) + MaterialUBO_Legacy + materialF.glsl) cross dirty、cadence mismatch 重大 (= per-frame 変化を PerProgram で運ぶ stale risk) → PerFrame 降格候補 [要 L0-4 結果反映 / 要 AYA 判断]、binding=1 衝突 = Asset_GLTFMaterials (PerAsset) [要 verify L0-1 dispatch]、setter 不明 [要追加調査]、工数 group 全体 L 内
**関連**: L0-1 dispatch (= binding=1 衝突 SkyV ↔ Asset_GLTFMaterials) / L0-4 cadence (= PerFrame 降格候補) / §3.5.7 sub-cluster (c) SkyF (= V/F pair 同 sky preset 連動) / sub-cluster (d) CloudsV/CloudsF (= camPosLocal cross UBO 同 data source 5 use site)


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

