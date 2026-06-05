# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.E **decomposition design-lock complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: Phase 1.E (= Phase 1.D complete marker 後の本論 phase = multi-asset / multi-skin / worker thread = 設計原則 (2) Core プロセス分散実現) を **PC-N-11..PC-N-15 5 sub-step に分解** する design-lock phase 完了 marker = ambiguity (E-1)..(E-16) 16 件 全 AYA literal「全件推奨で OK」record (2026-06-05) + 各 sub-step 概略 + 依存関係 + 着手順序 + 全体 Exit Criteria 明文化。`indra/` 改変 0 件 (= `feedback_design_phase_no_code_write` 整合)。

> **本 doc 位置付け**: Phase 1.E **全体分解** の overview。各 PC-N-? sub-step (= PC-N-11..PC-N-15) の詳細実装 design-lock は当該 sub-step 着手時に **別 session で個別起案** (= `feedback_ubo_migration_one_at_a_time` 厳格遵守、5 sub-step を一括設計しない)。本 doc は分解粒度 + 依存関係 + 着手順序の固定のみ。Phase 1.D decomposition design-lock (= `handoff-...-phase1-d-decomposition-design-lock.md`) と同形 pattern 踏襲。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「r41 Phase 1.E design-lock 着手お願いします」literal 受領 (2026-06-05、PC-N-10 complete commit `cccd411486` = Phase 1.D complete marker 後の継続 session = 別 session の fresh context) + 必読 1 件 (PC-N-10 complete handoff doc) Read + pinpoint reference (= recordGltfAssetDraw PC-N-8 (f) real Asset path block + GLTFSceneManager::render setCurrentAsset/Skin/Primitive 配線 + Phase 1.D decomposition design-lock template + cross-platform spec §6 table format) Read → ambiguity (E-1)..(E-16) 16 件 + 推奨案 + 採用根拠提示 → AYA literal「全件推奨で OK」一括確認受領 (2026-06-05) で本 design-lock doc 起案。

**Phase 1.E 分解 literal scope** (= 5 sub-step、PC-N-11..PC-N-15):

1. **PC-N-11** = multi-skin (= `sGltfStubSkin` sentinel 段階的卒業 = `sCurrentSkin` guard 追加 + fall-through to `sGltfStubSkin`、storage 撤去は PC-N-15 cleanup phase へ持越し) ((E-6) B)
2. **PC-N-12** = real node modelview push constant 配線 (= identity → real Node modelview via `pdata.mNodeIndex` + `asset.mNodes[]` 経由 modelview 解決) ((E-7) A)
3. **PC-N-13** = real per-draw light params + multi-asset verification (= zero-buffer PerDrawUBO_LightParams → real per-draw data + 既存 SL サンプル動作確認) ((E-8) B + (E-13) A)
4. **PC-N-14** = worker thread design-lock (= per-Primitive UBO write + cmdbuf recording 並列化設計、design 09 既存 doc 参照 + 必要時新設 doc) ((E-9) B + (E-14) B)
5. **PC-N-15** = worker thread 実装 + cleanup + Phase 1.E complete marker 起案 (= `sGltfStubSkin` storage 撤去 + 残余 placeholder 撤去 + Phase 1.E complete marker 統合明示) ((E-10) B)

