# RlvFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= Phase 1.C shell 配置されていない、Phase 2 で shell → 実 member + 実 dirty + 実 flush 全配線対象)

**本実装化に必要な作業**: blueprint `rlv_f_param_ubo_legacy.glsl` は実 shader `class1/deferred/rlvF.glsl ifdef LL_VULKAN_GLSL block` から literal extract 済。Phase 2 で host C++ 側に setter 配線 + dirty 判定 + per-program flush logic 追加。**RLVa (RestrainedLove API) Sphere effect 関連 UBO** (= memory `project_ayastorm_rlv_user_base` ユーザー層に RLV ヘビーが含まれる前提、機能維持必須)。

---

## §1. UBO identity

- **block_name**: `RlvFParamUBO_Legacy`
- **block_hash**: `0x8bfd442eu` (= FNV-1a("RlvFParamUBO_Legacy"))
- **block_size**: 256 B (= std140 80 B、device-padded 256 B)
- **member_count**: 9
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_rlvfparamubo_legacy.inl
struct RlvFParamUBO_LegacyLayout {
    static constexpr std::uint32_t rlvEffectParam1_OFFSET = 0u;  // size=16 align=16
    static constexpr std::uint32_t rlvEffectParam2_OFFSET = 16u;  // size=16 align=16
    static constexpr std::uint32_t rlvEffectParam4_OFFSET = 32u;  // size=16 align=16
    static constexpr std::uint32_t rlvEffectParam5_OFFSET = 48u;  // size=8 align=8
    static constexpr std::uint32_t rlvEffectParam3_uvec_OFFSET = 56u;  // size=8 align=8
    static constexpr std::uint32_t rlvEffectMode_OFFSET = 64u;  // size=4 align=4
    static constexpr std::uint32_t _pad_rlv_legacy_0_OFFSET = 68u;  // size=4 align=4
    static constexpr std::uint32_t _pad_rlv_legacy_1_OFFSET = 72u;  // size=4 align=4
    static constexpr std::uint32_t _pad_rlv_legacy_2_OFFSET = 76u;  // size=4 align=4
};
inline constexpr std::uint32_t RlvFParamUBO_Legacy_SIZE = 256u; // std140=80, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/rlv_f_param_ubo_legacy.glsl
layout(std140, set = 3, binding = 56) uniform RlvFParamUBO_Legacy
{
    vec4  rlvEffectParam1;            // 0-15  Sphere origin (in local coordinates)
    vec4  rlvEffectParam2;            // 16-31 Min/max dist + min/max value
    vec4  rlvEffectParam4;            // 32-47 Sphere params (=color when using blend)
    vec2  rlvEffectParam5;            // 48-55 Blur direction (not used for blend)
    uvec2 rlvEffectParam3_uvec;       // 56-63 Min/max dist extend (bvec2 → uvec2 promote)
    int   rlvEffectMode;              // 64-67 ESphereMode
    int   _pad_rlv_legacy_0;          // 68-71
    int   _pad_rlv_legacy_1;          // 72-75
    int   _pad_rlv_legacy_2;          // 76-79
};
```

= 実 data 6 member (rlvEffectParam1/2/4/5 + rlvEffectParam3_uvec + rlvEffectMode) + std140 padding 3 member。**RLVa Sphere effect** (= 視界 sphere 演出、blur/blend/color overlay) パラメータ集約。

**注記**: blueprint comment literal `bvec2 → uvec2 promote` = std140 で bvec2 が許可されないため uvec2 promote (= η-7 phase 1 範式)、shader main() body 内は `SPHERE_DISTEXTEND` #define alias で参照不変保持。

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 56
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定、verify 要)
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:98` `{ "RlvFParamUBO_Legacy", 0x8bfd442eu, 256u, 3u, 56u, 0u, 1u, 9u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: program 切替時に flush、RLVa Sphere effect 有効時のみ意味あり (= 通常 program では 0 値で良い)
- **source**: ubo_metadata.inl:98 + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **shell 段階**: 未通電
- **本実装化後の data source 候補** (= **不明 / verify 要**):
  - **`RlvHandler` / `RlvActions`** 内 sphere effect state (= RLVa core、`indra/newview/rlv*.h/cpp` 配下、grep verify 要)
  - RLVa Sphere mode (= ESphereMode enum、blueprint comment literal)
  - sphere origin (local coord) / dist params / blur direction / mode 等の集約 state
- **lifetime**: program 単位 (= sphere effect 対象 shader bind 中)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/rlv_f_param_ubo_legacy.glsl`
- **実 shader use site**:
  - **`class1/deferred/rlvF.glsl:62 ifdef LL_VULKAN_GLSL block`** (= blueprint comment literal、η-7 phase 1 RLVa Sphere bare uniforms UBO wrap)
- **使用 uniform**: rlvEffectParam1/2/4/5 (vec4/vec2) / rlvEffectParam3_uvec (uvec2 = bvec2 promote) / rlvEffectMode (int / ESphereMode)
- **shader main() body**: `SPHERE_DISTEXTEND` #define alias で uvec2 → bvec2 cast 参照、main 内コード不変

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: なし
- **本実装化後 setter** (= **不明 / verify 要**):
  - `RlvHandler` / `RlvActions` 内 sphere effect uniform 書込 site (= grep verify 要、`uniform4fv(rlvEffectParam1)` / `uniform2fv` / `uniform1i(rlvEffectMode)` 等)
  - 31 setter 経路で UBO 化対応要

