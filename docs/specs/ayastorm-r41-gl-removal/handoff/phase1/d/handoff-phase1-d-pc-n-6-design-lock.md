# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.D **PC-N-6 design-lock complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: Phase 1.D 内 1st sub-step (PC-N-6 = stub vertex buffer Vulkan 経路通電、(N6-1) B 採用 = file-static `sGltfStubVertexBuffer` を VMA buffer 経由 allocate + initial upload + `vkCmdBindVertexBuffers` 配線 + 別 pipeline 新設 `sGltfStubAssetPipeline` で `sAvatarBonePipeline` 不変温存 + `AYAGltfStubVertexBufferEnabled` 段階 cvar) の **design-lock 完了 marker** = ambiguity (N6-1)..(N6-13) 13 件 全 AYA literal「すべて推奨でお願いします」record (2026-06-05) + 実装計画 (a)-(g) 7 step 分解 + Exit Criteria 10 項明文化。`indra/` 改変 0 件 (= `feedback_design_phase_no_code_write` 厳格遵守、`feedback_ubo_migration_one_at_a_time` 厳格遵守)。

> **本 doc 位置付け**: Phase 1.D decomposition design-lock (= `handoff-...-phase1-d-decomposition-design-lock.md`) で確定された PC-N-6..PC-N-10 5 sub-step のうち **1st sub-step (PC-N-6)** の **詳細実装 design-lock** 起案。PC-N-7..PC-N-10 は別 session で個別 design-lock 起案 (`feedback_ubo_migration_one_at_a_time` 厳格遵守)。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「r41 Phase 1.D PC-N-6 design-lock 着手お願いします」literal 受領 (2026-06-05、Phase 1.D decomposition design-lock commit `901d51d3ac` 後の継続 session = 別 session の fresh context) + 必読 1 件 (Phase 1.D decomposition design-lock doc) Read + pinpoint reference 7 件 (= `recordGltfAssetDraw` 現状 + LL::GLTF::Primitive 構造体 + LL::GLTF::Asset uploadTransforms + GLTFSceneManager::render + sAvatarBonePipeline pipeline 構築 + bindVertexBufferVk wrap + cross-platform spec §6) Read → Explore agent 経由 10 項現状調査 → ambiguity (N6-1)..(N6-13) 13 件 + 推奨案 + 採用根拠提示 → AYA literal「すべて推奨でお願いします」一括確認受領 (2026-06-05) で本 design-lock doc 起案。

**PC-N-6 literal scope** (= (N6-1) B + (N6-2) A + (N6-3) A + (N6-4) B + (N6-5) B + (N6-6) A + (N6-7) B + (N6-8) A + (N6-9) C + (N6-10) B 採用後):

1. **`sGltfStubVertexBuffer` file-static VMA buffer 新設** = LLVKLoader anonymous namespace 内、`VkBuffer + VmaAllocation + void* mapped + VkDeviceSize size` 4 件、PC-N-5 `sGltfStubSkin` sentinel と並列 file-static lifecycle ((N6-4) B + (N6-6) A)
2. **stub vertex data initial upload** = `static const F32 sGltfStubVertexData[9] = { position vec3 × 3, CCW triangle }` (9 float = 36 B)、`VMA_MEMORY_USAGE_AUTO + VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT + VMA_ALLOCATION_CREATE_MAPPED_BIT` (= 永続 mapped)、initVulkan で 1 回 memcpy ((N6-3) A + (N6-5) B + (N6-6) A)
3. **`sGltfStubAssetPipeline` 別 pipeline 新設** = `sAvatarBonePipeline` 同形 + sky smoke shader module 流用 + vertex input state 拡張 (= 1 binding (= binding=0, stride=12 B, INPUT_RATE_VERTEX) + 1 attribute (= location=0, binding=0, format=R32G32B32_SFLOAT, offset=0)) + `sAvatarBonePipeline` 不変温存 ((N6-7) B + (N6-8) A)
4. **shutdownVulkan vmaDestroyBuffer** = `sGltfStubVertexBuffer` 対称 lifecycle + `sGltfStubAssetPipeline` vkDestroyPipeline ((N6-6) A 整合)
5. **`recordGltfAssetDraw` 改変** = signature 不変、内部で `AYAGltfStubVertexBufferEnabled` cvar 分岐、cvar=true 時のみ別経路発火 = `vkCmdBindPipeline(sGltfStubAssetPipeline)` + `bindVertexBufferVk(sGltfStubVertexBuffer, 0)` + 既存 UBO bind sequence 維持 + `vkCmdDraw(sGltfStubVertexCount, 1, 0, 0)` (= 任意 N、初期値 3) ((N6-2) A + (N6-8) A + (N6-9) C + (N6-10) B)
6. **`AYAGltfStubVertexBufferEnabled` cvar 新設** = settings.xml Boolean default false Persist=1、`PC-N-5 AYAGltfStubDrawEnabled` 隣に配置 ((N6-9) C)
7. **build verify** = llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131 (Linux primary) ((N6-11) A)

**Phase 1.D 境界**: PC-N-6 = Phase 1.D 1st sub-step、PC-N-7..PC-N-10 残 4 sub-step は別 session で個別 design-lock + 実装、Phase 1.D complete = PC-N-10 完了時 marker。

---

## §1. 必読 1 件 + pinpoint reference

**次 session 必読 (= PC-N-6 実装 phase 着手前)**:

