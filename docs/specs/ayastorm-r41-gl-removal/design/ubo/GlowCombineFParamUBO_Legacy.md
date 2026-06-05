# GlowCombineFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: shell 通電済 + per-program write 経路通電済 (= Phase 1.A PA-8 + 1.C PC-7γ-1、blueprint 宣言済、実 shader UBO block は `class1/interface/glowcombineF.glsl:44` 内 `#ifdef LL_VULKAN_GLSL` block で literal extract 確認)

**本実装化に必要な作業**: 既存 `greyscale_str` + `sepia_str` + `num_colors` setter (= `pipeline.cpp:9589-9591, 9595-9597, 10706-10715` literal で `LLShaderMgr::DEFERRED_GREYSCALE_STRENGTH` / `DEFERRED_SEPIA_STRENGTH` / `DEFERRED_NUM_COLORS` 経由) の forwardToUboUpload PER_PROGRAM 経路 UBO redirect 通電

---

## §1. UBO identity

- **block_name**: `GlowCombineFParamUBO_Legacy`
- **block_hash**: `0x1a12dc30u` (= `ubo_metadata.inl:46` literal)
- **block_size**: 256 B (= std140=16, device-padded=256、`ubo_layout_glowcombinefparamubo_legacy.inl:18` literal)
- **member_count**: 4
- **struct definition**:

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_glowcombinefparamubo_legacy.inl
struct GlowCombineFParamUBO_LegacyLayout {
    static constexpr std::uint32_t greyscale_str_OFFSET                    = 0u;  // size=4 align=4
    static constexpr std::uint32_t sepia_str_OFFSET                        = 4u;  // size=4 align=4
    static constexpr std::uint32_t num_colors_OFFSET                       = 8u;  // size=4 align=4
    static constexpr std::uint32_t _pad_glowcombine_f_legacy_0_OFFSET      = 12u; // size=4 align=4
};
inline constexpr std::uint32_t GlowCombineFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/glow_combine_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 49) uniform GlowCombineFParamUBO_Legacy
{
    float greyscale_str;
    float sepia_str;
    float num_colors;
    float _pad_glowcombine_f_legacy_0;
};
```

= **4 member (3 active + 1 pad)、std140 で float ×4 = 16 B → 256 B padded**、color grading post-process (greyscale / sepia / 色数減色)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 49
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:46` `{ "GlowCombineFParamUBO_Legacy", 0x1a12dc30u, 256u, 3u, 49u, 0u, 1u, 4u }` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program**
- **flush 経路**: `LLVKLoader::flushProgramUbos`
- **write 経路**: `LLVKLoader::writeProgramUbo`
- **storage**: `sProgramUboDirty`

---

## §4. 物理 owner

- **owner**: `LLPipeline::renderPostProcess` glow combine pass (= `pipeline.cpp:9583-9597` literal + `:10706-10715` literal)
- **data source**:
  - `greyscale_str` = `RenderGreyscaleStrength` cvar (= `pipeline.cpp:9589` literal)
  - `sepia_str` = `RenderSepiaStrength` cvar (= `pipeline.cpp:9590` literal)
  - `num_colors` = `RenderNumColors` cvar (= `pipeline.cpp:9591` literal)
- **lifetime**: per-program (= glow combine pass bind 時)

---

## §5. use site (shader)

- **blueprint file**: `set3/glow_combine_f_param_ubo_legacy.glsl`
- **blueprint origin**: `class1/interface/glowcombineF.glsl:44` (= literal extract source)
- **実 shader use site**: `class1/interface/glowcombineF.glsl` (= grep result 唯一)
- **consume**: glow combine post-process fragment shader、color grading (= greyscale + sepia tint + posterize)

---

## §6. 既存 setter call site (host C++)

- **`greyscale_str` setter**: 
  - `pipeline.cpp:9589` literal: `gGlowCombineProgram.uniform1f(LLShaderMgr::DEFERRED_GREYSCALE_STRENGTH, RenderGreyscaleStrength);`
  - `pipeline.cpp:9595` literal: `gGlowCombineProgram.uniform1f(LLShaderMgr::DEFERRED_GREYSCALE_STRENGTH, 0.0f);` (= disable path)
  - `pipeline.cpp:10706` literal: `shader.uniform1f(LLShaderMgr::DEFERRED_GREYSCALE_STRENGTH, RenderGreyscaleStrength);`
  - `pipeline.cpp:10713` literal: `shader.uniform1f(LLShaderMgr::DEFERRED_GREYSCALE_STRENGTH, 0.0f);`
- **`sepia_str` setter**:
  - `pipeline.cpp:9590` literal: `gGlowCombineProgram.uniform1f(LLShaderMgr::DEFERRED_SEPIA_STRENGTH, RenderSepiaStrength);`
  - `pipeline.cpp:9596` literal: `gGlowCombineProgram.uniform1f(LLShaderMgr::DEFERRED_SEPIA_STRENGTH, 0.0f);`
  - `pipeline.cpp:10707` literal: `shader.uniform1f(LLShaderMgr::DEFERRED_SEPIA_STRENGTH, RenderSepiaStrength);`
  - `pipeline.cpp:10714` literal: `shader.uniform1f(LLShaderMgr::DEFERRED_SEPIA_STRENGTH, 0.0f);`
