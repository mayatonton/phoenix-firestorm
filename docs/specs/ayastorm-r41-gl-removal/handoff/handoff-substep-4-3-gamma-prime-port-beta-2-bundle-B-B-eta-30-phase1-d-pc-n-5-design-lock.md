# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.D **PC-N-5 design-lock complete**

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-N-5 (= Phase 1.D 着手起点 = 実 GLTF Vulkan draw 通電 1 stub) **design-lock phase 完了 marker**。ambiguity (N5-1)..(N5-10) 10 件 AYA literal「OK」record (2026-06-05) + 実装計画 (a)-(g) 7 step 分解 + Exit Criteria 10 項明文化。`indra/` 改変 0 件 (= `feedback_design_phase_no_code_write` 厳格遵守)。

> **本 doc 位置付け**: PC-8 Linux primary marker (= `3c0c72d34f`) 後の PC-N-5 = **Phase 1.D 着手起点 design-lock phase** = 別 session で **PC-N-5 実装着手** 前の最後の準備 phase。実装は本 commit 完了後の別 session の fresh context で進行 (= ambiguity 確認 + 実装計画分解後の実装 phase)。

---

## §0. 本 session 着手契機 + PC-N-5 literal scope

**契機**: AYA 指示「着手お願いします」literal 受領 (2026-06-05、PC-8 Linux primary marker commit `3c0c72d34f` 後の継続 session = 別 session の fresh context) + 必読 1 件 = `handoff-...-phase1-c-pc-8-linux-primary-marker.md` (= Phase 1.C strict 線形終了 marker + Linux primary build verify 取得済 §2) Read + pinpoint reference 3 件 (= PC-N-3 complete §2.7 address-only sentinel pattern + PC-N decomposition §5 PC-N-5 概略 + cross-platform spec §6 OS 依存懸念追記欄) Read → Explore agent 経由現状調査 (= design 09-phase-roadmap.md Phase 1.D 定義 + design 06b §2.5 GLTFSceneManager 経路 + design 06c §2.5 set=3 binding=2 wire + gltfscenemanager.cpp:640 render() + lldrawpoolpbropaque.cpp + pipeline.cpp renderGLTFObjects + recordAvatarPlaceholderDraw / recordPlaceholderPoolDraw 現状 + sSkinUboDirty 現状 + AYARingBufferSizeMB / AYAPipelineCacheSizeMB 既存 cvar) → ambiguity (N5-1)..(N5-10) 10 件 + 推奨案 + 採用根拠提示 → AYA literal「OK」一括確認受領 (2026-06-05) で本 design-lock doc 起案。

**PC-N-5 literal scope** (= AYA 確認済 (N5-1) A + (N5-3) A + (N5-5) A 採用後):

1. **`sGltfStubSkin` sentinel storage 新設** = 第 2 の address-only sentinel pattern (= `sPlaceholderSkin` 同形 = `alignas(void*) char sGltfStubSkinStorage[1]` + `reinterpret_cast<LL::GLTF::Skin*>`、`anonymous namespace` 内、PC-N-3 §2.7 address-only sentinel pattern 踏襲)
2. **`recordGltfAssetDraw` 関数新設** = `recordAvatarPlaceholderDraw` 同形 pattern、identity matrix bone matrix data 構築 + `writeSkinUbo` + `flushSkinUbos` + `bindV3aRigged` 正規 sequence
3. **`initVulkan` 内 stub Skin register + 初回 wire** = `registerSkinUbo(sGltfStubSkin, Skin_GLTFJoints, block_size)` + `wireSkinUboSetV3aToBinding2(sGltfStubSkin)` ((N5-3) A + (N5-7) A 採用後)
4. **`shutdownVulkan` 内 stub Skin unregister** = `unregisterSkinUbo(sGltfStubSkin, Skin_GLTFJoints)` 対称 lifecycle
5. **`AYAGltfStubDrawEnabled` cvar 新設** (= Boolean, default `false`, Persist=1) + `recordGltfAssetDraw` 発火 hook ((N5-4) A 採用後、live A/B 可能、`GATE-B` 整合)

> **`feedback_no_scope_shrink` 整合**: PC-N decomposition §4.5 literal「実 GLTF Vulkan draw 通電 1 stub」を **(N5-1) A 採用** で「Skin_GLTFJoints UBO bind 経由 rigged draw、第 2 sentinel-like skin で並走通電」と確定 = scope 縮小ではなく **literal 解釈確定**、(N5-1) B (= 実 LL::GLTF::Asset 経由 vertex buffer upload + bind) + (N5-1) C (= GLTFSceneManager::render 内 mUseUBO gate 配線) は Phase 1.D 内後続 sub-step (= PC-N-6 以降仮定) に分離 (= AYA literal「OK」record 済段階分離、`feedback_ubo_migration_one_at_a_time` 厳格遵守)。

---

## §1. 必読 + pinpoint reference (= 次 session = PC-N-5 実装 phase 向け)

**次 session 必読**:

