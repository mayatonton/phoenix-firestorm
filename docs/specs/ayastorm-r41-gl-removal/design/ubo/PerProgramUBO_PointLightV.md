# PerProgramUBO_PointLightV — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `per_program_ubo_point_light_v.glsl` は実 shader `class3/deferred/pointLightV.glsl:63 ifdef LL_VULKAN_GLSL block` から literal extract 済 (= V/F cross-stage 共有 binding=5)。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_PointLightV`
- **block_hash**: `0xebfee557u` (= FNV-1a("PerProgramUBO_PointLightV"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 2
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_pointlightv.inl
struct PerProgramUBO_PointLightVLayout {
    static constexpr std::uint32_t center_OFFSET = 0u;  // size=12 align=16
    static constexpr std::uint32_t size_OFFSET = 12u;  // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_PointLightV_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_point_light_v.glsl
layout(std140, set = 2, binding = 5) uniform PerProgramUBO_PointLightV
{
    vec3  center;
    float size;
};
```

= 全 2 member 実 data slot 確定 (= blueprint comment literal: `Source: literal extract from class3/deferred/pointLightV.glsl:63 ifdef LL_VULKAN_GLSL block` + `(verified identical across 2 sample sites = pointLightV / spotLightF [cross-stage V+F shared])`)。**特記**: blueprint comment literal `spotLightF.glsl:151 で declared-but-unused (= host bind は同 binding 共有、frag は別 UBO 経由で size 参照、η-28-C type 3 範式)`。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 5
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通)
- **pipeline layout**: `sAYAStandardLayout`
- **set 2 配置**: PerDraw + PerProgram 帯
- **source**: `ubo_metadata.inl:83` `{ "PerProgramUBO_PointLightV", 0xebfee557u, 256u, 2u, 5u, 0u, 1u, 2u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush、program 単位で値を保持。Point light V stage で center / size を変換に使用
- **source**: ubo_metadata.inl:83 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source**:
  - `center` = light position (= `pipeline.cpp:11486` `gDeferredLightProgram.uniform3fv(LLShaderMgr::LIGHT_CENTER, 1, c)`)
  - `size` = light influence radius (= `pipeline.cpp:11487` `gDeferredLightProgram.uniform1f(LLShaderMgr::LIGHT_SIZE, s)`)
- **lifetime**: 描画 light 単位 (= per-light で center/size が変化、program 内で複数 dispatch)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_point_light_v.glsl`
- **実 shader use site**: 
  - **`class3/deferred/pointLightV.glsl:63 ifdef LL_VULKAN_GLSL block`** (= primary site)
  - 既存 UBO block (pointLightV.glsl:64-65):
    ```glsl
    vec3  center;
    float size;
    ```
  - 既存 OpenGL `#else` block (pointLightV.glsl:69-70):
    ```glsl
    uniform vec3 center;
    uniform float size;
    ```
  - 使用箇所: `pointLightV.glsl:91` (`vec3 p = position*size+center`) + `:94` (`trans_center = (modelview_matrix*vec4(center.xyz, 1.0)).xyz`)
  - shader comment literal `pointLightV.glsl:54-55`: `center / size bare uniform を PerProgramUBO_PointLightV (set=2, binding=5) に集約。main() で position*size+center として`
- **同 binding 共有 site** (= cross-stage 共有):
  - **`class3/deferred/spotLightF.glsl:151`** (= declared-but-unused、blueprint comment literal `spotLightF.glsl:151 で declared-but-unused (= host bind は同 binding 共有、frag は別 UBO 経由で size 参照、η-28-C type 3 範式)`)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **既存 OpenGL 経路 setter**:
  - `llshadermgr.cpp:1630-1631` `mReservedUniforms.push_back("center" / "size")` (= reserved uniform 登録)
  - `llshadermgr.h:154-155` `LIGHT_CENTER, // "center"` + `LIGHT_SIZE, // "size"`
  - 実 setter (= 3 site):
    - **`pipeline.cpp:11486-11487`** `gDeferredLightProgram.uniform3fv(LLShaderMgr::LIGHT_CENTER, 1, c)` + `.uniform1f(LLShaderMgr::LIGHT_SIZE, s)` (= point light)
    - **`pipeline.cpp:11548-11549`** `gDeferredSpotLightProgram.uniform3fv(LLShaderMgr::LIGHT_CENTER, 1, c)` + `.uniform1f(LLShaderMgr::LIGHT_SIZE, s)` (= spot light)
    - **`pipeline.cpp:11625-11626`** `gDeferredMultiSpotLightProgram.uniform3fv(LLShaderMgr::LIGHT_CENTER, 1, glm::value_ptr(tc))` + `.uniform1f(LLShaderMgr::LIGHT_SIZE, light_size_final)` (= multi spot)
- **本実装化後 setter**:
  - 31 setter 経路 (= mUseUBO 分岐) で UBO 化対応要

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **通電内容**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step + η-25 + η-28 範囲、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電**: blueprint ベースで host 側 buffer 配置 + descriptor set 配線
2. **実 member data 流入**:
   - `pipeline.cpp:11486-11487` (point light)、`:11548-11549` (spot light)、`:11625-11626` (multi spot) の setter を UBO 化
3. **dirty 判定 logic 追加**:
   - PerProgram cadence + per-light 切替時 dirty (= light 単位で center/size 変化、追加 trigger 要)
4. **flush logic 追加**: PerProgram cadence flush (= `writeProgramUbo` 経路)
5. **shader 接続**:
   - 実 shader `class3/deferred/pointLightV.glsl:63 ifdef LL_VULKAN_GLSL block` UBO declaration 既存 = 追加 shader 改変なし
   - 既存 OpenGL `#else` block uniform 個別宣言は温存
6. **cross-stage 共有 binding 整理**:
   - spotLightF.glsl:151 declared-but-unused 状態を維持 (= η-28-C type 3 範式、host bind は同 binding 共有、frag は別 UBO 経由で size 参照)
   - Vulkan path で同 binding を多 program 間共有する設計の確認 (= verify 要)

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 binding=5 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**per-light dirty trigger risk**: PerProgram cadence で対応するには、light volume bind の都度 UBO write を `writeProgramUbo` 経路で実施する必要、設計上想定されているか確認要 (= verify 要、Phase 2 で per-draw cadence 移行検討)。

**η-28-C type 3 範式 (declared-but-unused 共有 binding)**: spotLightF が binding=5 を declared-but-unused (= host bind は本 UBO で済む、frag stage は別 UBO 経由) する設計。SPIR-V validation 上 declared-but-unused は warning 候補、Phase 2 で必要に応じ衝突調整 (= verify 要)。

**3 site (point/spot/multi-spot) 共通利用**: 同 LIGHT_CENTER / LIGHT_SIZE を 3 program で setter、本 UBO はそれぞれの program で binding。各 program で値が異なるため、program 切替時 dirty が cadence 上正しい。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 = PerDraw + PerProgram 混在

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- ubo_metadata.inl で 73 件の最大 cluster

### §11.3 同 shader consume UBO (= class3/deferred/pointLightV.glsl)

- **PerProgramUBO_PointLightF** (= set=2 binding=25、pair F stage)
- **不明 / verify 要**: FrameViewProj (= modelview_matrix 使用、:94)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **PerProgramUBO_SpotLightF** (= set=2 binding=10、`proj_origin` 等で同 light source 由来の他 member 持つ)
- **PerProgramUBO_ShadowCubeV** (= set=2 binding=14、同 `vec3 + float` 構造で似た shape、別 binding 別用途、verify 要)
- **PerDrawUBO_MultiLight** (= set=2 binding=1、multi-spot setter pipeline.cpp:11625-11626 と同 setter 経由)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- per-light 切替時、関連 light F UBO (= PerProgramUBO_PointLightF / SpotLightF) と同時 dirty 候補

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時に同時 bind
- cross-stage 共有: 同 binding を point/spot 両 program で reuse (= η-28-C type 3)

---

## §10. 不明事項

1. **η-28-C type 3 範式の Vulkan 設計詳細** = 同 binding を多 program で reuse する設計 doc 参照要 (= verify 要)
2. **per-light dirty trigger 経路** = PerProgram cadence で light 切替の都度 UBO write をする経路 (= verify 要)
3. **spotLightF.glsl:151 declared-but-unused SPIR-V 影響** = validation warning 発生有無 (= Phase 2 cold launch 時確認)
4. **PerProgramUBO_PointLightF (binding=25) との pair binding 順序** = V/F pair で同 program 内で binding (= verify 要)
5. **shell 通電 commit** = 未来作業
6. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = verify 要
7. **multi-spot (pipeline.cpp:11625) 経路の cadence** = MultiSpot は本 UBO 使用 or PerDrawUBO_MultiLight 使用、どちらか (= verify 要)

= 上記 7 項目は本 UBO file 完成時に逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.2 同期)

**Layer**: L4-2 sub-cluster (c) (= light cvar 3 UBO V/F pair + cross-stage)
**status**: **起案済** (= 2026-06-06 C-6、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.2` (= single source of truth)
**要点**: light cvar 3 UBO (= PointLightF + SpotLightF + PointLightV)、本 UBO `center`/`size` (offset=0/12) per-light、PointLightF V/F pair + SpotLightF cross-stage declared-but-unused 共有 (η-28-C type 3、host bind 1 instance/frag 別 UBO 経由 size 参照)、per-light dirty PerDraw 降格候補 [要 L0-4 結果反映]、multi-spot (pipeline.cpp:11625) は §3.5.16 MultiLight に分岐、SPIR-V validation 0 warning verify [要 Phase 2 cold launch]、工数 L (group 全体)
**関連**: L0-1 dispatch (= 同 binding=5 多 program reuse) / L0-4 cadence 再評価 (= per-light PerDraw 降格) / PointLightF (= V/F pair) / SpotLightF (= cross-stage declared-but-unused) / §3.5.16 MultiLight (= multi-spot 分岐) / §5.4 visual regression policy


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

