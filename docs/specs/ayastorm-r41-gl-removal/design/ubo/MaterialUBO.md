# MaterialUBO — UBO design (= 実コードベース調査資料)

**通電状態**: shell 通電済 + per-program write 経路通電済 (= Phase 1.A PA-8 blueprint 起案 + 1.C PC-7γ-1 で `sProgramUboDirty` map allocate + `writeProgramUbo` PER_PROGRAM case 通電、実 shader UBO block は `class1/deferred/pbropaqueF.glsl:44` + 3 PBR-extended site (= pbropaqueV / pbralphaV / class2/pbralphaF) で literal extract 確認)

**本実装化に必要な作業**: 既存 PBR material setter 群 (= TEXTURE_MATRIX0 / TEXTURE_BASE_COLOR_TRANSFORM / TEXTURE_EMISSIVE_TRANSFORM / metallicFactor / roughnessFactor / emissiveColor / DIFFUSE_COLOR 等) の forwardToUboUpload PER_PROGRAM 経由 UBO redirect 通電 + per-shader UBO block 宣言拡大 (= 現 6-member base shader 45+ file → 10-member full 化または layout-compat 慣用での view)

---

## §1. UBO identity

- **block_name**: `MaterialUBO`
- **block_hash**: `0x220a6028u` (= `ubo_metadata.inl:52` literal)
- **block_size**: 256 B (= std140=176, device-padded=256、`ubo_layout_materialubo.inl:24` literal)
- **member_count**: 10
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_materialubo.inl
struct MaterialUBOLayout {
    static constexpr std::uint32_t texture_matrix0_OFFSET               = 0u;   // size=64 align=16
    static constexpr std::uint32_t texture_base_color_transform_OFFSET  = 64u;  // size=32 align=16 stride=16
    static constexpr std::uint32_t texture_emissive_transform_OFFSET    = 96u;  // size=32 align=16 stride=16
    static constexpr std::uint32_t color_OFFSET                         = 128u; // size=16 align=16
    static constexpr std::uint32_t emissiveColor_OFFSET                 = 144u; // size=12 align=16
    static constexpr std::uint32_t _pad_emissive_OFFSET                 = 156u; // size=4  align=4
    static constexpr std::uint32_t metallicFactor_OFFSET                = 160u; // size=4  align=4
    static constexpr std::uint32_t roughnessFactor_OFFSET               = 164u; // size=4  align=4
    static constexpr std::uint32_t _pad_material0_OFFSET                = 168u; // size=4  align=4
    static constexpr std::uint32_t _pad_material1_OFFSET                = 172u; // size=4  align=4
};
inline constexpr std::uint32_t MaterialUBO_SIZE = 256u; // std140=176, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set1/material_ubo.glsl
layout(std140, set = 1, binding = 0) uniform MaterialUBO
{
    mat4  texture_matrix0;
    vec4  texture_base_color_transform[2];
    vec4  texture_emissive_transform[2];
    vec4  color;
    vec3  emissiveColor;
    float _pad_emissive;
    float metallicFactor;
    float roughnessFactor;
    float _pad_material0;
    float _pad_material1;
};
```

= **PBR-extended 10-member canonical form** (= AYA option (I) 採用)、blueprint comment literal: "46 file 中、4 PBR site が 10-member、残り 45 site が 6-member base のみを宣言。AYA option (I) 採用 = 10-member full を canonical blueprint とする = host C++ 側 allocate は full size、6-member shader は trailing 4 member 未参照 view (= Vulkan std140 layout-compat 慣用: larger buffer に smaller block view 合法、既存 OpenGL 動作で proven、shader 改修ゼロ堅持)。"

---

## §2. binding 配線

- **descriptor_set**: 1
- **binding**: 0
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **set 1 構成** (= set 1 = Material 専用):
  - set=1 binding=0 = **MaterialUBO (本 UBO、10-member PBR full)**
  - set=1 binding=0 (別 hash) = MaterialUBO_Legacy (= 6-member base、`ubo_metadata.inl:53` literal)
- **set 1 内 binding=0 重複の解釈**: 不明 / verify 要 = `ubo_metadata.inl:52-53` で `MaterialUBO` と `MaterialUBO_Legacy` 両者 set=1 binding=0、これは異なる shader program で互いに排他選択 (= shader 単位で 6/10 member 切替) を意味する想定、verify 要
- **source**: `ubo_metadata.inl:52` `{ "MaterialUBO", 0x220a6028u, 256u, 1u, 0u, 0u, 1u, 10u }` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program** (= `llglslshader.cpp:95` literal: `constexpr U32 kCadencePerProgram = 1u; // ubo_metadata.inl で 88 件最大`)
- **flush 経路**: `LLVKLoader::flushProgramUbos(shader)` (= `llvkloader.cpp:5142`)
- **write 経路**: `LLVKLoader::writeProgramUbo(shader, block_hash, offset, data, size)` (= `llglslshader.cpp:2148` literal)
- **storage**: `sProgramUboDirty` map<UboInstanceKey, UboInstance> (= shader × block_hash key、`llvkloader.cpp:576` literal)

