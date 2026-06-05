# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.D **decomposition design-lock complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: Phase 1.D (= PC-N-5 Phase 1.D 着手起点 marker 後の本論 phase = 1 GLTF asset 完全 Vulkan draw 通電) を **PC-N-6..PC-N-10 5 sub-step に分解** する design-lock phase 完了 marker = ambiguity (D-1)..(D-12) 12 件 全 AYA literal「OK」record (2026-06-05) + 各 sub-step 概略 + 依存関係 + 着手順序 + 全体 Exit Criteria 明文化。`indra/` 改変 0 件 (= `feedback_design_phase_no_code_write` 整合)。

> **本 doc 位置付け**: Phase 1.D **全体分解** の overview。各 PC-N-? sub-step (= PC-N-6..PC-N-10) の詳細実装 design-lock は当該 sub-step 着手時に **別 session で個別起案** (= `feedback_ubo_migration_one_at_a_time` 厳格遵守、5 sub-step を一括設計しない)。本 doc は分解粒度 + 依存関係 + 着手順序の固定のみ。PC-N decomposition design-lock (= `handoff-...-phase1-c-pc-n-decomposition-design-lock.md`) と同形 pattern 踏襲。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「r41 Phase 1.D 内後続 sub-step (= PC-N-6 仮定以降) design-lock 着手お願いします」literal 受領 (2026-06-05、PC-N-5 complete commit `675529a891` 後の継続 session = 別 session の fresh context) + 必読 1 件 (PC-N-5 complete handoff doc) Read + pinpoint reference 6 件 (PC-N decomposition + design 06b §2.5 + design 06c §2.5 + cross-platform spec + recordGltfAssetDraw + GLTFSceneManager::render) Read → Explore agent 経由 10 項現状調査 (= A PC-N-5 design-lock deferred scope + B design 09 Phase 1.D 定義 + C GLTFSceneManager::render 現状 + D LL::GLTF::Asset vertex buffer OpenGL path + E Asset Vulkan path + F recordGltfAssetDraw 現状 + G recordAvatarPlaceholderDraw 対称性 + H mUseUBO gate 現状 + I AYAGltfStubDrawEnabled 状態 + J design 06b/06c §2.5 hook site 現状) → ambiguity (D-1)..(D-12) 12 件 batch 提示 → AYA literal「OK」一括確認受領 (2026-06-05) で本 design-lock doc 起案。

**Phase 1.D 分解 literal scope** (= 5 sub-step、PC-N-6..PC-N-10):

1. **PC-N-6** = 実 LL::GLTF::Asset 経由 vertex buffer upload (= 最小 1 mesh stub、`recordGltfAssetDraw` 内 `vkCmdBindVertexBuffers` 配線、`vkCmdDraw(N)` 実 vertex count) ((D-7) A)
2. **PC-N-7** = 実 LL::GLTF::Asset 経由 index buffer upload (= 1 index、`vkCmdBindIndexBuffer` + `vkCmdDrawIndexed`) (PC-N-6 後続)
3. **PC-N-8** = material/transform UBO 実 bind 配線 (= `Asset_GLTFNodes` + `Asset_GLTFMaterials` set=3 binding=0/1 経路通電、現状 `flushAssetUbos` hook 経由 register/write infrastructure 完備済を `recordGltfAssetDraw` 内で bind 通電)
4. **PC-N-9** = `GLTFSceneManager::render` 統合 (= 並走 Vulkan dispatcher 経路新設、call site 温存原則遵守、`AYAGltfRealDrawEnabled` cvar gate 配線、live A/B 経路) ((D-5) C + (D-12) A)
5. **PC-N-10** = cleanup + `AYAGltfStubDrawEnabled` deprecate (= PC-N-5 stub draw 経路撤去、`sGltfStubSkin` sentinel deprecate、実 LL::GLTF::Asset 経由 draw に統合完了)

**Phase 境界**: PC-N-6..PC-N-10 完了 = **Phase 1.D complete** marker = 1 GLTF asset 完全 Vulkan draw 通電 ((D-2) A + (D-11) A)。**Phase 1.E** = multi-asset / multi-skin / per-Asset / per-Skin worker thread (= memory `project_ayastorm_r41_design_principles` (2) Core プロセス分散実現) は本 Phase 1.D scope 外、Phase 1.E 以降で別途分解。

---

## §1. 必読 1 件 + pinpoint reference

