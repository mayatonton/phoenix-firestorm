# handoff = r41 Phase 2.L0 着手 entry (= L0-1〜L0-4 4 protocol 実装 + READINESS.md update + Phase 2.L0 Exit)

**起案日**: 2026-06-06
**前提条件**: r41 Phase 2 前提条件 work 全完走 (= handoff `phase2-prep/handoff-phase2-prep-complete.md` 参照、AYA 承認受領 2026-06-06)
**本 session 担当 (= 次 session)**: Phase 2.L0 着手 = L0-1〜L0-4 4 protocol 実装 + cold launch validation + READINESS.md update + Phase 2.L0 Exit handoff
**設計 phase 解除点**: 本 Phase 2 = **実装 phase**、`indra/` 配下 code 改変開始 (= memory `feedback_design_phase_no_code_write` 規律解除点)

---

## §1. 経緯

### §1.1 r41 milestone 進行状況

| Phase | 状態 | 完了 commit / handoff |
|---|---|---|
| Phase 0 計測 | ✅ 完了 | commit `4e40fd2ab0` (2026-06-03) |
| Phase 1.A codegen pipeline | ✅ 完了 | handoff `phase1/a/handoff-phase1-a-complete.md` |
| Phase 1.B host C++ redirect 層 | ✅ 完了 | handoff `phase1/b/handoff-phase1-b-complete.md` |
| Phase 1.C cadence + dirty + descriptor set bind | ✅ shell 段階完了 | handoff `phase1/c/handoff-phase1-c-complete.md` |
| Phase 1.D/1.E pilot 先回り着手 | ✅ 完了 | handoff `phase1/e/handoff-phase1-e-complete.md` |
| **Phase 2 前提条件 work** | ✅ 全完走 | commit `aa65bf96f1` + `323b6bf0df` + `bd292e6968` + `289fa2842c`、handoff `phase2-prep/handoff-phase2-prep-complete.md` |
| **Phase 2.L0 着手 (= 本 handoff)** | 🔜 次 session 着手 | (本 handoff) |
| Phase 2.L1a〜L5 | 着手前 | Phase 2.L0 Exit 後 separate session |
| Phase 3 (Linux 確証) | 着手前 | Phase 2 全完走後 |
| Phase 4 (Windows 確証) | 着手前 | Phase 3 完走後 |
| Phase 5 (macOS 確証) | 着手前 | Phase 4/5 並走候補 |
| Phase 6 (release) | 着手前 | Phase 5 完走後 |
| r42 milestone (OpenGL 撤廃) | r42 scope 移管 | r41 内 Phase 一覧外 |

### §1.2 AYA literal 確定 2026-06-06 record

1. **「全 UBO を Phase 2 のスコープとします、工程を勝手に解釈しないでください」** (= 当初 Template A R3-R6 split を AYA さん明示 reject、Phase 2 = 全 94 UBO + L0 4 protocol 確定)
2. **「Phase 3 以降は繰上げで OK」** (= 旧 Phase K+1〜K+5 → Phase 3-6 繰上げ、OpenGL 撤廃 = r42 milestone scope 移管維持)
3. **「READINESS.md update を Phase 2.L0 完了 Exit 条件に含めるのが妥当でしょうか?」「はいそうしてください」** (= Phase 2.L0 Exit に READINESS.md update 必須化)
4. **「了解、それでは L0 着手ということで次のセッションから作業をお願いします」** (= Phase 2.L0 着手承認、本 handoff entry)
5. 「現状の見た目とほぼ変わらない描画が望まれる」 (= visual regression ゼロ §5.4 policy 継承)
6. 「進められるところまで進めてください + 推奨案で OK」 (= 自走承認継続、4 原則 + 視覚 regression ゼロ violation でない限り推奨案採用)

### §1.3 本 handoff 位置付け

Phase 2 前提条件 work (= WORK_ORDER + READINESS + RELATIONS + INDEX + 各 UBO file §12 全件起案) 完了状態から、**Phase 2 実装 phase の最初の sub-step = Phase 2.L0** (= L0-1〜L0-4 4 protocol 実装) に着手する handoff。

---

## §2. Phase 2.L0 scope (= 本 session 着手範囲)

### §2.1 L0 4 protocol 実装 (= WORK_ORDER §2.1-§2.4 詳細起案済)

