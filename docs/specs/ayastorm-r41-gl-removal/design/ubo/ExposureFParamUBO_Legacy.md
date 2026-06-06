# ExposureFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: shell 通電済 + per-program write 経路通電済 (= Phase 1.A PA-8 blueprint 起案 + 1.C PC-7γ-1 で `writeProgramUbo` PER_PROGRAM case 通電、blueprint 宣言済、実 shader UBO block は `class1/deferred/exposureF.glsl:49` 内 `#ifdef LL_VULKAN_GLSL` block で literal extract 確認)

**本実装化に必要な作業**: 既存 exposure setter (= `pipeline.cpp:8863-8865` literal `noiseVec` + `dynamic_exposure_params` + `dynamic_exposure_params2`) + `dt` setter (= `pipeline.cpp:8815` literal `LLStaticHashedString dt`) の forwardToUboUpload PER_PROGRAM 経由 UBO redirect 通電 + `mUseUBO=true` cold launch 検証

---

## §1. UBO identity

- **block_name**: `ExposureFParamUBO_Legacy`
- **block_hash**: `0x1e3d5c45u` (= `ubo_metadata.inl:39` literal)
- **block_size**: 256 B (= std140=48, device-padded=256、`ubo_layout_exposurefparamubo_legacy.inl:18` literal)
- **member_count**: 4
- **struct definition**:

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_exposurefparamubo_legacy.inl
struct ExposureFParamUBO_LegacyLayout {
    static constexpr std::uint32_t dt_OFFSET                       = 0u;   // size=4  align=4
    static constexpr std::uint32_t noiseVec_OFFSET                 = 8u;   // size=8  align=8
    static constexpr std::uint32_t dynamic_exposure_params_OFFSET  = 16u;  // size=16 align=16
    static constexpr std::uint32_t dynamic_exposure_params2_OFFSET = 32u;  // size=16 align=16
};
inline constexpr std::uint32_t ExposureFParamUBO_Legacy_SIZE = 256u; // std140=48, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/exposure_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 25) uniform ExposureFParamUBO_Legacy
{
    float dt;
    vec2 noiseVec;
    vec4 dynamic_exposure_params;
    vec4 dynamic_exposure_params2;
};
```

= **4 member、std140 で `float dt` (offset 0) + 4 B pad + `vec2 noiseVec` (offset 8) + `vec4 dynamic_exposure_params` (offset 16) + `vec4 dynamic_exposure_params2` (offset 32)、合計 48 B → 256 B device-padded**

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 25
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 構成**: Legacy + Asset + Skin (= `llvkloader.cpp` 内 set 3 帯 ~58 UBO)
- **source**: `ubo_metadata.inl:39` `{ "ExposureFParamUBO_Legacy", 0x1e3d5c45u, 256u, 3u, 25u, 0u, 1u, 4u }` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program** (= `llglslshader.cpp:95`)
- **flush 経路**: `LLVKLoader::flushProgramUbos(shader)` (= `llvkloader.cpp:5142`)
- **write 経路**: `LLVKLoader::writeProgramUbo` (= `llglslshader.cpp:2148`)
- **storage**: `sProgramUboDirty` map<UboInstanceKey, UboInstance> (shader × block_hash key)

---

## §4. 物理 owner

- **owner**: post-deferred exposure pass (= `pipeline.cpp:8791-8865` literal、`gExposureProgram` / `gExposureProgramNoFade` 経路)
- **data source**:
  - `dt` = frame time delta (= verify 要、`pipeline.cpp:8815` literal `LLStaticHashedString dt("dt")` 経由)
  - `noiseVec` = per-frame random offset (= `pipeline.cpp:8863` literal `shader->uniform2f(noiseVec, ll_frand() * 2.0f - 1.0f, ll_frand() * 2.0f - 1.0f);`)
  - `dynamic_exposure_params` = `(dynamic_exposure_coefficient, exp_min, exp_max, dynamic_exposure_speed_error)` (= `pipeline.cpp:8864` literal)
  - `dynamic_exposure_params2` = `(sky->getHDROffset(should_auto_adjust()), exp_min, exp_max, dynamic_exposure_speed_target)` (= `pipeline.cpp:8865` literal)
- **lifetime**: per-program (= exposure pass bind 時に確定)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/exposure_f_param_ubo_legacy.glsl` (= Phase 1.A PA-8 起案)
- **blueprint origin**: `class1/deferred/exposureF.glsl:49` (= literal extract source)
- **実 shader use site**: `class1/deferred/exposureF.glsl` (= grep result 唯一)
- **consume**: dynamic exposure pass の fragment shader、HDR exposure 計算

