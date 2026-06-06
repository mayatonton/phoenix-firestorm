# GlobalFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: shell 通電済 + per-program write 経路通電済 (= Phase 1.A PA-8 + 1.C PC-7γ-1、blueprint 宣言済、実 shader UBO block は `class1/deferred/globalF.glsl:32` 内 `#ifdef LL_VULKAN_GLSL` block で literal extract 確認 + `class1/deferred/pbropaqueF.glsl` でも参照 grep result)

**本実装化に必要な作業**: 既存 `mirror_flag` + `clipSign` setter (= `llshadermgr.h:100,102` literal `MIRROR_FLAG` / `CLIP_SIGN` reserved enum、`llshadermgr.cpp:1572,1574` literal `mReservedUniforms.push_back("mirror_flag"); mReservedUniforms.push_back("clipSign");`) の forwardToUboUpload PER_PROGRAM 経路 UBO redirect 通電 + 実 setter call site 完全特定 (= 推定 `pipeline.cpp` 内 mirror pass setup 経路)

---

## §1. UBO identity

- **block_name**: `GlobalFParamUBO_Legacy`
- **block_hash**: `0xc8998fa7u` (= `ubo_metadata.inl:44` literal)
- **block_size**: 256 B (= std140=16, device-padded=256、`ubo_layout_globalfparamubo_legacy.inl:18` literal)
- **member_count**: 4
- **struct definition**:

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_globalfparamubo_legacy.inl
struct GlobalFParamUBO_LegacyLayout {
    static constexpr std::uint32_t mirror_flag_OFFSET        = 0u;  // size=4 align=4
    static constexpr std::uint32_t clipSign_OFFSET           = 4u;  // size=4 align=4
    static constexpr std::uint32_t _pad_globalf_0_OFFSET     = 8u;  // size=4 align=4
    static constexpr std::uint32_t _pad_globalf_1_OFFSET     = 12u; // size=4 align=4
};
inline constexpr std::uint32_t GlobalFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/global_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 11) uniform GlobalFParamUBO_Legacy
{
    float mirror_flag;
    float clipSign;
    float _pad_globalf_0;
    float _pad_globalf_1;
};
```

= **4 member (2 active + 2 pad)、std140 で float ×4 = 16 B → 256 B padded**

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 11
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:44` `{ "GlobalFParamUBO_Legacy", 0xc8998fa7u, 256u, 3u, 11u, 0u, 1u, 4u }` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program**
- **flush 経路**: `LLVKLoader::flushProgramUbos`
- **write 経路**: `LLVKLoader::writeProgramUbo`
- **storage**: `sProgramUboDirty`

---

## §4. 物理 owner

- **owner**: mirror pass / clip plane 管理 (= `LLPipeline::mHeroProbeManager.isMirrorPass()` 経路、`pipeline.cpp:3039` literal: `return (gPipeline.mHeroProbeManager.isMirrorPass()) ? false : ...`)
- **data source**:
  - `mirror_flag` = mirror pass 中フラグ (= `LLHeroProbeManager::isMirrorPass()` 由来、verify 要)
  - `clipSign` = clip plane sign (= reflection vs refraction pass で +/-1 切替、verify 要)
- **lifetime**: per-program (= mirror / clip pass 切替時 set)

---

## §5. use site (shader)

- **blueprint file**: `set3/global_f_param_ubo_legacy.glsl`
- **blueprint origin**: `class1/deferred/globalF.glsl:32` (= literal extract source)
- **実 shader use site**: `class1/deferred/globalF.glsl` (= blueprint origin) + `class1/deferred/pbropaqueF.glsl` (= grep result で `GlobalFParamUBO_Legacy` ヒット)
- **consume**: deferred lighting / PBR opaque で mirror/clip 判定 (= 反射時の clip plane test)

---

## §6. 既存 setter call site (host C++)

- **`mirror_flag` setter**: 不明 / verify 要 (= `llshadermgr.h:100` literal `MIRROR_FLAG` 経由、`llshadermgr.cpp:1572` literal で `mReservedUniforms.push_back("mirror_flag");` 宣言確認、実 setter call は `LLPipeline` 内 mirror pass setup 想定)
- **`clipSign` setter**: 不明 / verify 要 (= `llshadermgr.h:102` literal `CLIP_SIGN` 経由、`llshadermgr.cpp:1574` literal で reserved 宣言確認、実 setter call は reflection vs refraction pass setup 想定)
- **共通 redirect 経路**: `LLGLSLShader::uniform1f` (= 既 PC-7γ-1)
- **PER_PROGRAM case**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo`

---

## §7. 現状通電状態

- **状態**: shell 通電済 + write 経路本格化済
- **bind 経路**: set=3 binding=11 で program 切替時 bind
- **write 経路**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo`
- **MUSEUBO-A 整合**: `mUseUBO=false` default で不到達

---

## §8. 本実装化に必要な作業

