# PerProgramUBO_SpotLightF — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `per_program_ubo_spot_light_f.glsl` は実 shader `class3/deferred/spotLightF.glsl:74 ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。本 UBO は **member_count=10 = 本 batch 13 個中最大**、10 件 bare uniform を 1 UBO に集約。

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_SpotLightF`
- **block_hash**: `0xa49d77b3u` (= FNV-1a("PerProgramUBO_SpotLightF"))
- **block_size**: 256 B (= std140 48 B、device-padded 256 B)
- **member_count**: 10 (= 3 chunk: 4 scalar + vec3+float + 4 scalar)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_spotlightf.inl
struct PerProgramUBO_SpotLightFLayout {
    static constexpr std::uint32_t proj_near_OFFSET = 0u;  // size=4 align=4
    static constexpr std::uint32_t proj_ambient_lod_OFFSET = 4u;  // size=4 align=4
    static constexpr std::uint32_t near_clip_OFFSET = 8u;  // size=4 align=4
    static constexpr std::uint32_t far_clip_OFFSET = 12u;  // size=4 align=4
    static constexpr std::uint32_t proj_origin_OFFSET = 16u;  // size=12 align=16
    static constexpr std::uint32_t sun_wash_OFFSET = 28u;  // size=4 align=4
    static constexpr std::uint32_t proj_shadow_idx_OFFSET = 32u;  // size=4 align=4
    static constexpr std::uint32_t shadow_fade_OFFSET = 36u;  // size=4 align=4
    static constexpr std::uint32_t falloff_OFFSET = 40u;  // size=4 align=4
    static constexpr std::uint32_t global_light_strength_OFFSET = 44u;  // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_SpotLightF_SIZE = 256u; // std140=48, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_spot_light_f.glsl
layout(std140, set = 2, binding = 10) uniform PerProgramUBO_SpotLightF
{
    // chunk 0 (4 scalar)
    float proj_near;
    float proj_ambient_lod;
    float near_clip;
    float far_clip;
    // chunk 1 (vec3 + float)
    vec3  proj_origin;
    float sun_wash;
    // chunk 2 (4 scalar)
    int   proj_shadow_idx;
    float shadow_fade;
    float falloff;
    float global_light_strength;
};
```

