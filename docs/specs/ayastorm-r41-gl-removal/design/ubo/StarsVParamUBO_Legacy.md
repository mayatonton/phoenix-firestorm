# StarsVParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.A PA-8 blueprint 起案済、Phase 1.C PC-2/PC-7δ per-program cadence 一括通電対象に含まれている可能性大 / verify 要)

**本実装化に必要な作業**: shell 1 member (= stars_v_time) を実 stars V time data に置換 + dirty 判定 logic 追加 + 実 shader (= class1/deferred/starsV.glsl) consume 接続検証

---

## §1. UBO identity

- **block_name**: `StarsVParamUBO_Legacy`
- **block_hash**: `0x6a17ccbfu`
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_starsvparamubo_legacy.inl
struct StarsVParamUBO_LegacyLayout {
    static constexpr std::uint32_t stars_v_time_OFFSET = 0u;  // size=4 align=4
};
inline constexpr std::uint32_t StarsVParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/stars_v_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 45) uniform StarsVParamUBO_Legacy
{
    float stars_v_time;
};
```

= **blueprint literal extract from `class1/deferred/starsV.glsl:67` `#ifdef LL_VULKAN_GLSL` block** (= blueprint header line 3 明示)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 45
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 内訳**: 本 UBO binding=45 = Legacy 帯独立 binding
- **source**: `ubo_metadata.inl:111` `{ "StarsVParamUBO_Legacy", 0x6a17ccbfu, 256u, 3u, 45u, 0u, 1u, 1u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` `kCadencePerProgram = 1u`)
- **source**: ubo_metadata.inl:111 + llglslshader.cpp:95

---

## §4. 物理 owner

- **shell 段階 owner**: 未確認
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `stars_v_time` = stars V shader time (= per-frame time accumulator、stars position animation 等、LLEnvironment 経由、verify 要)
- **lifetime**: per-program (= stars V rendering shader instance)
- **命名 `stars_v_time`**: V (vertex) shader 専用 time、F shader の `time` (= StarsFParamUBO_Legacy.time member) と命名分離 (= shader 側 nameless block member 衝突回避 rename と推定、verify 要)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/stars_v_param_ubo_legacy.glsl` (= Phase 1.A PA-8 起案、Source `class1/deferred/starsV.glsl:67`)
- **実 shader use site**:
  - `class1/deferred/starsV.glsl` (= blueprint header line 3 literal reference)
- **consume status**: blueprint literal extract source ゆえ既 consume

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter** (= verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL setter = `uniform1f("time", ...)` (= shader 側 nameless block member 命名は `stars_v_time` だが #define alias の可能性、verify 要)
  - data source = LLEnvironment day cycle time / per-frame time accumulator

---

## §7. 現状通電状態

- **状態**: **shell 通電有無 verify 要**
- **通電内容** (= 推定): zero dummy buffer write + per-program cadence 経路通電

---

## §8. 本実装化に必要な作業

1. **shell `stars_v_time` → 実 stars V time data 接続**:
   - per-frame time accumulator 接続
2. **dirty 判定 logic 追加**:
   - dirty 判定 trigger = per-frame time update
3. **flush logic 追加**:
   - per-program cadence ゆえ既経路活用
4. **shader 接続検証**:
   - 既存 `class1/deferred/starsV.glsl` `#ifdef LL_VULKAN_GLSL` block (line 67 周辺) で UBO member access 動作確認

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 (binding=45 独立) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16 B → 256 B padded (1 × float = 4 B + 12 B padding = 16 B std140) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint = 実 shader literal extract |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**cadence 再評価候補**:
- `stars_v_time` は per-frame で変化、per-program cadence では stale data risk
- per-frame 系への移動候補、Phase 2 で再評価

**命名 rename 状態 verify**:
- `stars_v_time` 命名は F shader `time` (= StarsFParamUBO_Legacy.time member) と分離
- shader 側 nameless block member 衝突回避の rename pattern と推定 (= memory `feedback_handoff_minimal_pre_req_read` 参照、underWaterF/waterFog で対処済 pattern と同形、verify 要)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- Asset/Skin 帯 + Legacy 帯各種、本 UBO binding=45 は独立

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- PerProgram cluster 全 **80 件** (= Phase 2.L0 sub-session 3 step 1 grep 確定、`llglslshader.cpp:95` source comment 「88 件最大」は historical literal)

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- `class1/deferred/starsV.glsl` 内同時 consume UBO (= verify 要):
  - 推定候補: FrameViewProj (set=0 binding=0、view+proj matrix)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **StarsFParamUBO_Legacy** (= set=3 binding=42、PerProgram、time member) = stars V/F pair (= 同 stars rendering)
  - `stars_v_time` (本 UBO) と `time` (StarsFParamUBO_Legacy) は同 source の可能性大、shader 別 (V/F) ゆえ UBO 別宣言

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **StarsFParamUBO_Legacy** (= V/F pair、同 stars rendering 同時 dirty 高確率)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- per-program cadence ゆえ starsV program bind 時に descriptor set 3 更新

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電有無** = handoff doc chain 参照要
2. **stars_v_time data source** = LLEnvironment day cycle time / per-frame time accumulator 経路 (= verify 要)
3. **既存 OpenGL setter call site** = uniform1f("time", ...) または "stars_v_time" 名前 grep verify 要
4. **命名 rename 経緯** = shader 側 nameless block member 衝突回避の rename pattern verify 要
5. **cadence 適正性** = per-frame 変化 stars_v_time の per-program cadence stale risk (= verify 要)
6. **StarsFParamUBO_Legacy.time との data source 共有** = 同 stars rendering V/F pair の time member 共有経路 (= verify 要)

= 上記 6 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.7 同期)

**Layer**: L4-7 sub-cluster (e) (= Stars F/V + SunDisc + Moon 4 UBO)
**status**: **起案済** (= 2026-06-06 C-6-b、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.7` (= single source of truth)
**要点**: 1 member (stars_v_time、float)、StarsFParamUBO_Legacy.time の V 側 rename 版 (= shader 側 nameless block member 衝突回避、underWaterF/waterFog rename pattern と同形 [要 verify])、per-frame time accumulator (= stars position animation)、cadence mismatch (= per-frame 変化を PerProgram で運ぶ stale risk) → PerFrame 降格候補 [要 L0-4 結果反映]、setter 不明 (= OpenGL setter 名前 `time` か `stars_v_time` か [要追加調査])、StarsF.time との同 data source verify 必須 [要 verify D4 突合]、工数 group 全体 L 内
**関連**: L0-1 dispatch (= 衝突なし binding=45) / L0-4 cadence (= PerFrame 降格候補) / §3.5.7 sub-cluster (e) StarsF (= V/F pair 同 stars rendering、time/stars_v_time data source 共有 verify) / sub-cluster (a) FrameAtmosphere_Lighting (= 同 LLEnvironment day cycle 連動)


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

