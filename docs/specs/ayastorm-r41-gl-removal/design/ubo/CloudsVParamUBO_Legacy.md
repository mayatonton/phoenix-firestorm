# CloudsVParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= shell 通電なし、host C++ writer / register 配線無し、shader 側 LL_VULKAN_GLSL block でのみ宣言済、cloudsV + cloudsF 両 stage で参照)

**本実装化に必要な作業**: per-program cadence register / write 配線追加 + 既存 OpenGL 経路 setter (= camPosLocal + cloud_scale + cloud_color uniform setter site) の mUseUBO 分岐経路から `forwardToUboUpload` → `writeProgramUbo` 経由で本 UBO に書込み開始 + `cloudsV.glsl` の LL_VULKAN_GLSL block 活性化 + cloudsF.glsl 内複製 block の整合性 verify

---

## §1. UBO identity

- **block_name**: `CloudsVParamUBO_Legacy`
- **block_hash**: `0x7d4955feu`
- **block_size**: 256 B (= std140 32 B、device-padded 256 B)
- **member_count**: 4
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_cloudsvparamubo_legacy.inl:12-17
struct CloudsVParamUBO_LegacyLayout {
    static constexpr std::uint32_t camPosLocal_OFFSET = 0u;                  // size=12 align=16
    static constexpr std::uint32_t cloud_scale_OFFSET = 12u;                 // size=4 align=4
    static constexpr std::uint32_t cloud_color_OFFSET = 16u;                 // size=12 align=16
    static constexpr std::uint32_t _pad_clouds_v_legacy_0_OFFSET = 28u;      // size=4 align=4
};
inline constexpr std::uint32_t CloudsVParamUBO_Legacy_SIZE = 256u; // std140=32, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/clouds_v_param_ubo_legacy.glsl:10-16
layout(std140, set = 3, binding = 3) uniform CloudsVParamUBO_Legacy
{
    vec3  camPosLocal;
    float cloud_scale;
    vec3  cloud_color;
    float _pad_clouds_v_legacy_0;
};
```

= **camera local position + cloud scale + cloud color** (= cloud vertex shader 参照、cloudsF でも cloud_scale 参照ゆえ複製併存)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 3
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:36` literal: `{ "CloudsVParamUBO_Legacy", 0x7d4955feu, 256u, 3u, 3u, 0u, 1u, 4u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: cloud vertex shader program bind 単位で update
- **source**: ubo_metadata.inl:36 + llglslshader.cpp:95

---

## §4. 物理 owner

- **data source**:
  - `camPosLocal` (vec3): camera local position (= 推定 = `LLViewerCamera::getOrigin()` 由来 local space、verify 要、per-frame 性質ゆえ PerProgram cadence でも安全)
  - `cloud_scale` (float): WL cloud scale
  - `cloud_color` (vec3): WL cloud color
- **既存 OpenGL 経路 writer site**: **不明 / verify 要** (= grep `cloud_scale` / `cloud_color` / `camPosLocal` で writer 特定要、推定 llsettingsvo.cpp / pipeline.cpp 経路)
- **lifetime**: sky preset 切替時 (cloud_scale / cloud_color) + camera move 時 (camPosLocal、frame 単位)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/clouds_v_param_ubo_legacy.glsl`
- **実 shader use site** (= 2 file confirmed):
  - `indra/newview/app_settings/shaders/class1/deferred/cloudsV.glsl:106` (= blueprint source literal、起源)
  - `indra/newview/app_settings/shaders/class1/deferred/cloudsF.glsl:79` (= η-14 path G-β 複製、fragment 側 cloud_scale 参照解決)
- **blueprint コメント**: 「Multi-site: verified identical at class1/deferred/cloudsF.glsl:79」(= blueprint:4 literal)
- **既存 OpenGL 経路**: 同 file 内 `#else` block で uniform 個別宣言

---

## §6. 既存 setter call site (host C++)

- **writer call site**: **不明 / verify 要** (= grep `cloud_scale` / `cloud_color` / `camPosLocal` で writer 特定要、推定 llsettingsvo.cpp 経路)
- **本 UBO 名指 setter**: なし

---

## §7. 現状通電状態

- **状態**: **untouched** (= shell 通電もされていない)
- **PC-7γ-1 PerProgram register**: 条件付き
- **PC-7γ-1 PerProgram write**: 条件付き

---

## §8. 本実装化に必要な作業

