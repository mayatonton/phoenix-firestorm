# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C **PC-8 Linux primary marker**

**作成日**: 2026-06-05
**起案者**: Claude (AYAstorm r41 担当)
**目的**: PC-8 (= 当初設計「3 OS build verify」) を AYA 方針「Linux primary 完成 → 他者補完」採用で **「Linux primary build verify marker」** に再定義 + Mac/Win 開発者向け OS 依存資料は別 file (`ayastorm-r41-cross-platform-port-spec.md`) に stub 起案。本 doc は PC-8 sub-step **本 session 完了 marker** = step 全実装 + Exit Criteria 全充足。

> **本 doc 位置付け**: PC-N-3 complete (= `71f7bb2a89`) 後の PC-8 sub-step 着手 + AYA 方針確定 (= 「Linux primary 完成 → 他者補完」literal record 2026-06-05) に基づく PC-8 再定義 + 完了 marker。**Phase 1.C strict 線形終了**。次 strict 線形 = **PC-N-5 = Phase 1.D 着手起点** (= 実 GLTF Vulkan draw 通電 1 stub)。

---

## §0. 本 session 着手契機 + 完了 scope record

**契機**: AYA 指示「PC-8 着手お願いします」literal 受領 (2026-06-05、PC-N-3 complete commit `71f7bb2a89` 後の継続 session = 別 session の fresh context) + 必読 1 件 = `handoff-...-pc-n-3-complete.md` (= Linux primary literal 取得済 §3) Read → ambiguity 4 件 (PC-8-1 marker 定義 + PC-8-2 Win/Mac command 列 + PC-8-3 feature branch 取込 + PC-8-4 design-lock 要否) 推奨案提示 → AYA literal「WindowsとMacOSですが、同時に開発する計画にありません。まずLinuxで完成の後、それを提供してつないでもらう（足りないところを追加してもらう）そういう考えています。それともPhase１より上位構造でOS依存が懸念されますか？」record (2026-06-05) → Phase 1 より上位構造で OS 依存懸念検討 (= 6 項表) 提示 + macOS MoltenVK 制約 (= descriptor set 数 + Vulkan version) 本質的存在指摘 → AYA literal「Bを採用、PC-8 marker doc 起案後 PC-N-5 へ （MacとWin開発者には最終的に本環境と依存問題を示す資料を提供したい）」record (2026-06-05) → (PC-8-doc-1) Mac/Win 向け資料配置 + (PC-8-doc-2) Mac/Win 向け資料初期 scope 提示 → AYA literal「B + A で起案お願いします」record (2026-06-05) → 本 PC-8 marker doc + `ayastorm-r41-cross-platform-port-spec.md` stub 起案。

**PC-8 完了 scope** (= 本 session 実施分):

1. ✅ **PC-8 再定義 = 「Linux primary build verify marker」** (AYA literal「Bを採用」record 2026-06-05、= 3 OS 揃え戦略を「Linux primary 完成 → Mac/Win 派生 fix 補完」モデルで実現)
2. ✅ **Linux primary build verify literal 取得確認** (= PC-N-3 complete §3 全 6 行 PASS = 本 PC-8 §2 で reference)
3. ✅ **Mac/Win 開発者向け OS 依存資料 stub 起案** (= `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md`、(PC-8-doc-1) B + (PC-8-doc-2) A 採用)
4. ⏳ **Win/Mac 環境 build verify** = AYA 方針採用ゆえ Phase 1.D + 以降全完了後の別 phase で扱う (= 本 sub-step scope 外)

---

## §1. 必読 + pinpoint reference (次 session = PC-N-5 向け)

**次 session 必読**:

1. **本 PC-8 marker doc 全文**: `handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-8-linux-primary-marker.md`
2. **PC-N-3 complete doc 全文**: `handoff-...-pc-n-3-complete.md` (= H10-A avatar bone storage 経路再配線 + sPlaceholderSkin sentinel 完了状態 = PC-N-5 着手 baseline)

**pinpoint reference** (PC-N-5 で必要分のみ):

