# r41 UBO 全体設計 Chapter 07: 現状 Vulkan API 実装棚卸し + 拡張仕様確定

**起案日**: 2026-06-03
**位置付け**: chapter 06 (06a/06a-prep/06b/06c) で確定した **redirect 層 / cadence / dirty / descriptor set 配置** に対し、実 Vulkan API 実装側 (= `indra/llrender/llvkloader.{cpp,h}` + VMA + volk) の **現状棚卸し** と **本設計を満たすための拡張仕様** を確定する。実 `vkCreateDescriptorPool` / `vkAllocateDescriptorSets` / `vkUpdateDescriptorSets` 配線、device limit query 仕様、PSO compatibility 確保、triple-buffering 実装、dynamic offset / ring buffer 容量、sampler 49 個配置 (S3) の確定、descriptor pool 容量算定 (W)、reflection update sync 方式 を本 chapter に集約する。実 program 単位 migration 順序 / phase 番号は chapter 09 譲り。
**pre-requisite**:
- `01-overview.md` §2 (2 大設計原則) / §3 (用語) / §5 (確定事項 13 件)
- `04-codegen-ubo.md` §5 (`ubo_metadata.inl` で block_name → (size, set, binding))
- `05-existing-inventory-link.md` §4 E3 (= 79 個独立保持)
- `06a-cache-structure-and-setter-redirect.md` §3 (cache 構造) / §5.6 (sampler は OpenGL path 強制 + Vulkan path descriptor set 経由)
- `06b-cadence-update-site-and-dirty.md` §4 (flush 関数 5 種) / §4.3 (triple-buffering U1=3) / §5.3 (L1+L2)
- `06c-descriptor-set-bind-wiring.md` §2 (set 4 帯配置) / §3 (UB_\* ↔ 84 接合表) / §7 (set=0 rotate 方式 A) / §8 (sampler bridge) / §10 ((V1)(V3)(S3) 持越)
- `ayastorm-r41-ubo-current-state-inventory.md` §1 (OpenGL 実働 UBO 4 種 + UB_\* enum)

---

## §0 本 chapter の scope

### §0.1 scope (= 本 chapter で確定するもの)

1. **現状 Vulkan API 実装棚卸し** (= `llvkloader.cpp/h` + `vk_mem_alloc.h` + `volk.{c,h}` の既存配線整理)
2. **device limit query 仕様** (= 既存 `queryAndLogDeviceLimits()` の本設計向け拡張、(V1) 解消)
3. **set=1 layout 方式確定** (= (V3) 解消、全 program 共通 vs program 別)
4. **sampler 49 個 descriptor set 配置確定** (= (S3) 解消、S1 / S2 / S4 案再評価)
5. **descriptor pool 容量算定** (= (W) 新規、cadence 別 maxSets / pool size)
6. **dynamic offset (L2) 実 Vulkan 配線** (= 06b §5.3 確定を Vulkan API call レベルに展開、ring buffer 容量)
7. **triple-buffering (U1=3) 実装** (= 既存 `FRAMES_IN_FLIGHT=3` の本設計適用、set=0 以外 cadence への波及判定)
8. **PSO layout / VkPipelineLayout compatibility** (= shader bind 時の pipeline cache hit 率最大化)
9. **reflection update sync 方式** (= 06c §7.3 Global\_ReflectionProbes の GPU fence 経由 sync)

### §0.2 非 scope (= 他 chapter / 他 phase 譲り)

- shader 単位 migration 順序 / Phase 番号体系 → **chapter 09 (phase-roadmap)**
- Codegen 出力 `ubo_metadata.inl` build pipeline → **chapter 08 (build-codegen-pipeline)**
- per-LLImageGL VkImage 配線 / sampler per-texture mipmap・anisotropy 配信 → 領域 7 sub-step 7.5 (= **本設計の外**、`llvkloader.h:209-213` 既述)
- bare uniform 267 個 集約先 UBO 確定 → **chapter 05a (= bare-uniform-mapping 切出し doc)**
- secondary cmdbuf / multi-thread record / core 分散実 wire → **r41 後続 phase (= chapter 09 §N)**
- Vulkan validation layer 設定 / VK_KHR_dynamic_rendering vs render pass 採用議論 → **既存 3.1b PSO foundation で確定済 (本 chapter は既存採用を前提)**

---

## §1 入力契約

| 入力 source | 本 chapter での用途 |
|---|---|
| `04-codegen-ubo.md` §5.1 `ubo_metadata.inl` (block_name → (size, set, binding)) | §4 set=1 layout 構築、§6 pool 容量算定 |
| `04-codegen-ubo.md` §5.3 `UniformLocation` (= `block_hash` で物理 UBO 識別) | §7 dynamic offset の loc → block_hash 解決 |
| `05-existing-inventory-link.md` §4 E3 (= 79 個独立保持) | §4 set=1 layout binding 数確定 (79) |
| `06a-cache-structure-and-setter-redirect.md` §5.6 sampler 強制 OpenGL path | §5 sampler 配置決定の入力 |
| `06b-cadence-update-site-and-dirty.md` §4.1 flush 関数 5 種 | §6 cadence 別 pool 容量算定 |
| `06b-cadence-update-site-and-dirty.md` §4.3 triple-buffering (U1=3) | §8 実装方式、set=0 既存 + 他 cadence 波及判定 |
| `06b-cadence-update-site-and-dirty.md` §5.3 L1+L2 default | §7 dynamic offset + ring buffer 配線 |
| `06c-descriptor-set-bind-wiring.md` §2 set 4 帯配置 | §6 cadence 別 pool 容量算定 |
| `06c-descriptor-set-bind-wiring.md` §3 UB_\* ↔ 84 接合表 | §4 set=1 layout 構築 |
| `06c-descriptor-set-bind-wiring.md` §7 set=0 rotate 方式 A | §8 既存 `FRAMES_IN_FLIGHT=3` 配線への適用 |
| `06c-descriptor-set-bind-wiring.md` §8 sampler bridge (S1/S2/S3 案) | §5 配置確定 |
| `06c-descriptor-set-bind-wiring.md` §10 持越 (V1)(V3)(S3) | §3 / §4 / §5 で本 chapter 解消 |
| `ayastorm-r41-ubo-current-state-inventory.md` §1 OpenGL 4 種 UBO | §2 既存 LL bind 経路の Vulkan 並走配置 |