1. **mUseUBO ON 化**
2. **shader 側 LL_VULKAN_GLSL block 活性化** (= cloudsV.glsl:106 + cloudsF.glsl:79 複製、起案済確認要)
3. **writer call site 特定**:
   - `cloud_scale` / `cloud_color` writer (= 推定 llsettingsvo.cpp)
   - `camPosLocal` writer (= 推定 pipeline.cpp per-frame)
4. **camPosLocal cadence mismatch 確認**: per-frame 性質を PerProgram cadence で運ぶ、shader bind 毎 = per-frame で実質一致だが verify 要
5. **fragment 側複製 (cloudsF.glsl:79) 整合**: 両 stage 同 binding 参照の Vulkan 仕様整合確認
6. **codegen 再実行不要**

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set=3 binding=3) |
| OS-3 | std140 padding 厳守 | ✅ 32B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ⚠️ cloudsV.glsl + cloudsF.glsl に LL_VULKAN_GLSL block 追加済 (fragment 側は複製) |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**Multi-stage 複製 pattern**:
- cloudsV.glsl が起源 + cloudsF.glsl が同 binding=3 を複製 (= η-14 path G-β)
- Vulkan 仕様: 1 pipeline 内で同 set=N binding=M を vertex/fragment 両 stage で参照可能 (= descriptor set bind は pipeline 単位、stage visibility は pipeline layout で制御)
- 但し layout (member 構成) が両 stage で完全一致必須 = blueprint コメントで verify 済「verified identical」

**cadence mismatch (camPosLocal)**:
- camPosLocal は per-frame data、PerProgram cadence で運ぶ
- cloud program bind = per-frame で実質一致だが、camera 移動 + program 未 bind の状況で stale data 可能性 = verify 要

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守)

1. **camPosLocal writer 特定** = uniform setter site (推定 pipeline.cpp)
2. **cloud_scale / cloud_color writer 特定** = llsettingsvo.cpp 経路 (推定)
3. **camPosLocal 座標系** = local space の origin (= region center?) verify 要
4. **fragment 側複製の Vulkan validation warning 有無**

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- **CloudsVParamUBO_Legacy (binding=3、本 UBO)**
- **CloudsFParamUBO_Legacy (binding=4、密関連、fragment 側複製受け側)**
- SkyVParamUBO_Legacy (binding=1)
- SkyFParamUBO_Legacy (binding=2)
- AtmoExtraUBO_Legacy (binding=0)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 同 windlight 系 cluster

### §11.3 同 shader consume UBO

- cloudsV.glsl 内同時 consume:
  - **AtmoExtraUBO_Legacy** (= 推定、verify 要、windlight 系)
  - **FrameViewProj** (= 推定、vertex shader projection)
- cloudsF.glsl 内同時 consume:
  - **CloudsFParamUBO_Legacy** (= 同 file 内)
  - **本 UBO (複製併存)**
  - **AtmoExtraUBO_Legacy** (= 推定)

### §11.4 同 data source UBO (= LLSettingsSky 由来)

- **CloudsFParamUBO_Legacy** (= 同 sky preset cloud_pos_density1/2 / blend_factor / cloud_variance)
- **SkyVParamUBO_Legacy / SkyFParamUBO_Legacy** (= 同 sky preset 由来候補、verify 要)
- **AtmoExtraUBO_Legacy** (= 同 sky preset 由来)

### §11.5 dirty 連動 UBO

- sky preset 切替時に連動 dirty: CloudsFParamUBO_Legacy / SkyFParamUBO_Legacy / SkyVParamUBO_Legacy / AtmoExtraUBO_Legacy
- camera 移動時 camPosLocal のみ dirty (= per-frame)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- set=3 帯 bind は `bindV3aStatic` 経路で全帯一括
- vertex/fragment 両 stage で同 set=3 binding=3 参照 (= 複製併存パターン、Multi-stage 複製の Vulkan 仕様整合確認要)

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.7 同期)

