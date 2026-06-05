# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-7δ design-lock** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-7δ (= vkCmdBindDescriptorSets 通電 + set=3 swap + sAYAStandardLayout 経由 bind + V3a 5-set 構成完成 + SINGLETON case llassert_always → flushSingletonUbos 本格化) 着手前 design-lock + ambiguity 9+1 件 AYA 確認 record

---

## §0. PC-7δ literal scope (= AYA 指示 2026-06-05)

1. **vkCmdBindDescriptorSets 通電** = V3a 5-set bind 経路の実 firing
2. **set=3 swap** = rigged GLTF draw 切替時の set=2 ↔ set=3 swap helper 整備 (= design 07 §4.4.1 / §9.2)
3. **sAYAStandardLayout 経由 bind** = 既存 placeholder draw の bind layout を sSkySmokeLayout / sAvatarBoneLayout → sAYAStandardLayout に migrate
4. **V3a 5-set 構成完成** = VkDescriptorSet 実体 allocate + vkUpdateDescriptorSets + vkCmdBindDescriptorSets 通電
5. **SINGLETON case llassert_always → flushSingletonUbos 本格化** = forwardToUboUpload SINGLETON case 通電

**scope 外**:
- PC-7ε (= dynamic offset 経路 ring buffer chunk hand-off)
- PC-7α' (= codegen ubo_metadata.inl V1' set=1a/1b split)
- PC-8 (= 3 OS build verify)
- 実 GLTF Vulkan draw 通電 (= PC-N 以降、本 PC-7δ では placeholder draw に bind path 反映のみ)

---

## §1. 必読 3 件 (本 design-lock 起案前 Read 済)

1. **PC-7γ-3 complete handoff** = `handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-gamma-3-complete.md` (= codegen Asset_*/Skin_* 3 block 追加 + GLSL block 名 rename + dual-write 配線 + lifecycle hook 完了)
2. **design 06c §2-§5** (`docs/specs/ayastorm-r41-gl-removal/design/06c-descriptor-set-bind-wiring.md`) = V3a 5-set bind 配線、§4 flush 直後 bind sequence、§5 dynamic offset bind 側責務、§7 triple-buffering set=0 rotate 方式 A
3. **design 07 §4 / §7 / §8 / §9** (`docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md`) = §4.4.1 論理 5 set → bind 時 4 set 縮減、§9.1 sAYAStandardLayout、§9.2 set=3 swap configuration、§8.4 FRAMES_IN_FLIGHT 同期 rotate

---

## §2. 現状調査結果 (= design-lock 判断材料)

### §2.1 Vulkan 描画経路の実態

| 項目 | 現状 |
|------|------|
| 実 GLTF / per-pool draw | **全 OpenGL** 経由 (Vulkan command buffer 未通過) |
| Vulkan command buffer 内の draw call | placeholder のみ = `recordPlaceholderPoolDraw` (12 pool 共用 fullscreen tri) + `recordAvatarPlaceholderDraw` (avatar push descriptor) |
| placeholder 描画先 | `sFramebuffer` offscreen FBO、画面到達なし (= 視覚 no-op 等価) |
| placeholder pipeline | `sSkySmokePipeline` / `sAvatarBonePipeline` (= 既存) |
| placeholder bind layout | **`sSkySmokeLayout` / `sAvatarBoneLayout`** (= PC-7α `sAYAStandardLayout` 不使用) |

### §2.2 V3a 5-set scaffolding 現状 (PC-7α 完了)

| 構成要素 | 現状 |
|---|---|
| 5 VkDescriptorSetLayout | object 化済 (`sFrameUboLayoutV3a` / `sProgramUboLayoutA` / `sProgramUboLayoutB` / `sDrawUboLayoutV3a` / `sAssetUboLayoutV3a`) |
| 4 VkDescriptorPool | object 化済 (`sFrameUboPoolV3a` / `sProgramUboPoolV3a` / `sDrawUboPoolV3a` / `sAssetUboPoolV3a`) |
| VkPipelineLayout | `sAYAStandardLayout` object 化済 (5 set + push constant 64 B、VERTEX|FRAGMENT) |
| VkDescriptorSet 実体 | **0 件 allocate** = PC-7α では layout/pool/pipeline-layout のみ |
| vkUpdateDescriptorSets | **0 件 invoke** = descriptor → buffer 紐付け未実施 |
| vkCmdBindDescriptorSets via sAYAStandardLayout | **0 件 invoke** = bind 経路未通電 |

### §2.3 flush\*Ubos 6 件の現状 (PC-7γ-1..γ-3 完了)