- **`num_colors` setter**:
  - `pipeline.cpp:9591` literal: `gGlowCombineProgram.uniform1f(LLShaderMgr::DEFERRED_NUM_COLORS, (GLfloat)RenderNumColors);`
  - `pipeline.cpp:9597` literal: `gGlowCombineProgram.uniform1f(LLShaderMgr::DEFERRED_NUM_COLORS, 1.0f);`
  - `pipeline.cpp:10708` literal: `shader.uniform1f(LLShaderMgr::DEFERRED_NUM_COLORS, (GLfloat)RenderNumColors);`
  - `pipeline.cpp:10715` literal: `shader.uniform1f(LLShaderMgr::DEFERRED_NUM_COLORS, 1.0f);`
- **enum 宣言**: `llshadermgr.h:417-419` literal: `DEFERRED_SEPIA_STRENGTH, //  "sepia_strength" / DEFERRED_GREYSCALE_STRENGTH, //  "greyscale_strength" / DEFERRED_NUM_COLORS, //  "num_colors"`
- **string 宣言**: `llshadermgr.cpp:1896-1898` literal: `mReservedUniforms.push_back("sepia_str"); mReservedUniforms.push_back("greyscale_str"); mReservedUniforms.push_back("num_colors");`
- **共通 redirect 経路**: `LLGLSLShader::uniform1f` (= 既 PC-7γ-1)
- **PER_PROGRAM case**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo`

**注: enum 名 vs reserved string 名の不一致**
- enum 名: `sepia_strength` / `greyscale_strength` (= `llshadermgr.h:417,418` literal comment)
- reserved string 名: `sepia_str` / `greyscale_str` (= `llshadermgr.cpp:1896-1897` literal)
- blueprint member 名: `sepia_str` / `greyscale_str` (= shader 内 GLSL identifier に合わせる)
- = enum comment が古い表記、reserved string と shader UBO member 名は一致

---

## §7. 現状通電状態

- **状態**: shell 通電済 + write 経路本格化済
- **bind 経路**: set=3 binding=49 で program 切替時 bind
- **write 経路**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo`
- **MUSEUBO-A 整合**: `mUseUBO=false` default で不到達

---

## §8. 本実装化に必要な作業

1. **`mUseUBO=true` cold launch 検証**:
   - glow combine pass の `pipeline.cpp:9589-9591` 3 setter call から forwardToUboUpload PER_PROGRAM 経由 writeProgramUbo 実走確認
   - disable path (`:9595-9597`) でも 0.0/1.0 setter 通電確認
   - `pipeline.cpp:10706-10715` 別 path も同様 (= 別 shader 経由想定、verify 要)
2. **codegen 再実行不要**: blueprint member 不変

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ float×4 = 16 B std140 → 256 B padded |
| OS-4 | minUniformBufferOffsetAlignment | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ glowcombineF で UBO block 宣言済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**setter 4 path 重複**:
- `pipeline.cpp:9589-9597` (= gGlowCombineProgram 専属)
- `pipeline.cpp:10706-10715` (= shader 引数版、別 path)
- 両 path とも PER_PROGRAM case 経由で writeProgramUbo に集約、shader 切替で dirty 重複なし (= `sProgramUboDirty` key = shader × block_hash)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3、Legacy 帯)

- 同 set=3 = ~58 UBO

### §11.2 同 cadence cluster UBO (= cadence_tag=1 per-program)

- 全 Legacy UBO + 本 UBO

### §11.3 同 shader consume UBO

- `class1/interface/glowcombineF.glsl` で同時 consume = vertex shader 由来 UBO (= verify 要、推定 FrameViewProj for vertex transform)

### §11.4 同 data source UBO

- **同 owner**: `LLPipeline::renderPostProcess` glow chain
- 同 owner = `GlowFParamUBO_Legacy` / `GlowVParamUBO_Legacy` / `GlowExtractFParamUBO_Legacy` (= 同 glow pipeline 4 UBO 一族)

### §11.5 dirty 連動 UBO

- glow combine pass bind 時に本 UBO 単独 dirty
- 連動 = なし (= 他 Glow UBO は別 pass で別 program bind)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- glow combine pass program bind 時に set=3 binding=49 更新
- glow chain は extract → blur (V) → blur (F) → combine の順 sequential dispatch

---

## §10. 不明事項

1. **`pipeline.cpp:10706-10715` shader 引数版の specific shader** = `gGlowCombineProgram` 以外で本 UBO consume する shader 特定要
2. **enum comment 表記の古さ** = `llshadermgr.h:417,418` enum comment が `sepia_strength` / `greyscale_strength` だが reserved string は `sepia_str` / `greyscale_str`、blueprint member 名と一致 (= comment 修正候補だが本 UBO scope 外)
3. **set 3 bind 単位** = program 切替時 set 3 全 binding rebind か個別 rebind か
4. **glow chain 全体の dirty 順序** = extract → blur → combine の 4 UBO 連動 dirty trigger 経路
5. **240 B dead space** = std140=16 B vs padded 256 B、Phase 2 で color grading param 追加候補

= 上記 5 項目は本 UBO file 完成時に grep + Read で逐次解消。
