# PerProgramUBO_VelocityAlphaV — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `per_program_ubo_velocity_alpha_v.glsl` は実 shader `class1/deferred/velocityAlphaV.glsl:60 ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_VelocityAlphaV`
- **block_hash**: `0xe42501f4u` (= FNV-1a("PerProgramUBO_VelocityAlphaV"))
- **block_size**: 256 B (= std140 64 B = mat4、device-padded 256 B)
- **member_count**: 1 (= mat4)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_velocityalphav.inl
struct PerProgramUBO_VelocityAlphaVLayout {
    static constexpr std::uint32_t last_object_matrix_OFFSET = 0u;  // size=64 align=16
};
inline constexpr std::uint32_t PerProgramUBO_VelocityAlphaV_SIZE = 256u; // std140=64, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_velocity_alpha_v.glsl
layout(std140, set = 2, binding = 19) uniform PerProgramUBO_VelocityAlphaV
{
    mat4 last_object_matrix;
};
```

= 1 member (= `last_object_matrix` mat4 64 B) 実 data slot 確定 (= blueprint comment literal: `Source: literal extract from class1/deferred/velocityAlphaV.glsl:60 ifdef LL_VULKAN_GLSL block (single site)` + `last_object_matrix mat4 を UBO 化。velocityAlphaF は plain uniform 無し、skinnedVelocityAlphaV は last_object_matrix 不使用 (skin matrix 経由)、本 V 単独 attach`)。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 19
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通)
- **pipeline layout**: `sAYAStandardLayout`
- **set 2 配置**: PerDraw + PerProgram 帯
- **source**: `ubo_metadata.inl:90` `{ "PerProgramUBO_VelocityAlphaV", 0xe42501f4u, 256u, 2u, 19u, 0u, 1u, 1u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush。Velocity alpha V stage で前 frame の object matrix を保持 (= motion blur 用 velocity vector 計算)
- **特記**: object matrix は **per-draw で変化** (= 同 shader program で複数 object 描画)、PerProgram cadence は cadence mismatch の可能性 (= verify 要、Phase 2 で per-draw cadence 移行検討)
- **source**: ubo_metadata.inl:90 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source**:
  - `last_object_matrix` = 前 frame の `LLMatrix4a` model matrix (= `lldrawpool.cpp:807` comment literal `via the LAST_OBJECT_MATRIX uniform (set by Step 3 enum), draw, then store`)
  - `lldrawpool.cpp:845` `LLGLSLShader::sCurBoundShaderPtr->uniformMatrix4fv(LLShaderMgr::LAST_OBJECT_MATRIX, 1, GL_FALSE, (GLfloat*)last_mat->mMatrix)` + `:934` 同 setter
  - `lldrawpooltree.cpp:202` 同 setter (tree pool)
  - `lldrawpoolterrain.cpp:248` 同 setter (terrain pool)
- **lifetime**: per-draw (= object 単位、program 切替伴わず)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_velocity_alpha_v.glsl`
- **実 shader use site**: **`class1/deferred/velocityAlphaV.glsl:60 ifdef LL_VULKAN_GLSL block`** (= single site)
  - 既存 UBO block (velocityAlphaV.glsl:61):
    ```glsl
    mat4 last_object_matrix;     // offset 0, size 64 (4 × vec4 配置、std140 素直)
    ```
  - 既存 OpenGL `#else` block (velocityAlphaV.glsl:65):
    ```glsl
    uniform mat4 last_object_matrix;
    ```
  - 使用箇所: `velocityAlphaV.glsl:133` (`vec4 last_pos = projection_matrix * last_modelview_matrix * last_object_matrix * vec4(position.xyz, 1.0)`)
  - shader comment literal `velocityAlphaV.glsl:54-55`: `last_object_matrix mat4 を UBO 化。velocityAlphaF は plain uniform 無し、skinnedVelocityAlphaV は last_object_matrix 不使用 (skin matrix 経由)、本 V 単独 attach`
- **同 setter 他 shader**:
  - `class1/deferred/velocityV.glsl` (= 同 LAST_OBJECT_MATRIX 使用、別 UBO `VelocityVParamUBO_Legacy` (= `ubo_metadata.inl:116` set=3 binding=55) で対応)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **既存 OpenGL 経路 setter**:
  - `llshadermgr.cpp:1876` `mReservedUniforms.push_back("last_object_matrix")` (= reserved uniform 登録)
  - `llshadermgr.h:395` `LAST_OBJECT_MATRIX, // "last_object_matrix"`
  - 実 setter (= 4 site):
    - **`lldrawpool.cpp:845`** `LLGLSLShader::sCurBoundShaderPtr->uniformMatrix4fv(LLShaderMgr::LAST_OBJECT_MATRIX, 1, GL_FALSE, (GLfloat*)last_mat->mMatrix)`
    - **`lldrawpool.cpp:934`** 同 setter (= 別 draw path)
    - **`lldrawpooltree.cpp:202`** 同 setter (tree pool)
    - **`lldrawpoolterrain.cpp:248`** 同 setter (terrain pool)
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
   - `lldrawpool.cpp:845 / 934`、`lldrawpooltree.cpp:202`、`lldrawpoolterrain.cpp:248` 4 setter site を UBO 化
3. **dirty 判定 logic 追加**:
   - PerProgram cadence + per-draw object 切替時 dirty (= per-draw で `last_object_matrix` 変化、追加 trigger 要)
   - **cadence mismatch 課題**: per-draw 単位で値が変化、PerProgram cadence では捉えきれない (= verify 要、Phase 2 で per-draw cadence 移行検討推奨)
4. **flush logic 追加**: PerProgram cadence flush (= `writeProgramUbo` 経路)
5. **shader 接続**:
   - 実 shader `class1/deferred/velocityAlphaV.glsl:60 ifdef LL_VULKAN_GLSL block` UBO declaration 既存 = 追加 shader 改変なし
   - 既存 OpenGL `#else` block uniform 個別宣言は温存
6. **関連 velocity UBO との重複整理**:
   - `VelocityVParamUBO_Legacy` (set=3 binding=55) と同 `last_object_matrix` 由来 (= velocityV.glsl 用、別 program)

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 binding=19 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 64B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**cadence mismatch 重大 risk**: `last_object_matrix` は **per-draw cadence** が本来適切。PerProgram cadence では、program 中 N object 描画時に最後の object の matrix のみ反映 = motion blur 全 object 同 last_matrix 使用 = **誤描画 / motion blur 退化** (= 重大 risk、verify 要、Phase 2 で per-draw cadence 移行は事実上必須の可能性)。

**4 setter site (drawpool / tree / terrain) cross-pool 共有**: 全 draw pool で同 setter、Vulkan 化後は 4 site 全て UBO write 経由必須。

**`VelocityVParamUBO_Legacy` との関係**: 別 UBO で同 `last_object_matrix` 由来 (= velocityV.glsl 用)、本 alpha 用と別 program 別 UBO、duplicate data 持つ可能性 (= verify 要)。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 = PerDraw + PerProgram 混在

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- ubo_metadata.inl で 73 件の最大 cluster

### §11.3 同 shader consume UBO (= class1/deferred/velocityAlphaV.glsl)

- **不明 / verify 要** = velocityAlphaV.glsl 内で他に consume される UBO (= FrameViewProj for `projection_matrix` + `last_modelview_matrix` 等、grep verify 要)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **VelocityVParamUBO_Legacy** (= set=3 binding=55、velocityV.glsl 用、同 `last_object_matrix` 由来)
- **PerDrawUBO_AvatarVelocity** (= set=2 binding=0、avatar 用 velocity、`PerDrawUBO_SkinnedVelocity` (= set=2 binding=0 別 instance) と共に skinned variant)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- per-draw object 切替時、関連 velocity UBO (= VelocityVParamUBO_Legacy / PerDrawUBO_AvatarVelocity) と同時 dirty 候補

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時に同時 bind (= 仕様上)、しかし per-draw dirty が事実上必要 (= verify 要)

---

## §10. 不明事項

1. **PerProgram cadence の per-draw mismatch 解決策** = Phase 2 で per-draw cadence 移行 (PerDrawUBO 化) するか、PerProgram 内で per-draw update 経路追加か (= **重大、verify 要**、設計判断)
2. **4 setter site (drawpool/tree/terrain) 共通整理** = 各 pool で program 別 UBO instance か、共通 UBO か (= verify 要)
3. **`VelocityVParamUBO_Legacy` との関係性** = 別 UBO 別 set で同 `last_object_matrix` を duplicate 持つか、別 path 別 program か (= verify 要)
4. **同 shader file 内同時 consume UBO 一覧** = velocityAlphaV.glsl 内全 UBO declaration grep 要
5. **shell 通電 commit** = 未来作業
6. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = verify 要
7. **`PerDrawUBO_AvatarVelocity` / `PerDrawUBO_SkinnedVelocity` との skinned variant 関係** = avatar 系 velocity UBO は別 cadence で対応済、本 UBO は非 skinned 専用か (= verify 要)

= 上記 7 項目は本 UBO file 完成時に逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.8 同期)