| function | 配置 site (llvkloader.cpp) | 現状 |
|---|---|---|
| `flushFrameUbos` | line 3881-3891 | `flushDummyUboWrite("flushFrameUbos")` 単独 + beginFrame() frame index advance (= per-frame cadence 専用) |
| `flushProgramUbos(shader)` | line 3906-3934 | dirty entry walk + `exchange(false)` 既配線 + `flushDummyUboWrite("flushProgramUbos")` |
| `flushDrawUbos` | line 3936-3943 | `flushDummyUboWrite("flushDrawUbos")` 単独 (= per-draw は PC-7ε scope) |
| `flushAssetUbos(asset)` | line 3960-3980 | dirty walk + match check + `flushDummyUboWrite("flushAssetUbos")` |
| `flushSkinUbos(skin)` | line 3985-4005 | dirty walk + match check + `flushDummyUboWrite("flushSkinUbos")` |
| `flushSingletonUbos` | line 4021-4024 | `flushDummyUboWrite("flushSingletonUbos")` 単独 |

→ 全 6 件 = `flushDummyUboWrite()` placeholder のみ、**実 GPU bind / upload 0 件**。

### §2.4 SINGLETON case 現状 (= llglslshader.cpp:2113-2222)

| cadence | forwardToUboUpload case 内容 |
|---|---|
| PER_FRAME | `LLVKLoader::writeFrameUbo(block_hash, offset, data, size)` (PC-7γ-1) |
| PER_PROGRAM | `LLVKLoader::writeProgramUbo(shader, block_hash, offset, data, size)` (PC-7γ-1) |
| PER_DRAW | `LL_WARNS_ONCE("PC-7ε scope")` stub |
| PER_ASSET | `getCurrentAsset() + writeAssetUbo` (PC-7γ-2 defensive) |
| PER_SKIN | `getCurrentSkin() + writeSkinUbo` (PC-7γ-2 defensive) |
| **SINGLETON** | **`llassert_always(false && "SINGLETON forwarded via forwardToUboUpload unexpected ...")`** ← **PC-7δ 対象** |
| SAMPLER / UNKNOWN / INVALID | defensive return |

`bringupTestUBO()` (line 2256-2283) = `LLVKLoader::flushSingletonUbos()` 直呼出済 (= Global_ReflectionProbes singleton)。
→ PC-7δ では SINGLETON setter 経由 write も `writeSingletonUbo` helper 経由で受ける必要あり (= setter ↔ flush の 2 経路統合)。

### §2.5 per-frame Vulkan envelope 確認

| 呼出 site | function |
|---|---|
| `llappviewer.cpp:1774-1786` | `LLVKLoader::beginFrame() → gPipeline.recordVulkanPools() → display() → LLVKLoader::endFrame()` |
| `pipeline.cpp:4956-4973` | `LLPipeline::recordVulkanPools()` walks mPools → `poolp->recordPoolDraws(cmd_buf)` |
| `pipeline.cpp:5114` | `renderGeomDeferred` 入口で `LLVKLoader::flushFrameUbos()` 呼出済 (= per-frame cadence flush 起点) |

→ Vulkan command buffer 内で vkCmdBindDescriptorSets 呼出可能な timing は **`recordVulkanPools()` 内 12 pool walk** = placeholder draw 経路。

### §2.6 GATE-B / MUSEUBO-A 整合確認

- **GATE-B** (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件) = PC-7δ 改変は host C++ Vulkan init 層 + LLGLSLShader::forwardToUboUpload SINGLETON case のみ、`#ifdef LL_VULKAN_GLSL` 新規追加なし
- **MUSEUBO-A** (= mUseUBO=false default で既存 OpenGL 描画 100% 維持) = PC-7δ で vkCmdBindDescriptorSets 実発火しても、bind 先は placeholder draw 経路 (= sFramebuffer offscreen、画面到達なし) ゆえ実 OpenGL 描画には影響ゼロ

---

## §3. AYA 確認済 ambiguity 9+1 件 (= 2026-06-05 literal「OK」受領)

### (H1) bind path target = 「通電」literal の対象

**判断点**: 実 draw が全 OpenGL のため、vkCmdBindDescriptorSets 実 firing は placeholder draw 経路でしか達成不能。

| 案 | 内容 |
|---|---|
| **(H1-A)** placeholder draw 2 件を sAYAStandardLayout + V3a 5-set bind に migrate (= **Claude 推奨**) | `recordPlaceholderPoolDraw` + `recordAvatarPlaceholderDraw` 内 bind layout を sSkySmokeLayout/sAvatarBoneLayout → sAYAStandardLayout に置換 + V3a 5-set (set=0/1a/1b/2/3) bind 経路追加 |
| (H1-B) 既存 placeholder bind 不変、V3a 5-set bind は flush\*Ubos 内 dummy bind 追加 | bind 実行されるが draw が消費しない (= scaffolding 完成止まり) |
| (H1-C) V3a descriptor set allocate + update のみ、bind は PC-N へ持越 | 「通電」literal 違反 |

**AYA 確認**: (H1-A) 採用 = 2026-06-05 literal「OK」

**根拠**:
- 「通電」literal 整合 (= vkCmdBindDescriptorSets が実 draw 経路で firing)
- placeholder draw が fullscreen tri × offscreen FBO で視覚 no-op 等価 = MUSEUBO-A 整合維持
- sAYAStandardLayout が初めて actual draw 経路に投入 = V3a 5-set 構成完成 literal 達成

