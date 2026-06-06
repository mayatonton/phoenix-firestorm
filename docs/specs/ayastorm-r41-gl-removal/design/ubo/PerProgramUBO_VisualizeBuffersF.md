# PerProgramUBO_VisualizeBuffersF — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `per_program_ubo_visualize_buffers_f.glsl` は実 shader `class1/deferred/postDeferredVisualizeBuffers.glsl:40 ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_VisualizeBuffersF`
- **block_hash**: `0x587c355eu` (= FNV-1a("PerProgramUBO_VisualizeBuffersF"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 4 (= 1 active + 3 tail pad)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_visualizebuffersf.inl
struct PerProgramUBO_VisualizeBuffersFLayout {
    static constexpr std::uint32_t mipLevel_OFFSET = 0u;  // size=4 align=4
    static constexpr std::uint32_t _pad_visbuf0_OFFSET = 4u;  // size=4 align=4
    static constexpr std::uint32_t _pad_visbuf1_OFFSET = 8u;  // size=4 align=4
    static constexpr std::uint32_t _pad_visbuf2_OFFSET = 12u;  // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_VisualizeBuffersF_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_visualize_buffers_f.glsl
layout(std140, set = 2, binding = 16) uniform PerProgramUBO_VisualizeBuffersF
{
    float mipLevel;
    float _pad_visbuf0;
    float _pad_visbuf1;
    float _pad_visbuf2;
};
```

= active member 1 個 (= `mipLevel` 4 B) + tail pad 12 B。blueprint comment literal: `Source: literal extract from class1/deferred/postDeferredVisualizeBuffers.glsl:40 ifdef LL_VULKAN_GLSL block (single site)`。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 16
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通)
- **pipeline layout**: `sAYAStandardLayout`
- **set 2 配置**: PerDraw + PerProgram 帯
- **source**: `ubo_metadata.inl:91` `{ "PerProgramUBO_VisualizeBuffersF", 0x587c355eu, 256u, 2u, 16u, 0u, 1u, 4u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush。Buffer visualize F stage で sampling mip level を保持 (= debug visualize 用)
- **source**: ubo_metadata.inl:91 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source**:
  - `mipLevel` = debug visualize switch (= `pipeline.cpp:8707-8711`):
    ```cpp
    static LLStaticHashedString mipLevel("mipLevel");
    gDeferredBufferVisualProgram.uniform1f(mipLevel, 0);  // :8709
    gDeferredBufferVisualProgram.uniform1f(mipLevel, 8);  // :8711
    ```
- **lifetime**: program 単位 (= buffer visualize render 中)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_visualize_buffers_f.glsl`
- **実 shader use site**: **`class1/deferred/postDeferredVisualizeBuffers.glsl:40 ifdef LL_VULKAN_GLSL block`** (= single site)
  - 既存 UBO block (postDeferredVisualizeBuffers.glsl:41):
    ```glsl
    float mipLevel;
    ```
  - 既存 OpenGL `#else` block (postDeferredVisualizeBuffers.glsl:48):
    ```glsl
    uniform float mipLevel;
    ```
  - 使用箇所: `postDeferredVisualizeBuffers.glsl:55` (`vec4 diff = textureLod(diffuseRect, vary_fragcoord.xy, mipLevel)`)
- **同 setter 他 shader use** (= 同 `mipLevel` LLStaticHashedString は別 shader でも使用候補):
  - `class1/interface/radianceGenF.glsl` (= 同 `mipLevel` 使用、別 UBO `RadianceGenFParamUBO_Legacy` (= set=3 binding=51) で対応の可能性、verify 要)
  - `class3/deferred/screenSpaceReflUtil.glsl` (= cinematic_bd 配下、同 `mipLevel` 使用、別経路、verify 要)
  - `pipeline.cpp:11819` で AYAR15Godrays 関連の `mipLevel` 言及あり (= 名前空間 collision の可能性、verify 要)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **既存 OpenGL 経路 setter**:
  - **`pipeline.cpp:8707-8711`**:
    ```cpp
    static LLStaticHashedString mipLevel("mipLevel");
    if (...)
        gDeferredBufferVisualProgram.uniform1f(mipLevel, 0);   // :8709
    else
        gDeferredBufferVisualProgram.uniform1f(mipLevel, 8);   // :8711
    ```
  - **`pipeline.cpp:8707-8711` 経由 `gDeferredBufferVisualProgram`** 専用 (= `LLShaderMgr` enum 経由でない、直接 `LLStaticHashedString` 使用、特殊)
- **本実装化後 setter**:
  - 31 setter 経路 (= mUseUBO 分岐) で UBO 化対応要

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **通電内容**: なし
- **blueprint 配置 commit**: 不明

---

## §8. 本実装化に必要な作業

1. **shell 通電**: blueprint ベースで host 側 buffer 配置 + descriptor set 配線
2. **実 member data 流入**:
   - `pipeline.cpp:8709 / 8711` 2 setter site を UBO 化 (= `gDeferredBufferVisualProgram` 専用)
3. **dirty 判定 logic 追加**:
   - PerProgram cadence + debug visualize mode 切替時 dirty
4. **flush logic 追加**: PerProgram cadence flush (= `writeProgramUbo` 経路)
5. **shader 接続**:
   - 実 shader `class1/deferred/postDeferredVisualizeBuffers.glsl:40 ifdef LL_VULKAN_GLSL block` UBO declaration 既存 = 追加 shader 改変なし
   - 既存 OpenGL `#else` block uniform 個別宣言は温存
6. **tail pad 維持**: `_pad_visbuf0/1/2` (12 B) は std140 padding (16 B 完成のため)
7. **同名 collision 整理**:
   - 別 shader (= radianceGenF / screenSpaceReflUtil) で同名 `mipLevel` uniform、本 UBO は postDeferredVisualizeBuffers 専用、別 UBO で対応されているか確認 (= verify 要)

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 binding=16 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**debug-only feature**: buffer visualize は debug 専用 (= 通常 release では未使用 path)、Vulkan 化優先度低の可能性。ただし shader 既存で UBO declaration 済ゆえ実装は必要。

**`LLStaticHashedString` 直接 setter**: 通常 `LLShaderMgr` enum 経由でなく `LLStaticHashedString` 直接使用、UBO 化時の 31 setter 経路で本 pattern が対応されているか確認 (= verify 要)。

**同名 `mipLevel` 多 shader 共有**: radianceGenF / screenSpaceReflUtil 等で同名 uniform、別 UBO 別 program でそれぞれ持つ設計 (= 名前 collision でなく program 別)、本 UBO は本 shader 専用ゆえ問題なし。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 = PerDraw + PerProgram 混在

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- ubo_metadata.inl で 73 件の最大 cluster

### §11.3 同 shader consume UBO (= class1/deferred/postDeferredVisualizeBuffers.glsl)

- **不明 / verify 要** = postDeferredVisualizeBuffers.glsl 内で他に consume される UBO (= FrameViewProj / diffuseRect sampler 等、grep verify 要)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **RadianceGenFParamUBO_Legacy** (= set=3 binding=51、radianceGenF.glsl 用、同名 `mipLevel` 使用候補、別 program 別 UBO)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- debug visualize mode 切替時、debug 系 UBO 限定の連動候補

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **`LLStaticHashedString` 直接 setter pattern の UBO 化対応** = 31 setter 経路で `LLShaderMgr` enum 経由でない `LLStaticHashedString` 直接呼出が UBO 化 redirect されるか (= verify 要)
2. **`gDeferredBufferVisualProgram` 経路の cold launch 確認** = debug-only path、通常 release で発火しないため Phase 2 cold launch 時に validation 確認できるか (= verify 要)
3. **同名 `mipLevel` 多 shader 別 UBO 設計** = radianceGenF / screenSpaceReflUtil 等で同 uniform 名、別 UBO 別 program で対応の design pattern 確認 (= verify 要)
4. **同 shader file 内同時 consume UBO 一覧** = postDeferredVisualizeBuffers.glsl 内全 UBO declaration grep 要
5. **shell 通電 commit** = 未来作業
6. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = verify 要
7. **tail pad 12 B 将来 member 追加意図** = 予約 slot か、std140 padding のみか (= verify 要)

= 上記 7 項目は本 UBO file 完成時に逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.1.3 同期)

**Layer**: L1a-3 (= debug only + LLStaticHashedString 直接 setter pattern 補強)
**status**: **起案済** (= 2026-06-06 C-3、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.1.3` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 + L0-2 + L1a-1 + L1a-2 pilot verify 完了 (= redirect 経路成熟確認)
- **(2) 不明事項**: LLStaticHashedString 直接 setter pattern UBO 化対応 (= `LLShaderMgr` enum 経由でない直接呼出) [L0-2 protocol-A 確認] / debug-only path cold launch (= 通常 release で発火しない) [要 verify] / 同名 `mipLevel` 多 shader (= radianceGenF / screenSpaceReflUtil) 別 UBO 設計 [要追加調査] / 同 shader file 同時 consume UBO 一覧 [要追加調査] / tail pad 12 B 将来意図 [要 verify]
- **(3) 調査手法**: D1 + D2 + D4
- **(4) 設計 task**: PerProgram cadence triple-buffer (= debug visualize mode active 時のみ) / `pipeline.cpp:8709/8711` 2 LLStaticHashedString call intercept / `gDeferredBufferVisualProgram` bind flush / `class1/deferred/postDeferredVisualizeBuffers.glsl:40` LL_VULKAN_GLSL 活性化
- **(5) 工程**: trace 順 L1a 3 件目 (= debug-only 補強、pilot 検証完了後)、工数 **S**、L1a-1 / L1a-2 並列可
- **(6) A 確定**: mUseUBO ON + shader 活性化 + 2 setter 通電 + AYA live verify (= debug menu 経由 buffer visualize 起動、出力既存と同一、visual regression ゼロ §5.4、verify protocol 要事前 AYA 確認) + Vulkan validation 0 件 + 同名 `mipLevel` 別 UBO collision 解消 verify
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `postDeferredVisualizeBuffers.glsl #else` block uniform 個別宣言維持、通常 release path 影響なし (= debug-only)

**関連**: L0-1 (= WORK_ORDER §2.1) + L0-2 (= §2.2) / visual regression policy (= §5.4)


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

