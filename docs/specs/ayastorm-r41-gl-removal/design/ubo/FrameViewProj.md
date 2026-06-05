# FrameViewProj — UBO design (= 実コードベース調査資料)

**通電状態**: shell 通電済 (= Phase 1.A PA-8 + 1.C PC-7γ で `sFrameUboInstances` 経路 allocate + per-frame `vkCmdBindDescriptorSets` 通電完了、`syncMatrices` 既存 32 経路 OpenGL uniformMatrix4fv 経路と並走、Vulkan 側 host write 経路は `writeFrameUbo` 経由 PER_FRAME case で通電済、実 shader UBO consume は既存 `#ifdef LL_VULKAN_GLSL` block 内に layout 宣言済 = `mUseUBO=false` default で OpenGL path 100% 維持)

**本実装化に必要な作業**: shell 段階で既に member 名 / offset / size は実 production data と一致 (= literal extract from pbropaqueF/simpleNoColorV/previewPhysicsV/bumpV/simpleNoAtmosV)。残作業 = `syncMatrices` 内 31 setter 経由 forwardToUboUpload PER_FRAME case 経路の実 hot path 通電 + `mUseUBO=true` cold launch 検証 + GLSL UBO block 内 member access の per-shader 通電拡大

---

## §1. UBO identity

- **block_name**: `FrameViewProj`
- **block_hash**: `0x06aff62cu` (= FNV-1a("FrameViewProj")、`ubo_metadata.inl:42` literal + `block_hash::FrameViewProj` namespace constexpr)
- **block_size**: 512 B (= std140=496, device-padded=512、`ubo_layout_frameviewproj.inl:23` literal)
- **member_count**: 9
- **struct definition** (= 実コード source 直接 reference):

```cpp
// build-linux-x86_64/codegen/ubo/ubo_layout_frameviewproj.inl
struct FrameViewProjLayout {
    static constexpr std::uint32_t modelview_projection_matrix_OFFSET = 0u;   // size=64 align=16
    static constexpr std::uint32_t modelview_matrix_OFFSET            = 64u;  // size=64 align=16
    static constexpr std::uint32_t projection_matrix_OFFSET           = 128u; // size=64 align=16
    static constexpr std::uint32_t inv_proj_OFFSET                    = 192u; // size=64 align=16
    static constexpr std::uint32_t proj_mat_OFFSET                    = 256u; // size=64 align=16
    static constexpr std::uint32_t last_modelview_matrix_OFFSET       = 320u; // size=64 align=16
    static constexpr std::uint32_t env_mat_OFFSET                     = 384u; // size=48 align=16
    static constexpr std::uint32_t normal_matrix_OFFSET               = 432u; // size=48 align=16
    static constexpr std::uint32_t screen_res_OFFSET                  = 480u; // size=8  align=8
};
inline constexpr std::uint32_t FrameViewProj_SIZE = 512u; // std140=496, device-padded=512
```

```glsl
// indra/newview/app_settings/shaders/aya_r41_blueprints/set0/frame_view_proj.glsl
layout(std140, set = 0, binding = 0) uniform FrameViewProj
{
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    mat4 projection_matrix;
    mat4 inv_proj;
    mat4 proj_mat;
    mat4 last_modelview_matrix;
    mat3 env_mat;
    mat3 normal_matrix;
    vec2 screen_res;
};
```

= **mat4×6 + mat3×2 + vec2 の 9 member、std140 で 496 B、device-padded 512 B**、Frame 系最大 size UBO

---

## §2. binding 配線

- **descriptor_set**: 0
- **binding**: 0
- **VkDescriptorType**: `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` (= 全 UBO 共通)
- **pipeline layout**: `sAYAStandardLayout` (= Phase 1.A 確立 5-set layout、`llvkloader.cpp:881` literal)
- **set 0 内訳** (= `llvkloader.cpp:858` `V3A_FRAME_SET_BINDINGS = 4` literal):
  - set=0 binding=0 = **FrameViewProj (本 UBO)**
  - set=0 binding=1 = FrameLights
  - set=0 binding=2 = FrameAtmosphere_Lighting
  - set=0 binding=3 = Global_ReflectionProbes
- **source**: `ubo_metadata.inl:42` `{ "FrameViewProj", 0x06aff62cu, 512u, 0u, 0u, 0u, 0u, 9u }` literal