| protocol | 内容 | WORK_ORDER §参照 |
|---|---|---|
| **L0-1** | name-based dispatch logic 確立 (= host dispatch logic 仕様 + shader-side UBO block 名称規約 + set=3 binding 衝突 3 site 解消 + 既存 pilot 通電 UBO logic 整合) | §2.1 |
| **L0-2** | LLStaticHashedString UBO redirect 経路確立 (= setter intercept + UBO redirect mapping table + dual-write 経路 = GL path 既存 uniform 維持 + Vulkan path UBO write) | §2.2 |
| **L0-3** | per-shader UBO block 拡大方式確立 (= shader build pipeline preprocessor inject + `LL_VULKAN_GLSL` gate 範式 + upstream merge conflict 自動検出 strategy) | §2.3 |
| **L0-4** | cadence 再評価 + 再分類 (= per-program stale data risk 判定 + PerFrame/PerProgram/PerDraw 帰属再分類 + sliced UBO 化判断) | §2.4 |

### §2.2 各 protocol 着手前 AYA literal 確認 6 件 (= WORK_ORDER §2.5.2 既起案)

L0 着手前に AYA literal 判断が必要な事項:
1. **set=3 binding 衝突 3 site 解消方針** (= L0-1 protocol-C: 新規 binding allocation / runtime dispatch / 設計再考) [要 AYA 判断]
2. **LLStaticHashedString mapping table 構築方式** (= L0-2 protocol-B: code-gen / introspection / ハードコード) [要 AYA 判断]
3. **shader build pipeline preprocessor inject 方式** (= L0-3 protocol-B: static include / build macro / shader loader 自動) [要 AYA 判断]
4. **cadence 再分類 strategy** (= L0-4 protocol-B: sliced UBO / PerDraw 移行 / stale 許容、各 UBO 適用判断) [要 AYA 判断]
5. **upstream merge conflict 自動検出 strategy** (= L0-3 protocol-D) [要 AYA 判断]
6. **sliced UBO 化が Phase 3 移管対象か Phase 2 内か** (= L0-4 (7) 原則 3、本 milestone 確定で Phase 2 内に内包) [要 AYA 判断]

次 session 開始時に AYA literal 確認推奨。AYA literal「推奨案で OK」自走承認継続なら、各 protocol 推奨案 (= WORK_ORDER §2.1.4 / §2.2.4 / §2.3.4 / §2.4.4 内 protocol-A〜D) 採用で進行可能。

### §2.3 Phase 2.L0 Exit 条件 (= WORK_ORDER §2.5.3 + §4.3 R-5 確定)

| # | Exit 項目 | 判定基準 |
|---|---|---|
| 1 | L0-1〜L0-4 4 protocol 実装完了 | host C++ + GLSL shader 全件通電 + cold launch validation + Linux validation layer warnings 0 件 |
| 2 | 既存通電 UBO regression 確認 | Phase 1.E までの通電 UBO に L0 protocol 適用後 visual regression なし (= visual regression ゼロ §5.4 policy) |
| 3 | **READINESS.md update** | §3 (C 判定 16 group) + §4 (B 判定 31 件) + §5 集計 + §6 着手順序ヒント、L0 解消事項反映、B 判定 31 件 → A 昇格 candidate 確定 + 残 B/C 個別解消事項明示 |
| 4 | A 昇格判定基準明示 | READINESS update 内記載: 全件 A 昇格 = L0 後の自動昇格ではない、sub-work (6) A 確定条件達成で個別 sub-step 内最終確定 |
| 5 | Phase 2.L1a〜L5 sub-step 着手 unblocking | 各 sub-step 着手前提条件 (= L0 4 protocol 経路依存) 解消確認 |

---

## §3. 着手前必読 (= memory `feedback_handoff_minimal_pre_req_read` 適用)

### §3.1 最低限 3 件

1. **`design/ubo/WORK_ORDER.md`** = §2 L0 protocol 4 件詳細 + §2.5 AYA review check list + §4 4 原則 gate (必読)
2. **`design/ubo/READINESS.md`** = update 対象 doc、§3 + §4 + §5 + §6 構造把握
3. **本 handoff doc** = Phase 2.L0 着手 entry record + AYA literal 確定指示

### §3.2 pinpoint Read (必要時)

- `design/ubo/RELATIONS.md` §5 dirty 連動 group 20+ + §8 不明事項 15 dimension (= L0-1 dispatch + L0-2 redirect 整合判断時)
- `design/ubo/INDEX.md` §2 全 94 UBO summary + cadence_tag mapping (= L0-4 cadence 再評価時)
- `design/06a-prep-phase0-measurement.md` §2-§4 Phase 0 計測 spec (= cadence 再評価 input data)
- `design/06b-dirty-flag.md` / `design/06c-descriptor-set-bind.md` (= Phase 1.B/1.C 既設実装 reference)
- `design/08-build-codegen-pipeline.md` §6.4 std140 padding / §13 3 OS 確証 (= L0-3 shader pipeline 設計時)
- `design/01-overview.md` §2 2 大設計原則 (= 上流 merge conflict strategy 判断時)
- 前 handoff `phase2-prep/handoff-phase2-prep-complete.md` (= Phase 2 前提条件 work 全完走 record)