1. **本 PC-N-5 design-lock doc 全文**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-d-pc-n-5-design-lock.md`

**pinpoint reference** (= 実装 phase で必要分のみ Read):

- **PC-N-3 complete §2.7**: address-only sentinel pattern (= layering 制約 + LL::GLTF::Skin forward-decl only 制約 + `alignas(void*) char` storage pattern)
- **PC-N-3 complete §2.1**: `sPlaceholderSkin` sentinel 実装 example (= line 563-579 = `sGltfStubSkin` 同形 source)
- **PC-N-3 complete §2.1**: `recordAvatarPlaceholderDraw` 内 `writeSkinUbo` + `flushSkinUbos` + `bindV3aRigged` sequence (= line 5395-5480 = `recordGltfAssetDraw` 同形 source)
- **PC-N-3 complete §2.1**: `initVulkan` 内 `registerSkinUbo(sPlaceholderSkin)` + `wireSkinUboSetV3aToBinding2(sPlaceholderSkin)` (= line 3701-3748 = stub Skin register source)
- **PC-N-3 complete §2.1**: `shutdownVulkan` 内 `unregisterSkinUbo(sPlaceholderSkin)` (= line 4051-4058 = 対称 unregister source)
- **wireSkinUboSetV3aToBinding2(LL::GLTF::Skin*) helper**: llvkloader.cpp:2746-2820 (= PC-N-3 (d) tag block)
- **writeSkinUbo + flushSkinUbos + registerSkinUbo signature**: llvkloader.cpp:4504/5035/5094/5110 (= PC-7γ-2 配置済 helper)
- **ubo_metadata.inl g_block_metadata**: `Skin_GLTFJoints` block_hash + block_size lookup (= `ubo_layout_skin_gltfjoints.inl` line 15 = 16384 B std140 upper bound)
- **bindV3aRigged signature**: llvkloader.cpp (= PC-N-2 で復活 + set=3 swap で `sAssetUboSetV3a[frame]` bind、PC-N-7 A 採用で signature 不変)
- **design 06b §2.5**: `flushSkinUbos(skin)` の GLTFSceneManager::render 直前 pattern (= 06b-cadence-update-site-and-dirty.md line 108-119)
- **design 06c §2.5**: set=3 binding=2 = Skin_GLTFJoints per-skin cadence owner (= 06c-descriptor-set-bind-wiring.md line 121-128)
- **GATE-B literal**: memory `project_r41_phase1b_vulkan_host_gate` (= host C++ は `mUseUBO` runtime flag のみで gate、PC-N-5 では `AYAGltfStubDrawEnabled` cvar 追加で並走切替)
- **cross-platform spec §6**: PC-N-5 OS 依存懸念追記欄 (= 本 commit で `ayastorm-r41-cross-platform-port-spec.md` §6 PC-N-5 行追記)

---

## §2. 現状調査 (= Explore agent 経由 pinpoint 取得)

### §2.1 design doc Phase 1.D / PC-N-5 関連

| # | doc + 場所 | literal / 要約 |
|---|----------|--------------|
| 1 | design 09-phase-roadmap.md | Phase 1.A/B/C 定義あり、**Phase 1.D 明示記載なし** = (N5-2) A 採用根拠 = 「Phase 1.D = PC-N-5 着手起点 marker のみ、全体構造は次の design-lock phase で別途分解」 |
| 2 | design 06b §2.5 line 108-119 | `flushSkinUbos(skin)` 配置点 = 「既存 `GLTFSceneManager::render(variant)` (`gltfscenemanager.cpp:736`) 直前で `flushSkinUbos(skin)` を呼ぶ — rigged draw 順序に同期」 = (N5-1) A 整合 (= 同 pattern を `recordGltfAssetDraw` 内で踏襲) |
| 3 | design 06c §2.5 line 121-128 | `set=3 binding=2` = `Skin_GLTFJoints`、per-skin cadence owner 単位 = (N5-7) A 整合 (= `bindV3aRigged` signature 不変、`UboSkinKey` で sentinel と stub Skin* 同居) |
| 4 | design 08-build-codegen-pipeline.md | Phase 1.D / PC-N-5 直接記載なし = (N5-2) A 整合 |

### §2.2 GLTF render path 現状

| # | 場所 | 内容 |
|---|------|------|
| 5 | gltfscenemanager.cpp:640 | `void GLTFSceneManager::render(Asset& asset, U8 variant)` = asset draw main path、内部で shader bind + `flushAssetUbos` (line 702) + `flushSkinUbos` (line 759) + draw call、**Vulkan gate `mUseUBO` 未依存、OpenGL path のまま通電** = (N5-8) A 採用根拠 (= PC-N-5 scope は `LLVKLoader::recordGltfAssetDraw` 新設のみ、`GLTFSceneManager::render` は OpenGL path 不変) |
| 6 | lldrawpoolpbropaque.cpp | GLTF path 無し (= 既存 OpenGL PBR pool) → GLTF は `GLTFSceneManager` 経由単独 |
| 7 | pipeline.cpp line 8495 | `LLPipeline::renderGLTFObjects()` → `GLTFSceneManager::instance().renderOpaque/render()`、`mUseUBO` gate 無し = (N5-8) A 整合 (= GLTFSceneManager 統合は別 sub-step) |

### §2.3 llvkloader Vulkan draw path 現状

| # | 関数 / 場所 | 内容 |
|---|------------|------|
| 8a | `recordAvatarPlaceholderDraw` (line 5395-5480) | PC-N-3 完了状態 = sentinel `sPlaceholderSkin` → `writeSkinUbo` (zero 256B) → `flushSkinUbos` → `bindV3aRigged` set=3 swap sequence、**`recordGltfAssetDraw` 同形 source** |
| 8b | `recordPlaceholderPoolDraw` (line 5293-5377) | PC-N-1 完了状態 = `writeDrawUbo` (PerDrawUBO_LightParams zero 256B) → `bindV3aStatic` set=0/1/2 → `vkCmdDraw(3,1,0,0)` |
| 8c | **実 GLTF asset 用 record 関数** | **未存在** = PC-N-5 で `recordGltfAssetDraw` 新設要 ((N5-5) A 採用根拠) |
| 8d | public API entry | `recordPlaceholderPoolDraw` / `recordAvatarPlaceholderDraw` (line 5293/5395) のみ、`recordGltfAssetDraw` 新設で 3rd entry 追加想定 |

### §2.4 cvar / flag 現状

| # | cvar | default | 場所 | 用途 |
|---|------|---------|------|------|
| 9a | `AYARingBufferSizeMB` | 4 MB | settings.xml line 1416 | r41 既存 |
| 9b | `AYAPipelineCacheSizeMB` | 64 MB | settings.xml line 1596 | r41 既存 |
| 9c | `UseVulkan` / `mUseUBO` | (cvar なし) | - | host C++ inline bool member、GATE-B 確定 |
| 9d | **`AYAGltfStubDrawEnabled`** | (本 PC-N-5 で新設) | settings.xml | (N5-4) A 採用 = Boolean default `false` Persist=1、debug live A/B |

### §2.5 sSkinUboDirty 上の同居経路

| # | 状態 | 内容 |
|---|------|------|
| 10a | 現状 register | sentinel `sPlaceholderSkin` 1 個 (= line 579 + `initVulkan` 内 `registerSkinUbo` line 3726) |
| 10b | UboSkinKey map 構造 | line 577 = `<LL::GLTF::Skin*, U32 block_hash>` (= sentinel と stub Skin* discrimination 可能 = (N5-7) A 採用根拠) |
| 10c | 実 stub Skin* register 経路 | **PC-N-5 で新規配線** = `initVulkan` 内 `registerSkinUbo(sGltfStubSkin, Skin_GLTFJoints, 16384B)` + `wireSkinUboSetV3aToBinding2(sGltfStubSkin)` 想定 ((N5-3) A) |

---

## §3. ambiguity 10 件 + AYA literal record + 採用根拠

**AYA literal record**: 2026-06-05、Claude 提示の (N5-1)..(N5-10) 10 件全件推奨案に対して AYA literal「OK」一括確認受領 = 全件採用確定。

| # | ambiguity 項目 | 候補 | 採用 | 根拠 |
|---|--------------|------|------|------|
| (N5-1) | **「実 GLTF Vulkan draw」literal 解釈** | A=Skin_GLTFJoints UBO bind 経由 rigged draw / B=実 LL::GLTF::Asset 経由 vertex+index buffer upload / C=GLTFSceneManager::render 内 Vulkan dispatch | **A** | 最小実装、PC-N-1..PC-N-4 + PC-N-3 infrastructure 全活用、`feedback_ubo_migration_one_at_a_time` 厳格遵守、B/C は Phase 1.D 内後続 sub-step に分離 |
| (N5-2) | **Phase 1.D scope 境界** | A=PC-N-5 着手起点 marker のみ / B=Phase 1.D 全 sub-step 仮定義 | **A** | PC-N decomposition design-lock pattern 同形、PC-N-5 単独 sub-step 集中、design 09 Phase 1.D 明示なし整合 |
| (N5-3) | **stub Skin storage 配置** | A=`sGltfStubSkin` 第 2 sentinel address-only pattern / B=実 LL::GLTF::Skin instance load / C=`sPlaceholderSkin` 再利用 + key 拡張 | **A** | PC-N-3 §2.7 確立 address-only sentinel pattern 踏襲、layering 制約遵守 (= `LL::GLTF::Skin` forward-decl only)、sentinel と実 stub Skin* 並走 baseline |
| (N5-4) | **placeholder 経路との切替** | A=`AYAGltfStubDrawEnabled` cvar 新設 (Boolean default false Persist=1) / B=build-time flag (= `#ifdef`、GATE-B 違反) / C=切替なし並走発火 | **A** | GATE-B 整合、live A/B、`feedback_visual_decisions_need_live_ab` 整合、Persist=1 で起動間設定保持 |
| (N5-5) | **record 関数新設 / 既存拡張** | A=`recordGltfAssetDraw` 新設 / B=`recordAvatarPlaceholderDraw` signature 拡張 | **A** | 関数 1 責任原則、`record*Draw` pattern 統一、sentinel 専用経路温存、§2.3 8c (実 GLTF record 関数未存在) 整合 |
| (N5-6) | **bone matrix data 構築 source** | A=identity matrix 1 個 (4×4 mat4 padded to 256 B) / B=zero matrix (= sentinel と識別性なし) / C=T-pose matrix hardcoded | **A** | 最小 + 識別性、Skin_GLTFJoints UBO layout 通電確認可能、zero (= sentinel) と区別可能 |
| (N5-7) | **bindV3aRigged signature** | A=不変 (`UboSkinKey` で sentinel と stub Skin* 同居) / B=第 2 引数 Skin* 拡張 | **A** | PC-N-3 で sentinel 経路通電済、複数 skin 同居設計済 (= UboSkinKey map 第 1 要素 Skin* pointer compare)、signature 安定性最大 |
| (N5-8) | **GLTFSceneManager::render との関係** | A=PC-N-5 scope は LLVKLoader 完結、GLTFSceneManager 不変 / B=PC-N-5 で GLTFSceneManager::render 内 mUseUBO gate 配線 | **A** | `feedback_ubo_migration_one_at_a_time` 厳格遵守、PC-N-5 = llrender 層完結、GLTFSceneManager 統合は別 sub-step |
| (N5-9) | **build verify scope** | A=llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131 (Linux primary、PC-N-1..PC-N-4 同形) | **A** | PC-8 Linux primary marker 採用後の唯一 verify scope、newview 改変 0 件ゆえ llrender 単独で十分 |
| (N5-10) | **PC-N-5 Exit Criteria 項目数** | A=10 項 (PC-N-1/2/3/4 同形 template) | **A** | 一貫性 + template 流用 |