**Layer**: L4-7 sub-cluster (d) (= Clouds V/F pair 2 UBO + cloudsF.glsl:79 複製併存、camPosLocal/cloud_scale 共有)
**status**: **起案済** (= 2026-06-06 C-6-b、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.7` (= single source of truth)
**要点**: 4 member (camPosLocal vec3 + cloud_scale float + cloud_color vec3 + pad)、起源 = cloudsV.glsl:106、η-14 path G-β 複製 cloudsF.glsl:79 で fragment 側参照 (= CLOUDS_V_PARAM_UBO_LEGACY_DEFINED guard、Vulkan 仕様 1 pipeline 内同 set/binding 両 stage 参照可能、blueprint コメント `verified identical` 担保)、camPosLocal cross UBO 5 use site (= 本 UBO + SkyV + CloudsF 複製 + MaterialUBO_Legacy + materialF.glsl)、cadence mismatch (= per-frame camPosLocal を PerProgram で運ぶ stale risk) → PerFrame 降格候補 [要 L0-4 結果反映]、cloud_scale/cloud_color/camPosLocal writer 不明 [要追加調査]、工数 group 全体 L 内
**関連**: L0-1 dispatch (= 衝突なし binding=3) / L0-4 cadence (= PerFrame 降格候補 camPosLocal) / §3.5.7 sub-cluster (d) CloudsF (= V/F pair 複製受け側) / sub-cluster (c) SkyV (= camPosLocal cross UBO 同 source) / §3.5.5 group MaterialUBO_Legacy (= camPosLocal cross UBO 5 use site の 1 つ)


---

## §13. Phase 2.α 案 X 確定 record (= blueprint dir 位置付け + 二重 source 同期 protocol)

### §13.1 blueprint dir の位置付け = codegen 入力 source of truth

- **blueprint file** (= `aya_r41_blueprints/<set>/<ubo_lower>.glsl`) は本 UBO の **codegen 入力 source of truth** (= 案 X 確定 2026-06-06)。`indra/cmake/AyaUboCodegen.cmake` の `AYA_UBO_CODEGEN_BLUEPRINT_DIR` 経由で `scripts/ubo_codegen/main.py` の入力に渡され、`ubo_metadata.inl` + `ubo_layout_<ubo>.inl` を生成する。
- **AYAstorm shader runtime compile target は別 GLSL 系統** (= `class*/` + `cinematic_bd/` 配下の実 shader use site) で並列 build process (= design/04-codegen-ubo.md §2.2 literal「別 GLSL 並列 build process」)。
- 二系統は二重 source として共存し、**`scripts/ubo_codegen/main.py` の二重 source 同期 protocol で整合 verify** される (= §13.2)。
- 案 X 確定 source of truth = `docs/specs/ayastorm-r41-gl-removal/handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D.9
- blueprint dir 内 README = `indra/newview/app_settings/shaders/aya_r41_blueprints/README.md` (= phase B commit `f95182ded5`、位置付け literal source)

### §13.2 二重 source 同期 protocol (= main.py で formal化)

- **`_verify_block_match`** (= α-2 commit `b66ec99f72`) = 同名 UBO 複数 file (= blueprint + actual の cross-source pair、または cinematic_bd 上書き path) の set/binding + subset/cadence + member 全件 layout 一致を構造的 verify。不一致時 `CodegenError` で abort。
- **`_verify_blueprint_actual_consistency`** + **`--verify-target-paths`** option (= phase F commit `868bc38cc9`) = blueprint と actual の二重 source 整合 verify を formal化、`--verify-target-paths` で blueprint と actual を区別して対称的 cross-verify。
- 本 UBO の場合 = blueprint file (= §5 / §1 で記載) と実 shader use site (= §5 で記載) が **両 path で同一 layout (set/binding/member)** を保持する protocol。改修時は両方を同期書換するか、blueprint 側のみ書換後 codegen 再生成 + actual の `#ifdef LL_VULKAN_GLSL` block を手動同期する。
- sub-session 5 step 2-batch-0-a 7 UBO の同期書換 record = phase E commit `09ee5e8a8e` (= actual class*/ + cinematic_bd/ 14 file の新 set/binding を blueprint dir 内 7 UBO 7 file に同期反映、案 X 確定後の整合修復)

### §13.3 cross-ref

- 設計 doc = `design/04-codegen-ubo.md` §2.2 (= 別 GLSL 並列 build process) / §4.4 (= 同名 UBO 複数 GLSL 宣言の整合 verify)
- handoff doc = `handoff/phase2/alpha/handoff-phase2-alpha-codegen-single-source-of-truth-entry.md` §D.9 (= 案 X 確定 source of truth、6 commit revert record + 改修方針 9 件)
- blueprint dir README = `indra/newview/app_settings/shaders/aya_r41_blueprints/README.md` (= phase B commit `f95182ded5`)
- 二重 source 同期 protocol formal化 = `scripts/ubo_codegen/main.py` `_verify_block_match` + `_verify_blueprint_actual_consistency`

