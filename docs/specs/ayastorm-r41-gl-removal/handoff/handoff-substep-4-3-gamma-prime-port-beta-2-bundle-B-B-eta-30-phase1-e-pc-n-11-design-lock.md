# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.E **PC-N-11 design-lock complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: Phase 1.E 内 **1st sub-step = PC-N-11 = multi-skin** (= `sGltfStubSkin` sentinel 段階卒業 + real Skin owner 切替 + Skin_GLTFJoints UBO 実 data write + `AYAGltfMultiSkinEnabled` cvar 新設) の design-lock phase 完了 marker = ambiguity (N11-1)..(N11-16) 16 件 全 AYA literal「全件推奨で OK」record (2026-06-05) + 実装計画 (a)-(g) 7 step 分解 + Exit Criteria 9+10 項明文化。`indra/` 改変 0 件 (= `feedback_design_phase_no_code_write` 整合)。

> **本 doc 位置付け**: PC-N-11 詳細 design-lock。Phase 1.E decomposition design-lock (= `handoff-...-phase1-e-decomposition-design-lock.md`) で確立した PC-N-11..PC-N-15 5 sub-step 分解 + 16 件 ambiguity (E-1)..(E-16) AYA 全件推奨採用 baseline 上に、PC-N-11 単独の **詳細実装計画** + **想定 code diff example** + **ambiguity 16 件 (N11-1)..(N11-16)** + **Exit Criteria 10 項** を確定。Phase 1.D PC-N-10 design-lock doc と同形 pattern 踏襲 (= `handoff-...-phase1-d-pc-n-10-design-lock.md`)。実装は別 session で別途着手 (= `feedback_ubo_migration_one_at_a_time` 厳格遵守)。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「r41 Phase 1.E PC-N-11 design-lock 着手お願いします。直前 commit = `01cd001d07` (Phase 1.E decomposition design-lock complete = PC-N-11..PC-N-15 5 sub-step 分解 + ambiguity (E-1)..(E-16) 16 件 AYA 全件推奨採用)。必読 1 件: `handoff-...-phase1-e-decomposition-design-lock.md`。PC-N-11 scope = multi-skin = `sGltfStubSkin` sentinel 段階卒業 + real Skin owner 切替 (= `sCurrentSkin` 経由、`gltfscenemanager.cpp:765` `setCurrentSkin` 既配線済) + Skin_GLTFJoints UBO 実 data write + `AYAGltfMultiSkinEnabled` cvar 新設。`feedback_ubo_migration_one_at_a_time` 厳格遵守で ambiguity 出し + 採用案提示 + AYA 確認 → design-lock doc 起案 (= `indra/` 改変 0 件、`feedback_design_phase_no_code_write` 遵守)。」literal 受領 (2026-06-05、Phase 1.E decomposition design-lock commit `01cd001d07` 後の継続 session = 別 session の fresh context)。

**PC-N-11 literal scope** (= AYA task statement 直訳、4 項):

1. **`sGltfStubSkin` sentinel 段階卒業** (= `sCurrentSkin` guard 追加 + fall-through to `sGltfStubSkin`、storage 自体の撤去は PC-N-15 cleanup phase へ持越し ((N11-3) A 採用 + (E-6) B (E-11) A 整合))
2. **real Skin owner 切替** (= `sCurrentSkin` 経由、`gltfscenemanager.cpp:765` `setCurrentSkin(&skin)` 既配線済を `recordGltfAssetDraw` PC-N-8 (f) 内で消費)
3. **Skin_GLTFJoints UBO 実 data write** (= `Skin::uploadMatrixPalette` PC-7γ-3 (j) dual-write upstream で既配線済 = `recordGltfAssetDraw` 側 inline write 不要 ((N11-4) A 採用)、ここでは `flushSkinUbos(sCurrentSkin)` + `wireSkinUboSetV3aToBinding2(sCurrentSkin)` で descriptor binding 配線のみ ((N11-5) A 採用))
4. **`AYAGltfMultiSkinEnabled` cvar 新設** (= Boolean default 0 Persist 1、PC-N-6/7/9 同形 r41 pattern ((N11-2) A 採用)、settings.xml 1 件追加 + LLCachedControl 配置)

**Phase 境界**: PC-N-11 完了 = Phase 1.E 内 1st sub-step 完了 = real Skin owner 経由 multi-skin path 通電 baseline 確立。`sGltfStubSkin` storage 撤去 + fall-through 経路撤去は PC-N-15 cleanup phase へ持越し ((E-11) A + (E-10) B 整合)。

---

## §1. 必読 1 件 + pinpoint reference

### §1.1 必読 1 件 (= 次 session = PC-N-11 実装 phase 着手前)