---

## §3. cadence

- **ubo_metadata.inl cadence_tag**: 0
- **意味**: **per-frame** (= `llglslshader.cpp:94` literal: `constexpr U32 kCadencePerFrame = 0u; // FrameAtmosphere_Lighting / FrameLights / FrameViewProj`)
- **flush 経路**: `LLVKLoader::flushFrameUbos()` (= `llvkloader.cpp:5117` literal)
- **write 経路**: `LLVKLoader::writeFrameUbo(block_hash, offset, data, size)` (= `llglslshader.cpp:2144` literal、PER_FRAME case)
- **storage**: `sFrameUboInstances` map<block_hash, UboInstance> (= `llvkloader.cpp:653` literal、block_hash 単独 key)

---

## §4. 物理 owner

- **owner**: `LLRender` (= `indra/llrender/llrender.cpp` 、`LLRender::syncMatrices()` 内で mModelview/mProjection/mTextureMatrix 等 GL matrix stack を shader uniform へ転送)
- **data source**:
  - `modelview_matrix` / `projection_matrix` / `modelview_projection_matrix` / `inv_proj` / `last_modelview_matrix` / `proj_mat` = `LLRender::mMatrix[]` stack (= `llrender.cpp:988-1124` `LLRender::syncMatrices` literal)
  - `normal_matrix` = modelview_matrix の inverse-transpose 上 3x3 (= `llrender.cpp:1050` `uniformMatrix3fv(NORMAL_MATRIX, ...)`)
  - `env_mat` = environment cubemap orientation matrix (= verify 要、`LLShaderMgr::DEFERRED_ENV_MAT` 経由)
  - `screen_res` = `gPipeline.mScreen` width/height (= `LLShaderMgr::DEFERRED_SCREEN_RES`、`llshadermgr.h:183` literal)
- **lifetime**: per-frame (= camera + viewport state、frame 開始時に決定し frame 内 stable)

---

## §5. use site (shader)

- **blueprint file**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set0/frame_view_proj.glsl` (= Phase 1.A PA-8 起案)
- **実 shader use site** (= UBO block 宣言済 shader、`#ifdef LL_VULKAN_GLSL` block 内):
  - `class1/post/exoPostBaseV.glsl:29-39` (= literal `mat4 modelview_projection_matrix` 等 9 member 宣言)
  - `class1/post/snapshotFrameF.glsl:17-25` (= 同 9 member 宣言)
  - `class1/post/exoVignetteF.glsl:26-34` (= 同 9 member 宣言)
  - 他 verify 要 (= grep result 50+ files で `modelview_matrix` 参照、UBO block 内宣言と raw uniform 宣言の両方が混在)
- **blueprint origin (literal extract source)**: `class1/deferred/pbropaqueF.glsl:198` + 4 sample sites (= `frame_view_proj.glsl:4` literal comment)
- **consume pattern**: vertex shader で gl_Position 計算 + fragment で screen-space tc + normal transform で使用

---

## §6. 既存 setter call site (host C++)

- **主 entry**: `LLRender::syncMatrices()` (= `llrender.cpp:988`)
  - `modelview_matrix` 更新: `shader->uniformMatrix4fv(MODELVIEW_MATRIX, 1, GL_FALSE, ...)` (= `llrender.cpp:1028`)
  - `normal_matrix` 更新: `shader->uniformMatrix3fv(NORMAL_MATRIX, 1, GL_FALSE, ...)` (= `llrender.cpp:1050`)
  - `modelview_projection_matrix` 更新: `shader->uniformMatrix4fv(MODELVIEW_PROJECTION_MATRIX, 1, GL_FALSE, ...)` (= `llrender.cpp:1073, 1115`)
  - `inv_proj` 更新: `shader->uniformMatrix4fv(INVERSE_PROJECTION_MATRIX, 1, false, ...)` (= `llrender.cpp:1088`)
  - `projection_matrix` 更新: `shader->uniformMatrix4fv(PROJECTION_MATRIX, 1, GL_FALSE, ...)` (= `llrender.cpp:1097`)
