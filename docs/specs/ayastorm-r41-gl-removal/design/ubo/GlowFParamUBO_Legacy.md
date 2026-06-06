# GlowFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: shell 通電済 + per-program write 経路通電済 (= Phase 1.A PA-8 + 1.C PC-7γ-1、blueprint 宣言済、実 shader UBO block は `class1/effects/glowF.glsl:39` 内 `#ifdef LL_VULKAN_GLSL` block で literal extract 確認)

**本実装化に必要な作業**: 既存 `glowStrength` setter (= `pipeline.cpp:9116` literal で `LLShaderMgr::GLOW_STRENGTH` 経由) の forwardToUboUpload PER_PROGRAM 経路 UBO redirect 通電

---

## §1. UBO identity

- **block_name**: `GlowFParamUBO_Legacy`
- **block_hash**: `0x8ec2d1c1u` (= `ubo_metadata.inl:48` literal)
- **block_size**: 256 B (= std140=16, device-padded=256、`ubo_layout_glowfparamubo_legacy.inl:15` literal)
- **member_count**: 1
- **struct definition**:

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_glowfparamubo_legacy.inl
struct GlowFParamUBO_LegacyLayout {
    static constexpr std::uint32_t glowStrength_OFFSET = 0u; // size=4 align=4
};
inline constexpr std::uint32_t GlowFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/glow_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 18) uniform GlowFParamUBO_Legacy
{
    float glowStrength;
};
```

= **1 member、float 4 B → 256 B padded** (= 最小 member UBO)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 18
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:48` `{ "GlowFParamUBO_Legacy", 0x8ec2d1c1u, 256u, 3u, 18u, 0u, 1u, 1u }` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program**
- **flush 経路**: `LLVKLoader::flushProgramUbos`
- **write 経路**: `LLVKLoader::writeProgramUbo`
- **storage**: `sProgramUboDirty`

---

## §4. 物理 owner

- **owner**: `LLPipeline::renderPostProcess` glow blur F (= horizontal pass、`pipeline.cpp:9100-9140` 周辺、`gGlowProgram` 経路)
- **data source**:
  - `glowStrength` = `strength` (= `pipeline.cpp:9116` literal、derive 元は frame glow accumulator 関連、verify 要)
- **lifetime**: per-program (= glow blur pass bind 時)

---

## §5. use site (shader)

- **blueprint file**: `set3/glow_f_param_ubo_legacy.glsl`
- **blueprint origin**: `class1/effects/glowF.glsl:39` (= literal extract source)
- **実 shader use site**: `class1/effects/glowF.glsl` (= grep result 唯一)
- **consume**: glow blur fragment shader (= horizontal blur)、`glowStrength` で blur intensity scale

---

## §6. 既存 setter call site (host C++)

- **`glowStrength` setter**: `pipeline.cpp:9116` literal: `gGlowProgram.uniform1f(LLShaderMgr::GLOW_STRENGTH, strength);`
- **enum 宣言**: `llshadermgr.h:165` literal: `GLOW_STRENGTH, //  "glowStrength"`
- **string 宣言**: `llshadermgr.cpp:1643` literal: `mReservedUniforms.push_back("glowStrength");`
- **共通 redirect 経路**: `LLGLSLShader::uniform1f`
- **PER_PROGRAM case**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo`

---

## §7. 現状通電状態

- **状態**: shell 通電済 + write 経路本格化済
- **bind 経路**: set=3 binding=18 で program 切替時 bind
- **write 経路**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo`
- **MUSEUBO-A 整合**: `mUseUBO=false` default で不到達

---

## §8. 本実装化に必要な作業

1. **`mUseUBO=true` cold launch 検証**:
   - `pipeline.cpp:9116` 1 setter call から forwardToUboUpload PER_PROGRAM 経由 writeProgramUbo 実走確認
2. **`strength` derive 元特定**:
   - `pipeline.cpp:9116` literal `strength` local 変数の derive 元 grep 要 (= 推定 `RenderGlowStrength` cvar)
