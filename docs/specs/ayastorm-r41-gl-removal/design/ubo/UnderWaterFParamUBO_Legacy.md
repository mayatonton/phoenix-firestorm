# UnderWaterFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.A PA-8 blueprint 起案済、Phase 1.C PC-2/PC-7δ per-program cadence 一括通電対象に含まれている可能性大 / verify 要)

**本実装化に必要な作業**: shell 14 member (= fogCol / lightDir_underwater_legacy / lightExp / specular / refScale / fbScale / znear / zfar / kd / eyeVec_underwater_legacy / waterFogColor_underwater_legacy / waterFogColorLinear / waterFogKS_underwater_legacy / screenRes) を実 underwater rendering data (= LLDrawPoolWater / LLEnvironment 水中 path、verify 要) に置換 + dirty 判定 logic 追加 + 実 shader (= class3/environment/underWaterF.glsl) consume 接続検証

**多 member UBO 注意**: 14 member 256 B device-padded、std140 144 B (= 全 member 詰込み、member size 確認済 fogCol(16) + lightDir(12) + lightExp(4) + specular(12) + refScale(4) + fbScale(8) + znear(4) + zfar(4) + kd(4) + (16 B align pad) + eyeVec(12) + (4 B pad) + waterFogColor(16) + waterFogColorLinear(12) + waterFogKS(4) + screenRes(8) = 144 B)

---

## §1. UBO identity

- **block_name**: `UnderWaterFParamUBO_Legacy`
- **block_hash**: `0x4a1973ddu`
- **block_size**: 256 B (= std140 144 B、device-padded 256 B)
- **member_count**: 14
- **struct definition** (= 実コード source 直接 reference、`build-linux-x86_64/codegen/ubo/ubo_layout_underwaterfparamubo_legacy.inl`):

```cpp
struct UnderWaterFParamUBO_LegacyLayout {
    static constexpr std::uint32_t fogCol_OFFSET = 0u;                                   // size=16 align=16
    static constexpr std::uint32_t lightDir_underwater_legacy_OFFSET = 16u;              // size=12 align=16
    static constexpr std::uint32_t lightExp_OFFSET = 28u;                                // size=4  align=4
    static constexpr std::uint32_t specular_OFFSET = 32u;                                // size=12 align=16
    static constexpr std::uint32_t refScale_OFFSET = 44u;                                // size=4  align=4
    static constexpr std::uint32_t fbScale_OFFSET = 48u;                                 // size=8  align=8
    static constexpr std::uint32_t znear_OFFSET = 56u;                                   // size=4  align=4
    static constexpr std::uint32_t zfar_OFFSET = 60u;                                    // size=4  align=4
    static constexpr std::uint32_t kd_OFFSET = 64u;                                      // size=4  align=4
    static constexpr std::uint32_t eyeVec_underwater_legacy_OFFSET = 80u;                // size=12 align=16
    static constexpr std::uint32_t waterFogColor_underwater_legacy_OFFSET = 96u;         // size=16 align=16
    static constexpr std::uint32_t waterFogColorLinear_OFFSET = 112u;                    // size=12 align=16
    static constexpr std::uint32_t waterFogKS_underwater_legacy_OFFSET = 124u;           // size=4  align=4
    static constexpr std::uint32_t screenRes_OFFSET = 128u;                              // size=8  align=8
};
inline constexpr std::uint32_t UnderWaterFParamUBO_Legacy_SIZE = 256u; // std140=144, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/under_water_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 39) uniform UnderWaterFParamUBO_Legacy
{
    vec4  fogCol;
    vec3  lightDir_underwater_legacy;
    float lightExp;
    vec3  specular;
    float refScale;
    vec2  fbScale;
    float znear;
    float zfar;
    float kd;
    vec3  eyeVec_underwater_legacy;
    vec4  waterFogColor_underwater_legacy;
    vec3  waterFogColorLinear;
    float waterFogKS_underwater_legacy;
    vec2  screenRes;
};
```

= **blueprint literal extract from `class3/environment/underWaterF.glsl:63` `#ifdef LL_VULKAN_GLSL` block** (= blueprint header line 3 明示)