- **screen_res setter**: 不明 / verify 要 (= grep で `screen_res` の uniform2f setter call site 直接ヒットせず、`LLShaderMgr::DEFERRED_SCREEN_RES` 経由の汎用 setter 経路使用想定)
- **last_modelview_matrix setter**: 不明 / verify 要 (= per-frame で前 frame modelview 保存、velocity buffer 用、pipeline.cpp 内 setter site grep 要)
- **env_mat setter**: 不明 / verify 要 (= `LLShaderMgr::DEFERRED_ENV_MAT` 経由、cubemap orientation 関連)
- **共通 redirect 経路**: `LLGLSLShader::uniformMatrix4fv` / `uniformMatrix3fv` (= 既 PC-7γ-1 で `mUseUBO=true` 時 `forwardToUboUpload(loc, ...)` 経路接続済、`llglslshader.cpp` 内 31 setter call site の 1 群)
- **PER_FRAME case**: `forwardToUboUpload` 内 `case kCadencePerFrame: LLVKLoader::writeFrameUbo(loc.block_hash, loc.offset, data, size);` (= `llglslshader.cpp:2143-2145` literal)

---

## §7. 現状通電状態

- **状態**: **shell 通電済 + write 経路本格化済** (= Phase 1.A PA-8 blueprint 起案 + 1.C PC-7γ-1 で writeFrameUbo path + sFrameUboInstances allocate 完了、`llvkloader.cpp:4089` `UboInstance& ubo_inst = sFrameUboInstances[meta.block_hash];` literal)
- **bind 経路**: `llvkloader.cpp:2839` `V3A_FRAME_SET_BINDINGS` 経由 `vkCmdBindDescriptorSets` で set=0 全 4 UBO 同時 bind (= `llvkloader.cpp:2918` `V3A_FRAME_SET_BINDINGS * V3A_FRAME_POOL_MAX_SETS` descriptor pool allocate)
- **write 経路**: `forwardToUboUpload` PER_FRAME case → `writeFrameUbo` → memcpy + dirty.store(true)、flush 側 `flushFrameUbos` で `vkCmdUpdateBuffer` または mapped pointer write (= verify 要)
- **MUSEUBO-A 整合**: `mUseUBO=false` default で本 entry 不到達 = 既存 OpenGL 描画 100% 維持 (= `llglslshader.cpp:2135-2137` literal note)

---

## §8. 本実装化に必要な作業

1. **`mUseUBO=true` cold launch 検証**:
   - 既存 31 setter 経路 (= uniformMatrix4fv / uniformMatrix3fv / uniform2f 等) の forwardToUboUpload 経路で PER_FRAME case 実走確認
   - `flushFrameUbos` で 9 member 全 offset の memcpy が dirty.store(true) と整合
2. **per-shader UBO consume 拡大**:
   - 現 blueprint origin sample = 5 file (pbropaqueF / simpleNoColorV / previewPhysicsV / bumpV / simpleNoAtmosV) + post-process 3 file (exoPostBaseV / snapshotFrameF / exoVignetteF) で UBO block 宣言済
   - 残 shader (= grep result 50+ files で `modelview_matrix` raw uniform 参照、UBO block 未宣言) に対し `#ifdef LL_VULKAN_GLSL` block 内 UBO block 宣言 + raw uniform 宣言 `#else` block 移動の patch 拡大要
   - 上限 = upstream Firestorm 由来全 shader file (= 設計原則 (1) Upstream 取り込みやすさ維持と緊張、verify 要)
3. **screen_res / env_mat / last_modelview_matrix setter site 完全特定**:
   - 各 member の host C++ setter call site grep verify (= §6 不明 3 件)
   - 既存 setter 経路で UBO redirect 通電確認
4. **`writeFrameUbo` 内 ring buffer / triple-buffer 経路 verify**:
   - FRAMES_IN_FLIGHT=3 (= verify 要、`llvkloader.cpp:4112` `FRAMES_IN_FLIGHT * V3A_FRAME_SET_BINDINGS` literal)
   - per-frame 切替で frame index に応じた UboInstance offset 解決
5. **codegen 再実行不要**:
   - blueprint member 不変 (= 実 production data と一致)、Phase 1.D+ の per-shader 拡大時に layout 改変なし

---

## §9. risk / 注意点

OS-1〜OS-10 gate 照合 (= memory `project_r41_phase2_4_principles` 原則 2):