1. **本 PC-N-6 design-lock doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-d-pc-n-6-design-lock.md`

**pinpoint reference (実装時に必要分のみ)**:

- **Phase 1.D decomposition design-lock doc**: `handoff-...-phase1-d-decomposition-design-lock.md` = PC-N-6 scope §4.1 + 依存関係 §2.3 + (D-7) A + (D-12) A 確認 record
- **PC-N-5 complete doc**: `handoff-...-phase1-d-pc-n-5-complete.md` = `recordGltfAssetDraw` 現状 (5479-5591) + `sGltfStubSkin` sentinel pattern + `AYAGltfStubDrawEnabled` cvar 配置 site
- **`recordGltfAssetDraw` 現状**: `indra/llrender/llvkloader.cpp:5479-5591` = PC-N-5 stub (3 vertex shader generate、empty vertex input)、PC-N-6 で cvar 分岐 + 別 pipeline + vertex buffer bind 追加
- **`sAvatarBonePipeline` 構築**: `indra/llrender/llvkloader.cpp:3300-3395` (= `createAvatarBonePipeline`)、PC-N-6 で同形 + vertex input state 追加した `sGltfStubAssetPipeline` 新設関数 `createGltfStubAssetPipeline` 同形配置
- **`bindVertexBufferVk` / `bindIndexBufferVk` wrap**: `indra/llrender/llvkloader.cpp:6001-6028` = PC-N-6 が `bindVertexBufferVk` 初 caller、`bindIndexBufferVk` は PC-N-7 持越し
- **PC-N-5 (e) cvar 発火 hook**: `llvkloader.cpp:5714-5734` (= `recordAvatarPlaceholderDraw` 末尾 `AYAGltfStubDrawEnabled` cvar gate)、PC-N-6 は `recordGltfAssetDraw` **内側** で別 cvar 分岐 = 発火 hook 自体は PC-N-5 経由維持
- **PC-N-5 stub asset draw**: `llvkloader.cpp:5494-5589` (= `recordGltfAssetDraw` 関数 body)、PC-N-6 で cvar 分岐追加 + 既存 PC-N-5 経路温存
- **`settings.xml`**: `AYAGltfStubDrawEnabled` 既配置 (PC-N-5) + 隣に `AYAGltfStubVertexBufferEnabled` 新設
- **LL::GLTF::Primitive 構造体**: `indra/newview/gltf/primitive.h:58-86` = mPositions/mNormals/.../mIndexArray + mVertexBuffer LLPointer backpointer (= PC-N-7..PC-N-8 で参照、PC-N-6 では使用なし)
- **VMA helper 前例**: `indra/llrender/llvkloader.cpp:2050-2089` (= `uploadPlaceholderWhiteSmoke` の staging buffer 配置)、PC-N-6 は host-visible mapped 経路ゆえ簡略化
- **cross-platform spec §6 PC-N-6 行**: `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md:107` (= 本 commit 更新済)
- **GATE-B literal**: memory `project_r41_phase1b_vulkan_host_gate` (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件維持)
- **設計原則**: memory `project_ayastorm_r41_design_principles` (1) Upstream OpenGL 取り込みやすさ維持 + (2) Core プロセス分散実現

---

## §2. 現状調査結果 (= Explore agent 10 項要約 + pinpoint Read 補強)

### §2.1 現 code 状態 (indra/)

| # | 項目 | file:line | 現状要約 |
|---|------|-----------|---------|
| 1 | `recordGltfAssetDraw` body | `llvkloader.cpp:5494-5589` | PC-N-5 完了、`vkCmdDraw(3,1,0,0)` shader generate、empty vertex input、`AYAGltfStubDrawEnabled` cvar 発火 hook は **PC-N-5 (e) `recordAvatarPlaceholderDraw` 末尾**で配置 |
| 2 | `sAvatarBonePipeline` 構築 | `llvkloader.cpp:3300-3395` (`createAvatarBonePipeline`) | sky smoke shader 流用 + `vi = {}` empty vertex input state + `sAvatarBoneLayout = sAYAStandardLayout` alias、PC-N-6 で同形 `createGltfStubAssetPipeline` 新設 |
| 3 | `sAvatarBonePipeline` vertex input state | `llvkloader.cpp:3330-3331` | `vi.sType` 設定のみ、binding/attribute description 0 件 = empty vertex input、shader 内 `gl_VertexIndex` で generate |
| 4 | `bindVertexBufferVk` wrap | `llvkloader.cpp:6003-6016` | PC-N-6 が初 caller、`firstBinding=0` + `bindingCount=1` で 1 buffer bind、VkBuffer + offset 受領 |
| 5 | `bindIndexBufferVk` wrap | `llvkloader.cpp:6018-6028` | PC-N-7 持越し、PC-N-6 では caller 追加なし |
| 6 | `sGltfStubSkin` sentinel + lifecycle | `llvkloader.cpp:582-601` + `initVulkan`/`shutdownVulkan` | PC-N-5 file-static address-only sentinel + register/unregister 対称、PC-N-6 vertex buffer は同形 file-static + VMA allocate/destroy 対称配置 |
| 7 | `AYAGltfStubDrawEnabled` cvar | `settings.xml:10405-10420` + `llvkloader.cpp:5714-5734` | PC-N-5 Boolean default false Persist=1、PC-N-6 で新 cvar `AYAGltfStubVertexBufferEnabled` 隣に配置 |
| 8 | `recordAvatarPlaceholderDraw` 末尾 hook | `llvkloader.cpp:5714-5734` | PC-N-5 (e) で `AYAGltfStubDrawEnabled` cvar gate 経由 `recordGltfAssetDraw(cmd_buf)` 並走発火、本 hook は PC-N-6 で改変なし (= `recordGltfAssetDraw` 内側で別 cvar 分岐) |

### §2.2 LL::GLTF 構造体 (PC-N-6 では参照のみ、改変なし)

| # | 項目 | file:line | PC-N-6 関連性 |
|---|------|-----------|--------------|
| 9 | `LL::GLTF::Primitive` raw vertex data | `gltf/primitive.h:58-66` | `mPositions / mNormals / mTangents / mTexCoords0/1 / mWeights / mJoints / mColors / mIndexArray` separate per-attribute、**PC-N-6 では参照なし**、PC-N-7..PC-N-8 で参照 |
| 10 | `LL::GLTF::Asset::uploadTransforms` | `gltf/asset.cpp:164-232` | PC-7γ-3 dual-write infrastructure (= `registerAssetUbo` + `writeAssetUbo`)、PC-N-6 では参照なし、PC-N-8 で vertex buffer 同形 infrastructure 拡張時に参照 |

### §2.3 PC-N-6 内部の step 依存関係

```
(a) sGltfStubVertexBuffer file-static declare
    ↓
(b) sGltfStubAssetPipeline 新設関数 createGltfStubAssetPipeline 配置
    ↓ (vertex input state 確定後 pipeline compile 可能)
(c) initVulkan 内 VMA allocate + initial upload + createGltfStubAssetPipeline call
    ↓
(d) shutdownVulkan 内 vmaDestroyBuffer + vkDestroyPipeline 対称配置
    ↓
(e) recordGltfAssetDraw 改変 = cvar 分岐 + 別 pipeline bind + bindVertexBufferVk + vkCmdDraw(N) (cvar gate)
    + AYAGltfStubVertexBufferEnabled cvar 新設 (settings.xml + llvkloader.cpp 読込)
    ↓
(f) build verify = llrender + warning 0 + TUT 11+10+13 + codegen 131/131
    ↓
