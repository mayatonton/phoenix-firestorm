# PerProgramUBO_BlurLightF — UBO design (= 実コードベース調査資料)

**通電状態**: **untouched** (= host C++ で `PerProgramUBO_BlurLightF` への setter 呼出ゼロ、grep 確認済 2026-06-06)

**本実装化に必要な作業**: 実 SSAO blur 系定数 (= `delta` / `dist_factor` / `blur_size` / `kern[4]` / `kern_scale`) を host から PerProgram cadence setter 経由で書込 + class1/deferred/blurLightF.glsl 内 UBO consume へ切替

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_BlurLightF`
- **block_hash**: `0xa43668deu`
- **block_size**: 256 B (= std140=96 B, device-padded 256 B)
- **member_count**: 8 (= 実 5 + pad 3)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_blurlightf.inl:12-21
struct PerProgramUBO_BlurLightFLayout {
    static constexpr std::uint32_t delta_OFFSET = 0u;        // size=8 align=8
    static constexpr std::uint32_t dist_factor_OFFSET = 8u;  // size=4 align=4
    static constexpr std::uint32_t blur_size_OFFSET = 12u;   // size=4 align=4
    static constexpr std::uint32_t kern_OFFSET = 16u;        // size=64 align=16 stride=16
    static constexpr std::uint32_t kern_scale_OFFSET = 80u;  // size=4 align=4
    static constexpr std::uint32_t _pad0_OFFSET = 84u;       // size=4 align=4
    static constexpr std::uint32_t _pad1_OFFSET = 88u;       // size=4 align=4
    static constexpr std::uint32_t _pad2_OFFSET = 92u;       // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_BlurLightF_SIZE = 256u; // std140=96, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_blur_light_f.glsl:9-19
layout(std140, set = 2, binding = 22) uniform PerProgramUBO_BlurLightF
{
    vec2  delta;
    float dist_factor;
    float blur_size;
    vec3  kern[4];
    float kern_scale;
    float _pad0;
    float _pad1;
    float _pad2;
};
```

= **5 実 member (vec2 delta + 2 float + vec3[4] kern + 1 float scalar) + 3 pad = SSAO/blur kernel + dist factor + blur size**

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 22 (= set=2 内高 binding、PerProgram 専用)
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: ubo_metadata.inl:72 `{ "PerProgramUBO_BlurLightF", 0xa43668deu, 256u, 2u, 22u, 0u, 1u, 8u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95`)
- **flush 経路**: PerProgram cadence setter (= `llglslshader.cpp:2143-2150` `case kCadencePerProgram:` dispatch)

---

## §4. 物理 owner

- **shell 段階**: owner なし
- **本実装化後の data source** (= **verify 要**):
  - `gPipeline.mSSAOParams` (= 推定、SSAO blur kernel + dist factor)
  - 既存 OpenGL 経路で `blur_size` / `kern` / `dist_factor` uniform 書込 site grep verify 要 (= `LLShaderMgr` 経由)
- **lifetime**: program lifetime

---

## §5. use site (shader)

- **blueprint file**: `aya_r41_blueprints/set2/per_program_ubo_blur_light_f.glsl`
  - source extract from `class1/deferred/blurLightF.glsl:64` ifdef LL_VULKAN_GLSL block (single site)
- **実 shader use site** (= grep 結果):
  - `indra/newview/app_settings/shaders/class1/deferred/blurLightF.glsl` (= single site、fragment shader、SSAO blur pass)

---

## §6. 既存 setter call site (host C++)

- **現状**: **PerProgramUBO_BlurLightF 専用 setter 不在** (= grep 確認済 2026-06-06)
- **shell 段階通電経路**: bringupTestUBO 経由 generic zero buffer 通電 (= 推定 / verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - `blurLightF.glsl` 用 SSAO kernel 書込 host site (= `LLPipeline::doSSAO` 等候補、grep verify 要)

---

## §7. 現状通電状態

- **状態**: **untouched** (= Phase 1.E 終了時点)
- **通電 commit**: なし
- **通電内容**: shader 側 UBO block 既配置、host C++ writer ゼロ

---

## §8. 本実装化に必要な作業

1. **host C++ setter 配線** = blurLightF program bind 時に SSAO kernel + delta + dist_factor + blur_size + kern_scale 書込
2. **data source 特定** = `gPipeline.mSSAOParams` 等 grep verify
3. **kern[4] (vec3 stride=16)** = std140 array stride=16 ゆえ host 側 vec3 → vec4 padding 要 (= codegen で stride=16 明示済)
4. **dirty 判定** = SSAO 設定変化時 dirty、program bind 時 flush
5. **shader 側 #else block 撤去** (= Phase 2+ 時)

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | set=2 binding=22 独立 | ✅ 独立配置 |
| OS-3 | std140 padding + vec3 array stride=16 | ✅ stride=16 明示 (codegen)、vec3 単独は std140 で vec4 alignment 罠あり = 罠回避 |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既存 GLSL #ifdef 既配置 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**vec3 array std140 罠**:
- vec3 単独は std140 で vec4 align (= 12 B 実体 + 4 B trailing pad / element)
- codegen stride=16 で明示 = host 側 4 vec4 で書込 (= 64 B 確保)
- shader 側 `vec3 kern[4]` access = compiler が 16 B stride 解釈

---

## §10. 不明事項

1. **shell 通電 commit hash**
2. **既存 OpenGL setter call site** = SSAO kernel 書込 host site
3. **data source 上流** = `gPipeline.mSSAOParams` 等構造 verify 要
4. **delta / dist_factor / blur_size data source** 個別 verify 要

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 4 binding、本 UBO は binding=22

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 本 batch PerProgram UBO 群 8 件

### §11.3 同 shader consume UBO

- `blurLightF.glsl` 内 = Frame_* + set=1 Material + 他 PerProgram (= verify 要)

### §11.4 同 data source UBO

- SSAO 系 UBO 関連 = `AOUtilParamUBO_Legacy` (set=3 binding=8 cadence=1) と data source 共有可能性 (= 推定、verify 要)

### §11.5 dirty 連動 UBO

- SSAO 設定変化時 = AOUtilParamUBO_Legacy と連動可能性

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ blurLightF program bind 時 flush

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.4.13 同期)

**Layer**: L3-13 (= B Tier β setter 推定済、PerProgram cadence、SSAO blur kernel)
**status**: **起案済** (= 2026-06-06 C-5、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.4.13` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch + L0-4 cadence + L2-2 完了 (= AOUtil 経路 pattern 確立)
- **(2) 不明事項**: SSAO blur kernel setter site (= `LLPipeline::doSSAO` 等候補) [要追加調査] / AOUtil / SoftenLight との data source 共有可能性 [要 verify]
- **(3) 調査手法**: D1 (`blur_size` / `kern` / `dist_factor` setter grep) + D4 (AOUtil / SoftenLight との data 共有 verify)
- **(4) 設計 task**: L3 全件共通 (= PerProgram triple-buffer / `forwardToUboUpload` PER_PROGRAM / program bind 単位 flush / `blurLightF.glsl` LL_VULKAN_GLSL 活性化)
- **(5) 工程**: trace L3-13 (= L2-2 後)、工数 **S-M**、L3 内独立並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= SSAO blur 効果既存と同一、visual regression ゼロ §5.4)
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `blurLightF.glsl #else` block uniform 個別宣言維持

**関連**: L0-1 + L0-4 / §5.4 / L2-2 AOUtil (= 経路 pattern + data 共有) / L3-17 SoftenLight (= data 共有候補)