### §3.3 memory 必読

- **`project_r41_phase2_4_principles`** = 4 原則確定 (= C1-C6 / OS-1〜OS-10 / R-1〜R-5 / O-1〜O-5) + Phase 2.L0 Exit 条件
- **`project_ayastorm_r41_design_principles`** = r41 全体 2 大設計原則 (= Upstream 取込 + Core 分散)
- **`project_r41_phase1b_vulkan_host_gate`** = host C++ redirect 層 = `mUseUBO` runtime flag のみ、`LL_VULKAN_GLSL` C++ 不使用
- **`feedback_ubo_migration_one_at_a_time`** = 1 UBO ずつ実装 + cold launch 検証 PASS → 次 UBO (Phase 2.L0 内 protocol 単位も同規律)
- **`feedback_build_only_verified`** = 効果未確認 commit / 設計倒れ spec を積まない、確認できないものは revert
- **`feedback_admit_unknown`** = 推論禁止、不明明示 ([要追加調査] / [要 verify] / [要 AYA 判断] マーク)
- **`feedback_no_scope_shrink`** = AYA literal scope 厳守、4 原則 violation 提案禁止
- **`feedback_self_bug_no_defer_option`** = 自作 bug の「先送り/disable」を提案として並べない
- **`feedback_visual_decisions_need_live_ab`** = 視覚表現章 cvar live A/B 必須、机上推論で BD vs AYAstorm 決めない
- **`feedback_proactive_handoff`** = context 圧迫時自分から引き継ぎ提案
- **`feedback_release_flow`** = push / PR は AYA 側、Claude はローカル commit まで
- **`feedback_no_claude_coauthor`** = 全 commit message で Claude 共著行を含めない
- **`feedback_tests_dir_never_commit`** = root /tests/ のみ commit しない、indra/**/tests/ は通常 commit 可

---

## §4. 着手手順

### §4.1 session 開始時の最初の AYA 確認

「上記 handoff 確認、Phase 2.L0 着手 (= L0-1〜L0-4 4 protocol 実装 + READINESS.md update + Exit handoff) で OK か?」

AYA literal「OK」受領後、§2.2 AYA review 6 件確認 (= 推奨案で OK 自走承認継続なら省略可能) → 着手。

### §4.2 着手順序 (= 推奨案、AYA literal「推奨案で OK」自走承認継続前提)

| step | 内容 | 想定 sub-session |
|---|---|---|
| 1 | L0-1 name-based dispatch logic 実装 (= host dispatch + program 識別 + set=3 binding 衝突 3 site 解消 + 既存 pilot 通電 UBO 整合 verify) | 数 sub-session |
| 2 | L0-2 LLStaticHashedString redirect 経路実装 (= mapping table + intercept + dual-write 経路) | 数 sub-session |
| 3 | L0-3 per-shader UBO block 拡大方式実装 (= preprocessor inject + `LL_VULKAN_GLSL` gate 範式 + upstream merge conflict 検出) | 数 sub-session |
| 4 | L0-4 cadence 再評価 + 再分類 (= Phase 0 計測結果 input + per-program stale risk 判定 + UBO 単位 PerFrame/PerProgram/PerDraw 帰属再分類) | 数 sub-session |
| 5 | 全 protocol 完了 cold launch validation (= 全既存通電 UBO regression 確認) | 1 sub-session |
| 6 | READINESS.md update (= §3 + §4 + §5 + §6、L0 解消事項反映、B 判定 31 件 → A 昇格 candidate 確定) | 1 sub-session |
| 7 | Phase 2.L0 Exit handoff doc 起案 (= `phase2/handoff-phase2-l0-complete.md` 新規) | 1 sub-session |
| 8 | commit + AYA literal Phase 2.L0 Exit 承認 → Phase 2.L1a 着手 separate session entry | (handoff 切替) |

### §4.3 各 protocol 実装規律