(g) handoff complete doc 起案 + Exit Criteria 10 項 self-verify + AYA commit 指示後 commit
```

---

## §3. ambiguity (N6-1)..(N6-13) 13 件 AYA literal「すべて推奨でお願いします」record (2026-06-05) + 採用根拠

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N6-1) | PC-N-6 literal scope 最小単位 | **B**: stub 経路維持 = file-static stub vertex data + VMA buffer 経由 bind | OK (2026-06-05) | PC-N-5 第 2 sentinel pattern 同形、`feedback_ubo_migration_one_at_a_time` 厳格遵守 = 1 sub-step 1 機能、A は実 LL::GLTF::Asset 入手経路 + ownership lifecycle 不確定で scope 肥大、C は caller 無し helper 配置のみで通電なし = Phase 1.D 着手起点原則 (= 必ず通電 1 stub) 違反 |
| (N6-2) | `recordGltfAssetDraw` signature | **A**: 不変 = `recordGltfAssetDraw(VkCommandBuffer cmd_buf)` | OK (2026-06-05) | (N6-1) B 採用ゆえ stub vertex data は file-static literal、Asset* 引数不要、PC-N-8 で signature 拡張持越し |
| (N6-3) | Stub vertex data source | **A**: file-static `const F32 sGltfStubVertexData[9]` = position vec3 × 3 | OK (2026-06-05) | 最小限通電原則、shader (sAvatarBonePipeline の sky smoke shader 流用) は vertex attribute 実 consumption 不要 = vkCmdBindVertexBuffers 経路通電の事実確立、normal/uv は PC-N-6 scope 外 |
| (N6-4) | Vertex buffer 配置 site | **B**: LLVKLoader 内 file-static stub = `sGltfStubVertexBuffer` + VMA allocation + lifecycle (initVulkan / shutdownVulkan 対称) | OK (2026-06-05) | (N6-1) B + (N6-2) A 採用整合、PC-N-5 `sGltfStubSkin` sentinel 並列 lifecycle、PC-N-7..PC-N-10 で実 Asset 経路移行時に asset.cpp 側 infrastructure 新設、layer 違反なし |
| (N6-5) | VMA usage flag | **B**: `VMA_MEMORY_USAGE_AUTO` + `VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT` + `VMA_ALLOCATION_CREATE_MAPPED_BIT` (host-visible 永続 mapped) | OK (2026-06-05) | stub vertex data 36 B 極小、staging buffer overkill、PC-7β UBO ring buffer 同形 pattern、initVulkan で 1 回 memcpy で完結 |
| (N6-6) | Vertex buffer cadence | **A**: 1 回 initVulkan で allocate + initial upload (file-static、shutdown で destroy) | OK (2026-06-05) | stub vertex data 変更なし固定 literal、ring buffer overkill、PC-N-5 `sGltfStubSkin` sentinel lifecycle 整合 |
| (N6-7) | `sAvatarBonePipeline` vertex input state 改変要否 | **B**: 別 pipeline 新設 = `sGltfStubAssetPipeline` = sAvatarBonePipeline 同形 + vertex input state 追加 | OK (2026-06-05) | PC-N-5 stub draw 経路を破壊しない、live A/B 経路独立、PC-N-9 並走 Vulkan dispatcher 思想 ((D-5) C) 整合、shader 改変ゼロ維持、`sAvatarBonePipeline` は H10-A avatar bone placeholder 専用に温存 |
| (N6-8) | PC-N-5 stub draw 経路との関係 | **A**: 並走維持 = PC-N-5 経路 (shader generate) + PC-N-6 経路 (vertex buffer bind) 別 cvar gate で並走 | OK (2026-06-05) | (N6-7) B 採用整合、PC-N-5 sentinel + recordGltfAssetDraw lifecycle 不変、PC-N-6 は第 2 stub draw 経路として並走、PC-N-10 で両 stub 経路 cleanup、`feedback_visual_decisions_need_live_ab` 整合 |
| (N6-9) | `AYAGltfRealDrawEnabled` cvar 新設 timing | **C**: `AYAGltfStubVertexBufferEnabled` 段階 cvar 新設 | OK (2026-06-05) | PC-N-5 `AYAGltfStubDrawEnabled` 同形 pattern、stub vertex buffer 経路の独立 gate、PC-N-7..PC-N-10 で段階遷移、`AYAGltfRealDrawEnabled` literal は PC-N-9 で実 LL::GLTF::Asset 経由 draw 時に新設 |
| (N6-10) | vkCmdDraw 実 vertex count | **B**: 任意 N (`sGltfStubVertexCount` const、初期値 3) | OK (2026-06-05) | PC-N-7 index buffer + vkCmdDrawIndexed 移行時 extensibility、(N6-3) A position vec3 × 3 整合 file-static initial、PC-N-9 で実 vertex count 注入時に rename |
| (N6-11) | build verify scope | **A**: llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131 (Linux primary) | OK (2026-06-05) | PC-8 Linux primary marker 採用後標準、(D-9) A 整合、cold launch literal は AYA 環境依存ゆえ各 sub-step 個別判断 |
| (N6-12) | Exit Criteria 項目数 | **A**: 10 項 (PC-N-5 同形 template) | OK (2026-06-05) | PC-N-5 / PC-N-1..PC-N-4 同形 template、consistency |
| (N6-13) | implementation step 分解粒度 | **A**: 7 step (a)-(g) (PC-N-5 同形 template) | OK (2026-06-05) | PC-N-5 step (a)-(g) 同形 template、consistency |

---

## §4. 実装計画 (a)-(g) 7 step ((N6-13) A 採用)

> **注**: 本 §4 は実装 phase 着手用設計、本 design-lock phase では **`indra/` 改変 0 件** (= `feedback_design_phase_no_code_write` 厳格遵守)。

### §4.1 (a) `sGltfStubVertexBuffer` file-static declare

**配置 site**: `indra/llrender/llvkloader.cpp` anonymous namespace 内、`sGltfStubSkin` sentinel 直後 (line 602 近傍)。

**declare**:

```cpp
// <AYAstorm r41 PC-N-6 (a)> GLTF stub vertex buffer file-static + VMA allocation。
//   (N6-4) B + (N6-6) A 採用 = LLVKLoader 内 file-static + 1 回 initVulkan 配置 +
//   shutdownVulkan 対称破棄。PC-N-5 sGltfStubSkin sentinel と並列 lifecycle。
//
//   (N6-3) A 採用 = position vec3 × 3 hardcoded CCW triangle (= 9 float = 36 B、
//   stride=12 B、binding=0、attribute=0、location=0、format=R32G32B32_SFLOAT)。
//   shader 内 attribute 実 consumption は不要 (sky smoke shader 流用 = gl_VertexIndex
//   経路設計)、vkCmdBindVertexBuffers 経路通電の事実確立が scope。
VkBuffer       sGltfStubVertexBuffer       = VK_NULL_HANDLE;
VmaAllocation  sGltfStubVertexAllocation   = VK_NULL_HANDLE;
void*          sGltfStubVertexMapped       = nullptr;
constexpr U32  sGltfStubVertexCount        = 3u;  // (N6-10) B 採用 = 任意 N 経路、初期値 3
constexpr U32  sGltfStubVertexStride       = 12u; // (N6-3) A position vec3 = 3 × sizeof(F32)
constexpr U32  sGltfStubVertexBufferSize   = sGltfStubVertexCount * sGltfStubVertexStride; // 36 B
// </AYAstorm r41 PC-N-6 (a)>
```

**stub vertex data const**:

```cpp
// <AYAstorm r41 PC-N-6 (a)> stub vertex data initial literal (= CCW triangle、
//   NDC 内中央配置、offscreen FBO 内可視範囲)。
static const F32 sGltfStubVertexData[9] = {
    // position vec3 × 3 (= NDC CCW triangle、shader generate stub と同形視覚)
    -0.5f, -0.5f, 0.0f,  // v0
     0.5f, -0.5f, 0.0f,  // v1
     0.0f,  0.5f, 0.0f,  // v2
};
static_assert(sizeof(sGltfStubVertexData) == sGltfStubVertexBufferSize,
              "PC-N-6 (a) sGltfStubVertexData size mismatch sGltfStubVertexBufferSize");