### (H2) VkDescriptorSet allocation timing

**判断点**: VkDescriptorSet 実体の確保 timing。

| 案 | 内容 |
|---|---|
| **(H2-A)** initVulkan で V3a pool から eager allocate (= **Claude 推奨**) | FRAMES_IN_FLIGHT=3 × cadence 固定数を一括 allocate |
| (H2-B) lazy allocate (= 初回 register/flush 時) | register hook 内 allocate、cold launch 時の latency 散発 |
| (H2-C) per-frame allocate + free (= beginFrame で取得、endFrame で free) | descriptor pool churn 増、grow only 設計に反する |

**AYA 確認**: (H2-A) 採用 = 2026-06-05 literal「OK」

**根拠**:
- design 07 §6.4 grow only pool 整合
- initVulkan timing = sAllocator 生存後で safe
- per-frame churn 回避 (= scaffolding fixed)

**eager allocate 数** (= PC-7δ scope 内):
- set=0 sFrameUboSetV3a × FRAMES_IN_FLIGHT (= 3) (per-frame + singleton 同居)
- set=1a sProgramUboSetA × FRAMES_IN_FLIGHT (= 3) (PC-7δ では default shader 1 件のみ、後段 multi-shader は PC-N)
- set=1b sProgramUboSetB × FRAMES_IN_FLIGHT (= 3) (同上)
- set=2 sDrawUboSetV3a × 1 (= ring buffer + dynamic offset で 1 set 固定、design 07 §7.4)
- set=3 sAssetUboSetV3a × FRAMES_IN_FLIGHT (= 3) (per-asset+per-skin 同居、PC-7δ では asset 1 件 demo 規模)

→ **計 13 set** (= 3 + 3 + 3 + 1 + 3) を initVulkan で 1 度に allocate

### (H3) vkUpdateDescriptorSets timing

**判断点**: descriptor → UBO buffer 紐付け timing。

| 案 | 内容 |
|---|---|
| **(H3-A)** register\*Ubo 内 = UboInstance 確保直後に update (= **Claude 推奨**) | per-instance pair で hot path 除外、register-once + bind-many |
| (H3-B) flush\*Ubos 内 dirty 検出時 update | per-flush call、hot path 内 cost |
| (H3-C) initVulkan 1 度のみ update + frame index 別 set | scaffolding 内静的紐付け、register 不在の per-frame UBO 用 |

**AYA 確認**: (H3-A) 採用 = 2026-06-05 literal「OK」

**根拠**:
- hot path 除外 (= register 1 回 → bind 多数)
- UboInstance 確保と同 site で update 配置 = diff localize
- design 07 §8.4 frame index 別 set で frame rotate 対応

**実装場所**:
- **per-frame (set=0)**: initVulkan PER_FRAME 3 block allocate ループ末で update (= block_hash → buffer 紐付け、FRAMES_IN_FLIGHT=3 別 set)
- **per-program (set=1a / 1b)**: `registerProgramUbo` 内 allocate 後 update (= 1 program × 3 frame × 2 帯 = 6 update)
- **per-draw (set=2)**: ring buffer fixed allocate (PC-7ε で dynamic offset 動作)、PC-7δ では bind 経路のみ整備
- **per-asset (set=3)**: `registerAssetUbo` 内 allocate 後 update (= 1 asset × 3 frame = 3 update)
- **per-skin (set=3)**: `registerSkinUbo` 内 allocate 後 update (= 1 skin × 3 frame = 3 update、set=3 を asset と共有)
- **singleton (set=0 同居)**: `flushSingletonUbos` 初回 firing 時 lazy update (= dirty 検出時 1 度のみ、以降 reuse)

### (H4) set=3 swap 実走 timing

**判断点**: rigged GLTF draw 切替時の set=2 ↔ set=3 swap。

| 案 | 内容 |
|---|---|
| (H4-A) placeholder draw に set=3 swap demo 組込 | rigged placeholder で set=2 → set=3 swap、demo 用 |
| **(H4-B)** bind helper 整備のみ、実 swap は実 GLTF Vulkan draw 通電 sub-step (PC-N 以降) で発火 (= **Claude 推奨**) | bind helper API として整備、PC-7δ では actual swap firing なし |
| (H4-C) set=3 bind 経路一切なし、set=2 までで止める | literal「set=3 swap」違反 |

**AYA 確認**: (H4-B) 採用 = 2026-06-05 literal「OK」

**根拠**:
- 実 GLTF Vulkan draw 不在段階で実 swap firing は意味なし
- bind helper API 整備 (= `bindV3aSet3()` / `bindV3aSet2()` 等の helper 関数 + 4-set bind 構成 swap rule 明文化) で PC-N 通電準備
- design 07 §9.2 set=3 swap configuration の API 形を本 PC-7δ で確定

**実装形** (= placeholder draw 内):
- `recordPlaceholderPoolDraw`: set=0 + set=1a + set=1b + set=2 を bind (= static draw)
- `recordAvatarPlaceholderDraw`: set=0 + set=1a + set=1b + set=3 を bind (= rigged draw、set=2 ↔ set=3 swap 実走)
- → avatar pool は rigged の placeholder ゆえ set=3 swap demo を兼ねる