- **`ayastorm-r41-cross-platform-port-spec.md`**: Mac/Win 開発者向け OS 依存資料 (= 本 PC-8 で stub 起案、Phase 1.D 以降の sub-step で随時追記)
- **PC-N decomposition design-lock**: `handoff-...-pc-n-decomposition-design-lock.md` §5 = Phase 1.D 着手起点 = PC-N-5 (= 実 GLTF Vulkan draw 通電 1 stub) 定義
- **PC-N-3 complete §2.7**: address-only sentinel pattern (= LL::GLTF::Skin forward-decl 制約遵守、PC-N-5 で実 Skin* と sentinel を sSkinUboDirty 上で同居の baseline)
- **GATE-B literal**: memory `project_r41_phase1b_vulkan_host_gate` (= host C++ は `mUseUBO` runtime flag のみで gate)

---

## §2. Linux primary build verify literal (= PC-N-3 complete §3 reference)

PC-N-3 complete commit `71f7bb2a89` §3 全 6 行 (= 本 PC-8 sub-step Linux primary 完了 marker としての literal 結果):

| # | check | 結果 | source |
|---|-------|------|--------|
| 1 | `cd build-linux-x86_64 && make -j4 llrender` | ✅ PASS (= `[100%] Built target llrender`) | PC-N-3 complete §3 #1 |
| 2 | ERROR 0 + WARNING 0 | ✅ (= `grep -iE "warning\|error"` 0 件) | PC-N-3 complete §3 #2 |
| 3 | `INTEGRATION_TEST_lluboringbuffer` | ✅ 11/11 PASS YAY (= PC-3 algorithm 層 regression なし) | PC-N-3 complete §3 #3 |
| 4 | `INTEGRATION_TEST_llassetubopool` | ✅ 10/10 PASS YAY (= PC-4 algorithm 層 regression なし) | PC-N-3 complete §3 #4 |
| 5 | `INTEGRATION_TEST_llpipelinecachestorage` | ✅ 13/13 PASS YAY (= PC-5 algorithm 層 regression なし) | PC-N-3 complete §3 #5 |
| 6 | `scripts/ubo_codegen` unittest | ✅ 131/131 PASS (= Phase 1.A / 1.B / 1.C PC-1..PC-N-4 regression なし、131 維持) | PC-N-3 complete §3 #6 |

**= Linux primary build verify literal 取得済**、本 PC-8 sub-step Linux primary 完了 marker。

---

## §3. AYA 方針「Linux primary 完成 → 他者補完」採用根拠

**採用**: (A) PC-8 撤回 / (B) **PC-8 = Linux primary marker doc 起案** / (C) PC-8 維持 = 3 OS 揃え から **B** を AYA literal「Bを採用」record (2026-06-05) で確定。

**根拠** (= 本 session で AYA に提示し合意取得):

1. **macOS MoltenVK 制約は本質的、Linux 先行で検出不可** = descriptor set 数 (= `maxBoundDescriptorSets` 可能性 4) + Vulkan version (= 1.2 subset) は実機 (= macOS Metal API 経由 SPIR-V → MSL 翻訳) 検証必須
2. **早期 macOS 制約考慮で Linux 開発進行遅延回避** = Phase 1.A/B/C 段階で MoltenVK 制約を全項目反映すると Phase 1 完成が大幅遅延
3. **Phase 1 全完了 = Linux 動作確定 → macOS / Windows 移植時に制約発覚 → Linux 設計から派生 fix で対応** = Linux primary 完成度高いほど派生 fix 範囲明確、設計影響最小化
4. **他開発者補完戦略は AYAstorm 従来モデル踏襲** = Mac は @t-noami 既往モデル + Windows 開発者の補完 phase 想定、memory `project_ayastorm_three_platforms` 「3 OS 揃え原則」は「Linux primary 完成 → Mac/Win 派生 fix」で実現
5. **memory `feedback_mac_only_fixes_accept_as_is` 整合** = AYA は Mac 不所持ゆえ Mac 限定 fix は他開発者検証信任で as-is 受け入れ、本戦略と整合

