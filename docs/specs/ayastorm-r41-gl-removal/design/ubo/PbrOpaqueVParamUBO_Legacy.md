# PbrOpaqueVParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= blueprint + codegen metadata 生成済、host C++ register/write/flush 経路未着工)

**本実装化に必要な作業**: register/write/flush 経路新設 (= per-program cadence、`texture_normal_transform[2]` / `texture_metallic_roughness_transform[2]` setter を UBO write に redirect) + `llfetchedgltfmaterial.cpp` の `uniform4fv` 呼出 site を UBO 経由に切替

---

## §1. UBO identity

- **block_name**: `PbrOpaqueVParamUBO_Legacy`
- **block_hash**: `0xdbdabcc9u` (= FNV-1a("PbrOpaqueVParamUBO_Legacy"))
- **block_size**: 256 B (= std140 64 B、device-padded 256 B)
- **member_count**: 2 (= 2 vec4 array, each [2])
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_pbropaquevparamubo_legacy.inl:12-15
struct PbrOpaqueVParamUBO_LegacyLayout {
    static constexpr std::uint32_t texture_normal_transform_OFFSET = 0u;              // size=32 align=16 stride=16
    static constexpr std::uint32_t texture_metallic_roughness_transform_OFFSET = 32u; // size=32 align=16 stride=16
};
inline constexpr std::uint32_t PbrOpaqueVParamUBO_Legacy_SIZE = 256u; // std140=64, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/class1/deferred/pbropaqueV.glsl:88-91
layout(set=3, binding=53, std140) uniform PbrOpaqueVParamUBO_Legacy {
    vec4 texture_normal_transform[2];
    vec4 texture_metallic_roughness_transform[2];
};
```

= Blueprint (= `aya_r41_blueprints/set3/pbr_opaque_v_param_ubo_legacy.glsl:9-13`) 一致。

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 53
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: 不明 / verify 要
- **source**: `ubo_metadata.inl:62` `{ "PbrOpaqueVParamUBO_Legacy", 0xdbdabcc9u, 256u, 3u, 53u, 0u, 1u, 2u }` + `class1/deferred/pbropaqueV.glsl:88` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program** (= `llglslshader.cpp:95`)
- **意味詳細**: PBR opaque vertex shader の texture transform (= GLTF spec の KHR_texture_transform extension 由来)、material 単位だが program load 時に固定可能 (= 通常 material 切替で program reload なし、ただし material 切替で uniform 再 write 必須 = 実 cadence は per-draw 寄り、ただし metadata は per-program)
- **source**: ubo_metadata.inl + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **owner**: `LLFetchedGLTFMaterial::bind()` 等の GLTF material bind 経路 (= `indra/newview/llfetchedgltfmaterial.cpp:136-140` literal):
  ```cpp
  shader->uniform4fv(LLShaderMgr::TEXTURE_NORMAL_TRANSFORM, 2, (F32*)normal_packed);
  shader->uniform4fv(LLShaderMgr::TEXTURE_METALLIC_ROUGHNESS_TRANSFORM, 2, (F32*)metallic_roughness_packed);
  ```
- **lifetime**: per-material/per-draw (= material 切替で再 write、ただし metadata は per-program cadence)
- **用途**: KHR_texture_transform = scale/offset/rotation を 2 vec4 で encode (= GLTF spec、`texture_normal_transform[0]` = scale + offset、`[1]` = rotation matrix 等、verify 要)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/pbr_opaque_v_param_ubo_legacy.glsl`
- **実 shader use site** = **`class1/deferred/pbropaqueV.glsl:88`** (= blueprint header literal)
- **同 member 名 別 shader 使用** (= grep 結果):
  - `class1/deferred/pbralphaV.glsl:99-100` (= 同 layout 宣言、別 UBO? = PerProgramUBO_PbrAlphaV (set=2 binding=11) 候補、verify 要)
  - `class1/gltf/pbrmetallicroughnessV.glsl:85-86` (= bare local 宣言、UBO なし)
- consume 内容 (= grep 確認済):
  - `pbropaqueV.glsl:195` `normal_texcoord = texture_transform(texcoord0, texture_normal_transform, texture_matrix0);`
  - `pbropaqueV.glsl:196` `metallic_roughness_texcoord = texture_transform(texcoord0, texture_metallic_roughness_transform, texture_matrix0);`
  - `pbropaqueV.glsl:209` `vec4 transformed_tangent = tangent_space_transform(vec4(t, tangent.w), n, texture_normal_transform, texture_matrix0);`

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: 未配線
- **既存 OpenGL 経路 setter**:
  - `indra/newview/llfetchedgltfmaterial.cpp:136` `shader->uniform4fv(LLShaderMgr::TEXTURE_NORMAL_TRANSFORM, 2, (F32*)normal_packed);`
  - `indra/newview/llfetchedgltfmaterial.cpp:140` `shader->uniform4fv(LLShaderMgr::TEXTURE_METALLIC_ROUGHNESS_TRANSFORM, 2, (F32*)metallic_roughness_packed);`
