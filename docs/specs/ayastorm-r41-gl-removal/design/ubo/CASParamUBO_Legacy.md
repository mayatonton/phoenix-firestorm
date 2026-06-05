# CASParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= shell 通電なし、host C++ writer は LLStaticHashedString 経由特定済 = `pipeline.cpp:9185-9199`、shader 側 LL_VULKAN_GLSL block 宣言済)

**本実装化に必要な作業**: per-program cadence register / write 配線追加 + 既存 OpenGL 経路 setter (= `pipeline.cpp:9196-9199` の `sharpen_shader->uniform4uiv` × 2 + `sharpen_shader->uniform2f`) の mUseUBO 分岐経路から `forwardToUboUpload` → `writeProgramUbo` 経由で本 UBO に書込み開始 + `CASF.glsl` の LL_VULKAN_GLSL block 活性化

---

## §1. UBO identity

- **block_name**: `CASParamUBO_Legacy`
- **block_hash**: `0x7062b2c1u`
- **block_size**: 256 B (= std140 48 B、device-padded 256 B)
- **member_count**: 5 (ubo_metadata.inl 値)、blueprint コメントでは「6 member」と記述ありだが実際は struct member 5 個
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_casparamubo_legacy.inl:12-18
struct CASParamUBO_LegacyLayout {
    static constexpr std::uint32_t out_screen_res_OFFSET = 0u;  // size=8 align=8
    static constexpr std::uint32_t _pad_cas_0_OFFSET = 8u;      // size=4 align=4
    static constexpr std::uint32_t _pad_cas_1_OFFSET = 12u;     // size=4 align=4
    static constexpr std::uint32_t cas_param_0_OFFSET = 16u;    // size=16 align=16
    static constexpr std::uint32_t cas_param_1_OFFSET = 32u;    // size=16 align=16
};
inline constexpr std::uint32_t CASParamUBO_Legacy_SIZE = 256u; // std140=48, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/cas_param_ubo_legacy.glsl:9-16
layout(std140, set = 3, binding = 12) uniform CASParamUBO_Legacy
{
    vec2  out_screen_res;
    float _pad_cas_0;
    float _pad_cas_1;
    uvec4 cas_param_0;
    uvec4 cas_param_1;
};
```

= **AMD CAS (Contrast Adaptive Sharpening) params**、`out_screen_res` (vec2) + `cas_param_0/1` (uvec4 × 2 = CasSetup output, packed uint constants)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 12
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **set=3 内 Legacy 帯**: binding=12
- **source**: `ubo_metadata.inl:33` literal: `{ "CASParamUBO_Legacy", 0x7062b2c1u, 256u, 3u, 12u, 0u, 1u, 5u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: CAS sharpening pass shader bind 単位で update
- **source**: ubo_metadata.inl:33 + llglslshader.cpp:95 + llglslshader.cpp:2147-2149

---

## §4. 物理 owner

- **data source**:
  - `out_screen_res` (vec2): destination render target サイズ (= `pipeline.cpp:9199` literal: `(AF1)dst->getWidth(), (AF1)dst->getHeight()`)
  - `cas_param_0` (uvec4): CasSetup output `const0` (= AMD FidelityFX CAS 内部 const)
  - `cas_param_1` (uvec4): CasSetup output `const1`
- **既存 OpenGL 経路 writer site** (= **特定済**):
  - `pipeline.cpp:9182-9200` literal:
    ```cpp
    sharpen_shader->bind();
    {
        static LLStaticHashedString cas_param_0("cas_param_0");
        static LLStaticHashedString cas_param_1("cas_param_1");
        static LLStaticHashedString out_screen_res("out_screen_res");
        varAU4(const0);
        varAU4(const1);
        CasSetup(const0, const1, cas_sharpness(),
            (AF1)src->getWidth(), (AF1)src->getHeight(),
            (AF1)dst->getWidth(), (AF1)dst->getHeight());
        sharpen_shader->uniform4uiv(cas_param_0, 1, const0);
        sharpen_shader->uniform4uiv(cas_param_1, 1, const1);
        sharpen_shader->uniform2f(out_screen_res, (AF1)dst->getWidth(), (AF1)dst->getHeight());
    }
    ```
- **uniform 名 ↔ ReservedUniform**: `llglslshader.cpp:1081, 1087` literal で `"cas_param_0", "cas_param_1", "clip_plane", ..., "out_screen_res", ...` (= ReservedUniform list)
- **lifetime**: CAS sharpening pass 毎 (= post-process pass、1 frame 1 回呼出)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/cas_param_ubo_legacy.glsl`
- **実 shader use site** (= 1 file confirmed):
  - `indra/newview/app_settings/shaders/class1/deferred/CASF.glsl:52` (= blueprint source literal、`#ifdef LL_VULKAN_GLSL` block)
- **既存 OpenGL 経路**: 同 file 内 `#else` block で uniform 個別宣言

---

## §6. 既存 setter call site (host C++)

- **`pipeline.cpp:9196-9199`** (= 唯一の writer、§4 引用):
  - `sharpen_shader->uniform4uiv(cas_param_0, 1, const0)` (= LLStaticHashedString 経由)
  - `sharpen_shader->uniform4uiv(cas_param_1, 1, const1)`
  - `sharpen_shader->uniform2f(out_screen_res, ...)`
- **Vulkan UBO 経路**: 当該 `uniform*` call は `mUseUBO=true` 時に LLGLSLShader 内部の UBO redirect 層から forwardToUboUpload → writeProgramUbo に dispatch
- **本 UBO 名指 setter**: なし (= `Grep "CASParamUBO_Legacy" indra/llrender` 結果 0 件)

---

## §7. 現状通電状態

- **状態**: **untouched** (= shell 通電もされていない)
- **PC-7γ-1 PerProgram register**: 条件付き = mUseUBO=true 時 CASF.glsl link 時に走る
- **PC-7γ-1 PerProgram write**: 条件付き = mUseUBO=true で `pipeline.cpp:9196-9199` setter が走ると writeProgramUbo に流れる (default OFF)

---

## §8. 本実装化に必要な作業

1. **mUseUBO ON 化**
2. **shader 側 LL_VULKAN_GLSL block 活性化** (= CASF.glsl:52、起案済確認要)
3. **setter 経路 verify** (= `pipeline.cpp:9196-9199` の 3 call が forwardToUboUpload → writeProgramUbo へ dispatch)
4. **LLStaticHashedString 経路の UBO redirect 確認** (= `uniform4uiv` / `uniform2f` で LLStaticHashedString version の redirect 経路は別 code path の可能性、verify 要)
5. **CasSetup uvec4 packing 整合確認** (= AMD FidelityFX CAS 規約と std140 uvec4 align 16B 整合)
6. **codegen 再実行不要**

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set=3 binding=12) |
| OS-3 | std140 padding 厳守 | ✅ 48B → 256B padded、`_pad_cas_0/1` で uvec4 16B align 整合 |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ⚠️ CASF.glsl に LL_VULKAN_GLSL block 追加済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**LLStaticHashedString uniform path**:
- `pipeline.cpp:9196` の setter は LLStaticHashedString 名前 hash 経由 = UBO redirect 層が LLStaticHashedString version 全てで作動するか verify 要 (= memory `feedback_render_full_trace_first` 遵守要)
- uniform4uiv (= uint vector) の UBO write path 整合確認 (= std140 uvec4 と CPU `varAU4` packing 一致)