---

## §6. 既存 setter call site (host C++)

- **`dt` setter**: `pipeline.cpp:8815` literal `static LLStaticHashedString dt("dt");` 宣言済、実 setter call は不明 / verify 要 (= 同 block 内で `shader->uniform1f(dt, ...)` 形式想定、`pipeline.cpp:8815-8865` 範囲内)
- **`noiseVec` setter**: `pipeline.cpp:8863` literal `shader->uniform2f(noiseVec, ll_frand() * 2.0f - 1.0f, ll_frand() * 2.0f - 1.0f);`
- **`dynamic_exposure_params` setter**: `pipeline.cpp:8864` literal `shader->uniform4f(dynamic_exposure_params, dynamic_exposure_coefficient, exp_min, exp_max, dynamic_exposure_speed_error);`
- **`dynamic_exposure_params2` setter**: `pipeline.cpp:8865` literal `shader->uniform4f(dynamic_exposure_params2, sky->getHDROffset(should_auto_adjust()), exp_min, exp_max, dynamic_exposure_speed_target);`
- **共通 redirect 経路**: `LLGLSLShader::uniform1f` / `uniform2f` / `uniform4f` (= 既 PC-7γ-1 で UBO redirect 通電)
- **PER_PROGRAM case**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo` (= `llglslshader.cpp:2147-2149`)

---

## §7. 現状通電状態

- **状態**: shell 通電済 + write 経路本格化済
- **bind 経路**: set=3 binding=25 で program 切替時に bind (= per-program、verify 要)
- **write 経路**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo` → memcpy + dirty.store
- **MUSEUBO-A 整合**: `mUseUBO=false` default で本 entry 不到達

---

## §8. 本実装化に必要な作業

1. **`mUseUBO=true` cold launch 検証**:
   - `pipeline.cpp:8863-8865` の 3 setter call から forwardToUboUpload PER_PROGRAM 経由 writeProgramUbo 実走確認
   - `dt` setter 単独 verify (= `pipeline.cpp:8815` 宣言箇所からの実 setter call site 特定要)
2. **per-shader UBO consume 拡大**:
   - 現 1 file (= `class1/deferred/exposureF.glsl`) のみ宣言、追加なし
3. **setter site 完全特定**:
   - `dt` 実 setter call (= `shader->uniform1f(dt, ?)`) site grep 要 (= `pipeline.cpp:8815` LLStaticHashedString 宣言箇所からの飛び先)
4. **`dynamic_exposure_enabled` との関係**: `pipeline.cpp:8819` literal `LLStaticHashedString dynamic_exposure_e("dynamic_exposure_enabled");` 宣言あり、本 UBO member に含まれず (= 不明 / verify 要 = shader 内で別 uniform 経路 vs gating-only)
5. **codegen 再実行不要**: blueprint member 不変

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set 3 = Legacy 帯) |
| OS-3 | std140 padding 厳守 | ✅ float→pad→vec2→vec4×2 = 48 B std140 → 256 B padded |
| OS-4 | minUniformBufferOffsetAlignment | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既存 shader (= `exposureF.glsl:49`) で UBO block 宣言済、`#ifdef LL_VULKAN_GLSL` gate |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**per-frame に近い perceived cadence**:
- `noiseVec` = `ll_frand()` で毎 setter 呼出変化、`pipeline.cpp:8863` literal 経路は per-frame 呼出 (= exposure pass は frame 内 1 回)
- 本 UBO は per-program cadence (= cadence_tag=1) だが、setter call frequency は per-frame に近い
- write_program (= shader × block_hash key dirty) と flush 経路で frame 内 1 回 GPU upload で済む

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3、Legacy 帯)