---

## §4. 実装計画 (a)-(g) 7 step

### §4.1 step (a): `sGltfStubSkin` sentinel storage 新設

**場所**: `indra/llrender/llvkloader.cpp` anonymous namespace 内、**`sPlaceholderSkin` 直後** (= 現状 line ~580 想定、PC-N-3 (b) tag block 直後)

**実装**:

```cpp
// <AYAstorm r41 PC-N-5 (a)>
// PC-N-5: GLTF stub skin sentinel (= address-only sentinel pattern、PC-N-3 §2.7 同形).
// sPlaceholderSkin と異なる **第 2 の固定 address** で、Skin_GLTFJoints UBO bind 経由
// rigged draw 通電 baseline を確立。sSkinUboDirty 上で sentinel と並走可能
// (= UboSkinKey 第 1 要素 Skin* pointer compare で discrimination)。
// AYA literal「OK」record 2026-06-05 = (N5-1) A + (N5-3) A 採用。
alignas(void*) char sGltfStubSkinStorage[1] = {};
LL::GLTF::Skin* const sGltfStubSkin = reinterpret_cast<LL::GLTF::Skin*>(&sGltfStubSkinStorage[0]);
// </AYAstorm r41 PC-N-5 (a)>
```

### §4.2 step (b): `recordGltfAssetDraw` 関数新設

