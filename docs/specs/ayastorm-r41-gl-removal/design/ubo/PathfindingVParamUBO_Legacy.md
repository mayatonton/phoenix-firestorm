# PathfindingVParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= blueprint + codegen metadata 生成済、host C++ register/write/flush 経路未着工)

**本実装化に必要な作業**: register/write/flush 経路新設 (= per-program cadence、`tint`/`ambiance`/`alpha_scale` setter を UBO write に redirect) + pathfinding visualization 経路の setter call site 棚卸し (= debug 用途、優先度低)

---

## §1. UBO identity

- **block_name**: `PathfindingVParamUBO_Legacy`
- **block_hash**: `0x98071e46u` (= FNV-1a("PathfindingVParamUBO_Legacy"))
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 4 (= 3 float + 1 pad)
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_pathfindingvparamubo_legacy.inl:12-17
struct PathfindingVParamUBO_LegacyLayout {
    static constexpr std::uint32_t tint_OFFSET = 0u;                                 // size=4 align=4
    static constexpr std::uint32_t ambiance_OFFSET = 4u;                             // size=4 align=4
    static constexpr std::uint32_t alpha_scale_OFFSET = 8u;                          // size=4 align=4
    static constexpr std::uint32_t _pad_pathfinding_v_legacy_0_OFFSET = 12u;         // size=4 align=4
};
inline constexpr std::uint32_t PathfindingVParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/class1/interface/pathfindingV.glsl:70-74
layout(set=3, binding=47, std140) uniform PathfindingVParamUBO_Legacy {
    float tint;
    float ambiance;
    float alpha_scale;
    float _pad_pathfinding_v_legacy_0;
};
```

= Blueprint 一致。

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 47
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 推定)
- **pipeline layout**: 不明 / verify 要
- **source**: `ubo_metadata.inl:61` `{ "PathfindingVParamUBO_Legacy", 0x98071e46u, 256u, 3u, 47u, 0u, 1u, 4u }` + `class1/interface/pathfindingV.glsl:70` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **per-program** (= `llglslshader.cpp:95`)
- **意味詳細**: pathfinding visualization (= debug 描画) with normal lighting 経路、ambiance パラメータあり (= 兄弟 PathfindingNoNormalVParamUBO_Legacy には ambiance なし、light dependent)
- **source**: ubo_metadata.inl + llglslshader.cpp:95 literal

---

## §4. 物理 owner

- **owner**: 不明 / verify 要 (= 兄弟 UBO と同じく `LLPathfindingNavMesh` / `LLPathfindingPathTool` 等の pathfinding visualization 経路候補)
- **lifetime**: pathfinding debug menu state 単位
- **用途**: pathfinding mesh の vertex color tint + ambiance + alpha scale (= `pathfindingV.glsl:94-96` literal):
  - `lit = clamp(lit, ambiance, 1.0);` (= ambiance を min lighting として clamp)
  - `vertex_color = vec4(diffuse_color.rgb * tint * lit, diffuse_color.a*alpha_scale);`

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/pathfinding_v_param_ubo_legacy.glsl`
- **実 shader use site** = **`class1/interface/pathfindingV.glsl:70` 単独** (= blueprint header literal、grep 結果 1 file)
- consume 内容 (= grep 確認済):
  - `pathfindingV.glsl:94` `lit = clamp(lit, ambiance, 1.0);`
  - `pathfindingV.glsl:96` `vertex_color = vec4(diffuse_color.rgb * tint * lit, diffuse_color.a*alpha_scale);`

---

## §6. 既存 setter call site (host C++)