= 全 10 member 実 data slot 確定 (= blueprint comment literal: `Source: literal extract from class3/deferred/spotLightF.glsl:74 ifdef LL_VULKAN_GLSL block (single site)` + `η-27 1d/1e-A + η-28 Phase 2d-α で vec3 center 配置 (= PointLightV と分離)`)。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 10
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通)
- **pipeline layout**: `sAYAStandardLayout`
- **set 2 配置**: PerDraw + PerProgram 帯
- **source**: `ubo_metadata.inl:89` `{ "PerProgramUBO_SpotLightF", 0xa49d77b3u, 256u, 2u, 10u, 0u, 1u, 10u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush。Spot light F stage で projector + shadow + light 系 10 member 統合
- **source**: ubo_metadata.inl:89 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source** (= 複数 owner 由来):
  - `proj_near` = `pipeline.cpp:12180` `shader.uniform1f(LLShaderMgr::PROJECTOR_NEAR, near_clip)`
  - `proj_ambient_lod` = `pipeline.cpp:12257` `shader.uniform1f(LLShaderMgr::PROJECTOR_AMBIENT_LOD, llclamp(...))`
  - `near_clip` = `pipeline.cpp:10661` `shader.uniform1f(LLShaderMgr::DEFERRED_NEAR_CLIP, LLViewerCamera::getInstance()->getNear()*2.f)`
  - `far_clip` = **不明 / verify 要** (= `LLViewerCamera` getFar 経由想定)
  - `proj_origin` = `pipeline.cpp:12183` `shader.uniform3fv(LLShaderMgr::PROJECTOR_ORIGIN, 1, glm::value_ptr(screen_origin))`
  - `sun_wash` = `pipeline.cpp:10628` `shader.uniform1f(LLShaderMgr::DEFERRED_SUN_WASH, RenderDeferredSunWash)`
  - `proj_shadow_idx` = `pipeline.cpp:12196` `shader.uniform1i(LLShaderMgr::PROJECTOR_SHADOW_INDEX, s_idx)`
  - `shadow_fade` = `pipeline.cpp:12200` `shader.uniform1f(LLShaderMgr::PROJECTOR_SHADOW_FADE, 1.f-mSpotLightFade[s_idx])` (or `:12204` `1.f`)
  - `falloff` = `pipeline.cpp:11551` `gDeferredSpotLightProgram.uniform1f(LLShaderMgr::LIGHT_FALLOFF, volume->getLightFalloff(DEFERRED_LIGHT_FALLOFF))` (= spot setter)
  - `global_light_strength` = `pipeline.cpp:10705 / 10712` `shader.uniform1f(LLShaderMgr::DEFERRED_LIGHT_STRENGTH, RenderGlobalLightStrength / 1.0f)`
- **lifetime**: program 単位 (= spot light render program)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_spot_light_f.glsl`
- **実 shader use site**: **`class3/deferred/spotLightF.glsl:74 ifdef LL_VULKAN_GLSL block`** (= single site)
  - 既存 UBO block (spotLightF.glsl:76-87):
    ```glsl
    float proj_near;
    float proj_ambient_lod;
    float near_clip;
    float far_clip;
    vec3  proj_origin;
    float sun_wash;
    int   proj_shadow_idx;
    float shadow_fade;
    float falloff;
    float global_light_strength;
    ```
  - 既存 OpenGL `#else` block (spotLightF.glsl:91-197) で個別 uniform 宣言 (= 10 件)
  - 使用箇所: `spotLightF.glsl:253` (`if (proj_shadow_idx >= 0)`) + `:256` (`shadow = (proj_shadow_idx==0)?shd.b:shd.a`) + `:257` (`shadow += shadow_fade`) + `:265` (`float dist_atten = calcLegacyDistanceAttenuation(dist, falloff)`) + `:271` (`lv = proj_origin-pos.xyz`) + `:415` (`final_color *= global_light_strength`)
  - shader comment literal `spotLightF.glsl:66-68`: `10 件 bare uniform 集約 (proj_near / proj_ambient_lod / near_clip / far_clip / proj_origin / sun_wash / proj_shadow_idx / shadow_fade / falloff / global_light_strength)`

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **既存 OpenGL 経路 setter** (= `llshadermgr.h` 内 enum 定義 + `pipeline.cpp` 内 setter 多数):
  - `llshadermgr.h:85` `PROJECTOR_NEAR, // "proj_near"`
  - `llshadermgr.h:88` `PROJECTOR_ORIGIN, // "proj_origin"`
  - `llshadermgr.h:91` `PROJECTOR_SHADOW_INDEX, // "proj_shadow_idx"`
  - `llshadermgr.h:92` `PROJECTOR_SHADOW_FADE, // "shadow_fade"`
  - `llshadermgr.h:95` `PROJECTOR_AMBIENT_LOD, // "proj_ambient_lod"`
  - `llshadermgr.h:156` `LIGHT_FALLOFF, // "falloff"`
  - `llshadermgr.h:175` `DEFERRED_SUN_WASH, // "sun_wash"`
  - `llshadermgr.h:184` `DEFERRED_NEAR_CLIP, // "near_clip"`
  - `llshadermgr.h:416` `DEFERRED_LIGHT_STRENGTH, // "global_light_strength"`
  - 実 setter (= pipeline.cpp 内 §4 参照、全 10 member の uniform setter site 特定済 except `far_clip`)
- **本実装化後 setter**:
  - 31 setter 経路 (= mUseUBO 分岐) で UBO 化対応要、10 setter site で UBO write 経由に差替

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **通電内容**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step + η-27 + η-28 範囲、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電**: blueprint ベースで host 側 buffer 配置 + descriptor set 配線
2. **実 member data 流入**: §4 で特定済 10 setter site (= `pipeline.cpp` 内) を UBO 化、`far_clip` setter site のみ追加特定 (= **要 verify**)
3. **dirty 判定 logic 追加**:
   - PerProgram cadence + per-light dirty + per-projector dirty (= projector source / shadow index 変化時、cvar 変化時)