**Layer**: L4-8 sub-cluster (c) (= per-program velocity matrix pair 2 UBO、cadence mismatch 重大)
**status**: **起案済** (= 2026-06-06 C-6-c、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.8` (= single source of truth)
**要点**: 1 member (last_object_matrix mat4)、**cadence mismatch 重大** (= per-draw 性質を PerProgram で運ぶ、N object 描画で最後の 1 値のみ反映)、**PerProgram → PerDraw 降格必須** [要 AYA 判断 必須] [要 L0-4 結果反映]、setter 4 site cross-pool 共有 (= `lldrawpool.cpp:845/934` + `lldrawpooltree.cpp:202` + `lldrawpoolterrain.cpp:248` literal LAST_OBJECT_MATRIX `uniformMatrix4fv`)、reserved 登録 `llshadermgr.cpp:1876` + `llshadermgr.h:395` LAST_OBJECT_MATRIX enum literal 確認、VelocityV (set=3 binding=55) と同 data 別 UBO duplicate [要 verify D4 突合]、velocityAlphaV.glsl singleton site、shader comment literal「last_object_matrix mat4 を UBO 化、本 V 単独 attach」、工数 group 全体 L 内
**関連**: L0-1 dispatch (= 衝突なし binding=19、ただし set=2 内 PerDraw 帯ゆえ降格時 binding=0 共有候補) / L0-4 cadence (= PerDraw 降格必須) / §3.5.8 sub-cluster (c) VelocityV (= 同 data 別 UBO duplicate verify) / 4 setter site cross-pool (drawpool/tree/terrain)


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

