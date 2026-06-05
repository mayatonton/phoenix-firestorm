# MaterialUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= blueprint + codegen metadata 生成済、host C++ register/write/flush 経路未着工、shader use site = `class3/deferred/materialF.glsl:38` の `#ifdef LL_VULKAN_GLSL` block 単独宣言 = singleton site)

**本実装化に必要な作業**: register/write/flush 経路新設 (= per-program cadence 経路で program load 時 register、material 切替 setter で write、cmdbuf bind 経路で flush) + 既存 8 uniform setter call site 棚卸し + UBO upload 化 + set=1 binding=0 を MaterialUBO と排他運用する program 識別 logic 確立

---

## §1. UBO identity

- **block_name**: `MaterialUBO_Legacy`
- **block_hash**: `0x5f4eef2eu` (= FNV-1a("MaterialUBO_Legacy"))
- **block_size**: 256 B (= std140 64 B、device-padded 256 B)
- **member_count**: 8
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_materialubo_legacy.inl:12-22
struct MaterialUBO_LegacyLayout {
    static constexpr std::uint32_t morphFactor_OFFSET = 0u;            // size=16 align=16
    static constexpr std::uint32_t specular_color_OFFSET = 16u;        // size=16 align=16
    static constexpr std::uint32_t camPosLocal_OFFSET = 32u;           // size=12 align=16
    static constexpr std::uint32_t emissive_brightness_OFFSET = 44u;   // size=4  align=4
    static constexpr std::uint32_t is_mirror_OFFSET = 48u;             // size=4  align=4
    static constexpr std::uint32_t env_intensity_OFFSET = 52u;         // size=4  align=4
    static constexpr std::uint32_t aya_sss_skin_flag_OFFSET = 56u;     // size=4  align=4
    static constexpr std::uint32_t _pad_material_legacy_0_OFFSET = 60u;// size=4  align=4
};
inline constexpr std::uint32_t MaterialUBO_Legacy_SIZE = 256u; // std140=64, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/class3/deferred/materialF.glsl:38-47
layout(set=1, binding=0, std140) uniform MaterialUBO_Legacy {
    vec4  morphFactor;
    vec4  specular_color;
    vec3  camPosLocal;
    float emissive_brightness;
    float is_mirror;
    float env_intensity;
    float aya_sss_skin_flag;
    float _pad_material_legacy_0;
};
```

= Blueprint file (= `aya_r41_blueprints/set1/material_ubo_legacy.glsl:13-23`) は実 shader と layout 完全一致。

---

## §2. binding 配線

- **descriptor_set**: 1
- **binding**: 0
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、metadata literal は cadence_tag=1 PerProgram、set=1 帯は `V3A_PROGRAM_SET_A_BINDINGS=40` の non-dynamic UBO、verify 要)
- **pipeline layout**: 不明 (= `sAYAStandardLayout` 5-set V3a layout 内 set=1 帯は `V3A_PROGRAM_SET_A_BINDINGS=40 / V3A_PROGRAM_SET_B_BINDINGS=40` split 想定だが、metadata 上は MaterialUBO/MaterialUBO_Legacy 両者 set=1 binding=0 共有 = blueprint header note「同 program 内で両者 attach は不可 (= program ごとに片方のみ宣言される運用)。metadata 上は 2 entry 共存、host C++ 側 dispatch は name 経由で分離する想定」)
- **source**: `ubo_metadata.inl:53` `{ "MaterialUBO_Legacy", 0x5f4eef2eu, 256u, 1u, 0u, 0u, 1u, 8u }` + `class3/deferred/materialF.glsl:38` literal + `aya_r41_blueprints/set1/material_ubo_legacy.glsl:7-9` header literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program** (= `llglslshader.cpp:95` literal: `constexpr U32 kCadencePerProgram = 1u; // ubo_metadata.inl で 88 件最大`)
- **意味詳細**: program 切替時に rebind、program load 時 `registerProgramUbo` で sProgramUboDirty に triple-buffer entry 確保、material 切替 setter (= 既存 uniform setter call site) で write
- **source**: ubo_metadata.inl + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **owner**: 不明 / verify 要
- **候補**:
  - `morphFactor` (vec4) = avatar morph (= LLVOAvatar / LLViewerJoint 由来候補、verify 要)
  - `specular_color` (vec4) = legacy material specular = `LLMaterial::mSpecularLightColor` 由来 (= `indra/llprimitive/llmaterial.cpp:362` `material_data[MATERIALS_CAP_SPECULAR_COLOR_FIELD] = mSpecularLightColor.getValue();` literal)
  - `camPosLocal` (vec3) = sky/cloud camera local position = `LLDrawPoolWLSky::renderDome(const LLVector3& camPosLocal, ...)` 経路 (= `indra/newview/lldrawpoolwlsky.cpp:97`)、`static LLStaticHashedString sCamPosLocal("camPosLocal");` (`lldrawpoolwlsky.cpp:52`)
  - `emissive_brightness` (float) = alpha pool emissive flag = `LLShaderMgr::EMISSIVE_BRIGHTNESS` (`llshadermgr.h` enum + `lldrawpoolalpha.cpp:685` `uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, 1.f);` literal)
  - `is_mirror` (float) = reflection probe mirror flag = `LLReflectionProbeParams::setIsMirror(bool is_mirror)` (`llprimitive.cpp:1988` + `llvovolume.cpp:3708` `setReflectionProbeIsMirror`)
  - `env_intensity` (float) = legacy material env intensity = `LLMaterial::mEnvironmentIntensity` (= `lldrawpoolalpha.cpp:971` `uniform1f(LLShaderMgr::ENVIRONMENT_INTENSITY, env_intensity);`)
  - `aya_sss_skin_flag` (float) = r20 Phase C AYA SSS skin flag (= `llshadermgr.cpp:1611` `mReservedUniforms.push_back("aya_sss_skin_flag"); // <FS:AYA r20 Phase C>` literal)