1. **本 PC-N-11 design-lock doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-e-pc-n-11-design-lock.md`

### §1.2 pinpoint reference 12 件 (= 実装 phase で必要分のみ Read)

1. **`recordGltfAssetDraw` PC-N-8 (f) real Asset path block**: `indra/llrender/llvkloader.cpp:5906-6021` = PC-N-11 改変対象本体 (= sCurrentSkin guard + cvar guard + wireSkinUboSetV3aToBinding2 追加 site)
2. **`recordGltfAssetDraw` PC-N-8 (f) writeSkinUbo + flushSkinUbos site**: `indra/llrender/llvkloader.cpp:5968-5974` = sentinel + identity 64 B 書込から real Skin owner 切替対象 site
3. **`recordGltfAssetDraw` PC-N-8 (f) bindV3aRigged call**: `indra/llrender/llvkloader.cpp:5975` = bindV3aRigged 前に wireSkinUboSetV3aToBinding2 挿入対象 site
4. **`setCurrentSkin` accessor**: `indra/llrender/llvkloader.cpp:5730-5743` = PC-7γ-2 既配線済 + `sCurrentSkin` static field (line 668)
5. **`gltfscenemanager.cpp:765` `setCurrentSkin(&skin)`**: PC-7γ-2 既配線済 (= per-Primitive scope `Skin& skin = asset.mSkins[node.mSkin]` reference 投入、`:787` clearCurrentSkin 対称配置)
6. **`writeSkinUbo` 実装**: `indra/llrender/llvkloader.cpp:5527-5557` = signature 確認 (= `Skin*, block_hash, offset, data, size`)
7. **`flushSkinUbos` 実装**: `indra/llrender/llvkloader.cpp:4921` 付近 = dirty exchange 経路
8. **`wireSkinUboSetV3aToBinding2` 実装**: `indra/llrender/llvkloader.cpp:2828-2886` = 共有 `sAssetUboSetV3a[f]` descriptor set の binding=2 を指定 Skin の UBO buffer に再 wire helper
9. **`registerSkinUbo` 実装**: `indra/llrender/llvkloader.cpp:5452-5509` = lazy register pattern + 初回 register 時 descriptor wire (= line 5460-5507)
10. **`Skin::uploadMatrixPalette` PC-7γ-3 (j) dual-write**: `indra/newview/gltf/animation.cpp:413-478` = real Skin の Skin_GLTFJoints UBO 実 data write (= `node.mAssetMatrix * mInverseBindMatricesData[i]` joint matrix palette、PC-7γ-3 (m) lazy register 経由)
11. **`Asset::update` → `Skin::uploadMatrixPalette` per-frame caller**: `indra/newview/gltf/asset.cpp:536-539` = per-frame で全 Skin upload、`recordGltfAssetDraw` PC-N-8 (f) 発火前に実行
12. **settings.xml `AYAGltfRealDrawEnabled` cvar 配置**: `indra/newview/app_settings/settings.xml` (= PC-N-9 (c) で line ?? 配置、本 PC-N-11 で `AYAGltfMultiSkinEnabled` 直後並列追加 ((N11-10) A 採用))

---

## §2. 現状調査結果 (= Phase 1.D complete baseline + PC-7γ-3 既配線確認)

### §2.1 PC-N-8 (f) 現状 (= 改変対象 site)

| # | site | file:line | 現状 | PC-N-11 改変 |
|---|------|-----------|------|-------------|
| 1 | sGltfStubSkin sentinel + identity 64 B writeSkinUbo | `llvkloader.cpp:5968-5974` | `writeSkinUbo(sGltfStubSkin, Skin_GLTFJoints, 0, identity_buf, 64)` + `flushSkinUbos(sGltfStubSkin)` | cvar guard + `sCurrentSkin` guard で real Skin path 分岐 = real Skin path = `wireSkinUboSetV3aToBinding2(sCurrentSkin)` + `flushSkinUbos(sCurrentSkin)` (= inline writeSkinUbo 不要、upstream uploadMatrixPalette 既配線済) / fall-through path = 既 sentinel writeSkinUbo + flushSkinUbos 維持 |
| 2 | bindV3aRigged call | `llvkloader.cpp:5975` | `bindV3aRigged(cmd_buf, sFrameIndex, real_asset_dynamic_offsets)` | 不変 (= wireSkinUboSetV3aToBinding2 でこの bindV3aRigged 直前に binding=2 を current Skin に再 wire 済 = bindV3aRigged が正確な Skin UBO buffer に bind) |
| 3 | push constant identity | `llvkloader.cpp:5979-5990` | identity 64 B vkCmdPushConstants | 不変 (= PC-N-12 scope) |
| 4 | zero-buffer PerDrawUBO_LightParams | `llvkloader.cpp:5943-5950` | 256 B zero buf writeDrawUbo | 不変 (= PC-N-13 scope) |

### §2.2 上流既配線確認 (= PC-7γ-3 + PC-N-9)

| # | 項目 | 配置 | 状態 |
|---|------|------|------|
| 5 | `Skin::uploadMatrixPalette` PC-7γ-3 (m) lazy register | `animation.cpp:425` | 配線済 = first call で `registerSkinUbo(this, Skin_GLTFJoints, 16384u)`、descriptor wire も自動 (= `registerSkinUbo` line 5460-5507 内蔵) |
| 6 | `Skin::uploadMatrixPalette` PC-7γ-3 (j) dual-write | `animation.cpp:476-478` | 配線済 = `writeSkinUbo(this, Skin_GLTFJoints, 0, glmp.data(), joint_count*48)` で real bone matrix palette 投入 |
| 7 | `Skin::~Skin()` PC-7γ-3 (o) unregister | `animation.cpp:407-410` | 配線済 = `unregisterSkinUbo(this, Skin_GLTFJoints)` Skin lifecycle teardown |
| 8 | `Asset::update` per-frame Skin upload loop | `asset.cpp:536-539` | 配線済 = per-frame で全 `mSkins` iterate + `skin.uploadMatrixPalette(*this)` |
| 9 | `gltfscenemanager.cpp:765` `setCurrentSkin(&skin)` | PC-7γ-2 | 配線済 = per-Primitive `if (rigged)` 内、`&asset.mSkins[node.mSkin]` reference 投入、`:787` `clearCurrentSkin()` 対称配置 |
| 10 | `recordAvatarPlaceholderDraw` 末尾 `AYAGltfRealDrawEnabled` entry hook | PC-N-10 (a) | 配線済 = cvar=true 時のみ `recordGltfAssetDraw` 発火 = Phase 1.E entry point 一本化済 |

### §2.3 重大 finding: descriptor binding stale 化 risk

**(原理)**: `registerSkinUbo` (`llvkloader.cpp:5460-5507`) は **初回 register 時のみ** 共有 `sAssetUboSetV3a[f]` descriptor set binding=2 を当該 Skin の UBO buffer に wire。Frame 内で 2nd Skin が register されると binding=2 は 2nd Skin に上書き、初 Skin の binding は stale 化 (= `recordGltfAssetDraw` 内 `flushSkinUbos(初 Skin)` 後 `bindV3aRigged` で binding=2 = 2nd Skin の UBO buffer が bind される)。

**(現状)**: PC-N-8 (f) は `flushSkinUbos(sGltfStubSkin)` 後 `bindV3aRigged` を per-draw 呼出するが、`wireSkinUboSetV3aToBinding2(sGltfStubSkin)` per-draw 呼出はしていない (= initVulkan PC-N-5 (c) での 1 回 wire 依存)。real Skin の lazy register が走った瞬間に sGltfStubSkin の binding は stale 化、現 PC-N-8 (f) sentinel-only path も multi-real-Skin 環境では binding mismatch リスク内包。

**(PC-N-11 解決)**: real Skin path / sGltfStubSkin fall-through path 問わず `wireSkinUboSetV3aToBinding2(skin_to_use)` を `bindV3aRigged` 直前で per-draw 呼出 ((N11-5) A 採用) = multi-skin 環境で正確な descriptor binding 保証。`vkUpdateDescriptorSets` per-draw cost は Vulkan API 仕様 design intent 範囲内、per-Skin descriptor set 別建ては Phase 1.F 候補で温存。

### §2.4 (E-12) doc body B vs commit msg (E-6) A vs AYA PC-N-11 task statement の正式 resolve ((N11-1) A 採用)

| source | cvar 戦略 |
|--------|----------|
| Phase 1.E decomposition design-lock doc 本文 §3.3 (E-12) | B: 単一 cvar (`AYAGltfRealDrawEnabled`) で完結 = 追加 cvar 0 件 |
| Phase 1.E decomposition design-lock commit `01cd001d07` commit message (E-6) | A: 各 sub-step 単一 cvar gate = `AYAGltfMultiSkinEnabled` / `AYAGltfRealModelviewEnabled` / `AYAGltfRealLightParamsEnabled` / `AYAGltfMultiAssetCanary` / `AYAGltfWorkerThreadEnabled` 5 cvar 段階通電 |
| AYA PC-N-11 task statement (2026-06-05) | literal: `AYAGltfMultiSkinEnabled` cvar 新設 |
| **(N11-1) AYA 全件推奨採用結果** | **A**: AYA task statement literal 採用 = `AYAGltfMultiSkinEnabled` cvar 新設 (commit msg (E-6) A 整合) |

**採用根拠**: AYA task statement が直近 source of truth + commit msg (E-6) A 5 cvar 段階通電方針整合 + PC-N-9 同形 per-feature cvar pattern 踏襲 + 各 sub-step independent live A/B 価値高 = doc 本文 §3.3 (E-12) B は本 (N11-1) A 採用で deprecated として扱う (= 各 sub-step design-lock 時に個別 resolve 原則)。

---

## §3. ambiguity (N11-1)..(N11-16) 16 件 AYA literal「全件推奨で OK」record (2026-06-05) + 採用根拠

### §3.1 cvar 戦略系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N11-1) | cvar 戦略整合 (= doc 本文 (E-12) B vs commit msg (E-6) A vs AYA task statement literal 不整合 resolve) | **A**: AYA PC-N-11 task statement literal 採用 = `AYAGltfMultiSkinEnabled` cvar 新設 (commit msg (E-6) A 整合) | OK (2026-06-05) | AYA task statement が直近 source of truth + commit msg (E-6) A 5 cvar 段階通電方針整合 + PC-N-9 同形 per-feature cvar pattern 踏襲 + 各 sub-step independent live A/B 価値高 |
| (N11-2) | `AYAGltfMultiSkinEnabled` default + Persist | **A**: Boolean default=0 (false) + Persist=1 | OK (2026-06-05) | 既存 PC-N-6/7/9 cvar default off pattern 厳格踏襲、MUSEUBO-A 整合 (= default で OpenGL 描画 100% 維持、`recordGltfAssetDraw` PC-N-8 (f) real Skin path 不発火 = sentinel fall-through 経路維持) |

### §3.2 PC-N-8 (f) 改変系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N11-3) | Skin 選択 logic | **A**: `sCurrentSkin != nullptr` 時 real Skin path / `sCurrentSkin == nullptr` 時 fall-through to `sGltfStubSkin` (非 rigged primitive 等の natural guard) | OK (2026-06-05) | (E-6) B 段階的卒業 + (E-11) A `sGltfStubSkin` storage 撤去 = PC-N-15 cleanup phase 持越し pattern 厳格踏襲、fall-through で graceful degrade 維持 |
| (N11-4) | real Skin path での `writeSkinUbo` inline call 要否 | **A**: 不要 (= `Skin::uploadMatrixPalette` PC-7γ-3 (j) dual-write upstream で実 data 既書込済、ここで再書込は冗長) | OK (2026-06-05) | upstream 既配線資産活用、二重 write 冗長排除、real Skin path = `uploadMatrixPalette` が source of truth、sGltfStubSkin fall-through path は既存 identity 64 B write 維持 (= PC-N-15 cleanup まで温存) |
| (N11-5) | `wireSkinUboSetV3aToBinding2(skin_to_use)` per-draw 呼出要否 | **A**: 必要 (= multi-skin で frame 内 Skin 切替時 descriptor binding が前回 Skin に stale 化、per-draw rewire で正確性確保、real / sentinel 問わず unconditional) | OK (2026-06-05) | multi-skin 本質 = PC-N-11 scope core、`vkUpdateDescriptorSets` per-draw cost は Vulkan API 仕様 design intent 範囲内、per-Skin descriptor set 別建ては Phase 1.F 候補で温存 = 設計原則 (1) 上流互換維持で正確性優先、§2.3 descriptor binding stale 化 risk 解決 |
| (N11-6) | `flushSkinUbos(skin_to_use)` 呼出 | **A**: real Skin / sGltfStubSkin 問わず unconditional 呼出 (= 既 PC-N-8 (f) pattern 維持) | OK (2026-06-05) | dirty exchange は両 path 共通必須、unconditional 呼出で graceful、コード簡素 |

### §3.3 構造系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N11-7) | tag block 配置 + 命名 | **A**: 既 PC-N-8 (f) tag block 内側に新規 `<AYAstorm r41 PC-N-11 (a)>` 内 tag で sCurrentSkin guard + cvar guard 包む (= surgical insertion、PC-N-8 (f) outer block 構造温存) | OK (2026-06-05) | code archaeology 容易 + 各 PC-N-? 改変 surgical 配置 = PC-N-9/PC-N-10 同形 pattern 踏襲 |
| (N11-8) | `AYAGltfMultiSkinEnabled` cvar guard 配置 | **A**: PC-N-8 (f) 内 Skin handling 部分のみを guard 内に入れる (= 内側、push constant / vertex buffer bind は cvar 外) | OK (2026-06-05) | PC-N-11 scope は Skin handling のみゆえ guard 内側配置で最小範囲化、push constant / vertex buffer / index buffer / draw call は PC-N-12/13 で別途 guard 化、cvar guard 階層化で各 sub-step independent A/B 維持 |

### §3.4 副次系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N11-9) | first-fire `LL_INFOS` marker 追加 | **A**: PC-N-11 専用 first-fire marker 追加 (= "PC-N-11 (a) real Skin multi-skin path first fire" diagnostic、`s_first_pcn11_real_skin_fire` atomic flag) | OK (2026-06-05) | PC-N-6/7/8/9/10 同形 pattern 踏襲、通電 literal 確認用 |
| (N11-10) | settings.xml `AYAGltfMultiSkinEnabled` cvar 配置位置 | **A**: 既 `AYAGltfRealDrawEnabled` cvar 直後並列 (= Phase 1.E cvar group 起点、PC-N-12/13/14/15 cvar もここに並べる想定) | OK (2026-06-05) | Phase 1.E cvar group 専用領域確立、`AYAGltfRealDrawEnabled` 直後で「Phase 1.E 追加 cvar」明示 |
| (N11-11) | cvar Comment 内容 | **B**: `AYAGltfMultiSkinEnabled` 単独のみ (将来 cvar は当該 sub-step 着手時に追記) | OK (2026-06-05) | `feedback_no_scope_shrink` 違反ではなく、各 sub-step 別 session で別途 design-lock = 現時点で他 sub-step cvar の literal 確定なし、untouched 領域 silence 原則 |

### §3.5 検証 + Exit Criteria 系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N11-12) | build verify scope | **A**: PC-N-6/7/8/9/10 同形 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 + GATE-B integrity (= LL_VULKAN_GLSL count llvkloader.cpp=6 不変) | OK (2026-06-05) | (E-8) commit msg / (E-16) doc body 整合 |
| (N11-13) | Exit Criteria 項目数 | **A**: 10 項 (PC-N-6/7/8/9/10 実装 phase 同形) | OK (2026-06-05) | PC-N-11 = 実装系 sub-step ゆえ実装 phase Exit Criteria 10 項 pattern 踏襲 |
| (N11-14) | multi-skin verification 方法 | **A**: 既存 SL inv item の rigged GLTF (複数 Skin 持つ asset or 複数 rigged asset) で AYA 実機確認 ((E-14) A integration approach 整合) | OK (2026-06-05) | real SL sample 信任、synthetic 不要、AYA 実機 with rigged GLTF 装着物等で multi-skin path 自然発火 |

### §3.6 design-lock phase 整合系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N11-15) | 想定改変 file 件数 (実装 phase) | **A**: 4 file = `indra/llrender/llvkloader.cpp` (PC-N-8 (f) 内 sCurrentSkin guard + cvar guard + wireSkinUboSetV3aToBinding2 + tag block + LLCachedControl 宣言) + `indra/newview/app_settings/settings.xml` (`AYAGltfMultiSkinEnabled` cvar 1 件追加) + cross-platform spec §6 PC-N-11 行 ✅ 反映 + new handoff complete doc | OK (2026-06-05) | upstream `Skin::uploadMatrixPalette` PC-7γ-3 (j) 既配線済ゆえ `animation.cpp` 改変不要、host-side `recordGltfAssetDraw` 内のみで完結 |
| (N11-16) | design-lock phase `indra/` 改変 0 件遵守 | **A**: 本 PC-N-11 design-lock doc 起案 + cross-platform spec §6 PC-N-11 行 design-lock 内容更新のみ、`indra/` 改変 0 件 = `feedback_design_phase_no_code_write` 厳格遵守 | OK (2026-06-05) | 全 design-lock phase 共通 pattern 踏襲 |

---

## §4. 実装計画 (a)-(g) 7 step (= 別 session で着手)

> **注**: 本 §4 は **実装 phase 用 step 分解 + 想定 code diff example** = 本 design-lock phase は `indra/` 改変 0 件、実装は別 session で別途着手 (= `feedback_design_phase_no_code_write` + `feedback_ubo_migration_one_at_a_time` 厳格遵守)。

### §4.1 step (a) — `AYAGltfMultiSkinEnabled` cvar 新設 (settings.xml)

`indra/newview/app_settings/settings.xml` の既 `AYAGltfRealDrawEnabled` cvar 直後並列 ((N11-10) A) で `AYAGltfMultiSkinEnabled` cvar 1 件追加。Boolean default=0 Persist=1 ((N11-2) A)、PC-N-6/7/9 同形 r41 pattern。Comment は本 cvar 単独説明のみ ((N11-11) B = 将来 cvar 列挙しない)。

**想定 XML diff example**:

```xml
<key>AYAGltfMultiSkinEnabled</key>
<map>
    <key>Comment</key>
    <string>
        AYAstorm r41 Phase 1.E PC-N-11 = multi-skin sentinel 段階卒業 cvar
        (= recordGltfAssetDraw PC-N-8 (f) real Asset path 内 sCurrentSkin
        経由 real Skin owner 切替 + Skin_GLTFJoints UBO 実 data write 経路
        通電 = sGltfStubSkin sentinel fall-through path と独立 live A/B)。
        OFF (default) = 既 PC-N-8 (f) sentinel + identity 64 B writeSkinUbo
        path 維持 (= Phase 1.D complete baseline 不変)。
        ON = sCurrentSkin 非 null 時 real Skin path 発火 = upstream
        Skin::uploadMatrixPalette PC-7γ-3 (j) dual-write 既 real bone matrix
        palette を消費 + wireSkinUboSetV3aToBinding2(sCurrentSkin) per-draw
        rewire で multi-skin descriptor binding 正確性確保。
        Prerequisite: AYAGltfRealDrawEnabled=1 (= recordGltfAssetDraw fire entry)。
    </string>
    <key>Persist</key>
    <integer>1</integer>
    <key>Type</key>
    <string>Boolean</string>
    <key>Value</key>
    <integer>0</integer>