### (H5) SINGLETON case 本格化 (= forwardToUboUpload SINGLETON case)

**判断点**: `llassert_always(false)` → ? の置換。

| 案 | 内容 |
|---|---|
| **(H5-A)** `LLVKLoader::writeSingletonUbo(block_hash, offset, data, size)` 新設 + 呼出 (= **Claude 推奨**) | setter 経由 write 経路、flushSingletonUbos が内部利用する 2 階層構造 |
| (H5-B) `LLVKLoader::flushSingletonUbos()` 直呼出 | setter から flush 同経路、bringupTestUBO と同形 |
| (H5-C) `llassert_always(false)` 維持 | PC-6ε-1 別経路維持、literal 違反 |

**AYA 確認**: (H5-A) 採用 = 2026-06-05 literal「OK」

**根拠**:
- literal「flushSingletonUbos 本格化」整合
- writeSingletonUbo = 単一 block write の primitive、flushSingletonUbos = block 全件 walk + write の composite
- writeFrameUbo / writeProgramUbo / writeAssetUbo / writeSkinUbo と pattern 統一 (= cadence 別 write helper の symmetric extension)
- `bringupTestUBO()` 既呼出 (`flushSingletonUbos()`) と並列共存 = setter 由来 + bringup 由来両方の write 経路統合

**実装形**:
- LLVKLoader 新 method = `writeSingletonUbo(U32 block_hash, U32 offset, const void* data, size_t size) -> void` (= writeFrameUbo signature 同形)
- 内部 = `sSingletonUboInstances` map (= block_hash key の per-singleton UboInstance) を find + bounds check + memcpy + dirty.store
- forwardToUboUpload SINGLETON case = `LLVKLoader::writeSingletonUbo(block_hash, offset, data, size);` 呼出
- flushSingletonUbos = sSingletonUboInstances walk + dirty exchange + vkCmdBindDescriptorSets via sAYAStandardLayout (set=0 内 singleton 帯 binding)

→ sSingletonUboInstances は **sFrameUboInstances と別 map** (= cadence 隔離、map key 単純化)

### (H6) maxBoundDescriptorSets=4 維持

**判断点**: 5 set 同時 bind 不可、bind 順序 + 4 set 構成。

**design 07 §4.4.1 literal**:
- shader bind 時 (= static draw): set=0 (frame+singleton) + set=1a (program A) + set=1b (program B) + set=2 (per-draw)
- rigged GLTF draw 切替時: set=2 unbind 後 set=3 (per-asset+per-skin) bind = set=0/1a/1b/3 構成

**AYA 確認**: §4.4.1 literal 採用 = 2026-06-05 literal「OK」

**根拠**:
- maxBoundDescriptorSets=4 死守 (= Vulkan 1.3 minimum)
- swap rule 単純化 = static vs rigged の 2 mode のみ
- pipeline layout は logical 5 set 包含、実 bind は 4 set 構成

### (H7) GATE-B 整合

**判断点**: PC-7δ 改変で `#ifdef LL_VULKAN_GLSL` 新規追加なし。

**AYA 確認**: 追加なし採用 = 2026-06-05 literal「OK」