- **lifetime**: 不明 / verify 要 (= 各 member の owner が異なる = 同一 UBO に集約された 8 member は機能的に異種混在、本実装化時 dirty 判定でどの owner trigger でどの member を update するか logic 起案要)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set1/material_ubo_legacy.glsl`
- **実 shader use site** = **`class3/deferred/materialF.glsl:38` 単独** (= blueprint header literal「singleton site = 1 file のみ宣言」、grep 結果 `class3/deferred/materialF.glsl` 1 file)
- 周辺で member 名を含む shader (= UBO 宣言なし、`#else` block の bare uniform):
  - `class3/deferred/fullbrightShinyF.glsl` (= `emissive_brightness` 等)
  - `class1/deferred/skyV.glsl` (= `camPosLocal` 等)
  - `class1/deferred/skinSSSF.glsl` (= `aya_sss_skin_flag` 等)
  - `class1/deferred/pbropaqueF.glsl` (= `aya_sss_skin_flag` 等、別 UBO PBROpaqueExtraUBO_Legacy 経由)
  - `class1/deferred/cloudsF.glsl` / `cloudsV.glsl` (= `camPosLocal` 等)
  - `class1/deferred/avatarF.glsl` (= `emissive_brightness` 等)
- **不明 / verify 要**: 上記周辺 shader は本 UBO_Legacy を consume するのか、別 UBO (= AvatarFParamUBO / SkyVParamUBO / CloudsVParamUBO 等) で個別 wrap 済か、Phase 2 着工前に 1 by 1 verify 要

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: 未配線 (= bringupTestUBO は Global_ReflectionProbes 専用、本 UBO は register/write 経路 0 件)
- **本実装化後 setter** (= 既存 OpenGL 経路の uniform setter call site、verify 済 source literal):
  - `morphFactor` = 不明 / verify 要 (= grep で morphFactor uniform setter 未取得、avatar morph 経路要)
  - `specular_color` = `indra/newview/lldrawpoolalpha.cpp:970` `current_shader->uniform4f(LLShaderMgr::SPECULAR_COLOR, spec_color.mV[VRED], ...)`
  - `camPosLocal` = `LLDrawPoolWLSky` 経路 (= `static LLStaticHashedString sCamPosLocal("camPosLocal");` `lldrawpoolwlsky.cpp:52`)、setter 不明 / verify 要
  - `emissive_brightness` = `indra/newview/lldrawpoolalpha.cpp:685/694/723/972` `shader->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, 1.f);` 4 site
  - `is_mirror` = `LLVOVolume::setReflectionProbeIsMirror` (`llvovolume.cpp:3708`) 経路、host 側 uniform setter 不明 / verify 要 (= `uniform1f` 直接 call が探せていない)
  - `env_intensity` = `indra/newview/lldrawpoolalpha.cpp:971` `current_shader->uniform1f(LLShaderMgr::ENVIRONMENT_INTENSITY, env_intensity);`
  - `aya_sss_skin_flag` = r20 Phase C 経路、setter 不明 / verify 要 (= `llshadermgr.cpp:1611` の reserved uniform 登録のみ確認、setter 呼出 site 別 grep 要)