3. **codegen 再実行不要**: blueprint member 不変

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ float = 4 B std140 → 256 B padded |
| OS-4 | minUniformBufferOffsetAlignment | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ glowF で UBO block 宣言済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**252 B dead space**:
- 1 member 4 B vs padded 256 B、最大 dead space ratio
- Phase 2 で glow chain 関連 param 追加候補 (= layout 不変前提)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3、Legacy 帯)

- 同 set=3 = ~58 UBO

### §11.2 同 cadence cluster UBO (= cadence_tag=1 per-program)

- 全 Legacy UBO + 本 UBO

### §11.3 同 shader consume UBO

- `class1/effects/glowF.glsl` で同時 consume = vertex shader `glowV.glsl` 由来 `GlowVParamUBO_Legacy` (= 同 pair 構成)

### §11.4 同 data source UBO

- **同 owner**: `LLPipeline::renderPostProcess` glow chain
- 同 owner = `GlowVParamUBO_Legacy` (= 同 glow blur pass の vertex shader) / `GlowExtractFParamUBO_Legacy` / `GlowCombineFParamUBO_Legacy` (= Glow 4 UBO 一族)

### §11.5 dirty 連動 UBO

- glow blur F pass bind 時に本 UBO + `GlowVParamUBO_Legacy` (= 同 pass の vertex shader、`glowDelta` 経由) 連動 dirty 可能性
- verify 要 (= horizontal/vertical 2 pass dispatch で `glowDelta` 切替と `glowStrength` の独立性)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- glow chain: extract → blur V (`glowV.glsl` + 本 UBO 用 vertex `GlowVParamUBO_Legacy`) → blur F (本 UBO) → combine
- horizontal/vertical 2 pass dispatch (= `gGlowProgram.uniform2f(GLOW_DELTA, delta, 0)` vs `(0, delta)`、`pipeline.cpp:9134, 9138` literal)

---

## §10. 不明事項

1. **`strength` derive 元** = `pipeline.cpp:9116` literal `strength` local 変数の元 (= `RenderGlowStrength` cvar 推定、verify 要)
2. **`GlowVParamUBO_Legacy` との連動 dirty** = 同 pass で vertex + fragment 両 UBO 連動 dirty trigger
3. **horizontal/vertical 2 pass dispatch 経路** = `glowDelta` setter 2 回切替 (`:9134, 9138`) と `glowStrength` setter (= 1 回設定) の関係
4. **set 3 bind 単位** = program 切替時 set 3 全 binding rebind か個別 rebind か
5. **252 B dead space** = Phase 2 で glow blur param 追加候補

= 上記 5 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.11 同期)

**Layer**: L4-11 sub-cluster (b) (= glow blur V/F pair 2 UBO)
**status**: **起案済** (= 2026-06-06 C-6-f、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.11` (= single source of truth)
**要点**: 1 member (glowStrength float)、**最小 member UBO** (= 4B std140 / 256B padded、252B dead space 最大)、**shell 通電済 + write 経路本格化済** (= PA-8 + PC-7γ-1)、setter `pipeline.cpp:9116` literal `gGlowProgram.uniform1f(LLShaderMgr::GLOW_STRENGTH, strength);`、reserved 登録 `llshadermgr.cpp:1643` + `llshadermgr.h:165` GLOW_STRENGTH 確認、`strength` derive 元不明 (= 推定 `RenderGlowStrength` cvar) [要追加調査]、GlowV と V/F pair 連動 dirty trigger [要 verify]、horizontal/vertical 2 pass で同値共有 (= 2 pass 間 setter 再呼出不要、1 回設定で 2 dispatch 共有)、glow chain 第 2-3 段、cadence PerProgram 維持、工数 group 全体 M 内
**関連**: L0-1 dispatch (= 衝突なし binding=18) / §3.5.11 sub-cluster (b) GlowV (= V/F pair 連動 dirty、horizontal/vertical 2 pass で GlowV のみ切替本 UBO は共有) / sub-cluster (a) GlowExtract (= 第 1 段) / sub-cluster (c) GlowCombine (= 第 4 段)