---

## §4. Mac/Win 開発者向け資料 stub 起案 (= (PC-8-doc-1) B + (PC-8-doc-2) A 採用)

**配置**: `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` (= specs dir 直下、handoff dir 外、`ayastorm-r41-` prefix で既存 spec doc 群 (= `ayastorm-r41-ubo-current-state-inventory.md`) と整合)

**起案 scope** (= (PC-8-doc-2) A 採用):

- **§0**: AYA 方針 record (= literal「WindowsとMacOSですが、同時に開発する計画にありません。まずLinuxで完成の後、それを提供してつないでもらう」2026-06-05)
- **§1**: Linux 環境 spec (= 12 項表、確定済 + 取得要分け literal 明示)
- **§2**: r41 設計の Vulkan 要求 version (= Phase 1.D 以降で literal 確定の留意 + 想定 Vulkan 1.2 以上 + 取得要 extension list 留意)
- **§3**: OS 依存性表 6 項 (= 本 PC-8 session で確定: Vulkan API / GLSL → SPIR-V codegen / VMA + volk dependency / descriptor set 数 / push descriptor / Vulkan version 要件 = 各 macOS 派生 fix 候補 + Windows 派生 fix 候補列付き)
- **§4**: macOS MoltenVK 既知制約 list (= 9 項 stub、Phase 1.D 以降で literal 確認要)
- **§5**: Windows 既知考慮事項 (= 8 項 stub、派生 fix 候補少想定)
- **§6**: 各 PC-* sub-step での OS 依存懸念記録欄 (= PC-0..PC-N-4 ✅ 既着手分 retrospective 留意 + PC-N-5 ⏳ 次着手起点)
- **§7**: Mac/Win 開発者向け提供 checklist (= Phase 1 全完了時の確定形 + AYA 提供承認 7 項)

Phase 1.D 以降の各 PC-* sub-step で随時追記、**Phase 1 全完了時に確定形で Mac/Win 開発者に提供** (= AYA literal「MacとWin開発者には最終的に本環境と依存問題を示す資料を提供したい」2026-06-05)。

---

## §5. PC-8 sub-step Exit Criteria 全充足

| # | Criteria | 充足 |
|---|----------|------|
| (i) | PC-8 再定義 = 「Linux primary build verify marker」 (AYA literal「Bを採用」2026-06-05) | ✅ §0 + §3 |
| (ii) | Linux primary build verify literal 取得確認 (= PC-N-3 complete §3 全 6 行 PASS reference) | ✅ §2 |
| (iii) | Mac/Win 開発者向け OS 依存資料 stub 起案 = 別 file `ayastorm-r41-cross-platform-port-spec.md` | ✅ §4 |
| (iv) | Mac/Win 向け資料 stub 初期 scope = OS 依存性表 6 項 + Linux 環境 spec + r41 Vulkan 要求 version + macOS MoltenVK 既知制約 list ((PC-8-doc-2) A 採用) | ✅ §4 |
| (v) | GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件 (= doc 起案のみで `indra/` 改変なし) | ✅ §6 |
| (vi) | MUSEUBO-A 整合 = `mUseUBO=false` default 描画 100% 維持 (= doc 起案のみで code 改変 0 件ゆえ既存維持) | ✅ §6 |
| (vii) | build verify = `indra/` 改変 0 件ゆえ build verify 対象外 (= `feedback_design_phase_no_code_write` 同等整合 = doc 起案のみ phase) | ✅ §6 |
| (viii) | tag block = (= code 改変 0 件ゆえ tag block 配置なし、PC-8 marker doc + cross-platform spec stub doc 2 件起案のみ) | ✅ §6 |
| (ix) | handoff complete doc 起案 + AYA commit 指示後 commit (= Co-Authored-By 不在 + 個別 file 指定 + 新 file 2 (本 doc + stub doc) + CMake 改変 0 + settings.xml 改変 0 + codegen 改変 0 + shader 改変 0 + `indra/` 改変 0) | ✅ 本 doc + commit pending AYA 指示 |

---

