# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-7δ design-lock phase complete** handoff

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-7δ design-lock phase 完了 marker + 次 session 実装 phase 引継

---

## §0. 本 handoff の意味

PC-7δ (= vkCmdBindDescriptorSets 通電 + set=3 swap + sAYAStandardLayout 経由 bind + V3a 5-set 構成完成 + SINGLETON case llassert_always → flushSingletonUbos 本格化) の **design-lock phase が完了**、次 session で実装 phase 起動準備が整った状態。

session 境界の根拠:
- 本 session = PC-7γ-3 commit (`947e848bcb`) からの継続で context 残量圧迫気味 (= feedback_proactive_handoff 整合)
- PC-7δ 実装 scope = (a)-(p) 16 step × 5 sub-scope の大塊 = 別 session の fresh context で安全に実施
- design-lock 完了 = 自然な session 境界 (= AYA 確認受領済 + ambiguity 0 件残)
- AYA literal 「実装は別 session で行ったほうが安全では？」(2026-06-05) → Claude「賛成」→ AYA literal 「OK」(2026-06-05) 受領で session 分離確定

---

## §1. 本 session 成果物

| # | 成果物 | 内容 |
|---|---|---|
| 1 | `handoff-substep-...-pc-7-delta-design-lock.md` (本 commit 同梱) | PC-7δ design-lock doc = ambiguity 9+1 件 AYA 確認 record + 実装計画 (a)-(p) 16 step + Exit Criteria 11 項 |
| 2 | `handoff-substep-...-pc-7-delta-design-lock-complete.md` (本 doc) | design-lock phase 完了 marker + 次 session 引継 |

**`indra/` 改変**: **0 件** (= feedback_design_phase_no_code_write 整合)
**CMake 改変**: 0 件
**settings.xml 改変**: 0 件

---

## §2. 9+1 件 ambiguity AYA 確認 record (2026-06-05)

| # | 判断点 | AYA 採用案 | Claude 推奨理由 (主) |
|---|---|---|---|
| (H1) | bind path target = 「通電」literal の対象 | **(H1-A)** placeholder draw 2 件を sAYAStandardLayout + V3a 5-set bind に migrate | 「通電」literal 整合 + 視覚 no-op 等価維持 |
| (H2) | VkDescriptorSet allocation timing | **(H2-A)** initVulkan で V3a pool から eager allocate (= 計 13 set) | design 07 §6.4 grow only 整合 |
| (H3) | vkUpdateDescriptorSets timing | **(H3-A)** register\*Ubo 内 = UboInstance 確保直後に update | hot path 除外 + register-once + bind-many |
| (H4) | set=3 swap 実走 timing | **(H4-B)** bind helper 整備のみ、実 swap は実 GLTF Vulkan draw 通電 sub-step (PC-N) で発火 | 実 GLTF Vulkan draw 不在で実 swap は意味なし |
| (H5) | SINGLETON case 本格化 | **(H5-A)** `LLVKLoader::writeSingletonUbo(block_hash, ...)` 新設 + 呼出 | writeFrameUbo/writeProgramUbo 等と pattern 統一 |
| (H6) | maxBoundDescriptorSets=4 維持 | **§4.4.1 literal 採用** = static は set=0/1a/1b/2、rigged は set=0/1a/1b/3 | Vulkan 1.3 minimum 死守 |
| (H7) | GATE-B 整合 | **追加なし** = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 | GLSL 改変 0 件、host C++ + forwardToUboUpload のみ |
| (H8) | MUSEUBO-A 整合 | **(H1-A) で成立** = placeholder offscreen FBO 経路ゆえ実 OpenGL 描画影響ゼロ | bind 実発火は不可視 transition |
| (H9) | PC-7δ scope cadence 範囲 | **(H9-A)** V3a 5-set 全 cadence bind 配線 | literal「V3a 5-set 構成完成」整合 |
| (H10) | avatar placeholder pipeline layout compatibility (= 起案中追加発見) | **(H10-A)** avatar push descriptor 経路を PC-7δ で disable + sAvatarBonePipeline & sSkySmokePipeline の pipeline layout を sAYAStandardLayout で再構築 | scope 拡大せず set=3 swap literal 達成 |

---

## §3. 次 session 着手 1 line

> **PC-7δ 実装着手お願いします。design-lock 完了済 (= 本 session commit `<hash>`)。必読 1 件 = `handoff-substep-...-pc-7-delta-design-lock.md`。9+1 件 ambiguity 全 AYA literal「OK」record 済。**

---

## §4. 次 session 必読 1 件 + pinpoint reference

### 必読 1 件

1. **`docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-7-delta-design-lock.md`** = PC-7δ design-lock doc (= 全 9+1 件 ambiguity 採用案 + (a)-(p) 16 step 実装計画 + Exit Criteria 11 項)

### pinpoint reference (= 実装中必要時のみ Read)

