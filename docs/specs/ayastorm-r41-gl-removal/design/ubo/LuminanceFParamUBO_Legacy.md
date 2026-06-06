# LuminanceFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: shell 通電済 + per-program write 経路通電済 (= Phase 1.A PA-8 + 1.C PC-7γ-1、blueprint 宣言済、実 shader UBO block は `class1/deferred/luminanceF.glsl:60` 内 `#ifdef LL_VULKAN_GLSL` block で literal extract 確認)

**本実装化に必要な作業**: 既存 `diffuse_luminance_scale` setter (= `pipeline.cpp:8754` literal `gLuminanceProgram.uniform1f(diffuse_luminance_scale_s, diffuse_luminance_scale);`) の forwardToUboUpload PER_PROGRAM 経路 UBO redirect 通電

---

## §1. UBO identity

- **block_name**: `LuminanceFParamUBO_Legacy`
- **block_hash**: `0xf69cbf74u` (= `ubo_metadata.inl:51` literal)
- **block_size**: 256 B (= std140=16, device-padded=256、`ubo_layout_luminancefparamubo_legacy.inl:15` literal)
- **member_count**: 1
- **struct definition**:

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_luminancefparamubo_legacy.inl
struct LuminanceFParamUBO_LegacyLayout {
    static constexpr std::uint32_t diffuse_luminance_scale_OFFSET = 0u; // size=4 align=4
};
inline constexpr std::uint32_t LuminanceFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/luminance_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 26) uniform LuminanceFParamUBO_Legacy
{
    float diffuse_luminance_scale;
};
```

= **1 member、float 4 B → 256 B padded** (= 最小 member UBO)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 26
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:51` `{ "LuminanceFParamUBO_Legacy", 0xf69cbf74u, 256u, 3u, 26u, 0u, 1u, 1u }` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program**
- **flush 経路**: `LLVKLoader::flushProgramUbos`
- **write 経路**: `LLVKLoader::writeProgramUbo`
- **storage**: `sProgramUboDirty`

---

## §4. 物理 owner

- **owner**: `LLPipeline::renderPostProcess` luminance pass (= `pipeline.cpp:8729-8762` literal、`gLuminanceProgram` 経路)
- **data source**:
  - `diffuse_luminance_scale` = `RenderDiffuseLuminanceScale` cvar (= `pipeline.cpp:8731` literal: `static LLCachedControl<F32> diffuse_luminance_scale(gSavedSettings, "RenderDiffuseLuminanceScale", 1.0f);`)
- **lifetime**: per-program (= luminance pass bind 時)

---

## §5. use site (shader)

- **blueprint file**: `set3/luminance_f_param_ubo_legacy.glsl`
- **blueprint origin**: `class1/deferred/luminanceF.glsl:60` (= literal extract source)
- **実 shader use site**: `class1/deferred/luminanceF.glsl` (= grep result 唯一)
- **consume**: luminance pass fragment shader、scene luminance 計算で diffuse channel scaling (= adaptive exposure / tonemap pipeline 上流)

---

## §6. 既存 setter call site (host C++)

