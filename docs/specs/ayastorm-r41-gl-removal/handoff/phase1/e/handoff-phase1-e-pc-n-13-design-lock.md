# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.E **PC-N-13 design-lock complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: Phase 1.E 内 **3rd sub-step = PC-N-13 = real per-draw light params + multi-asset verify** (= zero-buffer `PerDrawUBO_LightParams` 卒業 + `AYAGltfRealLightParamsEnabled` cvar 新設 + `AYAGltfMultiAssetCanary` debug-only cvar 新設 + 複数 GLTF asset 同時 draw 動作検証) の design-lock phase 完了 marker = ambiguity (N13-1)..(N13-16) 16 件 全 AYA literal「全件推奨で OK」record (2026-06-05) + 実装計画 (a)-(g) 7 step 分解 + Exit Criteria 9+10 項明文化。`indra/` 改変 0 件 (= `feedback_design_phase_no_code_write` 整合)。

> **本 doc 位置付け**: PC-N-13 詳細 design-lock。Phase 1.E decomposition design-lock (= `handoff-...-phase1-e-decomposition-design-lock.md`) + PC-N-11 design-lock + PC-N-11 実装 + PC-N-12 design-lock + PC-N-12 実装 (commit `b6b39bfd9f`) baseline 上に、PC-N-13 単独の **詳細実装計画** + **想定 code diff example** + **ambiguity 16 件 (N13-1)..(N13-16)** + **Exit Criteria 10 項** を確定。PC-N-11/PC-N-12 design-lock doc と同形 pattern 踏襲。実装は別 session で別途着手 (= `feedback_ubo_migration_one_at_a_time` 厳格遵守)。
>
> **⭐ 重大 finding (= (N13-1) C 採用根拠)**: `sGltfStubAssetPipeline` が消費する sky_smoke shader (`sSkySmokeVertModule`/`sSkySmokeFragModule`) は `PerDrawUBO_LightParams` を **shader 側で非 consume**。`PerDrawUBO_LightParams` 実消費 shader = `pointLightF.glsl` / `spotLightF.glsl` / `multiPointLightF.glsl` / `deferredUtil.glsl` / `starsV.glsl` (= 全て post-deferred lighting で GLTF asset draw とは無関係)。すなわち PC-N-8 (f) real Asset path での `writeDrawUbo` 256 B host write は **set=2 binding=0 descriptor layout 充足のみが目的** で、shader による consume は構造的に発生しない (= `llvkloader.cpp:5843-5845` literal comment「shader 未参照でも GPU error なし」既明示)。本 (N13-1) C 採用 = **zero IS real data 解釈** = sky_smoke pipeline 流用 architectural truth 尊重 + cvar gate 明示化 + first-fire marker で「現 phase で zero が real」を log 記録 + Phase 1.F+ 実 PBR shader 接続時に再着手。別 UBO 切替は scope 拡張で `feedback_ubo_migration_one_at_a_time` 違反 risk ゆえ回避。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「r41 Phase 1.E PC-N-13 design-lock 着手お願いします。直前 commit = `b6b39bfd9f` (PC-N-12 complete = Phase 1.E 内 2nd sub-step 実装完了 = real node modelview 通電、Option A layering-safe pointer accessor approach 採用)。PC-N-13 scope = real per-draw + multi-asset verify = zero-buffer `PerDrawUBO_LightParams` 卒業 (= 実 data 通電) + `AYAGltfMultiAssetCanary` cvar (debug-only) で複数 asset 同時描画検証 + `AYAGltfRealLightParamsEnabled` cvar 新設 (Boolean default=0 Persist=1、`AYAGltfRealModelviewEnabled` 直後並列 = Phase 1.E cvar group 連続配置)。必読 1 件: `handoff-...-phase1-e-pc-n-12-complete.md`。design-lock phase = `indra/` 改変 0 件、`feedback_design_phase_no_code_write` 厳格遵守で ambiguity 出し + 採用案提示 + AYA 確認 → design-lock doc 起案 + cross-platform spec §6 PC-N-13 行更新。GATE-B: `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (count llvkloader.cpp=6 維持)。MUSEUBO-A: 2 cvar default OFF + OpenGL 描画 100% 維持 + 5 段 graceful degrade 内部維持。`feedback_ubo_migration_one_at_a_time` 厳格遵守 (PC-N-14/PC-N-15 は別 session)。`feedback_self_verify_before_handoff` 遵守。」literal 受領 (2026-06-05、PC-N-12 complete commit `b6b39bfd9f` 後の継続 session = 別 session の fresh context)。

**PC-N-13 literal scope** (= AYA task statement 直訳、4 項):

1. **zero-buffer `PerDrawUBO_LightParams` 卒業** = `recordGltfAssetDraw` PC-N-8 (f) 内 line 5984-5995 の `writeDrawUbo(PerDrawUBO_LightParams, zero_buf, 256)` を `<AYAstorm r41 PC-N-13 (a)>` tag block で wrap + cvar guard + first-fire marker = **案 C semantic 解釈採用 = zero IS real data** ((N13-1) C 採用 = sky_smoke shader 非 consume architectural truth 尊重、Phase 1.F+ 実 PBR shader 接続時 data 内容置換)
2. **`AYAGltfRealLightParamsEnabled` cvar 新設** = Boolean default=0 Persist=1 ((N13-4) A 採用)、settings.xml `AYAGltfRealModelviewEnabled` 直後並列 = Phase 1.E cvar group 連続配置 ((N13-5) A 採用)
3. **`AYAGltfMultiAssetCanary` debug-only cvar 新設** = Boolean default=0 Persist=1 ((N13-3) A + (N13-4) A 採用)、settings.xml `AYAGltfRealLightParamsEnabled` 直後並列 = Phase 1.E cvar group 末尾 ((N13-6) A 採用)
4. **複数 GLTF asset 同時 draw 動作検証** = `<AYAstorm r41 PC-N-13 (b)>` tag block で file-static `std::unordered_set<const void*>` 経由 asset address 追跡 + `AYAGltfMultiAssetCanary` cvar=ON 時 2nd 以降 asset 検出時 LL_INFOS 1 回 ((N13-11) A) + 既存 SL inv の複数 GLTF asset を AYA が同時 rez し実機目視 + log 確認 ((N13-12) A)

**Phase 境界**: PC-N-13 完了 = Phase 1.E 内 3rd sub-step 完了 = zero-buffer `PerDrawUBO_LightParams` cvar gate 明示化 + multi-asset 動作確認 baseline 確立。worker thread design + 実装は PC-N-14/15 持越し、sentinel storage 撤去 + Phase 1.E complete marker は PC-N-15 持越し ((E-11) A + (E-14) A 整合)。実 data 通電 (= sky_smoke 非 sentinel-like 別 pipeline 接続) は Phase 1.F+ で実 PBR shader 統合時に再着手 (= scope 拡張回避、`feedback_ubo_migration_one_at_a_time` 厳格遵守)。

---

## §1. 必読 1 件 + pinpoint reference

### §1.1 必読 1 件 (= 次 session = PC-N-13 実装 phase 着手前)

1. **本 PC-N-13 design-lock doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-e-pc-n-13-design-lock.md`

### §1.2 pinpoint reference 12 件 (= 実装 phase で必要分のみ Read)

