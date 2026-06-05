# SkinSSSPrototypeFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `skin_sss_prototype_f_param_ubo_legacy.glsl` は実 shader `class1/deferred/skinSSSF.glsl ifdef LL_VULKAN_GLSL block` から literal extract 済 (= η-6 phase 2-A Cluster E skinSSSF non-opaque uniforms UBO wrap)。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加。**AYAstorm r20 Skin SSS (Subsurface Scattering) prototype 機能関連 UBO** = r14+ 視覚表現章 + r20 BoM body SSS prototype の中核 UBO。

---

## §1. UBO identity

- **block_name**: `SkinSSSPrototypeFParamUBO_Legacy`
- **block_hash**: `0x3aec494au` (= FNV-1a("SkinSSSPrototypeFParamUBO_Legacy"))
- **block_size**: 256 B (= std140 64 B、device-padded 256 B)
- **member_count**: 7 (= metadata literal、blueprint は 8 entries で `aya_glow_color` vec3 が 1 entry とカウント = 整合)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_skinsssprototypefparamubo_legacy.inl
struct SkinSSSPrototypeFParamUBO_LegacyLayout {
    static constexpr std::uint32_t aya_blur_dir_OFFSET = 0u;  // size=8 align=8
    static constexpr std::uint32_t aya_strength_OFFSET = 8u;  // size=4 align=4
    static constexpr std::uint32_t aya_blur_radius_OFFSET = 12u;  // size=4 align=4
    static constexpr std::uint32_t aya_glow_gain_OFFSET = 16u;  // size=4 align=4
    static constexpr std::uint32_t aya_glow_color_OFFSET = 32u;  // size=12 align=16
    static constexpr std::uint32_t aya_visual_realism_enabled_skinsss_legacy_OFFSET = 44u;  // size=4 align=4
    static constexpr std::uint32_t aya_r20_skin_sss_enabled_OFFSET = 48u;  // size=4 align=4
};
inline constexpr std::uint32_t SkinSSSPrototypeFParamUBO_Legacy_SIZE = 256u; // std140=64, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/skin_sss_prototype_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 30) uniform SkinSSSPrototypeFParamUBO_Legacy
{
    vec2  aya_blur_dir;
    float aya_strength;
    float aya_blur_radius;
    float aya_glow_gain;
    vec3  aya_glow_color;
    int   aya_visual_realism_enabled_skinsss_legacy;
    int   aya_r20_skin_sss_enabled;
};
```

= 実 data 7 member (= aya_blur_dir + aya_strength + aya_blur_radius + aya_glow_gain + aya_glow_color + 2 enable flag)。**SSS blur direction + strength + radius + glow + 2 visibility enable** (= multi-pass SSS blur control)。

**注記**: blueprint comment literal `aya_visual_realism_enabled` は AtmoExtraUBO_Legacy nameless block でも参照される (= cross-UBO 同名 member の値整合要、η-6 phase 2-A 範式)。

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 30
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、verify 要)
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:104` `{ "SkinSSSPrototypeFParamUBO_Legacy", 0x3aec494au, 256u, 3u, 30u, 0u, 1u, 7u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush、SSS blur direction は multi-pass (horizontal / vertical) で値変化 = PerProgram cadence で multi pass 中 dirty 連続発火 pattern
- **source**: ubo_metadata.inl:104 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - AYAstorm r20 Skin SSS pipeline (= `LLPipeline` 内 SSS render 経路、grep verify 要)
  - r20 SSS cvar 群 (= AYARenderSkinSSS_* 等、verify 要)
  - r14+ visual realism enable cvar (= AYAVisualRealismEnabled、memory `project_aya_visual_realism_alpha_protect` 関連)
  - aya_blur_dir = SSS multi-pass direction (= horizontal pass = vec2(1, 0)、vertical pass = vec2(0, 1))
- **lifetime**: program 単位 (= SSS post shader bind 中、multi-pass で値変化)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/skin_sss_prototype_f_param_ubo_legacy.glsl`
- **実 shader use site**:
  - **`class1/deferred/skinSSSF.glsl:80 ifdef LL_VULKAN_GLSL block`** (= blueprint comment literal、η-6 phase 2-A Cluster E)