**次 session 必読 (= PC-N-6 design-lock phase 着手前)**:

1. **本 Phase 1.D decomposition design-lock doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-d-decomposition-design-lock.md`

**pinpoint reference (各 PC-N-? design-lock phase 着手時に必要分のみ)**:

- **PC-N-5 complete doc**: `handoff-...-phase1-d-pc-n-5-complete.md` = `recordGltfAssetDraw` 5 site (llvkloader.cpp:582-601 / 5479-5591 / 3770-3823 / 4134-4140 / 5714-5734) + `AYAGltfStubDrawEnabled` cvar + 第 2 sentinel `sGltfStubSkin` baseline
- **PC-N decomposition design-lock doc**: `handoff-...-phase1-c-pc-n-decomposition-design-lock.md` = 本 doc の pattern source (= 5 sub-step 分解 + ambiguity 9 件 + 採用根拠 + 概略 + 依存関係)
- **design 06b §2.4 (per-asset cadence)**: `design/06b-cadence-update-site-and-dirty.md:99-106` = `GLTFSceneManager::render(variant)` (`gltfscenemanager.cpp:693/696`) 直前で `flushAssetUbos(asset)` 配線済 pattern (= PC-N-8 + PC-N-9 source)
- **design 06b §2.5 (per-skin cadence)**: `design/06b-cadence-update-site-and-dirty.md:108-118` = `GLTFSceneManager::render(variant)` (`gltfscenemanager.cpp:736`) 直前で `flushSkinUbos(skin)` 配線済 pattern (= PC-N-9 source)
- **design 06c §2.5 (set=3 配置)**: `design/06c-descriptor-set-bind-wiring.md:121-132` = set=3 binding=0/1/2 = `Asset_GLTFNodes` / `Asset_GLTFMaterials` / `Skin_GLTFJoints` 配置 (= PC-N-6/7/8 binding source)
- **`recordGltfAssetDraw` (PC-N-5)**: `indra/llrender/llvkloader.cpp:5479-5591` = vertex buffer bind 未実装 (= `vkCmdDraw(3,1,0,0)` 3-vertex generated in shader)、PC-N-6 で `vkCmdBindVertexBuffers` 追加 + PC-N-7 で `vkCmdBindIndexBuffer` + `vkCmdDrawIndexed` 置換
- **`GLTFSceneManager::render` 現状**: `indra/newview/gltfscenemanager.cpp:640-786` = OpenGL path unconditional、`mUseUBO` 参照 0 件、PC-N-9 で並走 Vulkan dispatcher 経路新設 + cvar gate
- **`LL::GLTF::Asset` register/write infrastructure**: `indra/newview/gltf/asset.cpp:213-291` = `registerAssetUbo` (line 218) + `writeAssetUbo` (line 229) dual-write 既実装 (PC-7γ-3)、PC-N-8 で vertex buffer 同形拡張
- **`LL::GLTF::Skin` register/write infrastructure**: `indra/newview/gltf/animation.cpp:401-471` = `registerSkinUbo` + `writeSkinUbo` 既実装 (PC-7γ-3)、PC-N-9 で `flushSkinUbos` 経路通電
- **cross-platform spec §6**: `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` = PC-N-6..PC-N-10 各 sub-step 着手時に OS 依存懸念記録欄追記要
- **GATE-B literal**: memory `project_r41_phase1b_vulkan_host_gate` (= 全 sub-step `#ifdef LL_VULKAN_GLSL` 新規追加 0 件維持)
- **設計原則**: memory `project_ayastorm_r41_design_principles` (1) Upstream OpenGL 取り込みやすさ維持 = call site 温存 = PC-N-9 並走 Vulkan dispatcher 採用根拠 + (2) Core プロセス分散実現 = Phase 1.E 以降 worker thread 分離

---

## §2. 現状調査結果 (= Explore agent 10 項要約)

### §2.1 現 code 状態 (indra/)

