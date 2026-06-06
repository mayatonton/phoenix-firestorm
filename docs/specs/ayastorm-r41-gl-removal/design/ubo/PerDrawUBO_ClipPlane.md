# PerDrawUBO_ClipPlane — UBO design (= 実コードベース調査資料)

**通電状態**: **untouched** (= host C++ で `PerDrawUBO_ClipPlane` への `writeDrawUbo` / `register*` 呼出ゼロ、Phase 1.C bringupTestUBO 系 generic 経路の zero buffer 通電のみ想定 = verify 要)

**本実装化に必要な作業**: 実 `clipPlane` vec4 (= viewport clip plane 4 平面係数) を host から writeDrawUbo 経由で書込 + 該当 shader (= class1/deferred/pbropaqueF.glsl + class1/gltf/pbrmetallicroughnessF.glsl + class3/deferred/softenLightF.glsl + class3/deferred/reflectionProbeF.glsl + class1/deferred/globalF.glsl) で UBO consume へ切替

---

## §1. UBO identity

- **block_name**: `PerDrawUBO_ClipPlane`
- **block_hash**: `0x142da0d9u` (= FNV-1a("PerDrawUBO_ClipPlane"))
- **block_size**: 256 B (= std140=16 B, device-padded 256 B)
- **member_count**: 1
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_perdrawubo_clipplane.inl:12-15
struct PerDrawUBO_ClipPlaneLayout {
    static constexpr std::uint32_t clipPlane_OFFSET = 0u;  // size=16 align=16
};
inline constexpr std::uint32_t PerDrawUBO_ClipPlane_SIZE = 256u; // std140=16, device-padded=256
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_draw_ubo_clip_plane.glsl:18-21
layout(std140, set = 2, binding = 0) uniform PerDrawUBO_ClipPlane
{
    vec4 clipPlane;
};
```

= **single vec4 member、real value 化 target 確定 (clipPlane = 視錐台クリップ平面係数)**

---

## §2. binding 配線

- **descriptor_set**: 2
- **binding**: 0
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC` (= set=2 UBO_DYNAMIC 帯、`llvkloader.cpp:861` literal `set=2: ... (UBO_DYNAMIC)`)
- **pipeline layout**: `sAYAStandardLayout` (= Phase 1.A 確立 5-set layout、set=2 = `sDrawUboLayoutV3a`、`llvkloader.cpp:2183`)
- **set=2 内訳** (= `llvkloader.cpp:861` `V3A_DRAW_SET_BINDINGS = 4` literal):
  - set=2 帯 binding=0 は **6 UBO 名で共有** (= per_draw_ubo_clip_plane.glsl:9-14 header literal):
    - PerDrawUBO_ClipPlane (本 UBO)
    - PerDrawUBO_LightParams
    - PerDrawUBO_AvatarSkin
    - PerDrawUBO_ObjectSkin
    - PerDrawUBO_SkinnedVelocity
    - PerDrawUBO_AvatarVelocity
  - 共有解決策 = **AYA option (I) 採用** = 各 UBO 名を独立 blueprint emit + Phase 1.B host wiring で **name-based dispatch** (= MaterialUBO/MaterialUBO_Legacy 同位 finding precedent)
- **source**: ubo_metadata.inl:66 `{ "PerDrawUBO_ClipPlane", 0x142da0d9u, 256u, 2u, 0u, 0u, 2u, 1u }` + blueprint header literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 2
- **意味**: **PerDraw** (= `llglslshader.cpp:96` literal: `constexpr U32 kCadencePerDraw = 2u; // PC-7ε で ring buffer 経路本格化`)
- **flush 経路**: `LLVKLoader::writeDrawUbo(block_hash, offset, data, size, dynamic_offset)` (= `llglslshader.cpp:2167` setter dispatch `case kCadencePerDraw:`)
- **意味詳細**: per-draw cadence ゆえ draw call 毎に ring buffer allocate + memcpy + dynamic_offset 返却 (= PC-7ε ring buffer 経路、`sDrawUboRingBufferMgr`)
- **source**: ubo_metadata.inl + llglslshader.cpp:96 + llglslshader.cpp:2151-2167 literal

---

## §4. 物理 owner

- **shell 段階**: owner なし (= writeDrawUbo 経路は per-draw transient、ring buffer 内 dynamic offset で識別)
- **本実装化後の data source** (= **verify 要**):
  - `LLDrawPoolAlpha` / `LLDrawPool*` 経由 (= 既存 OpenGL 経路で `gDeferredPBROpaqueProgram->uniform4fv(LLShaderMgr::CLIP_PLANE, clip_plane)` 等で書込んでいる site を grep verify 要)
  - 上流 = `LLPipeline::mTransformedClip` 等 (= 推定、grep verify 要)