// </AYAstorm r41 PC-N-6 (a)>
```

### §4.2 (b) `sGltfStubAssetPipeline` 新設関数 `createGltfStubAssetPipeline` 配置

**配置 site**: `indra/llrender/llvkloader.cpp` anonymous namespace 内、`createAvatarBonePipeline` (line 3300-3395) 直後。

**pipeline storage**:

```cpp
// <AYAstorm r41 PC-N-6 (b)> GLTF stub asset pipeline = sAvatarBonePipeline 同形 +
//   vertex input state 追加 ((N6-7) B 採用)。sAvatarBonePipeline 不変温存 = PC-N-5
//   stub draw 経路 (shader generate 3 vertex) を break しない、live A/B 経路独立。
VkPipeline sGltfStubAssetPipeline = VK_NULL_HANDLE;
// </AYAstorm r41 PC-N-6 (b)>
```

**関数 declare** (createAvatarBonePipeline 同形、改変点のみ抜粋):

```cpp
// <AYAstorm r41 PC-N-6 (b)> createGltfStubAssetPipeline = createAvatarBonePipeline
//   同形 + vertex input state 拡張。shader = sSkySmokeVertModule + sSkySmokeFragModule
//   流用 (shader 改変ゼロ)、layout = sAYAStandardLayout alias 共用 (set=0/1a/1b/2/3 V3a)。
//
//   vertex input state 拡張: 1 binding (= binding=0, stride=12 B, INPUT_RATE_VERTEX) +
//   1 attribute (= location=0, binding=0, format=R32G32B32_SFLOAT, offset=0)。
//   shader 内 layout(location=0) in vec3 が declared でなくても valid Vulkan (=
//   driver implementation-defined unused、PSO compile success)。
bool createGltfStubAssetPipeline()
{
    if (!sDeviceLimits.pushDescriptorSupported)
    {
        return true;
    }
    if (sSkySmokeVertModule == VK_NULL_HANDLE || sSkySmokeFragModule == VK_NULL_HANDLE)
    {
        LL_WARNS("Vulkan") << "createGltfStubAssetPipeline: sky smoke shader modules not ready" << LL_ENDL;
        return false;
    }
    if (sAYAStandardLayout == VK_NULL_HANDLE)
    {
        LL_WARNS("Vulkan") << "createGltfStubAssetPipeline: sAYAStandardLayout not initialized" << LL_ENDL;
        return false;
    }

    VkPipelineShaderStageCreateInfo stages[2] = {};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = sSkySmokeVertModule;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = sSkySmokeFragModule;
    stages[1].pName  = "main";

    // PC-N-6 (b) vertex input state 拡張 (= sAvatarBonePipeline empty vi との差分)。
    VkVertexInputBindingDescription vbd = {};
    vbd.binding   = 0;
    vbd.stride    = sGltfStubVertexStride; // 12 B = position vec3
    vbd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vad = {};
    vad.location = 0;
    vad.binding  = 0;
    vad.format   = VK_FORMAT_R32G32B32_SFLOAT;
    vad.offset   = 0;

    VkPipelineVertexInputStateCreateInfo vi = {};
    vi.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount   = 1;
    vi.pVertexBindingDescriptions      = &vbd;
    vi.vertexAttributeDescriptionCount = 1;
    vi.pVertexAttributeDescriptions    = &vad;

    // (以降 ia / vp / rs / ms / ds / cb / ci は createAvatarBonePipeline と完全同一)
    // ... (createAvatarBonePipeline body line 3333-3387 と同形 copy) ...
    ci.pVertexInputState = &vi;
    ci.layout            = sAYAStandardLayout;
    // ...

    if (!compileGraphicsPipeline(ci, sGltfStubAssetPipeline))
    {
        LL_WARNS("Vulkan") << "GLTF stub asset graphics pipeline compile failed" << LL_ENDL;
        return false;
    }
    LL_INFOS("Vulkan") << "GLTF stub asset PSO compiled (set_layouts[5] = V3a + vertex input "
                          "(1 binding, 1 attribute = position R32G32B32_SFLOAT), shader = sky smoke 流用、"
                          "shader 改変ゼロ、sAvatarBonePipeline 並走温存)" << LL_ENDL;
    return true;
}
// </AYAstorm r41 PC-N-6 (b)>
```

### §4.3 (c) initVulkan VMA allocate + initial upload + createGltfStubAssetPipeline call

**配置 site**: `indra/llrender/llvkloader.cpp` `initVulkan` 内、`createAvatarBonePipeline` call 直後 + PC-N-5 sGltfStubSkin registerSkinUbo block 近傍。

**code**:

```cpp
// <AYAstorm r41 PC-N-6 (c)> GLTF stub vertex buffer VMA allocate + initial upload。
//   (N6-5) B 採用 = host-visible 永続 mapped、(N6-6) A 採用 = 1 回 initVulkan 配置。
//   sAllocator nullptr guard で MUSEUBO-A graceful degrade (= cvar=false default で no-op)。
if (sAllocator != VK_NULL_HANDLE && sGltfStubVertexBuffer == VK_NULL_HANDLE)
{
    VkBufferCreateInfo buf_ci = {};
    buf_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_ci.size        = sGltfStubVertexBufferSize;
    buf_ci.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_ci = {};
    alloc_ci.usage = VMA_MEMORY_USAGE_AUTO;
    alloc_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                   | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo alloc_info = {};
    if (vmaCreateBuffer(sAllocator, &buf_ci, &alloc_ci,
                        &sGltfStubVertexBuffer,
                        &sGltfStubVertexAllocation,
                        &alloc_info) == VK_SUCCESS)
    {
        sGltfStubVertexMapped = alloc_info.pMappedData;
        if (sGltfStubVertexMapped)
        {
            memcpy(sGltfStubVertexMapped, sGltfStubVertexData, sGltfStubVertexBufferSize);
            LL_INFOS("Vulkan") << "PC-N-6 (c) sGltfStubVertexBuffer allocated + initial upload: "
                                  "size=" << sGltfStubVertexBufferSize
                               << " B, vertices=" << sGltfStubVertexCount
                               << ", stride=" << sGltfStubVertexStride
                               << ", mapped=" << sGltfStubVertexMapped << LL_ENDL;
        }
        else
        {
            LL_WARNS_ONCE("Vulkan") << "PC-N-6 (c) sGltfStubVertexBuffer mapped=nullptr "
                                       "(VMA host-visible mapped flag fail、cvar=true 時 silent no-op)" << LL_ENDL;
        }
    }
    else
    {
        LL_WARNS_ONCE("Vulkan") << "PC-N-6 (c) sGltfStubVertexBuffer vmaCreateBuffer fail "
                                   "(cvar=true 時 silent no-op)" << LL_ENDL;
    }
}