- **使用 uniform**: aya_blur_dir (vec2) / aya_strength (float) / aya_blur_radius (float) / aya_glow_gain (float) / aya_glow_color (vec3) / aya_visual_realism_enabled_skinsss_legacy (int) / aya_r20_skin_sss_enabled (int)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **本実装化後 setter** (= **不明 / verify 要**):
  - LLPipeline SSS pass setter (= `uniform2fv(aya_blur_dir)` + `uniform1f(aya_strength/blur_radius/glow_gain)` + `uniform3fv(aya_glow_color)` + `uniform1i(enable flag × 2)`、grep verify 要)
  - multi-pass で 2 program bind 毎に aya_blur_dir update (= verify 要)

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 範囲、η-6 phase 2-A、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電** + dummy buffer write + bind 経路通電
2. **実 member data 流入**: r20 SSS cvar + r14 visual realism enable cvar 由来の値を UBO write
3. **dirty 判定 logic 追加**: PerProgram cadence + multi-pass で 2 回連続 dirty (= horizontal → vertical)
4. **flush logic 追加**: PerProgram cadence flush
5. **shader 接続**: 実 shader 既存 UBO declaration 使用 = 追加 shader 改変なし
6. **AYAstorm 機能維持確認**: r20 Skin SSS prototype + r14+ visual realism enable 機能維持必須 (= memory `project_aya_visual_realism_alpha_protect` 関連、frag_color.a 破壊禁止)

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=3 binding=30 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 + vec3 + int packing | ✅ 64B → 256B padded、vec3 (offset 32, size 12) + int (offset 44, size 4) で 16 B 境界完結 |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader (= η-6 phase 2-A Cluster E) |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**multi-pass dirty risk**: SSS は horizontal + vertical 2 pass 通常、aya_blur_dir 値が pass 毎変化 = PerProgram cadence で 1 frame 内 2 回連続 dirty/flush 発火、PerDraw cadence 化候補 (= 設計再検討余地)。

**cross-UBO same-named member risk**: `aya_visual_realism_enabled` は AtmoExtraUBO_Legacy nameless block でも参照される (= blueprint comment literal)、host 側で 2 UBO に同値書込必要 = dirty 連動設計要 (= AtmoExtraUBO_Legacy との同期 update、verify 要)。

**memory `project_aya_visual_realism_alpha_protect` 整合**: r14+ で additive (ONE/ONE) する shader は frag_color.a=0 必須、SSS shader 内も同遵守確認要。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)
- set=3 帯 Legacy 群

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)
- 73 件 (推定) cluster

### §11.3 同 shader consume UBO (= class1/deferred/skinSSSF.glsl)
- **不明 / verify 要** = skinSSSF.glsl 内同時 consume UBO (= FrameViewProj + diffuseRect/emissiveRect/depthMap sampler 等、verify 要)

### §11.4 同 data source UBO

- **AtmoExtraUBO_Legacy** (set=3 binding=0、ubo_metadata.inl:29) = `aya_visual_realism_enabled` 同名 member 共有 (= blueprint comment literal、cross-UBO 同値書込要)
- r14+ visual realism / r20 SSS の AYAstorm 独自機能 UBO 群との data source 共有 (= 他 AYA* prefix UBO 確認要、verify 要)

### §11.5 dirty 連動 UBO

- **AtmoExtraUBO_Legacy** (= `aya_visual_realism_enabled` 同名 member、cvar 変更時同時 dirty 必須)
- 他 AYA r14+ / r20 機能 cvar 由来 UBO 群

### §11.6 layout 共有関係
- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係
- PerProgram cadence ゆえ program bind 時に同時 bind
- SSS multi-pass (horizontal → vertical) で連続 program bind = 2 回 bind、各 bind で aya_blur_dir 値変化

---

## §10. 不明事項

1. **既存 OpenGL 経路 setter call site** = LLPipeline SSS pass setter (= grep verify 要)
2. **r20 SSS cvar 群一覧** = AYARenderSkinSSS_* / aya_strength / aya_blur_radius / aya_glow_gain / aya_glow_color 由来 cvar (= grep verify 要)
3. **r14+ visual realism enable cvar** = AYAVisualRealismEnabled or equivalent (= grep verify 要)
4. **AtmoExtraUBO_Legacy との `aya_visual_realism_enabled` 同期 update protocol** = cvar 変更時 2 UBO 同時 dirty (= 実装設計要)
5. **multi-pass aya_blur_dir 値設定 logic** = horizontal vec2(1,0) / vertical vec2(0,1) 値の host 側選択 logic (= grep verify 要)
6. **shell 通電 commit** = 未来作業
7. **cadence 設計再検討** = PerProgram vs PerDraw (= multi-pass dirty pattern 適合性、Phase 2 で再評価)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.2 同期)

**Layer**: L4-2 sub-cluster (a) (= visual_realism 2 UBO cross-write)
**status**: **起案済** (= 2026-06-06 C-6、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.2` (= single source of truth)
**要点**: aya_visual_realism 2 UBO cross-write (= AtmoExtra + SkinSSS)、本 UBO `aya_visual_realism_enabled_skinsss_legacy` offset=44 rename 版 (η-6 phase 2-A 範式)、multi-pass SSS blur 2 連続 dirty (PerProgram cadence 妥当性 [要 L0-4 結果反映 / PerDraw 降格候補])、r14+ 視覚表現章機能維持必須 (memory `project_aya_visual_realism_alpha_protect`)、aya_blur_dir 値選択 logic 未取得 [要追加調査]、工数 L (group 全体)
**関連**: L0-1 dispatch (= skinSSSF program 識別) / L0-4 cadence 再評価 (= multi-pass 2 連続 dirty + PerDraw 降格) / §3.5.1 r20 SSS (= AvatarF/PBROpaqueExtra と同 r20 章別系統) / §5.4 visual regression policy
