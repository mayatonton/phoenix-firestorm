# PerDrawUBO_LightParams — UBO design (= 実コードベース調査資料)

**通電状態**: **pilot zero IS real data 通電済** (= Phase 1.E PC-N-13 (a) で `AYAGltfRealLightParamsEnabled` cvar gate + 「zero IS real data」semantic 確立、PC-N-1 (c) + PC-N-13 (a) 2 site で `writeDrawUbo(PerDrawUBO_LightParams, ..., zero_buf, 256, ...)` 経路通電完了、host write 256 B zero buffer が architectural truth = sky_smoke shader 非 consume)

**本実装化に必要な作業**: Phase 1.F+ 実 PBR shader 接続時に PC-N-13.1 等で zero data を real `spot_light_color`/`spot_light_size` (= 既存 OpenGL `LLShaderMgr::LIGHT_DIFFUSE` / `LIGHT_SIZE` 等 uniform 経路の host 書込) で置換 (= handoff `phase1/e/handoff-phase1-e-pc-n-13-complete.md` §0 literal 既明示)

---

## §1. UBO identity

- **block_name**: `PerDrawUBO_LightParams`
- **block_hash**: `0x9ebc071fu` (= FNV-1a("PerDrawUBO_LightParams"))
- **block_size**: 256 B (= std140=16 B, device-padded 256 B)
- **member_count**: 2
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perdrawubo_lightparams.inl:12-16
struct PerDrawUBO_LightParamsLayout {
    static constexpr std::uint32_t spot_light_color_OFFSET = 0u;  // size=12 align=16
    static constexpr std::uint32_t spot_light_size_OFFSET = 12u;  // size=4 align=4
};
inline constexpr std::uint32_t PerDrawUBO_LightParams_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_draw_ubo_light_params.glsl:12-16
layout(std140, set = 2, binding = 0) uniform PerDrawUBO_LightParams
{
    vec3  spot_light_color;
    float spot_light_size;
};
```

= **2 member (vec3 + float、tight pack で 1 vec4 slot 充填) = spot light per-draw 属性**

**alias 定義** (= deferredUtil.glsl:180-181 literal):
```glsl
#define color spot_light_color
#define size  spot_light_size
```
= scope-limit alias で deferredUtil 後段 attach 文書 (shadowUtil 等) backward-compat 維持

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 0
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC` (= set=2 UBO_DYNAMIC 帯、`llvkloader.cpp:861` literal)
- **pipeline layout**: `sAYAStandardLayout` (= `sDrawUboLayoutV3a`、`llvkloader.cpp:2183`)
- **set=2 binding=0 共有 6 UBO の 1 名** (= per_draw_ubo_clip_plane.glsl:9-14 header 参照、name-based dispatch)
- **source**: ubo_metadata.inl:67 `{ "PerDrawUBO_LightParams", 0x9ebc071fu, 256u, 2u, 0u, 0u, 2u, 2u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 2
- **意味**: **PerDraw** (= `llglslshader.cpp:96` literal)
- **flush 経路**: `LLVKLoader::writeDrawUbo(block_hash, offset, data, size, dynamic_offset)` + ring buffer allocate + memcpy + dynamic_offset 返却
- **source**: llglslshader.cpp:2151-2167 literal

---

## §4. 物理 owner

- **shell 段階**: owner なし (= per-draw transient、ring buffer 内 dynamic offset で識別)
- **PC-N-1 (c) 通電** = recordPlaceholderPoolDraw (= sky_smoke pipeline placeholder draw) で zero buffer 書込 (= `llvkloader.cpp:6470-6475` literal):
  ```cpp
  LLVKLoader::writeDrawUbo(
      ubo::block_hash::PerDrawUBO_LightParams,
      /*offset=*/0u,
      zero_buf, sizeof(zero_buf), dynamic_offset);
  ```
- **PC-N-13 (a) 通電** = recordGltfAssetDraw (= sGltfStubAssetPipeline real GLTF Asset path) で zero buffer 書込 (= `llvkloader.cpp:6612-6617` literal、同形 256 B zero buffer)
- **本実装化後 owner**: 既存 OpenGL 経路で `spot_light_color`/`spot_light_size` を per-draw 書込む site (= grep verify 要) の host 上流

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_draw_ubo_light_params.glsl:12-16`
  - source extract from `class1/deferred/deferredUtil.glsl:175` ifdef LL_VULKAN_GLSL block (single site) (= per_draw_ubo_light_params.glsl:1-8 header literal)