// PC-N-6 (c) sGltfStubAssetPipeline 配置 (= createAvatarBonePipeline 直後同位置)。
if (!createGltfStubAssetPipeline())
{
    LL_WARNS("Vulkan") << "PC-N-6 (c) createGltfStubAssetPipeline fail (= cvar=true 時 silent no-op)" << LL_ENDL;
}
// </AYAstorm r41 PC-N-6 (c)>
```

### §4.4 (d) shutdownVulkan vmaDestroyBuffer + vkDestroyPipeline 対称配置

**配置 site**: `indra/llrender/llvkloader.cpp` `shutdownVulkan` 内、bulk teardown 前 (= PC-N-5 sGltfStubSkin unregister 近傍)。

**code**:

```cpp
// <AYAstorm r41 PC-N-6 (d)> GLTF stub vertex buffer + pipeline 対称破棄
//   (= (N6-6) A 採用 = 1 回 initVulkan 配置 / 1 回 shutdownVulkan 破棄)。
if (sGltfStubAssetPipeline != VK_NULL_HANDLE && sDevice != VK_NULL_HANDLE)
{
    vkDestroyPipeline(sDevice, sGltfStubAssetPipeline, nullptr);
    sGltfStubAssetPipeline = VK_NULL_HANDLE;
}

if (sGltfStubVertexBuffer != VK_NULL_HANDLE && sAllocator != VK_NULL_HANDLE)
{
    vmaDestroyBuffer(sAllocator, sGltfStubVertexBuffer, sGltfStubVertexAllocation);
    sGltfStubVertexBuffer     = VK_NULL_HANDLE;
    sGltfStubVertexAllocation = VK_NULL_HANDLE;
    sGltfStubVertexMapped     = nullptr;
}
// </AYAstorm r41 PC-N-6 (d)>
```

### §4.5 (e) `recordGltfAssetDraw` 改変 + cvar gate 配線

**配置 site**: `indra/llrender/llvkloader.cpp:5494-5589` `recordGltfAssetDraw` 関数 body 内、`vkCmdBindPipeline(sAvatarBonePipeline)` (line 5508) 直前で cvar 分岐挿入。

**signature**: 不変 ((N6-2) A) = `void recordGltfAssetDraw(VkCommandBuffer cmd_buf)`。

**改変方針** ((N6-8) A 並走維持):

```cpp
// PC-N-5 既存 guard (line 5499-5506) 維持。
if (cmd_buf == VK_NULL_HANDLE || ...) { return; }

// <AYAstorm r41 PC-N-6 (e)> stub vertex buffer 経路の cvar 分岐
//   ((N6-8) A 並走維持 + (N6-9) C `AYAGltfStubVertexBufferEnabled` 段階 cvar)。
//   cvar=true 時のみ別 pipeline (sGltfStubAssetPipeline) + vertex buffer bind 経路発火、
//   cvar=false default 時は PC-N-5 stub draw (sAvatarBonePipeline + shader generate 3
//   vertex) 経路継続 = MUSEUBO-A 整合 + live A/B 経路独立。
static LLCachedControl<bool> gltf_stub_vb_enabled(
    gSavedSettings, "AYAGltfStubVertexBufferEnabled", false);

if (gltf_stub_vb_enabled
    && sGltfStubAssetPipeline != VK_NULL_HANDLE
    && sGltfStubVertexBuffer  != VK_NULL_HANDLE)
{
    // PC-N-6 (e) 別 pipeline bind + vertex buffer bind 経路。UBO bind sequence は
    //   PC-N-5 と完全同形 (= writeDrawUbo + writeSkinUbo + flushSkinUbos + bindV3aRigged)、
    //   pipeline bind と vkCmdBindVertexBuffers のみ差分。
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sGltfStubAssetPipeline);

    // PC-N-5 と同形 per-draw UBO 配線 (= line 5510-5522 copy)。
    static const U8 stub_draw_zero_buf[256] = {};
    U32 stub_dynamic_offset = 0u;
    LLVKLoader::writeDrawUbo(
        ubo::block_hash::PerDrawUBO_LightParams,
        /*offset=*/0u,
        stub_draw_zero_buf,
        sizeof(stub_draw_zero_buf),
        stub_dynamic_offset);
    const U32 stub_dynamic_offsets[V3A_DRAW_SET_BINDINGS] = {
        stub_dynamic_offset, stub_dynamic_offset, stub_dynamic_offset, stub_dynamic_offset,
    };

    // PC-N-5 と同形 per-Skin UBO 配線 (= line 5524-5552 copy)。
    static const F32 stub_identity_skin_buf[64] = { /* identity mat4 + zero padding 256 B */ };
    LLVKLoader::writeSkinUbo(
        sGltfStubSkin,
        ubo::block_hash::Skin_GLTFJoints,
        /*offset=*/0u,
        reinterpret_cast<const U8*>(stub_identity_skin_buf),
        sizeof(stub_identity_skin_buf));
    LLVKLoader::flushSkinUbos(sGltfStubSkin);
    bindV3aRigged(cmd_buf, sFrameIndex, stub_dynamic_offsets);

    // PC-N-5 と同形 push constant (= line 5556-5569 copy)。
    const float stub_identity_modelview[16] = { /* identity */ };
    vkCmdPushConstants(cmd_buf, sAvatarBoneLayout, VK_SHADER_STAGE_VERTEX_BIT,
                       /*offset=*/0, /*size=*/64, stub_identity_modelview);

    // PC-N-6 (e) vertex buffer bind + vkCmdDraw(N) (= PC-N-6 核心差分)。
    LLVKLoader::bindVertexBufferVk(cmd_buf, sGltfStubVertexBuffer, /*offset=*/0);
    vkCmdDraw(cmd_buf, sGltfStubVertexCount, 1, 0, 0);

    // PC-N-6 (e) first-fire LL_INFOS marker。
    static std::atomic<bool> s_first_pcn6_fire{true};
    if (s_first_pcn6_fire.exchange(false, std::memory_order_acq_rel))
    {
        LL_INFOS("Vulkan") << "PC-N-6 (e) GLTF stub vertex buffer draw 通電 (first fire): "
                              "sGltfStubAssetPipeline + sGltfStubVertexBuffer (= position vec3 × "
                           << sGltfStubVertexCount << " hardcoded CCW triangle、VMA host-visible "
                              "mapped) = bindV3aRigged → bindVertexBufferVk → vkCmdDraw("
                           << sGltfStubVertexCount << ",1,0,0)、sAvatarBonePipeline 並走温存" << LL_ENDL;
    }
    return; // PC-N-5 経路 (shader generate) はスキップ = 別 cvar gate で並走分離
}
// </AYAstorm r41 PC-N-6 (e)>

