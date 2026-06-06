# PerProgramUBO_PostDeferredNoDoFF — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `per_program_ubo_post_deferred_no_dof_f.glsl` は実 shader `class1/deferred/postDeferredNoDoFF.glsl:81 ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_PostDeferredNoDoFF`
- **block_hash**: `0xdefa2506u` (= FNV-1a("PerProgramUBO_PostDeferredNoDoFF"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 4 (= 1 active + 3 tail pad)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_postdeferrednodoff.inl
struct PerProgramUBO_PostDeferredNoDoFFLayout {
    static constexpr std::uint32_t chroma_str_OFFSET = 0u;  // size=4 align=4
    static constexpr std::uint32_t _pad_nodof0_OFFSET = 4u;  // size=4 align=4
    static constexpr std::uint32_t _pad_nodof1_OFFSET = 8u;  // size=4 align=4
    static constexpr std::uint32_t _pad_nodof2_OFFSET = 12u;  // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_PostDeferredNoDoFF_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_post_deferred_no_dof_f.glsl
layout(std140, set = 2, binding = 12) uniform PerProgramUBO_PostDeferredNoDoFF
{
    float chroma_str;
    float _pad_nodof0;
    float _pad_nodof1;
    float _pad_nodof2;
};
```

= active member 1 個 (= `chroma_str` 4 B) + tail pad 12 B。blueprint comment literal: `Source: literal extract from class1/deferred/postDeferredNoDoFF.glsl:81 ifdef LL_VULKAN_GLSL block (single site)`。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 12
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通)
- **pipeline layout**: `sAYAStandardLayout`
- **set 2 配置**: PerDraw + PerProgram 帯
- **source**: `ubo_metadata.inl:85` `{ "PerProgramUBO_PostDeferredNoDoFF", 0xdefa2506u, 256u, 2u, 12u, 0u, 1u, 4u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush。DoF 無し post-deferred F stage で chroma aberration strength を保持
- **source**: ubo_metadata.inl:85 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source**:
  - `chroma_str` = `RenderChromaStrength` cvar (= `pipeline.cpp:9555` `const F32 nodof_chroma_str = gSavedSettings.getF32("RenderChromaStrength")` + `:9557` `gDeferredPostNoDoFProgram.uniform1f(LLShaderMgr::DEFERRED_CHROMA_STRENGTH, nodof_chroma_str)`)
- **lifetime**: program 単位

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_post_deferred_no_dof_f.glsl`
- **実 shader use site**: **`class1/deferred/postDeferredNoDoFF.glsl:81 ifdef LL_VULKAN_GLSL block`** (= single site)
  - 既存 UBO block (postDeferredNoDoFF.glsl:82):
    ```glsl
    float chroma_str;
    ```
  - 既存 OpenGL `#else` block (postDeferredNoDoFF.glsl:89):
    ```glsl
    uniform float chroma_str;
    ```
  - 使用箇所: `postDeferredNoDoFF.glsl:137` (`re-sample with radial shift = chroma_str * 0.0005 * r2, no`) + `:169` (`chroma_str *`) + `:218` (`chroma_str=10 → ~1% edge shift (subtle)` comment) + `:221` (`float shift = chroma_str * 0.0005 * r2`)
  - 設計意図 (postDeferredNoDoFF.glsl:211): `grows toward screen edges. chroma_str=0 → no shift (free)` (= 0 = OFF、edge-aware chromatic aberration)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **既存 OpenGL 経路 setter**:
  - **`pipeline.cpp:9552-9557`** (= `<AYAstorm r30 P4 step 4> BD chroma_str (vignette path runs when HAS_DOF_CHROMA==0)` comment literal):
    ```cpp
    const F32 nodof_chroma_str = gSavedSettings.getF32("RenderChromaStrength");
    gDeferredPostNoDoFProgram.uniform1f(LLShaderMgr::DEFERRED_CHROMA_STRENGTH, nodof_chroma_str);
    ```
  - **`pipeline.cpp:10381`** で関連 vignette path 内 (= 同 `BD chroma_str (vignette path runs when HAS_DOF_CHROMA==0)` comment)
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
   - `pipeline.cpp:9557` の `chroma_str` setter を UBO 化
3. **dirty 判定 logic 追加**:
   - PerProgram cadence + cvar `RenderChromaStrength` 変更時 dirty
4. **flush logic 追加**: PerProgram cadence flush (= `writeProgramUbo` 経路)
5. **shader 接続**:
   - 実 shader `class1/deferred/postDeferredNoDoFF.glsl:81 ifdef LL_VULKAN_GLSL block` UBO declaration 既存 = 追加 shader 改変なし
   - 既存 OpenGL `#else` block uniform 個別宣言は温存
6. **tail pad 維持**: `_pad_nodof0/1/2` (12 B) は std140 padding (16 B 完成のため)、layout 不変契約で維持

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 binding=12 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**AYAstorm r30 P4 BD 由来**: `RenderChromaStrength` は AYAstorm r30 BD 改善由来 cvar (= `<AYAstorm r30 P4 step 4>` comment literal)、upstream Firestorm に無い AYAstorm 独自。

**`PerProgramUBO_PostDeferredF` (binding=20) との重複**: 同 `chroma_str` member 持つが別 program (= DoF 有り版/無し版) で別 UBO 別 binding に分離。各 program で別 UBO instance ゆえ double-write は必須 (= cvar listener から 2 UBO 同時 dirty)。

**1 active member + 3 tail pad の意図**: tail pad 12 B は 16 B 整列のため、将来 member 追加余地 (= verify 要、PerProgramUBO_PostDeferredF と同 layout で合わせる意図の可能性)。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 = PerDraw + PerProgram 混在

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- ubo_metadata.inl で 73 件の最大 cluster

### §11.3 同 shader consume UBO (= class1/deferred/postDeferredNoDoFF.glsl)

- **不明 / verify 要** = postDeferredNoDoFF.glsl 内で他に consume される UBO (= FrameViewProj / sampler 等、grep verify 要)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **PerProgramUBO_PostDeferredF** (= set=2 binding=20、同 `chroma_str` を持つ DoF 有り版、同 `RenderChromaStrength` cvar 由来)
- **PerProgramUBO_PostDeferredV** (= set=2 binding=7、本 NoDoFF の V stage pair の可能性、verify 要)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- `RenderChromaStrength` 変化時、PerProgramUBO_PostDeferredF と同時 dirty 候補

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **tail pad 12 B の将来 member 追加意図** = 将来追加 member の予約 slot か、std140 padding のみか (= verify 要、Phase 2 設計判断)
2. **`PerProgramUBO_PostDeferredF` との UBO 合併可能性** = 同 cvar 由来で別 UBO に分離する必要があったか (= verify 要、historical 整理)
3. **postDeferredNoDoFF program ↔ postDeferredF program の切替条件** = HAS_DOF_CHROMA permutation での program 切替 (= verify 要)
4. **同 shader file 内同時 consume UBO 一覧** = postDeferredNoDoFF.glsl 内全 UBO declaration grep 要
5. **shell 通電 commit** = 未来作業
6. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = verify 要
7. **vignette path (`pipeline.cpp:10381`) の setter 経路** = `chroma_str` を vignette 経路でも write しているか (= verify 要)

= 上記 7 項目は本 UBO file 完成時に逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.2 同期)

**Layer**: L4-2 sub-cluster (b) (= chroma_str 2 UBO cross-write)
**status**: **起案済** (= 2026-06-06 C-6、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.2` (= single source of truth)
**要点**: chroma_str 2 UBO cross-write (= PostDeferredF + NoDoFF)、本 UBO `chroma_str` offset=0 (1 active member + 3 pad)、AYAstorm r30 P4 BD 改善 (RenderChromaStrength cvar)、HAS_DOF_CHROMA==0 経路 (vignette path 含)、vignette path 別 setter `pipeline.cpp:10381` [要追加調査]、工数 L (group 全体)
**関連**: L0-1 dispatch (= postDeferredNoDoFF program 識別 + HAS_DOF_CHROMA permutation 切替) / §5.4 visual regression policy


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

