# FrameLights — UBO design (= 実コードベース調査資料)

**通電状態**: shell 通電済 + write 経路本格化済 (= Phase 1.A PA-8 blueprint 起案 + 1.C PC-7γ-1 で `sFrameUboInstances` allocate 完了、`writeFrameUbo` 経路 PER_FRAME case 通電済、実 shader UBO block は `#ifdef LL_VULKAN_GLSL` block 内 sumLightsV/sumLightsSpecularV/waterFogF/atmosphericsV/sumLightsV/simpleColorF/cinematic_bd/shadowUtil で宣言済)

**本実装化に必要な作業**: 既存 light_position[8] / light_diffuse[8] 等の 8 array uniform setter 群を forwardToUboUpload PER_FRAME 経由 UBO redirect 通電 + `mUseUBO=true` cold launch 検証 + per-shader UBO block 宣言拡大

---

## §1. UBO identity

- **block_name**: `FrameLights`
- **block_hash**: `0xed61ac9bu` (= `ubo_metadata.inl:41` literal)
- **block_size**: 768 B (= std140=704, device-padded=768、`ubo_layout_framelights.inl:23` literal)
- **member_count**: 9
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_framelights.inl
struct FrameLightsLayout {
    static constexpr std::uint32_t sun_up_factor_OFFSET              = 0u;   // size=4   align=4
    static constexpr std::uint32_t sun_dir_OFFSET                    = 16u;  // size=12  align=16
    static constexpr std::uint32_t moon_dir_OFFSET                   = 32u;  // size=12  align=16
    static constexpr std::uint32_t waterPlane_OFFSET                 = 48u;  // size=16  align=16
    static constexpr std::uint32_t light_position_OFFSET             = 64u;  // size=128 align=16 stride=16
    static constexpr std::uint32_t light_direction_OFFSET            = 192u; // size=128 align=16 stride=16
    static constexpr std::uint32_t light_attenuation_OFFSET          = 320u; // size=128 align=16 stride=16
    static constexpr std::uint32_t light_diffuse_OFFSET              = 448u; // size=128 align=16 stride=16
    static constexpr std::uint32_t light_deferred_attenuation_OFFSET = 576u; // size=128 align=16 stride=16
};
inline constexpr std::uint32_t FrameLights_SIZE = 768u; // std140=704, device-padded=768
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set0/frame_lights.glsl
layout(std140, set = 0, binding = 1) uniform FrameLights
{
    int  sun_up_factor;
    vec3 sun_dir;
    vec3 moon_dir;
    vec4 waterPlane;
    vec4 light_position[8];
    vec3 light_direction[8];
    vec4 light_attenuation[8];
    vec3 light_diffuse[8];
    vec2 light_deferred_attenuation[8];
};
```

= **8 light slot 配列 (= local light 上限 8)、std140 で 16 B stride 適用**、Frame 系最大 size (= 768 B vs FrameViewProj 512 B vs FrameAtmosphere 256 B)

---

## §2. binding 配線

- **descriptor_set**: 0
- **binding**: 1
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout` (= `llvkloader.cpp:881` literal)
- **set 0 内訳** (= `llvkloader.cpp:858` `V3A_FRAME_SET_BINDINGS = 4` literal):
  - set=0 binding=0 = FrameViewProj
  - set=0 binding=1 = **FrameLights (本 UBO)**
  - set=0 binding=2 = FrameAtmosphere_Lighting
  - set=0 binding=3 = Global_ReflectionProbes
- **source**: `ubo_metadata.inl:41` `{ "FrameLights", 0xed61ac9bu, 768u, 0u, 1u, 0u, 0u, 9u }` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 0
- **意味**: **per-frame** (= `llglslshader.cpp:94` literal: `constexpr U32 kCadencePerFrame = 0u; // FrameAtmosphere_Lighting / FrameLights / FrameViewProj`)
- **flush 経路**: `LLVKLoader::flushFrameUbos()` (= `llvkloader.cpp:5117`)
- **write 経路**: `LLVKLoader::writeFrameUbo(block_hash, offset, data, size)` (= `llglslshader.cpp:2144`)
- **storage**: `sFrameUboInstances` map<block_hash, UboInstance> (= `llvkloader.cpp:653`)

---

## §4. 物理 owner