---

## §4. 物理 owner

- **owner**: PBR material rendering 経路 (= `LLDrawPoolPBR*` / `LLGLTFMaterial` 経由、verify 要)
- **data source**:
  - `texture_matrix0` = legacy texture matrix (= `LLShaderMgr::TEXTURE_MATRIX0` 経由、`llrender.cpp:997` literal)
  - `texture_base_color_transform[2]` / `texture_emissive_transform[2]` = GLTF transform (= `llshadermgr.h:58,62` literal: `TEXTURE_BASE_COLOR_TRANSFORM` / `TEXTURE_EMISSIVE_TRANSFORM`)
  - `color` = base diffuse color (= `LLShaderMgr::DIFFUSE_COLOR` 経由、verify 要)
  - `emissiveColor` / `metallicFactor` / `roughnessFactor` = PBR factor (= `llshadermgr.h:97,98,99` literal: `EMISSIVE_COLOR` / `METALLIC_FACTOR` / `ROUGHNESS_FACTOR`)
- **lifetime**: per-program (= 各 material バインド時に確定、program 切替で再 set)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set1/material_ubo.glsl` (= Phase 1.A PA-8 起案、10-member PBR full)
- **blueprint origin (literal extract source)**: `class1/deferred/pbropaqueF.glsl:44` (= `material_ubo.glsl:4` literal comment、4 PBR-extended sample sites = pbropaqueF / pbropaqueV / pbralphaV / class2/pbralphaF)
- **実 shader use site** (= grep result 52 file で UBO block 宣言):
  - **10-member PBR full** (4 file): `class1/deferred/pbropaqueF.glsl` + `pbropaqueV.glsl` + `pbralphaV.glsl` + `class2/deferred/pbralphaF.glsl`
  - **6-member base** (45+ file): 残 deferred / object / interface / avatar shader 全 (= 例 `class1/objects/simpleNoAtmosV.glsl` / `class1/avatar/avatarV.glsl` 等)
- **base 6 member**:
  - texture_matrix0 / texture_base_color_transform / texture_emissive_transform / color / emissiveColor / (= 推定、verify 要 = 6 member 内訳特定要)
- **layout-compat 慣用**: larger buffer (10 member 256 B allocate) に smaller block view (6 member) → trailing 4 member (metallicFactor / roughnessFactor / _pad_material0 / _pad_material1) 未参照
- **raw uniform 並走 pattern** (= `#else` block):
  - `class2/deferred/pbralphaF.glsl:61-65` literal: `uniform float metallicFactor; uniform float roughnessFactor; uniform vec3 emissiveColor;`
  - = OpenGL path で raw uniform 維持

---

## §6. 既存 setter call site (host C++)

- **`metallicFactor` / `roughnessFactor` setter**: `llmaterialeditor.cpp:2686-2687` literal: `setMetalnessFactor((F32)material_in.pbrMetallicRoughness.metallicFactor); setRoughnessFactor((F32)material_in.pbrMetallicRoughness.roughnessFactor);` (= material edit 経路、shader uniform 直接 setter は別 site)
- **GLTF asset load**: `gltf/asset.cpp:1346-1347, 1358-1359` literal: `copy(src, "metallicFactor", mMetallicFactor); copy(src, "roughnessFactor", mRoughnessFactor); write(mMetallicFactor, "metallicFactor", dst, 1.f); write(mRoughnessFactor, "roughnessFactor", dst, 1.f);` (= GLTF JSON ↔ struct conversion、shader uniform 直接 setter は別 site)
- **shader uniform setter**: 不明 / verify 要 (= `LLDrawPoolPBR*` / `LLViewerObject` の draw 経路で `shader->uniform1f(METALLIC_FACTOR, ...)` 等の setter call site grep 要)
- **`texture_matrix0` setter**: `LLShaderMgr::TEXTURE_MATRIX0` 経由、`llrender.cpp:997` literal で `syncMatrices` 内 texture matrix sync (= per-program cadence、frame 内 program 切替で再 set)
- **`texture_base_color_transform` / `texture_emissive_transform` setter**: 不明 / verify 要 (= GLTF transform setter call site)
- **`color` setter**: 不明 / verify 要 (= `LLShaderMgr::DIFFUSE_COLOR` 経由想定)
- **共通 redirect 経路**: `LLGLSLShader::uniformMatrix4fv` / `uniform4fv` / `uniform3fv` / `uniform1f` (= 既 PC-7γ-1 で UBO redirect 通電)
- **PER_PROGRAM case**: `forwardToUboUpload` 内 `case kCadencePerProgram: LLVKLoader::writeProgramUbo(this, loc.block_hash, loc.offset, data, size);` (= `llglslshader.cpp:2147-2149`)