---

## §7. 現状通電状態

- **状態**: **untouched**
- **通電 commit**: なし
- **blueprint 配置 commit**: 不明 (= Phase 1.A PA-8 sub-step 範囲、η-7 phase 1 RLVa Sphere bare uniforms UBO wrap、verify 要)

---

## §8. 本実装化に必要な作業

1. **shell 通電** + dummy buffer write + bind 経路通電
2. **実 member data 流入**: `RlvHandler` / `RlvActions` 内 sphere effect state を UBO 化
3. **dirty 判定 logic 追加**: PerProgram cadence + sphere effect on/off 切替時 dirty
4. **flush logic 追加**: PerProgram cadence flush
5. **shader 接続**: 実 shader 既存 UBO declaration + `SPHERE_DISTEXTEND` #define alias 使用 = 追加 shader 改変なし
6. **RLVa 機能維持確認**: memory `project_ayastorm_rlv_user_base` 遵守 = RLV ヘビーユーザー対象 sphere effect 必須維持

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=3 binding=56 配置 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ 80B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ blueprint extracted from existing shader (= `#define SPHERE_DISTEXTEND` alias で main 不変) |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**bvec2 → uvec2 promote risk**: std140 制約で bvec2 直接使えず uvec2 promote、shader main() 内 cast (= `bvec2(rlvEffectParam3_uvec != uvec2(0))` 等) が host 側書込と整合する必要、verify 要。

**RLVa 機能後退 risk**: memory `project_ayastorm_rlv_user_base` で RLV ヘビーユーザー前提、sphere effect は重要機能 = UBO 化で挙動変化 (= dirty/flush 経路の latency / 値反映タイミング) ないか実機 verify 必須。

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)
- set=3 帯 Legacy 群

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)
- 73 件 (推定) cluster

### §11.3 同 shader consume UBO (= class1/deferred/rlvF.glsl)
- **不明 / verify 要** = rlvF.glsl 内同時 consume UBO (= FrameViewProj + sampler binding 等可能性、verify 要)

### §11.4 同 data source UBO
- **不明 / verify 要** = RlvHandler 由来の他 UBO 確認 (= 単独可能性大)

### §11.5 dirty 連動 UBO
- **不明 / verify 要** = sphere effect on/off 切替時の同時 dirty UBO

### §11.6 layout 共有関係
- 全 program 共通 `sAYAStandardLayout`

### §11.7 bind 順序関係
- PerProgram cadence ゆえ program bind 時に同時 bind

---

## §10. 不明事項

1. **既存 OpenGL 経路 setter call site** = `RlvHandler` / `RlvActions` 内 sphere effect uniform 書込 site (= grep verify 要)
2. **ESphereMode enum 定義** = sphere effect mode の取り得る値 (= `indra/newview/rlv*.h` 内 verify 要)
3. **sphere effect on/off 判定** = RLVa runtime state からの dirty trigger (= verify 要)
4. **`rlvEffectParam3_uvec` bvec2 cast 整合** = shader main 内 cast pattern と host 書込値の整合 verify 要
5. **shell 通電 commit** = 未来作業
6. **rlvF.glsl が consume される対象 shader chain** = post-process pass 内位置 verify 要 (= deferred lighting の前 or 後)
7. **RLVa 機能 default OFF / ON** = AYAstorm における RLVa default 値 (= memory `project_ayastorm_rlv_user_base` で AYA 自身は不使用とあるが viewer 側 default OFF か ON か verify 要)

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.4.18 同期)

**Layer**: L3-18 (= B Tier β setter 推定済、PerProgram cadence、RLVa Sphere effect)
**status**: **起案済** (= 2026-06-06 C-5、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.4.18` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch + L0-4 cadence
- **(2) 不明事項**: `RlvHandler` / `RlvActions` 内 sphere effect uniform 書込 site [要追加調査] / `ESphereMode` enum 定義 [要 verify] / `rlvEffectParam3_uvec` bvec2→uvec2 promote cast 整合 [要 verify] / RLV 機能維持 (memory `project_ayastorm_rlv_user_base`) **[要 AYA 判断 = 機能維持必須]**
- **(3) 調査手法**: D1 (`rlvEffectParam1/2/4/5` / `rlvEffectMode` setter grep + `RlvHandler` / `RlvActions` 内 sphere effect 関連 grep) + D4 (uvec2 promote cast 整合)
- **(4) 設計 task**: L3 全件共通 (= PerProgram triple-buffer / `forwardToUboUpload` PER_PROGRAM / program bind 単位 flush / `rlvF.glsl` LL_VULKAN_GLSL 活性化)
- **(5) 工程**: trace L3-18、工数 **M** (= RLVa core 調査 + uvec2 promote verify)、AYAstorm RLV ユーザー機能維持必須
- **(6) A 確定**: setter 通電 + Vulkan 0 + AYA live verify (= RLVa Sphere effect (blur/blend/color overlay) 既存と同一、RLV ヘビーユーザー機能維持、visual regression ゼロ §5.4)
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `rlvF.glsl #else` block uniform 個別宣言維持

**関連**: L0-1 + L0-4 / §5.4 / `RlvHandler` / `RlvActions` (= setter 経路) / memory `project_ayastorm_rlv_user_base`


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

