# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-N-3 design-lock complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-N-3 (= avatar bone storage 経路再配線 = H10-A 持越 = push descriptor disable 状態下で set=3 binding=2 = `Skin_GLTFJoints` 経由再 wire + placeholder skin sentinel 経由 `writeSkinUbo` + `flushSkinUbos` 通電 + PC-7δ `sAvatarBoneStorageBuffer` static deprecate) の **design-lock phase 完了** marker = ambiguity (N3-1)..(N3-11) 11 件 + 重要 gap G-1 (= PC-N decomposition §4.4 文言「`writeAvatarBoneStorage` 新設」vs design 06b/06c §2.5 確定 + 実装現状 `writeSkinUbo` PC-7γ-2 既存) 全 AYA literal「推奨案採用 OK」record (2026-06-05) + 実装計画 (a)-(g) 7 step 分解 + Exit Criteria 10 項明文化。`indra/` 改変 0 件 (= `feedback_design_phase_no_code_write` 整合)。

> **本 doc 位置付け**: PC-N-4 complete (= `206db3a7c6`) 後の PC-N-3 design-lock phase。`writeAvatarBoneStorage` は **新設不要** (= G-1 解消方針 A 採用) = 既存 `writeSkinUbo` (PC-7γ-2 実装済) + `flushSkinUbos` (PC-7γ-2 実装済) + design 06b/06c §2.5 確定経路を通電。PC-7γ-3 で blueprint + codegen 配置済 (= `ubo_layout_skin_gltfjoints.inl` 既存、N3-3 B 採用ゆえ codegen 改変 0 件)。次 strict 線形 = PC-N-3 実装 = **Phase 1.C complete** → PC-8 (3 OS build verify) → PC-N-5 = Phase 1.D 着手起点。

---

## §0. PC-N-3 literal scope (4 件 = G-1 + (N3-1)..(N3-11) AYA literal「推奨案採用 OK」record 後確定)

1. **`sAvatarBoneStorageBuffer` + `sAvatarBoneStorageAllocation` + `sAvatarBoneStorageMapped` + `allocateAvatarBoneStorageBuffer()` deprecate** ((N3-1) A) = PC-7δ で配置済 7040 B static mapped buffer 撤去、置換は per-Skin `UboInstance` 経由 (= design 06b §2.5 正準)
2. **placeholder skin sentinel 新設 + `initVulkan` 内 `registerSkinUbo(&sPlaceholderSkin, ubo::block_hash::Skin_GLTFJoints, ...)` + `unloadInternal` 内 `unregisterSkinUbo`** ((N3-2) A + (N3-6) A + (N3-7) A) = `recordAvatarPlaceholderDraw` の placeholder phase で `LL::GLTF::Skin*` が nullptr ゆえ sentinel skin で `sSkinUboDirty` entry 確保
3. **`wireSkinUboSetV3aToBinding2(LL::GLTF::Skin* skin)` helper 新設 + `initVulkan` で初回 wire** ((N3-4) A) = `sAssetUboSetV3a[frame]` の binding=2 を `sSkinUboDirty[key].buffer[frame]` に `vkUpdateDescriptorSets`、`wireDrawUboSetV3aToRingBuffer` (PC-7ε) 同形 pattern
4. **`recordAvatarPlaceholderDraw` 内 `writeSkinUbo` + `flushSkinUbos` 挿入** ((N3-8) A) = `bindV3aRigged` 呼出前に `writeSkinUbo(&sPlaceholderSkin, ubo::block_hash::Skin_GLTFJoints, 0, identity_or_zero_buf, size)` → `flushSkinUbos(&sPlaceholderSkin)` → bind の正規 sequence (= write → flush → bind)

**codegen 改変 0 件** ((N3-3) B 採用) = PC-7γ-3 で `aya_r41_blueprints/set3/skin_gltf_joints.glsl` blueprint + `build-linux-x86_64/codegen/ubo/ubo_layout_skin_gltfjoints.inl` 配置済、`ubo::block_hash::Skin_GLTFJoints` constexpr U32 既存、`llglslshader.h:169` `UB_GLTF_JOINTS` enum + `llglslshader.cpp:1961` `"Skin_GLTFJoints"` name mapping 既存ゆえ追加配置不要。

**`bindV3aRigged` signature 不変** ((N3-9) A) = per-Skin `UboInstance` は STATIC `UNIFORM_BUFFER` (= dynamic offset 不要、register 時に固定 `VkBuffer` × `FRAMES_IN_FLIGHT`)、wire は `initVulkan` + 書き換え後 `flushSkinUbos` 経由 descriptor update 完結、`bindV3aRigged` 第 2 call (set=3 swap) で `sAssetUboSetV3a[frame]` bind するだけで OK。

