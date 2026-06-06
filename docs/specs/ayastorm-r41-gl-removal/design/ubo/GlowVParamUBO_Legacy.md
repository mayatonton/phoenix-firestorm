# GlowVParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: shell 通電済 + per-program write 経路通電済 (= Phase 1.A PA-8 + 1.C PC-7γ-1、blueprint 宣言済、実 shader UBO block は `class1/effects/glowV.glsl:54` 内 `#ifdef LL_VULKAN_GLSL` block で literal extract 確認)

**本実装化に必要な作業**: 既存 `glowDelta` setter (= `pipeline.cpp:9134, 9138` literal で `LLShaderMgr::GLOW_DELTA` 経由、horizontal `(delta, 0)` vs vertical `(0, delta)`) の forwardToUboUpload PER_PROGRAM 経路 UBO redirect 通電

---

## §1. UBO identity

- **block_name**: `GlowVParamUBO_Legacy`
- **block_hash**: `0xa4b42951u` (= `ubo_metadata.inl:49` literal)
- **block_size**: 256 B (= std140=16, device-padded=256、`ubo_layout_glowvparamubo_legacy.inl:15` literal)
- **member_count**: 1
- **struct definition**:

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_glowvparamubo_legacy.inl
struct GlowVParamUBO_LegacyLayout {
    static constexpr std::uint32_t glowDelta_OFFSET = 0u; // size=8 align=8
};
inline constexpr std::uint32_t GlowVParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/glow_v_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 19) uniform GlowVParamUBO_Legacy
{
    vec2 glowDelta;
};
```

= **1 member、vec2 8 B → 256 B padded**

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 19
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:49` `{ "GlowVParamUBO_Legacy", 0xa4b42951u, 256u, 3u, 19u, 0u, 1u, 1u }` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program**
- **flush 経路**: `LLVKLoader::flushProgramUbos`
- **write 経路**: `LLVKLoader::writeProgramUbo`
- **storage**: `sProgramUboDirty`

---

## §4. 物理 owner

- **owner**: `LLPipeline::renderPostProcess` glow blur (= `pipeline.cpp:9100-9140` 周辺、`gGlowProgram` vertex shader)
- **data source**:
  - `glowDelta` = horizontal/vertical 2 pass dispatch 用 2D offset (= horizontal `(delta, 0)` `:9134` literal / vertical `(0, delta)` `:9138` literal)
  - `delta` derive 元 = 不明 / verify 要 (= `pipeline.cpp:9134` literal の `delta` local 変数の元、推定 `RenderGlowSize` cvar 周辺)
- **lifetime**: per-program (= glow blur pass bind 時、horizontal/vertical 切替で再 set)

---

## §5. use site (shader)

- **blueprint file**: `set3/glow_v_param_ubo_legacy.glsl`
- **blueprint origin**: `class1/effects/glowV.glsl:54` (= literal extract source)
- **実 shader use site**: `class1/effects/glowV.glsl` (= grep result 唯一)
- **consume**: glow blur vertex shader、`glowDelta` で sampling offset 計算

---

## §6. 既存 setter call site (host C++)

- **`glowDelta` setter (horizontal)**: `pipeline.cpp:9134` literal: `gGlowProgram.uniform2f(LLShaderMgr::GLOW_DELTA, delta, 0);`
- **`glowDelta` setter (vertical)**: `pipeline.cpp:9138` literal: `gGlowProgram.uniform2f(LLShaderMgr::GLOW_DELTA, 0, delta);`
- **enum 宣言**: `llshadermgr.h:166` literal: `GLOW_DELTA, //  "glowDelta"`
- **string 宣言**: `llshadermgr.cpp:1644` literal: `mReservedUniforms.push_back("glowDelta");`
- **共通 redirect 経路**: `LLGLSLShader::uniform2f`
- **PER_PROGRAM case**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo`

---

## §7. 現状通電状態

- **状態**: shell 通電済 + write 経路本格化済
- **bind 経路**: set=3 binding=19 で program 切替時 bind
- **write 経路**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo`
- **MUSEUBO-A 整合**: `mUseUBO=false` default で不到達

---

## §8. 本実装化に必要な作業

1. **`mUseUBO=true` cold launch 検証**:
   - `pipeline.cpp:9134, 9138` 2 setter call (= horizontal/vertical) から forwardToUboUpload PER_PROGRAM 経由 writeProgramUbo 実走確認
   - horizontal/vertical 2 pass で 同 program 内 setter 連続 = dirty 連続発火、flush 1 回で済む確認
