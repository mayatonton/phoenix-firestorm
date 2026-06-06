# ScreenSpaceReflPostFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `screen_space_refl_post_f_param_ubo_legacy.glsl` は実 shader `class3/deferred/screenSpaceReflPostF.glsl ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加。**SSR (Screen Space Reflections) post-process pass** 専用 UBO。

**注記**: memory `project_transparent_ssao_ssr_no_work` = AYAstorm で SSR は glass 限定で勝つが material 判定重い、no scheduled work 維持。本 UBO は既存 SSR 機能維持用 (= 拡張 phase なし)。

---

## §1. UBO identity

- **block_name**: `ScreenSpaceReflPostFParamUBO_Legacy`
- **block_hash**: `0x254e6465u` (= FNV-1a("ScreenSpaceReflPostFParamUBO_Legacy"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 2
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_screenspacereflpostfparamubo_legacy.inl
struct ScreenSpaceReflPostFParamUBO_LegacyLayout {
    static constexpr std::uint32_t zNear_OFFSET = 0u;  // size=4 align=4
    static constexpr std::uint32_t zFar_OFFSET = 4u;  // size=4 align=4
};
inline constexpr std::uint32_t ScreenSpaceReflPostFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/screen_space_refl_post_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 28) uniform ScreenSpaceReflPostFParamUBO_Legacy
{
    float zNear;
    float zFar;
};
```

= 2 member (zNear / zFar) = depth linearize 用 camera near/far plane。

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 28
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、verify 要)
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:101` `{ "ScreenSpaceReflPostFParamUBO_Legacy", 0x254e6465u, 256u, 3u, 28u, 0u, 1u, 2u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush、camera zNear/zFar は frame 単位で固定 = SSR pass bind 時に 1 回 update
- **source**: ubo_metadata.inl:101 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - `LLViewerCamera` の zNear/zFar (= camera projection state、grep verify 要)
  - SSR post-process pipeline (= LLPipeline::renderPostProcess 内 SSR 経路)
- **lifetime**: program 単位 (= SSR post shader bind 中)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/screen_space_refl_post_f_param_ubo_legacy.glsl`
- **実 shader use site**:
  - **`class3/deferred/screenSpaceReflPostF.glsl:57 ifdef LL_VULKAN_GLSL block`** (= blueprint comment literal)
- **使用 uniform**: zNear (float) / zFar (float)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **本実装化後 setter** (= **不明 / verify 要**):
  - SSR pass setter (= `uniform1f(zNear)` + `uniform1f(zFar)` 呼出 site、grep verify 要)
  - LLViewerCamera::getNear() / getFar() 由来 (= verify 要)

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 範囲、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電** + dummy buffer write + bind 経路通電
2. **実 member data 流入**: LLViewerCamera から zNear/zFar 取得 + UBO write
3. **dirty 判定 logic 追加**: PerProgram cadence + camera near/far 変化時 dirty (= 通常 frame 内不変、camera mode 切替時のみ)
4. **flush logic 追加**: PerProgram cadence flush
5. **shader 接続**: 実 shader 既存 UBO declaration 使用 = 追加 shader 改変なし

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=3 binding=28 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**重複 owner risk**: FrameViewProj UBO (set=0 binding=0) に projection matrix が既に含まれる場合、zNear/zFar は derive 可能 = 本 UBO の独立性 verify 要 (= Frame 系 UBO で十分カバーされる可能性、ただし shell layout 不変契約遵守)。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)
- set=3 帯 Legacy 群

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)
- 73 件 (推定) cluster

### §11.3 同 shader consume UBO (= class3/deferred/screenSpaceReflPostF.glsl)
- **不明 / verify 要** = screenSpaceReflPostF.glsl 内同時 consume UBO (= FrameViewProj + sampler bindings 等可能性、verify 要)

### §11.4 同 data source UBO
- **可能性** (= verify 要): **FrameViewProj** (set=0 binding=0、ubo_metadata.inl:42) = camera projection matrix 由来、zNear/zFar derivative
- 他 SSR / post-process UBO 群 (= LuminanceFParamUBO_Legacy / ExposureFParamUBO_Legacy 等) との camera state 共有

### §11.5 dirty 連動 UBO
- **可能性** (= verify 要): camera state 由来全 UBO (= FrameViewProj、その他 zNear/zFar 使用 UBO)

### §11.6 layout 共有関係
- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係
- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **既存 OpenGL 経路 setter call site** = SSR pass の zNear/zFar 書込 site (= grep verify 要)
2. **camera zNear/zFar owner** = LLViewerCamera state / LLPipeline state (= verify 要)
3. **FrameViewProj UBO との関係** = projection matrix から derive 可能か独立 verify 要
4. **SSR enable cvar** = AYAstorm 側で SSR on/off 制御 (= verify 要)
5. **shell 通電 commit** = 未来作業
6. **camera mode 切替時の zNear/zFar 変化 trigger** = dirty 判定 logic 設計 (= verify 要)
7. **同 post-process 内の他 zNear/zFar 使用 UBO** = 重複設計か独立か (= verify 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.4.3 同期)

**Layer**: L3-3 (= B Tier β setter 推定済、PerProgram cadence、SSR zNear/zFar)
**status**: **起案済** (= 2026-06-06 C-5、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.4.3` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch + L0-4 cadence
- **(2) 不明事項**: `zNear` / `zFar` setter (= `LLViewerCamera::getNear()/getFar()` 由来明示) [要追加調査] / FrameViewProj から derive 可能性 (= 重複 owner risk) [要 verify]
- **(3) 調査手法**: D1 (`zNear` / `zFar` setter grep) + D4 (FrameViewProj との重複 owner verify)
- **(4) 設計 task**: L3 全件共通 (= PerProgram triple-buffer / `forwardToUboUpload` PER_PROGRAM / program bind 単位 flush / `screenSpaceReflPostF.glsl` LL_VULKAN_GLSL 活性化)
- **(5) 工程**: trace L3-3、工数 **S**、並列可
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= SSR effect 既存と同一、glass 限定で勝つ既存挙動維持、visual regression ゼロ §5.4)
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `screenSpaceReflPostF.glsl #else` block uniform 個別宣言維持

**関連**: L0-1 + L0-4 / §5.4 / FrameViewProj (= 重複 owner risk verify) / memory `project_transparent_ssao_ssr_no_work` (= SSR no scheduled work)


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

