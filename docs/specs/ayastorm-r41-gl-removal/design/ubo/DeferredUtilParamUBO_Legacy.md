# DeferredUtilParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= shell 通電なし、host C++ writer は部分特定済 = `pipeline.cpp:12180-12184, 12255-12257` 他 + `waterSign` 別経路 writer、shader 側 LL_VULKAN_GLSL block 宣言済)

**本実装化に必要な作業**: per-program cadence register / write 配線追加 + 既存 OpenGL 経路 setter (= projector params 8 件 + waterSign) の mUseUBO 分岐経路から `forwardToUboUpload` → `writeProgramUbo` 経由で本 UBO に書込み開始 + `deferredUtil.glsl` の LL_VULKAN_GLSL block 活性化

---

## §1. UBO identity

- **block_name**: `DeferredUtilParamUBO_Legacy`
- **block_hash**: `0x2bcf5361u`
- **block_size**: 256 B (= std140 48 B、device-padded 256 B)
- **member_count**: 8
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_deferredutilparamubo_legacy.inl:12-21
struct DeferredUtilParamUBO_LegacyLayout {
    static constexpr std::uint32_t proj_n_OFFSET = 0u;                            // size=12 align=16
    static constexpr std::uint32_t proj_focus_OFFSET = 12u;                       // size=4 align=4
    static constexpr std::uint32_t proj_p_OFFSET = 16u;                           // size=12 align=16
    static constexpr std::uint32_t proj_lod_OFFSET = 28u;                         // size=4 align=4
    static constexpr std::uint32_t proj_range_OFFSET = 32u;                       // size=4 align=4
    static constexpr std::uint32_t proj_ambiance_OFFSET = 36u;                    // size=4 align=4
    static constexpr std::uint32_t waterSign_OFFSET = 40u;                        // size=4 align=4
    static constexpr std::uint32_t _pad_deferred_util_legacy_0_OFFSET = 44u;      // size=4 align=4
};
inline constexpr std::uint32_t DeferredUtilParamUBO_Legacy_SIZE = 256u; // std140=48, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/deferred_util_param_ubo_legacy.glsl:9-19
layout(std140, set = 3, binding = 6) uniform DeferredUtilParamUBO_Legacy
{
    vec3  proj_n;          // projector normal
    float proj_focus;      // distance from plane to begin blurring
    vec3  proj_p;          // plane projection is emitting from (in screen space)
    float proj_lod;        // (number of mips in proj map)
    float proj_range;      // range between near clip and far clip plane of projection
    float proj_ambiance;
    float waterSign;
    float _pad_deferred_util_legacy_0;
};
```

= **projector parameters (proj_n / proj_focus / proj_p / proj_lod / proj_range / proj_ambiance) + waterSign**、deferred lighting util 共通参照

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 6
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:37` literal: `{ "DeferredUtilParamUBO_Legacy", 0x2bcf5361u, 256u, 3u, 6u, 0u, 1u, 8u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: deferred lighting util consume shader program bind 単位で update (= deferredUtil.glsl は global scope helper、複数 deferred lighting program で link)
- **source**: ubo_metadata.inl:37 + llglslshader.cpp:95

---

## §4. 物理 owner

- **data source**:
  - `proj_n` (vec3): projector normal vector (= `llshadermgr.cpp:1556` literal: `mReservedUniforms.push_back("proj_n")`、`llshadermgr.h:87`: `PROJECTOR_N`)
  - `proj_focus` (float): focus distance
  - `proj_p` (vec3): projection plane origin (screen space)
  - `proj_lod` (float): proj map mips count
  - `proj_range` (float): proj near→far range
  - `proj_ambiance` (float): proj ambient strength
  - `waterSign` (float): water clip plane sign (above/below water)
- **既存 OpenGL 経路 writer site** (= **部分特定済**):
  - **proj_p**: `pipeline.cpp:12181` literal: `shader.uniform3fv(LLShaderMgr::PROJECTOR_P, 1, glm::value_ptr(p1))`
  - **proj_n**: `pipeline.cpp:12182` literal: `shader.uniform3fv(LLShaderMgr::PROJECTOR_N, 1, glm::value_ptr(n))`
  - **proj_range**: `pipeline.cpp:12184` literal: `shader.uniform1f(LLShaderMgr::PROJECTOR_RANGE, proj_range)`
  - **proj_focus**: `pipeline.cpp:12255` literal: `shader.uniform1f(LLShaderMgr::PROJECTOR_FOCUS, focus)`
  - **proj_ambiance / proj_lod**: `pipeline.cpp:12257` literal: `shader.uniform1f(LLShaderMgr::PROJECTOR_AMBIENT_LOD, llclamp(...))` (= 統合 uniform 名 PROJECTOR_AMBIENT_LOD、proj_lod / proj_ambiance のどちらか、verify 要)
  - **waterSign**:
    - `lldrawpoolwaterexclusion.cpp:73-74` literal: `static LLStaticHashedString waterSign("waterSign"); gDrawColorProgram.uniform1f(waterSign, 1.f);`
    - `lldrawpoolalpha.cpp:101, 118, 123` literal: 同
- **lifetime**: projector light pass / alpha pool draw / water exclusion pool draw 毎

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/deferred_util_param_ubo_legacy.glsl`
- **実 shader use site** (= 2 file confirmed):
  - `indra/newview/app_settings/shaders/class1/deferred/deferredUtil.glsl:99` (= blueprint source literal、起源、global scope helper)
  - `indra/newview/app_settings/shaders/class3/deferred/spotLightF.glsl:70` literal (= 参照、deferredUtil.glsl 経由 global scope 取得を明示、`PerProgramUBO_SpotLightF` 側で proj_n/proj_focus/proj_p/proj_lod/proj_range/proj_ambiance を **重複 declare 禁止** = `spotLightF.glsl:67-71` literal)