---

## §1. 必読 1 件 + pinpoint reference (実装 phase = 次 session 向け)

**次 session 必読**:

1. **本 PC-N-3 design-lock doc 全文** = `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-n-3-design-lock.md`

**pinpoint reference** (実装 phase で必要分のみ):

- **PC-N-4 complete doc**: `handoff-...-pc-n-4-complete.md` §0 + §5 (= grow re-wire hook 通電状態 = PC-N-3 着手 baseline)
- **PC-N decomposition design-lock**: `handoff-...-pc-n-decomposition-design-lock.md` §4.4 (= PC-N-3 旧 scope、G-1 解消方針で更新済)
- **design 06b §2.5**: `docs/specs/ayastorm-r41-gl-removal/design/06b-cadence-update-site-and-dirty.md` line 108-118 (= per-skin cadence 確定 = `writeSkinUbo` + `flushSkinUbos` + `gltf::Skin::updateTransforms()` 配置点)
- **design 06c §2.5**: `docs/specs/ayastorm-r41-gl-removal/design/06c-descriptor-set-bind-wiring.md` line 121-132 + line 186 (= set=3 binding=2 = `Skin_GLTFJoints` 確定)
- **既存 `writeSkinUbo` 実装**: `indra/llrender/llvkloader.cpp:5110-5140` (= PC-7γ-2 で実装済、`UboSkinKey = <Skin*, block_hash>` 経由 `UboInstance.mapped_ptr[sFrameIndex]` 直書 + dirty.store)
- **既存 `flushSkinUbos` 実装**: `indra/llrender/llvkloader.cpp:4504-4524` (= PC-7γ-2 で実装済、現状 `sSkinUboDirty` 0 件ゆえ `flushDummyUboWrite` 呼出のみ)
- **既存 `registerSkinUbo` / `unregisterSkinUbo` 実装**: `indra/llrender/llvkloader.cpp:5035+ / :5094+` (= PC-7γ-2 で実装済、`UboSkinKey = <Skin*, block_hash>` 単位 entry 確保)
- **`recordAvatarPlaceholderDraw` 現状**: `indra/llrender/llvkloader.cpp:5278-5365` (= PC-N-2 で `bindV3aRigged` set=2 復活通電済、PC-N-3 で `writeSkinUbo` + `flushSkinUbos` 挿入 site)
- **`sAvatarBoneStorageBuffer` 系 declaration**: `indra/llrender/llvkloader.cpp:97-109` (= PC-7δ 配置の static 7040B mapped buffer、PC-N-3 で deprecate 対象) + `allocateAvatarBoneStorageBuffer()` impl site (= grep 要)
- **`wireDrawUboSetV3aToRingBuffer` template**: `indra/llrender/llvkloader.cpp:2667-2704` (= PC-7ε + PC-N-4 同形 helper、PC-N-3 で `wireSkinUboSetV3aToBinding2` 新設の template)

---

## §2. 現状調査 (= AYA literal「推奨案採用 OK」record 前の事実確認)

### §2.1 code 現状 11 項表

| # | 項目 | 場所 | 内容 |
|---|------|------|------|
| 1 | `writeSkinUbo(Skin*, block_hash, offset, data, size)` | `llvkloader.cpp:5110-5140` | PC-7γ-2 で実装済、`sSkinUboDirty[UboSkinKey].mapped_ptr[sFrameIndex]` 直書 + dirty.store。defensive 配線 (= sSkinUboDirty 0 件で実走しない) |
| 2 | `flushSkinUbos(Skin*)` | `llvkloader.cpp:4504-4524` | PC-7γ-2 で実装済、`sSkinUboDirty` walk + dirty exchange、現状 `flushDummyUboWrite("flushSkinUbos")` 呼出のみ |
| 3 | `registerSkinUbo(Skin*, block_hash, block_size)` | `llvkloader.cpp:5035+` | PC-7γ-2 で実装済、`UboSkinKey` 単位 entry 確保 + `UboInstance` allocate (VkBuffer × FRAMES_IN_FLIGHT) |
| 4 | `unregisterSkinUbo(Skin*, block_hash)` | `llvkloader.cpp:5094+` | PC-7γ-2 で実装済、entry destroy + erase |
| 5 | `sSkinUboDirty` map | `llvkloader.cpp:556` | `std::unordered_map<UboSkinKey, UboInstance, UboSkinKeyHash>`、現状 0 件 |
| 6 | `sAvatarBoneStorageBuffer` + `Allocation` + `Mapped` | `llvkloader.cpp:97-109` | PC-7δ 配置の 7040B static mapped、descriptor wiring 抜き済 = H10-A 持越、PC-N-3 で deprecate |
| 7 | `allocateAvatarBoneStorageBuffer()` | `initVulkan` 内 (要 grep) | PC-7δ で配置、PC-N-3 で撤去対象 |
| 8 | `ubo::block_hash::Skin_GLTFJoints` constexpr | `build-linux-x86_64/codegen/ubo/ubo_perfect_hash.inl` | PC-7γ-3 で codegen 配置済、`fnv1a_32("Skin_GLTFJoints")` 値 (perfect_hash.py:324) |
| 9 | `ubo_layout_skin_gltfjoints.inl` | `build-linux-x86_64/codegen/ubo/` | PC-7γ-3 で配置、block 内 layout (size 等) emit |
| 10 | `recordAvatarPlaceholderDraw` (PC-N-2 後) | `llvkloader.cpp:5278-5365` | `writeDrawUbo(PerDrawUBO_LightParams) → dynamic_offsets[4] → bindV3aRigged(set=0/1a/1b/2 + set=3 swap)` 完了状態 |
| 11 | `bindV3aRigged` (PC-N-2 後) | `llvkloader.cpp:1797-1839` | 4 set bind (set=0/1a/1b/2) + 第 2 call set=3 swap = `sAssetUboSetV3a[frame]` bind |