1. **`recordGltfAssetDraw` PC-N-8 (f) writeDrawUbo site**: `indra/llrender/llvkloader.cpp:5984-5995` = PC-N-13 (a) tag block で wrap 対象 (= `writeDrawUbo(PerDrawUBO_LightParams, /*offset=*/0u, real_asset_draw_zero_buf, 256, real_asset_dynamic_offset)`)
2. **`recordGltfAssetDraw` PC-N-8 (f) real Asset path block 全体**: `indra/llrender/llvkloader.cpp:5906-6147` = PC-N-13 (a) + (b) tag block 配置場所 = PC-N-11 (a) inner block (line 5997-6056) 並列 + PC-N-12 (a) inner block (line 6059-6120) 並列
3. **architectural truth comment**: `indra/llrender/llvkloader.cpp:5843-5845` = `PerDrawUBO_LightParams` literal「shader 未参照でも GPU error なし」明示 (= (N13-1) C 採用根拠)
4. **`sGltfStubAssetPipeline` sky_smoke shader bind**: `indra/llrender/llvkloader.cpp:3464-3582` = pipeline creation 経路 (= `sSkySmokeVertModule`/`sSkySmokeFragModule` consume site、sky_smoke shader は `PerDrawUBO_LightParams` 非 consume = (N13-1) C architectural truth)
5. **`PerDrawUBO_LightParams` shader 実 consume site**: `indra/newview/app_settings/shaders/class3/deferred/pointLightF.glsl` / `spotLightF.glsl` / `multiPointLightF.glsl` / `deferredUtil.glsl` / `starsV.glsl` = post-deferred lighting で GLTF asset draw とは無関係 (= `setCurrentAsset` 経路非接続、確認のみ用)
6. **`PerDrawUBO_LightParams` blueprint GLSL**: `indra/newview/app_settings/shaders/aya_r41_blueprints/set2/per_draw_ubo_light_params.glsl` = `vec3 spot_light_color + float spot_light_size` literal 16 B (= 256 B host write は descriptor layout 充足、shader 側 16 B 消費)
7. **`writeDrawUbo` signature**: `indra/llrender/llvkloader.cpp` 既 PC-N-1/2 配線 = `block_hash + offset + data + size + out dynamic_offset` (= host-side allocate + memcpy + dynamic_offset 返却、PerDrawUBO ring buffer)
8. **PC-N-11 (a) tag block 構造**: `indra/llrender/llvkloader.cpp:5997-6056` = surgical insertion pattern 参照 (= PC-N-13 (a) 同形 surgical insertion 配置)
9. **PC-N-12 (a) tag block 構造**: `indra/llrender/llvkloader.cpp:6059-6120` = surgical insertion pattern 参照 + first-fire marker pattern (= `s_first_pcn12_real_modelview_fire` atomic flag)
10. **`bindV3aRigged` per-Primitive 配線**: `indra/llrender/llvkloader.cpp:6057` = PC-N-8 (f) writeDrawUbo 直後 set=3 binding=2 = real Skin UBO bind site (PC-N-13 (a) wrap 後も同位置維持)
11. **`gltfscenemanager.cpp` 複数 Asset iteration source**: `indra/newview/gltfscenemanager.cpp:599` `GLTFSceneManager::render(U8 variant)` 内 `mObjects` loop (= 複数 GLTF asset 自然 iteration、PC-N-13 (b) multi-asset canary 発火経路源)
12. **settings.xml `AYAGltfRealModelviewEnabled` 配置**: `indra/newview/app_settings/settings.xml` PC-N-12 で追加済 cvar、PC-N-13 `AYAGltfRealLightParamsEnabled` cvar はこの直後並列追加 ((N13-5) A) + `AYAGltfMultiAssetCanary` はさらに直後並列追加 ((N13-6) A)

---

## §2. 現状調査結果 (= PC-N-12 complete baseline + 改変対象 site 確認)

### §2.1 PC-N-8 (f) 内 writeDrawUbo zero buffer site (= 改変対象本体)

| # | site | file:line | 現状 | PC-N-13 改変 |
|---|------|-----------|------|-------------|
| 1 | `real_asset_draw_zero_buf[256]` 配列 | `llvkloader.cpp:5984` | `static const U8 real_asset_draw_zero_buf[256] = {};` | `<AYAstorm r41 PC-N-13 (a)>` tag block 内に移動 + cvar guard 内に配置 + `(N13-1) C 採用 = zero IS real data` 整合 |
| 2 | `writeDrawUbo` invocation | `llvkloader.cpp:5986-5991` | `LLVKLoader::writeDrawUbo(PerDrawUBO_LightParams, /*offset=*/0u, real_asset_draw_zero_buf, 256, real_asset_dynamic_offset);` | tag block 内 unconditional 維持 (= dynamic_offset 構築は両 path 必須、ring buffer allocate 経路必須) + cvar gate は data 内容 vs first-fire marker 起動のみに作用 |
| 3 | `real_asset_dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 構築 | `llvkloader.cpp:5992-5995` | 4 binding 同 dynamic offset (= ε-2 A pattern 継承) | unchanged (= PC-N-13 改変対象外、PC-N-8 (f) 既配線温存) |

### §2.2 ⭐ critical finding: sky_smoke shader は `PerDrawUBO_LightParams` を consume しない

**(原理)**: `sGltfStubAssetPipeline` は `sSkySmokeVertModule` + `sSkySmokeFragModule` を bind (= PC-N-6 で確立、PC-N-8 (f) で再利用)。sky_smoke shader (= `indra/newview/app_settings/shaders/class3/deferred/skyV.glsl` 等) は atmospheric scattering 用ゆえ `PerDrawUBO_LightParams` (= `vec3 spot_light_color + float spot_light_size`) を **非 consume**。

**(実 consumer)**: `PerDrawUBO_LightParams` を実消費する shader = `pointLightF.glsl` (point light per-light params) / `spotLightF.glsl` (spot light per-light params) / `multiPointLightF.glsl` (multi point light batch) / `deferredUtil.glsl` (`#define color spot_light_color`/`#define size spot_light_size` で post-deferred lighting 配線) / `starsV.glsl` (star size)。全て post-deferred lighting で GLTF asset draw とは無関係 = `setCurrentAsset` 経路非接続。

**(host-side architectural truth)**: PC-N-8 (f) real Asset path の `writeDrawUbo` 256 B host write は **set=2 binding=0 descriptor layout 充足のみが目的** = `llvkloader.cpp:5843-5845` literal comment「placeholder PSO 用 real value 構築は別 sub-step、shader 未参照でも GPU error なし」と完全整合 = sky_smoke shader 使用前提では zero buffer が architectural truth = 別 UBO 切替は scope 拡張、real PBR shader 接続まで実 data 通電は構造的に発生不能。

**(PC-N-13 resolution = (N13-1) C 採用)**: **zero IS real data 解釈** = sky_smoke pipeline 流用 architectural truth 尊重 + cvar gate 明示化 (= `AYAGltfRealLightParamsEnabled` cvar で「現 phase は zero が real」と log 記録) + first-fire marker (= `s_first_pcn13_real_light_params_fire` atomic flag) で通電 literal 取得 + Phase 1.F+ 実 PBR shader 接続時に PC-N-13.1 等で data 内容置換着手。別 UBO 切替や別 shader 接続は scope 拡張で `feedback_ubo_migration_one_at_a_time` 違反 risk + (E-2) B「実 data 通電」thesis は cvar gate 明示化 + first-fire marker log で literal 充足。

### §2.3 caller-side multi-asset iteration

| # | site | line | 用途 |
|---|------|------|------|
| 4 | `GLTFSceneManager::render(U8 variant)` mObjects loop | `gltfscenemanager.cpp:599` | mObjects 内 複数 GLTF asset を 自動 iterate (= PC-N-13 multi-asset canary 発火経路源、自然 iteration、機能 cvar 不要) |
| 5 | `LLVKLoader::setCurrentAsset(&asset)` | `gltfscenemanager.cpp:697` | per-Asset Asset& setting (= PC-N-13 (b) tag block で `getCurrentAsset()` 経由 address 追跡可能) |

