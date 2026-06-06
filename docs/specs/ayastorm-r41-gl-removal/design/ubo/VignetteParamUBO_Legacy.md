# VignetteParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.A PA-8 blueprint 起案済、Phase 1.C PC-2/PC-7δ per-program cadence 一括通電対象に含まれている可能性大 / verify 要)

**本実装化に必要な作業**: shell 2 member (= vignette + _pad_vignette_legacy_0) を実 vignette post-pass data (= AYAstorm r14+ exoVignette / debug settings 経由、verify 要) に置換 + dirty 判定 logic 追加 + 実 shader (= class1/post/exoVignetteF.glsl) consume 接続検証

---

## §1. UBO identity

- **block_name**: `VignetteParamUBO_Legacy`
- **block_hash**: `0x3ad94ffeu`
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 2
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_vignetteparamubo_legacy.inl
struct VignetteParamUBO_LegacyLayout {
    static constexpr std::uint32_t vignette_OFFSET = 0u;  // size=12 align=16
    static constexpr std::uint32_t _pad_vignette_legacy_0_OFFSET = 12u;  // size=4 align=4
};
inline constexpr std::uint32_t VignetteParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/vignette_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 46) uniform VignetteParamUBO_Legacy
{
    vec3  vignette;
    float _pad_vignette_legacy_0;
};
```

= **blueprint literal extract from `class1/post/exoVignetteF.glsl:42` `#ifdef LL_VULKAN_GLSL` block** (= blueprint header line 3 明示)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 46
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 内訳**: 本 UBO binding=46 = Legacy 帯独立 binding
- **source**: `ubo_metadata.inl:117` `{ "VignetteParamUBO_Legacy", 0x3ad94ffeu, 256u, 3u, 46u, 0u, 1u, 2u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` `kCadencePerProgram = 1u`)
- **source**: ubo_metadata.inl:117 + llglslshader.cpp:95

---

## §4. 物理 owner

- **shell 段階 owner**: 未確認
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `vignette` = vignette params (vec3、3 component の内訳不明、verify 要、推定: intensity / radius / softness 等)
  - `_pad_vignette_legacy_0` = std140 vec3 + 4 B alignment padding (= unused)
- **lifetime**: per-program (= vignette post-pass shader instance)
- **exoVignette 由来**: shader 名 `exoVignetteF.glsl` の `exo` prefix は Exodus / Black Dragon 等の derivative branch 経由実装の可能性 (= verify 要、AYAstorm r14+ 視覚表現章で borrow した可能性)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/vignette_param_ubo_legacy.glsl` (= Phase 1.A PA-8 起案、Source `class1/post/exoVignetteF.glsl:42`)
- **実 shader use site**:
  - `class1/post/exoVignetteF.glsl` (= blueprint header line 3 literal reference、grep `uniform vec3 vignette` 1 件確認)
- **consume status**: blueprint literal extract source ゆえ既 consume

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter** (= verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL setter = `uniform3fv("vignette", ...)` (= verify 要)
  - data source = debug settings (= AYAstorm r14+ Vignette 関連 cvar / Cinematic mode、verify 要)

---

## §7. 現状通電状態

- **状態**: **shell 通電有無 verify 要**
- **通電内容** (= 推定): zero dummy buffer write + per-program cadence 経路通電

---

## §8. 本実装化に必要な作業

1. **shell `vignette` → 実 vignette data 接続**:
   - vec3 3 component 内訳特定 (= shader 内 access pattern 確認要)
   - debug settings / Cinematic mode 経由 host setter 接続
2. **dirty 判定 logic 追加**:
   - dirty 判定 trigger = debug settings 変更 / Cinematic mode 切替
3. **flush logic 追加**:
   - per-program cadence ゆえ既経路活用
4. **shader 接続検証**:
   - 既存 `class1/post/exoVignetteF.glsl` `#ifdef LL_VULKAN_GLSL` block (line 42 周辺) で UBO member access 動作確認

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 (binding=46 独立) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16 B → 256 B padded (vec3 + 4 B pad = 16 B std140) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint = 実 shader literal extract |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**exoVignette 由来の upstream 取り込み risk**:
- shader 名 `exo` prefix は Exodus / Black Dragon derivative 由来の可能性
- 設計原則 (1) Upstream OpenGL 取り込みやすさ維持 (= memory `project_ayastorm_r41_design_principles`) との緊張
- 上流の vignette 実装と Vulkan path 整合性 verify 要

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- Asset/Skin 帯 + Legacy 帯各種、本 UBO binding=46 は独立

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- PerProgram cluster 全 **80 件** (= Phase 2.L0 sub-session 3 step 1 grep 確定、`llglslshader.cpp:95` source comment 「88 件最大」は historical literal)

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- `class1/post/exoVignetteF.glsl` 内同時 consume UBO (= verify 要):
  - 推定候補: post-processing pipeline 経路 UBO (= TonemapUBO_Legacy / PerProgramUBO_PostDeferredF / PerProgramUBO_ColorGrading 等)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **TonemapUBO_Legacy** (= set=3 binding=10、PerProgram、tonemap params) = post-pass 連動可能性
- **PerProgramUBO_ColorGrading** (= set=2 binding=4、PerProgram、color grading params) = AYAstorm r14+ 視覚表現章 post-pass 連動可能性

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **不明 / verify 要** = Cinematic mode 切替時の連動 UBO 確認要

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- per-program cadence ゆえ exoVignetteF program bind 時に descriptor set 3 更新

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電有無** = handoff doc chain 参照要
2. **vignette vec3 3 component 内訳** = shader 内 access pattern (= intensity / radius / softness 等の具体内訳、verify 要)
3. **vignette data source** = debug settings / Cinematic mode 経由 host setter 経路 (= verify 要)
4. **既存 OpenGL setter call site** = uniform3fv("vignette", ...) grep verify 要
5. **exoVignette 由来 upstream** = Exodus / Black Dragon derivative 由来か AYAstorm 独自か (= verify 要)
6. **dirty 判定 trigger** = Cinematic mode 切替 event hook 具体実装 (= verify 要)
7. **同 shader consume UBO 完全特定** = class1/post/exoVignetteF.glsl 内同時 consume UBO 群 (= grep verify 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.10 同期)

**Layer**: L4-10 sub-cluster (d) (= Vignette 1 UBO、exoVignette Exodus/BD derivative)
**status**: **起案済** (= 2026-06-06 C-6-e、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.10` (= single source of truth)
**要点**: 2 member (vignette vec3 + pad)、setter 不明 (= `uniform3fv("vignette", ...)` 経路想定) [要追加調査]、**vec3 3 component 内訳不明** (= intensity/radius/softness 等推定、shader 内 access pattern verify 要) [要追加調査]、AYAstorm r14+ 視覚表現章 / Cinematic mode vignette 関連 cvar、**exoVignette 由来 risk** (= shader `exo` prefix Exodus/BlackDragon derivative 可能性、設計原則 (1) Upstream 取り込みやすさ整合判定要) [要 verify]、exoVignetteF.glsl:42 singleton site、post-process chain 最終段 (= final display 直前 pass)、cadence PerProgram 維持、工数 group 全体 L 内
**関連**: L0-1 dispatch (= 衝突なし binding=46) / §3.5.10 sub-cluster (b) Tonemap (= post-process chain pre-step) / sub-cluster (c) ColorGrading/GammaCorrect (= AYAstorm 視覚機能交差) / AYAstorm r14+ 視覚表現章 (= memory `project_ayastorm_visual_realism_chapter`)