</map>
```

### §4.2 step (b) — `LLCachedControl<bool>` 宣言 追加 (llvkloader.cpp)

`indra/llrender/llvkloader.cpp` 内 `recordGltfAssetDraw` 関数内に PC-N-9 同形 pattern で `static LLCachedControl<bool> sAyastormGltfMultiSkinEnabled(gSavedSettings, "AYAGltfMultiSkinEnabled", false)` 配置 ((N11-1) A + (N11-2) A、PC-N-9 (b) `sAyastormGltfRealDrawEnabled` と同形)。

### §4.3 step (c) — PC-N-8 (f) writeSkinUbo + flushSkinUbos site の sCurrentSkin guard 追加 (llvkloader.cpp)

`recordGltfAssetDraw` PC-N-8 (f) 内の writeSkinUbo + flushSkinUbos site (= line 5968-5974) を新規 `<AYAstorm r41 PC-N-11 (a)>` tag block で wrap ((N11-7) A、surgical insertion)。cvar guard 内で `sCurrentSkin` non-null 時 real Skin path / null 時 fall-through to `sGltfStubSkin` ((N11-3) A)。real Skin path では inline `writeSkinUbo` 不要 ((N11-4) A)、`wireSkinUboSetV3aToBinding2(sCurrentSkin)` + `flushSkinUbos(sCurrentSkin)` のみ ((N11-5) A + (N11-6) A)。

**想定 C++ diff example** (= line 5956-5975 改変):

```cpp
// PC-N-8 (f) 同形 per-Skin UBO 配線 (= PC-N-7 (e) 同形、stub sentinel 共用)。
//   PC-N-9 で real Skin 経路へ置換予定 ((N8-1) B = per-Primitive scope
//   ゆえ Skin owner 切替は PC-N-9 GLTFSceneManager::render 統合 phase)。
// <AYAstorm r41 PC-N-11 (a)> multi-skin sentinel 段階卒業 ((N11-1)..(N11-16) AYA
//   literal「全件推奨で OK」record 2026-06-05 採用)。AYAGltfMultiSkinEnabled cvar=true
//   かつ sCurrentSkin != nullptr 時 real Skin owner path = upstream uploadMatrixPalette
//   PC-7γ-3 (j) dual-write 既書込 real bone matrix palette を消費 (inline writeSkinUbo
//   不要 (N11-4) A) + wireSkinUboSetV3aToBinding2(sCurrentSkin) per-draw rewire で
//   multi-skin descriptor binding 正確性確保 ((N11-5) A、§2.3 stale 化 risk 解決)。
//   cvar=false or sCurrentSkin==nullptr 時 fall-through to sGltfStubSkin sentinel path
//   = 既 identity 64 B writeSkinUbo + flushSkinUbos 維持 ((N11-3) A、PC-N-15 cleanup phase
//   まで storage 温存 = (E-11) A 整合)。flushSkinUbos + wireSkinUboSetV3aToBinding2 は
//   real / sentinel 問わず unconditional 呼出 ((N11-5) A + (N11-6) A、code 簡素)。
static LLCachedControl<bool> sAyastormGltfMultiSkinEnabled(
    gSavedSettings, "AYAGltfMultiSkinEnabled", false);