### §2.2 design doc 確定状態 3 件

- **design 06b §2.5** (line 108-118): per-skin cadence = `writeSkinUbo` + `flushSkinUbos` + `gltf::Skin::updateTransforms()` 配置点 + `Skin_GLTFJoints` UBO 確定
- **design 06c §2.5** (line 121-132): set=3 binding=2 = `Skin_GLTFJoints` (= `UB_SKIN_GLTF_JOINTS` = 旧 `UB_GLTF_JOINTS`) per-skin owner、flushSkinUbos 経由
- **design 09 Phase 5**: per-Skin 系 UBO (`UB_GLTF_JOINTS`) = rigged GLTF avatar attachment、PC-N-3 で placeholder 通電 + PC-N-5 で実 GLTF 投入

### §2.3 G-1 重要 gap 別記

**観察**:

- **PC-N decomposition §4.4 文言** (2026-06-05 早朝起案): 「`writeAvatarBoneStorage(skin_hash, mat4*, count)` helper 新設」
- **design 06b §2.5 / 06c §2.5 確定 (= 2026-06-04 確定済)**: `writeSkinUbo(Skin*, block_hash, offset, data, size)` + `flushSkinUbos(Skin*)` + set=3 binding=2 = `Skin_GLTFJoints`
- **実装現状 (PC-7γ-2 = 2026-06-04+)**: `writeSkinUbo` + `flushSkinUbos` + `registerSkinUbo` + `unregisterSkinUbo` 全て実装済 (defensive 配線、cadence_tag=4 codegen 0 件で実走しない状態)
- **PC-7γ-3 (= 2026-06-05 前半)**: `Skin_GLTFJoints` blueprint + codegen 出力配置済 (= `ubo_layout_skin_gltfjoints.inl`)

**G-1 解消方針 (= AYA literal「推奨案採用 OK」record 2026-06-05)**: 後発確定の design 06b/06c §2.5 + PC-7γ-2/3 実装現状を正準採用、PC-N decomposition §4.4 文言は暫定文言ゆえ撤回、PC-N-3 正準 scope は「**既存 `writeSkinUbo` + `flushSkinUbos` 通電 + descriptor wire helper 新設 + `sAvatarBoneStorageBuffer` deprecate + placeholder skin sentinel 経路追加**」と再解釈。

---

## §3. ambiguity (N3-1)..(N3-11) + G-1 AYA literal「推奨案採用 OK」record (2026-06-05)

