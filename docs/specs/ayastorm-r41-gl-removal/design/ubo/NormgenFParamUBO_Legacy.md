# NormgenFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= blueprint + codegen metadata 生成済、host C++ register/write/flush 経路未着工)

**本実装化に必要な作業**: register/write/flush 経路新設 (= per-program cadence、4 uniform setter を UBO write に redirect) + normal map generation (bump-to-normal) 経路の setter call site 棚卸し

---

## §1. UBO identity

- **block_name**: `NormgenFParamUBO_Legacy`
- **block_hash**: `0x262f71c2u` (= FNV-1a("NormgenFParamUBO_Legacy"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 4
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_normgenfparamubo_legacy.inl:12-17
struct NormgenFParamUBO_LegacyLayout {
    static constexpr std::uint32_t stepX_OFFSET = 0u;      // size=4 align=4
    static constexpr std::uint32_t stepY_OFFSET = 4u;      // size=4 align=4
    static constexpr std::uint32_t norm_scale_OFFSET = 8u; // size=4 align=4
    static constexpr std::uint32_t bump_code_OFFSET = 12u; // size=4 align=4
};
inline constexpr std::uint32_t NormgenFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/class1/deferred/normgenF.glsl:53-58
layout(set=3, binding=33, std140) uniform NormgenFParamUBO_Legacy {
    float stepX;
    float stepY;
    float norm_scale;
    int   bump_code;
};
```

= Blueprint (= `aya_r41_blueprints/set3/normgen_f_param_ubo_legacy.glsl:9-15`) 一致。

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 33
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: 不明 / verify 要
- **source**: `ubo_metadata.inl:57` `{ "NormgenFParamUBO_Legacy", 0x262f71c2u, 256u, 3u, 33u, 0u, 1u, 4u }` + `class1/deferred/normgenF.glsl:53` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program** (= `llglslshader.cpp:95`)
- **意味詳細**: normgen shader (= bump-to-normal conversion) は per-program、texture サイズ dependent (`stepX`/`stepY`) + per-material bump 設定 dependent (`norm_scale`/`bump_code`)
- **source**: ubo_metadata.inl + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **owner**:
  - `stepX`/`stepY` = 入力 bump texture の 1 pixel UV step (= 推定 1/width, 1/height、bump texture upload 時計算)
  - `norm_scale` = normal scale factor (= 推定 material setting / texture height range)
  - `bump_code` = bump type 識別 (= grep `normgenF.glsl:78` `if (bump_code == BE_DARKNESS)` literal、bumpiness mode enum)
- **owner host source**: 不明 / verify 要 (= bump-to-normal 生成 dispatcher 特定要、`LLBumpImageList::onSourceLoaded` 周辺候補)
- **lifetime**: bump texture 生成単位 (= texture upload 時 1 回 dispatch、cache 後再利用)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/normgen_f_param_ubo_legacy.glsl`
- **実 shader use site** = **`class1/deferred/normgenF.glsl:53` 単独** (= blueprint header literal、grep 結果 1 file)
- consume 内容 (= grep 確認済):
  - `normgenF.glsl:78` `if (bump_code == BE_DARKNESS)` (= bumpiness mode 判定)
  - `normgenF.glsl:93` `vec3 right = vec3(norm_scale, 0, (getBumpValue(vary_texcoord0+vec2(stepX, 0))-c)*scaler);`
  - `normgenF.glsl:94-96` `stepX`/`stepY` で 4 隣接 sample → cross product で normal 計算

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: 未配線
- **既存 OpenGL 経路 setter**:
  - 不明 / verify 要 (= `LLShaderMgr` 内 reserved uniform list に存在不明、bump generation dispatcher の uniform setter 特定要)
  - `LLGLSLShader` reserved uniform 一覧 (`llglslshader.cpp:1080`) に `"bump_code"` / `"camPosLocal"` 等記載、`"norm_scale"` も `llglslshader.cpp:1087` `"norm_scale", "object_id_packed", ...` literal
- **reserved uniform 登録**:
  - `bump_code` = `llglslshader.cpp:1080` reserved list
  - `norm_scale` = `llglslshader.cpp:1087` reserved list
  - `stepX` / `stepY` = grep 未取得 / verify 要

---

## §7. 現状通電状態

- **状態**: **untouched**
- **codegen 生成済**: layout `inl` + metadata entry + block_hash constexpr 生成済
- **blueprint 起案済**: `aya_r41_blueprints/set3/normgen_f_param_ubo_legacy.glsl`
- **shader 宣言済**: `class1/deferred/normgenF.glsl:53` `#ifdef LL_VULKAN_GLSL` block
- **register/write/flush 経路**: 未配線

---

## §8. 本実装化に必要な作業

1. **register 経路**: `mapUniforms()` で normgen program に `registerProgramUbo(this, block_hash::NormgenFParamUBO_Legacy, 256u)` 呼出
2. **write 経路**: 4 uniform setter (= `stepX`/`stepY`/`norm_scale`/`bump_code`) を `forwardToUboUpload` 経由 UBO write に redirect
3. **flush 経路**: cmdbuf bind 経路で sProgramUboDirty を flush
4. **shader 接続**: shader 追加改変なし

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
- `bump_code` member 型 `int` (= GLSL 32-bit int) = host 側 enum (= `BE_DARKNESS` 等) と整合、type 整合確認要
- normgen dispatch 頻度低 (= texture cache 後 dispatch 不要) = UBO write 頻度低 = ring buffer 利用効率良
- bump texture を 4 隣接 sample する shader = `stepX`/`stepY` が texture size 依存、texture 切替時 dirty 再 write 必須

---

## §10. 不明事項

1. **normgen dispatcher 特定** = bump-to-normal 生成を起動する host 経路 (= `LLBumpImageList::onSourceLoaded` 等候補、verify 要)
2. **`stepX`/`stepY` setter call site** = grep 未取得 (= reserved uniform list 登録もない可能性、shader 専用 uniform で host 側 dynamic 計算で setter 呼出かも)
3. **`norm_scale` setter call site** = grep で `fspanelface.cpp` panel UI 値表示はあるが、shader 直接 setter は別 site
4. **set=3 帯 layout の binding 上限** = V3a 設計整合性

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- set=3 帯 Legacy UBO 群と共存
- 直近 binding 同居: binding=32 ClipFParamUBO_Legacy / binding=34 / binding=37 NormaldebugVParamUBO_Legacy

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 73 件 (推定) per-program cluster 内の 1 UBO

### §11.3 同 shader consume UBO (= `class1/deferred/normgenF.glsl` で同時 consume)

- 不明 / verify 要 (= normgenF.glsl 全文の UBO 宣言群確認要)

### §11.4 同 data source UBO

- 単独 (= bump texture generation 専用、他 UBO と data source 共有なし)

### §11.5 dirty 連動 UBO

- 単独 dirty

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- normgen program 切替時 set=3 帯全 binding を一括 rebind
