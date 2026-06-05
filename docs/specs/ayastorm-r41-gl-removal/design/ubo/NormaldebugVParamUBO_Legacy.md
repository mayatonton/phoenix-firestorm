# NormaldebugVParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= blueprint + codegen metadata 生成済、host C++ register/write/flush 経路未着工)

**本実装化に必要な作業**: register/write/flush 経路新設 (= per-program cadence、`LLShaderMgr::DEBUG_NORMAL_DRAW_LENGTH` setter を UBO write に redirect) + debug 用途ゆえ通電優先度は低い (= Phase 2 最終 batch 候補)

---

## §1. UBO identity

- **block_name**: `NormaldebugVParamUBO_Legacy`
- **block_hash**: `0x16b57ce8u` (= FNV-1a("NormaldebugVParamUBO_Legacy"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_normaldebugvparamubo_legacy.inl:12-14
struct NormaldebugVParamUBO_LegacyLayout {
    static constexpr std::uint32_t debug_normal_draw_length_OFFSET = 0u;  // size=4 align=4
};
inline constexpr std::uint32_t NormaldebugVParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/class1/interface/normaldebugV.glsl:56-58
layout(set=3, binding=37, std140) uniform NormaldebugVParamUBO_Legacy {
    float debug_normal_draw_length;
};
```

= Blueprint (= `aya_r41_blueprints/set3/normaldebug_v_param_ubo_legacy.glsl:9-12`) 一致。

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 37
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: 不明 / verify 要 (= V3a 設計 set=3 binding 配置整合性)
- **source**: `ubo_metadata.inl:56` `{ "NormaldebugVParamUBO_Legacy", 0x16b57ce8u, 256u, 3u, 37u, 0u, 1u, 1u }` + `class1/interface/normaldebugV.glsl:56` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program** (= `llglslshader.cpp:95`)
- **意味詳細**: normal debug 描画は debug 用途 (= `RenderDebugNormalScale` cvar / Debug menu の Normals visualization)、program load 時 register、debug setting 変更時に write
- **source**: ubo_metadata.inl + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **owner**: 不明 / verify 要 (= debug setting cvar 候補、grep で setter 直接 call site 未取得)
- **lifetime**: debug setting 単位 (= 起動中通常不変、debug menu 操作時に変更)
- **用途**: world space normal 可視化 (= `normaldebugV.glsl:99` `world_norm.xyz = debug_normal_draw_length * normalize(world_norm.xyz);` 用)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/normaldebug_v_param_ubo_legacy.glsl`
- **実 shader use site** = **`class1/interface/normaldebugV.glsl:56` 単独** (= blueprint header literal「Source: literal extract from class1/interface/normaldebugV.glsl:56」、grep 結果 1 file)
- consume 内容 (= grep 確認済):
  - `normaldebugV.glsl:99` `world_norm.xyz = debug_normal_draw_length * normalize(world_norm.xyz);`

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: 未配線
- **既存 OpenGL 経路 setter**:
  - 不明 / verify 要 (= grep で `uniform1f(DEBUG_NORMAL_DRAW_LENGTH, ...)` 直接 call site 未取得)
- **reserved uniform 登録**: `indra/llrender/llshadermgr.cpp:1846` `mReservedUniforms.push_back("debug_normal_draw_length");`
- **enum entry**: `indra/llrender/llshadermgr.h:363` `DEBUG_NORMAL_DRAW_LENGTH, // "debug_normal_draw_length"`

---

## §7. 現状通電状態

- **状態**: **untouched**
- **codegen 生成済**: layout `inl` + metadata entry + block_hash constexpr 生成済
- **blueprint 起案済**: `aya_r41_blueprints/set3/normaldebug_v_param_ubo_legacy.glsl`
- **shader 宣言済**: `class1/interface/normaldebugV.glsl:56` `#ifdef LL_VULKAN_GLSL` block
- **register/write/flush 経路**: 未配線

---

## §8. 本実装化に必要な作業

1. **register 経路**: `mapUniforms()` で normaldebug program に `registerProgramUbo(this, block_hash::NormaldebugVParamUBO_Legacy, 256u)` 呼出
2. **write 経路**: `uniform1f(DEBUG_NORMAL_DRAW_LENGTH, ...)` setter を `forwardToUboUpload` 経由 UBO write に redirect (= setter call site 特定要)
3. **flush 経路**: cmdbuf bind 経路で sProgramUboDirty を flush
4. **shader 接続**: shader 追加改変なし

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.6.3 同期)

**Layer**: L5-3 (= B Tier γ、debug 用途、検証優先度低、Phase 2 前提条件 94 UBO 全件 A 化の締め)
**status**: **起案済** (= 2026-06-06 C-7、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.6.3` (= single source of truth)
**要点**: trace L5-3 (= L5 締め)、工数 S、1 member only、setter 不明 (= debug menu trigger / `DEBUG_NORMAL_DRAW_LENGTH` reserved enum)、debug-only ゆえ通常 release path 影響なし、AYA live verify (= debug menu 経由 normal 可視化、visual regression ゼロ §5.4、verify protocol 要事前 AYA 確認)
**関連**: L0-1 + L0-4 / §5.4 visual regression policy

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
- debug 用途 = 通常 frame で program load されない (= debug menu 有効時のみ) = register/flush 経路は debug 有効時のみ稼働 = lazy init 設計でも問題なし
- Phase 2 全 UBO 一括着工方針 (= AYA literal record) には含まれるが、検証優先度は低 (= debug 視認で動作確認、production frame 不使用)

---

## §10. 不明事項

1. **debug_normal_draw_length setter call site** = grep で取得できず、debug 描画 dispatcher (= `LLPipeline` 等) の verify 要
2. **debug menu trigger** = どの menu item / cvar が本 debug 描画を enable するか
3. **set=3 帯 layout の binding 上限** = V3a 設計と Legacy UBO 実 binding 配置の整合性
4. **normaldebugV.glsl 全文の他 UBO consume** = FrameViewProj 等 common UBO 同 file 宣言確認要

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- set=3 帯 Legacy UBO 群と共存
- 直近 binding 同居: binding=36 / binding=38 SnapshotFrameFParamUBO_Legacy

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 73 件 (推定) per-program cluster 内の 1 UBO

### §11.3 同 shader consume UBO (= `class1/interface/normaldebugV.glsl` で同時 consume)

- 不明 / verify 要 (= normaldebugV.glsl 全文の UBO 宣言群確認要)

### §11.4 同 data source UBO

- 単独 (= debug setting 専用)

### §11.5 dirty 連動 UBO

- 単独 dirty (= debug setting 変更 trigger 単独)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- normaldebug program 切替時 set=3 帯全 binding を一括 rebind
