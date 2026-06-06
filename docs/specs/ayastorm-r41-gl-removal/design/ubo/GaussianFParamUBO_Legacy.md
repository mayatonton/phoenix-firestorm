# GaussianFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: shell 通電済 + per-program write 経路通電済 (= Phase 1.A PA-8 blueprint 起案 + 1.C PC-7γ-1、blueprint 宣言済、実 shader UBO block は `class1/interface/gaussianF.glsl:46` 内 `#ifdef LL_VULKAN_GLSL` block で literal extract 確認)

**本実装化に必要な作業**: 既存 `resScale` + `direction` setter (= `LLShaderMgr::*` reserved uniform 経由、`llglslshader.cpp:1088` literal で reserved 宣言確認) の forwardToUboUpload PER_PROGRAM 経路 UBO redirect 通電 + 実 setter call site 完全特定

---

## §1. UBO identity

- **block_name**: `GaussianFParamUBO_Legacy`
- **block_hash**: `0xb4974e5fu` (= `ubo_metadata.inl:43` literal)
- **block_size**: 256 B (= std140=16, device-padded=256、`ubo_layout_gaussianfparamubo_legacy.inl:16` literal)
- **member_count**: 2
- **struct definition**:

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_gaussianfparamubo_legacy.inl
struct GaussianFParamUBO_LegacyLayout {
    static constexpr std::uint32_t resScale_OFFSET  = 0u; // size=4 align=4
    static constexpr std::uint32_t direction_OFFSET = 8u; // size=8 align=8
};
inline constexpr std::uint32_t GaussianFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/gaussian_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 58) uniform GaussianFParamUBO_Legacy
{
    float resScale;
    vec2 direction;          // std140 vec2 alignment 8 で resScale との間に 4 byte padding 自動挿入
};
```

= **2 member、std140 で `float resScale` (offset 0) + 4 B pad + `vec2 direction` (offset 8)、合計 16 B → 256 B device-padded** (= blueprint literal comment で padding 明記)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 58
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 構成**: Legacy 帯
- **source**: `ubo_metadata.inl:43` `{ "GaussianFParamUBO_Legacy", 0xb4974e5fu, 256u, 3u, 58u, 0u, 1u, 2u }` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program** (= `llglslshader.cpp:95`)
- **flush 経路**: `LLVKLoader::flushProgramUbos(shader)`
- **write 経路**: `LLVKLoader::writeProgramUbo`
- **storage**: `sProgramUboDirty`

---

## §4. 物理 owner

- **owner**: gaussian blur pass (= `gGaussianProgram`、`llviewershadermgr.cpp:86` literal `LLGLSLShader gGaussianProgram;` + `:3977` "Reflection Mip Shader" 用途)
- **data source**:
  - `resScale` = resolution scale factor (= verify 要、reflection mip 経路で使用)
  - `direction` = 2D blur direction (= 水平/垂直 separable gaussian blur 用)
- **lifetime**: per-program (= reflection mip 生成時に bind、verify 要)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/gaussian_f_param_ubo_legacy.glsl`
- **blueprint origin**: `class1/interface/gaussianF.glsl:46` (= literal extract source)
- **実 shader use site**: `class1/interface/gaussianF.glsl` (= grep result 唯一)
- **consume**: separable gaussian blur fragment shader (= reflection mip 用)、`gGaussianProgram` 使用 `interface/splattexturerectV.glsl` (vertex) + `interface/gaussianF.glsl` (fragment) (= `llviewershadermgr.cpp:3983-3984` literal)

---

## §6. 既存 setter call site (host C++)

- **`resScale` setter**: 不明 / verify 要 (= `llglslshader.cpp:1088` literal `"resScale"` reserved uniform 宣言確認、実 setter call は grep で `uniform1f.*resScale` または `LLShaderMgr::*RES_SCALE` hit せず)
- **`direction` setter**: 不明 / verify 要 (= `llglslshader.cpp:1082` literal `"direction"` reserved uniform 宣言確認、実 setter call は同様 hit せず = horizontal/vertical pass で setter call 想定)
- **gGaussianProgram bind site**: 不明 / verify 要 (= `gGaussianProgram.bind()` callsite 特定要、reflection probe regenerate path)
- **共通 redirect 経路**: `LLGLSLShader::uniform1f` / `uniform2f` (= 既 PC-7γ-1)
- **PER_PROGRAM case**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo`

---

## §7. 現状通電状態

- **状態**: shell 通電済 + write 経路本格化済
- **bind 経路**: set=3 binding=58 で program 切替時 bind
- **write 経路**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo`
- **MUSEUBO-A 整合**: `mUseUBO=false` default で不到達

