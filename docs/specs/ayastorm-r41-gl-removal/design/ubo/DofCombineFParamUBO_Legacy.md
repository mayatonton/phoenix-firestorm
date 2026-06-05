# DofCombineFParamUBO_Legacy — UBO design (= 実コードベース調査資料)

**通電状態**: untouched (= shell 通電なし、host C++ writer / register 配線無し、shader 側 LL_VULKAN_GLSL block でのみ宣言済)

**本実装化に必要な作業**: per-program cadence register / write 配線追加 + 既存 OpenGL 経路 setter (= DOF_RES_SCALE / DOF_WIDTH 系 uniform setter site) の mUseUBO 分岐経路から `forwardToUboUpload` → `writeProgramUbo` 経由で本 UBO に書込み開始 + `dofCombineF.glsl` の LL_VULKAN_GLSL block 活性化

---

## §1. UBO identity

- **block_name**: `DofCombineFParamUBO_Legacy`
- **block_hash**: `0x77b0758au`
- **block_size**: 256 B (= std140 16 B、device-padded 256 B)
- **member_count**: 3
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_dofcombinefparamubo_legacy.inl:12-16
struct DofCombineFParamUBO_LegacyLayout {
    static constexpr std::uint32_t res_scale_OFFSET = 0u;   // size=4 align=4
    static constexpr std::uint32_t dof_width_OFFSET = 4u;   // size=4 align=4
    static constexpr std::uint32_t dof_height_OFFSET = 8u;  // size=4 align=4
};
inline constexpr std::uint32_t DofCombineFParamUBO_Legacy_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set3/dof_combine_f_param_ubo_legacy.glsl:9-14
layout(std140, set = 3, binding = 24) uniform DofCombineFParamUBO_Legacy
{
    float res_scale;
    float dof_width;
    float dof_height;
};
```

= **DoF combine fragment shader params** (= resolution scale + DoF target width/height、post-process DoF combine pass)

---

## §2. binding 配線

- **descriptor_set**: 3
- **binding**: 24
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`
- **pipeline layout**: `sAYAStandardLayout`
- **source**: `ubo_metadata.inl:38` literal: `{ "DofCombineFParamUBO_Legacy", 0x77b0758au, 256u, 3u, 24u, 0u, 1u, 3u }`

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 1
- **意味**: **PerProgram** (= `llglslshader.cpp:95` literal)
- **意味詳細**: DoF combine fragment shader program bind 単位で update
- **source**: ubo_metadata.inl:38 + llglslshader.cpp:95

---

## §4. 物理 owner

- **data source**:
  - `res_scale` (float): resolution scale factor (= DoF low-res pass → output 倍率、推定)
  - `dof_width` (float): DoF render target width
  - `dof_height` (float): DoF render target height
- **uniform 名 reserved**: `llshadermgr.cpp:1704-1705` literal:
  ```cpp
  mReservedUniforms.push_back("res_scale");
  mReservedUniforms.push_back("dof_width");
  ```
  (= `dof_height` 不在は **不明 / verify 要** = reserved list 漏れ or shader 内 implicit 経路の可能性)
- **uniform 名 ↔ enum**: `llshadermgr.h:220-221` literal:
  ```cpp
  DOF_RES_SCALE,  //  "res_scale"
  DOF_WIDTH,      //  "dof_width"
  ```
- **既存 OpenGL 経路 writer site**: **不明 / verify 要** (= grep `DOF_RES_SCALE` / `DOF_WIDTH` で writer 特定要、推定 pipeline.cpp DoF combine pass 経路)
- **lifetime**: DoF combine pass 毎 (= post-process pass、1 frame 1 回呼出)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set3/dof_combine_f_param_ubo_legacy.glsl`
- **実 shader use site** (= 1 file confirmed):
  - `indra/newview/app_settings/shaders/class1/deferred/dofCombineF.glsl:98` (= blueprint source literal、`#ifdef LL_VULKAN_GLSL` block)
- **既存 OpenGL 経路**: 同 file 内 `#else` block で uniform 個別宣言

---

## §6. 既存 setter call site (host C++)