| 場所 | 用途 |
|---|---|
| `indra/llrender/llvkloader.cpp:2189-2386` | PC-7α V3a scaffolding (5 layout / 4 pool / sAYAStandardLayout) |
| `indra/llrender/llvkloader.cpp:3138-3155` | PER_FRAME UboInstance pre-allocate ループ (= vkUpdateDescriptorSets 追加 site) |
| `indra/llrender/llvkloader.cpp:3881-4024` | 6 flush\*Ubos 既存 placeholder (= 本格化対象) |
| `indra/llrender/llvkloader.cpp:4146-4312` | registerProgramUbo / registerAssetUbo / registerSkinUbo / writeProgramUbo / writeAssetUbo / writeSkinUbo 6 method (= vkUpdateDescriptorSets 末尾追加 site) |
| `indra/llrender/llvkloader.cpp:4355-4456` | recordPlaceholderPoolDraw / recordAvatarPlaceholderDraw (= bind path migrate 対象) |
| `indra/llrender/llglslshader.cpp:2113-2222` | forwardToUboUpload 9-case switch (= SINGLETON case 本格化対象) |
| `indra/llrender/llglslshader.cpp:2256-2283` | bringupTestUBO (= flushSingletonUbos 既呼出 site、変更不要) |
| `docs/specs/ayastorm-r41-gl-removal/design/06c-descriptor-set-bind-wiring.md` §4 / §5 | flush 直後 bind sequence + dynamic offset bind 側責務 |
| `docs/specs/ayastorm-r41-gl-removal/design/07-vulkan-api-state.md` §4.4.1 / §6 / §8.4 / §9 | 論理 5 set → bind 時 4 set 縮減 + descriptor pool 容量 + FRAMES_IN_FLIGHT rotate + sAYAStandardLayout + set=3 swap |

---

## §5. 実装 phase Exit Criteria 11 項 (= design-lock doc §5 引用)

| # | Exit Criteria |
|---|---|
| (i) | sFrameUboSetV3a / sProgramUboSetA / sProgramUboSetB / sDrawUboSetV3a / sAssetUboSetV3a 5 件 static array + `createV3aDescriptorSets()` helper + initVulkan 配線 (= H2-A eager allocate 計 13 set) |
| (ii) | vkUpdateDescriptorSets 呼出 = PER_FRAME initVulkan 内 3 件 + registerProgramUbo / registerAssetUbo / registerSkinUbo 末尾 各 method 内 (= H3-A register 時 update) |
| (iii) | sSingletonUboInstances map + writeSingletonUbo helper + initVulkan SINGLETON allocate ループ + shutdownVulkan teardown (= H5-A SINGLETON 本格化 foundation) |
| (iv) | forwardToUboUpload SINGLETON case 本格化 = `llassert_always(false)` → `writeSingletonUbo(...)` (= H5-A 通電) |
| (v) | bindV3aStatic / bindV3aRigged helper 新設 = vkCmdBindDescriptorSets で 4 set 構成 bind (= H6 maxBoundDescriptorSets=4 死守、set=2 ↔ set=3 swap) |
| (vi) | recordPlaceholderPoolDraw bind path migrate = bindV3aStatic 呼出 + sSkySmokePipeline の pipeline layout 再構築 (= sAYAStandardLayout 統一、H10-A 整合) |
| (vii) | recordAvatarPlaceholderDraw bind path migrate = bindV3aRigged 呼出 + sAvatarBonePipeline の pipeline layout 再構築 + push descriptor 経路 disable (= H1-A + H10-A 通電) |
| (viii) | flushSingletonUbos 本格化 = sSingletonUboInstances walk + dirty exchange + vkCmdBindDescriptorSets via sAYAStandardLayout |
| (ix) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 |
| (x) | MUSEUBO-A 整合 = mUseUBO=false default で既存 OpenGL 描画 100% 維持、bind 実発火は placeholder offscreen FBO 経路 |
| (xi) | build verify = llrender + newview TU rebuild PASS + warning 0 + Vulkan validation 0 件 (= bind compatibility check) + TUT 3 件 (11+10+13) + codegen 130/130 PASS |

---

## §6. 残 strict 線形 (= PC-7δ 以降)

```
PC-7δ (= 本次 session 着手)
  ↓
PC-7ε (= dynamic offset 経路 ring buffer chunk hand-off
        = set=2 per-draw を sDrawUboRingBufferMgr 経由 dynamic offset 計算
        + pDynamicOffsets[4] 配線)
  ↓
PC-7α' (= codegen ubo_metadata.inl V1' update
         = scripts/ubo_codegen/main.py で set=1a/1b split 実装
         + ubo_metadata.inl 再生成 + 130 件 unittest 回帰確認、PC-7δ 通電前に必要だったが
         本 PC-7δ scope では set=1a/1b は logical layout のみ、PC-N で実 program 通電時に実走 binding 必要)
  ↓
PC-8 (= 3 OS build verify、Linux primary + Win/Mac 後段)
  ↓
PC-N (= Phase 1.C complete marker + Phase 1.D / Phase 2 着手起点)
```

---

## §7. r41 milestone state

