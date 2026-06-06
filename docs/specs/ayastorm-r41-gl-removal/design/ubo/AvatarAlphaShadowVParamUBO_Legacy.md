# AvatarAlphaShadowVParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= shell 通電なし、host C++ writer / register 配線無し、shader 側 LL_VULKAN_GLSL block でのみ宣言済)

**本実装化に必要な作業**: per-program cadence register / write 配線追加 + 既存 OpenGL 経路 setter (= shadow target width uniform setter site) の mUseUBO 分岐経路から `forwardToUboUpload` → `writeProgramUbo` 経由で本 UBO に書込み開始 + `avatarAlphaShadowV.glsl` の LL_VULKAN_GLSL block 活性化

---

## §1. UBO identity

- **block_name**: `AvatarAlphaShadowVParamUBO_Legacy`
- **block_hash**: `0x2775be31u`
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_avataralphashadowvparamubo_legacy.inl:12-14
struct AvatarAlphaShadowVParamUBO_LegacyLayout {
    static constexpr std::uint32_t shadow_target_width_OFFSET = 0u;  // size=4 align=4
};
inline constexpr std::uint32_t AvatarAlphaShadowVParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/avatar_alpha_shadow_v_param_ubo_legacy.glsl:9-12
layout(std140, set = 3, binding = 22) uniform AvatarAlphaShadowVParamUBO_Legacy
{
    float shadow_target_width;
};
```

= **1 member only**、shadow target width (= 推定 = shadow map render target width、avatar alpha shadow path 専用)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 22
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **set=3 内 Legacy 帯**: binding=22 (= avatar 系 Legacy 帯、avatar V/F params 周辺と推定)
- **source**: `ubo_metadata.inl:30` literal: `{ "AvatarAlphaShadowVParamUBO_Legacy", 0x2775be31u, 256u, 3u, 22u, 0u, 1u, 1u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: avatar alpha shadow program bind 単位で update、`sProgramUboDirty` triple-buffer 経路で flush
- **source**: ubo_metadata.inl:30 + llglslshader.cpp:95 + llglslshader.cpp:2147-2149

---

## §4. 物理 owner

- **data source**: `shadow_target_width` (float) = shadow render target width (= 推定 = `RenderShadowTargetWidth` cvar or `LLPipeline::mSunShadowMaps[i]->getWidth()` 由来、verify 要)
- **uniform 名 reserved**: `llshadermgr.cpp:1677` literal: `mReservedUniforms.push_back("shadow_target_width")`
- **uniform 名 ↔ enum**: `llshadermgr.h:195` literal: `DEFERRED_SHADOW_TARGET_WIDTH, // "shadow_target_width"`
- **既存 OpenGL 経路 writer site**: **不明 / verify 要** (= grep `DEFERRED_SHADOW_TARGET_WIDTH` で writer 特定要)
- **lifetime**: shadow target resize 時 / shader bind 時 (= verify 要)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/avatar_alpha_shadow_v_param_ubo_legacy.glsl`
- **実 shader use site** (= 1 file confirmed):
  - `indra/newview/app_settings/shaders/class1/deferred/avatarAlphaShadowV.glsl:59` (= blueprint source literal、`#ifdef LL_VULKAN_GLSL` block)
- **既存 OpenGL 経路**: 同 file 内 `#else` block で uniform 個別宣言 (= verify 要)

---

## §6. 既存 setter call site (host C++)

- **uniform 名 reserved**: `llshadermgr.cpp:1677` literal
- **uniform 名 ↔ enum**: `llshadermgr.h:195` literal
- **writer call site**: **不明 / verify 要** (= grep `DEFERRED_SHADOW_TARGET_WIDTH` で特定要、推定 pipeline.cpp shadow render 経路)
- **本 UBO 名指 setter**: なし (= `Grep "AvatarAlphaShadowVParamUBO_Legacy" indra/llrender` 結果 0 件)

---

## §7. 現状通電状態

- **状態**: **untouched** (= shell 通電もされていない)
- **PC-7γ-1 PerProgram register**: 条件付き = mUseUBO=true 時 avatarAlphaShadowV.glsl link 時に走る
- **PC-7γ-1 PerProgram write**: 条件付き = mUseUBO=true で setter が走ると writeProgramUbo に流れる (default OFF)

---

## §8. 本実装化に必要な作業

1. **mUseUBO ON 化** (= PerProgram cluster 共通 gate)
2. **shader 側 LL_VULKAN_GLSL block 活性化** (= avatarAlphaShadowV.glsl:59、起案済確認要)
3. **writer call site 特定** = grep `DEFERRED_SHADOW_TARGET_WIDTH` で pipeline.cpp / avatar shadow render 経路特定
4. **setter 経路 verify** (= mUseUBO=true 時の forwardToUboUpload → writeProgramUbo dispatch 確認)
5. **codegen 再実行不要** (= layout / member 不変)

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set=3 binding=22) |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ⚠️ avatarAlphaShadowV.glsl に LL_VULKAN_GLSL block 追加済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**1 member only UBO**:
- 256 B padded で 16 B (1 vec4) のみ使用 = overhead 大 (= 設計仕様、Legacy 構造由来、roadmap §3 整合)
- UBO consolidation 候補 (= 他 avatar shadow params と統合)、ただし Phase 1 では既存構造維持

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守)