| ID | 項目 | 推奨案 | AYA OK record | 採用根拠 |
|----|------|--------|---------------|---------|
| **G-1** | `writeAvatarBoneStorage` 新設 vs `writeSkinUbo` 既存 | **A**: design 06b/06c 正準採用 = `writeSkinUbo` + `flushSkinUbos` 既存 path 通電、新設不要 | OK (2026-06-05) | design 06b §2.5 + 06c §2.5 が PC-N decomposition より後発の正準確定文書、PC-7γ-2 で `writeSkinUbo` + `flushSkinUbos` 既に実装済、二重路線回避、`feedback_ubo_migration_one_at_a_time` 整合 |
| **N3-1** | `sAvatarBoneStorageBuffer` 取扱 | **A**: PC-N-3 で deprecate = `sAvatarBoneStorageBuffer` + `Allocation` + `Mapped` 撤去 + `allocateAvatarBoneStorageBuffer()` 撤去 | OK (2026-06-05) | design 06b §2.5 + 06c §2.5 確定経路 = per-Skin `UboInstance` (= `sSkinUboDirty` 経由)、PC-7δ static buffer は H10-A 持越 placeholder、PC-N-3 で正規経路統合と同時撤去、残置は twin path 並走で複雑度増 |
| **N3-2** | placeholder 経路 set=3 binding=2 wire 方法 | **A**: `sSkinUboDirty` に dummy sentinel skin entry register、`recordAvatarPlaceholderDraw` 内で `writeSkinUbo(sentinel, Skin_GLTFJoints, 0, zero_buf, size)` → `flushSkinUbos(sentinel)` → bind の正規 sequence | OK (2026-06-05) | PC-N-1 `recordPlaceholderPoolDraw` 内 `PerDrawUBO_LightParams` 経由 `writeDrawUbo` 通常 path 通電と同形、defensive + 整合性最大、sentinel 1 個分の per-Skin UboInstance allocate は最小 cost (= 7040 B × 2 frames = 14 KB 程度) |
| **N3-3** | `UB_SKIN_GLTF_JOINTS` codegen 配置範囲 | **B**: codegen 改変 0 件 (= 既存 `Skin_GLTFJoints` block 活用、PC-7γ-3 で配置済) | OK (2026-06-05、推奨案 C → B 事実訂正 record) | PC-7γ-3 blueprint + codegen 配置済を Read で確認、`ubo::block_hash::Skin_GLTFJoints` constexpr U32 既存、`ubo_layout_skin_gltfjoints.inl` 既存、二重配置回避 |
| **N3-4** | descriptor wire helper 新設 | **A**: `wireSkinUboSetV3aToBinding2(LL::GLTF::Skin* skin)` helper 新設、`registerSkinUbo` 後 + 初回 wire (`initVulkan`) + ((skin が register された全 frame に対し)) descriptor update | OK (2026-06-05) | `wireDrawUboSetV3aToRingBuffer` (PC-7ε) 同形 pattern 踏襲、helper 独立で test 容易、UboInstance は static (= grow re-wire 不要、PC-N-4 hook 不要) |
| **N3-5** | `UB_SKIN_GLTF_JOINTS` block_hash 定数の存在 | **既存確認済**: `llglslshader.h:169` `UB_GLTF_JOINTS` enum + `llglslshader.cpp:1961` `"Skin_GLTFJoints"` name mapping + `ubo::block_hash::Skin_GLTFJoints` codegen 出力 | OK (2026-06-05) | PC-7γ-3 で配置確認、enum + name + block_hash 全て揃っているゆえ追加配置不要 |
| **N3-6** | placeholder skin sentinel storage 形式 | **A**: anonymous namespace 内 `static LL::GLTF::Skin sPlaceholderSkin{}` (= default-constructed instance、addr が sentinel) | OK (2026-06-05) | default 初期化で安全、`writeSkinUbo` + `flushSkinUbos` の `sSkinUboDirty[key]` 触っても OK、uninitialized addr は touch で crash |
| **N3-7** | `registerSkinUbo` 呼出 site (placeholder) | **A**: `initVulkan` 内 1 回呼出 = `registerSkinUbo(&sPlaceholderSkin, ubo::block_hash::Skin_GLTFJoints, block_size)`、`unloadInternal` で `unregisterSkinUbo(&sPlaceholderSkin, ubo::block_hash::Skin_GLTFJoints)` | OK (2026-06-05) | lifecycle 明確 + 既存 `register*Ubo` pattern 同形 (PC-7γ-2 確立)、lazy register は複雑度高 |
| **N3-8** | `flushSkinUbos` placeholder skin 通電 site | **A**: `recordAvatarPlaceholderDraw` 内 `writeSkinUbo` 直後 + `bindV3aRigged` 呼出前に `flushSkinUbos(&sPlaceholderSkin)` 挿入 (= write → flush → bind 正規 sequence) | OK (2026-06-05) | design 06b §2.5「`flushSkinUbos(skin)` を `GLTFSceneManager::render(variant)` 直前」pattern 整合 (= `recordAvatarPlaceholderDraw` が placeholder phase の `GLTFSceneManager::render` 相当) |
| **N3-9** | `bindV3aRigged` signature 変更要否 | **A**: 不変 = 第 2 call (set=3 swap) で `sAssetUboSetV3a` bind、その binding=2 は wire helper で確定済 | OK (2026-06-05) | per-Skin UboInstance は STATIC UNIFORM_BUFFER (= dynamic offset 不要)、wire は `initVulkan` + `flushSkinUbos` 経由 descriptor update 完結、`bindV3aRigged` 変更不要 |
| **N3-10** | build verify scope | **A**: llrender + WARNING 0 + TUT 11+10+13 + codegen **131/131** ((N3-3) B 採用ゆえ 132 ではなく 131 維持) | OK (2026-06-05) | PC-N-1/2/4 同形 pattern 踏襲、Linux primary、PC-8 で 3 OS 集約 |
| **N3-11** | PC-N-3 Exit Criteria 項目数 | **A**: 10 項 | OK (2026-06-05) | PC-N-1/2/4 同形 template 流用 |