- **shell 段階 setter**: 未配線
- **既存 OpenGL 経路 setter**:
  - 不明 / verify 要 (= grep で `uniform.*tint` / `uniform.*ambiance` / `uniform.*alpha_scale` 直接 call site 未取得)
  - 参考: `ambiance` 名は他に `proj_ambiance` (`llshadermgr.cpp:1559`) / `reflection_probe_ambiance` (`llshadermgr.cpp:1833`) として reserved uniform 登録あり、ただし pathfinding 専用 `ambiance` 名 (= prefix なし) の reserved 登録なし
  - `LLGLSLShader` reserved list (`llglslshader.cpp:1078`) に `"ambiance"` literal あり = 共通 reserved として登録済の可能性
- **reserved uniform 登録**: `ambiance` = `llglslshader.cpp:1078` reserved list `"ambiance", "aya_blur_dir", ...`

---

## §7. 現状通電状態

- **状態**: **untouched**
- **codegen 生成済**: layout `inl` + metadata entry + block_hash constexpr 生成済
- **blueprint 起案済**: `aya_r41_blueprints/set3/pathfinding_v_param_ubo_legacy.glsl`
- **shader 宣言済**: `class1/interface/pathfindingV.glsl:70` `#ifdef LL_VULKAN_GLSL` block
- **register/write/flush 経路**: 未配線

---

## §8. 本実装化に必要な作業

1. **register 経路**: `mapUniforms()` で pathfinding program に `registerProgramUbo(this, block_hash::PathfindingVParamUBO_Legacy, 256u)` 呼出
2. **write 経路**: `tint` / `ambiance` / `alpha_scale` setter を `forwardToUboUpload` 経由 UBO write に redirect (= setter call site 特定要)
3. **flush 経路**: cmdbuf bind 経路で sProgramUboDirty を flush
4. **shader 接続**: shader 追加改変なし
5. **PathfindingNoNormalVParamUBO_Legacy との区別**: 兄弟 UBO (= binding=48 PathfindingNoNormalVParamUBO_Legacy、ambiance 削除版) と program 識別で正しい UBO 選択必須

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
- debug 用途 = 通常 frame 不使用 = 検証優先度低
- 同 member 名 (= `tint`/`alpha_scale`) を **PathfindingNoNormalVParamUBO_Legacy** (set=3 binding=48) でも使用 = 兄弟 UBO で名前衝突、host C++ 側 program 識別で正しい UBO に dispatch 必須
- `ambiance` 名は他用途 (= proj_ambiance / reflection_probe_ambiance) と区別必要 = host C++ 側 reserved uniform handling で衝突回避要

---

## §10. 不明事項

1. **`tint`/`ambiance`/`alpha_scale` setter call site** = grep 未取得
2. **pathfinding visualization dispatcher** = `LLPathfindingPathTool` / `LLFloaterPathfindingConsole` 等の verify 要
3. **PathfindingNoNormalVParamUBO_Legacy との分離理由** = `ambiance` 有無で 2 UBO に分離している設計意図 verify 要 (= lighting あり / なしの shader pipeline 別)
4. **`ambiance` 名衝突 risk** = `proj_ambiance` / `reflection_probe_ambiance` 等と reserved uniform handling 整合性
5. **set=3 帯 layout の binding 上限** = V3a 設計整合性

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- set=3 帯 Legacy UBO 群と共存
- 直近 binding 同居: binding=46 VignetteParamUBO_Legacy / binding=48 PathfindingNoNormalVParamUBO_Legacy (= 兄弟 UBO)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 73 件 (推定) per-program cluster 内の 1 UBO

### §11.3 同 shader consume UBO (= `class1/interface/pathfindingV.glsl` で同時 consume)

- 不明 / verify 要

### §11.4 同 data source UBO (= 同 host data source から派生)

- **PathfindingNoNormalVParamUBO_Legacy** (set=3 binding=48) = **兄弟 UBO** = `tint`/`alpha_scale` 共有 = 同 pathfinding debug data source 由来 = host 側 setter call site も共通可能性大

### §11.5 dirty 連動 UBO

- **PathfindingNoNormalVParamUBO_Legacy** = pathfinding debug state 切替時に同時 dirty 化候補

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- pathfinding program 切替時 set=3 帯全 binding を一括 rebind
