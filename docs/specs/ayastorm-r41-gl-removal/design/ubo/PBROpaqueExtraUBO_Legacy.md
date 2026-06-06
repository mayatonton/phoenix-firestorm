# PBROpaqueExtraUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= blueprint + codegen metadata 生成済、host C++ register/write/flush 経路未着工)

**本実装化に必要な作業**: register/write/flush 経路新設 (= per-program cadence、`aya_sss_skin_flag` setter を UBO write に redirect) + r20 Phase C SSS skin flag dispatcher 棚卸し

---

## §1. UBO identity

- **block_name**: `PBROpaqueExtraUBO_Legacy`
- **block_hash**: `0x10b1c904u` (= FNV-1a("PBROpaqueExtraUBO_Legacy"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 4 (= 1 float + 3 pad)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_pbropaqueextraubo_legacy.inl:12-17
struct PBROpaqueExtraUBO_LegacyLayout {
    static constexpr std::uint32_t aya_sss_skin_flag_OFFSET = 0u;       // size=4 align=4
    static constexpr std::uint32_t _pad_pbropaque_0_OFFSET = 4u;        // size=4 align=4
    static constexpr std::uint32_t _pad_pbropaque_1_OFFSET = 8u;        // size=4 align=4
    static constexpr std::uint32_t _pad_pbropaque_2_OFFSET = 12u;       // size=4 align=4
};
inline constexpr std::uint32_t PBROpaqueExtraUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/class1/deferred/pbropaqueF.glsl:160-167
// r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-5 (b-1): bare uniform → nameless UBO wrap (PBROpaqueExtraUBO_Legacy)
layout(set=3, binding=13, std140) uniform PBROpaqueExtraUBO_Legacy {
    float aya_sss_skin_flag;
    float _pad_pbropaque_0;
    float _pad_pbropaque_1;
    float _pad_pbropaque_2;
};
```

= Blueprint (= `aya_r41_blueprints/set3/pbr_opaque_extra_ubo_legacy.glsl:9-15`) 一致。**注: 起案 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-5 (b-1) で bare uniform を nameless UBO に wrap した経緯あり** (= shader header comment literal)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 13
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: 不明 / verify 要
- **source**: `ubo_metadata.inl:59` `{ "PBROpaqueExtraUBO_Legacy", 0x10b1c904u, 256u, 3u, 13u, 0u, 1u, 4u }` + `class1/deferred/pbropaqueF.glsl:162` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program** (= `llglslshader.cpp:95`)
- **意味詳細**: PBR opaque shader の SSS skin flag (= r20 Phase C `aya_sss_skin_flag`) は per-program (= material 単位だが program load 時に固定可能)
- **source**: ubo_metadata.inl + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **owner**: r20 Phase C SSS skin flag (= 推定、`indra/llrender/llshadermgr.cpp:1611` `mReservedUniforms.push_back("aya_sss_skin_flag"); // <FS:AYA r20 Phase C>` literal)
- **lifetime**: material/program 単位 (= material 切替時に dirty 化候補)
- **用途**: PBR opaque shader で SSS (subsurface scattering) skin 判定 (= memory `reference_deferred_shader_routing` + r20 SSS phase 関連)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/pbr_opaque_extra_ubo_legacy.glsl`
- **実 shader use site** = **`class1/deferred/pbropaqueF.glsl:162` 単独** (= blueprint header literal「Source: literal extract from class1/deferred/pbropaqueF.glsl:162」、grep 結果 1 file)
- 同 member 名 別 UBO 使用: **MaterialUBO_Legacy** (set=1 binding=0) も `aya_sss_skin_flag` を member に持つ (= class3/deferred/materialF.glsl:45 literal) = 同 SSS flag が 2 UBO で 重複格納されている可能性、host C++ 側 dispatch で同 data を 2 UBO に同時 write が必要かも (= verify 要)

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: 未配線
- **既存 OpenGL 経路 setter**:
  - 不明 / verify 要 (= grep で `uniform1f.*aya_sss_skin_flag` 直接 call site 未取得)
- **reserved uniform 登録**: `indra/llrender/llshadermgr.cpp:1611` `mReservedUniforms.push_back("aya_sss_skin_flag"); // <FS:AYA r20 Phase C>`

---

## §7. 現状通電状態

- **状態**: **untouched**
- **codegen 生成済**: layout `inl` + metadata entry + block_hash constexpr 生成済
- **blueprint 起案済**: `aya_r41_blueprints/set3/pbr_opaque_extra_ubo_legacy.glsl`
- **shader 宣言済**: `class1/deferred/pbropaqueF.glsl:162` `#ifdef LL_VULKAN_GLSL` block (= 起案 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-5 (b-1) で bare uniform から wrap)
- **register/write/flush 経路**: 未配線

---

## §8. 本実装化に必要な作業

1. **register 経路**: `mapUniforms()` で PBR opaque program に `registerProgramUbo(this, block_hash::PBROpaqueExtraUBO_Legacy, 256u)` 呼出
2. **write 経路**: `aya_sss_skin_flag` setter を `forwardToUboUpload` 経由 UBO write に redirect
3. **flush 経路**: cmdbuf bind 経路で sProgramUboDirty を flush
4. **shader 接続**: shader 追加改変なし
5. **MaterialUBO_Legacy と同 data 重複格納の取扱**: 同 `aya_sss_skin_flag` を 2 UBO に同時 write するか、program 識別で片方のみ write するか設計決定要

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded (1 float + 3 pad = 16B) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既に UBO 宣言済 (= 起案時 wrap で導入済) |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**特記 risk**:
- 4 member 中 1 member 実 data、残 3 member pad = 容量効率 25%、ただし設計原則 (1) 維持
- 同 `aya_sss_skin_flag` を MaterialUBO_Legacy にも格納 = 2 UBO 重複 write 必要か host C++ 側 logic 設計要
- 起案 phase で bare uniform → nameless UBO wrap した経緯 (= shader header literal) = OpenGL 経路でも UBO 経由になっていない可能性 (= bare uniform setter が host C++ 側に残存しているか verify 要)

---

## §10. 不明事項

1. **`aya_sss_skin_flag` setter call site** = grep で直接 call site 未取得
2. **MaterialUBO_Legacy との重複格納取扱** = 同 member 名 2 UBO 同時 write 必要か、program 識別で分岐か
3. **bare uniform 残存確認** = 起案 phase で UBO wrap 後も OpenGL 経路 (= `#else` block) で bare uniform が残存しているか
4. **SSS skin 判定 trigger** = `is_sss_skin` 判定経路 (= material flag / texture detect / cvar) verify 要
5. **set=3 帯 layout の binding 上限** = V3a 設計整合性

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- set=3 帯 Legacy UBO 群と共存
- 直近 binding 同居: binding=12 CASParamUBO_Legacy / binding=14 SMAAParamUBO_Legacy

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 73 件 (推定) per-program cluster 内の 1 UBO

### §11.3 同 shader consume UBO (= `class1/deferred/pbropaqueF.glsl` で同時 consume)

- 不明 / verify 要 (= pbropaqueF.glsl 全文の UBO 宣言群確認要、MaterialUBO 等 PBR 関連 UBO 同 file 宣言の可能性大)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **MaterialUBO_Legacy** (set=1 binding=0) = `aya_sss_skin_flag` 同 member 名共有 = **同 SSS flag を 2 UBO で重複格納** (= 設計上 verify 要、本実装化で重複解消するか維持するか決定要)
- **SkinSSSPrototypeFParamUBO_Legacy** (set=3 binding=30) = r20 SSS 由来候補 (= verify 要)

### §11.5 dirty 連動 UBO

- **MaterialUBO_Legacy** = 同 SSS flag dirty 時に連動 (= 重複格納ゆえ必須)
- **SkinSSSPrototypeFParamUBO_Legacy** = SSS phase trigger で連動候補 (verify 要)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- PBR opaque program 切替時 set=3 帯全 binding を一括 rebind

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.1 同期)

**Layer**: L4-1 (= C 判定 cross-UBO triple-write group)
**status**: **起案済** (= 2026-06-06 C-6、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.1` (= single source of truth)
**要点**: aya_sss_skin_flag 3 UBO triple-write group (= MaterialUBO_Legacy + PBROpaqueExtraUBO_Legacy + AvatarFParamUBO_Legacy)、本 UBO offset=0 (1 active member)、PBR opaque program 専用、wrap 起案経緯 (= η-5 (b-1)) ゆえ bare uniform 残存 verify、工数 M、setter 未取得 [要追加調査]、AYA live verify (= r20 SSS PBR opaque 描画、visual regression ゼロ §5.4)
**関連**: L0-1 dispatch (= PBR opaque program 識別) / L0-4 cadence 再評価 (= per-draw 性質 PerDraw 降格候補) / §5.4 visual regression policy / §3.5.2 SkinSSS 経由交差 verify


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

