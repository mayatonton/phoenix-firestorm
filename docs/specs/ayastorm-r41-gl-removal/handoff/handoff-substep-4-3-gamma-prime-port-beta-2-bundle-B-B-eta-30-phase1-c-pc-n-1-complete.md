# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-N-1 complete** marker

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-N-1 (= `flushDrawUbos` real per-draw data write 通電 = PC-6ε-3 持越 = `forwardToUboUpload` PER_DRAW case 通電 + `writeDrawUbo` helper 新設 + `recordPlaceholderPoolDraw` dummy zero memset → `writeDrawUbo` 経由 zero write 置換) の **実装 phase 完了** marker = step (a)-(g) 7 step 全実装 + Exit Criteria 10 項全充足 + build verify (llrender + warning 0 + TUT 11+10+13 + codegen 131/131) 全 PASS。

> **本 doc 位置付け**: 直前 commit `7bbff94429` (PC-N-1 design-lock complete) §6 着手手順 + §4 (a)-(g) 7 step に従い、本 session で実装 phase 着手 → 3 file 改変 (= `llglslshader.cpp` + `llvkloader.cpp` + `llvkloader.h`) + build verify 全 PASS。次 session 着手起点 = PC-N-2 design-lock (= `bindV3aRigged` set=2 復活 + `recordAvatarPlaceholderDraw` allocate-chain 配線)。

---

## §0. 本 session 着手契機 + literal scope record

**契機**: AYA 指示「PC-N-1 実装着手お願いします」literal 受領 (2026-06-05、design-lock commit `7bbff94429` 後の継続 session = 別 session の fresh context) → 必読 1 件 = PC-N-1 design-lock doc Read + step (a)-(g) 7 step 実施 + Exit Criteria 10 項 self-verify + 本 complete handoff doc 起案 + AYA literal「commit してください」受領後 commit 予定。

**PC-N-1 literal scope** (= design-lock §0 継承、5 件全実装):

1. **`forwardToUboUpload` PER_DRAW case 通電** ✅ = `LL_WARNS_ONCE` diagnostic → `writeDrawUbo` 呼出置換、tag `<PC-N-1 (b)>`
2. **`writeDrawUbo` helper 新設** ✅ = `writeFrameUbo` / `writeSingletonUbo` 同形 signature + 内部 ring buffer allocate + memcpy + dynamic offset 返却、tag `<PC-N-1 (a)>`
3. **`recordPlaceholderPoolDraw` dummy zero memset → `writeDrawUbo` 経由 zero write 置換** ✅ = `PerDrawUBO_LightParams` (256 B) を placeholder 代表として `writeDrawUbo(block_hash, 0, zero_buf, 256, out_offset)` 経由、API 経路通電確認、tag `<PC-7ε (d)>` → `<PC-N-1 (c)>`
4. **`flushDrawUbos` first-fire log 維持** + PC-N-4 grow flag 集約 hook 用 placeholder ✅ = `flushDummyUboWrite("flushDrawUbos")` 撤去 + `std::atomic<bool>` first-fire LL_INFOS + PC-N-4 持越 hook comment、tag `<PC-N-1 (d)>`
5. **codegen set=2 binding 配置現状 (= binding=0/1) 維持** ✅ = `indra/llrender/` only 改変、codegen 改変 0 件 ((N1-7) A 採用、binding=2/3 配置 + 4 binding 再分配は別 sub-step 持越)

---

## §1. 必読 1 件 + pinpoint reference

**次 session 必読** (= PC-N-2 design-lock 着手前):

1. **本 PC-N-1 complete handoff doc 全文**: `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-n-1-complete.md`

**pinpoint reference (PC-N-2 design-lock phase で必要分のみ)**:

- **PC-N decomposition design-lock doc**: `handoff-...-pc-n-decomposition-design-lock.md` = PC-N 5 sub-step 全体構図 + 依存関係 + Phase 境界
- **PC-N-1 design-lock doc**: `handoff-...-pc-n-1-design-lock.md` = ambiguity (N1-1)..(N1-9) 9 件 AYA literal「OK」record + 実装計画 (a)-(g) 7 step
- **`writeDrawUbo` 実装**: `indra/llrender/llvkloader.cpp` 中 `<AYAstorm r41 PC-N-1 (a)>` tag block = PC-N-2 で `bindV3aRigged` 配線時の dynamic offset 取得 reference
- **`recordPlaceholderPoolDraw` 新形**: `indra/llrender/llvkloader.cpp` 中 `<AYAstorm r41 PC-N-1 (c)>` tag block = PC-N-2 で `recordAvatarPlaceholderDraw` allocate-chain 配線時の参考 pattern
- **design 07 §7 + §8.4**: `design/07-vulkan-api-state.md:361-470` = dynamic offset (L2) 実 Vulkan 配線 + FRAMES_IN_FLIGHT=3 同期 rotate