---

## §7. 現状通電状態

- **状態**: **untouched** (= shell 通電なし、Phase 1.C PC-2 で通電された Global_ReflectionProbes と異なり、本 UBO の register/write/flush 経路 0 件)
- **codegen 生成済**: layout `inl` + metadata entry + block_hash constexpr 生成済 (= `ubo_layout_materialubo_legacy.inl` + `ubo_metadata.inl:53` + `block_hash::MaterialUBO_Legacy = 0x5f4eef2eu`)
- **blueprint 起案済**: `aya_r41_blueprints/set1/material_ubo_legacy.glsl` (= Phase 1.A PA-8 で起案、blueprint header literal)
- **shader 宣言済**: `class3/deferred/materialF.glsl:38` `#ifdef LL_VULKAN_GLSL` block 単独 site (= GATE-B = LL_VULKAN_GLSL macro は GLSL 側専用、host C++ 不参照、memory `project_r41_phase1b_vulkan_host_gate` 整合)
- **register/write/flush 経路**: 未配線

---

## §8. 本実装化に必要な作業

1. **register 経路**:
   - `LLGLSLShader::mapUniforms()` 内で `block_hash::MaterialUBO_Legacy` を含む program に対し `LLVKLoader::registerProgramUbo(this, block_hash, block_size)` を呼出 (= `llglslshader.cpp:2093` 既存 path、本 UBO も該当)
   - unregister は `LLGLSLShader::deleteShader` で `unregisterProgramUbo` 呼出 (= `llglslshader.cpp:448` 既存 path)
2. **write 経路**:
   - 既存 8 uniform setter (= `EMISSIVE_BRIGHTNESS` / `SPECULAR_COLOR` / `ENVIRONMENT_INTENSITY` 等) を `forwardToUboUpload` 経由で UBO write に redirect (= `llvkloader.cpp:464` 既存設計骨子、PER_PROGRAM case)
   - dirty 判定 = uniform setter call で member offset と value 取得 → sProgramUboDirty に write entry 蓄積
3. **flush 経路**:
   - cmdbuf bind 経路で sProgramUboDirty を flush → triple-buffer 経由で `vkCmdBindDescriptorSets` set=1 帯 binding=0 に bind
4. **set=1 binding=0 排他運用 logic**:
   - MaterialUBO (= PBR 拡張 10-member canonical) と MaterialUBO_Legacy (= legacy 8-member) は同 set=1 binding=0 (= blueprint header literal「同 program 内で両者 attach は不可 = program ごとに片方のみ宣言される運用、host C++ 側 dispatch は name 経由で分離する想定」)
   - program 識別 logic = LLGLSLShader::mShaderName 等から MaterialUBO/MaterialUBO_Legacy どちらを期待するか判定 → register 時に該当 block_hash のみ wire
   - 06a §3.2 + blueprint set1/material_ubo.glsl header literal「46 file 中、4 PBR site が 10-member、残り 45 site が 6-member base のみを宣言。AYA option (I) 採用 = 10-member full を canonical blueprint とする = host C++ 側 allocate は full size、6-member shader は trailing 4 member 未参照 view」= MaterialUBO 側は「larger buffer に smaller block view 合法」前提、Legacy は **別 entry** = 真に program 毎切替
5. **shader 接続**:
   - 本 UBO は `class3/deferred/materialF.glsl:38` 1 file のみ既に UBO 宣言 (= shader 改変ゼロ)、`#else` block の bare uniform path を host C++ 側で UBO 経由に切替するだけ

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=1 内に収める | ✅ 維持 (= set=1 binding=0、blueprint 起案済) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 64B → 256B padded (codegen 出力で生成) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 (= per-program cadence、ring buffer 経路で query 結果使用するか確認要) |
| OS-5 | shader 改変ゼロ | ✅ `class3/deferred/materialF.glsl:38` の `#ifdef LL_VULKAN_GLSL` block は既に UBO 宣言済 = Phase 2 で shader 追加改変なし |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 (= Phase 2 cold launch 時) |

**set=1 binding=0 排他運用 risk**:
- MaterialUBO (10-member 256B) と MaterialUBO_Legacy (8-member 256B) は同 set=1 binding=0、block_size 同じ 256B、layout 異なる = host C++ が program 毎にどちらの layout を期待するか名前 dispatch 必須
- 同一 frame 内で異 program を順に bind する場合、buffer 内容が前 program 用 layout のままだと型不整合 = 各 program switch で再 write 必須

