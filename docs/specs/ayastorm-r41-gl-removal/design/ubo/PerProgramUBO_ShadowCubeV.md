# PerProgramUBO_ShadowCubeV — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `per_program_ubo_shadow_cube_v.glsl` は実 shader `class1/deferred/shadowCubeV.glsl:57 ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加 + 実 shader binding 接続。

---

## §1. UBO identity

- **block_name**: `PerProgramUBO_ShadowCubeV`
- **block_hash**: `0x62d19bfeu` (= FNV-1a("PerProgramUBO_ShadowCubeV"))
- **block_size**: 256 B (= std140 32 B、device-padded 256 B)
- **member_count**: 4 (= 2 active vec3 + 2 pad)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perprogramubo_shadowcubev.inl
struct PerProgramUBO_ShadowCubeVLayout {
    static constexpr std::uint32_t box_center_OFFSET = 0u;  // size=12 align=16
    static constexpr std::uint32_t _pad_shadowcube0_OFFSET = 12u;  // size=4 align=4
    static constexpr std::uint32_t box_size_OFFSET = 16u;  // size=12 align=16
    static constexpr std::uint32_t _pad_shadowcube1_OFFSET = 28u;  // size=4 align=4
};
inline constexpr std::uint32_t PerProgramUBO_ShadowCubeV_SIZE = 256u; // std140=32, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_shadow_cube_v.glsl
layout(std140, set = 2, binding = 14) uniform PerProgramUBO_ShadowCubeV
{
    vec3  box_center;
    float _pad_shadowcube0;
    vec3  box_size;
    float _pad_shadowcube1;
};
```

= 全 4 member (= 2 active vec3 + 2 trailing float pad) 実 data slot 確定 (= blueprint comment literal: `Source: literal extract from class1/deferred/shadowCubeV.glsl:57 ifdef LL_VULKAN_GLSL block (single site)`)。各 vec3 が 12 B + 4 B pad で 16 B 整列 (= std140 標準)。

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 14
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、全 UBO 共通)
- **pipeline layout**: `sAYAStandardLayout`
- **set 2 配置**: PerDraw + PerProgram 帯
- **source**: `ubo_metadata.inl:88` `{ "PerProgramUBO_ShadowCubeV", 0x62d19bfeu, 256u, 2u, 14u, 0u, 1u, 4u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush。Shadow cube V stage で box-shaped occluder の center / size を保持
- **source**: ubo_metadata.inl:88 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source** (= **不明 / verify 要**):
  - `box_center` / `box_size` = shadow occlusion 用 cube bounding box (= occlusion query / shadow cube map dispatch)
  - 直接 setter call site は pipeline.cpp 内に grep で直接見えず (= `BOX_CENTER` / `BOX_SIZE` enum は `llshadermgr.h:157-158` 定義済だが、pipeline.cpp に setter site 不在)
  - 候補: `LLDrawable` shadow box / `LLViewerOctree` occlusion box (= verify 要)
- **lifetime**: program 単位 (= shadow cube render 中の per-program 値)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_program_ubo_shadow_cube_v.glsl`
- **実 shader use site**: **`class1/deferred/shadowCubeV.glsl:57 ifdef LL_VULKAN_GLSL block`** (= single site)
  - 既存 UBO block (shadowCubeV.glsl:58, 60):
    ```glsl
    vec3  box_center;
    vec3  box_size;
    ```
  - 既存 OpenGL `#else` block (shadowCubeV.glsl:65-66):
    ```glsl
    uniform vec3 box_center;
    uniform vec3 box_size;
    ```
  - 使用箇所: `shadowCubeV.glsl:72` (`vec3 p = position*box_size+box_center`)
- **同 setter 利用 shader** (= 同 BOX_CENTER / BOX_SIZE enum 使用):
  - `class1/interface/occlusionCubeV.glsl` (= 同 box uniform 使用、別 UBO `OcclusionCubeVParamUBO_Legacy` (set=3 binding=50) で対応の可能性、verify 要)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **既存 OpenGL 経路 setter**:
  - `llshadermgr.h:157-158` `BOX_CENTER, // "box_center"` + `BOX_SIZE, // "box_size"` (= reserved uniform enum)
  - 実 setter call site = **pipeline.cpp 内に直接見えず** (= grep 結果から `BOX_CENTER` / `BOX_SIZE` の uniform3fv 等が pipeline.cpp に直接無し)
  - **要 verify**: shadowCube 関連 setter は `LLPipeline` / `LLViewerOctree` / `LLDrawable` 内、grep 拡大要
- **本実装化後 setter**:
  - 31 setter 経路 (= mUseUBO 分岐) で UBO 化対応要、setter 完全特定後 UBO write 経由に差替

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
   - `BOX_CENTER` / `BOX_SIZE` 直接 setter site 特定 (= **要 verify**、grep 拡大要) → UBO 化
3. **dirty 判定 logic 追加**:
   - PerProgram cadence + shadow cube box 変化時 dirty (= occlusion query / shadow render trigger)
4. **flush logic 追加**: PerProgram cadence flush (= `writeProgramUbo` 経路)
5. **shader 接続**:
   - 実 shader `class1/deferred/shadowCubeV.glsl:57 ifdef LL_VULKAN_GLSL block` UBO declaration 既存 = 追加 shader 改変なし
   - 既存 OpenGL `#else` block uniform 個別宣言は温存
6. **関連 occlusion UBO との重複整理**:
   - `OcclusionCubeVParamUBO_Legacy` (set=3 binding=50) との同 box data 重複確認 (= verify 要)

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 binding=14 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 32B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**setter site 不明 risk**: `BOX_CENTER` / `BOX_SIZE` 直接 setter が pipeline.cpp に grep で見えず、別 file (= `LLDrawable` / `LLViewerOctree` / `llselectmgr.cpp` 等) にある可能性 = grep 拡大要。setter 完全特定が Phase 2 着手前提。

**OcclusionCubeVParamUBO_Legacy との重複**: 同 box_center / box_size を別 UBO で持つ可能性、Vulkan 化時の整理 (= verify 要)。

**`PerProgramUBO_PointLightV` (binding=5) との shape 類似**: 両者とも `vec3 + float` 構造、用途は別 (shadow vs light position)、誤 bind 防止に layout 区別の design pattern 要 (= verify 要)。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2)