| # | 項目 | file:line | 現状要約 |
|---|------|-----------|---------|
| 1 | `recordGltfAssetDraw` | `llvkloader.cpp:5479-5591` | PC-N-5 完了、3-vertex generated in shader (`vkCmdDraw(3,1,0,0)`)、vertex/index buffer bind 未実装、identity matrix Skin_GLTFJoints UBO + zero PerDrawUBO_LightParams 経路通電済 |
| 2 | `sGltfStubSkin` sentinel | `llvkloader.cpp:582-601` | PC-N-5 第 2 address-only sentinel、`sPlaceholderSkin` (PC-N-3) と並走、`UboSkinKey<Skin*, block_hash>` で discrimination |
| 3 | `AYAGltfStubDrawEnabled` cvar | `settings.xml:10405-10420` + `llvkloader.cpp:5714-5734` | Boolean default false Persist=1、`recordAvatarPlaceholderDraw` 末尾 hook 経由 `recordGltfAssetDraw` 並走発火 (PC-N-5 (e)) |
| 4 | `GLTFSceneManager::render` | `gltfscenemanager.cpp:640-786` | OpenGL path unconditional、`mUseUBO` 参照 0 件、`flushAssetUbos(&asset)` (line 702) + `flushSkinUbos(&skin)` (line 759) hook 配線済 (PC-6δ) |
| 5 | `LL::GLTF::Asset` UBO infrastructure | `gltf/asset.cpp:213-291` | `glGenBuffers` (line 213) + `registerAssetUbo` (line 218) + `glBufferData` (line 223) + `writeAssetUbo` (line 229) dual-write 既実装 (PC-7γ-3)、vertex buffer 同形拡張 site available |
| 6 | `LL::GLTF::Skin` UBO infrastructure | `gltf/animation.cpp:401-471` | `Skin::uploadMatrixPalette()` 内 `registerSkinUbo` + `writeSkinUbo` 既実装 (PC-7γ-3)、per-skin cadence flush 駆動側 ready |
| 7 | `mUseUBO` gate | `llglslshader.cpp` 内 host inline bool member | `mUseUBO=true` default (Vulkan build)、`GLTFSceneManager::render` 内参照 0 件 = (D-5) C 採用根拠 (= 並走 Vulkan dispatcher 経路新設) |
| 8 | `recordAvatarPlaceholderDraw` 対称 | `llvkloader.cpp:5595-5735` | PC-N-3 sPlaceholderSkin 経路 + PC-N-5 hook で並走発火、sentinel 並走 pattern baseline |

### §2.2 design doc 章

| # | 章 | 出典 | 該当 PC-N-? |
|---|----|------|------------|
| 9 | per-asset + per-skin cadence flush site | `design/06b §2.4 + §2.5` (line 99-118) | PC-N-8 + PC-N-9 |
| 10 | set=3 binding=0/1/2 配置 (Asset_GLTFNodes / Asset_GLTFMaterials / Skin_GLTFJoints) | `design/06c §2.5` (line 121-132) | PC-N-6/7/8 binding source |

### §2.3 Phase 1.D 5 要素の依存関係

```
PC-N-6 (vertex buffer upload + bind)
    ↓ (vertex buffer 単独では incomplete draw、index 追加で正規 draw)
PC-N-7 (index buffer upload + vkCmdDrawIndexed)
    ↓ (geometry 配線後、material/transform UBO bind で見た目構築)
PC-N-8 (material/transform UBO 実 bind 配線)
    ↓ (recordGltfAssetDraw 完全配線後、GLTFSceneManager::render 経路接続)
PC-N-9 (GLTFSceneManager::render 統合 = 並走 Vulkan dispatcher + cvar gate)
    ↓ (実 GLTF draw 通電完了後、stub draw 経路撤去)
PC-N-10 (cleanup + AYAGltfStubDrawEnabled deprecate)
    = Phase 1.D complete
```

---

