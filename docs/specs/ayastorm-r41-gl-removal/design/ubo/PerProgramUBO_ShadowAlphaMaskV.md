# PerProgramUBO_ShadowAlphaMaskV — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `per_program_ubo_shadow_alpha_mask_v.glsl` は実 shader `class1/deferred/shadowAlphaMaskV.glsl:85 ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_ShadowAlphaMaskV`
- **block_hash**: `0x16dfb92fu` (= FNV-1a("PerProgramUBO_ShadowAlphaMaskV"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 4 (= 1 active + 3 tail pad)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_shadowalphamaskv.inl
struct PerProgramUBO_ShadowAlphaMaskVLayout {
    static constexpr std::uint32_t shadow_target_width_OFFSET = 0u;  // size=4 align=4
    static constexpr std::uint32_t _pad_sam0_OFFSET = 4u;  // size=4 align=4
    static constexpr std::uint32_t _pad_sam1_OFFSET = 8u;  // size=4 align=4
    static constexpr std::uint32_t _pad_sam2_OFFSET = 12u;  // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_ShadowAlphaMaskV_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_shadow_alpha_mask_v.glsl
layout(std140, set = 2, binding = 6) uniform PerProgramUBO_ShadowAlphaMaskV
{
    float shadow_target_width;
    float _pad_sam0;
    float _pad_sam1;
    float _pad_sam2;
};
```

= active member 1 個 (= `shadow_target_width` 4 B) + tail pad 12 B。blueprint comment literal: `Source: literal extract from class1/deferred/shadowAlphaMaskV.glsl:85 ifdef LL_VULKAN_GLSL block (single site)`。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 6
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通)
- **pipeline layout**: `sAYAStandardLayout`
- **set 2 配置**: PerDraw + PerProgram 帯
- **source**: `ubo_metadata.inl:87` `{ "PerProgramUBO_ShadowAlphaMaskV", 0x16dfb92fu, 256u, 2u, 6u, 0u, 1u, 4u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush。Shadow alpha mask V stage で target texture width を保持 (= shadow map size 依存の補正)
- **source**: ubo_metadata.inl:87 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source**:
  - `shadow_target_width` = `LLPipeline` shadow target render texture width (= `pipeline.cpp:8562 / 8570 / 8584 / 8592 / 12596 / 12611 / 12642` 7 site で `LLGLSLShader::sCurBoundShaderPtr->uniform1f(LLShaderMgr::DEFERRED_SHADOW_TARGET_WIDTH, (float)target_width)`)
- **lifetime**: program 単位 (= shadow render pass 中で program 単位 width 確定)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_shadow_alpha_mask_v.glsl`
- **実 shader use site**: **`class1/deferred/shadowAlphaMaskV.glsl:85 ifdef LL_VULKAN_GLSL block`** (= single site)
  - 既存 UBO block (shadowAlphaMaskV.glsl:86):
    ```glsl
    float shadow_target_width;
    ```
  - 既存 OpenGL `#else` block (shadowAlphaMaskV.glsl:93):
    ```glsl
    uniform float shadow_target_width;
    ```
  - 使用箇所: `shadowAlphaMaskV.glsl:149` (`target_pos_x = 0.5 * (shadow_target_width - 1.0) * pos.x`)
- **同 setter 共有 site** (= 同 uniform を別 shader でも使用、別 UBO 別配線):
  - `class1/deferred/pbrShadowAlphaMaskV.glsl` (= 同 shadow target width 使用、別 UBO の可能性、verify 要)
  - `class1/deferred/avatarAlphaShadowV.glsl` (= 同上、別 UBO 別 set の可能性 = `AvatarAlphaShadowVParamUBO_Legacy` 等、verify 要)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **既存 OpenGL 経路 setter**:
  - `llshadermgr.h:195` `DEFERRED_SHADOW_TARGET_WIDTH, // "shadow_target_width"`
  - 実 setter (= 7 site):
    - **`pipeline.cpp:8562 / 8570 / 8584 / 8592`** `LLGLSLShader::sCurBoundShaderPtr->uniform1f(LLShaderMgr::DEFERRED_SHADOW_TARGET_WIDTH, (float)target_width)` (= 4 site 連続、shadow rendering 経路内)
    - **`pipeline.cpp:12596 / 12611 / 12642`** 同 setter (= 3 site、別 shadow rendering 経路)
- **本実装化後 setter**:
  - 31 setter 経路 (= mUseUBO 分岐) で UBO 化対応要、本 7 site で UBO write 経由に差替

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
   - `pipeline.cpp:8562 / 8570 / 8584 / 8592 / 12596 / 12611 / 12642` 7 setter site を UBO 化
3. **dirty 判定 logic 追加**:
   - PerProgram cadence + shadow target resize 時 dirty (= cvar listener / pipeline state change trigger)
4. **flush logic 追加**: PerProgram cadence flush (= `writeProgramUbo` 経路)
5. **shader 接続**:
   - 実 shader `class1/deferred/shadowAlphaMaskV.glsl:85 ifdef LL_VULKAN_GLSL block` UBO declaration 既存 = 追加 shader 改変なし
   - 既存 OpenGL `#else` block uniform 個別宣言は温存
6. **tail pad 維持**: `_pad_sam0/1/2` (12 B) は std140 padding (16 B 完成のため)
7. **関連 shadow UBO との重複整理**:
   - pbrShadowAlphaMaskV.glsl / avatarAlphaShadowV.glsl の関連 UBO (= PbrShadowAlphaMaskVParamUBO_Legacy / AvatarAlphaShadowVParamUBO_Legacy) との data source 共通化整理 (= verify 要)

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 binding=6 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**7 site setter で program 切替を毎回実施**: shadow rendering 経路で program を多回切替、その都度 setter (= 7 site)。Vulkan 化後は 7 site 全てで UBO write 必要 (= cadence 上 PerProgram は妥当)。

**関連 shadow UBO 重複**: 同 `shadow_target_width` を `PbrShadowAlphaMaskVParamUBO_Legacy` (set=3 binding=21) / `AvatarAlphaShadowVParamUBO_Legacy` (set=3 binding=22) でも使用と推定 (= verify 要)、別 UBO 別 set で重複 data を持つか確認要。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 = PerDraw + PerProgram 混在

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- ubo_metadata.inl で 73 件の最大 cluster

### §11.3 同 shader consume UBO (= class1/deferred/shadowAlphaMaskV.glsl)

- **不明 / verify 要** = shadowAlphaMaskV.glsl 内で他に consume される UBO (= FrameViewProj / shadow view-projection 等、grep verify 要)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **PbrShadowAlphaMaskVParamUBO_Legacy** (= set=3 binding=21、同 shadow alpha mask PBR 版、別 program 別 UBO、`shadow_target_width` 共有候補)
- **AvatarAlphaShadowVParamUBO_Legacy** (= set=3 binding=22、avatar shadow alpha 版、`shadow_target_width` 共有候補)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- shadow target resize 時、関連 shadow UBO 全部 (= PbrShadowAlpha / AvatarAlphaShadow) と同時 dirty 候補

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **関連 shadow UBO (= PbrShadowAlphaMask / AvatarAlphaShadow) との重複整理** = 別 UBO 別 set で同 `shadow_target_width` 持つか、Vulkan 化時の整理 (= verify 要)
2. **7 site setter (pipeline.cpp 4+3) の trigger 経路** = どの program rebind で発火するか、program 切替の都度か (= verify 要)
3. **tail pad 12 B の将来 member 追加意図** = 予約 slot か、std140 padding のみか (= verify 要)
4. **同 shader file 内同時 consume UBO 一覧** = shadowAlphaMaskV.glsl 内全 UBO declaration grep 要
5. **shell 通電 commit** = 未来作業
6. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = verify 要
7. **target_width 値の source** = `LLPipeline` 内 shadow rendering target の width 取得経路 (= verify 要、pipeline.cpp:8562 周辺の `target_width` 変数源)

= 上記 7 項目は本 UBO file 完成時に逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.3 同期)

**Layer**: L4-3 (= C 判定 shadow_target_width triple-write group)
**status**: **起案済** (= 2026-06-06 C-6、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.3` (= single source of truth)
**要点**: shadow_target_width 3 UBO triple-write (= ShadowAlphaMaskV + PbrShadowAlphaMaskV + AvatarAlphaShadowV)、本 UBO offset=0 (1 active member + 3 pad)、non-PBR shadow alpha program 専用、setter 7 site 全特定済 (`pipeline.cpp:8562/8570/8584/8592/12596/12611/12642`)、`sCurBoundShaderPtr` 経由ゆえ 1:1 redirect、tail pad 12B 将来 member 追加意図 [要 verify]、工数 S-M、AYA live verify (= shadow alpha mask 描画、visual regression ゼロ §5.4)
**関連**: L0-1 dispatch (= shadowAlphaMaskV program 識別) / L3-19 ShadowUtilParamUBO_Legacy (= shadow render 全 program 共有 pattern) / §3.5.4 ShadowCubeV (= shadow cube target_width 関係 verify) / §5.4 visual regression policy


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