- **実 shader use site** (= grep 結果、`PerDrawUBO_LightParams` 文字列 hit、blueprint 除く):
  - `indra/newview/app_settings/shaders/class3/deferred/multiPointLightF.glsl`
  - `indra/newview/app_settings/shaders/class3/deferred/reflectionProbeF.glsl`
  - `indra/newview/app_settings/shaders/class3/deferred/spotLightF.glsl`
  - `indra/newview/app_settings/shaders/class3/deferred/pointLightF.glsl`
  - `indra/newview/app_settings/shaders/class3/deferred/softenLightF.glsl`
  - `indra/newview/app_settings/shaders/class1/deferred/deferredUtil.glsl` (= primary site、blueprint source)
  - `indra/newview/app_settings/shaders/class1/deferred/globalF.glsl`
  - `indra/newview/app_settings/shaders/class1/deferred/pbropaqueF.glsl`
  - `indra/newview/app_settings/shaders/class1/deferred/starsV.glsl`
  - `indra/newview/app_settings/shaders/class1/gltf/pbrmetallicroughnessF.glsl`
- **deferredUtil.glsl literal** (= deferredUtil.glsl:175-181):
  ```glsl
  layout(set=2, binding=0, std140) uniform PerDrawUBO_LightParams {
      vec3  spot_light_color;
      float spot_light_size;
  };
  #define color spot_light_color
  #define size spot_light_size
  ```

---

## §6. 既存 setter call site (host C++)

詳細は handoff `docs/specs/ayastorm-r41-gl-removal/handoff/phase1/e/handoff-phase1-e-pc-n-13-complete.md` 参照。

- **PC-N-1 (c) writeDrawUbo site** = `indra/llrender/llvkloader.cpp:6470-6475` (= recordPlaceholderPoolDraw 内、placeholder phase ゆえ zero buffer)
- **PC-N-13 (a) writeDrawUbo site** = `indra/llrender/llvkloader.cpp:6612-6617` (= recordGltfAssetDraw 内、real GLTF Asset path、AYAGltfRealLightParamsEnabled cvar gate)
- **PC-N-2 (= sky_smoke pipeline draw) writeDrawUbo site** = `indra/llrender/llvkloader.cpp:6870-6880` 付近 ((N2-1) A 1 call 4 set bind + (N2-2) A PerDrawUBO_LightParams 共有 = `llvkloader.cpp:6842/6876/6928` literal grep hit)
- **cvar gate**: `AYAGltfRealLightParamsEnabled` (= settings.xml Boolean default=0 Persist=1、PC-N-13 step (a))
- **first-fire LL_INFOS marker** = cvar=true 時に「zero IS real data」semantic 通電 literal 取得 (= llvkloader.cpp:6622-6638 literal)

---

## §7. 現状通電状態

- **状態**: **pilot zero IS real data 通電済** (= Phase 1.E PC-N-13 完了)
- **通電 commit**: Phase 1.E PC-N-13 commit (= handoff `handoff-phase1-e-pc-n-13-complete.md` 参照、commit hash 別記)
- **通電内容**:
  - host write = 256 B zero buffer 維持 (= sky_smoke shader 非 consume architectural truth = `llvkloader.cpp:5843-5845` 既明示)
  - shader 側 GPU error なし (= descriptor set layout 充足のみ目的)
  - cvar gate 起動 = `AYAGltfRealLightParamsEnabled=true` 時に first-fire LL_INFOS marker 出力
  - 「zero IS real data」semantic = 現 phase 解釈 ((N13-1) C 採用) = sGltfStubAssetPipeline 流用 sky_smoke shader (= sSkySmokeVertModule / sSkySmokeFragModule) は PerDrawUBO_LightParams を非 consume = host write 256 B zero buffer が descriptor set layout 充足 architectural truth