| gate | 該当 risk | 対応 |
|---|---|---|
| OS-2 | descriptor set 数 5 維持 + 既設 set=0 内に収める | ✅ 維持 (= set 0 binding=0、Phase 1.A 配置済) |
| OS-3 | std140 padding 厳守 + offset 二重保証 | ✅ 496 B std140 → 512 B padded (codegen 出力で生成) |
| OS-4 | minUniformBufferOffsetAlignment 動的取得 | ⚠️ verify 要 (= 512 B = 大半 GPU の 256 B alignment 倍数充足、device-padded で吸収) |
| OS-5 | shader 改変ゼロ | ⚠️ **本 UBO は per-shader UBO block 宣言追加が本実装化必須** = `#ifdef LL_VULKAN_GLSL` block 内追加、`#else` 既 raw uniform 保護で OpenGL 100% 維持 |
| OS-7 | Linux validation layer warnings 0 件 | ⚠️ verify 要 (= Phase 2 cold launch 時) |

**per-shader 拡大 risk**:
- upstream Firestorm 由来全 vertex/fragment shader で `modelview_matrix` 等が raw uniform として宣言、UBO block 化拡大 = upstream patch conflict risk (= 設計原則 (1) Upstream 取り込みやすさ維持と緊張)
- 対応 = `#ifdef LL_VULKAN_GLSL` block で UBO block を gate、`#else` で既 raw uniform 維持 (= GATE-B 整合、shader file 単位の Vulkan / OpenGL 並走)

**Frame 系最大 size UBO**:
- 512 B = Frame 系 4 UBO 中最大 (= FrameAtmosphere_Lighting 256 / FrameLights 768 / Global_ReflectionProbes 256 / FrameViewProj 512、total 1792 B)
- per-frame で 9 member 全更新 (= matrix stack push/pop 含む modelview_matrix 経路) → frame 内 memcpy 頻度高、ring buffer / mapped pointer 経路 perf verify 要

---

## §11. 他 UBO との関係

### §11.1 同 set 同居 UBO (= set=0、`llvkloader.cpp:858` `V3A_FRAME_SET_BINDINGS = 4` literal)

- **FrameViewProj (set=0 binding=0、本 UBO)**
- FrameLights (set=0 binding=1)
- FrameAtmosphere_Lighting (set=0 binding=2)
- Global_ReflectionProbes (set=0 binding=3)

### §11.2 同 cadence cluster UBO (= cadence_tag=0 per-frame、`flushFrameUbos` 共通経路)

- **FrameViewProj (本 UBO)**
- FrameLights
- FrameAtmosphere_Lighting
- = per-frame 3 UBO 全件 (Global_ReflectionProbes は cadence=5 singleton ゆえ別経路)

### §11.3 同 shader consume UBO (= 同 shader file 内同時 consume)

- ほぼ全 vertex shader で同時 consume (= modelview/projection 経由の gl_Position 計算)
- 特に post-process shader (exoPostBaseV / snapshotFrameF / exoVignetteF) で 9 member 全 consume
- deferred lighting shader で `modelview_matrix` + `inv_proj` + `screen_res` 経由 G-buffer tc 復元

### §11.4 同 data source UBO (= 同 host data source から派生)

- **同 owner**: `LLRender` (= matrix stack + screen state)
- 他 UBO で同 owner = なし (= matrix data は本 UBO 専属)

### §11.5 dirty 連動 UBO (= 本 UBO dirty 時に同時 dirty)

- `LLRender::syncMatrices()` 経由で frame 開始時に 9 member 全更新 → frame 内 stable
- 連動 dirty UBO = なし (= FrameLights / FrameAtmosphere_Lighting は独立 data source、別 dirty trigger)

### §11.6 layout 共有関係

- 全 program 共通 `sAYAStandardLayout` 5-set V3a layout (= Phase 1.A 確立、全 UBO 共通)

### §11.7 bind 順序関係

- frame start で set=0 全 4 UBO 同時 bind (= `vkCmdBindDescriptorSets` 1 回呼出で set=0 帯全 binding 含む、`llvkloader.cpp:2839` literal)
- 本 UBO bind timing = frame start、frame 内 stable (= matrix stack 変化は memcpy + dirty.store のみ、bind 自体は再発火しない)

---