---

## §4. 実装計画 (a)-(g) 7 step 分解

### §4.1 step (a) — `sAvatarBoneStorageBuffer` 系 deprecate ((N3-1) A)

- `llvkloader.cpp:97-109` 内 `sAvatarBoneStorageBuffer` + `sAvatarBoneStorageAllocation` + `sAvatarBoneStorageMapped` declaration 撤去 (= 3 line)
- `allocateAvatarBoneStorageBuffer()` 関数 body + 呼出 site (= `initVulkan` 内) 撤去
- `unloadInternal` 内 `sAvatarBoneStorageBuffer` 解放 site も撤去
- tag block `<AYAstorm r41 PC-N-3 (a)>` 配置、撤去 site に「PC-N-3 で per-Skin `UboInstance` (= `sSkinUboDirty`) 経路に統合、`writeSkinUbo` + `flushSkinUbos` 通電」comment 残置
- `AVATAR_BONE_MATRIX_COUNT` 定数 (= 110) は `Skin_GLTFJoints` block_size 算出に流用予定ゆえ保持判定 (= `Skin_GLTFJoints` blueprint で `vec4 gltf_joints[1024]` = upper bound 16384 B 固定、110 joint = 110 × 3 vec4 = 330 vec4 = 5280 B 実使用)、撤去か残置かは実装時 codegen 出力 size 参照後判断

### §4.2 step (b) — placeholder skin sentinel 新設 ((N3-6) A)

- anonymous namespace 内 `static LL::GLTF::Skin sPlaceholderSkin{};` 新設 (= default-constructed instance)
- tag block `<AYAstorm r41 PC-N-3 (b)>` 配置、comment「placeholder phase で `LL::GLTF::Skin*` nullptr を回避するための sentinel、addr (= `&sPlaceholderSkin`) を `sSkinUboDirty` の `UboSkinKey` 第 1 要素として使用、PC-N-5 で実 GLTF avatar 投入時に並走」記載
- `LL::GLTF::Skin` の header include 要確認 (= `gltf/animation.h` 等)

### §4.3 step (c) — `initVulkan` register + `unloadInternal` unregister ((N3-7) A)

- `initVulkan` 内 (= `sAssetUboLayoutV3a` + `sAssetUboSetV3a` 初期化後の適切位置) で `registerSkinUbo(&sPlaceholderSkin, ubo::block_hash::Skin_GLTFJoints, block_size)` 呼出
- `block_size` は codegen 出力 `ubo_layout_skin_gltfjoints.inl` 内の `kSkinGLTFJointsSize` (or 類似) 定数参照 (= 16384 B std140 upper bound、実装時 grep 確認)
- `unloadInternal` 内で `unregisterSkinUbo(&sPlaceholderSkin, ubo::block_hash::Skin_GLTFJoints)` 呼出
- 失敗時 fallback = `LL_WARNS_ONCE` + 続行 (= `recordAvatarPlaceholderDraw` 内 sentinel register 不在時の `writeSkinUbo` 早期 return で graceful degrade)
- tag block `<AYAstorm r41 PC-N-3 (c)>` 配置

### §4.4 step (d) — `wireSkinUboSetV3aToBinding2` helper 新設 + `initVulkan` で初回 wire ((N3-4) A)

- anonymous namespace 内 helper `bool wireSkinUboSetV3aToBinding2(LL::GLTF::Skin* skin)` 新設
- body 構造 (= `wireDrawUboSetV3aToRingBuffer` PC-7ε 同形):
  - guard: `sDevice` + `skin` + `sAssetUboSetV3a[frame_index]` 確認
  - `UboSkinKey key{skin, ubo::block_hash::Skin_GLTFJoints}` で `sSkinUboDirty.find(key)` lookup、miss は `LL_WARNS_ONCE` + return false
  - `UboInstance& ubo_inst = it->second;` 経由 `ubo_inst.buffer[frame_index]` 取得
  - `FRAMES_IN_FLIGHT` frame 分 vkUpdateDescriptorSets ループ実行:
    - `VkDescriptorBufferInfo binfo = { ubo_inst.buffer[frame], 0, VK_WHOLE_SIZE };`
    - `VkWriteDescriptorSet write` (`dstSet = sAssetUboSetV3a[frame]`, `dstBinding = 2`, `descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER`, `pBufferInfo = &binfo`)
    - `vkUpdateDescriptorSets(sDevice, 1, &write, 0, nullptr)`
  - return true