---

## §2 現状 Vulkan API 実装棚卸し

### §2.1 base loader / instance / device

- `llvkloader.cpp:2118` = `volkInitialize()` (volk dynamic loader)
- `llvkloader.cpp:164-179` = `vkCreateInstance` + `volkLoadInstanceOnly()`
- `llvkloader.cpp:553-560` = `vkCreateDevice` + `volkLoadDevice()`
- `llvkloader.cpp:281` / `:424` = `vkGetPhysicalDeviceProperties` / `vkGetPhysicalDeviceProperties2`
- header = `indra/llrender/volk.h` (Vulkan symbol forwarding)
- = **base 配線完了**、本 chapter は extension query (§3) の拡張のみ加える

### §2.2 VMA allocator

- `llvkloader.cpp:362` = `VmaAllocator sAllocator` (file-local)
- `llvkloader.cpp:784-808` = `createVmaAllocator()` (`vmaCreateAllocator` 呼出)
- `llvkloader.cpp:1186` / `:1967` / `:3047` = `vmaCreateBuffer` 使用箇所
- vendored = `indra/llrender/vendor/vk_mem_alloc.h`
- 設計境界 = `VmaAllocation` handle は `llvkloader.cpp` 1 TU 限定 (header 不露出、`void*` opaque で外出) — `llvkloader.h:209-213` / `:238-240` で確認
- = **VMA 基本配線完了**、本 chapter は per-cadence UBO 用 `vmaCreateBuffer` 量産経路を §6 / §7 で確定

### §2.3 device limits 既存 query

`llvkloader.cpp:339-353` の `DeviceLimits` struct:

| field | 現状値出典 | 本設計での用途 |
|---|---|---|
| `maxBoundDescriptorSets` | `VkPhysicalDeviceLimits` (Vulkan 1.3 最小 4) | §4 set=0/1/2/3 = 4 帯運用、Vulkan 1.3 最小値で必要十分 |
| `maxPushConstantsSize` | (Vulkan 1.3 最小 128) | 既存 modelview push (64 B) のみ、本設計で拡張なし |
| `maxPushDescriptors` | `VkPhysicalDevicePushDescriptorPropertiesKHR` | per-skin (set=3) 既存使用、本設計で温存 |
| `maxPerStageDescriptorSampledImages` | (Vulkan 1.3 最小 16) | §5 sampler 配置の (S3) 評価軸 |
| `maxDescriptorSetSamplers` | (Vulkan 1.3 最小 96) | §5 sampler 配置の (S3) 評価軸 |
| `maxColorAttachments` | (Vulkan 1.3 最小 4) | 本設計では使用しない |
| `pushDescriptorSupported` | extension 検出 | §3 で本設計向け fallback と接続 |
| `memoryBudgetSupported` | extension 検出 | VMA budget log 用、本設計で温存 |

**追加すべき field** (= 本設計で query 必要):
- `maxDescriptorSetUniformBuffers` (Vulkan 1.3 最小 72) — §3.1 / §4 set=1 79 binding 評価で必須
- `maxDescriptorSetUniformBuffersDynamic` (Vulkan 1.3 最小 8) — §7 dynamic offset 上限
- `maxPerStageDescriptorUniformBuffers` (Vulkan 1.3 最小 12) — §4 stage 単位上限
- `minUniformBufferOffsetAlignment` (Vulkan 1.3 最大 256) — §7 ring buffer offset alignment

= **§3 で `DeviceLimits` 構造体に 4 field 追加**。

### §2.4 descriptor pool / descriptor set 既存配線

- `llvkloader.cpp:121` = `sPerFrameDescriptorPool` (= per-frame UBO 用、`FRAMES_IN_FLIGHT * 2` UBO 帯)
- `llvkloader.cpp:363` = `sSharedDescriptorPool` (= placeholder per-material 7 PBR slot 用、`vkCreateDescriptorPool` 呼出 `:837`)
- `llvkloader.cpp:1422-1450` = `vkCreateDescriptorPool` + `vkAllocateDescriptorSets` for per-frame
- `llvkloader.cpp:927` = `vkAllocateDescriptorSets` for per-material
- `llvkloader.cpp:986` = `vkCmdBindDescriptorSets` (placeholder pool draw)
- = **既存は placeholder 3 setup のみ** (per-frame 1 種 + per-material 1 種 + avatar bone 1 種)、本設計の 4 帯 (= set=0/1/2/3) フル配線は **§6 で新規確定**

### §2.5 descriptor set layout / pipeline layout

- `llvkloader.cpp:382` = `sPerMaterialDescriptorSetLayout` (set=1 帯、7 binding、現状 placeholder material 専用)
- `llvkloader.cpp:120` = `sPerFrameDescriptorSetLayout` (set=0 帯、2 binding `PerFrameMatrixUBO` + `TextureMatrixUBO`)
- `llvkloader.cpp:87` = `sAvatarBoneDescriptorSetLayout` (set=2 帯、avatar bone SSBO)
- `llvkloader.cpp:2557-2576` = `createStandardPipelineLayout()` helper (`vkCreatePipelineLayout`)
- = **placeholder 3 set 配線は既に成立**、本設計の per-program 79 binding / per-draw 4 binding / per-asset+per-skin owner instance への拡張は **§4 / §6 で新規確定**

