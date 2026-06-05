# Handoff: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.E PC-N-11 complete

**Status**: ✅ **PC-N-11 complete = Phase 1.E 内 1st sub-step 実装完了 = multi-skin
sentinel 段階卒業 + real Skin owner 切替 + Skin_GLTFJoints UBO 実 data write +
AYAGltfMultiSkinEnabled cvar 新設**

**Date**: 2026-06-05
**Branch**: `feature/ayastorm-r41-gl-removal`
**Previous commit**: `87560a4dc7` (PC-N-11 design-lock complete)

---

## §0. PC-N-11 literal scope 実装結果 (= 全件採用案通り実装完了)

PC-N-11 = **Phase 1.E 内 1st sub-step = multi-skin = sGltfStubSkin sentinel
段階卒業 + real Skin owner 切替 + Skin_GLTFJoints UBO 実 data write +
AYAGltfMultiSkinEnabled cvar 新設**。

literal scope 4 件全件実装完了 ((N11-1)..(N11-16) 16 件 AYA literal「全件推奨で OK」
record (2026-06-05) 全件採用案通り):

1. ✅ `sGltfStubSkin` sentinel 段階卒業 = `sCurrentSkin != nullptr` 時 real Skin
   path / null or cvar=false 時 fall-through to `sGltfStubSkin` sentinel ((N11-3)
   A、sentinel storage 撤去は PC-N-15 cleanup phase 持越し = (E-11) A 整合)
2. ✅ real Skin owner 切替 = `sCurrentSkin` 経由 (= `gltfscenemanager.cpp:765`
   PC-7γ-2 既配線 `setCurrentSkin` 資産活用、本 sub-step caller-side 改変 0 件)
3. ✅ Skin_GLTFJoints UBO 実 data write = `Skin::uploadMatrixPalette` PC-7γ-3 (j)
   upstream dual-write 既配線資産活用 ((N11-4) A、`recordGltfAssetDraw` 側 inline
   `writeSkinUbo` 不要 = 再書込冗長排除)
4. ✅ `AYAGltfMultiSkinEnabled` cvar 新設 = Boolean default=0 Persist=1
   ((N11-1) A + (N11-2) A、settings.xml `AYAGltfRealDrawEnabled` 直後並列 =
   Phase 1.E cvar group 起点 ((N11-10) A))

---

## §1. 実装結果 = 7 step (a)-(g) 全実装

### §1.1 step (a) settings.xml `AYAGltfMultiSkinEnabled` cvar 追加 ((N11-1)/(N11-2)/(N11-10) A)

`indra/newview/app_settings/settings.xml`:

- 配置: `AYAGltfRealDrawEnabled` 直後並列 (Phase 1.E cvar group 起点、
  PC-N-12/13/14/15 cvar もここに並べる想定 ((N11-10) A))
- 仕様: `Type=Boolean` + `Value=0` (= default OFF) + `Persist=1`
- Comment: 本 cvar 単独説明 ((N11-11) B 採用 = 将来 cvar は当該 sub-step 着手時に
  追記 = 各 sub-step 別 session 別途 design-lock 原則、untouched 領域 silence)
- 機能変化: `AYAGltfMultiSkinEnabled=true` かつ `sCurrentSkin != nullptr` 時に
  real Skin owner path 通電、それ以外は既 sentinel fall-through path 維持 =
  MUSEUBO-A 整合

### §1.2 step (b) `recordGltfAssetDraw` 内 `LLCachedControl<bool>` 配置

`indra/llrender/llvkloader.cpp` `recordGltfAssetDraw` PC-N-8 (f) block 内
writeSkinUbo + flushSkinUbos site:

```cpp
static LLCachedControl<bool> sAyastormGltfMultiSkinEnabled(
    gSavedSettings, "AYAGltfMultiSkinEnabled", false);
```

- 配置: PC-N-8 (f) `<AYAstorm r41 PC-N-8 (f)>` outer tag block 内側、新規
  `<AYAstorm r41 PC-N-11 (a)>` inner tag block 冒頭 ((N11-7) A surgical insertion)
- pattern: PC-N-10 (a) entry hook の `LLCachedControl<bool> sAyastormGltfRealDrawEnabled`
  と同形

### §1.3 step (c) PC-N-8 (f) writeSkinUbo + flushSkinUbos site を PC-N-11 (a) tag block で wrap ((N11-7)/(N11-8) A)