LL::GLTF::Skin* const skin_to_use =
    (sAyastormGltfMultiSkinEnabled && LLVKLoader::getCurrentSkin() != nullptr)
        ? LLVKLoader::getCurrentSkin()
        : sGltfStubSkin;
if (skin_to_use == sGltfStubSkin)
{
    // fall-through sentinel path = 既 identity 64 B writeSkinUbo 維持 (PC-N-15 cleanup まで)。
    static const F32 real_asset_identity_skin_buf[64] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
        0.f, 0.f, 0.f, 0.f,  0.f, 0.f, 0.f, 0.f,  0.f, 0.f, 0.f, 0.f,  0.f, 0.f, 0.f, 0.f,
        0.f, 0.f, 0.f, 0.f,  0.f, 0.f, 0.f, 0.f,  0.f, 0.f, 0.f, 0.f,  0.f, 0.f, 0.f, 0.f,
        0.f, 0.f, 0.f, 0.f,  0.f, 0.f, 0.f, 0.f,  0.f, 0.f, 0.f, 0.f,  0.f, 0.f, 0.f, 0.f,
    };
    LLVKLoader::writeSkinUbo(
        skin_to_use,
        ubo::block_hash::Skin_GLTFJoints,
        /*offset=*/0u,
        reinterpret_cast<const U8*>(real_asset_identity_skin_buf),
        sizeof(real_asset_identity_skin_buf));
}
// real Skin path では inline writeSkinUbo 不要 (= uploadMatrixPalette 既書込)。
// flushSkinUbos + wireSkinUboSetV3aToBinding2 は両 path unconditional。
LLVKLoader::wireSkinUboSetV3aToBinding2(skin_to_use);
LLVKLoader::flushSkinUbos(skin_to_use);
// </AYAstorm r41 PC-N-11 (a)>
bindV3aRigged(cmd_buf, sFrameIndex, real_asset_dynamic_offsets);
```

> **注**: `wireSkinUboSetV3aToBinding2` は namespace `LLVKLoader` 内 free function、`getCurrentSkin` は LLVKLoader namespace public accessor。可視性確認は実装時に再 grep verify (`LLVKLoader::wireSkinUboSetV3aToBinding2` 既 public か anonymous namespace 内かは実装時確認、anonymous の場合は public 化 1 site 改変併発)。

### §4.4 step (d) — first-fire `LL_INFOS` marker 追加 (llvkloader.cpp)

PC-N-11 (a) tag block 内、real Skin path 発火時 first-fire marker 追加 ((N11-9) A、PC-N-8 (f) 同形 pattern)。

**想定 C++ diff example**:

```cpp
// PC-N-11 (a) first-fire LL_INFOS marker (= multi-skin real Skin path 通電 literal)。
if (skin_to_use != sGltfStubSkin)
{
    static std::atomic<bool> s_first_pcn11_real_skin_fire{true};
    if (s_first_pcn11_real_skin_fire.exchange(false, std::memory_order_acq_rel))
    {
        LL_INFOS("Vulkan") << "PC-N-11 (a) multi-skin real Skin path 通電 (first fire): "
                              "skin=" << skin_to_use
                           << ", sentinel(sGltfStubSkin)=" << (void*)sGltfStubSkin
                           << "; AYAGltfMultiSkinEnabled=true + sCurrentSkin 非 null = "
                              "upstream Skin::uploadMatrixPalette PC-7γ-3 (j) dual-write 経由 "
                              "real bone matrix palette 消費 + wireSkinUboSetV3aToBinding2 "
                              "per-draw rewire で descriptor binding=2 を real Skin UBO buffer "
                              "に切替、bindV3aRigged 後 set=3 binding=2 = real Skin UBO bind"
                           << LL_ENDL;
    }
}
```

### §4.5 step (e) — cross-platform spec §6 PC-N-11 行 design-lock 内容更新 (= 本 design-lock phase 改変対象)

`docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` §6 PC-N-11 行を本 PC-N-11 design-lock complete 内容に更新 (= AYA 確認待ち → design-lock complete 16 件 ambiguity 採用案サマリ + macOS / Windows 派生 fix 候補なし明示) + §A 履歴 1 行追記。

### §4.6 step (f) — build verify literal 取得 (= 別 session 実装 phase)

`indra/` 改変後 ((N11-12) A 採用 scope):

```bash
# (1) llrender build
make -j4 llrender
# (2) INTEGRATION_TEST_lluboringbuffer
./build-linux-x86_64/test/INTEGRATION_TEST_lluboringbuffer
# (3) INTEGRATION_TEST_llassetubopool
./build-linux-x86_64/test/INTEGRATION_TEST_llassetubopool
# (4) INTEGRATION_TEST_llpipelinecachestorage
./build-linux-x86_64/test/INTEGRATION_TEST_llpipelinecachestorage
# (5) codegen unittest
cd scripts/ubo_codegen && python3 -m unittest discover tests
# (6) GATE-B integrity
grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp   # 期待値 = 6 (PC-N-10 commit cccd411486 同数)
```

期待 literal = (1) PASS / ERROR 0 / WARNING 0 + (2) 11/11 PASS YAY + (3) 10/10 PASS YAY + (4) 13/13 PASS YAY + (5) 131/131 OK + (6) 6。

### §4.7 step (g) — handoff complete doc 起案 + AYA commit 指示後 commit (= 別 session 実装 phase)

`handoff-substep-...-phase1-e-pc-n-11-complete.md` 起案 = step (a)-(g) 全実施 record + Exit Criteria 10 項全充足 + build verify literal 取得 + GATE-B integrity record + self-verify 9 観点 全 ✅ + 引き継ぎ memory + 次 session 着手 1 line (= PC-N-12 design-lock 着手 = real node modelview push constant)。AYA literal 「commit してください」受領後 commit ((feedback_no_auto_commit + feedback_release_branch_workflow + feedback_no_claude_coauthor 遵守))。

### §4.8 GATE-B 整合 (= 全 step 共通)

`#ifdef LL_VULKAN_GLSL` 新規追加 0 件 = memory `project_r41_phase1b_vulkan_host_gate` 遵守。host 側 redirect 層は runtime flag (`AYAGltfMultiSkinEnabled` cvar) のみで gate。新規 cvar 追加 1 件 (`LLCachedControl<bool>`) は runtime flag ゆえ GATE-B 違反なし。count llvkloader.cpp=6 不変維持。

