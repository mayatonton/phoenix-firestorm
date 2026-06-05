# AvatarFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= shell 通電なし、host C++ writer / register 配線無し、shader 側 LL_VULKAN_GLSL block でのみ宣言済)

**本実装化に必要な作業**: per-program cadence register / write 配線追加 + 既存 OpenGL 経路 setter (= `aya_sss_skin_flag` uniform setter site) の mUseUBO 分岐経路から `forwardToUboUpload` → `writeProgramUbo` 経由で本 UBO に書込み開始 + `avatarF.glsl` の LL_VULKAN_GLSL block 活性化

---

## §1. UBO identity

- **block_name**: `AvatarFParamUBO_Legacy`
- **block_hash**: `0x30bd28b9u`
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 4 (= 1 actual + 3 padding)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_avatarfparamubo_legacy.inl:12-17
struct AvatarFParamUBO_LegacyLayout {
    static constexpr std::uint32_t aya_sss_skin_flag_OFFSET = 0u;       // size=4 align=4
    static constexpr std::uint32_t _pad_avatar_f_legacy_0_OFFSET = 4u;  // size=4 align=4
    static constexpr std::uint32_t _pad_avatar_f_legacy_1_OFFSET = 8u;  // size=4 align=4
    static constexpr std::uint32_t _pad_avatar_f_legacy_2_OFFSET = 12u; // size=4 align=4
};
inline constexpr std::uint32_t AvatarFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/avatar_f_param_ubo_legacy.glsl:9-15
layout(std140, set = 3, binding = 54) uniform AvatarFParamUBO_Legacy
{
    float aya_sss_skin_flag;
    float _pad_avatar_f_legacy_0;
    float _pad_avatar_f_legacy_1;
    float _pad_avatar_f_legacy_2;
};
```

= **AYAstorm r20 SSS (Subsurface Scattering) skin flag** (= 唯一の actual member、3 padding member は std140 align 整合)。AYA r20 章追加 cvar = `<FS:AYA r20 Phase C>` 記載 (= `llshadermgr.cpp:1611` literal)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 54
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **set=3 内 Legacy 帯**: binding=54 (= avatar 系 Legacy 高 binding 帯)
- **source**: `ubo_metadata.inl:32` literal: `{ "AvatarFParamUBO_Legacy", 0x30bd28b9u, 256u, 3u, 54u, 0u, 1u, 4u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: avatar fragment shader program bind 単位で update
- **source**: ubo_metadata.inl:32 + llglslshader.cpp:95 + llglslshader.cpp:2147-2149

---

## §4. 物理 owner

- **data source**:
  - `aya_sss_skin_flag` (float): AYAstorm r20 SSS skin flag (= memory `project_skin_hash_collision_bom_body` / `project_aya_visual_realism_alpha_protect` 関連、推定 = LLDrawInfo 単位の SSS opt-in flag、verify 要)
  - `_pad_*` (float × 3): std140 align padding (= 16 B aligned 1 vec4 整形)
- **uniform 名 reserved**: `llshadermgr.cpp:1611` literal: `mReservedUniforms.push_back("aya_sss_skin_flag");  // <FS:AYA r20 Phase C>`
- **uniform 名 ↔ enum**: `llshadermgr.h:138` literal: `AYA_SSS_SKIN_FLAG, //  "aya_sss_skin_flag" <FS:AYA r20 Phase C>`
- **既存 OpenGL 経路 writer site**: **不明 / verify 要** (= AYAstorm r20 SSS 章実装、grep `AYA_SSS_SKIN_FLAG` で writer 特定要、推定 `lldrawpoolavatar.cpp` or `pipeline.cpp` SSS 経路)
- **lifetime**: avatar draw 毎 (= per-draw だが PerProgram cadence で program 単位 update)
- **ambiguity**: SSS skin flag は LLDrawInfo 単位の per-draw 性質 = PerProgram cadence と本来不一致 (= verify 要、PerDraw に降格すべき可能性)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/avatar_f_param_ubo_legacy.glsl`
- **実 shader use site** (= 1 file confirmed):
  - `indra/newview/app_settings/shaders/class1/deferred/avatarF.glsl:76` (= blueprint source literal、`#ifdef LL_VULKAN_GLSL` block)
- **既存 OpenGL 経路**: 同 file 内 `#else` block で uniform 個別宣言

---

## §6. 既存 setter call site (host C++)

- **uniform 名 reserved**: `llshadermgr.cpp:1611` literal
- **uniform 名 ↔ enum**: `llshadermgr.h:138` literal
- **writer call site**: **不明 / verify 要** (= grep `AYA_SSS_SKIN_FLAG` で writer 特定要、推定 AYA r20 SSS shader bind 経路)
- **本 UBO 名指 setter**: なし

---

## §7. 現状通電状態

- **状態**: **untouched** (= shell 通電もされていない)
- **PC-7γ-1 PerProgram register**: 条件付き = mUseUBO=true 時 avatarF.glsl link 時に走る
- **PC-7γ-1 PerProgram write**: 条件付き = mUseUBO=true で setter が走ると writeProgramUbo に流れる (default OFF)

---

## §8. 本実装化に必要な作業

1. **mUseUBO ON 化**
2. **shader 側 LL_VULKAN_GLSL block 活性化** (= avatarF.glsl:76、起案済確認要)
3. **writer call site 特定** = grep `AYA_SSS_SKIN_FLAG` で writer 特定要 (= r20 SSS 経路)
4. **cadence 妥当性 review** (= per-draw 性質を PerProgram に乗せる設計、PerDraw cadence への降格検討)
5. **setter 経路 verify**
6. **codegen 再実行不要** (= layout / member 不変)

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set=3 binding=54) |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded (3 padding member 明示) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ⚠️ avatarF.glsl に LL_VULKAN_GLSL block 追加済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**cadence mismatch risk**:
- SSS skin flag は LLDrawInfo (= per-draw) 単位の attribute
- PerProgram cadence で write すると 1 program 内 multi-draw で stale data risk (= verify 要)
- 解決案: PerDraw cadence (cadence_tag=2) UBO へ移管 + dynamic offset 経路

