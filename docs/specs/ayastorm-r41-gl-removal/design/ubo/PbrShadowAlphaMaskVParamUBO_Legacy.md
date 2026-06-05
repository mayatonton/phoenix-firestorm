# PbrShadowAlphaMaskVParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= blueprint + codegen metadata 生成済、host C++ register/write/flush 経路未着工)

**本実装化に必要な作業**: register/write/flush 経路新設 (= per-program cadence、`LLShaderMgr::DEFERRED_SHADOW_TARGET_WIDTH` setter を UBO write に redirect) + `pipeline.cpp` 7 setter call site を UBO 経由に切替

---

## §1. UBO identity

- **block_name**: `PbrShadowAlphaMaskVParamUBO_Legacy`
- **block_hash**: `0x7300d628u` (= FNV-1a("PbrShadowAlphaMaskVParamUBO_Legacy"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_pbrshadowalphamaskvparamubo_legacy.inl:12-14
struct PbrShadowAlphaMaskVParamUBO_LegacyLayout {
    static constexpr std::uint32_t shadow_target_width_OFFSET = 0u;  // size=4 align=4
};
inline constexpr std::uint32_t PbrShadowAlphaMaskVParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/class1/deferred/pbrShadowAlphaMaskV.glsl:89-91
layout(set=3, binding=21, std140) uniform PbrShadowAlphaMaskVParamUBO_Legacy {
    float shadow_target_width;
};
```

= Blueprint (= `aya_r41_blueprints/set3/pbr_shadow_alpha_mask_v_param_ubo_legacy.glsl:9-12`) 一致。

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 21
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: 不明 / verify 要
- **source**: `ubo_metadata.inl:63` `{ "PbrShadowAlphaMaskVParamUBO_Legacy", 0x7300d628u, 256u, 3u, 21u, 0u, 1u, 1u }` + `class1/deferred/pbrShadowAlphaMaskV.glsl:89` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program** (= `llglslshader.cpp:95`)
- **意味詳細**: shadow map render target の幅 (= sun/moon shadow / spot light shadow 等)、shadow pass dispatch 時に target サイズ依存で変化、program 単位
- **source**: ubo_metadata.inl + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **owner**: `target_width` (= `LLPipeline` shadow render target 幅、`pipeline.cpp` 各 shadow pass で算出)
- **lifetime**: shadow pass 単位 (= render target size 変更時 dirty)
- **用途**: shadow alpha mask vertex shader で target_pos_x スケーリング (= `pbrShadowAlphaMaskV.glsl:149` `target_pos_x = 0.5 * (shadow_target_width - 1.0) * pos.x;` literal)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/pbr_shadow_alpha_mask_v_param_ubo_legacy.glsl`
- **実 shader use site** = **`class1/deferred/pbrShadowAlphaMaskV.glsl:89`** (= blueprint header literal)
- **同 member 名 別 shader 使用** (= grep 結果):
  - `class1/deferred/shadowAlphaMaskV.glsl:86/93` (= 同 `shadow_target_width` 宣言、別 UBO? = PerProgramUBO_ShadowAlphaMaskV (set=2 binding=6) 候補、verify 要)
  - `class1/deferred/avatarAlphaShadowV.glsl:60/63` (= 同 `shadow_target_width` 宣言、別 UBO? = AvatarAlphaShadowVParamUBO_Legacy (set=3 binding=22) 候補、verify 要)
- consume 内容 (= grep 確認済):
  - `pbrShadowAlphaMaskV.glsl:149` `target_pos_x = 0.5 * (shadow_target_width - 1.0) * pos.x;`

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: 未配線
- **既存 OpenGL 経路 setter** (= grep 結果、`pipeline.cpp` 7 site):
  - `indra/newview/pipeline.cpp:8562` `LLGLSLShader::sCurBoundShaderPtr->uniform1f(LLShaderMgr::DEFERRED_SHADOW_TARGET_WIDTH, (float)target_width);`
  - `pipeline.cpp:8570` 同上
  - `pipeline.cpp:8584` 同上
  - `pipeline.cpp:8592` 同上
  - `pipeline.cpp:12596` 同上
  - `pipeline.cpp:12611` 同上
  - `pipeline.cpp:12642` 同上
- **reserved uniform 登録**: `indra/llrender/llshadermgr.cpp:1677` `mReservedUniforms.push_back("shadow_target_width");`
- **enum entry**: `indra/llrender/llshadermgr.h:195` `DEFERRED_SHADOW_TARGET_WIDTH, // "shadow_target_width"`

---

## §7. 現状通電状態

- **状態**: **untouched**
- **codegen 生成済**: layout `inl` + metadata entry + block_hash constexpr 生成済
- **blueprint 起案済**: `aya_r41_blueprints/set3/pbr_shadow_alpha_mask_v_param_ubo_legacy.glsl`
- **shader 宣言済**: `class1/deferred/pbrShadowAlphaMaskV.glsl:89` `#ifdef LL_VULKAN_GLSL` block
- **register/write/flush 経路**: 未配線

---

## §8. 本実装化に必要な作業

1. **register 経路**: `mapUniforms()` で PBR shadow alpha mask V program に `registerProgramUbo(this, block_hash::PbrShadowAlphaMaskVParamUBO_Legacy, 256u)` 呼出
2. **write 経路**: `pipeline.cpp` 7 site の `uniform1f(DEFERRED_SHADOW_TARGET_WIDTH, ...)` を `forwardToUboUpload` 経由 UBO write に redirect
3. **flush 経路**: cmdbuf bind 経路で sProgramUboDirty を flush
4. **shader 接続**: shader 追加改変なし
5. **同 member shadowAlphaMaskV / avatarAlphaShadowV との分離**: 3 shader (= pbr / shadow / avatar) で同 `shadow_target_width` を別 UBO に格納 = host C++ 側 program 識別で正しい UBO に dispatch 必須

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既に UBO 宣言済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**特記 risk**:
- 1 member 1 UBO = 容量効率 25% (16B/64B std140 + 240B pad to 256B)
- 同 member 名 3 UBO (= pbr / shadow / avatar) で重複格納 = host C++ 側で同 data を 3 UBO に同時 write 必要 (= program 識別で 1 つだけ write も可能、ただし shadow pass 内で 3 program 切替時に冗長)
- 7 setter call site = `pipeline.cpp` のみで集中、UBO 化作業は 1 file 内で完結

---

## §10. 不明事項

1. **同 `shadow_target_width` 3 UBO 重複格納の取扱** = host C++ 側で 3 UBO 全て write か、program 識別で 1 UBO だけ write か設計決定要
2. **shadow pass dispatch flow** = `pipeline.cpp:8562` 周辺の shadow rendering flow (= sun shadow / spot shadow / cube shadow 等の分岐) verify 要
3. **PerProgramUBO_ShadowAlphaMaskV (set=2 binding=6) との分離理由** = pbr / non-pbr で別 UBO に分離している設計意図 verify 要
4. **AvatarAlphaShadowVParamUBO_Legacy (set=3 binding=22) との分離理由** = avatar 専用 UBO 分離の設計意図
5. **set=3 帯 layout の binding 上限** = V3a 設計整合性

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- set=3 帯 Legacy UBO 群と共存
- 直近 binding 同居: binding=20 GlowExtractFParamUBO_Legacy / binding=22 AvatarAlphaShadowVParamUBO_Legacy

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 73 件 (推定) per-program cluster 内の 1 UBO

### §11.3 同 shader consume UBO (= `class1/deferred/pbrShadowAlphaMaskV.glsl` で同時 consume)

- 不明 / verify 要 (= pbrShadowAlphaMaskV.glsl 全文の UBO 宣言群確認要、Frame UBO 群 + PBR material UBO 群同時 consume 候補)

### §11.4 同 data source UBO (= 同 host data source から派生)

- **PerProgramUBO_ShadowAlphaMaskV** (set=2 binding=6) = `shadow_target_width` 同 member 名共有 = 同 shadow target size data source 由来
- **AvatarAlphaShadowVParamUBO_Legacy** (set=3 binding=22) = 同 `shadow_target_width` member 共有 (= avatar shadow pass、verify 要)

### §11.5 dirty 連動 UBO

- shadow render target resize 時 = **PerProgramUBO_ShadowAlphaMaskV** + **AvatarAlphaShadowVParamUBO_Legacy** + 本 UBO 同時 dirty 化必須

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- PBR shadow program 切替時 set=3 帯全 binding を一括 rebind

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.3 同期)

**Layer**: L4-3 (= C 判定 shadow_target_width triple-write group)
**status**: **起案済** (= 2026-06-06 C-6、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.3` (= single source of truth)
**要点**: shadow_target_width 3 UBO triple-write (= ShadowAlphaMaskV + PbrShadowAlphaMaskV + AvatarAlphaShadowV)、本 UBO offset=0 (1 active member)、PBR shadow alpha program 専用、setter 7 site 全特定済、3 UBO 同時 write vs program 識別 1 UBO write 選択 [要 AYA 判断]、工数 S-M、AYA live verify (= PBR shadow alpha mask 描画、visual regression ゼロ §5.4)
**関連**: L0-1 dispatch (= pbrShadowAlphaMaskV program 識別) / L3-19 ShadowUtilParamUBO_Legacy (= shadow render 全 program 共有 pattern) / §5.4 visual regression policy