**場所**: `indra/llrender/llvkloader.cpp` anonymous namespace 内、**`recordAvatarPlaceholderDraw` 直後** (= 現状 line ~5480 想定)

**実装** (= `recordAvatarPlaceholderDraw` (line 5395-5480) 同形 + `sGltfStubSkin` 投入 + identity matrix bone data):

```cpp
// <AYAstorm r41 PC-N-5 (b)>
// PC-N-5: GLTF stub asset draw record (= 実 GLTF Vulkan draw 通電 1 stub、
// Phase 1.D 着手起点). recordAvatarPlaceholderDraw 同形 pattern で sGltfStubSkin
// 経由 Skin_GLTFJoints UBO bind + identity bone matrix data + bindV3aRigged 正規
// sequence で通電 baseline を確立。AYAGltfStubDrawEnabled cvar=true 時のみ発火。
// AYA literal「OK」record 2026-06-05 = (N5-1) A + (N5-5) A + (N5-6) A 採用。
void recordGltfAssetDraw(VkCommandBuffer cmd, U32 frame_idx) {
    if (sDevice == VK_NULL_HANDLE || cmd == VK_NULL_HANDLE) {
        LL_WARNS_ONCE("AYAstormR41") << "PC-N-5: device or cmd null, skip GLTF stub draw" << LL_ENDL;
        return;
    }

    // PC-N-5 (b) identity matrix bone data 構築 = 4×4 mat4 identity padded to 256 B
    // (= Skin_GLTFJoints std140 layout 最小通電実例、zero (sentinel) と区別可能).
    static const F32 identity_skin_buf[64] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        // 残 48 floats = 0.0f (= padding to 256 B)
    };

    // PC-N-5 (b) writeSkinUbo + flushSkinUbos = design 06b §2.5 正規 sequence
    // (= GLTFSceneManager::render(variant) 直前 pattern を本 stub 経路で踏襲).
    LLVKLoader::writeSkinUbo(sGltfStubSkin, ubo::block_hash::Skin_GLTFJoints,
                              /*offset=*/0u, identity_skin_buf, sizeof(identity_skin_buf));
    LLVKLoader::flushSkinUbos(sGltfStubSkin);

    // PC-N-5 (b) writeDrawUbo + bindV3aRigged = recordAvatarPlaceholderDraw 同形.
    // PerDrawUBO_LightParams zero 256B + set=3 swap で sGltfStubSkin の
    // Skin_GLTFJoints UBO bind (= UboSkinKey map 第 1 要素 pointer compare で
    // sentinel と discrimination).
    static const U8 zero_draw_buf[256] = {};
    LLVKLoader::writeDrawUbo(ubo::block_hash::PerDrawUBO_LightParams,
                              /*offset=*/0u, zero_draw_buf, sizeof(zero_draw_buf));
    U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS] = {0, 0, 0, 0};
    bindV3aRigged(cmd, frame_idx, dynamic_offsets);

    // PC-N-5 (b) push constant + draw call (= recordAvatarPlaceholderDraw 同形 vkCmdDraw(3,1,0,0)).
    // ... (PC-N-2 で確立した push constant 64 B / VERTEX_BIT setup)
    vkCmdDraw(cmd, 3, 1, 0, 0);

    // PC-N-5 (b) first-fire LL_INFOS marker.
    static std::atomic<bool> s_first_fire{true};
    if (s_first_fire.exchange(false, std::memory_order_acq_rel)) {
        LL_INFOS("AYAstormR41") << "PC-N-5 (b) GLTF stub draw 通電 (first fire): "
                                   "sGltfStubSkin Skin_GLTFJoints identity matrix path 経由 "
                                   "(= 実 GLTF Vulkan draw 1 stub = Phase 1.D 着手起点)"
                                << LL_ENDL;
    }
}
// </AYAstorm r41 PC-N-5 (b)>
```