## §6. 改変内容 (= 本 PC-8 session)

### §6.1 新 file 2 件

| # | file | 行数 | 内容 |
|---|------|------|------|
| 1 | `docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-8-linux-primary-marker.md` | (= 本 doc) | PC-8 marker doc = Linux primary build verify marker + Mac/Win 向け資料 stub reference |
| 2 | `docs/specs/ayastorm-r41-gl-removal/ayastorm-r41-cross-platform-port-spec.md` | 約 100 行 | Mac/Win 開発者向け OS 依存資料 stub = (PC-8-doc-2) A 採用 scope (= OS 依存性表 6 項 + Linux 環境 spec + r41 Vulkan 要求 version + macOS MoltenVK 既知制約 list) |

### §6.2 改変なし

- `indra/` 改変 0 件 = `feedback_design_phase_no_code_write` 同等整合 (= PC-8 は doc 起案 phase のみ)
- CMake 改変 0 件
- `settings.xml` 改変 0 件
- codegen (`scripts/ubo_codegen` + `build-linux-x86_64/codegen/`) 改変 0 件
- shader (`*.glsl` + `aya_r41_blueprints/`) 改変 0 件
- memory 改変 0 件 (= 既存 memory `project_ayastorm_three_platforms` の方針更新は AYA 判断、本 session では cross-platform spec stub 内 §0 で AYA literal record 集約のみ)

### §6.3 build verify 対象外

- `indra/` 改変 0 件ゆえ build verify (= llrender + WARNING 0 + TUT 11+10+13 + codegen 131/131) 対象外
- PC-N-3 complete commit `71f7bb2a89` の Linux primary literal 結果が本 PC-8 marker doc §2 で reference され、本 PC-8 sub-step Linux primary 完了 marker として有効

---

## §7. 残 strict 線形 (= Phase 1.C strict 線形終了)

**Phase 1.C complete** ✅ (= PC-N-3 commit `71f7bb2a89` で達成済)
**PC-8 (Linux primary marker)** ✅ 本 commit

次:

- **PC-N-5** = **Phase 1.D 着手起点** (= 実 GLTF Vulkan draw 通電 1 stub = sentinel skin と実 Skin* を sSkinUboDirty 上で同居、bone matrix data 構築 + bindV3aRigged 経由実 draw) ⏳ 次 session
- ... (Phase 1.D 内 sub-step 群)
- **Phase 1 全完了** → **Mac/Win 開発者補完 phase** (= 別 chapter or sub-step、本 PC-8 marker doc + `ayastorm-r41-cross-platform-port-spec.md` 確定形提供) ⏳

---

## §8. r41 milestone state

Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅ + PC-0..PC-6ζ ✅ + PC-7α ✅ + PC-7β ✅ + PC-7γ-1 ✅ + PC-7γ-2 ✅ + PC-7γ-3 ✅ + PC-7δ design-lock ✅ + PC-7δ ✅ + PC-7α' design-lock ✅ + PC-7α' ✅ + PC-7ε design-lock ✅ + PC-7ε ✅ + PC-N decomposition design-lock ✅ + PC-N-1 design-lock ✅ + PC-N-1 ✅ + PC-N-2 design-lock ✅ + PC-N-2 ✅ + PC-N-4 design-lock ✅ + PC-N-4 ✅ + PC-N-3 design-lock ✅ + PC-N-3 ✅ = **Phase 1.C complete ✅** + **PC-8 Linux primary marker ✅ 本 commit = Phase 1.C strict 線形終了 ✅** + PC-N-5 = Phase 1.D 着手起点 ⏳

---

## §9. self-verify 9 観点 全 ✅