**member 出自 risk**:
- 8 member は機能的に異種混在 (= morph / specular / camPosLocal / emissive / is_mirror / env / SSS flag) = 単一 cadence (per-program) で全 member を upload するのは効率悪、ただし設計原則 (1) Upstream 取り込みやすさ維持から既存 layout 温存

**shader site 数 risk**:
- 本 UBO の `#ifdef LL_VULKAN_GLSL` 宣言は `class3/deferred/materialF.glsl` 1 file のみ (= blueprint header literal「singleton site」) だが、周辺 8+ shader が同 member 名 (`emissive_brightness` 等) を bare uniform として使用 → 同 member は別 UBO で wrap 済か個別 verify 要

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **set=1 帯の descriptor type 確定** = `V3A_PROGRAM_SET_A_BINDINGS=40 / B=40` split layout で本 UBO がどちらの帯に属するか (= llvkloader.cpp:859-860 V1' split)
2. **set=1 binding=0 同居の MaterialUBO 排他運用具体 logic** = program 識別を name 経由でどう実装するか (= mShaderName parse / shader feature flag / 別 mechanism)
3. **morphFactor 出自** = avatar morph の uniform setter call site (= grep 未取得、`LLViewerJointMesh` 等周辺 verify 要)
4. **is_mirror uniform setter call site** = host 側で `LLVOVolume::getReflectionProbeIsMirror()` の結果を shader に伝える `uniform1f` 直接 call (= grep 未取得)
5. **aya_sss_skin_flag setter call site** = r20 Phase C で追加された setter (= `llshadermgr.cpp:1611` reserved uniform 登録のみ確認)
6. **`camPosLocal` setter call site** = `lldrawpoolwlsky.cpp:52` で `LLStaticHashedString sCamPosLocal` 宣言済だが実 setter (`uniform3f`/`uniform3fv`) 呼出 site 未取得
7. **周辺 shader (= `class3/deferred/fullbrightShinyF.glsl` 等 8 file) の本 UBO consume 関係** = それぞれ別 UBO に wrap されているか、bare uniform 残存か
8. **bringupTestUBO 同等の通電 marker 起案要否** = shell 通電 phase で zero buffer marker を起こすか、Phase 2 で直接本実装化に進むか

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=1)

- **MaterialUBO** (set=1 binding=0、本 UBO と同 binding = **排他運用**、blueprint header literal「program ごとに片方のみ宣言される運用」)
- **不明 / verify 要**: set=1 帯 binding=1..39 (= `V3A_PROGRAM_SET_A_BINDINGS=40`) の他 UBO 配置 (= ubo_metadata.inl set=1 entry は MaterialUBO + MaterialUBO_Legacy のみ、binding=1..39 は空き枠? V3a layout 設計と metadata 一致確認要)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- per-program cadence は ubo_metadata.inl 上 73 件 (推定、INDEX.md §1) = 全 Legacy UBO 群 + Program_* 群と同 cluster
- 同 cluster で flush 経路 (= sProgramUboDirty + program 切替 trigger) を共有

### §11.3 同 shader consume UBO (= `class3/deferred/materialF.glsl` で同時 consume)

- `class3/deferred/materialF.glsl` で同時 consume されている UBO (= Phase 2 着工前 verify 要):
  - FrameViewProj / FrameLights / FrameAtmosphere_Lighting (= 必須、deferred lighting 全 shader 共通)
  - 不明 / verify 要: 他 Legacy UBO (= AtmoExtraUBO_Legacy 等) も同 file で宣言されているか grep verify 要

### §11.4 同 data source UBO (= 同 host data source から派生)

- **PBROpaqueExtraUBO_Legacy** (set=3 binding=13) = `aya_sss_skin_flag` member 共有 = 同 r20 Phase C SSS skin flag 由来 (= blueprint set3/pbr_opaque_extra_ubo_legacy.glsl + 本 UBO 両者で `aya_sss_skin_flag` 宣言)
- **AvatarFParamUBO_Legacy** (set=3 binding=54) = `emissive_brightness` 類似可能性 (= verify 要)

### §11.5 dirty 連動 UBO

- 不明 / verify 要 (= 各 member の owner trigger 別、`emissive_brightness` setter 経路でどの他 UBO が同時 dirty 化するか確認要)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout (= Phase 1.A 確立、全 UBO 共通)

### §11.7 bind 順序関係

- program 切替時 set=1 帯全 binding を一括 rebind (= `vkCmdBindDescriptorSets` 1 回呼出で set=1 帯全 binding 含む想定、PC-7δ 経路)
- 本 UBO の bind timing = program 切替時 (= per-program cadence 標準)