> **注**: 具体的 line 番号 / push constant setup detail / `V3A_DRAW_SET_BINDINGS` 定数値 / `bindV3aRigged` 引数 calling convention 等は実装 phase で `recordAvatarPlaceholderDraw` source (= line 5395-5480) を pinpoint Read で確認しつつ literal 同形踏襲。

### §4.3 step (c): `initVulkan` 内 stub Skin register + 初回 wire

**場所**: `indra/llrender/llvkloader.cpp` `LLVKLoader::initVulkan` 内、**`registerSkinUbo(sPlaceholderSkin, ...)` + `wireSkinUboSetV3aToBinding2(sPlaceholderSkin)` 直後** (= 現状 line 3701-3748 想定、PC-N-3 (c)+(d) tag block 直後)

**実装**:

```cpp
// <AYAstorm r41 PC-N-5 (c)>
// PC-N-5: GLTF stub Skin register + 初回 wire (= sPlaceholderSkin 同形 lifecycle).
// AYA literal「OK」record 2026-06-05 = (N5-3) A + (N5-7) A 採用. 3 段 graceful
// degrade (block lookup miss / register fail / wire fail で LL_WARNS_ONCE).
{
    U32 skin_block_size = 0;
    for (auto const& m : ubo::g_block_metadata) {
        if (m.block_hash == ubo::block_hash::Skin_GLTFJoints) {
            skin_block_size = m.block_size;
            break;
        }
    }
    if (skin_block_size == 0) {
        LL_WARNS_ONCE("AYAstormR41") << "PC-N-5 (c): Skin_GLTFJoints block_size lookup miss, "
                                        "skip sGltfStubSkin register" << LL_ENDL;
    } else if (!registerSkinUbo(sGltfStubSkin, ubo::block_hash::Skin_GLTFJoints, skin_block_size)) {
        LL_WARNS_ONCE("AYAstormR41") << "PC-N-5 (c): registerSkinUbo(sGltfStubSkin) failed, "
                                        "skip wire" << LL_ENDL;
    } else if (!wireSkinUboSetV3aToBinding2(sGltfStubSkin)) {
        LL_WARNS_ONCE("AYAstormR41") << "PC-N-5 (c): wireSkinUboSetV3aToBinding2(sGltfStubSkin) failed" << LL_ENDL;
    } else {
        LL_INFOS("AYAstormR41") << "PC-N-5 (c) GLTF stub Skin register + 初回 wire 完了 = "
                                   "Phase 1.D 着手起点 baseline 確立" << LL_ENDL;
    }
}
// </AYAstorm r41 PC-N-5 (c)>
```

### §4.4 step (d): `shutdownVulkan` 内 stub Skin unregister

**場所**: `indra/llrender/llvkloader.cpp` `LLVKLoader::shutdownVulkan` 内、**`unregisterSkinUbo(sPlaceholderSkin, Skin_GLTFJoints)` 直後** (= 現状 line 4051-4058 想定、PC-N-3 (c) tag block 直後)

**実装**:

```cpp
// <AYAstorm r41 PC-N-5 (d)>
// PC-N-5: GLTF stub Skin unregister (= 対称 lifecycle、(c) と対). AYA literal「OK」
// record 2026-06-05 = (N5-3) A 採用.
unregisterSkinUbo(sGltfStubSkin, ubo::block_hash::Skin_GLTFJoints);
// </AYAstorm r41 PC-N-5 (d)>
```

### §4.5 step (e): `AYAGltfStubDrawEnabled` cvar 新設 + 発火 hook

**場所 1** (cvar 新設): `indra/newview/app_settings/settings.xml`

```xml
<key>AYAGltfStubDrawEnabled</key>
<map>
  <key>Comment</key>
  <string>AYAstorm r41 PC-N-5: enable GLTF stub draw (= Skin_GLTFJoints UBO bind 経由 rigged draw 1 stub for Phase 1.D 着手起点). Default OFF, debug live A/B 用.</string>
  <key>Persist</key>
  <integer>1</integer>
  <key>Type</key>
  <string>Boolean</string>
  <key>Value</key>
  <integer>0</integer>
</map>
```

**場所 2** (発火 hook): `indra/llrender/llvkloader.cpp` 内、**`recordAvatarPlaceholderDraw` 呼出 site と並走** (= 実装 phase で `recordAvatarPlaceholderDraw` caller site を grep で literal 確認、同 site で `AYAGltfStubDrawEnabled` cvar 読出 + true なら `recordGltfAssetDraw` 並走発火、`feedback_visual_decisions_need_live_ab` 整合)

**実装** (= 発火 hook 部、caller site で):