## §3. ambiguity (D-1)..(D-12) 12 件 AYA literal「OK」record (2026-06-05) + 採用根拠

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (D-1) | Phase 1.D decomposition pattern | **A**: Phase 1.D decomposition design-lock 先行 (= PC-N decomposition 同形 overview) | OK (2026-06-05) | PC-N decomposition pattern 同形、`feedback_ubo_migration_one_at_a_time` 整合、高レベル構造先行明文化で個別 sub-step 着手手順 fixed、AYA literal「OK」record pattern 踏襲 |
| (D-2) | Phase 1.D 全体 scope 範囲 | **A**: 1 GLTF asset 完全 Vulkan draw 通電 (vertex + index + material + transform + render 統合) | OK (2026-06-05) | PC-N-5 = 1 GLTF asset draw stub literal 整合、multi-asset / worker thread は Phase 1.E 以降 ((D-11) A 整合) |
| (D-3) | Phase 1.D 分解粒度 | **B**: 中粒度 5 sub-step (PC-N-6 vertex + PC-N-7 index + PC-N-8 material/transform + PC-N-9 render 統合 + PC-N-10 cleanup) | OK (2026-06-05) | PC-N decomposition 5 sub-step 整合、各 step 単独 testable + build verify 独立、`feedback_ubo_migration_one_at_a_time` 厳格遵守 |
| (D-4) | Phase 1.D 着手順序 | **A**: 上流 → 下流 (vertex → index → material/transform → render 統合 → cleanup) | OK (2026-06-05) | 下流が上流に依存、PC-N decomposition (N-2) A pattern 踏襲、各 step real write 通電後に統合 |
| (D-5) | `GLTFSceneManager::render` 内 `mUseUBO` gate 配置 | **C**: 既存 OpenGL path 温存 + 別 Vulkan dispatcher (並走経路、live A/B、`AYAGltfRealDrawEnabled` cvar gate) | OK (2026-06-05) | memory `project_ayastorm_r41_design_principles` (1) Upstream OpenGL 取り込みやすさ維持 = call site 温存 + 別 Vulkan dispatcher、`feedback_visual_decisions_need_live_ab` 整合 + PC-N-5 hook pattern 踏襲 |
| (D-6) | vertex/index buffer 構築方法 | **B**: `LL::GLTF::Asset` 内 Vulkan buffer 配置 (`mNodesUBO` 同形、asset.cpp 内 register + write) | OK (2026-06-05) | design 06b §2.5 per-asset cadence pattern 整合、asset.cpp 内 dual-write 既存パターン踏襲、ownership 明確、layer 違反なし |
| (D-7) | 第 1 sub-step (PC-N-6) scope | **A**: 実 LL::GLTF::Asset 1 個経由 vertex buffer upload のみ (最小 1 mesh stub、`recordGltfAssetDraw` 内 `vkCmdBindVertexBuffers` 配線、`vkCmdDraw(N)` 実 vertex count) | OK (2026-06-05) | `feedback_ubo_migration_one_at_a_time` 厳格遵守、最小 1 sub-step = vertex buffer のみ、index は次 sub-step、`recordGltfAssetDraw` 既配線済 stub から段階拡張 |
| (D-8) | PC-N-6 後続 5 sub-step 概略 | **A**: PC-N-6 (vertex) → PC-N-7 (index) → PC-N-8 (material/transform UBO bind) → PC-N-9 (GLTFSceneManager::render 統合 + cvar) → PC-N-10 (cleanup + AYAGltfStubDrawEnabled deprecate) | OK (2026-06-05) | (D-4) A 上流 → 下流順序整合、infrastructure 段階通電、各 step 単独 build verify 独立 |
| (D-9) | build verify scope | **A**: llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131 (PC-N-5 同形 = Linux primary marker 採用後標準) | OK (2026-06-05) | PC-8 Linux primary marker 採用後の Phase 1.D 内 sub-step 標準 scope、cold launch literal は AYA 環境依存、各 sub-step 個別 design-lock phase で検討 |
| (D-10) | Phase 1.D decomposition Exit Criteria 項目数 | **A**: 9 項 (PC-N decomposition 同形 template) | OK (2026-06-05) | PC-N decomposition pattern 踏襲、consistency |
| (D-11) | Phase 1.D vs Phase 1.E 境界 | **A**: Phase 1.D = 1 GLTF asset 完全 Vulkan draw 通電、Phase 1.E = multi-asset / multi-skin / worker thread | OK (2026-06-05) | memory `project_ayastorm_r41_design_principles` (1) + (2) を Phase で分離、(1) Upstream OpenGL 取り込みやすさ = Phase 1.D 実 draw 通電、(2) Core プロセス分散 = Phase 1.E worker thread |
| (D-12) | `AYAGltfStubDrawEnabled` cvar との関係 | **A**: PC-N-5 cvar 維持 (sGltfStubSkin 経由 stub draw 用)、PC-N-6 以降は新 cvar `AYAGltfRealDrawEnabled` で実 LL::GLTF::Asset 経由 draw 切替、PC-N-10 で `AYAGltfStubDrawEnabled` deprecate | OK (2026-06-05) | stub vs real 明確分離、sentinel address-only pattern (sGltfStubSkin) vs 実 LL::GLTF::Skin 経路の区別、live A/B 経路独立、PC-N-10 で PC-N-5 stub draw 経路撤去 |