- 同 set=3 = ~58 UBO (= Legacy 全 + Asset 3 + Skin 1)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 per-program)

- ubo_metadata.inl で cadence_tag=1 は **80 件** (= Phase 2.L0 sub-session 3 step 1 grep 確定、`llglslshader.cpp:95` source comment 「88 件最大」は historical literal)、本 UBO 含む

### §11.3 同 shader consume UBO

- `class1/deferred/exposureF.glsl` で同時 consume = FrameViewProj + FrameLights + FrameAtmosphere_Lighting (= 全 fragment shader で frame UBO consume 推定、verify 要)

### §11.4 同 data source UBO

- **同 owner**: `LLPipeline::renderPostProcess` exposure pass (= `pipeline.cpp:8791-8865` literal)
- 同 owner = `LuminanceFParamUBO_Legacy` (= 同 luminance / exposure pass、`pipeline.cpp:8729-8762` literal)

### §11.5 dirty 連動 UBO

- exposure pass bind 時に本 UBO 単独 dirty
- 連動 = `LuminanceFParamUBO_Legacy` (= 同 exposure pipeline 経路、tonemap chain)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- exposure pass program bind 時に set=3 全 ~58 UBO の binding 25 のみ更新 (= verify 要 = set 3 全 binding 同時 bind vs binding 単位再 bind)

---

## §10. 不明事項

1. **`dt` 実 setter call site** = `pipeline.cpp:8815` LLStaticHashedString 宣言からの実 setter call site grep 要
2. **`dynamic_exposure_enabled` 経路** = `pipeline.cpp:8819` 宣言、本 UBO member 不在ゆえ shader 内 raw uniform 経路 vs gating-only 判定要
3. **set 3 bind 単位** = program 切替時に set 3 全 ~58 UBO 同時 bind か binding 単位 rebind か (= `llvkloader.cpp` 内 set 3 bind 経路 verify 要)
4. **PER_PROGRAM dirty 判定単位** = `sProgramUboDirty` map key = `UboInstanceKey` (= shader × block_hash) と整合確認 (= `llvkloader.cpp:576` literal)
5. **per-frame に近い cadence の最適化** = `noiseVec` setter は per-frame だが本 UBO 全体は per-program、frame 内 1 回 GPU upload で済むか perf verify 要

= 上記 5 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.10 同期)

**Layer**: L4-10 sub-cluster (a) (= auto-exposure chain 2 UBO、Luminance → Exposure)
**status**: **起案済** (= 2026-06-06 C-6-e、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.10` (= single source of truth)
**要点**: 4 member (dt + noiseVec vec2 + dynamic_exposure_params vec4 + dynamic_exposure_params2 vec4)、**shell 通電済 + write 経路本格化済** (= PA-8 + PC-7γ-1)、setter 4 site (= `pipeline.cpp:8863-8865` literal noiseVec/dynamic_exposure_params/dynamic_exposure_params2 + `:8815` dt LLStaticHashedString)、`dt` 実 setter call site 不明 [要追加調査]、`dynamic_exposure_enabled` (= `:8819` 宣言、本 UBO 外、別経路 vs gating-only) [要 verify]、auto-exposure per-frame trigger で Luminance と同期 dirty、cadence PerProgram 維持 (= frame 内 exposure pass 1 回ゆえ frame 内 1 回 GPU upload で済む)、工数 group 全体 L 内
**関連**: L0-1 dispatch (= 衝突なし binding=25) / L0-2 LLStaticHashedString redirect (= dt 経由) / §3.5.10 sub-cluster (a) Luminance (= auto-exposure chain pair) / sub-cluster (b) Tonemap (= exposure data source 共有候補) / `pipeline.cpp:8791-8865` literal