**(N13-11) A 採用根拠**: multi-asset iteration は upstream `GLTFSceneManager::render` mObjects loop 経由ですでに自動経路化済ゆえ機能 cvar (= 機能制限) は不要 + MUSEUBO-A 違反 risk 回避 = debug-only canary cvar で「複数 asset draw fire 確認」のみが PC-N-13 scope。

### §2.4 PC-N-8 (f) tag block 構造 (= 既配線)

| # | tag block | line | sub-step |
|---|-----------|------|----------|
| 6 | PC-N-8 (f) outer block | `llvkloader.cpp:5906-6147` | Phase 1.D 5th sub-step = real Asset path 確立 |
| 7 | PC-N-11 (a) inner block | `llvkloader.cpp:5997-6056` | Phase 1.E 1st sub-step = multi-skin real Skin path |
| 8 | PC-N-12 (a) inner block | `llvkloader.cpp:6059-6120` | Phase 1.E 2nd sub-step = real node modelview |
| 9 | **PC-N-13 (a) inner block (= 新規配置対象)** | `llvkloader.cpp:5984-5995` 上に新規 | Phase 1.E 3rd sub-step = writeDrawUbo zero buffer cvar wrap (= zero IS real data 解釈) |
| 10 | **PC-N-13 (b) inner block (= 新規配置対象)** | PC-N-13 (a) 隣接 (= 推奨位置: `bindV3aRigged` 直後 or PC-N-12 (a) 直後) | Phase 1.E 3rd sub-step = multi-asset canary marker |

### §2.5 (E-14) commit msg + AYA task statement の整合 ((N13-3) A 採用)

| source | multi-asset verify アプローチ |
|--------|-----------------------------|
| Phase 1.E decomposition design-lock commit `094546889b` commit message (E-14) | A: `AYAGltfMultiAssetCanary` cvar (debug-only) |
| AYA PC-N-13 task statement (2026-06-05) | literal: `AYAGltfMultiAssetCanary` cvar (debug-only) で複数 asset 同時描画検証 |
| **(N13-3) AYA 全件推奨採用結果** | **A**: debug-only Boolean cvar (= 機能影響なし、log 出力のみ) |

**採用根拠**: AYA task statement literal「debug-only」整合 + 機能制限は MUSEUBO-A 違反 risk + 既 multi-asset iteration は `GLTFSceneManager::render` mObjects loop 自動経路ゆえ機能 cvar 不要 = debug-only Boolean cvar で log 出力のみが PC-N-13 scope (= `feedback_no_scope_shrink` 整合、scope 正確化)。

---

## §3. ambiguity (N13-1)..(N13-16) 16 件 AYA literal「全件推奨で OK」record (2026-06-05) + 採用根拠

### §3.1 ⭐ critical resolve 系 (= (N13-1) C 採用)

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N13-1) | zero-buffer `PerDrawUBO_LightParams` 卒業 semantic 解釈 | **C**: **zero IS real data 解釈** = sky_smoke pipeline 流用 architectural truth 尊重 + cvar gate 明示化 + first-fire marker で「現 phase で zero が real」を log 記録 + Phase 1.F+ 実 PBR shader 接続時に PC-N-13.1 等で data 内容置換着手 | OK (2026-06-05) | sky_smoke shader が `PerDrawUBO_LightParams` 非 consume = §2.2 architectural truth、`llvkloader.cpp:5843-5845` literal comment「shader 未参照でも GPU error なし」既明示、別 UBO 切替は scope 拡張で `feedback_ubo_migration_one_at_a_time` 違反 risk、AYA scope literal「卒業」= cvar gate 明示化 + first-fire marker 通電 literal 取得で充足、Phase 1.F+ 実 PBR shader 接続時に再着手 |

### §3.2 cvar 戦略系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N13-2) | `AYAGltfRealLightParamsEnabled` cvar 命名 | **A**: AYA task statement literal `AYAGltfRealLightParamsEnabled` 採用 | OK (2026-06-05) | AYA task statement literal source of truth + 既 r41 cvar pattern (`AYAGltf*Enabled`) 整合 |
| (N13-3) | `AYAGltfMultiAssetCanary` cvar 性質 | **A**: debug-only Boolean cvar (= 機能影響なし、log 出力のみ、(E-14) A integration approach 整合) | OK (2026-06-05) | AYA task statement literal「debug-only」整合 + MUSEUBO-A 違反 risk 回避 + 既 multi-asset iteration は `GLTFSceneManager::render` mObjects loop 自動経路ゆえ機能 cvar 不要 = §2.5 |
| (N13-4) | 2 cvar default + Persist | **A**: Boolean default=0 Persist=1 (= 既存 PC-N-6/7/9/11/12 cvar default off pattern 厳格踏襲) | OK (2026-06-05) | r41 cvar pattern 踏襲、MUSEUBO-A 整合 (= default OFF で既 zero buffer 維持 + OpenGL 描画 100% 維持) |
| (N13-5) | settings.xml `AYAGltfRealLightParamsEnabled` 配置 | **A**: 既 `AYAGltfRealModelviewEnabled` cvar 直後並列 (= Phase 1.E cvar group 連続配置、(N11-10) A + (N12-4) A 整合) | OK (2026-06-05) | Phase 1.E cvar group 起点維持、PC-N-14/15 cvar もここに並べる想定 |
| (N13-6) | settings.xml `AYAGltfMultiAssetCanary` 配置 | **A**: `AYAGltfRealLightParamsEnabled` 直後並列 (= Phase 1.E cvar group 末尾追加) | OK (2026-06-05) | Phase 1.E cvar group 連続配置 + 2 cvar セット運用整合 (= PC-N-13 単独 sub-step 内 2 cvar、機能 vs canary 並列配置) |
| (N13-7) | cvar Comment 内容 | **A**: 各 cvar 単独説明 (= PC-N-11 (N11-11) B + PC-N-12 (N12-5) B 同形、untouched 領域 silence) | OK (2026-06-05) | 各 sub-step 別 session 別途 design-lock 原則 |

### §3.3 PC-N-8 (f) 改変系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N13-8) | tag block 命名 + 配置 (PC-N-13 (a)) | **A**: PC-N-8 (f) 内 writeDrawUbo site (= line 5984-5995) を新規 `<AYAstorm r41 PC-N-13 (a)>` tag block で wrap (= PC-N-11 (a)/PC-N-12 (a) 同形 surgical insertion、PC-N-8 (f) outer block 構造温存) | OK (2026-06-05) | PC-N-11/12 同形 pattern 踏襲、code archaeology 容易、各 sub-step 独立 tag block で git blame 解析整合 |
| (N13-9) | cvar guard 配置 | **A**: PC-N-13 (a) tag block 内に `LLCachedControl<bool> sAyastormGltfRealLightParamsEnabled` 配置 + cvar guard = real path 発火時のみ first-fire marker 起動 + data 内容引続 zero (案 C 採用時) + `writeDrawUbo` 自体は unconditional 呼出 (= dynamic_offset 構築は両 path 必須、ring buffer allocate 経路必須) | OK (2026-06-05) | (N13-1) C zero IS real data 解釈整合、cvar gate は marker 起動 only、data path は unchanged、graceful degrade 維持 |
| (N13-10) | first-fire marker | **A**: `s_first_pcn13_real_light_params_fire` atomic flag (= PC-N-6/7/8/9/10/11/12 同形 pattern) | OK (2026-06-05) | 通電 literal 取得用、PC-N-13 (a) real light params path first fire 時 1 回出力 + log 内容で「現 phase は zero IS real data = sky_smoke shader 非 consume」明示、AYA literal 受領用 |
| (N13-11) | multi-asset canary 発火位置 | **A**: PC-N-8 (f) real Asset path 内 新規 `<AYAstorm r41 PC-N-13 (b)>` tag block (= PC-N-13 (a) 隣接、推奨位置: PC-N-12 (a) 直後または `bindV3aRigged` 直後) で `AYAGltfMultiAssetCanary` cvar=ON + 2nd 以降 asset 検出時 LL_INFOS 1 回 (= file-static `std::unordered_set<const void*>` で seen asset 追跡、size>1 時 fire、debug-only ゆえ機能影響なし) | OK (2026-06-05) | upstream `GLTFSceneManager::render` mObjects loop 自然 iteration を信任 + per-asset address 追跡で multi-asset 動作 literal 取得 + std::unordered_set は file-static main thread 専有ゆえ mutex 不要 |