```cpp
// <AYAstorm r41 PC-N-5 (e)>
// PC-N-5: AYAGltfStubDrawEnabled cvar=true 時、GLTF stub draw 並走発火.
// AYA literal「OK」record 2026-06-05 = (N5-4) A 採用. recordAvatarPlaceholderDraw
// 直後並走 (= sentinel と stub Skin* 同居 baseline 通電).
static LLCachedControl<bool> ayastormGltfStubDrawEnabled(gSavedSettings, "AYAGltfStubDrawEnabled", false);
if (ayastormGltfStubDrawEnabled) {
    recordGltfAssetDraw(cmd, frame_idx);
}
// </AYAstorm r41 PC-N-5 (e)>
```

> **注**: 具体的 cvar 読出 location (= `LLVKLoader` 内 anonymous namespace static or `recordAvatarPlaceholderDraw` 関数内 static) は実装 phase で `LLCachedControl` pattern (= viewer 内既存 pattern) + `gSavedSettings` 到達性を確認しつつ確定。

### §4.6 step (f): build verify

**コマンド** (= PC-N-1..PC-N-4 同形 + Linux primary、(N5-9) A 採用):

1. `cd build-linux-x86_64 && make -j4 llrender` → PASS + ERROR 0 / WARNING 0 想定
2. `INTEGRATION_TEST_lluboringbuffer` → 11/11 PASS YAY 想定
3. `INTEGRATION_TEST_llassetubopool` → 10/10 PASS YAY 想定
4. `INTEGRATION_TEST_llpipelinecachestorage` → 13/13 PASS YAY 想定
5. `scripts/ubo_codegen` unittest → 131/131 PASS 想定 (= codegen 改変 0 件)

### §4.7 step (g): handoff complete doc 起案 + Exit Criteria 10 項 self-verify + AYA commit 指示後 commit

**実装 phase 完了 marker** doc 起案 = `handoff-substep-...-pc-n-5-complete.md`。

### §4.8 GATE-B 整合 (= memory `project_r41_phase1b_vulkan_host_gate` 遵守)

`#ifdef LL_VULKAN_GLSL` **新規追加 0 件**。host C++ は `mUseUBO` runtime flag のみで gate。PC-N-5 では cvar `AYAGltfStubDrawEnabled` で並走切替するが、`#ifdef` 経由でなく runtime cvar 読出ゆえ GATE-B 違反なし。

### §4.9 MUSEUBO-A 整合 (= memory `feedback_ubo_migration_one_at_a_time` 整合)

- `mUseUBO=false` default で **既存 OpenGL 描画 100% 維持** (= `recordGltfAssetDraw` は Vulkan placeholder offscreen FBO 経路、mUseUBO 不問だが `AYAGltfStubDrawEnabled=false` default で発火しない)
- `AYAGltfStubDrawEnabled=false` default で発火なし = `recordAvatarPlaceholderDraw` 単独経路維持 = PC-N-3 完了状態と機能等価
- `AYAGltfStubDrawEnabled=true` 時のみ `recordGltfAssetDraw` 並走発火 = debug live A/B 経路
- 3 段 graceful degrade (block lookup miss / register fail / wire fail) で fallback

---

## §5. design-lock Exit Criteria 9 項 全 ✅