- **reserved uniform 登録**:
  - `indra/llrender/llshadermgr.cpp:1521` `mReservedUniforms.push_back("texture_normal_transform"); // (GLTF)`
  - `indra/llrender/llshadermgr.cpp:1522` `mReservedUniforms.push_back("texture_metallic_roughness_transform"); // (GLTF)`
- **enum entry**:
  - `indra/llrender/llshadermgr.h:59` `TEXTURE_NORMAL_TRANSFORM, // "texture_normal_transform" (GLTF)`
  - `indra/llrender/llshadermgr.h:60` `TEXTURE_METALLIC_ROUGHNESS_TRANSFORM, // "texture_metallic_roughness_transform" (GLTF)`

---

## §7. 現状通電状態

- **状態**: **untouched**
- **codegen 生成済**: layout `inl` + metadata entry + block_hash constexpr 生成済
- **blueprint 起案済**: `aya_r41_blueprints/set3/pbr_opaque_v_param_ubo_legacy.glsl`
- **shader 宣言済**: `class1/deferred/pbropaqueV.glsl:88` `#ifdef LL_VULKAN_GLSL` block
- **register/write/flush 経路**: 未配線

---

## §8. 本実装化に必要な作業

1. **register 経路**: `mapUniforms()` で PBR opaque V program に `registerProgramUbo(this, block_hash::PbrOpaqueVParamUBO_Legacy, 256u)` 呼出
2. **write 経路**: `llfetchedgltfmaterial.cpp:136/140` の `uniform4fv` を `forwardToUboUpload` 経由 UBO write に redirect
3. **flush 経路**: cmdbuf bind 経路で sProgramUboDirty を flush
4. **shader 接続**: shader 追加改変なし
5. **同 member pbralphaV / pbrmetallicroughnessV との分離**: pbralphaV (= 別 UBO PerProgramUBO_PbrAlphaV set=2 binding=11 候補) + pbrmetallicroughnessV (= bare local 計算、UBO 不経由) と本 UBO の使い分け確立要

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ 64B → 256B padded (vec4[2] stride=16, alignment 充足) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既に UBO 宣言済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**特記 risk**:
- 実 cadence vs metadata cadence 乖離: material 切替で uniform 再 write 必須 = per-draw 寄りだが metadata は per-program、本実装化で per-program triple-buffer で頻繁 write になる可能性 (= ring buffer 利用効率低下、PerDraw cadence への格上げ検討要)
- 同 member 名 (= `texture_normal_transform` / `texture_metallic_roughness_transform`) を別 shader (= pbralphaV / pbrmetallicroughnessV) でも使用 = host C++ 側 program 識別で正しい UBO に dispatch 必須
- pbrmetallicroughnessV は bare local 計算 (= `gltf_material_data` UBO 経由で derive) ゆえ本 UBO 経路と異なる data flow

---

## §10. 不明事項

1. **GLTF KHR_texture_transform encoding 詳細** = `vec4[2]` の中身 (= scale.xy, offset.xy / rotation 等) の正確な layout
2. **実 cadence: per-draw か per-program か** = material 切替頻度に依存、PerDraw cadence への格上げ検討要
3. **pbralphaV (PerProgramUBO_PbrAlphaV set=2 binding=11) との分離理由** = opaque/alpha で別 UBO に分離している設計意図 verify 要
4. **pbrmetallicroughnessV.glsl の data flow** = `gltf_material_data` UBO (= Asset_GLTFMaterials 候補) から derive、本 UBO 不経由の正確な確認要
5. **set=3 帯 layout の binding 上限** = V3a 設計整合性

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- set=3 帯 Legacy UBO 群と共存
- 直近 binding 同居: binding=52 IrradianceGenFParamUBO_Legacy / binding=54 AvatarFParamUBO_Legacy

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 73 件 (推定) per-program cluster 内の 1 UBO

### §11.3 同 shader consume UBO (= `class1/deferred/pbropaqueV.glsl` で同時 consume)

- 不明 / verify 要 (= pbropaqueV.glsl 全文の UBO 宣言群確認要、Frame UBO 群 + Skin UBO 群同時 consume の可能性大)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **PerProgramUBO_PbrAlphaV** (set=2 binding=11) = `texture_normal_transform` / `texture_metallic_roughness_transform` 同 member 名共有 = 同 LLFetchedGLTFMaterial data source 由来候補 = host 側 setter call site 共通可能性大
- **Asset_GLTFMaterials** (set=3 binding=1) = pbrmetallicroughnessV で `gltf_material_data` 経由 derive (= 同 GLTF material data の別経路)
- **MaterialUBO** (set=1 binding=0) = PBR 拡張 canonical (= `texture_base_color_transform[2]` / `texture_emissive_transform[2]` member、本 UBO と並列の texture transform 群)