- **owner**: light state aggregator (= verify 要、推定 `LLPipeline::setupHWLights` / `LLPipeline::calcNearbyLights` 等)
- **data source**:
  - `sun_up_factor` = day/night discriminator (= verify 要、推定 `LLEnvironment::isDaytime`)
  - `sun_dir` / `moon_dir` = sun / moon direction (= verify 要、`LLShaderMgr::SUN_DIR` / `MOON_DIR` 経由)
  - `waterPlane` = water plane equation (= `pipeline.cpp` 内 water rendering 経路、verify 要)
  - `light_position[8]` = 8 local light position (= `pipeline.cpp` setupHWLights 経由、`uniform vec4 light_position[8]` raw uniform 並走)
  - `light_direction[8]` / `light_attenuation[8]` / `light_diffuse[8]` / `light_deferred_attenuation[8]` = 同 8 light slot 配列
- **lifetime**: per-frame (= light state は frame 開始時に計算 + 確定)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set0/frame_lights.glsl` (= Phase 1.A PA-8 起案)
- **blueprint origin (literal extract source)**: `class1/windlight/atmosphericsV.glsl:33` (= `frame_lights.glsl:4` literal comment)
- **実 shader use site** (= UBO block 宣言済 shader、grep verify):
  - `class1/lighting/sumLightsV.glsl:31-39` (= 9 member 宣言)
  - `class1/lighting/sumLightsSpecularV.glsl:39-47` (= 9 member 宣言)
  - `class1/environment/waterFogF.glsl:33-41` (= 9 member 宣言)
  - 他 verify 要 (= atmosphericsV / atmosphericsFuncs / simpleColorF / cinematic_bd/shadowUtil = blueprint origin sample 5 件)
- **raw uniform 並走 pattern** (= `#else` block):
  - `sumLightsV.glsl:43-44` literal: `uniform vec4 light_position[8]; uniform vec3 light_diffuse[8];`
  - = OpenGL path で raw uniform 維持、Vulkan path で UBO block 化

---

## §6. 既存 setter call site (host C++)

- **light_position/direction/attenuation/diffuse setter**: 不明 / verify 要 (= `pipeline.cpp::setupHWLights` 想定、`shader->uniform4fv(LIGHT_POSITION, 8, ...)` 等の array setter 経路)
- **sun_dir / moon_dir setter**: 不明 / verify 要 (= `LLShaderMgr::SUN_DIR` / `LLShaderMgr::MOON_DIR` 経由、environment system 由来)
- **waterPlane setter**: 不明 / verify 要 (= water rendering 経路、`pipeline.cpp` 内 water plane uniform setter site)
- **sun_up_factor setter**: 不明 / verify 要 (= day/night transition 用 int)
- **共通 redirect 経路**: `LLGLSLShader::uniform4fv` / `uniform3fv` / `uniform1i` / `uniform2fv` (= 既 PC-7γ-1 で `mUseUBO=true` 時 `forwardToUboUpload(loc, ...)` 経路接続済)
- **PER_FRAME case**: `forwardToUboUpload` 内 `case kCadencePerFrame: LLVKLoader::writeFrameUbo(loc.block_hash, loc.offset, data, size);` (= `llglslshader.cpp:2143-2145`)
- **array uniform 経路**: `uniform4fv(index, count=8, v)` → `forwardToUboUpload(loc, v, count * 4 * sizeof(GLfloat))` (= `llglslshader.cpp:3037` literal)

---

## §7. 現状通電状態

- **状態**: **shell 通電済 + write 経路本格化済** (= Phase 1.A PA-8 blueprint 起案 + 1.C PC-7γ-1)
- **bind 経路**: `llvkloader.cpp:2839` `V3A_FRAME_SET_BINDINGS` 経由 set=0 全 4 UBO 同時 bind
- **write 経路**: `forwardToUboUpload` PER_FRAME case → `writeFrameUbo` → memcpy + dirty.store(true)
- **MUSEUBO-A 整合**: `mUseUBO=false` default で本 entry 不到達 = 既存 OpenGL 描画 100% 維持

---

## §8. 本実装化に必要な作業

1. **`mUseUBO=true` cold launch 検証**:
   - `pipeline.cpp::setupHWLights` 等 light state 集約点で 9 member 全 writeFrameUbo 通電確認
   - 8 array stride=16 (= vec3 → vec4 padding) memcpy 正常性 verify
