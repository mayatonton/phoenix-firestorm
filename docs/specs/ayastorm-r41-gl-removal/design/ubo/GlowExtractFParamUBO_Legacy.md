# GlowExtractFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: shell 通電済 + per-program write 経路通電済 (= Phase 1.A PA-8 + 1.C PC-7γ-1、blueprint 宣言済、実 shader UBO block は `class1/effects/glowExtractF.glsl:71` 内 `#ifdef LL_VULKAN_GLSL` block で literal extract 確認)

**本実装化に必要な作業**: 既存 5 setter (= `pipeline.cpp:9061-9068` literal で `LLShaderMgr::GLOW_LUM_WEIGHTS` / `GLOW_WARMTH_WEIGHTS` / `GLOW_WARMTH_AMOUNT` / `GLOW_MIN_LUMINANCE` / `GLOW_MAX_EXTRACT_ALPHA` 経由) の forwardToUboUpload PER_PROGRAM 経路 UBO redirect 通電

---

## §1. UBO identity

- **block_name**: `GlowExtractFParamUBO_Legacy`
- **block_hash**: `0x5e38a750u` (= `ubo_metadata.inl:47` literal)
- **block_size**: 256 B (= std140=48, device-padded=256、`ubo_layout_glowextractfparamubo_legacy.inl:19` literal)
- **member_count**: 5
- **struct definition**:

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_glowextractfparamubo_legacy.inl
struct GlowExtractFParamUBO_LegacyLayout {
    static constexpr std::uint32_t lumWeights_OFFSET      = 0u;  // size=12 align=16
    static constexpr std::uint32_t minLuminance_OFFSET    = 12u; // size=4  align=4
    static constexpr std::uint32_t warmthWeights_OFFSET   = 16u; // size=12 align=16
    static constexpr std::uint32_t maxExtractAlpha_OFFSET = 28u; // size=4  align=4
    static constexpr std::uint32_t warmthAmount_OFFSET    = 32u; // size=4  align=4
};
inline constexpr std::uint32_t GlowExtractFParamUBO_Legacy_SIZE = 256u; // std140=48, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/glow_extract_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 20) uniform GlowExtractFParamUBO_Legacy
{
    vec3 lumWeights;
    float minLuminance;
    vec3 warmthWeights;
    float maxExtractAlpha;
    float warmthAmount;
};
```

= **5 member、std140 で vec3 + float tail packing ×2 + 末尾 float = 48 B → 256 B padded**

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 20
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:47` `{ "GlowExtractFParamUBO_Legacy", 0x5e38a750u, 256u, 3u, 20u, 0u, 1u, 5u }` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program**
- **flush 経路**: `LLVKLoader::flushProgramUbos`
- **write 経路**: `LLVKLoader::writeProgramUbo`
- **storage**: `sProgramUboDirty`

---

## §4. 物理 owner

- **owner**: `LLPipeline::renderPostProcess` glow extract pass (= `pipeline.cpp:9049-9099` literal、`gGlowExtractProgram` 経路)
- **data source**:
  - `lumWeights` = `RenderGlowLumWeights` cvar (= `pipeline.cpp:9057, 9064-9065` literal)
  - `warmthWeights` = `RenderGlowWarmthWeights` cvar (= `pipeline.cpp:9058, 9066-9067` literal)
  - `warmthAmount` = `RenderGlowWarmthAmount` cvar (= `pipeline.cpp:9056, 9068` literal)
  - `minLuminance` = `RenderGlowMinLuminance` cvar (= `pipeline.cpp:9061` literal)
  - `maxExtractAlpha` = `maxAlpha` (= `pipeline.cpp:9063` literal、derive 元 verify 要)
- **lifetime**: per-program (= glow extract pass bind 時)

---

## §5. use site (shader)

- **blueprint file**: `set3/glow_extract_f_param_ubo_legacy.glsl`
- **blueprint origin**: `class1/effects/glowExtractF.glsl:71` (= literal extract source)
- **実 shader use site**: `class1/effects/glowExtractF.glsl` (= grep result 唯一)
- **consume**: glow extract fragment shader、HDR scene から glow source 抽出 (= luminance weight 経由 + warmth tint + min/max threshold)

---

## §6. 既存 setter call site (host C++)