`indra/llrender/llvkloader.cpp` line 5956-6015 (= 旧 PC-N-8 (f) 内 writeSkinUbo +
flushSkinUbos site = line 5956-5975 を新規 `<AYAstorm r41 PC-N-11 (a)>` tag block
で wrap):

- outer `<AYAstorm r41 PC-N-8 (f)>` tag block 構造温存 (surgical insertion)
- inner `<AYAstorm r41 PC-N-11 (a)>` tag block 配置 = PC-N-9/PC-N-10 同形 pattern
- cvar guard 配置 = Skin handling 部分のみ guard 内 ((N11-8) A) = push constant
  / vertex buffer bind / index buffer bind / draw call は cvar 外 (= PC-N-12/13
  で別途 guard 化、各 sub-step independent A/B 維持)

### §1.4 step (d) cvar guard + `sCurrentSkin` null guard + fall-through 分岐 ((N11-3) A)

```cpp
LL::GLTF::Skin* const skin_to_use =
    (sAyastormGltfMultiSkinEnabled && sCurrentSkin != nullptr)
        ? sCurrentSkin
        : sGltfStubSkin;
if (skin_to_use == sGltfStubSkin)
{
    // fall-through sentinel path = 既 identity 64 B writeSkinUbo 維持
    // (PC-N-15 cleanup まで storage 温存 = (E-11) A 整合)
    static const F32 real_asset_identity_skin_buf[64] = { /* identity 4x4 */ };
    LLVKLoader::writeSkinUbo(
        skin_to_use, ubo::block_hash::Skin_GLTFJoints,
        /*offset=*/0u,
        reinterpret_cast<const U8*>(real_asset_identity_skin_buf),
        sizeof(real_asset_identity_skin_buf));
}
```

- cvar=false or `sCurrentSkin==nullptr` 時 fall-through to `sGltfStubSkin` =
  non-rigged primitive 等の natural guard、graceful degrade 維持
- real Skin path inline `writeSkinUbo` 不要 ((N11-4) A) = upstream
  `Skin::uploadMatrixPalette` PC-7γ-3 (j) dual-write で既書込済の real bone
  matrix palette を消費 = 再書込冗長排除、upstream 既配線資産活用

### §1.5 step (e) `wireSkinUboSetV3aToBinding2(skin_to_use)` per-draw rewire + `flushSkinUbos(skin_to_use)` unconditional 呼出 ((N11-5)/(N11-6) A)

```cpp
wireSkinUboSetV3aToBinding2(skin_to_use);
LLVKLoader::flushSkinUbos(skin_to_use);
```

- `wireSkinUboSetV3aToBinding2` per-draw rewire ((N11-5) A): `registerSkinUbo`
  1st-register-only descriptor wire pattern に対する multi-skin stale 化 risk
  (= frame 内 2nd Skin register で binding=2 上書き → 1st Skin binding stale 化)
  を per-draw `vkUpdateDescriptorSets` 呼出で吸収。per-Skin descriptor set 別建ては
  Phase 1.F 候補温存。
- `flushSkinUbos` unconditional 呼出 ((N11-6) A): real Skin / sGltfStubSkin
  問わず dirty exchange 両 path 共通必須、code 簡素化

### §1.6 step (f) first-fire `LL_INFOS` marker ((N11-9) A)