### §11.5 dirty 連動 UBO

- material 切替時 = **PerProgramUBO_PbrAlphaV** + **MaterialUBO** (= PBR 系) 同時 dirty 化候補
- texture pack 切替時 = 全 GLTF texture transform UBO 同時 dirty 化候補

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- PBR opaque V program 切替時 set=3 帯全 binding を一括 rebind

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.5 同期)

**Layer**: L4-5 (= C 判定 GLTF texture transform 3 UBO program 識別 dispatch group)
**status**: **起案済** (= 2026-06-06 C-6、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.5` (= single source of truth)
**要点**: GLTF texture transform 3 UBO + 1 bare local (= PbrOpaqueV + PbrAlphaV + MaterialUBO + pbrmetallicroughnessV bare)、本 UBO pbropaqueV program 専用 (set=3 binding=53)、`texture_normal_transform[2]`/`texture_metallic_roughness_transform[2]` (offset=0/32)、setter 全特定済 (`llfetchedgltfmaterial.cpp:136-140`)、cadence material 切替 per-draw 寄り → PerDraw 降格候補 [要 L0-4 結果反映]、工数 L (group 全体)、AYA live verify (= PBR opaque V 描画、visual regression ゼロ §5.4)
**関連**: L0-1 dispatch (= pbropaqueV program 識別) / L0-4 cadence 再評価 (= PerDraw 降格) / PbrAlphaV (= 同 layout pair) / MaterialUBO (= GLTF transform 統合) / §3.5.14 Asset_GLTFMaterials (= bare local path 経由) / §5.4 visual regression policy


---

## §13. Phase 2.α 案 X 確定 record (= blueprint dir 位置付け + 二重 source 同期 protocol)

### §13.1 blueprint dir の位置付け = codegen 入力 source of truth

- **blueprint file** (= `aya_r41_blueprints/<set>/<ubo_lower>.glsl`) は本 UBO の **codegen 入力 source of truth** (= 案 X 確定 2026-06-06)。`indra/cmake/AyaUboCodegen.cmake` の `AYA_UBO_CODEGEN_BLUEPRINT_DIR` 経由で `scripts/ubo_codegen/main.py` の入力に渡され、`ubo_metadata.inl` + `ubo_layout_<ubo>.inl` を生成する。
- **AYAstorm shader runtime compile target は別 GLSL 系統** (= `class*/` + `cinematic_bd/` 配下の実 shader use site) で並列 build process (= design/04-codegen-ubo.md §2.2 literal「別 GLSL 並列 build process」)。
- 二系統は二重 source として共存し、**`scripts/ubo_codegen/main.py` の二重 source 同期 protocol で整合 verify** される (= §13.2)。
- 案 X 確定 source of truth = `docs/specs/ayastorm-r41-gl-removal/handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D.9
- blueprint dir 内 README = `indra/newview/app_settings/shaders/aya_r41_blueprints/README.md` (= phase B commit `f95182ded5`、位置付け literal source)

### §13.2 二重 source 同期 protocol (= main.py で formal化)

- **`_verify_block_match`** (= α-2 commit `b66ec99f72`) = 同名 UBO 複数 file (= blueprint + actual の cross-source pair、または cinematic_bd 上書き path) の set/binding + subset/cadence + member 全件 layout 一致を構造的 verify。不一致時 `CodegenError` で abort。
- **`_verify_blueprint_actual_consistency`** + **`--verify-target-paths`** option (= phase F commit `868bc38cc9`) = blueprint と actual の二重 source 整合 verify を formal化、`--verify-target-paths` で blueprint と actual を区別して対称的 cross-verify。
- 本 UBO の場合 = blueprint file (= §5 / §1 で記載) と実 shader use site (= §5 で記載) が **両 path で同一 layout (set/binding/member)** を保持する protocol。改修時は両方を同期書換するか、blueprint 側のみ書換後 codegen 再生成 + actual の `#ifdef LL_VULKAN_GLSL` block を手動同期する。
- sub-session 5 step 2-batch-0-a 7 UBO の同期書換 record = phase E commit `09ee5e8a8e` (= actual class*/ + cinematic_bd/ 14 file の新 set/binding を blueprint dir 内 7 UBO 7 file に同期反映、案 X 確定後の整合修復)

### §13.3 cross-ref

- 設計 doc = `design/04-codegen-ubo.md` §2.2 (= 別 GLSL 並列 build process) / §4.4 (= 同名 UBO 複数 GLSL 宣言の整合 verify)
- handoff doc = `handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D.9 (= 案 X 確定 source of truth、6 commit revert record + 改修方針 9 件)
- blueprint dir README = `indra/newview/app_settings/shaders/aya_r41_blueprints/README.md` (= phase B commit `f95182ded5`)
- 二重 source 同期 protocol formal化 = `scripts/ubo_codegen/main.py` `_verify_block_match` + `_verify_blueprint_actual_consistency`