**rename history**: `_underwater_legacy` suffix 付き member は nameless block member の global scope export 衝突回避 rename (= `class3/environment/underWaterF.glsl:50-62` 注記、waterV.glsl bare uniform `lightDir`/`eyeVec` + waterFogF.glsl UBO member `waterFogColor`/`waterFogKS` との衝突回避、`η-6 §3.3 範式 (nameless block × nameless block member collision)` 同形 rename pattern 適用)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 39
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: `sAYAStandardLayout`
- **set 3 内訳**: 本 UBO binding=39 = Legacy 帯独立 binding
- **source**: `ubo_metadata.inl:115` `{ "UnderWaterFParamUBO_Legacy", 0x4a1973ddu, 256u, 3u, 39u, 0u, 1u, 14u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` `kCadencePerProgram = 1u`)
- **source**: ubo_metadata.inl:115 + llglslshader.cpp:95

---

## §4. 物理 owner

- **shell 段階 owner**: 未確認
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `fogCol` = underwater fog color (vec4 RGBA、LLDrawPoolWater 由来)
  - `lightDir_underwater_legacy` = underwater light direction (vec3、sun_dir 派生)
  - `lightExp` = underwater light exponent (float)
  - `specular` = underwater specular params (vec3)
  - `refScale` = underwater reflection scale (float)
  - `fbScale` = underwater framebuffer scale (vec2)
  - `znear` / `zfar` = camera near/far planes (float、LLViewerCamera 由来)
  - `kd` = underwater diffuse coefficient (float)
  - `eyeVec_underwater_legacy` = underwater eye vector (vec3、camera position 派生)
  - `waterFogColor_underwater_legacy` = water fog color (vec4 RGBA、LLEnvironment water 由来)
  - `waterFogColorLinear` = linear-space water fog color (vec3、sRGB→linear 変換済)
  - `waterFogKS_underwater_legacy` = water fog Ks (float、scattering coefficient)
  - `screenRes` = screen resolution (vec2、framebuffer dimensions)