- **Phase 1.F+ 持越**: 実 PBR shader 接続時に PC-N-13.1 等で data 内容置換着手予定

---

## §8. 本実装化に必要な作業

詳細は handoff `phase1/e/handoff-phase1-e-pc-n-13-complete.md` §0 + cross-platform spec §6 PC-N-13.1 持越項目参照。

1. **実 PBR shader 接続 (Phase 1.F+)** = sGltfStubAssetPipeline / sSkySmokePipeline を実 PBR shader (= class1/gltf/pbrmetallicroughnessF.glsl 等) に置換
2. **data 内容置換** = host C++ 側 zero buffer を real `spot_light_color` (= 既存 OpenGL `LLShaderMgr::LIGHT_DIFFUSE` 等候補、grep verify 要) + real `spot_light_size` (= `LIGHT_DEFERRED_ATTENUATION.w` 等候補、grep verify 要) で書込
3. **dirty 判定** = per-draw cadence ゆえ毎 draw 書込み許容、dirty 判定不要 (= per-light per-draw 新値、ring buffer allocate 経路で transient)
4. **cvar gate 撤去** = `AYAGltfRealLightParamsEnabled` cvar を Phase 1.F+ 本実装化完了後に撤去 (= Phase 1.E cleanup pattern = AYAGltfRealDrawEnabled / AYAGltfMultiSkinEnabled 同位、PC-N-15c 撤去 precedent)
5. **shader 側 #else block 撤去** (= Phase 2+ OpenGL gate 完全撤去時) = 個別 uniform 宣言削除

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | set=2 binding=0 共有 6 UBO | ✅ name-based dispatch 既配置 |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded (codegen)、vec3 (12B) + float (4B) = 1 vec4 slot tight pack |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既存 GLSL #ifdef LL_VULKAN_GLSL 既配置 |
| OS-7 | Linux validation layer warnings 0 件 | ✅ AYA live verify「通常通りに描画されてます」record 2026-06-06 (= Phase 1.E complete marker、PC-N-13 含む) |

**「zero IS real data」semantic risk**:
- 現 phase の zero buffer 書込 = architectural truth ((N13-1) C 採用根拠)
- sGltfStubAssetPipeline 流用 sky_smoke shader が PerDrawUBO_LightParams 非 consume = 「real data 内容置換不要」状態
- Phase 1.F+ 実 PBR shader 接続時に semantic 解釈変化 = 「zero IS placeholder data」へ移行、real value 置換必須

**std140 alignment 罠**:
- vec3 + float の連続配置で tight pack 16 B = std140 仕様充足 (= vec3 align=16 + 4 B float が 同 vec4 slot trailing)
- 連続 array / nested struct なし = padding 罠なし

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **本実装化時 real value source** = `spot_light_color` / `spot_light_size` の host 側 data source (= `LLShaderMgr::LIGHT_DIFFUSE` / `LIGHT_DEFERRED_ATTENUATION` 等候補、grep verify 要)
2. **per-light per-draw 書込み legacy 経路** = 既存 OpenGL 経路で `color` / `size` uniform を per-draw 書込む call site (= `LLPipeline::renderDeferredLighting` 等候補、grep verify 要)
3. **deferredUtil alias scope** = `#define color spot_light_color` の影響 scope (= deferredUtil 後段 attach 全 shader、deferredUtil.glsl:777-779 既明示 `r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-20: scope-limit PerDrawUBO_LightParams alias`)
4. **PC-N-13 通電 commit hash** = handoff doc 内 commit 引用要 (= `cd253cb754` (PC-N-13 design-lock) の次の実装 commit、本 file 起案で逐次解消)
5. **Phase 1.F+ data 内容置換実装 plan 詳細** = PC-N-13.1 等の具体 step (= cross-platform spec §6 持越項目 entry、verify 要)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 4 binding × 多 UBO 名共有、本 UBO は binding=0 (= 6 共有 UBO のうち 1)