- **lifetime**: per-draw call

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_draw_ubo_clip_plane.glsl:18-21`
  - source extract from `class1/deferred/pbropaqueF.glsl:180` ifdef LL_VULKAN_GLSL block (verified identical across 5 sample sites = pbropaqueF / class1/gltf/pbrmetallicroughnessF / class3/deferred/softenLightF / class3/deferred/reflectionProbeF / class1/deferred/globalF) (= per_draw_ubo_clip_plane.glsl:1-7 header literal)
- **実 shader use site** (= grep 結果、`PerDrawUBO_ClipPlane` 文字列 hit、blueprint 除く):
  - `indra/newview/app_settings/shaders/class1/deferred/pbropaqueF.glsl:180`
  - `indra/newview/app_settings/shaders/class1/gltf/pbrmetallicroughnessF.glsl`
  - `indra/newview/app_settings/shaders/class3/deferred/softenLightF.glsl`
  - `indra/newview/app_settings/shaders/class3/deferred/reflectionProbeF.glsl`
  - `indra/newview/app_settings/shaders/class1/deferred/globalF.glsl`
- **実 shader 構造** (= pbropaqueF.glsl:180-185 literal):
  ```glsl
  layout(set=2, binding=0, std140) uniform PerDrawUBO_ClipPlane {
      vec4 clipPlane;
  };
  // ... #else block ...
  uniform vec4 clipPlane;
  ```
  = `#ifdef LL_VULKAN_GLSL` 切替で OpenGL 側 (`#else`) は uniform 個別宣言、Vulkan 側は UBO consume

---

## §6. 既存 setter call site (host C++)

- **現状**: **PerDrawUBO_ClipPlane 専用 setter 不在** (= `llvkloader.cpp` + `llglslshader.cpp` 内 `PerDrawUBO_ClipPlane` 文字列 hit 0 件、grep 確認済 2026-06-06)
- **shell 段階通電経路**: 推定 = `LLGLSLShader::bringupTestUBO` 経由 generic zero buffer 通電 (= verify 要、PerDrawUBO_LightParams と異なり個別 PC-N-1 (c) site なし)
- **本実装化後 setter** (= **不明 / verify 要**):
  - 既存 OpenGL 経路で `clipPlane` uniform を書込む site を grep verify 要 (= 候補: `LLShaderMgr::CLIP_PLANE` uniform handle 経由、`LLPipeline::beginRenderDeferred` 等)

---

## §7. 現状通電状態

- **状態**: **untouched** (= Phase 1.E 終了時点、host C++ で本 UBO 個別配線ゼロ)
- **通電 commit**: なし
- **通電内容**:
  - host C++ 側 `writeDrawUbo(PerDrawUBO_ClipPlane, ...)` 呼出 0 件 (= grep 確認済)
  - shader 側 UBO consume は **既存 GLSL に既存** (= `class1/deferred/pbropaqueF.glsl:180` 等 5 file で `#ifdef LL_VULKAN_GLSL` block 既配置) = Vulkan path enable 時 zero buffer 読込み
- **shell 通電経路** (= 推定 / verify 要): bringupTestUBO 経路で全 UBO 共通 zero buffer 通電想定、PerDraw cadence の場合 sDrawUboRingBufferMgr 経由 dynamic offset 構築要

---

## §8. 本実装化に必要な作業

1. **host C++ writeDrawUbo 配線** = recordGltfAssetDraw / recordAvatarPlaceholderDraw 等の per-draw call site で `LLVKLoader::writeDrawUbo(ubo::block_hash::PerDrawUBO_ClipPlane, 0u, clipPlane_data, 16, dynamic_offset)` 配線追加
2. **data source 特定** = 既存 OpenGL 経路の `clipPlane` uniform 書込 site grep verify (= `LLShaderMgr::CLIP_PLANE` 等候補)
3. **dirty 判定** = clipPlane は viewport / camera 変化時に更新 = per-frame 1 回計算 → per-draw 同値 broadcast、dirty 判定不要 (= per-draw cadence ゆえ毎 draw 書込み許容)
4. **set=2 binding=0 共有解決** = name-based dispatch (= MaterialUBO 同位 precedent)、Phase 1.B host wiring で別 UBO (LightParams / AvatarSkin / ObjectSkin / SkinnedVelocity / AvatarVelocity) と program 別に切替必要 = PSO 別 program 毎に 1 名のみ binding=0 占有
5. **shader 側 #else block 撤去** (= Phase 2+ OpenGL gate 完全撤去時) = `uniform vec4 clipPlane;` 削除して UBO 単独参照化

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合:

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + set=2 内 binding=0 共有 6 UBO | ✅ 既配置 (= blueprint header 既明示)、program 別に 1 名選択 |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 16B → 256B padded (codegen) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 |
| OS-5 | shader 改変ゼロ | ✅ 既存 GLSL `#ifdef LL_VULKAN_GLSL` block 既配置、改変なし (Phase 2+ #else 撤去時のみ改変) |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 |

**set=2 binding=0 共有 risk**:
- 6 UBO で binding=0 共有 = program 毎に 1 名のみ active = host wiring で program 識別 + 該当 UBO 名 dispatch 必要 (= name-based dispatch precedent、blueprint header 既明示)
- 誤った UBO 名 dispatch 時 = layout 不一致 validation 違反 + GPU error risk

**layout 互換性**:
- shell 段階 zero buffer 通電 → real value 化で buffer size 256 B / set / binding 不変契約

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

1. **shell 通電 commit 特定** = bringupTestUBO 経由 generic 通電 commit hash (= Phase 1.C handoff doc chain 参照要)
2. **既存 OpenGL setter call site** = `clipPlane` uniform 書込 host C++ site (= `LLShaderMgr::CLIP_PLANE` uniform handle 経由候補、grep verify 要)
3. **data source 上流** = `LLPipeline::mTransformedClip` / `LLViewerCamera` 等の data source (= 推定、grep verify 要)
4. **set=2 binding=0 name-based dispatch 実装詳細** = host wiring で program 識別 + UBO 名 dispatch logic (= Phase 1.B 設計済、実装は Phase 2 本実装化時)
5. **5 shader file 内 PerDrawUBO_ClipPlane consume 統一確認** = pbropaqueF + pbrmetallicroughnessF + softenLightF + reflectionProbeF + globalF 各 file で UBO block 内容完全一致確認 (= blueprint header「verified identical across 5 sample sites」literal 既宣言、再 verify 要)

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=2、`llvkloader.cpp:861` `V3A_DRAW_SET_BINDINGS = 4` literal)