## §10. 不明事項 (= memory `feedback_admit_unknown` 遵守、推論で埋めない)

本 file 起案時点で実コード調査で確定できなかった項目:

1. **`screen_res` setter call site** = grep で uniform2f("screen_res", ...) 直接 hit せず、`LLShaderMgr::DEFERRED_SCREEN_RES` 経由汎用 setter 経路想定、pipeline.cpp 内 grep 詳細要
2. **`env_mat` setter call site** = `LLShaderMgr::DEFERRED_ENV_MAT` 経由想定、`llrender.cpp:1279` literal 注記「the shaders don't actually reference anything beyond texture_matrix0/1 outside of terrain rendering」と矛盾しないか確認要
3. **`last_modelview_matrix` setter call site** = per-frame で前 frame modelview 保存 (= velocity buffer 用)、`pipeline.cpp` 内 motion blur / velocity 経路 setter site 特定要
4. **per-shader UBO block 拡大対象 file 全列挙** = grep で `modelview_matrix` 参照 50+ file 中、UBO block 宣言済 5+ 件 vs raw uniform のみ 45+ 件の分類要
5. **`FRAMES_IN_FLIGHT` 定数値** = `llvkloader.cpp:4112` literal、cold launch 検証時に triple-buffer 経路で per-frame UboInstance offset 切替詳細 verify 要
6. **`flushFrameUbos` 内 GPU upload 経路** = `vkCmdUpdateBuffer` vs mapped pointer + memory barrier の選択、`llvkloader.cpp:5117` 詳細実装 verify 要
7. **PER_FRAME `writeFrameUbo` の dirty 判定単位** = block_hash 単独 key (= `sFrameUboInstances` 同) と整合確認要、複数 member 同時更新時の dirty 重複排除有無

= 上記 7 項目は本 UBO file 完成時に grep + Read で逐次解消、確定後に「不明」記載削除 + 確定 literal 追記。

---

## §12. Phase 2 sub-work 進捗 (= WORK_ORDER.md §3.2.1 同期)

**Layer**: L1b-1 (= per-frame matrix fundamental + per-shader UBO block 50+ file 拡大)
**status**: **起案済** (= 2026-06-06 C-4、設計・工程 doc 化完了、実装着手前)
**詳細・最新版**: `docs/specs/ayastorm-r41-gl-removal/design/ubo/WORK_ORDER.md §3.2.1` (= single source of truth)

**sub-work 7 dim 要点**:
- **(1) 前提条件**: L0-1 dispatch (= set=0 binding=0 固定) + L0-3 per-shader UBO block 拡大方式 + L0-4 cadence
- **(2) 不明事項**: screen_res / env_mat / last_modelview_matrix setter 3 件 [要追加調査] / per-shader 拡大対象 50+ file 列挙 [要追加調査] / FRAMES_IN_FLIGHT triple-buffer verify / flushFrameUbos GPU upload 経路 [要 verify]
- **(3) 調査手法**: D1 (setter Grep 3 件) + D2 (syncMatrices + per-shader UBO block 既存 8 file pattern) + D4 (50+ file 拡大対象列挙)
- **(4) 設計 task**: per-frame cadence triple-buffer (= 既通電) / 既存 31 setter PER_FRAME case (= 既通電) / `flushFrameUbos()` frame start (= 既通電) / **本 phase 主作業 = 50+ shader file `#ifdef LL_VULKAN_GLSL` block 追加 + `#else` raw uniform 維持** (= L0-3 include header / inject 方式)
- **(5) 工程**: trace 順 L1b 1 件目 (= matrix fundamental)、工数 **L** (= 1 日 +)、L1b-2 並列可
- **(6) A 確定**: per-shader UBO block 拡大 50+ file 全件完了 + cold launch + Vulkan validation 0 + AYA live verify (= 全 deferred/forward pass 描画既存と同一、**visual regression ゼロ §5.4**、一括 verify)
- **(7) 4 原則 gate**: 全 ✅、原則 4 = `#ifdef LL_VULKAN_GLSL` gate で全 file `#else` raw uniform 維持 (**重要 gate**、L0-3 protocol-C 整合)

**関連**: L0-1 + L0-3 + L0-4 (= WORK_ORDER §2) / §5.4 visual regression policy / FrameLights (= L1b-2 同 cadence cluster)
