# VelocityVParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.A PA-8 blueprint 起案済、Phase 1.C PC-2/PC-7δ per-program cadence 一括通電対象に含まれている可能性大 / verify 要)

**本実装化に必要な作業**: shell 1 member (= last_object_matrix mat4) を実 motion blur velocity matrix (= LLDrawable previous frame matrix 経由、verify 要) に置換 + dirty 判定 logic 追加 + 実 shader (= class1/deferred/velocityV.glsl) consume 接続検証

---

## §1. UBO identity

- **block_name**: `VelocityVParamUBO_Legacy`
- **block_hash**: `0x1f44215bu`
- **block_size**: 256 B (= std140 64 B、device-padded 256 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_velocityvparamubo_legacy.inl
struct VelocityVParamUBO_LegacyLayout {
    static constexpr std::uint32_t last_object_matrix_OFFSET = 0u;  // size=64 align=16
};
inline constexpr std::uint32_t VelocityVParamUBO_Legacy_SIZE = 256u; // std140=64, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/velocity_v_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 55) uniform VelocityVParamUBO_Legacy
{
    mat4 last_object_matrix;
};
```

= **blueprint literal extract from `class1/deferred/velocityV.glsl:54` `#ifdef LL_VULKAN_GLSL` block** (= blueprint header line 3 明示)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 55
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 内訳**: 本 UBO binding=55 = Legacy 帯独立 binding
- **source**: `ubo_metadata.inl:116` `{ "VelocityVParamUBO_Legacy", 0x1f44215bu, 256u, 3u, 55u, 0u, 1u, 1u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` `kCadencePerProgram = 1u`)
- **意味注記**: `last_object_matrix` は per-object per-frame で変化 (= motion blur previous frame matrix)、per-program cadence では同 program 内で複数 object 間 stale data risk
- **本来 cadence 候補**: per-draw 系 (= PerDrawUBO_AvatarVelocity と同形パターン) への移動候補、Phase 2 で再評価
- **source**: ubo_metadata.inl:116 + llglslshader.cpp:95

---

## §4. 物理 owner

- **shell 段階 owner**: 未確認
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `last_object_matrix` = previous frame object modelview matrix (mat4、LLDrawable / LLViewerObject 経由 previous frame matrix tracking、verify 要)
- **lifetime**: per-program (= velocity rendering shader instance、本来 per-draw 系の可能性大)
- **同名 member**: `class1/deferred/velocityAlphaV.glsl` (= grep `last_object_matrix` 結果)、velocity alpha V shader でも同名 member、別 UBO の可能性 (= PerProgramUBO_VelocityAlphaV、verify 要)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/velocity_v_param_ubo_legacy.glsl` (= Phase 1.A PA-8 起案、Source `class1/deferred/velocityV.glsl:54`)
- **実 shader use site**:
  - `class1/deferred/velocityV.glsl` (= blueprint header line 3 literal reference)
- **同名 member 別 shader**:
  - `class1/deferred/velocityAlphaV.glsl` (= velocity alpha V shader、別 UBO `PerProgramUBO_VelocityAlphaV` (= set=2 binding=19、PerProgram、1 member) の可能性、verify 要)
- **consume status**: blueprint literal extract source ゆえ既 consume

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter** (= verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL setter = `uniformMatrix4fv("last_object_matrix", ...)` (= verify 要)
  - data source = LLDrawable / LLViewerObject 内 previous frame matrix tracking + motion blur pass

---

## §7. 現状通電状態

- **状態**: **shell 通電有無 verify 要**
- **通電内容** (= 推定): zero dummy buffer write + per-program cadence 経路通電

---

## §8. 本実装化に必要な作業

1. **shell `last_object_matrix` → 実 motion blur velocity matrix 接続**:
   - LLDrawable / LLViewerObject 内 previous frame matrix tracking
   - 各 object 毎 previous matrix 保持 + 現 frame matrix と差分計算で velocity 算出
2. **dirty 判定 logic 追加**:
   - dirty 判定 trigger = per-draw object 切替 (= per-program cadence は不適切、per-draw 系へ移動推奨)
3. **flush logic 追加**:
   - 現 per-program cadence → 推奨 per-draw cadence、cadence 再設計要
4. **shader 接続検証**:
   - 既存 `class1/deferred/velocityV.glsl` `#ifdef LL_VULKAN_GLSL` block (line 54 周辺) で UBO member access 動作確認

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 (binding=55 独立) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 64 B → 256 B padded (mat4 = 4 × vec4 = 64 B std140) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint = 実 shader literal extract |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**cadence 再評価必須**:
- `last_object_matrix` は per-object per-frame で変化、現 per-program cadence では正常動作不可
- per-draw 系 (= PerDrawUBO_AvatarVelocity と同形 cadence) への移動が本実装化必須条件
- Phase 2 で cadence 再設計 (= ubo_metadata.inl cadence_tag=1 → cadence_tag=2 PerDraw 変更 + member 配置検討)