- 呼出 site 1: `initVulkan` 内 register 直後 (= step (c) 直後) で初回 wire
- grow re-wire は不要 (= UboInstance は static、`writeDrawUbo` ring buffer grow とは別構造) ゆえ PC-N-4 hook 不要
- tag block `<AYAstorm r41 PC-N-3 (d)>` 配置 + first-wire `LL_INFOS` marker

### §4.5 step (e) — `recordAvatarPlaceholderDraw` 内 write → flush → bind 配線 ((N3-8) A)

- `llvkloader.cpp:5278-5365` 内 `recordAvatarPlaceholderDraw` の `bindV3aRigged(cmd_buf, sFrameIndex, dynamic_offsets)` 呼出前に挿入:
  - `static const U8 placeholder_skin_buf[XXX] = {};` (= zero buf、size = block_size、identity matrix が必要なら別途、placeholder phase は zero で経路通電のみ)
  - `LLVKLoader::writeSkinUbo(&sPlaceholderSkin, ubo::block_hash::Skin_GLTFJoints, 0, placeholder_skin_buf, sizeof(placeholder_skin_buf));`
  - `LLVKLoader::flushSkinUbos(&sPlaceholderSkin);`
- 直後の既存 `bindV3aRigged` 呼出 で set=3 swap call が `sAssetUboSetV3a[sFrameIndex]` bind ⇒ binding=2 = `Skin_GLTFJoints` が `sPlaceholderSkin` 経由 wire 済の `UboInstance.buffer[sFrameIndex]` 参照 = 経路通電
- tag block `<AYAstorm r41 PC-N-3 (e)>` 配置、旧 PC-N-2 (c)/(d) tag block は historical record として保持、新 tag block 追加形
- first-fire `LL_INFOS` marker 「`PC-N-3` placeholder skin sentinel writeSkinUbo + flushSkinUbos 通電済 (set=3 binding=2 Skin_GLTFJoints, sPlaceholderSkin sentinel経路)」literal 追加

### §4.6 step (f) — build verify ((N3-10) A)

- `cd build-linux-x86_64 && make -j4 llrender` PASS + ERROR 0 / WARNING 0
- `INTEGRATION_TEST_lluboringbuffer` 11/11 PASS YAY (= PC-3 algorithm 層 regression なし)
- `INTEGRATION_TEST_llassetubopool` 10/10 PASS YAY (= PC-4 algorithm 層 regression なし)
- `INTEGRATION_TEST_llpipelinecachestorage` 13/13 PASS YAY (= PC-5 algorithm 層 regression なし)
- `scripts/ubo_codegen` unittest 131/131 PASS (= codegen 改変 0 件ゆえ regression なし)

### §4.7 step (g) — handoff complete doc 起案

- `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-n-3-complete.md` 起案
- Exit Criteria 10 項 self-verify 全 ✅ 明文化
- AYA commit 指示受領後 commit (= `feedback_no_auto_commit`)
- commit message に Co-Authored-By 行不在 (= `feedback_no_claude_coauthor`)
- `git add` 個別 file 指定 (= `feedback_tests_dir_never_commit`)

### §4.8 GATE-B 整合

`#ifdef LL_VULKAN_GLSL` 新規追加 0 件:

- (a) deprecate は declarations 撤去 (= GLSL 関係なし)
- (b) sentinel skin は anonymous namespace 内 C++ 変数
- (c) register/unregister は既存 `registerSkinUbo` / `unregisterSkinUbo` 呼出 (= 内部に `LL_VULKAN_GLSL` gate 不要)
- (d) wire helper は host C++ Vulkan path 専有
- (e) `recordAvatarPlaceholderDraw` 内挿入も host C++

= `project_r41_phase1b_vulkan_host_gate` GATE-B 整合

### §4.9 MUSEUBO-A 整合

`mUseUBO=false` default 描画 100% 維持:

- `recordAvatarPlaceholderDraw` は Vulkan placeholder offscreen FBO 経路 = `mUseUBO=false` default で OpenGL viewer 描画には影響しない
- 多重 nullptr fallback (= `sAvatarBonePipeline` / `sAvatarBoneLayout` / `sAYAStandardLayout` / `sDrawUboRingBufferMgr` 既存 guard + `sPlaceholderSkin` register 失敗時 `writeSkinUbo` 早期 return) で graceful degrade
- sentinel skin 経由 placeholder zero write は Vulkan 描画歪み許容 (= 視覚 no-op 等価、PC-N-5 で実 GLTF 投入時に正規 data 流入)

---

