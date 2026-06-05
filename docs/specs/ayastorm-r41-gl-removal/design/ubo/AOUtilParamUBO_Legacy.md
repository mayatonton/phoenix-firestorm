# AOUtilParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= shell 未通電、host C++ writer / register 配線無し、shader 側 LL_VULKAN_GLSL block でのみ宣言済、Vulkan host UBO write path 未接続)

**本実装化に必要な作業**: per-program cadence の register / write / flush 配線追加 + 既存 OpenGL 経路 setter (= pipeline.cpp:10636-10643 `shader.uniform1f(DEFERRED_SSAO_RADIUS/MAX_RADIUS/FACTOR/FACTOR_INV)`) の mUseUBO 分岐経路から `forwardToUboUpload` → `writeProgramUbo` 経由で本 UBO に書込み開始 + shader 側 `#ifdef LL_VULKAN_GLSL` block の活性化

---

## §1. UBO identity

- **block_name**: `AOUtilParamUBO_Legacy`
- **block_hash**: `0xe2430ca4u` (= FNV-1a("AOUtilParamUBO_Legacy"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 4
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_aoutilparamubo_legacy.inl:12-17
struct AOUtilParamUBO_LegacyLayout {
    static constexpr std::uint32_t ssao_radius_OFFSET = 0u;        // size=4 align=4
    static constexpr std::uint32_t ssao_max_radius_OFFSET = 4u;    // size=4 align=4
    static constexpr std::uint32_t ssao_factor_OFFSET = 8u;        // size=4 align=4
    static constexpr std::uint32_t ssao_factor_inv_OFFSET = 12u;   // size=4 align=4
};
inline constexpr std::uint32_t AOUtilParamUBO_Legacy_SIZE = 256u;  // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/ao_util_param_ubo_legacy.glsl:9-15
layout(std140, set = 3, binding = 8) uniform AOUtilParamUBO_Legacy
{
    float ssao_radius;
    float ssao_max_radius;
    float ssao_factor;
    float ssao_factor_inv;
};
```

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 8
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通、verify 要)
- **pipeline layout**: `sAYAStandardLayout` (= Phase 1.A 確立 5-set layout)
- **set 3 属性**: set=3 は Asset + Skin + Legacy 同居帯 (`llvkloader.cpp:862` literal: `V3A_ASSET_SET_BINDINGS = 3 // set=3: Asset_GLTFNodes + Asset_GLTFMaterials + Skin_GLTFJoints`)。Legacy UBO 群 (本 UBO 含む) は同 set=3 上の binding=4..62 帯に並列配置 (= set=3 内の Legacy 帯は Asset/Skin と物理同居だが cadence_tag=1 PerProgram で別経路 flush)
- **source**: `ubo_metadata.inl:26` literal: `{ "AOUtilParamUBO_Legacy", 0xe2430ca4u, 256u, 3u, 8u, 0u, 1u, 4u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal: `constexpr U32 kCadencePerProgram = 1u; // ubo_metadata.inl で 88 件最大`)
- **意味詳細**: shader program bind 単位で update、`sProgramUboDirty` triple-buffer 経路で flush (= `forwardToUboUpload` PerProgram case → `LLVKLoader::writeProgramUbo(this, loc.block_hash, loc.offset, data, size)` = `llglslshader.cpp:2148`)
- **register**: `llglslshader.cpp:2079-2099` literal: `mUseUBO=true` block 内で `registerProgramUbo(this, loc.block_hash, block_size)` を `seen_program_hashes` で 1 block 1 回呼出
- **source**: ubo_metadata.inl:26 + llglslshader.cpp:95 + llglslshader.cpp:2079-2099 + llglslshader.cpp:2147-2149

---

## §4. 物理 owner

- **data source**: `LLPipeline` (= viewer cvar `RenderSSAOScale` / `RenderSSAOMaxScale` / `RenderSSAOFactor` 由来)
- **既存 OpenGL 経路 writer**: `pipeline.cpp:10636-10643` literal:
  ```cpp
  shader.uniform1f(LLShaderMgr::DEFERRED_SSAO_RADIUS, RenderSSAOScale / screen_to_target_scale_factor);
  shader.uniform1f(LLShaderMgr::DEFERRED_SSAO_MAX_RADIUS, RenderSSAOMaxScale / screen_to_target_scale_factor);
  F32 ssao_factor = RenderSSAOFactor;
  shader.uniform1f(LLShaderMgr::DEFERRED_SSAO_FACTOR, ssao_factor);
  shader.uniform1f(LLShaderMgr::DEFERRED_SSAO_FACTOR_INV, 1.0f/ssao_factor);
  ```
- **uniform 名 reserved**: `llshadermgr.cpp:1660-1663` literal: `mReservedUniforms.push_back("ssao_radius" / "ssao_max_radius" / "ssao_factor" / "ssao_factor_inv")`
- **uniform 名 ↔ enum**: `llshadermgr.h:178-181` literal: `DEFERRED_SSAO_RADIUS` / `DEFERRED_SSAO_MAX_RADIUS` / `DEFERRED_SSAO_FACTOR` / `DEFERRED_SSAO_FACTOR_INV`
- **lifetime**: pipeline 生存中常時 (= shader bind 単位で update)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/ao_util_param_ubo_legacy.glsl`
- **実 shader use site** (= 既存 OpenGL/Vulkan 共通 GLSL file):
  - `indra/newview/app_settings/shaders/class1/deferred/aoUtil.glsl:40-52` literal:
    ```glsl
    #ifdef LL_VULKAN_GLSL
    layout(set=3, binding=8, std140) uniform AOUtilParamUBO_Legacy {
        float ssao_radius;
        float ssao_max_radius;
        float ssao_factor;
        float ssao_factor_inv;
    };
    #else
    uniform float ssao_radius;
    uniform float ssao_max_radius;
    uniform float ssao_factor;
    uniform float ssao_factor_inv;
    #endif
    ```
- **shader registration**: `llviewershadermgr.cpp:965` literal: `shaders.push_back( make_pair( "deferred/aoUtil.glsl", 1) )` (= snippet shader、SSAO consumer program に link される)
- **consumer program** (= aoUtil.glsl を link する program): **不明 / verify 要** (= SSAO consume program 群 = sun shadow / deferred soften light 等候補、grep verify 要)

---

## §6. 既存 setter call site (host C++)

- **pipeline.cpp:10636-10643** (= 唯一の writer、§4 引用) = `shader.uniform1f` 4 連続 call
- **Vulkan UBO 経路では**: 当該 `shader.uniform1f` call は `mUseUBO=true` 時に `LLGLSLShader::uniform1f` 内部の UBO redirect 層から `forwardToUboUpload` → `writeProgramUbo` (= `llglslshader.cpp:2147-2149`) に dispatch される設計 (= PC-7γ-1 redirect 層)
- **本 UBO 専用の host C++ setter / register call**: なし (= `Grep "AOUtilParamUBO_Legacy" indra/llrender` 結果 0 件、本 UBO 名は host C++ には現れない、cadence_tag=1 PerProgram 経路は block_hash 経由 generic register/write)

---

## §7. 現状通電状態

- **状態**: **untouched** (= shell 通電もされていない)
- **shell 段階通電**: なし (= Phase 1.C PC-2 bringupTestUBO は Global_ReflectionProbes 1 件のみ通電、本 UBO は対象外)
- **PC-7γ-1 PerProgram register 経路通電**: 条件付き = `mUseUBO=true` で本 UBO を含む shader を link する際に `registerProgramUbo` が呼ばれる (= `llglslshader.cpp:2079-2099` literal、但し `mUseUBO=false` default ゆえ実走しない)
- **PC-7γ-1 PerProgram write 経路通電**: 条件付き = `mUseUBO=true` で setter (= pipeline.cpp:10636-10643) が走ると `writeProgramUbo` 経路に流れる (= 同上、default OFF)
- **flush / bind**: 条件付き通電 (= PC-7δ で sProgramUboDirty triple-buffer flush 経路完成、cadence=1 PerProgram は generic 経路で本 UBO 個別 hook なし)

---

## §8. 本実装化に必要な作業

1. **mUseUBO ON 化** (= 本 UBO 個別ではなく PerProgram cluster 全体の通電 gate)
2. **shader 側 `#ifdef LL_VULKAN_GLSL` block 活性化** (= `aoUtil.glsl:40-46` 既起案、build verify 要)
3. **setter 経路 verify** (= pipeline.cpp:10636-10643 の `shader.uniform1f` 4 call が `forwardToUboUpload` 経由で正しく writeProgramUbo に dispatch されること cold launch + validation layer 確認)
4. **consumer program 列挙** (= aoUtil.glsl を link する program 全件確認、各 program に対し registerProgramUbo が走ること verify)
5. **codegen 再実行不要** (= layout / member 不変)

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 (= set=3 binding=8、Phase 1.A で配置済) |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded (codegen 出力) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ⚠️ 本 UBO は既存 GLSL に LL_VULKAN_GLSL block 追加済 = shader 改変済、既存 OpenGL path 不変なので OS-5 整合 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**設計原則 (1) Upstream 取り込みやすさ**:
- aoUtil.glsl は upstream LL 由来 + `#ifdef LL_VULKAN_GLSL` block で AYAstorm 追加 = upstream merge conflict 中程度 risk

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守)

1. **consumer program 列挙** = aoUtil.glsl を link する program 全件 (= SSAO consume program、grep verify 要)
2. **screen_to_target_scale_factor** divisor の Vulkan path 整合 (= pipeline.cpp:10634 で FS:WW divisor 適用、本値は per-frame 変動、PerProgram cadence で良いか verify 要)
3. **PC-7γ-1 register 経路の本 UBO 通電確認 commit** = mUseUBO=true 環境で実走確認 commit 不明
4. **VkDescriptorType 確定** = `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` で確定の verify 要

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

set=3 は Asset + Skin + Legacy 大同居帯 (Legacy 帯 binding 4..62)。本 UBO (binding=8) 周辺:
- ClipFParamUBO_Legacy (binding=32)
- CASParamUBO_Legacy (binding=12)
- WaterFogUBO_Legacy (binding=9)
- TonemapUBO_Legacy (binding=10)
- GlobalFParamUBO_Legacy (binding=11)
- DeferredUtilParamUBO_Legacy (binding=6)
- SoftenLightParamUBO_Legacy (binding=5)
- ShadowUtilParamUBO_Legacy (binding=7)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

ubo_metadata.inl 上 cadence_tag=1 は 88 件最大 (= `llglslshader.cpp:95` literal)、Legacy 全 56 件 + Per* 系の一部含む。flush は `sProgramUboDirty` 統一経路。

### §11.3 同 shader consume UBO (= aoUtil.glsl 内同時宣言 UBO)

- **FrameViewProj** (= aoUtil.glsl:58-68 literal で同時宣言確認: `layout(set=0, binding=0, std140) uniform FrameViewProj { ... };`)
- 他 UBO は aoUtil.glsl 内では宣言なし (= consumer program 側で別宣言される)

### §11.4 同 data source UBO

- なし (= SSAO 専用 cvar `RenderSSAO*` のみ、他 UBO と data 共有なし)

### §11.5 dirty 連動 UBO

- 不明 / verify 要 (= cvar `RenderSSAO*` 変更 trigger で連動 dirty が必要な他 UBO は不明、推定なし)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- set=3 帯 bind は `bindV3aStatic` / `bindV3aRigged` 経路 (= `llvkloader.cpp:2168, 2172` literal)、Legacy UBO 個別 bind ではなく set=3 全帯一括 (= set=3 swap 1 回呼出)
- shader program switch 時に PerProgram cadence の triple-buffer 経路で update + 自動 visibility (= dynamic offset 経路では本 binding=8 は固定 binding)
