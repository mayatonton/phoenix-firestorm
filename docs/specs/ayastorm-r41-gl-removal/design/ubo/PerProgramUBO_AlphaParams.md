# PerProgramUBO_AlphaParams — UBO design (= 実コードベース調査資料)

**通電状態**: **untouched** (= host C++ で `PerProgramUBO_AlphaParams` への setter 呼出ゼロ、grep 確認済 2026-06-06)

**本実装化に必要な作業**: 実 `near_clip` (= viewport near clip plane distance) を host から PerProgram cadence setter 経由で書込 + class1/deferred/alphaV.glsl 内 UBO consume へ切替

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_AlphaParams`
- **block_hash**: `0x68e4c001u` (= FNV-1a("PerProgramUBO_AlphaParams"))
- **block_size**: 256 B (= std140=16 B, device-padded 256 B)
- **member_count**: 4 (= 実 1 + pad 3)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_alphaparams.inl:12-17
struct PerProgramUBO_AlphaParamsLayout {
    static constexpr std::uint32_t near_clip_OFFSET = 0u;   // size=4 align=4
    static constexpr std::uint32_t _pad_ap0_OFFSET = 4u;    // size=4 align=4
    static constexpr std::uint32_t _pad_ap1_OFFSET = 8u;    // size=4 align=4
    static constexpr std::uint32_t _pad_ap2_OFFSET = 12u;   // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_AlphaParams_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_alpha_params.glsl:9-15
layout(std140, set = 2, binding = 3) uniform PerProgramUBO_AlphaParams
{
    float near_clip;
    float _pad_ap0;
    float _pad_ap1;
    float _pad_ap2;
};
```

= **実 member 1 (near_clip) + pad 3 float = 1 vec4 slot 充填、alphaV.glsl per-program 定数**

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 3
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC` (= set=2 UBO_DYNAMIC 帯、`llvkloader.cpp:861` literal)
- **pipeline layout**: `sAYAStandardLayout`
- **set=2 binding=3** = 独立 binding (= set=2 binding=0 共有 6 UBO 群とは別 binding)
- **source**: ubo_metadata.inl:71 `{ "PerProgramUBO_AlphaParams", 0x68e4c001u, 256u, 2u, 3u, 0u, 1u, 4u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal: `constexpr U32 kCadencePerProgram = 1u; // ubo_metadata.inl で 88 件最大`)
- **flush 経路**: PerProgram cadence setter (= `llglslshader.cpp:2143-2150` `case kCadencePerProgram:` dispatch)
- **意味詳細**: program 単位 (= LLGLSLShader 単位) で 1 UBO instance、program bind 時に flush
- **source**: llglslshader.cpp:95 + setter dispatch literal

---

## §4. 物理 owner

- **shell 段階**: owner なし
- **本実装化後の data source** (= **verify 要**):
  - `LLViewerCamera::getNear()` (= 推定、viewport near plane)
  - 既存 OpenGL 経路で `LLShaderMgr::NEAR_CLIP` uniform を書込む site (= grep verify 要)
- **lifetime**: program lifetime (= LLGLSLShader instance 単位、program 再 link 時更新)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_alpha_params.glsl`
  - source extract from `class1/deferred/alphaV.glsl:145` ifdef LL_VULKAN_GLSL block (single site)
- **実 shader use site** (= grep 結果):
  - `indra/newview/app_settings/shaders/class1/deferred/alphaV.glsl` (= single site、vertex shader)

---

## §6. 既存 setter call site (host C++)

- **現状**: **PerProgramUBO_AlphaParams 専用 setter 不在** (= grep 確認済 2026-06-06)
- **shell 段階通電経路**: 推定 = bringupTestUBO 経由 generic zero buffer 通電
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL 経路で `near_clip` uniform 書込 site (= `LLShaderMgr::NEAR_CLIP` 等候補、grep verify 要)
  - PerProgram cadence ゆえ `LLGLSLShader::bind` 時に flush 経路 (= verify 要)

---

## §7. 現状通電状態

- **状態**: **untouched** (= Phase 1.E 終了時点)
- **通電 commit**: なし
- **通電内容**: shader 側 `class1/deferred/alphaV.glsl` で UBO consume block 既配置、host C++ writer ゼロ

---

## §8. 本実装化に必要な作業

1. **host C++ setter 配線** = alphaV program bind 時に `near_clip` 値書込 (= PerProgram cadence 経路、`llglslshader.cpp:2143-2150` setter dispatch)
2. **data source 特定** = `LLViewerCamera::getNear()` 等 grep verify
3. **dirty 判定** = camera near plane 変化時 dirty、program bind 時 flush
4. **shader 側 #else block 撤去** (= Phase 2+ 時)

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | set=2 binding=3 独立 binding | ✅ 独立配置 |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded、1 vec4 slot |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既存 GLSL #ifdef LL_VULKAN_GLSL 既配置 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**pad 3 float の罠**: std140 で float 単独宣言は次 vec4 boundary までの自動 pad ありだが、codegen は明示 pad で確実化 = 罠なし

---

## §10. 不明事項

1. **shell 通電 commit hash** = bringupTestUBO 経路通電 commit
2. **既存 OpenGL setter call site** = `near_clip` uniform 書込 host site
3. **data source 上流** = `LLViewerCamera::getNear()` か別経路か verify 要

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 4 binding、本 UBO は binding=3

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram、最多 88 件)

- 本 batch 担当 PerProgram UBO 7 (= BlurLightF / CofF / ColorGrading / FsObjectIdF / FullbrightShinyV / FxaaF / GammaCorrect)
- 他 PerProgram UBO 多数 (= ubo_metadata.inl で 88 件最大、`llglslshader.cpp:95` literal)

### §11.3 同 shader consume UBO

- `class1/deferred/alphaV.glsl` 内同時 consume = Frame_* + set=1 Material + 他 PerDraw / PerProgram (= verify 要)

### §11.4 同 data source UBO

- camera near plane 由来 = FrameViewProj (set=0 binding=0) と関連可能性 (= 推定、verify 要)

### §11.5 dirty 連動 UBO

- camera 変化時 = FrameViewProj 等と連動可能性

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時 flush + set=2 binding 構成内に常駐
