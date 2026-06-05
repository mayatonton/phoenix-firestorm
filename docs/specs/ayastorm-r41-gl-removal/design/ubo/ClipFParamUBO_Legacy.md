# ClipFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= shell 通電なし、host C++ writer は LLStaticHashedString 経由特定済 = `llmaniptranslate.cpp:1715-1716`、shader 側 LL_VULKAN_GLSL block 宣言済)

**本実装化に必要な作業**: per-program cadence register / write 配線追加 + 既存 OpenGL 経路 setter (= `llmaniptranslate.cpp:1716` の `gClipProgram.uniform4fv(sClipPlane, 1, plane.v)`) の mUseUBO 分岐経路から `forwardToUboUpload` → `writeProgramUbo` 経由で本 UBO に書込み開始 + `clipF.glsl` の LL_VULKAN_GLSL block 活性化

---

## §1. UBO identity

- **block_name**: `ClipFParamUBO_Legacy`
- **block_hash**: `0x6bd83712u`
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_clipfparamubo_legacy.inl:12-14
struct ClipFParamUBO_LegacyLayout {
    static constexpr std::uint32_t clip_plane_OFFSET = 0u;  // size=16 align=16
};
inline constexpr std::uint32_t ClipFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/clip_f_param_ubo_legacy.glsl:9-12
layout(std140, set = 3, binding = 32) uniform ClipFParamUBO_Legacy
{
    vec4 clip_plane;
};
```

= **clip plane vec4** (= 4-component plane equation Ax + By + Cz + D = 0)、manip translate fragment shader 専用

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 32
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **set=3 内 Legacy 帯**: binding=32
- **source**: `ubo_metadata.inl:34` literal: `{ "ClipFParamUBO_Legacy", 0x6bd83712u, 256u, 3u, 32u, 0u, 1u, 1u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: clip program bind 単位で update
- **source**: ubo_metadata.inl:34 + llglslshader.cpp:95 + llglslshader.cpp:2147-2149

---

## §4. 物理 owner

- **data source**: `clip_plane` (vec4) = LLPlane manip translate clip plane (= `llmaniptranslate.cpp:1715-1716` literal)
- **既存 OpenGL 経路 writer site** (= **特定済**):
  - `llmaniptranslate.cpp:1715-1716` literal:
    ```cpp
    static LLStaticHashedString sClipPlane("clip_plane");
    gClipProgram.uniform4fv(sClipPlane, 1, plane.v);
    ```
- **uniform 名 ↔ ReservedUniform**: `llglslshader.cpp:1081` literal: `"cas_param_0", "cas_param_1", "clip_plane", "contrast", "contrastBase", ...`
- **owner program**: `gClipProgram` (= manip translate clip pass shader)
- **lifetime**: manip translate clip pass 毎 (= 1 frame 0..1 回呼出、translate manipulator visible 時のみ)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/clip_f_param_ubo_legacy.glsl`
- **実 shader use site** (= 1 file confirmed):
  - `indra/newview/app_settings/shaders/class1/interface/clipF.glsl:46` (= blueprint source literal、`#ifdef LL_VULKAN_GLSL` block)
- **既存 OpenGL 経路**: 同 file 内 `#else` block で uniform 個別宣言

---

## §6. 既存 setter call site (host C++)

- **`llmaniptranslate.cpp:1715-1716`** (= 唯一の writer、§4 引用):
  ```cpp
  static LLStaticHashedString sClipPlane("clip_plane");
  gClipProgram.uniform4fv(sClipPlane, 1, plane.v);
  ```
- **Vulkan UBO 経路**: 当該 `uniform4fv` call は `mUseUBO=true` 時に LLGLSLShader 内部の UBO redirect 層から forwardToUboUpload → writeProgramUbo に dispatch
- **本 UBO 名指 setter**: なし

---

## §7. 現状通電状態

- **状態**: **untouched** (= shell 通電もされていない)
- **PC-7γ-1 PerProgram register**: 条件付き = mUseUBO=true 時 clipF.glsl (gClipProgram) link 時に走る
- **PC-7γ-1 PerProgram write**: 条件付き = mUseUBO=true で setter が走ると writeProgramUbo に流れる (default OFF)

---

## §8. 本実装化に必要な作業

1. **mUseUBO ON 化**
2. **shader 側 LL_VULKAN_GLSL block 活性化** (= clipF.glsl:46、起案済確認要)
3. **setter 経路 verify** (= `llmaniptranslate.cpp:1716` の LLStaticHashedString uniform4fv が forwardToUboUpload → writeProgramUbo へ dispatch)
4. **LLStaticHashedString uniform4fv の UBO redirect 経路確認** (= memory `feedback_render_full_trace_first` 遵守要)
5. **codegen 再実行不要**

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set=3 binding=32) |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ⚠️ clipF.glsl に LL_VULKAN_GLSL block 追加済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**1 member only UBO**:
- 16 B 実 data + 240 B padding = overhead 大
- ただし manip translate pass は 1 frame 0..1 回呼出ゆえ overhead 影響 minimal

**LLStaticHashedString uniform4fv path**:
- `gClipProgram.uniform4fv(sClipPlane, 1, plane.v)` 経路の UBO redirect (= CAS / clip 共通課題)

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守)

1. **LLStaticHashedString uniform4fv 経路の UBO redirect** = CASParamUBO_Legacy と共通課題、verify 要
2. **gClipProgram の link 構造** = clip 専用 program か共有 program か
3. **clip_plane の運用** = LLPlane → vec4 変換経路、座標系 (world / view / projection)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- **ClipFParamUBO_Legacy (binding=32、本 UBO)**
- NormgenFParamUBO_Legacy (binding=33)
- (周辺 binding 30-33 帯 = SkinSSSPrototypeFParamUBO_Legacy / Normgen + 本 UBO)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 他 LLStaticHashedString 経由 setter UBO (= CASParamUBO_Legacy 等)

### §11.3 同 shader consume UBO

- clipF.glsl 内同時 consume: 不明 / verify 要 (= 推定 FrameViewProj のみ、clip pass shader は単純構成)

### §11.4 同 data source UBO

- なし (= clip_plane は manip translate 専用、他 UBO と data 共有なし)

### §11.5 dirty 連動 UBO

- manip translate visible 切替時の連動 UBO は推定なし (= clip 単独 lifecycle)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- set=3 帯 bind は `bindV3aStatic` 経路 (= manip translate は static draw)
- gClipProgram bind 時 PerProgram cadence triple-buffer flush