- **既存 OpenGL 経路**: 同 file 内 `#else` block で uniform 個別宣言

---

## §6. 既存 setter call site (host C++)

- **pipeline.cpp 12180-12184** (= projector p1 / n / proj_range setter):
  ```cpp
  shader.uniform1f(LLShaderMgr::PROJECTOR_NEAR, near_clip);  // PROJECTOR_NEAR は別 UBO の可能性、verify 要
  shader.uniform3fv(LLShaderMgr::PROJECTOR_P, 1, glm::value_ptr(p1));
  shader.uniform3fv(LLShaderMgr::PROJECTOR_N, 1, glm::value_ptr(n));
  shader.uniform1f(LLShaderMgr::PROJECTOR_RANGE, proj_range);
  ```
- **pipeline.cpp 12255-12257** (= projector focus / ambient lod setter):
  ```cpp
  shader.uniform1f(LLShaderMgr::PROJECTOR_FOCUS, focus);
  shader.uniform1f(LLShaderMgr::PROJECTOR_AMBIENT_LOD, llclamp(...));
  ```
- **lldrawpoolwaterexclusion.cpp:73-74 + lldrawpoolalpha.cpp:101, 118, 123** (= waterSign setter):
  ```cpp
  static LLStaticHashedString waterSign("waterSign");
  gDrawColorProgram.uniform1f(waterSign, 1.f);
  // OR
  shader->uniform1f(waterSign, water_sign);
  ```
- **本 UBO 名指 setter**: なし

---

## §7. 現状通電状態

- **状態**: **untouched** (= shell 通電もされていない)
- **PC-7γ-1 PerProgram register**: 条件付き = mUseUBO=true 時 deferredUtil.glsl link program (複数 deferred lighting program) で走る
- **PC-7γ-1 PerProgram write**: 条件付き = mUseUBO=true で setter が走ると writeProgramUbo に流れる

---

## §8. 本実装化に必要な作業

1. **mUseUBO ON 化**
2. **shader 側 LL_VULKAN_GLSL block 活性化** (= deferredUtil.glsl:99、起案済確認要)
3. **setter 経路 verify** (= pipeline.cpp 11 系 + waterSign 3 系の writer が forwardToUboUpload → writeProgramUbo へ dispatch)
4. **PROJECTOR_NEAR の所属 UBO 確認** (= 本 UBO の `proj_near` member 不在、別 UBO 担当の可能性、verify 要)
5. **PROJECTOR_AMBIENT_LOD enum 値 ↔ proj_ambiance / proj_lod 対応関係** verify
6. **spotLightF.glsl 重複宣言禁止整合**: `PerProgramUBO_SpotLightF` 側で proj_n/proj_focus/proj_p/proj_lod/proj_range/proj_ambiance を declare せず本 UBO 経由参照 (= spotLightF.glsl:67-71 literal)
7. **codegen 再実行不要**

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set=3 binding=6) |
| OS-3 | std140 padding 厳守 | ✅ 48B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ⚠️ deferredUtil.glsl に LL_VULKAN_GLSL block 追加済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**deferredUtil.glsl 共有 helper の cadence 問題**:
- deferredUtil.glsl は global scope helper、複数 deferred lighting program で link
- 各 program bind 毎に PerProgram cadence で本 UBO write → triple-buffer flush
- writer side は projector light pass のみで write (= pipeline.cpp 12180+)、他 program では同値 stale data 共有

**waterSign の per-program 問題**:
- waterSign は draw 単位で異なる (= above/below water、alpha pool 別 sign)
- PerProgram cadence で運ぶと 1 program 内 multi-draw で stale data risk
- PerDraw cadence (cadence_tag=2) UBO への分離検討 (= 本 phase では既存構造維持)

**spotLightF.glsl 重複宣言禁止整合**:
- `spotLightF.glsl:67-71` literal で `PerProgramUBO_SpotLightF` 側 declare 内容を制限 (= deferredUtil.glsl 経由 global scope 取得済 member を排除)
- 設計上 PerProgramUBO_SpotLightF と本 UBO は member 重複しない契約

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守)