4. **flush logic 追加**: PerProgram cadence flush (= `writeProgramUbo` 経路)
5. **shader 接続**:
   - 実 shader `class3/deferred/spotLightF.glsl:74 ifdef LL_VULKAN_GLSL block` UBO declaration 既存 = 追加 shader 改変なし
   - 既存 OpenGL `#else` block uniform 個別宣言は温存
6. **PerProgramUBO_PointLightV (binding=5) declared-but-unused 整理**:
   - blueprint comment literal `spotLightF.glsl:151 で declared-but-unused (= host bind は同 binding 共有、frag は別 UBO 経由で size 参照、η-28-C type 3 範式)` の維持 (= 設計 doc 参照)

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 binding=10 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 48B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**10 member 統合 risk**: 10 setter site で個別 data source、それぞれ独立に変化、dirty trigger 多数。PerProgram cadence で program 切替時 batch write で対応するが、program 中の per-light / per-projector 変化への追加 trigger 設計要 (= verify 要、η-28-C type 3 範式参照)。

**PerProgramUBO_PointLightF (binding=25) との重複**: 同 `sun_wash` / `falloff` / `global_light_strength` を Point/Spot 両方で持つ = double-write 必須 (= 別 program 別 UBO instance、cvar listener から 2 UBO 同時 dirty)。

**PerProgramUBO_PointLightV (binding=5) cross-stage 共有**: η-28-C type 3 範式 (= 同 binding=5 を spot frag で declared-but-unused)、Vulkan validation 影響確認要 (= Phase 2 cold launch 時)。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 = PerDraw + PerProgram 混在

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- ubo_metadata.inl で 73 件の最大 cluster

### §11.3 同 shader consume UBO (= class3/deferred/spotLightF.glsl)

- **PerProgramUBO_PointLightV** (= set=2 binding=5、declared-but-unused 共有、η-28-C type 3)
- **不明 / verify 要**: FrameViewProj / FrameLights / FrameAtmosphere_Lighting / projector texture sampler

### §11.4 同 data source UBO (= 同 host data source から派生)

- **PerProgramUBO_PointLightF** (= set=2 binding=25、同 `sun_wash` / `falloff` / `global_light_strength` を 5 member subset で持つ)
- **PerDrawUBO_MultiLight** (= set=2 binding=1、multi-spot setter `pipeline.cpp:11628` 同 LIGHT_FALLOFF 経由)
- **PerDrawUBO_LightParams** (= set=2 binding=0、light params 別 PerDraw UBO の可能性、verify 要)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- per-light 切替時、関連 light F UBO (= PerProgramUBO_PointLightF) と同時 dirty 候補
- `RenderGlobalLightStrength` / `RenderDeferredSunWash` cvar 変化時、両 light F UBO 同時 dirty

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **`far_clip` setter call site** = `pipeline.cpp` 内で `DEFERRED_FAR_CLIP` 等の setter 行番号 (= grep verify 要、enum も verify 要)
2. **per-light / per-projector 中間 dirty trigger** = program 中の light/projector 切替時 UBO 更新経路 (= verify 要、η-28-C type 3 設計 doc 参照)
3. **`PerProgramUBO_PointLightF` (binding=25) との double-write 整合** = `sun_wash` / `falloff` / `global_light_strength` 同 cvar 由来の 2 UBO への同時 write 経路 (= verify 要)
4. **PerProgramUBO_PointLightV (binding=5) declared-but-unused SPIR-V 影響** = validation warning 発生有無 (= Phase 2 cold launch 確認)
5. **同 shader file 内同時 consume UBO 一覧** = spotLightF.glsl 内全 UBO declaration grep 要
6. **shell 通電 commit** = 未来作業
7. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = verify 要

= 上記 7 項目は本 UBO file 完成時に逐次解消。