1. **writer call site 特定** = `DEFERRED_SHADOW_TARGET_WIDTH` uniform setter site
2. **shadow_target_width data source** = cvar 由来 or runtime resize 由来 (= verify 要)
3. **update cadence** = shadow target resize 時のみか shader bind 毎か
4. **avatarAlphaShadowV.glsl 内 `#else` block 内容** = OpenGL path での uniform 宣言詳細

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

avatar 系 Legacy 帯 binding 周辺:
- **AvatarAlphaShadowVParamUBO_Legacy (binding=22、本 UBO)**
- PbrShadowAlphaMaskVParamUBO_Legacy (binding=21)
- PerProgramUBO_BlurLightF (set=2 binding=22、別 set)
- AvatarFParamUBO_Legacy (binding=54)
- AvatarClothVParamUBO_Legacy (binding=57)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 同 avatar 系 PerProgram cluster (= AvatarFParamUBO_Legacy / AvatarClothVParamUBO_Legacy / PbrShadowAlphaMaskVParamUBO_Legacy)

### §11.3 同 shader consume UBO

- avatarAlphaShadowV.glsl 内同時 consume: 不明 / verify 要 (= 推定 FrameViewProj + AvatarFParamUBO_Legacy or PerDrawUBO_AvatarSkin)

### §11.4 同 data source UBO

- 不明 / verify 要 (= shadow target params は他に shadow_bias / shadow_offset / shadow_blur 系 uniform あり、UBO 化されているか不明)

### §11.5 dirty 連動 UBO

- shadow target resize 時の dirty 連動 UBO 不明 (= verify 要、推定 PerProgramUBO_ShadowAlphaMaskV / PerProgramUBO_ShadowCubeV 系)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- set=3 帯 bind は `bindV3aStatic` / `bindV3aRigged` で全帯一括
- avatar shadow program bind 時 PerProgram cadence triple-buffer flush

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.3 同期)

**Layer**: L4-3 (= C 判定 shadow_target_width triple-write group)
**status**: **起案済** (= 2026-06-06 C-6、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.3` (= single source of truth)
**要点**: shadow_target_width 3 UBO triple-write (= ShadowAlphaMaskV + PbrShadowAlphaMaskV + AvatarAlphaShadowV)、本 UBO offset=0 (1 active member)、avatar alpha shadow program 専用、setter 7 site 全特定済、target_width data source [要追加調査 = cvar or runtime shadow buffer width]、工数 S-M、AYA live verify (= avatar shadow alpha 描画、visual regression ゼロ §5.4)
**関連**: L0-1 dispatch (= avatarAlphaShadowV program 識別) / L3-19 ShadowUtilParamUBO_Legacy (= shadow render 全 program 共有 pattern) / §5.4 visual regression policy


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

