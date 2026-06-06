# SnapshotFrameFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.A PA-8 blueprint 起案済、Phase 1.C PC-2/PC-7δ per-program cadence 一括通電対象に含まれている可能性大 / verify 要)

**本実装化に必要な作業**: shell 3 member (= frame_rect / border_color / border_thickness) を実 snapshot frame border data (= LLSnapshotFloater / snapshot UI 経由、verify 要) に置換 + dirty 判定 logic 追加 + 実 shader (= class1/post/snapshotFrameF.glsl) consume 接続検証

---

## §1. UBO identity

- **block_name**: `SnapshotFrameFParamUBO_Legacy`
- **block_hash**: `0x66d91607u`
- **block_size**: 256 B (= std140 32 B、device-padded 256 B)
- **member_count**: 3
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_snapshotframefparamubo_legacy.inl
struct SnapshotFrameFParamUBO_LegacyLayout {
    static constexpr std::uint32_t frame_rect_OFFSET = 0u;  // size=16 align=16
    static constexpr std::uint32_t border_color_OFFSET = 16u;  // size=12 align=16
    static constexpr std::uint32_t border_thickness_OFFSET = 28u;  // size=4 align=4
};
inline constexpr std::uint32_t SnapshotFrameFParamUBO_Legacy_SIZE = 256u; // std140=32, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/snapshot_frame_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 38) uniform SnapshotFrameFParamUBO_Legacy
{
    vec4  frame_rect;
    vec3  border_color;
    float border_thickness;
};
```

= **blueprint literal extract from `class1/post/snapshotFrameF.glsl:33` `#ifdef LL_VULKAN_GLSL` block** (= blueprint header line 3 明示)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 38
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 内訳**: 本 UBO binding=38 = Legacy 帯独立 binding (= Asset/Skin 帯 binding=0..2 とは衝突なし)
- **source**: `ubo_metadata.inl:108` `{ "SnapshotFrameFParamUBO_Legacy", 0x66d91607u, 256u, 3u, 38u, 0u, 1u, 3u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` `kCadencePerProgram = 1u`)
- **source**: ubo_metadata.inl:108 + llglslshader.cpp:95

---

## §4. 物理 owner

- **shell 段階 owner**: 未確認
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `frame_rect` = snapshot UI frame rectangle (x, y, w, h、vec4、LLSnapshotFloater 経由)
  - `border_color` = snapshot UI border color (vec3 RGB)
  - `border_thickness` = snapshot UI border thickness (float pixels)
- **lifetime**: per-program (= snapshot post-processing shader instance)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/snapshot_frame_f_param_ubo_legacy.glsl` (= Phase 1.A PA-8 起案、Source `class1/post/snapshotFrameF.glsl:33`)
- **実 shader use site**:
  - `class1/post/snapshotFrameF.glsl` (= blueprint header line 3 literal reference)
- **consume status**: blueprint literal extract source ゆえ既 consume

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter** (= verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL setter = `uniform4fv("frame_rect", ...)` / `uniform3fv("border_color", ...)` / `uniform1f("border_thickness", ...)` 経路 (= verify 要)
  - data source = snapshot UI floater (= `llfloatersnapshot.cpp` / `llsnapshotlivepreview.cpp` 等、verify 要)

---

## §7. 現状通電状態

- **状態**: **shell 通電有無 verify 要**
- **通電内容** (= 推定): zero dummy buffer write + per-program cadence 経路通電

---

## §8. 本実装化に必要な作業

1. **shell 3 member → 実 snapshot frame data 接続**:
   - snapshot UI floater 経由 frame_rect / border_color / border_thickness 集約
2. **dirty 判定 logic 追加**:
   - dirty 判定 trigger = snapshot UI 切替 / frame サイズ変更 / border 設定変更
3. **flush logic 追加**:
   - per-program cadence ゆえ既経路活用
4. **shader 接続検証**:
   - 既存 `class1/post/snapshotFrameF.glsl` `#ifdef LL_VULKAN_GLSL` block (line 33 周辺) で UBO member access 動作確認

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 (binding=38 独立) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 32 B → 256 B padded (vec4 + vec3+float pack = 32 B) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint = 実 shader literal extract |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**snapshot 用途特化**:
- snapshot 撮影時のみ shader bind = 通常 frame では dispatcher 経由 skip
- per-program cadence 適切 (= snapshot shader instance 単位で更新)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- Asset/Skin 帯 + Legacy 帯各種、本 UBO binding=38 は独立

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- PerProgram cluster 全 **80 件** (= Phase 2.L0 sub-session 3 step 1 grep 確定、`llglslshader.cpp:95` source comment 「88 件最大」は historical literal)

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- `class1/post/snapshotFrameF.glsl` 内同時 consume UBO (= verify 要):
  - 推定候補: post-processing pipeline 経路 UBO (= TonemapUBO_Legacy / GlowCombineFParamUBO_Legacy / DofCombineFParamUBO_Legacy 等の post-pass UBO)

### §11.4 同 data source UBO (= 同 host data source から派生)

- snapshot UI 系 UBO は本 UBO のみ (= ubo_metadata.inl 内 "Snapshot" 命名持つ UBO は本 UBO 単独)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **不明 / verify 要** = snapshot UI 切替時の連動 UBO 確認要

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- per-program cadence ゆえ snapshotFrameF program bind 時に descriptor set 3 更新

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電有無** = handoff doc chain 参照要
2. **frame_rect data source** = LLSnapshotFloater / llsnapshotlivepreview.cpp 経路 (= verify 要)
3. **既存 OpenGL setter call site** = uniform4fv("frame_rect", ...) 等 grep verify 要
4. **snapshot UI 起動 timing** = 通常 frame での dispatcher skip 動作 verify 要
5. **dirty 判定 trigger** = snapshot UI 切替 event hook 具体実装 (= verify 要)
6. **同 shader consume UBO 完全特定** = class1/post/snapshotFrameF.glsl 内同時 consume UBO 群 (= grep verify 要)

= 上記 6 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.4.5 同期)

**Layer**: L3-5 (= B Tier β setter 推定済、PerProgram cadence、snapshot UI border)
**status**: **起案済** (= 2026-06-06 C-5、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.4.5` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch + L0-4 cadence
- **(2) 不明事項**: snapshot UI floater (= `LLSnapshotFloater` / `llsnapshotlivepreview.cpp`) 内 setter call site [要追加調査] / snapshot UI 起動 timing [要 verify]
- **(3) 調査手法**: D1 (`frame_rect` / `border_color` / `border_thickness` setter grep) + D2 (snapshot UI floater 構造)
- **(4) 設計 task**: L3 全件共通 (= PerProgram triple-buffer / `forwardToUboUpload` PER_PROGRAM / program bind 単位 flush / `snapshotFrameF.glsl` LL_VULKAN_GLSL 活性化)
- **(5) 工程**: trace L3-5、工数 **S** (= UI 起動 timing 確認含む)、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= snapshot UI 起動時 border 描画既存と同一、AYA r30 撮影描画章関連、visual regression ゼロ §5.4)
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `snapshotFrameF.glsl #else` block uniform 個別宣言維持

**関連**: L0-1 + L0-4 / §5.4 / AYA r30 撮影描画章 (= memory `project_ayastorm_r30_cinematic_chapter`)


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