**根拠**:
- PC-7δ は host C++ Vulkan init 層 (= llvkloader.cpp) + LLGLSLShader::forwardToUboUpload SINGLETON case (= llglslshader.cpp) のみ改変
- GLSL 改変 0 件 (= shader 側 layout 宣言は PC-7α' で codegen 同梱、本 PC-7δ scope 外)
- `#ifdef LL_VULKAN_GLSL` は GLSL 専用 macro、C++ では未定義 (= GATE-B literal)

### (H8) MUSEUBO-A 整合 = mUseUBO=false default 維持

**判断点**: vkCmdBindDescriptorSets 実発火後も mUseUBO=false で既存 OpenGL 描画 100% 維持。

**AYA 確認**: (H1-A) で MUSEUBO-A 維持成立 = 2026-06-05 literal「OK」

**根拠**:
- placeholder draw は `sFramebuffer` offscreen FBO に描画 = 画面到達なし = 既存 OpenGL 描画 visible 影響ゼロ
- bind layout 切替 (= sSkySmokeLayout → sAYAStandardLayout) は placeholder 内部の不可視 transition
- mUseUBO=false default で setter→forwardToUboUpload 不到達、flush\*Ubos 内 dirty 不立 → vkCmdBindDescriptorSets 実発火経路は placeholder draw 内のみ (= UBO instance 内容は dummy / initial state)

### (H9) PC-7δ scope cadence 範囲

**判断点**: V3a 5-set 全 cadence bind 配線 vs 部分通電。

| 案 | 内容 |
|---|---|
| **(H9-A)** V3a 5-set 全 cadence bind 配線 (= **Claude 推奨**) | PER_FRAME + PER_PROGRAM A/B + PER_DRAW (placeholder bind only) + PER_ASSET/SKIN (set=3) + SINGLETON (set=0 同居) 全通電 |
| (H9-B) 重要 cadence 先行 (PER_FRAME + SINGLETON のみ通電) | literal「V3a 5-set 構成完成」違反 |
| (H9-C) placeholder demo 1 件のみ通電 | literal 違反 |

**AYA 確認**: (H9-A) 採用 = 2026-06-05 literal「OK」

**根拠**:
- literal「V3a 5-set 構成完成」整合
- UBO migration one-at-a-time 規律は **PC-7 全体 sub-step 区切り** (= γ-1/γ-2/γ-3/δ/ε) で成立、cadence 内分割は不要
- per-draw (set=2) は ring buffer fixed allocate のみ実装 = dynamic offset は PC-7ε scope (= scope 重複なし)

### (H10) avatar placeholder pipeline layout compatibility (= design-lock 起案中追加発見)

**判断点**: (H1-A) 採用で `recordAvatarPlaceholderDraw` の bind を sAYAStandardLayout 経由に migrate する場合、avatar pipeline (`sAvatarBonePipeline`) と V3a 5-set bind の pipeline layout compatibility 違反懸念。

**状況**:
- 既存 `recordAvatarPlaceholderDraw` = `sAvatarBonePipeline` + `sAvatarBoneLayout` + push descriptor 経路 (set=2 binding 0 = STORAGE_BUFFER で avatar bone storage buffer × 110 mat4) を bind
- V3a sAYAStandardLayout = 全 UBO (= 5 set 全部 UNIFORM_BUFFER / UBO_DYNAMIC)、STORAGE_BUFFER binding 不在
- Vulkan validation 視点: bound pipeline の layout と bound descriptor sets の layout は compatible 必要 (design 07 §9.3)

| 案 | 内容 | 副作用 |
|---|---|---|
| **(H10-A)** avatar placeholder の push descriptor 経路 (STORAGE_BUFFER) を PC-7δ で disable + bindV3aRigged のみ実行 (= **Claude 推奨**) | 既存 avatar bone storage 経路は scope 外 (= PC-N 実 GLTF Vulkan draw 通電時に再配線) | avatar bone mat 適用なしの fullscreen tri (= 視覚 no-op 等価維持、placeholder ゆえ無問題) |
| (H10-B) sAvatarBonePipeline を sAYAStandardLayout 経由で再構築 = pipeline layout 置換、push descriptor 経路は別 binding に migrate | 既存 PSO 構築コード touch、storage buffer binding 用に sAYAStandardLayout 拡張 or 別 layout 併用 | sAYAStandardLayout に STORAGE_BUFFER 追加 = V3a 5-set 設計から逸脱 |
| (H10-C) avatar placeholder の bind は既存 sAvatarBoneLayout 維持 + V3a 5-set bind は別 frame phase で実施 | placeholder 経路 2 種 (= old + new) 並走、bindV3aRigged 実 firing 経路がなくなる | 「set=3 swap 通電」literal 違反 (= set=3 swap demo 経路消失) |

**AYA 確認**: (H10-A) 採用 = 2026-06-05 literal「(H10-A) で進行 OK」

**根拠**:
- avatar bone storage buffer 経路 (= push descriptor + STORAGE_BUFFER) は PC-7δ scope 外 (= 実 GLTF avatar Vulkan draw 通電時に再配線)
- (H10-A) で `bindV3aRigged` が avatar placeholder で実 firing = set=3 swap literal 達成
- placeholder ゆえ視覚 no-op 等価 (= avatar bone mat 不適用でも視覚影響ゼロ)
- pipeline layout 再構築 (H10-B) は scope 拡大、(H10-C) は literal 違反

**実装形** (= §4.1.2 step (j) 修正):
- `recordAvatarPlaceholderDraw` 内 `if (!sDeviceLimits.pushDescriptorSupported || ...)` fallback path を維持 + push descriptor 呼出本体を skip (or `#if 0` 相当 disable / 別 path 経由)
- push descriptor 経路の代わりに `bindV3aRigged(cmd_buf, sFrameIndex)` 呼出で V3a 5-set rigged bind 通電
- pipeline は **sAvatarBonePipeline の pipeline layout を sAYAStandardLayout 経由で再構築** = 既存 sAvatarBoneLayout 廃止 (= PC-7δ scope 内で safe disable)、または pipeline 自体を sAYAStandardLayout 経由の新 PSO (= `sAvatarV3aPipeline` 別建て) で構築

**Claude 推奨実装形 = sAvatarBonePipeline の pipeline layout を sAYAStandardLayout で再構築** (= PSO は 1 件維持、avatar push descriptor 経路は PC-7δ で disable + PC-N で再配線):
- 単一 PSO で sAYAStandardLayout 完全準拠
- push descriptor 経路の disable は PC-7δ literal scope 外 (= avatar bone storage は別 foundation)、PC-N で実 GLTF avatar Vulkan draw 通電時に再配線 (= storage buffer は別 layout 路で再導入)

---

## §4. PC-7δ 実装計画 (= AYA 確認後実施)

### §4.1 §0 scope 5 件分解

#### §4.1.1 scope 1+4: VkDescriptorSet 実体 allocate + vkUpdateDescriptorSets + vkCmdBindDescriptorSets 通電

| step | 内容 | file 改変 |
|---|---|---|
| (a) anonymous ns に sFrameUboSetV3a / sProgramUboSetA / sProgramUboSetB / sDrawUboSetV3a / sAssetUboSetV3a 5 件 static array 追加 (= H2-A eager allocate 受皿) | `llvkloader.cpp` anonymous ns |
| (b) `createV3aDescriptorSets()` helper 新設 (= initVulkan で V3a pool から 13 set allocate) | `llvkloader.cpp` anonymous ns |
| (c) initVulkan 配線 = `createV3aDescriptorPools()` 後 `createV3aDescriptorSets()` 呼出 | `llvkloader.cpp` initVulkan |
| (d) PER_FRAME 3 block allocate ループ末に vkUpdateDescriptorSets 呼出 (= set=0 × FRAMES_IN_FLIGHT 別 frame index で update) | `llvkloader.cpp` initVulkan |
| (e) `registerProgramUbo` / `registerAssetUbo` / `registerSkinUbo` 末尾に vkUpdateDescriptorSets 呼出追加 (= 各 method 内 UboInstance 確保直後) | `llvkloader.cpp:4146+` / `4178+` / `4246+` |
| (f) shutdownVulkan teardown = V3a 5 set は pool destroy 時に implicit free (= 別途 vkFreeDescriptorSets 呼出不要、grow only pool 整合) | `llvkloader.cpp` shutdownVulkan |

#### §4.1.2 scope 2+3: placeholder draw に sAYAStandardLayout bind 配線 (= set=3 swap helper 整備)

| step | 内容 | file 改変 |
|---|---|---|
| (g) `bindV3aStatic(cmd_buf, frame_index)` helper 新設 = set=0 + set=1a + set=1b + set=2 を 1 度に bind (= static draw 用 4 set 構成、vkCmdBindDescriptorSets 1 呼出で firstSet=0 + descriptorSetCount=4) | `llvkloader.cpp` anonymous ns |
| (h) `bindV3aRigged(cmd_buf, frame_index)` helper 新設 = set=0 + set=1a + set=1b + set=3 を 1 度に bind (= rigged draw 用 4 set 構成、set=3 swap 実現) | `llvkloader.cpp` anonymous ns |
| (i) `recordPlaceholderPoolDraw` 内 bind 経路を `bindV3aStatic` 呼出に migrate = pipeline は sSkySmokePipeline 維持、bind layout は sAYAStandardLayout 経由 | `llvkloader.cpp:4355-4407` |
| (j) `recordAvatarPlaceholderDraw` 内 bind 経路を `bindV3aRigged` 呼出に migrate = pipeline は sAvatarBonePipeline 維持、bind layout は sAYAStandardLayout 経由 + 既存 push descriptor 経路 (set=2 binding 0 avatar bone storage) は set=3 swap 後 conflict なきよう調整 (= push descriptor は set=2 binding 0 → set=3 binding 0 等の新位置に migrate、ただし v3a layout の set=3 binding 0 が UBO 型ゆえ STORAGE_BUFFER 型衝突 = require care) |

**(j) の方針 = (H10-A) 採用 (= 2026-06-05 AYA literal「(H10-A) で進行 OK」record)**:
- `recordAvatarPlaceholderDraw` の **push descriptor 経路 (STORAGE_BUFFER) を PC-7δ で disable** (= avatar bone storage buffer bind 経路 skip)
- bindV3aRigged 呼出で V3a 5-set rigged bind 経路通電 (= set=3 swap literal 達成)
- placeholder ゆえ視覚 no-op 等価 (= avatar bone mat 不適用でも視覚影響ゼロ)
- pipeline = sAvatarBonePipeline の pipeline layout を **sAYAStandardLayout 経由で再構築** (= 既存 sAvatarBoneLayout 廃止 / placeholder PSO 再構築)
- push descriptor 経路は PC-N で実 GLTF avatar Vulkan draw 通電時に再配線 (= storage buffer は別 layout 路で再導入)

**`recordPlaceholderPoolDraw` 同様 pipeline layout 確認**:
- 既存 sSkySmokePipeline の pipeline layout は sSkySmokeLayout (= PC-7α 以前)、sAYAStandardLayout と非互換
- (H10-A) の sAvatarBonePipeline 再構築と同様に、**sSkySmokePipeline も pipeline layout を sAYAStandardLayout 経由で再構築** = 両 placeholder PSO を統一 sAYAStandardLayout 配下に集約
- 既存 sSkySmokeLayout / sAvatarBoneLayout は本 PC-7δ で deprecate (= shutdownVulkan teardown 維持で leak なし、再構築後 obsolete)

**実装形**:
- (j-A) sSkySmokePipeline 再構築 = pipeline layout を sAYAStandardLayout に置換 (= 既存 builder 関数の layout 引数差替)
- (j-B) sAvatarBonePipeline 再構築 = 同上 + push descriptor 経路 disable (= STORAGE_BUFFER binding 経路 skip + bindV3aRigged 通電)
- (j-C) shutdownVulkan で旧 sSkySmokeLayout / sAvatarBoneLayout は依然 destroy (= PSO 再構築で参照失効、shutdown 時 destroy は safe)
- (j-D) `bindPerMaterialDescriptorSet` helper (= 既存 set=1 PerMaterial bind 経路) は本 PC-7δ で deprecate 経路 (= V3a 5-set bind が代替)、ただし `beginFrame` で別途呼出 (= 段階 3 sky transit smoke) あり、本 PC-7δ では beginFrame 経路の helper 呼出は維持 + recordPlaceholderPoolDraw / recordAvatarPlaceholderDraw 経路のみ migrate

#### §4.1.3 scope 5: SINGLETON case 本格化 (= flushSingletonUbos 本格化)

| step | 内容 | file 改変 |
|---|---|---|
| (k) `sSingletonUboInstances` map + `UboInstance` key=U32 block_hash 単独 (= sFrameUboInstances 同形、map 構造別) | `llvkloader.cpp` anonymous ns |
| (l) `writeSingletonUbo(U32 block_hash, U32 offset, const void* data, size_t size) -> void` 新設 (= writeFrameUbo signature 同形) | `llvkloader.cpp` anonymous ns + extern decl `llvkloader.h` |
| (m) initVulkan SINGLETON cadence allocate ループ追加 = codegen ubo_metadata.inl から SINGLETON tag entry を walk + 各 block allocate + sSingletonUboInstances insert (= 現 codegen で SINGLETON 1 件 `Global_ReflectionProbes`) | `llvkloader.cpp` initVulkan |
| (n) shutdownVulkan SINGLETON teardown = sSingletonUboInstances walk + destroyUboInstanceBuffers + clear | `llvkloader.cpp` shutdownVulkan |
| (o) `flushSingletonUbos` 本格化 = sSingletonUboInstances walk + dirty exchange + vkCmdBindDescriptorSets via sAYAStandardLayout (= set=0 内 singleton 帯 binding、ただし本 PC-7δ では実 bind は placeholder draw 経路で実行、flushSingletonUbos 内 bind は scaffolding 内部の no-op safe path) | `llvkloader.cpp:4021-4024` |
| (p) `forwardToUboUpload` SINGLETON case 本格化 = `llassert_always(false)` → `LLVKLoader::writeSingletonUbo(block_hash, offset, data, size);` | `llglslshader.cpp:2113-2222` SINGLETON case |

### §4.2 GATE-B / MUSEUBO-A 整合確認

- GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 = host C++ + LLGLSLShader::forwardToUboUpload 改変のみ、GLSL 0 件
- MUSEUBO-A = vkCmdBindDescriptorSets 実発火は placeholder draw 経路 (= sFramebuffer offscreen、視覚 no-op 等価)、実 OpenGL 描画影響ゼロ

### §4.3 ASSET/SKIN 5-set scaffolding 完成と PER_DRAW (set=2) bind

| cadence | set | bind 経路 (PC-7δ) | actual UBO content (PC-7δ) |
|---|---|---|---|
| PER_FRAME / SINGLETON | set=0 | bindV3aStatic / bindV3aRigged 内 | sFrameUboInstances (PC-7γ-1 既存) + sSingletonUboInstances (PC-7δ 新設) で initial state / dirty exchange 経由 |
| PER_PROGRAM A | set=1a | 同上 | sProgramUboDirty (PC-7γ-1 既存) で initial state / dirty exchange 経由 |
| PER_PROGRAM B | set=1b | 同上 | sProgramUboDirty (PC-7γ-1 既存) で initial state |
| PER_DRAW | set=2 | bindV3aStatic 内 (rigged は set=3 で swap、static は set=2 で bind) | sDrawUboRingBufferMgr 経由 ring buffer (PC-6β 既存)、dynamic offset は PC-7ε scope = PC-7δ では offset=0 固定 |
| PER_ASSET / PER_SKIN | set=3 | bindV3aRigged 内 | sAssetUboInstances (PC-7γ-2 既存) + sSkinUboInstances (PC-7γ-2 既存) で initial state / dirty exchange 経由 |

→ **per-draw (set=2)** = sDrawUboRingBufferMgr の最初の chunk を fixed allocate して buffer 紐付け (= bind helper の vkCmdBindDescriptorSets には dynamicOffset=0 1 件渡し、ring buffer の actual offset 計算は PC-7ε で配線)

---

## §5. Exit Criteria 11 項 (= PC-7δ 完了判定)

| # | Exit Criteria |
|---|---|
| (i) | sFrameUboSetV3a / sProgramUboSetA / sProgramUboSetB / sDrawUboSetV3a / sAssetUboSetV3a 5 件 static array + `createV3aDescriptorSets()` helper + initVulkan 配線 (= H2-A eager allocate 計 13 set) |
| (ii) | vkUpdateDescriptorSets 呼出 = PER_FRAME initVulkan 内 3 件 + registerProgramUbo / registerAssetUbo / registerSkinUbo 末尾 各 method 内 (= H3-A register 時 update) |
| (iii) | sSingletonUboInstances map + writeSingletonUbo helper + initVulkan SINGLETON allocate ループ + shutdownVulkan teardown (= H5-A SINGLETON 本格化 foundation) |
| (iv) | forwardToUboUpload SINGLETON case 本格化 = `llassert_always(false)` → `writeSingletonUbo(...)` (= H5-A 通電) |
| (v) | bindV3aStatic / bindV3aRigged helper 新設 = vkCmdBindDescriptorSets で 4 set 構成 bind (= H6 maxBoundDescriptorSets=4 死守、set=2 ↔ set=3 swap) |
| (vi) | recordPlaceholderPoolDraw bind path migrate = bindV3aStatic 呼出 + sSkySmokePipeline の pipeline layout 再構築 (= sAYAStandardLayout 統一、H10-A 整合) |
| (vii) | recordAvatarPlaceholderDraw bind path migrate = bindV3aRigged 呼出 + sAvatarBonePipeline の pipeline layout 再構築 + push descriptor 経路 disable (= H1-A + H10-A 通電) |
| (viii) | flushSingletonUbos 本格化 = sSingletonUboInstances walk + dirty exchange + vkCmdBindDescriptorSets via sAYAStandardLayout (= H5-A flush 本格化) |
| (ix) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 |
| (x) | MUSEUBO-A 整合 = mUseUBO=false default で既存 OpenGL 描画 100% 維持、bind 実発火は placeholder offscreen FBO 経路 |
| (xi) | build verify = llrender + newview TU rebuild PASS + warning 0 + Vulkan validation 0 件 (= bind compatibility check) + TUT 3 件 (11+10+13) + codegen 130/130 PASS |

---

## §6. PC-7δ 着手手順

1. (AYA 確認受領済) **9 件 ambiguity AYA literal「OK」 record 完了 (2026-06-05)**
2. (本 design-lock doc 完成) 着手前の design 確定
3. **実装 phase 起動** (= §4.1 step (a)-(p) 順序実施)
   - 実装途中で `recordAvatarPlaceholderDraw` の pipeline layout compatibility (= §4.3) 観察必要時、AYA に追加確認 (= avatar push descriptor disable 案 (j-3) の採否)
4. 各 step 完了ごとに incremental build + Vulkan validation 観察
5. 全 step 完了後 §5 Exit Criteria 11 項 self-verify
6. PC-7ε / PC-7α' 次 session 引継 marker 作成 = `handoff-substep-...-pc-7-delta-complete.md`
7. AYA に handoff doc 提示 + 「commit してください」literal 受領待ち
8. literal 受領後 commit (= feedback_no_auto_commit + feedback_no_claude_coauthor 遵守)

---

## §7. 引き継ぎ memory 14 件 遵守確認

- **feedback_proactive_handoff**: 本 design-lock doc は AYA 確認後 phase の起案、handoff 候補 doc 作成済
- **feedback_handoff_minimal_pre_req_read**: 必読 3 件 + pinpoint reference 別記 (§1 / §2)
- **feedback_self_verify_before_handoff**: §5 Exit Criteria 11 項を verify 対象として明文化
- **feedback_build_only_verified**: 実装 phase で TUT 11+10+13 + codegen 130/130 + Vulkan validation 0 件 literal 検証取得
- **feedback_no_scope_shrink**: PC-7δ literal scope 5 件全件 §0 で確認 + §4.1 で完全分解、scope 縮小なし
- **feedback_doubt_self_first**: 9 件 ambiguity 発見で停止 + 推奨案提示 + AYA 確認 + literal「OK」受領
- **feedback_confirm_referent_before_acting**: 9 件 batch AYA 確認、推測実装なし
- **feedback_ubo_migration_one_at_a_time**: PC-7δ = vkCmdBindDescriptorSets 通電 + set=3 swap + V3a 5-set 完成 + SINGLETON 本格化を 1 sub-step で実施、dynamic offset (PC-7ε) + codegen V1' (PC-7α') は分離
- **feedback_design_phase_no_code_write**: 本 design-lock phase では `indra/` 配下改変 0 件、code write 実装 phase 移行後
- **feedback_release_branch_workflow**: feature branch `feature/ayastorm-r41-gl-removal` 上で実装 + commit
- **feedback_no_auto_commit**: 「commit してください」literal 受領まで commit せず
- **feedback_no_claude_coauthor**: Co-Authored-By: Claude 行不在
- **GATE-B**: `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= §4.2 / §5 (ix))
- **MUSEUBO-A**: mUseUBO=false default で既存 OpenGL 描画 100% 維持 (= §4.2 / §5 (x))

---

**design-lock 確定 = 2026-06-05、AYA 確認受領 record = 9 件 ambiguity literal「OK」**
