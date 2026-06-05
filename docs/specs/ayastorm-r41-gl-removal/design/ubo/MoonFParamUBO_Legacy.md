# MoonFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= blueprint + codegen metadata 生成済、host C++ register/write/flush 経路未着工)

**本実装化に必要な作業**: register/write/flush 経路新設 (= per-program cadence、`LLShaderMgr::MOON_BRIGHTNESS` `uniform1f` setter を UBO write に redirect) + `lldrawpoolwlsky.cpp:455` の setter call site を UBO 経由に切替

---

## §1. UBO identity

- **block_name**: `MoonFParamUBO_Legacy`
- **block_hash**: `0x6e1e8905u` (= FNV-1a("MoonFParamUBO_Legacy"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_moonfparamubo_legacy.inl:12-14
struct MoonFParamUBO_LegacyLayout {
    static constexpr std::uint32_t moon_brightness_OFFSET = 0u;  // size=4 align=4
};
inline constexpr std::uint32_t MoonFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/class1/deferred/moonF.glsl:67
layout(set=3, binding=44, std140) uniform MoonFParamUBO_Legacy {
    float moon_brightness;
};
```

= Blueprint (= `aya_r41_blueprints/set3/moon_f_param_ubo_legacy.glsl:9-12`) 一致。

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 44
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、metadata literal は cadence_tag=1 PerProgram、set=3 帯は Legacy UBO 群)
- **pipeline layout**: 不明 / verify 要 (= `sAYAStandardLayout` 5-set V3a layout 内 set=3 帯は `V3A_ASSET_SET_BINDINGS=3` literal だが、metadata Legacy UBO は binding 値 1..62 まで分布 = V3a 設計と Legacy UBO 配置の整合性不明)
- **source**: `ubo_metadata.inl:54` `{ "MoonFParamUBO_Legacy", 0x6e1e8905u, 256u, 3u, 44u, 0u, 1u, 1u }` + `class1/deferred/moonF.glsl:67` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program** (= `llglslshader.cpp:95` literal: `constexpr U32 kCadencePerProgram = 1u;`)
- **意味詳細**: moon shader (= sky 描画) は per-program で program load 時 register、moon brightness 変化時 (= settings.xml `moon_brightness` 変更 / sky environment 切替時) に write
- **source**: ubo_metadata.inl + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **owner**: `LLSettingsSky::mMoonBrightness` (= `indra/llinventory/llsettingssky.cpp:1201` `mMoonBrightness = (F32)settings[SETTING_MOON_BRIGHTNESS].asReal();`、setter は `setMoonBrightness(F32 brightness_factor)` `llsettingssky.cpp:2152`)
- **lifetime**: sky environment 単位 (= environment 切替で再 load、各 environment 内では設定変更まで不変)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/moon_f_param_ubo_legacy.glsl`
- **実 shader use site** = **`class1/deferred/moonF.glsl:67` 単独** (= blueprint header literal「Source: literal extract from class1/deferred/moonF.glsl:67」、grep 結果 1 file)
- consume 内容 = moon brightness を moon color に乗算 (= 実コード 内容 verify 要、`moonF.glsl` 全文 reference 未取得だが、`llsettingssky.cpp:1756` `mMoonDiffuse = componentMult(moonlight, light_transmittance) * moon_brightness;` から sky setting 側で先計算済の可能性あり)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: 未配線
- **既存 OpenGL 経路 setter**:
  - `indra/newview/lldrawpoolwlsky.cpp:453-455`:
    ```cpp
    F32 moon_brightness = (float)psky->getMoonBrightness();
    ...
    moon_shader->uniform1f(LLShaderMgr::MOON_BRIGHTNESS, moon_brightness);
    ```
  - `indra/newview/llsettingsvo.cpp:853` `draw_real(shader, getMoonBrightness(), LLShaderMgr::MOON_BRIGHTNESS);`
- **reserved uniform 登録**: `indra/llrender/llshadermgr.cpp:1831` `mReservedUniforms.push_back("moon_brightness");`
- **enum entry**: `indra/llrender/llshadermgr.h:347` `MOON_BRIGHTNESS, // "moon_brightness"`

---

## §7. 現状通電状態

- **状態**: **untouched**
- **codegen 生成済**: layout `inl` + metadata entry + block_hash constexpr 生成済
- **blueprint 起案済**: `aya_r41_blueprints/set3/moon_f_param_ubo_legacy.glsl`
- **shader 宣言済**: `class1/deferred/moonF.glsl:67` `#ifdef LL_VULKAN_GLSL` block
- **register/write/flush 経路**: 未配線

---

## §8. 本実装化に必要な作業

1. **register 経路**: `LLGLSLShader::mapUniforms()` 内で moon program に対し `registerProgramUbo(this, block_hash::MoonFParamUBO_Legacy, 256u)` 呼出
2. **write 経路**: `lldrawpoolwlsky.cpp:455` `uniform1f(LLShaderMgr::MOON_BRIGHTNESS, ...)` を `forwardToUboUpload` 経由 UBO write に redirect
3. **flush 経路**: cmdbuf bind 経路で sProgramUboDirty を flush
4. **shader 接続**: 本 UBO は `class1/deferred/moonF.glsl:67` 1 file 既に UBO 宣言済 = shader 追加改変なし

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set=3 binding=44) |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ shader 既に UBO 宣言済、Phase 2 で追加改変なし |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**特記 risk**:
- set=3 帯は metadata 上 binding=1..62 分布 = `V3A_ASSET_SET_BINDINGS=3` literal と矛盾 = V3a 5-set 設計と Legacy UBO 実 binding 配置の整合性 verify 要 (= set=3 帯 layout binding 数の真の上限確認要)
- 1 member 1 UBO = ring buffer 利用効率は低い (= 16B data に対し 256B 確保)、ただし設計原則 (1) 維持

---

## §10. 不明事項

1. **set=3 帯 layout の binding 上限** = V3a 設計 `V3A_ASSET_SET_BINDINGS=3` literal と metadata Legacy UBO binding=1..62 分布の整合性
2. **moon_brightness の write timing 詳細** = environment 切替時のみか毎 frame か (= `lldrawpoolwlsky.cpp:455` 呼出頻度 verify 要)
3. **moonF.glsl 全文 use site** = `moon_brightness` がどう乗算されるか実コード内容 (= 本起案では line 67 layout 宣言のみ確認、consume site 未確認)
4. **register/write/flush 経路の本実装化 trigger** = 全 Legacy UBO 一括着工か段階着工か (= Phase 2 工程議論未確定)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- set=3 帯は metadata 上 58+ Legacy UBO (= Asset_GLTFNodes/Materials + Skin_GLTFJoints + 全 Legacy *) が共存
- 直近 binding 同居: binding=43 SunDiscFParamUBO_Legacy / binding=45 StarsVParamUBO_Legacy

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 73 件 (推定) per-program cluster 内の 1 UBO
- 同 flush 経路 = sProgramUboDirty 共有

### §11.3 同 shader consume UBO (= `class1/deferred/moonF.glsl` で同時 consume)

- 不明 / verify 要 (= moonF.glsl 全文 reference 未取得、FrameViewProj / FrameAtmosphere_Lighting 等 common UBO の同 file 宣言確認要)

### §11.4 同 data source UBO

- **SunDiscFParamUBO_Legacy** (set=3 binding=43) = sky setting 由来、同 `LLSettingsSky` 派生候補 (= verify 要)
- **StarsFParamUBO_Legacy / StarsVParamUBO_Legacy** = sky setting 由来候補
- **SkyFParamUBO_Legacy / SkyVParamUBO_Legacy** = sky setting 由来候補

### §11.5 dirty 連動 UBO

- sky environment 切替時 (= `LLSettingsSky` 切替 trigger) で同時 dirty 化: SunDisc / Stars / Sky 系 (推定)、verify 要

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- moon program 切替時 set=3 帯全 binding を一括 rebind (= per-program cadence 標準)
