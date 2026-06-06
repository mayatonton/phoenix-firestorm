# PerProgramUBO_PbrAlphaV — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `per_program_ubo_pbr_alpha_v.glsl` は実 shader `class1/deferred/pbralphaV.glsl:98 ifdef LL_VULKAN_GLSL block` から literal extract 済 (= shader 側 UBO declaration 既存)。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_PbrAlphaV`
- **block_hash**: `0xacb2b335u` (= FNV-1a("PerProgramUBO_PbrAlphaV"))
- **block_size**: 256 B (= std140 64 B、device-padded 256 B)
- **member_count**: 2
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_pbralphav.inl
struct PerProgramUBO_PbrAlphaVLayout {
    static constexpr std::uint32_t texture_normal_transform_OFFSET = 0u;  // size=32 align=16 stride=16
    static constexpr std::uint32_t texture_metallic_roughness_transform_OFFSET = 32u;  // size=32 align=16 stride=16
};
inline constexpr std::uint32_t PerProgramUBO_PbrAlphaV_SIZE = 256u; // std140=64, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_pbr_alpha_v.glsl
layout(std140, set = 2, binding = 11) uniform PerProgramUBO_PbrAlphaV
{
    vec4 texture_normal_transform[2];
    vec4 texture_metallic_roughness_transform[2];
};
```

= 全 2 member (= vec4[2] × 2) 実 data slot 確定 (= blueprint comment literal: `Source: literal extract from class1/deferred/pbralphaV.glsl:98 ifdef LL_VULKAN_GLSL block (single site)`)。GLTF PBR material texture transform を 2x3 affine matrix で表現 (= vec4 × 2 packed)。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 11
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通、verify 要)
- **pipeline layout**: `sAYAStandardLayout` (= Phase 1.A 確立 5-set V3a layout)
- **set 2 配置**: PerDraw + PerProgram 帯 (= INDEX.md §B.4 set mapping)
- **source**: `ubo_metadata.inl:80` `{ "PerProgramUBO_PbrAlphaV", 0xacb2b335u, 256u, 2u, 11u, 0u, 1u, 2u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal: `constexpr U32 kCadencePerProgram = 1u`)
- **意味詳細**: program 切替時に flush、program 単位で値を保持 (= `forwardToUboUpload` switch case `kCadencePerProgram` 経路、`llglslshader.cpp:2147`)
- **特記**: GLTF material は本来 per-draw cadence 候補 (= material 単位で値が変わる)。**PerProgram にされている理由**は材質情報を program-binding scope で扱うため、material 切替時に再 flush する設計と推定 (= verify 要、`llfetchedgltfmaterial.cpp:136-140` setter pattern 参照)。
- **source**: ubo_metadata.inl:80 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電 (= shell も未配置)
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `LLFetchedGLTFMaterial` (= `llfetchedgltfmaterial.cpp:136` literal `shader->uniform4fv(LLShaderMgr::TEXTURE_NORMAL_TRANSFORM, 2, (F32*)normal_packed)` + `:140` `shader->uniform4fv(LLShaderMgr::TEXTURE_METALLIC_ROUGHNESS_TRANSFORM, 2, (F32*)metallic_roughness_packed)`)
  - GLTF material 内 texture transform 構造 (= UV scale + offset + rotation の 2x3 affine matrix を vec4 × 2 packed)
- **lifetime**: material bind ごと (= GLTF material 切替時に上書き、material が同一なら値据置)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_pbr_alpha_v.glsl` (= Phase 1.A PA-8 起案、2 member 実 data slot)
- **実 shader use site**:
  - **`class1/deferred/pbralphaV.glsl:98 ifdef LL_VULKAN_GLSL block`** (= single site、blueprint comment literal `Source: literal extract from class1/deferred/pbralphaV.glsl:98 ifdef LL_VULKAN_GLSL block (single site)`)
  - 既存 UBO block (pbralphaV.glsl:99-100):
    ```glsl
    vec4 texture_normal_transform[2];
    vec4 texture_metallic_roughness_transform[2];
    ```
  - 既存 OpenGL `#else` block (pbralphaV.glsl:104-105):
    ```glsl
    uniform vec4[2] texture_normal_transform;
    uniform vec4[2] texture_metallic_roughness_transform;
    ```
  - 使用箇所: `pbralphaV.glsl:211` (`normal_texcoord = texture_transform(texcoord0, texture_normal_transform, texture_matrix0)`) + `:212` (`metallic_roughness_texcoord = texture_transform(texcoord0, texture_metallic_roughness_transform, texture_matrix0)`) + `:225` (`tangent_space_transform(vec4(t, tangent.w), n, texture_normal_transform, texture_matrix0)`)