2. **`delta` derive 元特定**:
   - `pipeline.cpp:9134` literal `delta` local 変数の元 grep 要
3. **codegen 再実行不要**: blueprint member 不変

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ vec2 = 8 B std140 → 256 B padded |
| OS-4 | minUniformBufferOffsetAlignment | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ glowV で UBO block 宣言済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**horizontal/vertical 2 pass dispatch**:
- 同 program 内で 2 回 setter 連続呼出 (= horizontal pass dispatch → vertical pass dispatch)
- writeProgramUbo で 2 回 memcpy + dirty.store(true) → flushProgramUbos で 1 回 GPU upload
- ただし 2 pass 間で必ず flush + dispatch が挟まる (= 2 回 GPU upload 必須、dirty store 連続上書きでなく flush 必須)
- verify 要 (= flush timing が dispatch 間に挟まる経路)

**248 B dead space**:
- 1 member 8 B vs padded 256 B、Phase 2 で glow blur param 追加候補

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3、Legacy 帯)

- 同 set=3 = ~58 UBO

### §11.2 同 cadence cluster UBO (= cadence_tag=1 per-program)

- 全 Legacy UBO + 本 UBO

### §11.3 同 shader consume UBO

- `class1/effects/glowV.glsl` で同時 consume = fragment shader `glowF.glsl` 由来 `GlowFParamUBO_Legacy` (= 同 pair 構成)

### §11.4 同 data source UBO

- **同 owner**: `LLPipeline::renderPostProcess` glow chain
- 同 owner = `GlowFParamUBO_Legacy` (= 同 glow blur pass の fragment shader、`glowStrength` 経由) / `GlowExtractFParamUBO_Legacy` / `GlowCombineFParamUBO_Legacy` (= Glow 4 UBO 一族)

### §11.5 dirty 連動 UBO

- glow blur pass bind 時に本 UBO + `GlowFParamUBO_Legacy` 連動 dirty 可能性
- verify 要 (= vertex + fragment shader 両 UBO 連動 trigger 経路)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- glow chain: extract → **blur V (本 UBO) + blur F (`GlowFParamUBO_Legacy`)** = 同 pass、horizontal/vertical 2 dispatch → combine
- horizontal pass: `glowDelta = (delta, 0)` → dispatch → flush
- vertical pass: `glowDelta = (0, delta)` → dispatch → flush
- = 同 program / 同 binding に対して 2 回 write + 2 回 flush (= 2 回 GPU upload)

---

## §10. 不明事項

1. **`delta` derive 元** = `pipeline.cpp:9134` literal `delta` local 変数の元
2. **horizontal/vertical 2 dispatch 間 flush** = 2 回 GPU upload で dirty store 連続上書き回避できるか (= flush_program timing 経路 verify 要)
3. **`GlowFParamUBO_Legacy` との連動 dirty** = 同 blur pass で 2 UBO 連動 dirty trigger
4. **set 3 bind 単位** = program 切替時 set 3 全 binding rebind か個別 rebind か
5. **248 B dead space** = Phase 2 で glow blur param 追加候補

= 上記 5 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.11 同期)

**Layer**: L4-11 sub-cluster (b) (= glow blur V/F pair 2 UBO、horizontal/vertical 2 pass dispatch)
**status**: **起案済** (= 2026-06-06 C-6-f、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.11` (= single source of truth)
**要点**: 1 member (glowDelta vec2)、248B dead space、**shell 通電済 + write 経路本格化済** (= PA-8 + PC-7γ-1)、setter 2 site 全件特定済 (= `pipeline.cpp:9134` horizontal `(delta, 0)` + `:9138` vertical `(0, delta)`)、reserved 登録 `llshadermgr.cpp:1644` + `llshadermgr.h:166` GLOW_DELTA 確認、`delta` derive 元不明 (= 推定 `RenderGlowSize` cvar) [要追加調査]、**horizontal/vertical 2 pass dispatch で 2 回 GPU upload 必須** (= dirty store 連続上書き回避、flush timing 経路実装 verify 必須) [要 verify]、GlowF と V/F pair 連動 dirty trigger [要 verify]、glow chain 第 2-3 段、cadence PerProgram 維持、工数 group 全体 M 内
**関連**: L0-1 dispatch (= 衝突なし binding=19) / §3.5.11 sub-cluster (b) GlowF (= V/F pair 連動 dirty) / sub-cluster (a) GlowExtract (= 第 1 段) / sub-cluster (c) GlowCombine (= 第 4 段)