```cpp
if (skin_to_use != sGltfStubSkin)
{
    static std::atomic<bool> s_first_pcn11_real_skin_fire{true};
    if (s_first_pcn11_real_skin_fire.exchange(false, std::memory_order_acq_rel))
    {
        LL_INFOS("Vulkan") << "PC-N-11 (a) multi-skin real Skin path 通電 (first fire): "
                              "skin=" << (void*)skin_to_use
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

- PC-N-6/7/8/9/10 同形 pattern: `std::atomic<bool>` + `exchange` で single-fire
- diagnostic 内容: real Skin addr + sentinel addr + 通電経路サマリで通電 literal
  AYA 受領用

### §1.7 step (g) build verify + handoff complete doc 起案 ((N11-12) A)

§2 + 本 doc 参照。

---

## §2. build verify literal 取得

| # | check | result |
|---|-------|--------|
| 1 | `make -j4 llrender` | ✅ `[100%] Built target llrender` (= ERROR 0 / WARNING 0) |
| 2 | `INTEGRATION_TEST_lluboringbuffer` | ✅ 11/11 PASS YAY!! |
| 3 | `INTEGRATION_TEST_llassetubopool` | ✅ 10/10 PASS YAY!! |
| 4 | `INTEGRATION_TEST_llpipelinecachestorage` | ✅ 13/13 PASS YAY!! |
| 5 | `python3 -m unittest discover tests` (codegen) | ✅ 131/131 OK |
| 6 | `grep -c LL_VULKAN_GLSL indra/llrender/llvkloader.cpp` | ✅ 6 (= PC-N-10 commit `16a26f6272` 同数、GATE-B integrity 維持) |

---

## §3. PC-N-11 実装 phase Exit Criteria (= 10 項全充足)

| # | criterion | status |
|---|-----------|--------|
| i | `recordGltfAssetDraw` PC-N-8 (f) 内 writeSkinUbo + flushSkinUbos site を新規 `<AYAstorm r41 PC-N-11 (a)>` tag block で wrap + cvar guard + `sCurrentSkin` null guard + real Skin / sentinel fall-through 分岐 ((N11-3)/(N11-7)/(N11-8) A) | ✅ |
| ii | `wireSkinUboSetV3aToBinding2(skin_to_use)` per-draw rewire + `flushSkinUbos(skin_to_use)` unconditional 呼出 + real Skin path inline `writeSkinUbo` 不要 (upstream `Skin::uploadMatrixPalette` PC-7γ-3 (j) dual-write 既配線資産活用) ((N11-4)/(N11-5)/(N11-6) A) | ✅ |
| iii | settings.xml `AYAGltfMultiSkinEnabled` Boolean cvar 1 件追加 (default=0 Persist=1、`AYAGltfRealDrawEnabled` 直後並列 = Phase 1.E cvar group 起点、Comment 単独説明) ((N11-1)/(N11-2)/(N11-10)/(N11-11) A/B) | ✅ |
| iv | PC-N-11 (a) first-fire `LL_INFOS` marker (`s_first_pcn11_real_skin_fire` atomic flag、PC-N-6/7/8/9/10 同形 pattern) ((N11-9) A) | ✅ |
| v | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6 不変、PC-N-10 commit `16a26f6272` 同数) | ✅ |
| vi | MUSEUBO-A 整合 = `AYAGltfMultiSkinEnabled=false` default で sentinel fall-through 経路維持 + OpenGL 描画 100% 維持 + PC-N-8 (f) 5 段 graceful degrade 内部維持 | ✅ |
| vii | build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11+10+13 + codegen 131/131 ((N11-12) A) | ✅ |
| viii | cross-platform spec §6 PC-N-11 行 ✅ 反映 + §A 履歴 1 行追記 ((N11-15) A 採用) | ✅ |
| ix | handoff complete doc 起案 ((N11-15) A 採用) | ✅ |
| x | self-verify 9 観点 全 ✅ | ✅ |

---

## §4. 改変 file 4 件

| # | file | 改変概要 | 実 LoC |
|---|------|---------|--------|
| 1 | `indra/llrender/llvkloader.cpp` | step (b)-(f) = PC-N-8 (f) writeSkinUbo + flushSkinUbos site を `<AYAstorm r41 PC-N-11 (a)>` tag block で wrap + `LLCachedControl<bool> sAyastormGltfMultiSkinEnabled` 配置 + `sCurrentSkin` null guard + real Skin / sentinel fall-through 分岐 + `wireSkinUboSetV3aToBinding2(skin_to_use)` per-draw rewire + `flushSkinUbos(skin_to_use)` unconditional + sentinel path のみ inline `writeSkinUbo` + first-fire `LL_INFOS` marker | +60/-19 (net +41) |
| 2 | `indra/newview/app_settings/settings.xml` | step (a) `AYAGltfMultiSkinEnabled` Boolean cvar 1 件追加 (`AYAGltfRealDrawEnabled` 直後並列、default=0 Persist=1) | +30/-0 |
| 3 | `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` | §6 PC-N-11 行 状態 ✅ 反映 + §A 履歴 1 行追記 | +2/-1 |
| 4 | (本 file) `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-...-phase1-e-pc-n-11-complete.md` | step (g) 新規 PC-N-11 complete handoff doc 起案 | new |

`llvkloader.h` 改変 0 件 (= 新規 API 追加 0 件、既 `setCurrentSkin`/
`getCurrentSkin`/`wireSkinUboSetV3aToBinding2`/`flushSkinUbos`/`writeSkinUbo`
全て既宣言・既配線済 ゆえ header 改変不要)。shader 改変 0 件、codegen 改変 0 件、
CMake 改変 0 件、tests/ 改変 0 件。

---

## §5. PC-N-11 達成事項 = multi-skin real Skin path 通電

### §5.1 通電経路 (= `AYAGltfMultiSkinEnabled=true` + `sCurrentSkin != nullptr` 時)

1. **upstream owner 切替** (PC-7γ-2 既配線): `GLTFSceneManager::render` per-Primitive
   loop body 内 `setCurrentSkin(&skin)` (= `gltfscenemanager.cpp:765`) で
   `sCurrentSkin` に real `LL::GLTF::Skin*` 設定
2. **upstream UBO write** (PC-7γ-3 (j) 既配線): `Skin::uploadMatrixPalette` 内
   dual-write hook で `writeSkinUbo` 経由 Skin_GLTFJoints UBO ring buffer に
   real bone matrix palette を書込
3. **per-draw descriptor rewire** (本 sub-step 新設、(N11-5) A): `recordGltfAssetDraw`
   PC-N-11 (a) block で `wireSkinUboSetV3aToBinding2(sCurrentSkin)` を per-draw
   呼出 = descriptor set V3a binding=2 を real Skin UBO buffer に都度切替 =
   multi-skin stale 化 risk 吸収
4. **per-draw flush** (本 sub-step 新設、(N11-6) A): `flushSkinUbos(sCurrentSkin)`
   呼出で dirty range を `vkFlushMappedMemoryRanges` 経由 GPU 可視化
5. **descriptor set bind** (PC-N-8 (f) 既配線): `bindV3aRigged(cmd_buf, sFrameIndex,
   real_asset_dynamic_offsets)` で set=3 binding=2 = real Skin UBO bind

### §5.2 fall-through path (= `AYAGltfMultiSkinEnabled=false` or `sCurrentSkin==nullptr` 時)

- `skin_to_use = sGltfStubSkin` sentinel
- 既 identity 64 B `writeSkinUbo` 維持 (= PC-N-8 (f) 同形 path)
- `wireSkinUboSetV3aToBinding2(sGltfStubSkin)` + `flushSkinUbos(sGltfStubSkin)`
  unconditional 呼出 = graceful degrade
- PC-N-15 cleanup phase で sentinel storage 撤去予定 ((N11-3) A + (E-11) A 整合)

### §5.3 Phase 1.E 残課題 (= PC-N-12..PC-N-15)

- **PC-N-12** (= 2nd sub-step) real node modelview 切替 = identity push constant
  modelview → real `Asset::mNodes[node_index].mMatrix` 経由
- **PC-N-13** (= 3rd sub-step) real per-draw + multi-asset verify =
  zero-buffer `PerDrawUBO_LightParams` 卒業 + `AYAGltfMultiAssetCanary` cvar
  (debug-only) で複数 asset 同時描画検証
- **PC-N-14** (= 4th sub-step) worker thread design-lock (= per-Primitive UBO
  write + cmdbuf record 並列化 design)
- **PC-N-15** (= 5th = 最終 sub-step) worker thread 実装 + cleanup (=
  `sGltfStubSkin` sentinel storage 撤去 + Phase 1.E complete marker)

---

## §6. 残 strict 線形

- ✅ Phase 1.A / 1.B / (Z) SSS / (W) uniform4iv / (Y) Phase 1.C prep
- ✅ PC-0..PC-7ε / PC-N decomposition / PC-N-1..PC-N-4 (= Phase 1.C complete)
- ✅ PC-8 Linux primary marker (= Phase 1.C strict 線形終了)
- ✅ PC-N-5 (= Phase 1.D 着手起点) / Phase 1.D decomposition design-lock
- ✅ PC-N-6 / PC-N-7 / PC-N-8 / PC-N-9 / PC-N-10 (= Phase 1.D complete = 1 GLTF
  asset 完全 Vulkan draw 通電 達成)
- ✅ Phase 1.E decomposition design-lock (commit `094546889b`) + PC-N-11
  design-lock (commit `87560a4dc7`)
- ✅ **PC-N-11 実装 ✅ 本 commit = Phase 1.E 内 1st sub-step 実装完了 =
  multi-skin real Skin path 通電**
- ⏳ PC-N-12 design-lock + 実装 (= real node modelview)
- ⏳ PC-N-13 design-lock + 実装 (= real per-draw + multi-asset verify)
- ⏳ PC-N-14 design-lock (= worker thread design)
- ⏳ PC-N-15 実装 + cleanup (= worker thread 実装 + Phase 1.E complete marker)
- ⏳ Phase 1.E complete → Phase 1 全完了 → Mac/Win 開発者補完 phase

---

## §7. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ +
(Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α..PC-7ε ✅ +
PC-N decomposition design-lock ✅ + PC-N-1..PC-N-4 ✅ = Phase 1.C complete ✅ +
PC-8 Linux primary marker ✅ + PC-N-5 ✅ + Phase 1.D decomposition design-lock ✅
+ PC-N-6 ✅ + PC-N-7 ✅ + PC-N-8 ✅ + PC-N-9 ✅ + PC-N-10 ✅ = Phase 1.D
complete ✅ (= 1 GLTF asset 完全 Vulkan draw 通電 達成) +
Phase 1.E decomposition design-lock ✅ + PC-N-11 design-lock ✅ +
**PC-N-11 ✅ 本 commit = Phase 1.E 内 1st sub-step 実装完了 = multi-skin real
Skin path 通電** +
⏳ PC-N-12..PC-N-15 design-lock + 実装 + Phase 1.E complete + Phase 1 全完了 +
Mac/Win 開発者補完 phase

---

## §8. self-verify 9 観点 全 ✅

1. ✅ Exit Criteria 10 項全充足 (§3)
2. ✅ 必読 1 件 (PC-N-11 design-lock doc) + pinpoint reference 別記 = full file
   dump なし (= `feedback_handoff_minimal_pre_req_read` 整合)
3. ✅ step (a)/(b)/(c)/(d)/(e)/(f)/(g) 7 step 全実装 (§1)
4. ✅ ambiguity (N11-1)..(N11-16) 16 件 AYA literal「全件推奨で OK」record
   (2026-06-05) design-lock 継承 + 本実装で全件採用案通り実装
5. ✅ GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= count llvkloader.cpp=6
   不変、PC-N-10 commit `16a26f6272` 同数)
6. ✅ MUSEUBO-A 整合 = `AYAGltfMultiSkinEnabled=false` default で sentinel
   fall-through 経路維持 + OpenGL 描画 100% 維持 + PC-N-8 (f) 5 段 graceful
   degrade 内部維持
7. ✅ build verify literal 取得 = llrender PASS + WARNING 0 + TUT 11/11 + 10/10 +
   13/13 + codegen 131/131 全 PASS
8. ✅ commit 内容 = 2 modified (indra/) + 1 modified (cross-platform spec) +
   1 new doc (本 complete handoff) + CMake 改変 0 + codegen 改変 0 + shader 改変
   0 + .h 改変 0 + tests/ 改変 0 + Co-Authored-By 不在
9. ✅ `feedback_no_scope_shrink` 遵守 = PC-N-11 literal scope 4 件 §0 全件実装、
   (N11-3) A sentinel 維持 + (N11-4) A inline `writeSkinUbo` 不要 = upstream
   `Skin::uploadMatrixPalette` PC-7γ-3 (j) dual-write 既配線資産活用ゆえ縮小
   ではなく既配線活用、sentinel storage 撤去は PC-N-15 cleanup phase 持越し =
   (E-11) A 整合 = 別 phase 分解 = `feedback_ubo_migration_one_at_a_time` 厳格
   遵守整合

---

## §9. 次 session 着手 1 line

PC-N-12 design-lock 着手 = real node modelview 切替 (= identity push constant
modelview → real `Asset::mNodes[node_index].mMatrix` 経由) +
`AYAGltfRealModelviewEnabled` cvar 新設 + ambiguity 確認 + 実装計画分解 +
Exit Criteria 明文化 = `feedback_ubo_migration_one_at_a_time` 厳格遵守で本
PC-N-11 multi-skin 通電 baseline 上に Phase 1.E 内 2nd sub-step として
別 session 別途 design-lock。

---

## §A. feedback 遵守 record

- ✅ `feedback_proactive_handoff` (本 PC-N-11 complete handoff doc 起案)
- ✅ `feedback_handoff_minimal_pre_req_read` (必読 1 件 = PC-N-11 design-lock doc
  + pinpoint Read のみ、full file dump なし)
- ✅ `feedback_self_verify_before_handoff` (9 観点 self-verify 全 ✅)
- ✅ `feedback_build_only_verified` (llrender + WARNING 0 + TUT 11/11 + 10/10 +
  13/13 + codegen 131/131 + GATE-B integrity LL_VULKAN_GLSL=6 不変で literal
  検証取得)
- ✅ `feedback_no_scope_shrink` (PC-N-11 literal scope 4 件 §0 全件実装、
  (N11-3) A sentinel 維持 + (N11-4) A inline `writeSkinUbo` 不要 = upstream
  `Skin::uploadMatrixPalette` PC-7γ-3 (j) dual-write 既配線資産活用ゆえ縮小
  ではなく既配線活用、sentinel storage 撤去は PC-N-15 cleanup phase 持越し =
  (E-11) A 整合 = 別 phase 分解整合)
- ✅ `feedback_doubt_self_first` (design-lock phase で ambiguity 16 件発見 +
  推奨案提示 + AYA literal「全件推奨で OK」record 後本実装、本実装中も namespace
  visibility (`namespace LLVKLoader` 内 anonymous namespace 内で `sCurrentSkin`
  + `wireSkinUboSetV3aToBinding2` 直接 access 可能) を Read で literal 確認後
  unqualified 呼出採用、推測実装なし)
- ✅ `feedback_confirm_referent_before_acting` (16 件 batch AYA 確認 design-lock
  phase で完了、本実装中も PC-N-8 (f) outer tag block 構造温存 + PC-N-11 (a)
  inner tag block 配置 = (N11-7) A literal 確認後採用、推測実装なし)
- ✅ `feedback_ubo_migration_one_at_a_time` 厳格遵守 (PC-N-11 = multi-skin
  単独 sub-step、PC-N-12..PC-N-15 (= real node modelview / real per-draw +
  multi-asset verify / worker thread design + 実装) は別 phase の別 session で
  別途分解)
- ✅ `feedback_design_phase_no_code_write` 整合 (本 PC-N-11 は実装 phase = 改変
  あり、design-lock commit `87560a4dc7` で `indra/` 改変 0 件完了済)
- ✅ `feedback_release_branch_workflow` (feature branch
  `feature/ayastorm-r41-gl-removal` 上 commit)
- ✅ `feedback_no_auto_commit` (AYA 明示 commit 指示受領後 commit 予定)
- ✅ `feedback_no_claude_coauthor` (Co-Authored-By 行不在)
- ✅ `feedback_no_bare_reference_ids` ((N11-1)..(N11-16) 各 ID に項目名 / 採用案
  内容併記 + (a)..(g) 各 step に作業内容併記)
- ✅ `feedback_tests_dir_never_commit` 整合 (tests/ 改変 0 件、git add 個別
  file 指定予定)
- ✅ memory `project_ayastorm_r41_design_principles` 整合
  ((1) Upstream OpenGL 取り込みやすさ維持 = `recordGltfAssetDraw` signature 不変
  + `Skin::uploadMatrixPalette` PC-7γ-3 (j) upstream 既配線活用 +
  `GLTFSceneManager::render` 改変 0 件 + shader 改変ゼロ +
  (2) Core プロセス分散実現 = per-Primitive Skin handling で primitive-level
  granularity 維持、PC-N-14/15 worker thread 分散 design 整合)
- ✅ memory `project_r41_phase1b_vulkan_host_gate` 整合 (GATE-B = `#ifdef
  LL_VULKAN_GLSL` 新規追加 0 件、`AYAGltfMultiSkinEnabled` cvar runtime gate
  のみ、count=6 不変)
- ✅ memory `project_ayastorm_three_platforms` 整合 (cross-platform spec §6
  PC-N-11 行状態 ✅ 反映 = host-side `wireSkinUboSetV3aToBinding2` per-draw
  rewire は `vkUpdateDescriptorSets` 呼出ゆえ MoltenVK 標準対応範囲 +
  `Skin::uploadMatrixPalette` PC-7γ-3 (j) upstream dual-write は OS 非依存 +
  descriptor set 数 5 維持 + `sGltfStubSkin` sentinel + `sGltfStubAssetPipeline`
  維持で MoltenVK 影響増なし + `AYAGltfMultiSkinEnabled` cvar XML は OS 非依存
  ゆえ macOS 派生 fix 候補なし想定 + Windows full Vulkan ゆえ派生 fix 候補なし、
  Linux primary 完成 → 他者補完 model 整合)