### §2.6 sampler

- `llvkloader.cpp:383` = `sPlaceholderSampler` (= placeholder linear/clamp 共用 1 個)
- `llvkloader.cpp:890-903` = `vkCreateSampler` 1 回呼出
- = per-texture sampler (mipmap / anisotropy) は領域 7 sub-step 7.5 持越し (`llvkloader.cpp:378-380` 注記)、本 chapter §5 は **配置 set 帯** の確定のみ

### §2.7 triple-buffering (FRAMES_IN_FLIGHT=3)

- `llvkloader.cpp:113` = `constexpr U32 FRAMES_IN_FLIGHT = 3`
- `llvkloader.cpp:122-125` = `sPerFrameUboBuffer/Memory/Mapped/DescriptorSet[FRAMES_IN_FLIGHT]`
- `llvkloader.cpp:129` = `sFrameIndex` (= frame counter)
- `llvkloader.cpp:2448` = `sFrameIndex = (sFrameIndex + 1) % FRAMES_IN_FLIGHT` (= frame rotate)
- `llvkloader.cpp:2620-2645` = `writeCurrentPerFrameMatrixUBO` / `writeCurrentTextureMatrixUBO` (= rotate buffer への memcpy)
- = **set=0 帯の 3 set rotate (= 06c §7.2 方式 A) は既存配線で成立**、本 chapter §8 は他 cadence への波及判定 + 本設計 84 UBO 全体への適用

### §2.8 SPIR-V module / dynamic rendering / vertex buffer 並走