| Phase | state |
|---|---|
| Phase 1.A | ✅ complete |
| Phase 1.B | ✅ complete |
| (Z) SSS | ✅ |
| (W) uniform4iv | ✅ |
| (Y) Phase 1.C prep | ✅ |
| PC-0 .. PC-6ζ | ✅ |
| PC-7α | ✅ |
| PC-7β | ✅ |
| PC-7γ-1 | ✅ |
| PC-7γ-2 | ✅ (commit `ed08d7b43e`) |
| PC-7γ-3 | ✅ (commit `947e848bcb`) |
| **PC-7δ design-lock** | **✅ 本 commit (= 本 doc + design-lock doc 2 件)** |
| PC-7δ 実装 | ⏳ 次 session |
| PC-7ε .. PC-N | ⏳ |

---

## §8. self-verify 9 観点 (= design-lock phase 完了判定)

| # | 観点 | 状態 |
|---|---|---|
| 1 | PC-7δ literal scope 5 件全件 §0 確認 | ✅ design-lock doc §0 |
| 2 | 必読 3 件 Read 完了 (= PC-7γ-3 complete handoff + design 06c + design 07) | ✅ design-lock doc §1 |
| 3 | ambiguity 全件 AYA literal「OK」record (= 9+1 件) | ✅ design-lock doc §3 + 本 doc §2 |
| 4 | 採用案根拠明文化 | ✅ design-lock doc §3 各案根拠 |
| 5 | 実装計画分解 = (a)-(p) 16 step + scope 5 件分解 | ✅ design-lock doc §4.1 |
| 6 | Exit Criteria 11 項明文化 | ✅ design-lock doc §5 + 本 doc §5 |
| 7 | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 | ✅ §4.2 / §5 (ix) |
| 8 | MUSEUBO-A 整合 = mUseUBO=false default 描画 100% 維持 | ✅ §4.2 / §5 (x) |
| 9 | `indra/` 改変 0 件 = feedback_design_phase_no_code_write 整合 | ✅ 本 doc §1 |

---

## §9. 引き継ぎ memory 14 件 遵守確認

- **feedback_proactive_handoff**: ✅ 本 session で能動的 session 境界判断 + design-lock complete handoff 起案
- **feedback_handoff_minimal_pre_req_read**: ✅ 次 session 必読 1 件 + pinpoint reference 別記 (§4)
- **feedback_self_verify_before_handoff**: ✅ §8 self-verify 9 観点全 ✅
- **feedback_build_only_verified**: ✅ design-lock phase は build verify 対象外 (= `indra/` 改変 0 件)、実装 phase で TUT + codegen + Vulkan validation 検証取得
- **feedback_no_scope_shrink**: ✅ PC-7δ literal scope 5 件全件 §0 確認 + §4.1 完全分解、scope 縮小なし
- **feedback_doubt_self_first**: ✅ 9+1 件 ambiguity 発見で停止 + 推奨案提示 + AYA 確認 + literal「OK」受領
- **feedback_confirm_referent_before_acting**: ✅ 9 件 batch AYA 確認 + (H10) 単発 AYA 確認 + 推測実装なし
- **feedback_ubo_migration_one_at_a_time**: ✅ PC-7δ = vkCmdBindDescriptorSets 通電 + set=3 swap + V3a 5-set 完成 + SINGLETON 本格化 単一 sub-step、PC-7ε (dynamic offset) + PC-7α' (codegen V1') は分離
- **feedback_design_phase_no_code_write**: ✅ 本 session は design-lock phase、`indra/` 改変 0 件
- **feedback_release_branch_workflow**: ✅ feature branch `feature/ayastorm-r41-gl-removal` 上で実装 phase 進行
- **feedback_no_auto_commit**: ✅ 本 commit は AYA「commit してください」literal 受領後実施 (= 次の AYA 指示待ち)
- **feedback_no_claude_coauthor**: ✅ Co-Authored-By: Claude 行不在
- **GATE-B**: ✅ `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= 本 design-lock doc 含む)
- **MUSEUBO-A**: ✅ design-lock 完了時点で `indra/` 改変 0 件ゆえ runtime 動作影響 0 件 (= mUseUBO=false default 維持自明)

---

## §10. 本 commit 内容予定

| ファイル | 種別 | サイズ目安 |
|---|---|---|
| `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-...-pc-7-delta-design-lock.md` | 新 file | ~13 KB (= ambiguity 9+1 件 + 実装計画 + Exit Criteria) |
| `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-...-pc-7-delta-design-lock-complete.md` | 新 file (本 doc) | ~6 KB |
| **計** | **2 新 file** | **`indra/` 改変 0 件 / CMake 0 / settings.xml 0** |

**commit message 候補** (= 引用付き、design-lock phase 完了 marker):
```
r41: sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-7δ design-lock complete** marker
= ambiguity 9+1 件 AYA 確認 record (= 2026-06-05 literal「OK」受領) + 実装計画 (a)-(p) 16 step + Exit Criteria 11 項
= 2 new doc (design-lock + design-lock-complete handoff) / `indra/` 改変 0 件
= feedback_proactive_handoff 整合 = 実装は次 session 分離 (AYA 「実装は別 session で行ったほうが安全では？」literal 受領 2026-06-05)
```

---

**design-lock phase 完了 = 2026-06-05 / 実装 phase は次 session で起動**
