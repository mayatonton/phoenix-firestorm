# IrradianceGenFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: shell 通電済 + per-program write 経路通電済 (= Phase 1.A PA-8 + 1.C PC-7γ-1、blueprint 宣言済、実 shader UBO block は `class2/interface/irradianceGenF.glsl:42` 内 `#ifdef LL_VULKAN_GLSL` block で literal extract 確認)

**本実装化に必要な作業**: 既存 `sourceIdx` setter (= `llreflectionmapmanager.cpp:977` literal `gIrradianceGenProgram.uniform1i(sSourceIdx, sourceIdx);`) + `max_probe_lod` setter (= `llshadermgr.cpp:1834` literal で reserved 宣言確認、実 setter call は別 site 想定 verify 要) の forwardToUboUpload PER_PROGRAM 経路 UBO redirect 通電

---

## §1. UBO identity

- **block_name**: `IrradianceGenFParamUBO_Legacy`
- **block_hash**: `0xbdd9ded2u` (= `ubo_metadata.inl:50` literal)
- **block_size**: 256 B (= std140=16, device-padded=256、`ubo_layout_irradiancegenfparamubo_legacy.inl:18` literal)
- **member_count**: 4
- **struct definition**:

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_irradiancegenfparamubo_legacy.inl
struct IrradianceGenFParamUBO_LegacyLayout {
    static constexpr std::uint32_t sourceIdx_OFFSET                       = 0u;  // size=4 align=4
    static constexpr std::uint32_t max_probe_lod_OFFSET                   = 4u;  // size=4 align=4
    static constexpr std::uint32_t _pad_irradiance_gen_f_legacy_0_OFFSET  = 8u;  // size=4 align=4
    static constexpr std::uint32_t _pad_irradiance_gen_f_legacy_1_OFFSET  = 12u; // size=4 align=4
};
inline constexpr std::uint32_t IrradianceGenFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/irradiance_gen_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 52) uniform IrradianceGenFParamUBO_Legacy
{
    int   sourceIdx;
    float max_probe_lod;
    float _pad_irradiance_gen_f_legacy_0;
    float _pad_irradiance_gen_f_legacy_1;
};
```

= **4 member (2 active + 2 pad)、int + float ×3 = 16 B → 256 B padded**

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 52
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:50` `{ "IrradianceGenFParamUBO_Legacy", 0xbdd9ded2u, 256u, 3u, 52u, 0u, 1u, 4u }` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program**
- **flush 経路**: `LLVKLoader::flushProgramUbos`
- **write 経路**: `LLVKLoader::writeProgramUbo`
- **storage**: `sProgramUboDirty`

---

## §4. 物理 owner

- **owner**: `LLReflectionMapManager::doProbeUpdate` irradiance generation pass (= `llreflectionmapmanager.cpp:920-977` literal、`gIrradianceGenProgram` 経路)
- **data source**:
  - `sourceIdx` = reflection probe index (= `llreflectionmapmanager.cpp:810-814, 977` literal: `S32 sourceIdx = mReflectionProbeCount; sourceIdx += 1; gIrradianceGenProgram.uniform1i(sSourceIdx, sourceIdx);`)
  - `max_probe_lod` = LOD clamp (= `class2/interface/irradianceGenF.glsl:215` literal: `lod = clamp(lod, 0, max_probe_lod);`、derive 元 verify 要)
- **lifetime**: per-program (= irradiance gen pass bind 時)

---

## §5. use site (shader)

- **blueprint file**: `set3/irradiance_gen_f_param_ubo_legacy.glsl`
- **blueprint origin**: `class2/interface/irradianceGenF.glsl:42` (= literal extract source)
- **実 shader use site**: `class2/interface/irradianceGenF.glsl` (= grep result 唯一)
- **consume detail**:
  - `class2/interface/irradianceGenF.glsl:215` literal: `lod = clamp(lod, 0, max_probe_lod);`
  - `class2/interface/irradianceGenF.glsl:217` literal: `vec4 lambertian = textureLod(reflectionProbes, vec4(H, sourceIdx), lod);`
- **raw uniform 並走** (`#else` block):
  - `class2/interface/irradianceGenF.glsl:49,51` literal: `uniform int sourceIdx; uniform float max_probe_lod;`

---

## §6. 既存 setter call site (host C++)

- **`sourceIdx` setter**: `llreflectionmapmanager.cpp:977` literal: `gIrradianceGenProgram.uniform1i(sSourceIdx, sourceIdx);` (= via `static LLStaticHashedString sSourceIdx("sourceIdx");` `:920` literal)
- **`sourceIdx` 他 program 用 setter**:
  - `llreflectionmapmanager.cpp:930` literal: `gRadianceGenProgram.uniform1i(sSourceIdx, sourceIdx);` (= 別 program、`RadianceGenFParamUBO_Legacy` 用)
  - `llheroprobemanager.cpp:456` literal: `gHeroRadianceGenProgram.uniform1i(sSourceIdx, sourceIdx);` (= 別 program)