// PC-N-5 (b) 既存経路 (= sAvatarBonePipeline + shader generate 3 vertex、line 5508-5589) 不変。
vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sAvatarBonePipeline);
// ... (PC-N-5 既存 body 維持) ...
```

### §4.6 (e') `AYAGltfStubVertexBufferEnabled` cvar 新設 (settings.xml)

**配置 site**: `indra/newview/app_settings/settings.xml`、`AYAGltfStubDrawEnabled` (PC-N-5、line 10405-10420 近傍) 直後。

**XML**:

```xml
<key>AYAGltfStubVertexBufferEnabled</key>
<map>
  <key>Comment</key>
  <string>r41 PC-N-6 stub GLTF asset vertex buffer Vulkan draw 経路発火切替 (= AYAGltfStubDrawEnabled と並走の第 2 stub 経路、別 pipeline sGltfStubAssetPipeline + VMA host-visible mapped vertex buffer 経由、live A/B、PC-N-10 で deprecate 予定)</string>
  <key>Persist</key>
  <integer>1</integer>
  <key>Type</key>
  <string>Boolean</string>
  <key>Value</key>
  <integer>0</integer>
</map>
```

### §4.7 (f) build verify ((N6-11) A 採用)

実装 phase 末尾で取得予定:

- `make -j4 llrender` PASS
- ERROR 0 / WARNING 0
- `INTEGRATION_TEST_lluboringbuffer` 11/11
- `INTEGRATION_TEST_llassetubopool` 10/10
- `INTEGRATION_TEST_llpipelinecachestorage` 13/13
- codegen unittest 131/131

### §4.8 (g) handoff complete doc 起案

実装 phase 末尾で `handoff-...-phase1-d-pc-n-6-complete.md` 起案 + Exit Criteria 10 項 self-verify + AYA commit 指示後 commit。

### §4.9 GATE-B 整合 (memory `project_r41_phase1b_vulkan_host_gate`)

**`#ifdef LL_VULKAN_GLSL` 新規追加 0 件** = `AYAGltfStubVertexBufferEnabled` は `LLCachedControl<bool>` runtime cvar 経路ゆえ `#ifdef` 非依存 = GATE-B 違反なし。新 pipeline + 新 VMA buffer + 新 cvar 全 runtime gate のみ。

### §4.10 MUSEUBO-A 整合

`mUseUBO=false` default で既存 OpenGL 描画 100% 維持。`AYAGltfStubVertexBufferEnabled=false` default で本 PC-N-6 新経路発火なし = PC-N-5 完了状態 (= PC-N-5 stub draw 経路 cvar 経由) と機能等価。多段 graceful degrade:

1. `sAllocator nullptr` (= Vulkan 未初期化) → VMA allocate 自体スキップ
2. `sGltfStubVertexBuffer == VK_NULL_HANDLE` (= allocate fail) → cvar=true でも分岐内 nullptr guard で silent fall-through
3. `sGltfStubAssetPipeline == VK_NULL_HANDLE` (= pipeline compile fail) → 同上
4. `sGltfStubVertexMapped == nullptr` (= mapped fail) → memcpy スキップ + LL_WARNS_ONCE、bind は valid buffer ゆえ vkCmdDraw 自体は success (= zero-content draw)

### §4.11 設計原則整合 (memory `project_ayastorm_r41_design_principles`)

- **(1) Upstream OpenGL 取り込みやすさ維持** = `recordGltfAssetDraw` signature 不変 + 別 pipeline 新設 + `sAvatarBonePipeline` 不変 + shader 改変ゼロ + `GLTFSceneManager::render` 改変 0 件 = upstream LL からの取り込み時に call site 不変
- **(2) Core プロセス分散実現** = stub vertex buffer は file-static の 1 回配置ゆえ本 sub-step では並列化 baseline 拡張なし、PC-N-7 以降の per-Primitive vertex buffer ownership design で実現

---

## §5. PC-N-6 design-lock Exit Criteria 9 項