### §3.4 検証 + 失敗時 escalation 系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N13-12) | multi-asset 動作確認手順 | **A**: 既存 SL inv の複数 GLTF asset を AYA が同時 rez し実機目視 + log で `AYAGltfMultiAssetCanary` ON 時 multi-asset canary fire 確認 ((E-14) A integration approach 整合、real SL sample 信任、synthetic 不要) | OK (2026-06-05) | (E-14) A integration approach 整合、AYA 実機 with 複数 GLTF rez で自然発火、synthetic 不要 |
| (N13-13) | 失敗時 escalation 経路 | **A**: 設計段階では成功想定 + 失敗時 PC-N-13 内 fix or 別 sub-step (= PC-N-13.1) 起案 = AYA 判断明示、解析段階では推測不可 | OK (2026-06-05) | upstream `mObjects` loop 既動作で multi-asset 自然 iterate ゆえ成功想定が design-lock default、失敗時の具体 fix path は実機現象見て初めて判断可能 |
| (N13-14) | build verify scope | **A**: llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 + GATE-B integrity `LL_VULKAN_GLSL count llvkloader.cpp=6` 不変 (= PC-N-12 commit `b6b39bfd9f` 同数想定) | OK (2026-06-05) | (E-8) commit msg / (E-16) doc body 整合、既 PC-N-6/7/8/9/10/11/12 同形 build verify pattern 踏襲 |

### §3.5 Exit + 改変規模 + design-lock 整合 系

| # | 項目 | 採用案 | AYA 確認 | 採用根拠 |
|---|------|--------|---------|---------|
| (N13-15) | Exit Criteria 項目数 | **A**: 10 項 (= PC-N-6/7/8/9/10/11/12 同形、実装 phase 整合) | OK (2026-06-05) | PC-N-13 = 実装系 sub-step ゆえ実装 phase Exit Criteria 10 項 pattern 踏襲 |
| (N13-16) | 想定改変 file 件数 (実装 phase) | **A**: 4 file = (1) `indra/llrender/llvkloader.cpp` (= PC-N-8 (f) 内 writeDrawUbo site を `<AYAstorm r41 PC-N-13 (a)>` tag block で wrap + `<AYAstorm r41 PC-N-13 (b)>` multi-asset canary marker 配置 + `LLCachedControl<bool>` 2 cvar 宣言 + first-fire marker + file-static `std::unordered_set<const void*>` for asset address tracking) + (2) `indra/newview/app_settings/settings.xml` (= `AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` 2 cvar 追加) + (3) 本 cross-platform spec §6 PC-N-13 行 状態 ✅ 反映 + §A 履歴 1 行追記 + (4) new handoff complete doc 起案、design-lock phase `indra/` 改変 0 件 = `feedback_design_phase_no_code_write` 厳格遵守 | OK (2026-06-05) | PC-N-12 (6 file) より −2 (header + caller 改変なし) は構造的必然 (= PC-N-13 は既 PC-N-8 (f) writeDrawUbo site wrap + multi-asset canary marker のみ、新規 accessor 不要 + caller-side 配線不要 = PC-N-11 同形 4 file pattern 踏襲)、`feedback_no_scope_shrink` 整合 |

---

## §4. 実装計画 (a)-(g) 7 step (= 別 session で着手)

> **注**: 本 §4 は **実装 phase 用 step 分解 + 想定 code diff example** = 本 design-lock phase は `indra/` 改変 0 件、実装は別 session で別途着手 (= `feedback_design_phase_no_code_write` + `feedback_ubo_migration_one_at_a_time` 厳格遵守)。

### §4.1 step (a) — `AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` cvar 新設 (settings.xml)

`indra/newview/app_settings/settings.xml` の既 `AYAGltfRealModelviewEnabled` cvar 直後並列 ((N13-5) A) で `AYAGltfRealLightParamsEnabled` cvar 追加、その直後並列 ((N13-6) A) で `AYAGltfMultiAssetCanary` cvar 追加。Boolean default=0 Persist=1 ((N13-4) A)。Comment は各 cvar 単独説明 ((N13-7) A)。

**想定 XML diff example**:

```xml
<key>AYAGltfRealLightParamsEnabled</key>
<map>
    <key>Comment</key>
    <string>
        AYAstorm r41 Phase 1.E PC-N-13 = real per-draw light params cvar gate
        (= recordGltfAssetDraw PC-N-8 (f) real Asset path 内 writeDrawUbo
        PerDrawUBO_LightParams zero buffer 256 B 投入 site を cvar gate で wrap、
        第 3 placeholder-like 段階卒業 marker)。
        OFF (default) = 既 PC-N-8 (f) zero buffer 256 B writeDrawUbo path 維持
        (= Phase 1.D + PC-N-11 + PC-N-12 baseline 不変)。
        ON = first-fire marker 起動 + 「zero IS real data」semantic 記録 =
        sGltfStubAssetPipeline 流用 sky_smoke shader が PerDrawUBO_LightParams
        を非 consume = 現 phase で zero buffer が architectural truth (=
        descriptor set layout 充足のみが目的、shader 側 GPU error なし)。
        Phase 1.F+ 実 PBR shader 接続時に data 内容置換着手予定 (PC-N-13.1)。
        Prerequisite: AYAGltfRealDrawEnabled=1 (= recordGltfAssetDraw fire entry)。
    </string>
    <key>Persist</key>
    <integer>1</integer>
    <key>Type</key>
    <string>Boolean</string>
    <key>Value</key>
    <integer>0</integer>
</map>
<key>AYAGltfMultiAssetCanary</key>
<map>
    <key>Comment</key>
    <string>
        AYAstorm r41 Phase 1.E PC-N-13 = multi-asset GLTF draw canary (debug-only)。
        OFF (default) = log 出力なし、機能影響ゼロ。
        ON = recordGltfAssetDraw PC-N-8 (f) real Asset path で 2nd 以降の
        異なる Asset address を検出した時に LL_INFOS で 1 回 log 出力 (=
        既存 SL inv の複数 GLTF asset を同時 rez 時に発火、GLTFSceneManager::render
        mObjects loop の自然 iteration を信任した debug-only canary)。
        機能制限なし = MUSEUBO-A 整合、log のみで multi-asset draw fire literal を
        AYA 実機検証時に取得可能。
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

### §4.2 step (b) — `LLCachedControl<bool>` 2 cvar 宣言 + file-static `std::unordered_set` 追加 (llvkloader.cpp)

`recordGltfAssetDraw` 内 PC-N-8 (f) 内 PC-N-12 (a) inner block 直後並列に `LLCachedControl<bool>` 2 件配置 ((N13-9) A、PC-N-12 (d) 同形 pattern)。file-static `std::unordered_set<const void*>` は anonymous namespace 内に配置 (= main thread 専有ゆえ mutex 不要)。

**想定 C++ diff example** (`LLCachedControl<bool>`):

```cpp
static LLCachedControl<bool> sAyastormGltfRealLightParamsEnabled(
    gSavedSettings, "AYAGltfRealLightParamsEnabled", false);
