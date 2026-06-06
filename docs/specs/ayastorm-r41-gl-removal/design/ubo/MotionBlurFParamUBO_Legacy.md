# MotionBlurFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= blueprint + codegen metadata 生成済、host C++ register/write/flush 経路未着工)

**本実装化に必要な作業**: register/write/flush 経路新設 (= per-program cadence、`LLShaderMgr::MOTION_BLUR_STRENGTH` `uniform1i` setter を UBO write に redirect) + `pipeline.cpp:10240` 周辺 motion blur 経路の setter call site を UBO 経由に切替

---

## §1. UBO identity

- **block_name**: `MotionBlurFParamUBO_Legacy`
- **block_hash**: `0x48bfa3ebu` (= FNV-1a("MotionBlurFParamUBO_Legacy"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_motionblurfparamubo_legacy.inl:12-14
struct MotionBlurFParamUBO_LegacyLayout {
    static constexpr std::uint32_t motion_blur_strength_OFFSET = 0u;  // size=4 align=4
};
inline constexpr std::uint32_t MotionBlurFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/class1/deferred/motionBlurF.glsl:66
layout(set=3, binding=27, std140) uniform MotionBlurFParamUBO_Legacy {
    int motion_blur_strength;
};
```

= Blueprint (= `aya_r41_blueprints/set3/motion_blur_f_param_ubo_legacy.glsl:9-12`) 一致。**注: member 型は `int`** (= GLSL int = 4 B、std140 align=4)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 27
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: 不明 / verify 要 (= V3a 設計 set=3 binding 配置整合性)
- **source**: `ubo_metadata.inl:55` `{ "MotionBlurFParamUBO_Legacy", 0x48bfa3ebu, 256u, 3u, 27u, 0u, 1u, 1u }` + `class1/deferred/motionBlurF.glsl:66` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program** (= `llglslshader.cpp:95`)
- **意味詳細**: motion blur shader (= post-deferred) は program load 時 register、`RenderMotionBlurStrength` cvar 変更時に write
- **source**: ubo_metadata.inl + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **owner**: `LLCachedControl<S32> motion_blur_strength(gSavedSettings, "RenderMotionBlurStrength", 32);` (= `indra/newview/pipeline.cpp:10240` literal)
- **lifetime**: cvar 単位 (= user 設定変更まで不変、起動時 default = 32)
- **用途**: motion blur 強度 (= `motionBlurF.glsl:110` `float max_blur = float(motion_blur_strength);` で float 化して使用)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/motion_blur_f_param_ubo_legacy.glsl`
- **実 shader use site** = **`class1/deferred/motionBlurF.glsl:66` 単独** (= blueprint header literal「Source: literal extract from class1/deferred/motionBlurF.glsl:66」、grep 結果 1 file)
- consume 内容 (= grep 確認済):
  - `motionBlurF.glsl:110` `float max_blur = float(motion_blur_strength);`
  - `motionBlurF.glsl:167` `float max_blur = float(motion_blur_strength);`

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: 未配線
- **既存 OpenGL 経路 setter**:
  - `indra/newview/pipeline.cpp:10240-10242` (literal):
    ```cpp
    static LLCachedControl<S32> motion_blur_strength(gSavedSettings, "RenderMotionBlurStrength", 32);
    ...
    if (RenderMotionBlur && mVelocityMap.isComplete() && motion_blur_strength > 0 && !gCubeSnapshot)
    ```
  - 上記直後で `uniform1i(LLShaderMgr::MOTION_BLUR_STRENGTH, motion_blur_strength)` 等の setter 呼出があるはず (= 完全 trace 未取得、verify 要)
- **reserved uniform 登録**: `indra/llrender/llshadermgr.cpp:1877` `mReservedUniforms.push_back("motion_blur_strength");`
- **enum entry**: `indra/llrender/llshadermgr.h:396` `MOTION_BLUR_STRENGTH, // "motion_blur_strength"`

---

## §7. 現状通電状態

- **状態**: **untouched**
- **codegen 生成済**: layout `inl` + metadata entry + block_hash constexpr 生成済
- **blueprint 起案済**: `aya_r41_blueprints/set3/motion_blur_f_param_ubo_legacy.glsl`
- **shader 宣言済**: `class1/deferred/motionBlurF.glsl:66` `#ifdef LL_VULKAN_GLSL` block
- **register/write/flush 経路**: 未配線

---

## §8. 本実装化に必要な作業

1. **register 経路**: `mapUniforms()` で motion blur program に `registerProgramUbo(this, block_hash::MotionBlurFParamUBO_Legacy, 256u)` 呼出
2. **write 経路**: pipeline.cpp:10240 周辺の `uniform1i(MOTION_BLUR_STRENGTH, ...)` setter を `forwardToUboUpload` 経由 UBO write に redirect
3. **flush 経路**: cmdbuf bind 経路で sProgramUboDirty を flush
4. **shader 接続**: shader 追加改変なし (= 既に UBO 宣言済)

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set=3 binding=27) |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既に UBO 宣言済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**特記 risk**:
- member 型 `int` (= GLSL 32-bit int) = host 側 `S32` (`LLCachedControl<S32>`) と直接互換、type mismatch なし
- cvar 単位 dirty = user 設定変更まで dirty 立たない → flush 頻度低 = ring buffer 利用効率良
- per-program cadence と cvar 寿命の整合: cvar は viewer process 寿命、per-program write は冗長だが既存設計準拠

---

## §10. 不明事項

1. **`uniform1i(MOTION_BLUR_STRENGTH, ...)` setter call site** = pipeline.cpp:10240 周辺の正確な setter line (= 本起案では cvar 取得のみ確認、setter 直接 grep 未取得)
2. **dirty trigger 詳細** = cvar 変更通知の subscribe pattern (= `LLCachedControl` から `forwardToUboUpload` までの bridge logic)
3. **set=3 帯 layout の binding 上限** = V3a 設計と Legacy UBO 実 binding 配置の整合性
4. **motionBlurF.glsl 全文の他 UBO consume** = 同 file で同時 consume される他 UBO (= FrameViewProj 等)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- set=3 帯 Legacy UBO 群と共存
- 直近 binding 同居: binding=26 LuminanceFParamUBO_Legacy / binding=28 ScreenSpaceReflPostFParamUBO_Legacy

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 73 件 (推定) per-program cluster 内の 1 UBO

### §11.3 同 shader consume UBO (= `class1/deferred/motionBlurF.glsl` で同時 consume)

- 不明 / verify 要 (= motionBlurF.glsl 全文の UBO 宣言群確認要)
- 関連 shader (= velocity 経路、grep 結果): `skinnedVelocityV.glsl` / `skinnedVelocityAlphaV.glsl` / `avatarVelocityV.glsl` (= `lastMatrixPalette` 共有、別 UBO 系)、`motionBlurF.glsl:82` comment「buffer fully — accepts the upstream avatar lastMatrixPalette」

### §11.4 同 data source UBO

- 単独 (= `RenderMotionBlurStrength` cvar 専用 UBO、他 UBO と data source 共有なし)

### §11.5 dirty 連動 UBO

- 単独 dirty (= cvar 変更 trigger 単独)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- post-deferred motion blur program 切替時 set=3 帯全 binding を一括 rebind

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.3.3 同期)