- **1 protocol ずつ実装 + cold launch 検証 PASS → 次 protocol** (= memory `feedback_ubo_migration_one_at_a_time` 範式適用)
- 推論禁止、不明明示 (= memory `feedback_admit_unknown`、[要追加調査] / [要 verify] / [要 AYA 判断] マーク)
- 4 原則 死守 + 視覚 regression ゼロ (= 全 protocol 実装で sub-work (7) 4 原則 gate 評価)
- `mUseUBO` runtime flag default OFF 維持 (= dual-path 出荷、OpenGL path fallback 提供、原則 4 §4.4 O-2)
- `#ifdef LL_VULKAN_GLSL` C++ 側不使用 (= GLSL shader 側のみ有効、原則 4 §4.4 O-3、memory `project_r41_phase1b_vulkan_host_gate`)
- 4 原則 violation 検知時 = §4.7 protocol (= stage 1 提案撤回 / stage 2 設計再考 / stage 3 AYA literal 確認)
- AYA literal「現状の見た目とほぼ変わらない描画」継承 (= visual regression ゼロ §5.4 policy)

### §4.4 build / 検証手順 (= memory `project_build_procedure` 参照)

- configure → build → install → cache clear (= 完全フロー、`--fmodstudio` + `LL_DULLAHAN_AUDIO_CALLBACK` フラグ含む)
- shader のみ変更時は autobuild 不要 (= installed dir に直接 cp + cache clear で反映、`~/ayastorm/app_settings/shaders/...` 直接 cp、`~/.ayastorm_x64/cache/shader_cache/` clear)
- AYA build / 起動 / 動作確認、Claude は log grep / 解析

---

## §5. 起案規律 (= Phase 2 実装 phase 規律、design-phase 規律解除点)

### §5.1 design-phase 規律解除 (= memory `feedback_design_phase_no_code_write` 解除)

Phase 2 = 実装 phase ゆえ `indra/` 配下 code 改変開始可能。ただし以下を遵守:
- **「正しいことを積み上げる (実機検証 only)」** (= memory `feedback_build_only_verified`)、効果未確認 commit / 設計倒れ spec を積まない
- 全 protocol 実装で cold launch validation 必須 (= Vulkan validation 0 + visual regression ゼロ)
- AYA live verify 必須 (= visual regression ゼロ最終確認)

### §5.2 context 容量管理

- 1 protocol = 数 sub-session 想定、L0 4 protocol 完走 = 複数 session 想定
- 70-80% context 消費で handoff 切替推奨 (= memory `feedback_proactive_handoff`)
- 各 protocol 完了境界が自然な handoff 切替境界

### §5.3 Phase 2.L0 完了 Exit handoff (= 本 session 完了時起案)

handoff doc = `handoff/phase2/handoff-phase2-l0-complete.md` (新規) として:
- L0-1〜L0-4 実装完了 record (= 各 protocol commit hash + cold launch validation log)
- READINESS.md update 内容 (= B 判定 31 件 → A 昇格 candidate list)
- Phase 2.L1a 着手前 AYA literal 確認 candidate list (= L1a 各 UBO 個別 review 要事項)
- 次 session = Phase 2.L1a 着手 separate session entry record

---

## §A. 本 handoff 関連 commit

| commit | 内容 |
|---|---|
| `aa65bf96f1` | Phase 2 前提条件 WORK_ORDER C-6 L4 全 16 group 起案完了 |
| `323b6bf0df` | Phase 2 前提条件 work 全完走 (= C-8 §4 + §2.1.1 + L3 20 file §12) |
| `bd292e6968` | Phase 2 範囲確定 = AYA literal「全 UBO を Phase 2 のスコープ」+ Phase 3-6 繰上げ反映 |
| `289fa2842c` | Phase 2.L0 Exit 条件 = READINESS.md update 確定反映 |
| (本 handoff) | Phase 2.L0 着手 entry handoff |

---

## §B. 次 session 開始時 AYA 確認

「上記要約を確認、Phase 2.L0 着手 (= L0-1 から順次 4 protocol 実装 + READINESS.md update + Phase 2.L0 Exit handoff) で OK か?」

**AYA literal「OK」受領後の即時着手項目**:
1. §2.2 AYA review 6 件 = AYA literal 確認 (= 推奨案 OK なら省略可能、自走承認継続)
2. memory pinpoint 確認 (= 4 原則 + 2 大設計原則 + Phase 1.B mUseUBO gate + 実装 phase 規律)
3. L0-1 実装着手 (= host dispatch logic 設計 + implementation + cold launch validation)

**recover protocol**: 4 原則 + 視覚 regression ゼロ violation 検知時は §4.7 stage 1/2/3 即時適用、AYA literal 確認 escalate。