static LLCachedControl<bool> sAyastormGltfMultiAssetCanary(
    gSavedSettings, "AYAGltfMultiAssetCanary", false);
```

**想定 C++ diff example** (file-static `std::unordered_set`、anonymous namespace 内):

```cpp
// <AYAstorm r41 PC-N-13 (b)> multi-asset canary 用 seen asset address tracker
//   ((N13-11) A、AYA literal「全件推奨で OK」record 2026-06-05)。
//   main thread 専有 (recordGltfAssetDraw は GLTFSceneManager::render から呼出)
//   ゆえ mutex 不要。debug-only ゆえ AYAGltfMultiAssetCanary cvar=ON 時のみ
//   insert + size>1 で fire (= file-static = process lifetime、frame 跨ぎで蓄積)。
std::unordered_set<const void*> sPcn13MultiAssetSeen;
// </AYAstorm r41 PC-N-13 (b)>
```

### §4.3 step (c) — PC-N-8 (f) writeDrawUbo site の `<AYAstorm r41 PC-N-13 (a)>` tag wrap + cvar guard + first-fire marker (llvkloader.cpp)

`recordGltfAssetDraw` PC-N-8 (f) 内 writeDrawUbo site (= line 5984-5995) を新規 `<AYAstorm r41 PC-N-13 (a)>` tag block で wrap ((N13-8) A、surgical insertion)。cvar guard = real path 発火時のみ first-fire marker 起動、data 内容引続 zero ((N13-9) A = zero IS real data 解釈)、`writeDrawUbo` 自体は unconditional 呼出 (= dynamic_offset 構築必須)。

**想定 C++ diff example** (= line 5984-5995 改変):

```cpp
// <AYAstorm r41 PC-N-13 (a)> real per-draw light params cvar gate ((N13-1) C 採用
//   = zero IS real data 解釈、AYA literal「全件推奨で OK」record 2026-06-05)。
//   sGltfStubAssetPipeline 流用 sky_smoke shader (sSkySmokeVertModule/sSkySmokeFragModule)
//   は PerDrawUBO_LightParams を非 consume = 現 phase で zero buffer が architectural
//   truth (= descriptor set layout 充足のみが目的、shader 側 GPU error なし、
//   llvkloader.cpp:5843-5845 既明示)。AYAGltfRealLightParamsEnabled cvar=true 時
//   first-fire marker 起動 + log で「現 phase は zero IS real data = sky_smoke shader
//   非 consume」明示。Phase 1.F+ 実 PBR shader 接続時に data 内容置換 (PC-N-13.1)。
//   writeDrawUbo 自体は unconditional 呼出 (= dynamic_offset 構築は両 path 必須、
//   ring buffer allocate 経路必須、cvar gate は data 内容 vs marker 起動のみ作用)。
static const U8 real_asset_draw_zero_buf[256] = {};
U32 real_asset_dynamic_offset = 0u;
LLVKLoader::writeDrawUbo(
    ubo::block_hash::PerDrawUBO_LightParams,
    /*offset=*/0u,
    real_asset_draw_zero_buf,
    sizeof(real_asset_draw_zero_buf),
    real_asset_dynamic_offset);

// PC-N-13 (a) first-fire LL_INFOS marker ((N13-10) A、PC-N-6/7/8/9/10/11/12 同形 pattern)。
if (sAyastormGltfRealLightParamsEnabled)
{
    static std::atomic<bool> s_first_pcn13_real_light_params_fire{true};
    if (s_first_pcn13_real_light_params_fire.exchange(false, std::memory_order_acq_rel))
    {
        LL_INFOS("Vulkan") << "PC-N-13 (a) real per-draw light params cvar gate 通電 (first fire): "
                              "AYAGltfRealLightParamsEnabled=true; "
                              "現 phase は zero IS real data 解釈 ((N13-1) C 採用) = "
                              "sGltfStubAssetPipeline 流用 sky_smoke shader "
                              "(sSkySmokeVertModule/sSkySmokeFragModule) は "
                              "PerDrawUBO_LightParams を非 consume = "
                              "host write 256 B zero buffer が descriptor set layout 充足 "
                              "architectural truth (llvkloader.cpp:5843-5845 既明示)。"
                              "Phase 1.F+ 実 PBR shader 接続時に PC-N-13.1 等で data 内容置換着手予定。"
                           << LL_ENDL;
    }
}

const U32 real_asset_dynamic_offsets[V3A_DRAW_SET_BINDINGS] = {
    real_asset_dynamic_offset, real_asset_dynamic_offset,
    real_asset_dynamic_offset, real_asset_dynamic_offset,
};
// </AYAstorm r41 PC-N-13 (a)>
```

### §4.4 step (d) — PC-N-13 (b) multi-asset canary marker 配置 (llvkloader.cpp)

PC-N-8 (f) real Asset path 内に新規 `<AYAstorm r41 PC-N-13 (b)>` tag block 配置 ((N13-11) A、推奨位置: PC-N-12 (a) inner block 直後または `bindV3aRigged` 直後)。`AYAGltfMultiAssetCanary` cvar=ON 時 file-static `std::unordered_set<const void*>` に asset address insert + size>1 時 LL_INFOS 1 回出力。

**想定 C++ diff example**:

```cpp
// <AYAstorm r41 PC-N-13 (b)> multi-asset GLTF draw canary marker ((N13-11) A、
//   AYA literal「全件推奨で OK」record 2026-06-05)。AYAGltfMultiAssetCanary cvar=ON 時
//   PC-N-8 (f) real Asset path で異なる Asset address を検出した時に LL_INFOS で
//   log 出力 (= 既存 SL inv の複数 GLTF asset を同時 rez 時に発火、GLTFSceneManager::render
//   mObjects loop の自然 iteration を信任した debug-only canary、機能影響ゼロ =
//   MUSEUBO-A 整合)。file-static std::unordered_set<const void*> で seen asset
//   address 追跡 (= sPcn13MultiAssetSeen、main thread 専有ゆえ mutex 不要)。
if (sAyastormGltfMultiAssetCanary)
{
    sPcn13MultiAssetSeen.insert(static_cast<const void*>(asset));
    if (sPcn13MultiAssetSeen.size() > 1u)
    {
        static std::atomic<bool> s_first_pcn13_multi_asset_canary_fire{true};
        if (s_first_pcn13_multi_asset_canary_fire.exchange(false, std::memory_order_acq_rel))
        {
            LL_INFOS("Vulkan") << "PC-N-13 (b) multi-asset GLTF draw canary 発火 (first fire): "
                                  "seen_asset_count=" << sPcn13MultiAssetSeen.size()
                               << ", current_asset=" << static_cast<const void*>(asset)
                               << "; GLTFSceneManager::render mObjects loop で複数 GLTF asset "
                                  "を同時 iterate 検証 PASS ((N13-11)/(N13-12) A integration "
                                  "approach 整合、real SL sample 信任、synthetic 不要)。"
                                  "AYAGltfMultiAssetCanary debug-only canary ゆえ機能影響ゼロ。"
                               << LL_ENDL;
        }
    }
}
// </AYAstorm r41 PC-N-13 (b)>
```

### §4.5 step (e) — build verify literal 取得

PC-N-11/12 同形 ((N13-14) A):
1. `make -j4 llrender` → `[100%] Built target llrender` (= ERROR 0 / WARNING 0)
2. `INTEGRATION_TEST_lluboringbuffer` → 11/11 PASS
3. `INTEGRATION_TEST_llassetubopool` → 10/10 PASS
4. `INTEGRATION_TEST_llpipelinecachestorage` → 13/13 PASS
5. `python3 -m unittest discover tests` (codegen) → 131/131 OK
6. `grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp` → 6 (= PC-N-12 commit `b6b39bfd9f` 同数、GATE-B integrity 維持)

### §4.6 step (f) — cross-platform spec §6 PC-N-13 行 ✅ 反映 + §A 履歴追記

`docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md`:
- §6 PC-N-13 行 状態 ⏳ → ✅ 反映 (= 実装 complete content)
- §A 履歴 1 行追記 (= chronological order: PC-N-13 design-lock entry → PC-N-13 ✅ 反映 entry 順)

### §4.7 step (g) — handoff complete doc 起案

`handoff-substep-...-phase1-e-pc-n-13-complete.md` 起案 = step (a)-(g) 全実施 record + Exit Criteria 10 項全充足 + build verify literal 取得 + GATE-B integrity record + self-verify 9 観点 全 ✅ + 引き継ぎ memory + 次 session 着手 1 line (= PC-N-14 design-lock 着手 = worker thread design)。AYA literal 「commit してください」受領後 commit (`feedback_no_auto_commit` + `feedback_release_branch_workflow` + `feedback_no_claude_coauthor` 遵守)。

### §4.8 GATE-B 整合

- `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count `llvkloader.cpp=6` 不変、PC-N-12 commit `b6b39bfd9f` 同数想定)
- `AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` cvar runtime gate のみで marker 起動 vs canary log 制御
- shader 改変 0 件 (= 既 sky_smoke shader pipeline + zero buffer 256 B host write の architectural truth 維持、shader 側は cvar 状態を知らない)
- memory `project_r41_phase1b_vulkan_host_gate` 整合