---

## §4. PC-N-6..PC-N-10 各 sub-step 概略

> **注**: 各 sub-step の **詳細 step (a)-(g)** + **ambiguity 確認** + **Exit Criteria** は当該 sub-step 着手時に **別 session で個別 design-lock 起案** (= `feedback_ubo_migration_one_at_a_time` + `feedback_design_phase_no_code_write` 厳格遵守)。本 §4 は overview のみ。

### §4.1 PC-N-6 = 実 LL::GLTF::Asset 経由 vertex buffer upload

- **scope**: `LL::GLTF::Asset` 内 vertex buffer Vulkan VMA buffer 配置 ((D-6) B) + register/write infrastructure 拡張 (= `mNodesUBO` 同形 dual-write pattern 踏襲、`gltf/asset.cpp:213-229` 同 site で vertex buffer 同形配置) + `recordGltfAssetDraw` 内 `vkCmdBindVertexBuffers` 配線 + `vkCmdDraw(3,1,0,0)` → `vkCmdDraw(N,1,0,0)` 実 vertex count 置換 ((D-7) A)
- **想定改変 file**: `indra/newview/gltf/asset.cpp` + `gltf/asset.h` + `indra/llrender/llvkloader.cpp` (= `recordGltfAssetDraw` 内 vertex buffer bind 追加)
- **想定 design-lock ambiguity** (= 着手時に確認): vertex buffer VMA usage flag (= `VMA_MEMORY_USAGE_GPU_ONLY` or `_AUTO`)、staging buffer 経路 (= 直接 mapped or staging copy)、vertex format layout (= position + normal + uv の 1 buffer or 複数 buffer)、register 配置 site (= Asset コンストラクタ or `uploadTransforms` 内)
- **依存**: PC-N-5 完了 ✅ (= `recordGltfAssetDraw` 配線済)
- **後続**: PC-N-7 (index buffer + `vkCmdDrawIndexed` 置換) が PC-N-6 vertex buffer に依存

### §4.2 PC-N-7 = 実 LL::GLTF::Asset 経由 index buffer upload + vkCmdDrawIndexed

- **scope**: `LL::GLTF::Asset` 内 index buffer Vulkan VMA buffer 配置 + `recordGltfAssetDraw` 内 `vkCmdBindIndexBuffer` 配線 + `vkCmdDraw(N,1,0,0)` → `vkCmdDrawIndexed(M,1,0,0,0)` 置換
- **想定改変 file**: `gltf/asset.cpp` + `gltf/asset.h` + `llvkloader.cpp` (= `recordGltfAssetDraw` 内 index buffer bind + vkCmdDrawIndexed 置換)
- **想定 design-lock ambiguity**: index format (= `VK_INDEX_TYPE_UINT16` or `_UINT32`)、index buffer VMA usage flag、register 配置 site
- **依存**: PC-N-6 (vertex buffer) → PC-N-7
- **後続**: PC-N-8 (material/transform UBO bind) は PC-N-7 geometry 配線後

### §4.3 PC-N-8 = material/transform UBO 実 bind 配線

- **scope**: `recordGltfAssetDraw` 内 set=3 binding=0/1 (= `Asset_GLTFNodes` + `Asset_GLTFMaterials`) 実 bind 通電 (= 現状 register/write infrastructure 完備済 (`gltf/asset.cpp:213-291` PC-7γ-3) を `recordGltfAssetDraw` 内で bind 通電) + `flushAssetUbos(&asset)` 経路通電 + `recordGltfAssetDraw` signature 拡張 (= 実 `LL::GLTF::Asset*` 引数化)
- **想定改変 file**: `llvkloader.cpp` (= `recordGltfAssetDraw` signature 拡張 + set=3 binding=0/1 bind 配線) + `llvkloader.h` (= signature 変更)
- **想定 design-lock ambiguity**: `recordGltfAssetDraw` signature (= `VkCommandBuffer + LL::GLTF::Asset*` or `VkCommandBuffer + asset_hash`)、Asset 引数 source (= GLTFSceneManager 内 1 asset literal 指定 or 既存 GLTF inventory 経由)、識別 mat (= identity → 実 transform matrix 段階移行)
- **依存**: PC-N-6 + PC-N-7 (geometry 配線完了) → PC-N-8
- **後続**: PC-N-9 (`GLTFSceneManager::render` 統合) は PC-N-8 完全配線後