**1 actual member only**:
- 16 B 実 data + 240 B padding = overhead 大、PerDraw 化検討時に statistics / atomics 他 SSS 関連 member と統合可能

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守)

1. **writer call site 特定** = `AYA_SSS_SKIN_FLAG` uniform setter (= AYAstorm r20 SSS chapter 実装)
2. **aya_sss_skin_flag semantic** = bool flag (0.0/1.0) か LLDrawInfo per-prim opt-in か (推定 = per-prim、memory `project_skin_hash_collision_bom_body` の M4.17 `mFSPickerLocalID` 経路に類似)
3. **cadence 妥当性** = PerProgram で per-draw 性質を運ぶ妥当性 (= verify 要)
4. **SkinSSSPrototypeFParamUBO_Legacy との関係** = SSS 関連 UBO は別途存在、本 UBO との役割分担不明

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

avatar 系 Legacy 帯:
- **AvatarFParamUBO_Legacy (binding=54、本 UBO)**
- AvatarClothVParamUBO_Legacy (binding=57)
- AvatarAlphaShadowVParamUBO_Legacy (binding=22)
- VelocityVParamUBO_Legacy (binding=55)
- RlvFParamUBO_Legacy (binding=56)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 同 avatar 系 PerProgram cluster (= AvatarClothVParamUBO_Legacy / AvatarAlphaShadowVParamUBO_Legacy)

### §11.3 同 shader consume UBO

- avatarF.glsl 内同時 consume: 不明 / verify 要 (= 推定 FrameViewProj / FrameLights / FrameAtmosphere_Lighting + Material UBO)

### §11.4 同 data source UBO

- **SkinSSSPrototypeFParamUBO_Legacy** (binding=30) = SSS 関連 = AYA r20 SSS 章実装、本 UBO と data source 共有可能性 (= verify 要)

### §11.5 dirty 連動 UBO

- 推定: avatar SSS preset 変更時 SkinSSSPrototypeFParamUBO_Legacy と連動 dirty (= verify 要)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- set=3 帯 bind は `bindV3aStatic` / `bindV3aRigged` で全帯一括
- avatar program bind 時 PerProgram cadence triple-buffer flush