1. **PC-8 sub-step Exit Criteria 全充足** §5 ✅
2. **必読 1 件 (本 PC-8 marker doc) + PC-N-3 complete doc 全文** §1 + pinpoint reference 4 件別記 (= cross-platform spec stub + PC-N decomposition §5 + PC-N-3 complete §2.7 + GATE-B literal memory) ✅
3. **PC-8 完了 scope 4 件 §0** 全実施 (= 再定義 + Linux primary verify 確認 + Mac/Win 資料 stub 起案 + Win/Mac build verify は Phase 1 全完了後の別 phase 明示) ✅
4. **AYA literal 3 件 record** (= 「WindowsとMacOSですが、同時に開発する計画にありません。まずLinuxで完成の後、それを提供してつないでもらう（足りないところを追加してもらう）そういう考えています」2026-06-05 + 「Bを採用、PC-8 marker doc 起案後 PC-N-5 へ （MacとWin開発者には最終的に本環境と依存問題を示す資料を提供したい）」2026-06-05 + 「B + A で起案お願いします」2026-06-05) ✅
5. **GATE-B 整合 = `#ifdef LL_VULKAN_GLSL` 新規追加 0 件** §5 (v) + §6.2 ✅
6. **MUSEUBO-A 整合 = `mUseUBO=false` default 経路不変** §5 (vi) + §6.2 (= code 改変 0 件で既存維持) ✅
7. **build verify 対象外 = `indra/` 改変 0 件** §5 (vii) + §6.3 (= `feedback_design_phase_no_code_write` 同等整合) ✅
8. **commit 内容** = 新 file 2 (本 PC-8 marker doc + cross-platform spec stub doc) + 既存 file 改変 0 + CMake 改変 0 + settings.xml 改変 0 + codegen 改変 0 + shader 改変 0 + `indra/` 改変 0 + Co-Authored-By 不在予定 ✅
9. **`feedback_no_scope_shrink` 遵守** = PC-8 literal scope 4 件 §0 全件実施、(B) 採用 + Mac/Win 補完戦略は AYA literal record 後の方針確定ゆえ縮小ではなく **scope 再定義** ✅

---

## §10. 引き継ぎ memory 14 件

- (本 session で memory 改変 0 件、既存参照のみ)
- 関連 memory: `project_ayastorm_r41_vulkan_migration` (= r41 章 milestone pointer) + `project_ayastorm_three_platforms` (= 3 OS 揃え原則、Linux primary 完成 → Mac/Win 派生 fix モデルで実現) + `project_r41_phase1b_vulkan_host_gate` (= GATE-B literal) + `feedback_mac_only_fixes_accept_as_is` (= Mac 限定 fix as-is 受け入れモデル整合) + `feedback_ubo_migration_one_at_a_time` (= UBO 化 1 つずつ厳格) + `feedback_no_scope_shrink` (= literal scope 完全実施) + `feedback_self_verify_before_handoff` (= 9 観点 self-verify) + `feedback_build_only_verified` (= literal 検証取得、本 PC-8 は PC-N-3 §3 reference) + `feedback_doubt_self_first` (= ambiguity 4 件発見後 AYA literal「Bを採用」record + (PC-8-doc-1) B + (PC-8-doc-2) A record 後本起案) + `feedback_confirm_referent_before_acting` (= AYA 方針提示後 PC-8 再定義案 3 候補提示 + AYA literal「Bを採用」record + (PC-8-doc-1) + (PC-8-doc-2) 2 件 batch 確認 + AYA literal「B + A で」record) + `feedback_design_phase_no_code_write` (= 本 PC-8 は doc 起案 phase、`indra/` 改変 0 件) + `feedback_release_branch_workflow` (= feature branch `feature/ayastorm-r41-gl-removal` 上 commit) + `feedback_no_auto_commit` (= AYA commit 指示後 commit) + `feedback_no_claude_coauthor` (= Co-Authored-By 不在予定) + `feedback_no_bare_reference_ids` (= (PC-8-1)..(PC-8-4) + (PC-8-doc-1)..(PC-8-doc-2) 各 ID に項目名併記) + `feedback_tests_dir_never_commit` (= `tests/` 改変 0 件、`git add` 個別 file 指定 + `git add -A` 不使用)

---

## §11. 次 session 着手 1 line