| # | Criteria | 充足 |
|---|----------|------|
| (i) | PC-N-5 literal scope 5 件 §0 明文化 ((N5-1) A + (N5-3) A + (N5-5) A 採用後) | ✅ §0 |
| (ii) | 必読 1 件 §1 + pinpoint reference 12 件別記 | ✅ §1 |
| (iii) | ambiguity 10 件 + AYA literal「OK」record (2026-06-05) | ✅ §3 |
| (iv) | 採用案根拠 10 件明文化 | ✅ §3 |
| (v) | 実装計画 (a)-(g) 7 step 分解 | ✅ §4 |
| (vi) | 実装 phase Exit Criteria 10 項明文化 | ✅ §6 |
| (vii) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 | ✅ §4.8 |
| (viii) | MUSEUBO-A 整合 = `mUseUBO=false` default 描画 100% 維持 + `AYAGltfStubDrawEnabled=false` default で stub draw 発火なし | ✅ §4.9 |
| (ix) | `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件 = `feedback_design_phase_no_code_write` 整合 | ✅ §6.2 |

---

## §6. 実装 phase Exit Criteria 10 項 (= 次 session 着手用)

| # | Criteria |
|---|----------|
| (i) | `sGltfStubSkin` sentinel storage 新設 (`anonymous namespace`, `sPlaceholderSkin` 直後) ((N5-3) A) |
| (ii) | `recordGltfAssetDraw` 関数新設 (= `recordAvatarPlaceholderDraw` 同形 pattern、identity matrix + writeSkinUbo + flushSkinUbos + bindV3aRigged + vkCmdDraw + first-fire LL_INFOS marker) ((N5-1) A + (N5-5) A + (N5-6) A) |
| (iii) | `initVulkan` 内 stub Skin register + 初回 wire (3 段 graceful degrade) ((N5-3) A + (N5-7) A) |
| (iv) | `shutdownVulkan` 内 stub Skin unregister (= 対称 lifecycle) |
| (v) | `AYAGltfStubDrawEnabled` cvar 新設 (Boolean default `false` Persist=1) + `recordGltfAssetDraw` 発火 hook (LLCachedControl pattern、`recordAvatarPlaceholderDraw` 直後並走) ((N5-4) A) |
| (vi) | identity matrix bone data writeSkinUbo + flushSkinUbos + bindV3aRigged 正規 sequence (= design 06b §2.5 GLTFSceneManager::render 直前 pattern 踏襲) ((N5-6) A + (N5-7) A) |
| (vii) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= cvar 切替で gate、runtime のみ) |
| (viii) | MUSEUBO-A 整合 = `mUseUBO=false` default + `AYAGltfStubDrawEnabled=false` default で stub draw 発火なし、PC-N-3 完了状態と機能等価 |
| (ix) | build verify llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131 全 PASS ((N5-9) A) |
| (x) | tag block 統一 PC-N-5 (a)/(b)/(c)/(d)/(e) + first-fire LL_INFOS marker + handoff complete doc 起案 |

---

## §6.2 想定改変 file (= 実装 phase = 別 session)

| # | file | 改変内容 |
|---|------|---------|
| 1 | `indra/llrender/llvkloader.cpp` | (a) `sGltfStubSkin` sentinel storage 新設 + (b) `recordGltfAssetDraw` 関数新設 + (c) `initVulkan` 内 stub Skin register + 初回 wire + (d) `shutdownVulkan` 内 stub Skin unregister + (e) cvar 発火 hook = 5 編集 step |
| 2 | `indra/newview/app_settings/settings.xml` | (e) `AYAGltfStubDrawEnabled` Boolean cvar 1 件追加 (= default `false` Persist=1) |
| 3 | `indra/llrender/llvkloader.h` | 改変 0 件想定 (= `sGltfStubSkin` + `recordGltfAssetDraw` は anonymous namespace 内 file-static + helper、`LLVKLoader::` 公開 API 不変) |
| 4 | codegen (`scripts/ubo_codegen` + `build-linux-x86_64/codegen/`) | 改変 0 件 (= 既存 `Skin_GLTFJoints` block 再利用、PC-7γ-3 で配置済) |
| 5 | shader (`*.glsl` + `aya_r41_blueprints/`) | 改変 0 件 (= GLSL UBO layout は既存 `Skin_GLTFJoints` 再利用) |
| 6 | GLTFSceneManager / pipeline.cpp / lldrawpoolpbropaque.cpp | 改変 0 件 ((N5-8) A 採用、PC-N-5 = llrender 層完結) |

---

## §7. 着手手順 (= 次 session で PC-N-5 実装着手)

1. AYA 指示「PC-N-5 実装着手お願いします」literal 受領待ち
2. **本 PC-N-5 design-lock doc 全文 Read** (= 必読 1 件)
3. **pinpoint reference 12 件** §1 から Read (= PC-N-3 complete §2.7 + §2.1 各 site + wireSkinUboSetV3aToBinding2 helper + writeSkinUbo/flushSkinUbos/registerSkinUbo signature + ubo_metadata.inl + bindV3aRigged signature + design 06b §2.5 + design 06c §2.5 + GATE-B literal + cross-platform spec §6)
4. **`recordAvatarPlaceholderDraw` source (line 5395-5480) を pinpoint Read** = `recordGltfAssetDraw` 同形踏襲 source として詳細確認 (= push constant setup + `V3A_DRAW_SET_BINDINGS` 定数 + `bindV3aRigged` 引数 calling convention)
5. **`recordAvatarPlaceholderDraw` caller site grep** = `AYAGltfStubDrawEnabled` cvar 発火 hook 配置 site 確定
6. step (a)-(g) 7 step 実装
7. build verify (= llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131)
8. handoff complete doc 起案 + Exit Criteria 10 項 self-verify + AYA commit 指示後 commit

---

## §8. 残 strict 線形

**Phase 1.C complete** ✅ (= PC-N-3 commit `71f7bb2a89`)
**PC-8 Linux primary marker** ✅ (= commit `3c0c72d34f`、Phase 1.C strict 線形終了)
**PC-N-5 design-lock** ✅ 本 commit (= Phase 1.D 着手起点 design-lock)

次:

- **PC-N-5 実装** ⏳ 次 session (= step (a)-(g) 7 step 実施 + Exit Criteria 10 項 self-verify + handoff complete doc 起案)
- ... (= Phase 1.D 内後続 sub-step 群 = PC-N-6 仮定以降、PC-N-5 complete 後の design-lock phase で別途分解)
- **Phase 1 全完了** → **Mac/Win 開発者補完 phase** (= `ayastorm-r41-cross-platform-port-spec.md` 確定形提供)

---

## §9. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1 design-lock ✅ + PC-N-1 ✅ + PC-N-2 design-lock ✅ + PC-N-2 ✅ + PC-N-4 design-lock ✅ + PC-N-4 ✅ + PC-N-3 design-lock ✅ + PC-N-3 ✅ = **Phase 1.C complete ✅** + PC-8 Linux primary marker ✅ = **Phase 1.C strict 線形終了 ✅** + **PC-N-5 design-lock ✅ 本 commit = Phase 1.D 着手起点 design-lock** + PC-N-5 実装 ⏳ 次 session + Phase 1.D 内後続 ⏳

---

## §10. self-verify 9 観点 全 ✅

1. **PC-N-5 literal scope 5 件 §0 完全分解** ((N5-1) A + (N5-3) A + (N5-5) A 採用後、(a) sGltfStubSkin sentinel storage + (b) recordGltfAssetDraw 関数 + (c) initVulkan register + 初回 wire + (d) shutdownVulkan unregister + (e) AYAGltfStubDrawEnabled cvar + 発火 hook) ✅
2. **必読 1 件 §1 + pinpoint reference 12 件別記** (PC-N-3 complete §2.7 + §2.1 各 site + wireSkinUboSetV3aToBinding2 + writeSkinUbo/flushSkinUbos/registerSkinUbo + ubo_metadata.inl + bindV3aRigged + design 06b §2.5 + design 06c §2.5 + GATE-B literal + cross-platform spec §6) ✅
3. **現状調査 §2 10 項網羅** (= design doc 4 件 + GLTF render path 3 件 + llvkloader 4 件 + cvar 4 件 + sSkinUboDirty 3 件) ✅
4. **ambiguity (N5-1)..(N5-10) 10 件 AYA literal「OK」record (2026-06-05)** §3 + 採用根拠 10 件明文化 ✅
5. **実装計画 (a)-(g) 7 step 分解** §4 + 各 step に具体 code stub example 添付 ✅
6. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件** §4.8 (= cvar 切替で runtime gate のみ) ✅
7. **MUSEUBO-A 整合 = `mUseUBO=false` default + `AYAGltfStubDrawEnabled=false` default 経路不変** §4.9 (= PC-N-3 完了状態と機能等価、3 段 graceful degrade) ✅
8. **design-lock Exit Criteria 9 項 §5** + **実装 phase Exit Criteria 10 項 §6** 明文化 ✅
9. **`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件** = `feedback_design_phase_no_code_write` 整合 ✅ (= cross-platform spec §6 PC-N-5 行追記 + 本 design-lock doc 1 件起案のみ)

