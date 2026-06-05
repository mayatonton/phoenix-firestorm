# PerProgramUBO_PointLightF — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `per_program_ubo_point_light_f.glsl` は実 shader `class3/deferred/pointLightF.glsl:53 ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_PointLightF`
- **block_hash**: `0xfbfefe87u` (= FNV-1a("PerProgramUBO_PointLightF"))
- **block_size**: 256 B (= std140 32 B、device-padded 256 B)
- **member_count**: 5 (= 4 active + 1 tail pad)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_pointlightf.inl
struct PerProgramUBO_PointLightFLayout {
    static constexpr std::uint32_t viewport_OFFSET = 0u;  // size=16 align=16
    static constexpr std::uint32_t sun_wash_OFFSET = 16u;  // size=4 align=4
    static constexpr std::uint32_t falloff_OFFSET = 20u;  // size=4 align=4
    static constexpr std::uint32_t global_light_strength_OFFSET = 24u;  // size=4 align=4
    static constexpr std::uint32_t _pad0_OFFSET = 28u;  // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_PointLightF_SIZE = 256u; // std140=32, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_point_light_f.glsl
layout(std140, set = 2, binding = 25) uniform PerProgramUBO_PointLightF
{
    vec4  viewport;
    float sun_wash;
    float falloff;
    float global_light_strength;
    float _pad0;
};
```

= active member 4 個 + tail pad 1 個。blueprint comment literal: `Source: literal extract from class3/deferred/pointLightF.glsl:53 ifdef LL_VULKAN_GLSL block (single site)` + **`sun_wash は dead uniform (host setter あり、shader 本体未参照)`** (= shader.glsl:46-47 literal: `sun_wash は GLSL 本体未参照 dead-uniform だが parse 通過のため UBO 含める`)。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 25
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通)
- **pipeline layout**: `sAYAStandardLayout`
- **set 2 配置**: PerDraw + PerProgram 帯
- **source**: `ubo_metadata.inl:82` `{ "PerProgramUBO_PointLightF", 0xfbfefe87u, 256u, 2u, 25u, 0u, 1u, 5u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush、program 単位で値を保持。Point light shader は program 単位で viewport / sun_wash / falloff / global_light_strength を保持
- **source**: ubo_metadata.inl:82 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source**:
  - `viewport` = `LLPipeline` (= `pipeline.cpp:10537-10540` `shader.uniform4f(LLShaderMgr::VIEWPORT, gGLViewport[0], ...)`)
  - `sun_wash` (= dead) = `LLPipeline` (= `pipeline.cpp:10628` `shader.uniform1f(LLShaderMgr::DEFERRED_SUN_WASH, RenderDeferredSunWash)`)
  - `falloff` = `LLDrawable` (= `pipeline.cpp:11489` `gDeferredLightProgram.uniform1f(LLShaderMgr::LIGHT_FALLOFF, volume->getLightFalloff(DEFERRED_LIGHT_FALLOFF))`)
  - `global_light_strength` = `RenderGlobalLightStrength` cvar (= `pipeline.cpp:10705` / `10712`)
- **lifetime**: program 単位

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_point_light_f.glsl`
- **実 shader use site**: **`class3/deferred/pointLightF.glsl:53 ifdef LL_VULKAN_GLSL block`** (= single site)
  - 既存 UBO block (pointLightF.glsl:54-57):
    ```glsl
    vec4  viewport;                // offset 0
    float sun_wash;                // offset 16 (dead uniform、host setter あり / GLSL 本体未参照)
    float falloff;                 // offset 20
    float global_light_strength;   // offset 24
    ```
  - 既存 OpenGL `#else` block (pointLightF.glsl:62, :80, :118, :153):
    ```glsl
    uniform float sun_wash;
    uniform float falloff;
    uniform vec4 viewport;
    uniform float global_light_strength;
    ```
  - 使用箇所: `pointLightF.glsl:209` (`float dist_atten = calcLegacyDistanceAttenuation(dist, falloff)`) + `:286` (`final_color *= global_light_strength`)
  - `sun_wash` は **shader 本体未参照** (= dead uniform、shader comment literal `sun_wash は GLSL 本体未参照 dead-uniform だが parse 通過のため UBO 含める`)
  - `viewport` の shader 本体での使用 = **要 verify** (grep 結果に直接 `viewport` 使用箇所は見えず、screen-space 系で使用と推定)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **既存 OpenGL 経路 setter**:
  - `llshadermgr.h:73` `VIEWPORT, //  "viewport"` + `:156` `LIGHT_FALLOFF, //  "falloff"` + `:175` `DEFERRED_SUN_WASH, //  "sun_wash"` + `:416` `DEFERRED_LIGHT_STRENGTH, //  "global_light_strength"`
  - **`pipeline.cpp:10537-10540`** `if (shader.getUniformLocation(LLShaderMgr::VIEWPORT) != -1) shader.uniform4f(LLShaderMgr::VIEWPORT, gGLViewport[0], ...)`
  - **`pipeline.cpp:10628`** `shader.uniform1f(LLShaderMgr::DEFERRED_SUN_WASH, RenderDeferredSunWash)`
  - **`pipeline.cpp:11489`** `gDeferredLightProgram.uniform1f(LLShaderMgr::LIGHT_FALLOFF, volume->getLightFalloff(DEFERRED_LIGHT_FALLOFF))`
  - **`pipeline.cpp:10705`** `shader.uniform1f(LLShaderMgr::DEFERRED_LIGHT_STRENGTH, RenderGlobalLightStrength)` + **`:10712`** `shader.uniform1f(LLShaderMgr::DEFERRED_LIGHT_STRENGTH, 1.0f)`
- **本実装化後 setter**:
  - 31 setter 経路 (= mUseUBO 分岐) で UBO 化対応要

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **通電内容**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 範囲)

---

## §8. 本実装化に必要な作業

1. **shell 通電**: blueprint ベースで host 側 buffer 配置 + descriptor set 配線
2. **実 member data 流入**:
   - `viewport`: pipeline.cpp:10537-10540 setter を UBO 化
   - `sun_wash` (dead): pipeline.cpp:10628 setter を UBO 化 (= shader 未参照だが host setter は維持、layout 不変契約)
   - `falloff`: pipeline.cpp:11489 setter を UBO 化
   - `global_light_strength`: pipeline.cpp:10705/10712 setter を UBO 化
3. **dirty 判定 logic 追加**:
   - PerProgram cadence + per-light 切替時 dirty (= falloff は light 単位で変化、要追加 trigger)
4. **flush logic 追加**: PerProgram cadence flush (= `writeProgramUbo` 経路)
5. **shader 接続**:
   - 実 shader `class3/deferred/pointLightF.glsl:53 ifdef LL_VULKAN_GLSL block` UBO declaration 既存 = 追加 shader 改変なし
   - 既存 OpenGL `#else` block uniform 個別宣言は温存
6. **dead member 維持**: `sun_wash` (16 B offset) は維持 (= layout 不変契約)、host setter は OpenGL 経路では生きてる、Vulkan 経路では UBO write してよい (= shader 未参照のため dead store)

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 binding=25 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 32B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**`falloff` per-light dirty**: `falloff` は per-light (volume->getLightFalloff) で変化、PerProgram cadence では light 切替を捕捉できない → 描画毎に light volume bind の都度 UBO write が必要 (= verify 要、Phase 2 で per-draw cadence 移行検討)。

**`viewport` shader 使用箇所**: shader 本体内での `viewport` 参照箇所が grep で直接見えず、screen-space 計算で `gl_FragCoord` と組合せて使用と推定 (= verify 要)。

**dead `sun_wash` 維持必須**: layout size 32 B 確定の前提として `sun_wash` slot 維持必須、shader 本体未参照でも UBO member は削除不可。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 = PerDraw + PerProgram 混在

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- ubo_metadata.inl で 73 件の最大 cluster

### §11.3 同 shader consume UBO (= class3/deferred/pointLightF.glsl)

- **PerProgramUBO_PointLightV** (= set=2 binding=5、本 pointLightF と pair で point light V/F program で同 program に bind の可能性、verify 要)
- **不明 / verify 要**: FrameViewProj / FrameLights / FrameAtmosphere_Lighting 等

### §11.4 同 data source UBO (= 同 host data source から派生)

- **PerProgramUBO_SpotLightF** (= set=2 binding=10、同 `sun_wash` / `falloff` / `global_light_strength` を 10 member 統合で持つ、本 PointLightF はその subset)
- **PerDrawUBO_MultiLight** (= set=2 binding=1、`pipeline.cpp:11625-11628` 同 LIGHT_CENTER / LIGHT_SIZE / LIGHT_FALLOFF 経由)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- viewport 変化時 (= window resize) で関連 viewport UBO (= FrameViewProj 等) と同時 dirty 候補
- light 切替時 (= falloff per-light) で PerDrawUBO_MultiLight と同時 dirty 候補

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **`viewport` shader 本体使用箇所** = pointLightF.glsl 内で `viewport` がどう使われるか (= grep verify 要、`gl_FragCoord / viewport.xy` 等 screen-space 計算想定)
2. **`falloff` per-light cadence ミスマッチ対応** = light volume bind 毎の UBO write 経路の存在 (= verify 要、Phase 2 設計判断)
3. **dead `sun_wash` host setter 維持**: UBO 化後 OpenGL 経路で setter を残すか削除するか (= layout 不変契約上 UBO 側維持必須だが host 側自由度あり)
4. **PerProgramUBO_SpotLightF との data source 重複** = 同 viewport / falloff / global_light_strength を別 UBO で persisted = double-write 必要か (= verify 要)
5. **shell 通電 commit** = 未来作業
6. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = verify 要
7. **同 shader file 内同時 consume UBO 一覧** = pointLightF.glsl 内 grep 要

= 上記 7 項目は本 UBO file 完成時に逐次解消。