### §4.9 MUSEUBO-A 整合 (= 全 step 共通)

`AYAGltfMultiSkinEnabled=false` default で PC-N-11 (a) real Skin path 不発火 = sentinel fall-through path 維持 = Phase 1.D PC-N-10 complete baseline 不変 = OpenGL 描画 100% 維持。`AYAGltfRealDrawEnabled=false` default で `recordGltfAssetDraw` 全経路発火なし = 二重 gate 維持。

### §4.10 設計原則整合 (= memory `project_ayastorm_r41_design_principles`)

- **(1) Upstream OpenGL 取り込みやすさ維持**: `Skin::uploadMatrixPalette` (animation.cpp:413) OpenGL path 完全温存 + dual-write defensive hook PC-7γ-3 (j) 不変 + `recordGltfAssetDraw` signature 不変 + GLTFSceneManager::render 改変 0 件 + shader 改変ゼロ
- **(2) Core プロセス分散実現**: per-Primitive scope `sCurrentSkin` accessor 経由 Skin owner 切替 = primitive-level granularity の worker thread 分散余地確保 (= PC-N-14/15 worker thread design-lock + 実装で per-Primitive UBO write + cmdbuf record 並列化達成、PC-N-11 はその baseline)

---

## §5. PC-N-11 design-lock Exit Criteria 9 項