---

## §8. 本実装化に必要な作業

1. **`mUseUBO=true` cold launch 検証**:
   - reflection mip 生成 (= `gGaussianProgram` bind) path で 2 member 全 writeProgramUbo 実走確認
2. **setter site 完全特定**:
   - `resScale` / `direction` setter call site grep 要 = 推定 `LLReflectionMapManager` 内 reflection mip generation 経路
3. **horizontal/vertical separable pass 経路**:
   - `direction` 経由で水平/垂直 2 pass dispatch 想定、setter 経路 verify 要
4. **codegen 再実行不要**: blueprint member 不変

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ float→4B pad→vec2 = 16 B std140 → 256 B padded、blueprint literal で padding 明記 |
| OS-4 | minUniformBufferOffsetAlignment | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既存 `gaussianF.glsl:46` で UBO block 宣言済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**最小 member UBO**:
- 2 member 16 B std140 → 256 B padded = 240 B 余剰 (= dead space)
- 同形 (= 2 member 256 B) UBO 多数 (= 全 Legacy UBO の最小 size 単位)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3、Legacy 帯)

- 同 set=3 = ~58 UBO

### §11.2 同 cadence cluster UBO (= cadence_tag=1 per-program)

- 全 Legacy UBO + 本 UBO

### §11.3 同 shader consume UBO

- `class1/interface/gaussianF.glsl` で同時 consume = vertex shader `splattexturerectV.glsl` 由来 UBO (= verify 要、推定 FrameViewProj)

### §11.4 同 data source UBO

- **同 owner**: `LLReflectionMapManager` reflection mip generation 経路 (= 推定)
- 同 owner = `IrradianceGenFParamUBO_Legacy` (= 同 reflection mip / irradiance 経路、`llreflectionmapmanager.cpp:977` literal `gIrradianceGenProgram.uniform1i(sSourceIdx, sourceIdx);`)
- + `RadianceGenFParamUBO_Legacy` (= `llreflectionmapmanager.cpp:930` literal)

### §11.5 dirty 連動 UBO

- reflection probe regenerate 時に本 UBO + Irradiance + Radiance 連動 dirty 想定

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- reflection mip generation pass で `gGaussianProgram` bind 時に set=3 binding=58 更新

---

## §10. 不明事項

1. **`resScale` / `direction` setter call site** = grep で hit せず、`LLReflectionMapManager` 内 reflection mip generation 経路要特定
2. **`gGaussianProgram` bind callsite** = reflection probe regenerate path 特定要
3. **horizontal/vertical separable pass dispatch 経路** = 2 pass 構造で `direction` setter 切替経路
4. **set 3 bind 単位** = program 切替時 set 3 全 binding rebind か個別 rebind か
5. **240 B dead space** = std140=16 B vs padded 256 B、Phase 2 で member 追加候補となるか (= 設計原則 (1) Upstream 取り込みやすさ維持の観点で固定推奨)

= 上記 5 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.9 同期)

**Layer**: L4-9 sub-cluster (c) (= IBL mip pipeline 3 UBO)
**status**: **起案済** (= 2026-06-06 C-6-d、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.9` (= single source of truth)
**要点**: 2 member (resScale float + direction vec2、std140 4B pad 自動挿入)、**shell 通電済 + write 経路本格化済** (= PA-8 + PC-7γ-1)、reserved 宣言 `llglslshader.cpp:1082/1088` literal `"direction"` / `"resScale"`、実 setter call grep hit せず (= `LLReflectionMapManager` 内 reflection mip generation 経路想定) [要追加調査]、`gGaussianProgram` bind callsite 不明 [要追加調査]、reflection mip blur (= separable gaussian、horizontal/vertical 2 pass) で direction 経由切替、mip chain loop で PerProgram cadence 多回 dirty → PerDraw cadence 化候補 [要 L0-4 結果反映 / 要 AYA 判断]、gaussianF.glsl:46 singleton site、240B dead space (= 最小 member UBO)、工数 group 全体 M-L 内
**関連**: L0-1 dispatch (= 衝突なし binding=58) / L0-4 cadence (= mip chain loop PerDraw 降格候補) / §3.5.9 sub-cluster (c) IrradianceGen/RadianceGen (= 同 IBL pipeline 連動) / `gGaussianProgram` (= reflection mip 用、`llviewershadermgr.cpp:3977` "Reflection Mip Shader")