---

## §7. 現状通電状態

- **状態**: **shell 通電済 + write 経路本格化済** (= Phase 1.A PA-8 + 1.C PC-7γ-1)
- **bind 経路**: set=1 binding=0 で program 切替時に bind (= per-program cadence、verify 要 = set 1 descriptor set allocate site)
- **write 経路**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo` → memcpy + dirty.store(true) (= shader × block_hash key、`llvkloader.cpp:576` literal)
- **MUSEUBO-A 整合**: `mUseUBO=false` default で本 entry 不到達

---

## §8. 本実装化に必要な作業

1. **`mUseUBO=true` cold launch 検証**:
   - PBR draw 経路 (= `LLDrawPoolPBR*::render`) で 10 member 全 writeProgramUbo 実走確認
   - 6-member base shader での layout-compat 慣用動作確認 (= trailing 4 member 未参照で validation layer warning 0)
2. **per-shader UBO consume 拡大**:
   - 現 4 file (= PBR-extended) で UBO block 宣言済
   - 残 45+ file (= 6-member base) に UBO block 宣言追加 = layout-compat 慣用で 6-member view (= AYA option (I) 採用)
3. **setter site 完全特定**:
   - `LLShaderMgr::METALLIC_FACTOR` / `ROUGHNESS_FACTOR` / `EMISSIVE_COLOR` / `DIFFUSE_COLOR` / `TEXTURE_BASE_COLOR_TRANSFORM` / `TEXTURE_EMISSIVE_TRANSFORM` の setter call site grep 要
   - `LLDrawPoolPBR*` / `LLDrawPoolMaterial*` / `LLViewerObject::renderPbr*` 内の draw-time uniform setter 経路特定要
4. **set 1 binding=0 重複の解釈**:
   - `MaterialUBO` (= 10-member、本 UBO) と `MaterialUBO_Legacy` (= 6-member、`ubo_metadata.inl:53`) が同 set=1 binding=0
   - shader 単位での選択排他 (= 同 shader 内に両 block_hash 宣言は禁止) 想定、verify 要
5. **codegen 再実行不要**:
   - blueprint member 不変、layout 改変なし

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set 1 = Material 専用、Phase 1.A 確立) |
| OS-3 | std140 padding 厳守 | ✅ 176 B std140 → 256 B padded |
| OS-4 | minUniformBufferOffsetAlignment | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ⚠️ **45+ file の base shader に UBO block 宣言追加が本実装化必須**、layout-compat 慣用で 6-member view (= AYA option (I))、`#ifdef LL_VULKAN_GLSL` gate で OpenGL 100% 維持 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ **layout-compat 6-member view で trailing 4 member 未参照、Vulkan validation layer の挙動 verify 必須** (= `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` で smaller block view 合法性) |

**layout-compat 慣用 risk**:
- larger buffer (10 member 256 B) に smaller block view (6 member) = Vulkan spec で UBO はrange 指定可、shader 宣言 block と allocate buffer の size 不一致は spec で許容 (= `VkDescriptorBufferInfo::range` で view 範囲指定)
- 既存 OpenGL で同 pattern (= 6-member shader 用に 10-member buffer allocate) が動作する根拠 = blueprint comment literal「既存 OpenGL 動作で proven」
- validation layer warning 出ないかは Phase 2 cold launch 時 verify 必須

**set 1 専用設計**:
- set 1 = Material 専用、Phase 1.A 確立 (= memory `project_r41_phase1b_vulkan_host_gate` 5-set V3a layout)
- 他 set との混在なし、bind timing は per-program 切替時

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=1、Material 専用)

- **MaterialUBO (set=1 binding=0、本 UBO、10-member PBR full)**
- MaterialUBO_Legacy (set=1 binding=0、6-member base、`ubo_metadata.inl:53` literal、shader 単位排他選択)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 per-program、`flushProgramUbos` 共通経路)