| # | Criteria | 充足 |
|---|----------|------|
| (i) | PC-N-11 literal scope §0 明文化 (= AYA task statement literal 4 項 + (E-6) B (E-11) A 整合の段階卒業方針) | ✅ |
| (ii) | 必読 1 件 §1.1 (本 doc) + pinpoint reference 12 件 §1.2 別記 = full file dump なし | ✅ |
| (iii) | 現状調査 §2 10 項網羅 (= PC-N-8 (f) 改変対象 site 4 項 + 上流既配線確認 PC-7γ-3 + PC-N-9 6 項 + §2.3 descriptor binding stale 化 risk + §2.4 (E-12) doc body vs commit msg vs AYA task statement resolve) | ✅ |
| (iv) | ambiguity (N11-1)..(N11-16) 16 件 + AYA literal「全件推奨で OK」record (2026-06-05) §3 | ✅ |
| (v) | 採用根拠 16 件 §3 明文化 (= 各 (N11-*) ID に項目名 + 採用案内容 + 根拠併記) | ✅ |
| (vi) | 実装計画 (a)-(g) 7 step §4 分解 + 各 step 具体 code diff example 添付 | ✅ |
| (vii) | GATE-B 整合 §4.8 + MUSEUBO-A 整合 §4.9 + 設計原則整合 §4.10 | ✅ |
| (viii) | 想定改変 file 4 件 §3.6 (N11-15) A 明文化 | ✅ |
| (ix) | `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合 | ✅ |

---

## §6. PC-N-11 実装 phase Exit Criteria 10 項 ((N11-13) A 採用、= 別 session で充足)

| # | Criteria |
|---|----------|
| (i) | step (a) `AYAGltfMultiSkinEnabled` cvar 新設 (settings.xml) + Comment 内容 ((N11-11) B = 単独説明のみ) |
| (ii) | step (b) `LLCachedControl<bool> sAyastormGltfMultiSkinEnabled` 宣言 (recordGltfAssetDraw 関数内) |
| (iii) | step (c) PC-N-8 (f) writeSkinUbo + flushSkinUbos site の sCurrentSkin guard 追加 + cvar guard 追加 + wireSkinUboSetV3aToBinding2 per-draw 呼出追加 + tag block 新規 `<AYAstorm r41 PC-N-11 (a)>` ((N11-3)..(N11-8) 採用) |
| (iv) | step (d) PC-N-11 (a) first-fire LL_INFOS marker 追加 ((N11-9) A) |
| (v) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6 不変、PC-N-10 commit `cccd411486` 同数) |
| (vi) | MUSEUBO-A 整合 = `AYAGltfMultiSkinEnabled=false` default で PC-N-11 (a) real Skin path 不発火 + sentinel fall-through path 維持 + OpenGL 描画 100% 維持 |
| (vii) | build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 ((N11-12) A) |
| (viii) | cross-platform spec §6 PC-N-11 行 ✅ 反映 + §A 履歴 1 行追記 |
| (ix) | handoff complete doc 起案 = (a)-(g) 全実施 record + Exit Criteria 10 項全充足 + self-verify 9 観点 全 ✅ |
| (x) | feedback 全件遵守 record (= `feedback_handoff_minimal_pre_req_read` + `feedback_self_verify_before_handoff` + `feedback_build_only_verified` + `feedback_no_scope_shrink` + `feedback_doubt_self_first` + `feedback_confirm_referent_before_acting` + `feedback_ubo_migration_one_at_a_time` + `feedback_release_branch_workflow` + `feedback_no_auto_commit` + `feedback_no_claude_coauthor` + `feedback_no_bare_reference_ids` + `feedback_tests_dir_never_commit`) |

---

## §7. 着手手順 (= 次 session で PC-N-11 実装 phase 着手)

1. AYA 指示「PC-N-11 実装着手お願いします」literal 受領待ち
2. 本 PC-N-11 design-lock doc 全文 Read (= 必読 1 件 §1.1)
3. pinpoint reference 12 件 §1.2 中、改変対象 site (= 1, 2, 3, 4, 12) を Read で literal 再確認 (= `LLVKLoader::wireSkinUboSetV3aToBinding2` の可視性確認含む = anonymous namespace 内か public か)
4. step (a)-(g) 7 step を順次実施 (= settings.xml cvar 1 件追加 → llvkloader.cpp LLCachedControl 宣言 + PC-N-11 (a) tag block 追加 + sCurrentSkin guard + wireSkinUboSetV3aToBinding2 per-draw + first-fire marker)
5. build verify literal 取得 ((N11-12) A scope)
6. self-verify 9 観点 (= Exit Criteria 10 項 + 必読 + step + ambiguity 採用 + GATE-B + MUSEUBO-A + build verify + commit 内容 + feedback 遵守)
7. cross-platform spec §6 PC-N-11 行 ✅ 反映 + §A 履歴 1 行追記
8. handoff complete doc 起案 = `handoff-substep-...-phase1-e-pc-n-11-complete.md`
9. AYA literal「commit してください」受領後 commit (= `feedback_no_auto_commit` 遵守、feature branch `feature/ayastorm-r41-gl-removal` 上 = `feedback_release_branch_workflow` 遵守、Co-Authored-By 行不在 = `feedback_no_claude_coauthor` 遵守)
10. 以後 PC-N-12 design-lock 着手 (= real node modelview push constant 配線 = identity → real Node modelview 切替 design-lock)

---

## §8. 残 strict 線形

Phase 1.D complete ✅ (= 1 GLTF asset 完全 Vulkan draw 通電 達成) + Phase 1.E decomposition design-lock ✅ + **PC-N-11 design-lock ✅ 本 commit** + PC-N-11 実装 ⏳ 次 session + PC-N-12 design-lock + 実装 ⏳ + PC-N-13 design-lock + 実装 ⏳ + PC-N-14 design-lock ⏳ + PC-N-15 design-lock + 実装 + cleanup + Phase 1.E complete marker ⏳ = Phase 1.E complete ⏳ (= 設計原則 (2) Core プロセス分散実現達成) + Phase 1 全完了 ⏳ + Mac/Win 開発者補完 phase ⏳

---

## §9. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α..PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1..PC-N-4 ✅ = Phase 1.C complete ✅ + PC-8 Linux primary marker ✅ + PC-N-5 ✅ + Phase 1.D decomposition design-lock ✅ + PC-N-6 ✅ + PC-N-7 ✅ + PC-N-8 ✅ + PC-N-9 ✅ + PC-N-10 ✅ = **Phase 1.D complete marker ✅** = 1 GLTF asset 完全 Vulkan draw 通電 達成 ✅ + Phase 1.E decomposition design-lock ✅ + **PC-N-11 design-lock ✅ 本 commit** + PC-N-11 実装 ⏳ + PC-N-12 ⏳ + PC-N-13 ⏳ + PC-N-14 ⏳ + PC-N-15 ⏳ = Phase 1.E complete ⏳ = 設計原則 (2) Core プロセス分散実現達成 ⏳ + Phase 1 全完了 ⏳ + Mac/Win 開発者補完 phase ⏳

---

## §10. self-verify 9 観点 全 ✅

1. **PC-N-11 literal scope §0 完全分解 4 項** = sGltfStubSkin sentinel 段階卒業 + real Skin owner 切替 (sCurrentSkin 経由) + Skin_GLTFJoints UBO 実 data write (upstream uploadMatrixPalette 既配線済) + AYAGltfMultiSkinEnabled cvar 新設 (= AYA task statement literal 直訳) ✅
2. **必読 1 件 §1.1 + pinpoint reference 12 件 §1.2 別記** = Phase 1.E decomposition design-lock doc + recordGltfAssetDraw PC-N-8 (f) block + writeSkinUbo + flushSkinUbos site + bindV3aRigged + setCurrentSkin accessor + gltfscenemanager.cpp:765 + writeSkinUbo signature + flushSkinUbos + wireSkinUboSetV3aToBinding2 + registerSkinUbo + Skin::uploadMatrixPalette + Asset::update + settings.xml AYAGltfRealDrawEnabled = full file dump なし (feedback_handoff_minimal_pre_req_read 整合) ✅
3. **現状調査 §2 10 項網羅** = PC-N-8 (f) 改変対象 site 4 項 (writeSkinUbo + flushSkinUbos site / bindV3aRigged / push constant identity / zero-buffer PerDrawUBO_LightParams) + 上流既配線 PC-7γ-3 + PC-N-9 6 項 (uploadMatrixPalette lazy register / dual-write / Skin dtor unregister / Asset::update per-frame loop / setCurrentSkin caller / AYAGltfRealDrawEnabled entry hook) + §2.3 descriptor binding stale 化 risk + §2.4 (E-12) doc body vs commit msg vs AYA task statement resolve ✅
4. **ambiguity (N11-1)..(N11-16) 16 件 AYA literal「全件推奨で OK」record (2026-06-05) §3 + 採用根拠 16 件明文化** (特に (N11-1) (E-12) B vs commit msg (E-6) A vs AYA task statement literal の正式 resolve + (N11-4) A inline writeSkinUbo 不要根拠 = upstream uploadMatrixPalette PC-7γ-3 (j) 既配線確認 + (N11-5) A wireSkinUboSetV3aToBinding2 per-draw 必要根拠 = §2.3 stale 化 risk) ✅
5. **実装計画 (a)-(g) 7 step §4 + 各 step 具体 code diff example 添付** = settings.xml XML diff + llvkloader.cpp PC-N-11 (a) tag block C++ diff + first-fire marker C++ diff + cross-platform spec §6 行更新 + build verify command + handoff complete doc 起案 ✅
6. **GATE-B 整合 §4.8** = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar 新設は runtime gate のみ、count llvkloader.cpp=6 不変想定 ✅
7. **MUSEUBO-A 整合 §4.9** = `AYAGltfMultiSkinEnabled=false` default で PC-N-11 (a) real Skin path 不発火 + sentinel fall-through path 維持 + Phase 1.D PC-N-10 complete baseline 不変 + OpenGL 描画 100% 維持 ✅
8. **設計原則整合 §4.10** = (1) Upstream OpenGL 取り込みやすさ維持 = Skin::uploadMatrixPalette OpenGL path 完全温存 + dual-write defensive hook 不変 + recordGltfAssetDraw signature 不変 + GLTFSceneManager::render 改変 0 件 + shader 改変ゼロ + (2) Core プロセス分散実現 = per-Primitive scope sCurrentSkin accessor 経由 = primitive-level granularity の worker thread 分散余地確保 ✅
9. **`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件** = `feedback_design_phase_no_code_write` 整合 ✅