1. **PROJECTOR_NEAR の所属 UBO** (= `pipeline.cpp:12180` literal で `LLShaderMgr::PROJECTOR_NEAR` set 済だが本 UBO に proj_near member 不在)
2. **PROJECTOR_AMBIENT_LOD enum の write 先 member** = proj_ambiance / proj_lod のどちらか (= `pipeline.cpp:12257` literal: `llclamp((proj_range-focus)/proj_range*lod_range, 0.f, 1.f)` は lod 値計算ゆえ proj_lod 推定)
3. **waterSign の PerProgram vs PerDraw 適合性** = 1 program 内 multi-draw 時の stale data 影響範囲
4. **deferredUtil.glsl link 先 program 全列挙** (= 複数 deferred lighting program、PerProgram register が各 program で走る確認要)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- **DeferredUtilParamUBO_Legacy (binding=6、本 UBO)**
- SoftenLightParamUBO_Legacy (binding=5)
- ShadowUtilParamUBO_Legacy (binding=7)
- AOUtilParamUBO_Legacy (binding=8)
- WaterFogUBO_Legacy (binding=9)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 同 deferred util 系 cluster (= SoftenLightParamUBO_Legacy / ShadowUtilParamUBO_Legacy / AOUtilParamUBO_Legacy)

### §11.3 同 shader consume UBO

- deferredUtil.glsl 内同時宣言: 不明 / verify 要 (= FrameViewProj 推定)
- spotLightF.glsl 内 consume:
  - **PerProgramUBO_SpotLightF** (set=2 binding=10) と組合せ、deferredUtil.glsl 経由本 UBO 共有 (= 重複宣言禁止整合)

### §11.4 同 data source UBO

- **PerProgramUBO_SpotLightF** (set=2 binding=10): 同 projector data 由来候補 (= spotLightF.glsl 整合的に重複禁止 = data source 部分共有、verify 要)

### §11.5 dirty 連動 UBO

- projector light pass 切替時の連動: PerProgramUBO_SpotLightF / PerProgramUBO_PointLightF 等候補 (= verify 要)
- waterSign は alpha pool / water exclusion pool draw 毎 update

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- set=3 帯 bind は `bindV3aStatic` 経路で全帯一括
- deferred lighting program 毎に PerProgram cadence triple-buffer flush
- deferredUtil.glsl が複数 program で link されるゆえ本 UBO の register は各 program で走る (= seen_program_hashes 重複排除で 1 block 1 UBO instance ゆえ問題なし、`llglslshader.cpp:2079-2099` literal 整合)

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.3.4 同期)

**Layer**: L2-4 (= B Tier α setter 部分特定済、PerProgram cadence、projector + waterSign 複雑)
**status**: **起案済** (= 2026-06-06 C-4、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.3.4` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch + L0-4 cadence + L2-3 完了 (= setter 行特定 D1 pattern 確立)
- **(2) 不明事項**: `PROJECTOR_NEAR` 所属 UBO (= 本 UBO に proj_near member 不在) **[要 AYA 判断 / 設計再考]** / `PROJECTOR_AMBIENT_LOD` write 先 = proj_ambiance / proj_lod (推定 proj_lod) [要 verify] / `waterSign` PerProgram vs PerDraw 適合 **[要 AYA 判断]** / deferredUtil.glsl link 先 program 全列挙 [要追加調査]
- **(3) 調査手法**: D1 (11 setter site 詳細読解) + D2 (deferredUtil.glsl link 先 program 全列挙) + D3 (waterSign per-draw 変動 vs PerProgram cadence) + D4 (`PerProgramUBO_SpotLightF` 重複宣言禁止整合)
- **(4) 設計 task**: PerProgram cadence triple-buffer (= 各 deferred lighting program で register、seen_program_hashes 重複排除) / 11 setter call を `forwardToUboUpload` → `writeProgramUbo` (= projector 5 + waterSign 多 site) / 各 deferred lighting program bind 単位 flush / `deferredUtil.glsl:99` 既存 LL_VULKAN_GLSL block 活性化 + `PerProgramUBO_SpotLightF` 重複宣言禁止整合
- **(5) 工程**: trace 順 L2 4 件目 (= L2 内最複雑、L2 締め)、工数 **M** (= 半日 +)、L2-1 / L2-2 / L2-3 並列可
- **(6) A 確定**: mUseUBO ON + shader 活性化 + 11 setter 通電 + AYA live verify (= projector light + water exclusion + alpha pool 描画既存と同一、**visual regression ゼロ §5.4**) + Vulkan validation 0 + waterSign cadence 妥当性確定 (AYA 判断) + PROJECTOR_NEAR 所属 UBO 明確化 + SpotLightF 重複宣言禁止整合 verify
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `deferredUtil.glsl #else` block 維持、副作用 risk = waterSign stale で above/below water 切替 artifact

**関連**: L0-1 + L0-4 (= WORK_ORDER §2) / §5.4 visual regression policy / `PerProgramUBO_SpotLightF` (= L4 §3.2 cross-stage 重複禁止) / L4 §3.6 water 系 (= waterSign 連動)