2. **per-shader UBO consume 拡大**:
   - 現 blueprint origin 5 sample + grep ヒット 3 file (sumLightsV / sumLightsSpecularV / waterFogF) で UBO block 宣言済
   - 残 lighting shader (= class1/lighting/* + class1/environment/* + class3/deferred/softenLightF など local light consume site) に対し UBO block 宣言拡大要
3. **setter site 完全特定**:
   - light_position/direction/attenuation/diffuse の 4 array setter (= 8 element 配列) 呼出 site grep 要
   - sun_dir / moon_dir / waterPlane / sun_up_factor の単発 setter site grep 要
4. **std140 vec3 array padding 検証**:
   - `light_direction[8]` (vec3 stride=16) / `light_diffuse[8]` (vec3 stride=16) / `light_deferred_attenuation[8]` (vec2 stride=16) の 16 B stride で host C++ data layout と一致確認
   - 既存 raw uniform 経路 = `uniform vec3 light_diffuse[8]` も std140 stride 16 適用 (= OpenGL も std140 で同 layout) ゆえ整合と推定
5. **codegen 再実行不要**:
   - blueprint member 不変、layout 改変なし

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=0 内 | ✅ 維持 (= set 0 binding=1) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 704 B std140 → 768 B padded、stride=16 ×8 element 計算済 |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 (= 768 B = 256 B alignment 倍数充足) |
| OS-5 | shader 改変ゼロ | ⚠️ **per-shader UBO block 拡大必須**、`#ifdef LL_VULKAN_GLSL` gate で OpenGL 100% 維持 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**8 array stride risk**:
- vec3 を 8 element 配列にすると std140 stride=16 適用 = 128 B 占有 (= float×4×8)
- 既存 OpenGL `uniform vec3 light_diffuse[8]` も同 std140 layout を採用していると推定 (= verify 要、raw uniform でも std140 layout 規則は同じ)

**Frame 系最大 size**:
- 768 B = Frame 系 4 UBO 中最大、per-frame で 4 array 全更新で memcpy 量大
- ring buffer / mapped pointer 経路 perf verify 要

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=0)

- FrameViewProj (set=0 binding=0)
- **FrameLights (set=0 binding=1、本 UBO)**
- FrameAtmosphere_Lighting (set=0 binding=2)
- Global_ReflectionProbes (set=0 binding=3)

### §11.2 同 cadence cluster UBO (= cadence_tag=0 per-frame、`flushFrameUbos` 共通経路)

- FrameViewProj
- **FrameLights (本 UBO)**
- FrameAtmosphere_Lighting

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- 推定: lighting shader (sumLightsV / sumLightsSpecularV / waterFogF) で同時 consume = FrameAtmosphere_Lighting (= 同 atmospheric 計算経路、blueprint origin が atmosphericsV)
- = `class1/lighting/sumLightsV.glsl` + `class1/windlight/atmosphericsV.glsl` で同 set=0 binding=2 consume の可能性高 (= verify 要)
- vertex stage で modelview 経由 light_position transform → FrameViewProj も同時 consume

### §11.4 同 data source UBO (= 同 host data source から派生)

- **同 owner**: `LLPipeline` light state aggregator (= 推定)
- 他 UBO で同 owner = `FrameAtmosphere_Lighting` (= 推定、同 environment system 由来の atmospheric color + light 連動)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- 推定: `FrameAtmosphere_Lighting` (= environment 変化 = 日中夜遷移時に sun_dir/moon_dir + sunlight_color/moonlight_color 同時更新)
- verify 要 (= dirty trigger 経路調査要)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- frame start で set=0 全 4 UBO 同時 bind (= `llvkloader.cpp:2839` `vkCmdBindDescriptorSets` 1 回呼出)

---

## §10. 不明事項

1. **`light_position[8]` 等 array setter call site** = `pipeline.cpp::setupHWLights` 内 setter 経路 grep 要、推定 `shader->uniform4fv(LIGHT_POSITION, 8, light_pos_array)` 形式
2. **`sun_dir` / `moon_dir` setter call site** = `LLShaderMgr::SUN_DIR` / `MOON_DIR` 経由 (= verify 要、environment system 由来)
3. **`waterPlane` setter call site** = water rendering 経路、pipeline.cpp 内 water plane uniform setter
4. **`sun_up_factor` setter call site** = day/night transition 用 int (= verify 要)
5. **UBO block 宣言済 shader 全列挙** = grep verify (= blueprint origin 5 sample + ヒット 3 file = 8 件確認、残 lighting/environment shader の UBO block 化状況)
6. **8 array stride=16 host data layout** = host C++ side で std140 stride 適用済の data 構造 (= 推定 raw uniform 経路と同一、verify 要)
7. **dirty 連動 detail** = environment transition 時の同時 dirty trigger 経路 (= FrameAtmosphere_Lighting との連動 dirty 判定)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。