### §4.9 MUSEUBO-A 整合

- `AYAGltfRealLightParamsEnabled=false` + `AYAGltfMultiAssetCanary=false` default で既 PC-N-8 (f) zero buffer 256 B writeDrawUbo + 既 multi-asset iteration log なし path 維持 = PC-N-12 baseline + Phase 1.D complete baseline 不変
- OpenGL 描画 100% 維持 (= 本 PC-N-13 改変は Vulkan path `recordGltfAssetDraw` 内のみ、OpenGL path 無関係)
- 5 段 graceful degrade 維持 (= cvar gate は marker 起動 only、data path は unchanged、`writeDrawUbo` unconditional 呼出維持)

### §4.10 設計原則整合

- **(1) Upstream OpenGL 取り込みやすさ維持**:
  - `recordGltfAssetDraw` signature 不変維持
  - sky_smoke shader pipeline (= `sGltfStubAssetPipeline`) 既配線活用 = `PerDrawUBO_LightParams` 非 consume architectural truth 尊重 = upstream 互換性最大化
  - shader 改変ゼロ = `PerDrawUBO_LightParams` 256 B host layout 不変
  - `GLTFSceneManager::render` 改変なし = caller-side 配線不要 ((N13-16) A 採用根拠)
- **(2) Core プロセス分散実現**:
  - cvar gate は per-Primitive granularity 維持 (= each PC-N-8 (f) fire で cvar check)
  - file-static `std::unordered_set<const void*>` は main thread 専有ゆえ thread-safe (= PC-N-14/15 worker thread 分散 design 着手時に必要なら lock-free 構造に置換可能)

---

## §5. PC-N-13 design-lock Exit Criteria (= 9 項全充足、本 commit)