## §5. PC-N-3 design-lock Exit Criteria 9 項全充足

| # | Criteria | 充足 |
|---|----------|------|
| (i) | PC-N-3 literal scope 4 件 §0 明文化 ((N3-3) B 採用後の事実訂正適用済) | ✅ |
| (ii) | 必読 1 件 §1 + pinpoint reference 10 件別記 | ✅ |
| (iii) | ambiguity (N3-1)..(N3-11) 11 件 + G-1 全 AYA literal「推奨案採用 OK」record (2026-06-05) | ✅ §3 |
| (iv) | 採用案根拠明文化 + G-1 解消方針明文化 | ✅ §3 + §2.3 |
| (v) | 実装計画 (a)-(g) 7 step 分解 | ✅ §4 |
| (vi) | PC-N-3 実装 phase Exit Criteria 10 項明文化 | ✅ §6 |
| (vii) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 | ✅ §4.8 |
| (viii) | MUSEUBO-A 整合 = `mUseUBO=false` default 描画 100% 維持 + 多重 nullptr fallback | ✅ §4.9 |
| (ix) | `indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件 = `feedback_design_phase_no_code_write` 整合 | ✅ |

---

## §6. PC-N-3 実装 phase Exit Criteria 10 項 (次 session で全充足)

| # | Criteria |
|---|----------|
| (i) | `sAvatarBoneStorageBuffer` + `Allocation` + `Mapped` + `allocateAvatarBoneStorageBuffer()` deprecate ((N3-1) A) |
| (ii) | placeholder skin sentinel `sPlaceholderSkin` 新設 ((N3-6) A) |
| (iii) | `initVulkan` 内 `registerSkinUbo(&sPlaceholderSkin, Skin_GLTFJoints, ...)` + `unloadInternal` 内 `unregisterSkinUbo` ((N3-7) A) |
| (iv) | `wireSkinUboSetV3aToBinding2(skin)` helper 新設 + `initVulkan` で初回 wire ((N3-4) A) |
| (v) | `recordAvatarPlaceholderDraw` 内 `writeSkinUbo` + `flushSkinUbos` + bindV3aRigged 正規 sequence 配線 ((N3-8) A) |
| (vi) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 |
| (vii) | MUSEUBO-A 整合 = `mUseUBO=false` default 描画 100% 維持 + 多重 nullptr fallback |
| (viii) | build verify llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131 全 PASS ((N3-10) A) |
| (ix) | tag block 統一 `<AYAstorm r41 PC-N-3 (a)>` + `(b)` + `(c)` + `(d)` + `(e)` + first-wire `LL_INFOS` marker + first-fire `LL_INFOS` marker |
| (x) | handoff complete doc 起案 + AYA commit 指示後 commit (= Co-Authored-By 不在 + 個別 file 指定 + 新 file 0 除 doc + CMake 改変 0 + settings.xml 改変 0 + codegen 改変 0 + shader 改変 0) |

---

## §7. 着手手順 8 step (= 次 session で実施)

1. 本 PC-N-3 design-lock doc 全文 Read (= 必読 1 件)
2. pinpoint reference 10 件のうち最低 4 件 Read (= `writeSkinUbo` + `flushSkinUbos` + `registerSkinUbo`/`unregisterSkinUbo` + `recordAvatarPlaceholderDraw` 現状) — full file dump 禁止 (`feedback_handoff_minimal_pre_req_read`)
3. `sAvatarBoneStorageBuffer` + `allocateAvatarBoneStorageBuffer` の全 site grep (= deprecate 対象確認)
4. `ubo_layout_skin_gltfjoints.inl` 内 block_size 定数 grep (= `registerSkinUbo` 第 3 引数値確定)
5. step (a)-(e) 5 編集 step 順次実装 (= deprecate → sentinel → register → wire → 配線)
6. step (f) build verify (= make -j4 llrender + TUT 11+10+13 + codegen unittest)
7. step (g) handoff complete doc 起案 + Exit Criteria 10 項 self-verify (= 9 観点 self-verify 全 ✅ 確認)
8. AYA commit 指示受領後 commit (= 個別 file 指定 + Co-Authored-By 不在)

---

## §8. 残 strict 線形

PC-N-3 実装 = **Phase 1.C complete** → PC-8 (= 3 OS build verify、Linux primary + Win/Mac AYA 環境依頼) → PC-N-5 = **Phase 1.D 着手起点** (= 実 GLTF Vulkan draw 通電 1 stub)

---

## §9. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1 design-lock ✅ + PC-N-1 ✅ + PC-N-2 design-lock ✅ + PC-N-2 ✅ + PC-N-4 design-lock ✅ + PC-N-4 ✅ + **PC-N-3 design-lock ✅ 本 commit** + PC-N-3 実装 ⏳ 次 session = Phase 1.C complete ⏳ + PC-8 (3 OS build verify) ⏳ + PC-N-5 = Phase 1.D 着手起点 ⏳

---

## §10. self-verify 9 観点 全 ✅

1. **PC-N-3 literal scope 4 件 §0 完全分解** ((N3-3) B 採用で 5 件 → 4 件事実訂正、(a) `sAvatarBoneStorageBuffer` deprecate + (b) placeholder sentinel + (c) register/unregister + (d) wire helper + (e) `recordAvatarPlaceholderDraw` 配線) ✅
2. **必読 1 件 §1 + pinpoint reference 10 件別記** ✅
3. **現状調査 §2 11 項網羅 (= code 11 項表) + design doc 3 件 + G-1 重要 gap 別記 §2.3** ✅
4. **ambiguity (N3-1)..(N3-11) 11 件 + G-1 AYA literal「推奨案採用 OK」record (2026-06-05) §3** (= (N3-3) は推奨案 C → B 事実訂正後の AYA literal「B で」record 含む) ✅
5. **採用根拠 11 件 + G-1 解消方針明文化 §3 表** ✅
6. **実装計画 (a)-(g) 7 step 分解 §4** ✅
7. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 §4.8** ✅
8. **MUSEUBO-A 整合 = `mUseUBO=false` default 経路不変 + 多重 nullptr fallback §4.9** ✅
9. **Exit Criteria 9 項明文化 §5 + 実装 phase Exit Criteria 10 項 §6 + `indra/` 改変 0 件 = `feedback_design_phase_no_code_write` 整合** ✅

---

## §11. 次 session 着手 1 line

**PC-N-3 実装着手** = step (a)-(g) 7 step 実施 = (a) `sAvatarBoneStorageBuffer` + `allocateAvatarBoneStorageBuffer` deprecate + (b) `sPlaceholderSkin` sentinel 新設 + (c) `initVulkan` register + `unloadInternal` unregister + (d) `wireSkinUboSetV3aToBinding2` helper 新設 + `initVulkan` で初回 wire + (e) `recordAvatarPlaceholderDraw` 内 `writeSkinUbo` + `flushSkinUbos` + bindV3aRigged 配線 + (f) build verify (= llrender + warning 0 + TUT 11+10+13 + codegen 131/131) + (g) handoff complete doc 起案 + Exit Criteria 10 項 self-verify + AYA commit 指示受領後 commit。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 design-lock handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 次 session 必読 1 件 + pinpoint reference 10 件別記、本 session も Read pinpoint のみ (= PC-N-4 complete doc + PC-N decomposition §4.4 + writeSkinUbo + writeSkinUbo register + skin_gltf_joints.glsl + design 06b §2.5 + design 06c §2.5)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §10
- **feedback_build_only_verified** 遵守 = design-lock phase は `indra/` 改変 0 件で build verify 対象外、実装 phase で literal 検証取得予定
- **feedback_no_scope_shrink** 遵守 = PC-N-3 literal scope 4 件 §0 完全分解、(N3-3) B 採用は事実訂正 (= PC-7γ-3 で codegen 配置済の発見) ゆえ「縮小」ではない、(N3-2) A placeholder sentinel + (N3-3) B codegen 不変 + (N3-10) A 131/131 維持は全て AYA literal「OK」record 済段階分離 = 縮小ではない
- **feedback_doubt_self_first** 遵守 = ambiguity 11 件 + 重要 gap G-1 発見で停止 + 推奨案提示 + AYA literal「推奨案採用 OK」 + 1 件 (N3-3) 事実訂正で 「B で」record 後本 design-lock doc 起案、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = 12 件 batch AYA 確認 2026-06-05、design 06b §2.5 + 06c §2.5 + 実装現状 + PC-7γ-3 配置済 codegen 出力の literal 差異も AYA に明示提示 + literal「OK」受領で正準採用確定、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N-3 = avatar bone storage 経路再配線 H10-A 持越単独 sub-step、PC-N-5 (実 GLTF avatar draw) は分離、本 doc 起案も PC-N-3 単独 design-lock のみ
- **feedback_design_phase_no_code_write** 整合 = 本 PC-N-3 design-lock phase は doc 起案のみ、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示受領後 commit
- **feedback_no_claude_coauthor** 遵守 / Co-Authored-By 行不在
- **feedback_no_bare_reference_ids** 遵守 = (N3-1)..(N3-11) 各 ID に項目名 / 採用案内容併記 §3 + G-1 に literal 差異内容併記 + (a)..(g) 各 step に作業内容併記 §4
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、`git add` 個別 file 指定 + `git add -A` 不使用予定

---