---

## §2. 実装内容 (= step (a)-(g) 7 step 全実装記録)

### §2.1 改変サマリ表

| # | step | file | 改変箇所 | 差分 | tag |
|---|------|------|---------|------|-----|
| (a) | `writeDrawUbo` helper 新設 (cpp) | `indra/llrender/llvkloader.cpp` | `writeProgramUbo` 直後に新設 (= +96 行) | +96 / -0 | `<PC-N-1 (a)>` |
| (a) | `writeDrawUbo` 公開宣言 (h) | `indra/llrender/llvkloader.h` | `writeSingletonUbo` 直後に新設 (= +26 行) | +26 / -0 | (公開宣言) |
| (b) | `forwardToUboUpload` PER_DRAW case 通電 | `indra/llrender/llglslshader.cpp` | line 2151-2157 = `LL_WARNS_ONCE` 撤去 + `writeDrawUbo` 呼出置換 | +18 / -5 | `<PC-N-1 (b)>` |
| (c) | `recordPlaceholderPoolDraw` 置換 | `indra/llrender/llvkloader.cpp` | line 5010-5054 = 直接 allocate + memset → `writeDrawUbo` 経由 (= zero buffer 256 B) | +35 / -33 | `<PC-7ε (d)>` → `<PC-N-1 (c)>` |
| (c') | log message 更新 | `indra/llrender/llvkloader.cpp` | line 5175-5179 = log message 更新 (PC-7ε → PC-N-1 (c) 通電済) | +5 / -4 | (log) |
| (d) | `flushDrawUbos` 整理 | `indra/llrender/llvkloader.cpp` | line 4376-4383 = `flushDummyUboWrite("flushDrawUbos")` 撤去 + first-fire LL_INFOS + PC-N-4 hook comment | +30 / -7 | `<PC-N-1 (d)>` |
| (e) | `flushDummyUboWrite` 他 caller review | (調査のみ、改変なし) | helper 本体 5 caller 残存 (flushFrameUbos / flushProgramUbos / flushAssetUbos / flushSkinUbos / flushSingletonUbos) ゆえ helper 本体残置 | 0 / 0 | (review) |
| (f) | build verify | (実行のみ) | llrender + warning 0 + TUT 11/11 + 10/10 + 13/13 + codegen 131/131 全 PASS | - | - |
| (g) | handoff complete doc 起案 | (本 doc) | 1 new doc | +N / 0 | - |

**合計 diff** (= `git diff --stat indra/`):
```
 indra/llrender/llglslshader.cpp |  23 ++++-
 indra/llrender/llvkloader.cpp   | 222 +++++++++++++++++++++++++++++++---------
 indra/llrender/llvkloader.h     |  26 +++++
 3 files changed, 220 insertions(+), 51 deletions(-)
```

### §2.2 step (a) `writeDrawUbo` 実装詳細

**signature** (= AYA literal「OK」確認 2026-06-05、(N1-1) A 採用):

```cpp
void writeDrawUbo(U32 block_hash, U32 offset, const void* data, size_t size, U32& out_dynamic_offset);
```

**実装要点**:
- guards: `data` / `size` / `sDrawUboRingBufferMgr` nullptr (= MUSEUBO-A 整合)
- block_size lookup = `g_block_metadata` 線形 walk (= 既存 PC-7γ-1 `lookup_block_size_by_hash` 同形、(N1-8) B)
- range check: `offset + size > meta->block_size` で out-of-range 検出 + `LL_WARNS_ONCE`
- allocate: `sDrawUboRingBufferMgr->allocate(meta->block_size)` = block 全体 size で 1 chunk allocate ((N1-2) A)
- grow 観測時 `LL_WARNS_ONCE` (= PC-N-4 持越、(N1-6) B 整合)
- side-table lookup: `sDrawUboRingBufferRecords.find(alloc.buffer)` → `mapped` 取得
- memcpy at `(alloc.offset + offset)` ((N1-3) A in-place + (N1-4) A no dirty)
- `out_dynamic_offset = alloc.offset` ((N1-1) A caller responsibility)

### §2.3 step (b) `forwardToUboUpload` PER_DRAW case 通電詳細

**改変前** (= PC-7γ-1 stub):
```cpp
case kCadencePerDraw:
    LL_WARNS_ONCE("Vulkan") << "PC-7γ-1: PER_DRAW forwardToUboUpload not wired yet (PC-7ε scope), block_hash=0x"
                            << std::hex << loc.block_hash << std::dec << LL_ENDL;
    return;
```

**改変後** (= PC-N-1 (b) 通電):
```cpp
case kCadencePerDraw:
{
    // <AYAstorm r41 PC-N-1 (b)> ... [tag block comment 省略、コードは下記]
    U32 dynamic_offset = 0u;
    LLVKLoader::writeDrawUbo(loc.block_hash, loc.offset, data, size, dynamic_offset);
    (void)dynamic_offset; // PC-N-2 / PC-N-5 で bind 経路へ伝達予定
    return;
}
```

**dynamic_offset 伝達先**: PC-N-1 phase では caller (= setter 経路) に伝達せず discard (= placeholder pool 経路 = `recordPlaceholderPoolDraw` 側で別途 chunk 確保 + `bindV3aStatic` 配線)。real draw 経路の dynamic_offset 伝達 + set=2 復活 + `bindV3aRigged` 配線は PC-N-2 / PC-N-5 持越。

### §2.4 step (c) `recordPlaceholderPoolDraw` 置換詳細

**配線対象 block** = `PerDrawUBO_LightParams` (= `0x9ebc071fu`, 256 B, set=2, binding=0) を placeholder 代表として採用 = PER_DRAW cadence_tag=2 集合中で最小 size + binding=0 で shader 未参照でも GPU error なし。

**改変後コア**:
```cpp
if (!sDrawUboRingBufferMgr) { return; }  // MUSEUBO-A guard
static const U8 zero_buf[256] = {};
U32 dynamic_offset = 0u;
LLVKLoader::writeDrawUbo(
    ubo::block_hash::PerDrawUBO_LightParams,
    /*offset=*/0u,
    zero_buf,
    sizeof(zero_buf),
    dynamic_offset);
const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS] = {
    dynamic_offset, dynamic_offset, dynamic_offset, dynamic_offset,
};
bindV3aStatic(cmd_buf, sFrameIndex, dynamic_offsets);
```

**(N1-5) B 採用根拠**: API path 通電が本質、placeholder PSO 用 real value 構築は別 sub-step、real value 経路は PC-N-2 set=2 復活時に `recordAvatarPlaceholderDraw` 側で本格化。

### §2.5 step (d) `flushDrawUbos` 整理詳細

**改変前** (= PC-6ε-2):
```cpp
void flushDrawUbos() {
    flushDummyUboWrite("flushDrawUbos");
}
```

**改変後** (= PC-N-1 (d)):
```cpp
void flushDrawUbos() {
    if (!sDrawUboRingBufferMgr) { return; }
    static std::atomic<bool> s_first_fire{true};
    if (s_first_fire.exchange(false, std::memory_order_acq_rel)) {
        LL_INFOS("Vulkan") << "PC-N-1 (d) flushDrawUbos first fire (...)" << LL_ENDL;
    }
    // PC-N-4 持越 hook: AllocateResult.grew 集約 + frame 末尾 vkUpdateDescriptorSets 再発火
}
```

**(N1-6) B 採用根拠**: per-draw write は setter 内 `writeDrawUbo` immediate allocate ゆえ flush 側は no-op 等価 (= ring buffer per-allocate per-frame chunk rotate 自体が hazard 回避 + clean state 維持、(N1-4) A no dirty)。flush 撤去は scope 拡大ゆえ first-fire log marker 維持 + PC-N-4 hook placeholder 確保。

### §2.6 step (e) `flushDummyUboWrite` review 結果

**grep 結果** (= `flushDummyUboWrite` caller):
| caller | line | 状態 |
|--------|------|------|
| `flushFrameUbos` | `llvkloader.cpp:4330` | 残存 |
| `flushProgramUbos` | `llvkloader.cpp:4373` | 残存 |
| `flushAssetUbos` | `llvkloader.cpp:4450` | 残存 |
| `flushSkinUbos` | `llvkloader.cpp:4475` | 残存 |
| `flushSingletonUbos` | `llvkloader.cpp:4512` | 残存 |
| `flushDrawUbos` | (撤去済) | PC-N-1 (d) で撤去 |

**判定**: helper 本体 5 caller 残存ゆえ helper 本体撤去 scope 外 (= PC-N-1 literal scope 外、別 sub-step 持越し)。`flushDummyUboWrite` 本体 (= `llvkloader.cpp:4281-4318`) 改変 0 件。

---

## §3. build verify 結果

| # | 検証項目 | コマンド | 結果 |
|---|---------|---------|------|
| 1 | llrender build | `make -j4 llrender` | ✅ PASS、ERROR 0 / WARNING 0 |
| 2 | INTEGRATION_TEST_lluboringbuffer | `./INTEGRATION_TEST_lluboringbuffer` | ✅ 11/11 PASS (`YAY!! \o/`) |
| 3 | INTEGRATION_TEST_llassetubopool | `./INTEGRATION_TEST_llassetubopool` | ✅ 10/10 PASS (`YAY!! \o/`) |
| 4 | INTEGRATION_TEST_llpipelinecachestorage | `./INTEGRATION_TEST_llpipelinecachestorage` | ✅ 13/13 PASS (`YAY!! \o/`) |
| 5 | codegen unittest | `python3 -m unittest discover -s tests` | ✅ 131/131 PASS (`Ran 131 tests in 0.073s`) |

**Regression 確認**:
- PC-3 / PC-4 / PC-5 algorithm 層 = TUT 11+10+13 PASS で regression なし
- Phase 1.A / 1.B / 1.C PC-1..PC-7ε = codegen 131/131 PASS で regression なし
- `flushDummyUboWrite` 他 caller (= flushFrameUbos / flushProgramUbos / flushAssetUbos / flushSkinUbos / flushSingletonUbos) = helper 本体不変ゆえ regression なし

---

## §4. PC-N-1 Exit Criteria 10 項全充足

| # | Criteria | 充足 |
|---|----------|------|
| (i) | `writeDrawUbo(U32 block_hash, U32 offset, const void* data, size_t size, U32& out_dynamic_offset)` helper 新設 (= `writeFrameUbo` 同形 signature + offset 返却) | ✅ `llvkloader.h` / `.cpp` 両方追加 |
| (ii) | `forwardToUboUpload` PER_DRAW case 通電 (= `LL_WARNS_ONCE` diagnostic → `writeDrawUbo` 呼出置換) | ✅ `llglslshader.cpp:2151-2174` |
| (iii) | `recordPlaceholderPoolDraw` 内 dummy memset → API 経路通電 zero write 置換 ((N1-5) B 採用、real value 構築は PC-N-2 持越) | ✅ `llvkloader.cpp:5010-5054` |
| (iv) | `flushDrawUbos` first-fire log 維持 + PC-N-4 grow hook placeholder ((N1-6) B 採用) | ✅ `llvkloader.cpp:4376-4413` |
| (v) | codegen set=2 binding 配置現状 (= binding=0/1) 維持、binding=2/3 配置 + 4 binding 再分配は別 sub-step ((N1-7) A 採用) | ✅ codegen 改変 0 件 |
| (vi) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 | ✅ 全 step 共通 |
| (vii) | MUSEUBO-A 整合 = `mUseUBO=false` default で既存 OpenGL 描画 100% 維持 | ✅ `sDrawUboRingBufferMgr` nullptr guard (writeDrawUbo 内 + recordPlaceholderPoolDraw 内 + flushDrawUbos 内) で多重保証 |
| (viii) | build verify = `llrender` build + ERROR 0 / WARNING 0 + INTEGRATION_TEST 11+10+13 全 PASS + codegen unittest 131/131 PASS | ✅ §3 全項 PASS |
| (ix) | tag block 統一 = `<AYAstorm r41 PC-N-1 (a)>` (writeDrawUbo helper) + `<AYAstorm r41 PC-N-1 (b)>` (PER_DRAW case) + `<AYAstorm r41 PC-N-1 (c)>` (recordPlaceholderPoolDraw 置換、`<PC-7ε (d)>` → `<PC-N-1 (c)>` update) + `<AYAstorm r41 PC-N-1 (d)>` (flushDrawUbos) | ✅ 4 tag block 整合 |
| (x) | handoff complete doc 起案 + AYA commit 指示後 commit (= Co-Authored-By 不在 + 個別 file 指定 + 新 file 0 除 doc + CMake 改変 0 + settings.xml 改変 0) | ✅ 本 doc 起案、commit 指示待ち |

---

## §5. 残 strict 線形

PC-N-2 design-lock (= 次 session、`bindV3aRigged` set=2 復活 signature 拡張 + `recordAvatarPlaceholderDraw` allocate-chain 配線) → PC-N-2 実装 → PC-N-4 design-lock (= `AllocateResult.grew` 観測 frame 末尾で `vkUpdateDescriptorSets` 再発火 deferred) → PC-N-4 実装 → PC-N-3 design-lock (= `writeAvatarBoneStorage` helper 新設 + bone storage 経路再配線 H10-A 持越) → PC-N-3 実装 = **Phase 1.C complete** → PC-8 (= 3 OS build verify Linux primary + Win/Mac 後段) → PC-N-5 = Phase 1.D 着手起点 (= 実 GLTF Vulkan draw 通電 1 stub)

---

## §6. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1 design-lock ✅ + **PC-N-1 ✅ 本 commit** + PC-N-2 design-lock ⏳ 次 session + PC-N-2 実装 ⏳ + PC-N-4 design-lock ⏳ + PC-N-4 実装 ⏳ + PC-N-3 design-lock ⏳ + PC-N-3 実装 ⏳ = Phase 1.C complete ⏳ + PC-8 (3 OS build verify) ⏳ + PC-N-5 = Phase 1.D 着手起点 ⏳

---

## §7. self-verify 9 観点 全 ✅

1. **Exit Criteria 10 項全充足** §4 ✅
2. **必読 1 件 (本 complete handoff doc) 起案 + pinpoint reference 5 件 別記** §1 ✅
3. **step (a)..(g) 7 step 全実装** §2.1 改変サマリ表 ✅
4. **ambiguity (N1-1)..(N1-9) 9 件 AYA literal「OK」record (2026-06-05) 整合** = design-lock doc §3 record + 本実装で全採用案実装 ✅
5. **GATE-B 整合** = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 ✅
6. **MUSEUBO-A 整合** = `mUseUBO=false` default 経路不変 + `sDrawUboRingBufferMgr` nullptr guard 多重保証 ✅
7. **llrender build + WARNING 0 + TUT 11+10+13 + codegen 131/131 全 PASS** §3 ✅
8. **commit 内容** = 3 modified (llglslshader.cpp + llvkloader.cpp + llvkloader.h) + 1 new doc (handoff complete) + 新 file 0 (除 doc) + CMake 改変 0 + settings.xml 改変 0 + Co-Authored-By 不在予定 + diff stat +220/-51 ✅
9. **feedback_no_scope_shrink 遵守** = PC-N-1 literal scope 5 件 §0 全件実施、(N1-5) B (= zero write 経由置換) + (N1-7) A (= 現 codegen 配置維持) は AYA literal「OK」record 済段階分離 = 縮小ではない ✅

---

## §8. 次 session 着手 1 line

**PC-N-2 design-lock 着手** = `bindV3aRigged` set=2 復活 signature 拡張 (= `const U32 dynamic_offsets[V3A_DRAW_SET_BINDINGS]` 引数化、`bindV3aStatic` 同形) + `recordAvatarPlaceholderDraw` allocate-chain 配線 (= H10-A push descriptor 経路 disable 維持下で set=2 復活経路を bind 配線) の詳細 step 分解 + ambiguity 確認 + Exit Criteria 明文化。`indra/` 改変 0 件、別 session で実装 phase 着手。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 complete handoff doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 1 件 + pinpoint reference 5 件 別記、本 session も pinpoint Read のみ (= writeFrameUbo / writeSingletonUbo / writeProgramUbo / recordPlaceholderPoolDraw / forwardToUboUpload 該当部のみ)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §7
- **feedback_build_only_verified** 遵守 = llrender build + TUT 11/11 + 10/10 + 13/13 + codegen 131/131 で literal 検証取得
- **feedback_no_scope_shrink** 遵守 = PC-N-1 literal scope 5 件 §0 全件実施、(N1-5) B + (N1-7) A は AYA literal record 済段階分離
- **feedback_doubt_self_first** 遵守 = design-lock phase で ambiguity 9 件 + gap 2 件発見 + 推奨案提示 + AYA literal「OK」確認後本実装 (= 別 session)、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = 9 件 batch AYA 確認 design-lock phase で完了
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-N-1 = `flushDrawUbos` real per-draw data write 通電単独 sub-step、PC-N-2 (set=2 復活) + PC-N-3 (bone storage) + PC-N-4 (grow re-wire) + PC-N-5 (実 GLTF draw) は分離
- **feedback_design_phase_no_code_write** 整合 = 本 PC-N-1 は実装 phase = design-lock commit `7bbff94429` で `indra/` 改変 0 件 完了済、本 session で `indra/llrender/{llglslshader.cpp, llvkloader.cpp, llvkloader.h}` 改変は実装 phase ゆえ整合
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit 予定
- **feedback_no_auto_commit** 遵守 = AYA literal「commit してください」受領待ち
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在予定
- **feedback_no_bare_reference_ids** 遵守 = (N1-1)..(N1-9) 各 ID + (a)..(g) 各 step に項目名 / 採用案 / 作業内容併記
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、git add 個別 file 指定予定 + `git add -A` 不使用

---