- set=2 帯 = PerDraw + PerProgram 混在

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- ubo_metadata.inl で 73 件の最大 cluster

### §11.3 同 shader consume UBO (= class1/deferred/shadowCubeV.glsl)

- **不明 / verify 要** = shadowCubeV.glsl 内で他に consume される UBO (= FrameViewProj / shadow view-projection 等、grep verify 要)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **OcclusionCubeVParamUBO_Legacy** (= set=3 binding=50、occlusionCubeV.glsl 用、同 `BOX_CENTER` / `BOX_SIZE` enum 由来、別 program 別 UBO)
- **PerProgramUBO_PointLightV** (= set=2 binding=5、shape 類似だが別 data source = light center/size)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- shadow cube box 変化時、関連 occlusion box UBO (= OcclusionCubeVParamUBO_Legacy) と同時 dirty 候補

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係

- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **`BOX_CENTER` / `BOX_SIZE` 直接 setter call site** = pipeline.cpp に grep で見えず、別 file (= LLDrawable / LLViewerOctree / llselectmgr.cpp 等、grep 拡大要)
2. **box data source の owner class** = shadow cube box の owner (= LLPipeline / LLDrawable / occlusion query 経路、verify 要)
3. **`OcclusionCubeVParamUBO_Legacy` との重複/関係性** = 別 UBO 別 set で同 box data を duplicate 持つか、片方 program 別 (= verify 要)
4. **同 shader file 内同時 consume UBO 一覧** = shadowCubeV.glsl 内全 UBO declaration grep 要
5. **shell 通電 commit** = 未来作業
6. **PerProgram flush 経路の VkDescriptorBufferInfo bind 詳細** = verify 要
7. **`PerProgramUBO_PointLightV` (binding=5) との shape 類似の意図** = 同 layout 共有設計か、独立設計か (= verify 要)

= 上記 7 項目は本 UBO file 完成時に逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.4 同期)

**Layer**: L4-4 (= C 判定 box_center/box_size 2 UBO program 識別 dispatch group)
**status**: **起案済** (= 2026-06-06 C-6、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.4` (= single source of truth)
**要点**: box_center/box_size 2 UBO program 識別 dispatch (= Occlusion + ShadowCube)、本 UBO shadowCubeV program 専用 (set=2 binding=14)、setter site 全件 [要追加調査] (= `LLPipeline::generateSunShadow` cube 経路候補)、PointLightV (binding=5) と shape 類似誤 bind 防止、cadence PerProgram 妥当性 verify、工数 M、AYA live verify (= shadow cube map 描画、visual regression ゼロ §5.4)
**関連**: L0-1 dispatch (= 同 enum で別 program 別 UBO 識別) / OcclusionCube (= 同 enum cross-UBO) / §3.5.2 PointLightV (= shape 類似誤 bind 防止) / §3.5.3 shadow_target_width (= shadow render 連動 verify) / §5.4 visual regression policy


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