### §4.4 PC-N-9 = `GLTFSceneManager::render` 統合 + `AYAGltfRealDrawEnabled` cvar gate

- **scope**: `GLTFSceneManager::render(asset, variant)` 内に **並走 Vulkan dispatcher 経路新設** (= 既存 OpenGL path 温存 + `AYAGltfRealDrawEnabled` cvar gate で並走経路発火、(D-5) C 採用) + `recordGltfAssetDraw(&asset)` 呼出 hook + `AYAGltfRealDrawEnabled` cvar 新設 (Boolean default false Persist=1)
- **想定改変 file**: `gltfscenemanager.cpp` (= `render(variant)` 内 cvar gate + `recordGltfAssetDraw` hook 追加) + `settings.xml` (= `AYAGltfRealDrawEnabled` cvar 1 件追加)
- **想定 design-lock ambiguity**: hook 配置 site (= `render` 関数末尾 or `flushAssetUbos` 直後 or `flushSkinUbos` 直後)、cvar gate 粒度 (= per-asset or per-render-call)、cvar name (= `AYAGltfRealDrawEnabled` 確定 (D-12) A) 確認
- **依存**: PC-N-6 + PC-N-7 + PC-N-8 (= `recordGltfAssetDraw` 完全配線完了) → PC-N-9
- **後続**: PC-N-10 (cleanup) は PC-N-9 実 draw 通電完了後

### §4.5 PC-N-10 = cleanup + `AYAGltfStubDrawEnabled` deprecate

- **scope**: PC-N-5 stub draw 経路撤去 (= `recordAvatarPlaceholderDraw` 末尾 hook 撤去 + `AYAGltfStubDrawEnabled` cvar deprecate + `sGltfStubSkin` sentinel deprecate) + 実 `recordGltfAssetDraw` 経路に統合完了 + Phase 1.D complete marker 起案
- **想定改変 file**: `llvkloader.cpp` (= PC-N-5 hook 撤去 + sentinel deprecate) + `settings.xml` (= `AYAGltfStubDrawEnabled` cvar deprecate or 削除)
- **想定 design-lock ambiguity**: cvar 撤去 vs deprecate (= cvar 行削除 or comment で deprecated mark)、sentinel storage 撤去順序 (= unregisterSkinUbo timing)、Phase 1.D complete marker doc 起案順序
- **依存**: PC-N-6 + PC-N-7 + PC-N-8 + PC-N-9 (= 実 GLTF draw 通電完了) → PC-N-10
- **後続**: **Phase 1.D complete** marker = Phase 1.E (= multi-asset / multi-skin / worker thread) 着手起点

### §4.6 GATE-B 整合 (全 sub-step 共通)

`#ifdef LL_VULKAN_GLSL` 新規追加 0 件 = memory `project_r41_phase1b_vulkan_host_gate` 遵守。host 側 redirect 層は `mUseUBO` runtime flag のみで gate (= GATE-B 確定 2026-06-04)。PC-N-9 cvar gate (`AYAGltfRealDrawEnabled`) は `LLCachedControl<bool>` runtime cvar 経由ゆえ `#ifdef` 非依存 = GATE-B 違反なし。

### §4.7 MUSEUBO-A 整合 (全 sub-step 共通)

`mUseUBO=false` default で既存 OpenGL 描画 100% 維持。本 Phase 1.D 5 sub-step は全て:

- PC-N-6/7/8 = `recordGltfAssetDraw` 内 vertex/index/UBO bind 配線拡張 (= `AYAGltfStubDrawEnabled` cvar gate 経由ゆえ default OFF で発火なし、PC-N-5 完了状態と機能等価)
- PC-N-9 = `GLTFSceneManager::render` 並走 Vulkan dispatcher 経路新設 (= `AYAGltfRealDrawEnabled` cvar gate 経由ゆえ default OFF で発火なし、OpenGL path unconditional 維持)
- PC-N-10 = cleanup (= cvar deprecate + sentinel deprecate ゆえ default OFF 経路再不変)

の範囲で完結 = 実 OpenGL 描画影響ゼロ + `AYAGltfRealDrawEnabled=true` 時のみ並走 Vulkan dispatcher 発火 (= live A/B 経路)。

### §4.8 設計原則整合 (memory `project_ayastorm_r41_design_principles`)