- set=2 帯は **4 binding × 複数 UBO 名共有** = blueprint header 既明示 6 UBO + 別 binding (1/3/4/.../22) 多数 UBO
- 本 UBO は set=2 binding=0 (= 6 共有 UBO のうち 1)

### §11.2 同 cadence cluster UBO (= cadence_tag=2 PerDraw)

- PerDrawUBO_AvatarSkin (set=2 binding=0)
- PerDrawUBO_AvatarVelocity (set=2 binding=0)
- **PerDrawUBO_ClipPlane (本 UBO、set=2 binding=0)**
- PerDrawUBO_LightParams (set=2 binding=0) = Phase 1.E PC-N-13 で「zero IS real data」semantic 通電済
- PerDrawUBO_MultiLight (set=2 binding=1)
- PerDrawUBO_ObjectSkin (set=2 binding=0)
- PerDrawUBO_SkinnedVelocity (set=2 binding=0)
- = PerDraw cluster は 7 UBO

### §11.3 同 shader consume UBO

- 同 shader file (= class1/deferred/pbropaqueF.glsl 等 5 file) で同時 consume されうる UBO group = verify 要 (= 同 file 内 `set=2` 他 binding UBO 探索要)

### §11.4 同 data source UBO

- `clipPlane` 独立 data source ゆえ他 UBO と共有なし (= 推定)

### §11.5 dirty 連動 UBO

- viewport / camera 変更時に同時更新 = Frame_ViewProj (set=0 binding=0) と連動可能性 (= 推定)、verify 要

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout

### §11.7 bind 順序関係

- per-draw cadence ゆえ draw call 毎に `bindV3aStatic` / `bindV3aRigged` 経由 set=2 帯 4 binding 同時 bind (= `llvkloader.cpp:2192/2233` literal、`V3A_DRAW_SET_BINDINGS=4` 個 dynamic_offset 渡し)

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.5.15 同期)

**Layer**: L4-15 (= set=2 binding=0 共有残 2 UBO、name-based dispatch 6 UBO 集約 group)
**status**: **起案済** (= 2026-06-06 C-6-j、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.5.15` (= single source of truth)
**要点**: 1 member (clipPlane vec4)、5 file consume (= pbropaqueF/pbrmetallicroughnessF/softenLightF/reflectionProbeF/globalF、blueprint `verified identical across 5 sample sites`)、setter 不明 (= `LLShaderMgr::CLIP_PLANE` 経由候補) [要追加調査]、data source 上流 = LLPipeline::mTransformedClip / LLViewerCamera [要追加調査]、状態 = **untouched** (writeDrawUbo 0 件)、per-draw 同値 broadcast (= per-frame 1 回計算 → 毎 draw 書込み許容、dirty 判定不要)、name-based dispatch precedent 集約 (= §3.5.8 group 4 UBO + 本 group 2 UBO = 6 UBO 同 binding=0)、cadence PerDraw 維持、工数 group 全体 M 内
**関連**: L0-1 dispatch (= name-based dispatch 6 UBO 集約) / §3.5.15 sibling LightParams (= 同 binding=0 共有) / §3.5.8 group 4 UBO (= 同 binding=0 共有残) / FrameViewProj (= viewport/camera 連動 dirty 候補)