**PC-N-5 (Phase 1.D 着手起点) 着手** = 実 GLTF Vulkan draw 通電 1 stub = sentinel skin と実 Skin* を sSkinUboDirty 上で同居、bone matrix data 構築 + bindV3aRigged 経由実 draw。design-lock phase 着手要 (= 本 commit 完了後、別 session の fresh context で ambiguity 確認 + 実装計画分解後の実装 phase)。

---

## §A. feedback 遵守 record

- **feedback_proactive_handoff** 遵守 = 本 PC-8 marker doc 起案
- **feedback_handoff_minimal_pre_req_read** 遵守 = 必読 1 件 + PC-N-3 complete doc + pinpoint reference 4 件別記、本 session も Read pinpoint のみ (= PC-N-3 complete doc 全文 + ls handoff dir + ls specs dir + AYA 方針確認のみ)、full file dump なし
- **feedback_self_verify_before_handoff** 遵守 = 9 観点 self-verify 全 ✅ §9
- **feedback_build_only_verified** 遵守 = PC-N-3 complete §3 (= llrender build + WARNING 0 + TUT 11/11 + 10/10 + 13/13 + codegen 131/131) の literal 検証結果を本 PC-8 §2 で reference
- **feedback_no_scope_shrink** 遵守 = PC-8 literal scope 4 件 §0 全件実施、(B) 採用 + Mac/Win 補完戦略は AYA literal record 後の方針確定ゆえ縮小ではなく **scope 再定義**、Mac/Win 向け資料 stub 起案も (PC-8-doc-1) B + (PC-8-doc-2) A 採用で AYA literal record 済
- **feedback_doubt_self_first** 遵守 = ambiguity 4 件 (= PC-8-1 marker 定義 + PC-8-2 Win/Mac command 列 + PC-8-3 feature branch 取込 + PC-8-4 design-lock 要否) 発見 + 推奨案提示 + AYA 方針 (= Phase 1 より上位構造で OS 依存懸念) 提示 + 6 項表 + macOS MoltenVK 制約指摘 + AYA literal「Bを採用」record 後本起案、推測実装なし
- **feedback_confirm_referent_before_acting** 遵守 = 4 件 batch AYA 確認 + AYA literal「Bを採用、PC-8 marker doc 起案後 PC-N-5 へ」record + (PC-8-doc-1) + (PC-8-doc-2) 2 件 batch 確認 + AYA literal「B + A で」record で本 PC-8 marker doc + cross-platform spec stub doc 起案、推測実装なし
- **feedback_ubo_migration_one_at_a_time** 厳格遵守 = PC-8 = Linux primary marker 単独 sub-step、PC-N-5 (= 実 GLTF Vulkan draw 通電 1 stub) は分離、本 doc 起案も PC-8 単独 marker のみ
- **feedback_design_phase_no_code_write** 同等整合 = 本 PC-8 は doc 起案 phase、`indra/` 改変 0 件 + codegen 改変 0 件 + shader 改変 0 件
- **feedback_release_branch_workflow** 遵守 = feature branch `feature/ayastorm-r41-gl-removal` 上 commit
- **feedback_no_auto_commit** 遵守 = AYA 明示 commit 指示後 commit 予定
- **feedback_no_claude_coauthor** 遵守 = Co-Authored-By 行不在予定
- **feedback_no_bare_reference_ids** 遵守 = (PC-8-1)..(PC-8-4) + (PC-8-doc-1)..(PC-8-doc-2) 各 ID に項目名 / 採用案内容併記 + PC-N-* sub-step 群に内容併記
- **feedback_tests_dir_never_commit** 整合 = `tests/` 改変 0 件、`git add` 個別 file 指定 + `git add -A` 不使用予定
- **feedback_mac_only_fixes_accept_as_is** 整合 = AYA Mac 不所持ゆえ Mac 限定 fix は他開発者検証信任 as-is 受け入れモデル、本 PC-8 marker + cross-platform spec stub 戦略と整合
- **memory `project_ayastorm_three_platforms`** 整合 = 「3 OS 揃え原則」を「Linux primary 完成 → Mac/Win 派生 fix 補完」モデルで実現、AYA literal「Linux primary 完成 → 他者補完」採用で本旨温存

---