1. **`mUseUBO=true` cold launch 検証**:
   - mirror pass 突入時の setter call で forwardToUboUpload PER_PROGRAM 経由 writeProgramUbo 実走確認
2. **setter site 完全特定**:
   - `MIRROR_FLAG` / `CLIP_SIGN` setter call site grep 要 = 推定 `pipeline.cpp` 内 `gPipeline.mHeroProbeManager.isMirrorPass()` 周辺
3. **per-shader UBO consume 拡大**:
   - 現 2 file (= globalF + pbropaqueF) で宣言、他 mirror pass 経由 shader 拡大要 (= deferred lighting 全般)
4. **codegen 再実行不要**: blueprint member 不変

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ float×4 = 16 B std140 → 256 B padded |
| OS-4 | minUniformBufferOffsetAlignment | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ globalF / pbropaqueF で UBO block 宣言済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**256 B 内 16 B active**:
- 4 member 中 2 active + 2 pad、Phase 2 で global state 追加候補 (= layout 不変前提)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3、Legacy 帯)

- 同 set=3 = ~58 UBO

### §11.2 同 cadence cluster UBO (= cadence_tag=1 per-program)

- 全 Legacy UBO + 本 UBO

### §11.3 同 shader consume UBO

- `class1/deferred/globalF.glsl` / `pbropaqueF.glsl` で同時 consume = FrameViewProj + FrameLights + FrameAtmosphere_Lighting + MaterialUBO

### §11.4 同 data source UBO

- **同 owner**: mirror / clip plane setup (= `LLHeroProbeManager`)
- 同 owner = `MoonFParamUBO_Legacy` / `SunDiscFParamUBO_Legacy` 等 sky 系で mirror pass 連動の可能性 (= verify 要)

### §11.5 dirty 連動 UBO

- mirror pass 突入時に本 UBO + reflection 関連 UBO 連動 dirty (= `Global_ReflectionProbes` / `ReflectionProbeUBO_Legacy` 連動の可能性、verify 要)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- mirror pass program bind 時に set=3 binding=11 更新

---

## §10. 不明事項

1. **`mirror_flag` / `clipSign` 実 setter call site** = `llshadermgr.h` reserved 宣言済、実 setter call site grep 要
2. **mirror pass setup 経路** = `LLHeroProbeManager::isMirrorPass()` から本 UBO setter までの flow 特定要
3. **per-shader UBO block 拡大対象** = mirror pass 経由 shader 全列挙 (= deferred lighting 全般想定)
4. **dirty 連動 detail** = `Global_ReflectionProbes` / `ReflectionProbeUBO_Legacy` との連動 dirty trigger
5. **clipSign vs ClipFParamUBO_Legacy** = 別 UBO (= `ubo_metadata.inl:34` `ClipFParamUBO_Legacy` set=3 binding=32) との clip plane 機能重複の整理 (= verify 要)

= 上記 5 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.4.20 同期)

**Layer**: L3-20 (= B Tier β setter 推定済、PerProgram cadence、mirror_flag/clipSign、**shell + write 経路通電済**)
**status**: **起案済** (= 2026-06-06 C-5、設計・工程 doc 化完了、shell + write 通電済 = Phase 1.A PA-8 + 1.C PC-7γ-1)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.4.20` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch + L0-4 cadence (= 既 shell + write 経路通電済、L1a-1 pilot 経路と同様 cold launch verify)
- **(2) 不明事項**: `MIRROR_FLAG` / `CLIP_SIGN` 実 setter call site (= `LLHeroProbeManager::isMirrorPass()` 経由推定) [要追加調査] / mirror pass setup 経路 [要 verify] / `clipSign` vs ClipFParamUBO_Legacy 機能重複整理 [要 AYA 判断]
- **(3) 調査手法**: D1 (`MIRROR_FLAG` / `CLIP_SIGN` setter grep + `LLHeroProbeManager` 内 mirror state grep) + D4 (ClipFParamUBO_Legacy との機能重複 verify)
- **(4) 設計 task**: L3 全件共通 (= PerProgram triple-buffer / `forwardToUboUpload` PER_PROGRAM / program bind 単位 flush / `globalF.glsl` + `pbropaqueF.glsl` LL_VULKAN_GLSL 活性化、既 shell + write 通電済ゆえ setter 行特定のみ)
- **(5) 工程**: trace L3-20 (= L3 締め、shell 通電済で cold launch verify のみ)、工数 **S** (= 既通電、setter 行特定のみ)、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= mirror pass + reflection clip 描画既存と同一、visual regression ゼロ §5.4) + ClipFParamUBO_Legacy 機能重複整理 (= AYA 判断)
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `globalF.glsl #else` block uniform 個別宣言維持

**関連**: L0-1 + L0-4 / §5.4 / L1a-1 ClipFParamUBO_Legacy (= clipSign 機能重複整理対象) / `LLHeroProbeManager` (= mirror pass dispatcher)