---

## §11. 次 session 着手 1 line

**PC-N-5 実装着手** = step (a)-(g) 7 step 実施 = (a) `sGltfStubSkin` sentinel storage 新設 + (b) `recordGltfAssetDraw` 関数新設 + (c) `initVulkan` 内 stub Skin register + 初回 wire + (d) `shutdownVulkan` 内 stub Skin unregister + (e) `AYAGltfStubDrawEnabled` cvar 新設 (settings.xml) + 発火 hook 配線 + (f) build verify (= llrender + warning 0 + TUT 11+10+13 + codegen 131/131) + (g) handoff complete doc 起案 + Exit Criteria 10 項 self-verify + AYA commit 指示受領後 commit。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 PC-N-5 design-lock handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 次 session 必読 1 件 + pinpoint reference 12 件別記、本 session も Read pinpoint のみ (= PC-8 marker doc 全文 + cross-platform spec 全文 + PC-N-3 complete §2.7 + PC-N decomposition §5 + Explore agent pinpoint 報告)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §10
- **feedback_build_only_verified** 遵守 = design-lock phase は `indra/` 改変 0 件で build verify 対象外、実装 phase で literal 検証取得予定 ((N5-9) A 採用)
- **feedback_no_scope_shrink** 遵守 = PC-N-5 literal scope 5 件 §0 完全分解、(N5-1) A 採用は「実 GLTF Vulkan draw」literal 解釈確定ゆえ縮小ではない (= AYA literal「OK」record 済段階分離)、(N5-1) B (= 実 LL::GLTF::Asset 経由 vertex buffer upload) + (N5-1) C (= GLTFSceneManager::render 内 mUseUBO gate 配線) は Phase 1.D 内後続 sub-step 持越 = `feedback_ubo_migration_one_at_a_time` 厳格遵守整合
- **feedback_doubt_self_first** 遵守 = ambiguity 10 件発見で停止 + 推奨案提示 + AYA literal「OK」record 後本 design-lock doc 起案、推測実装なし、特に (N5-1) literal 解釈の核心 ambiguity は 3 候補全列挙 + 根拠明示後 AYA 確認
- **feedback_confirm_referent_before_acting** 遵守 = 10 件 batch AYA 確認 (2026-06-05)、各候補 + 推奨案 + 根拠明示後 AYA literal「OK」record 受領で確定、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N-5 = 実 GLTF Vulkan draw 通電 1 stub 単独 sub-step (= 第 2 sentinel-like skin + recordGltfAssetDraw 新設 + cvar 切替)、Phase 1.D 内後続 sub-step (= PC-N-6 仮定以降、実 LL::GLTF::Asset 経由 vertex buffer + GLTFSceneManager::render 統合) は分離、本 doc 起案も PC-N-5 単独 design-lock のみ
- **feedback_design_phase_no_code_write** 厳格遵守 = 本 PC-N-5 design-lock phase は doc 起案のみ、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 + settings.xml 改変 0 件
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示後 commit 予定
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在予定
- **feedback_no_bare_reference_ids** 遵守 = (N5-1)..(N5-10) 各 ID に項目名 / 採用案内容併記 §3 + (a)..(g) 各 step に作業内容併記 §4
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、`git add` 個別 file 指定 + `git add -A` 不使用予定
- **memory `project_ayastorm_r41_design_principles`** 整合 = (1) Upstream OpenGL 取り込みやすさ維持 = `GLTFSceneManager::render` 改変 0 件 ((N5-8) A 採用) + (2) Core プロセス分散実現 = UBO/cmdbuf 並列化容易な設計 = sentinel + stub Skin* 並走 baseline (= 後段の per-Skin parallelization 基盤)
- **memory `project_r41_phase1b_vulkan_host_gate`** 整合 = GATE-B = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件、cvar runtime gate のみ
- **memory `project_ayastorm_three_platforms`** 整合 = cross-platform spec §6 PC-N-5 行追記で macOS / Windows 派生 fix 候補欄起案、Linux primary 完成 → 他者補完 model と整合

---