**Phase 境界**: PC-N-11..PC-N-15 完了 = **Phase 1.E complete** marker = multi-asset / multi-skin / worker thread 完成 = 設計原則 (2) Core プロセス分散実現達成 ((E-3) A)。**Phase 1.F** (= 必要性発生時に分離、e.g., Vulkan command pool per-thread 等 large redesign、`sGltfStubAssetPipeline` rename ((E-11) B 維持判断後の cosmetic phase) は本 Phase 1.E scope 外、Phase 1.F 以降で別途分解。

---

## §1. 必読 1 件 + pinpoint reference

**次 session 必読 (= PC-N-11 design-lock phase 着手前)**:

1. **本 Phase 1.E decomposition design-lock doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-e-decomposition-design-lock.md`

**pinpoint reference (各 PC-N-? design-lock phase 着手時に必要分のみ)**:

- **PC-N-10 complete doc**: `handoff-...-phase1-d-pc-n-10-complete.md` = Phase 1.D complete marker = 1 GLTF asset 完全 Vulkan draw 通電 baseline + 残 placeholder 3 件 (sGltfStubSkin sentinel + identity push constant modelview + zero-buffer PerDrawUBO_LightParams)
- **Phase 1.D decomposition design-lock doc**: `handoff-...-phase1-d-decomposition-design-lock.md` = 本 doc の pattern source (= 5 sub-step 分解 + ambiguity 12 件 + 採用根拠 + 概略 + 依存関係)
- **`recordGltfAssetDraw` PC-N-8 (f) real Asset path**: `indra/llrender/llvkloader.cpp:5906-6010` 付近 = sGltfStubSkin sentinel + identity matrix push constant + zero-buffer PerDrawUBO_LightParams 配置箇所 (PC-N-11/12/13 site)
- **`GLTFSceneManager::render` setCurrentAsset/Skin/Primitive 配線**: `indra/newview/gltfscenemanager.cpp:697` (setCurrentAsset) + `:752` (setCurrentPrimitive) + `:765` (setCurrentSkin if rigged) + `:787` (clearCurrentSkin) + `:792` (clearCurrentPrimitive) + `:798` (clearCurrentAsset) = real Skin owner は既に `&skin` で resolved (PC-N-11 source)
- **design 09 (Phase 1 全体設計)**: `docs/specs/ayastorm-r41-gl-removal/design/` 配下 = worker thread 並列化設計参照 (PC-N-14 source)
- **cross-platform spec §6**: `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` = PC-N-11..PC-N-15 各 sub-step 着手時に OS 依存懸念記録欄追記要
- **GATE-B literal**: memory `project_r41_phase1b_vulkan_host_gate` (= 全 sub-step `#ifdef LL_VULKAN_GLSL` 新規追加 0 件維持、count llvkloader.cpp=6 不変)
- **設計原則**: memory `project_ayastorm_r41_design_principles` (1) Upstream OpenGL 取り込みやすさ維持 = call site 温存 + (2) Core プロセス分散実現 = Phase 1.E worker thread 本丸
- **`AYAGltfRealDrawEnabled` cvar entry hook**: `indra/llrender/llvkloader.cpp` `recordAvatarPlaceholderDraw` 末尾 PC-N-10 (a) tag block = Phase 1.E は本 cvar 1 件で完結 ((E-12) B)、追加 cvar 0 件

---

## §2. 現状調査結果 (Phase 1.D complete baseline 把握)

### §2.1 現 code 状態 (indra/) = Phase 1.D complete (commit `cccd411486`) baseline

| # | 項目 | file:line | 現状要約 |
|---|------|-----------|---------|
| 1 | `recordGltfAssetDraw` PC-N-8 (f) real Asset path | `llvkloader.cpp:5906-6010` 付近 | Phase 1.D で通電完了、per-Primitive `sPrimitiveVertexBuffers`/`sPrimitiveIndexBuffers` 経由 vkCmdDrawIndexed(real_index_count) 実行、5 段 graceful degrade 維持 |
| 2 | `sGltfStubSkin` sentinel + writeSkinUbo | `llvkloader.cpp:598-600` + `:5959-5974` | (N10-3) B で意図的維持、PC-N-8 (f) 内で writeSkinUbo(sGltfStubSkin, identity matrix) + flushSkinUbos(sGltfStubSkin) で rigged binding 通電、**PC-N-11 で real Skin owner (sCurrentSkin) 切替対象** |
| 3 | push constant `real_asset_identity_modelview` | `llvkloader.cpp:5979-5990` | identity matrix 4×4 を `vkCmdPushConstants` VERTEX_BIT で投入、**PC-N-12 で real Node modelview 切替対象** |
| 4 | `writeDrawUbo(PerDrawUBO_LightParams, zero_buf)` | `llvkloader.cpp:5943-5950` | 256 B zero buffer 投入、**PC-N-13 で real per-draw light params 切替対象** |
| 5 | `GLTFSceneManager::render` setCurrentAsset/Skin/Primitive | `gltfscenemanager.cpp:697/752/765/787/792/798` | PC-7γ-2 + PC-N-9 で全配線済、`sCurrentAsset`/`sCurrentSkin`/`sCurrentPrimitive` 全て `recordGltfAssetDraw` 内消費可能 = Phase 1.E source |
| 6 | `sGltfStubAssetPipeline` | `llvkloader.cpp` | (N10-5) B で意図的維持、機能は real Asset 共用 (Vulkan dynamic state)、**Phase 1.E でも維持 ((E-11) B)**、rename は Phase 1.F 候補温存 |
| 7 | `AYAGltfRealDrawEnabled` cvar entry hook | `llvkloader.cpp` `recordAvatarPlaceholderDraw` 末尾 PC-N-10 (a) | Phase 1.D 完了で一本化済、**Phase 1.E は本 cvar 1 件で完結 ((E-12) B)、追加 cvar 0 件** |
| 8 | per-Primitive ownership | `llvkloader.cpp` PC-N-8 (a)/(b)/(c)/(d) | `sPrimitiveVertexBuffers`/`sPrimitiveIndexBuffers` `unordered_map<Primitive*, PrimitiveVulkanBuffer>` + Primitive::uploadVulkanBuffers + Asset::uploadTransforms 末尾 hook + dtor 配線 = **worker thread 分散余地確保済 (= per-Primitive granularity = PC-N-14 source)** |

### §2.2 design doc 章

| # | 章 | 出典 | 該当 PC-N-? |
|---|----|------|------------|
| 9 | Phase 1 全体設計 (worker thread 並列化方針) | `design/` 配下 (= design 09 候補) | PC-N-14 worker thread design-lock source |
| 10 | per-Asset / per-Skin cadence pattern | `design/06b §2.4 + §2.5` | PC-N-11 real Skin owner 切替 + PC-N-12 real Node modelview 切替 reference |

### §2.3 Phase 1.E 5 要素の依存関係

```
PC-N-11 (multi-skin = sGltfStubSkin sentinel 段階的卒業 = sCurrentSkin guard)
    ↓ (real Skin owner 切替後、real Node modelview に進む = "実 data 通電" 順序)
PC-N-12 (real node modelview push constant 配線)
    ↓ (modelview 配線後、per-draw light params に進む)
PC-N-13 (real per-draw light params + multi-asset verification)
    ↓ (PC-N-8 (f) 内 placeholder 全卒業後、worker thread 設計に進む)
PC-N-14 (worker thread design-lock)
    ↓ (設計確定後、worker thread 実装 + 残余 cleanup + Phase 1.E complete marker)
PC-N-15 (worker thread 実装 + cleanup + Phase 1.E complete marker)
    = Phase 1.E complete = 設計原則 (2) Core プロセス分散実現達成
```

---

## §3. ambiguity (E-1)..(E-16) 16 件 AYA literal「全件推奨で OK」record (2026-06-05) + 採用根拠

### §3.1 構造系 (= decomposition pattern + 範囲)

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (E-1) | Phase 1.E decomposition pattern | **A**: Phase 1.D 同形 decomposition design-lock doc 先行 | OK (2026-06-05) | Phase 1.D pattern 踏襲、`feedback_ubo_migration_one_at_a_time` 整合、overview 先行で着手順序 fixed、AYA literal「全件推奨で OK」record pattern 踏襲 |
| (E-2) | Phase 1.E literal scope 範囲 | **B**: AYA 起案 3 scope (multi-asset / multi-skin / worker thread) + real node modelview + real per-draw light params + sentinel-related fix も含める | OK (2026-06-05) | Phase 1.D complete = 1 GLTF asset draw 通電だが PC-N-8 (f) に identity/zero/sentinel placeholder 3 件残存、Phase 1.E = "実 data 通電" を thesis に統合的解決、各 placeholder は multi-skin/multi-asset と同じ "sentinel 卒業" カテゴリ |
| (E-3) | Phase 1.E vs Phase 1.F 境界 | **A**: worker thread = Phase 1.E 内 (AYA 起案直訳) | OK (2026-06-05) | AYA 起案 3 scope 直訳、設計原則 (2) を 1 phase 内で実現、Phase 1.F は worker thread 完了後の post-Phase-1 (Mac/Win 補完 phase 等) に保留 |
| (E-4) | Phase 1.E 分解粒度 | **B**: 中粒度 5 sub-step | OK (2026-06-05) | Phase 1.D 同形 5 sub-step pattern 踏襲、各 step 独立 testable、`feedback_ubo_migration_one_at_a_time` 厳格遵守整合 |
| (E-5) | Phase 1.E 着手順序 (= 上流→下流) | **A**: multi-skin → real node modelview → real per-draw + multi-asset verify → worker thread design → worker thread 実装 + cleanup | OK (2026-06-05) | "実 data 通電" を先行 (PC-N-8 (f) placeholder 卒業) → multi-asset verification → worker thread (= 設計原則 (2)) を最終、各 step 単独 testable + 下流が上流に依存 |

### §3.2 sub-step scope 系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (E-6) | PC-N-11 (1st) = multi-skin (sGltfStubSkin sentinel 卒業) | **B**: 段階的 = (i) sCurrentSkin guard 追加 + fall-through to sGltfStubSkin → (ii) sGltfStubSkin storage 撤去は PC-N-15 cleanup phase で | OK (2026-06-05) | 段階的なら 1 sub-step 内で independent fall-through 経路維持、`feedback_doubt_self_first` 整合、build verify 各段階 PASS 確認可能 |
| (E-7) | PC-N-12 (2nd) = real node modelview push constant | **A**: 単独 sub-step | OK (2026-06-05) | identity → real Node modelview 配線は GLTFSceneManager::render 内 per-Primitive `pdata.mNodeIndex` + `asset.mNodes[]` 経由 modelview 解決必要、独立 sub-step として scope 明確化 |
| (E-8) | PC-N-13 (3rd) = real per-draw light params + multi-asset verification | **B**: 同 sub-step (両方 "実 data 通電" 系) | OK (2026-06-05) | per-draw light params = PerDrawUBO_LightParams zero-buffer → real data、multi-asset verification = 既存 SL サンプル動作確認 (実装改変少ない)、2 つを同 sub-step に統合で sub-step 数最適化 |
| (E-9) | PC-N-14 (4th) = worker thread design-lock | **B**: design 09 参照 + 必要に応じ Phase 1.E worker thread 設計 doc 新設 | OK (2026-06-05) | design 09 = Phase 1 全体設計、Phase 1.E worker thread 設計は per-Primitive granularity 具体化 ゆえ design 09 だけでは不足の可能性、必要時新設 doc で柔軟対応 |
| (E-10) | PC-N-15 (5th) = worker thread 実装 + cleanup + Phase 1.E complete marker | **B**: worker thread 実装 + cleanup + Phase 1.E complete marker 統合 1 sub-step | OK (2026-06-05) | Phase 1.D PC-N-10 同形「最終 sub-step = 実装 + cleanup + complete marker 統合」pattern 踏襲、sub-step 数最適化 |

### §3.3 副次系 (= naming + cvar + verify)

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (E-11) | sGltfStubAssetPipeline naming | **B**: 維持 | OK (2026-06-05) | (N10-5) B で意図的維持確認済、Phase 1.E でも機能は real Asset 共用 (Vulkan 仕様 dynamic state)、rename は cosmetic で副作用なし → 後の phase (Phase 1.F 等) で rename 候補温存 |
| (E-12) | Phase 1.E cvar gate strategy | **B**: 単一 cvar (AYAGltfRealDrawEnabled) で完結 = 追加 cvar 0 件 | OK (2026-06-05) | Phase 1.D で entry hook AYAGltfRealDrawEnabled 一本化済、Phase 1.E 各 sub-step は内部 path 切替のみ (= caller 側 cvar 切替不要)、cvar 数最小化 + settings.xml 改変最小化 |
| (E-13) | multi-asset verification 経路 | **A**: 既存 SL サンプル (複数 GLTF inv item) で AYA 動作確認 | OK (2026-06-05) | per-Primitive ownership は PC-N-8 で確立済 (`sPrimitiveVertexBuffers` map = primitive key) ゆえ multi-asset は概念的に動作可能、verification は AYA 既存 SL inv item で実機確認、synthetic stub 不要 |
| (E-14) | worker thread scope | **B**: UBO write + cmdbuf recording 両方並列化 | OK (2026-06-05) | 設計原則 (2) Core プロセス分散実現の本丸 = UBO write + cmdbuf recording 並列化、両方含めて 1 単位、C は scope 過大 (= Vulkan command pool per-thread 等 large redesign 必要、Phase 1.F に保留) |
| (E-15) | Phase 1.E decomposition Exit Criteria 項目数 | **A**: 9 項 (Phase 1.D decomposition 同形) | OK (2026-06-05) | Phase 1.D decomposition pattern 踏襲、consistency |
| (E-16) | build verify scope (各 sub-step) | **A**: PC-N-6/7/8/9/10 同形 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 + GATE-B integrity (LL_VULKAN_GLSL=6 不変) | OK (2026-06-05) | Phase 1.D 内 sub-step 標準 scope 踏襲、Linux primary marker 採用後標準 |

---

## §4. PC-N-11..PC-N-15 各 sub-step 概略

> **注**: 各 sub-step の **詳細 step (a)-(g)** + **ambiguity 確認** + **Exit Criteria** は当該 sub-step 着手時に **別 session で個別 design-lock 起案** (= `feedback_ubo_migration_one_at_a_time` + `feedback_design_phase_no_code_write` 厳格遵守)。本 §4 は overview のみ。

### §4.1 PC-N-11 = multi-skin (sGltfStubSkin sentinel 段階的卒業) ((E-6) B)

- **scope**: PC-N-8 (f) 内 `writeSkinUbo(sGltfStubSkin, ...)` + `flushSkinUbos(sGltfStubSkin)` 2 site で `sCurrentSkin` 経由 real Skin owner 切替 = (i) `sCurrentSkin` 非 null 時 real Skin owner 経路 + (ii) `sCurrentSkin == nullptr` 時 fall-through to `sGltfStubSkin` (= non-rigged primitive 等の natural guard)。`sGltfStubSkin` storage 自体は PC-N-15 cleanup phase で撤去。
- **想定改変 file**: `indra/llrender/llvkloader.cpp` (= PC-N-8 (f) writeSkinUbo + flushSkinUbos site 2 site 改変)
- **想定 design-lock ambiguity** (= 着手時に確認): `sCurrentSkin` accessor 配置 (= 既存 PC-7γ-2 既配線済確認のみ vs 追加 hook 必要)、real Skin owner 経路 writeSkinUbo 引数 (= `sCurrentSkin` 直接 vs `*sCurrentSkin` dereferenced)、identity matrix → real Skin.mInverseBindMatrices 切替 data source、fall-through 経路 cvar gate 有無
- **依存**: Phase 1.D complete ✅ (= PC-N-9 で `setCurrentSkin`/`clearCurrentSkin` 既配線済、`sGltfStubSkin` sentinel 維持済)
- **後続**: PC-N-12 (real node modelview) が PC-N-11 完了後の "実 data 通電" 第 2 段

### §4.2 PC-N-12 = real node modelview push constant 配線 ((E-7) A)

- **scope**: PC-N-8 (f) 内 `vkCmdPushConstants` VERTEX_BIT 64 B identity matrix → real Node modelview 切替 = `sCurrentPrimitive`/`sCurrentAsset` 経由 `pdata.mNodeIndex` + `asset.mNodes[]` から modelview matrix 解決 → push constant 投入。modelview matrix source 確定 (= Node.mAssetMatrix or computed transform via Asset::uploadTransforms 結果)。
- **想定改変 file**: `indra/llrender/llvkloader.cpp` (= PC-N-8 (f) push constant site 改変)
- **想定 design-lock ambiguity**: modelview source (= `Node::mAssetMatrix` direct vs computed transform)、Node lookup 経路 (= `sCurrentPrimitive` から NodeIndex 解決経路、`Primitive` 構造体内 mNodeIndex field 存否確認要)、identity fallback 経路 (= Node lookup 失敗時 graceful degrade)
- **依存**: PC-N-11 (multi-skin) → PC-N-12 ("実 data 通電" 順序、上流→下流)
- **後続**: PC-N-13 (real per-draw light params + multi-asset verification) は real node modelview 配線完了後

### §4.3 PC-N-13 = real per-draw light params + multi-asset verification ((E-8) B + (E-13) A)

- **scope**: PC-N-8 (f) 内 `writeDrawUbo(PerDrawUBO_LightParams, ..., zero_buf, 256)` zero buffer → real per-draw light params 切替 + multi-asset verification (= 既存 SL サンプル複数 GLTF inv item で実機動作確認、per-Primitive ownership map による multi-asset 概念動作 verify)。
- **想定改変 file**: `indra/llrender/llvkloader.cpp` (= PC-N-8 (f) writeDrawUbo site 改変) + verification は実機確認のみ (= `indra/` 改変 0 件 for verification)
- **想定 design-lock ambiguity**: per-draw light params data source (= shader 側 PerDrawUBO_LightParams layout 確認 + host 側 source field 確定)、multi-asset 動作確認手順 (= 既存 SL サンプル inv item 候補 + 期待結果 + 失敗時 fallback)、multi-asset 失敗時の追加 sub-step (= PC-N-13.5 等の動的追加可能性)
- **依存**: PC-N-12 (real node modelview) → PC-N-13 ("実 data 通電" 第 3 段)
- **後続**: PC-N-14 (worker thread design-lock) は PC-N-8 (f) 内 placeholder 全卒業後

### §4.4 PC-N-14 = worker thread design-lock ((E-9) B + (E-14) B)

- **scope**: per-Primitive granularity の UBO write + cmdbuf recording 並列化設計 = design 09 既存 doc 参照 + 必要時 Phase 1.E worker thread 設計 doc 新設 (= `docs/specs/ayastorm-r41-gl-removal/design/09-worker-thread-distribution.md` 等の新設候補)。Vulkan thread safety (= per-thread command pool / descriptor pool / VMA allocation thread-safety) + 既存 main-thread 経路との並走互換性確認。
- **想定改変 file**: `docs/specs/ayastorm-r41-gl-removal/design/` 配下 新設 doc + 本 cross-platform spec §6 PC-N-14 行更新 + 本 doc handoff complete (= design-lock phase ゆえ `indra/` 改変 0 件)
- **想定 design-lock ambiguity**: worker thread 数 (= fixed N vs dynamic based on CPU count)、command pool 配置 (= per-thread vs shared with mutex)、VMA allocation thread-safety (= 既 VMA は thread-safe vs 別途 sync 必要)、main thread vs worker thread 同期 mechanism (= fence vs semaphore vs CPU barrier)、cvar gate (= AYAGltfWorkerThreadEnabled 新設 vs AYAGltfRealDrawEnabled 単独完結 = (E-12) B 整合)
- **依存**: PC-N-11 + PC-N-12 + PC-N-13 (= "実 data 通電" 完了、PC-N-8 (f) 内 placeholder 全卒業) → PC-N-14
- **後続**: PC-N-15 (実装 + cleanup + Phase 1.E complete marker) は design-lock 確定後

### §4.5 PC-N-15 = worker thread 実装 + cleanup + Phase 1.E complete marker ((E-10) B)

- **scope**: PC-N-14 design-lock 確定後の worker thread 実装 + cleanup 残余 (= `sGltfStubSkin` sentinel storage 撤去 + `recordGltfAssetDraw` 内 fall-through 経路撤去 if any + `sGltfStubAssetPipeline` rename 候補保留 ((E-11) B = Phase 1.F 候補)) + **Phase 1.E complete marker** doc 起案 (= 設計原則 (2) Core プロセス分散実現達成明示) + Phase 1 全完了 marker 候補検討 (= Phase 1.F 設定有無確認)。
- **想定改変 file**: `indra/llrender/llvkloader.cpp` (= worker thread 実装 + sGltfStubSkin storage 撤去) + `indra/newview/gltf/` 配下 if worker thread 関連 hook 必要 + cross-platform spec §6 PC-N-15 行更新 + 本 doc handoff complete (= Phase 1.E complete marker 統合明示)
- **想定 design-lock ambiguity**: cleanup scope (= sGltfStubSkin storage 撤去のみ vs sGltfStubAssetPipeline rename 同時)、Phase 1.E complete marker doc 起案位置 (= PC-N-15 complete handoff doc 内統合 vs 別 doc)、Phase 1 全完了 marker 候補 (= Phase 1.E complete = Phase 1 完了 vs Phase 1.F 設定)
- **依存**: PC-N-11 + PC-N-12 + PC-N-13 + PC-N-14 (= worker thread design-lock 確定) → PC-N-15
- **後続**: **Phase 1.E complete** marker = 設計原則 (2) Core プロセス分散実現達成 = Phase 1 全完了 候補 (= Mac/Win 開発者補完 phase 着手起点) or Phase 1.F (= 必要性発生時に分離) 設定検討

### §4.6 GATE-B 整合 (全 sub-step 共通)

`#ifdef LL_VULKAN_GLSL` 新規追加 0 件 = memory `project_r41_phase1b_vulkan_host_gate` 遵守。host 側 redirect 層は runtime flag のみで gate (= GATE-B 確定 2026-06-04)。Phase 1.E は `AYAGltfRealDrawEnabled` cvar 単独で完結 ((E-12) B) ゆえ新規 `LLCachedControl` 追加 0 件 = GATE-B 違反なし。count llvkloader.cpp=6 不変維持。

### §4.7 MUSEUBO-A 整合 (全 sub-step 共通)

`AYAGltfRealDrawEnabled=false` default で `recordGltfAssetDraw` 発火経路ゼロ + OpenGL 描画 100% 維持 = Phase 1.D PC-N-10 確立済 baseline 不変。本 Phase 1.E 5 sub-step は全て:

- PC-N-11/12/13 = `recordGltfAssetDraw` 内 PC-N-8 (f) real Asset path block の "実 data 通電" 拡張 (= cvar=false 時不発火、`recordGltfAssetDraw` 内 5 段 graceful degrade 維持)
- PC-N-14 = design-lock phase = `indra/` 改変 0 件
- PC-N-15 = worker thread 実装 + cleanup (= cvar=false 時不発火維持、worker thread も `AYAGltfRealDrawEnabled` cvar gate 内発火)

の範囲で完結 = 実 OpenGL 描画影響ゼロ + `AYAGltfRealDrawEnabled=true` 時のみ Phase 1.E 拡張経路発火 (= live A/B 経路維持)。

### §4.8 設計原則整合 (memory `project_ayastorm_r41_design_principles`)

- **(1) Upstream OpenGL 取り込みやすさ維持** = (E-12) B 採用 = AYAGltfRealDrawEnabled 単独 cvar 完結 = `GLTFSceneManager::render` 改変最小化 + call site API 温存 + `recordGltfAssetDraw` signature 不変 ((N8-6) A 維持) + shader 改変ゼロ
- **(2) Core プロセス分散実現** = (E-3) A + (E-14) B 採用 = Phase 1.E 内 worker thread (PC-N-14 design-lock + PC-N-15 実装) で UBO write + cmdbuf recording 両方並列化達成 = per-Primitive granularity の worker thread 分散 = PC-N-8 baseline 上に実現

---

## §5. Phase 1.E decomposition Exit Criteria 9 項 ((E-15) A 採用)

| # | Criteria |
|---|----------|
| (i) | Phase 1.E 分解 5 sub-step (PC-N-11..PC-N-15) literal scope §0 明文化 |
| (ii) | 必読 1 件 (本 doc) + pinpoint reference 8 件 §1 列挙 |
| (iii) | 現状調査 §2 10 項網羅 (= code 8 項 + design doc 2 項) |
| (iv) | ambiguity (E-1)..(E-16) 16 件 AYA literal「全件推奨で OK」record (2026-06-05) §3 |
| (v) | 採用根拠 16 件 §3 明文化 |
| (vi) | PC-N-11..PC-N-15 5 sub-step 概略 + 依存関係 §4 明文化 |
| (vii) | Phase 境界明文化 = PC-N-11..PC-N-15 完了 = Phase 1.E complete = 設計原則 (2) Core プロセス分散実現達成、Phase 1.F = 必要性発生時に分離 §0 + §4.5 |
| (viii) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 §4.6 + MUSEUBO-A 整合 §4.7 + 設計原則整合 §4.8 |
| (ix) | `indra/` 改変 0 件 = `feedback_design_phase_no_code_write` 整合 |

---

## §6. 着手手順 (= 次 session で PC-N-11 design-lock 着手)

1. AYA 指示「PC-N-11 design-lock 着手お願いします」literal 受領待ち
2. 本 Phase 1.E decomposition design-lock doc 全文 Read (= 必読 1 件)
3. PC-N-10 complete doc + recordGltfAssetDraw PC-N-8 (f) writeSkinUbo + flushSkinUbos site + GLTFSceneManager::render setCurrentSkin 既配線 + design 06b §2.5 per-skin cadence pattern pinpoint Read (= PC-N-11 source)
4. PC-N-11 詳細現状調査 = `sCurrentSkin` accessor 既配線確認 + writeSkinUbo signature + real Skin owner data source 確定 (Skin.mInverseBindMatrices or computed bone matrices)
5. PC-N-11 ambiguity (= 想定 §4.1) を列挙 → 推奨案併記 → AYA literal 確認
6. PC-N-11 design-lock doc 起案 (= step (a)-(g) + Exit Criteria) → `indra/` 改変 0 件 → AYA commit 指示後 commit
7. 別 session で PC-N-11 実装着手 → complete handoff doc 起案 → AYA commit 指示後 commit
8. 以後 PC-N-12 → PC-N-13 → PC-N-14 → PC-N-15 を同パターンで進行

---

## §7. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α..PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1..PC-N-4 ✅ = Phase 1.C complete ✅ + PC-8 Linux primary marker ✅ = Phase 1.C strict 線形終了 ✅ + PC-N-5 ✅ = Phase 1.D 着手起点 ✅ + Phase 1.D decomposition design-lock ✅ + PC-N-6 ✅ + PC-N-7 ✅ + PC-N-8 ✅ + PC-N-9 ✅ + PC-N-10 ✅ = **Phase 1.D complete marker ✅** = 1 GLTF asset 完全 Vulkan draw 通電 達成 ✅ + **Phase 1.E decomposition design-lock ✅ 本 commit** + PC-N-11 design-lock ⏳ 次 session + PC-N-11 ⏳ + PC-N-12 ⏳ + PC-N-13 ⏳ + PC-N-14 ⏳ + PC-N-15 ⏳ = Phase 1.E complete ⏳ = 設計原則 (2) Core プロセス分散実現達成 ⏳ + Phase 1 全完了 ⏳ + Mac/Win 開発者補完 phase ⏳

---

## §8. self-verify 9 観点 全 ✅

1. **Phase 1.E 分解 literal scope 5 sub-step §0 完全分解** = PC-N-11 multi-skin + PC-N-12 real node modelview + PC-N-13 real per-draw light params + multi-asset verify + PC-N-14 worker thread design-lock + PC-N-15 worker thread 実装 + cleanup + Phase 1.E complete marker ✅
2. **必読 1 件 §1 + pinpoint reference 8 件別記** = PC-N-10 complete doc + Phase 1.D decomposition + recordGltfAssetDraw PC-N-8 (f) + GLTFSceneManager::render setCurrentAsset/Skin/Primitive + design 09 + cross-platform spec §6 + GATE-B + 設計原則 ✅
3. **現状調査 §2 10 項網羅** = code 8 項 (recordGltfAssetDraw PC-N-8 (f) real Asset path + sGltfStubSkin sentinel + push constant identity modelview + zero-buffer PerDrawUBO_LightParams + GLTFSceneManager::render setCurrent* 配線 + sGltfStubAssetPipeline 維持 + AYAGltfRealDrawEnabled 一本化 + per-Primitive ownership map) + design doc 2 項 + 依存関係図 ✅
4. **ambiguity (E-1)..(E-16) 16 件 AYA literal「全件推奨で OK」record (2026-06-05) §3** ✅
5. **採用根拠 16 件明文化 §3** ✅
6. **PC-N-11..PC-N-15 5 sub-step 概略 §4 + 依存関係 §2.3** ✅
7. **Phase 境界明文化 = (E-3) A 採用 = PC-N-11..PC-N-15 完了 = Phase 1.E complete = 設計原則 (2) Core プロセス分散実現達成、Phase 1.F = 必要性発生時に分離** ✅
8. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 §4.6 + MUSEUBO-A 整合 §4.7 + 設計原則整合 §4.8** ✅
9. **`indra/` 改変 0 件 = `feedback_design_phase_no_code_write` 整合 + cross-platform spec §6 Phase 1.E 行追記 + 5 sub-step stub 行追記のみ** ✅

---

## §9. 次 session 着手 1 line

**PC-N-11 design-lock 着手** = multi-skin (`sGltfStubSkin` sentinel 段階的卒業 = `sCurrentSkin` guard 追加 + fall-through to `sGltfStubSkin`、storage 撤去は PC-N-15 cleanup phase へ持越し) の詳細 step 分解 + ambiguity 確認 + Exit Criteria 明文化。`indra/` 改変 0 件、別 session で実装 phase 着手。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 Phase 1.E decomposition design-lock handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 次 session 必読 1 件 + pinpoint reference 8 件別記、本 session も Read pinpoint のみ (= PC-N-10 complete doc 全文 + Phase 1.D decomposition doc 全文 + recordGltfAssetDraw PC-N-8 (f) block + GLTFSceneManager::render setCurrentAsset/Skin/Primitive 配線 + cross-platform spec §6 table format)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §8
- **feedback_build_only_verified** 遵守 = design-lock phase は `indra/` 改変 0 件で build verify 対象外、各 sub-step 実装 phase で literal 検証取得予定 ((E-16) A 採用)
- **feedback_no_scope_shrink** 遵守 = Phase 1.E 分解 5 sub-step §0 完全分解、AYA 起案 3 scope (multi-asset / multi-skin / worker thread) + PC-N-8 (f) 内 placeholder 3 件卒業 ((E-2) B 拡張採用) を統合的に 5 sub-step で網羅 = 縮小ではなく "実 data 通電" thesis 統合、(E-3) A worker thread = Phase 1.E 内 (AYA 起案直訳) 採用、Phase 1.F は必要性発生時に分離 = literal scope 完全網羅
- **feedback_doubt_self_first** 遵守 = ambiguity 16 件発見で停止 + 推奨案提示 + AYA literal「全件推奨で OK」確認後本 design-lock doc 起案、推測実装なし、特に (E-2) B 範囲拡張 + (E-3) A vs Phase 1.F 境界 + (E-6) B 段階的 vs 一括は緊張点として明示後 AYA 確認
- **feedback_confirm_referent_before_acting** 遵守 = 16 件 batch AYA 確認 (2026-06-05)、各候補 + 推奨案 + 根拠明示後 AYA literal「全件推奨で OK」record 受領で確定、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = Phase 1.E を 5 sub-step に分解、各 sub-step は別 session で個別 design-lock + 実装、本 doc は overview のみ
- **feedback_design_phase_no_code_write** 整合 = 本 Phase 1.E decomposition design-lock は doc 起案のみ、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領まで commit せず
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
- **feedback_no_bare_reference_ids** 遵守 = (E-1)..(E-16) 各 ID に項目名 / 採用案内容併記 §3 + (PC-N-11)..(PC-N-15) 各 ID に scope 内容併記 §0 + §4
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、git add 個別 file 指定予定
- **memory `project_ayastorm_r41_design_principles`** 整合 = (1) Upstream OpenGL 取り込みやすさ維持 = (E-12) B 採用 = AYAGltfRealDrawEnabled 単独 cvar 完結 + GLTFSceneManager::render 改変最小化 + signature 不変 + shader 改変ゼロ §4.8 + (2) Core プロセス分散実現 = (E-3) A + (E-14) B 採用 = Phase 1.E 内 worker thread (PC-N-14 + PC-N-15) で UBO write + cmdbuf recording 並列化達成 §4.8
- **memory `project_r41_phase1b_vulkan_host_gate`** 整合 = GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、Phase 1.E は AYAGltfRealDrawEnabled cvar 単独完結 = 新規 LLCachedControl 追加 0 件、count llvkloader.cpp=6 不変維持 §4.6
- **memory `project_ayastorm_three_platforms`** 整合 = cross-platform spec §6 Phase 1.E 行追記 + 5 sub-step stub 行追記で macOS / Windows 派生 fix 候補欄起案、Linux primary 完成 → 他者補完 model と整合

---
