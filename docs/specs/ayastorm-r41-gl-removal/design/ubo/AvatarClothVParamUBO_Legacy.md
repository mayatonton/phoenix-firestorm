# AvatarClothVParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= shell 通電なし、host C++ writer / register 配線無し、shader 側 LL_VULKAN_GLSL block でのみ宣言済)

**本実装化に必要な作業**: per-program cadence register / write 配線追加 + 既存 OpenGL 経路 setter (= avatar cloth wind/sinwave/gravity uniform setter site) の mUseUBO 分岐経路から `forwardToUboUpload` → `writeProgramUbo` 経由で本 UBO に書込み開始 + `avatarV.glsl` の LL_VULKAN_GLSL block 活性化

---

## §1. UBO identity

- **block_name**: `AvatarClothVParamUBO_Legacy`
- **block_hash**: `0x13e3ee95u`
- **block_size**: 256 B (= std140 48 B、device-padded 256 B)
- **member_count**: 3
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_avatarclothvparamubo_legacy.inl:12-16
struct AvatarClothVParamUBO_LegacyLayout {
    static constexpr std::uint32_t gWindDir_OFFSET = 0u;        // size=16 align=16
    static constexpr std::uint32_t gSinWaveParams_OFFSET = 16u; // size=16 align=16
    static constexpr std::uint32_t gGravity_OFFSET = 32u;       // size=16 align=16
};
inline constexpr std::uint32_t AvatarClothVParamUBO_Legacy_SIZE = 256u; // std140=48, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/avatar_cloth_v_param_ubo_legacy.glsl:9-14
layout(std140, set = 3, binding = 57) uniform AvatarClothVParamUBO_Legacy
{
    vec4 gWindDir;
    vec4 gSinWaveParams;
    vec4 gGravity;
};
```

= **avatar cloth simulation params** (wind direction / sine wave / gravity)、3 vec4 = 48 B 実 data + 208 B padding

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 57
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **set=3 内 Legacy 帯**: binding=57 (= avatar 系 Legacy 高 binding 帯)
- **source**: `ubo_metadata.inl:31` literal: `{ "AvatarClothVParamUBO_Legacy", 0x13e3ee95u, 256u, 3u, 57u, 0u, 1u, 3u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: avatar cloth program bind 単位で update、`sProgramUboDirty` triple-buffer 経路で flush
- **source**: ubo_metadata.inl:31 + llglslshader.cpp:95 + llglslshader.cpp:2147-2149

---

## §4. 物理 owner

- **data source**:
  - `gWindDir` (vec4): wind direction vector (= 推定 = LLWind / WL wind 由来、verify 要)
  - `gSinWaveParams` (vec4): sine wave params (= 推定 = cloth animation phase / amplitude / frequency packed)
  - `gGravity` (vec4): gravity vector
- **uniform 名 reserved**: `llshadermgr.cpp:1777-1779` literal:
  ```cpp
  mReservedUniforms.push_back("gWindDir");
  mReservedUniforms.push_back("gSinWaveParams");
  mReservedUniforms.push_back("gGravity");
  ```
- **uniform 名 ↔ enum**: `llshadermgr.h:290-292` literal:
  ```cpp
  AVATAR_WIND,        //  "gWindDir"
  AVATAR_SINWAVE,     //  "gSinWaveParams"
  AVATAR_GRAVITY,     //  "gGravity"
  ```
- **既存 OpenGL 経路 writer site**: **不明 / verify 要** (= grep `AVATAR_WIND` / `AVATAR_SINWAVE` / `AVATAR_GRAVITY` で特定要、推定 `llvoavatar.cpp` or `lldrawpoolavatar.cpp`)
- **lifetime**: avatar cloth simulation tick / shader bind 時

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/avatar_cloth_v_param_ubo_legacy.glsl`
- **実 shader use site** (= 1 file confirmed):
  - `indra/newview/app_settings/shaders/class1/deferred/avatarV.glsl:109` (= blueprint source literal、`#ifdef LL_VULKAN_GLSL` block)
- **既存 OpenGL 経路**: 同 file 内 `#else` block で uniform 個別宣言

---

## §6. 既存 setter call site (host C++)

- **uniform 名 reserved**: `llshadermgr.cpp:1777-1779` literal
- **uniform 名 ↔ enum**: `llshadermgr.h:290-292` literal
- **writer call site**: **不明 / verify 要** (= grep `AVATAR_WIND` / `AVATAR_SINWAVE` / `AVATAR_GRAVITY` で writer 特定要、推定 `lldrawpoolavatar.cpp` の cloth simulation tick update)
- **本 UBO 名指 setter**: なし (= `Grep "AvatarClothVParamUBO_Legacy" indra/llrender` 結果 0 件)

---

## §7. 現状通電状態

- **状態**: **untouched** (= shell 通電もされていない)
- **PC-7γ-1 PerProgram register**: 条件付き = mUseUBO=true 時 avatarV.glsl link 時に走る
- **PC-7γ-1 PerProgram write**: 条件付き = mUseUBO=true で setter が走ると writeProgramUbo に流れる (default OFF)

---

## §8. 本実装化に必要な作業

1. **mUseUBO ON 化**
2. **shader 側 LL_VULKAN_GLSL block 活性化** (= avatarV.glsl:109、起案済確認要)
3. **writer call site 特定** = grep `AVATAR_WIND` / `AVATAR_SINWAVE` / `AVATAR_GRAVITY` で特定要
4. **setter 経路 verify** (= mUseUBO=true 時の forwardToUboUpload → writeProgramUbo dispatch 確認)
5. **codegen 再実行不要** (= layout / member 不変)
6. **cloth simulation tick との同期確認** (= per-frame update 経路で stale data risk あり、verify 要)

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set=3 binding=57) |
| OS-3 | std140 padding 厳守 | ✅ 48B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ⚠️ avatarV.glsl に LL_VULKAN_GLSL block 追加済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**cloth simulation tick との cadence 不一致 risk**:
- cloth は per-frame physics tick で update、PerProgram cadence は shader bind 毎 ≈ per-frame で実質一致 (= verify 要)
- 但し 1 frame 内 multi-pass で本 program が複数 bind される場合 PerProgram で stale data の可能性 (= verify 要)

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守)