### §11.2 同 cadence cluster UBO (= cadence_tag=2 PerDraw)

- PerDraw cluster 7 UBO (PerDrawUBO_ClipPlane §11.2 参照)
- **PerDrawUBO_LightParams (本 UBO) = PerDraw cluster の pilot 通電 UBO** (= Phase 1.E PC-N-13)

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- 同 file (= class3/deferred/multiPointLightF / spotLightF / pointLightF / softenLightF / reflectionProbeF + class1/deferred/deferredUtil / globalF / pbropaqueF / starsV + class1/gltf/pbrmetallicroughnessF) で `PerDrawUBO_MultiLight` (binding=1) + Frame_* set=0 帯 + PerProgram set=2 帯多 binding と同時 consume (= verify 要)

### §11.4 同 data source UBO

- spot light data 由来 = `PerProgramUBO_SpotLightF` (set=2 binding=10) と関連可能性 (= 推定、verify 要)
- `PerProgramUBO_PointLightF` (set=2 binding=25) + `PerProgramUBO_PointLightV` (set=2 binding=5) と light 系 group (= verify 要)

### §11.5 dirty 連動 UBO

- per-light per-draw cadence ゆえ light list 変化時に同時 dirty = FrameLights (set=0 binding=1) + PerDrawUBO_MultiLight (set=2 binding=1) と連動可能性

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- per-draw cadence ゆえ毎 draw call で `bindV3aStatic` / `bindV3aRigged` 経由 set=2 帯 4 binding 同時 bind

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.15 同期)

**Layer**: L4-15 (= set=2 binding=0 共有残 2 UBO、**Phase 3 R5 メインターゲット**)
**status**: **起案済** (= 2026-06-06 C-6-j、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.15` (= single source of truth)
**要点**: 2 member (spot_light_color vec3 + spot_light_size float、1 vec4 slot tight pack)、10 file consume (= primary deferredUtil.glsl:175 + 9 file)、alias `#define color spot_light_color` + `#define size spot_light_size` で deferredUtil 後段 attach (shadowUtil 等) backward-compat、**pilot zero IS real data 通電済** (= Phase 1.E PC-N-13)、`AYAGltfRealLightParamsEnabled` cvar gate (= settings.xml Boolean default=0 Persist=1)、first-fire LL_INFOS marker (`llvkloader.cpp:6622-6638`)、writeDrawUbo 3 site (= `llvkloader.cpp:6470-6475` PC-N-1 (c) + `:6612-6617` PC-N-13 (a) + `:6870-6880` 付近 PC-N-2)、「zero IS real data」semantic = sGltfStubAssetPipeline 流用 sky_smoke 非 consume architectural truth、**Phase 3 R5 = real value 置換** (= spot_light_color/spot_light_size を実 light data へ、`LLShaderMgr::LIGHT_DIFFUSE`/`LIGHT_DEFERRED_ATTENUATION` 経由候補) [要追加調査]、AYAstorm light cvar §3.5.2 sub-cluster (c) 連動候補 [要 verify D4 突合]、cvar gate 撤去予定 (= AYAGltfRealDrawEnabled/AYAGltfMultiSkinEnabled 同位 PC-N-15c precedent)、工数 group 全体 M 内
**関連**: L0-1 dispatch (= name-based dispatch 6 UBO 集約) / §3.5.15 sibling ClipPlane (= 同 binding=0 共有) / §3.5.8 group 4 UBO (= 同 binding=0 共有残) / §3.5.2 sub-cluster (c) PointLightF/SpotLightF/PointLightV (= AYAstorm light cvar 連動候補) / FrameLights (= light list 連動) / §3.5.16 PerDrawUBO_MultiLight (= 同 deferred lighting pipeline) / Phase 1.E PC-N-13 (= pilot 通電 commit) / Phase 3 R5 = real value 置換