| # | Criteria |
|---|----------|
| (i) | PC-N-6 literal scope §0 明文化 ((N6-1) B + (N6-3) A + (N6-4) B + (N6-7) B + (N6-9) C 採用後の 7 件) |
| (ii) | 必読 1 件 §1 + pinpoint reference 13 件別記 |
| (iii) | ambiguity (N6-1)..(N6-13) 13 件 AYA literal「すべて推奨でお願いします」record (2026-06-05) §3 |
| (iv) | 採用根拠 13 件 §3 明文化 |
| (v) | 実装計画 (a)-(g) 7 step 分解 §4 + 各 step に具体 code stub example 添付 |
| (vi) | 実装 phase Exit Criteria 10 項 §6 明文化 |
| (vii) | GATE-B 整合 §4.9 + MUSEUBO-A 整合 §4.10 + 設計原則整合 §4.11 |
| (viii) | 想定改変 file 3 件 §6.2 明文化 |
| (ix) | `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合 (cross-platform spec §6 PC-N-6 行追記のみ) |

---

## §6. 実装 phase Exit Criteria 10 項 ((N6-12) A 採用)

| # | Criteria |
|---|----------|
| (i) | `sGltfStubVertexBuffer` file-static + VMA allocation declare 配置 (PC-N-6 (a) tag block) |
| (ii) | initVulkan VMA allocate + initial upload (= memcpy `sGltfStubVertexData` to mapped) 配線 (PC-N-6 (c) tag block) |
| (iii) | `sGltfStubAssetPipeline` 新設関数 `createGltfStubAssetPipeline` 配置 + initVulkan call (PC-N-6 (b) + (c) tag block) |
| (iv) | shutdownVulkan 内 `vmaDestroyBuffer` + `vkDestroyPipeline` 対称破棄配線 (PC-N-6 (d) tag block) |
| (v) | `recordGltfAssetDraw` 内 `AYAGltfStubVertexBufferEnabled` cvar 分岐 + 別 pipeline bind + `bindVertexBufferVk` + `vkCmdDraw(N)` 配線 (PC-N-6 (e) tag block) |
| (vi) | `AYAGltfStubVertexBufferEnabled` cvar 新設 (settings.xml) + LLCachedControl<bool> default false Persist=1 |
| (vii) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= cvar runtime gate のみ) |
| (viii) | MUSEUBO-A 整合 = `mUseUBO=false` default + `AYAGltfStubVertexBufferEnabled=false` default 経路不変、4 段 graceful degrade (= sAllocator / sGltfStubVertexBuffer / sGltfStubAssetPipeline / sGltfStubVertexMapped 各 nullptr guard) |
| (ix) | build verify llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131 全 PASS ((N6-11) A) |
| (x) | tag block 統一 PC-N-6 (a)/(b)/(c)/(d)/(e) + LL_INFOS first-fire marker PC-N-6 (e) 通電 literal 追加 + handoff complete doc 起案 |

### §6.1 GATE-B / MUSEUBO-A integrity check (実装 phase 末尾検証項)

- `git grep "LL_VULKAN_GLSL" indra/` 件数が PC-N-5 commit (`675529a891`) 時点と同数 = GATE-B 違反なし
- `AYAGltfStubVertexBufferEnabled=false` default で `recordGltfAssetDraw` 経路は PC-N-5 完了状態と機能等価 = MUSEUBO-A 整合
- 4 段 graceful degrade を Vulkan 未初期化条件下で各個 trip させて silent no-op 確認 (= cold launch + AYAGltfStubVertexBufferEnabled=true でクラッシュなし)

### §6.2 想定改変 file 3 件 (= 実装 phase = 別 session)

1. `indra/llrender/llvkloader.cpp` (+150〜180 / -2 推定) = (a) sGltfStubVertexBuffer file-static declare + sGltfStubVertexData const + (b) createGltfStubAssetPipeline 関数新設 + sGltfStubAssetPipeline storage + (c) initVulkan VMA allocate + initial upload + createGltfStubAssetPipeline call + (d) shutdownVulkan vmaDestroyBuffer + vkDestroyPipeline + (e) recordGltfAssetDraw cvar 分岐 + 別 pipeline bind + bindVertexBufferVk + vkCmdDraw(N) = 5 編集 step
2. `indra/newview/app_settings/settings.xml` (+19 / -0 推定) = AYAGltfStubVertexBufferEnabled Boolean cvar 1 件追加 ((N6-9) C)
3. `indra/llrender/llvkloader.h` 改変 0 件想定 (= 新 declare 全 anonymous namespace 内、register/unregister API signature 変更なし、bindVertexBufferVk 既配置)

### §6.3 想定 build verify command (= 実装 phase 末尾、Linux primary)

```
make -j4 llrender
ctest -R INTEGRATION_TEST_lluboringbuffer    # 11/11 PASS 確認
ctest -R INTEGRATION_TEST_llassetubopool     # 10/10 PASS 確認
ctest -R INTEGRATION_TEST_llpipelinecachestorage  # 13/13 PASS 確認
ctest -R codegen   # 131/131 PASS 確認
```

---

## §7. 着手手順 (= 次 session で PC-N-6 実装 phase 着手)

1. AYA 指示「PC-N-6 実装着手お願いします」literal 受領待ち
2. 本 PC-N-6 design-lock doc 全文 Read (= 必読 1 件)
3. pinpoint Read = `recordGltfAssetDraw` body (5479-5591) + `createAvatarBonePipeline` body (3300-3395) + `bindVertexBufferVk` wrap (6001-6028) + `settings.xml` `AYAGltfStubDrawEnabled` 配置近傍
4. step (a)-(g) 順で実装 (= §4 stub example を base に、PC-N-5 同形 tag block style 踏襲、indra/llrender/llvkloader.cpp + settings.xml 2 file 改変)
5. build verify literal 取得 (= §6.3 command 実行 + 各 PASS 数 record)
6. Exit Criteria 10 項 §6 self-verify + GATE-B / MUSEUBO-A integrity check §6.1
7. handoff complete doc 起案 (= PC-N-5 complete 同形 template) + cross-platform spec §6 PC-N-6 状態 ✅ 反映 + §A 更新履歴追記
8. AYA commit 指示後 commit (= feature branch `feature/ayastorm-r41-gl-removal` 上、`indra/llrender/llvkloader.cpp` + `indra/newview/app_settings/settings.xml` + 新 doc 1 件 + cross-platform spec 1 件改変 = 4 件 git add 個別指定、`git add -A` 不使用)

---

## §8. 残 strict 線形

PC-N-6 design-lock ✅ 本 commit → PC-N-6 実装 (= 次 session 着手、step (a)-(g) 7 step 実施 + Exit Criteria 10 項 self-verify + handoff complete doc 起案) → PC-N-7 design-lock (= 実 LL::GLTF::Asset 経由 index buffer upload + `vkCmdBindIndexBuffer` + `vkCmdDrawIndexed` 置換、別 session で個別 design-lock) → PC-N-7 実装 → PC-N-8 design-lock + 実装 → PC-N-9 design-lock + 実装 → PC-N-10 design-lock + 実装 → **Phase 1.D complete** → Phase 1 全完了 → Mac/Win 開発者補完 phase。

---

## §9. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1 design-lock ✅ + PC-N-1 ✅ + PC-N-2 design-lock ✅ + PC-N-2 ✅ + PC-N-4 design-lock ✅ + PC-N-4 ✅ + PC-N-3 design-lock ✅ + PC-N-3 ✅ = Phase 1.C complete ✅ + PC-8 Linux primary marker ✅ = Phase 1.C strict 線形終了 ✅ + PC-N-5 design-lock ✅ + PC-N-5 ✅ = Phase 1.D 着手起点 実装完了 ✅ + Phase 1.D decomposition design-lock ✅ + **PC-N-6 design-lock ✅ 本 commit** + PC-N-6 実装 ⏳ 次 session + PC-N-7..PC-N-10 各 design-lock + 実装 ⏳ = Phase 1.D complete ⏳ + Phase 1.E (multi-asset / multi-skin / worker thread) ⏳

---

## §10. self-verify 9 観点 全 ✅

1. **PC-N-6 literal scope §0 完全分解 7 件** = (a) sGltfStubVertexBuffer file-static + (b) sGltfStubAssetPipeline 新設 + (c) initVulkan VMA allocate + initial upload + (d) shutdownVulkan 対称破棄 + (e) recordGltfAssetDraw cvar 分岐 + (e') AYAGltfStubVertexBufferEnabled cvar 新設 + (f)/(g) build verify + handoff ✅
2. **必読 1 件 §1 + pinpoint reference 13 件別記** = Phase 1.D decomposition + PC-N-5 complete + recordGltfAssetDraw + createAvatarBonePipeline + bindVertexBufferVk + PC-N-5 (e) hook + PC-N-5 stub asset draw + settings.xml + Primitive 構造体 + Asset uploadTransforms + VMA helper + cross-platform spec + GATE-B literal + 設計原則 ✅
3. **現状調査 §2 10 項網羅** = code 8 項 (recordGltfAssetDraw + sAvatarBonePipeline + vertex input state + bindVertexBufferVk + bindIndexBufferVk + sGltfStubSkin + cvar + hook) + LL::GLTF 2 項 + step 依存関係 ✅
4. **ambiguity (N6-1)..(N6-13) 13 件 AYA literal「すべて推奨でお願いします」record (2026-06-05) §3** ✅
5. **採用根拠 13 件明文化 §3** ✅
6. **実装計画 (a)-(g) 7 step §4 分解 + 各 step に具体 code stub example 添付** ✅
7. **GATE-B 整合 §4.9 (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar runtime gate のみ) + MUSEUBO-A 整合 §4.10 (= `mUseUBO=false` + `AYAGltfStubVertexBufferEnabled=false` default 経路不変、4 段 graceful degrade) + 設計原則整合 §4.11 (= (1) call site 不変 + shader 改変ゼロ + sAvatarBonePipeline 並走温存)** ✅
8. **design-lock Exit Criteria 9 項 §5 + 実装 phase Exit Criteria 10 項 §6 + 想定改変 file 3 件 §6.2 明文化** ✅
9. **`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合 (= cross-platform spec §6 PC-N-6 行追記 + 本 design-lock doc 1 件起案のみ)** ✅