- **同 setter 他 shader use**:
  - `class1/gltf/pbrmetallicroughnessV.glsl` (= 同 transform 利用、別 program で同名 uniform 使用、別 UBO で対応の可能性、verify 要)
  - `class1/deferred/pbropaqueV.glsl` (= 同上、別 UBO = PbrOpaqueVParamUBO_Legacy の可能性、verify 要)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし (= untouched)
- **既存 OpenGL 経路 setter**:
  - `llshadermgr.cpp:1521-1522` で `mReservedUniforms.push_back("texture_normal_transform" / "texture_metallic_roughness_transform")` (= GLTF reserved uniform 登録)
  - `llshadermgr.h:59-60` `TEXTURE_NORMAL_TRANSFORM` + `TEXTURE_METALLIC_ROUGHNESS_TRANSFORM` (= reserved uniform enum)
  - 実 setter: **`llfetchedgltfmaterial.cpp:136`** `shader->uniform4fv(LLShaderMgr::TEXTURE_NORMAL_TRANSFORM, 2, (F32*)normal_packed)` + **`:140`** `shader->uniform4fv(LLShaderMgr::TEXTURE_METALLIC_ROUGHNESS_TRANSFORM, 2, (F32*)metallic_roughness_packed)`
- **本実装化後 setter** (= **不明 / verify 要**):
  - 31 setter 経路 (= mUseUBO 分岐) で UBO 化対応要 (= `forwardToUboUpload` redirect 経路、`llglslshader.cpp:2114-2151`)
  - `uniform4fv` count=2 (= vec4[2] 16 B × 2 = 32 B) で memcpy

---

## §7. 現状通電状態

- **状態**: **untouched** (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)
- **通電 commit**: なし (= 未通電)
- **通電内容**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 範囲、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電** (= Phase 2 着手時):
   - blueprint `per_program_ubo_pbr_alpha_v.glsl` ベースで host 側 buffer 配置 + descriptor set 配線
   - dummy buffer write + bind 経路通電
2. **実 member data 流入**:
   - 既存 OpenGL 経路 `llfetchedgltfmaterial.cpp:136-140` の `uniform4fv` 呼出 site を UBO 化 (= mUseUBO 分岐で UBO write、OpenGL では既経路温存)
3. **dirty 判定 logic 追加**:
   - PerProgram cadence ゆえ program 切替時に dirty (= 既経路 `forwardToUboUpload` `kCadencePerProgram` case 活用)
   - **特記**: GLTF material 切替は program 切替なしでも値変化、追加 dirty trigger 要 (= material bind 時 dirty flag set、verify 要)
4. **flush logic 追加**:
   - PerProgram cadence flush (= `writeProgramUbo` 経路、`llvkloader.cpp:5503` literal、verify 要)
5. **shader 接続**:
   - 実 shader `class1/deferred/pbralphaV.glsl:98 ifdef LL_VULKAN_GLSL block` で UBO declaration 既存 = 追加 shader 改変なし (= OS-5 充足)
   - 既存 OpenGL `#else` block (`:104-105`) uniform 個別宣言は温存 (= 設計原則 (1) Upstream 取り込みやすさ維持)
6. **関連 PBR UBO との重複整理**:
   - `PbrOpaqueVParamUBO_Legacy` (= set=3 binding=53) との data source 重複 (= 同 GLTF material 由来) 確認 (= verify 要、別 shader 別 UBO で同じ material data を duplicate 持ちの可能性)

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合 (= memory `project_r41_phase2_4_principles` 原則 2):

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 binding=11 配置 | ✅ 維持 (= Phase 1.A 確立 set 2 帯) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 64B → 256B padded (codegen 出力) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**GLTF material 切替 cadence 不整合 risk**: cadence_tag=1 (PerProgram) だが GLTF material は per-draw で切替 (= 同 shader program で複数 material 描画)。本 UBO を PerProgram で扱うと **material 切替の都度 program rebind 相当の処理が要**。実装時に per-draw dirty trigger を入れる必要があり、`writeProgramUbo` の意味論と矛盾する可能性 (= verify 要、Phase 2 で per-draw cadence 移行 (PerDrawUBO 化) を検討)。