- **(1) Upstream OpenGL 取り込みやすさ維持** = (D-5) C 採用 = `GLTFSceneManager::render` 並走 Vulkan dispatcher 新設 = OpenGL path 温存 = upstream LL からの取り込み時に call site 不変
- **(2) Core プロセス分散実現** = Phase 1.E 以降に分離 ((D-11) A) = Phase 1.D 単独 GLTF asset draw 完成後、Phase 1.E で per-Asset / per-Skin worker thread 設計

---

## §5. Phase 1.D decomposition Exit Criteria 9 項 ((D-10) A 採用)

| # | Criteria |
|---|----------|
| (i) | Phase 1.D 分解 5 sub-step (PC-N-6..PC-N-10) literal scope §0 明文化 |
| (ii) | 必読 1 件 (本 doc) + pinpoint reference 12 件 §1 列挙 |
| (iii) | 現状調査 10 項 §2 網羅 (= code 8 項 + design doc 2 項) |
| (iv) | ambiguity (D-1)..(D-12) 12 件 AYA literal「OK」record (2026-06-05) §3 |
| (v) | 採用根拠 12 件 §3 明文化 |
| (vi) | PC-N-6..PC-N-10 5 sub-step 概略 + 依存関係 §4 明文化 |
| (vii) | Phase 境界明文化 = PC-N-6..PC-N-10 完了 = Phase 1.D complete、Phase 1.E = multi-asset / worker thread §0 + §4.5 |
| (viii) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 §4.6 + MUSEUBO-A 整合 §4.7 |
| (ix) | `indra/` 改変 0 件 = `feedback_design_phase_no_code_write` 整合 |

---

## §6. 着手手順 (= 次 session で PC-N-6 design-lock 着手)

1. AYA 指示「PC-N-6 design-lock 着手お願いします」literal 受領待ち
2. 本 Phase 1.D decomposition design-lock doc 全文 Read (= 必読 1 件)
3. PC-N-5 complete doc + design 06b §2.4 + design 06c §2.5 + `gltf/asset.cpp:213-291` + `recordGltfAssetDraw` (llvkloader.cpp:5479-5591) pinpoint Read (= PC-N-6 source)
4. Explore agent で PC-N-6 詳細現状調査 = `LL::GLTF::Asset` vertex buffer OpenGL path 詳細 + `mNodesUBO` 配置 pattern + VMA usage flag + vertex format layout + register 配置 site
5. PC-N-6 ambiguity (= 想定 §4.1) を列挙 → 推奨案併記 → AYA literal 確認
6. PC-N-6 design-lock doc 起案 (= step (a)-(g) + Exit Criteria) → `indra/` 改変 0 件 → AYA commit 指示後 commit
7. 別 session で PC-N-6 実装着手 → complete handoff doc 起案 → AYA commit 指示後 commit
8. 以後 PC-N-7 → PC-N-8 → PC-N-9 → PC-N-10 を同パターンで進行

---

## §7. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1 design-lock ✅ + PC-N-1 ✅ + PC-N-2 design-lock ✅ + PC-N-2 ✅ + PC-N-4 design-lock ✅ + PC-N-4 ✅ + PC-N-3 design-lock ✅ + PC-N-3 ✅ = Phase 1.C complete ✅ + PC-8 Linux primary marker ✅ = Phase 1.C strict 線形終了 ✅ + PC-N-5 design-lock ✅ + PC-N-5 ✅ = Phase 1.D 着手起点 実装完了 ✅ + **Phase 1.D decomposition design-lock ✅ 本 commit** + PC-N-6 design-lock ⏳ 次 session + PC-N-6 ⏳ + PC-N-7 ⏳ + PC-N-8 ⏳ + PC-N-9 ⏳ + PC-N-10 ⏳ = Phase 1.D complete ⏳ + Phase 1.E (multi-asset / multi-skin / worker thread) ⏳

---

## §8. self-verify 9 観点 全 ✅