---

## §11. 次 session 着手 1 line

**PC-N-6 実装着手** = step (a)-(g) 7 step 実施 = (a) `sGltfStubVertexBuffer` file-static + `sGltfStubVertexData` const 配置 + (b) `createGltfStubAssetPipeline` 関数新設 + `sGltfStubAssetPipeline` storage + (c) initVulkan VMA allocate (host-visible mapped) + initial memcpy + createGltfStubAssetPipeline call + (d) shutdownVulkan vmaDestroyBuffer + vkDestroyPipeline 対称配置 + (e) `recordGltfAssetDraw` 内 `AYAGltfStubVertexBufferEnabled` cvar 分岐 + 別 pipeline bind + `bindVertexBufferVk` + `vkCmdDraw(N)` + (e') settings.xml cvar 1 件追加 + (f) build verify (= llrender + warning 0 + TUT 11+10+13 + codegen 131/131) + (g) handoff complete doc 起案 + Exit Criteria 10 項 self-verify + AYA commit 指示受領後 commit。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 PC-N-6 design-lock handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 次 session 必読 1 件 + pinpoint reference 13 件別記、本 session も Read pinpoint のみ (= Phase 1.D decomposition doc 全文 + recordGltfAssetDraw body + asset.cpp uploadTransforms + gltfscenemanager.cpp render + createAvatarBonePipeline body + bindVertexBufferVk wrap + Primitive 構造体 + cross-platform spec §6 + Explore agent pinpoint 報告)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §10
- **feedback_build_only_verified** 遵守 = design-lock phase は `indra/` 改変 0 件で build verify 対象外、実装 phase で literal 検証取得予定 ((N6-11) A 採用)
- **feedback_no_scope_shrink** 遵守 = PC-N-6 literal scope §0 完全分解 7 件 ((N6-1) B 採用は「PC-N-6 scope 最小単位」の定義確定ゆえ縮小ではない = Phase 1.D decomposition (D-7) A 原文「最小 1 mesh stub」整合、実 LL::GLTF::Asset 経路は PC-N-7..PC-N-10 持越し)
- **feedback_doubt_self_first** 遵守 = ambiguity 13 件発見で停止 + 推奨案提示 + AYA literal「すべて推奨でお願いします」一括確認後本 design-lock doc 起案、推測実装なし、特に (N6-7) sAvatarBonePipeline vertex input state 改変要否は 3 候補全列挙 + 根拠明示後 AYA 確認
- **feedback_confirm_referent_before_acting** 遵守 = 13 件 batch AYA 確認 (2026-06-05)、各候補 + 推奨案 + 根拠明示後 AYA literal 一括「すべて推奨でお願いします」record 受領で確定、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N-6 = stub vertex buffer 経路通電単独 sub-step = file-static + 別 pipeline + cvar 切替、PC-N-7..PC-N-10 残 4 sub-step は分離、本 doc 起案も PC-N-6 単独 design-lock のみ
- **feedback_design_phase_no_code_write** 厳格遵守 = 本 PC-N-6 design-lock phase は doc 起案のみ、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 (= cross-platform spec §6 PC-N-6 行追記 + 本 design-lock doc 1 件起案のみ)
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領待ち
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
- **feedback_no_bare_reference_ids** 遵守 = (N6-1)..(N6-13) 各 ID に項目名 / 採用案内容併記 §3 + (a)..(g) 各 step に作業内容併記 §4
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、git add 個別 file 指定予定
- **memory `project_ayastorm_r41_design_principles`** 整合 = (1) Upstream OpenGL 取り込みやすさ維持 = (N6-2) A signature 不変 + (N6-7) B 別 pipeline 新設 (sAvatarBonePipeline 不変温存) + GLTFSceneManager::render 改変 0 件 + shader 改変ゼロ §4.11 + (2) Core プロセス分散実現 = PC-N-7 以降の per-Primitive vertex buffer ownership design で実現 §4.11
- **memory `project_r41_phase1b_vulkan_host_gate`** 整合 = GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar runtime gate のみ §4.9
- **memory `project_ayastorm_three_platforms`** 整合 = cross-platform spec §6 PC-N-6 行追記で macOS / Windows 派生 fix 候補欄起案 (= MoltenVK 標準対応範囲ゆえ派生 fix 候補なし)、Linux primary 完成 → 他者補完 model 整合

---