**ubo_metadata.inl member_count=5 vs blueprint コメント「6 member」**:
- struct member は実 5 個 (= out_screen_res / _pad_cas_0 / _pad_cas_1 / cas_param_0 / cas_param_1)
- blueprint コメント「6 member」は誤記または旧 design 残骸の可能性 (= verify 要)

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守)

1. **LLStaticHashedString UBO redirect 経路** = uniform4uiv / uniform2f の LLStaticHashedString version が forwardToUboUpload にどう dispatch されるか
2. **cas_sharpness() data source** = cvar `RenderSharpness` 等候補、verify 要
3. **blueprint コメント member_count 不一致** = 「6 member」記述の根拠
4. **CAS pass の bind 単位** = 1 frame 1 回 bind か multi-bind か (= multi-bind なら PerProgram cadence で stale risk)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

post-process 系 Legacy 帯:
- **CASParamUBO_Legacy (binding=12、本 UBO)**
- GlobalFParamUBO_Legacy (binding=11)
- TonemapUBO_Legacy (binding=10)
- PBROpaqueExtraUBO_Legacy (binding=13)
- SMAAParamUBO_Legacy (binding=14)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 他 post-process UBO (= TonemapUBO_Legacy / SMAAParamUBO_Legacy / DofCombineFParamUBO_Legacy 等)

### §11.3 同 shader consume UBO

- CASF.glsl 内同時 consume: 不明 / verify 要 (= 推定 FrameViewProj のみ、post-process pass shader は単純構成)

### §11.4 同 data source UBO

- **DofCombineFParamUBO_Legacy** (binding=24): `dof_width` / `dof_height` は output target サイズ由来、本 UBO `out_screen_res` と同 source の可能性 (= verify 要)
- post-process pass の output target サイズ共有関係

### §11.5 dirty 連動 UBO

- window resize 時 `out_screen_res` 連動 dirty = 他 post-process UBO (DofCombineFParamUBO_Legacy / GlowCombineFParamUBO_Legacy 等) と連動 dirty 推定

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- set=3 帯 bind は `bindV3aStatic` 経路 (= post-process pass は static draw)
- CAS pass bind 時 PerProgram cadence triple-buffer flush