- `llvkloader.h:179` = `loadSpirvShaderModule()` (= SPIR-V → `VkShaderModule`)
- `llvkloader.h:195` = `loadSpirvShaderModuleFromMemory()` (= production SPIR-V sink)
- `llvkloader.h:153-158` = `beginDynamicRendering` / `endDynamicRendering` (= `vkCmdBeginRenderingKHR` wrap)
- `llvkloader.h:250-280` = `createVertexBufferVk` / `createIndexBufferVk` / `bindVertexBufferVk` / `bindIndexBufferVk` (= LLVertexBuffer Vulkan 並走 helper、4.3-β' 確定)
- = **shader load / draw record / vertex buffer 並走は既配線**、本 chapter で再設計しない

---

## §3 device limit query 仕様 (= (V1) 解消)

### §3.1 追加 query field

`DeviceLimits` struct に **4 field 追加** (= §2.3 棚卸し結論):

```cpp
struct DeviceLimits {
    // 既存 (省略)
    U32  maxDescriptorSetUniformBuffers;        // Vulkan 1.3 minimum 72
    U32  maxDescriptorSetUniformBuffersDynamic; // Vulkan 1.3 minimum 8
    U32  maxPerStageDescriptorUniformBuffers;   // Vulkan 1.3 minimum 12
    U32  minUniformBufferOffsetAlignment;       // Vulkan 1.3 maximum 256
};
```

= `queryAndLogDeviceLimits()` (`llvkloader.cpp:386`) で `VkPhysicalDeviceLimits` から追加 copy + LL_INFOS 出力。

### §3.2 (V1) set=1 79 binding 評価

| device 想定 | `maxDescriptorSetUniformBuffers` | set=1 79 binding 可否 |
|---|---|---|
| Vulkan 1.3 spec 最小 | 72 | **NG (= 7 不足)** |
| AMD RDNA2/3 driver | 1.5M (= 実質無制限) | OK |
| NVIDIA Ampere/Ada | 1M+ | OK |
| Intel Iris Xe (1.3) | 256+ | OK |

**spec 最小 72 で 79 不足** = device 検知時の対応:

| 案 | 内容 | 評価 |
|---|---|---|
| V1' | set=1 を 2 帯に split (= set=1a + set=1b、Codegen で 40/39 振分け) | **Claude 推奨 default、§4.3 で確定** |
| V1'' | 79 binding 強制、device 検知時に **起動 abort + 説明 dialog** | 起動性損失大、不採用 |
| V1''' | 79 binding 強制、device 検知時に **OpenGL fallback** | OpenGL 撤廃 (r41 章 thesis) と矛盾、不採用 |

**V1' 採用根拠**:
- Vulkan 1.3 spec 最小 72 ≥ 39 = **半分 split で全 device 必ず 1 set 適合**
- PSO layout は set=1a + set=1b の 2 帯固定 = shader compile 時に確定 (= runtime split 不要)
- 79 個の Program\_\* UBO は **どの program でも同一 layout 参照** (= chapter 02 §3 rename 後、UB_\* enum 不変) のため、80/20 のような不均等 split は不要、Codegen 時 名前順 sort で 40/39 自動振分け可能

**実装**: `ubo_metadata.inl` 出力時 (= chapter 04 §5.1) に set=1 帯の binding 数 = 79 を **2 帯に automatic split** + set 番号 1 → 1+2 へ shift (= set=2 が以降 set=3 へ shift)。chapter 06c §2 表は本 chapter §3.2 確定後、**set 帯総数 4 → 5 へ拡張案** に reflect。

### §3.3 (V1) Phase 0 計測項目

実装 phase 入口 (= 06a-prep §3 と並走) で **AYA 実機 device 計測** を 3 項目追加:

| 項目 | LL_INFOS 出力例 |
|---|---|
| `maxDescriptorSetUniformBuffers` | "Device limit: maxDescriptorSetUniformBuffers=1535000 (set=1 79 binding OK / NG 判定)" |
| `maxDescriptorSetUniformBuffersDynamic` | "Device limit: maxDescriptorSetUniformBuffersDynamic=8 (set=2 dynamic offset 上限)" |
| `maxPerStageDescriptorUniformBuffers` | "Device limit: maxPerStageDescriptorUniformBuffers=12 (stage 単位 = 1 stage で 12 UBO bind 可)" |

AYA 実機 (= AYAstorm 3 OS = Linux/Win/Mac) で OK 確認後、(V1') split を **本配線時に取込み**。1 OS でも NG なら **V1' default 起動**。

---

## §4 set=1 layout 方式確定 (= (V3) 解消)

### §4.1 方式案 (= 06c §10 (V3) 再掲)

| # | 方式 | layout | PSO compatibility | dummy 占有 |
|---|---|---|---|---|
| V3a | 全 program 共通 79 binding | 1 set layout | **最大 (= shader 単位 layout 同一)** | 各 program 未使用 binding は dummy buffer 参照 |
| V3b | program 別 layout (= 必要 binding のみ) | program 数 layout | program 切替で pipeline layout 再 compile (= cache miss 増) | dummy なし |

### §4.2 V3a 採用根拠 (= **本 chapter §4 確定**)

1. **PSO compatibility 最大**: PSO は `(pipeline_layout, render_pass)` キーで cache。layout 1 種 = shader 数 × 1 = cache hit 率最大、layout N 種 = cache miss × N = compile cost N 倍
2. **shader bind cost 最小**: `vkCmdBindDescriptorSets` は **layout が同一なら descriptor set 切替のみ** (= driver internal cost 最小)。layout 異なれば re-validate 必要
3. **dummy buffer cost 微小**: 未使用 binding に dummy `VkBuffer` を bind するのは memory + bind table の 1 pointer のみ、**実 upload は走らない** (= GPU read も走らない、shader 中で参照されない binding は driver が optimize out)
4. **Codegen 静的生成と整合**: `ubo_metadata.inl` で 79 binding 固定出力 = compile-time layout 確定、runtime 動的 layout 構築不要

= **V3a 採用、§4.3 で実装仕様確定**。

### §4.3 set=1 layout 実装仕様

§3.2 (V1') split を併用すると **2 set 帯 (set=1a + set=1b)**:

```
set=1a: 40 binding (= Program_AAA ... Program_LLL, std140)
set=1b: 39 binding (= Program_MMM ... Program_ZZZ, std140)
stage: VERTEX_BIT | FRAGMENT_BIT (= 全 stage 共通)
```

dummy buffer:
- 起動時 1 個 `vmaCreateBuffer` (size=1 KB, HOST_VISIBLE+MAPPED) = 全 program で未使用 binding 全てが参照
- `vkUpdateDescriptorSets` で未使用 binding を **shader bind 時に一括 dummy 投入** (= program 切替時 1 回)

flush 連動 (= 06b §4.1 `flushProgramUbos()` 直後):
1. dirty UBO 群について `vmaMapMemory` + memcpy + `vmaUnmapMemory` (= HOST_VISIBLE+MAPPED は map 維持で memcpy 直書き)
2. `vkUpdateDescriptorSets` で **dirty binding のみ** 更新 (= dummy 入替 / 実 buffer 入替 の双方)
3. `vkCmdBindDescriptorSets(set=1a, set=1b)` 2 帯 bind

### §4.4 chapter 06c §2 表への波及

set 帯総数 **4 → 5** に拡張:
- set=0 per-frame + singleton (4 binding)
- set=1a per-program 40 binding
- set=1b per-program 39 binding
- set=2 per-draw 4 binding (dynamic offset)
- set=3 per-asset + per-skin (3 binding × N owner)

`maxBoundDescriptorSets` = Vulkan 1.3 最小 4 を**超過**:

→ §4.4.1 で **set=2 / set=3 への dynamic offset 寄せ** を確定 (= bind set 数を 4 に維持)。

### §4.4.1 5 set → 4 bind set 縮減

shader bind 時 (= `vkCmdBindDescriptorSets` 呼出) は **必ず set=0..3 の 4 set 同時 bind** を維持:

- set=0 (per-frame, frame 開始 1 回 bind)
- set=1a (per-program 40 binding)
- set=1b (per-program 39 binding) — **set=1a と同時 bind**
- set=2 (per-draw dynamic offset)

set=3 (per-asset+per-skin) は **draw 直前に set=2 と入れ替え bind** (= owner 切替時のみ):
- 例: rigged GLTF draw = set=2 + set=3 同時 bind (= 4 set 制約に収まる)
- 非 skinned static = set=2 only (= 3 set bind)

= `maxBoundDescriptorSets=4` を**死守**しつつ、PSO compatibility (V3a) を維持。

= **(V3) 解消、§4.4 で set 帯 5 個へ拡張 + bind 時 4 set 制約死守を確定**。

---

## §5 sampler 49 個 配置確定 (= (S3) 解消)

### §5.1 6c §8 で先送りの 3 案再評価

| # | 配置 | 影響 |
|---|---|---|
| S1 | 新規 set=4 (sampler 専用帯) | `maxBoundDescriptorSets` 制約 (§4.4) で **新規 set 帯不可** → 不採用 |
| S2 | set=1 に sampler binding 追加 | set=1 が 79 + 49 = 128 binding 超過 (`maxDescriptorSetUniformBuffers`/`Samplers` 別 limit のため別計上だが、§3.2 V1' split 後に sampler を set=1a/1b に追加 = layout 複雑化) |
| S3' | **set=3 に sampler 49 個追加** (= per-asset+per-skin 帯と同居) | per-asset cadence で sampler rebind (= GLTF asset 切替時に diffuseMap/normalMap 等を 1 set で同時更新)、`maxDescriptorSetSamplers` 評価必要 |
| S4 | **set=2 に sampler 49 個 dynamic 寄せ** (= per-draw 帯と同居) | per-draw cadence で sampler rebind = 不必要過剰更新 (= 同一 material で同一 sampler を毎 draw rebind)、cadence mismatch |

### §5.2 確定 = **S3' 採用** (= set=3 per-asset 帯 + sampler 49 同居)

**S3' 採用根拠**:
1. **cadence 整合**: sampler 49 個の主流 cadence = per-asset (= GLTF asset 切替時に material texture set 更新) — set=3 帯 cadence と一致
2. **bind 回数最小**: per-asset 切替時に 1 回 bind = per-draw bind の何倍も少ない (= draw 単位の rebind 不要)
3. **`maxDescriptorSetSamplers` 適合**: Vulkan 1.3 spec 最小 96 ≥ 49 = **全 device 必ず 1 set 適合** (= split 不要)
4. **`maxPerStageDescriptorSampledImages` 適合**: Vulkan 1.3 spec 最小 16 だが、**stage 単位 = 同時 stage 16 → 49 superset でも shader 中で 1 stage が同時 sample するのは画面あたり 7-16 (= PBR + shadow map 数)** = stage 16 制約に shader 単位 fit (= Codegen 側で warn 出力可能)

### §5.3 sampler 49 個実装仕様

- set=3 binding 配置: 既存 per-asset (Asset_\* 2) + per-skin (Skin_\* 1) = 3 UBO binding + sampler 49 binding = **計 52 binding / set=3**
- descriptor type = `COMBINED_IMAGE_SAMPLER` × 49 (既存 `sPerMaterialDescriptorSet` 7 PBR slot は **set=3 へ移植**)
- stage = `FRAGMENT_BIT` (= 全 sampler、頂点 stage で sample しない)
- per-asset 切替 (= GLTF asset bind) 時に sampler 49 個一括 `vkUpdateDescriptorSets` (= per-asset cache 構造、chapter 06a 設計外、`gltf::Asset` owner 側 lifetime)

### §5.4 chapter 06c §2 表への波及

set=3 帯 binding 数 = 3 (UBO) + 49 (sampler) = **52 binding / set=3 / per-asset owner instance**。set=3 layout は `gltf::Asset` 単位で構築、`maxDescriptorSetSamplers` 96 ≥ 49 = 1 set 内で fit。

= **(S3) 解消、§5.2-§5.4 で S3' set=3 per-asset+sampler 同居を確定**。

---

## §6 descriptor pool 容量算定 (= (W) 新規)

### §6.1 cadence 別 maxSets / pool size

| pool | maxSets | UBO 帯 binding 数 | sampler 帯 binding 数 |
|---|---|---|---|
| `sFrameUboPool` (set=0) | 3 (= `FRAMES_IN_FLIGHT`) | 4 × 3 = 12 | 0 |
| `sProgramUboPool` (set=1a + set=1b) | shader 数 (= 暫定 200 program) × 3 frame = 600 set 帯 × 2 (1a/1b) = 1200 | 79 × 1200 = 94800 (over-spec のため §6.2 で再算定) | 0 |
| `sDrawUboPool` (set=2) | 1 set 固定 (= dynamic offset で全 draw 再利用) | 4 | 0 |
| `sAssetUboPool` (set=3) | gltf asset 数 × 3 frame (= asset N × 3) | 3 × N × 3 = 9N | 49 × N × 3 = 147N |

### §6.2 (W) Program\_\* pool 過剰算定の縮減

shader 数 × 3 frame × 2 (1a/1b) = 1200 set は **過剰**。実際は:

- shader bind 時に 1 set bind で十分 (= bind 時 program 切替で descriptor 内容書換)
- triple-buffering 適用は **set 帯ごと**、Program\_\* も 3 frame rotate 必要 (= write-after-read hazard 回避)

= 縮減 case:
- (a) **shader 数 × 3 frame** = 200 × 3 = 600 (= 2 帯 split を考慮し set=1a 600 + set=1b 600 = 1200) → やはり 1200
- (b) **active set 数のみ** (= shader bind 中の current only) × 3 frame × 2 帯 = 1 × 3 × 2 = 6 set のみ。non-active program は **shader bind 直前に descriptor set update**。

**(b) 採用**:
- pool sizing 大幅縮減 (1200 → 6 set)
- 引換に shader bind 時に `vkUpdateDescriptorSets` が走る (= program 切替毎の cost 増)
- frame 内 program 切替 ≈ 100 回想定 → cost 計算: 100 × 79 binding × 1 frame = **8K update / frame** = driver internal で OK (= AMD/NVIDIA spec で 数万 update / frame 想定範囲)

= (b) で **`sProgramUboPool` maxSets=6 / pool size = 79 × 6 = 474 UBO descriptor**。

### §6.3 確定 pool 容量

| pool | descriptorCount (UBO) | descriptorCount (SAMPLER) | maxSets |
|---|---|---|---|
| `sFrameUboPool` (set=0) | 4 × 3 = 12 + Global\_\* 1 = 13 | 0 | 3 |
| `sProgramUboPool` (set=1a + set=1b) | 79 × 6 = 474 | 0 | 6 |
| `sDrawUboPool` (set=2) | 4 (= dynamic) | 0 | 1 |
| `sAssetUboPool` (set=3) | 3 × N × 3 = 9N (= asset N 動的、 起動時 N=64 prealloc → grow) | 49 × N × 3 = 147N | N × 3 |

`gltf::Asset` 数 N は scene 規模で動的 → **pool grow 機構** が必要:
- N=64 起動時 prealloc (= 64 × 3 = 192 set / 64 × 9 = 576 UBO / 64 × 147 = 9408 SAMPLER)
- pool 枯渇 detect 時 = 新 pool 追加 (= `sAssetUboPools[]` 配列化、各 64 grow)
- = chapter 09 Phase Roadmap の **per-asset cache 本実装 phase** で grow 機構実装

### §6.4 1 pool vs 4 pool

`sSharedDescriptorPool` (= 既存) を **4 cadence pool に split** 採用根拠:
1. pool grow 影響範囲を cadence 単位に閉じ込め (= asset pool grow が frame pool に波及しない)
2. cadence cleanup タイミング独立 (= shutdown 時の pool reset cadence 別)
3. `VkDescriptorPoolCreateFlags` を cadence 別に最適化可能 (= asset pool = `FREE_DESCRIPTOR_SET_BIT` 不要 = grow only / draw pool = `FREE_DESCRIPTOR_SET_BIT` 不要)

= **4 pool split 採用、§6.3 容量で実装**。

---

## §7 dynamic offset (L2) 実 Vulkan 配線 + ring buffer 容量

### §7.1 06b §5.3 確定の再掲

per-draw UBO は **1 物理 VkBuffer (= ring buffer)** + `vkCmdBindDescriptorSets(..., pDynamicOffsets=...)` で **draw 単位 offset 切替**。

### §7.2 ring buffer 容量算定

per-draw UBO 4 種 (= Draw\_\* 2 + Material\* 2) × **想定 draw 数 / frame** × `FRAMES_IN_FLIGHT=3`:

| 想定 draw 数 / frame | per-draw UBO 合計 size | × 3 frame ring | 切上 alignment 反映 |
|---|---|---|---|
| 5000 draw (= 通常 sim) | 5000 × 4 × 64 B = 1.28 MB | 3.84 MB | **4 MB ring buffer** |
| 20000 draw (= 過密 sim) | 20000 × 4 × 64 B = 5.12 MB | 15.36 MB | **16 MB ring buffer** |

= **4 MB 起動時 prealloc + 16 MB grow 上限** (= chapter 09 Phase 入口で `AYARingBufferSizeMB` cvar 配信、initial=4)

### §7.3 offset alignment

`minUniformBufferOffsetAlignment` (§3.1) = device 依存 (= Vulkan 1.3 spec 最大 256, AMD/NVIDIA 典型 = 64):

- ring buffer の draw 単位 offset を `align_up(prev_offset + struct_size, minUniformBufferOffsetAlignment)` で計算
- = Codegen 側で UBO struct size を **alignment 倍数 (256 B safe)** で padding 出力 (= chapter 04 §5.1 既述)

### §7.4 draw 単位 offset 投入経路

`flushDrawUbos()` (= 06b §4.1) → ring buffer write → **`pDynamicOffsets` 配列構築**:

```cpp
U32 offsets[4] = {
    draw_n_offset_for_DrawTransform,
    draw_n_offset_for_DrawLights,
    draw_n_offset_for_MaterialPBR,
    draw_n_offset_for_MaterialDithering,
};
vkCmdBindDescriptorSets(cmd_buf, ..., set=2, 1, &sDrawDescriptorSet,
                        4, offsets);  // dynamic offset 4 個
```

set=2 descriptor set 自体は **1 set 固定** (= `sDrawDescriptorSet`)、ring buffer 全範囲を 4 binding で参照 → dynamic offset で draw 単位切替。

### §7.5 ring buffer 枯渇判定

ring write head が 1 frame 周回前位置に追いつくと **write-after-read hazard**:
- `vmaCreateBuffer` を 1 frame 単位で **3 段 chunk** に分け (= 4 MB / 3 ≈ 1.33 MB / frame chunk)
- chunk write head が chunk_size 超過 → **次 chunk へ wrap、N-2 chunk は GPU 読込完了済保証** (= `FRAMES_IN_FLIGHT=3` triple-buffering と同形)
- chunk 内枯渇 = **ring buffer grow** (= 4 MB → 8 MB → 16 MB) + LL_WARNS log 1 回

---

## §8 triple-buffering (U1=3) 実装

### §8.1 06b §4.3 / 06c §7 既述

per-frame UBO の write-after-read hazard 回避 = `FRAMES_IN_FLIGHT=3` (= 既存 `llvkloader.cpp:113`、§2.7 棚卸し済)。

### §8.2 set=0 rotate (= 06c §7.2 方式 A) 実装

既存 `sPerFrameDescriptorSet[FRAMES_IN_FLIGHT]` = 3 set rotate **既配線**。本設計の Frame\_\* 3 UBO + Global\_\* 1 UBO 合計 4 binding を **既存 2 binding 構造から 4 binding 拡張** (= layout 更新):

- 既存 = `binding 0=PerFrameMatrixUBO` + `binding 1=TextureMatrixUBO`
- 拡張後 = `binding 0=FrameViewProj` + `binding 1=FrameLights` + `binding 2=FrameAtmosphere` + `binding 3=Global_ReflectionProbes`

`sPerFrameDescriptorSetLayout` (`llvkloader.cpp:120`) 構築 (`:1324-1340`) を **4 binding 化**、`sPerFrameUboBuffer/Memory/Mapped` を **3 instance × 4 UBO = 12 buffer** 化。

### §8.3 他 cadence への波及判定

triple-buffering を per-program / per-draw / per-asset にも適用するか:

| cadence | write-after-read hazard? | 必要 | rotate 方式 |
|---|---|---|---|
| per-frame (set=0) | YES (= frame 内で host write + GPU read 同時) | **必要** | 3 set rotate (= 既述) |
| per-program (set=1a/1b) | YES (= program 切替で host update + 前 program GPU 読込中) | **必要** | descriptor set 6 個 (§6.2 (b) = 1 active × 3 frame × 2 帯) |
| per-draw (set=2) | YES (= draw 連続で host write + GPU 読込中) | **必要** | ring buffer 3 chunk (§7.5) |
| per-asset (set=3 UBO) | YES (低頻度だが scene 切替で hazard 可) | **必要** | per-asset owner cache 内で 3 instance (= 9N) |
| per-asset (set=3 sampler) | NO (= sampler は read only、host write は texture upload 経路で別 sync) | 不要 | 単一 sampler set |
| per-skin (set=3 SSBO) | YES (= frame 毎 joint update) | **必要** | per-skin owner cache 内で 3 instance (= 3M) |
| singleton (Global\_\*) | NO (= update 頻度 < frame、reflection update 時のみ、GPU fence 経由) | 不要 (§10 GPU fence) | 単一 buffer (= 06c §7.3) |

= **per-frame / per-program / per-draw / per-asset UBO / per-skin SSBO の 5 cadence で triple-buffering 必須**、UBO 側 sampler は不要、Global\_\* は GPU fence で代替。

### §8.4 frame index 共有

`sFrameIndex` (= `llvkloader.cpp:129`) を **全 cadence rotate で共有** (= cadence 別 index 持たない):
- set=0 rotate = `sPerFrameDescriptorSet[sFrameIndex]`
- set=1a/1b rotate = `sProgramDescriptorSet[sFrameIndex]` (= 各 program shader bind 時)
- set=2 ring chunk = `sDrawRingChunk[sFrameIndex]`
- set=3 per-asset rotate = `Asset->sDescriptorSet[sFrameIndex]`
- set=3 per-skin rotate = `Skin->sDescriptorSet[sFrameIndex]`

= `beginFrame()` 1 回の `sFrameIndex` 進行で **全 cadence 同期 rotate**。

---

## §9 PSO layout / VkPipelineLayout compatibility

### §9.1 pipeline layout 構成

shader 全体で **共通 pipeline layout**:

```
VkPipelineLayout sAYAStandardLayout = {
    .descriptor_set_layouts = {
        sPerFrameSetLayout,         // set=0
        sProgramSetLayoutA,         // set=1a
        sProgramSetLayoutB,         // set=1b
        sDrawSetLayout,             // set=2 (dynamic offset)
        // set=3 (per-asset) は draw 時に sDrawSetLayout と入替 bind (§4.4.1)
    },
    .push_constant_ranges = {
        { 0, 64, VK_SHADER_STAGE_VERTEX_BIT },  // modelview push (既存)
    },
};
```

= **shader 全体で 1 pipeline layout 共有** (= V3a 採用直結、§4.2)。

### §9.2 set=3 swap 配線

draw 時に **set=2 か set=3 の一方を bind** (= §4.4.1):
- rigged GLTF draw = `vkCmdBindDescriptorSets(set=2)` + `vkCmdBindDescriptorSets(set=3 per-asset/skin)`
- non-skinned static = `vkCmdBindDescriptorSets(set=2)` only

`maxBoundDescriptorSets=4` 制約死守:
- shader bind 時 = set=0, 1a, 1b, 2 (= 4 set 同時)
- draw 切替時 = set=2 を **set=3 と置換** (= maxBound 不変)

### §9.3 PSO cache 戦略

`VkPipelineCache sPipelineCache` (= `llvkloader.h:36` `getPipelineCache()` 既配線):
- pipeline 1 layout 共有 = 全 PSO が同一 layout = cache hit 率最大
- `pCachedData` を 起動時 disk から load (= `~/.ayastorm_x64/cache/pipeline_cache.bin`)、終了時 save

= **§9.1 + §9.3 で PSO cache hit 率最大化を確定**。

---

## §10 reflection update sync 方式 (= Global\_ReflectionProbes 単一 buffer)

### §10.1 06c §7.3 既述の sync 方式確定

`Global_ReflectionProbes` は singleton + 1 物理 buffer (= triple-buffering 不要)、reflection update 時のみ host write:

- update タイミング = `LLReflectionMapManager::updateProbeFace()` 等 (= 既存 OpenGL path、inventory §1)
- write hazard 回避 = **`VkFence` 経由 GPU 完了待ち**:

```cpp
void writeGlobalReflectionProbes(const GlobalReflectionProbes& data)
{
    // 1. 前 frame の GPU read 完了を待つ
    vkWaitForFences(sDevice, 1, &sReflectionUpdateFence,
                    VK_TRUE, UINT64_MAX);
    vkResetFences(sDevice, 1, &sReflectionUpdateFence);

    // 2. host write
    std::memcpy(sGlobalReflectionProbesMapped, &data, sizeof(data));

    // 3. next 完了 signal = endFrame() の vkQueueSubmit に sReflectionUpdateFence
}
```

reflection update **頻度 << frame 頻度** (= 数 frame に 1 回〜数秒に 1 回) のため、fence wait cost は無視可能。

### §10.2 update 頻度の log 出力

reflection update 1 回ごとに LL_DEBUGS 出力 (= 起動 1 分間に 100 回超なら warn):
- AYA 実機で update 頻度を観察 → 過剰なら chapter 09 Phase で update throttle 検討

---

## §11 chapter 04 / 06a / 06b / 06c / 08 / 09 との分担境界

| chapter | 本 chapter からの入力 | 本 chapter への出力 |
|---|---|---|
| **04** (codegen-ubo) | `ubo_metadata.inl` (= block_name → set/binding) / std140 layout / alignment 出力 | §3.1 set=1 split (1a/1b) / §7.3 offset alignment / §9.1 共通 layout 反映を Codegen 側で保証 |
| **06a** (cache-structure-and-setter-redirect) | `mUseUBO` flag / `mUniformUBOLoc[index]` / sampler 強制 OpenGL path | §5 sampler set=3 配置確定、06a §5.6 注記更新 |
| **06b** (cadence-update-site-and-dirty) | flush 関数 5 種 / L1+L2 / triple-buffering | §7 dynamic offset bind 実装 / §8 5 cadence rotate / §10 reflection fence sync |
| **06c** (descriptor-set-bind-wiring) | set 4 帯 → 5 帯拡張案 (§4.4) / sampler 配置決定 (§5) / triple-buffering 全 cadence 波及 (§8) | (本 chapter は提供側) — 06c §2 表 / §3 接合表 / §8 sampler placeholder を本 chapter §4.4 / §5.4 / §8.3 で確定形に書換え |
| **08** (build-codegen-pipeline) | Codegen 出力 `ubo_metadata.inl` の set 帯 5 化 (= set=0/1a/1b/2/3) + sampler binding | (本 chapter は提供側) — set=1 split 実装、padding alignment 256 B 出力契約 |
| **09** (phase-roadmap) | `LLPhaseMigrationList::isUboReady()` / cvar `AYAUboRedirectEnabled` / pool grow phase 計画 | (本 chapter は提供側) — §6 per-asset pool grow / §7.2 ring buffer grow / §3.3 (V1) 実機計測 入り口を Phase に登録 |

---

## §12 未確定事項 (→ chapter 08 / 09 / 10 持越 / AYA 判断仰ぎ)

| # | 項目 | 解消先 | default 採用案 |
|---|---|---|---|
| (V1') | set=1 79 binding → 40/39 split 採用 | **chapter 10 / AYA 判断** | **本 chapter §3.2 で V1' default、AYA 判断仰ぎ候補** |
| (V3') | 全 program 共通 layout (V3a) vs program 別 (V3b) | **chapter 10 / AYA 判断** | **本 chapter §4.2 で V3a default、AYA 判断仰ぎ候補** |
| (S3') | sampler 49 set=3 per-asset 同居 vs 別案 | **chapter 10 / AYA 判断** | **本 chapter §5.2 で S3' default、AYA 判断仰ぎ候補** |
| (W) | `sProgramUboPool` maxSets = 6 (active × 3 × 2) vs 1200 (shader 数 × 3 × 2) | **chapter 10 / AYA 判断** | **本 chapter §6.2 で maxSets=6 default、AYA 判断仰ぎ候補** |
| (W2) | `sAssetUboPool` 起動時 prealloc N=64 / grow chunk 64 | **chapter 09 Phase Roadmap** | N=64 prealloc + 64 grow chunk default |
| (R1) | ring buffer 起動時 4 MB / 上限 16 MB | **chapter 09 Phase Roadmap** | initial=4 / max=16 default、cvar `AYARingBufferSizeMB` で配信 |
| (PSC) | PSO cache disk persist path / 容量上限 | **chapter 09 Phase Roadmap** | `~/.ayastorm_x64/cache/pipeline_cache.bin`、上限 64 MB |
| (RF) | reflection update fence throttle (> 100 回/分で warn) | 実装 phase 入口 | LL_DEBUGS log + chapter 09 Phase で throttle 判定 |

---

## §13 update 規律

- §2 棚卸しは `llvkloader.{cpp,h}` 更新時に live 反映 (= 行番号 / 構造体 field 等の drift 防止)
- §3 device limit struct field 追加は本 chapter に集約、Codegen `ubo_metadata.inl` 出力との整合 chapter 04 §5.1 へ波及
- §4.4 set 帯 5 化 + bind 時 4 set 制約 は本 chapter で確定、06c §2 / §3 表を chapter 07 確定後に書換え
- §5 sampler 配置確定 (S3') は 06a §5.6 注記 + 06c §3 接合表 / §8 を chapter 07 確定後に書換え
- §6 pool 容量算定は chapter 09 Phase Roadmap で per-asset/per-skin 動的 grow 機構実装後、本 chapter §6.3 を確定形に書換え
- §7 ring buffer 容量 / chunk 構造 は実装 phase 入口で AYA 実機計測値 reflect、本 chapter §7.2 を update
- §8 全 cadence rotate は実装で各 cadence 段階的に配線、本 chapter §8.3 表を **段階 reflect**
- §10 reflection fence は実装で `LLReflectionMapManager::updateProbeFace()` への hook 追加後、本 chapter §10.1 コードスニペットを実装形に reflect
- §12 (V1')(V3')(S3')(W)(W2)(R1)(PSC)(RF) 持越は AYA 判断 / 実装 phase / chapter 09/10 で消化したら本 chapter §12 から「保留候補」を剥がして reflect

---

**= 本 chapter で現状 Vulkan API 実装棚卸し + device limit query 拡張 + set=1 layout 方式 (V3a 全共通) + set=1 79 → 40/39 split (V1') + sampler 49 set=3 per-asset 同居 (S3') + descriptor pool 4 cadence split + 容量算定 + dynamic offset ring buffer 4 MB + triple-buffering 5 cadence 全適用 + 共通 PSO layout + reflection fence sync が確定したため、chapter 06 確定の redirect 層 / cadence / dirty / set 配置 を実 Vulkan API call レベルで満たすための実装仕様を確定 state に到達。chapter 08 (build-codegen-pipeline) で Codegen 側 build 統合 + `ubo_metadata.inl` 出力契約に進める**。