- **`max_probe_lod` setter**: 不明 / verify 要 (= `llshadermgr.cpp:1834` literal `mReservedUniforms.push_back("max_probe_lod");` 宣言確認、実 setter call は別 site 想定)
- **共通 redirect 経路**: `LLGLSLShader::uniform1i` / `uniform1f`
- **PER_PROGRAM case**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo`

---

## §7. 現状通電状態

- **状態**: shell 通電済 + write 経路本格化済
- **bind 経路**: set=3 binding=52 で program 切替時 bind
- **write 経路**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo`
- **MUSEUBO-A 整合**: `mUseUBO=false` default で不到達

---

## §8. 本実装化に必要な作業

1. **`mUseUBO=true` cold launch 検証**:
   - `llreflectionmapmanager.cpp:977` setter call から forwardToUboUpload PER_PROGRAM 経由 writeProgramUbo 実走確認
   - reflection probe regenerate path で probe count loop の sourceIdx 更新ごとに UBO write
2. **`max_probe_lod` setter 完全特定**:
   - reserved 宣言済、実 setter call site grep 要 (= `LLReflectionMapManager` 内候補)
3. **per-probe UBO update 経路**:
   - probe count 分の dispatch 回数 = 各 dispatch 前に sourceIdx 更新 → writeProgramUbo + flushProgramUbos
   - = pass 内 N 回 GPU upload (= per-probe upload)、perf verify 要
4. **codegen 再実行不要**: blueprint member 不変

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ int+float+pad×2 = 16 B std140 → 256 B padded |
| OS-4 | minUniformBufferOffsetAlignment | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ irradianceGenF で UBO block 宣言済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**per-probe upload risk**:
- probe count loop で sourceIdx 更新 → UBO write → dispatch
- = N probe × (write + flush + dispatch) で N 回 GPU upload
- 既存 OpenGL の uniform 更新と同 frequency、perf inversion なし想定 (= verify 要)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3、Legacy 帯)

- 同 set=3 = ~58 UBO

### §11.2 同 cadence cluster UBO (= cadence_tag=1 per-program)

- 全 Legacy UBO + 本 UBO

### §11.3 同 shader consume UBO

- `class2/interface/irradianceGenF.glsl` で同時 consume = vertex shader 由来 UBO + reflection probe texture sampler

### §11.4 同 data source UBO

- **同 owner**: `LLReflectionMapManager`
- 同 owner = `RadianceGenFParamUBO_Legacy` (= `ubo_metadata.inl:69` literal、同 manager の radiance gen pass、`llreflectionmapmanager.cpp:930` literal で sSourceIdx 共有)
- + `Global_ReflectionProbes` (= singleton 候、reflection probe global state)
- + `ReflectionProbeUBO_Legacy` (= `ubo_metadata.inl:72` literal、別経路想定)

### §11.5 dirty 連動 UBO

- reflection probe regenerate 時に本 UBO + RadianceGen + Gaussian (= reflection mip blur) + Global_ReflectionProbes 連動 dirty (= 推定、verify 要)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- reflection probe regenerate pass: gaussian blur → irradiance gen (本 UBO) → radiance gen (別 UBO) → 反映
- program bind 時に set=3 binding=52 更新 (= probe ごと N 回)

---

## §10. 不明事項

1. **`max_probe_lod` setter call site** = `llshadermgr.cpp:1834` reserved 宣言済、実 setter call site grep 要
2. **per-probe UBO update perf** = N probe × (write + flush + dispatch) 経路、既存 OpenGL との perf 比較
3. **`RadianceGenFParamUBO_Legacy` との関係** = 同 manager 由来、`sSourceIdx` 共有 + 異なる block_hash で 2 UBO 並走
4. **`Global_ReflectionProbes` との連動** = singleton 経路と per-program 経路の同時 dirty trigger 整理
5. **set 3 bind 単位** = program 切替時 set 3 全 binding rebind か個別 rebind か

= 上記 5 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.9 同期)

**Layer**: L4-9 sub-cluster (c) (= IBL mip pipeline 3 UBO)
**status**: **起案済** (= 2026-06-06 C-6-d、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.9` (= single source of truth)
**要点**: 2 active member + pad ×2 (= sourceIdx int + max_probe_lod float)、**shell 通電済 + write 経路本格化済** (= Phase 1.A PA-8 + 1.C PC-7γ-1、`mUseUBO=false` default で不到達)、setter sourceIdx 特定済 `llreflectionmapmanager.cpp:977` literal `gIrradianceGenProgram.uniform1i(sSourceIdx, sourceIdx);` (= sSourceIdx static 共有 RadianceGen と)、max_probe_lod setter 不明 (= `llshadermgr.cpp:1834` reserved 宣言確認、実 setter call 別 site 想定) [要追加調査]、probe count loop で sSourceIdx 共有 (= N probe × write + flush + dispatch)、irradianceGenF.glsl:42 singleton site、cadence PerProgram 維持、本 group 最先着手可、工数 group 全体 M-L 内 (S 部分)
**関連**: L0-1 dispatch (= 衝突なし binding=52) / §3.5.9 sub-cluster (c) RadianceGen (= sSourceIdx static 共有 sibling) / Gaussian (= 同 IBL pipeline 連動) / sub-cluster (b) ReflectionProbeUBO_Legacy (= max_probe_lod 同名共有候補)