- **lifetime**: per-program (= underwater rendering shader instance)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/under_water_f_param_ubo_legacy.glsl` (= Phase 1.A PA-8 起案、Source `class3/environment/underWaterF.glsl:63`)
- **実 shader use site**:
  - `class3/environment/underWaterF.glsl` (= blueprint header line 3 literal reference)
- **consume status**: blueprint literal extract source ゆえ既 consume

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter** (= verify 要)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL setter = 14 member 各 uniform setter (= rename 前の名前 `lightDir` / `eyeVec` / `waterFogColor` / `waterFogKS` で `uniform3fv`/`uniform4fv`/`uniform1f` 経路、verify 要)
  - data source = LLDrawPoolWater / LLEnvironment underwater settings / LLViewerCamera / framebuffer state

---

## §7. 現状通電状態

- **状態**: **shell 通電有無 verify 要**
- **通電内容** (= 推定): zero dummy buffer write + per-program cadence 経路通電

---

## §8. 本実装化に必要な作業

1. **shell 14 member → 実 underwater rendering data 接続**:
   - LLDrawPoolWater underwater pass 内 14 member host 計算 + 集約
2. **dirty 判定 logic 追加**:
   - dirty 判定 trigger = camera position 変化 (= per-frame の可能性、cadence 再評価要) + framebuffer resize + LLEnvironment water settings 変更
3. **flush logic 追加**:
   - per-program cadence ゆえ既経路活用
4. **shader 接続検証**:
   - 既存 `class3/environment/underWaterF.glsl` `#ifdef LL_VULKAN_GLSL` block (line 63 周辺) で UBO member access 動作確認
   - rename 後 member 名 `lightDir_underwater_legacy` 等で main() 参照 (= shader 側 `#define alias` 不要、main() 内未参照と注記)

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=3 内に収める | ✅ 維持 (binding=39 独立) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 144 B → 256 B padded (= 14 member 詰込み、layout `lightDir_underwater_legacy_OFFSET=16` (vec3 後 lightExp_OFFSET=28 で vec3+float pack) + `specular_OFFSET=32` (vec3+refScale_OFFSET=44 で vec3+float pack) + `fbScale_OFFSET=48` (vec2) + `znear/zfar/kd_OFFSET=56/60/64` (3×float = 12 B) + 16 B align padding + `eyeVec_underwater_legacy_OFFSET=80` (vec3 + 4 B implicit pad) + `waterFogColor_underwater_legacy_OFFSET=96` (vec4) + `waterFogColorLinear_OFFSET=112` (vec3 + waterFogKS_OFFSET=124 で vec3+float pack) + `screenRes_OFFSET=128` (vec2) = 136 B end + 8 B → 144 B std140 alignment) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint = 実 shader literal extract |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**multi-member rename history**:
- 4 member (lightDir / eyeVec / waterFogColor / waterFogKS) で `_underwater_legacy` suffix rename 適用 (= η-4/η-6 §3.2/§3.3 範式)
- rename 理由: waterV.glsl bare uniform `lightDir`/`eyeVec` + waterFogF.glsl UBO member `waterFogColor`/`waterFogKS` との nameless block member global scope export 衝突
- shader main() 内未参照ゆえ `#define alias` 不要、GL `#else` path 不可触 (= charter §3 #1 担保)

**cadence 再評価候補**:
- `eyeVec_underwater_legacy` / `screenRes` / `znear` / `zfar` は per-frame で変化、per-program cadence stale risk
- per-frame 系への移動候補、Phase 2 で再評価

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- Asset/Skin 帯 + Legacy 帯各種、本 UBO binding=39 は独立

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- PerProgram cluster 全 88 件

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- `class3/environment/underWaterF.glsl` 内同時 consume UBO (= 既 grep 結果、underWaterF.glsl 内に FrameLights 直接宣言確認、`#ifndef FRAME_LIGHTS_DEFINED` guard wrap):
  - **FrameLights** (set=0 binding=1、PerFrame、underWaterF.glsl:94-100 literal block 確認)
  - 推定他候補: FrameViewProj (set=0 binding=0) + FrameAtmosphere_Lighting (set=0 binding=2) + WaterFogUBO_Legacy (set=3 binding=9、waterFog 系 data source 共有可能性)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **WaterFogUBO_Legacy** (= set=3 binding=9、PerProgram、5 member、ubo_metadata.inl:118) = water fog UBO、本 UBO `waterFogColor_underwater_legacy` / `waterFogKS_underwater_legacy` と同 data source (= waterFogF.glsl の `waterFogColor` / `waterFogKS` member rename と一致、`class3/environment/underWaterF.glsl:57-62` 注記)
- **WaterVParamUBO_Legacy** (= set=3 binding=60、PerProgram、6 member、ubo_metadata.inl:119) = water V UBO、`lightDir` / `eyeVec` 同名 member rename source、本 UBO の rename 4 member の 2 件と data source 共有
- **PerProgramUBO_WaterF** (= set=2 binding=23、PerProgram、8 member、ubo_metadata.inl:92) = water F UBO、water rendering 共通 data source

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- **WaterFogUBO_Legacy** / **WaterVParamUBO_Legacy** / **PerProgramUBO_WaterF** (= water rendering 系 同時 dirty 高確率、camera move + framebuffer resize で連動)
- FrameViewProj (= per-frame camera state、cadence 異なる)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- per-program cadence ゆえ underWaterF program bind 時に descriptor set 3 更新

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電有無** = handoff doc chain 参照要
2. **14 member 各 data source** = LLDrawPoolWater underwater pass / LLEnvironment underwater settings / LLViewerCamera / framebuffer state 経路 (= verify 要)
3. **既存 OpenGL setter call site** = 14 member 各 uniform setter grep verify 要 (rename 前の名前で grep)
4. **cadence 適正性** = per-frame 変化 member (eyeVec / screenRes / znear / zfar) の per-program cadence stale risk (= verify 要)
5. **WaterFogUBO_Legacy / WaterVParamUBO_Legacy との data source 共有経路** = 同 host data source から派生する member の重複書込み回避 (= verify 要、duplicate write 性能 risk)
6. **rename 4 member の shader 側 main() 参照状況** = blueprint header note "本 file の main() 内で参照されない" 確認要 (= verify 要、shader 実コード参照確認)
7. **同 shader consume UBO 完全特定** = class3/environment/underWaterF.glsl 内同時 consume UBO 群 (= 既 FrameLights 確認、他 grep verify 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。