**Layer**: L2-3 (= B Tier α setter 推定済、PerProgram cadence、1 member 単純)
**status**: **起案済** (= 2026-06-06 C-4、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.3.3` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch + L0-4 cadence
- **(2) 不明事項**: `uniform1i(MOTION_BLUR_STRENGTH, ...)` setter 行 (= `pipeline.cpp:10240` 周辺の正確な行) [要 D1 Grep] / dirty trigger 詳細 (= `LLCachedControl` から `forwardToUboUpload` までの bridge logic) [要追加調査] / set=3 帯 binding 上限 [要 verify]
- **(3) 調査手法**: D1 (`MOTION_BLUR_STRENGTH` uniform1i call 行特定) + D2 (`LLCachedControl<S32>` subscribe pattern + dirty trigger)
- **(4) 設計 task**: PerProgram cadence triple-buffer (= motion blur program active 時) / setter 1 uniform1i call を `forwardToUboUpload` → `writeProgramUbo` (= 4 B memcpy) / motion blur program bind 単位 flush (= cvar 変更時 dirty) / `motionBlurF.glsl:66` 既存 LL_VULKAN_GLSL block 活性化
- **(5) 工程**: trace 順 L2 3 件目、工数 **S** (= 数時間、1 member + setter 行特定のみ)、L2-1 / L2-2 / L2-4 並列可
- **(6) A 確定**: mUseUBO ON + shader 活性化 + setter 通電 + AYA live verify (= motion blur 効果既存と同一強度、**visual regression ゼロ §5.4**) + Vulkan validation 0 + cvar `RenderMotionBlurStrength` 変更時 dirty trigger 反映 verify
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `motionBlurF.glsl #else` block uniform 維持

**関連**: L0-1 + L0-4 (= WORK_ORDER §2) / §5.4 visual regression policy


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