- ubo_metadata.inl で cadence_tag=1 は 88 件中最大 (= `llglslshader.cpp:95` literal)
- 全 Legacy UBO + PerProgramUBO_* + 本 UBO + MaterialUBO_Legacy が同 cluster

### §11.3 同 shader consume UBO

- PBR draw 経路で同時 consume = FrameViewProj + FrameLights + FrameAtmosphere_Lighting (= 全 vertex shader で modelview + light 計算経由)
- + Global_ReflectionProbes (= PBR IBL 経路、reflection probe consume)
- + Skin_GLTFJoints (= rigged PBR、`ubo_metadata.inl:105` literal)
- + Asset_GLTFMaterials / Asset_GLTFNodes (= GLTF asset cadence)

### §11.4 同 data source UBO

- **同 owner**: `LLGLTFMaterial` / PBR draw 経路
- 同 data source = `MaterialUBO_Legacy` (= 6-member subset)、`Asset_GLTFMaterials` (= GLTF asset 単位 material array)

### §11.5 dirty 連動 UBO

- material 切替時 (= program bind 時) に本 UBO 単独 dirty
- 連動 = Texture sampler 切替 (= サンプラー記述は別 UBO でなく descriptor set 直接 bind)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- program 切替時に set=1 bind = `vkCmdBindDescriptorSets` で set=1 を含む subset rebind
- set=0 (Frame) は frame start 1 回 bind、set=1 (Material) は program 切替毎 bind

---

## §10. 不明事項

1. **set 1 binding=0 重複解釈** = `MaterialUBO` (10-member) vs `MaterialUBO_Legacy` (6-member) が同 set=1 binding=0、shader 単位選択排他想定、`llvkloader.cpp` 内 set 1 allocate 経路 verify 要
2. **shader uniform setter call site** = `LLShaderMgr::METALLIC_FACTOR` / `ROUGHNESS_FACTOR` / `EMISSIVE_COLOR` / `DIFFUSE_COLOR` / `TEXTURE_BASE_COLOR_TRANSFORM` / `TEXTURE_EMISSIVE_TRANSFORM` の draw-time setter call site (= `LLDrawPoolPBR*` 内)
3. **base 6 member の内訳** = 6-member base shader 45+ file が宣言する 6 member 具体 (= 推定 texture_matrix0 + texture_base_color_transform + texture_emissive_transform + color + emissiveColor + ?、verify 要)
4. **layout-compat 慣用の validation layer 挙動** = smaller block view (6-member) at larger buffer (10-member) の Vulkan validation 出力、Phase 2 cold launch verify
5. **`texture_matrix0` per-program cadence 整合** = `syncMatrices` 内 texture matrix sync は per-program 切替時、本 UBO 内配置整合性 verify 要
6. **GLTF asset 連動** = `Asset_GLTFMaterials` (= per-asset cadence) との data source 重複、本 UBO は per-program で per-asset と diff 経路の整理要
7. **per-shader UBO block 拡大対象 file 全列挙** = grep result 52 file で UBO block 宣言済、残 file の状況確認要

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.5 同期)

**Layer**: L4-5 (= C 判定 GLTF texture transform 3 UBO program 識別 dispatch group)
**status**: **起案済 (shell + write 通電済 = Phase 1.A/1.C 完了)** (= 2026-06-06 C-6、設計・工程 doc 化完了、本 group 中唯一通電済 UBO)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.5` (= single source of truth)
**要点**: GLTF texture transform 3 UBO + 1 bare local (= PbrOpaqueV + PbrAlphaV + MaterialUBO + pbrmetallicroughnessV bare)、本 UBO 10-member PBR full canonical (= AYA option (I))、base_color/emissive offset (=64/96) 本 group で扱う + normal/metallic-roughness は PbrOpaque/PbrAlpha 経由、set=1 binding=0 排他 (§3.5.1 MaterialUBO_Legacy と program 単位選択排他)、45+ base shader 拡大 (L0-3 依存)、layout-compat 6-member view 合法性 [要 Phase 2 cold launch verify]、工数 L (group 全体)、AYA live verify (= PBR 描画 + factor + transform、visual regression ゼロ §5.4)
**関連**: L0-1 dispatch (= set=1 排他切替 PBR-extended ↔ base) / L0-3 per-shader 拡大 (= 45+ base shader 対象) / L0-4 cadence 再評価 / §3.5.1 MaterialUBO_Legacy (= 排他切替) / §3.5.14 Asset_GLTFMaterials (= GLTF asset 連動) / §5.4 visual regression policy
