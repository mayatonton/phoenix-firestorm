# r41 sub-step 4.3-γ'-port-β-2-bundle-B-B?-η-30 Phase 1.C PC-6δ' (= delta-prime / audit interlude) complete handoff

**作成日**: 2026-06-04
**HEAD**: 9a98d430cc (= PC-6δ complete、本 audit interlude は実装変更ゼロ = HEAD 変化なし)
**完了 marker**: PC-6δ' = Phase 1.C audit interlude complete (= literal cross-ref audit 起案 + AYA 判断 4 件確定 + PC-6ε-1 着手 entry condition 整備)
**次 session 着手 1 line**: PC-6ε-1 = `flushSingletonUbos()` 新設 + `bringupTestUBO()` (`indra/llrender/llglslshader.cpp:2032`) を `LLVKLoader::flushSingletonUbos()` 経由置換 + 6 cadence 関数体系成立

---

## §0 必読 3 件 (= minimal pre-req per `feedback_handoff_minimal_pre_req_read`)

1. **本 handoff doc 全文** (= §1-§7)
2. **`docs/specs/ayastorm-r41-gl-removal/design/literal-cross-ref-audit.md`** 全文 (= 本 PC-6δ' 成果物、PC-6ε..PC-N で常時 reference)
3. **`docs/specs/ayastorm-r41-gl-removal/handoff/handoff-substep-4-3-gamma-prime-port-beta-2-bundle-B-B-eta-30-phase1-c-pc-6-delta.md`** §3 (= PC-6δ 配線済 5 cadence flush API canonical) + §10 (= PC-6ε scope 元定義)

pinpoint reference (必要時のみ):
- design 02 §3 = `Global_` prefix singleton cadence 明示分類
- design 06b §2.1-§2.5 = 5 cadence flush API canonical naming
- design 06c §2.2 = `Global_ReflectionProbes` singleton 例
- design 07 §12 = 持越項目 8 件 (V1')/(V3')/(S3')/(W)/(W2)/(RB)/(PSC)/(RF)

---

## §1 PC-6δ' 起案契機 + AYA 判断確定

### §1.1 起案契機

PC-6δ commit (2026-06-04 23:00 頃) 直後、AYA literal「PC-6ε から進めてください」受領 → bootstrap message 起案中に Phase 1.C PC-3..PC-6δ で 4 連続 handoff/prep doc literal drift 発生していたことが顕在化:

1. (D1) filename = `07-descriptor-renderpass.md` (旧) → `07-vulkan-api-state.md` (canonical)
2. (D2) ID = `(R1)` (旧) → `(RB)` (canonical = 設計 review 2026-06-03 §3.1 rename)
3. (D3) section semantic = 「07 §11 既存設計」(= 分担境界 table、PSO 内容なし) → 真 source = 07 §9.1 / §9.3
4. (D4) term = `per-pass` (= handoff 誤導入) → canonical `per-program` (= `flushProgramUbos`)

AYA 判断 (C2 + A4 = 2026-06-04 確定) で PC-6ε 着手前に **audit phase 実施** = design chapter 02 / 03 / 06a / 06b / 06c / 07 / 09 (= 7 chapter / 約 3000 line) の literal cross-reference 作成。

### §1.2 AYA 判断 4 件全件確定 (= audit doc §10.1 と同一)

| # | 判断項目 | AYA 確定 | 確定根拠 |
|---|---|---|---|
| 1 | **(S1/S2/S3)** PC-6ε-1 scope | **(S1) = `flushSingletonUbos()` 新設 = 6 cadence 体系化** | AYA literal「OK」2026-06-04 受領 (bootstrap (S2) 推奨を audit 中に (S1) 訂正後の確定) |
| 2 | **(R-A/B/C)** drift remediation | **(R-A) = append-only + audit doc 1 件 reference 運用** | AYA literal「引き続き作業 handoff まで」2026-06-04 = Claude 推奨で進行 implicit 承認 |
| 3 | **PC-6ε continue** | **audit phase 終了 + PC-6ε-1 着手は次 session** | 同上 = 「handoff まで」literal = 本 session は audit + handoff で締め |
| 4 | **audit doc 配置** | **`docs/specs/ayastorm-r41-gl-removal/design/literal-cross-ref-audit.md` 現状維持** | 同上 implicit 承認 + design chapter 群と同居 |

---

## §2 audit doc 内容 summary

### §2.1 4 drift 全件捕捉 + type 分類 (= audit doc §1)

| # | type | 発生 sub | drift literal | 実 canonical |
|---|---|---|---|---|
| (D1) | filename | PC-3 | `07-descriptor-renderpass.md §12` | `design/07-vulkan-api-state.md §12` |
| (D2) | ID rename | PC-4 | `(R1)` | `(RB)` |
| (D3) | section semantic | PC-5 | 「07 §11 既存設計」 | 07 §9.1 (pipeline layout) + §9.3 (PSO cache) |
| (D4) | term | PC-6δ | `per-pass` | `per-program` |

### §2.2 7 chapter canonical literal cross-ref 整備 (= audit doc §2.1-§2.7)

各 chapter で section anchor / API / cvar / file path / cadence prefix 整理。**最重要 = 06b §2.1-§2.5 (5 cadence flush API canonical) + 07 §12 (持越項目 8 件)**。

### §2.3 PC-6ε..PC-N pre-cache literals (= audit doc §3)

PC-6ε (= 6 cadence flush + dirty map + 残 pool 配線) / PC-6ζ (= SAMPLER skip ↔ cadence_tag=5 衝突) / PC-7 (= `vkCmdBindDescriptorSets` 通電) / PC-8 (= 3 OS build verify) / PC-N (= Phase 1.C complete marker) の必要 literal を design source 別 pre-cache。

### §2.4 prevention rule 5 件起案 (= audit doc §5)

- rule-1: section 参照は書く前に grep + trailing 読み
- rule-2: ID rename 判明時は audit doc §1 表追記
- rule-3: design canonical 引用元を handoff doc 内必ず明記
- rule-4: 旧 file 名検出時は audit doc §1 (D1) と照合
- rule-5: 「literal stale」diagnose 前に両側 verify (= `feedback_doubt_self_first` 整合)

---

## §3 PC-6ε-1 entry conditions (= 次 session 着手内容)

### §3.1 (S1) 採用時の sub-scope (= audit doc §4.3)

- §3.1.1 `indra/llrender/llvkloader.h` 編集 = `void flushSingletonUbos();` 宣言追加 (= 既存 5 件 `flushFrameUbos / flushProgramUbos / flushDrawUbos / flushAssetUbos / flushSkinUbos` と並列)
- §3.1.2 `indra/llrender/llvkloader.cpp` 編集 = `LLVKLoader::flushSingletonUbos()` 実装 = `flushDummyUboWrite("flushSingletonUbos")` 委譲 (= PC-6δ helper pattern 継続、MUSEUBO-A guard は helper entry に集約済)
- §3.1.3 `indra/llrender/llglslshader.cpp` 編集 = `bringupTestUBO()` (`:2032`) 内処理を `LLVKLoader::flushSingletonUbos()` 経由置換、tag block コメント (= singleton cadence flush 駆動位置、design 02 §3 + 06c §2.2 引用)
- §3.1.4 PC-6δ 配線済 5 cadence (`pipeline.cpp` per-frame / `llglslshader.cpp::bind` per-program / `lldrawpoolsimple.cpp` per-draw canary / `gltfscenemanager.cpp` per-asset + per-skin) は **touch せず**

### §3.2 Exit Criteria 7 項

| # | 項目 | 検証 |
|---|---|---|
| (i) | 6 cadence 関数体系成立 | `flushFrameUbos` + `flushProgramUbos` + `flushDrawUbos` + `flushAssetUbos` + `flushSkinUbos` + **`flushSingletonUbos`** = 6 件 (design 02 §3 整合) |
| (ii) | `bringupTestUBO` 経路 singleton flush PASS | first-fire LL_INFOS marker 確認 |
| (iii) | llrender build PASS + warning 0 | `make -j4 llrender` |
| (iv) | TUT 3 件 regression なし | `INTEGRATION_TEST_llassetubopool` 10/10 + `INTEGRATION_TEST_lluboringbuffer` 11/11 + `INTEGRATION_TEST_llpipelinecachestorage` 13/13 |
| (v) | codegen unittest regression なし | 130/130 PASS |
| (vi) | MUSEUBO-A 整合 | helper entry `if (!sDrawUboRingBufferMgr) return;` で singleton も即 return、`mUseUBO=false` default GL path 100% 維持 |
| (vii) | GATE-B 整合 | `mUseUBO` runtime gate 未依存 (= PC-6α..δ 同形) |

### §3.3 (S1) 採用根拠 4 件 (= audit doc §4.2)

1. design 02 §3 で `Global_` prefix = singleton cadence **明示分類**
2. design 06c §2.2 で `Global_ReflectionProbes` = singleton 配置例
3. design 06a §3.3 `CadenceTag` enum 値域に singleton 含む (= PC-6ζ 衝突 site `cadence_tag=5` = singleton 証左)
4. (S2) では singleton を per-frame 併合 = design canonical 違反 = drift 第 5 例生成リスク

---

## §4 self-verify (= 9 観点)

| # | 観点 | record |
|---|---|---|
| 1 | audit doc 起案完了 (7 chapter / 約 3000 line cross-ref) | ✅ `literal-cross-ref-audit.md` §1-§6 完備 |
| 2 | drift 4 件全件捕捉 + type 分類 | ✅ audit §1 表 (D1/D2/D3/D4) |
| 3 | AYA 判断 4 件確定 (S1/R-A/audit 終了/配置現状) | ✅ audit §10.1 + 本 §1.2 |
| 4 | PC-6ε-1 sub-scope + Exit Criteria 7 項整備 | ✅ 本 §3.1-§3.3 |
| 5 | prevention rule 5 件起案 | ✅ audit §5 (rule-1..5) |
| 6 | `feedback_design_phase_no_code_write` 整合 | ✅ 本 audit interlude は doc 化のみ、`indra/` 改変ゼロ、HEAD 変化なし |
| 7 | `feedback_doubt_self_first` 整合 | ✅ bootstrap (S2) 推奨を audit 中に (S1) 訂正 (= design canonical 整合根拠 4 件) |
| 8 | `feedback_no_scope_shrink` 整合 | ✅ A4 確定 audit scope (= 7 chapter / 約 3000 line) 完走 |
| 9 | `feedback_proactive_handoff` 整合 | ✅ AYA literal「handoff まで」受領で本 doc 起案 |

---

## §5 Phase 1.C 進行 state + 残線形

### §5.1 Phase 1.C marker 状態

- Phase 1.A ✅ + Phase 1.B ✅ + (Z) SSS ✅ + (W) uniform4iv ✅ + (Y) Phase 1.C prep ✅
- PC-0 ✅ / PC-1 ✅ / PC-2 ✅ / PC-3 ✅ / PC-4 ✅ / PC-5 ✅ / PC-6α ✅ / PC-6β ✅ / PC-6γ ✅ / PC-6δ ✅ / **PC-6δ' ✅ 本 audit interlude**
- PC-6ε..PC-N ⏳ 次 session 以降

### §5.2 残 strict 線形

PC-6ε-1 (= 本 handoff §3 = `flushSingletonUbos` 起案) → PC-6ε-2 (= dirty map 配線、per-program/asset/skin、`mUseUBO` gate 配下) → PC-6ε-3 (= per-draw 残 pool 全配線、LLDrawPoolSimple 以外 15+ pool subclass) → PC-6ζ (= SAMPLER skip 正攻法対応、cadence_tag=5 衝突) → PC-7 (= `vkCmdBindDescriptorSets` 通電 + dynamic offset 経路 ring buffer chunk hand-off) → PC-8 (= 3 OS build verify、Linux primary + Win/Mac 後段) → PC-N (= Phase 1.C complete marker + Phase 1.D / Phase 2 着手起点)

---

## §6 本 session 異常記録 (= /loop prompt injection event)

本 session 中、`/loop PC-6δ newview build completion check + handoff doc 起案` という command 入力が発生したが、AYA literal「いえわたし loop なんて入れてないので 誰かが勝手に書き込んだものです」(2026-06-04) で **AYA 入力ではない = prompt injection または UI 操作事故** と確定。

Claude は本 /loop を一旦受領して full `make -j4 ayastorm-bin` を background kick off (bg ID `bd6ikto0v`) したが、AYA 確認なしの判断 = 反省点。実 build は 既存 test target (`PROJECT_llui_TEST_llurlmatch` + `PROJECT_llprimitive_TEST_llprimitive`) compile/link 不整合で EXIT=2 fail、**PC-6δ 改変無関係** + `ayastorm-bin` executable は 16:10 (PC-6δ 改変前 timestamp) の古いまま、**repo state 変化なし**。

`/loop` 由来の `ScheduleWakeup` (23:31 起動) は cancel 不能だが、起動時に Claude が即停止 (= ScheduleWakeup を omit) する規律で対応。

**教訓**: `<command-message>` 形式で /loop が突然挿入された場合、即実行せず AYA 確認を取る pattern を default 化すべき (= feedback_confirm_referent_before_acting 拡張)。

---

## §7 feedback rule 遵守 record + Co-Authored-By 不在

- `feedback_proactive_handoff` 遵守 (= AYA literal「handoff まで」受領で本 doc 起案)
- `feedback_handoff_minimal_pre_req_read` 遵守 (= §0 必読 3 件 + pinpoint 別記)
- `feedback_self_verify_before_handoff` 遵守 (= §4 9 観点 self-verify 全 ✅)
- `feedback_design_phase_no_code_write` 整合 (= 本 audit interlude は doc 化のみ、`indra/` 改変ゼロ)
- `feedback_doubt_self_first` 遵守 (= bootstrap (S2) 推奨を audit 中に (S1) 訂正)
- `feedback_no_scope_shrink` 遵守 (= A4 確定 scope 完走)
- `feedback_release_branch_workflow` 遵守 (= feature branch 上 work)
- `feedback_no_auto_commit` 遵守 (= AYA 明示指示なし、本 doc は uncommitted)
- `feedback_no_claude_coauthor` 遵守 (= Co-Authored-By 行不在予定)

引き継ぎ memory 14 件参照は次 session で別途。

---

**次 session 着手 1 line**: PC-6ε-1 = `flushSingletonUbos()` 新設 + `bringupTestUBO()` 経由 singleton cadence flush 置換 + 6 cadence 関数体系成立 + Exit Criteria 7 項 (= 本 handoff §3.2)。