1. **Phase 1.D 分解 literal scope 5 sub-step §0 完全分解** = PC-N-6 vertex + PC-N-7 index + PC-N-8 material/transform + PC-N-9 render 統合 + PC-N-10 cleanup ✅
2. **必読 1 件 §1 + pinpoint reference 12 件別記** = PC-N-5 complete + PC-N decomposition + design 06b §2.4 + design 06b §2.5 + design 06c §2.5 + recordGltfAssetDraw + GLTFSceneManager::render + Asset register/write + Skin register/write + cross-platform spec §6 + GATE-B + 設計原則 ✅
3. **現状調査 §2 10 項網羅** = code 8 項 (recordGltfAssetDraw + sGltfStubSkin + AYAGltfStubDrawEnabled + GLTFSceneManager::render + Asset infra + Skin infra + mUseUBO + recordAvatarPlaceholderDraw) + design doc 2 項 + 依存関係図 ✅
4. **ambiguity (D-1)..(D-12) 12 件 AYA literal「OK」record (2026-06-05) §3** ✅
5. **採用根拠 12 件明文化 §3** ✅
6. **PC-N-6..PC-N-10 5 sub-step 概略 §4 + 依存関係 §2.3** ✅
7. **Phase 境界明文化 = (D-11) A 採用 = PC-N-6..PC-N-10 完了 = Phase 1.D complete、Phase 1.E = multi-asset / worker thread** ✅
8. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 §4.6 + MUSEUBO-A 整合 §4.7 + 設計原則整合 §4.8** ✅
9. **`indra/` 改変 0 件 = `feedback_design_phase_no_code_write` 整合 + cross-platform spec §6 Phase 1.D 行追記のみ** ✅

---

## §9. 次 session 着手 1 line

**PC-N-6 design-lock 着手** = 実 `LL::GLTF::Asset` 経由 vertex buffer upload (= 最小 1 mesh stub、`recordGltfAssetDraw` 内 `vkCmdBindVertexBuffers` 配線、`vkCmdDraw(3,1,0,0)` → `vkCmdDraw(N,1,0,0)` 実 vertex count 置換) の詳細 step 分解 + ambiguity 確認 + Exit Criteria 明文化。`indra/` 改変 0 件、別 session で実装 phase 着手。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 Phase 1.D decomposition design-lock handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 次 session 必読 1 件 + pinpoint reference 12 件別記、本 session も Read pinpoint のみ (= PC-N-5 complete doc + PC-N decomposition doc + design 06b §2.4/§2.5 + design 06c §2.5 + cross-platform spec + Explore agent 経由 10 項 pinpoint 報告)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §8
- **feedback_build_only_verified** 遵守 = design-lock phase は `indra/` 改変 0 件で build verify 対象外、各 sub-step 実装 phase で literal 検証取得予定 ((D-9) A 採用)
- **feedback_no_scope_shrink** 遵守 = Phase 1.D 分解 5 sub-step §0 完全分解、各 sub-step は段階分離 = 縮小ではない、(D-2) A + (D-11) A 採用は AYA literal「OK」record 済段階分離 (= Phase 1.D = 1 GLTF asset 完全 Vulkan draw 通電、multi-asset / worker thread は Phase 1.E 以降の literal 境界確定)
- **feedback_doubt_self_first** 遵守 = ambiguity 12 件発見で停止 + 推奨案提示 + AYA literal「OK」確認後本 design-lock doc 起案、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = 12 件 batch AYA 確認 (2026-06-05)、各候補 + 推奨案 + 根拠明示後 AYA literal「OK」record 受領で確定、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = Phase 1.D を 5 sub-step に分解、各 sub-step は別 session で個別 design-lock + 実装、本 doc は overview のみ
- **feedback_design_phase_no_code_write** 整合 = 本 Phase 1.D decomposition design-lock は doc 起案のみ、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領まで commit せず
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
- **feedback_no_bare_reference_ids** 遵守 = (D-1)..(D-12) 各 ID に項目名 / 採用案内容併記 §3 + (PC-N-6)..(PC-N-10) 各 ID に scope 内容併記 §0 + §4
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、git add 個別 file 指定予定
- **memory `project_ayastorm_r41_design_principles`** 整合 = (1) Upstream OpenGL 取り込みやすさ維持 = (D-5) C 採用 = `GLTFSceneManager::render` 並走 Vulkan dispatcher 新設 = call site 温存 §4.8 + (2) Core プロセス分散実現 = Phase 1.E 以降に分離 ((D-11) A) §4.8
- **memory `project_r41_phase1b_vulkan_host_gate`** 整合 = GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar runtime gate のみ §4.6
- **memory `project_ayastorm_three_platforms`** 整合 = cross-platform spec §6 Phase 1.D 行追記で macOS / Windows 派生 fix 候補欄起案、Linux primary 完成 → 他者補完 model と整合

---