- **`diffuse_luminance_scale` setter**: `pipeline.cpp:8754` literal: `gLuminanceProgram.uniform1f(diffuse_luminance_scale_s, diffuse_luminance_scale);`
- **`diffuse_luminance_scale_s` 宣言**: `pipeline.cpp:8753` literal: `static LLStaticHashedString diffuse_luminance_scale_s("diffuse_luminance_scale");`
- **reserved string 宣言**: `llglslshader.cpp:1082` literal: `"delta", "diffuse_luminance_scale", "direction", ...` (= reserved uniform 一覧内)
- **共通 redirect 経路**: `LLGLSLShader::uniform1f`
- **PER_PROGRAM case**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo`

---

## §7. 現状通電状態

- **状態**: shell 通電済 + write 経路本格化済
- **bind 経路**: set=3 binding=26 で program 切替時 bind
- **write 経路**: `forwardToUboUpload` PER_PROGRAM → `writeProgramUbo`
- **MUSEUBO-A 整合**: `mUseUBO=false` default で不到達

---

## §8. 本実装化に必要な作業

1. **`mUseUBO=true` cold launch 検証**:
   - `pipeline.cpp:8754` setter call から forwardToUboUpload PER_PROGRAM 経由 writeProgramUbo 実走確認
2. **`LLStaticHashedString` 経路の UBO redirect**:
   - `LLStaticHashedString` 引数版 uniform1f が `LLGLSLShader::uniform1f(LLStaticHashedString)` overload 経由で UBO redirect されるか verify
   - 推定: 内部で getUniformLocation 経由 mUniformUBOLoc index 解決 → forwardToUboUpload (= verify 要、`llglslshader.cpp:2661` 周辺 literal で確認)
3. **codegen 再実行不要**: blueprint member 不変

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ float = 4 B std140 → 256 B padded |
| OS-4 | minUniformBufferOffsetAlignment | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ luminanceF で UBO block 宣言済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**`LLStaticHashedString` overload risk**:
- 既存 setter が `LLStaticHashedString` 引数版 (= name-based) を使用
- UBO redirect path = mUniformUBOLoc index-based、name → index 解決経路で UBO redirect が透過するか verify 必須
- 既存 PC-7γ-1 で 31 setter 経路は index 版 + LLStaticHashedString 版両対応想定 (= `llglslshader.cpp:2661` 周辺 literal で `mUniformUBOLoc[index]` 経路確認、name 版経路の確認要)

**252 B dead space**:
- 1 member 4 B vs padded 256 B

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3、Legacy 帯)

- 同 set=3 = ~58 UBO

### §11.2 同 cadence cluster UBO (= cadence_tag=1 per-program)

- 全 Legacy UBO + 本 UBO

### §11.3 同 shader consume UBO

- `class1/deferred/luminanceF.glsl` で同時 consume = vertex shader 由来 UBO + `LLShaderMgr::DEFERRED_DIFFUSE` / `DEFERRED_EMISSIVE` / `NORMAL_MAP` texture sampler (= `pipeline.cpp:8734, 8740, 8746` literal)

### §11.4 同 data source UBO

- **同 owner**: `LLPipeline::renderPostProcess` luminance / exposure / tonemap chain
- 同 owner = `ExposureFParamUBO_Legacy` (= 同 exposure pass、`pipeline.cpp:8791-8865` literal) / `TonemapUBO_Legacy` (= `ubo_metadata.inl:114` literal、同 tonemap chain)

### §11.5 dirty 連動 UBO

- luminance pass bind 時に本 UBO 単独 dirty
- 連動 = `ExposureFParamUBO_Legacy` (= 同 luminance → exposure → tonemap pipeline 経路、cvar 連動)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- post-process chain: luminance (本 UBO) → exposure → tonemap → glow chain
- program bind 時に set=3 binding=26 更新

---

## §10. 不明事項

1. **`LLStaticHashedString` overload 版 UBO redirect 経路** = name-based setter が `mUniformUBOLoc` index-based UBO redirect に到達するか経路確認
2. **`RenderDiffuseLuminanceScale` cvar 変更時 dirty trigger** = cvar 変更で連続 frame 更新か、setter 呼出時のみ更新か
3. **set 3 bind 単位** = program 切替時 set 3 全 binding rebind か個別 rebind か
4. **luminance → exposure → tonemap chain dirty 連動** = pipeline chain での連動 dirty trigger 経路
5. **252 B dead space** = Phase 2 で luminance 関連 param 追加候補

= 上記 5 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.10 同期)

**Layer**: L4-10 sub-cluster (a) (= auto-exposure chain 2 UBO)
**status**: **起案済** (= 2026-06-06 C-6-e、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.10` (= single source of truth)
**要点**: 1 member (diffuse_luminance_scale float)、**最小 member UBO** (= 4B std140 / 256B padded、252B dead space)、**shell 通電済 + write 経路本格化済** (= PA-8 + PC-7γ-1)、setter `pipeline.cpp:8754` literal `gLuminanceProgram.uniform1f(diffuse_luminance_scale_s, diffuse_luminance_scale);` (= LLStaticHashedString name-based setter)、data source `RenderDiffuseLuminanceScale` cvar (= `pipeline.cpp:8731` literal LLCachedControl)、**LLStaticHashedString overload 版 UBO redirect 経路 verify 必須** (= L0-2 経路、name-based setter が mUniformUBOLoc index 経路に到達するか) [要 verify]、auto-exposure per-frame trigger で Exposure と同期 dirty、cadence PerProgram 維持、工数 group 全体 L 内 (S 部分、本 group 最先着手可)
**関連**: L0-1 dispatch (= 衝突なし binding=26) / L0-2 LLStaticHashedString redirect (= 本 UBO redirect pilot) / §3.5.10 sub-cluster (a) Exposure (= auto-exposure chain pair) / `pipeline.cpp:8729-8762` literal


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