**他 shader 共通 setter risk**: `TEXTURE_NORMAL_TRANSFORM` / `TEXTURE_METALLIC_ROUGHNESS_TRANSFORM` は `pbralphaV.glsl` 以外でも使用 (= `pbrmetallicroughnessV.glsl` / `pbropaqueV.glsl`)、それぞれ別 program 別 UBO で対応されているか確認要 (= UBO 重複 / 別 UBO migration 必要性検証)。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 = PerDraw + PerProgram 混在 (= INDEX.md §B.4 set mapping)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- ubo_metadata.inl で 73 件 (推定、INDEX.md §1) の最大 cluster

### §11.3 同 shader consume UBO (= class1/deferred/pbralphaV.glsl)

- **不明 / verify 要** = pbralphaV.glsl 内で他に consume される UBO (= FrameViewProj / FrameLights / FrameAtmosphere_Lighting / 関連 PBR transform UBO 等、grep verify 要)

### §11.4 同 data source UBO (= 同 host data source から派生)

- `PbrOpaqueVParamUBO_Legacy` (= set=3 binding=53、`ubo_metadata.inl:62`) と同 `LLFetchedGLTFMaterial` 由来候補 (= 同 GLTF transform を別 program (opaque) で消費の可能性、verify 要)
- `pbr_opaque_v_param_ubo_legacy.glsl` 内に同名 member 存在 (= grep 結果から確認)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- GLTF material 切替時、関連 PBR UBO (= MaterialUBO / MaterialUBO_Legacy / PbrOpaque*) と同時 dirty 候補 (= verify 要)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時に同時 bind (= `vkCmdBindDescriptorSets` set=2 帯)

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **GLTF material 切替時の dirty trigger** = `llfetchedgltfmaterial.cpp` の material bind 経路で本 UBO 用 dirty flag set 必要、具体実装 (= verify 要)
2. **PerProgram cadence 妥当性** = GLTF material 切替は per-draw 単位、PerProgram cadence で十分か、PerDraw cadence への移行が望ましいか (= verify 要、設計判断)
3. **同 setter 利用他 shader での UBO 化状況** = pbrmetallicroughnessV.glsl / pbropaqueV.glsl での同 transform UBO 化状況 (= grep verify 要)
4. **`uniform4fv count=2` の packing** = `(F32*)normal_packed` で渡される 32 B 値の memory layout が UBO 側 `vec4[2]` と一致するか (= verify 要、`llfetchedgltfmaterial.cpp` 内 packing logic 確認)
5. **shell 通電 commit** = 本 UBO 用 shell が今後配置される際の commit (= 未来作業、現時点不明)
6. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = `writeProgramUbo` 経路実装詳細 (= verify 要)
7. **`PbrOpaqueVParamUBO_Legacy` (set=3) との重複/関係性** = 別 UBO 別 set で同 GLTF transform を duplicate 持つか、片方が unused か (= verify 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消、確定後に「不明」記載削除 + 確定 literal 追記。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.5 同期)

**Layer**: L4-5 (= C 判定 GLTF texture transform 3 UBO program 識別 dispatch group)
**status**: **起案済** (= 2026-06-06 C-6、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.5` (= single source of truth)
**要点**: GLTF texture transform 3 UBO + 1 bare local (= PbrOpaqueV + PbrAlphaV + MaterialUBO + pbrmetallicroughnessV bare)、本 UBO pbralphaV program 専用 (set=2 binding=11)、`texture_normal_transform[2]`/`texture_metallic_roughness_transform[2]` (offset=0/32) PbrOpaqueV と同 layout、setter 全特定済 (`llfetchedgltfmaterial.cpp:136-140`)、material 切替 PerDraw 降格候補 [要 L0-4 結果反映]、工数 L (group 全体)、AYA live verify (= PBR alpha V 描画、visual regression ゼロ §5.4)
**関連**: L0-1 dispatch (= pbralphaV program 識別) / L0-4 cadence 再評価 (= PerDraw 降格) / PbrOpaqueV (= 同 layout pair) / MaterialUBO (= GLTF transform 統合) / §5.4 visual regression policy


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