**同形 UBO 候補比較**:
- **PerDrawUBO_AvatarVelocity** (set=2 binding=0、PerDraw、ubo_metadata.inl:65) = avatar velocity UBO、本 UBO と同 motion blur 用途、cadence 適正な参考実装
- **PerDrawUBO_SkinnedVelocity** (set=2 binding=0、PerDraw、ubo_metadata.inl:70) = skinned velocity UBO、本 UBO の skinned 対応版
- **PerProgramUBO_VelocityAlphaV** (set=2 binding=19、PerProgram、1 member、ubo_metadata.inl:90) = velocity alpha V UBO、本 UBO と同形 cadence (PerProgram) ゆえ同じ cadence 再評価対象

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- Asset/Skin 帯 + Legacy 帯各種、本 UBO binding=55 は独立

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- PerProgram cluster 全 88 件

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- `class1/deferred/velocityV.glsl` 内同時 consume UBO (= verify 要):
  - 推定候補: FrameViewProj (= view+proj matrix) + PerDrawUBO_AvatarVelocity / PerDrawUBO_SkinnedVelocity (= velocity 系 per-draw UBO)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **PerDrawUBO_AvatarVelocity** (= set=2 binding=0、PerDraw、ubo_metadata.inl:65) = avatar velocity UBO、本 UBO と data source 共有可能性大 (= LLDrawable previous frame matrix tracking 共通)
- **PerDrawUBO_SkinnedVelocity** (= set=2 binding=0、PerDraw、ubo_metadata.inl:70) = skinned velocity UBO、本 UBO の skinned 対応版
- **PerProgramUBO_VelocityAlphaV** (= set=2 binding=19、PerProgram、ubo_metadata.inl:90) = velocity alpha V UBO、velocityAlphaV shader で同名 member `last_object_matrix` の可能性 (= verify 要)
- **MotionBlurFParamUBO_Legacy** (= set=3 binding=27、PerProgram、ubo_metadata.inl:55) = motion blur F UBO、velocity 計算後段 post-pass

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **PerDrawUBO_AvatarVelocity** / **PerDrawUBO_SkinnedVelocity** (= velocity 系 per-draw UBO 同時 dirty 高確率)
- **MotionBlurFParamUBO_Legacy** (= motion blur post-pass 連動)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- 現 per-program cadence ゆえ velocityV program bind 時に descriptor set 3 更新
- 推奨 per-draw cadence への変更後は per-draw timing で descriptor set 2 更新

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電有無** = handoff doc chain 参照要
2. **last_object_matrix data source** = LLDrawable / LLViewerObject 内 previous frame matrix tracking 経路 (= verify 要)
3. **既存 OpenGL setter call site** = uniformMatrix4fv("last_object_matrix", ...) grep verify 要
4. **cadence 再評価必須性** = per-object per-frame 変化 vs per-program cadence の正常動作可否 (= verify 要、現実装で正常動作確認要)
5. **PerDrawUBO_AvatarVelocity / PerDrawUBO_SkinnedVelocity との data source 共有経路** = LLDrawable previous frame matrix tracking 共通経路 (= verify 要)
6. **velocityAlphaV shader 同名 member 帰属** = PerProgramUBO_VelocityAlphaV か本 UBO か (= verify 要、別 shader/別 UBO 確定要)
7. **MotionBlurFParamUBO_Legacy 連動経路** = motion blur post-pass で本 UBO velocity 結果 consume 経路 (= verify 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.8 同期)

**Layer**: L4-8 sub-cluster (c) (= per-program velocity matrix pair 2 UBO、cadence mismatch 重大)
**status**: **起案済** (= 2026-06-06 C-6-c、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.8` (= single source of truth)
**要点**: 1 member (last_object_matrix mat4)、**cadence mismatch 重大** (= per-object per-frame 変化を PerProgram で運ぶ stale data、N object 描画で最後の 1 値のみ反映 = motion blur 退化)、**PerProgram → PerDraw 降格必須** (= ubo_metadata.inl cadence_tag=1 → cadence_tag=2 + set/binding 再配置) [要 AYA 判断 必須] [要 L0-4 結果反映]、setter 不明 (= `uniformMatrix4fv("last_object_matrix", ...)` grep verify 要) [要追加調査]、VelocityAlphaV (set=2 binding=19) と同 data 別 UBO duplicate [要 verify D4 突合]、velocityV.glsl singleton site、工数 group 全体 L 内
**関連**: L0-1 dispatch (= 衝突なし binding=55) / L0-4 cadence (= PerDraw 降格必須) / §3.5.8 sub-cluster (c) VelocityAlphaV (= 同 data 別 UBO duplicate verify) / §3.3.3 group MotionBlurFParamUBO_Legacy (= motion blur post-pass 連動)