- **uniform 名 reserved**: `llshadermgr.cpp:1704-1705` literal
- **uniform 名 ↔ enum**: `llshadermgr.h:220-221` literal
- **writer call site**: **不明 / verify 要** (= grep `DOF_RES_SCALE` / `DOF_WIDTH` で writer 特定要、推定 pipeline.cpp DoF combine pass)
- **本 UBO 名指 setter**: なし

---

## §7. 現状通電状態

- **状態**: **untouched** (= shell 通電もされていない)
- **PC-7γ-1 PerProgram register**: 条件付き
- **PC-7γ-1 PerProgram write**: 条件付き

---

## §8. 本実装化に必要な作業

1. **mUseUBO ON 化**
2. **shader 側 LL_VULKAN_GLSL block 活性化** (= dofCombineF.glsl:98、起案済確認要)
3. **writer call site 特定** = grep `DOF_RES_SCALE` / `DOF_WIDTH` (= pipeline.cpp DoF combine pass)
4. **dof_height reserved list 漏れ確認**: `llshadermgr.cpp:1704-1705` で `res_scale` / `dof_width` は reserved だが `dof_height` 不在、shader 内 implicit 経路か別 enum で reserved か verify 要
5. **setter 経路 verify**
6. **codegen 再実行不要**

---

## §9. risk / 注意点

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 | ✅ 維持 (= set=3 binding=24) |
| OS-3 | std140 padding 厳守 | ✅ 16B → 256B padded |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ⚠️ dofCombineF.glsl に LL_VULKAN_GLSL block 追加済 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**dof_height reserved 漏れ risk**:
- `llshadermgr.cpp:1704-1705` で `res_scale` / `dof_width` は明示 reserved だが `dof_height` 不在
- shader 内 active uniform として extracted されているか確認要 (= mUniform[] 配列に entry あるか)
- 不在の場合 PC-7γ-1 register 経路で本 UBO の write_size と member offset 不整合 risk

**memory 関連 (透過 DoF)**:
- memory `project_transparent_dof_design_constraint` 参照: 透過 DoF (forward alpha BLEND) 構造制約あり、本 UBO は post-process DoF combine pass の base 経路 = 透過 DoF とは別経路 (= verify 要)

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守)

1. **dof_height reserved 漏れ確認** = `llshadermgr.cpp:1704-1705` に entry 不在の意図
2. **writer call site 特定** = DOF_RES_SCALE / DOF_WIDTH uniform setter (推定 pipeline.cpp DoF combine pass)
3. **res_scale の意味確認** = DoF low-res pass output 倍率か別 scale か
4. **DoF combine pass の bind 単位** = 1 frame 1 回 bind か multi-bind か

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=3)

- **DofCombineFParamUBO_Legacy (binding=24、本 UBO)**
- PerProgramUBO_CofF (set=2 binding=21、別 set DoF CoF pass)
- ScreenSpaceReflPostFParamUBO_Legacy (binding=28)
- MotionBlurFParamUBO_Legacy (binding=27)
- ExposureFParamUBO_Legacy (binding=25)
- LuminanceFParamUBO_Legacy (binding=26)

### §11.2 同 cadence cluster UBO (= cadence_tag=1 PerProgram)

- 同 post-process 系 cluster (= ExposureFParamUBO_Legacy / LuminanceFParamUBO_Legacy / MotionBlurFParamUBO_Legacy 等)

### §11.3 同 shader consume UBO

- dofCombineF.glsl 内同時 consume: 不明 / verify 要 (= 推定 FrameViewProj のみ、post-process pass shader は単純構成)

### §11.4 同 data source UBO

- **CASParamUBO_Legacy** (binding=12): post-process output target サイズ由来共有候補 (= verify 要)
- **PerProgramUBO_CofF** (set=2 binding=21): DoF CoF pass 関連、`dof_width` / `dof_height` 共有候補 (= verify 要)
- post-process target サイズ共有 UBO 群

### §11.5 dirty 連動 UBO

- window resize 時連動 dirty: CASParamUBO_Legacy / PerProgramUBO_CofF / 他 post-process target サイズ参照 UBO

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- set=3 帯 bind は `bindV3aStatic` 経路 (= post-process pass は static draw)
- DoF combine pass bind 時 PerProgram cadence triple-buffer flush