| # | criterion | status |
|---|-----------|--------|
| i | PC-N-13 literal scope §0 完全分解 4 件 (= AYA task statement literal 4 件直訳、(N13-1) C で「zero IS real data」semantic 正式 resolve = §2.2 architectural truth) | ✅ |
| ii | 必読 1 件 §1.1 (本 PC-N-13 design-lock doc) + pinpoint reference 12 件 §1.2 別記 = full file dump なし (= `feedback_handoff_minimal_pre_req_read` 整合) | ✅ |
| iii | ambiguity (N13-1)..(N13-16) 16 件 + AYA literal「全件推奨で OK」record (2026-06-05) §3 | ✅ |
| iv | 採用根拠 16 件明文化 §3 (特に (N13-1) ⭐ critical C「zero IS real data」採用根拠 + (N13-3) A debug-only Boolean cvar の根拠 + (N13-11) A multi-asset canary 発火位置 + (N13-13) A 失敗時 escalation 経路の trade-off 明示) | ✅ |
| v | 実装計画 (a)-(g) 7 step 分解 §4 + 各 step 具体 code stub example 添付 | ✅ |
| vi | 実装 phase Exit Criteria 10 項明文化 (§6) | ✅ |
| vii | GATE-B 整合 §4.8 + MUSEUBO-A 整合 §4.9 + 設計原則整合 §4.10 | ✅ |
| viii | 想定改変 file 4 件明文化 (= PC-N-12 6 件より −2 (header + caller 改変なし)、§3.5 (N13-16) A 採用根拠明示) | ✅ |
| ix | `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合 | ✅ |

---

## §6. PC-N-13 実装 phase Exit Criteria (= 10 項、別 session)

| # | criterion |
|---|-----------|
| i | settings.xml `AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` 2 Boolean cvar 追加 (default=0 Persist=1、`AYAGltfRealModelviewEnabled` 直後並列 + `AYAGltfRealLightParamsEnabled` 直後並列、Comment 各単独説明) ((N13-2)/(N13-3)/(N13-4)/(N13-5)/(N13-6)/(N13-7) A) |
| ii | `llvkloader.cpp` `recordGltfAssetDraw` 内 PC-N-12 (a) inner block 直後並列に `LLCachedControl<bool>` 2 件配置 + anonymous namespace 内 file-static `std::unordered_set<const void*> sPcn13MultiAssetSeen` 配置 ((N13-9)/(N13-11) A) |
| iii | `recordGltfAssetDraw` PC-N-8 (f) 内 writeDrawUbo site (line 5984-5995) を `<AYAstorm r41 PC-N-13 (a)>` tag block で wrap + cvar guard + first-fire marker + data 内容引続 zero (= zero IS real data 解釈) + `writeDrawUbo` unconditional 呼出維持 ((N13-1) C + (N13-8) A + (N13-9) A) |
| iv | PC-N-8 (f) real Asset path 内 `<AYAstorm r41 PC-N-13 (b)>` tag block で multi-asset canary marker 配置 (= `AYAGltfMultiAssetCanary` cvar=ON + 2nd 以降 asset 検出時 LL_INFOS 1 回) ((N13-11) A) |
| v | PC-N-13 (a) first-fire `LL_INFOS` marker (`s_first_pcn13_real_light_params_fire` atomic flag、PC-N-6/7/8/9/10/11/12 同形 pattern、log 内容で「現 phase は zero IS real data = sky_smoke shader 非 consume」明示) ((N13-10) A) |
| vi | PC-N-13 (b) first-fire `LL_INFOS` marker (`s_first_pcn13_multi_asset_canary_fire` atomic flag、`sPcn13MultiAssetSeen.size() > 1` 時 fire) ((N13-11) A) |
| vii | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6 不変、PC-N-12 commit `b6b39bfd9f` 同数想定) |
| viii | MUSEUBO-A 整合 = 2 cvar default OFF で既 zero buffer + 既 multi-asset iteration log なし path 維持 + OpenGL 描画 100% 維持 + 5 段 graceful degrade 内部維持 |
| ix | build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 ((N13-14) A) + cross-platform spec §6 PC-N-13 行 ✅ 反映 + §A 履歴 1 行追記 + handoff complete doc 起案 |
| x | self-verify 9 観点 全 ✅ + AYA literal commit 指示受領後 commit (= `feedback_no_auto_commit` + `feedback_release_branch_workflow` + `feedback_no_claude_coauthor` 遵守) |

---

## §7. 次 session 着手手順 (= PC-N-13 実装 phase)

1. 必読 1 件 (本 PC-N-13 design-lock doc) を Read
2. §1.2 pinpoint reference 12 件のうち実装必要分のみ Read (= full file dump なし、`feedback_handoff_minimal_pre_req_read` 整合)
3. step (a) settings.xml `AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` 2 cvar 追加
4. step (b) `llvkloader.cpp` `LLCachedControl<bool>` 2 件 + file-static `std::unordered_set` 配置
5. step (c) PC-N-8 (f) writeDrawUbo site を `<AYAstorm r41 PC-N-13 (a)>` tag wrap + cvar guard + first-fire marker
6. step (d) PC-N-8 (f) real Asset path 内 `<AYAstorm r41 PC-N-13 (b)>` tag block で multi-asset canary marker 配置
7. step (e) build verify literal 取得 (= llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 + GATE-B integrity LL_VULKAN_GLSL count llvkloader.cpp=6 不変)
8. step (f) cross-platform spec §6 PC-N-13 行 ✅ 反映 + §A 履歴 1 行追記
9. step (g) handoff complete doc 起案 + Exit Criteria 10 項 self-verify + AYA literal commit 指示受領後 commit
10. AYA 実機検証 = 既存 SL inv の複数 GLTF asset を AYA 同時 rez し log 確認 (= `AYAGltfRealLightParamsEnabled=true` で PC-N-13 (a) first-fire marker + `AYAGltfMultiAssetCanary=true` で PC-N-13 (b) multi-asset canary first-fire marker 両方発火確認、(N13-12) A integration approach 整合)
11. 以後 PC-N-14 design-lock 着手 (= worker thread design = per-Primitive UBO write + cmdbuf record 並列化 design)

---

## §8. 残 strict 線形

- ✅ Phase 1.A / 1.B / (Z) SSS / (W) uniform4iv / (Y) Phase 1.C prep
- ✅ PC-0..PC-7ε / PC-N decomposition / PC-N-1..PC-N-4 (= Phase 1.C complete)
- ✅ PC-8 Linux primary marker (= Phase 1.C strict 線形終了)
- ✅ PC-N-5 (= Phase 1.D 着手起点) / Phase 1.D decomposition design-lock
- ✅ PC-N-6 / PC-N-7 / PC-N-8 / PC-N-9 / PC-N-10 (= Phase 1.D complete = 1 GLTF asset 完全 Vulkan draw 通電 達成)
- ✅ Phase 1.E decomposition design-lock (commit `094546889b`)
- ✅ PC-N-11 design-lock (commit `87560a4dc7`) + PC-N-11 実装 (commit `797332ee81` = Phase 1.E 内 1st sub-step 実装完了 = multi-skin real Skin path 通電)
- ✅ PC-N-12 design-lock (commit `9a62f11416`) + PC-N-12 実装 (commit `b6b39bfd9f` = Phase 1.E 内 2nd sub-step 実装完了 = real node modelview 通電 + 案 A layering-safe pointer accessor approach 採用)
- ✅ **PC-N-13 design-lock ✅ 本 commit = Phase 1.E 内 3rd sub-step design-lock 完了**
- ⏳ PC-N-13 実装 (= 別 session、step (a)-(g) 7 step 実施 + Exit Criteria 10 項 self-verify + handoff complete doc 起案)
- ⏳ PC-N-14 design-lock (= worker thread design = per-Primitive UBO write + cmdbuf record 並列化 design)
- ⏳ PC-N-15 実装 + cleanup (= worker thread 実装 + `sGltfStubSkin` sentinel storage 撤去 + Phase 1.E complete marker)
- ⏳ Phase 1.E complete = 実 data 通電 + multi-asset / multi-skin / worker thread 分散達成 → Phase 1 全完了 → Mac/Win 開発者補完 phase

---

## §9. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α..PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1..PC-N-4 ✅ = Phase 1.C complete ✅ + PC-8 Linux primary marker ✅ + PC-N-5 ✅ + Phase 1.D decomposition design-lock ✅ + PC-N-6 ✅ + PC-N-7 ✅ + PC-N-8 ✅ + PC-N-9 ✅ + PC-N-10 ✅ = Phase 1.D complete ✅ (= 1 GLTF asset 完全 Vulkan draw 通電 達成) + Phase 1.E decomposition design-lock ✅ + PC-N-11 design-lock ✅ + PC-N-11 ✅ (= Phase 1.E 内 1st sub-step 実装完了 = multi-skin real Skin path 通電) + PC-N-12 design-lock ✅ + PC-N-12 ✅ (= Phase 1.E 内 2nd sub-step 実装完了 = real node modelview 通電 + 案 A layering-safe pointer accessor approach 採用) + **PC-N-13 design-lock ✅ 本 commit** + PC-N-13 実装 ⏳ 次 session + PC-N-14 design-lock ⏳ + PC-N-15 設計 + 実装 + cleanup ⏳ + Phase 1.E complete ⏳ + Phase 1 全完了 ⏳ + Mac/Win 開発者補完 phase ⏳

---

## §10. self-verify 9 観点 全 ✅

1. ✅ PC-N-13 literal scope §0 完全分解 4 件 (= AYA task statement literal が source of truth、(N13-1) C で「zero IS real data」semantic 正式 resolve = §2.2 architectural truth、sky_smoke shader 非 consume が PerDrawUBO_LightParams の host 256 B write を descriptor layout 充足のみに限定する構造的事実を明示)
2. ✅ 必読 1 件 §1.1 + pinpoint reference 12 件 §1.2 別記 = full file dump なし (= `feedback_handoff_minimal_pre_req_read` 整合)
3. ✅ 現状調査 §2 完全分解 (= writeDrawUbo zero buffer site + ⭐ critical sky_smoke 非 consume architectural truth + multi-asset iteration source + PC-N-8 (f) tag block 構造 + (E-14) commit msg vs AYA task statement 整合 resolve)
4. ✅ ambiguity (N13-1)..(N13-16) 16 件 AYA literal「全件推奨で OK」record (2026-06-05) §3 + 採用根拠 16 件明文化 (特に (N13-1) ⭐ critical C「zero IS real data」採用根拠 + (N13-3) A debug-only Boolean cvar の根拠 = MUSEUBO-A + (N13-11) A multi-asset canary 発火位置 = std::unordered_set + (N13-13) A 失敗時 escalation 経路の trade-off 明示)
5. ✅ 実装計画 (a)-(g) 7 step §4 + 各 step 具体 code stub example 添付
6. ✅ GATE-B 整合 §4.8 (= `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、`AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` cvar runtime gate のみ、shader 改変ゼロ)
7. ✅ MUSEUBO-A 整合 §4.9 (= 2 cvar default OFF で既 zero buffer + 既 multi-asset iteration log なし path 維持 + OpenGL 描画 100% 維持 + 5 段 graceful degrade 内部維持)
8. ✅ 設計原則整合 §4.10 (= (1) Upstream OpenGL 取り込みやすさ維持 = `recordGltfAssetDraw` signature 不変 + sky_smoke shader pipeline 既配線活用 + shader 改変ゼロ + `GLTFSceneManager::render` 改変なし + (2) Core プロセス分散実現 = cvar gate per-Primitive granularity 維持 + file-static `std::unordered_set` main thread 専有、PC-N-14/15 worker thread 分散 design 着手時に必要なら lock-free 構造に置換可能)
9. ✅ `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合

---

## §11. 次 session 着手 1 line

PC-N-13 実装着手 = step (a)-(g) 7 step 実施 = (a) settings.xml `AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` 2 cvar 追加 (`AYAGltfRealModelviewEnabled` 直後並列 + `AYAGltfRealLightParamsEnabled` 直後並列、Boolean default=0 Persist=1、各 cvar 単独説明) + (b) `llvkloader.cpp` PC-N-12 (a) inner block 直後並列に `LLCachedControl<bool>` 2 件配置 + anonymous namespace 内 file-static `std::unordered_set<const void*> sPcn13MultiAssetSeen` 配置 + (c) PC-N-8 (f) 内 writeDrawUbo site (line 5984-5995) を `<AYAstorm r41 PC-N-13 (a)>` tag block で wrap + cvar guard + first-fire `LL_INFOS` marker (`s_first_pcn13_real_light_params_fire` atomic flag、log 内容で「現 phase は zero IS real data = sky_smoke shader 非 consume」明示) + `writeDrawUbo` unconditional 呼出維持 + (d) PC-N-8 (f) real Asset path 内 `<AYAstorm r41 PC-N-13 (b)>` tag block で multi-asset canary marker 配置 (`AYAGltfMultiAssetCanary` cvar=ON + asset address insert + size>1 時 first-fire `LL_INFOS` marker) + (e) build verify literal 取得 (= llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 + GATE-B integrity LL_VULKAN_GLSL count llvkloader.cpp=6 不変) + (f) cross-platform spec §6 PC-N-13 行 ✅ 反映 + §A 履歴 1 行追記 + (g) handoff complete doc 起案 + Exit Criteria 10 項 self-verify + AYA literal commit 指示受領後 commit + AYA 実機検証 = 既存 SL inv 複数 GLTF asset 同時 rez で 2 marker 発火確認

---

## §A. feedback 遵守 record

- ✅ `feedback_proactive_handoff` (本 PC-N-13 design-lock doc 起案)
- ✅ `feedback_handoff_minimal_pre_req_read` (必読 1 件 + pinpoint reference 12 件別記、本 session も Read pinpoint のみ = PC-N-12 complete doc 全文 + `recordGltfAssetDraw` PC-N-8 (f) writeDrawUbo site + `PerDrawUBO_LightParams` blueprint GLSL + sky_smoke shader bind 経路 + architectural truth comment + PC-N-11 (a) / PC-N-12 (a) tag block 構造 + `GLTFSceneManager::render` mObjects loop + settings.xml `AYAGltfRealModelviewEnabled` 配置、full file dump なし)
- ✅ `feedback_self_verify_before_handoff` (9 観点 self-verify 全 ✅)
- ✅ `feedback_build_only_verified` (design-lock phase は `indra/` 改変 0 件で build verify 対象外、実装 phase で literal 検証取得予定 (N13-14) A 採用)
- ✅ `feedback_no_scope_shrink` (PC-N-13 literal scope §0 完全分解 4 件 = AYA task statement literal が source of truth、(N13-1) C「zero IS real data」採用は sky_smoke shader 非 consume architectural truth に基づく semantic 正式 resolve = §2.2 = scope 縮小ではなく構造的事実の明示化、別 UBO 切替や別 shader 接続は scope 拡張で `feedback_ubo_migration_one_at_a_time` 違反 risk ゆえ Phase 1.F+ 持越し、(N13-16) A 4 file 改変は PC-N-12 6 file より −2 (header + caller 改変なし) = 構造的必然 (PC-N-13 は writeDrawUbo site wrap + canary marker のみ、新規 accessor 不要)、`feedback_ubo_migration_one_at_a_time` 厳格遵守整合)
- ✅ `feedback_doubt_self_first` (design-lock phase で ambiguity 16 件発見 + 推奨案提示 + AYA literal「全件推奨で OK」record 後本 design-lock doc 起案、特に (N13-1) ⭐ critical「卒業」semantic は sky_smoke shader が `PerDrawUBO_LightParams` を非 consume という構造的事実を Grep/Read で literal 確認後発見 = §2.2、`llvkloader.cpp:5843-5845` architectural truth comment を Read で literal 確認後 (N13-1) C「zero IS real data」semantic 正式採用、推測実装なし)
- ✅ `feedback_confirm_referent_before_acting` (16 件 batch AYA 確認 (2026-06-05)、各候補 + 推奨案 + 根拠明示後 AYA literal 一括「全件推奨で OK」record 受領で確定、推測実装なし、特に (N13-1) ⭐ critical「卒業」semantic は sky_smoke 非 consume architectural truth + 別 UBO 切替の scope 拡張 risk を全件提示後採用)
- ✅ `feedback_ubo_migration_one_at_a_time` 厳格遵守 (PC-N-13 = real per-draw light params cvar gate + multi-asset canary 単独 sub-step = zero IS real data 解釈 + `AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` 2 cvar 新設、PC-N-14 (worker thread design) + PC-N-15 (worker thread 実装 + cleanup) は分離 = 各 sub-step 別 session で別途 design-lock + 実装、本 doc 起案も PC-N-13 単独 design-lock のみ、別 UBO 切替・別 shader 接続は Phase 1.F+ 持越し)
- ✅ `feedback_design_phase_no_code_write` 厳格遵守 (本 PC-N-13 design-lock phase は doc 起案 + cross-platform spec §6 行更新のみ、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件)
- ✅ `feedback_release_branch_workflow` (feature branch `feature/ayastorm-r41-gl-removal` 上 commit 想定)
- ✅ `feedback_no_auto_commit` (AYA 明示 commit 指示受領後 commit 予定)
- ✅ `feedback_no_claude_coauthor` (Co-Authored-By 行不在予定)
- ✅ `feedback_no_bare_reference_ids` ((N13-1)..(N13-16) 各 ID に項目名 / 採用案内容併記 + (a)..(g) 各 step に作業内容併記)
- ✅ `feedback_tests_dir_never_commit` 整合 (tests/ 改変 0 件、git add 個別 file 指定予定)
- ✅ `feedback_admit_unknown` 整合 ((N13-1) ⭐ critical で「sky_smoke shader が PerDrawUBO_LightParams を consume するかどうか」が不明な状態で推測しなかった = Grep + Read で literal 確認後 architectural truth 採用、推測実装なし + (N13-13) A 失敗時 escalation 経路で「設計段階では成功想定 + 失敗時 PC-N-13 内 fix or 別 sub-step (PC-N-13.1) 起案 = AYA 判断明示、解析段階では推測不可」と明示)
- ✅ memory `project_ayastorm_r41_design_principles` 整合 ((1) Upstream OpenGL 取り込みやすさ維持 = `recordGltfAssetDraw` signature 不変 + sky_smoke shader pipeline 既配線活用 (= `PerDrawUBO_LightParams` 非 consume architectural truth 尊重) + `GLTFSceneManager::render` 改変なし + shader 改変ゼロ + (2) Core プロセス分散実現 = cvar gate per-Primitive granularity 維持 + file-static `std::unordered_set` main thread 専有、PC-N-14/15 worker thread 分散 design 着手時に必要なら lock-free 構造に置換可能)
- ✅ memory `project_r41_phase1b_vulkan_host_gate` 整合 (GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、2 cvar runtime gate のみ、count=6 不変想定)
- ✅ memory `project_ayastorm_three_platforms` 整合 (cross-platform spec §6 PC-N-13 行更新で macOS / Windows 派生 fix 候補欄起案 = host-side `writeDrawUbo` zero data + cvar gate 配置 + multi-asset canary host-side `std::unordered_set` は OS 非依存 + `PerDrawUBO_LightParams` UBO bind 経路 PC-N-1/2 既配線済 (= 256 B set=2 binding=0 ring buffer + dynamic offset、MoltenVK 標準対応範囲) + descriptor set 数 5 維持 + `AYAGltfRealLightParamsEnabled` + `AYAGltfMultiAssetCanary` cvar XML は OS 非依存 ゆえ macOS 派生 fix 候補なし想定 + Windows full Vulkan ゆえ派生 fix 候補なし想定、Linux primary 完成 → 他者補完 model 整合)