---

## §11. 次 session 着手 1 line

**PC-N-11 実装着手** = step (a) settings.xml `AYAGltfMultiSkinEnabled` cvar 1 件追加 + (b) llvkloader.cpp LLCachedControl 宣言 + (c) PC-N-8 (f) writeSkinUbo + flushSkinUbos site の sCurrentSkin guard + cvar guard + wireSkinUboSetV3aToBinding2 per-draw + tag block 新規 `<AYAstorm r41 PC-N-11 (a)>` + (d) first-fire LL_INFOS marker + (e) cross-platform spec §6 PC-N-11 行 ✅ 反映 + §A 履歴 + (f) build verify literal 取得 + (g) handoff complete doc 起案 → AYA commit 指示後 commit。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 PC-N-11 design-lock handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 次 session 必読 1 件 §1.1 + pinpoint reference 12 件 §1.2 別記、本 session も Read pinpoint のみ (= Phase 1.E decomposition design-lock doc 全文 + recordGltfAssetDraw PC-N-8 (f) block + writeSkinUbo + flushSkinUbos site + setCurrentSkin accessor + gltfscenemanager.cpp:765 setCurrentSkin caller + Skin::uploadMatrixPalette PC-7γ-3 (j) + Asset::update per-frame loop + wireSkinUboSetV3aToBinding2 + registerSkinUbo + cross-platform spec §6 PC-N-11 行 stub + Phase 1.D PC-N-10 design-lock doc template)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §10
- **feedback_build_only_verified** 遵守 = design-lock phase は `indra/` 改変 0 件で build verify 対象外、実装 phase で literal 検証取得予定 ((N11-12) A 採用)
- **feedback_no_scope_shrink** 遵守 = PC-N-11 literal scope §0 完全分解 4 項 = AYA task statement literal が source of truth、(N11-4) A inline writeSkinUbo 不要は upstream uploadMatrixPalette PC-7γ-3 (j) dual-write 既配線確認結果 = 縮小ではなく既配線資産活用、(N11-3) A sGltfStubSkin sentinel fall-through 維持は (E-11) A storage 撤去 PC-N-15 cleanup phase 持越し pattern 整合 = 縮小ではなく段階卒業、`feedback_ubo_migration_one_at_a_time` 厳格遵守整合
- **feedback_doubt_self_first** 遵守 = design-lock phase で ambiguity 16 件発見 + 推奨案提示 + AYA literal「全件推奨で OK」record 後本 design-lock doc 起案、特に (N11-1) cvar 戦略 doc 本文 (E-12) B vs commit msg (E-6) A vs AYA task statement literal 不整合 を Read で literal 確認後正式 resolve + (N11-4) inline writeSkinUbo 要否は Skin::uploadMatrixPalette PC-7γ-3 (j) dual-write 既配線を Read で literal 確認後 A 採用 + (N11-5) wireSkinUboSetV3aToBinding2 per-draw 必要根拠は §2.3 descriptor binding stale 化 risk を registerSkinUbo line 5460-5507 + wireSkinUboSetV3aToBinding2 line 2828-2886 + initVulkan PC-N-5 (c) line 3940-3990 の Read で literal 確認後判断、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = 16 件 batch AYA 確認 (2026-06-05)、各候補 + 推奨案 + 根拠明示後 AYA literal 一括「全件推奨で OK」record 受領で確定、推測実装なし、特に (N11-1) (E-12) B vs commit msg (E-6) A vs AYA task statement literal 不整合 は 3 source literal 並記後 AYA 採用判断、(N11-3) A 段階卒業 vs (N11-3) B 一括撤去 は (E-6) B (E-11) A literal 整合判断、(N11-5) A per-draw rewire vs (N11-5) B init-time のみ は §2.3 stale 化 risk 根拠明示後 AYA 採用判断
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N-11 = multi-skin sentinel 段階卒業単独 sub-step、Skin handling のみ scope 内、push constant (PC-N-12) / per-draw light params (PC-N-13) / worker thread (PC-N-14/15) は別 sub-step に分離、本 doc 起案も PC-N-11 単独 design-lock のみ
- **feedback_design_phase_no_code_write** 厳格遵守 = 本 PC-N-11 design-lock phase は doc 起案のみ、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示「commit してください」literal 受領まで commit せず
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在
- **feedback_no_bare_reference_ids** 遵守 = (N11-1)..(N11-16) 各 ID に項目名 / 採用案内容併記 §3 + (a)..(g) 各 step に作業内容併記 §4 + (E-1)..(E-16) 親 doc 参照時も項目名併記
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、git add 個別 file 指定予定
- **memory `project_ayastorm_r41_design_principles`** 整合 = (1) Upstream OpenGL 取り込みやすさ維持 = `Skin::uploadMatrixPalette` OpenGL path 完全温存 + dual-write defensive hook PC-7γ-3 (j) 不変 + `recordGltfAssetDraw` signature 不変 ((N8-6) A 維持) + `GLTFSceneManager::render` 改変 0 件 + shader 改変ゼロ §4.10 + (2) Core プロセス分散実現 = per-Primitive scope `sCurrentSkin` accessor 経由 Skin owner 切替 = primitive-level granularity の worker thread 分散余地確保 = PC-N-14/15 worker thread design-lock + 実装の baseline §4.10
- **memory `project_r41_phase1b_vulkan_host_gate`** 整合 = GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar 新設 1 件 (`AYAGltfMultiSkinEnabled`) は runtime gate のみ、count llvkloader.cpp=6 不変維持 §4.8
- **memory `project_ayastorm_three_platforms`** 整合 = cross-platform spec §6 PC-N-11 行更新で macOS / Windows 派生 fix 候補欄起案 = host-side path swap + cvar guard + wireSkinUboSetV3aToBinding2 per-draw rewire は host-side change ゆえ OS 非依存 + descriptor set 数 5 維持 + MoltenVK 影響増なし + Windows full Vulkan ゆえ派生 fix 候補なし、Linux primary 完成 → 他者補完 model 整合

---