1. **writer call site 特定** = `AVATAR_WIND` / `AVATAR_SINWAVE` / `AVATAR_GRAVITY` setter (推定 lldrawpoolavatar.cpp)
2. **gSinWaveParams packing 詳細** = 4 component packing 意味 (= phase / amp / freq / time?)
3. **gWindDir / gGravity data source** = wind は WL wind / parcel wind の何経由か、gravity は固定 vec4(0,0,-9.81,0) か可変か
4. **update cadence** = per-frame か per-tick か
5. **cloth simulation enable cvar** = AYAstorm 側で cloth disable された avatar での通電有無

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

avatar 系 Legacy 帯:
- **AvatarClothVParamUBO_Legacy (binding=57、本 UBO)**
- AvatarFParamUBO_Legacy (binding=54)
- AvatarAlphaShadowVParamUBO_Legacy (binding=22)
- PbrOpaqueVParamUBO_Legacy (binding=53)
- VelocityVParamUBO_Legacy (binding=55)
- RlvFParamUBO_Legacy (binding=56)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 同 avatar 系 PerProgram cluster (= AvatarFParamUBO_Legacy / AvatarAlphaShadowVParamUBO_Legacy)

### §11.3 同 shader consume UBO

- avatarV.glsl 内同時 consume: 不明 / verify 要 (= 推定 FrameViewProj + PerDrawUBO_AvatarSkin or PerDrawUBO_ObjectSkin)

### §11.4 同 data source UBO

- **WaterVParamUBO_Legacy** (binding=60) = water + cloth は wind direction を共有候補 (= verify 要、推定共有あり)

### §11.5 dirty 連動 UBO

- wind direction 変化時の連動 dirty 候補 (= 推定 WaterVParamUBO_Legacy + 水面シミュ系)
- gravity 変化は希少 (= sim transition 時のみ)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- set=3 帯 bind は `bindV3aStatic` / `bindV3aRigged` で全帯一括
- avatar cloth program bind 時 PerProgram cadence triple-buffer flush

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.4.8 同期)

**Layer**: L3-8 (= B Tier β setter 推定済、PerProgram cadence、avatar cloth simulation 3 vec4)
**status**: **起案済** (= 2026-06-06 C-5、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.4.8` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch + L0-4 cadence
- **(2) 不明事項**: writer call site (= `lldrawpoolavatar.cpp` cloth simulation tick update 推定) [要追加調査] / `gSinWaveParams` 4 component 意味 (= phase/amplitude/frequency packed?) [要 verify] / cloth simulation tick cadence (= per-frame か独立 tick か) [要 verify]
- **(3) 調査手法**: D1 (`AVATAR_WIND` / `AVATAR_SINWAVE` / `AVATAR_GRAVITY` setter grep) + D3 (cloth simulation tick 頻度)
- **(4) 設計 task**: L3 全件共通 (= PerProgram triple-buffer / `forwardToUboUpload` PER_PROGRAM / program bind 単位 flush / `avatarV.glsl` LL_VULKAN_GLSL 活性化)
- **(5) 工程**: trace L3-8、工数 **S**、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= avatar cloth animation 既存と同一、visual regression ゼロ §5.4)
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `avatarV.glsl #else` block uniform 個別宣言維持

**関連**: L0-1 + L0-4 / §5.4 / `lldrawpoolavatar.cpp` (= cloth simulation tick dispatcher)