- **`lumWeights` setter**: `pipeline.cpp:9064-9065` literal: `gGlowExtractProgram.uniform3f(LLShaderMgr::GLOW_LUM_WEIGHTS, lumWeights.mV[0], lumWeights.mV[1], lumWeights.mV[2]);`
- **`warmthWeights` setter**: `pipeline.cpp:9066-9067` literal: `gGlowExtractProgram.uniform3f(LLShaderMgr::GLOW_WARMTH_WEIGHTS, warmthWeights.mV[0], warmthWeights.mV[1], warmthWeights.mV[2]);`
- **`warmthAmount` setter**: `pipeline.cpp:9068` literal: `gGlowExtractProgram.uniform1f(LLShaderMgr::GLOW_WARMTH_AMOUNT, warmthAmount);`
- **`minLuminance` setter**: `pipeline.cpp:9061` literal: `gGlowExtractProgram.uniform1f(LLShaderMgr::GLOW_MIN_LUMINANCE, RenderGlowMinLuminance);`
- **`maxExtractAlpha` setter**: `pipeline.cpp:9063` literal: `gGlowExtractProgram.uniform1f(LLShaderMgr::GLOW_MAX_EXTRACT_ALPHA, maxAlpha);`
- **enum 宣言**: `llshadermgr.h:160-164` literal: `GLOW_MIN_LUMINANCE / GLOW_MAX_EXTRACT_ALPHA / GLOW_LUM_WEIGHTS / GLOW_WARMTH_WEIGHTS / GLOW_WARMTH_AMOUNT`
- **string 宣言**: `llshadermgr.cpp:1638-1642` literal: `mReservedUniforms.push_back("minLuminance"); mReservedUniforms.push_back("maxExtractAlpha"); mReservedUniforms.push_back("lumWeights"); mReservedUniforms.push_back("warmthWeights"); mReservedUniforms.push_back("warmthAmount");`
- **`sLumWeights` static**: `llrender/llpostprocess.cpp:39` literal: `static LLStaticHashedString sLumWeights("lumWeights");` (= 別 path 参照、verify 要)
- **共通 redirect 経路**: `LLGLSLShader::uniform1f` / `uniform3f`
- **PER_PROGRAM case**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo`

---

## §7. 現状通電状態

- **状態**: shell 通電済 + write 経路本格化済
- **bind 経路**: set=3 binding=20 で program 切替時 bind
- **write 経路**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo`
- **MUSEUBO-A 整合**: `mUseUBO=false` default で不到達

---

## §8. 本実装化に必要な作業

1. **`mUseUBO=true` cold launch 検証**:
   - `pipeline.cpp:9061-9068` 5 setter call から forwardToUboUpload PER_PROGRAM 経由 writeProgramUbo 実走確認
   - vec3 (lumWeights / warmthWeights) memcpy 12 B + stride padding に注意
2. **`llpostprocess.cpp:39` `sLumWeights` 別 path 参照の確認**:
   - 既存 OpenGL でも別 path 経由参照あり、本 UBO redirect で集約されるか確認
3. **codegen 再実行不要**: blueprint member 不変

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ vec3+float×2 + 末 float = 48 B std140 → 256 B padded |
| OS-4 | minUniformBufferOffsetAlignment | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ glowExtractF で UBO block 宣言済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**vec3 + float tail packing**:
- `vec3 lumWeights; float minLuminance;` (= offset 0-12 + 12-16) → vec3 後の trailing 4 B を float が埋める
- 同 pattern × 2 (= `warmthWeights / maxExtractAlpha`) + 末尾 `warmthAmount`
- host C++ で vec3 = LLVector3 mV[3] 直接 memcpy 時 align 注意要

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3、Legacy 帯)

- 同 set=3 = ~58 UBO

### §11.2 同 cadence cluster UBO (= cadence_tag=1 per-program)

- 全 Legacy UBO + 本 UBO

### §11.3 同 shader consume UBO

- `class1/effects/glowExtractF.glsl` で同時 consume = vertex shader 由来 UBO (= verify 要)
- + `DEFERRED_SCREEN_RES` (= `pipeline.cpp:9078` literal `gGlowExtractProgram.uniform2f(LLShaderMgr::DEFERRED_SCREEN_RES, ...)`)、FrameViewProj.screen_res 経由

### §11.4 同 data source UBO

- **同 owner**: `LLPipeline::renderPostProcess` glow chain (= `pipeline.cpp:9049-9099` literal)
- 同 owner = `GlowFParamUBO_Legacy` / `GlowVParamUBO_Legacy` / `GlowCombineFParamUBO_Legacy` (= Glow 4 UBO 一族)

### §11.5 dirty 連動 UBO

- glow extract pass bind 時に本 UBO 単独 dirty
- 連動 = なし (= 他 Glow UBO は別 pass で別 program bind、shader × block_hash key で独立)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- glow chain: extract (本 UBO) → blur V (`GlowVParamUBO_Legacy`) → blur F (`GlowFParamUBO_Legacy`) → combine (`GlowCombineFParamUBO_Legacy`)
- pass 単位 program bind + set=3 binding=20 更新

---

## §10. 不明事項

1. **`maxAlpha` derive 元** = `pipeline.cpp:9063` literal で setter call、`maxAlpha` local 変数の derivation 経路 grep 要
2. **`llpostprocess.cpp:39` `sLumWeights` 別 path** = 旧 post-process 経路の残骸 vs active path 判定要
3. **set 3 bind 単位** = program 切替時 set 3 全 binding rebind か個別 rebind か
4. **glow chain 全体の dirty 順序** = 4 UBO 連動 dirty trigger 経路 (= 各 pass で独立 dirty trigger)
5. **vec3 + float tail packing 動作確認** = std140 規則通り offset 配置、host C++ memcpy で stride 違反なし verify 要

= 上記 5 項目は本 UBO file 完成時に grep + Read で逐次解消。
